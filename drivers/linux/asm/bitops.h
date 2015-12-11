#ifndef __ASM_BITOPS_H__
#define __ASM_BITOPS_H__

#define BIT(nr)                 (1UL << (nr))
#define BIT_ULL(nr)		(1ULL << (nr))
#define BITS_PER_LONG		64
#define BIT_MASK(nr)            (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#define BITS_PER_BYTE           8
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

#define DECLARE_BITMAP(name,bits) \
        unsigned long name[BITS_TO_LONGS(bits)]

int test_and_set_bit(int nr, volatile unsigned long *addr);
int test_and_clear_bit(int nr, volatile unsigned long *addr);
void set_bit(int nr, volatile unsigned long *addr);
void clear_bit(int nr, volatile unsigned long *addr);
int test_bit(int nr, const volatile unsigned long *addr);

static inline void clear_bit_unlock(long nr, volatile unsigned long *addr) {
	clear_bit(nr, addr);
}
#endif /* __ASM_BITOPS_H__ */
