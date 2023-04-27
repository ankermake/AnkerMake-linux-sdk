#ifndef __ASOC_DMA_ENCRYPT_H__
#define __ASOC_DMA_ENCRYPT_H__

static int inline __data_encrypt_init(void *handle, unsigned char *buf, struct snd_pcm_hw_params *params)
{
	printk("dummy data_encrypt_init function\n");
	return 0;
}

static int inline __data_encrypt(void *handle, unsigned char *buf, unsigned int len)
{
	printk("dummy data_encrypt function\n");
	return 0;
}

static inline void * __data_encrypt_acquire(struct device *dev)
{
	printk("dummy data_open function\n");
	return NULL;
}

static inline void __data_encrypt_release(void *handle)
{
	printk("dummy data_close function\n");
	return;
}

int data_encrypt_init(void *handle, unsigned char *, struct snd_pcm_hw_params *) __attribute__ ((weak,alias("__data_encrypt_init")));
int data_encrypt(void *handle, unsigned char *, unsigned int) __attribute__ ((weak,alias("__data_encrypt")));
void* data_encrypt_acquire(struct device *dev) __attribute__ ((weak,alias("__data_encrypt_acquire")));
void data_encrypt_release(void *) __attribute__ ((weak,alias("__data_encrypt_release")));

#endif /*__ASOC_DMA_ENCRYPT_H__*/
