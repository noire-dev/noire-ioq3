/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../qcommon/vm_javascript.h"

void CG_LoadingString(const char* s, float value) {
	Q_strncpyz(cg.infoScreenText, va("Loading... %s", s), sizeof(cg.infoScreenText));
	if(value != -1) cg.infoScreenValue = value;
	trap_UpdateScreen();
}

void CG_LoadingItem(int itemNum) {
	gitem_t* item;

	item = &bg_itemlist[itemNum];
	CG_LoadingString(item->pickup_name, -1);
}
void CG_LoadingClient(int clientNum) {
	const char* info;
	char personality[MAX_QPATH];

	info = CG_ConfigString(CS_PLAYERS + clientNum);
	Q_strncpyz(personality, Info_ValueForKey(info, "n"), sizeof(personality));
	Q_CleanStr(personality);
	CG_LoadingString(personality, -1);
}

void CG_DrawInformation(void) {
	const char* s;
	const char* info;
	int y;
	qhandle_t levelshot;
	qhandle_t detail;
	qhandle_t fade;
	qhandle_t logo;
	qhandle_t loading;
	char buf[64];
	vec4_t color_white = {1.00f, 1.00f, 1.00f, 1.00f};
	vec4_t color_whiteblack = {0.90f, 0.90f, 0.90f, 1.00f};
	vec4_t color_grey = {0.30f, 0.30f, 0.30f, 1.00f};
	vec4_t color_lightgrey = {0.50f, 0.50f, 0.50f, 1.00f};

	info = CG_ConfigString(CS_SERVERINFO);

	s = Info_ValueForKey(info, "mapname");
	levelshot = trap_R_RegisterShaderNoMip(va("levelshots/%s", s));
	if(!levelshot) levelshot = trap_R_RegisterShaderNoMip("menu/assets/unknownmap");
	detail = trap_R_RegisterShaderNoMip("menu/assets/loadingoverlay");
	fade = trap_R_RegisterShaderNoMip("menu/assets/blacktrans");
	logo = trap_R_RegisterShaderNoMip("menu/logo");
	loading = trap_R_RegisterShaderNoMip("menu/assets/loading");
	trap_R_SetColor(NULL);

	drawhShaderAdjusted(0 - cgui.wideoffset, 0, 640 + (cgui.wideoffset * 2), 480, levelshot);
	drawhShaderAdjusted(0 - cgui.wideoffset, 0, 640 + (cgui.wideoffset * 2), 480, detail);

	drawRoundedRectAdjusted(410 + cgui.wideoffset, 445, 220, 30, 1, color_lightgrey, 0);
	CG_DrawProgressBar(415 + cgui.wideoffset, 459, 210, 12, cg.infoScreenValue, 8, color_white, color_grey);
	drawStringAdjusted(415 + cgui.wideoffset, 449, cg.infoScreenText, FONTSTYLE_LEFT, color_whiteblack, 0.40, 256);

	drawhShaderAdjusted(320 - 50, 240 - 75, 100, 100, logo);
	drawhShaderAdjusted(320 - 24, 320 - 48, 48, 48, loading);

	drawhShaderAdjusted(0 - cgui.wideoffset, 0, 300, 85, fade);
	drawhShaderAdjusted(5 - cgui.wideoffset, 5, 100, 75, levelshot);

	y = 10;
	Q_strncpyz(buf, Info_ValueForKey(info, "sv_hostname"), 64);
	Q_CleanStr(buf);
	drawStringAdjusted(110 - cgui.wideoffset, y, buf, FONTSTYLE_LEFT, color_white, 0.75, 256);
	y += 25;

	Q_strncpyz(buf, Info_ValueForKey(info, "mapname"), 64);
	Q_CleanStr(buf);
	drawStringAdjusted(110 - cgui.wideoffset, y, buf, FONTSTYLE_LEFT, color_white, 0.75, 256);
	y += 25;

	switch(cgs.gametype) {
		case GT_FFA: s = "Free For All"; break;
		case GT_TEAM: s = "Team Deathmatch"; break;
		default: s = "Unknown Gametype"; break;
	}

	drawStringAdjusted(110 - cgui.wideoffset, y, s, FONTSTYLE_LEFT, color_white, 0.75, 256);
}
