#ifndef __IOAPIC_H__
#define __IOAPIC_H__

#define IOAPIC_REG_SELECT		0x00	// RW
#define IOAPIC_REG_WINDOW		0x10	// RW

#define IOAPIC_IDX_ID			0x00	// RW
#define IOAPIC_IDX_VERSION		0x01	// R
#define IOAPIC_IDX_ARBITRATION_ID	0x02	// R
#define IOAPIC_IDX_REDIRECTION_TABLE	0x10	// ~0x3f RW, Redirection Table 24 entries * 64 bits

void ioapic_init();

uint32_t ioapic_read32(int idx);
void ioapic_write32(int idx, uint32_t v);
uint64_t ioapic_read64(int idx);
void ioapic_write64(int idx, uint64_t v);

#endif /* __IOAPIC_H__ */
