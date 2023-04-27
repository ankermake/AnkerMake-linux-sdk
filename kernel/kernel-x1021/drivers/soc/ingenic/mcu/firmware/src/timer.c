#include <common.h>

static unsigned long long get_timer64(void)
{
	union clycle_type {
		unsigned long long cycle64;
		unsigned int cycle32[2];
	} cycle;

	do {
		cycle.cycle32[1] = readl(TCU_IOBASE + OST_CNTH);
		cycle.cycle32[0] = readl(TCU_IOBASE + OST_CNTL);
	} while(cycle.cycle32[1] != readl(TCU_IOBASE + OST_CNTH));

	return cycle.cycle64;
}

void udelay(unsigned long usec)
{
	unsigned long long end = get_timer64() + ((unsigned long long)usec * 6);
	while (get_timer64() < end);
}

void mdelay(unsigned long msec)
{
	while (msec--)
		udelay(1000);
}

void ost_timer(void)
{
	writel(0, TCU_IOBASE + OST_DR);
	writel(0, TCU_IOBASE + OST_CNTL);
	writel(0, TCU_IOBASE + OST_CNTH);
	writel(OSTCSR_CNT_MD | CSR_DIV4 | CSR_EXT_EN, TCU_IOBASE + OST_CSR);
	writel(TER_OSTEN, TCU_IOBASE + TCU_TESR);
}

