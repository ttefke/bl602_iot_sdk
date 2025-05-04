// FreeRTOS includes
#include <FreeRTOS.h>

// BouffaloLabs includes
#include <blog.h>
#include <bl_uart.h>
#include <bl_irq.h>

// Own headers
#include "grove_dls.h"
#include "grove_dls_handler.h"

// Define heap regions
extern uint8_t _heap_start;
extern uint8_t _heap_size;
extern uint8_t _heap_wifi_start;
extern uint8_t _heap_wifi_size;
static HeapRegion_t xHeapRegions[] =
{
        { &_heap_start,  (unsigned int) &_heap_size},
        { &_heap_wifi_start, (unsigned int) &_heap_wifi_size },
        { NULL, 0 },
        { NULL, 0 }
};

/* Main function */
void bfl_main() {
    // Initialize UART (used to send messages via USB)
    bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);

    // Redefine heap for FreeRTOS
    vPortDefineHeapRegions(xHeapRegions);

    // Initialize system
    blog_init(); /* BouffaloLabs logging required for I2C */
    bl_irq_init(); /* Interrupt requests required for I2C*/

    // Create task to read sensor frequently
    static StackType_t grove_stack[512];
    static StaticTask_t grove_task;

    xTaskCreateStatic(grove_handler, (char*)"grove sensor stack", 512, NULL, 15, grove_stack, &grove_task);
    vTaskStartScheduler();
}
