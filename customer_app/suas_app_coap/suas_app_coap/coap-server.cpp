extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// HAL for TRNG and timer
#include <bl_sec.h>
#include <bl_timer.h>

// StdLib include
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// CoAP includes
#include <coap3/coap.h>
}
#include <etl/string.h>
#include <etl/to_string.h>

#include "include/coap-common.h"
#include "include/coap-log.h"
#include "include/coap-server.h"

// Local variable to store coap state information
static coap_context_t *main_coap_context;

// Deleter
struct AddressInfoDeleter {
  void operator()(coap_addr_info_t *info) const {
    if (info) {
      coap_free_address_info(info);
    }
  }
};

// Resource handler: generate random number and return it
void handler_random([[gnu::unused]] coap_resource_t *resource,
                    [[gnu::unused]] coap_session_t *session,
                    [[gnu::unused]] const coap_pdu_t *request,
                    [[gnu::unused]] const coap_string_t *query,
                    coap_pdu_t *response) {
  // 0. Create variable: buffer to hold response data
  auto buf = etl::string<30>();

  // 1. Set response code (success)
  coap_pdu_set_code(response,
                    static_cast<coap_pdu_code_t>(COAP_RESPONSE_CODE(205)));

  // 2. Set response content format
  coap_add_option(
      response, COAP_OPTION_CONTENT_FORMAT,
      coap_encode_var_safe(reinterpret_cast<uint8_t *>(buf.data()),
                           buf.max_size(), COAP_MEDIATYPE_TEXT_PLAIN),
      reinterpret_cast<uint8_t *>(buf.data()));

  // 3. Generate random number
  auto random = bl_sec_get_random_word();

  // 4. Get string of random number
  etl::to_string(random, buf);

  // 5. Add random number to response
  coap_add_data(response, buf.size(), reinterpret_cast<uint8_t *>(buf.data()));

  printf("[%s] Sending response (value: %" PRIu32 ")\r\n", __func__, random);
}

// Initialize server
void server_coap_init() {
  // 1. Start CoAP stack
  coap_startup();

  // 2. Set logging
  coap_set_log_handler(coap_log_handler);
  coap_set_log_level(COAP_LOG_INFO);

#ifdef WITH_COAPS
  coap_dtls_set_log_level(COAP_LOG_WARN);
#endif

  // 3. Create server context
  main_coap_context = coap_new_context(nullptr);
  LWIP_ASSERT("Failed to initialize context", main_coap_context != nullptr);

#ifdef WITH_COAPS
  auto coapsPSK = etl::string_view(COAPS_PSK);
  coap_dtls_spsk_t server_coaps_data{};

  server_coaps_data.version = COAP_DTLS_SPSK_SETUP_VERSION;
  server_coaps_data.psk_info.key.s =
      reinterpret_cast<const uint8_t *>(coapsPSK.data());
  server_coaps_data.psk_info.key.length = coapsPSK.length();
  coap_context_set_psk2(main_coap_context, &server_coaps_data);
#endif

  // 4. Set keepalive time for sessions
  coap_context_set_keepalive(main_coap_context, 60);

  // 5. Set connection schemes
  auto schemes = coap_get_available_scheme_hint_bits(
#ifdef WITH_COAPS
      /* Use PSK */ 1,
#else
      /* Use PSK */ 0,
#endif
      /* Enable Websockets */ 0,
      /* IP sockets */ COAP_PROTO_NONE);

  // 6. Set IP address to listen to (listen on all interfaces)
  auto ipAddress = etl::string_view("0.0.0.0");
  coap_str_const_t ipAddressInfo = {
      ipAddress.length(), reinterpret_cast<const uint8_t *>(ipAddress.data())};

  // 7. Create server properties container
  auto infoList = etl::unique_ptr<coap_addr_info_t, AddressInfoDeleter>(
      coap_resolve_address_info(
          /* Address to resolve */ &ipAddressInfo,
          /* CoAP port */ 5683,
#ifdef WITH_COAPS
          /* CoAPs port */ 5684,
#else
          /* CoAPs port */ 0,
#endif
          /* WS port */ 0,
          /* WSS port */ 0,
          /* Additional IP address flags */ 0,
          /* Supported URI schemes */ schemes,
          /* Connection type */ COAP_RESOLVE_TYPE_REMOTE));

  // 8. Register CoAP endpoint on all interfaces
  for (auto info = infoList.get(); info != nullptr; info = info->next) {
    auto ep = coap_new_endpoint(main_coap_context, &info->addr, info->proto);
    LWIP_ASSERT("Failed to initiialize context", ep != nullptr);
  }

  // 9. Limit number of idle connections (this is strictly required!)
#if MEMP_NUM_COAPSESSION < 2
#error Insufficient amount of possible sessions, need at least two!
#else
  coap_context_set_max_idle_sessions(main_coap_context,
                                     MEMP_NUM_COAPSESSION - 1);
#endif

  // 10. Initialize resource
  auto resource = coap_resource_init(coap_make_str_const("random"), 0);

  // Check if resource could be created
  if (!resource) {
    goto error;
  }

  // 11. Register handler
  coap_register_handler(resource, COAP_REQUEST_GET, handler_random);

  // 12. Add resource to server context
  coap_add_resource(main_coap_context, resource);
  return;

error:
  printf("[%s] Cannot create resource\r\n", __func__);
}

// CoAP server task
extern "C" void task_coap_server([[gnu::unused]] void *pvParameters) {
  // Wait until WiFi is set up
  vTaskDelay(pdMS_TO_TICKS(3 * 1000));

  // Initialize CoAP server
  server_coap_init();
  printf("[%s] CoAP server initialized\r\n", __func__);

  // Keep task alive
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(60 * 1000));
  }

  /* Loop ended - terminate server process - clean up */
  // 1. Release context
  coap_free_context(main_coap_context);

  // 2. Cleanup CoAP stack
  coap_cleanup();
  printf("[%s] CoAP server terminated\r\n", __func__);

  // 3. Delete task
  vTaskDelete(nullptr);
}