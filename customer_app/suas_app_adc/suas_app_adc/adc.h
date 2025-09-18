// Header guard begin
#ifndef __SUAS_ADC_H__
#define __SUAS_ADC_H__ 

// ADC libraries
#include <bl_adc.h>     //  ADC HAL

// Define constants

// Set conversion mode
// 40Hz - 1300Hz
#define NORMAL_ADC_CONVERSION_MODE 0

// 500Hz - 16000 Hz
#define MICROPHONE_ADC_CONVERSION_MODE 1

#define CONVERSION_MODE NORMAL_ADC_CONVERSION_MODE

// Define frequency and number of samples
// Adjust this for your use case!
#define ADC_FREQUENCY 50
#define NUMBER_OF_SAMPLES ADC_FREQUENCY // equals 1 measurement per second

// Check whether selected frequency is valid
#if CONVERSION_MODE == NORMAL_ADC_CONVERSION_MODE
  #if (ADC_FREQUENCY < 40) || (ADC_FREQUENCY > 1300)
    #error Selected frequency does not match for normal ADC conversion mode!
  #endif
#elif CONVERSION_MODE == MICROPHONE_ADC_CONVERSION_MODE
  #if (ADC_FREQUENCY < 500) || (ADC_FREQUENCY > 16000)
    #error Selected frequency does not match for microphone ADC conversion mode!
  #endif
#endif

// More / less output
#define DEBUG 0

// Define function prototypes
// (needed for compiler, functions are implemented in c file)
void task_adc(void *pvParameters);

// Give this function a second name which
// has a name that suits our use case better
#define adc_channel_exists(x) bl_adc_get_channel_by_gpio(x)

// Header guard end
#endif