#include <lock.h>
#include "lock_test.h"

A_Test void test_lock_lock() {
	volatile uint8_t lock = 0;
	lock_lock(&lock);
	assertEqualsM("Lock_lock must make lock 1 when unlocked", 1, lock);
}

A_Test void test_lock_unlock() {
	volatile uint8_t lock = 1;
	lock_unlock(&lock);
	assertEqualsM("Lock_unlock must make lock 0 when locked", 0, lock);
}
