// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// BouffaloLabs includes
#include <bl_dma.h>
#include <bl_gpio.h>
#include <bl_irq.h>
#include <bl_rtc.h>
#include <bl_sec.h>
#include <bl_sys.h>
#include <bl_uart.h>

#include <hal_boot2.h>
#include <hal_board.h>
#include <hal_button.h>
#include <hal_uart.h>

#include <blog.h>

// VFS
#include <event_device.h>
#include <libfdt.h>
#include <vfs.h>

// AOS real-time loop
#include <looprt.h>
#include <loopset.h>
#include <aos/yloop.h>
#include <cli.h>

// Standard input/output
#include <stdio.h>

// Bluetooth HAL
#include <ble_lib_api.h>
#include <conn.h>

// Own headers
#include "include/ble.h"
#include "include/central.h"
#include "include/main.h"
#include "include/peripheral.h"

/* Define heap regions */
extern uint8_t _heap_start;
extern uint8_t _heap_size;
extern uint8_t _heap_wifi_start;
extern uint8_t _heap_wifi_size;

static HeapRegion_t xHeapRegions[] =
{
  { &_heap_start, (unsigned int) &_heap_size},
  { &_heap_wifi_start, (unsigned int) &_heap_wifi_size },
  { NULL, 0},
  { NULL, 0}
};

/* Used to differentiate between central and peripheral role */
static enum app_ble_role app_role;

/* Helper function to turn off all LEDs */
void board_leds_off() {
  bl_gpio_output_set(LED_BLUE, 1);
  bl_gpio_output_set(LED_GREEN, 1);
  bl_gpio_output_set(LED_RED, 1);
}

/* Listener for BLE events */
void event_cb_ble_event(input_event_t *event, void *private_data) {
  /* Turn off LEDS */
  board_leds_off();

  /* Output event visually */
  switch (event->code) {
    /* Started advertising */
    case BLE_ADV_START:
      printf("[BLE] Started advertising\r\n");
      bl_gpio_output_set(LED_BLUE, 0);
      break;
    /* Stopped advertising*/
    case BLE_ADV_STOP:
      printf("[BLE] Stopped advertising\r\n");
      break;
    /* Started scanning */
    case BLE_SCAN_START:
      printf("[BLE] Started scanning\r\n");
      bl_gpio_output_set(LED_BLUE, 0);
      break;
    /* Stopped scanning*/
    case BLE_SCAN_STOP:
      printf("[BLE] Stopped scanning\r\n");
      break;
    /* Device connected */
    case BLE_DEV_CONN:
      printf("[BLE] Device connected\r\n");
      bl_gpio_output_set(LED_GREEN, 0);
      break;
    /* Device disconnected */
    case BLE_DEV_DISCONN:
      printf("[BLE] Device disconnected\r\n");
      bl_gpio_output_set(LED_RED, 0);

      // Wait 5s and retry to connect
      vTaskDelay(5000);

      if (app_role == CENTRAL) {
        ble_central_start_scanning();
      } else { // Peripheral
        ble_peripheral_start_advertising();
      }
      break;
    /* Only called by client: exchange MTU size*/
    case BLE_DEV_SUBSCRIBED:
      bl_gpio_output_set(LED_GREEN, 0);
      ble_central_exchange_mtu();
      break;
    /* Unknown event */
    default:
      printf("[BLE] Unknown code\r\n");
  }
}

/* Listener for key events */
void event_cb_key_event(input_event_t *event, void *private_data) {
  switch (event->code) {
    // Short key press (read a high value on GPIO pin 2 for 100-3000ms)
    // -> start as peripheral
    case KEY_1:
      printf("[KEY] Short press detected\r\n");
      app_role = PERIPHERAL;
      start_peripheral_application();
      break;
    // Long key press (read a high value on GPIO pin 2 for 6-10s)
    // -> start as central
    case KEY_2:
      printf("[KEY] Long press detected\r\n");
      app_role = CENTRAL;
      start_central_application();
      break;
    // Very long key press (read a high value on GPIO pin 2 for at least 15s)
    // -> send data
    case KEY_3:
      printf("[KEY] Very long press detected\r\n");      
      if (app_role == CENTRAL) {
        // Invoke ATT Command
        ble_central_write();
      } else {
        // Invoke ATT Notification
        ble_peripheral_send_notification();
      }
      break;
    default:
      printf("[KEY] Unknown key event\r\n");
  }
}

/* Read device tree*/
int get_dts_addr(const char *name, uint32_t *start, uint32_t *off) {
  uint32_t addr = hal_board_get_factory_addr();
  const void *fdt = (const void *) addr;
  uint32_t offset;

  if (!name || !start || ! off) {
    return -1;
  }

  offset = fdt_subnode_offset(fdt, 0, name);
  if (offset <= 0) {
    printf("[ERR] Could not find device in device tree\r\n");
    return -1;
  }

  *start = (uint32_t) fdt;
  *off = offset;

  return 0;
}

/* Event loop */
void aos_loop_proc(void *pvParameters) {
  /* Init real-time loop */
  static StackType_t proc_stack_looprt[512];
  static StaticTask_t proc_task_looprt;

  looprt_start(proc_stack_looprt, 512, &proc_task_looprt);
  loopset_led_hook_on_looprt();

  /* Initialize VFS */
  vfs_init();
  vfs_device_init();

  /* Initialize UART */
  uint32_t fdt = 0, offset = 0;
  if (get_dts_addr("uart", &fdt, &offset) == 0)
  {
    vfs_uart_init(fdt, offset);
  }

  /* Initialize GPIO button */
  if (get_dts_addr("gpio", &fdt, &offset) == 0)
  {
    fdt_button_module_init((const void *)fdt, (int) offset);
  }

  /* Start loop */
  aos_loop_init();

  /* Console */
  int fd_console = aos_open("/dev/ttyS0", 0);
  if (fd_console >= 0) {
    printf("[SYS] Console initialization");
    aos_cli_init(0);
    aos_poll_read_fd(fd_console, aos_cli_event_cb_read_get(), (void *) 0x12345678);
  }

  /* Register event filters */
  aos_register_event_filter(EV_KEY, event_cb_key_event, NULL);
  aos_register_event_filter(EV_BLE_TEST, event_cb_ble_event, NULL);

  aos_loop_run();

  printf("Exited real time loop!\r\n");
  vTaskDelete(NULL);
}

/* Keepalive task */
void keep_alive_entry(void *pvParameters) {
  while(1) {
    vTaskDelay(60 * 1000);
  }
  vTaskDelete(NULL);
}

void bfl_main(void) {
  /* Task declarations */
  static StackType_t keep_alive_stack[512];
  static StaticTask_t keep_alive_task;

  static StackType_t aos_loop_proc_stack[1024];
  static StaticTask_t aos_loop_proc_task;

  /* Initialize system */
  bl_sys_early_init();

  /* Initialize UART for USB debugging
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);
  printf("Initialized USB\r\n");

  bl_sys_init();

  /* LEDs */
  bl_gpio_enable_output(LED_BLUE, 1, 0);
  bl_gpio_enable_output(LED_RED, 1, 0);
  bl_gpio_enable_output(LED_GREEN, 1, 0);
  board_leds_off();

  /* Redefine heap */
  vPortDefineHeapRegions(xHeapRegions);
  
  /* Initialize hardware functions */
  blog_init();
  bl_irq_init();
  bl_sec_init();
  bl_sec_test();
  bl_dma_init();
  bl_rtc_init();
  hal_boot2_init();
  hal_board_cfg(0);

  /* Create tasks */
  xTaskCreateStatic(keep_alive_entry, (char *) "keepalive", 512, NULL, 15, keep_alive_stack, &keep_alive_task);
  xTaskCreateStatic(aos_loop_proc, (char *) "event loop", 1024, NULL, 15, aos_loop_proc_stack, &aos_loop_proc_task);

  /* Start tasks */
  vTaskStartScheduler();
}
