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
  auto myString = etl::string<32>("Hello World");
  myString += " from ETL";

  printf("String: \"%s\", len: %d, capacity: %d\r\n", myString.data(),
         myString.size(), myString.max_size());
}

/* Array example */
void example_array() {
  auto arr = etl::array<int, 3>{1, 2, 3};

  printf("Array: ");
  for (auto val : arr) {
    printf("%d ", val);
  }

  arr[0] = 42;
  printf(", modified array element: %d\r\n", arr[0]);
}

/* Vector example */
void example_vector() {
  auto vec = etl::vector<int, 5>();
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
    printf("Smart pointer %d constructed\r\n", value);
  }
  ~SmartPointerExample() { printf("Smart pointer %d destroyed\r\n", value); }
  int value;
};

etl::unique_ptr<SmartPointerExample> example_pointer() {
  // Create a smart pointer that only lives within this function
  auto ptr1 = etl::unique_ptr<SmartPointerExample>(new SmartPointerExample(1));
  printf("Smart pointer 1 value: %d\r\n", ptr1->value);

  // Create a smart pointer that is returned (and transferred)
  auto ptr2 = etl::unique_ptr<SmartPointerExample>(new SmartPointerExample(2));
  printf("Smart pointer 2 value: %d\r\n", ptr2->value);

  // Transfer ownership
  auto ptr3 = etl::unique_ptr<SmartPointerExample>(etl::move(ptr2));
  if (!ptr2) {
    printf("Original pointer 2 is now null\r\n");
  }

  return etl::move(ptr3);
}

void examples([[gnu::unused]] void *pvParameters) {
  // We need to put the code into curly braces here.
  // When the task exists, vTaskDelete is called and it deletes the current task
  // immediately. The stack may is reclaimed before local variables go out of
  // scope. This would prevent the smart pointer from being freed.We have to add
  // the curly braces here to ensure the smart pointer goes out of scope before
  // calling vTaskDelete so that it is freed properly.
  {
    example_string();
    example_array();
    example_vector();

    auto smartPointer = example_pointer();
    printf("Smart pointer value in calling function: %d\r\n",
           smartPointer->value);
  }
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

  xTaskCreateStatic(examples, etl::string_view("examples").data(), STACK_SIZE,
                    nullptr, 15, examples_stack, &examples_task);

  /* Start task scheduler */
  vTaskStartScheduler();
}
