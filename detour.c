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

typedef struct {
	unsigned int imm10: 10;
	unsigned int s: 1;
	unsigned int opcode: 5;
	unsigned int imm11: 11;
	unsigned int j2: 1;
	unsigned int not_x: 1;
	unsigned int j1: 1;
	unsigned int has_link: 1;
	unsigned int set: 1;
} branch_t;

typedef struct {
	unsigned int padding: 1;
	unsigned int imm11: 11;
	unsigned int imm10: 10;
	unsigned int i2: 1;
	unsigned int i1: 1;
	unsigned int s: 8;
} imm_t;

uint32_t encode_branch(uint32_t imm, bool has_link, bool not_x) {
	branch_t out;
	out.imm11 = ((imm_t *)&imm)->imm11 & 0x7ff;
	out.imm10 = ((imm_t *)&imm)->imm10 & 0x3ff;
	out.j2 = (~((imm_t *)&imm)->i2) & 0x1;
	out.j1 = (~((imm_t *)&imm)->i1) & 0x1;
	out.s = (((imm_t *)&imm)->s >> 31) & 0x1;
	out.not_x = not_x ? 1 : 0;
	out.has_link = has_link ? 1 : 0;
	out.opcode = 0b11110;
	out.set = 1;
	return *(uint32_t *)&out;
}

void detour(void *dst_addr, void *src_addr) {
	long page_size = sysconf(_SC_PAGESIZE);
	void *protect = (void *)(((uintptr_t)dst_addr - 1) & -page_size);
	mprotect(protect, 4, PROT_READ | PROT_WRITE | PROT_EXEC);
	uint32_t b = encode_branch((uint32_t)src_addr - (uint32_t)dst_addr - 4, 0, 1);
	*(uint32_t *)(dst_addr - 1) = b;
	mprotect(protect, 4, PROT_EXEC);
}
