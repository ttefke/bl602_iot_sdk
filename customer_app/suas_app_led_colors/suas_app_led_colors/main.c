// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Input/Output
#include <stdio.h>

// UART library
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

/* Size of the stack for the task */
#define LED_STACK_SIZE 512

void bfl_main(void)
{
  /* Initialize UART
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);
  
   /* (Re)define Heap */
  vPortDefineHeapRegions(xHeapRegions);
  
  /* Set up LED task */
  static StackType_t led_stack[LED_STACK_SIZE];
  static StaticTask_t led_task;
  
  /* Start up LED task */
  extern void task_led(void *pvParameters); 
  /* This function is defined in led.c but add prototype here so that the compiler knows we define this in another file.
  You could also use a header file for this */

  /* Create the task */
  xTaskCreateStatic(
    task_led, /* name of the function implementing the task -> defined in led.c */
    (char*)"led", /* human readable name of the task */
    LED_STACK_SIZE, /* Stack size */
    NULL, /* parameters for the function -> not required here */
    15, /* Task priority */
    led_stack, /* Stack to use for the task */
    &led_task /* Task handle (could be referenced in API calls later, e.g. for changing its priority )*/    
  );
  
  /* Also start the task (task creation only creates the task control block) */
  vTaskStartScheduler();
}
