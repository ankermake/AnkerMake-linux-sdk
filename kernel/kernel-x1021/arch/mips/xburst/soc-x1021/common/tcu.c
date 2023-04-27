#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/syscore_ops.h>
#include <irq.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>
#include <linux/gpio.h>

#include <asm/div64.h>

#include <soc/base.h>
#include <soc/extal.h>
#include <soc/gpio.h>
#include <soc/tcu.h>
#include <soc/irq.h>

#include <mach/jztcu.h>

#define TCU_SIZE 0x150
#define regr(off)	readl(jz_tcu->reg + (off))
#define regw(val, off)	writel(val, jz_tcu->reg + (off))

#define tcu_enable_counter(id)	regw(BIT(id), TCU_TESR)
#define tcu_disable_counter(id)	regw(BIT(id), TCU_TECR)
#define tcu_start_clock(id)	regw(BIT(id), TCU_TSCR)
#define tcu_stop_clock(id)	regw(BIT(id), TCU_TSSR)
#define tcu_clear_counter_to_zero(id)	regw(0, CH_TCNT(id));

#define set_tcu_counter_value(id, count_value)	regw(count_value, CH_TCNT(id))

struct jz_pwm_gpio pwm_gpio_array[] = {
#ifdef CONFIG_JZ_PWM_GPIO_PB17
		{ .name = "pwm0", .id = 0, .func = GPIO_FUNC_0, .gpio = GPIO_PB(17), },
#endif
#ifdef CONFIG_JZ_PWM_GPIO_PB18
		{ .name = "pwm1", .id = 1, .func = GPIO_FUNC_0, .gpio = GPIO_PB(18), },
#endif
#ifdef CONFIG_JZ_PWM_GPIO_PC11
		{ .name = "pwm2", .id = 2, .func = GPIO_FUNC_1, .gpio = GPIO_PC(11), },
#endif
#ifdef CONFIG_JZ_PWM_GPIO_PC12
		{ .name = "pwm3", .id = 3, .func = GPIO_FUNC_1, .gpio = GPIO_PC(12), },
#endif
#ifdef CONFIG_JZ_PWM_GPIO_PC13
		{ .name = "pwm4", .id = 4, .func = GPIO_FUNC_1, .gpio = GPIO_PC(13), },
#endif
#ifdef CONFIG_JZ_PWM_GPIO_PC14
		{ .name = "pwm5", .id = 5, .func = GPIO_FUNC_1, .gpio = GPIO_PC(14), },
#endif
#ifdef CONFIG_JZ_PWM_GPIO_PC15
		{ .name = "pwm6", .id = 6, .func = GPIO_FUNC_1, .gpio = GPIO_PC(15), },
#endif
#ifdef CONFIG_JZ_PWM_GPIO_PC16
		{ .name = "pwm7", .id = 7, .func = GPIO_FUNC_1, .gpio = GPIO_PC(16), },
#endif
#ifdef CONFIG_JZ_PWM_GPIO_PC17
		{ .name = "pwm0", .id = 0, .func = GPIO_FUNC_0, .gpio = GPIO_PC(17), },
#endif
#ifdef CONFIG_JZ_PWM_GPIO_PC18
		{ .name = "pwm1", .id = 1, .func = GPIO_FUNC_0, .gpio = GPIO_PC(18), },
#endif
    };
int pwm_gpio_array_size = ARRAY_SIZE(pwm_gpio_array);

struct jz_tcu_t {
	void __iomem	*reg;
	struct mutex	lock;

	unsigned int irq;
	unsigned int irq_base;
};

static struct jz_tcu_t *jz_tcu;

static DECLARE_BITMAP(tcu_map, NR_TCU_CH) = {(1<<NR_TCU_CH) - 1, };

static struct jz_pwm_gpio *tcu_get_pwm_gpio_func_def(int id)
{
	int i;
	struct jz_pwm_gpio *def;

	for (i = 0; i < pwm_gpio_array_size; i++) {
		def = &pwm_gpio_array[i];
		if (def->id == id)
			return def;
	}

	return NULL;
}

static int tcu_pwm_output_enable(int id)
{
	int tmp;
	tmp = regr(CH_TCSR(id));
	regw((tmp | TCSR_PWM_EN), CH_TCSR(id));
	return 0;
}

static void tcu_pwm_output_disable(int id)
{
	int tmp;
	tmp = regr(CH_TCSR(id));
	regw(tmp & (~TCSR_PWM_EN), CH_TCSR(id));
}

static void tcu_shutdown_counter(struct tcu_device *tcu)
{
	if (tcu->pwm_flag)
		tcu_pwm_output_disable(tcu->id); /*disable PWM_EN*/

	tcu_disable_counter(tcu->id);
}

static void set_tcu_full_half_value(int id, unsigned int full_num,
		unsigned int half_num)
{
	regw(full_num, CH_TDFR(id));
	regw(half_num, CH_TDHR(id));
}

static void tcu_set_pwm_shutdown(int id, unsigned int shutdown)/*only use in TCU1_MODE*/
{
	int tmp;
	tmp = regr(CH_TCSR(id));
	if (shutdown)
		regw((tmp | TCSR_PWM_SD), CH_TCSR(id));
	else {
		regw((tmp & (~TCSR_PWM_SD)), CH_TCSR(id));
	}
}

static void tcu_set_start_state(struct tcu_device *tcu)
{
	tcu_disable_counter(tcu->id);
	tcu_start_clock(tcu->id);
	regw(0, CH_TCSR(tcu->id));
	tcu_set_pwm_shutdown(tcu->id, tcu->pwm_shutdown);
}

static void tcu_select_division_ratio(int id, unsigned int ratio)
{
	/*division ratio 1/4/16/256/1024/mask->no internal input clock*/
	int tmp;
	tmp = regr(CH_TCSR(id));
	switch (ratio) {
	case 0: regw((tmp | CSR_DIV1), CH_TCSR(id)); break;
	case 1: regw((tmp | CSR_DIV4), CH_TCSR(id)); break;
	case 2: regw((tmp | CSR_DIV16), CH_TCSR(id)); break;
	case 3: regw((tmp | CSR_DIV64), CH_TCSR(id)); break;
	case 4: regw((tmp | CSR_DIV256), CH_TCSR(id)); break;
	case 5: regw((tmp | CSR_DIV1024), CH_TCSR(id)); break;
	default:
		regw((tmp | CSR_DIV_MSK), CH_TCSR(id)); break;
	}
}

static void tcu_select_clk(int id, int clock)
{
	int tmp;
	tmp = regr(CH_TCSR(id));
	switch (clock) {
	case EXT_EN:  regw((tmp | CSR_EXT_EN), CH_TCSR(id)); break;
	case RTC_EN:  regw((tmp | CSR_RTC_EN), CH_TCSR(id)); break;
	case PCLK_EN: regw((tmp | CSR_PCK_EN), CH_TCSR(id)); break;
	case CLK_MASK:
		regw((tmp & (~CSR_CLK_MSK)), CH_TCSR(id));
	}
}

static void tcu_set_pwm_output_init_level(int id, int level)
{
	int tmp;
	tmp = regr(CH_TCSR(id));
	if (level){
		regw((tmp | TCSR_PWM_HIGH), CH_TCSR(id));
	} else {
		regw((tmp &(~TCSR_PWM_HIGH)), CH_TCSR(id));
	}
}

static void tcu_set_close_state(struct tcu_device *tcu)
{
	tcu_stop_clock(tcu->id);
	tcu_clear_counter_to_zero(tcu->id);

	tcu->clock = CLK_MASK;
	tcu->pwm_flag = 0;
}

int tcu_counter_read(struct tcu_device *tcu)
{
	if ((tcu->id < 0) || (tcu->id > (NR_TCU_CH - 1)))
		return -EINVAL;

	return (regr(CH_TCNT(tcu->id)));
}

/* NOTE: user selete irq mode according irq request */
int tcu_as_timer_config(struct tcu_device *tcu)
{
	mutex_lock(&tcu->tcu_lock);

	if ((tcu->id < 0) || (tcu->id > (NR_TCU_CH - 1))) {
		pr_err("TCU %d isn't in 0~7\n", tcu->id);
		mutex_unlock(&tcu->tcu_lock);
		return -EINVAL;
	}

	if (test_bit(tcu->id, tcu_map)) {
		pr_err("TCU %d been used!\n", tcu->id);
		mutex_unlock(&tcu->tcu_lock);
		return -EBUSY;
	}

	/* tcu->half_num = tcu->full_num, indicated duty = 0 */
	if (tcu->half_num == tcu->full_num) {
		if (tcu->io_func != (tcu->init_level ? GPIO_OUTPUT1 : GPIO_OUTPUT0)) {
			tcu->io_func = tcu->init_level ? GPIO_OUTPUT1 : GPIO_OUTPUT0;
			jz_gpio_set_func(tcu->gpio_def->gpio, tcu->io_func);
		}
	} else if (tcu->half_num == 0) {  /* tcu->half_num = 0, indicated duty = 100% */
		if (tcu->io_func != (tcu->init_level ? GPIO_OUTPUT0 : GPIO_OUTPUT1)) {
			tcu->io_func = tcu->init_level ? GPIO_OUTPUT0 : GPIO_OUTPUT1;
			jz_gpio_set_func(tcu->gpio_def->gpio, tcu->io_func);
		}
	} else {
		if (tcu->io_func != tcu->gpio_def->func) {
			tcu->io_func = tcu->gpio_def->func;
			jz_gpio_set_func(tcu->gpio_def->gpio, tcu->io_func);
		}

		if (tcu->init_level != tcu->init_level_reg) {
			tcu->init_level_reg = tcu->init_level;
			tcu_set_pwm_output_init_level(tcu->id, tcu->init_level_reg);
		}
	}

	tcu_disable_counter(tcu->id);
	set_tcu_full_half_value(tcu->id, tcu->full_num, tcu->half_num);
	tcu_select_division_ratio(tcu->id, tcu->divi_ratio);
	tcu_enable_counter(tcu->id);

	mutex_unlock(&tcu->tcu_lock);

	return 0;
}

int tcu_as_timer_init(struct tcu_device *tcu)
{
	mutex_lock(&tcu->tcu_lock);
	if ((tcu->id < 0) || (tcu->id > (NR_TCU_CH - 1))) {
		pr_err("TCU %d isn't in 0~7\n", tcu->id);
		mutex_unlock(&tcu->tcu_lock);
		return -EINVAL;
	}

	tcu_set_start_state(tcu);
	tcu_select_clk(tcu->id, tcu->clock);
	set_tcu_counter_value(tcu->id, tcu->count_value);

	mutex_unlock(&tcu->tcu_lock);
	return 0;
}

struct tcu_device *tcu_request(int channel_num)
{
	struct tcu_device *tcu;

	tcu = kzalloc(sizeof(struct tcu_device), GFP_KERNEL);
	if(tcu == NULL) {
		pr_err("kzalloc tcu_device failed\n");
		return ERR_PTR(-ENOMEM);
	}

	mutex_lock(&jz_tcu->lock);
	if ((channel_num < 0) || (channel_num > (NR_TCU_CH - 1))) {
		mutex_unlock(&jz_tcu->lock);
		kfree(tcu);
		return ERR_PTR(-ENODEV);
	}
	if (!test_bit(channel_num, tcu_map)) {
		mutex_unlock(&jz_tcu->lock);
		kfree(tcu);
		return ERR_PTR(-EBUSY);
	}

	tcu->id = channel_num;
	tcu->gpio_def = tcu_get_pwm_gpio_func_def(tcu->id);
	if (!tcu->gpio_def) {
		pr_err("TCU get pwm%d IO default function failed\n", tcu->id);
		goto tcu_err;
	}
	tcu->io_func = -1;
	tcu->init_level_reg = -1;

	if(gpio_request(tcu->gpio_def->gpio, tcu->gpio_def->name)) {
		pr_err("TCU request pwm%d IO failed\n", tcu->id);
		goto tcu_err;
    }

	pr_debug("request TCU channel number:%d\n", channel_num);

	mutex_init(&(tcu->tcu_lock));
	clear_bit(channel_num, tcu_map);
	mutex_unlock(&jz_tcu->lock);
	return tcu;

tcu_err:
	mutex_unlock(&jz_tcu->lock);
	kfree(tcu);
	return ERR_PTR(-ENODEV);
}

int tcu_enable(struct tcu_device *tcu)
{
	int ret = 0;
	mutex_lock(&tcu->tcu_lock);

	if (tcu->pwm_flag)
		ret = tcu_pwm_output_enable(tcu->id);

	tcu_enable_counter(tcu->id);

	mutex_unlock(&tcu->tcu_lock);
	return ret;
}

void tcu_disable(struct tcu_device *tcu)
{
	mutex_lock(&tcu->tcu_lock);
	if (test_bit(tcu->id, tcu_map)) {
		mutex_unlock(&tcu->tcu_lock);
		return;
	}

	tcu_shutdown_counter(tcu);

	mutex_unlock(&tcu->tcu_lock);
}

void tcu_free(struct tcu_device *tcu)
{
	mutex_lock(&tcu->tcu_lock);
	tcu_set_close_state(tcu);
	set_bit(tcu->id, tcu_map);
	gpio_free(tcu->gpio_def->gpio);
	mutex_unlock(&tcu->tcu_lock);
	kfree(tcu);
}

static void jz_tcu_irq_mask(struct irq_data *data)
{
	struct jz_tcu_t *tcu = irq_data_get_irq_chip_data(data);
	unsigned int bit_tmp;
	bit_tmp = data->irq - tcu->irq_base;
	if (bit_tmp >= 8) {
		bit_tmp += 8;
	}

	regw(BIT(bit_tmp), TCU_TMSR);
}

static void jz_tcu_irq_unmask(struct irq_data *data)
{
	struct jz_tcu_t *tcu = irq_data_get_irq_chip_data(data);
	unsigned int bit_tmp;

	bit_tmp = data->irq - tcu->irq_base;
	if (bit_tmp >= 8) {
		bit_tmp += 8;
	}

	regw(BIT(bit_tmp), TCU_TMCR);
}

static void jz_tcu_irq_ack(struct irq_data *data)
{
	struct jz_tcu_t *tcu = irq_data_get_irq_chip_data(data);
	unsigned int bit_tmp = data->irq - tcu->irq_base;
	if (bit_tmp >= 8) {
		bit_tmp += 8;
	}

	regw(BIT(bit_tmp), TCU_TFCR);
}

static struct irq_chip jz_tcu_irq_chip = {
	.name = "jz-tcu",
	.irq_mask = jz_tcu_irq_mask,
	.irq_disable = jz_tcu_irq_mask,
	.irq_unmask = jz_tcu_irq_unmask,
	.irq_ack = jz_tcu_irq_ack,
};

static void jz_tcu_irq_demux(unsigned int irq, struct irq_desc *desc)
{
	struct jz_tcu_t *tcu = irq_desc_get_handler_data(desc);
	unsigned int flags;
	unsigned int mask;
	unsigned int i;

	flags = regr(TCU_TFR);
	mask = ~regr(TCU_TMR);

	for (i = 0; i < 8; i++) {
		if ((mask & BIT(i)) && (flags & BIT(i))) {
			generic_handle_irq(tcu->irq_base + i);
		}
	}

	for (i = 8; i < 16; i++) {
		if ((mask & BIT(i + 8)) && (flags & BIT(i + 8))) {
			generic_handle_irq(tcu->irq_base + i);
		}
	}

}

int __init tcu_init(void)
{
	unsigned int irq;
	jz_tcu = (struct jz_tcu_t *)kzalloc(sizeof(struct jz_tcu_t),
			GFP_KERNEL);
	jz_tcu->reg = ioremap(TCU_IOBASE, TCU_SIZE);
	mutex_init(&jz_tcu->lock);
	jz_tcu->irq = IRQ_TCU2;
	jz_tcu->irq_base = IRQ_TCU_BASE;

	for (irq = IRQ_TCU_BASE; irq <= IRQ_TCU_END; ++irq) {
		irq_set_chip_data(irq, jz_tcu);
		irq_set_chip_and_handler(irq, &jz_tcu_irq_chip,
				handle_level_irq);
	}

	irq_set_handler_data(jz_tcu->irq, jz_tcu);
	irq_set_chained_handler(jz_tcu->irq, jz_tcu_irq_demux);

	return 0;
}
arch_initcall(tcu_init);
