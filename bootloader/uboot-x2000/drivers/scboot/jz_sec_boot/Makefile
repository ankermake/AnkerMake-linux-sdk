include $(TOPDIR)/config.mk

LIB	:= $(obj)libsplscboot.o
COBJS-y :=
ifdef CONFIG_SPL_BUILD
COBJS-$(CONFIG_SPL_SCBOOT) += scboot.o
endif

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
