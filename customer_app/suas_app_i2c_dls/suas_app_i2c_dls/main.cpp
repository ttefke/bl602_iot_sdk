extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard library
#include <stdio.h>

// Real-time looping
#include <looprt.h>

// Sensor header
#include <suas_dls.h>
}

#include <etl/string.h>

/* Connection setup
    DLS     PineCone
    GND     GND
    VCC     3V3
    SCL     IO4
    SDA     IO3
*/

// Function implementing the task to query sensor regularly
void grove_handler([[gnu::unused]] void *pvParameters) {
  // Set up real-time looping to listen for hardware events (separate thread)
  constexpr uint16_t LOOPRT_STACK_SIZE = 256;
  constinit static StackType_t proc_stack_looprt[LOOPRT_STACK_SIZE]{};
  constinit static StaticTask_t proc_task_looprt{};

  // Start real-time looping (also starts that thread)
  looprt_start(proc_stack_looprt, LOOPRT_STACK_SIZE, &proc_task_looprt);

  // Initialize sensor (see separate function below)
  printf("Initializing Grove Digital Light Sensor V1.1\r\n");
  suas_dls_init();

  // Constantly get data and print it (see function definitions below)
  while (1) {
    printf("========================================\r\n");
    printf("IR luminosity: %d\r\n", suas_dls_read_ir());
    printf("Full spectrum luminosity: %d\r\n", suas_dls_read_fs());
    printf("Visible Lux: %ld\r\n", suas_dls_read_visible_lux());
    vTaskDelay(pdMS_TO_TICKS(5000));
  }

  // Show error message if tasks exists and remove task from lists of tasks
  printf("Should never happen - exiting loop\r\n");
  vTaskDelete(nullptr);
}

/* Main function */
extern "C" void bfl_main() {
  // Create task to read sensor frequently
  constexpr uint16_t GROVE_STACK_SIZE = 512;
  constinit static StackType_t grove_stack[GROVE_STACK_SIZE]{};
  constinit static StaticTask_t grove_task{};

  // Initialize system
  vInitializeBL602();

  xTaskCreateStatic(grove_handler,
                    etl::string_view("grove sensor stack").data(),
                    GROVE_STACK_SIZE, nullptr, 15, grove_stack, &grove_task);
  vTaskStartScheduler();
}
