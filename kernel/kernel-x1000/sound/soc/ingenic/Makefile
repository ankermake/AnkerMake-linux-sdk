
snd-asoc-dma-encrypt-objs	:= lib/asoc-dma-encrypt.obj
snd-asoc-dma-hrtimer-objs := asoc-dma-hrtimer.o

snd-asoc-dma-v12-objs 	:= asoc-v12/asoc-dma-v12.o
snd-asoc-aic-v12-objs 	:= asoc-v12/asoc-aic-v12.o
snd-asoc-i2s-v12-objs 	:= asoc-v12/asoc-i2s-v12.o asoc-v12/aic_new.o

snd-asoc-pcm-v12-objs 	:= asoc-v12/asoc-pcm-v12.o
snd-asoc-spdif-v12-objs := asoc-v12/asoc-spdif-v12.o
snd-asoc-dmic-v12-objs := asoc-v12/asoc-dmic-v12.o

snd-asoc-dma-v13-objs 	:= asoc-v13/asoc-dma-v13.o
snd-asoc-i2s-v13-objs 	:= asoc-v13/asoc-i2s-v13.o
snd-asoc-pcm-v13-objs 	:= asoc-v13/asoc-pcm-v13.o
snd-asoc-spdif-v13-objs := asoc-v13/asoc-spdif-v13.o
snd-asoc-dmic-v13-objs := asoc-v13/asoc-dmic-v13.o
snd-asoc-dmic-module-objs := asoc-v13/asoc-dma-dmic.o asoc-v13/asoc-dmic-module.o

snd-asoc-icdc-d1-objs := icodec/icdc_d1.o
snd-asoc-icdc-d2-objs := icodec/icdc_d2.o
snd-asoc-icdc-d3-objs := icodec/icdc_d3.o
snd-asoc-icdc-d4-objs := icodec/icdc_d4.o
snd-asoc-pcm-dump-objs := icodec/pcm_dump.o
snd-asoc-spdif-dump-objs := icodec/spdif_dump.o
snd-asoc-dmic-dump-objs := icodec/dmic_dump.o

obj-$(CONFIG_SND_ASOC_DMA_ENCRPTY) += snd-asoc-dma-encrypt.o
obj-$(CONFIG_SND_ASOC_DMA_HRTIMER) += snd-asoc-dma-hrtimer.o
obj-$(CONFIG_SND_ASOC_DMA_V12) += snd-asoc-dma-v12.o
obj-$(CONFIG_SND_ASOC_DMA_V13) += snd-asoc-dma-v13.o
obj-$(CONFIG_SND_ASOC_JZ_AIC_V12) += snd-asoc-aic-v12.o
obj-$(CONFIG_SND_ASOC_JZ_AIC_I2S_V12) += snd-asoc-i2s-v12.o
obj-$(CONFIG_SND_ASOC_JZ_AIC_I2S_V13) += snd-asoc-i2s-v13.o

obj-$(CONFIG_SND_ASOC_JZ_ICDC_D1) += snd-asoc-icdc-d1.o
obj-$(CONFIG_SND_ASOC_JZ_ICDC_D2) += snd-asoc-icdc-d2.o
obj-$(CONFIG_SND_ASOC_JZ_ICDC_D3) += snd-asoc-icdc-d3.o
obj-$(CONFIG_SND_ASOC_JZ_ICDC_D4) += snd-asoc-icdc-d4.o

obj-$(CONFIG_SND_ASOC_JZ_PCM_V12) += snd-asoc-pcm-v12.o
obj-$(CONFIG_SND_ASOC_JZ_PCM_V13) += snd-asoc-pcm-v13.o
obj-$(CONFIG_SND_ASOC_JZ_PCM_DUMP_CDC) += snd-asoc-pcm-dump.o

obj-$(CONFIG_SND_ASOC_JZ_AIC_SPDIF_V12) += snd-asoc-spdif-v12.o
obj-$(CONFIG_SND_ASOC_JZ_AIC_SPDIF_V13) += snd-asoc-spdif-v13.o
obj-$(CONFIG_SND_ASOC_JZ_SPDIF_DUMP_CDC) += snd-asoc-spdif-dump.o

obj-$(CONFIG_SND_ASOC_JZ_DMIC_V12) += snd-asoc-dmic-v12.o
obj-$(CONFIG_SND_ASOC_JZ_DMIC_V13) += snd-asoc-dmic-v13.o
obj-$(CONFIG_SND_ASOC_JZ_DMIC_DUMP_CDC) += snd-asoc-dmic-dump.o

obj-$(CONFIG_SND_ASOC_JZ_DMIC_MODULE) += snd-asoc-dmic-module.o


snd-soc-dorado-icdc-objs   := asoc-board/dorado_icdc.o
snd-soc-watch-icdc-objs   := asoc-board/watch_icdc.o
snd-soc-newton2-plus-icdc-objs   := asoc-board/newton2_plus_icdc.o
snd-soc-mensa-icdc-objs	   := asoc-board/mensa_icdc.o
snd-soc-mensa-spdif-objs   := asoc-board/mensa_spdif.o
snd-soc-phoenix-icdc-objs  := asoc-board/phoenix_icdc.o
snd-soc-canna-icdc-objs  := asoc-board/canna_icdc.o
snd-soc-pd-x1830-audio-icdc-objs := asoc-board/pd_x1830_audio_icdc.o
snd-soc-pd-x1830-suning-alarm-icdc-objs := asoc-board/pd_x1830_suning_alarm_icdc.o

obj-$(CONFIG_SND_ASOC_INGENIC_DORADO_ICDC) += snd-soc-dorado-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_WATCH_AW808_ICDC) += snd-soc-watch-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_NEWTON2_PLUS_ICDC) += snd-soc-newton2-plus-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_MENSA_ICDC) += snd-soc-mensa-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_MENSA_SPDIF) += snd-soc-mensa-spdif.o
obj-$(CONFIG_SND_ASOC_INGENIC_PHOENIX_ICDC) += snd-soc-phoenix-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_CANNA_ICDC) += snd-soc-canna-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_PDX1830_AUDIO_ICDC) += snd-soc-pd-x1830-audio-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_PDX1830_SUNING_ALARM_ICDC) += snd-soc-pd-x1830-suning-alarm-icdc.o
$(obj)/asoc-board/phoenix_icdc.o:include/generated/autoconf.h


snd-soc-phoenix-spdif-objs := asoc-board/phoenix_spdif.o
obj-$(CONFIG_SND_ASOC_INGENIC_PHOENIX_SPDIF) += snd-soc-phoenix-spdif.o

snd-soc-ak4951-board-objs  := asoc-board/ak4951_board.o
snd-soc-pd-x1830-sy6026l-board-objs  := asoc-board/pd_x1830_sy6026l.o
snd-soc-pd-suning-sma1301-es7241-board-objs  := asoc-board/pd_suning_sma1301_es7241.o
snd-soc-pd-im501-devel-board-objs := asoc-board/pd_im501_devel.o
obj-$(CONFIG_SND_ASOC_INGENIC_EXTCODEC_AK4951) += snd-soc-ak4951-board.o
obj-$(CONFIG_SND_ASOC_INGENIC_PDX1830_SY6026L) += snd-soc-pd-x1830-sy6026l-board.o
obj-$(CONFIG_SND_ASOC_INGENIC_SUNING_SMA1301_ES7241) += snd-soc-pd-suning-sma1301-es7241-board.o
obj-$(CONFIG_SND_ASOC_INGENIC_IM501_DEVEL_BOARD) += snd-soc-pd-im501-devel-board.o
