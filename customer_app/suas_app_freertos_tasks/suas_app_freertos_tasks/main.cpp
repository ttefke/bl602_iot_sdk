extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>
}

#include <etl/string.h>

// Stack size defintion
constexpr uint8_t STACK_SIZE = 128;

/* First task: define counter that shows its value every seconds and
 * increments
 */
void task_one([[gnu::unused]] void *pvParameters) {
  uint16_t i = 0;
  while (1) {
    // Increase i
    i++;

    // Print current number of loops
    printf("%s: looped %d times\r\n", __func__, i);

    // Wait one second
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Delete task (this will never be called but should be stated nevertheless)
  vTaskDelete(nullptr);
}

/* Second task: prints the handed over number every five seconds */
void task_two(void *pvParameters) {
  // Receive number from parameters (void pointer!)
  auto number = reinterpret_cast<uintptr_t>(pvParameters);

  while (1) {
    // Print handed over number
    printf("%s: received number %d\r\n", __func__, number);

    // Wait 5 seconds
    vTaskDelay(pdMS_TO_TICKS(5000));
  }

  // Delete task
  vTaskDelete(nullptr);
}

/* Third task function one: increases number stored in the task */
void task_three_function_one(void) {
  // Get number, increase it and store it again
  auto number = reinterpret_cast<uintptr_t>(pvTaskGetThreadLocalStoragePointer(
      /* Task (this task -> NULL ) */ nullptr,
      /* Local storage index */ 0));

  number++;

  vTaskSetThreadLocalStoragePointer(
      /* Task */ nullptr,
      /* Local storage index */ 0,
      /* value to be stored */
      reinterpret_cast<void *>(static_cast<uintptr_t>(number)));
}

/* Third task function two: prints number stored in the task */
void task_three_function_two(void) {
  auto number = reinterpret_cast<uintptr_t>(
      pvTaskGetThreadLocalStoragePointer(nullptr, 0));
  printf("%s: current number value: %d\r\n", __func__, number);
}

/* Third task: calls two other functions sharing a variable ten times, then
 * deletes itself and task two */
void task_three(void *pvParameters) {
  // Receive handle for task three from parameters
  auto second_task = static_cast<TaskHandle_t>(pvParameters);

  // Create number
  uint8_t number = 0;

  // Store number
  vTaskSetThreadLocalStoragePointer(
      /* Task handle (this task -> NULL) */ nullptr,
      /* Local storage index */ 0,
      /* Data */ reinterpret_cast<void *>(static_cast<uintptr_t>(number)));

  // Call both functions
  for (auto i = 0; i < 10; i++) {
    task_three_function_one();
    task_three_function_two();

    // Wait 3 seconds
    vTaskDelay(pdMS_TO_TICKS(3000));
  }

  // Delete task two
  printf("Deleting task two\r\n");
  vTaskDelete(second_task);

  // Deletes itself
  printf("Deleting task three\r\n");
  vTaskDelete(nullptr);
}

/* Main function - entry point */
extern "C" void bfl_main(void) {
  // Initialize system
  vInitializeBL602();

  /* Set up three tasks in total */
  xTaskCreate(
      task_one, /* function implementing the task */
      etl::string_view("first task").data(), /* human readable task name */
      STACK_SIZE,                            /* maximum stack depth */
      nullptr, /* parameters handed over -> no parameters*/
      10,      /* task priority */
      nullptr  /* task handle (can be used to access the task later or null) */
  );

  TaskHandle_t second_task_handle;
  xTaskCreate(
      task_two, /* function implementing the task */
      etl::string_view("second task").data(), /* human readable task name */
      STACK_SIZE,                             /* stack depth */
      reinterpret_cast<void *>(42), /* parameter handed over -> number, must be
                     casted to void pointer */
      10,                           /* task priority */
      &second_task_handle           /* task handle */
  );

  xTaskCreate(task_three, etl::string_view("third task").data(), STACK_SIZE,
              static_cast<void *>(second_task_handle), 10, nullptr);

  // Start task scheduler -> starts tasks
  vTaskStartScheduler();
}
