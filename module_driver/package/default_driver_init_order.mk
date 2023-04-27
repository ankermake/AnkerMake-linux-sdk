#
# 定义默认的 driver 的初始化顺序
# 越早加入 driver_init_order 变量的, 越早初始化
# 没有在此文件中定义的初始化脚本自动排在后面
#

driver_init_order += utils.sh
driver_init_order += rmem_manager.sh
driver_init_order += soc_utils.sh
driver_init_order += soc_i2c.sh
driver_init_order += soc_spi.sh
driver_init_order += spi_fb.sh
driver_init_order += soc_sslv.sh
driver_init_order += i2c_gpio_add.sh
driver_init_order += spi_gpio_add.sh
driver_init_order += keyboard_gpio_add.sh
driver_init_order += soc_gpio.sh
driver_init_order += rtc_clk_out.sh
driver_init_order += soc_pwm.sh
driver_init_order += pwm_backlight.sh
driver_init_order += soc_rotator.sh
driver_init_order += soc_fb_layer_mixer.sh
driver_init_order += soc_fb.sh
driver_init_order += soc_camera.sh
driver_init_order += soc_audio_dma.sh
driver_init_order += soc_aic.sh
driver_init_order += soc_icodec.sh
driver_init_order += soc_msc.sh
driver_init_order += soc_adc.sh
driver_init_order += gpio_regulator.sh
driver_init_order += soc_conn.sh
driver_init_order += soc_dtrng.sh
driver_init_order += soc_hash.sh