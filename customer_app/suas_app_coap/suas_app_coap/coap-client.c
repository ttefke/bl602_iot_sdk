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
#include "include/coap-client.h"
#include "include/coap-log.h"

// Local message processing variables
static coap_context_t *main_coap_context;
static coap_session_t *session = NULL;
static coap_uri_t uri;
static coap_address_t dst;
static bool firstPDU;
static bool lastPDU;
bool sendRequests = true;

// Handle incoming messages
coap_response_t message_handler([[gnu::unused]] coap_session_t *session,
  [[gnu::unused]] const coap_pdu_t *sent, const coap_pdu_t *received,
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
      printf("[%s] Received data: %*.*s", __func__,
              (int) len, (int) len, (const char *) data);
      firstPDU = false;
    } else {
      printf("%s", (const char *) data);
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
void nack_handler(coap_session_t * session COAP_UNUSED,
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
int resolve_address(const char *host, const char *service,
  coap_address_t *dst, coap_proto_t *proto, int scheme) {
  // 0. Variables
  coap_addr_info_t *addr_info;
  int ret = 0;

  // 1. Get port
  uint16_t port = service ? atoi(service): 0;

  // 2. Define hostname
  coap_str_const_t str_host =
    { strlen(host), (const uint8_t *) host };

  // 3. Get address information for the host
  addr_info = coap_resolve_address_info(&str_host,
    port, port, port, port, AF_UNSPEC, scheme,
    COAP_RESOLVE_TYPE_REMOTE);

  // 4. Set the variables we require (IP address + protocol)
  if (addr_info) {
    ret = 1;
    *dst = addr_info->addr;
    *proto = addr_info->proto;
  }

  // 5. Free address information container and return
  coap_free_address_info(addr_info);
  return ret;
}

// Initialize session
void client_coap_init() {
  // 0. Variables
  coap_proto_t proto;
  const char *uri_const = COAP_URI;
  char portbuf[8];
  unsigned char buf[100];
  int len;

  // 1. Initialize CoAP stack
  coap_startup();

  // 2. Set logging
  coap_set_log_handler(coap_log_handler);
  coap_set_log_level(COAP_LOG_INFO);

  // 3. Parse URI
  len = coap_split_uri((const unsigned char *) uri_const, strlen(uri_const), &uri);
  LWIP_ASSERT("Failed to parse URI", len == 0);

  snprintf(portbuf, sizeof(portbuf), "%d", uri.port);
  snprintf((char *) buf, sizeof(buf), "%*.*s", (int) uri.host.length,
    (int) uri.host.length, (const char *) uri.host.s);

  len = resolve_address((const char*) buf, portbuf, &dst, &proto, 1 << uri.scheme);
  LWIP_ASSERT("Failed to resolve address", len > 0);

  // 4. Create CoAP context
  main_coap_context = coap_new_context(NULL);
  LWIP_ASSERT("Failed to initialize context", main_coap_context != NULL);

  // 5. Set block mode
  coap_context_set_block_mode(main_coap_context, COAP_BLOCK_USE_LIBCOAP);

  // 6. Create session
  session = coap_new_client_session(main_coap_context, NULL, &dst, proto);
  LWIP_ASSERT("Failed to create session", session != NULL);

  // 7. Register response handler
  coap_register_response_handler(main_coap_context, message_handler);

  // 8. Register non-acknowledged handler
  coap_register_nack_handler(main_coap_context, nack_handler);
}

// Send a PDU (request)
void client_coap_send_pdu() {
  /* 0. Check if the session is valid*/
  if (session == NULL) {
    printf("[%s] No session created yet!\r\n", __func__);
    return;
  }

  // 1. Construct CoAP PDU
  coap_pdu_t *pdu = coap_pdu_init(COAP_MESSAGE_CON,
    COAP_REQUEST_CODE_GET,
    coap_new_message_id(session),
    coap_session_max_pdu_size(session));
  LWIP_ASSERT("Failed to create PDU", pdu != NULL);

  // 2. Create list of options
  coap_optlist_t *optlist = NULL;
  int res = coap_uri_into_optlist(&uri, &dst, &optlist, 1);
  LWIP_ASSERT("Failed to create options", res == 1);

  // 3. Add option list to PDU
  if (optlist) {
    res = coap_add_optlist_pdu(pdu, &optlist);
    LWIP_ASSERT("Failed to add options to PDU", res == 1);
  }

  // 4. Send the PDU
  coap_mid_t mid = coap_send(session, pdu);
  LWIP_ASSERT("Failed to send PDU", mid != COAP_INVALID_MID);

  // 5. Cleanup
  coap_delete_optlist(optlist);
  optlist = NULL;
}

// CoAP client task
void task_coap_client([[gnu::unused]] void *pvParameters) {
  // Wait until WiFi is set up
  vTaskDelay(3 * 1000);

  // Initialize 
  client_coap_init();
  sendRequests = true;

  // Send request to server once a second
  while (sendRequests) {
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
    vTaskDelay(1000);
  }

  // Destruct CoAP data containers
  printf("[%s] Shutting down CoAP\r\n", __func__);
  coap_free_context(main_coap_context);
  main_coap_context = NULL;
  coap_cleanup();
  printf("[%s] Done\r\n", __func__);

  // Remove task
  vTaskDelete(NULL);
}