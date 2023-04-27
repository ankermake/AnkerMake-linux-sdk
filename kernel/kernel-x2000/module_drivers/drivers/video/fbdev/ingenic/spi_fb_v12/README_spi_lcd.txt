

## linux config:
CONFIG_SPI_FB_V12=y
CONFIG_INGENIC_SFC_OLED_V2=y
CONFIG_OLED_FT2306=y

## product board dts:

// sfc_pd, 1v8, FT2306_OLED_InitialCode_BOE_1p47_194x368_V01_202001119_gamma_minibee_QSPI.c
// sfc clk Drive Strength
&sfc_pd_4bit {
	ingenic,pincfg = <&gpd 17 22 PINCFG_PACK(PINCTL_CFG_DRIVE_STRENGTH, 3)>; // Drive Strength TYPEA: [0..7]
	//ingenic,pincfg = <&gpd 17 17 PINCFG_PACK(PINCTL_CFG_DRIVE_STRENGTH, 3)>; // Drive Strength TYPEA: [0..7]
};

&sfc {
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&sfc_pd_4bit>;
	ingenic,sfc-init-frequency = <40000000>;
	ingenic,sfc-max-frequency = <160000000>;
	ingenic,use_ofpart_info  = /bits/ 8 <0>;
	ingenic,spiflash_param_offset = <0>;
	ingenic,te-gpio = <&gpb 27 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;
	ingenic,lcd-pwr-gpio = <&gpb 18 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>; //  LCD_PWR_EN_PB18
	ingenic,tp-pwr-gpio = <&gpb 24 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>; //  TP_PB24_EN
#if 1
	ingenic,rst-gpio = <&gpd 23 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;
	ingenic,swire-gpio = <&gpa 8 GPIO_ACTIVE_HIGH INGENIC_GPIO_PULLDOWN>;
	ingenic,tpint-gpio = <&gpa 10 GPIO_ACTIVE_LOW INGENIC_GPIO_NOBIAS>;
#endif
	norflash@0 {
		partitions {
			compatible = "fixed-partitions";
			#address-cells = <1>;
			#size-cells = <1>;

			/* spi nor flash partition */
			uboot@0 {
				label = "uboot";
				reg = <0x0000000 0x40000>;
				/*read-only;*/
			};

			kernel@40000 {
				label = "kernel";
				reg = <0x40000 0x300000>;
			};

			rootfs@360000 {
				label = "rootfs";
				reg = <0x360000 0xca0000>;
			};
		};
	};

	nandflash@1 {
		partitions {
			compatible = "fixed-partitions";
			#address-cells = <1>;
			#size-cells = <1>;

			/* spi nand flash partition */
			partition@0 {
				label = "uboot";
				reg = <0x0000000 0x100000>;
				/*read-only;*/
			};

			partition@100000 {
				label = "kernel";
				reg = <0x100000 0x800000>;
			};

			partition@900000 {
				label = "rootfs";
				reg = <0x900000 0xf700000>;
			};
		};
	};

};
