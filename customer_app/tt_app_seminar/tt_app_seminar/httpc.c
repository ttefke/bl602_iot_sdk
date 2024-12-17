#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <http_client.h>
#include <wifi_mgmr_ext.h>
#include <cJSON.h>

#include "conf.h"

/* helper function: callback function for the result of the HTTP request */
static void cb_httpc_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
    /* evaluate data transfer result */
    httpc_state_t **req = (httpc_state_t **)arg;

    if (httpc_result == HTTPC_RESULT_OK) {
        printf("[HTTPC] Data transfer finished successfully\r\n");
    } else {
        printf("[HTTPC] Data transfer failed.\r\n");
    }

    *req = NULL;
}

/* helper function: HTTP headers are received*/
static err_t cb_httpc_headers_done_fn(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
    /* headers are ignored */
    return ERR_OK;
}

/* helper function: received the data from the queried URL*/
static err_t cb_altcp_recv_fn(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err)
{
    /* we receive nothing -> just delete data structures */
    altcp_recved(conn, p->tot_len);
    pbuf_free(p);

    return ERR_OK;
}

/* send HTTP request */
void send_http_request(unsigned char data[])
{
    /* create connection data structure*/
    static httpc_connection_t connection;
    memset(&connection, 0, sizeof(connection));

    /* configure connection */
    connection.use_proxy = 0;
    connection.req_type = REQ_TYPE_POST;
    connection.data = data;
    connection.content_type = CONTENT_TYPE_JSON;
    connection.result_fn = cb_httpc_result;
    connection.headers_done_fn = cb_httpc_headers_done_fn;

    /* create and send request */       
    static httpc_state_t *request;
    printf("[HTTPC] Sending HTTP request\r\n");
    httpc_get_file_dns(
        GATEWAY_IP_ADDRESS,
        GATEWAY_PORT,
        GATEWAY_ROUTE_LUX,
        &connection,
        cb_altcp_recv_fn,
        &request,
        &request);
}

/* http task */
void task_http(void *pvParameters)
{
    /* variables to identify measurements */
    unsigned long message_id = 1;
    extern volatile unsigned long lux;

    /* delay sending data until WiFi is available */
    vTaskDelay(35 * 1000 / portTICK_PERIOD_MS);

    while (1)
    {
        /* create data structure to store data to be sent */
        cJSON *request_data = cJSON_CreateObject();
        cJSON_AddNumberToObject(request_data, "device_id", DEVICE_ID);
        cJSON_AddNumberToObject(request_data, "message_id", message_id);
        cJSON_AddNumberToObject(request_data, "lux", lux);

        /* send data */
        unsigned char* json_data = (unsigned char *) cJSON_PrintUnformatted(request_data);
        send_http_request(json_data);

        /* free memory, increase message id and wait for one second */
        cJSON_Delete(request_data);
        free(json_data);
        message_id++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    /* while loop should never exit */
    printf("Deleting task - should not happen\r\n");
    vTaskDelete(NULL);
}