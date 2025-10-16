extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard library
#include <stdio.h>

// Real-time looping
#include <looprt.h>

// Sensor header
#include <suas_ssd1306.h>
}

#include <etl/string.h>

/* Connection setup
    SSD1306 PineCone
    GND     GND
    VCC     3V3
    SCL     IO4
    SDA     IO3
*/

// Function to handle the display
void ssd1306_handler([[gnu::unused]] void *pvParameters) {
  // Set up real-time looping to listen for hardware events (separate thread)
  constexpr uint16_t LOOPRT_STACKS_SIZE = 256;
  constinit static StackType_t proc_stack_looprt[LOOPRT_STACKS_SIZE]{};
  constinit static StaticTask_t proc_task_looprt{};

  // Start real-time looping (also starts that thread)
  looprt_start(proc_stack_looprt, LOOPRT_STACKS_SIZE, &proc_task_looprt);

  // Initialize display
  suas_ssd1306_init();

  // Move cursor
  suas_ssd1306_set_cursor(3, 32);

  // Show text
  auto text = etl::string<16>("Hello PineCone");
  suas_ssd1306_print_text(text.data());

  // And we are done
  vTaskDelete(nullptr);
}

/* Main function */
extern "C" void bfl_main() {
  // Create task for display
  constexpr uint16_t SSD1306_STACK_SIZE = 1024;
  constinit static StackType_t ssd1306_stack[SSD1306_STACK_SIZE]{};
  constinit static StaticTask_t ssd1306_task{};

  // Initialize system
  vInitializeBL602();
  xTaskCreateStatic(ssd1306_handler, etl::string_view("display task").data(),
                    SSD1306_STACK_SIZE, nullptr, 15, ssd1306_stack,
                    &ssd1306_task);
  vTaskStartScheduler();
}
