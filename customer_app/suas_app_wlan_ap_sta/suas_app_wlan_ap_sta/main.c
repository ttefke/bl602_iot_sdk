// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>

// TCP/IP stack
#include <lwip/tcpip.h>

// Own header
#include "wifi.h"

/* main function, execution starts here */
void bfl_main(void)
{
  /* Define information containers for tasks */
  static StackType_t wifi_stack[1024];
  static StaticTask_t wifi_task;

  /* Initialize hardware drivers */
  vInitializeBL602();

  /* Start tasks */
  /* WiFi task */
  printf("[SYSTEM] Starting WiFi task\r\n");
  xTaskCreateStatic(task_wifi, (char*)"wifi", 1024, NULL, 16, wifi_stack, &wifi_task);

  /* Start TCP/IP stack */
  printf("[SYSTEM] Starting TCP/IP stack\r\n");
  tcpip_init(NULL, NULL);
  
  /* Start scheduler */
  printf("[SYSTEM] Starting scheduler\r\n");
  vTaskStartScheduler();
}
