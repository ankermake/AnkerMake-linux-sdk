#include <common.h>
#include <tcsm.h>
#include <timer.h>
#include <debug.h>

/*
 * 0xf4001b00 --- req
 * 0xf4001b10 --- sda_pin | scl_pin | udelay | retries
 * 0xf4001b20 --- num | addr[0] | flags[0] | len[0] | buf[0]
 *                | addr [1] | flags[1] | len[1] | buf[1] | ...
 */

struct i2c_jz {
	unsigned int sda_pin;
	unsigned int scl_pin;
	int udelay;
	int retries;
} i2c;

struct i2c_msg {
	unsigned short addr;
	unsigned short flags;
	unsigned short len;
	unsigned int buf;
} pmsg;

static void gpio_set_value(int port, int pin, int value)
{
	if (value)
		writel(1 << pin, GPIO_IOBASE + port * GPIO_PORT_OFF + PXPAT0S);
	else
		writel(1 << pin, GPIO_IOBASE + port * GPIO_PORT_OFF + PXPAT0C);
}

static int gpio_get_value(int port, int pin)
{
	return (readl(GPIO_IOBASE + port * GPIO_PORT_OFF + PXPIN)
			& (1 << pin)) ? 1 : 0;
}

static void gpio_direction_input(int port, int pin)
{
	writel(1 << pin, GPIO_IOBASE + port * GPIO_PORT_OFF + PXINTC);
	writel(1 << pin, GPIO_IOBASE + port * GPIO_PORT_OFF + PXMASKS);
	writel(1 << pin, GPIO_IOBASE + port * GPIO_PORT_OFF + PXPAT1S);
}

static void gpio_direction_output(int port, int pin, int value)
{
	writel(1 << pin, GPIO_IOBASE + port * GPIO_PORT_OFF + PXINTC);
	writel(1 << pin, GPIO_IOBASE + port * GPIO_PORT_OFF + PXMASKS);
	writel(1 << pin, GPIO_IOBASE + port * GPIO_PORT_OFF + PXPAT1C);

	gpio_set_value(port, pin, value);
}
static void setsda(unsigned int sda_pin, int state)
{
	if (state)
		gpio_direction_input(sda_pin / 32, sda_pin % 32);
	else
		gpio_direction_output(sda_pin / 32, sda_pin % 32, 0);
}

static void setscl(unsigned int scl_pin, int state)
{
	if (state)
		gpio_direction_input(scl_pin / 32, scl_pin % 32);
	else
		gpio_direction_output(scl_pin / 32, scl_pin % 32, 0);
}

static int getsda(unsigned int sda_pin)
{
	return gpio_get_value(sda_pin / 32, sda_pin % 32);
}

static inline void sdalo(struct i2c_jz *i2c)
{
	setsda(i2c->sda_pin, 0);
	udelay((i2c->udelay + 1) / 2);
}

static inline void sdahi(struct i2c_jz *i2c)
{
	setsda(i2c->sda_pin, 1);
	udelay((i2c->udelay + 1) / 2);
}

static inline void scllo(struct i2c_jz *i2c)
{
	setscl(i2c->scl_pin, 0);
	udelay(i2c->udelay / 2);
}

static int sclhi(struct i2c_jz *i2c)
{
	setscl(i2c->scl_pin, 1);
	udelay(i2c->udelay);
	return 0;
}

static void i2c_start(struct i2c_jz *i2c)
{
	/* assert: scl, sda are high */
	setsda(i2c->sda_pin, 0);
	udelay(i2c->udelay);
	scllo(i2c);
}

static void i2c_repstart(struct i2c_jz *i2c)
{
	/* assert: scl is low */
	sdahi(i2c);
	sclhi(i2c);
	setsda(i2c->sda_pin, 0);
	udelay(i2c->udelay);
	scllo(i2c);
}

static void i2c_stop(struct i2c_jz *i2c)
{
	/* assert: scl is low */
	sdalo(i2c);
	sclhi(i2c);
	setsda(i2c->sda_pin, 1);
	udelay(i2c->udelay);
}

#define I2C_M_IGNORE_NAK	0x1000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NOSTART		0x4000	/* if I2C_FUNC_NOSTART */
#define I2C_M_RD			0x0001	/* read data, from slave to master */
#define I2C_M_RECV_LEN		0x0400	/* length will be first received byte */
#define I2C_M_NO_RD_ACK		0x0800	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_TEN			0x0010	/* this is a ten bit chip address */
#define I2C_M_REV_DIR_ADDR	0x2000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_SMBUS_BLOCK_MAX	32		/* As specified in SMBus standard */

#define	EIO			 5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	EPROTO		71	/* Protocol error */
#define	ETIMEDOUT	110	/* Connection timed out */

static int i2c_inb(struct i2c_jz *i2c)
{
	/* read byte via i2c port, without start/stop sequence	*/
	/* acknowledge is sent in i2c_read.			*/
	int i;
	unsigned char indata = 0;

	/* assert: scl is low */
	sdahi(i2c);
	for (i = 0; i < 8; i++) {
		if (sclhi(i2c) < 0) { /* timeout */
			return -ETIMEDOUT;
		}
		indata *= 2;
		if (getsda(i2c->sda_pin))
			indata |= 0x01;
		setscl(i2c->scl_pin, 0);
		udelay(i == 7 ? i2c->udelay / 2 : i2c->udelay);
	}
	/* assert: scl is low */
	return indata;
}

static int i2c_outb(struct i2c_jz *i2c, unsigned char c)
{
	int i;
	int sb;
	int ack;

	/* assert: scl is low */
	for (i = 7; i >= 0; i--) {
		sb = (c >> i) & 1;
		setsda(i2c->sda_pin, sb);
		udelay((i2c->udelay + 1) / 2);
		if (sclhi(i2c) < 0) { /* timed out */
			return -ETIMEDOUT;
		}
		/* FIXME do arbitration here:
		 * if (sb && !getsda(adap)) -> ouch! Get out of here.
		 *
		 * Report a unique code, so higher level code can retry
		 * the whole (combined) message and *NOT* issue STOP.
		 */
		scllo(i2c);
	}
	sdahi(i2c);
	if (sclhi(i2c) < 0) { /* timeout */
		return -ETIMEDOUT;
	}

	/* read ack: SDA should be pulled down by slave, or it may
	 * NAK (usually to report problems with the data we wrote).
	 */
	ack = !getsda(i2c->sda_pin);    /* ack: sda is pulled low -> success */

	scllo(i2c);
	return ack;
	/* assert: scl is low (sda undef) */
}

static int try_address(struct i2c_jz *i2c,
		       unsigned char addr, int retries)
{
	int i, ret = 0;

	for (i = 0; i <= retries; i++) {
		ret = i2c_outb(i2c, addr);
		if (ret == 1 || i == retries)
			break;
		i2c_stop(i2c);
		udelay(i2c->udelay);
		i2c_start(i2c);
	}

	return ret;
}

static int bit_doAddress(struct i2c_jz *i2c, struct i2c_msg *msg)
{
	unsigned short flags = msg->flags;
	unsigned short nak_ok = msg->flags & I2C_M_IGNORE_NAK;

	unsigned char addr;
	int ret, retries;

	retries = nak_ok ? 0 : i2c->retries;

	if (flags & I2C_M_TEN) {
		/* a ten bit address */
		addr = 0xf0 | ((msg->addr >> 7) & 0x06);
		/* try extended address code...*/
		ret = try_address(i2c, addr, retries);
		if ((ret != 1) && !nak_ok)  {
			return -ENXIO;
		}
		/* the remaining 8 bit address */
		ret = i2c_outb(i2c, msg->addr & 0xff);
		if ((ret != 1) && !nak_ok) {
			/* the chip did not ack / xmission error occurred */
			return -ENXIO;
		}
		if (flags & I2C_M_RD) {
			i2c_repstart(i2c);
			/* okay, now switch into reading mode */
			addr |= 0x01;
			ret = try_address(i2c, addr, retries);
			if ((ret != 1) && !nak_ok) {
				return -EIO;
			}
		}
	} else {		/* normal 7bit address	*/
		addr = msg->addr << 1;
		if (flags & I2C_M_RD)
			addr |= 1;
		if (flags & I2C_M_REV_DIR_ADDR)
			addr ^= 1;
		ret = try_address(i2c, addr, retries);
		if ((ret != 1) && !nak_ok)
			return -ENXIO;
	}

	return 0;
}

static int acknak(struct i2c_jz *i2c, int is_ack)
{
	/* assert: sda is high */
	if (is_ack)		/* send ack */
		setsda(i2c->sda_pin, 0);
	udelay((i2c->udelay + 1) / 2);
	if (sclhi(i2c) < 0) {	/* timeout */
		return -ETIMEDOUT;
	}
	scllo(i2c);
	return 0;
}

static int readbytes(struct i2c_jz *i2c, struct i2c_msg *msg)
{
	int inval;
	int rdcount = 0;	/* counts bytes read */
	unsigned char *temp = (unsigned char *)msg->buf;
	int count = msg->len;
	const unsigned flags = msg->flags;

	while (count > 0) {
		inval = i2c_inb(i2c);
		if (inval >= 0) {
			*temp = inval;
			rdcount++;
		} else {   /* read timed out */
			break;
		}

		temp++;
		count--;

		/* Some SMBus transactions require that we receive the
		   transaction length as the first read byte. */
		if (rdcount == 1 && (flags & I2C_M_RECV_LEN)) {
			if (inval <= 0 || inval > I2C_SMBUS_BLOCK_MAX) {
				if (!(flags & I2C_M_NO_RD_ACK))
					acknak(i2c, 0);
				return -EPROTO;
			}
			/* The original count value accounts for the extra
			   bytes, that is, either 1 for a regular transaction,
			   or 2 for a PEC transaction. */
			count += inval;
			msg->len += inval;
		}

		if (!(flags & I2C_M_NO_RD_ACK)) {
			inval = acknak(i2c, count);
			if (inval < 0) {
				return inval;
			}
		}
	}
	return rdcount;
}

static int sendbytes(struct i2c_jz *i2c, struct i2c_msg *msg)
{
	const unsigned char *temp = (unsigned char *)msg->buf;
	int count = msg->len;
	unsigned short nak_ok = msg->flags & I2C_M_IGNORE_NAK;
	int retval;
	int wrcount = 0;

	while (count > 0) {
		retval = i2c_outb(i2c, *temp);

		/* OK/ACK; or ignored NAK */
		if ((retval > 0) || (nak_ok && (retval == 0))) {
			count--;
			temp++;
			wrcount++;

		/* A slave NAKing the master means the slave didn't like
		 * something about the data it saw.  For example, maybe
		 * the SMBus PEC was wrong.
		 */
		} else if (retval == 0) {
			return -EIO;

		/* Timeout; or (someday) lost arbitration
		 *
		 * FIXME Lost ARB implies retrying the transaction from
		 * the first message, after the "winning" master issues
		 * its STOP.  As a rule, upper layer code has no reason
		 * to know or care about this ... it is *NOT* an error.
		 */
		} else {
			return retval;
		}
	}
	return wrcount;
}

static void i2c_xfer(struct i2c_jz *i2c)
{
	int i, ret;
	int num;
	unsigned short nak_ok;

	num = readl(TCSM_BASE_ADDR + ARGS + 0x20);

	i2c_start(i2c);

	for (i = 0; i < num; i++) {
		pmsg.addr = REG16(TCSM_BASE_ADDR + ARGS + 0x24 + 12 * i);
		pmsg.flags = REG16(TCSM_BASE_ADDR + ARGS + 0x26 + 12 * i);
		pmsg.len = REG16(TCSM_BASE_ADDR + ARGS + 0x28 + 12 * i);
		pmsg.buf = REG32(TCSM_BASE_ADDR + ARGS + 0x2c + 12 * i);
		nak_ok = pmsg.flags & I2C_M_IGNORE_NAK;
		if (!(pmsg.flags & I2C_M_NOSTART)) {
			if (i) {
				i2c_repstart(i2c);
			}
			ret = bit_doAddress(i2c, &pmsg);
			if ((ret != 0) && !nak_ok) {
				goto bailout;
			}
		}
		if (pmsg.flags & I2C_M_RD) {
			/* read bytes into buffer*/
			ret = readbytes(i2c, &pmsg);
			if (ret < pmsg.len) {
				if (ret >= 0)
					ret = -EIO;
				goto bailout;
			}
		} else {
			/* write bytes from buffer */
			ret = sendbytes(i2c, &pmsg);
			if (ret < pmsg.len) {
				if (ret >= 0)
					ret = -EIO;
				goto bailout;
			}
		}
	}
	ret = i;
bailout:
	i2c_stop(i2c);
	writel(ret, TCSM_BASE_ADDR + ARGS + 0x20);
}

void handle_i2c_irq(void)
{
	i2c.sda_pin = readl(TCSM_BASE_ADDR + ARGS + 0x10);
	i2c.scl_pin = readl(TCSM_BASE_ADDR + ARGS + 0x14);
	i2c.udelay = readl(TCSM_BASE_ADDR + ARGS + 0x18);
	i2c.retries = readl(TCSM_BASE_ADDR + ARGS + 0x1c);
	i2c_xfer(&i2c);
	writel(INTC_PDMAM, INTC_IOBASE + ICMCR1);
	writel(readl(INTC_IOBASE + DMR1) | INTC_PDMAM, INTC_IOBASE + DMR1);
	writel(DMINT_S_IMSK, PDMA_IOBASE + DMINT);
	writel(0xFFFFFFFF, PDMA_IOBASE + DMNMB);
}

