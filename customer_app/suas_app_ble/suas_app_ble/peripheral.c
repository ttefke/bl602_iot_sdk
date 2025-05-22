// FreeRTOS
#include <FreeRTOS.h>

// Bluetooth stack
#include <ble_lib_api.h>
#include <gatt.h>
#include <hci_driver.h>

// AOS HAL
#include <aos/yloop.h>

// Standard library
#include <stdio.h>
#include <stdint.h>

// Own headers
#include "include/ble.h"
#include "include/peripheral.h"

/* Function prototypes */
void ble_bl_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t vblfue);
int ble_blf_recv(struct bt_conn *conn,
    const struct bt_gatt_attr *attr, const void *buf,
    u16_t len, u16_t offset, u8_t flags);
void ble_peripheral_connected(struct bt_conn *conn, uint8_t err);
void ble_peripheral_disconnected(struct bt_conn *conn, uint8_t reason);

/* Connection callback function definitions */

static struct bt_conn_cb conn_callbacks = {
    .connected = ble_peripheral_connected,
    .disconnected = ble_peripheral_disconnected
};

/* Is notify feature enabled? */
bool notify_flag = false;

/* Advertising data */
const struct bt_data advertising_data[] = {
    /* Gerneral discoverable, BR/EDR nor supported (BLE only) */
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),

    /* Name of discoverable device */
    BT_DATA(BT_DATA_NAME_COMPLETE, "BL_602", 6),

    /* Manufacturer specific data */
    BT_DATA(BT_DATA_MANUFACTURER_DATA, "BL_602", 6),
};

/* Definition of the server */
static struct bt_gatt_attr blattrs[] = {
     /* (Primary) Service */
    BT_GATT_PRIMARY_SERVICE(BT_UUID_TEST), /* Service UUID */

     /* Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_TEST_RX, /* Attribute UUID */
        BT_GATT_CHRC_NOTIFY, /* Atribute properties: permit notifications sent from client */
        BT_GATT_PERM_READ, /* Attribute access permissions (read-only) */
        NULL, /* Attribute read callback */
        NULL, /* Attribute write callback */
        NULL), /* Attribute value */

    /* Client Characteristic Configuration */
    BT_GATT_CCC(ble_bl_ccc_cfg_changed, /* Configuration changed callback */
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), /* CCC access permissions: read and write */

    /* Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_TEST_TX, /* Attribute UUID*/
        BT_GATT_CHRC_WRITE_WITHOUT_RESP, /* Attribute properties: write without response */
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, /* Attribute access permissions (read and write) */
        NULL, /* Attribute read callback */
        ble_blf_recv, /* Attribute write callback */
        NULL) /* Attribute value */
};

/* Create server data structure*/
struct bt_gatt_service ble_bl_server = BT_GATT_SERVICE(blattrs);

/* Send notification */
void ble_peripheral_send_notification() {
    // Data to send
    char data[22] = "Hello from Peripheral";

    // Send data if connection available and notifications allowed
    if (default_conn != NULL && notify_flag == true) {
        printf("[PERIPHERAL] Sending notification\r\n");
        
        /* Send notification:
            Parameters:
            1: Connection
            2: Characteristic to handle: notification
            3: Data to send
            4: Length of the data
        */
        bt_gatt_notify(default_conn, &blattrs[1], data, 22);
    }
}

/* Received data callback function */
int ble_blf_recv(struct bt_conn *conn,
    const struct bt_gatt_attr *attr, const void *buf,
    u16_t len, u16_t offset, u8_t flags)
{
    // Allocate storage to hold received data
    uint8_t *recv_buffer;
    recv_buffer = pvPortMalloc(sizeof(uint8_t) * len);

    // Copy received data to allocated storage
    memcpy(recv_buffer, buf, len);

    // Print received data byte-wise, interpreted as character
    printf("[PERIPHERAL] Received data: '");
    for (uint16_t i = 0; i < len; i++) {
        printf("%c", recv_buffer[i]);
    }
    printf("'\r\n");

    // Free data structure and return
    vPortFree(recv_buffer);
    return 0;
}

/* Changes in client characteristic configuration */
void ble_bl_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value) {
    // Enable notifications if requested
    if (value == BT_GATT_CCC_NOTIFY) {
        notify_flag = true;
        printf("[PERIPHERAL] Enabled notify\r\n");
    } else {
        notify_flag = false;
        printf("[PERIPHERAL] Disabled notify\r\n");
    }
}

/* Start advertising */
void ble_peripheral_start_advertising() {
    bt_set_name("blf_602");
    printf("[PERIPHERAL] Started advertising\r\n");

    /* Start advertising:
        Parameters:
            1: Advertising parameters (defined by macro)
            2: Advertising data 
            3: Size of advertising data
            4: Data to send in scan response packets to other devices
            5: Size of scan response data
    */
    int err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, advertising_data, ARRAY_SIZE(advertising_data), NULL, 0);
    if (err) {
        printf("[PERIPHERAL] Advertising failed to start: %d\r\n", err);
    } else {
        // Send advertising start message to event handler
        aos_post_event(EV_BLE_TEST, BLE_ADV_START, NULL);
    }
}

/* Bluetooth stack started callback */
void ble_peripheral_init(int err) {
    if (err != 0) {
        printf("[PERIPHERAL] Failed to start Bluetooth stack\r\n");
    } else {
        printf("[PERIPHERAL] Bluetooth initialized\r\n");

        // Start advertising
        ble_peripheral_start_advertising();
    }
}

/* Connected to device */
void ble_peripheral_connected(struct bt_conn *conn, uint8_t err) {
    // Set connection parameters
    struct bt_le_conn_param param;
    param.interval_max = 24;
    param.interval_min = 24;
    param.latency = 0;
    param.timeout = 600;

    if (err) {
        printf("[PERIPHERAL] Connection failed: 0x%02x", err);
    } else {
        printf("[PERIPHERAL] Connected to a device\r\n");

        // Update connection data
        default_conn = conn;
        int update_err = bt_conn_le_param_update(conn, &param);

        if (update_err) {
            printf("[PERIPHERAL] Connection update failed: %d\r\n", update_err);
        } else {
            printf("[PERIPHERAL] Connection update initiated\r\n");
        }

        // Send connection established message to device handler
        aos_post_event(EV_BLE_TEST, BLE_DEV_CONN, NULL);
    }
}

/* Device disconnected */
void ble_peripheral_disconnected(struct bt_conn *conn, uint8_t reason) {
    printf("[PERIPHERAL] Disconnected, reason: 0x%02x\r\n", reason);

    // Send device disconnected message to device handler
    aos_post_event(EV_BLE_TEST, BLE_DEV_DISCONN, NULL);
}

/* Start bluetooth stack */
void ble_stack_start() {
    // Start up controller
    ble_controller_init(configMAX_PRIORITIES - 1); // BLE Controller has maximum priority

    // Initialize host-controller driver
    hci_driver_init();

    // Enable bluetooth
    // Parameter: Callback function
    bt_enable(ble_peripheral_init);
}

/* Set up device as peripheral */
void start_peripheral_application() {
    // Start bluetooth stack
    ble_stack_start();

    // Register connection callbacks
    bt_conn_cb_register(&conn_callbacks);

    // Register GATT service
    int err = bt_gatt_service_register(&ble_bl_server);

    if (err == 0) {
        printf("[PERIPHERAL] Service started\r\n");
    } else {
        printf("[PERIPHERAL] Error happened during service registration\r\n");
    }
}