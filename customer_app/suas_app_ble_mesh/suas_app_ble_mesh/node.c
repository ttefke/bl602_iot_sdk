// Bluetooth HAL
#include <access.h>
#include <ble_lib_api.h>
#include <bluetooth.h>
#include <cfg_srv.h>
#include <health_srv.h>
#include <hci_driver.h>
#include <main.h>
#include <mesh_model_opcode.h>

// Include security library to generate device address
#include <bl_sec.h>

// Standard input/output
#include <stdio.h>

// Own headers
#include "include/client.h"
#include "include/health.h"
#include "include/node.h"
#include "include/provisioning.h"
#include "include/server.h"

// Server configuration
struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_DISABLED,
	.beacon = BT_MESH_BEACON_ENABLED,
#if defined(CONFIG_BT_MESH_FRIEND)
	.frnd = BT_MESH_FRIEND_ENABLED,
#else
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	.gatt_proxy = BT_MESH_GATT_PROXY_ENABLED,
#else
	.gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
	.default_ttl = 3,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(2, 20),
};

// Server operations:
// Parameters:
// 1: Opcode
// 2: Payload length (must match actual payload size otherwise this will result in an error)
// 3: Handler function
const struct bt_mesh_model_op gen_onoff_srv_op[] = {
	{ BLE_MESH_MODEL_OP_GEN_ONOFF_GET,			0,	gen_onoff_get },
	{ BLE_MESH_MODEL_OP_GEN_ONOFF_SET,			2,	gen_onoff_set_with_ack },
	{ BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK,	2,	gen_onoff_set_unack },
	BT_MESH_MODEL_OP_END,
};

// Client operations: same structure as server operations
struct bt_mesh_model_op gen_onoff_cli_op[] = {
	{BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, 1, gen_onoff_status},
	 BT_MESH_MODEL_OP_END,
};

// Define server and client names
// Parameters:
// 1: Variable name given to context (must match model publication in next structure)
// 2: Optional message callback
// 3: Message length
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_srv, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_cli, NULL, 2 + 2);

// Define model
// Parameters:
// 1: Service UUID
// 2: Operations (must match operations structures names above)
// 3: Model publication status (must match variable name given above)
// 4: User data structure that is being exchanged
struct bt_mesh_model models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op, &gen_onoff_pub_srv, &state),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, gen_onoff_cli_op, &gen_onoff_pub_cli, &state),
};

// Specify elements of the service
// Parameters:
// 1: Location descriptor
// 2: Array of models
// 3: Array of vendor models
struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, models, BT_MESH_MODEL_NONE),
};

// Specify node composition
// Parameters:
// 1: Company identifier (set to Linux Foundation here)
// 2: Elements the node consists of
// 3: Number of elements
struct bt_mesh_comp comp = {
	.cid = BT_COMP_ID_LF,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

// Network configuration
struct bt_mesh_msg_ctx network_configuration = {
	.net_idx = 0, // Network ID
	.app_idx = MESH_APP_ID, // Application ID
	.addr = MESH_GROUP_ADDRESS, // Receiver address
};

// Bluetooth started callback function
void bt_ready_cb(int err) {
	// Check for errors
	if (err) {
		printf("[NODE] Bluetooth initialization failed\r\n");
		return;
	}

	// Initialize mesh support
	err = bt_mesh_init(&prov, &comp);
	if (err) {
		printf("[NODE] Initializing mesh failed\r\n");
		return;
	}

	// Enable provisioning
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
	printf("[NODE] Mesh mode initialized\r\n");
}

// Initialize bluetooth
void mesh_init() {
	// Set device UUID to random address
	bl_rand_stream(dev_uuid, 16);

	// Initialize bluetooth controller
	ble_controller_init(configMAX_PRIORITIES -1);
	hci_driver_init();

	// Start bluetooth and register callback function
	int err = bt_enable(bt_ready_cb);
	if (err) {
		printf("[NODE] Bluetooth initialization failed\r\n");
	}
}