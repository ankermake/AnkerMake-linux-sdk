#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/input/mt.h>
#include <linux/irq.h>
#include <linux/gpio.h>

#include <linux/regulator/consumer.h>

#include <utils/i2c.h>
#include <utils/gpio.h>

#define I2C_ADDR                    0x38
#define FT6236_EVENT_PRESS_DOWN     0
#define FT6236_EVENT_LIFT_UP        1
#define FT6236_EVENT_CONTACT        2
#define FT6236_EVENT_NO_EVENT       3

#define INPUT_DEV_PHYS      "input/ts"
#define TPD_DRIVER_NAME     "ft6236-ts"

static int int_gpio         = -1;       /* PB17 */
static int reset_gpio       = -1;       /* PB14 */
static int i2c_bus_num      = -1;       /* I2C1 PB15 PB16 */
static int power_gpio       = -1;       /* PA31 */

static int x_coords_max     = -1;
static int y_coords_max     = -1;
static int x_coords_flip    = -1;
static int y_coords_flip    = -1;
static int x_y_coords_exchange  = -1;

static int max_touch_number = -1;

static char *tp_regulator_name = "";

module_param_named(ft6236_regulator_name, tp_regulator_name, charp, 0644);
module_param_named(ft6236_max_touch_number, max_touch_number, int, 0644);

module_param_named(ft6236_x_coords_max, x_coords_max, int, 0644);
module_param_named(ft6236_y_coords_max, y_coords_max, int, 0644);
module_param_named(ft6236_x_coords_flip, x_coords_flip, int, 0644);
module_param_named(ft6236_y_coords_flip, y_coords_flip, int, 0644);
module_param_named(ft6236_x_y_coords_exchange, x_y_coords_exchange, int, 0644);
module_param_named(ft6236_i2c_bus_num , i2c_bus_num, int, 0644);   /* I2C1 PB15 PB16 */

module_param_gpio_named(ft6236_int_gpio, int_gpio, 0644);
module_param_gpio_named(ft6236_reset_gpio, reset_gpio, 0644);
module_param_gpio_named(ft6236_power_gpio, power_gpio, 0644);

struct ft6236_data {
    struct i2c_client *client;
    struct input_dev *input;
    u32 max_x;
    u32 max_y;
    bool invert_x;
    bool invert_y;
    bool swap_xy;
};

struct ft6236_touchpoint {
    union {
        u8 xhi;
        u8 event;
    };
    u8 xlo;
    union {
        u8 yhi;
        u8 id;
    };
    u8 ylo;
};

struct ft6236_packet {
    u8 dev_mode;
    u8 gest_id;
    u8 touches;
    struct ft6236_touchpoint points[2];
};

static struct regulator *ft6236_regulator;
static struct ft6236_data *ft6236;
static struct i2c_client *i2c_client;
static struct work_struct report_work;
static struct workqueue_struct *ft6236_wq;
static const struct i2c_device_id ft6236_tpd_id[] = {{TPD_DRIVER_NAME,0},{}};

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "ft6236_touch: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
        return ret;
    }

    return 0;
}

static inline void m_gpio_free(int gpio)
{
    if (gpio >= 0)
        gpio_free(gpio);
}

static void ft6236_free_gpio(void)
{
    m_gpio_free(int_gpio);
    m_gpio_free(reset_gpio);
    m_gpio_free(power_gpio);

    if (ft6236_regulator)
        regulator_put(ft6236_regulator);
}

static int ft6236_request_gpio(void)
{
    if (strcmp("-1", tp_regulator_name) && strlen(tp_regulator_name)) {
        ft6236_regulator = regulator_get(NULL, tp_regulator_name);
        if(!ft6236_regulator) {
            printk(KERN_ERR "ft6236_touch: ft6236_regulator get err!\n");
            return -EINVAL;
        }
    }

    int ret = m_gpio_request(int_gpio, "ft6236_int_gpio");
    if (ret < 0)
        goto err_int;

    ret = m_gpio_request(reset_gpio, "ft6236_reset_gpio");
    if (ret < 0)
        goto err_reset;

    ret = m_gpio_request(power_gpio, "ft6236_power_enable");
    if(ret < 0)
        goto err_power;

    return 0;

err_power:
    m_gpio_free(reset_gpio);
err_reset:
    m_gpio_free(int_gpio);

err_int:
    if (ft6236_regulator)
        regulator_put(ft6236_regulator);

    return ret;
}

static void ft6236_chip_power(void)
{
    if (ft6236_regulator)
        regulator_enable(ft6236_regulator);

    if (power_gpio >= 0)
        gpio_direction_output(power_gpio, 1);
}

static void ft6236_chip_reset(void)
{
    if (reset_gpio != -1) {
        gpio_direction_output(reset_gpio, 0);
        msleep(50);
        gpio_direction_output(reset_gpio, 1);
        msleep(300);
    }
}


static int ft6236_read(struct i2c_client *client, u8 reg, u8 len, void *data)
{
    int ret = i2c_smbus_read_i2c_block_data(client, reg, len, data);
    if (ret < 0)
        return ret;

    if (ret != len)
        return -EIO;

    return 0;
}

static void ft6236_touch_report(struct work_struct *work)
{
    u8 touches;
    int i, error;
    struct ft6236_packet buf;
    struct input_dev *input = ft6236->input;

    error = ft6236_read(ft6236->client, 0, sizeof(buf), &buf);
    if (error) {
        printk(KERN_ERR "ft6236_touch: read touchdata failed %d\n", error);
        return;
    }

    touches = buf.touches & 0xf;
    if (touches > max_touch_number) {
        printk(KERN_ERR "%d touch points reported, only %d are supported\n", touches, max_touch_number);
        touches = max_touch_number;
    }

    for (i = 0; i < touches; i++) {
        struct ft6236_touchpoint *point = &buf.points[i];
        u16 x = ((point->xhi & 0xf) << 8) | buf.points[i].xlo;
        u16 y = ((point->yhi & 0xf) << 8) | buf.points[i].ylo;
        u8 id = point->id >> 4;

        if (ft6236->invert_x)
            x = ft6236->max_x - x;

        if (ft6236->invert_y)
            y = ft6236->max_y - y;

        input_report_key(input, BTN_TOUCH, 1);
        input_report_abs(input, ABS_MT_TOOL_TYPE, 1);
        input_report_abs(input, ABS_MT_TRACKING_ID, id);
        input_report_abs(input, ABS_MT_PRESSURE, 20);
        input_report_abs(input, ABS_MT_TOUCH_MAJOR, 20);
        input_report_abs(input, ABS_MT_WIDTH_MAJOR, 20);

        input_report_key(input, BTN_TOOL_FINGER, true);

        if (ft6236->swap_xy) {
            input_report_abs(input, ABS_MT_POSITION_X, y);
            input_report_abs(input, ABS_MT_POSITION_Y, x);
        } else {
            input_report_abs(input, ABS_MT_POSITION_X, x);
            input_report_abs(input, ABS_MT_POSITION_Y, y);
        }

        input_mt_sync(input);
    }

    if (touches == 0) {
        input_report_key(input, BTN_TOUCH, 0);
        input_mt_sync(input);
    }

    input_sync(input);
}


static irqreturn_t ft6236_ts_irq_handler(int irq, void *dev_id)
{
    queue_work(ft6236_wq, &report_work);

    return IRQ_HANDLED;
}

static int ft6236_tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    struct input_dev *input;
    struct device *dev = &client->dev;

    ft6236 = kzalloc(sizeof(*ft6236), GFP_KERNEL);
    if (ft6236 == NULL) {
        printk(KERN_ERR "ft6236_touch: alloc GFP_KERNEL memory failed.");
        return -ENOMEM;
    }

    ft6236->client = client;
    ft6236->max_x = x_coords_max;
    ft6236->max_y = y_coords_max;
    ft6236->invert_x = x_coords_flip;
    ft6236->invert_y = y_coords_flip;
    ft6236->swap_xy = x_y_coords_exchange;

    ret = ft6236_request_gpio();
    if (ret < 0)
        goto kfree_ft6236;


    ft6236_chip_power();
    ft6236_chip_reset();

    input = devm_input_allocate_device(dev);
    if (input == NULL) {
        ret = -ENOMEM;
        goto free_gpio;
    }

    ft6236->input = input;
    input->name = client->name;
    input->id.bustype = BUS_I2C;

    __set_bit(EV_SYN, input->evbit);
    __set_bit(EV_KEY, input->evbit);
    __set_bit(EV_ABS, input->evbit);
    __set_bit(BTN_TOUCH, input->keybit);
    __set_bit(INPUT_PROP_DIRECT, input->propbit);

    if (ft6236->swap_xy) {
        input_set_abs_params(input, ABS_MT_POSITION_X, 0, ft6236->max_y, 0, 0);
        input_set_abs_params(input, ABS_MT_POSITION_Y, 0, ft6236->max_x, 0, 0);
    } else {
        input_set_abs_params(input, ABS_MT_POSITION_X, 0, ft6236->max_x, 0, 0);
        input_set_abs_params(input, ABS_MT_POSITION_Y, 0, ft6236->max_y, 0, 0);
    }

    input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, 255, 0, 0);

    input_set_abs_params(input, ABS_MT_TOOL_TYPE, 0, MT_TOOL_MAX, 0, 0);
    input_set_abs_params(input, ABS_MT_PRESSURE, 0, 479, 0, 0);

    INIT_WORK(&report_work, ft6236_touch_report);

    ret = request_irq(gpio_to_irq(int_gpio), ft6236_ts_irq_handler, IRQ_TYPE_EDGE_RISING, client->name, ft6236);
    if (ret) {
        printk(KERN_ERR "ft6236_touch: request IRQ failed!ERRNO:%d.", ret);
        goto free_input;
    }

    ret = input_register_device(input);
    if (ret) {
        printk(KERN_ERR "ft6236_touch: failed to register input device: %d\n", ret);
        goto free_irq;
    }

    return 0;

free_irq:
    free_irq(gpio_to_irq(int_gpio), ft6236);
free_input:
    input_free_device(ft6236->input);
free_gpio:
    ft6236_free_gpio();
kfree_ft6236:
    kfree(ft6236);

    return ret;
}

static int ft6236_tpd_remove(struct i2c_client *client)
{
    free_irq(gpio_to_irq(int_gpio), ft6236);

    ft6236_free_gpio();

    input_unregister_device(ft6236->input);
    input_free_device(ft6236->input);
    kfree(ft6236);

    return 0;
}

static struct i2c_driver ft6236_ts_driver = {
    .driver = {
        .name = TPD_DRIVER_NAME,
    },

    .probe    = ft6236_tpd_probe,
    .remove   = ft6236_tpd_remove,
    .id_table = ft6236_tpd_id,
};

static struct i2c_board_info touch_ft6236_ts_info = {
    .type = TPD_DRIVER_NAME,
    .addr = I2C_ADDR,
};


static int __init ft6236_touch_init(void)
{
    ft6236_wq = create_singlethread_workqueue("ft6236_wq");
    if (ft6236_wq == NULL) {
        printk(KERN_ERR "ft6236_touch: creat workqueue failed.\n");
        return -ENOMEM;
    }

    int ret = i2c_add_driver(&ft6236_ts_driver);
    if (ret) {
        printk(KERN_ERR "ft6236_touch: failed to register i2c driver\n");
        return ret;
    }

    i2c_client = i2c_register_device(&touch_ft6236_ts_info, i2c_bus_num);
    if (i2c_client == NULL) {
        printk(KERN_ERR "ft6236_touch: failed to register i2c device\n");
        i2c_del_driver(&ft6236_ts_driver);
        return -EINVAL;
    }

    return 0;
}

static void __exit ft6236_touch_exit(void)
{
    i2c_unregister_device(i2c_client);

    i2c_del_driver(&ft6236_ts_driver);
    if (ft6236_wq)
        destroy_workqueue(ft6236_wq);
}

module_init(ft6236_touch_init);
module_exit(ft6236_touch_exit);

MODULE_DESCRIPTION("Ft6236 Series Driver");
MODULE_LICENSE("GPL");
