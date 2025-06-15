/*
This file is part of Quick Menu Plus
Copyright © 2020 浅倉麗子
Copyright © 2020 Princess-of-Sleeping

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

#ifndef SCEPAF_H
#define SCEPAF_H

#include <psp2/paf.h>

#define QUICK_MENU_BOX_ID  0x0EE0C8AF

#define POWER_TEXT_ID      0xC24DAB89
#define VOLUME_TEXT_ID     0xADBEB9FF

#define STANDBY_BUTTON_ID  0xC6D3C5FB
#define POWEROFF_BUTTON_ID 0xCCD55012

#define POWEROFF_LABEL_ID  0xFA617A89
#define POWER_LABEL_ID     0xC75A2636
#define VOLUME_LABEL_ID    0x33FCE3F2

typedef struct ScePafStyleVTable ScePafStyleVTable;

typedef struct ScePafStyle {
	ScePafStyleVTable *vptr;
	// size is unknown
} ScePafStyle;

typedef struct ScePafStyleVTable {
	char unk_0x0[0x30];
	void (*set_colour)(ScePafStyle*, float*);
	// size is unknown
} ScePafStyleVTable;

typedef struct ScePafWidgetVTable ScePafWidgetVTable;

typedef struct ScePafWidget {
	ScePafWidgetVTable *vptr;
	char unk_0x4[0x148];
	SceUInt32 id;
	char unk_0x150[0x47];
	char flags;
	// size is unknown
} ScePafWidget;

typedef struct ScePafWidgetVTable {
	char unk_0x0[0xF8];
	ScePafStyle *(*get_style_obj)(ScePafWidget*, int);
	char unk_0xFC[0x20];
	int (*set_label)(ScePafWidget*, ScePafWString*);
	// size is unknown
} ScePafWidgetVTable;

typedef struct ScePafPlugin ScePafPlugin;

typedef struct ScePafResourceSearchParam {
	char *data;
	SceSize length;
	int unk;
	SceUInt32 id;
} ScePafResourceSearchParam;

void scePafButtonSetRepeat(ScePafWidget*, int, int);

ScePafPlugin *scePafPluginGetByName(const char*);

const SceWChar16 *scePafLabelFindById(ScePafPlugin*, const ScePafResourceSearchParam*);

ScePafWidget *scePafWidgetFindById(ScePafWidget*, const ScePafResourceSearchParam*, int);

void scePafWidgetSetColour(float, ScePafWidget*, float*, int, int, int, int, int);

#endif
