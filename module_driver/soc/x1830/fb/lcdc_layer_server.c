#include <common.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include "lcdc_layer.h"

static DEFINE_MUTEX(lock);
static DECLARE_WAIT_QUEUE_HEAD(waiter);

struct layer_data {
    struct lcdc_layer cfg;
    wait_queue_head_t waiter;
    struct mutex lock;
    int is_update;
    int is_enable;
    int is_sync;
};

static struct layer_data layers[2];

static int frame_index = -1;
static int is_update;
static int is_stop;

static struct lcdc_layer *get_layer_cfg(struct layer_data *layer)
{
    if (layer->is_update) {
        is_update--;
        layer->is_update = 0;
        wake_up_all(&layer->waiter);
    }

    if (layer->is_enable == -1)
        layer->is_enable = 0;

    return &layer->cfg;
}

static int layer_pan_display_thread(void *data)
{
    while (1) {
        while (!is_update && !is_stop) {
            wait_event_timeout(waiter, is_update || is_stop, msecs_to_jiffies(20));
        }

        if (is_stop)
            break;

        mutex_lock(&lock);
        frame_index = (frame_index + 1) % 3;
        struct lcdc_layer *cfg0 = get_layer_cfg(&layers[0]);
        struct lcdc_layer *cfg1 = get_layer_cfg(&layers[1]);
        enum lcdc_layer_order save_order0;
        int save_alpha0;
        enum lcdc_layer_order save_order1;
        int save_alpha1;

        /*
         * 注意:控制器BUG
         *   当一个layer使能时,使能的layer必须在bottom,不使能的必须在top
         *   并且alpha设置为不使能
         */
        if (cfg0->layer_enable != cfg1->layer_enable) {
            save_order0 = cfg0->layer_order;
            save_alpha0 = cfg0->alpha.enable;
            save_order1 = cfg1->layer_order;
            save_alpha1 = cfg1->alpha.enable;

            if (cfg0->layer_enable) {
                cfg0->layer_order = lcdc_layer_bottom;
                cfg1->layer_order = lcdc_layer_top;
            } else {
                cfg1->layer_order = lcdc_layer_bottom;
                cfg0->layer_order = lcdc_layer_top;
            }

            cfg0->alpha.enable = 0;
            cfg1->alpha.enable = 0;
        }

        lcdc_config_layer(frame_index, 0, cfg0);

        lcdc_config_layer(frame_index, 1, cfg1);

        if (cfg0->layer_enable != cfg1->layer_enable) {
            cfg0->layer_order = save_order0;
            cfg0->alpha.enable = save_alpha0;
            cfg1->layer_order = save_order1;
            cfg1->alpha.enable = save_alpha1;
        }
        mutex_unlock(&lock);

        lcdc_pan_display(frame_index);

        if (layers[0].is_sync) {
            layers[0].is_sync = 0;
            wake_up_all(&layers[0].waiter);
        }

        if (layers[1].is_sync) {
            layers[1].is_sync = 0;
            wake_up_all(&layers[1].waiter);
        }
    }

    is_stop = 0;
    wake_up_all(&waiter);

    return 0;
}

void lcdc_layer_server_update(unsigned int layer_id, struct lcdc_layer *cfg, int sync)
{
    assert(layer_id < 2);
    assert(cfg);

    struct layer_data *layer = &layers[layer_id];

    mutex_lock(&layer->lock);

    mutex_lock(&lock);

    while (layer->is_update) {
        mutex_unlock(&lock);
        wait_event_interruptible(layer->waiter, !layer->is_update);
        mutex_lock(&lock);
    }

    is_update++;
    layer->is_update = 1;
    layer->is_enable = 1;
    layer->is_sync = sync;
    layer->cfg = *cfg;

    mutex_unlock(&lock);

    wake_up_all(&waiter);

    wait_event_interruptible(layer->waiter, !layer->is_sync);

    mutex_unlock(&layer->lock);
}

void lcdc_layer_server_disable(unsigned int layer_id, int sync)
{
    assert(layer_id < 2);

    struct layer_data *layer = &layers[layer_id];

    mutex_lock(&layer->lock);

    mutex_lock(&lock);

    if (!layer->cfg.layer_enable) {
        mutex_unlock(&lock);
        mutex_unlock(&layer->lock);
        return;
    }

    while (layer->is_update) {
        mutex_unlock(&lock);
        wait_event_interruptible(layer->waiter, !layer->is_update);
        mutex_lock(&lock);
    }

    is_update++;
    layer->is_update = 1;
    layer->is_enable = -1;
    layer->is_sync = sync;
    layer->cfg.layer_enable = 0;

    mutex_unlock(&lock);

    wake_up_all(&waiter);

    wait_event_interruptible(layer->waiter, !layer->is_sync);

    mutex_unlock(&layer->lock);
}

static void init_layer(struct layer_data *layer)
{
    memset(layer, 0, sizeof(*layer));
    mutex_init(&layer->lock);
    init_waitqueue_head(&layer->waiter);
}

void lcdc_layer_server_init(void)
{
    init_layer(&layers[0]);
    init_layer(&layers[1]);

    /* 默认关闭所有的layer
     */
    lcdc_enable_layer(0, 0, 0);
    lcdc_enable_layer(1, 1, 0);

    is_stop = 0;
    kthread_run(layer_pan_display_thread, NULL, "jzfb layer pan pisplay");
}

void lcdc_layer_server_exit(void)
{
    is_stop = 1;
    wake_up_all(&waiter);

    wait_event(waiter, is_stop == 0);
}
