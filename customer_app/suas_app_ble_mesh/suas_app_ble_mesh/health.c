// Include headers necessary for health service
#include <access.h>
#include <health_srv.h>

// Standard input/output
#include <stdio.h>

// Own header
#include "include/health.h"

// Define health service
void attention_on(struct bt_mesh_model *mod) {
	printf("[HEALTH] Attention on\r\n");
}

void attention_off(struct bt_mesh_model *mod) {
	printf("[HEALTH] Attention off\r\n");
}

const struct bt_mesh_health_srv_cb health_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

struct bt_mesh_health_srv health_srv = {
	.cb = &health_cb,
};

