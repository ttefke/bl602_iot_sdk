/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Workqueue support functions
 */


#include <zephyr.h>
#include <log.h>
#include "errno.h"

struct k_thread work_q_thread;
#if !defined(BFLB_BLE)
static BT_STACK_NOINIT(work_q_stack, CONFIG_BT_WORK_QUEUE_STACK_SIZE);
#endif
struct k_work_q g_work_queue_main;

#if defined(BFLB_BLE)
static timer_rec_d timer_records[CONFIG_MAX_TIMER_REC];
static void timer_rec_init();
static int alloc_timer_record(struct k_delayed_work *delay_work);
#endif

static void k_work_submit_to_queue(struct k_work_q *work_q,
                                   struct k_work *work)
{
    if (!atomic_test_and_set_bit(work->flags, K_WORK_STATE_PENDING)) {
        k_fifo_put(&work_q->fifo, work);
    }
}

#if defined(BFLB_BLE)
static void work_queue_main(void *p1)
{
    struct k_work *work;
    UNUSED(p1);

    while (1) {
        work = k_fifo_get(&g_work_queue_main.fifo, K_FOREVER);

        if (atomic_test_and_clear_bit(work->flags, K_WORK_STATE_PENDING)) {
            work->handler(work);
        }

        k_yield();
    }
}

int k_work_q_start(void)
{
    timer_rec_init();
    k_fifo_init(&g_work_queue_main.fifo, 20);
    return k_thread_create(&work_q_thread, "work_q_thread",
                           CONFIG_BT_WORK_QUEUE_STACK_SIZE,
                           work_queue_main, CONFIG_BT_WORK_QUEUE_PRIO);
}

int k_work_init(struct k_work *work, k_work_handler_t handler)
{
    ASSERT(work, "work is NULL");

    atomic_clear_bit(work->flags, K_WORK_STATE_PENDING);
    work->handler = handler;
    return 0;
}

void k_work_submit(struct k_work *work)
{
    k_work_submit_to_queue(&g_work_queue_main, work);
}

static void work_timeout(void *timer)
{
    timer_rec_d *rec = get_timer_record(timer);
    BT_ASSERT(rec != NULL);
    struct k_delayed_work *w = rec->delay_work;

    /* submit work to workqueue */
    k_timer_stop(&w->timer);
    k_work_submit_to_queue(w->work_q, &w->work);
    /* detach from workqueue, for cancel to return appropriate status */
    w->work_q = NULL;
    rec->timer.hdl = NULL;
}

void k_delayed_work_init(struct k_delayed_work *work, k_work_handler_t handler)
{
    ASSERT(work, "delay work is NULL");
    k_work_init(&work->work, handler);
    k_timer_init(&work->timer, work_timeout, work);
    work->work_q = NULL;
}

static int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
        struct k_delayed_work *work,
        uint32_t delay)
{
    int key = irq_lock();
    int err;

    /* Work cannot be active in multiple queues */
    if (work->work_q && work->work_q != work_q) {
        err = -EADDRINUSE;
        goto done;
    }

    /* Cancel if work has been submitted */
    if (work->work_q == work_q) {
        err = k_delayed_work_cancel(work);

        if (err < 0) {
            goto done;
        }
    }

    /* Attach workqueue so the timeout callback can submit it */
    work->work_q = work_q;
    if (!delay) {
        /* Submit work if no ticks is 0 */
        k_work_submit_to_queue(work_q, &work->work);
        work->work_q = NULL;
    } else {
        /* Add timeout */
        k_timer_start(&work->timer, delay);
        err = add_timer_record(work);
        BT_ASSERT(err == 0);
    }

    err = 0;

done:
    irq_unlock(key);
    return err;
}

int k_delayed_work_submit(struct k_delayed_work *work, uint32_t delay)
{
    return k_delayed_work_submit_to_queue(&g_work_queue_main, work, delay);
}


int k_delayed_work_submit_periodic(struct k_delayed_work *work, s32_t period)
{
    // TODO
    return 0;
}

int k_delayed_work_cancel(struct k_delayed_work *work)
{
    int err = 0;
    int key = irq_lock();

    if (atomic_test_bit(work->work.flags, K_WORK_STATE_PENDING)) {
        err = -EINPROGRESS;
        goto exit;
    }

    if (!work->work_q) {
        err = -EINVAL;
        goto exit;
    }

    k_timer_stop(&work->timer);
    remv_timer_record(work);
    work->work_q = NULL;
    work->timer.timeout = 0;
    work->timer.start_ms = 0;

exit:
    irq_unlock(key);
    return err;
}

s32_t k_delayed_work_remaining_get(struct k_delayed_work *work)
{
    int32_t remain;
    k_timer_t *timer;

    if (work == NULL) {
        return 0;
    }

    timer = &work->timer;
    remain = timer->timeout - (k_now_ms() - timer->start_ms);
    if (remain < 0) {
        remain = 0;
    }
    return remain;
}

void k_delayed_work_del_timer(struct k_delayed_work *work)
{
    if(NULL == work || NULL == work->timer.timer.hdl)
        return;

    k_timer_delete(&work->timer);
    work->timer.timer.hdl = NULL;
}

int k_delayed_work_free(struct k_delayed_work *work)
{
    remv_timer_record(work);
    k_delayed_work_del_timer(work);
    return 0;
}


static void timer_rec_init()
{
    for(int i = 0; i < ARRAY_SIZE(timer_records); i++)
    {
        timer_records[i].timer.hdl = NULL;
        timer_records[i].delay_work = NULL;
    }
}

static int alloc_timer_record(struct k_delayed_work *delay_work)
{
     for(int i = 0; i < ARRAY_SIZE(timer_records); i++)
    {
        if(timer_records[i].timer.hdl == NULL)
        {
            timer_records[i].timer.hdl = delay_work->timer.timer.hdl;
            timer_records[i].delay_work = delay_work;
            return 0;
        }
    }
    return -1;
}

int add_timer_record(struct k_delayed_work *delay_work)
{
    for(int i = 0; i < ARRAY_SIZE(timer_records); i++)
    {
        if(timer_records[i].timer.hdl == delay_work->timer.timer.hdl)
            return 0;
    }

    return alloc_timer_record(delay_work);
}

int remv_timer_record(struct k_delayed_work *delay_work)
{
    for(int i = 0; i < ARRAY_SIZE(timer_records); i++)
    {
        if(timer_records[i].timer.hdl == delay_work->timer.timer.hdl)
        {
            timer_records[i].timer.hdl = NULL;
            break;
        }
    }

    return 0;
}

timer_rec_d *get_timer_record(void *hdl)
{
    for(int i = 0; i < ARRAY_SIZE(timer_records); i++){
        if(hdl == timer_records[i].timer.hdl)
            return &timer_records[i];
    }

    return NULL;
}

#else
static void work_q_main(void *work_q_ptr, void *p2, void *p3)
{
    struct k_work_q *work_q = work_q_ptr;

    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (1) {
        struct k_work *work;
        k_work_handler_t handler;

        work = k_queue_get(&work_q->queue, K_FOREVER);
        if (!work) {
            continue;
        }

        handler = work->handler;

        /* Reset pending state so it can be resubmitted by handler */
        if (atomic_test_and_clear_bit(work->flags,
                          K_WORK_STATE_PENDING)) {
            handler(work);
        }

        /* Make sure we don't hog up the CPU if the FIFO never (or
         * very rarely) gets empty.
         */
        k_yield();
    }
}

void k_work_q_start(struct k_work_q *work_q, k_thread_stack_t *stack,
            size_t stack_size, int prio)
{
    k_queue_init(&work_q->queue, 20);
    k_thread_create(&work_q->thread, stack, stack_size, work_q_main,
            work_q, 0, 0, prio, 0, 0);
    _k_object_init(work_q);
}

#ifdef CONFIG_SYS_CLOCK_EXISTS
static void work_timeout(struct _timeout *t)
{
    struct k_delayed_work *w = CONTAINER_OF(t, struct k_delayed_work,
                           timeout);

    /* submit work to workqueue */
    k_work_submit_to_queue(w->work_q, &w->work);
}

void k_delayed_work_init(struct k_delayed_work *work, k_work_handler_t handler)
{
    k_work_init(&work->work, handler);
    _init_timeout(&work->timeout, work_timeout);
    work->work_q = NULL;

    _k_object_init(work);
}

int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
                   struct k_delayed_work *work,
                   s32_t delay)
{
    unsigned int key = irq_lock();
    int err;

    /* Work cannot be active in multiple queues */
    if (work->work_q && work->work_q != work_q) {
        err = -EADDRINUSE;
        goto done;
    }

    /* Cancel if work has been submitted */
    if (work->work_q == work_q) {
        err = k_delayed_work_cancel(work);
        if (err < 0) {
            goto done;
        }
    }

    /* Attach workqueue so the timeout callback can submit it */
    work->work_q = work_q;

    if (!delay) {
        /* Submit work if no ticks is 0 */
        k_work_submit_to_queue(work_q, &work->work);
    } else {
        /* Add timeout */
        _add_timeout(NULL, &work->timeout, NULL,
                _TICK_ALIGN + _ms_to_ticks(delay));
    }

    err = 0;

done:
    irq_unlock(key);

    return err;
}

int k_delayed_work_cancel(struct k_delayed_work *work)
{
    unsigned int key = irq_lock();

    if (!work->work_q) {
        irq_unlock(key);
        return -EINVAL;
    }

    if (k_work_pending(&work->work)) {
        /* Remove from the queue if already submitted */
        if (!k_queue_remove(&work->work_q->queue, &work->work)) {
            irq_unlock(key);
            return -EINVAL;
        }
    } else {
        _abort_timeout(&work->timeout);
    }

    /* Detach from workqueue */
    work->work_q = NULL;

    atomic_clear_bit(work->work.flags, K_WORK_STATE_PENDING);
    irq_unlock(key);

    return 0;
}
#endif /* CONFIG_SYS_CLOCK_EXISTS */
#endif /* BFLB_BLE */