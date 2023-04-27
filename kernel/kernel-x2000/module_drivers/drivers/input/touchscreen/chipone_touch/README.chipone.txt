
## linux config:

CONFIG_TOUCHSCREEN_CHIPONE=m
CONFIG_TOUCHSCREEN_CHIPONE_DIRECTORY="chipone_touch"

## board config board.dts

&i2c3 {
	status = "okay";
	clock-frequency = <400000>;
	pinctrl-names = "default";
	pinctrl-0 = <&i2c3_pa>;

	chipone@0x48{
		compatible = "chipone-ts"; /* do not modify */
		reg = <0x48>; /* do not modify */
		interrupt-parent = <&gpa>; /* INT pin */
		interrupts = <10>;
		chipone,rst-gpio = <&gpa 11 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>; /* RST pin */
		chipone,irq-gpio = <&gpa 10 IRQ_TYPE_EDGE_FALLING INGENIC_GPIO_NOBIAS>; /* INT pin */
		chipone,x-res = <194>;
		chipone,y-res = <368>;
	/*	chipone,x-res = <368>;
		chipone,y-res = <194>; */

	};

};



## build module

* drivers/input/touchscreen/chipone_touch/chipone-ts.ko
* out/product/s20.x2000_mmc_4.4.94-eng/system/lib/modules/4.4.94+/kernel/drivers/input/touchscreen/chipone_touch/chipone-ts.ko

## insmod

drivers/input/touchscreen/chipone_touch/etc/firmware/readme
drivers/input/touchscreen/chipone_touch/etc/firmware/ICNT8918_for_test_0x0B02_bin
drivers/input/touchscreen/chipone_touch/etc/firmware/ICNT8918_for_test_0x0B03_bin

* push firmware: /etc/firmware/ICNT8918.bin



log:
```
# insmod /chipone-ts.ko 
[   99.683001] <I>CTS-I2CDrv Init
[   99.686408] <I>CTS-I2CDrv Probe i2c client: name='chipone-ts' addr=0x48 flags=0x00 irq=0
[   99.695939] <I>CTS-Plat Init
[   99.698945] #########################/data1/home/lgwang/SmartPen/HanWang/build/s30/kernel-4.4.94/drivers/input/touchscreen/chipone_touch/cts
_platform.c,176
[   99.698945] 
[   99.715651] <I>CTS-Plat Parse device tree
[   99.719849] <I>CTS-Plat   int gpio    : 52
[   99.724624] <I>CTS-Plat   irq num     : 79
[   99.728885] <I>CTS-Plat   rst gpio    : 53
[   99.733307] <I>CTS-Plat   X resolution: 194
[   99.737622] <I>CTS-Plat   Y resolution: 368
[   99.742374] input: chipone-ts as /devices/platform/apb/10052000.i2c/i2c-2/2-0048/input/input1
[   99.761779] <I>CTS-Plat Request resource
[   99.765856] <I>CTS-Plat Reset device
[   99.821303] <I>CTS-Core Probe device
[   99.825028] <I>CTS-Core Get device firmware id
[   99.829641] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x000a
[   99.836654] <I>CTS-Core Device firmware id: 8918
[   99.841516] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x000c
[   99.847841] <I>CTS-Core Device firmware version: 0c01
[   99.853752] <D>CTS-Core Init hardware data hwid: ffff fwid: 8918
[   99.860001] <I>CTS-Core Init hardware data name: ICNT8918 hwid: 8918 fwid: 8918 
[   99.868175] <I>CTS-Firmware Request from file '/etc/firmware/ICNT8918.bin' if version > 0c01
[   99.877100] <I>CTS-Firmware Filepath is fullpath, direct read it out
[   99.883736] <E>CTS-Firmware Open file '/etc/firmware/ICNT8918.bin' failed -2
[   99.891108] <E>CTS-Firmware Request from file '/etc/firmware/ICNT8918.bin' failed -2
[   99.899131] <E>CTS-Core no need update or request firmware error: ret =0
[   99.906084] <I>CTS-Core Init firmware data
[   99.910350] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x000c
[   99.916618] <I>CTS-Core   Firmware version        : 0c01
[   99.930107] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x0043
[   99.936428] <I>CTS-Core   pannel_id               : ffff
[   99.942479] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x8000
[   99.948809] <I>CTS-Core   X resolution            : 368
[   99.954764] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x8002
[   99.961150] <I>CTS-Core   Y resolution            : 192
[   99.966571] <D>CTS-Core Readb from slave_addr: 0x48 reg: 0x8004
[   99.973519] <I>CTS-Core   Num rows                : 4
[   99.978751] <D>CTS-Core Readb from slave_addr: 0x48 reg: 0x8005
[   99.985562] <I>CTS-Core   Num cols                : 7
[   99.990856] <D>CTS-Core Readb from slave_addr: 0x48 reg: 0x8083
[   99.997155] <I>CTS-Core   Swap axes               : False
[  100.003441] <I>CTS-Core   Swap axes               : 1
[  100.008681] <D>CTS-Core Readb from slave_addr: 0x48 reg: 0x8084
[  100.015533] <I>CTS-Core   Int polarity            : LOW
[  100.020969] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x8085
[  100.027238] <I>CTS-Core   Int keep time           : 5
[  100.032681] <I>CTS-Core Get project id
[  100.036551] <D>CTS-Core Readsb from slave_addr: 0x48 reg: 0x005a len: 10
[  100.043985] <I>CTS-Core Device firmware project id: ����������
[  100.049997] <I>CTS-Plat Init touch device
[  100.054179] <I>CTS-Tool Init
[  100.057173] <I>CTS-Sysfs Add device attr groups
[  100.062037] <I>CTS-I2CDrv Init FB notifier
[  100.066263] <I>CTS-Plat Request IRQ
[  100.069894] <D>CTS-Plat Disable IRQ
[  100.080034] <D>CTS-Plat Real disable IRQ
[  100.084080] <I>CTS-Core Start device...
[  100.088031] <I>CTS-Core Start device successfully
```


```
# 
# insmod /chipone-ts.ko 
[   77.404003] <I>CTS-I2CDrv Init
[   77.407399] <I>CTS-I2CDrv Probe i2c client: name='chipone-ts' addr=0x48 flags=0x00 irq=0
[   77.416597] <I>CTS-Plat Init
[   77.419601] #########################/data1/home/lgwang/SmartPen/HanWang/build/s30/kernel-4.4.94/drivers/input/touchscreen/chipone_touch/cts
_platform.c,176
[   77.419601] 
[   77.436003] <I>CTS-Plat Parse device tree
[   77.440299] <I>CTS-Plat   int gpio    : 52
[   77.444576] <I>CTS-Plat   irq num     : 79
[   77.448837] <I>CTS-Plat   rst gpio    : 53
[   77.453543] <I>CTS-Plat   X resolution: 194
[   77.457891] <I>CTS-Plat   Y resolution: 368
[   77.463067] input: chipone-ts as /devices/platform/apb/10052000.i2c/i2c-2/2-0048/input/input1
[   77.485538] <I>CTS-Plat Request resource
[   77.489621] <I>CTS-Plat Reset device
[   77.554589] <I>CTS-Core Probe device
[   77.558317] <I>CTS-Core Get device firmware id
[   77.563290] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x000a
[   77.569646] <I>CTS-Core Device firmware id: 8918
[   77.574799] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x000c
[   77.581220] <I>CTS-Core Device firmware version: 0c01
[   77.586456] <D>CTS-Core Init hardware data hwid: ffff fwid: 8918
[   77.593081] <I>CTS-Core Init hardware data name: ICNT8918 hwid: 8918 fwid: 8918 
[   77.600817] <I>CTS-Firmware Request from file '/etc/firmware/ICNT8918.bin' if version > 0c01
[   77.609572] <I>CTS-Firmware Filepath is fullpath, direct read it out
[   77.619490] <I>CTS-Firmware File '/etc/firmware/ICNT8918.bin' size: 35368 version: 0b03 <= 0c01(curr_version),no need update
[   77.631107] <E>CTS-Firmware Request from file '/etc/firmware/ICNT8918.bin' failed -22
[   77.639195] <E>CTS-Core no need update or request firmware error: ret =0
[   77.646324] <I>CTS-Core Init firmware data
[   77.650573] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x000c
[   77.656844] <I>CTS-Core   Firmware version        : 0c01
[   77.662662] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x0043
[   77.668984] <I>CTS-Core   pannel_id               : ffff
[   77.674806] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x8000
[   77.681191] <I>CTS-Core   X resolution            : 368
[   77.686613] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x8002
[   77.693323] <I>CTS-Core   Y resolution            : 192
[   77.698733] <D>CTS-Core Readb from slave_addr: 0x48 reg: 0x8004
[   77.705575] <I>CTS-Core   Num rows                : 4
[   77.710872] <D>CTS-Core Readb from slave_addr: 0x48 reg: 0x8005
[   77.717162] <I>CTS-Core   Num cols                : 7
[   77.722776] <D>CTS-Core Readb from slave_addr: 0x48 reg: 0x8083
[   77.729067] <I>CTS-Core   Swap axes               : False
[   77.734960] <I>CTS-Core   Swap axes               : 1
[   77.740248] <D>CTS-Core Readb from slave_addr: 0x48 reg: 0x8084
[   77.746565] <I>CTS-Core   Int polarity            : LOW
[   77.752331] <D>CTS-Core Readw from slave_addr: 0x48 reg: 0x8085
[   77.758671] <I>CTS-Core   Int keep time           : 5
[   77.764065] <I>CTS-Core Get project id
[   77.767938] <D>CTS-Core Readsb from slave_addr: 0x48 reg: 0x005a len: 10
[   77.775338] <I>CTS-Core Device firmware project id: ����������
[   77.781379] <I>CTS-Plat Init touch device
[   77.785518] <I>CTS-Tool Init
[   77.788503] <I>CTS-Sysfs Add device attr groups
[   77.793386] <I>CTS-I2CDrv Init FB notifier
[   77.797614] <I>CTS-Plat Request IRQ
[   77.801292] <D>CTS-Plat Disable IRQ
[   77.804890] <D>CTS-Plat Real disable IRQ
[   77.808934] <I>CTS-Core Start device...
[   77.812910] <I>CTS-Core Start device successfully
# 
# 
# 
# 
# 
# 
# 
# 
# [   86.007040] <D>CTS-Plat IRQ handler
[   86.010639] <D>CTS-Plat Disable IRQ
[   86.014229] <D>CTS-Plat Real disable IRQ
[   86.018319] <D>CTS-Plat IRQ work
[   86.021918] <D>CTS-Plat Handle IRQ
[   86.025458] <D>CTS-Core *** Lock ***
[   86.029172] <D>CTS-Core Enter IRQ handler
[   86.033380] <D>CTS-Core Get touch info
[   86.037582] <D>CTS-Core Readsb from slave_addr: 0x48 reg: 0x1000 len: 16
[   86.045271] <I>CTS-Core Touch info: gesture 0, palm=0,num_msg 1
[   86.051462] <D>CTS-Plat Process touch 1 msgs
[   86.055891] <D>CTS-Plat   Process touch msg[0]: id[0] ev=1 x=107 y=189 p=255
[   86.063215] <D>CTS-Core ### Un-Lock ###
[   86.187999] <D>CTS-Plat IRQ handler
[   86.191630] <D>CTS-Plat Disable IRQ
[   86.195224] <D>CTS-Plat Real disable IRQ
[   86.199289] <D>CTS-Plat IRQ work
[   86.202648] <D>CTS-Plat Handle IRQ
[   86.206346] <D>CTS-Core *** Lock ***
[   86.210066] <D>CTS-Core Enter IRQ handler
[   86.214271] <D>CTS-Core Get touch info
[   86.218137] <D>CTS-Core Readsb from slave_addr: 0x48 reg: 0x1000 len: 16
[   86.225621] <I>CTS-Core Touch info: gesture 0, palm=0,num_msg 1
[   86.231751] <D>CTS-Plat Process touch 1 msgs
[   86.236154] <D>CTS-Plat   Process touch msg[0]: id[0] ev=4 x=107 y=189 p=0
[   86.243273] <D>CTS-Core ### Un-Lock ###

# 
```
