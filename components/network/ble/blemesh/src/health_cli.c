/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <types.h>
#include <util.h>
#include <byteorder.h>

#include <bluetooth.h>
#include <conn.h>
#include <include/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_health_cli
#include "log.h"

#include "net.h"
#include "foundation.h"
#include "errno.h"

static s32_t msg_timeout = K_SECONDS(2);

static struct bt_mesh_health_cli *health_cli;

struct health_fault_param {
    u16_t   cid;
    u8_t   *expect_test_id;
    u8_t   *test_id;
    u8_t   *faults;
    size_t *fault_count;
};

static void health_fault_status(struct bt_mesh_model *model,
                struct bt_mesh_msg_ctx *ctx,
                struct net_buf_simple *buf)
{
    struct health_fault_param *param;
    u8_t test_id;
    u16_t cid;

    BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
           ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
           bt_hex(buf->data, buf->len));

    if (health_cli->op_pending != OP_HEALTH_FAULT_STATUS) {
        BT_WARN("Unexpected Health Fault Status message");
        return;
    }

    param = health_cli->op_param;

    test_id = net_buf_simple_pull_u8(buf);
    if (param->expect_test_id && test_id != *param->expect_test_id) {
        BT_WARN("Health fault with unexpected Test ID");
        return;
    }

    cid = net_buf_simple_pull_le16(buf);
    if (cid != param->cid) {
        BT_WARN("Health fault with unexpected Company ID");
        return;
    }

    if (param->test_id) {
        *param->test_id = test_id;
    }

    if (buf->len > *param->fault_count) {
        BT_WARN("Got more faults than there's space for");
    } else {
        *param->fault_count = buf->len;
    }

    memcpy(param->faults, buf->data, *param->fault_count);

    k_sem_give(&health_cli->op_sync);
}

static void health_current_status(struct bt_mesh_model *model,
                  struct bt_mesh_msg_ctx *ctx,
                  struct net_buf_simple *buf)
{
    struct bt_mesh_health_cli *cli = model->user_data;
    u8_t test_id;
    u16_t cid;

    BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
           ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
           bt_hex(buf->data, buf->len));

    test_id = net_buf_simple_pull_u8(buf);
    cid = net_buf_simple_pull_le16(buf);

    BT_DBG("Test ID 0x%02x Company ID 0x%04x Fault Count %u",
           test_id, cid, buf->len);

    if (!cli->current_status) {
        BT_WARN("No Current Status callback available");
        return;
    }

    cli->current_status(cli, ctx->addr, test_id, cid, buf->data, buf->len);
}

struct health_period_param {
    u8_t *divisor;
};

static void health_period_status(struct bt_mesh_model *model,
                 struct bt_mesh_msg_ctx *ctx,
                 struct net_buf_simple *buf)
{
    struct health_period_param *param;

    BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
           ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
           bt_hex(buf->data, buf->len));

    if (health_cli->op_pending != OP_HEALTH_PERIOD_STATUS) {
        BT_WARN("Unexpected Health Period Status message");
        return;
    }

    param = health_cli->op_param;

    *param->divisor = net_buf_simple_pull_u8(buf);

    k_sem_give(&health_cli->op_sync);
}

struct health_attention_param {
    u8_t *attention;
};

static void health_attention_status(struct bt_mesh_model *model,
                    struct bt_mesh_msg_ctx *ctx,
                    struct net_buf_simple *buf)
{
    struct health_attention_param *param;

    BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
           ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
           bt_hex(buf->data, buf->len));

    if (health_cli->op_pending != OP_ATTENTION_STATUS) {
        BT_WARN("Unexpected Health Attention Status message");
        return;
    }

    param = health_cli->op_param;

    if (param->attention) {
        *param->attention = net_buf_simple_pull_u8(buf);
    }

    k_sem_give(&health_cli->op_sync);
}

const struct bt_mesh_model_op bt_mesh_health_cli_op[] = {
    { OP_HEALTH_FAULT_STATUS,    3,   health_fault_status },
    { OP_HEALTH_CURRENT_STATUS,  3,   health_current_status },
    { OP_HEALTH_PERIOD_STATUS,   1,   health_period_status },
    { OP_ATTENTION_STATUS,       1,   health_attention_status },
    BT_MESH_MODEL_OP_END,
};

static int cli_prepare(void *param, u32_t op)
{
    if (!health_cli) {
        BT_ERR("No available Health Client context!");
        return -EINVAL;
    }

    if (health_cli->op_pending) {
        BT_WARN("Another synchronous operation pending");
        return -EBUSY;
    }

    health_cli->op_param = param;
    health_cli->op_pending = op;

    return 0;
}

static void cli_reset(void)
{
    health_cli->op_pending = 0U;
    health_cli->op_param = NULL;
}

static int cli_wait(void)
{
    int err;

    err = k_sem_take(&health_cli->op_sync, msg_timeout);

    cli_reset();

    return err;
}

int bt_mesh_health_attention_get(u16_t net_idx, u16_t addr, u16_t app_idx,
                 u8_t *attention)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 0 + 4);
    struct bt_mesh_msg_ctx ctx = {
        .net_idx = net_idx,
        .app_idx = app_idx,
        .addr = addr,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    struct health_attention_param param = {
        .attention = attention,
    };
    int err;

    err = cli_prepare(&param, OP_ATTENTION_STATUS);
    if (err) {
        return err;
    }

    bt_mesh_model_msg_init(&msg, OP_ATTENTION_GET);

    err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
    if (err) {
        BT_ERR("model_send() failed (err %d)", err);
        cli_reset();
        return err;
    }

    return cli_wait();
}

int bt_mesh_health_attention_set(u16_t net_idx, u16_t addr, u16_t app_idx,
                 u8_t attention, u8_t *updated_attention)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 1 + 4);
    struct bt_mesh_msg_ctx ctx = {
        .net_idx = net_idx,
        .app_idx = app_idx,
        .addr = addr,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    struct health_attention_param param = {
        .attention = updated_attention,
    };
    int err;

    err = cli_prepare(&param, OP_ATTENTION_STATUS);
    if (err) {
        return err;
    }

    if (updated_attention) {
        bt_mesh_model_msg_init(&msg, OP_ATTENTION_SET);
    } else {
        bt_mesh_model_msg_init(&msg, OP_ATTENTION_SET_UNREL);
    }

    net_buf_simple_add_u8(&msg, attention);

    err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
    if (err) {
        BT_ERR("model_send() failed (err %d)", err);
        cli_reset();
        return err;
    }

    if (!updated_attention) {
        cli_reset();
        return 0;
    }

    return cli_wait();
}

int bt_mesh_health_period_get(u16_t net_idx, u16_t addr, u16_t app_idx,
                  u8_t *divisor)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 0 + 4);
    struct bt_mesh_msg_ctx ctx = {
        .net_idx = net_idx,
        .app_idx = app_idx,
        .addr = addr,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    struct health_period_param param = {
        .divisor = divisor,
    };
    int err;

    err = cli_prepare(&param, OP_HEALTH_PERIOD_STATUS);
    if (err) {
        return err;
    }

    bt_mesh_model_msg_init(&msg, OP_HEALTH_PERIOD_GET);

    err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
    if (err) {
        BT_ERR("model_send() failed (err %d)", err);
        cli_reset();
        return err;
    }

    return cli_wait();
}

int bt_mesh_health_period_set(u16_t net_idx, u16_t addr, u16_t app_idx,
                  u8_t divisor, u8_t *updated_divisor)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 1 + 4);
    struct bt_mesh_msg_ctx ctx = {
        .net_idx = net_idx,
        .app_idx = app_idx,
        .addr = addr,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    struct health_period_param param = {
        .divisor = updated_divisor,
    };
    int err;

    err = cli_prepare(&param, OP_HEALTH_PERIOD_STATUS);
    if (err) {
        return err;
    }

    if (updated_divisor) {
        bt_mesh_model_msg_init(&msg, OP_HEALTH_PERIOD_SET);
    } else {
        bt_mesh_model_msg_init(&msg, OP_HEALTH_PERIOD_SET_UNREL);
    }

    net_buf_simple_add_u8(&msg, divisor);

    err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
    if (err) {
        BT_ERR("model_send() failed (err %d)", err);
        cli_reset();
        return err;
    }

    if (!updated_divisor) {
        cli_reset();
        return 0;
    }

    return cli_wait();
}

int bt_mesh_health_fault_test(u16_t net_idx, u16_t addr, u16_t app_idx,
                  u16_t cid, u8_t test_id, u8_t *faults,
                  size_t *fault_count)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 3 + 4);
    struct bt_mesh_msg_ctx ctx = {
        .net_idx = net_idx,
        .app_idx = app_idx,
        .addr = addr,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    struct health_fault_param param = {
        .cid = cid,
        .expect_test_id = &test_id,
        .faults = faults,
        .fault_count = fault_count,
    };
    int err;

    err = cli_prepare(&param, OP_HEALTH_FAULT_STATUS);
    if (err) {
        return err;
    }

    if (faults) {
        bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_TEST);
    } else {
        bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_TEST_UNREL);
    }

    net_buf_simple_add_u8(&msg, test_id);
    net_buf_simple_add_le16(&msg, cid);

    err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
    if (err) {
        BT_ERR("model_send() failed (err %d)", err);
        cli_reset();
        return err;
    }

    if (!faults) {
        cli_reset();
        return 0;
    }

    return cli_wait();
}

int bt_mesh_health_fault_clear(u16_t net_idx, u16_t addr, u16_t app_idx,
                   u16_t cid, u8_t *test_id, u8_t *faults,
                   size_t *fault_count)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 2 + 4);
    struct bt_mesh_msg_ctx ctx = {
        .net_idx = net_idx,
        .app_idx = app_idx,
        .addr = addr,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    struct health_fault_param param = {
        .cid = cid,
        .test_id = test_id,
        .faults = faults,
        .fault_count = fault_count,
    };
    int err;

    err = cli_prepare(&param, OP_HEALTH_FAULT_STATUS);
    if (err) {
        return err;
    }

    if (test_id) {
        bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_CLEAR);
    } else {
        bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_CLEAR_UNREL);
    }

    net_buf_simple_add_le16(&msg, cid);

    err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
    if (err) {
        BT_ERR("model_send() failed (err %d)", err);
        cli_reset();
        return err;
    }

    if (!test_id) {
        cli_reset();
        return 0;
    }

    return cli_wait();
}

int bt_mesh_health_fault_get(u16_t net_idx, u16_t addr, u16_t app_idx,
                 u16_t cid, u8_t *test_id, u8_t *faults,
                 size_t *fault_count)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 2 + 4);
    struct bt_mesh_msg_ctx ctx = {
        .net_idx = net_idx,
        .app_idx = app_idx,
        .addr = addr,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    struct health_fault_param param = {
        .cid = cid,
        .test_id = test_id,
        .faults = faults,
        .fault_count = fault_count,
    };
    int err;

    err = cli_prepare(&param, OP_HEALTH_FAULT_STATUS);
    if (err) {
        return err;
    }

    bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_GET);
    net_buf_simple_add_le16(&msg, cid);

    err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
    if (err) {
        BT_ERR("model_send() failed (err %d)", err);
        cli_reset();
        return err;
    }

    return cli_wait();
}

s32_t bt_mesh_health_cli_timeout_get(void)
{
    return msg_timeout;
}

void bt_mesh_health_cli_timeout_set(s32_t timeout)
{
    msg_timeout = timeout;
}

int bt_mesh_health_cli_set(struct bt_mesh_model *model)
{
    if (!model->user_data) {
        BT_ERR("No Health Client context for given model");
        return -EINVAL;
    }

    health_cli = model->user_data;

    return 0;
}

int bt_mesh_health_cli_init(struct bt_mesh_model *model, bool primary)
{
    struct bt_mesh_health_cli *cli = model->user_data;

    BT_DBG("primary %u", primary);

    if (!cli) {
        BT_ERR("No Health Client context provided");
        return -EINVAL;
    }

    cli = model->user_data;
    cli->model = model;

    k_sem_init(&cli->op_sync, 0, 1);

    /* Set the default health client pointer */
    if (!health_cli) {
        health_cli = cli;
    }

    return 0;
}
