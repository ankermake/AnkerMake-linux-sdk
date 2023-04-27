#JZ47xx
dtb-$(CONFIG_JZ4740_QI_LB60)	+= qi_lb60.dtb
dtb-$(CONFIG_JZ4780_CI20)	+= ci20.dtb

#X1500  X1000
dtb-$(CONFIG_DT_X1500_YAK)	+= yak.dtb
dtb-$(CONFIG_DT_HALLEY2_V20)	+= halley2v20.dtb

#X1021
dtb-$(CONFIG_DT_X1021_SEPAL_V10)	+= x1021_sepal.dtb

#X1520
dtb-$(CONFIG_DT_X1520_EVB_V10)	+= x1520_evb.dtb

#X1600
dtb-$(CONFIG_DT_PANDA_V10)	+= panda_v10.dtb
dtb-$(CONFIG_DT_HALLEY6_V10)	+= halley6_v10.dtb
dtb-$(CONFIG_DT_HALLEY6_V10_AWTK)	+= halley6_v10_awtk.dtb
dtb-$(CONFIG_DT_X1600_IRIS_V10)	+= x1600_iris_v10.dtb


#X1630 1830
dtb-$(CONFIG_DT_X1630_DARWIN_v12)	+= x1630_darwin.dtb

#X2000
dtb-$(CONFIG_DT_SEAL)		+= seal.dtb
dtb-$(CONFIG_DT_HALLEY5_V10)	+= halley5_v10.dtb
dtb-$(CONFIG_DT_HALLEY5_V20)	+= halley5_v20.dtb
dtb-$(CONFIG_DT_HALLEY5_V30)	+= halley5_v30.dtb
dtb-$(CONFIG_DT_HALLEY5_V20_QT)	+= halley5_v20_qt.dtb
dtb-$(CONFIG_DT_WICOM_CUSTOMER)	+= wicom_customer.dtb
dtb-$(CONFIG_DT_FULLCOLOR_7INCH)	+= fullcolor_7inch.dtb
dtb-$(CONFIG_DT_ZEBRA)		+= zebra.dtb
dtb-$(CONFIG_DT_M300_ZEBRA)	+= zebra_m300_power.dtb
dtb-$(CONFIG_DT_ZEBRA_ISP)	+= zebra_isp.dtb
dtb-$(CONFIG_DT_ZEBRA_CIM)	+= zebra_cim.dtb
dtb-$(CONFIG_DT_ZEBRA_POWER)	+= zebra_power.dtb
dtb-$(CONFIG_DT_X2000_EVB)	+= x2000_evb.dtb
dtb-$(CONFIG_DT_WICOM_HALLEY5_PANEL)	+= wicom_halley5_panel.dtb
dtb-$(CONFIG_DT_X2000_YIAN_V10) += x2000_yian_v10.dtb
dtb-$(CONFIG_DT_LIFESMART_480P) += lifesmart_480p.dtb
dtb-$(CONFIG_DT_X2000_HUABY) += x2000_huaby.dtb


#X2100
dtb-$(CONFIG_DT_ROBAM_CQ925)		+= robam_cq925.dtb
dtb-$(CONFIG_DT_ROBAM_DB620)		+= robam_db620.dtb
dtb-$(CONFIG_DT_ROBAM_9YC05)		+= robam_9yc05.dtb
dtb-$(CONFIG_DT_KALE)		+= kale.dtb
dtb-$(CONFIG_DT_HUABY)		+= huaby.dtb
dtb-$(CONFIG_DT_WICOM_KALE_PANEL)	+= wicom_kale_panel.dtb
dtb-$(CONFIG_DT_WICOM_KALE_PANEL_KD050)	+= wicom_kale_panel_kd050.dtb

#X2500
dtb-$(CONFIG_DT_GANDER_V10)		+= gander_v10.dtb
dtb-$(CONFIG_DT_HIPPO_V10)		+= hippo_v10.dtb
dtb-$(CONFIG_DT_BEAR_V10)		+= bear_v10.dtb
dtb-$(CONFIG_DT_HIPPO_V12)		+= hippo_v12.dtb
dtb-$(CONFIG_DT_DARWIN_V14)		+= darwin_v14.dtb

# x2000_module_base.dts
module_driver_x2000_dst_file := $(shell echo $(CONFIG_DT_X2000_MODULE_BASE_DTS_FILE))
module_driver_x2000_dst_file := $(basename $(module_driver_x2000_dst_file)).dtb
dtb-$(CONFIG_DT_X2000_MODULE_BASE) += $(module_driver_x2000_dst_file)

# x1600_module_base.dts
module_driver_x1600_dst_file := $(shell echo $(CONFIG_DT_X1600_MODULE_BASE_DTS_FILE))
module_driver_x1600_dst_file := $(basename $(module_driver_x1600_dst_file)).dtb
dtb-$(CONFIG_DT_X1600_MODULE_BASE) += $(module_driver_x1600_dst_file)

#M300
dtb-$(CONFIG_DT_GEWU)		+= gewu.dtb
dtb-$(CONFIG_DT_GEWU_V12)	+= gewu_v12.dtb


obj-y				+= $(patsubst %.dtb, %.dtb.o, $(dtb-y))

# Force kbuild to make empty built-in.o if necessary
obj-				+= dummy.o

always				:= $(dtb-y)
clean-files			:= *.dtb *.dtb.S
