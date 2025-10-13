#ifndef __MESH_NODE_H
#define __MESH_NODE_H

// Mesh group address (group addresses must be between 0xC000 and 0xFF00)
#define MESH_GROUP_ADDRESS 0xC000

// Application identifier
#define MESH_APP_ID 1

// Data exchange container
struct state {
  uint8_t data;
  uint8_t transaction_id;
};

extern struct state state;

// Function prototypes
void mesh_init();
#endif