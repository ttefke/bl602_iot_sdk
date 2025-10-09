// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard library
#include <stdio.h>

// Real-time looping
#include <looprt.h>

// Sensor header
#include <suas_ssd1306.h>

// Define size for display stack
#define SSD1306_STACK_SIZE 1024

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
    static StackType_t proc_stack_looprt[256];
    static StaticTask_t proc_task_looprt;

    // Start real-time looping (also starts that thread)
    looprt_start(proc_stack_looprt, 256, &proc_task_looprt);

    // Initialize display
    suas_ssd1306_init();

    // Move cursor
    suas_ssd1306_set_cursor(3, 32);

    // Show text
    char text[] = "Hello PineCone";
    suas_ssd1306_print_text(text);
    
    // And we are done
    vTaskDelete(NULL);
}

/* Main function */
void bfl_main() {
    // Create task for display
    static StackType_t ssd1306_stack[SSD1306_STACK_SIZE];
    static StaticTask_t ssd1306_task;

    // Initialize system
    vInitializeBL602();

    xTaskCreateStatic(ssd1306_handler, (char*)"display task", SSD1306_STACK_SIZE, NULL, 15, ssd1306_stack, &ssd1306_task);
    vTaskStartScheduler();
}
