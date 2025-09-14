// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>

// Stack size defintion
#define STACK_SIZE 128

/* First task: define counter that shows its value every seconds and increments */
void task_one([[gnu::unused]] void *pvParameters)
{
  uint16_t i = 0;
  while (1)
  {
    // Increase i

    i++;
    // Print current number of loops
    printf("%s: looped %d times\r\n",
      __func__, i);

    // Wait 1000 ticks
    vTaskDelay(1000);
  }

  // Delete task (this will never be called but should be stated nevertheless)
  vTaskDelete(NULL);
}

/* Second task: prints the handed over number every five seconds */
void task_two(void *pvParameters)
{
  // Receive number from parameters (void pointer!)
  uint8_t number = (uintptr_t) pvParameters;

  while (1)
  {
    // Print handed over number
    printf("%s: received number %d\r\n", __func__, number);

    // Wait 5000 ticks
    vTaskDelay(5000);
  }

  // Delete task
  vTaskDelete(NULL);
}

/* Third task function one: increases number stored in the task */
void task_three_function_one(void)
{
  // Get number, increase it and store it again
  uint8_t number = (uintptr_t) pvTaskGetThreadLocalStoragePointer(
    /* Task (this task -> NULL ) */ NULL,
    /* Local storage index */ 0
  );

  number++;

  vTaskSetThreadLocalStoragePointer(
    /* Task */ NULL,
    /* Local storage index */ 0,
    /* value to be stored */ (void *) (uintptr_t) number
  );
}

/* Third task function two: prints number stored in the task */
void task_three_function_two(void)
{
  uint8_t number = (uintptr_t) pvTaskGetThreadLocalStoragePointer(NULL, 0);
  printf("%s: current number value: %d\r\n", __func__, number);
}

/* Third task: calls two other functions sharing a variable ten times, then deletes itself and task two */
void task_three(void *pvParameters)
{
  // Receive handle for task three from parameters
  TaskHandle_t second_task = (TaskHandle_t) pvParameters;

  // Create number
  uint8_t number = 0;

  // Store number
  vTaskSetThreadLocalStoragePointer(
    /* Task handle (this task -> NULL) */ NULL,
    /* Local storage index */ 0,
    /* Data */ (void*) (uintptr_t) number
  );


  // Call both functions
  for(uint8_t i = 0; i < 10; i++)
  {
    task_three_function_one();
    task_three_function_two();

    // Wait 3000 ticks
    vTaskDelay(3000);
  }

  // Delete task two
  printf("Deleting task two\r\n");
  vTaskDelete(second_task);

  // Deletes itself
  printf("Deleting task three\r\n");
  vTaskDelete(NULL);
}

/* Main function - entry point */
void bfl_main(void)
{
  // Initialize system
  vInitializeBL602();

  /* Set up three tasks in total */
  xTaskCreate(
    task_one, /* function implementing the task */
    (char *) "first task", /* human readable task name */
    STACK_SIZE, /* maximum stack depth */
    NULL, /* parameters handed over -> no parameters*/
    10, /* task priority */
    NULL /* task handle (can be used to access the task later or null) */
  );

  TaskHandle_t second_task_handle = NULL;
  xTaskCreate(
    task_two, /* function implementing the task */
    (char *) "second task", /* human readable task name */
    STACK_SIZE, /* stack depth */
    (void *) 42, /* parameter handed over -> number, must be casted to void pointer */
    10, /* task priority */
    &second_task_handle /* task handle */    
  );

  xTaskCreate(
    task_three,
    (char *) "third task",
    STACK_SIZE,
    (void *) second_task_handle,
    10,
    NULL
  );

  // Start task scheduler -> starts tasks
  vTaskStartScheduler();
}
