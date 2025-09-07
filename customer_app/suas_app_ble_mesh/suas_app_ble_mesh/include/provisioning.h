#ifndef __PROVISIONING_H
#define __PROVISIONING_H

#include <main.h>

// Struct to set provisioning callback functions
extern struct bt_mesh_prov prov;

// Device UUID
extern uint8_t dev_uuid[16];

// Function prototype
void mesh_node_provision();
#endif