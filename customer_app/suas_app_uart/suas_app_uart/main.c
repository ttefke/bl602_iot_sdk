// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// BouffaloLabs includes (UART)
#include <bl_uart.h>

// Standard input/output
#include <stdio.h>

// Own headers
#include "main.h"
#include "mmWave.h"
#include "uart.h"

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
  /* Initialize UART for USB debugging
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(UART_PORT_USB_CHANNEL, 16, 7, 255, 255, UART_PORT_USB_BAUDRATE);
  printf("Initialized USB\r\n");

  /* Initialize UART for sensor
   * Ports: 3+4 (TX+RX)
   * Baudrate: 115200
   */
  bl_uart_init(UART_PORT_SENSOR_CHANNEL, 3, 4, 255, 255, UART_PORT_SENSOR_BAUDRATE);
  printf("Initialized sensor\r\n");

  /* Redefine heap */
  vPortDefineHeapRegions(xHeapRegions);

  /* Set up sensor reading task */
  static StackType_t sensor_stack[SENSOR_STACK_SIZE];
  static StaticTask_t sensor_task;

  /* Create and start task */
  xTaskCreateStatic(
    read_sensor, /* name of the function implementing the task */
    (char *) "Sensor reading task", /* Human readable name */
    SENSOR_STACK_SIZE, /* Stack size */
    NULL, /* Additional parameters for read_sensor (none)*/
    15, /* Task priority */
    sensor_stack, /* Stack to use*/
    &sensor_task /* Task handle for potential API calls */
  );

  vTaskStartScheduler();
}
