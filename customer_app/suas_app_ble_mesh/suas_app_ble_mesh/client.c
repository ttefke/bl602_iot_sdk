// Bluetooth HAL includes
#include <access.h>
#include <buf.h>
#include <mesh_backports.h>
#include <mesh_model_opcode.h>

// Standard intput/output
#include <stdio.h>

// Node header for mesh address
#include "include/node.h"

// Payload string for message encoding/decoding
const char *const onoff_str[] = { "off", "on" };

// Access model information from node.c
extern struct bt_mesh_model models[];

// Allow accessing network configuration
extern struct bt_mesh_msg_ctx network_configuration;

// Client: evaluate received GET packet
void gen_onoff_status([[gnu::unused]] struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf)
{
	// Get status variable from packet buffer
	uint8_t status = net_buf_simple_pull_u8(buf);

	// Print status information
	printf("[CLIENT] Received STATUS message from %d: %s\n",
		ctx->addr, onoff_str[status]);
}

// Send On/Off Get message to all nodes
void mesh_send_get_message() {
	// Construct message:
	//	Parameters:
	//		1: Name of data container
	//		2: Operation code for the requested operation
	//		3: Payload length
	BT_MESH_MODEL_BUF_DEFINE(buf, BLE_MESH_MODEL_OP_GEN_ONOFF_GET, 0);

	// Put operation code into message
	bt_mesh_model_msg_init(&buf, BLE_MESH_MODEL_OP_GEN_ONOFF_GET);

	// Publish message
	int err = bt_mesh_model_send(&models[3], &network_configuration, &buf, NULL, NULL);
	if (err == 0) {
		printf("[CLIENT] Published GET message\r\n");
	} else {
		printf("[CLIENT] Failed to publish GET message: %d\r\n", err);
	}
}

// Send OnOff Set message to all nodes
void mesh_send_set_message(bool acknowledged) {
	// Reverse state: turn LEDs off if turned on and vice versa
	state.data = state.data == 0 ? 1 : 0;
	
	// Increment Transaction ID
	if (state.transaction_id == 255) {
		state.transaction_id = 0;
	} else {
		state.transaction_id++;
	}

	// Construct message: Unacknowledged on/off message with 2 bytes payload
	// In the define macro, the 'BLE_MESH_MODEL_OP_GEN_ONOFF_SET' only sets the length
	// of the operationcode which is equal here for unacknowledged and
	// acknowledged messages so this is fine here
	BT_MESH_MODEL_BUF_DEFINE(buf, BLE_MESH_MODEL_OP_GEN_ONOFF_SET, 2);

	// Set operation code
	if (acknowledged) {
		bt_mesh_model_msg_init(&buf, BLE_MESH_MODEL_OP_GEN_ONOFF_SET);
	} else {
		bt_mesh_model_msg_init(&buf, BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
	}

	// Add payload
	net_buf_simple_add_u8(&buf, state.data);
	net_buf_simple_add_u8(&buf, state.transaction_id);

	// Publish message
	int err = bt_mesh_model_send(&models[3], &network_configuration, &buf, NULL, NULL);

	// Output possible errors
	if (err == 0) {
		printf("[CLIENT] Published SET message: %s, transaction id: %d\r\n",
			onoff_str[state.data], state.transaction_id);
	} else {
		printf("[CLIENT] Failed to publish SET message: %s, transaction id: %d, error: %d\r\n",
			onoff_str[state.data], state.transaction_id, err);
	}
}
