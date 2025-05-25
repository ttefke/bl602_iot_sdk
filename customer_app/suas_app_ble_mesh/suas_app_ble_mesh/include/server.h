#ifndef __SERVER_H
#define __SERVER_H

#include <access.h>
#include <buf.h>

// Function prototypes

void gen_onoff_get(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf);

void gen_onoff_set_with_ack(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf);

void gen_onoff_set_unack(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf);
#endif