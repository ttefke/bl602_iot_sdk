#include <FreeRTOS.h>
#include <task.h>

#include <stdio.h>
#include <bl_uart.h>

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

void bfl_main(void)
{
  /* Initialize UART
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);

  vPortDefineHeapRegions(xHeapRegions);
  static StackType_t dht22_stack[1024];
  static StaticTask_t dht22_task;

  extern void task_dht22(void *pvParameters);

  xTaskCreateStatic(task_dht22, (char* )"dht22", 1024, NULL, 15, dht22_stack, &dht22_task);

  vTaskStartScheduler();
}
