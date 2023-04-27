#ifndef _RING_MEM_H_
#define _RING_MEM_H_

struct ring_mem {
    unsigned long mem_addr;
    unsigned int mem_size;
    unsigned int write_pos;
    unsigned int read_pos;

    void *write_addr;
    void *read_addr;
};

#define DEFINE_RING_MEM(ringname, _array) \
    struct ring_mem ringname = { \
        .mem_addr = (unsigned long)_array, \
        .mem_size = sizeof(_array), \
        .write_pos = 0, \
        .read_pos = 0, \
        .write_addr = _array, \
        .read_addr = _array, \
    }

void ring_mem_init(struct ring_mem *ring, void *mem, int size);

int ring_mem_readable_size(struct ring_mem *ring);

int ring_mem_writable_size(struct ring_mem *ring);

int ring_mem_write(struct ring_mem *ring, void *buf, unsigned int size);

int ring_mem_read(struct ring_mem *ring, void *buf, unsigned int size);

void ring_mem_set_virt_addr_for_write(struct ring_mem *ring, void *write_addr);

void ring_mem_set_virt_addr_for_read(struct ring_mem *ring, void *read_addr);

#endif /* _RING_MEM_H_ */
