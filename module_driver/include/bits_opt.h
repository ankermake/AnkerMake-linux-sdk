#ifndef _BITS_OPT_H_
#define _BITS_OPT_H_

#ifndef BIT
#define BIT(bit) (1ul << (bit))
#endif

#define set_bits(val, bits) \
    do { \
        (val) |= (bits); \
    } while (0)

#define clear_bits(val, bits) \
    do { \
        (val) &= ~(bits); \
    } while (0)

#define bits_mask(start, end) \
    (((1ul << ((end) - (start) + 1)) - 1) << (start))

#endif /* _BITS_OPT_H_ */