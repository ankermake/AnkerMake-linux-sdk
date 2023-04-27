#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <si1143.h>
#include "si1143_priv.h"

#define DEVICE_NAME     "si1143"

static bool     platform_si1143_probed = false;
static int      platform_si1143_gpio_number;
static int      platform_si1143_i2c_bus_number;
static struct   i2c_client * si1143_i2c_client = NULL;

static DEFINE_MUTEX(si1143_lock);
static uint8_t si1143_read_emit_power(struct device * dev);


struct si1143_data 
{
    struct i2c_client             * client;
    struct input_dev              * input;
    int                             rsp_seq;
    int                             remove;
    struct workqueue_struct       * wqueue;
    struct work_struct              work_irq;
    struct delayed_work             work_check;
    uint16_t                        ps1_th;
    uint16_t                        ps2_th;
    bool                            ps1_closed;
    bool                            ps2_closed;
};

static uint8_t hw_si1143_hw_reset_registers[] =
{
    REG_COMMAND,    CMD_RESET,
    REG_HW_KEY,     0x17,
};

static uint8_t hw_si1143_init_registers[] = 
{
    REG_IRQ_ENABLE, 0x0C,
    REG_IRQ_MODE,   0x50,
    REG_INT_CFG,    0x01,
    REG_MEAS_RATE,  0x84,
    REG_PS_RATE,    0x08,
};

static uint8_t hw_si1143_init_commands[] = 
{
    CMD_PS_AUTO,
};

static uint8_t hw_si1143_init_params[] = 
{
    PARAM_PS_ADC_MISC,    0x04, // 室内模式
    // PARAM_PS_ADC_MISC,    0x24, // 室外模式
    PARAM_PS_ADC_GAIN,    0x00,
    PARAM_PS_ADC_COUNTER, 0x06 << 4,
    PARAM_CHLIST,         0x03,
    PARAM_PSLED12_SELECT, 0x21,
};

static int __si1143_command_reset(struct si1143_data *data)
{
    struct device *dev = &data->client->dev;
    unsigned long stop_jiffies;
    int ret;

    ret = i2c_smbus_write_byte_data(data->client, REG_COMMAND,
                              CMD_NOP);
    if (ret < 0)
        return ret;
    msleep(MINSLEEP_MS);

    stop_jiffies = jiffies + TIMEOUT_MS * HZ / 1000;
    while (true) {
        ret = i2c_smbus_read_byte_data(data->client,
                           REG_RESPONSE);
        if (ret <= 0)
            return ret;
        if (time_after(jiffies, stop_jiffies)) {
            dev_warn(dev, "timeout on reset\n");
            return -ETIMEDOUT;
        }
        msleep(MINSLEEP_MS);
        continue;
    }
}

static int si1143_command(struct si1143_data *data, u8 cmd)
{
    struct device *dev = &data->client->dev;
    unsigned long stop_jiffies;
    int ret;

    if (data->rsp_seq < 0) {
        ret = __si1143_command_reset(data);
        if (ret < 0) {
            dev_err(dev, "failed to reset command counter, ret=%d\n",
                ret);
            goto out;
        }
        data->rsp_seq = 0;
    }

    ret = i2c_smbus_write_byte_data(data->client, REG_COMMAND, cmd);
    if (ret) {
        dev_warn(dev, "failed to write command, ret=%d\n", ret);
        goto out;
    }
    /* Sleep a little to ensure the command is received */
    msleep(MINSLEEP_MS);

    stop_jiffies = jiffies + TIMEOUT_MS * HZ / 1000;
    while (true) {
        ret = i2c_smbus_read_byte_data(data->client,
                           REG_RESPONSE);
        if (ret < 0) {
            dev_warn(dev, "failed to read response, ret=%d\n", ret);
            break;
        }

        if ((ret & ~RSP_COUNTER_MASK) == 0) {
            if (ret == data->rsp_seq) {
                if (time_after(jiffies, stop_jiffies)) {
                    dev_warn(dev, "timeout on command %#02hhx\n",
                         cmd);
                    ret = -ETIMEDOUT;
                    break;
                }
                msleep(MINSLEEP_MS);
                continue;
            }
            if (ret == ((data->rsp_seq + 1) &
                RSP_COUNTER_MASK)) {
                data->rsp_seq = ret;
                ret = 0;
                break;
            }
            dev_warn(dev, "unexpected response counter %d instead of %d\n",
                 ret, (data->rsp_seq + 1) & RSP_COUNTER_MASK);
            ret = -EIO;
        } else {
            if (ret == RSP_INVALID_SETTING) {
                dev_warn(dev, "INVALID_SETTING error on command %#02hhx\n",
                     cmd);
                ret = -EINVAL;
            } else {
                /* All overflows are treated identically */
                dev_dbg(dev, "overflow, ret=%d, cmd=%#02hhx\n",
                    ret, cmd);
                ret = -EOVERFLOW;
            }
        }

        /* Force a counter reset next time */
        data->rsp_seq = -1;
        break;
    }

out:
    return ret;
}

static int si1143_config_table(struct si1143_data * data, int(*config_func)(struct si1143_data *, const uint8_t *), const uint8_t * start, size_t totalsize, int onesize, int sleep_ms)
{
    int ret;
    const uint8_t * end = start + totalsize / sizeof(uint8_t);
    while (start < end) {
        ret = config_func(data, start);
        if (ret < 0) {
            return ret;
        }
        if (sleep_ms) {
            msleep(sleep_ms);
        }
        start += onesize;
    }
    return 0;
}

static int si1143_config_set_register(struct si1143_data * data, const uint8_t * param)
{
    uint8_t reg, val;
    reg = param[0];
    val = param[1];
    dev_info(&data->client->dev, "config reg: 0x%02x val: 0x%02x\n", reg, val);
    return i2c_smbus_write_byte_data(data->client, reg, val);
}

static int si1143_config_send_command(struct si1143_data * data, const uint8_t * param)
{
    uint8_t cmd = *param;
    dev_info(&data->client->dev, "config cmd: 0x%02x\n", cmd);
    return si1143_command(data, cmd);
}

static int si1143_config_set_param(struct si1143_data * data, const uint8_t * param)
{
    int ret;
    uint8_t par, val;
    par = param[0];
    val = param[1];
    if ((par & 0x1f) != par) {
        dev_err(&data->client->dev, "bad param: 0x%02x\n", par);
        return -EINVAL;
    }
    dev_info(&data->client->dev, "config param: 0x%02x val: 0x%02x\n", par, val);
    ret = i2c_smbus_write_byte_data(data->client, REG_PARAM_WR, val);
    if (ret < 0) {
        return ret;
    }
    return si1143_command(data, CMD_PARAM_SET | par);
}


static int si1143_sample_ps(struct si1143_data * data, uint16_t ps_data[2]) {
    int ret;
    uint8_t buf[4];
    ret = i2c_smbus_read_i2c_block_data(data->client, REG_PS1_DATA, 4, buf);
    if (ret < 0) {
        return ret;
    }
    ps_data[0] = buf[0] | (buf[1] << 8);
    ps_data[1] = buf[2] | (buf[3] << 8);
    return 0;
}

#if 0
static int si1143_sample_force(struct si1143_data * data, uint16_t sample[2]) {
    int ret;
    ret = si1143_command(data, CMD_PS_FORCE);
    if (ret < 0) {
        return ret;
    }
    return si1143_sample_ps(data, sample);
}
#endif

static int si1143_hw_init(struct si1143_data * data)
{
    int ret;

    uint8_t emit_power = si1143_read_emit_power(&data->client->dev);

    uint8_t hw_si1143_threshold_registers[] = {
        REG_PS_LED21,   (emit_power << 4) | emit_power,
        REG_PS1_TH0,    data->ps1_th & 0xff,
        REG_PS1_TH1,    data->ps1_th >> 8,
        REG_PS2_TH0,    data->ps2_th & 0xff,
        REG_PS2_TH1,    data->ps2_th >> 8,
    };
    
    ret = si1143_config_table(data, si1143_config_set_register, hw_si1143_hw_reset_registers, sizeof(hw_si1143_hw_reset_registers), 2, TIMEOUT_MS);
    if (ret < 0) {
        return ret;
    }
    ret = si1143_config_table(data, si1143_config_set_register, hw_si1143_threshold_registers, sizeof(hw_si1143_threshold_registers), 2, 0);
    if (ret < 0) {
        return ret;
    }
    ret = si1143_config_table(data, si1143_config_set_register, hw_si1143_init_registers, sizeof(hw_si1143_init_registers), 2, 0);
    if (ret < 0) {
        return ret;
    }
    ret = si1143_config_table(data, si1143_config_set_param, hw_si1143_init_params, sizeof(hw_si1143_init_params), 2, 0);
    if (ret < 0) {
        return ret;
    }
    ret = si1143_config_table(data, si1143_config_send_command, hw_si1143_init_commands, sizeof(hw_si1143_init_commands), 1, 0);
    if (ret < 0) {
        return ret;
    }
    data->rsp_seq = 0;
    return 0;
}

static int si1143_check(struct si1143_data * data) 
{
    return 0x01 == i2c_smbus_read_byte_data(data->client, REG_INT_CFG);
}

static void si1143_check_work_entry(struct work_struct * ws)
{
    struct si1143_data * data = container_of(ws, struct si1143_data, work_check.work);
    if (si1143_check(data)) {
        dev_info(&data->client->dev, "check passed\n");
    } else {
        dev_err(&data->client->dev, "check failed, reinit si1143\n");
        // disable_irq(data->client->irq);
        si1143_hw_init(data);
        // enable_irq(data->client->irq);
    }
    queue_delayed_work(data->wqueue, &data->work_check, msecs_to_jiffies(5000));
}

static void si1143_irq_work_entry(struct work_struct * ws)
{
    uint16_t ps_data[2];
    struct si1143_data * data = container_of(ws, struct si1143_data, work_irq);
    int status;

    if (data->remove) {
        goto out;
    }
    
    status = i2c_smbus_read_byte_data(data->client, REG_IRQ_STATUS);
    if (status < 0) {
        dev_err(&data->client->dev, "read IRQ_STATUS failed\n");
    } else {
        if (status & 0x0C) {
            if (si1143_sample_ps(data, ps_data) == 0) {
                bool closed;
                if (status & 0x04) {
                    // PS1
                    closed = ps_data[0] > data->ps1_th;
                    if (closed != data->ps1_closed) {
                        data->ps1_closed = closed;
                        input_report_key(data->input, KEY_1, closed);
                    }
                }
                if (status & 0x08) {
                    // PS2
                    closed = ps_data[1] > data->ps2_th;
                    if (closed != data->ps2_closed) {
                        data->ps2_closed = closed;
                        input_report_key(data->input, KEY_2, closed);
                    }
                }
                input_sync(data->input);
            } else {
                dev_err(&data->client->dev, "sample failed\n");
            }
            i2c_smbus_write_byte_data(data->client, REG_IRQ_STATUS, status);
        } else {
            dev_err(&data->client->dev, "ingore IRQ_STATUS: 0x%02x\n", status);
        }
    }

out:
    enable_irq(data->client->irq);
}

static irqreturn_t si1143_handler(int irq, void * params)
{
    struct si1143_data * data = params;
    disable_irq_nosync(irq);
    queue_work(data->wqueue, &data->work_irq);
    return IRQ_HANDLED;
}

static int driver_file_read(const char * filename, unsigned char *data, unsigned int size)
{
    ssize_t ret;
    mm_segment_t oldfs;
    struct file * file;
    loff_t pos;

    file = filp_open(filename, O_RDONLY, 0);
    if (IS_ERR(file)) {
        return PTR_ERR(file);
    }
    pos = file->f_pos;
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    ret = vfs_read(file, data, size, &pos);
    set_fs(oldfs);
    filp_close(file, NULL);

    return (int)ret;
}

static uint8_t si1143_read_emit_power(struct device * dev)
{
    static const char * filename = "/usr/data/si1143-emit";
    const uint8_t default_power = 0xa;
    int ret;
    
    char buf[32];

    ret = driver_file_read(filename, buf, sizeof(buf) - 1);
    if (ret < 0) {
        dev_err(dev, "%s format error\n", filename);
        return default_power;
    }
    buf[ret] = 0;
    if (sscanf(buf, "%d", &ret) == 1 && 0 < ret && ret < 16) {
        return (uint8_t)ret;
    } else {
        dev_err(dev, "%s format error\n", filename);
        return default_power;
    }
}

static int si1143_read_threshold(struct device * dev, uint16_t * ps1_th, uint16_t * ps2_th)
{
    int ret = 0;
    unsigned int a, b;
    char buf[32];
    static const char * filename = "/usr/data/si1143-threshold";

    ret = driver_file_read(filename, buf, sizeof(buf) - 1);
    if (ret < 0) {
        dev_err(dev, "%s read failed\n", filename);
        return ret;
    }
    buf[ret] = 0;
    if (sscanf(buf, "%u,%u\n", &a, &b) != 2) {
        dev_err(dev, "%s format error\n", filename);
        ret = -EINVAL;
        goto end;
    }
    if (a > 0xffff || b > 0xffff) {
        dev_err(dev, "threshold is greater than 0xffff\n");
        ret = -EINVAL;
        goto end;
    }
    printk("%d,%s\n", __LINE__, __FUNCTION__);
    *ps1_th = (uint16_t)a;
    *ps2_th = (uint16_t)b;
end:
    return ret;
}

static int si1143_probe(struct i2c_client *client,
            const struct i2c_device_id *id)
{
    int ret;
    int part_id, rev_id, seq_id;
    struct si1143_data * data = NULL;
    uint16_t ps1_th , ps2_th;
    
    printk("%d,%s\n", __LINE__, __FUNCTION__);
    ret = si1143_read_threshold(&client->dev, &ps1_th, &ps2_th);
    if (ret < 0) {
        return ret;
    }

    part_id = i2c_smbus_read_byte_data(client, REG_PART_ID);
    if (part_id < 0)  {
        return part_id;
    }
    rev_id  = i2c_smbus_read_byte_data(client, REG_REV_ID);
    if (rev_id < 0) {
        return rev_id;
    }
    seq_id  = i2c_smbus_read_byte_data(client, REG_SEQ_ID);
    if (seq_id < 0) {
        return seq_id;
    }

    dev_info(&client->dev, "device part_id %02x rev %02x seq %02x\n",
            part_id, rev_id, seq_id);
    if (part_id != SI1143_PART_ID && part_id != SI1142_PART_ID) {
        dev_err(&client->dev, "part ID mismatch got %#02hhx, expected %#02x\n",
                part_id, SI1143_PART_ID);
        return -ENODEV;
    }

    data = kzalloc(sizeof(*data), 0);
    if (!data) {
        return -ENOMEM;
    }
    data->client = client;
    i2c_set_clientdata(client, data);
    data->ps1_th = ps1_th;
    data->ps2_th = ps2_th;
    ret = si1143_hw_init(data);
    if (ret < 0) {
        dev_err(&client->dev, "si1143_hw_init failed\n");
        goto free_data;
    }

    INIT_WORK(&data->work_irq, si1143_irq_work_entry);
    INIT_DELAYED_WORK(&data->work_check, si1143_check_work_entry);

    data->wqueue = create_workqueue("si1143_wq");
    if (!data->wqueue) {
        ret = -ENOMEM;
        dev_err(&client->dev, "create workqueue failed\n");
        goto free_data;
    }

    data->input = input_allocate_device();
    if (!data->input) {
        ret = -ENOMEM;
        dev_err(&client->dev, "allocate input device failed\n");
        goto free_wq;
    }
    data->input->name = "si1143";
    set_bit(EV_KEY, data->input->evbit);
    set_bit(KEY_1, data->input->keybit);
    set_bit(KEY_2, data->input->keybit);
    
    ret = input_register_device(data->input);
    if (ret < 0) {
        dev_err(&client->dev, "register input device failed\n");
        goto free_input;
    }
    ret = request_irq(client->irq,
              si1143_handler,
              IRQF_TRIGGER_LOW,
              "si1143_irq",
              data);
    
              
    if (ret < 0) {
        dev_err(&client->dev, "irq request failed\n");
        goto unregister_input;
    }
    queue_delayed_work(data->wqueue, &data->work_check, msecs_to_jiffies(5000));
    dev_info(&client->dev, "probe finish\n");
    return 0;
unregister_input:
    input_unregister_device(data->input);
free_input:
    input_free_device(data->input);
free_wq:
    destroy_workqueue(data->wqueue);
free_data:
    kfree(data);
    return ret;
}

static int si1143_remove(struct i2c_client *client)
{
    struct si1143_data * data = i2c_get_clientdata(client);
    data->remove = 1;

    cancel_delayed_work_sync(&data->work_check);
    cancel_work_sync(&data->work_irq);
    destroy_workqueue(data->wqueue);
    free_irq(client->irq, data);
    input_unregister_device(data->input);
    input_free_device(data->input);
    kfree(data);
    si1143_i2c_client = NULL;
    return 0;
}

static int si1143_register_device_nolock(void)
{
    struct i2c_adapter *adap;
    struct i2c_board_info info;
    struct i2c_client *client;
    int res, gpio;

    
    if (!platform_si1143_probed) {
        pr_err("no platform-si1143 found\n");
        return -ENODEV;
    }

    if (si1143_i2c_client) {
        pr_warn("device was register\n");
        return -EBUSY;
    }

    gpio = platform_si1143_gpio_number;
    adap = i2c_get_adapter(platform_si1143_i2c_bus_number);
    if (!adap) {
        res = -ENODEV;
        goto err;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    strcpy(info.type, DEVICE_NAME);

    info.addr = 0x5a;
    res = gpio_request(gpio, "SI1143_INT");
    if (res < 0) {
        pr_err("request gpio failed\n");
        goto put_adapter;
    }
    gpio_direction_input(gpio);
    res = gpio_to_irq(gpio);
    if (res < 0) {
        pr_err("gpio_to_irq failed\n");
        goto free_gpio;
    }
    info.irq = res;
    client = i2c_new_device(adap, &info);
    if (!client) {
        res = -EINVAL;
        goto free_gpio;
    }
    i2c_put_adapter(adap);
    si1143_i2c_client = client;
    return 0;

free_gpio:
    gpio_free(gpio);
put_adapter:
    i2c_put_adapter(adap);
err:
    return res;
}

static int si1143_unregister_device_nolock(void)
{
    if (!si1143_i2c_client) {
        pr_err("device was not register\n");
        return -EBUSY;
    }
    i2c_unregister_device(si1143_i2c_client);
    gpio_free(platform_si1143_gpio_number);
    si1143_i2c_client = NULL;
    return 0;
}


static int si1143_register_device(void)
{
    int ret;
    mutex_lock(&si1143_lock);
    ret = si1143_register_device_nolock();
    mutex_unlock(&si1143_lock);
    return ret; 
}

static int si1143_unregister_device(void)
{
    int ret;
    mutex_lock(&si1143_lock);
    ret = si1143_unregister_device_nolock();
    mutex_unlock(&si1143_lock);
    return ret; 
}


static ssize_t si1143_sysfs_register_device(struct device_driver *drv, const char *buf, size_t count) {
    int res = si1143_register_device();
    if (res < 0) {
        return res;
    }
    return count;
}

static ssize_t si1143_sysfs_unregister_device(struct device_driver *drv, const char *buf, size_t count) {
    int res = si1143_unregister_device();
    if (res < 0) {
        return res;
    }
    return count;
}

static DRIVER_ATTR(register,   S_IWUGO, NULL, si1143_sysfs_register_device);
static DRIVER_ATTR(unregister, S_IWUGO, NULL, si1143_sysfs_unregister_device);

static struct attribute *si1143_attrs[] = {
    &driver_attr_register.attr,
    &driver_attr_unregister.attr,
    NULL,
};

static struct attribute_group si1143_attr_group = {
    .attrs = si1143_attrs,
};

static const struct attribute_group *si1143_attr_groups[] = {
    &si1143_attr_group,
    NULL,
};

static const struct i2c_device_id si1143_ids[] = {
    { DEVICE_NAME, 0 },
    {}
};

static struct i2c_driver si1143_driver = {
    .driver = {
        .name   = "si1143",
        .groups = si1143_attr_groups,
    },
    .probe  = si1143_probe,
    .remove = si1143_remove,
    .id_table = si1143_ids,
};

static int si1143_platform_driver_probe(struct platform_device *dev) {
    int res = 0;
    struct si1143_platform_device *si1143 = (struct si1143_platform_device *)dev;
    mutex_lock(&si1143_lock);
    if (platform_si1143_probed) {
        dev_err(&dev->dev, "si1143 was probed\n");
        res = -EBUSY;
        goto out;
    }
    platform_si1143_probed = true;
    platform_si1143_i2c_bus_number = si1143->i2c;
    platform_si1143_gpio_number = si1143->gpio;
out:
    mutex_unlock(&si1143_lock);
    return res;
}

static struct platform_driver si1143_platform_driver = {
	.driver = {
		.name = SI1143_PLATFORM_NAME,
	},
};

module_platform_driver_probe(si1143_platform_driver, si1143_platform_driver_probe);

module_i2c_driver(si1143_driver);
MODULE_LICENSE("GPL");
