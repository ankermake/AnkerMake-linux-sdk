# 查找存在的文件
list_exist_file = $(wildcard $1)

# 用于检测文件是否存在
list_not_exist_file = $(filter-out $(call list_exist_file,$1),$1)

# 获得 所有的 .o 或者 .d,例子如下
# $(call to_obj, $(src-y), $(OBJDIR))
# $(call to_dep, $(src-y), $(OBJDIR))
to_obj = $(patsubst %,$(2)%.o,$1)
to_dep = $(patsubst %,$(2)%.d,$1)

# 获得 所有的 .c.o 或者 .c.d,例子如下
# $(call to_xobj, $(src-y), $(OBJDIR), .c)
# $(call to_xdep, $(src-y), $(OBJDIR), .c)
to_xobj = $(call to_obj, $(filter %$(strip $(3)), $(1)), $(2))
to_xdep = $(call to_dep, $(filter %$(strip $(3)), $(1)), $(2))

# 如果条件不为空则报错,例子如下
# $(call error_if, $(err_files), these files are unsupported)
error_if = $(if $(strip $1), $(warning $1)$(warning $2)$(error ))

# 如果条件为空则报错,例子如下
# $(call error_ifnot, $(files), input file can not be empty)
error_ifnot = $(if $(strip $1), ,$(warning $2)$(error ))

# If the list of objects to link is empty, just create an empty built-in.o
cmd_link_o_target = $(if $(strip $1),\
		      $(Q)$(LD) $(LDFLAGS) -r -o $@ $(strip $1),\
		      $(Q)rm -f $@; $(AR) rcs $@ )

# 如果变量值不为空则include文件,例子如下
# $(call include_if, CONFIG_YOUR_DEF, your_def.mk)
include_if = $(if $(strip $1),$(eval include $2))

MSG_CC            ?= "  CC       "
MSG_LD            ?= "  LD       "
MSG_LD_MODULE     ?= "  linking  "
MSG_LDS           ?= "  lds.in   "
MSG_CLEAN         ?= "  clean    "
MSG_OBJCOPY       ?= "  binary   "
MSG_LINKING       ?= "  linking  "
MSG_SYMBOLS       ?= "  symbols  "
MSG_FILE_NOT_EXIST ?= error: file not exist
MSG_FILE_NOT_SUPPORT ?= error: only support .c .s .S .cc .cpp format src file

# foreach 时命令输出之间的间隔,否则会交错在一起
define CMD_SEPARATOR
	$(warning, \n)
	$(warning, \n)
endef
