#include "asm.h"
#include "mp.h"
#include "isr.h"
#include "elf.h"

#include "idt.h"

void idt_init() {
	// NOTE: Although kernel load IDTR reigster by virutal address, //	 we must access IDTR physically to write here.
	IDTR* idtr = (IDTR*)VIRTUAL_TO_PHYSICAL(IDTR_ADDR);
	IDTR_INIT(*idtr);

	idtr->limit = IDT_END_ADDR - IDT_ADDR - 1;
	idtr->base = (uint64_t)IDT_ADDR;
	ID* id = (ID*)VIRTUAL_TO_PHYSICAL(IDT_ADDR);

	ID_INIT(id[0], (uint64_t)elf_get_symbol("isr_0"), 0x08, 1, 0x0e, 0, 1);	// Segment: 0x08, IST: 1, Type: Interrupt, DPL: 0, P: 1
	ID_INIT(id[1], (uint64_t)elf_get_symbol("isr_1"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[2], (uint64_t)elf_get_symbol("isr_2"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[3], (uint64_t)elf_get_symbol("isr_3"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[4], (uint64_t)elf_get_symbol("isr_4"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[5], (uint64_t)elf_get_symbol("isr_5"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[6], (uint64_t)elf_get_symbol("isr_6"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[7], (uint64_t)elf_get_symbol("isr_7"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[8], (uint64_t)elf_get_symbol("isr_8"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[9], (uint64_t)elf_get_symbol("isr_9"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[10], (uint64_t)elf_get_symbol("isr_10"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[11], (uint64_t)elf_get_symbol("isr_11"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[12], (uint64_t)elf_get_symbol("isr_12"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[13], (uint64_t)elf_get_symbol("isr_13"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[14], (uint64_t)elf_get_symbol("isr_14"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[15], (uint64_t)elf_get_symbol("isr_15"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[16], (uint64_t)elf_get_symbol("isr_16"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[17], (uint64_t)elf_get_symbol("isr_17"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[18], (uint64_t)elf_get_symbol("isr_18"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[19], (uint64_t)elf_get_symbol("isr_19"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[20], (uint64_t)elf_get_symbol("isr_20"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[21], (uint64_t)elf_get_symbol("isr_21"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[22], (uint64_t)elf_get_symbol("isr_22"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[23], (uint64_t)elf_get_symbol("isr_23"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[24], (uint64_t)elf_get_symbol("isr_24"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[25], (uint64_t)elf_get_symbol("isr_25"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[26], (uint64_t)elf_get_symbol("isr_26"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[27], (uint64_t)elf_get_symbol("isr_27"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[28], (uint64_t)elf_get_symbol("isr_28"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[29], (uint64_t)elf_get_symbol("isr_29"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[30], (uint64_t)elf_get_symbol("isr_30"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[31], (uint64_t)elf_get_symbol("isr_31"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[32], (uint64_t)elf_get_symbol("isr_32"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[33], (uint64_t)elf_get_symbol("isr_33"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[34], (uint64_t)elf_get_symbol("isr_34"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[35], (uint64_t)elf_get_symbol("isr_35"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[36], (uint64_t)elf_get_symbol("isr_36"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[37], (uint64_t)elf_get_symbol("isr_37"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[38], (uint64_t)elf_get_symbol("isr_38"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[39], (uint64_t)elf_get_symbol("isr_39"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[40], (uint64_t)elf_get_symbol("isr_40"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[41], (uint64_t)elf_get_symbol("isr_41"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[42], (uint64_t)elf_get_symbol("isr_42"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[43], (uint64_t)elf_get_symbol("isr_43"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[44], (uint64_t)elf_get_symbol("isr_44"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[45], (uint64_t)elf_get_symbol("isr_45"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[46], (uint64_t)elf_get_symbol("isr_46"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[47], (uint64_t)elf_get_symbol("isr_47"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[48], (uint64_t)elf_get_symbol("isr_48"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[49], (uint64_t)elf_get_symbol("isr_49"), 0x08, 2, 0x0e, 0, 1);	// User -> Kernel context switching
	ID_INIT(id[50], (uint64_t)elf_get_symbol("isr_50"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[51], (uint64_t)elf_get_symbol("isr_51"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[52], (uint64_t)elf_get_symbol("isr_52"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[53], (uint64_t)elf_get_symbol("isr_53"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[54], (uint64_t)elf_get_symbol("isr_54"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[55], (uint64_t)elf_get_symbol("isr_55"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[56], (uint64_t)elf_get_symbol("isr_56"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[57], (uint64_t)elf_get_symbol("isr_57"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[58], (uint64_t)elf_get_symbol("isr_58"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[59], (uint64_t)elf_get_symbol("isr_59"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[60], (uint64_t)elf_get_symbol("isr_60"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[61], (uint64_t)elf_get_symbol("isr_61"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[62], (uint64_t)elf_get_symbol("isr_62"), 0x08, 1, 0x0e, 0, 1);
	ID_INIT(id[63], (uint64_t)elf_get_symbol("isr_63"), 0x08, 1, 0x0e, 0, 1);
	int i;
	for(i = 64; i < 100; i++) {
		ID_INIT(id[i], (uint64_t)elf_get_symbol("isr_63"), 0x08, 1, 0x0e, 0, 1);
	}
}

void idt_load() {
	IDTR* idtr = (IDTR*)VIRTUAL_TO_PHYSICAL(IDTR_ADDR);
	lidt(idtr);
}
