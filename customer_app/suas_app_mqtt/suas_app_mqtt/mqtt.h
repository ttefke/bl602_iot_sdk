#ifndef __MQTT_H
#define __MQTT_H

// Credentials
// Do not hardcode credentials in production code
#define MQTT_USER "suas"
#define MQTT_PW "J4auBDJYzcrL8s9TEZJt"

// Security
#define FALSE 0
#define TRUE 1
#define ENABLE_MQTTS TRUE

// Quality of Service for messages
#define QUALITY_OF_SERVICE 1

// Function prototypes
void my_mqtt_connect();
void my_mqtt_disconnect();
void my_mqtt_publish();

#endif /* __MQTT_H */