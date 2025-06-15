/*
This file is part of Quick Menu Plus
Copyright © 2020 浅倉麗子

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "common.h"
#include "opcode.h"

static int decode_bl_common(int bl, int *imm, short mask1, short chk1, short mask2, short chk2) {
	// split into two shorts
	short bl_1 = bl & 0xFFFF;
	short bl_2 = (bl >> 16) & 0xFFFF;

	// verify the form
	RNE(bl_1 & mask1, chk1);
	RNE(bl_2 & mask2, chk2);

	// decode
	int S = (bl_1 & 0x0400) >> 10;
	int J1 = (bl_2 & 0x2000) >> 13;
	int J2 = (bl_2 & 0x0800) >> 11;
	int I1 = ~(J1 ^ S) & 1;
	int I2 = ~(J2 ^ S) & 1;
	int imm10 = bl_1 & 0x03FF;
	int imm11 = bl_2 & 0x07FF;

	// combine to 25 bits and sign extend
	*imm = (S << 31) | (I1 << 30) | (I2 << 29) | (imm10 << 19) | (imm11 << 8);
	*imm >>= 7;
	return 0;
}

int decode_bl_t1(int bl, int *imm) {
	return decode_bl_common(bl, imm, 0xF800, 0xF000, 0xD000, 0xD000);
}

int decode_blx_t2(int blx, int *imm) {
	return decode_bl_common(blx, imm, 0xF800, 0xF000, 0xD001, 0xC000);
}

static int decode_mov_common(int mov, short *imm, short mask1, short chk1, short mask2, short chk2) {
	// split into two shorts
	short mov_1 = mov & 0xFFFF;
	short mov_2 = (mov >> 16) & 0xFFFF;

	// verify the form
	RNE(mov_1 & mask1, chk1);
	RNE(mov_2 & mask2, chk2);

	// decode
	int imm4 = mov_1 & 0x000F;
	int i = (mov_1 & 0x0400) >> 10;
	int imm8 = mov_2 & 0x00FF;
	int imm3 = (mov_2 & 0x7000) >> 12;

	// combine to 16 bits
	*imm = (imm4 << 12) | (i << 11) | (imm3 << 8) | imm8;
	return 0;
}

int decode_movw_t3(int movw, int *imm) {
	return decode_mov_common(movw, (short*)imm, 0xFBF0, 0xF240, 0x8000, 0x0000);
}

int decode_movt_t1(int movt, int *imm) {
	return decode_mov_common(movt, (short*)imm + 1, 0xFBF0, 0xF2C0, 0x8000, 0x0000);
}

int get_addr_bl(const int *bl, int *addr) {
	int offset;
	RNE(decode_bl_t1(*bl, &offset), 0);

	// PC is one word ahead
	int pc = (int)bl + 4;
	*addr = pc + offset;

	return 0;
}

int get_addr_blx(const int *blx, int *addr) {
	int offset;
	RNE(decode_blx_t2(*blx, &offset), 0);

	// PC is one word ahead and aligned backwards to word boundary
	unsigned int pc = (unsigned int)blx + 4;
	*addr = pc - (pc % 4) + offset;

	return 0;
}
