#if WITH_SUAS_GROVE_DLS_1_1
#include <stdio.h>
#include <stdlib.h>

#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <hal_i2c.h>
#include "grove_dls.h"

int init_grove_dls() {
    uint8_t power_up[1] = {0x03};
    uint8_t power_down[1] =  {0x00};
    int result = -1;

    // send initialization data
    result = hal_i2c_write_block(GROVE_DLS_1_1_DEVICE_ADDRESS,
        (char*) power_up, ONE_BYTE, GROVE_DLS_1_1_SUBADDRESS_LENGTH,
        GROVE_DLS_1_1_CONTROL_ADDRESS);

    if (result != 0) {
        printf("Initialization error in stage 1\r\n");
        return result;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);

    result = hal_i2c_write_block(GROVE_DLS_1_1_DEVICE_ADDRESS,
        (char*) power_down, ONE_BYTE, GROVE_DLS_1_1_SUBADDRESS_LENGTH,
        GROVE_DLS_1_1_TIMING_ADDRESS);

    if (result != 0) {
        printf("Initialization error in stage 2\r\n");
        return result;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);

    result = hal_i2c_write_block(GROVE_DLS_1_1_DEVICE_ADDRESS,
        (char*) power_down, ONE_BYTE, GROVE_DLS_1_1_SUBADDRESS_LENGTH,
        GROVE_DLS_1_1_INTERRUPT_ADDRESS);
    
    if (result != 0) {
        printf("Initialization error in stage 3\r\n");
        return result;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    result = hal_i2c_write_block(GROVE_DLS_1_1_DEVICE_ADDRESS,
        (char*) power_down, ONE_BYTE, GROVE_DLS_1_1_SUBADDRESS_LENGTH,
        GROVE_DLS_1_1_CONTROL_ADDRESS);
    
    if (result != 0) {
        printf("Initialization error in stage 4\r\n");
        return result;
    }
    
    return result;
}

int8_t getLux() {
    char* readings = (char*) malloc(4*sizeof(char));
    int result = -1;
    uint8_t power_up[1] = {0x03};
    uint8_t power_down[1] =  {0x00};

    result = hal_i2c_write_block(GROVE_DLS_1_1_DEVICE_ADDRESS,
        (char*) power_up, ONE_BYTE, GROVE_DLS_1_1_SUBADDRESS_LENGTH,
        GROVE_DLS_1_1_CONTROL_ADDRESS);
    
    if (result != 0) {
        printf("Error preparing to read data\r\n");
        return -1;
    }

    vTaskDelay(14 / portTICK_PERIOD_MS);

    result = hal_i2c_read_block(GROVE_DLS_1_1_DEVICE_ADDRESS, readings,
        ONE_BYTE, GROVE_DLS_1_1_SUBADDRESS_LENGTH, GROVE_DLS_1_1_CHANNEL_0L);
    
    if (result != 0) {
        printf("Error reading 0L register\r\n");
        return -1;
    }

    result = hal_i2c_read_block(GROVE_DLS_1_1_DEVICE_ADDRESS, readings + 1,
        ONE_BYTE, GROVE_DLS_1_1_SUBADDRESS_LENGTH, GROVE_DLS_1_1_CHANNEL_0H);
    
    if (result != 0) {
        printf("Error reading 0H register\r\n");
        return -1;
    }

    result = hal_i2c_read_block(GROVE_DLS_1_1_DEVICE_ADDRESS, readings + 2,
        ONE_BYTE, GROVE_DLS_1_1_SUBADDRESS_LENGTH, GROVE_DLS_1_1_CHANNEL_1L);

    if (result != 0) {
        printf("Error reading 1L register\r\n");
        return -1;
    }

    result = hal_i2c_read_block(GROVE_DLS_1_1_DEVICE_ADDRESS, readings + 3,
        ONE_BYTE, GROVE_DLS_1_1_SUBADDRESS_LENGTH, GROVE_DLS_1_1_CHANNEL_1H);
        
    if (result != 0) {
        printf("Error reading 1H register\r\n");
        return -1;
    }

    ch0 = (readings[0] << 8) | readings[1];
    ch1 = (readings[2] << 8) | readings[3];

    free(readings);
    readings = NULL; 

    result = hal_i2c_write_block(GROVE_DLS_1_1_DEVICE_ADDRESS,
        (char*) power_down, ONE_BYTE, GROVE_DLS_1_1_SUBADDRESS_LENGTH,
        GROVE_DLS_1_1_TIMING_ADDRESS);
    
    if (result != 0) {
        printf("Error finishing to read data\r\n");
        return -1;
    }
    return 0;
}

uint16_t readIRLuminosity() {
    if (getLux() != 0) {
        return 0;
    }
    if (ch0 == 0) {
        return 0;
    } else if (ch0 / ch1 < 2 && ch0 > 4900) {
        printf("Invalid data received");
        return 0;
    } else {
        return ch1;
    }
}

uint16_t readFSLuminosity() {
    if (getLux() != 0) {
        return 0;
    }
    if (ch0 == 0) {
        return 0;
    } else if (ch0 / ch1 < 2 && ch0 > 4900) {
        printf("Invalid data received");
        return 0;
    } else {
        return ch0;
    } 
}

static unsigned long calculateLux(unsigned int iGain, unsigned int tInt, int iType) {
    unsigned long chScale;
    unsigned long channel0, channel1, temp, lux, ratio1;
    unsigned int b, m;
    switch (tInt) {
        case 0:  // 13.7 msec
            chScale = CHSCALE_TINT0;
            break;
        case 1: // 101 msec
            chScale = CHSCALE_TINT1;
            break;
        default: // assume no scaling
            chScale = (1 << CH_SCALE);
            break;
    }
    if (!iGain) {
        chScale = chScale << 4;    // scale 1X to 16X
    }
    // scale the channel values
    channel0 = (ch0 * chScale) >> CH_SCALE;
    channel1 = (ch1 * chScale) >> CH_SCALE;

    ratio1 = 0;
    if (channel0 != 0) {
        ratio1 = (channel1 << (RATIO_SCALE + 1)) / channel0;
    }
    // round the ratio value
    unsigned long ratio = (ratio1 + 1) >> 1;

    switch (iType) {
        case 0: // T package
            if (ratio <= K1T) {
                b = B1T;
                m = M1T;
            } else if (ratio <= K2T) {
                b = B2T;
                m = M2T;
            } else if (ratio <= K3T) {
                b = B3T;
                m = M3T;
            } else if (ratio <= K4T) {
                b = B4T;
                m = M4T;
            } else if (ratio <= K5T) {
                b = B5T;
                m = M5T;
            } else if (ratio <= K6T) {
                b = B6T;
                m = M6T;
            } else if (ratio <= K7T) {
                b = B7T;
                m = M7T;
            } else if (ratio > K8T) {
                b = B8T;
                m = M8T;
            }
            break;
        case 1:// CS package
            if (ratio <= K1C) {
                b = B1C;
                m = M1C;
            } else if (ratio <= K2C) {
                b = B2C;
                m = M2C;
            } else if (ratio <= K3C) {
                b = B3C;
                m = M3C;
            } else if (ratio <= K4C) {
                b = B4C;
                m = M4C;
            } else if (ratio <= K5C) {
                b = B5C;
                m = M5C;
            } else if (ratio <= K6C) {
                b = B6C;
                m = M6C;
            } else if (ratio <= K7C) {
                b = B7C;
                m = M7C;
            }
    }
    temp = ((channel0 * b) - (channel1 * m));
    temp += (1 << (LUX_SCALE - 1));
    // strip off fractional portion
    lux = temp >> LUX_SCALE;
    return (lux);
}

unsigned long readVisibleLux() {
    if (getLux() != 0) {
        return 0;
    }
    if (ch0 == 0) {
        return 0;
    } else if (ch0 / ch1 < 2 && ch0 > 4900) {
        printf("Invalid data received");
        return 0;
    } else {
        // calculate lux
        return calculateLux(0,0,0);
    } 
}
#endif