// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// I2C hardware abstraction layer
#include <hal_i2c.h>

// C standard library functionalities
#include <stdio.h>
#include <stdlib.h>

// Own header
#include "grove_dls.h"

// Data to exchange with the sensor
static const uint8_t POWER_UP_DATA = 0x03;
static const uint8_t POWER_DOWN_DATA =  0x00;

// Global variables
uint16_t ch0, ch1;
unsigned long chScale;
unsigned long channel0, channel1;

// Initialization function for sensor
int init_grove_dls() {
    // Define result value
    int result = -1;

    // Send initialization data: power up
    result = hal_i2c_write_block(GROVE_DLS_1_1_DEVICE_ADDRESS,
        (char*) &POWER_UP_DATA, 1, GROVE_DLS_1_1_REGISTER_LENGTH,
        GROVE_DLS_1_1_CONTROL_ADDRESS);

    // Return error if error occurred
    if (result != 0) {
        printf("Initialization error in stage 1\r\n");
        return result;
    }

    // Wait 25 ms
    vTaskDelay(25 / portTICK_PERIOD_MS);

    // Send initialization data: clear timing register
    result = hal_i2c_write_block(GROVE_DLS_1_1_DEVICE_ADDRESS,
        (char*) &POWER_DOWN_DATA, 1, GROVE_DLS_1_1_REGISTER_LENGTH,
        GROVE_DLS_1_1_TIMING_ADDRESS);

    // Return error if error occurred
    if (result != 0) {
        printf("Initialization error in stage 2\r\n");
        return result;
    }

    // Wait 25 ms
    vTaskDelay(25 / portTICK_PERIOD_MS);

    // Send initialization data: clear interrupt register
    result = hal_i2c_write_block(GROVE_DLS_1_1_DEVICE_ADDRESS,
        (char*) &POWER_DOWN_DATA, 1, GROVE_DLS_1_1_REGISTER_LENGTH,
        GROVE_DLS_1_1_INTERRUPT_ADDRESS);
        
    // Return error if error occurred
    if (result != 0) {
        printf("Initialization error in stage 3\r\n");
        return result;
    }

    // Wait 25 ms
    vTaskDelay(25 / portTICK_PERIOD_MS);
    
    // Send initialization data: power down
    result = hal_i2c_write_block(GROVE_DLS_1_1_DEVICE_ADDRESS,
        (char*) &POWER_DOWN_DATA, 1, GROVE_DLS_1_1_REGISTER_LENGTH,
        GROVE_DLS_1_1_CONTROL_ADDRESS);
    
    // Return error if error occurred
    if (result != 0) {
        printf("Initialization error in stage 4\r\n");
        return result;
    }

    // Wait 25 ms
    vTaskDelay(25 / portTICK_PERIOD_MS);
    
    // Return 0 (initialization successful)
    return 0;
}

// Read current total LUX from sensor
// Called internally by the functions readIRLuminosity, readFSLuminosity and readVisibleLux
// Stores data in global variables ch0 and ch1
int8_t getLux() {
    // Set up variables to read data
    char* readings = (char*) malloc(4*sizeof(char)); /* allocate array on heap -> must be freed later*/
    int result = -1;

    // Send data request information: power up
    result = hal_i2c_write_block(GROVE_DLS_1_1_DEVICE_ADDRESS,
        (char*) &POWER_UP_DATA, 1, GROVE_DLS_1_1_REGISTER_LENGTH,
        GROVE_DLS_1_1_CONTROL_ADDRESS);
    
    // Return error if error occurred
    if (result != 0) {
        printf("Error preparing to read data\r\n");
        goto error;
    }

    // Wait 14 ms
    vTaskDelay(14 / portTICK_PERIOD_MS);

    // Read lower part of channel 0, destination: readings[0]
    result = hal_i2c_read_block(GROVE_DLS_1_1_DEVICE_ADDRESS, readings,
        1, GROVE_DLS_1_1_REGISTER_LENGTH, GROVE_DLS_1_1_CHANNEL_0L);
    
    // Return error if error occurred
    if (result != 0) {
        printf("Error reading 0L register\r\n");
        goto error;
    }

    // Read upper part of channel 0, destination: readings[1]
    result = hal_i2c_read_block(GROVE_DLS_1_1_DEVICE_ADDRESS, readings + 1,
        1, GROVE_DLS_1_1_REGISTER_LENGTH, GROVE_DLS_1_1_CHANNEL_0H);
    
    // Return error if error occurred
    if (result != 0) {
        printf("Error reading 0H register\r\n");
        goto error;
    }

    // Read lower part of channel 1, destination: readings[2]
    result = hal_i2c_read_block(GROVE_DLS_1_1_DEVICE_ADDRESS, readings + 2,
        1, GROVE_DLS_1_1_REGISTER_LENGTH, GROVE_DLS_1_1_CHANNEL_1L);

    // Return error if error occurred
    if (result != 0) {
        printf("Error reading 1L register\r\n");
        goto error;
    }

    // Read higher part of channel 1, destination: readings[3]
    result = hal_i2c_read_block(GROVE_DLS_1_1_DEVICE_ADDRESS, readings + 3,
        1, GROVE_DLS_1_1_REGISTER_LENGTH, GROVE_DLS_1_1_CHANNEL_1H);
        
    // Return error if error occurred
    if (result != 0) {
        printf("Error reading 1H register\r\n");
        goto error;
    }

    // Combine upper and lower channel parts into 16-bit data structure
    ch0 = (readings[0] << 8) | readings[1];
    ch1 = (readings[2] << 8) | readings[3];

    // Free readings array
    free(readings);
    readings = NULL; 

    // Power down sensor
    result = hal_i2c_write_block(GROVE_DLS_1_1_DEVICE_ADDRESS,
        (char*) &POWER_DOWN_DATA, 1, GROVE_DLS_1_1_REGISTER_LENGTH,
        GROVE_DLS_1_1_TIMING_ADDRESS);
    
    // Return error if error occurred
    if (result != 0) {
        printf("Error finishing to read data\r\n");
        goto error;
    }

    // Return success
    return 0;

// Error handler: free allocated memory and return error
error:
    free(readings);
    readings = NULL;
    return -1;
}

// Functions below calculate the IR, full spectrum and visible Lux
// Taken from https://github.com/Seeed-Studio/Grove_Digital_Light_Sensor/blob/master/Digital_Light_TSL2561.cpp
// and adjusted for this project

// Return IR luminosity
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

// Return full spectrum luminosity
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

// Calculate visible lux helper function
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

// Return visible Lux
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