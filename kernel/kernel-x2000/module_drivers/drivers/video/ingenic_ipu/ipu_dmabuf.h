#include <linux/device.h>

struct ipu_dma_buffer {
	u32 size;
	dma_addr_t dma_handle;
	void *cpu_handle;
};

struct ipu_buffer_info {
	u32 bus_address;
	u32 size;
};

struct ipu_dma_info {
    __u32 fd;
    __u32 size;
    __u32 phy_addr;
};


int ipu_ioctl_get_dma_fd(struct device *dev, unsigned long arg);
int ipu_ioctl_get_dmabuf_dma_addr(struct device *dev, unsigned long arg);
int ipu_ioctl_dmabuf_cache_sync(struct device *dev, unsigned long arg);
