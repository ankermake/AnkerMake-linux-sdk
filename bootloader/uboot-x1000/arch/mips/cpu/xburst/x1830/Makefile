#
# Ingenic X1830 makefile
#
# Copyright (c) 2017  Ingenic Semiconductor Co.,Ltd
# Author: Zoro <ykli@ingenic.cn>
# Based on: arch/mips/cpu/xburst/jz4780/Makefile
#           Written by Paul Burton <paul.burton@imgtec.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(SOC).o

SOBJS-y	=
COBJS-y	= soc.o
COBJS-$(CONFIG_BURNER) += burner_pll.o
ifndef CONFIG_BURNER
COBJS-y += pll.o
endif
COBJS-y += clk.o
COBJS-y += ddr_set_dll.o
COBJS-y += ddr_x1830.o

COBJS-$(CONFIG_MULT_DDR_PARAMS_CREATOR) += ddr_get_params.o

COBJS-$(CONFIG_DDR_TEST) += ../ddr_test.o
COBJS-$(CONFIG_DDR_AUTO_REFRESH_TEST) += ../ddr_test_auto_refresh.o
COBJS-$(CONFIG_BURNER) += ../param_manage.o

SRCS	:= $(START:.o=.S) $(SOBJS-y:.o=.S) $(COBJS-y:.o=.c)
OBJS	:= $(addprefix $(obj),$(SOBJS-y) $(COBJS-y))
START	:= $(addprefix $(obj),$(START))

all:	$(obj).depend $(LIB)

$(LIB):	$(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
