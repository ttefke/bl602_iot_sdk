// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Input/output library
#include <stdio.h>

// Use all integer types
#include <inttypes.h>

// ADC libraries
#include <bl602_adc.h>  //  ADC driver
#include <bl_adc.h>     //  ADC HAL
#include <bl_dma.h>     //  DMA HAL

#include "adc.h"        // Our own header

// ADC task implementation for FreeRTOS
void task_adc([[gnu::unused]] void *pvParameters)
{
  printf("ADC task started\r\n");
  
  // Set GPIO pin for ADC. You can change this to any pin that supports ADC and has a sensor connected.
  int result = init_adc(5);
  if (result != 0) {
    printf("ADC initialization failed, exiting\r\n");
    vTaskDelete(NULL);
  } else {
    printf("ADC initialized successfully\r\n");  
    // Wait until initialization finished
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // Print current ADC values every second
    while (1) {
      printf("Current value of digitized analog signal: %"PRIu32"\r\n", read_adc());
#if DEBUG == 1
      vTaskDelay(5000 / portTICK_PERIOD_MS);
#else
      vTaskDelay(1000 / portTICK_PERIOD_MS);
#endif
    } 
    
    // Should never happen but would delete the task and free allocated resources
    vTaskDelete(NULL);
  }
}

void set_adc_gain(uint32_t gain1, uint32_t gain2) {
  // Read configuration hardware register
  uint32_t reg = BL_RD_REG(AON_BASE, AON_GPADC_REG_CONFIG2);
  
  // Set ADC gain bits
  reg = BL_SET_REG_BITS_VAL(reg, AON_GPADC_PGA1_GAIN, gain1);
  reg = BL_SET_REG_BITS_VAL(reg, AON_GPADC_PGA2_GAIN, gain2);
  
  // Set chop mode
  if (gain1 != ADC_PGA_GAIN_NONE || gain2 != ADC_PGA_GAIN_NONE) {
    reg = BL_SET_REG_BITS_VAL(reg, AON_GPADC_CHOP_MODE, 2);
  } else {
    reg = BL_SET_REG_BITS_VAL(reg, AON_GPADC_CHOP_MODE, 1);
  }
  
  // Enable ADC amplifier
  reg = BL_CLR_REG_BIT(reg, AON_GPADC_PGA_VCMI_EN);
  if (gain1 != ADC_PGA_GAIN_NONE || gain2 != ADC_PGA_GAIN_NONE) {
    reg = BL_SET_REG_BIT(reg, AON_GPADC_PGA_EN);
  } else {
    reg = BL_CLR_REG_BIT(reg, AON_GPADC_PGA_EN);
  }
  
  // Update ADC configuration hardware register
  BL_WR_REG(AON_BASE, AON_GPADC_REG_CONFIG2, reg);
}


// NOTE: pin must be of the following 4, 5, 6, 9, 10, 11, 12, 13, 14, 15
// Otherwise you may damage your device!
int init_adc(uint8_t pin) {
  // Ensure a valid pin was selected
  if (adc_channel_exists(pin) == -1) {
    printf("Invalid pin selected for ADC\r\n");
    return -1;
  }
  
  // Set frequency and single channel conversion mode
  bl_adc_freq_init(CONVERSION_MODE, ADC_FREQUENCY); // always returns 0
  
  // Initialize GPIO pin for single channel conversion mode
  bl_adc_init(CONVERSION_MODE, pin); // always returns 0
  
  // Enable ADC gain
  set_adc_gain(ADC_PGA_GAIN_1, ADC_PGA_GAIN_1); // returns nothing

  // Initialize DMA for the ADC channel and for single channel conversion mode
  int result = bl_adc_dma_init(CONVERSION_MODE, NUMBER_OF_SAMPLES);
  if (result != 0) {
    printf("Error occurred during DMA initialization\r\n");
    return result;
  }

  // Configure GPIO pin as ADC input
  bl_adc_gpio_init(pin); // always returns 0

  // Mark pin and ADC as configured
  int channel = bl_adc_get_channel_by_gpio(pin); // returns -1 if channel does not exist but we already tested for that
  adc_ctx_t *ctx = bl_dma_find_ctx_by_channel(ADC_DMA_CHANNEL);
  ctx -> chan_init_table |= (1 << channel);

  // Start reading process
  bl_adc_start(); // always returns 0
  return 0;
}

uint32_t read_adc() {
  // Array storing samples statically
  static uint32_t adc_data[NUMBER_OF_SAMPLES];
  
  // Get DMA context for ADC channel to read data
  adc_ctx_t *ctx = bl_dma_find_ctx_by_channel(ADC_DMA_CHANNEL);
  
  // Return if sampling failed or did not finish
  if (ctx -> channel_data == NULL) {
    return 0;
  }
  
  // Copy read samples from memory written dynamically by DMA to static array
  uint32_t received_number_of_samples =
    ctx->data_size <= sizeof(adc_data) ? ctx->data_size : sizeof(adc_data);
  
  memcpy(
    (uint8_t*) adc_data, /* destination: adc_data array */
    (uint8_t*) (ctx -> channel_data), /* source: channel_data element of ctx struct */
    received_number_of_samples  /* amount of bytes to be copied */
  );

#if DEBUG == 1
  printf("Read raw data:\r\n");
  for (uint32_t i = 0; i < received_number_of_samples; i++) {
    printf("%"PRIu32"\r\n", ctx->channel_data[i]);
  }
#endif
  
  // Calculate mean value
  uint32_t mean = 0;
  for (uint32_t i = 0; i < received_number_of_samples; i++) {
    mean += adc_data[i];
  }
  mean /= received_number_of_samples;

  // Scale down mean and return
  uint32_t scaled_mean = ((mean & 0xFFFF) * 4096) >> 16;  
  return scaled_mean;
}
