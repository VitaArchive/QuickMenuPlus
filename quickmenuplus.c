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

#include <stdatomic.h>
#include <stdbool.h>

#include <psp2/avconfig.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/paf.h>
#include <psp2/registrymgr.h>
#include <psp2/shellsvc.h>
#include <psp2/vshbridge.h>

#include <psp2dbg.h>
#include <taihen.h>

#include "common.h"
#include "config.h"
#include "opcode.h"
#include "scepaf.h"

#define N_INJECT 3
static SceUID inject_id[N_INJECT];

#define N_HOOK 7
static SceUID hook_id[N_HOOK];
static tai_hook_ref_t hook_ref[N_HOOK];

static SceUID inject_abs(int idx, void *dest, const void *src, size_t size) {
	SceUID ret = taiInjectAbs(dest, src, size);
	if (ret >= 0) {
		SCE_DBG_LOG_INFO("Injected %d UID %08X\n", idx, ret);
		inject_id[idx] = ret;
	} else {
		SCE_DBG_LOG_ERROR("Failed to inject %d error %08X\n", idx, ret);
	}
	return ret;
}
#define INJECT_ABS(idx, dest, src, size)\
	inject_abs(idx, dest, src, size)

static SceUID hook_offset(int idx, SceUID mod, uint32_t ofs, int th, const void *func) {
	SceUID ret = taiHookFunctionOffset(hook_ref + idx, mod, 0, ofs, th, func);
	if (ret >= 0) {
		SCE_DBG_LOG_INFO("Hooked %d UID %08X\n", idx, ret);
		hook_id[idx] = ret;
	} else {
		SCE_DBG_LOG_ERROR("Failed to hook %d error %08X\n", idx, ret);
	}
	return ret;
}
#define HOOK_OFFSET(idx, mod, ofs, th, func)\
	hook_offset(idx, mod, ofs, th, func##_hook)

static int UNINJECT(int idx) {
	int ret = 0;
	if (inject_id[idx] >= 0) {
		ret = taiInjectRelease(inject_id[idx]);
		if (ret == 0) {
			SCE_DBG_LOG_INFO("Uninjected %d UID %08X\n", idx, inject_id[idx]);
			inject_id[idx] = -1;
		} else {
			SCE_DBG_LOG_ERROR("Failed to uninject %d UID %08X error %08X\n", idx, inject_id[idx], ret);
		}
	} else {
		SCE_DBG_LOG_WARNING("Tried to uninject %d but not injected\n", idx);
	}
	return ret;
}

static int UNHOOK(int idx) {
	int ret = 0;
	if (hook_id[idx] >= 0) {
		ret = taiHookRelease(hook_id[idx], hook_ref[idx]);
		if (ret == 0) {
			SCE_DBG_LOG_INFO("Unhooked %d UID %08X\n", idx, hook_id[idx]);
			hook_id[idx] = -1;
			hook_ref[idx] = -1;
		} else {
			SCE_DBG_LOG_ERROR("Failed to unhook %d UID %08X error %08X\n", idx, hook_id[idx], ret);
		}
	} else {
		SCE_DBG_LOG_WARNING("Tried to unhook %d but not hooked\n", idx);
	}
	return ret;
}

typedef void btn_cb(void);
typedef int set_slidebar_pos(int, int, int);

static int (*vol_widget_init)(int, ScePafWidget*);

#define N_LANG 20

static const SceWChar16 *restart_text[N_LANG] = {
	u"再起動",
	u"Restart",
	u"Redémarrer",
	u"Reiniciar",
	u"Neustarten",
	u"Riavvio",
	u"Start opnieuw op",
	u"Reiniciar",
	u"Перезагрузка",
	u"재부팅",
	u"重新啟動",
	u"重新启動",
	u"Käynnistä uudelleen",
	u"Starta om",
	u"Genstart",
	u"Starte på nytt",
	u"Uruchom ponownie",
	u"Reiniciar",
	u"Restart",
	u"Yeniden başlatmak",
};

#define BG_STYLE_ORIGINAL    0
#define BG_STYLE_TRANSLUCENT 1
#define BG_STYLE_BLACK       2

static bool standby_is_restart = false;
static int bg_style = BG_STYLE_ORIGINAL;

static atomic_int system_volume;

static void request_cold_reset(void) {
	sceShellUtilRequestColdReset(0);
}

static void set_widget_labelf(ScePafWidget *widget, const SceWChar16 *fmt, ...) {
	SceWChar16 *buf = sce_paf_malloc(0x100 * sizeof(SceWChar16));
	if (buf) {
		va_list args;
		va_start(args, fmt);
		sce_paf_vswprintf(buf, 0x100, fmt, args);
		va_end(args);

		ScePafWString wlabel = {buf, sce_paf_wcslen(buf)};
		widget->vptr->set_label(widget, &wlabel);

		// Always free wlabel.data, because the original pointer
		// might already have been freed.
		sce_paf_free(wlabel.data);
	}
}

static const SceWChar16 *get_text_lang(const SceWChar16 **texts) {
	int lang;
	if (0 == sceRegMgrUtilityGetInt(0x37502, &lang) && lang < N_LANG) {
		return texts[lang];
	} else {
		return texts[0];
	}
}

static const SceWChar16 *get_label(SceUInt32 id, const SceWChar16 *default_label) {
	const SceWChar16 *ret = NULL;

	ScePafPlugin *impose_plugin = scePafPluginGetByName("impose_plugin");
	if (impose_plugin) {
		ScePafResourceSearchParam param = {NULL, 0, 0, id};

		// The return value of this function should not be freed.
		ret = scePafLabelFindById(impose_plugin, &param);
	}

	if (!ret) {
		ret = default_label;
	}
	return ret;
}

static ScePafWidget *get_widget(ScePafWidget *parent, SceUInt32 id) {
	ScePafResourceSearchParam param = {NULL, 0, 0, id};
	return scePafWidgetFindById(parent, &param, 0);
}

static void colour_convert_rgba(float *farr, SceUInt32 rgba) {
	farr[0] = (float)((rgba >> 0x18) & 0xFF) / 255.0;
	farr[1] = (float)((rgba >> 0x10) & 0xFF) / 255.0;
	farr[2] = (float)((rgba >> 0x08) & 0xFF) / 255.0;
	farr[3] = (float)((rgba >> 0x00) & 0xFF) / 255.0;
}

static void set_btn_colour(ScePafWidget *widget, SceUInt32 colour) {
	float _colour[4];
	colour_convert_rgba(_colour, colour);

	// style objs are indexed in the order they appear in the RCO XML
	ScePafStyle *plane_obj = widget->vptr->get_style_obj(widget, 0);
	if (plane_obj) {
		plane_obj->vptr->set_colour(plane_obj, _colour);
	}
}

static void set_widget_colour(ScePafWidget *widget, SceUInt32 colour) {
	float _colour[4];
	colour_convert_rgba(_colour, colour);
	scePafWidgetSetColour(0.0, widget, _colour, 0, 0x10001, 0, 0, 0);
}

static void set_power_text(ScePafWidget *parent) {
	ScePafWidget *box_widget = get_widget(parent, QUICK_MENU_BOX_ID);
	if (box_widget) {
		ScePafWidget *text_widget = get_widget(box_widget, POWER_TEXT_ID);
		if (text_widget) {
			set_widget_labelf(text_widget, u"%s\xF8EB", get_label(POWER_LABEL_ID, u""));
		}
	}
}

static void set_volume_text(ScePafWidget *parent) {
	ScePafWidget *text_widget = get_widget(parent, VOLUME_TEXT_ID);
	if (text_widget) {
		set_widget_labelf(text_widget, u"%s\xF8EB", get_label(VOLUME_LABEL_ID, u""));
	}
}

static void btn_init_hook(ScePafWidget *widget, int cb_type, btn_cb *cb, int r3) {

	if (standby_is_restart && widget->id == STANDBY_BUTTON_ID && cb_type == 0x10000008) {
		set_power_text(((ScePafWidget**)r3)[1]);

		set_widget_labelf(widget, u"%s", get_text_lang(restart_text));
		set_btn_colour(widget, 0x156AA2FF);
		TAI_NEXT(btn_init_hook, hook_ref[0], widget, cb_type, request_cold_reset, r3);

	} else if (!standby_is_restart && widget->id == POWEROFF_BUTTON_ID && cb_type == 0x10000008) {
		set_power_text(((ScePafWidget**)r3)[1]);

		set_widget_labelf(widget, u"%s・%s", get_label(POWEROFF_LABEL_ID, u""), get_text_lang(restart_text));
		set_btn_colour(widget, 0xCC8F00FF);

		// set holdable button with threshold 200ms and repeat threshold 800ms
		scePafButtonSetRepeat(widget, 200, 800);

		TAI_NEXT(btn_init_hook, hook_ref[0], widget, cb_type, cb, r3);
		TAI_NEXT(btn_init_hook, hook_ref[0], widget, 0x10000005, request_cold_reset, r3);

		// set holdable with physical button
		widget->flags &= 0xFD;

	} else {
		TAI_NEXT(btn_init_hook, hook_ref[0], widget, cb_type, cb, r3);
	}
}

// cannot TAI_NEXT due to lack of relocation by taiHEN
static int vol_slidebar_cb_hook(int r0) {
	int obj = *(int*)(r0 + 0x8);
	int vptr = *(int*)obj;
	int pos = *(int*)(obj + 0x294);

	SceUInt32 ctrl;
	SceBool muted, avls;
	RLZ(sceAVConfigGetVolCtrlEnable(&ctrl, &muted, &avls));
	if (avls && pos > SCE_AVCONFIG_VOLUME_AVLS_MAX) {
		pos = SCE_AVCONFIG_VOLUME_AVLS_MAX;
		RLZ(sceAVConfigSetSystemVol(pos));
		(**(set_slidebar_pos**)(vptr + 0x188))(obj, pos, 0);
	} else {
		RLZ(sceAVConfigSetSystemVol(pos));
	}
	atomic_store(&system_volume, pos);

	return 0;
}

static int sceAVConfigGetMasterVol_hook(int *v) {
	TAI_NEXT(sceAVConfigGetMasterVol_hook, hook_ref[2], v);
	return sceAVConfigGetSystemVol(v);
}

static int sceAVConfigWriteMasterVol_hook(void) {
	TAI_NEXT(sceAVConfigWriteMasterVol_hook, hook_ref[3]);

	int vol;
	RLZ(sceAVConfigGetSystemVol(&vol));
	return sceAVConfigWriteRegSystemVol(vol);
}

static int music_widget_init_hook(int r0, ScePafWidget *parent) {
	vol_widget_init(r0 + 0x40, parent);
	set_volume_text(parent);
	return TAI_NEXT(music_widget_init_hook, hook_ref[4], r0, parent);
}

// For reference on the function this function is hooking,
// see https://git.shotatoshounenwachigau.moe/vita/jav
static int process_volume_hook(int *audio_info, void *button_info) {
	int sys_vol = atomic_exchange(&system_volume, -1);
	if (sys_vol >= 0) {
		audio_info[5] = sys_vol;
	}
	return TAI_NEXT(process_volume_hook, hook_ref[5], audio_info, button_info);
}

// This function initialises some fields of the Quick Menu background plane
// which has ID 0xE83F6AF0. Disabling this function or using other arguments
// did not seem to make any difference.
// r1 - 3
// r2 - {960.0, 544.0}
// r3 - {0.0, 0.0}
static void bg_plane_init_hook(ScePafWidget *r0, int r1, float *r2, float *r3) {
	TAI_NEXT(bg_plane_init_hook, hook_ref[6], r0, r1, r2, r3);
	switch (bg_style) {
		case BG_STYLE_TRANSLUCENT:
			set_widget_colour(r0, 0x282828C0);
			break;
		case BG_STYLE_BLACK:
			set_widget_colour(r0, 0x000000FF);
			break;
	}
}

static void startup(void) {
	SCE_DBG_FILE_LOGGING_INIT("ux0:/quickmenuplus.log");
	sceClibMemset(inject_id, 0xFF, sizeof(inject_id));
	sceClibMemset(hook_id, 0xFF, sizeof(hook_id));
	sceClibMemset(hook_ref, 0xFF, sizeof(hook_ref));
	atomic_init(&system_volume, -1);
}

static void cleanup(void) {
	for (int i = 0; i < N_INJECT; i++) { UNINJECT(i); }
	for (int i = 0; i < N_HOOK; i++) { UNHOOK(i); }
	SCE_DBG_FILE_LOGGING_TERM();
}

USED int module_start(UNUSED SceSize args, UNUSED const void *argp) {
	startup();

	vshPowerSetPsButtonPushTime(config_read_key("pushtime", 500000));
	standby_is_restart = config_read_key("standbyisrestart", !vshSblAimgrIsDolce());
	bg_style = config_read_key("bgstyle", BG_STYLE_TRANSLUCENT);

	// get SceShell module info
	tai_module_info_t minfo;
	minfo.size = sizeof(minfo);
	GLZ(taiGetModuleInfo("SceShell", &minfo));

	SceKernelModuleInfo sce_minfo;
	sce_minfo.size = sizeof(sce_minfo);
	GLZ(sceKernelGetModuleInfo(minfo.modid, &sce_minfo));
	int seg0 = (int)sce_minfo.segments[0].vaddr;

	int quick_menu_init = seg0;

	switch(minfo.module_nid) {
		case 0x0552F692: // 3.60 retail
		case 0x532155E5: // 3.61 retail
			quick_menu_init += 0x14C408;
			break;
		case 0xBB4B0A3E: // 3.63 retail
		case 0x5549BF1F: // 3.65 retail
		case 0x34B4D82E: // 3.67 retail
		case 0x12DAC0F3: // 3.68 retail
		case 0x0703C828: // 3.69 retail
		case 0x2053B5A5: // 3.70 retail
		case 0xF476E785: // 3.71 retail
		case 0x939FFBE9: // 3.72 retail
		case 0x734D476A: // 3.73 retail
			quick_menu_init += 0x14C460;
			break;
		case 0xEAB89D5C: // 3.60 testkit
			quick_menu_init += 0x14483C;
			break;
		case 0x587F9CED: // 3.65 testkit
			quick_menu_init += 0x144894;
			break;
		case 0x6CB01295: // 3.60 Devkit
			quick_menu_init += 0x143E40;
			break;
		default:
			goto fail;
	}

	// power widget
	// template ID FAE265F7 from impose_plugin.rco

	// Poweroff and standby buttons are initialised with this function
	int btn_init;
	GLZ(get_addr_bl((int*)(quick_menu_init + 0x5B8), &btn_init));
	GLZ(HOOK_OFFSET(0, minfo.modid, btn_init - seg0, 1, btn_init));

	if (!vshSblAimgrIsDolce()) {
		// enable power widget (cmp r0, r0)
		GLZ(INJECT_ABS(0, (void*)(quick_menu_init + 0x44E), "\x80\x42", 2));

		// volume widget
		// template ID 84E0F33A from impose_plugin.rco

		// addr of branch to vol_widget_init from quick_menu_init
		int vol_widget_init_bl = quick_menu_init + 0xC1E;
		GLZ(get_addr_bl((int*)vol_widget_init_bl, (int*)&vol_widget_init));

		// addr of pointer to vol_slidebar_cb
		int ptr_vol_slidebar_cb;
		GLZ(decode_movw_t3(*(int*)(vol_widget_init + 0x1E0), &ptr_vol_slidebar_cb));
		GLZ(decode_movt_t1(*(int*)(vol_widget_init + 0x1E6), &ptr_vol_slidebar_cb));

		// addr of vol_slidebar_cb
		int vol_slidebar_cb = *(int*)ptr_vol_slidebar_cb;

		// Hook these functions to use system volume instead of master volume
		// Hook by offset in case NIDs are erased when not using Enso
		int sceAVConfigGetMasterVol, sceAVConfigWriteMasterVol;
		GLZ(get_addr_blx((int*)(vol_widget_init + 0x346), &sceAVConfigGetMasterVol));
		sceAVConfigWriteMasterVol = sceAVConfigGetMasterVol - 0x80;

		// set Thumb bit because we will call this function
		vol_widget_init = (void*)((int)vol_widget_init | 1);

		// Hook this function to move the position of the volume widget
		int music_widget_init;
		GLZ(get_addr_bl((int*)(quick_menu_init + 0x1006), &music_widget_init));

		// disable original call to vol_widget_init just in case (mov.w r0, #0)
		GLZ(INJECT_ABS(1, (void*)vol_widget_init_bl, "\x4f\xf0\x00\x00", 4));

		GLZ(HOOK_OFFSET(1, minfo.modid, vol_slidebar_cb - seg0, 1, vol_slidebar_cb));
		GLZ(HOOK_OFFSET(2, minfo.modid, sceAVConfigGetMasterVol - seg0, 0, sceAVConfigGetMasterVol));
		GLZ(HOOK_OFFSET(3, minfo.modid, sceAVConfigWriteMasterVol - seg0, 0, sceAVConfigWriteMasterVol));
		GLZ(HOOK_OFFSET(4, minfo.modid, music_widget_init - seg0, 1, music_widget_init));
		GLZ(HOOK_OFFSET(5, minfo.modid, quick_menu_init - 0x6FE6 - seg0, 1, process_volume));
	}

	// Disable quick menu gradient effect (cmp r0, r0)
	GLZ(INJECT_ABS(2, (void*)(quick_menu_init + 0x186), "\x80\x42", 2));

	// Custom style for the Quick Menu background
	if (bg_style != BG_STYLE_ORIGINAL) {
		int bg_plane_init;
		GLZ(get_addr_bl((int*)(quick_menu_init + 0x12C), &bg_plane_init));
		GLZ(HOOK_OFFSET(6, minfo.modid, bg_plane_init - seg0, 1, bg_plane_init));
	}

	SCE_DBG_LOG_INFO("module_start success\n");
	return SCE_KERNEL_START_SUCCESS;

fail:
	SCE_DBG_LOG_ERROR("module_start failed\n");
	cleanup();
	return SCE_KERNEL_START_FAILED;
}

USED int module_stop(UNUSED SceSize args, UNUSED const void *argp) {
	cleanup();
	return SCE_KERNEL_STOP_SUCCESS;
}
