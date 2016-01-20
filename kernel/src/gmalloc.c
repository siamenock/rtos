#include <stdio.h>
#include <string.h>
#include <tlsf.h>
#include <util/list.h>
#include <malloc.h>
#include "idt.h"
#include "pnkc.h"
#include "mp.h"
#include "shared.h"
#include "multiboot2.h"
#include "malloc.h"
#include "gmalloc.h"

void* gmalloc_pool;

uint32_t bmalloc_count;
uint64_t* bmalloc_pool;

static struct multiboot_tag_mmap* find_mmap() {
	PNKC* pnkc = (PNKC*)(0x200200 - sizeof(PNKC));
	uintptr_t addr = 0x200000 + (((uintptr_t)pnkc->smap_offset + (uintptr_t)pnkc->smap_size + 7) & ~7);
	uintptr_t end = addr + *(uint32_t*)addr;
	addr += 8;
	
	while(addr < end) {
		struct multiboot_tag* tag = (struct multiboot_tag*)addr;
		switch(tag->type) {
			case MULTIBOOT_TAG_TYPE_END:
				return NULL;
			case MULTIBOOT_TAG_TYPE_CMDLINE:
			case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
			case MULTIBOOT_TAG_TYPE_MODULE:
			case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
			case MULTIBOOT_TAG_TYPE_BOOTDEV:
				break;
			case MULTIBOOT_TAG_TYPE_MMAP:
				return (struct multiboot_tag_mmap*)tag;
			case MULTIBOOT_TAG_TYPE_VBE:
			case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
			case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
			case MULTIBOOT_TAG_TYPE_APM:
			case MULTIBOOT_TAG_TYPE_EFI32:
			case MULTIBOOT_TAG_TYPE_EFI64:
			case MULTIBOOT_TAG_TYPE_SMBIOS:
			case MULTIBOOT_TAG_TYPE_ACPI_OLD:
			case MULTIBOOT_TAG_TYPE_ACPI_NEW:
			case MULTIBOOT_TAG_TYPE_NETWORK:
			case MULTIBOOT_TAG_TYPE_EFI_MMAP:
			case MULTIBOOT_TAG_TYPE_EFI_BS:
				break;
		}
		
		addr = (addr + tag->size + 7) & ~7;
	}
	return NULL;
}

void gmalloc_init() {
	uint64_t start = VIRTUAL_TO_PHYSICAL(IDT_END_ADDR);
	uint64_t end = VIRTUAL_TO_PHYSICAL((uint64_t)shared);
	
	init_memory_pool(end - start, (void*)start, 0);
	
	gmalloc_pool = (void*)start;
	
	// Reserved memory blocks
	typedef struct {
		uintptr_t start;
		uintptr_t end;
	} Block;
	
	Block reserved[2 + MP_MAX_CORE_COUNT];
	int reserved_count = 0;
	reserved[reserved_count].start = 0x100000;	// Description table
	reserved[reserved_count].end = 0x200000;
	reserved_count++;
	
	reserved[reserved_count].start = 0x200000;	// Kernel global
	reserved[reserved_count].end = 0x400000;
	reserved_count++;
	
	reserved[reserved_count].start = 0x400000 + 0x200000 * MP_MAX_CORE_COUNT;	// RAM disk
	reserved[reserved_count].end = 0x400000 + 0x200000 * (MP_MAX_CORE_COUNT + 1);
	reserved_count++;
	
	uint8_t* core_map = mp_core_map();
	for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
		if(!core_map[i])
			continue;
		
		reserved[reserved_count].start = 0x400000 + 0x200000 * i;
		reserved[reserved_count].end = 0x400000 + 0x200000 * (i + 1);
		reserved_count++;
	}
	
	// Find mmap infomation structure
	struct multiboot_tag_mmap* mmap = find_mmap();
	if(!mmap) {
		printf("MMAP information not found!\n");
		while(1) asm("hlt");
	}
	
	int count = (mmap->size - 8) / mmap->entry_size;
	
	// Print MMAP information and find available blocks
	List* blocks = list_create(NULL);
	printf("System memory map\n");
	for(int i = 0; i < count; i++) {
		struct multiboot_mmap_entry* entry = &mmap->entries[i];
		char* type;
		switch(entry->type) {
			case MULTIBOOT_MEMORY_AVAILABLE:
				type = "Memory";
				Block* block = malloc(sizeof(Block));
				block->start = entry->addr;
				block->end = entry->addr + entry->len;
				list_add(blocks, block);
				break;
			case MULTIBOOT_MEMORY_RESERVED:
				type = "Reserved";
				break;
			case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
				type = "ACPI";
				break;
			case MULTIBOOT_MEMORY_NVS:
				type = "NVS";
				break;
			case MULTIBOOT_MEMORY_BADRAM:
				type = "Disabled";
				break;
			default:
				type = "Unknown";
		}
		
		printf("\t0x%016x - 0x%016x: %s(%d)\n", entry->addr, entry->addr + entry->len, type, entry->type);
	}
	
	// Remove reserved blocks
	for(int i = 0; i < list_size(blocks); i++) {
		Block* block = list_get(blocks, i);
		
		for(int j = 0; j < reserved_count; j++) {
			if(reserved[j].start >= block->end || reserved[j].end <= block->start) {
				continue;
			} else if(reserved[j].start <= block->start && reserved[j].end >= block->end) {
				free(block);
				list_remove(blocks, i);
				i--;
			} else if(reserved[j].start > block->start && reserved[j].end < block->end) {
				Block* block2 = malloc(sizeof(Block));
				block2->start = block->start;
				block2->end = reserved[j].start;
				list_add_at(blocks, i, block2);
				
				block->start = reserved[j].end;
			} else if(reserved[j].start > block->start && reserved[j].start <= block->end) {
				block->end = reserved[j].start;
			} else if(reserved[j].end >= block->start && reserved[j].end < block->end) {
				block->start = reserved[j].end;
			}
		}
	}
	
	// Extend gmalloc pool
	ListIterator iter;
	list_iterator_init(&iter, blocks);
	while(list_iterator_has_next(&iter)) {
		Block* b = list_iterator_next(&iter);
		uintptr_t start = (b->start + 0x200000 - 1) & ~((uintptr_t)0x200000 - 1);
		uintptr_t end = b->end & ~((uintptr_t)0x200000 - 1);
		
		if(start >= end) {
			add_new_area((void*)b->start, b->end, gmalloc_pool);
			b->start = b->end = 0;
		} else {
			if(start > b->start) {
				add_new_area((void*)b->start, start - b->start, gmalloc_pool);
				b->start = start;
			}
			
			if(end < b->end) {
				add_new_area((void*)end, b->end - end, gmalloc_pool);
				b->end = end;
			}
		}
		
		if(b->start < b->end) {
			bmalloc_count += (end - start) / 0x200000;
		} else {
			free(b);
			list_iterator_remove(&iter);
		}
	}
	
	// extend bmalloc pool
	bmalloc_pool = malloc(sizeof(uint64_t) * bmalloc_count);
	uint32_t bmalloc_index = 0;
	list_iterator_init(&iter, blocks);
	while(list_iterator_has_next(&iter)) {
		Block* b = list_iterator_next(&iter);
		uintptr_t start = b->start;
		uintptr_t end = b->end;
		
		while(start < end) {
			bmalloc_pool[bmalloc_index++] = start;
			start += 0x200000;
		}

		free(b);
		list_iterator_remove(&iter);
	}
	
	list_destroy(blocks);
	
	void print_pool(char* message, size_t total) {
		size_t mb = total / 1024 / 1024;
		size_t kb = total / 1024 - mb * 1024;
		printf("%s: %d.%dMB ", message, mb, kb);
	}
	
	printf("Memory pool: ");
	print_pool("local", malloc_total());
	print_pool("global", gmalloc_total());
	print_pool("block", bmalloc_total());
	printf("\n");
}

//void gmalloc_extend() {
//	#define MAX(a,b) ((a) > (b) ? (a) : (b))
//	
//	typedef struct {
//		uint64_t start;
//		uint64_t end;
//	} Block;
//	
//	// Make reserved blocks
//	Block reserved[3];
//	reserved[0].start = 0x100000;	// Description table
//	reserved[0].end = 0x200000;
//
//	PNKC* pnkc = (PNKC*)(0x200200 /* Kernel entry end */ - sizeof(PNKC));
//
//	reserved[1].start = 0x200000;	// Kernel global
//	reserved[1].end = 0x200000 + MAX(pnkc->text_offset + pnkc->text_size, pnkc->rodata_offset + pnkc->rodata_size);
//	
//	reserved[2].start = 0x400000;	// Kernel local
//	reserved[2].end = 0x400000 + 0x200000 * mp_core_count();
//	
////	// TODO: BFS dependent code. Need to extract common private data for serveral file systems
////	BFSPriv* priv = (BFSPriv*)driver->priv;
////	reserved[3].start = priv->reserved_area_addr;
////	reserved[3].end = reserved[3].start + priv->total_cluster_count * 512; 
////	// TODO: It's actually sector count. Need to change sector to cluster. 
//	
//	// Get free blocks
//	List* blocks = list_create(NULL);
//	
//	uint64_t total_size = 0;
//
//	printf("\tBIOS e820 System Address Map\n");
//	for(uint32_t i = 0; i < smap_count; i++) {
//		char* type;
//		uint64_t end = (uint64_t)smap[i].base + smap[i].length;
//		switch(smap[i].type) {
//			case SMAP_TYPE_MEMORY:
//				type = "Memory";
//				
//				total_size = total_size > end ? total_size : end;
//				
//				Block* block = malloc(sizeof(Block));
//				block->start = smap[i].base;
//				block->end = smap[i].base + smap[i].length;
//
//				list_add(blocks, block);
//				break;
//			case SMAP_TYPE_RESERVED:
//				type = "Reserved";
//				break;
//			case SMAP_TYPE_ACPI:
//				type = "ACPI";
//				break;
//			case SMAP_TYPE_NVS:
//				type = "NVS";
//				break;
//			case SMAP_TYPE_UNUSABLE:
//				type = "Unusable";
//				break;
//			case SMAP_TYPE_DISABLED:
//				type = "Disabled";
//				break;
//			default:
//				type = "Unknown";
//		}
//		
//		printf("\t\t0x%016x - 0x%016x: %s(%d)\n", smap[i].base, end, type, smap[i].extended);
//	}
//
//	// 1MB align
//	total_size = ((total_size + 0x100000 - 1) / 0x100000) * 0x100000;
//	printf("\tTotal memory size: %ld MB\n", total_size / 0x100000);
//	memory_total = total_size;
//	
//	// Remove reserved
//	for(int i = 0; i < list_size(blocks); i++) {
//		Block* b = list_get(blocks, i);
//		
//		for(int j = 0; j < 4; j++) {
//			Block* r = &reserved[j];
//			if(r->start >= b->end || r->end <= b->start) {
//				continue;
//			} else if(r->start <= b->start && r->end >= b->end) {
//				free(b);
//				list_remove(blocks, i);
//				i--;
//			} else if(r->start > b->start && r->end < b->end) {
//				Block* c = malloc(sizeof(Block));
//				c->start = b->start;
//				c->end = r->start;
//				list_add_at(blocks, i, c);
//				
//				b->start = r->end;
//			} else if(r->start > b->start && r->start <= b->end) {
//				b->end = r->start;
//			} else if(r->end >= b->start && r->end < b->end) {
//				b->start = r->end;
//			}
//		}
//	}
//	
//	// Allocate 2MB blocks
//	List* blocks_2mb = list_create(NULL);
//	
//	for(int i = 0; i < list_size(blocks); i++) {
//		Block* b = list_get(blocks, i);
//		
//		uint64_t start = (b->start + (0x200000 - 1)) & ~(0x200000 - 1);
//		uint64_t end = b->end & ~(0x200000 - 1);
//	
//		if(start + 0x200000 <= end) {
//			// Head partial
//			if(b->start != start) {
//				Block* partial = malloc(sizeof(Block));
//				partial->start = b->start;
//				partial->end = start;
//				
//				list_add_at(blocks, i, partial);
//				i++;
//			}
//			
//			while(start + 0x200000 <= end) {
//				Block* _2mb = malloc(sizeof(Block));
//				_2mb->start = start;
//				start += 0x200000;
//				_2mb->end = start;
//				b->start = start;
//	
//				list_add(blocks_2mb, _2mb);
//			}
//			
//			// Tail partial
//			if(b->start == b->end) {
//				list_remove(blocks, i);
//				i--;
//			}
//		}
//	}
//	
//	// bmalloc blocks
//	bmalloc_count = list_size(blocks_2mb);
//	bmalloc_pool = malloc(sizeof(uint64_t) * bmalloc_count);
//	ListIterator iter;
//	list_iterator_init(&iter, blocks_2mb);
//	int i = 0;
//	while(list_iterator_has_next(&iter)) {
//		Block* b = list_iterator_next(&iter);
//		bmalloc_pool[i++] = b->start;
//		free(b);
//	}
//	list_destroy(blocks_2mb);
//
//	// gmalloc blocks
//	list_iterator_init(&iter, blocks);
//	while(list_iterator_has_next(&iter)) {
//		Block* b = list_iterator_next(&iter);
//		add_new_area((void*)b->start, b->end - b->start, gmalloc_pool);
//		free(b);
//	}
//	list_destroy(blocks);
//}

inline size_t gmalloc_total() {
	return get_total_size(gmalloc_pool);
}

inline size_t gmalloc_used() {
	return get_used_size(gmalloc_pool);
}

inline void* gmalloc(size_t size) {
//	if(mp_core_id() != 0) {
//		printf("Core %d gmalloc\n", mp_core_id());
//		return NULL;
//	}
	
	do {
		void* ptr = malloc_ex(size, gmalloc_pool);
		if(ptr)
			return ptr;

		// TODO: print to stderr
		printf("WARN: Not enough global memory!!!\n");
		
		void* block = bmalloc();
		if(!block) {
			// TODO: print to stderr
			printf("ERROR: Not enough block memory!!!\n");
			return NULL;
		}
		
		add_new_area(block, 0x200000, gmalloc_pool);
	} while(1);
}

inline void gfree(void* ptr) {
	free_ex(ptr, gmalloc_pool);
}

inline void* grealloc(void* ptr, size_t size) {
	return realloc_ex(ptr, size, gmalloc_pool);
}

inline void* gcalloc(uint32_t nmemb, size_t size) {
	return calloc_ex(nmemb, size, gmalloc_pool);
}

void* bmalloc() {
//	if(mp_core_id() != 0) {
//		printf("Core %d bmalloc\n", mp_core_id());
//		return NULL;
//	}
	
	for(int i = 0; i < bmalloc_count; i++) {
		if(!(bmalloc_pool[i] & 0x01)) {
			void* ptr = (void*)bmalloc_pool[i];
			bmalloc_pool[i] |= 0x01;
			return ptr;
		}
	}
	
	// TODO: print to stderr
	printf("Not enough block memory!!!");
	
	return NULL;
}

void bfree(void* ptr) {
	uint64_t addr = (uint64_t)ptr;
	addr |= 0x01;
	
	for(int i = 0; i < bmalloc_count; i++) {
		if(bmalloc_pool[i] == addr) {
			bmalloc_pool[i] ^= 0x01;
			break;
		}
	}
}

size_t bmalloc_total() {
	return bmalloc_count * 0x200000;
}

size_t bmalloc_used() {
	size_t size = 0;
	for(int i = 0; i < bmalloc_count; i++) {
		if(bmalloc_pool[i] & 0x01) {
			size += 0x200000;
		}
	}
	
	return size;
}
