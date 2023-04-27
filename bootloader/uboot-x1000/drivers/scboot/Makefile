include $(TOPDIR)/config.mk

LIB	:= $(obj)libscboot.o

COBJS-$(CONFIG_JZ_SCBOOT) += secure.o aes.o rsa.o  test_common.o\
							otp.o spi_checksum.o get_rn.o cmd_sctest.o\
							test_scboot.o test_nku.o test_otp.o

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB): $(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
