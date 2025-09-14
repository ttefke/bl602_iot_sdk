// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>

// lwip TCP stack
#include <lwip/tcpip.h>

// Own header
#include "wifi.h"

#define WIFI_STACK_SIZE 1024

/* main function, execution starts here */
void bfl_main(void)
{
  /* Define information containers for tasks */
  static StackType_t wifi_stack[WIFI_STACK_SIZE];
  static StaticTask_t wifi_task;

  /* Initialize system */
  vInitializeBL602();
  
  /* Start tasks */
  /* WiFi task */
  printf("[SYSTEM] Starting WiFi task\r\n");
  xTaskCreateStatic(task_wifi, (char*)"wifi", WIFI_STACK_SIZE, NULL, 16, wifi_stack, &wifi_task);

  /* Start TCP/IP stack */
  printf("[SYSTEM] Starting TCP/IP stack\r\n");
  tcpip_init(NULL, NULL);
  
  /* Start scheduler */
  printf("[SYSTEM] Starting scheduler\r\n");
  vTaskStartScheduler();
}
