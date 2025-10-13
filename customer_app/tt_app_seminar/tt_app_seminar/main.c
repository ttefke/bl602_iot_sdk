// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>

/* Stack size definitions */
#define GROVE_STACK_SIZE 512
#define WIFI_STACK_SIZE 1024
#define TCP_STACK_SIZE 1024
#define HTTP_STACK_SIZE 768

/* Connection setup
    DLS     PineCone
    GND     GND
    VCC     3V3
    SCL     IO4
    SDA     IO3
*/

/* main function, execution starts here */
void bfl_main(void) {
  /* Define information containers for tasks */
  static StackType_t grove_stack[GROVE_STACK_SIZE];
  static StaticTask_t grove_task_dls;

  static StackType_t wifi_stack[WIFI_STACK_SIZE];
  static StaticTask_t wifi_task;

  static StackType_t tcp_stack[TCP_STACK_SIZE];
  static StaticTask_t tcp_task;

  static StackType_t http_stack[HTTP_STACK_SIZE];
  static StaticTask_t http_task;

  /* Initialize system */
  vInitializeBL602();

  /* Start tasks */
  printf("[SYSTEM] Starting grove light sensor task\r\n");
  extern void task_grove_dls(void *pvParameters);
  xTaskCreateStatic(task_grove_dls, (char *)"grove dls", GROVE_STACK_SIZE, NULL,
                    15, grove_stack, &grove_task_dls);

  printf("[SYSTEM] Starting WiFi task\r\n");
  extern void task_wifi(void *pvParameters);
  xTaskCreateStatic(task_wifi, (char *)"wifi", WIFI_STACK_SIZE, NULL, 17,
                    wifi_stack, &wifi_task);

  printf("[SYSTEM] Starting TCP/IP stack\r\n");
  extern void task_tcp(void *pvParameters);
  xTaskCreateStatic(task_tcp, (char *)"tcp", TCP_STACK_SIZE, NULL, 18,
                    tcp_stack, &tcp_task);

  printf("[SYSTEM] Starting http task\r\n");
  extern void task_http(void *pvParameters);
  xTaskCreateStatic(task_http, (char *)"http", HTTP_STACK_SIZE, NULL, 16,
                    http_stack, &http_task);

  /* Start scheduler */
  printf("[SYSTEM] Starting scheduler\r\n");
  vTaskStartScheduler();
}
