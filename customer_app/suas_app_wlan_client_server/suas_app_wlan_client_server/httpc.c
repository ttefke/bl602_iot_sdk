#include "wifi.h"
#if WIFI_MODE_PINECONE == WIFI_MODE_STA

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include <http_client.h>
#include <cJSON.h>

/* helper functions */
static void cb_httpc_result(void *arg, httpc_result_t httpc_result,
    [[gnu::unused]] u32_t rx_content_len, [[gnu::unused]] u32_t srv_res, [[gnu::unused]] err_t err)
{
    httpc_state_t **req = (httpc_state_t **)arg;

    if (httpc_result == HTTPC_RESULT_OK)
    {
        printf("[HTTPC] Data transfer finished successfully\r\n");
    }
    else
    {
        printf("[HTTPC] Data transfer failed.\r\n");
    }

    *req = NULL;
}

static err_t cb_httpc_headers_done_fn([[gnu::unused]] httpc_state_t *connection,
        [[gnu::unused]] void *arg, [[gnu::unused]] struct pbuf *hdr,
        [[gnu::unused]] u16_t hdr_len, [[gnu::unused]] u32_t content_len)
{
    /* headers received, could be processed here */
    return ERR_OK;
}

static err_t cb_altcp_recv_fn([[gnu::unused]] void *arg,
    struct altcp_pcb *conn, struct pbuf *p, [[gnu::unused]] err_t err)
{
    /* Receive response
    1 packet only. Must be modified for receiving multiple packets. 
    See lwip/src/include/lwip/pbuf.h*/
    char *response = (char*) calloc(p->tot_len, sizeof(char));
    strncpy(response, p->payload, p->tot_len);

    cJSON *parsed_response = cJSON_Parse(response);
    char *encoded_response = cJSON_PrintUnformatted(parsed_response);
    printf("%s\r\n", encoded_response);

    free(encoded_response);
    encoded_response = NULL;

    cJSON *ping_response = cJSON_GetObjectItem(parsed_response, "response");
    printf("Response to /ping: %s\r\n", ping_response->valuestring);

    cJSON *response_time = cJSON_GetObjectItem(parsed_response, "time");
    printf("Reponse sent at %s\r\n", response_time->valuestring);
    
    cJSON_Delete(parsed_response);
    altcp_recved(conn, p->tot_len);
    pbuf_free(p);
    free(response);

    return ERR_OK;
}

void send_http_request(void)
{
  static httpc_connection_t connection;
  memset(&connection, 0, sizeof(connection));
  connection.use_proxy = 0;
  connection.result_fn = cb_httpc_result;
  connection.headers_done_fn = cb_httpc_headers_done_fn;
        
  static httpc_state_t *request;
      
  printf("Creating HTTPS request\r\n");
  httpc_get_file_dns(
      "ping.iotdev.tobias.tefke.name",
      80,
      "/ping",
      &connection,
      cb_altcp_recv_fn,
      &request,
      &request);
}

/* http task */
void task_http([[gnu::unused]] void *pvParameters)
{
    while (1)
    {
        send_http_request();
        vTaskDelay(5000);
    }
    
    printf("Deleting task - should not happen\r\n");
    vTaskDelete(NULL);
}
#endif
