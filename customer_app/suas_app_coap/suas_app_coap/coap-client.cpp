extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// StdLib include
#include <stdbool.h>
#include <stdio.h>

// lwIP includes
#include <lwip/sockets.h>

// CoAP includes
#include <coap3/coap.h>
}
#include <etl/memory.h>
#include <etl/string.h>
#include <etl/to_string.h>

#include "include/coap-client.h"
#include "include/coap-common.h"
#include "include/coap-log.h"

// Local message processing variables
static coap_context_t *main_coap_context;
static coap_session_t *session = nullptr;
static coap_uri_t uri;
static coap_address_t dst;
static bool firstPDU;
static bool lastPDU;

// Deleters
struct AddressInfoDeleter {
  void operator()(coap_addr_info_t *ai) const {
    if (ai) {
      coap_free_address_info(ai);
    }
  }
};

struct OptlistDeleter {
  void operator()(coap_optlist_t *optlist) const {
    if (optlist) {
      coap_delete_optlist(optlist);
    }
  }
};

// Handle incoming messages
coap_response_t message_handler([[gnu::unused]] coap_session_t *session,
                                [[gnu::unused]] const coap_pdu_t *sent,
                                const coap_pdu_t *received,
                                [[gnu::unused]] const coap_mid_t id) {
  // 0. Variables
  const uint8_t *data;
  size_t len;
  size_t offset;
  size_t total;

  // 1. Receive PDU
  if (coap_get_data_large(received, &len, &data, &offset, &total)) {
    // 2. Print payload
    if (firstPDU) {
      printf("[%s] Received data: %*.*s", __func__, static_cast<int>(len),
             static_cast<int>(len), reinterpret_cast<const char *>(data));
      firstPDU = false;
    } else {
      printf("%s", reinterpret_cast<const char *>(data));
    }

    // Check if this is the last PDU of the data frame
    if (len + offset == total) {
      printf("\r\n");
      lastPDU = true;
    }
  }

  // 3. Return success
  return COAP_RESPONSE_OK;
}

// Handle errors (not acknowledged messages)
void nack_handler(coap_session_t *session COAP_UNUSED,
                  const coap_pdu_t *sent COAP_UNUSED,
                  const coap_nack_reason_t reason,
                  const coap_mid_t id COAP_UNUSED) {
  // Get reason and output it
  switch (reason) {
    case COAP_NACK_RST:
      printf("[%s] Connection reset\r\n", __func__);
      break;
    case COAP_NACK_TLS_FAILED:
      printf("[%s] DTLS failed\r\n", __func__);
      break;
    case COAP_NACK_TOO_MANY_RETRIES:
    case COAP_NACK_NOT_DELIVERABLE:
    case COAP_NACK_ICMP_ISSUE:
    default:
      printf("[%s] Can not send CoAP PDU\r\n", __func__);
  }
}

// Resolve address
int resolve_address(etl::string_view host, etl::string_view service,
                    coap_address_t &dst, coap_proto_t &proto, int scheme) {
  // 1. Get port -> must be uint16_t
  uint16_t port = service.data() ? atoi(service.data()) : 0;

  // 2. Define hostname
  auto str_host = coap_str_const_t{
      host.length(), reinterpret_cast<const uint8_t *>(host.data())};

  // 3. Get address information for the host
  auto addr_info = etl::unique_ptr<coap_addr_info_t, AddressInfoDeleter>(
      coap_resolve_address_info(&str_host, port, port, port, port, AF_UNSPEC,
                                scheme, COAP_RESOLVE_TYPE_REMOTE));

  // 4. Set the variables we require (IP address + protocol)
  if (addr_info) {
    dst = addr_info->addr;
    proto = addr_info->proto;
    return 1;
  }

  return 0;
}

// Initialize session
void client_coap_init() {
  // 0. Variables
  coap_proto_t proto;
  auto coapURI = etl::string_view(COAP_URI);
  auto portBuffer = etl::string<8>();

  // 1. Initialize CoAP stack
  coap_startup();

  // 2. Set logging
  coap_set_log_handler(coap_log_handler);
  coap_set_log_level(COAP_LOG_INFO);

#ifdef WITH_COAPS
  coap_dtls_set_log_level(COAP_LOG_WARN);
#endif

  // 3. Parse URI
  auto len = coap_split_uri(reinterpret_cast<const uint8_t *>(coapURI.data()),
                            coapURI.length(), &uri);
  LWIP_ASSERT("Failed to parse URI", len == 0);

  etl::to_string(uri.port, portBuffer);
  auto hostName = etl::string<32>(reinterpret_cast<const char *>(uri.host.s),
                                  uri.host.length);

  len = resolve_address(hostName, portBuffer, dst, proto, 1 << uri.scheme);
  LWIP_ASSERT("Failed to resolve address", len > 0);

  // 4. Create CoAP context
  main_coap_context = coap_new_context(nullptr);
  LWIP_ASSERT("Failed to initialize context", main_coap_context != nullptr);

  // 5. Set block mode
  coap_context_set_block_mode(main_coap_context, COAP_BLOCK_USE_LIBCOAP);

  // 6. Create session
#ifdef WITH_COAPS
  if (proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS) {
    auto coapsID = etl::string_view(COAPS_ID);
    auto coapsPSK = etl::string_view(COAPS_PSK);

    coap_dtls_cpsk_t client_coaps_data{};
    client_coaps_data.version = COAP_DTLS_CPSK_SETUP_VERSION;
    client_coaps_data.client_sni = hostName.data();
    client_coaps_data.psk_info.identity.s =
        reinterpret_cast<const uint8_t *>(coapsID.data());
    client_coaps_data.psk_info.identity.length = coapsID.length();
    client_coaps_data.psk_info.key.s =
        reinterpret_cast<const uint8_t *>(coapsPSK.data());
    client_coaps_data.psk_info.key.length = coapsPSK.length();

    session = coap_new_client_session_psk2(main_coap_context, nullptr, &dst,
                                           proto, &client_coaps_data);
  }
#else
  session = coap_new_client_session(main_coap_context, nullptr, &dst, proto);
#endif
  LWIP_ASSERT("Failed to create session", session != nullptr);

  // 7. Register response handler
  coap_register_response_handler(main_coap_context, message_handler);

  // 8. Register non-acknowledged handler
  coap_register_nack_handler(main_coap_context, nack_handler);
}

// Send a PDU (request)
void client_coap_send_pdu() {
  /* 0. Check if the session is valid*/
  if (session == nullptr) {
    printf("[%s] No session created yet!\r\n", __func__);
    return;
  }

  // 1. Construct CoAP PDU
  auto pdu = coap_pdu_init(COAP_MESSAGE_CON, COAP_REQUEST_CODE_GET,
                           coap_new_message_id(session),
                           coap_session_max_pdu_size(session));
  LWIP_ASSERT("Failed to create PDU", pdu != nullptr);

  // 2. Create list of options
  coap_optlist_t *optlist_raw = nullptr;
  auto res = coap_uri_into_optlist(&uri, &dst, &optlist_raw, 1);

  // Optlist to smart pointer
  auto optlist = etl::unique_ptr<coap_optlist_t, OptlistDeleter>(optlist_raw);
  LWIP_ASSERT("Failed to create options", res == 1);

  // 3. Add option list to PDU
  if (optlist_raw) {
    res = coap_add_optlist_pdu(pdu, &optlist_raw);
    LWIP_ASSERT("Failed to add options to PDU", res == 1);
  }

  // 4. Send the PDU
  auto mid = coap_send(session, pdu);
  LWIP_ASSERT("Failed to send PDU", mid != COAP_INVALID_MID);
}

// CoAP client task
void task_coap_client([[gnu::unused]] void *pvParameters) {
  // Wait until WiFi is set up
  vTaskDelay(pdMS_TO_TICKS(3 * 1000));

  // Initialize
  client_coap_init();

  // Send request to server once a second
  while (1) {
    // Set message processing variables
    firstPDU = true;
    lastPDU = false;

    // Send request PDU
    client_coap_send_pdu();

    // Poll for incoming response data
    while (!lastPDU) {
      // Wait for new packet with timeout of 500ms
      coap_io_process(main_coap_context, 500);
    }
    // Wait one second before sending next request
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Destruct CoAP data containers
  printf("[%s] Shutting down CoAP\r\n", __func__);
  coap_free_context(main_coap_context);
  coap_cleanup();
  printf("[%s] Done\r\n", __func__);

  // Remove task
  vTaskDelete(nullptr);
}