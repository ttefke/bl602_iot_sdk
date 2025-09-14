// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>

// IP stack
#include <lwip/tcpip.h>

/* Main function, execution starts here */
void bfl_main(void)
{
  /* Define information containers for wifi task */
  static StackType_t wifi_stack[1024];
  static StaticTask_t wifi_task;

  /* Initialize system */
  vInitializeBL602();
  
  /* Start tasks */
  printf("[SYSTEM] Starting WiFi task\r\n");
  extern void task_wifi(void *pvParameters);
  xTaskCreateStatic(task_wifi, (char*)"wifi", 1024, NULL, 16, wifi_stack, &wifi_task);

  /* Start TCP/IP stack */
  printf("[SYSTEM] Starting TCP/IP stack\r\n");
  tcpip_init(NULL, NULL);
  
  /* Start task scheduler */
  printf("[SYSTEM] Starting scheduler\r\n");
  vTaskStartScheduler();
}
