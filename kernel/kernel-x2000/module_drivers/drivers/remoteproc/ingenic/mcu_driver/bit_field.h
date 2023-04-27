#ifndef _BIT_FIELD2_H_
#define _BIT_FIELD2_H_


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
    return val << start;
}

/**
 * set value to the bit field
 * reg[start, end] = val;
 * @attention: this function do not check the value is valid for the bit field or not
 */
static inline unsigned long set_bit_field(unsigned long reg, int start, int end, unsigned long val)
{
    return (reg & ~bit_field_mask(start, end)) | (val << start);
}

/**
 * get value in the bit field
 * return reg[start, end];
 */
static inline unsigned long get_bit_field(unsigned long reg, int start, int end)
{
    return (reg >> start) & bit_field_max(start, end);
}

#endif /* _BIT_FIELD2_H_ */
