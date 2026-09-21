// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2

#include "../qcommon/vm_javascript.h"

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

void CG_DrawHead(float x, float y, float w, float h, int clientNum) {
	clientInfo_t* ci;

	ci = &cgs.clientinfo[clientNum];
	drawhShaderAdjusted(x, y, w, h, ci->modelIcon);
}

#define WICON_SIDE 32 * 0.60
#define WICON_SELECT 32 * 1.10
#define WICON_SPACE 3
#define WICONS_SIDES 12
#define WEAPON_SELECT_TIME 1350
#define WEAPON_SELECT_FADEOUT_TIME 350
#define WEAPON_SELECT_FADEIN_TIME 300
static void CG_DrawWeaponSelect(void) {
	vec4_t weaponWhiteColor = {1.0f, 1.0f, 1.0f, 1.0f};
	vec4_t weaponWhiteTextColor = {1.0f, 1.0f, 1.0f, 1.0f};
	vec4_t weaponBlackColor = {0.0f, 0.0f, 0.0f, 1.0f};
	float y, x, originalX = 320, originalY = 380;
	int i, j;
	float remaining, elapsed, alpha;

	if(cg.predictedPlayerState.stats[STAT_HEALTH] <= 0) return;
	if(!cg_draw2D.integer || cg.showScores) return;
	if(cg.time > cg.weaponSelectTime + WEAPON_SELECT_TIME) return;

	y = originalY + (WICON_SELECT * 0.25);
	x = originalX + (WICON_SELECT / 2) + WICON_SPACE;

	remaining = (float)(cg.weaponSelectTime + WEAPON_SELECT_TIME - cg.time);
	if(remaining < WEAPON_SELECT_FADEOUT_TIME && remaining > 0) {
		alpha = remaining / WEAPON_SELECT_FADEOUT_TIME;
		if(alpha < 0.00) alpha = 1.00;
		weaponWhiteColor[3] = alpha;
		weaponWhiteTextColor[3] = alpha;
		weaponBlackColor[3] = alpha;
	}

	elapsed = cg.time - cg.weaponSelectTime;
	if(elapsed < WEAPON_SELECT_FADEIN_TIME) {
		weaponWhiteTextColor[3] = elapsed / WEAPON_SELECT_FADEOUT_TIME;
	}

	drawStringAdjusted(320, 356, cg_weapons[cg.weaponSelect].item->pickup_name, FONTSTYLE_BOLD | FONTSTYLE_CENTER | FONTSTYLE_DROPSHADOW, weaponWhiteTextColor, 0.75, 256);

	for(i = cg.weaponSelect + 1, j = 0; i <= WEAPONS_NUM; i++) {
		if(j >= WICONS_SIDES) {
			continue;
		} else if(i >= WEAPONS_NUM) {
			i = 1;
		}

		if(!cg.swep_listcl[i]) continue;

		CG_RegisterWeapon(i);
		trap_R_SetColor(weaponBlackColor);
		drawhShaderAdjusted(x + 1, y + 1, WICON_SIDE, WICON_SIDE, cg_weapons[i].weaponIcon);
		trap_R_SetColor(weaponWhiteColor);
		drawhShaderAdjusted(x, y, WICON_SIDE, WICON_SIDE, cg_weapons[i].weaponIcon);

		if(cg.swep_listcl[i] == WS_NOAMMO) drawhShaderAdjusted(x, y, WICON_SIDE, WICON_SIDE, cgs.media.noammoShader);

		x += WICON_SIDE + WICON_SPACE;
		j++;
	}

	x = originalX - (WICON_SELECT / 2) - (WICON_SIDE + WICON_SPACE);
	for(i = cg.weaponSelect - 1, j = 0; i >= 0; i--) {
		if(j >= WICONS_SIDES) {
			continue;
		} else if(i <= 0) {
			i = WEAPONS_NUM - 1;
		}

		if(!cg.swep_listcl[i]) continue;

		CG_RegisterWeapon(i);
		trap_R_SetColor(weaponBlackColor);
		drawhShaderAdjusted(x + 1, y + 1, WICON_SIDE, WICON_SIDE, cg_weapons[i].weaponIcon);
		trap_R_SetColor(weaponWhiteColor);
		drawhShaderAdjusted(x, y, WICON_SIDE, WICON_SIDE, cg_weapons[i].weaponIcon);

		if(cg.swep_listcl[i] == WS_NOAMMO) drawhShaderAdjusted(x, y, WICON_SIDE, WICON_SIDE, cgs.media.noammoShader);

		x -= WICON_SIDE + WICON_SPACE;
		j++;
	}

	trap_R_SetColor(weaponBlackColor);
	drawhShaderAdjusted((originalX - WICON_SELECT / 2) + 1, (originalY + 2) + 1, WICON_SELECT, WICON_SELECT, cg_weapons[cg.weaponSelect].weaponIcon);
	trap_R_SetColor(weaponWhiteColor);
	drawhShaderAdjusted(originalX - WICON_SELECT / 2, originalY + 2, WICON_SELECT, WICON_SELECT, cg_weapons[cg.weaponSelect].weaponIcon);
	if(cg.swep_listcl[cg.weaponSelect] == WS_NOAMMO) drawhShaderAdjusted(originalX - WICON_SELECT / 2, originalY + 2, WICON_SELECT, WICON_SELECT, cgs.media.noammoShader);
}

static void CG_DrawCounterElement(float x, float y, const char* value, const char* text) {
	drawRoundedRectAdjusted(x, y, 50, 15, 0, color_background, 0);
	drawStringAdjusted(x + 4, y + 4, text, FONTSTYLE_LEFT | FONTSTYLE_DROPSHADOW, color_white, 0.35, 256);
	drawStringAdjusted(x + 44, y + 2, value, FONTSTYLE_BOLD | FONTSTYLE_RIGHT | FONTSTYLE_DROPSHADOW, color_white, 0.50, 256);
}

static void CG_UpdateWeaponStatus(void) {
	centity_t* cent = &cg_entities[cg.snap->ps.clientNum];
	playerState_t* ps = &cg.snap->ps;

	if(!cent->currentState.weapon) return;

	if(ps->stats[STAT_AMMO] <= 0 && ps->stats[STAT_AMMO] != -1)  // Noire's Mod weapon predict
		cg.swep_listcl[ps->weapon] = WS_NOAMMO;
	else
		cg.swep_listcl[ps->weapon] = WS_HAVE;
}

bool n_tip1 = false;
bool n_tip2 = false;

#define FPS_FRAMES 4
static void CG_DrawCounters(void) {
	int y;
	int value;
	int t, frameTime;
	static int previousTimes[FPS_FRAMES];
	static int index;
	static int previous;
	int i, total;
	int seconds;
	int msec;

	msec = cg.time - cgs.levelStartTime;
	seconds = msec / 1000;

	if(seconds < 3) {
		n_tip1 = false;
		n_tip2 = false;
	}

	if(seconds == 3 && !n_tip1) {
		CG_AddNotify("Welcome to Noire's Mod", NOTIFY_INFO, 0, "");
		n_tip1 = true;
	}

	if(seconds == 6 && !n_tip2) {
		CG_AddNotify("Press [Q] to open the spawn menu", NOTIFY_INFO, 0, "");
		n_tip2 = true;
	}

	y = 4;

	// FPS
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
		if(!total) total = 1;
		value = 1000 * FPS_FRAMES / total;
	}
	if(value <= 15 && seconds >= 3) trap_Cvar_Set("js.error", "2");
	if(cg_drawFPS.integer == 1) {
		CG_DrawCounterElement(640 + cgui.wideoffset - 54, y, va("%i", value), "FPS");
		y += 20;
	}
}

static void CG_ConsoleUpdateIdx(console_t* console, int chatHeight) {
	if(console->insertIdx < console->displayIdx) console->displayIdx = console->insertIdx;

	if(console->insertIdx - console->displayIdx > chatHeight) console->displayIdx = console->insertIdx - chatHeight;
}

static void CG_DrawGenericConsole(console_t* console, int maxlines, int time, int x, int y, float size) {
	int i, j;
	vec4_t hcolor;

	CG_ConsoleUpdateIdx(console, maxlines);
	hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0f;

	j = 0;
	for(i = console->displayIdx; i < console->insertIdx; ++i) {
		if(console->msgTimes[i % CONSOLE_MAXHEIGHT] + time < cg.time) continue;
		drawStringAdjusted(x + 1, y + (j * (FONT_SIZE * size)), console->msgs[i % CONSOLE_MAXHEIGHT], FONTSTYLE_LEFT | FONTSTYLE_DROPSHADOW, color_white, size, 256);
		j++;
	}
}

void CG_AddToGenericConsole(const char* str, console_t* console) {
	int len;
	char *p, *ls;

	len = 0;

	p = console->msgs[console->insertIdx % CONSOLE_MAXHEIGHT];
	*p = 0;

	ls = NULL;
	while(*str) {
		if(*str == ' ') ls = p;
		*p++ = *str++;
		len++;
	}
	*p = 0;

	console->msgTimes[console->insertIdx % CONSOLE_MAXHEIGHT] = cg.time;
	console->insertIdx++;
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

	cg.crosshairClientNum = trace.entityNum;
}

static void CG_DrawCrosshair(void) {
	float cSize;
	qhandle_t hShader;

	if(!cg_drawCrosshair.integer || cg.renderingThirdPerson) return;

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

	if(!cg_drawCrosshair.integer) return;

	if(cg.predictedPlayerState.pm_type == PM_DEAD) return;

	VectorCopy(cg.predictedPlayerState.origin, origin);
	origin[2] += cg.predictedPlayerState.viewheight;
	AnglesToAxis(cg.predictedPlayerState.viewangles, axis);
	VectorMA(origin, 65536, axis[0], endpos);

	CG_Trace(&trace, origin, NULL, NULL, endpos, 0, MASK_SHOT);
	CG_Draw3DCrosshair(trace.endpos[0], trace.endpos[1], trace.endpos[2]);
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

	CG_DrawGenericConsole(&cgs.chat, 5, 10000, 0 - cgui.wideoffset, 350, 0.50);

	CG_DrawCrosshair();
	CG_DrawCounters();

	if(!(catcher & KEYCATCH_UI)) {
		CG_ScanForCrosshairEntity();
		CG_Notify();
		CG_NSErrors();
	}

	CG_UpdateWeaponStatus();
	CG_DrawWeaponSelect();

	if(!(catcher & KEYCATCH_UI)) CG_DrawScoreboard();
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
