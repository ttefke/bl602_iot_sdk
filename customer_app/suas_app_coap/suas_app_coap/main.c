// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard library
#include <stdio.h>

// HALs
#include <bl_dma.h>
#include <bl_irq.h>
#include <bl_sec.h>
#include <bl_uart.h>
#include <hal_boot2.h>
#include <hal_board.h>
#include <hal_hwtimer.h>
#include <blog.h>

// lwIP
#include <lwip/tcpip.h>

// Wifi header
#include "include/wifi.h"

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

/* Main function, system starts here */
void bfl_main(void)
{
  /* Define containers for WiFi task */
  static StackType_t wifi_stack[512];
  static StaticTask_t wifi_task;

  /* Initialize UART
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);
  printf("[%s] Starting up!\r\n", __func__);
  
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
  /* WiFi task */
  printf("[%s] Starting WiFi task\r\n", __func__);
  xTaskCreateStatic(task_wifi, (char*)"wifi", 512, NULL, 16, wifi_stack, &wifi_task);

    /* Start TCP/IP stack */
  printf("[%s] Starting TCP/IP stack\r\n", __func__);
  tcpip_init(NULL, NULL);

  /* Start scheduler */
  printf("[%s] Starting scheduler\r\n", __func__);
  vTaskStartScheduler();
}
