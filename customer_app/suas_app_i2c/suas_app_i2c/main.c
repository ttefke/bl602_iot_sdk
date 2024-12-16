#include <FreeRTOS.h>
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <bl_uart.h>
#include <bl_sec.h>
#include <bl_irq.h>
#include <bl_dma.h>
#include <hal_boot2.h>
#include <hal_board.h>
#include <blog.h>
#include <looprt.h>
#include <loopset_i2c.h>
#include <hal_i2c.h>
#include <grove_dls.h>

/* Define heap regions */
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

static void grove_handler(void *pvParameters) {
    // yloop looping
    static StackType_t proc_stack_looprt[512];
    static StaticTask_t proc_task_looprt;
    looprt_start(proc_stack_looprt, 512, &proc_task_looprt);
    loopset_i2c_hook_on_looprt();
    
    //register i2c
    aos_register_event_filter(EV_I2C, event_cb_i2c_event, NULL);
    hal_i2c_init(0, 500);

    printf("Initializing Grove Digital Light Sensor V1.1\r\n");
    init_grove_dls();

    // endless loop to get data
    for (;;) {
        printf("IR luminosity: %d\r\n", readIRLuminosity());
        printf("Full spectrum luminosity: %d\r\n", readFSLuminosity());
        printf("Visible Lux: %d\r\n", readVisibleLux());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    printf("Should never happen - exiting loop\r\n");
    vTaskDelete(NULL);
}

void bfl_main() {
    // Initialize UART
    bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);

    // Redefine heap
    vPortDefineHeapRegions(xHeapRegions);

    // Initialize system
    blog_init();
    bl_irq_init();
    bl_sec_init();
    bl_sec_test();
    bl_dma_init();
    hal_boot2_init();

    /* board config is set after system is init*/
    hal_board_cfg(0);

    static StackType_t grove_stack[512];
    static StaticTask_t grove_task;

    puts("[OS] Starting grove handler task...\r\n");
    xTaskCreateStatic(grove_handler, (char*)"event_loop", 512, NULL, 15, grove_stack, &grove_task);

    puts("[OS] Starting OS Scheduler...\r\n");
    vTaskStartScheduler();
}
