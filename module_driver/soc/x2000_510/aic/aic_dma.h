#ifndef _AIC_DMA_H_
#define _AIC_DMA_H_

#include <sound/soc.h>

int aic_dma_pcm_open(struct snd_soc_component *component,
			   struct snd_pcm_substream *substream);
int aic_dma_pcm_close(struct snd_soc_component *component,
			    struct snd_pcm_substream *substream);
int aic_dma_pcm_prepare(struct snd_soc_component *component,
		       struct snd_pcm_substream *substream);
int aic_dma_mmap(struct snd_soc_component *component,
		    struct snd_pcm_substream *substream,
		    struct vm_area_struct *vma);
int aic_dma_pcm_hw_params(struct snd_soc_component *component,
				struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *hw_params);
int aic_dma_pcm_hw_free(struct snd_soc_component *component,
		       struct snd_pcm_substream *substream);
int aic_dma_trigger(struct snd_soc_component *component,
			      struct snd_pcm_substream *substream, int cmd);
snd_pcm_uframes_t aic_dma_pcm_pointer(struct snd_soc_component *component,
					    struct snd_pcm_substream *substream);
int aic_dma_pcm_new(struct snd_soc_component *component,
			  struct snd_soc_pcm_runtime *rtd);
void aic_dma_pcm_free(struct snd_soc_component *component,
			     struct snd_pcm *pcm);

#endif /* _AIC_DMA_H_ */
