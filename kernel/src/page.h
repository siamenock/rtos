#ifndef __PAGE_H__
#define __PAGE_H__

#include <stdint.h>

typedef union {
	uint64_t raw;
	
	struct {
		uint64_t p: 1;		// Present
		uint64_t rw: 1;		// Read/Write
		uint64_t us: 1;		// User/Supevisor
		uint64_t pwt: 1;	// Page Level Write-through
		uint64_t pcd: 1;	// Page Level Cache Disable
		uint64_t a: 1;		// Accessed
		uint64_t reserved_lower: 3;
		uint64_t avail_lower: 3;
		uint64_t base: 28;
		uint64_t reserved_upper: 12;
		uint64_t avail_upper: 11;
		uint64_t exb: 1;	// Execute Disable
	} s;
} __attribute__ ((packed)) PageDirectory;

typedef union {
	uint64_t raw;
	
	struct {
		uint64_t p: 1;		// Present
		uint64_t rw: 1;		// Read/Write
		uint64_t us: 1;		// User/Supervisor
		uint64_t pwt: 1;	// Page Level Write-through
		uint64_t pcd: 1;	// Page Cache Disable
		uint64_t a: 1;		// Accessed
		uint64_t d: 1;		// Dirty
		uint64_t ps: 1;		// Page Size
		uint64_t g: 1;		// Global
		uint64_t avail_lower: 3;
		uint64_t pat: 1;	// Page Attribute Table Index
		uint64_t reserved_lower: 8;
		uint64_t base: 19;
		uint64_t reserved_upper: 12;
		uint64_t avail_upper: 11;
		uint64_t exb: 1;	// Execute Disable
	} s;
} __attribute__ ((packed)) PageTable;

#endif /* __PAGE_H__ */

