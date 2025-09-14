// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Own headers
#include "grove_dls_handler.h"

#define GROVE_STACK_SIZE 512

/* Main function */
void bfl_main() {
    // Create task to read sensor frequently
    static StackType_t grove_stack[GROVE_STACK_SIZE];
    static StaticTask_t grove_task;

    // Initialize system
    vInitializeBL602();

    xTaskCreateStatic(grove_handler, (char*)"grove sensor stack", GROVE_STACK_SIZE, NULL, 15, grove_stack, &grove_task);
    vTaskStartScheduler();
}
