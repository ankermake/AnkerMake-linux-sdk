#
# Ingenic dorado makefile
#
# Copyright (c) 2013 Ingenic Semiconductor Co.,Ltd
# Author: Zoro <ykli@ingenic.cn>
# Based on: board/reliableIPTV/urboard/Makefile
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

LIB	= $(obj)lib$(BOARD).o

COBJS-y	:= board.o
COBJS-$(CONFIG_VIDEO_BM347WV_F_8991FTGF) += lcd-8991.o
COBJS-$(CONFIG_VIDEO_TRULY_TFT240240_2_E) += lcd-truly-tft240240.o
COBJS-$(CONFIG_VIDEO_BYD_BM8766U) += lcd-bydbm8766u.o
COBJS-$(CONFIG_VIDEO_BYD_9177AA) += lcd-9177aa.o
COBJS-$(CONFIG_VIDEO_LH155) += lcd-lh155.o
COBJS-$(CONFIG_VIDEO_ET017QG1) += epd-ET017QG1.o
COBJS-$(CONFIG_VIDEO_SPILCD_RRJ_R108103_GC9203_128x220) += spilcd-r108103_gc9203.o
COBJS-$(CONFIG_CMD_FASTBOOT) += fastboot.o

SRCS	:= $(SOBJS-y:.o=.S) $(COBJS-y:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS-y))
SOBJS   := $(addprefix $(obj),$(SOBJS-y))

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################
# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
