#include "access.h"
#include "crypto.h"
#include "foundation.h"
#include "log.h"
#include "net.h"

#include <stdio.h>
#include <stdint.h>

uint8_t bt_mesh_app_key_add(uint16_t app_idx, uint16_t net_idx,
			const uint8_t key[16])
{
	struct bt_mesh_app_key *app;

	printf("net_idx 0x%04x app_idx %04x val %s\r\n", net_idx, app_idx, bt_hex(key, 16));

	if (!bt_mesh_subnet_get(net_idx)) {
		printf("invalid netkey\r\n");
		return STATUS_INVALID_NETKEY;
	}

	app = bt_mesh_app_key_alloc(app_idx);
	if (!app) {
		printf("insufficient resources\r\n");
		return STATUS_INSUFF_RESOURCES;
	}

	if (app->app_idx == app_idx) {
		if (app->net_idx != net_idx) {
			printf("invalid netkey: %d, %d\r\n", net_idx, app->net_idx);
			return STATUS_INVALID_NETKEY;
		}

		if (memcmp(key, &app->keys[0].val, 16)) {
			printf("key already stored\r\n");
			return STATUS_IDX_ALREADY_STORED;
		}

		return STATUS_SUCCESS;
	}

	if (bt_mesh_app_id(key, &app->keys[0].id)) {
		printf("can not set key\r\n");
		return STATUS_CANNOT_SET;
	}

	printf("AppIdx 0x%04x AID 0x%02x\r\n", app_idx, app->keys[0].id);

	app->net_idx = net_idx;
	app->app_idx = app_idx;
	app->updated = false;
	void *destination = app->keys[0].val;
	if (memcpy(destination, key, 16) != destination) {
		printf("Unable to import application key\r\n");
		return STATUS_CANNOT_SET;
	}
	return STATUS_SUCCESS;
}