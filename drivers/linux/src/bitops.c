#include <asm/bitops.h>

int test_and_set_bit(int nr, volatile unsigned long *addr) {
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old = *p;
	
	*p = old | mask;
	return (old & mask) != 0;
}

int test_and_clear_bit(int nr, volatile unsigned long *addr) {
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old = *p;
	
	*p = old & ~mask;
	return (old & mask) != 0;
}

void set_bit(int nr, volatile unsigned long *addr) {
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	
	*p |= mask;
}

void clear_bit(int nr, volatile unsigned long *addr) {
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	
	*p &= ~mask;
}

int test_bit(int nr, const volatile unsigned long *addr) {
	return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}


