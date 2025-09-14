// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

#define DHT22_STACK_SIZE 1024

/* Main function */
void bfl_main(void)
{
  /* Task and stack definition */
  static StackType_t dht22_stack[DHT22_STACK_SIZE];
  static StaticTask_t dht22_task;

  extern void task_dht22(void *pvParameters);
  xTaskCreateStatic(task_dht22, (char* )"dht22", DHT22_STACK_SIZE, NULL, 15, dht22_stack, &dht22_task);

  vTaskStartScheduler();
}
