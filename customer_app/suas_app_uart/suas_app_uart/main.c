// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// BouffaloLabs includes (UART)
#include <bl_uart.h>

// Standard input/output
#include <stdio.h>

// Own header
#include "mmWave.h"

// Sensor stack size
#define SENSOR_STACK_SIZE 256

void bfl_main(void)
{
  /* Initialize system */
  vInitializeBL602();
  
  /* Initialize UART for sensor
   * Ports: 3+4 (TX+RX)
   * Baudrate: 115200
   */
  bl_uart_init(1 /* Channel 0 = USB, Channel 1 = user defined */, 3, 4,
      255, 255, 115200 /* from manual */);
  printf("Initialized sensor\r\n");

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
