extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// BouffaloLabs includes
#include <bl_dma.h>
#include <bl_gpio.h>
#include <blog.h>
#include <hal_board.h>
#include <hal_button.h>
#include <hal_uart.h>

// VFS
#include <event_device.h>
#include <libfdt.h>
#include <vfs.h>

// AOS real-time loop
#include <aos/yloop.h>
#include <cli.h>
#include <looprt.h>
#include <loopset.h>

// Standard input/output
#include <stdio.h>

// Bluetooth HAL
#include <ble_lib_api.h>

// Own headers
#include "include/ble.h"
#include "include/central.h"
#include "include/main.h"
#include "include/peripheral.h"
}

#include <etl/string.h>

/* Loop task - Thread Local Storage:

Index   Type                Description
0       enum app_ble_role   Used to differentiate between central and peripheral
                            role in the event listeners (BLE event, key presses)
*/

/* Helper function to turn off all LEDs */
void board_leds_off() {
  bl_gpio_output_set(LED_BLUE, 1);
  bl_gpio_output_set(LED_GREEN, 1);
  bl_gpio_output_set(LED_RED, 1);
}

/* Listener for BLE events */
void event_cb_ble_event(input_event_t *event,
                        [[gnu::unused]] void *private_data) {
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
    case BLE_DEV_DISCONN: {
      printf("[BLE] Device disconnected\r\n");
      bl_gpio_output_set(LED_RED, 0);

      // Wait 5s and retry to connect
      vTaskDelay(pdMS_TO_TICKS(5000));

      // Get app role from Thread Local Storage
      auto app_role = static_cast<enum app_ble_role>(
          reinterpret_cast<uintptr_t>(pvTaskGetThreadLocalStoragePointer(
              /* Task */ nullptr,
              /* Index */ 0)));

      // Central: start scanning
      if (app_role == CENTRAL) {
        ble_central_start_scanning();
      } else {  // Peripheral: start advertising -> defined in gatt_server.c
        ble_peripheral_start_advertising();
      }
    } break;
    /* Only called by central: exchange MTU size*/
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
void event_cb_key_event(input_event_t *event,
                        [[gnu::unused]] void *private_data) {
  /* Get current app role enum */
  auto app_role = static_cast<enum app_ble_role>(
      reinterpret_cast<uintptr_t>(pvTaskGetThreadLocalStoragePointer(
          /* Task */ nullptr,
          /* Index */ 0)));

  switch (event->code) {
    // Short key press (read a high value on GPIO pin 2 for 100-3000ms)
    // -> start as peripheral
    case KEY_1:
      if (app_role == UNINITIALIZED) {
        printf("[KEY] Short press detected - starting as peripheral\r\n");

        // Update app role
        app_role = PERIPHERAL;
        vTaskSetThreadLocalStoragePointer(
            /* Task */ nullptr,
            /* Index */ 0,
            /* Value */
            reinterpret_cast<void *>(static_cast<uintptr_t>(app_role)));

        // Start
        start_peripheral_application();
      }
      break;
    // Long key press (read a high value on GPIO pin 2 for 6-10s)
    // -> start as central
    case KEY_2:
      if (app_role == UNINITIALIZED) {
        printf("[KEY] Long press detected - starting as central\r\n");

        // Update app role
        app_role = CENTRAL;
        vTaskSetThreadLocalStoragePointer(
            /* Task */ nullptr,
            /* Index */ 0,
            /* Value */
            reinterpret_cast<void *>(static_cast<uintptr_t>(app_role)));

        // Start
        start_central_application();
      }
      break;
    // Very long key press (read a high value on GPIO pin 2 for at least 15s)
    // -> send data
    case KEY_3:
      printf("[KEY] Very long press detected\r\n");
      if (app_role == CENTRAL) {
        // Invoke ATT Command
        ble_central_write();
      } else if (app_role == PERIPHERAL) {
        // Invoke ATT Notification
        ble_peripheral_send_notification();
      }
      break;
    default:
      printf("[KEY] Unknown key event\r\n");
  }
}

/* Helper function to read device tree */
static int get_dts_addr(etl::string_view name, uint32_t &start, uint32_t &off) {
  /* Check we get valid data*/
  if (name.empty()) {
    return -1;
  }

  /* Compute device tree data */
  auto fdt = reinterpret_cast<const void *>(hal_board_get_factory_addr());
  auto offset = fdt_subnode_offset(fdt, 0, name.data());

  /* Check if offset is valid*/
  if (offset <= 0) {
    log_error("%s is invalid\r\n", name);
    return -1;
  }

  /* Return final data */
  start = reinterpret_cast<uint32_t>(fdt);
  off = offset;
  return 0;
}

/* Event loop */
void aos_loop_proc([[gnu::unused]] void *pvParameters) {
  // Setup looprt task
  uint32_t fdt = 0, offset = 0;  // these must be uint32_t
  constexpr uint16_t LOOPRT_STACK_SIZE = 512;
  constinit static StackType_t task_looprt_stack[LOOPRT_STACK_SIZE]{};
  constinit static StaticTask_t task_looprt_task{};

  /* Init looprt */
  looprt_start(task_looprt_stack, LOOPRT_STACK_SIZE, &task_looprt_task);
  loopset_led_hook_on_looprt();

  /* Initialize VFS */
  vfs_init();
  vfs_device_init();

  /* Setup UART */
  if (get_dts_addr(etl::string_view("uart"), fdt, offset) == 0) {
    vfs_uart_init(fdt, offset);
  }

  /* Setup GPIO */
  if (get_dts_addr(etl::string_view("gpio"), fdt, offset) == 0) {
    fdt_button_module_init(reinterpret_cast<const void *>(fdt),
                           static_cast<int>(offset));
  }
  /* Start loop */
  aos_loop_init();

  /* Store app role enumeration */
  enum app_ble_role app_role = UNINITIALIZED;
  vTaskSetThreadLocalStoragePointer(
      /* Task */ nullptr,
      /* Index */ 0,
      /* Value */ reinterpret_cast<void *>(static_cast<uintptr_t>(app_role)));

  /* Register event filters */
  aos_register_event_filter(EV_KEY, event_cb_key_event, nullptr);
  aos_register_event_filter(EV_BLE_TEST, event_cb_ble_event, nullptr);

  aos_loop_run();

  printf("Exited real time loop!\r\n");
  vTaskDelete(nullptr);
}

extern "C" void bfl_main(void) {
  /* Task declarations */
  constexpr uint16_t LOOP_PROC_STACK_SIZE = 1024;
  constinit static StackType_t aos_loop_proc_stack[LOOP_PROC_STACK_SIZE]{};
  constinit static StaticTask_t aos_loop_proc_task{};

  /* Initialize system */
  vInitializeBL602();

  /* LEDs */
  bl_gpio_enable_output(LED_BLUE, 1, 0);
  bl_gpio_enable_output(LED_RED, 1, 0);
  bl_gpio_enable_output(LED_GREEN, 1, 0);
  board_leds_off();

  /* Create tasks */
  xTaskCreateStatic(aos_loop_proc, etl::string_view("event loop").data(),
                    LOOP_PROC_STACK_SIZE, nullptr, 15, aos_loop_proc_stack,
                    &aos_loop_proc_task);

  /* Start tasks */
  vTaskStartScheduler();
}
