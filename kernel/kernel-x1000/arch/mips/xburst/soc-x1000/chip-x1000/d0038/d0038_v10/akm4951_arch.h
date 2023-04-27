#ifndef __AKM4951_ARCH_H__
#define __AKM4951_ARCH_H__

#include <linux/types.h>

struct akm4951_arch_data {
	uint8_t (*register_array)[2];
	int array_length;
	int (*volume_to_gain)(int vol);
};

#endif /* __AKM4951_ARCH_H__ */
