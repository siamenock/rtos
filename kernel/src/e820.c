#include <stdio.h>
#include <malloc.h>

#include "e820.h"
#include "multiboot_tag.h"

List* e820_get_mem_blocks() {
	//struct multiboot_tag_mmap* mmap = find_mmap();
	struct multiboot_tag_mmap* mmap = multiboot_tag_get(MULTIBOOT_TAG_TYPE_MMAP);
	if(!mmap) {
		printf("MMAP information not found!\n");
		while(1) asm("hlt");
	}

	int count = (mmap->size - 8) / mmap->entry_size;
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

		printf("\t0x%016lx - 0x%016lx: %s(%d)\n", entry->addr, entry->addr + entry->len, type, entry->type);
	}

	return blocks;
}
