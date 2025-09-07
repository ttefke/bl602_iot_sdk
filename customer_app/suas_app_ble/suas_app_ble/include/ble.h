#ifndef __BLE_H
#define __BLE_H

/* Event codes used by PineCone to communicate with the Bluetooth controller */
#define EV_BLE_TEST          0x0504
#define BLE_ADV_START   0x01
#define BLE_ADV_STOP    0x02
#define BLE_DEV_CONN    0x03
#define BLE_DEV_DISCONN 0x04
#define BLE_SCAN_START  0x05
#define BLE_SCAN_STOP   0x06
#define BLE_DEV_SUBSCRIBED 0x07

/* UUIDs to reference services */
#define BT_UUID_TEST        BT_UUID_DECLARE_16(0xFFF0)
#define BT_UUID_TEST_RX     BT_UUID_DECLARE_16(0xFFF1)
#define BT_UUID_TEST_TX     BT_UUID_DECLARE_16(0xFFF2)

/* Constants */
#define NAME_LEN        30
#endif