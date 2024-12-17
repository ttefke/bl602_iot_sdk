#include <FreeRTOS.h>
#include <task.h>

#include <stdio.h>

#include <bl_dma.h>
#include <bl_irq.h>
#include <bl_sec.h>
#include <bl_sys_time.h>
#include <bl_uart.h>
#include <hal_boot2.h>
#include <hal_board.h>
#include <hal_hwtimer.h>

#include <blog.h>

/* Define heap regions */
extern uint8_t _heap_start;
extern uint8_t _heap_size;
extern uint8_t _heap_wifi_start;
extern uint8_t _heap_wifi_size;

static HeapRegion_t xHeapRegions[] =
{
  { &_heap_start, (unsigned int) &_heap_size},
  { &_heap_wifi_start, (unsigned int) &_heap_wifi_size },
  { NULL, 0},
  { NULL, 0}
};

/* main function, execution starts here */
void bfl_main(void)
{
  /* Define information containers for tasks */
  static StackType_t grove_stack[512];
  static StaticTask_t grove_task_dls;

  static StackType_t wifi_stack[1024];
  static StaticTask_t wifi_task;
  
  static StackType_t tcp_stack[1024];
  static StaticTask_t tcp_task;

  static StackType_t http_stack[768];
  static StaticTask_t http_task;

  /* Initialize UART
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);
  printf("[SYSTEM] Starting up!\r\n");
  
  /* (Re)define Heap */
  vPortDefineHeapRegions(xHeapRegions);
  
  /* Initialize system */
  blog_init();
  bl_irq_init();
  bl_sec_init();
  bl_dma_init();
  hal_boot2_init();
  hal_board_cfg(0);
  
  /* Start tasks */
  printf("[SYSTEM] Starting grove light sensor task\r\n");
  extern void task_grove_dls(void *pvParameters);
  xTaskCreateStatic(task_grove_dls, (char*)"grove dls", 512, NULL, 15, grove_stack, &grove_task_dls);

  printf("[SYSTEM] Starting WiFi task\r\n");
  extern void task_wifi(void *pvParameters);
  xTaskCreateStatic(task_wifi, (char*)"wifi", 1024, NULL, 17, wifi_stack, &wifi_task);

  printf("[SYSTEM] Starting TCP/IP stack\r\n");
  extern void task_tcp(void *pvParameters);
  xTaskCreateStatic(task_tcp, (char*)"tcp", 1024, NULL, 18, tcp_stack, &tcp_task);

  printf("[SYSTEM] Starting http task\r\n");
  extern void task_http(void *pvParameters);
  xTaskCreateStatic(task_http, (char*)"http", 768, NULL, 16, http_stack, &http_task);
  
  /* Start scheduler */
  printf("[SYSTEM] Starting scheduler\r\n");
  vTaskStartScheduler();
}
