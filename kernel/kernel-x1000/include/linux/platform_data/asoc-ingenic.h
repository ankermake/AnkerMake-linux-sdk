#ifndef __ASOC_INGENIC_H__
#define __ASOC_INGENIC_H__
#if defined(CONFIG_SND) && defined(CONFIG_SND_ASOC_INGENIC)
#define SUPPLY_DMA_SOFT_MUTE_AMIXER 1
#define NO_SUPPLY_DMA_SOFT_MUTE_AMIXER 0
struct snd_dma_data {
	unsigned int dma_write_oncache;
};
struct snd_codec_pdata {
	/*register value below, -1 means use default val in driver*/
	int mic_dftvol;
	int hp_dftvol;
	int mic_maxvol;
	int hp_maxvol;
};
#endif
#endif /*__ASOC_INGENIC_H__*/
