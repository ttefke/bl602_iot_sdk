// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Input/output library
#include <stdio.h>

// Use all integer types
#include <inttypes.h>
#include <suas_adc.h>

// ADC task implementation for FreeRTOS
void task_adc([[gnu::unused]] void *pvParameters) {
  printf("ADC task started\r\n");

  // Create ADC configuration
  suas_adc_conf_t conf = {.pin = 5,
                          .conversion_mode = NORMAL_ADC_CONVERSION_MODE,
                          .number_of_samples = 50,
                          .frequency = 50};

  // Set GPIO pin for ADC. You can change this to any pin that supports ADC and
  // has a sensor connected.
  int result = suas_adc_init(&conf);
  if (result != 0) {
    printf("ADC initialization failed, exiting\r\n");
    vTaskDelete(NULL);
  } else {
    printf("ADC initialized successfully\r\n");
    // Wait until initialization finished
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Print current ADC values every second
    while (1) {
      printf("Current value of digitized analog signal: %" PRIu32 "\r\n",
             suas_adc_read(&conf));
      vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Should never happen but would delete the task and free allocated
    // resources
    vTaskDelete(NULL);
  }
}
