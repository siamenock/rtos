#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "msr.h"

uint64_t msr_read(uint32_t register_address) {
	int msr = open("/dev/cpu/0/msr", O_RDONLY);
	if(msr < 0) {
		perror("Failed to open MSR");
		return (uint64_t)-1;
	}


	uint64_t data;
	ssize_t nread  = pread(msr, &data, sizeof(data), (off_t)register_address);
	if(nread <= 0) {
		perror("Failed to read MSR");
		close(msr);
		return (uint64_t)-1;
	}

	close(msr);
	return data;
}

static void write_value(uint32_t dx, uint32_t ax, uint32_t register_address) {
	int msr = open("/dev/cpu/0/msr", O_WRONLY);
	if(msr < 0) {
		perror("Failed to open MSR");
		return;
	}

	uint64_t data;
	ssize_t nwrite;

	data = dx;
	nwrite  = pwrite(msr, &data, sizeof(data), (off_t)register_address);
	if(nwrite <= 0)
		perror("Failed to write DX reigster value");

	data = ax;
	nwrite  = pwrite(msr, &data, sizeof(data), (off_t)register_address);
	if(nwrite <= 0)
		perror("Failed to write AX register value");

	close(msr);
}

void msr_write(uint64_t value, uint32_t register_address) {
	write_value(value >> 32, value & 0xFFFFFFFF, register_address);
}

void msr_write2(uint32_t dx, uint32_t ax, uint32_t register_address) {
	write_value(dx, ax, register_address);
}
