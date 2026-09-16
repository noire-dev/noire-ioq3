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
//
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "../qcommon/vm_javascript.h"
#include "../qcommon/vm_javascript_core.h"

static queued3DString_t queued3DStrings[MAX_3D_STRING_QUEUE];
static int queued3DStringCount = 0;
void CG_Add3DString(float x, float y, float z, const char* str, int style, const vec4_t color, float fontSize, float min, float max, bool useTrace) {
	queued3DString_t* q = &queued3DStrings[queued3DStringCount++];

	if(queued3DStringCount >= MAX_3D_STRING_QUEUE) return;
	q->x = x;
	q->y = y;
	q->z = z;
	q->str = str;
	q->style = style;
	Vector4Copy(color, q->color);
	q->fontSize = fontSize;
	q->min = min;
	q->max = max;
	q->useTrace = useTrace;
}

static void CG_Draw3DStringQueue(void) {
	int i;
	for(i = 0; i < queued3DStringCount; i++) {
		queued3DString_t* q = &queued3DStrings[i];
		CG_Draw3DString(q->x, q->y, q->z, q->str, q->style, q->color, q->fontSize, q->min, q->max, q->useTrace);
	}
	queued3DStringCount = 0;
}

void CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles) {
	refdef_t refdef;
	refEntity_t ent;

	if(!cg_draw3dIcons.integer || !cg_drawIcons.integer) {
		return;
	}

	CG_AdjustFrom640(&x, &y, &w, &h);

	memset(&refdef, 0, sizeof(refdef));

	memset(&ent, 0, sizeof(ent));
	AnglesToAxis(angles, ent.axis);
	VectorCopy(origin, ent.origin);
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;  // no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear(refdef.viewaxis);

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene(&ent);
	trap_R_RenderScene(&refdef);
}

void CG_DrawHead(float x, float y, float w, float h, int clientNum) {
	clientInfo_t* ci;

	ci = &cgs.clientinfo[clientNum];
	drawhShaderAdjusted(x, y, w, h, ci->modelIcon);
}

void CG_DrawFlagModel(float x, float y, float w, float h, int team, bool force2D) {
	qhandle_t cm;
	float len;
	vec3_t origin, angles;
	vec3_t mins, maxs;
	qhandle_t handle;

	if(!force2D && cg_draw3dIcons.integer) {
		VectorClear(angles);

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds(cm, mins, maxs);

		origin[2] = -0.5 * (mins[2] + maxs[2]);
		origin[1] = 0.5 * (mins[1] + maxs[1]);

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * (maxs[2] - mins[2]);
		origin[0] = len / 0.268;  // len / tan( fov/2 )

		angles[YAW] = 60 * sin(cg.time / 2000.0);
		;

		if(team == TEAM_RED) {
			handle = cgs.media.redFlagModel;
		} else if(team == TEAM_BLUE) {
			handle = cgs.media.blueFlagModel;
		} else if(team == TEAM_FREE) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel(x, y, w, h, handle, 0, origin, angles);
	} else if(cg_drawIcons.integer) {
		gitem_t* item;

		if(team == TEAM_RED) {
			item = BG_FindItemForPowerup(PW_REDFLAG);
		} else if(team == TEAM_BLUE) {
			item = BG_FindItemForPowerup(PW_BLUEFLAG);
		} else if(team == TEAM_FREE) {
			item = BG_FindItemForPowerup(PW_NEUTRALFLAG);
		} else {
			return;
		}
		if(item) {
			CG_DrawPic(x, y, w, h, cg_items[ITEM_INDEX(item)].icon);
		}
	}
}

static void CG_DrawStatusBarHead(float x) {
	vec3_t angles;
	float size, stretch;
	float frac;

	VectorClear(angles);

	if(cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME) {
		frac = (float)(cg.time - cg.damageTime) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * (1.5 - frac * 0.5);

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos(crandom() * M_PI);
		cg.headEndPitch = 5 * cos(crandom() * M_PI);

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if(cg.time >= cg.headEndTime) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos(crandom() * M_PI);
			cg.headEndPitch = 5 * cos(crandom() * M_PI);
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if(cg.headStartTime > cg.time) {
		cg.headStartTime = cg.time;
	}

	frac = (cg.time - cg.headStartTime) / (float)(cg.headEndTime - cg.headStartTime);
	frac = frac * frac * (3 - 2 * frac);
	angles[YAW] = cg.headStartYaw + (cg.headEndYaw - cg.headStartYaw) * frac;
	angles[PITCH] = cg.headStartPitch + (cg.headEndPitch - cg.headStartPitch) * frac;

	// CG_DrawHead(x, 480 - size, size, size, cg.snap->ps.clientNum, angles);
}

static void CG_DrawStatusBarFlag(float x, int team) { CG_DrawFlagModel(x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, false); }

void CG_DrawTeamBackground(int x, int y, int w, int h, float alpha, int team) {
	vec4_t hcolor;

	hcolor[3] = alpha;
	if(team == TEAM_RED) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if(team == TEAM_BLUE) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor(hcolor);
	CG_DrawPic(x, y, w, h, cgs.media.teamStatusBar);
	trap_R_SetColor(NULL);
}

static void CG_DrawStatusBar(void) {
	int color;
	centity_t* cent;
	playerState_t* ps;
	int value;
	vec4_t hcolor;
	vec3_t angles;
	vec3_t origin;

	static float colors[4][4] = {                            //		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
	                             {1.0f, 0.69f, 0.0f, 1.0f},  // normal
	                             {1.0f, 0.2f, 0.2f, 1.0f},   // low health
	                             {0.5f, 0.5f, 0.5f, 1.0f},   // weapon firing
	                             {1.0f, 1.0f, 1.0f, 1.0f}};  // health > 100

	if(cg_drawStatus.integer == 0) {
		return;
	}

	// draw the team background
	CG_DrawTeamBackground(0, 420, 640, 60, 0.33f, cg.snap->ps.persistant[PERS_TEAM]);

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear(angles);

	// draw any 3D icons first, so the changes back to 2D are minimized
	if(cent->currentState.weapon && cg_weapons[cent->currentState.weapon].ammoModel) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin(cg.time / 1000.0);
		CG_Draw3DModel(CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cg_weapons[cent->currentState.weapon].ammoModel, 0, origin, angles);
	}

	CG_DrawStatusBarHead(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE);

	if(cg.predictedPlayerState.powerups[PW_REDFLAG]) {
		CG_DrawStatusBarFlag(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED);
	} else if(cg.predictedPlayerState.powerups[PW_BLUEFLAG]) {
		CG_DrawStatusBarFlag(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE);
	} else if(cg.predictedPlayerState.powerups[PW_NEUTRALFLAG]) {
		CG_DrawStatusBarFlag(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE);
	}

	if(ps->stats[STAT_ARMOR]) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = (cg.time & 2047) * 360 / 2048.0;
		CG_Draw3DModel(370 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cgs.media.armorModel, 0, origin, angles);
	}
	//
	// ammo
	//
	if(cent->currentState.weapon) {
		value = ps->ammo[cent->currentState.weapon];
		if(value > -1) {
			if(cg.predictedPlayerState.weaponstate == WEAPON_FIRING && cg.predictedPlayerState.weaponTime > 100) {
				// draw as dark grey when reloading
				color = 2;  // dark grey
			} else {
				if(value >= 0) {
					color = 0;  // green
				} else {
					color = 1;  // red
				}
			}
			trap_R_SetColor(colors[color]);

			// CG_DrawField(0, 432, 3, value);
			trap_R_SetColor(NULL);

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if(!cg_draw3dIcons.integer && cg_drawIcons.integer) {
				qhandle_t icon;

				icon = cg_weapons[cg.predictedPlayerState.weapon].ammoIcon;
				if(icon) {
					CG_DrawPic(CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, icon);
				}
			}
		}
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if(value > 100) {
		trap_R_SetColor(colors[3]);  // white
	} else if(value > 25) {
		trap_R_SetColor(colors[0]);  // green
	} else if(value > 0) {
		color = (cg.time >> 8) & 1;  // flash
		trap_R_SetColor(colors[color]);
	} else {
		trap_R_SetColor(colors[1]);  // red
	}

	// stretch the health up when taking damage
	// CG_DrawField(185, 432, 3, value);
	CG_ColorForHealth(hcolor);
	trap_R_SetColor(hcolor);

	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if(value > 0) {
		trap_R_SetColor(colors[0]);
		// CG_DrawField(370, 432, 3, value);
		trap_R_SetColor(NULL);
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if(!cg_draw3dIcons.integer && cg_drawIcons.integer) {
			CG_DrawPic(370 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon);
		}
	}
}

#define FPS_FRAMES 4
static float CG_DrawFPS(float y) {
	char* s;
	int w;
	static int previousTimes[FPS_FRAMES];
	static int index;
	int i, total;
	int fps;
	static int previous;
	int t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if(index > FPS_FRAMES) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for(i = 0; i < FPS_FRAMES; i++) {
			total += previousTimes[i];
		}
		if(!total) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va("%ifps", fps);
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

		CG_DrawBigString(635 - w, y + 2, s, 1.0F);
	}

	return y + BIGCHAR_HEIGHT + 4;
}

static float CG_DrawTimer(float y) {
	char* s;
	int w;
	int mins, seconds, tens;
	int msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va("%i:%i%i", mins, tens, seconds);
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	CG_DrawBigString(635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

static void CG_DrawUpperRight(void) {
	float y;

	y = 0;

	if(cg_drawFPS.integer) y = CG_DrawFPS(y);
	if(cg_drawTimer.integer) y = CG_DrawTimer(y);
}

static float CG_DrawScores(float y) {
	const char* s;
	int s1, s2, score;
	int x, w;
	int v;
	vec4_t color;
	float y1;
	gitem_t* item;

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -= BIGCHAR_HEIGHT + 8;

	y1 = y;

	// draw from the right side to left
	if(cgs.gametype >= GT_TEAM) {
		x = 640;
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 1.0f;
		color[3] = 0.33f;
		s = va("%2i", s2);
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color);
		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
			CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
		}
		CG_DrawBigString(x + 4, y, s, 1.0F);

		if(cgs.gametype == GT_CTF) {
			// Display flag status
			item = BG_FindItemForPowerup(PW_BLUEFLAG);

			if(item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if(cgs.blueflag >= 0 && cgs.blueflag <= 2) {
					CG_DrawPic(x, y1 - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.blueFlagShader[cgs.blueflag]);
				}
			}
		}
		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.33f;
		s = va("%2i", s1);
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color);
		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
			CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
		}
		CG_DrawBigString(x + 4, y, s, 1.0F);

		if(cgs.gametype == GT_CTF) {
			// Display flag status
			item = BG_FindItemForPowerup(PW_REDFLAG);

			if(item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if(cgs.redflag >= 0 && cgs.redflag <= 2) {
					CG_DrawPic(x, y1 - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.redFlagShader[cgs.redflag]);
				}
			}
		}

		if(cgs.gametype >= GT_CTF) {
			v = cgs.capturelimit;
		} else {
			v = cgs.fraglimit;
		}
		if(v) {
			s = va("%2i", v);
			w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
			x -= w;
			CG_DrawBigString(x + 4, y, s, 1.0F);
		}

	} else {
		bool spectator;

		x = 640;
		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR);

		// always show your score in the second box if not in first place
		if(s1 != score) {
			s2 = score;
		}
		if(s2 != SCORE_NOT_PRESENT) {
			s = va("%2i", s2);
			w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
			x -= w;
			if(!spectator && score == s2 && score != s1) {
				color[0] = 1.0f;
				color[1] = 0.0f;
				color[2] = 0.0f;
				color[3] = 0.33f;
				CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color);
				CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color);
			}
			CG_DrawBigString(x + 4, y, s, 1.0F);
		}

		// first place
		if(s1 != SCORE_NOT_PRESENT) {
			s = va("%2i", s1);
			w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
			x -= w;
			if(!spectator && score == s1) {
				color[0] = 0.0f;
				color[1] = 0.0f;
				color[2] = 1.0f;
				color[3] = 0.33f;
				CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color);
				CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color);
			}
			CG_DrawBigString(x + 4, y, s, 1.0F);
		}

		if(cgs.fraglimit) {
			s = va("%2i", cgs.fraglimit);
			w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
			x -= w;
			CG_DrawBigString(x + 4, y, s, 1.0F);
		}
	}

	return y1 - 8;
}

static float CG_DrawPowerups(float y) {
	int sorted[MAX_POWERUPS];
	int sortedTime[MAX_POWERUPS];
	int i, j, k;
	int active;
	playerState_t* ps;
	int t;
	gitem_t* item;
	int x;
	int color;
	float size;
	float f;
	static float colors[2][4] = {{0.2f, 1.0f, 0.2f, 1.0f}, {1.0f, 0.2f, 0.2f, 1.0f}};

	ps = &cg.snap->ps;

	if(ps->stats[STAT_HEALTH] <= 0) {
		return y;
	}

	// sort the list by time remaining
	active = 0;
	for(i = 0; i < MAX_POWERUPS; i++) {
		if(!ps->powerups[i]) {
			continue;
		}

		// ZOID--don't draw if the power up has unlimited time
		// This is true of the CTF flags
		if(ps->powerups[i] == INT_MAX) {
			continue;
		}

		t = ps->powerups[i] - cg.time;
		if(t <= 0) {
			continue;
		}

		// insert into the list
		for(j = 0; j < active; j++) {
			if(sortedTime[j] >= t) {
				for(k = active - 1; k >= j; k--) {
					sorted[k + 1] = sorted[k];
					sortedTime[k + 1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for(i = 0; i < active; i++) {
		item = BG_FindItemForPowerup(sorted[i]);

		if(item) {
			color = 1;

			y -= ICON_SIZE;

			trap_R_SetColor(colors[color]);
			// CG_DrawField(x, y, 2, sortedTime[i] / 1000);

			t = ps->powerups[sorted[i]];
			if(t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME) {
				trap_R_SetColor(NULL);
			} else {
				vec4_t modulate;

				f = (float)(t - cg.time) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
				trap_R_SetColor(modulate);
			}

			if(cg.powerupActive == sorted[i] && cg.time - cg.powerupTime < PULSE_TIME) {
				f = 1.0 - (((float)cg.time - cg.powerupTime) / PULSE_TIME);
				size = ICON_SIZE * (1.0 + (PULSE_SCALE - 1.0) * f);
			} else {
				size = ICON_SIZE;
			}

			CG_DrawPic(640 - size, y + ICON_SIZE / 2 - size / 2, size, size, trap_R_RegisterShader(item->icon));
		}
	}
	trap_R_SetColor(NULL);

	return y;
}

static void CG_DrawLowerRight(void) {
	float y;

	y = 480 - ICON_SIZE;

	y = CG_DrawScores(y);
	CG_DrawPowerups(y);
}

static int CG_DrawPickupItem(int y) {
	int value;
	float* fadeColor;

	if(cg.snap->ps.stats[STAT_HEALTH] <= 0) {
		return y;
	}

	y -= ICON_SIZE;

	value = cg.itemPickup;
	if(value) {
		fadeColor = CG_FadeColor(cg.itemPickupTime, 3000);
		if(fadeColor) {
			CG_RegisterItemVisuals(value);
			trap_R_SetColor(fadeColor);
			CG_DrawPic(8, y, ICON_SIZE, ICON_SIZE, cg_items[value].icon);
			CG_DrawBigString(ICON_SIZE + 16, y + (ICON_SIZE / 2 - BIGCHAR_HEIGHT / 2), bg_itemlist[value].pickup_name, fadeColor[0]);
			trap_R_SetColor(NULL);
		}
	}

	return y;
}

static void CG_DrawLowerLeft(void) {
	float y;

	y = 480 - ICON_SIZE;

	CG_DrawPickupItem(y);
}

static void CG_DrawTeamInfo(void) {
	int h;
	int i;
	vec4_t hcolor;
	int chatHeight;

#define CHATLOC_Y 420  // bottom end
#define CHATLOC_X 0

	if(cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if(chatHeight <= 0) return;  // disabled

	if(cgs.teamLastChatPos != cgs.teamChatPos) {
		if(cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		if(cgs.clientinfo[cg.clientNum].team == TEAM_RED) {
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		} else if(cgs.clientinfo[cg.clientNum].team == TEAM_BLUE) {
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		} else {
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor(hcolor);
		CG_DrawPic(CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar);
		trap_R_SetColor(NULL);

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for(i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
			CG_DrawStringExt(CHATLOC_X + TINYCHAR_WIDTH, CHATLOC_Y - (cgs.teamChatPos - i) * TINYCHAR_HEIGHT, cgs.teamChatMsgs[i % chatHeight], hcolor, false, false, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
	}
}

static void CG_DrawHoldableItem(void) {
	int value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if(value) {
		CG_RegisterItemVisuals(value);
		CG_DrawPic(640 - ICON_SIZE, (SCREEN_HEIGHT - ICON_SIZE) / 2, ICON_SIZE, ICON_SIZE, cg_items[value].icon);
	}
}

static void CG_DrawDisconnect(void) {
	float x, y;
	int cmdNum;
	usercmd_t cmd;
	const char* s;
	int w;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if(cmd.serverTime <= cg.snap->ps.commandTime || cmd.serverTime > cg.time) {  // special check for map_restart
		return;
	}

	// also add text in center of screen
	s = "Connection Interrupted";
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 100, s, 1.0F);

	// blink the icon
	if((cg.time >> 9) & 1) {
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic(x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga"));
}

void CG_CenterPrint(const char* str, int y, int charWidth) {
	char* s;

	Q_strncpyz(cg.centerPrint, str, sizeof(cg.centerPrint));

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while(*s) {
		if(*s == '\n') cg.centerPrintLines++;
		s++;
	}
}

static void CG_DrawCenterString(void) {
	char* start;
	int l;
	int x, y, w;
	float* color;

	if(!cg.centerPrintTime) return;

	color = CG_FadeColor(cg.centerPrintTime, 1000 * cg_centertime.value);
	if(!color) {
		return;
	}

	trap_R_SetColor(color);

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while(1) {
		char linebuffer[1024];

		for(l = 0; l < 50; l++) {
			if(!start[l] || start[l] == '\n') {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.centerPrintCharWidth * CG_DrawStrlen(linebuffer);

		x = (SCREEN_WIDTH - w) / 2;

		CG_DrawStringExt(x, y, linebuffer, color, false, true, cg.centerPrintCharWidth, (int)(cg.centerPrintCharWidth * 1.5), 0);

		y += cg.centerPrintCharWidth * 1.5;

		while(*start && (*start != '\n')) {
			start++;
		}
		if(!*start) {
			break;
		}
		start++;
	}

	trap_R_SetColor(NULL);
}

static void CG_ScanForCrosshairEntity(void) {
	trace_t trace;
	vec3_t start, end;
	int content;

	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, 131072, cg.refdef.viewaxis[0], end);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY);
	if(trace.entityNum >= MAX_CLIENTS) return;

	content = trap_CM_PointContents(trace.endpos, 0);
	if(content & CONTENTS_FOG) return;

	if(cg_entities[trace.entityNum].currentState.powerups & (1 << PW_INVIS)) return;

	cg.crosshairClientNum = trace.entityNum;
}

static void CG_DrawCrosshair(void) {
	float cSize;
	qhandle_t hShader;

	if(!cg_drawCrosshair.integer || cg.renderingThirdPerson || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) return;

	trap_R_SetColor(color_white);
	cSize = cg_crosshairScale.value;
	if(cg_drawCrosshair.integer > 0) {
		hShader = cgs.media.crosshairShader[cg_drawCrosshair.integer % NUM_CROSSHAIRS];
		if(!hShader) hShader = cgs.media.crosshairShader[0];
		drawhShaderAdjusted(320 - (cSize * 0.5), 240 - (cSize * 0.5), cSize, cSize, hShader);
	}
	trap_R_SetColor(NULL);
}

static void CG_Draw3DCrosshair(float x, float y, float z) {
	vec3_t dir;
	float localX, localY, localZ;
	float tanFovX, tanFovY;
	float finalx, finaly;
	float cSize;
	vec3_t worldPos;
	qhandle_t hShader;

	worldPos[0] = x;
	worldPos[1] = y;
	worldPos[2] = z;

	VectorSubtract(worldPos, cg.refdef.vieworg, dir);

	localX = -DotProduct(dir, cg.refdef.viewaxis[1]);
	localY = DotProduct(dir, cg.refdef.viewaxis[2]);
	localZ = DotProduct(dir, cg.refdef.viewaxis[0]);

	if(localZ <= 0) return;

	tanFovX = tan(DEG2RAD(cg.refdef.fov_x * 0.5f));
	tanFovY = tan(DEG2RAD(cg.refdef.fov_y * 0.5f));

	finalx = (localX / (localZ * tanFovX)) * (320 + cgui.wideoffset) + 320;
	finaly = (-localY / (localZ * tanFovY)) * 240 + 240;

	trap_R_SetColor(color_white);
	cSize = cg_crosshairScale.value;
	hShader = cgs.media.crosshairShader[cg_drawCrosshair.integer % NUM_CROSSHAIRS];
	drawhShaderAdjusted(finalx - (cSize * 0.5), finaly - (cSize * 0.5), cSize, cSize, hShader);
	trap_R_SetColor(NULL);
}

static void CG_DrawCrosshair3D(void) {
	trace_t trace;
	vec3_t origin, endpos;
	vec3_t axis[3];

	if(!cg_drawCrosshair.integer || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) return;

	if(cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION) return;

	VectorCopy(cg.predictedPlayerState.origin, origin);
	origin[2] += cg.predictedPlayerState.viewheight;
	AnglesToAxis(cg.predictedPlayerState.viewangles, axis);
	VectorMA(origin, 65536, axis[0], endpos);

	CG_Trace(&trace, origin, NULL, NULL, endpos, 0, MASK_SHOT);
	CG_Draw3DCrosshair(trace.endpos[0], trace.endpos[1], trace.endpos[2]);
}

static void CG_DrawSpectator(void) {
	CG_DrawBigString(320 - 9 * 8, 440, "SPECTATOR", 1.0F);
	if(cgs.gametype >= GT_TEAM) {
		CG_DrawBigString(320 - 39 * 8, 460, "press ESC and use the JOIN menu to play", 1.0F);
	}
}

static void CG_DrawIntermission(void) { CG_DrawScoreboard(); }

static void CG_DrawFollow(void) {
	const char* name;

	if(!(cg.snap->ps.pm_flags & PMF_FOLLOW)) return;

	name = cgs.clientinfo[cg.snap->ps.clientNum].name;
	drawStringAdjusted(320, 10, name, FONTSTYLE_CENTER, color_white, 0.50, 256);
}

static qhandle_t CG_FindModImage(int mod) {
	// if(mod > 0 && mod < MOD_WEAPONS) return cg_items[ITEM_INDEX(BG_FindItemForWeapon(mod))].icon;
	// if(mod == MOD_WATER) return trap_R_RegisterShaderNoMip("icons/d_water");
	// if(mod == MOD_SLIME) return trap_R_RegisterShaderNoMip("icons/d_slime");
	// if(mod == MOD_LAVA) return trap_R_RegisterShaderNoMip("icons/d_lava");
	// if(mod == MOD_CRUSH) return trap_R_RegisterShaderNoMip("icons/d_death");
	// if(mod == MOD_TELEFRAG) return trap_R_RegisterShaderNoMip("icons/teleporter");
	// if(mod == MOD_FALLING) return trap_R_RegisterShaderNoMip("icons/d_fall");
	// if(mod == MOD_SUICIDE) return trap_R_RegisterShaderNoMip("icons/d_death");
	// if(mod == MOD_TRIGGER_HURT) return trap_R_RegisterShaderNoMip("icons/d_death");
	// if(mod == MOD_KAMIKAZE) return trap_R_RegisterShaderNoMip("icons/kamikaze");
	// if(mod == MOD_JUICED) return trap_R_RegisterShaderNoMip("icons/d_death");
	// if(mod == MOD_CAR) return trap_R_RegisterShaderNoMip("icons/d_car");
	// if(mod == MOD_CAREXPLODE) return trap_R_RegisterShaderNoMip("icons/d_car");
	// if(mod == MOD_PROP) return trap_R_RegisterShaderNoMip("icons/iconw_propgun");
	return trap_R_RegisterShaderNoMip("icons/d_death");
}

static int NSErrorTime = 0;
static void CG_NSErrors(void) {
	const int fadeInDuration = 1000;
	const int visibleDuration = 1000;
	const int fadeOutDuration = 1000;
	float alpha = 1.0f, sizeX = 0;
	int elapsed;
	const char* text;

	vec4_t color = {0.92f, 0.72f, 0.20f, 1.00f};
	vec4_t colord = {0.30f, 0.24f, 0.06f, 1.00f};
	vec4_t colortex = {0.20f, 0.20f, 0.20f, 1.00f};
	vec4_t colorpic = {1.00f, 1.00f, 1.00f, 1.00f};

	if(!strlen(trap_Cvar_VariableString("js.error"))) return;

	if(!NSErrorTime) NSErrorTime = cg.time;

	elapsed = cg.time - NSErrorTime;

	if(elapsed >= fadeInDuration + visibleDuration + fadeOutDuration) {
		trap_Cvar_Set("js.error", "");
		NSErrorTime = 0;
		return;
	}

	if(trap_Cvar_VariableIntegerValue("js.error") == 2)
		text = "Low performance, physics may be unstable";
	else if(trap_Cvar_VariableIntegerValue("js.error") == 3)
		text = "Network unstable, switching to chunk mode";
	else
		text = va("Something is creating script errors - %s", trap_Cvar_VariableString("js.error"));

	if(elapsed < fadeInDuration)
		alpha = (float)elapsed / fadeInDuration;
	else if(elapsed > fadeInDuration + visibleDuration)
		alpha = 1.0f - ((float)(elapsed - fadeInDuration - visibleDuration) / fadeOutDuration);

	color[3] *= alpha;
	colord[3] *= alpha;
	colortex[3] *= alpha;
	colorpic[3] *= alpha;

	sizeX = stringWidth(text, 0.35, FONTSTYLE_LEFT, 256) + 24;
	drawRoundedRectAdjusted(21 - cgui.wideoffset, 21, sizeX, 16, 0, colord, 0);
	drawRoundedRectAdjusted(20 - cgui.wideoffset, 20, sizeX, 16, 0, color, 0);
	trap_R_SetColor(colorpic);
	drawhShaderAdjusted(23 - cgui.wideoffset, 23.5, 10, 10, cgs.media.errIcon);
	trap_R_SetColor(NULL);
	drawStringAdjusted(38 - cgui.wideoffset, 24, text, FONTSTYLE_LEFT, colortex, 0.35, 256);
}

void CG_AddNotify(const char* text, int type, int number, const char* picPath) {
	int i;
	int id = -1;

	for(i = MAX_NOTIFICATIONS - 1; i > 0; i--) cg.notifications[i] = cg.notifications[i - 1];

	Q_strncpyz(cg.notifications[0].text, text, 255);
	Q_strncpyz(cg.notifications[0].picPath, picPath, 255);
	cg.notifications[0].text[255] = '\0';
	cg.notifications[0].picPath[255] = '\0';
	cg.notifications[0].type = type;
	cg.notifications[0].number = number;
	cg.notifications[0].startTime = cg.time;
	cg.notifications[0].active = true;

	if(type == NOTIFY_INFO) trap_S_StartLocalSound(cgs.media.notifySound, CHAN_LOCAL_SOUND);
	if(type == NOTIFY_UNDO) trap_S_StartLocalSound(cgs.media.undoSound, CHAN_LOCAL_SOUND);
}

static void CG_Notify(void) {
	vec4_t backgroundColor = {0.10f, 0.10f, 0.125f, 0.70f};
	vec4_t textColor = {1.0f, 1.0f, 1.0f, 1.0f};
	vec4_t picColor = {1.0f, 1.0f, 1.0f, 1.0f};
	float margin = 4.0f, startX = 640 + cgui.wideoffset - 15, startY = 415 - margin, alpha, offsetX, yOffset, boxWidth;
	int i, timeElapsed, typeTime = 5000;

	for(i = 0; i < MAX_NOTIFICATIONS; i++) {
		if(!cg.notifications[i].active) continue;

		timeElapsed = cg.time - cg.notifications[i].startTime;
		if(timeElapsed > typeTime) {
			cg.notifications[i].active = false;
			continue;
		}

		alpha = 1.0f;
		offsetX = 0.0f;

		if(timeElapsed < NOTIFICATION_FADE_TIME) {
			alpha = (float)timeElapsed / NOTIFICATION_FADE_TIME;
			offsetX = LERP(300.0f, 0.0f, alpha);
		} else if(timeElapsed > typeTime - NOTIFICATION_FADE_TIME) {
			alpha = (float)(typeTime - timeElapsed) / NOTIFICATION_FADE_TIME;
		}

		backgroundColor[3] = alpha * 0.70f;
		textColor[3] = alpha;
		picColor[3] = alpha;

		boxWidth = stringWidth(cg.notifications[i].text, 0.50, FONTSTYLE_LEFT, 256) + 32;

		yOffset = startY - (20 + margin) * i;

		drawRoundedRectAdjusted(startX - boxWidth + offsetX, yOffset, boxWidth, 20, 4, backgroundColor, 0);

		trap_R_SetColor(picColor);
		if(cg.notifications[i].type == NOTIFY_INFO && cgs.media.notifyIcon) drawhShaderAdjusted(startX - boxWidth + offsetX + 5, yOffset + 2.5, 16, 16, cgs.media.notifyIcon);
		if(cg.notifications[i].type == NOTIFY_UNDO && cgs.media.undoIcon) drawhShaderAdjusted(startX - boxWidth + offsetX + 5, yOffset + 2.5, 16, 16, cgs.media.undoIcon);
		if(cg.notifications[i].type == NOTIFY_ITEM) drawhShaderAdjusted(startX - boxWidth + offsetX + 5, yOffset + 2.5, 16, 16, cg_items[cg.notifications[i].number].icon);
		if(cg.notifications[i].type == NOTIFY_KILL) drawhShaderAdjusted(startX - boxWidth + offsetX + 5, yOffset + 2.5, 16, 16, CG_FindModImage(cg.notifications[i].number));
		if(cg.notifications[i].type == NOTIFY_CUSTOM) drawShaderAdjusted(startX - boxWidth + offsetX + 5, yOffset + 2.5, 16, 16, cg.notifications[i].picPath);
		trap_R_SetColor(NULL);

		drawStringAdjusted(startX - boxWidth + offsetX + 26, yOffset + 5, cg.notifications[i].text, FONTSTYLE_LEFT | FONTSTYLE_DROPSHADOW, textColor, 0.50, 256);
	}
}

static void CG_Draw2D(void) {
	int catcher = trap_Key_GetCatcher();

	if(cg_draw2D.integer == 0) return;

	// if(!(catcher & KEYCATCH_MESSAGE)) CG_DrawGenericConsole(&cgs.console, 5, 10000, 0 - cgui.wideoffset, 0, 0.50);
	// if(!(catcher & KEYCATCH_UI)) {
	//	CG_DrawGenericConsole(&cgs.teamChat, 5, 10000, 0 - cgui.wideoffset, 80, 0.50);
	//	CG_DrawGenericConsole(&cgs.chat, 5, 10000, 0 - cgui.wideoffset, 350, 0.50);
	// }

	CG_DrawCrosshair();
	// CG_DrawCounters();
	if(!(catcher & KEYCATCH_UI)) {
		CG_ScanForCrosshairEntity();
		CG_DrawPowerups(0);
		CG_DrawFollow();
		CG_Notify();
		// if(!cg.scoreBoardShowing) CG_Draw1FCTF();
		// CG_NSErrors();
	}

	if(cg.snap->ps.pm_type == PM_INTERMISSION) {
		CG_DrawIntermission();
		return;
	}

	if(cg.snap->ps.pm_type != PM_INTERMISSION && cg.snap->ps.pm_type != PM_DEAD && cg.snap->ps.pm_type != PM_SPECTATOR) CG_DrawStatusBar();

	if(!(catcher & KEYCATCH_UI)) {
		CG_DrawScoreboard();
		// if(cgs.gametype != GT_SANDBOX) CG_DrawScores();
	}
}

void CG_DrawActive(stereoFrame_t stereoView) {
	if(!cg.snap) {
		CG_DrawInformation();
		return;
	}

	trap_R_RenderScene(&cg.refdef);  // draw 3D view
	CG_Draw3DStringQueue();          // draw 3D text
	CG_Draw2D();                     // draw C elements
	JS_HUDDraw();                    // draw JS elements
}
