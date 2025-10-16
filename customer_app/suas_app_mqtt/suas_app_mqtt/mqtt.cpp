extern "C" {
#include <bl_sec.h>
#include <lwip/apps/mqtt.h>
#include <lwip/apps/mqtt_opts.h>
#include <lwip/apps/mqtt_priv.h>
#include <lwip/ip4_addr.h>
#include <lwip/ip_addr.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
}
#include <etl/string.h>
#include <etl/to_string.h>

#include "mqtt.h"

#if ENABLE_MQTTS == TRUE
extern "C" {
#include <lwip/altcp_tls.h>
}
#include "keys.hpp"
#endif

// Data structures to hold state data
mqtt_client_t mqttClient;
ip_addr_t mqttBrokerIp;
static uint8_t topicNr;
auto mqttConnected = false;
auto subscribedTopic = etl::string_view("test");

// Disconnect from the network
void my_mqtt_disconnect() {
  // 1. Unsubscribe
  mqtt_unsubscribe(&mqttClient, subscribedTopic.data(), nullptr, 0);

  // 2. Disconnect
  mqtt_disconnect(&mqttClient);
  mqttConnected = false;

  printf("[%s] Done\r\n", __func__);
}

// Callback for published message:
// Check if message was published successfully
void my_mqtt_publish_cb([[gnu::unused]] void *arg, err_t result) {
  if (result == ERR_OK) {
    printf("[%s] Published message\r\n", __func__);
  } else {
    printf("[%s] Could not publish message: %d\r\n", __func__, result);
  }
}

// Publish a message
void my_mqtt_publish() {
  // Check if we are connected
  if (!mqttConnected) {
    printf("[%s] Not connected to a broker, connect first\r\n", __func__);
  } else {
    printf("[%s] Connected to a broker, publishing message\r\n", __func__);

    auto payload = etl::string_view("test message");
    auto retain = 0;  // Set retain field (0 or 1)
    auto err = mqtt_publish(&mqttClient, subscribedTopic.data(), payload.data(),
                            payload.length(), QUALITY_OF_SERVICE, retain,
                            my_mqtt_publish_cb, 0);

    if (err != ERR_OK) {
      printf("[%s] Could not publish message: %d\r\n", __func__, err);
    }
  }
}

// Incoming topic callback
void my_mqtt_incoming_topic_cb([[gnu::unused]] void *arg, const char *topic,
                               u32_t total_len) {
  auto messageTopic = etl::string_view(topic);
  printf("[%s] Incoming message, topic: %s, total length: %u\r\n", __func__,
         messageTopic.data(), static_cast<unsigned int>(total_len));

  /* Demultiplex topic (assign number for internal processing) */
  if (messageTopic == subscribedTopic) {
    topicNr = 0;
  } else {
    // We received a message for a topic we did not subscribe to
    topicNr = 1;
  }
}

// Incoming payload callback
void my_mqtt_incoming_payload_cb([[gnu::unused]] void *arg, const u8_t *data,
                                 u16_t len, u8_t flags) {
  printf("[%s] Incoming payload, length: %d, flags: %u\r\n", __func__, len,
         static_cast<unsigned int>(flags));

  if (flags & MQTT_DATA_FLAG_LAST) {
    // Only consider payloads that fit into one buffer
    // Check previously assigned topic number
    if (topicNr == 0) {
      printf("[%s] Incoming data on topic \"%s\": \"", __func__,
             subscribedTopic.data());
      for (auto i = 0; i < len; i++) {
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
void my_mqtt_sub_request_cb([[gnu::unused]] void *arg, err_t result) {
  if (result == ERR_OK) {
    printf("[%s] Subscribed\r\n", __func__);
  } else {
    printf("[%s] Could not subscribe to topic: %d\r\n", __func__, result);
  }
}

// Connection establishment status callback
void my_mqtt_connected_cb([[gnu::unused]] mqtt_client_t *client, void *arg,
                          mqtt_connection_status_t status) {
  // Connection accepted
  if (status == MQTT_CONNECT_ACCEPTED) {
    printf("[%s] Connected\r\n", __func__);
    mqttConnected = true;

    // Register callback for incoming messages
    mqtt_set_inpub_callback(&mqttClient, my_mqtt_incoming_topic_cb,
                            my_mqtt_incoming_payload_cb, arg);

    // Subscribe to topic subscribedTopic (variable!)
    auto err = mqtt_subscribe(&mqttClient, subscribedTopic.data(),
                              QUALITY_OF_SERVICE, my_mqtt_sub_request_cb, arg);

    // Evaluate subscription result
    if (err != ERR_OK) {
      printf("[%s] Could not subscribe to topic: %d", __func__, err);
    }
    // Disconnected or timeout
  } else if ((status == MQTT_CONNECT_DISCONNECTED) ||
             (status == MQTT_CONNECT_TIMEOUT)) {
    printf("[%s] Disconnected, attempting to reconnect\r\n", __func__);
    my_mqtt_connect();
    // Could not establish a connection
  } else {
    printf("[%s] Could not connect, reason: %d\r\n", __func__, status);
  }
}

// Connect to MQTT broker
void my_mqtt_connect() {
  // Connection variables
  struct mqtt_connect_client_info_t client_info;

  // Set IP address
  IP_ADDR4(&mqttBrokerIp, 192, 168, 1, 1);

  // Generate random client id with length of two bytes
  auto randomId = bl_rand();
  auto clientId = etl::string<16>();
  clientId.resize(16);
  snprintf(clientId.data(), clientId.size() + 1, "%02x", randomId);
  printf("[%s] Generated random client id: \"%s\"\r\n", __func__,
         clientId.data());

  // Setup client
  memset(&client_info, 0, sizeof(client_info));
  client_info.client_id = clientId.data();
  client_info.client_user = MQTT_USER;
  client_info.client_pass = MQTT_PW;
  client_info.keep_alive = 60;  // Keep-alive time in seconds

#if ENABLE_MQTTS == TRUE
  client_info.tls_config = altcp_tls_create_config_client_2wayauth(
      CA_CERT.data(), CA_CERT_LEN, PRIV_KEY.data(), PRIV_KEY_LEN, nullptr, 0,
      CERT.data(), CERT_LEN);

  // Initiate connection (TLS)
  auto err = mqtt_client_connect(&mqttClient, &mqttBrokerIp, MQTT_TLS_PORT,
                                 my_mqtt_connected_cb, 0, &client_info);
#else
  // Initiate connection (plain text)
  auto err = mqtt_client_connect(&mqttClient, &mqttBrokerIp, MQTT_PORT,
                                 my_mqtt_connected_cb, 0, &client_info);
#endif

  // Check for connection errors
  if (err != ERR_OK) {
    printf("[%s] Could not connect: %d\r\n", __func__, err);
  }
}