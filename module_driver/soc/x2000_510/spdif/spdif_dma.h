#ifndef _SPDIF_DMA_H_
#define _SPDIF_DMA_H_

#include <sound/soc.h>

int spdif_dma_pcm_open(struct snd_soc_component *component,
			   struct snd_pcm_substream *substream);
int spdif_dma_pcm_close(struct snd_soc_component *component,
			    struct snd_pcm_substream *substream);
int spdif_dma_pcm_prepare(struct snd_soc_component *component,
		       struct snd_pcm_substream *substream);
int spdif_dma_mmap(struct snd_soc_component *component,
		    struct snd_pcm_substream *substream,
		    struct vm_area_struct *vma);
int spdif_dma_pcm_hw_params(struct snd_soc_component *component,
				struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *hw_params);
int spdif_dma_pcm_hw_free(struct snd_soc_component *component,
		       struct snd_pcm_substream *substream);
int spdif_dma_trigger(struct snd_soc_component *component,
			      struct snd_pcm_substream *substream, int cmd);
snd_pcm_uframes_t spdif_dma_pcm_pointer(struct snd_soc_component *component,
					    struct snd_pcm_substream *substream);
int spdif_dma_pcm_new(struct snd_soc_component *component,
			  struct snd_soc_pcm_runtime *rtd);
void spdif_dma_pcm_free(struct snd_soc_component *component,
			     struct snd_pcm *pcm);

#endif /* _SPDIF_DMA_H_ */
