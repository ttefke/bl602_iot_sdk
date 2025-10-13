// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>

// GPIO HAL
#include <bl_gpio.h>

// TCP/IP stack
#include <lwip/tcpip.h>

// LEDs include
#include "leds.h"

// LED state
struct led_state led_state;

#define WIFI_STACK_SIZE 1024
#define HTTPD_STACK_SIZE 512

/* main function, execution starts here */
void bfl_main(void) {
  /* Define information containers for tasks */
  static StackType_t wifi_stack[WIFI_STACK_SIZE];
  static StaticTask_t wifi_task;

  static StackType_t httpd_stack[HTTPD_STACK_SIZE];
  static StaticTask_t httpd_task;

  /* Initialize system */
  vInitializeBL602();

  /* Configure LED pins as GPIO output */
  bl_gpio_enable_output(LED_R_PIN, 0, 0);
  bl_gpio_enable_output(LED_G_PIN, 0, 0);
  bl_gpio_enable_output(LED_B_PIN, 0, 0);

  /* Set all LEDs to low */
  led_state.state_led_red = LED_OFF;
  led_state.state_led_green = LED_OFF;
  led_state.state_led_blue = LED_OFF;
  apply_led_state();

  /* Start tasks */
  printf("[SYSTEM] Starting httpd task\r\n");
  extern void task_httpd(void *pvParameters);
  xTaskCreateStatic(task_httpd, (char *)"httpd", HTTPD_STACK_SIZE, NULL, 10,
                    httpd_stack, &httpd_task);

  printf("[SYSTEM] Starting WiFi task\r\n");
  extern void task_wifi(void *pvParameters);
  xTaskCreateStatic(task_wifi, (char *)"wifi", WIFI_STACK_SIZE, NULL, 16,
                    wifi_stack, &wifi_task);

  /* Start TCP/IP stack */
  printf("[SYSTEM] Starting TCP/IP stack\r\n");
  tcpip_init(NULL, NULL);

  /* Start scheduler */
  printf("[SYSTEM] Starting scheduler\r\n");
  vTaskStartScheduler();
}
