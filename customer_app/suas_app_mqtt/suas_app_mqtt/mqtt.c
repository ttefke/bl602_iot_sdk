#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <bl_sec.h>

#include <lwip/ip_addr.h>
#include <lwip/ip4_addr.h>
#include <lwip/apps/mqtt.h>
#include <lwip/apps/mqtt_opts.h>
#include <lwip/apps/mqtt_priv.h>

#include "mqtt.h"

#if ENABLE_MQTTS == TRUE
#include <lwip/altcp_tls.h>
extern const u8_t ca_certificate[];
extern const unsigned int ca_certificate_len;
extern const u8_t private_key[];
extern const unsigned int private_key_len;
extern const u8_t certificate[];
extern const unsigned int certificate_len;
#endif

// Data structures to hold state data
mqtt_client_t mqtt_client;
ip_addr_t mqtt_broker_ip;
static uint8_t topic_nr;
bool mqtt_connected = false;
const char subscribed_topic[] = "test";

// Disconnect from the network
void my_mqtt_disconnect() {
    // 1. Unsubscribe
    mqtt_unsubscribe(&mqtt_client, subscribed_topic, NULL, 0);
 
    // 2. Disconnect
    mqtt_disconnect(&mqtt_client);
    mqtt_connected = false;

    printf("[%s] Done\r\n", __func__);
}

// Callback for published message:
// Check if message was published successfully
void my_mqtt_publish_cb(void *arg, err_t result) {
    if (result == ERR_OK) {
        printf("[%s] Published message\r\n", __func__);
    } else {
        printf("[%s] Could not publish message: %d\r\n",
            __func__, result);
    }
}

// Publish a message
void my_mqtt_publish() {
    // Check if we are connected
    if (!mqtt_connected) {
        printf("[%s] Not connected to a broker, connect first\r\n", __func__);
    } else {
        const char payload[] = "test message";
        err_t err;
        u8_t retain = 0; // Set retain field (0 or 1)
        err = mqtt_publish(&mqtt_client, subscribed_topic, payload,
            strlen(payload), QUALITY_OF_SERVICE, retain, my_mqtt_publish_cb, 0);

        if (err != ERR_OK) {
            printf("[%s] Could not publish message: %d\r\n",
                __func__, err);
        }
    }
}

// Incoming topic callback
void my_mqtt_incoming_topic_cb(void *arg, const char *topic, u32_t total_len) {
    printf("[%s] Incoming message, topic: %s, total length: %u\r\n",
        __func__, topic, (unsigned int) total_len);
    
    /* Demultiplex topic (assign number for internal processing) */
    if (strcmp(subscribed_topic, topic) == 0) {
        topic_nr = 0;
    } else {
        // We received a message for a topic we did not subscribe to
        topic_nr = 1;
    }
}

// Incoming payload callback
void my_mqtt_incoming_payload_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    printf("[%s] Incoming payload, length: %d, flags: %u\r\n",
        __func__, len, (unsigned int) flags);

    if (flags & MQTT_DATA_FLAG_LAST) {
        // Only consider payloads that fit into one buffer
        // Check previously assigned topic number
        if (topic_nr == 0) {
            printf("[%s] Incoming data on topic \"%s\": \"",
                __func__, subscribed_topic);
            for (u16_t i = 0; i < len; i++) {
                printf("%c", data[i]);
            }
            printf("\"\r\n");
        } else {
            printf("[%s] Received data for topic we did not subscribe to\r\n",
                __func__);
        }
    } else {
        printf("[%s] Fragmented payloads are currently unsupported\r\n", __func__);
    }
}

// Subscription request result callback
void my_mqtt_sub_request_cb(void *arg, err_t result) {
    if (result == ERR_OK) {
        printf("[%s] Subscribed\r\n", __func__);
    } else {
        printf("[%s] Could not subscribe to topic: %d\r\n",
            __func__, result);
    }
}

// Connection establishment status callback
void my_mqtt_connected_cb(mqtt_client_t *client, void *arg,
    mqtt_connection_status_t status) {
    // Connection accepted
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("[%s] Connected\r\n", __func__);
        mqtt_connected = true;

        // Register callback for incoming messages
        mqtt_set_inpub_callback(&mqtt_client, my_mqtt_incoming_topic_cb,
                my_mqtt_incoming_payload_cb, arg);
        
        // Subscribe to topic subscribed_topic (variable!)
        err_t err = mqtt_subscribe(&mqtt_client, subscribed_topic, QUALITY_OF_SERVICE,
                my_mqtt_sub_request_cb, arg);

        // Evaluate subscription result
        if (err != ERR_OK) {
            printf("[%s] Could not subscribe to topic: %d",
                __func__, err);
        }
    // Disconnected or timeout
    } else if ((status == MQTT_CONNECT_DISCONNECTED) ||
        (status == MQTT_CONNECT_TIMEOUT)) {
        printf("[%s] Disconnected, attempting to reconnect\r\n",
            __func__);
        my_mqtt_connect();
    // Could not establish a connection
    } else {
        printf("[%s] Could not connect, reason: %d\r\n",
            __func__, status);
    }
}

// Connect to MQTT broker
void my_mqtt_connect() {
    // Connection variables
    struct mqtt_connect_client_info_t client_info;
    err_t err;

    // Set IP address
    IP_ADDR4(&mqtt_broker_ip, 192, 168, 1, 1);

    // Generate random client id
    int random_id = bl_rand();
    char client_id[sizeof(int) * 8 + 1];
    itoa(random_id, client_id, 16);
    printf("[%s] Generated random client id: \"%s\"\r\n",
        __func__, client_id);

    // Setup client
    memset(&client_info, 0, sizeof(client_info));
    client_info.client_id = client_id;
    client_info.client_user = MQTT_USER;
    client_info.client_pass = MQTT_PW;
    client_info.keep_alive = 60; // Keep-alive time in seconds

#if ENABLE_MQTTS == TRUE
    client_info.tls_config = altcp_tls_create_config_client_2wayauth(
        ca_certificate,
        ca_certificate_len,
        private_key,
        private_key_len,
        NULL,
        0,
        certificate,
        certificate_len
    );

    // Initiate connection (TLS)
    err = mqtt_client_connect(&mqtt_client, &mqtt_broker_ip,
        MQTT_TLS_PORT, my_mqtt_connected_cb, 0, &client_info);
#else
    // Initiate connection (plain text)
    err = mqtt_client_connect(&mqtt_client, &mqtt_broker_ip,
        MQTT_PORT, my_mqtt_connected_cb, 0, &client_info);
#endif        
    
    // Check for connection errors
    if (err != ERR_OK) {
        printf("[%s] Could not connect: %d\r\n", __func__, err);
    }
}