snd-asoc-dma-hrtimer-objs := asoc-dma-hrtimer.o
snd-asoc-dma-v12-objs 	:= asoc-v12/asoc-dma-v12.o
snd-asoc-aic-v12-objs 	:= asoc-v12/asoc-aic-v12.o
snd-asoc-i2s-v12-objs 	:= asoc-v12/asoc-i2s-v12.o
snd-asoc-pcm-v12-objs 	:= asoc-v12/asoc-pcm-v12.o
snd-asoc-spdif-v12-objs := asoc-v12/asoc-spdif-v12.o
snd-asoc-dmic-v12-objs := asoc-v12/asoc-dmic-v12.o
snd-soc-rd-x1630-audio-icdc-objs := asoc-board/rd_x1630_audio_icdc.o
snd-soc-sepal-x1021-icdc-objs := asoc-board/sepal_x1021_icdc.o

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

obj-$(CONFIG_SND_ASOC_DMA_V12) += snd-asoc-dma-v12.o
obj-$(CONFIG_SND_ASOC_DMA_V13) += snd-asoc-dma-v13.o
obj-$(CONFIG_SND_ASOC_JZ_AIC_V12) += snd-asoc-aic-v12.o
obj-$(CONFIG_SND_ASOC_JZ_AIC_I2S_V12) += snd-asoc-i2s-v12.o
obj-$(CONFIG_SND_ASOC_JZ_AIC_I2S_V13) += snd-asoc-i2s-v13.o
obj-$(CONFIG_SND_ASOC_DMA_HRTIMER) += snd-asoc-dma-hrtimer.o

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
snd-soc-phoenix-spdif-objs := asoc-board/phoenix_spdif.o
snd-soc-phoenix-icdc-objs  := asoc-board/phoenix_icdc.o
snd-soc-canna-icdc-objs  := asoc-board/canna_icdc.o
snd-soc-halley2-pro-icdc-objs  := asoc-board/halley2_pro_icdc.o

obj-$(CONFIG_SND_ASOC_INGENIC_DORADO_ICDC) += snd-soc-dorado-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_WATCH_AW808_ICDC) += snd-soc-watch-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_NEWTON2_PLUS_ICDC) += snd-soc-newton2-plus-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_HALLEY2_PRO_ICDC) += snd-soc-halley2-pro-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_MENSA_ICDC) += snd-soc-mensa-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_MENSA_SPDIF) += snd-soc-mensa-spdif.o
obj-$(CONFIG_SND_ASOC_INGENIC_PHOENIX_ICDC) += snd-soc-phoenix-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_RDX1630_AUDIO_ICDC) += snd-soc-rd-x1630-audio-icdc.o
obj-$(CONFIG_SND_ASOC_INGENIC_SEPAL_X1021_ICDC) += snd-soc-sepal-x1021-icdc.o
$(obj)/asoc-board/phoenix_icdc.o:include/generated/autoconf.h


obj-$(CONFIG_SND_ASOC_INGENIC_PHOENIX_SPDIF) += snd-soc-phoenix-spdif.o
obj-$(CONFIG_SND_ASOC_INGENIC_CANNA_ICDC) += snd-soc-canna-icdc.o
