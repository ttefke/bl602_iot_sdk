// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>

// Input/output
#include <stdio.h>

// GPIO library
#include <bl_gpio.h>

// Define LED pins
#define LED_R_PIN 17
#define LED_G_PIN 14
#define LED_B_PIN 11

// define outputs
#define LED_ON 1   // high voltage
#define LED_OFF 0  // low voltage

// Pullup/pulldown resistors
#define ENABLE_PULLUP 1
#define DISABLE_PULLUP 0

#define ENABLE_PULLDOWN 1
#define DISABLE_PULLDOWN 0

/* LED task */
void task_led([[gnu::unused]] void *pvParameters) {
  printf("LED task started\r\n");

  // define LEDs as outputs
  bl_gpio_enable_output(LED_R_PIN, DISABLE_PULLUP, DISABLE_PULLDOWN);
  bl_gpio_enable_output(LED_G_PIN, DISABLE_PULLUP, DISABLE_PULLDOWN);
  bl_gpio_enable_output(LED_B_PIN, DISABLE_PULLUP, DISABLE_PULLDOWN);

  // wait for 100ms
  vTaskDelay(pdMS_TO_TICKS(100));

  // counter
  unsigned char counter = 0;
  while (1) {
    // set output according to counter
    printf("Setting output: %x\r\n", counter);

    if (counter & 0x1) {
      bl_gpio_output_set(LED_R_PIN, LED_ON);
    } else {
      bl_gpio_output_set(LED_R_PIN, LED_OFF);
    }

    if ((counter >> 1) & 0x1) {
      bl_gpio_output_set(LED_G_PIN, LED_ON);
    } else {
      bl_gpio_output_set(LED_G_PIN, LED_OFF);
    }

    if ((counter >> 2) & 0x1) {
      bl_gpio_output_set(LED_B_PIN, LED_ON);
    } else {
      bl_gpio_output_set(LED_B_PIN, LED_OFF);
    }

    // increase counter until maximum is reached
    if (counter < 8) {
      counter++;
    } else {
      counter = 0;
    }

    // wait for 1s
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // should never happen but would delete the task and free allocated resources
  vTaskDelete(NULL);
}
