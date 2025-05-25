#ifndef __HEALTH_H
#define __HEALTH_H

// Include health server definitions
#include <health_srv.h>

// Declare health server
struct bt_mesh_health_srv health_srv;
BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);
#endif