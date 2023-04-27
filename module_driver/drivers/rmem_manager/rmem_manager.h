#ifndef _RMEM_MANAGER_H
#define _RMEM_MANAGER_H


void *rmem_alloc(int size);
void *rmem_alloc_aligned(int size, int align);
void rmem_free(void *ptr, int size);

#endif