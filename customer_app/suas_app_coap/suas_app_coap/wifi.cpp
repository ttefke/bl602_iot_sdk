extern "C" {
// StdLib includes
#include <stdbool.h>
#include <stdio.h>

// Hardware abstraction layers
#include <aos/yloop.h>
#include <bl_wifi.h>
#include <blog.h>
#include <easyflash.h>
#include <event_device.h>
#include <hal_board.h>
#include <hal_button.h>
#include <hal_gpio.h>
#include <hal_sys.h>
#include <hal_uart.h>
#include <hal_wifi.h>
#include <libfdt.h>
#include <looprt.h>
#include <loopset.h>
#include <vfs.h>
#include <wifi_mgmr_ext.h>

// Our headers
#include "include/coap-client.h"
#include "include/coap-server.h"
#include "include/wifi.h"
}

#include <etl/string.h>

// Role enum
constinit static app_wifi_role app_role{UNINITIALIZED};

/* WiFi configuration */
static auto is_wifi_initialized = false;
constinit static wifi_conf_t conf{
    .country_code = "EU",
    .channel_nums = {},
};

// Boolean that indicates whether to send requests (see client code)
bool sendRequests = true;

// CoAP task definitions (client or server)
constexpr uint16_t COAP_STACK_SIZE = 1024;
constinit static StackType_t coap_stack[COAP_STACK_SIZE]{};
constinit static StaticTask_t coap_task{};

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

/* Start a WiFi access point */
static void _start_ap_wifi(void) {
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
}

/* Connect to a WiFi access point */
static void _connect_sta_wifi() {
  // Enable station mode
  auto wifi_interface = wifi_mgmr_sta_enable();

  // Connect to access point
  wifi_mgmr_sta_connect(&wifi_interface, etl::string<32>(WIFI_SSID).data(),
                        etl::string<32>(WIFI_PW).data(), nullptr, 0, 0, 0);
  printf("[WIFI] Connected to a network\r\n");

  /* Enable automatic reconnect */
  wifi_mgmr_sta_autoconnect_enable();
}

/* React to WiFi events */
static void event_cb_wifi_event(input_event_t *event,
                                [[gnu::unused]] void *private_data) {
  switch (event->code) {
    case CODE_WIFI_ON_INIT_DONE:
      _configure_wifi();
      break;
    case CODE_WIFI_ON_MGMR_DONE:
      printf("[%s] WiFi MGMR done\r\n", __func__);
      if (app_role == STA) {
        /* Connect to a Wifi network*/
        printf("[%s] Connecting to a network\r\n", __func__);
        _connect_sta_wifi();
      } else {
        /* Start an access point */
        _start_ap_wifi();

        // Start CoAP server
        xTaskCreateStatic(task_coap_server,
                          etl::string_view("coap server").data(),
                          COAP_STACK_SIZE, nullptr, 15, coap_stack, &coap_task);
      }
      break;
    case CODE_WIFI_ON_SCAN_DONE:
      wifi_mgmr_cli_scanlist();
      break;
    case CODE_WIFI_ON_EMERGENCY_MAC:
      hal_reboot();
      break;
    case CODE_WIFI_ON_PROV_CONNECT:
      if (app_role == STA) {
        _connect_sta_wifi();
      }
      break;
    case CODE_WIFI_ON_CONNECTING:
      printf("[%s] Connecting to a WiFi network\r\n", __func__);
      break;
    case CODE_WIFI_ON_CONNECTED:
      printf("[%s] Connected to a network\r\n", __func__);
      break;
    case CODE_WIFI_ON_GOT_IP:
      printf("[%s] Received an IP address\r\n", __func__);
      if (app_role == STA) {
        // Start CoAP client
        xTaskCreateStatic(task_coap_client,
                          etl::string_view("coap client").data(),
                          COAP_STACK_SIZE, nullptr, 15, coap_stack, &coap_task);
      }
      break;
    case CODE_WIFI_ON_AP_STA_ADD:
      printf("[%s] New device present\r\n", __func__);
      break;
    case CODE_WIFI_ON_AP_STA_DEL:
      printf("[%s] Device was removed\r\n", __func__);
      break;
    case CODE_WIFI_ON_PROV_SSID:
    case CODE_WIFI_ON_PROV_BSSID:
    case CODE_WIFI_ON_PROV_PASSWD:
    case CODE_WIFI_ON_SCAN_DONE_ONJOIN:
    case CODE_WIFI_ON_MGMR_DENOISE:
    case CODE_WIFI_ON_DISCONNECT:
    case CODE_WIFI_CMD_RECONNECT:
    case CODE_WIFI_ON_PRE_GOT_IP:
    case CODE_WIFI_ON_PROV_DISCONNECT:
      // nothing
      break;
    default:
      printf("[%s] Unknown event\r\n", __func__);
  }
}

/* Listener for key events */
void event_cb_key_event(input_event_t *event,
                        [[gnu::unused]] void *private_data) {
  switch (event->code) {
    // Start as AP
    case KEY_1:
      if (app_role == UNINITIALIZED) {
        printf("[%s] Starting as AP\r\n", __func__);
        app_role = AP;
      } else if (app_role == STA) {
        sendRequests = false;
      }
      break;
    case KEY_2:
      // Start as station
      printf("[%s] Starting in STA mode\r\n", __func__);
      app_role = STA;
      break;
    default:
      printf("[%s] Key press not recognized\r\n", __func__);
  }

  /* Start wifi firmware if it is not started yet */
  if (!is_wifi_initialized) {
    printf("[%s] Starting WiFi stack\r\n", __func__);
    hal_wifi_start_firmware_task();
    aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
    printf("[%s] Firmware loaded\r\n", __func__);
    is_wifi_initialized = true;
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
    fdt_button_module_init(reinterpret_cast<const void *>(fdt),
                           static_cast<int>(offset));
  }

#ifdef CONF_USER_ENABLE_VFS_ROMFS
  romfs_register();
#endif

  /* Initialize aos loop */
  aos_loop_init();

  /* Register event filter for key events */
  aos_register_event_filter(EV_KEY, event_cb_key_event, nullptr);

  /* Register event filter for WiFi events*/
  aos_register_event_filter(EV_WIFI, event_cb_wifi_event, nullptr);

  printf("[%s] Task ready, press key to start AP or station\r\n", __func__);

  /*  Start aos loop */
  aos_loop_run();

  /* Will hopefully never happen */
  printf("[%s] Exiting WiFi task - should never happen\r\n", __func__);
  vTaskDelete(nullptr);
}