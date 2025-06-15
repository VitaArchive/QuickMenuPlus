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

#ifndef OPCODE_H
#define OPCODE_H

int decode_bl_t1(int bl, int *imm);

int decode_blx_t2(int blx, int *imm);

int decode_movw_t3(int movw, int *imm);

int decode_movt_t1(int movt, int *imm);

int get_addr_bl(const int *bl, int *addr);

int get_addr_blx(const int *blx, int *addr);

#endif
