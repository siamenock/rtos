#ifndef	__LINUX_BITOPS_H__
#define __LINUX_BITOPS_H__

#define BITS_PER_BYTE		8
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

int test_and_set_bit(int nr, volatile unsigned long *addr);
int test_and_clear_bit(int nr, volatile unsigned long *addr);
void set_bit(int nr, volatile unsigned long *addr);
void clear_bit(int nr, volatile unsigned long *addr);
int test_bit(int nr, const volatile unsigned long *addr);

#endif /* __LINUX_BITOPS_H__ */
