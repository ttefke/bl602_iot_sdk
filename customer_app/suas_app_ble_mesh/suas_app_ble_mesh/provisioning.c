// Backported HAL functions
#include <mesh_backports.h>

// Security engine
#include <bl_sec.h>

// Standard input/output
#include <stdio.h>

// Own headers
#include "include/board.h"
#include "include/node.h"
#include "include/provisioning.h"

// Allow accessing model information structure
extern struct bt_mesh_model models[];

// Provisioning completed callback
void prov_complete([[gnu::unused]] uint16_t net_idx, [[gnu::unused]] uint16_t addr) {
	printf("[PROVISIONING] Provisioning complete\r\n");
	board_prov_complete();
}

// Provisioning reset callback function
void prov_reset() {
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
	board_prov_reset();
}

// Specify provisioning properties
struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.output_actions = BT_MESH_NO_OUTPUT,
	.complete = prov_complete,
	.reset = prov_reset,
};

// Provision node:
// Here we do this automatically
// In production, use a provisioning device instead!
void mesh_node_provision() {
	// Hardcoded app and net key [!!!]
	static uint8_t app_key[16] = {0x00};
	static uint8_t net_key[16] = {0x00};

	// Generate device key
	static uint8_t dev_key[16];
	bl_rand_stream(dev_key, 16);

	// Generate unique device address:
	// Device addresses must be < 0x8000
	// So regenerate until we get a usable address
	uint16_t addr;
	do {
		addr = bl_sec_get_random_word();
	} while (addr >= 0x8000);

	// Start provisioning process
	printf("[PROVISIONING] Self-provisioning now \r\n");

	// Provision device with the following parameters:
	// 	1. Network key
	// 	2. Network (key) index
	//	3. Provisioning flags
	//	4. IV index
	//	5. Device address
	//	6. Device key
	int err = bt_mesh_provision(net_key, 0, 0, 0, addr, dev_key);
	if (err) {
		printf("[PROVISIONING] Provisioning failed: %d\r\n", err);
		return;
	}

	// Add application key
	// Parameters:
	// 	1: Application id (must be > 0)
	// 	2: Network (key) index
	// 	3: Application key
	err = bt_mesh_app_key_add(1, 0, app_key);
	if (err) {
		printf("[PROVISIONING] Could not set application key\r\n");
		return;
	}

	// Bind keys to application id
	// (this and the last step ensure we subscribed to the correct application)
	models[2].keys[0] = MESH_APP_ID;
	models[3].keys[0] = MESH_APP_ID;

	// Set group addresses
	models[2].groups[0] = MESH_GROUP_ADDRESS;
	models[3].groups[0] = MESH_GROUP_ADDRESS;

	// Finished provisioning
	printf("[PROVISIONING] Provisioned!\r\n");
}