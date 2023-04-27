#ifndef __aw9163_h__
#define __aw9163_h__

#include <linux/platform_device.h>

struct aw9163_io_data {
    int aw9163_pdn;
    int aw9163_int;
    int bus_number;
};

#endif
