extern "C" {
// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>

// Input/output
#include <stdio.h>

// PWM library
#include <suas_pwm.h>
}

/* LED task */
extern "C" void task_led_pwm([[gnu::unused]] void *pvParameters) {
  printf("LED PWM task started\r\n");

  /* 1. Create configuration for the PWM library */
  auto conf = suas_pwm_conf_t{
      .channel = 0,      // Set during initialization
      .pin = 17,         // Pin of the GPIO pin to use
      .initialized = 0,  // Not yet initialized
      .frequency =
          2048,  // Frequency to set (must be between 2,000 and 800,000)
      .frequency_divider = 4,  // Set a frequency divider.
                               // The actual frequency will be
                               // frequency/frequency_divider = 2048/4 = 512
      .duty = 1.0f,  // Set initial duty cycle (must be between 0 and 100)
  };

  /* 2. Initialize PWM */
  suas_pwm_init(&conf);

  /* 3. Start PWM */
  suas_pwm_start(&conf);

  while (1) {
    for (auto value = 0; value < 100; value++) {
      /* Change duty cycle */
      conf.duty = value;
      suas_pwm_set_duty(&conf);

      /* Wait for 20ms */
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }

  // Should never happen but would delete the task and free allocated resources
  vTaskDelete(nullptr);
}
