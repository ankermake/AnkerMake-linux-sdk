
#define i2s_read_reg(addr)        inl(AIC0_IOBASE + (addr))
#define i2s_write_reg(val,addr)   outl(val,AIC0_IOBASE + (addr))

#define i2s_set_reg(addr, val, mask, offset)     \
	do {                            \
		volatile unsigned int reg_tmp;              \
			reg_tmp = i2s_read_reg(addr);    \
			reg_tmp &= ~(mask);             \
			reg_tmp |= (val << offset) & mask;      \
			i2s_write_reg(reg_tmp, addr);    \
	} while(0)

#define AICCR		    (0x04)
#define AICCR_EREC_BIT      (0)
#define AICCR_EREC_MASK     (1 << AICCR_EREC_BIT)
#define AICCR_RDMS_BIT      (15)
#define AICCR_RDMS_MASK     (1 << AICCR_RDMS_BIT)


#define __i2s_enable_record()			i2s_set_reg(AICCR, 1, AICCR_EREC_MASK, AICCR_EREC_BIT)
#define __i2s_enable_receive_dma()		i2s_set_reg(AICCR, 1, AICCR_RDMS_MASK , AICCR_RDMS_BIT)

#define __i2s_disable_record()			i2s_set_reg(AICCR, 0, AICCR_EREC_MASK, AICCR_EREC_BIT)

#define __amic_is_enable()			(!!(i2s_read_reg(AICCR) & AICCR_EREC_MASK))
