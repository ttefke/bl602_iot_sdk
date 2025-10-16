extern "C" {
#include <aos/yloop.h>
#include <bl_wifi.h>
#include <blog.h>
#include <easyflash.h>
#include <event_device.h>
#include <hal_board.h>
#include <hal_gpio.h>
#include <hal_sys.h>
#include <hal_uart.h>
#include <hal_wifi.h>
#include <libfdt.h>
#include <looprt.h>
#include <loopset.h>
#include <stdio.h>
#include <vfs.h>
#include <wifi_mgmr_ext.h>
}
#include <etl/array.h>
#include <etl/string.h>

#include "wifi.h"

/* WiFi configuration */
constinit static wifi_conf_t conf{
    .country_code = "EU",
    .channel_nums = {},
};

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

// Helper function to ensure the wifi manager
// is started only once after initialization
static void _configure_wifi(void) {
  static auto is_initialized = false;
  if (!is_initialized) {
    // Start wifi manager
    printf("[WIFI] Initialized\r\n");
    wifi_mgmr_start_background(&conf);
    printf("[WIFI] MGMR now running in background\r\n");
    is_initialized = true;
  }
}

/* React to WiFi events */
static void event_cb_wifi_event(input_event_t *event,
                                [[gnu::unused]] void *private_data) {
  switch (event->code) {
    case CODE_WIFI_ON_INIT_DONE:
      _configure_wifi();
      break;
    case CODE_WIFI_ON_MGMR_DONE:
      printf("[WIFI] MGMR done\r\n");
      /* we do not implement a  _connect_wifi() because we want to use the AP */
      break;
    case CODE_WIFI_ON_SCAN_DONE:
      wifi_mgmr_cli_scanlist();
      break;
    case CODE_WIFI_ON_EMERGENCY_MAC:
      hal_reboot();
      break;
    case CODE_WIFI_ON_PROV_SSID:
    case CODE_WIFI_ON_PROV_BSSID:
    case CODE_WIFI_ON_PROV_PASSWD:
    case CODE_WIFI_ON_PROV_CONNECT:
    case CODE_WIFI_ON_SCAN_DONE_ONJOIN:
    case CODE_WIFI_ON_MGMR_DENOISE:
    case CODE_WIFI_ON_DISCONNECT:
    case CODE_WIFI_ON_CONNECTING:
    case CODE_WIFI_CMD_RECONNECT:
    case CODE_WIFI_ON_CONNECTED:
    case CODE_WIFI_ON_PRE_GOT_IP:
    case CODE_WIFI_ON_GOT_IP:
    case CODE_WIFI_ON_PROV_DISCONNECT:
    case CODE_WIFI_ON_AP_STA_ADD:
    case CODE_WIFI_ON_AP_STA_DEL:
      // nothing
      break;
    default:
      printf("[WIFI] Unknown event\r\n");
  }
}

/* WiFi task */
extern "C" void task_wifi([[gnu::unused]] void *pvParameters) {
  // Setup looprt task
  uint32_t fdt = 0, offset = 0;  // these must be uint32_t
  constexpr uint16_t LOOPRT_STACK_SIZE = 512;
  constinit static StackType_t task_looprt_stack[LOOPRT_STACK_SIZE]{};
  constinit static StaticTask_t task_looprt_task{};

  /* Init looprt */
  looprt_start(task_looprt_stack, LOOPRT_STACK_SIZE, &task_looprt_task);
  loopset_led_hook_on_looprt();

  /* Setup virtual file system */
  easyflash_init();
  vfs_init();
  vfs_device_init();

  /* Setup UART */
  if (get_dts_addr(etl::string_view("uart"), fdt, offset) == 0) {
    vfs_uart_init(fdt, offset);
  }

  /* Setup GPIO */
  if (get_dts_addr(etl::string_view("gpio"), fdt, offset) == 0) {
    vfs_uart_init(fdt, offset);
  }

  /* initialize command line */
#ifdef CONF_USER_ENABLE_VFS_ROMFS
  romfs_register();
#endif

  aos_loop_init();

  aos_register_event_filter(EV_WIFI, event_cb_wifi_event, nullptr);
  printf("[WIFI] Starting WiFi stack\r\n");

  /* start wifi firmware */
  hal_wifi_start_firmware_task();
  aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);

  printf("[WIFI] Firmware loaded\r\n");
  /* wait */
  vTaskDelay(pdMS_TO_TICKS(500));

  /* Start access point */
  /* Variable initialization */
  auto mac = etl::array<uint8_t, 6>();
  auto ssid_name = etl::string<32>(WIFI_SSID);
  auto ssid_pw = etl::string<32>(WIFI_PW);
  auto channel = 6;
  wifi_interface_t wifi_interface;

  /* Set MAC address */
  mac.fill(0);
  bl_wifi_mac_addr_get(mac.data());

  // Configure wifi manager and start access point
  printf("[WIFI] Starting up access point\r\n");
  _configure_wifi();
  wifi_interface = wifi_mgmr_ap_enable();
  wifi_mgmr_ap_start(&wifi_interface, ssid_name.data(), 0, ssid_pw.data(),
                     channel);
  printf("[WIFI] Started access point, should be reachable\r\n");

  aos_loop_run();

  /* will hopefully never happen */
  printf("[WIFI] Exiting WiFi task - should never happen\r\n");
  vTaskDelete(nullptr);
}
