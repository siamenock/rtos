#include "mp.h"
//#include "isr.h"

#include "idt.h"

void idt_init() {
	IDTR* idtr = (IDTR*)IDTR_ADDR;
	IDTR_INIT(*idtr);
	idtr->limit = IDT_END_ADDR - IDT_ADDR - 1;
	idtr->base = PHYSICAL_TO_VIRTUAL(IDT_ADDR);
	
	//TODO Read isr address from elf
	ID* id = (ID*)IDT_ADDR;
	uint64_t __offset;
 	ID_INIT(id[0], isr_0, 0x08, 1, 0x0e, 0, 1);	// Segment: 0x08, IST: 1, Type: Interrupt, DPL: 0, P: 1
 	ID_INIT(id[1], isr_1, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[2], isr_2, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[3], isr_3, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[4], isr_4, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[5], isr_5, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[6], isr_6, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[7], isr_7, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[8], isr_8, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[9], isr_9, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[10], isr_10, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[11], isr_11, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[12], isr_12, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[13], isr_13, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[14], isr_14, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[15], isr_15, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[16], isr_16, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[17], isr_17, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[18], isr_18, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[19], isr_19, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[20], isr_20, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[21], isr_21, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[22], isr_22, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[23], isr_23, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[24], isr_24, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[25], isr_25, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[26], isr_26, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[27], isr_27, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[28], isr_28, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[29], isr_29, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[30], isr_30, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[31], isr_31, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[32], isr_32, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[33], isr_33, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[34], isr_34, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[35], isr_35, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[36], isr_36, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[37], isr_37, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[38], isr_38, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[39], isr_39, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[40], isr_40, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[41], isr_41, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[42], isr_42, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[43], isr_43, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[44], isr_44, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[45], isr_45, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[46], isr_46, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[47], isr_47, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[48], isr_48, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[49], isr_49, 0x08, 2, 0x0e, 0, 1);	// User -> Kernel context switching
 	ID_INIT(id[50], isr_50, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[51], isr_51, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[52], isr_52, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[53], isr_53, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[54], isr_54, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[55], isr_55, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[56], isr_56, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[57], isr_57, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[58], isr_58, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[59], isr_59, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[60], isr_60, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[61], isr_61, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[62], isr_62, 0x08, 1, 0x0e, 0, 1);
 	ID_INIT(id[63], isr_63, 0x08, 1, 0x0e, 0, 1);
 	int i;
 	for(i = 64; i < 100; i++) {
 		ID_INIT(id[i], isr_63, 0x08, 1, 0x0e, 0, 1);
 	}
}
