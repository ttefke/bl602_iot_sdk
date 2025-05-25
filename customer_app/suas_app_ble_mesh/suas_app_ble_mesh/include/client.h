#ifndef __CLIENT_H
#define __CLIENT_H

#include <access.h>
#include <buf.h>
#include <stdbool.h>

// Function prototypes
void mesh_send_set_message(bool acknowledged);
void mesh_send_get_message();

void gen_onoff_status(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf);
#endif