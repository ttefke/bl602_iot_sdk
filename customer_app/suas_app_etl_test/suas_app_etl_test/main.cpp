extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>
// Standard input/output
#include <stdio.h>

// UART Hardware Abstraction Layer
#include <bl_uart.h>
}

#include <etl/array.h>
#include <etl/memory.h>
#include <etl/string.h>
#include <etl/vector.h>

#define STACK_SIZE 256

/* String example */
void example_string() {
  etl::string<32> myString = "Hello World";
  myString += " from ETL";

  printf("String: \"%s\", len: %d, capacity: %d\r\n", myString.data(),
         myString.size(), myString.max_size());
}

/* Array example */
void example_array() {
  etl::array<int, 3> arr = {1, 2, 3};

  printf("Array: ");
  for (auto val : arr) {
    printf("%d ", val);
  }

  arr[0] = 42;
  printf(", modified array element: %d\r\n", arr[0]);
}

/* Vector example */
void example_vector() {
  etl::vector<int, 5> vec;
  vec.push_back(-1);
  vec.push_back(-2);
  vec.push_back(-3);

  printf("Vector: ");
  for (auto v : vec) {
    printf("%d ", v);
  }
  printf(", size: %d, capacity: %d\r\n", vec.size(), vec.max_size());
}

/* Smart pointer example */
struct SmartPointerExample {
  SmartPointerExample(int i) : value(i) {
    printf("Smart pointer constructed\r\n");
  }
  ~SmartPointerExample() { printf("Smart pointer destroyed\r\n"); }
  int value;
};

void example_pointer() {
  etl::unique_ptr<SmartPointerExample> ptr(new SmartPointerExample(42));
  printf("Smart pointer value: %d\r\n", ptr->value);

  // Transfer ownership
  etl::unique_ptr<SmartPointerExample> ptr2 = etl::move(ptr);
  if (!ptr) {
    printf("Original pointer is now null!\r\n");
  }
}

void examples([[gnu::unused]] void *pvParameters) {
  example_string();
  example_array();
  example_vector();
  example_pointer();

  /* End task */
  vTaskDelete(nullptr);
}

extern "C" void bfl_main(void) {
  /* Initialize system */
  vInitializeBL602();

  /* Initialize UART
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(
      /* UART channel (0 = USB port)*/ 0,
      /* Transmission pin */ 16,
      /* Receive pin */ 7,
      /* Unused */ 255,
      /* Unused */ 255,
      /* Baud rate */ 2 * 1000 * 1000);

  /* Create and start task to test */
  static StackType_t examples_stack[STACK_SIZE];
  static StaticTask_t examples_task;

  xTaskCreateStatic(examples, (char *)"examples", STACK_SIZE, NULL, 15,
                    examples_stack, &examples_task);

  /* Start task scheduler */
  vTaskStartScheduler();
}
