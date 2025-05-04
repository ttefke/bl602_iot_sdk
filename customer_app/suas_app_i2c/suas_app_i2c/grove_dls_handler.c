// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// I2C hardware abstraction layer and related headers for real-time operations
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <hal_i2c.h>
#include <looprt.h>
#include <loopset_i2c.h>

// C standard library functionalities
#include <stdio.h>

// Own headers
#include "grove_dls.h"
#include "grove_dls_handler.h"

// Function implementing the task to query sensor regularly
void grove_handler(void *pvParameters) {
    // Set up real-time looping to listen for hardware events (separate thread)
    static StackType_t proc_stack_looprt[256];
    static StaticTask_t proc_task_looprt;

    // Start real-time looping (also starts that thread)
    looprt_start(proc_stack_looprt, 256, &proc_task_looprt);

    // Add I2C hook to real-time looping
    loopset_i2c_hook_on_looprt();
    
    // Register I2C interrupt (event) filter; callback function
    aos_register_event_filter(EV_I2C, event_cb_i2c_interrupt, NULL);

    // Register I2C channel: Channel 0 (pin 3: SDA, pin 4: SCL), frequency 500 kbps
    hal_i2c_init(0, 500);

    // Initialize sensor (see separate function below)
    printf("Initializing Grove Digital Light Sensor V1.1\r\n");
    init_grove_dls();

    // Constantly get data and print it (see function definitions below)
    while (1) {
        printf("========================================\r\n");
        printf("IR luminosity: %d\r\n", readIRLuminosity());
        printf("Full spectrum luminosity: %d\r\n", readFSLuminosity());
        printf("Visible Lux: %ld\r\n", readVisibleLux());
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    // Show error message if tasks exists and remove task from lists of tasks
    printf("Should never happen - exiting loop\r\n");
    vTaskDelete(NULL);
}

// Callback function to handle I2C interrupts
void event_cb_i2c_interrupt(input_event_t *event, void *private_data)
{
    switch (event->code) {
        case CODE_I2C_END:
            printf("[I2C][EVT] Transfer ended interrupt %lld\r\n", aos_now_ms());
            break;
        case CODE_I2C_ARB:
             printf("[I2C][EVT] Arbitration interrupt %lld\r\n", aos_now_ms());
            break;
        case CODE_I2C_NAK:
            printf("[I2C][EVT] Transfer not acknowledged interrupt  %lld\r\n", aos_now_ms());
            break;
        case CODE_I2C_FER:
            printf("[I2C][EVT] Transfer FIFO overflow/underflow interrupt %lld\r\n", aos_now_ms());
            break;
        default:
             printf("[I2C][EVT] Unknown error code %u, %lld\r\n", event->code, aos_now_ms());
    }
}
