// Include: Bluetooth HAL
//#include <conn.h>
//#include <gatt.h>
//#include <bluetooth.h>
//#include <ble_lib_api.h>
//#include <main.h>
//#include <mesh.h>
//#include <net.h>
//#include <transport.h>
//#include <hci_driver.h>
//#include <health_srv.h>
//#include <cfg_srv.h>
#include <common_srv.h>
#include <gen_srv.h>
//#include <foundation.h>
#include <mesh_backports.h>
//#include <bl_sec.h>
#include <access.h>
//#include <cfg_srv.h>
//#include <gap.h>
#include <mesh_model_opcode.h>
//#include <aos/kernel.h>
#include <stdio.h>

#include "include/board.h"
#include "include/node.h"

// Allow accessing model structure
extern struct bt_mesh_model models[];

// Allow accessing network configuration
extern struct bt_mesh_msg_ctx network_configuration;

// Send current STATUS message
// The message contains the current on/off state of the LED
void send_onoff_status(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *context)
{
	// Construct message:
	//	Parameters:
	//		1: Name of data container
	//		2: Operation code for the requested operation
	//		3: Payload length
	BT_MESH_MODEL_BUF_DEFINE(buf, BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, 1);

	// Put operation code into message
	bt_mesh_model_msg_init(&buf, BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS);

	// Add current state information to message
	net_buf_simple_add_u8(&buf, state.data);

	// Publish message
	int err = bt_mesh_model_send(&models[2], &network_configuration, &buf, NULL, NULL);
	if (err == 0) {
		printf("[SERVER] Published STATUS message\r\n");
	} else {
		printf("[SERVER] Failed to publish STATUS message: %d\r\n", err);
	}
}

// GET message handler -> check for valid model and send STATUS message
// to get the current LED state
void gen_onoff_get(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf)
{
	send_onoff_status(model, ctx);
}

// SET new state received by a client
void gen_onoff_set(struct bt_mesh_model *model,	
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf)
{

	// Check if we actually received data
	struct bt_mesh_gen_onoff_srv *data_model = model->user_data;
	struct state new_state;

	if (data_model == NULL) {
		printf("[SERVER] SET error: no data received \r\n");
		return;
	}

	// Obtain new value
	new_state.data = net_buf_simple_pull_u8(buf);

	// Check if received value is valid
	if (new_state.data > MESH_STATE_ON) {
		printf("[SERVER] Error: invalid state value\r\n");
		return;
	} else {
		printf("[SERVER] Received new state value: %d\r\n", new_state.data);
	}

	// Receive transaction ID
	new_state.transaction_id = net_buf_simple_pull_u8(buf);

	// Note:
	// In production, we would have to apply some input validation here:
	// 1. Check if the message is already known (known transaction ID)
	// 2. Check if the new state differs from the known one
	// 3. Check if the last state change is at least six seconds old

	// Update state
	state.data = new_state.data;
	state.transaction_id = new_state.transaction_id;
	board_change_state(state.data);
	printf("[SERVER] State changed\r\n");
}

// Acknowledged SET -> SET new status and send STATUS as response
void gen_onoff_set_with_ack(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf)
{
	printf("[SERVER] ACK SET called\r\n");
	gen_onoff_set(model, ctx, buf);
	send_onoff_status(model, ctx);
}

// Unacknowledged SET -> just SET new status
void gen_onoff_set_unack(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf)
{
	printf("[SERVER] SET called\r\n");
	gen_onoff_set(model, ctx, buf);
}