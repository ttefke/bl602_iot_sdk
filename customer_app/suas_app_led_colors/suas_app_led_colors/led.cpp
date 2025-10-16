extern "C" {
// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>

// Input/output
#include <stdio.h>

// GPIO library
#include <bl_gpio.h>
}

// Define LED pins
constexpr static uint8_t LED_R_PIN = 17;
constexpr static uint8_t LED_G_PIN = 14;
constexpr static uint8_t LED_B_PIN = 11;

// define outputs
constexpr static uint8_t LED_ON = 1;   // high voltage
constexpr static uint8_t LED_OFF = 0;  // low voltage

// Pullup/pulldown resistors
constexpr static uint8_t ENABLE_PULLUP = 1;
constexpr static uint8_t DISABLE_PULLUP = 0;
constexpr static uint8_t ENABLE_PULLDOWN = 1;
constexpr static uint8_t DISABLE_PULLDOWN = 0;

/* LED task */
extern "C" void task_led([[gnu::unused]] void *pvParameters) {
  printf("LED task started\r\n");

  // define LEDs as outputs
  bl_gpio_enable_output(LED_R_PIN, DISABLE_PULLUP, DISABLE_PULLDOWN);
  bl_gpio_enable_output(LED_G_PIN, DISABLE_PULLUP, DISABLE_PULLDOWN);
  bl_gpio_enable_output(LED_B_PIN, DISABLE_PULLUP, DISABLE_PULLDOWN);

  // wait for 100ms
  vTaskDelay(pdMS_TO_TICKS(100));

  // counter
  uint8_t counter = 0;
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
  vTaskDelete(nullptr);
}
