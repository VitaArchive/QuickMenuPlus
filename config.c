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

#include <psp2/kernel/clib.h>
#include <psp2/kernel/iofilemgr.h>

#include "config.h"

int config_read_key(const char *key, int default_val) {
	int ret = default_val;
	char buf[64];

	if (sceClibSnprintf(buf, sizeof(buf), "ur0:/data/quickmenuplus/%s.txt", key) >= 0) {
		SceUID fd = sceIoOpen(buf, SCE_O_RDONLY, 0);
		if (fd >= 0) {
			sceClibMemset(buf, 0x00, sizeof(buf));
			if (sceIoRead(fd, buf, sizeof(buf) - 1) >= 0) {
				ret = sceClibStrtoll(buf, NULL, 0);
			}
			sceIoClose(fd);
		}
	}

	return ret;
}
