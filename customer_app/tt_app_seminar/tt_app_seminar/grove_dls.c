#include <FreeRTOS.h>
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <stdio.h>
#include <looprt.h>
#include <loopset_i2c.h>
#include <hal_i2c.h>
#include <grove_dls.h>

#include "conf.h"

void event_cb_i2c_event(input_event_t *event, void *private_data)
{
    switch (event->code) {
        case CODE_I2C_END:
            printf("TRANS FINISH %lld\r\n", aos_now_ms());
            break;
        case CODE_I2C_ARB:
             printf("TRANS ERROR ARB %lld\r\n", aos_now_ms());
            break;
        case CODE_I2C_NAK:
            printf("TRANS ERROR NAK %lld\r\n", aos_now_ms());
            break;
        case CODE_I2C_FER:
            printf("TRANS ERROR FER %lld\r\n", aos_now_ms());
            break;
        default:
             printf("[I2C] [EVT] Unknown code %u, %lld\r\n", event->code, aos_now_ms());
    }
}

volatile unsigned long lux;
void task_grove_dls(void *pvParameters) {
    // yloop looping
    static StackType_t proc_stack_looprt[512];
    static StaticTask_t proc_task_looprt;
    
    looprt_start(proc_stack_looprt, 512, &proc_task_looprt);
    loopset_i2c_hook_on_looprt();
    
    //register i2c
    aos_register_event_filter(EV_I2C, event_cb_i2c_event, NULL);
    hal_i2c_init(0, 500);

    printf("Initializing Grove Digital Light Sensor\r\n");

    if (init_grove_dls() != 0) {
        // retry once if initialization fails
        init_grove_dls();
    }
    
    // wait until wifi is available
    vTaskDelay(30 * 1000 / portTICK_PERIOD_MS);

    // endless loop to get data twice per second
    while (1) {
        lux = readVisibleLux();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    printf("Should never happen - exiting loop\r\n");
    vTaskDelete(NULL);
}
