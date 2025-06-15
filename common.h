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

#ifndef COMMON_H
#define COMMON_H

#define USED __attribute__ ((used))
#define UNUSED __attribute__ ((unused))

#define GLZ(x) do {\
	if ((x) < 0) { goto fail; }\
} while (0)

#define RLZ(x) do {\
	__typeof__(x) __ret__ = (x);\
	if (__ret__ < 0) { return __ret__; }\
} while(0)

#define RNE(x, k) do {\
	if ((x) != (k)) { return -1; }\
} while(0)

#endif
