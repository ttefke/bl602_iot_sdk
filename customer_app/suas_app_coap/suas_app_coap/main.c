// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard library
#include <stdio.h>

// lwIP
#include <lwip/tcpip.h>

// Wifi header
#include "include/wifi.h"

#define WIFI_STACK_SIZE 512

/* Main function, system starts here */
void bfl_main(void) {
  /* Define containers for WiFi task */
  static StackType_t wifi_stack[WIFI_STACK_SIZE];
  static StaticTask_t wifi_task;

  /* Initialize system */
  vInitializeBL602();

  /* Start tasks */
  /* WiFi task */
  printf("[%s] Starting WiFi task\r\n", __func__);
  xTaskCreateStatic(task_wifi, (char*)"wifi", WIFI_STACK_SIZE, NULL, 16,
                    wifi_stack, &wifi_task);

  /* Start TCP/IP stack */
  printf("[%s] Starting TCP/IP stack\r\n", __func__);
  tcpip_init(NULL, NULL);

  /* Start scheduler */
  printf("[%s] Starting scheduler\r\n", __func__);
  vTaskStartScheduler();
}
