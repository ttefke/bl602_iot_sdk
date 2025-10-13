// FreeRTOS
#include <FreeRTOS.h>

// Bluetooth stack
#include <ble_lib_api.h>
#include <gatt.h>
#include <hci_driver.h>

// AOS HAL
#include <aos/yloop.h>

// Standard library
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Own headers
#include "include/ble.h"
#include "include/central.h"

/* Data structures used for bluetooth */
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;
static struct bt_gatt_exchange_params exchange_params;
static u16_t bt_gatt_write_without_handle = 0;
static struct bt_conn *default_conn;

/* Send message to peripheral device */
void ble_central_write() {
  // Data to send
  char data[19] = "Hello from Central";

  // Check if connection is available
  if (default_conn != NULL) {
    /* Send message
        Parameters:
            1: Connection
            2: Attribute handle
            3: Data to send
            4: Data length
            5: Whether to sign data
    */
    bt_gatt_write_without_response(default_conn, bt_gatt_write_without_handle,
                                   data, 19, 0);
  }
}

/* Callback for received notifications */
uint8_t ble_central_notify_function(
    [[gnu::unused]] struct bt_conn *conn,
    [[gnu::unused]] struct bt_gatt_subscribe_params *params, const void *data,
    uint16_t length) {
  /* Print received message */
  if (length > 0) {
    // Create buffer
    uint8_t recv_buffer[length];

    // Copy message into buffer
    memcpy(recv_buffer, data, length);

    // Print message
    printf("[CENTRAL] Received BLE notification: '");
    for (uint16_t i = 0; i < length; i++) {
      printf("%c", recv_buffer[i]);
    }
    printf("'\r\n");
  }

  return BT_GATT_ITER_CONTINUE;
}

/* Callback function indicating MTU exchange */
void ble_exchange_mtu_cb(
    struct bt_conn *conn, u8_t err,
    [[gnu::unused]] struct bt_gatt_exchange_params *params) {
  printf("[CENTRAL] MTU exchange %s, new MTU size: %d\r\n",
         err == 0U ? "sucessful" : "failed", bt_gatt_get_mtu(conn));
}

/* Initiate MTU exchange */
void ble_central_exchange_mtu() {
  if (!default_conn) {
    printf("[CENTRAL] Not connected!\r\n");
  } else {
    // Register callback function and exchange MTU
    exchange_params.func = ble_exchange_mtu_cb;
    printf("[CENTRAL] Current MTU size: %d, requesting MTU exchange\r\n",
           bt_gatt_get_mtu(default_conn));
    int err = bt_gatt_exchange_mtu(default_conn, &exchange_params);

    if (err) {
      printf("[CENTRAL] MTU exchange failed\r\n");
    } else {
      printf("[CENTRAL] MTU exchange pending\r\n");
    }
  }
}

// Discover offered services
uint8_t ble_central_discovery_function(struct bt_conn *conn,
                                       const struct bt_gatt_attr *attr,
                                       struct bt_gatt_discover_params *params) {
  int err;

  // Empty attribute table
  if (!attr) {
    printf("[CENTRAL] Discovery complete\r\n");
    memset(params, 0, sizeof(*params));
    return BT_GATT_ITER_STOP;
  }

  printf("[CENTRAL] Attribute handle %d\r\n", attr->handle);
  if (!bt_uuid_cmp(discover_params.uuid,
                   BT_UUID_TEST)) {  // BT_UUID_TEST received
    // Set discover data: discover characteristic with given TEST_RX UUID
    memcpy(&uuid, BT_UUID_TEST_RX, sizeof(uuid));
    discover_params.uuid = &uuid.uuid;
    discover_params.start_handle = attr->handle + 1;  // set start handle
    discover_params.type =
        BT_GATT_DISCOVER_CHARACTERISTIC;  // discover characteristic

    err = bt_gatt_discover(conn, &discover_params);
    if (err) {
      printf("[CENTRAL] Discovery failed: %d\r\n", err);
    }
  } else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_TEST_RX)) {
    // Set discover data: Discover CCC descriptor with given UUID
    // and set value handler for subscription:
    memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
    discover_params.uuid = &uuid.uuid;
    discover_params.start_handle = attr->handle + 2;
    discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
    subscribe_params.value_handle =
        bt_gatt_attr_value_handle(attr);  // returns handle of current attribute

    err = bt_gatt_discover(conn, &discover_params);
    if (err) {
      printf("[CENTRAL] Discovery failed: %d\r\n", err);
    }
  } else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_GATT_CCC)) {
    // Allow sending notifications
    subscribe_params.notify =
        ble_central_notify_function;              // Register callback function
    subscribe_params.value = BT_GATT_CCC_NOTIFY;  // Set subscription value
    subscribe_params.ccc_handle = attr->handle;   // Set handle

    /* Subscribe for notifications:
        Parameters:
            1: Connection
                2: Subscription parameters
    */
    err = bt_gatt_subscribe(conn, &subscribe_params);
    if (err && err != -EALREADY) {
      printf("[CENTRAL] Subscribe failed: %d\r\n", err);
    } else {
      printf("[CENTRAL] Subscribed successfully\r\n");
      bt_gatt_write_without_handle = subscribe_params.ccc_handle + 2;

      // Post message notifying a device subscribed
      aos_post_event(EV_BLE_TEST, BLE_DEV_SUBSCRIBED, NULL);
    }

    return BT_GATT_ITER_STOP;
  }

  return BT_GATT_ITER_STOP;
}

/* Device connected */
void ble_central_connected(struct bt_conn *conn, uint8_t conn_err) {
  // Convert bluetooth address to string
  char addr[BT_ADDR_LE_STR_LEN];

  /* Parameters:
      1: Get connection object
      2: Address of buffer containing the address
      3: Length of the address */
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  // Clean up data structures if connection fails
  if (conn_err) {
    printf("[CENTRAL] Failed to connect to %s: %u\r\n", addr, conn_err);
    bt_conn_unref(default_conn);
    default_conn = NULL;
    return;
  }

  printf("[CENTRAL] Connected %s\r\n", addr);

  // Post event to message broker
  aos_post_event(EV_BLE_TEST, BLE_DEV_CONN, NULL);

  // Check if the device we connected to is the same as
  // device whose advertisement packets we used to instanciate the connection
  if (conn == default_conn) {
    // Service discovery -> Discover offered services
    memcpy(&uuid, BT_UUID_TEST, sizeof(uuid));
    discover_params.uuid = &uuid.uuid;  // Set uuid to discover
    discover_params.func = ble_central_discovery_function;  // Callback function
    discover_params.start_handle = 0x0001;  // Included service start handle
    discover_params.end_handle =
        0xFFFF;  // Included service endle handle (check for UUIDs between both
                 // start and end values)
    discover_params.type =
        BT_GATT_DISCOVER_PRIMARY;  // Service type to discover

    // Call service discovery function
    //  Parameters:
    //      1: Connection object
    //      2: Discover parameters
    int err = bt_gatt_discover(default_conn, &discover_params);
    if (err) {
      printf("[CENTRAL] Device discovery failed: %d\r\n", err);
      return;
    }
  }
}

/* Device disconnected */
void ble_central_disconnected(struct bt_conn *conn, uint8_t reason) {
  char addr[BT_ADDR_LE_STR_LEN];

  // Convert address to string
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  printf("[CENTRAL] Disconnected: %s (reason: 0x%02x)\n", addr, reason);

  // Check if this is our connection
  if (default_conn != conn) {
    return;
  }

  // Empty connection data structure
  bt_conn_unref(default_conn);
  default_conn = NULL;

  // Post event to message broker
  aos_post_event(EV_BLE_TEST, BLE_DEV_DISCONN, NULL);
}

// Struct for connection callbacks
static struct bt_conn_cb conn_callbacks = {
    .connected = ble_central_connected,
    .disconnected = ble_central_disconnected,
};

// Evalue data callback: parse data we received from peripheral device
bool data_cb(struct bt_data *data, void *user_data) {
  char *name = user_data;
  u8_t len;

  switch (data->type) {
    // Shorten too long device messages
    case BT_DATA_NAME_SHORTENED:
    case BT_DATA_NAME_COMPLETE:
      len = (data->data_len > NAME_LEN - 1) ? (NAME_LEN - 1) : (data->data_len);
      memcpy(name, data->data, len);
      return false;
    default:
      return true;
  }
}

/* Found possible peripheral callback */
void ble_central_device_found(const bt_addr_le_t *addr, int8_t rssi,
                              uint8_t type, struct net_buf_simple *ad) {
  char dev[BT_ADDR_LE_STR_LEN];
  char name[NAME_LEN];

  // Set connection parameters
  struct bt_le_conn_param param = {
      .interval_min = BT_GAP_INIT_CONN_INT_MIN,
      .interval_max = BT_GAP_INIT_CONN_INT_MAX,
      .latency = 0,
      .timeout = 400,
  };

  // Convert address to string
  bt_addr_le_to_str(addr, dev, sizeof(dev));

  if (rssi < -20) {  // signal quality to bad -> ignore
    return;
  }

  // Only consider advertisement packets
  if (type == BT_LE_ADV_IND) {
    // Stop scanning
    bt_le_scan_stop();

    // Parse advertising data, shorten name
    memset(name, 0, sizeof(name));

    // Parsing function
    //  Parameters:
    //      1: Advertising data we received
    //      2: Callback function which parses each element
    //      3: User data to be passed to the callback
    bt_data_parse(ad, data_cb, name);
    printf("[CENTRAL] Device found: %s, RSSI: %i, Name: %s\r\n", dev, rssi,
           name);

    /* Try to connect
        Parameters:
            1: Address of remote device
            2: (Initial) connection parameters
    */
    default_conn = bt_conn_create_le(addr, &param);

    /* Check connection status */
    if (!default_conn) {
      printf("[CENTRAL] Connection failed\r\n");
    } else {
      printf("[CENTRAL] Connection pending\r\n");
    }
  }
}

/* Scan for peripherals (advertisement packets) */
void ble_central_start_scanning() {
  // Set scanning parameters
  struct bt_le_scan_param scan_param;
  scan_param.type = BT_LE_SCAN_TYPE_PASSIVE;
  scan_param.filter_dup = 0;
  scan_param.interval = 0x80;
  scan_param.window = 0x40;

  /* Start scanning:
      Parameters:
          1: Scan parameters
          2: Callback function */
  int err = bt_le_scan_start(&scan_param, ble_central_device_found);
  if (err) {
    printf("[CENTRAL] Scanning failed: %d\r\n", err);
    return;
  }

  // Post event to message broker
  aos_post_event(EV_BLE_TEST, BLE_SCAN_START, NULL);
  printf("[CENTRAL] Scanning started successfully\r\n");
}

/* Bluetooth started callback: start scanning for peripherals*/
void ble_central_init(int err) {
  if (err != 0) {
    printf("[CENTRAL] Bluetooth initialization failed\r\n");
  } else {
    printf("[CENTRAL] Bluetooth initialization succeeded\r\n");

    /* Start scanning */
    ble_central_start_scanning();
  }
}

/* Start application: initialize controller */
void start_central_application() {
  // Start up controller
  ble_controller_init(configMAX_PRIORITIES - 1);

  // Initialize host-controller driver
  hci_driver_init();

  // Enable bluetooth
  // Parameter: Callback function
  int err = bt_enable(ble_central_init);

  if (err) {
    printf("[CENTRAL] Bluetooth initialization failed\r\n");
    return;
  }

  // Register connection callbacks
  bt_conn_cb_register(&conn_callbacks);
}