
#ifndef __INTC_H__
#define __INTC_H__

// For CPU
#define ICSR0         (0x00)
#define ICMR0         (0x04)
#define ICMSR0        (0x08)
#define ICMCR0        (0x0C)
#define ICPR0         (0x10)

#define ICSR1         (0x20)
#define ICMR1         (0x24)
#define ICMSR1        (0x28)
#define ICMCR1        (0x2C)
#define ICPR1         (0x30)

// For PDMA
#define DSR0          (0x34)
#define DMR0          (0x38)
#define DPR0          (0x3C)

#define DSR1          (0x40)
#define DMR1          (0x44)
#define DPR1          (0x48)


// INT SR0 NAME
#define INTC_DMIC               (0x1 << 0)
#define INTC_AIC0               (0x1 << 1)
#define INTC_BCH                (0x1 << 2)
#define INTC_DSI                (0x1 << 3)
#define INTC_CSI                (0x1 << 4)
#define INTC_CHCI               (0x1 << 5)
#define INTC_IPU                (0x1 << 6)
#define INTC_SSI1               (0x1 << 7)
#define INTC_SSI0               (0x1 << 8)
#define INTC_PDMA               (0x1 << 10)
#define INTC_GPIO5              (0x1 << 12)
#define INTC_GPIO4              (0x1 << 13)
#define INTC_GPIO3              (0x1 << 14)
#define INTC_GPIO2              (0x1 << 15)
#define INTC_GPIO1              (0x1 << 16)
#define INTC_GPIO0              (0x1 << 17)
#define INTC_SADC               (0x1 << 18)
#define INTC_EPDC               (0x1 << 19)
#define INTC_EHCI               (0x1 << 20)
#define INTC_OTG                (0x1 << 21)
#define INTC_HASH               (0x1 << 22)
#define INTC_AES                (0x1 << 23)
#define INTC_TCU2               (0x1 << 25)
#define INTC_TCU1               (0x1 << 26)
#define INTC_TCU0               (0x1 << 27)
#define INTC_ISP                (0x1 << 29)
#define INTC_DELAY_LINE         (0x1 << 30)
#define INTC_LCD                (0x1 << 31)

// INT SR1 NAME
#define INTC_RTC                (0x1 << 0)
#define INTC_UART4              (0x1 << 2)
#define INTC_MSC2               (0x1 << 3)
#define INTC_MSC1               (0x1 << 4)
#define INTC_MSC0               (0x1 << 5)
#define INTC_NFI                (0x1 << 7)
#define INTC_PCM0               (0x1 << 8)
#define INTC_HARB2              (0x1 << 12)
#define INTC_HARB1              (0x1 << 13)
#define INTC_HARB0              (0x1 << 14)
#define INTC_CPM                (0x1 << 15)
#define INTC_UART3              (0x1 << 16)
#define INTC_UART2              (0x1 << 17)
#define INTC_UART1              (0x1 << 18)
#define INTC_UART0              (0x1 << 19)
#define INTC_DDR                (0x1 << 20)
#define INTC_EFUSE              (0x1 << 22)
#define INTC_I2C3               (0x1 << 25)
#define INTC_I2C2               (0x1 << 26)
#define INTC_I2C1               (0x1 << 27)
#define INTC_I2C0               (0x1 << 28)
#define INTC_PDMAM              (0x1 << 29)
#define INTC_VPU                (0x1 << 30)
#define INTC_GPU                (0x1 << 31)

#endif
