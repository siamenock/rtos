#include <stdio.h>
// Kernel header
#include "../gmalloc.h"
// Generated header
#include "gmalloc.h"

A_Test void test_gmalloc_repeat() {
	// Malloc & free repetition
	void* ptr;
	for(int i = 0; i < 100000; i++) {
		ptr = gmalloc(10000);	
		assertNotNullM("gmalloc should be return valid pointer "
				"even when repeated many times", ptr);
		gfree(ptr);
	}
}

A_Test void test_gmalloc_max() {
	// Maxmim size allocation
	void* ptr = gmalloc(gmalloc_total() - gmalloc_used());
	assertNotNullM("gmalloc should be return valid pointer "
			"when available maxmimum size requested", ptr);
		
	gfree(ptr);
	
	// Over maximum size allocation
	ptr = gmalloc(gmalloc_total() - gmalloc_used() + 1);
	assertNotNullM("gmalloc should be return pointer "
			"even when over maximum size requested", ptr);
		
	gfree(ptr);
}
