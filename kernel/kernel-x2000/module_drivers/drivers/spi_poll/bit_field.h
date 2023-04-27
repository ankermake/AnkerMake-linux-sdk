#ifndef _BIT_FIELD_H_
#define _BIT_FIELD_H_

static inline int bit_field_start(int start, int end)
{
    return start;
}

static inline int bit_field_end(int start, int end)
{
    return end;
}

static inline int bit_field_len(int start, int end)
{
    return end - start + 1;
}

/**
 * the max value of bit field "[start, end]"
 */
static inline unsigned long bit_field_max(int start, int end)
{
    return (1ul << (end - start + 1)) - 1;
}

/**
 * the mask of bit field
 * mask = bit_field_max(start, end) << start;
 */
static inline unsigned long bit_field_mask(int start, int end)
{
    return bit_field_max(start, end) << start;
}

/**
 * check value is valid for the bit field
 * return bit_field_max(start, end) >= val;
 */
static inline int check_bit_field(int start, int end, unsigned long val)
{
    return bit_field_max(start, end) >= val;
}

/**
 * return the bitfield value
 */
static inline unsigned long bit_field_val(int start, int end, unsigned long val)
{
    unsigned long mask = bit_field_mask(start, end);
    return ((val << start) & mask);
}

/**
 * set value to the bit field
 * reg[start, end] = val;
 * @attention: this function do not check the value is valid for the bit field or not
 */
static inline void set_bit_field(volatile unsigned long *reg, int start, int end, unsigned long val)
{
    unsigned long mask = bit_field_mask(start, end);
    *reg = (*reg & ~mask) | ((val << start) & mask);
}

/**
 * get value in the bit field
 * return reg[start, end];
 */
static inline unsigned long get_bit_field(volatile unsigned long *reg, int start, int end)
{
    return (*reg & bit_field_mask(start, end)) >> start;
}

#endif /* _BIT_FIELD_H_ */
