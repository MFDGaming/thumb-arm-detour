/*
   Copyright Alexander Argentakis
   Repo: https://github.com/MFDGaming/thumb-arm-detour
   This file is licensed under the MIT license
 */

#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include "detour.h"

uint32_t encode_movw(uint32_t rd, uint16_t imm16) {
    uint8_t imm8 = imm16 & 0xff;
    uint8_t imm3 = (imm16 >> 8) & 0x07;
    uint8_t i = (imm16 >> 11) & 0x01;
    uint8_t imm4 = (imm16 >> 12) & 0x0f;
    uint8_t opcode = 0b11110;
    uint32_t result = opcode << 27;
    result |= i << 26;
    result |= 0b100100 << 20;
    result |= imm4 << 16;
    result |= imm3 << 12;
    result |= rd << 8;
    result |= imm8;
    return ((result & 0xffff) << 16) | ((result >> 16) & 0xffff);
}

uint32_t encode_branch(uint32_t imm, uint8_t has_link, uint8_t is_b) {
	union {
            int32_t i;
            uint32_t u;
        } distance;
	uint16_t s, j1, j2, imm10, imm11;
	uint32_t result;
	distance.i = (int32_t)imm / 2;
	s = (distance.u >> 31) & 1;
	j1 = (~((distance.u >> 22) ^ s)) & 1;
	j2 = (~((distance.u >> 21) ^ s)) & 1;
	imm10 = (distance.u >> 11) & 0x000003ff;
        imm11 = distance.u & 0x000007ff;
	((uint16_t *)&result)[0] = 0xf000 | (s << 10) | imm10;
        ((uint16_t *)&result)[1] = 0x8000 | (has_link << 14) | (j1 << 13) | (is_b << 12) | (j2 << 11) | imm11;
	return result;
}

void detour(void *dst_addr, void *src_addr) {
	long page_size = sysconf(_SC_PAGESIZE);
	void *protect = (void *)(((uintptr_t)dst_addr - 1) & -page_size);
	mprotect(protect, 4, PROT_READ | PROT_WRITE | PROT_EXEC);
	uint32_t b = encode_branch((uint32_t)src_addr - (uint32_t)dst_addr - 4, 0, 1);
	*(uint32_t *)(dst_addr - 1) = b;
	mprotect(protect, 4, PROT_EXEC);
}
