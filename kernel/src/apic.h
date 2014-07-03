#ifndef __APIC_H__
#define __APIC_H__

#include <stdint.h>
#include <stdbool.h>

// Ref: http://www.osdever.net/tutorials/view/multiprocessing-support-for-hobby-oses-explained
// Ref: http://download.intel.com/design/archives/processors/pro/docs/24201606.pdf
// Ref: http://bochs.sourceforge.net/techspec/intel-82093-apic.pdf.gz

#define APIC_REG_ID			0x0020	// R
#define APIC_REG_VERSION		0x0030	// R
#define APIC_REG_TPR			0x0080	// RW, Task Priority Register
#define APIC_REG_PPR			0x00a0	// R, Processor Priority Register
#define APIC_REG_EOIR			0x00b0	// W
#define APIC_REG_LDR			0x00d0	// R, Logical Destination Register
#define APIC_REG_SIVR			0x00f0	// RW, Spurious Interrupt Vector Register
#define APIC_REG_ISR			0x0100	// R, In-Service Register
#define APIC_REG_TMR			0x0180	// R, Trigger Mode Register
#define APIC_REG_IRR			0x0200	// R, Interrupt Request Register
#define APIC_REG_ESR			0x0280	// RW, Error Status Register
#define APIC_REG_ICR			0x0300	// RW, Interrupt Command Register
#define APIC_REG_LVT_TR			0x0320	// RW, LVT Timer Register
#define APIC_REG_LVT_TSR		0x0330	// RW, LVT Thermal Sensor Register
#define APIC_REG_LVT_PMR		0x0340	// RW, LVT Performance Monitor Register
#define APIC_REG_LVT_LINT0R		0x0350	// RW, LVT LINT0 Register
#define APIC_REG_LVT_LINT1R		0x0360	// RW, LVT LINT1 Register
#define APIC_REG_LVT_ER			0x0370	// RW, LVT Error Register
#define APIC_REG_TIMER_ICR		0x0380	// RW, Initial Count Register for Timer
#define APIC_REG_TIMER_CCR		0x0390	// R, Current Count Register for Timer
#define APIC_REG_TIMER_DCR		0x03e0	// RW, Divide Configuration Register for Timer

#define APIC_DSH_NONE			(0x00 << 18)	// Destination shorthand
#define APIC_DSH_SELF			(0x01 << 18)
#define APIC_DSH_ALL			(0x02 << 18)
#define APIC_DSH_OTHERS			(0x03 << 18)
#define APIC_TM_EDGE			(0x00 << 15)	// Trigger mode
#define APIC_TM_LEVEL			(0x01 << 15)
#define APIC_LV_DEASSERT		(0x00 << 14)	// Level
#define APIC_LV_ASSERT			(0x01 << 14)
#define APIC_DS_IDLE			(0x00 << 12)	// Destination status
#define APIC_DS_PENDING			(0x01 << 12)
#define APIC_DM_PHYSICAL		(0x00 << 11)	// Destination mode
#define APIC_DM_LOGICAL			(0x01 << 11)
#define APIC_DMODE_FIXED		(0x00 << 8)	// Delivery mode
#define APIC_DMODE_LOWEST_PRIORITY	(0x01 << 8)
#define APIC_DMODE_SMI			(0x02 << 8)
#define APIC_DMODE_NMI			(0x04 << 8)
#define APIC_DMODE_INIT			(0x05 << 8)
#define APIC_DMODE_STARTUP		(0x06 << 8)

#define APIC_IM_ENABLED			(0x00 << 16)
#define APIC_IM_DISABLED		(0x01 << 16)
#define APIC_IRR_ACCEPT			(0x00 << 14)
#define APIC_IRR_EOI			(0x01 << 14)
#define APIC_PP_ACTIVEHIGH		(0x00 << 13)
#define APIC_PP_ACTIVELOW		(0x01 << 13)

typedef void (*APIC_Handler)(uint64_t,uint64_t);

void apic_activate();
void apic_init();
bool apic_enabled();
void apic_enable();
void apic_disable();
void apic_pause();
void apic_resume();
APIC_Handler apic_register(uint64_t vector, APIC_Handler handler);
void apic_dump(uint64_t vector, uint64_t error_code);

uint32_t apic_read32(int reg);
void apic_write32(int reg, uint32_t v);
uint64_t apic_read64(int reg);
void apic_write64(int reg, uint64_t v);

#define apic_eoi()	apic_write32(APIC_REG_EOIR, 0)

#endif /* __APIC_H__ */
