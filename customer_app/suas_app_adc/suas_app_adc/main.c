// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Input/Output
#include <stdio.h>

// UART library
#include <bl_uart.h>

// DMA library
#include <bl_dma.h>

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
#define ADC_STACK_SIZE 512

void bfl_main(void)
{
  /* Initialize UART
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);
  
   /* (Re)define Heap */
  vPortDefineHeapRegions(xHeapRegions);
  
  /* Initialize DMA */
  bl_dma_init();
  
  /* Set up ADC reading task */
  static StackType_t adc_stack[ADC_STACK_SIZE];
  static StaticTask_t adc_task;
  
  /* Start up ADC task */
  
  /* This function is defined in adc.c but add prototype here so that the compiler knows we define this in another file.
  You could also use a header file for this */
  extern void task_adc(void *pvParameters); 

  /* Create the task */
  xTaskCreateStatic(
    task_adc, /* name of the function implementing the task -> defined in adc.c */
    (char*)"adc", /* human readable name of the task */
    ADC_STACK_SIZE, /* Stack size */
    NULL, /* parameters for the function -> not required here */
    15, /* Task priority */
    adc_stack, /* Stack to use for the task */
    &adc_task /* Task handle (could be referenced in API calls later, e.g. for changing its priority )*/    
  );
  
  /* Also start the task (task creation only creates the task control block) */
  vTaskStartScheduler();
}
