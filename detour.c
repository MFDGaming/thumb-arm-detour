/*
   Copyright Alexander Argentakis
   Repo: https://github.com/MFDGaming/thumb-arm-detour
   This file is licensed under the MIT license
 */

#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include "detour.h"

uint32_t encode_branch(uint32_t pc, uint32_t branch_to, uint8_t has_link, uint8_t is_relative) {
	uint32_t offset = branch_to - ((pc + 4) & 0xFFFFFFFC);
	uint8_t s = (offset >> 31) & 1;
	uint8_t j = (~((offset >> 23) & 1) ^ s) & 1;
	uint8_t j2 = (~((offset >> 22) & 1) ^ s) & 1;
	uint16_t h = (offset >> 12) & 0x3ff;
	uint16_t l = (offset >> 1) & 0x7ff;
	uint8_t type = has_link ? 0b11 : 0b10;
	uint8_t thumb_bit = is_relative ? 1 : 0;
	uint8_t opcode = 0b11110;
	uint32_t result = opcode << 27;
	result |= s << 26;
	result |= h << 16;
	result |= type << 14;
	result |= j << 13;
	result |= thumb_bit << 12;
	result |= j2 << 11;
	result |= l;
	return ((result & 0xffff) << 16) | ((result >> 16) & 0xffff);
}

void detour(void* dst_addr, void* src_addr) {
	long page_size = sysconf(_SC_PAGESIZE);
	void *protect = (void *)(((uintptr_t)dst_addr-1) & -page_size);
	mprotect(protect, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
	uint32_t b = encode_branch((uint32_t)dst_addr-1, (uint32_t)src_addr-1, 0, 1);
	*(uint32_t *)((uint32_t)dst_addr-1) = b;
	mprotect(protect, page_size, PROT_EXEC);
}
