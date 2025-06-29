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
#include "include/coap-log.h"
#include "include/coap-server.h"

// Local variable
static coap_context_t *main_coap_context;

// Resource handler: generate random number and return it
void handler_random(coap_resource_t *resource, coap_session_t  *session,
  const coap_pdu_t *request, const coap_string_t *query,
  coap_pdu_t *response) {
  // 0. Create variable: buffer to hold response data
  unsigned char buf[30];

  // 1. Set response code (success)
  coap_pdu_set_code(response, COAP_RESPONSE_CODE(205));

  // 2. Set response content format
  coap_add_option(response, COAP_OPTION_CONTENT_FORMAT,
    coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_TEXT_PLAIN), buf);

  // 3. Generate random number
  uint32_t random = bl_sec_get_random_word();

  // 4. Get string length of random number
  size_t len = snprintf((char *) buf, sizeof(buf), "%u", (unsigned int) random);

  // 5. Add random number to response
  coap_add_data(response, len, buf);

  printf("[%s] Sending response (time: %ld)\r\n", __func__, bl_timer_now_us());
}

// Initialize server
void server_coap_init() {
  // 1. Start CoAP stack
  coap_startup();

  // 2. Set logging
  coap_set_log_handler(coap_log_handler);
  coap_set_log_level(COAP_LOG_INFO);

  // 3. Create server context
  main_coap_context = coap_new_context(NULL);
  LWIP_ASSERT("Failed to initialize context", main_coap_context != NULL);

  // 4. Set keepalive time for sessions
  coap_context_set_keepalive(main_coap_context, 60);

  // 5. Set connection schemes
  uint32_t schemes =
    coap_get_available_scheme_hint_bits(
      /* Use PSK */ 0,
      /* Enable Websockets */ 0,
      /* IP sockets */ COAP_PROTO_NONE
  );

  // 6. Set IP address to listen to (listen on all interfaces)
  const uint8_t ip_address[] = "0.0.0.0";
  coap_str_const_t ip_address_info = { strlen((char*)ip_address), ip_address };

  // 7. Create server properties container
  coap_addr_info_t *info_list = coap_resolve_address_info (
    /* Address to resolve */ &ip_address_info,
    /* CoAP port */ 5683,
    /* CoAPs port */ 0,
    /* WS port */ 0,
    /* WSS port */ 0,
    /* Additional IP address flags */ 0,
    /* Supported URI schemes */ schemes,
    /* Connection type */ COAP_RESOLVE_TYPE_REMOTE
  );

  // 8. Register CoAP endpoint on all interfaces
  for (coap_addr_info_t *info = info_list; info != NULL; info = info->next) {
    coap_endpoint_t *ep = coap_new_endpoint(main_coap_context, &info->addr, info->proto);
    LWIP_ASSERT("Failed to initiialize context", ep != NULL);
  }

  // 9. Deallocate server properties container
  coap_free_address_info(info_list);

  //10. Limit number of idle connections (this is strictly required!)
#if MEMP_NUM_COAPSESSION < 2
#error Insufficient amount of possible sessions, need at least two!
#else
  coap_context_set_max_idle_sessions(main_coap_context, MEMP_NUM_COAPSESSION -1);
#endif

  // 11. Initialize resource
  coap_resource_t *r = coap_resource_init(coap_make_str_const("random"), 0);

  // Check if resource could be created
  if (!r) {
    goto error;
  }
  
  // 12. Register handler
  coap_register_handler(r, COAP_REQUEST_GET, handler_random);

  // 13. Add resource to server context
  coap_add_resource(main_coap_context, r);
  return;

error:
  printf("[%s] Cannot create resource\r\n", __func__);
}

// CoAP server task
void task_coap_server(void *pvParameters) {
  // Wait until WiFi is set up
  vTaskDelay(3 * 1000);

  // Initialize CoAP server
  server_coap_init();
  printf("[%s] CoAP server initialized\r\n", __func__);

  // Keep task alive
  while (1) {
    vTaskDelay(60 * 1000);
  }

  /* Loop ended - terminate server process - clean up */
  // 1. Release context
  coap_free_context(main_coap_context);
  main_coap_context = NULL;

  // 2. Cleanup CoAP stack
  coap_cleanup();
  printf("[%s] CoAP server terminated\r\n", __func__);

  // 3. Delete task
  vTaskDelete(NULL);
}