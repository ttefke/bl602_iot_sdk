#ifndef __MESH_BACKPORTS_H
#define __MESH_BACKPORTS_H

#include <stdint.h>

uint8_t bt_mesh_app_key_add(uint16_t app_idx, uint16_t net_idx,
			const uint8_t key[16]);


/* Backports */
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define BT_MESH_MODEL_OP_LEN(_op) ((_op) <= 0xff ? 1 : (_op) <= 0xffff ? 2 : 3)

#define BT_MESH_MIC_SHORT 4

#define BT_MESH_MODEL_BUF_LEN(_op, _payload_len)                               \
	(BT_MESH_MODEL_OP_LEN(_op) + (_payload_len) + BT_MESH_MIC_SHORT)

#define BT_MESH_MODEL_BUF_DEFINE(_buf, _op, _payload_len)                      \
	NET_BUF_SIMPLE_DEFINE(_buf, BT_MESH_MODEL_BUF_LEN(_op, (_payload_len)))

#define SYS_FOREVER_MS (-1)

#endif