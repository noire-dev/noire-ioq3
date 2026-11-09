// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2

#include "../qcommon/vm_javascript.h"

static queued3DString_t queued3DStrings[MAX_3D_STRING_QUEUE];
static int queued3DStringCount = 0;
void CG_Add3DString(float x, float y, float z, const char* str, int style, const vec4_t color, float fontSize, float min, float max, bool useTrace) {
	queued3DString_t* q;

	if(queued3DStringCount >= MAX_3D_STRING_QUEUE) return;
	q = &queued3DStrings[queued3DStringCount++];
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
	drawhShaderAdjusted(x, y, w, h, ci->modelIcon ? ci->modelIcon : cgs.media.deferShader);
}

#define WEAPON_SELECTOR_SECTIONS 6
#define WEAPON_SELECTOR_Y 38.0f
#define WEAPON_SELECTOR_HEADER_ACTIVE_W 270.0f
#define WEAPON_SELECTOR_HEADER_ACTIVE_H 48.0f
#define WEAPON_SELECTOR_HEADER_IDLE 77.0f
#define WEAPON_SELECTOR_GAP 19.0f
#define WEAPON_SELECTOR_ROW_H 48.0f
#define WEAPON_SELECTOR_ROW_SELECTED_H 192.0f
#define WEAPON_SELECTOR_TIME 1800
#define WEAPON_SELECTOR_FADEOUT 300
#define WEAPON_SELECTOR_SCALE (1.0f / 2.0f)

/* Apply one-half scale to every selector element. The menu is centered
 * horizontally and remains anchored near the top of the screen. */
static void CG_DrawWeaponSelectorRect(float x, float y, float w, float h, float radius, vec4_t color) { drawRoundedRect(x * cgui.scale * WEAPON_SELECTOR_SCALE, y * cgui.scale * WEAPON_SELECTOR_SCALE, w * cgui.scale * WEAPON_SELECTOR_SCALE, h * cgui.scale * WEAPON_SELECTOR_SCALE, radius * cgui.scale * WEAPON_SELECTOR_SCALE, color, 0); }

static void CG_DrawWeaponSelectorText(float x, float y, const char* text, int style, vec4_t color, float size) { drawString(x * cgui.scale * WEAPON_SELECTOR_SCALE, y * cgui.scale * WEAPON_SELECTOR_SCALE, text, style, color, size * cgui.scale * WEAPON_SELECTOR_SCALE, 128); }

static int CG_GetWeaponSectionList(int section, int* weapons, int maxWeapons) {
	int count = 0;
	int order, weapon;

	for(order = 1; order <= jsd_weaponCount * 4; order++) {
		for(weapon = 1; weapon < jsd_weaponCount; weapon++) {
			weaponFx_t* fx = &jsd_weaponFx[weapon];
			int weaponOrder;

			if(fx->weaponSection != section || !cg.swep_listcl[weapon]) continue;
			weaponOrder = fx->sectionOrder > 0 ? fx->sectionOrder : weapon;
			if(weaponOrder != order) continue;
			if(count < maxWeapons) weapons[count] = weapon;
			count++;
		}
	}

	return count < maxWeapons ? count : maxWeapons;
}

static void CG_DrawWeaponSelectorIcon(int weapon, float x, float y, float w, float h, float alpha) {
	weaponFx_t* fx;
	vec4_t iconColor = {1.0f, 1.0f, 1.0f, 1.0f};
	qhandle_t icon;

	if(weapon <= WP_NONE || weapon >= jsd_weaponCount) return;
	fx = &jsd_weaponFx[weapon];
	icon = cg_weapons[weapon].weaponIcon;
	if(!icon) return;

	iconColor[3] = alpha;
	if(fx->colorizeItemIcon) {
		iconColor[0] = 1.0f;
		iconColor[1] = 0.78f;
		iconColor[2] = 0.20f;
	}

	trap_R_SetColor(iconColor);
	drawhShader(x * cgui.scale * WEAPON_SELECTOR_SCALE, y * cgui.scale * WEAPON_SELECTOR_SCALE, w * cgui.scale * WEAPON_SELECTOR_SCALE, h * cgui.scale * WEAPON_SELECTOR_SCALE, icon);
	trap_R_SetColor(NULL);

	if(cg.swep_listcl[weapon] == WS_NOAMMO) {
		drawhShader(x * cgui.scale * WEAPON_SELECTOR_SCALE, y * cgui.scale * WEAPON_SELECTOR_SCALE, w * cgui.scale * WEAPON_SELECTOR_SCALE, h * cgui.scale * WEAPON_SELECTOR_SCALE, cgs.media.noammoShader);
	}
}

static void CG_DrawWeaponSelect(void) {
	vec4_t inactiveColor = {0.0f, 0.0f, 0.0f, 1.00f};
	vec4_t activeColor = {0.0f, 0.0f, 0.0f, 1.00f};
	vec4_t selectedColor = {0.0f, 0.0f, 0.0f, 1.00f};
	vec4_t numberColor = {1.0f, 0.86f, 0.20f, 1.00f};
	vec4_t weaponNameColor = {1.0f, 0.86f, 0.20f, 1.00f};
	int sectionWeapons[MAX_WEAPONS];
	int activeSection, section, count, i, weapon;
	bool firstWeaponSelected;
	float x, y, width, height, activeX, alpha = 1.0f;
	float layoutWidth;
	float screenWidth;
	float remaining;

	if(cg.predictedPlayerState.stats[STAT_HEALTH] <= 0) return;
	if(!cg_draw2D.integer || cg.showScores || !cg.weaponSelectTime) return;
	if(cg.time > cg.weaponSelectTime + WEAPON_SELECTOR_TIME) return;

	remaining = (float)(cg.weaponSelectTime + WEAPON_SELECTOR_TIME - cg.time);
	if(remaining < WEAPON_SELECTOR_FADEOUT) alpha = remaining / WEAPON_SELECTOR_FADEOUT;
	if(alpha < 0.0f) alpha = 0.0f;
	if(alpha > 1.0f) alpha = 1.0f;

	activeSection = 1;
	if(cg.weaponSelect > WP_NONE && cg.weaponSelect < jsd_weaponCount && jsd_weaponFx[cg.weaponSelect].weaponSection >= 1 && jsd_weaponFx[cg.weaponSelect].weaponSection <= WEAPON_SELECTOR_SECTIONS) {
		activeSection = jsd_weaponFx[cg.weaponSelect].weaponSection;
	}

	// Keep horizontal centering, but anchor the menu at a fixed top offset.
	// Only the x origin depends on the current screen resolution.
	layoutWidth = WEAPON_SELECTOR_HEADER_ACTIVE_W + ((WEAPON_SELECTOR_SECTIONS - 1) * WEAPON_SELECTOR_HEADER_IDLE) + ((WEAPON_SELECTOR_SECTIONS - 1) * WEAPON_SELECTOR_GAP);
	screenWidth = (cgui.scale > 0.0f) ? ((float)glconfig.vidWidth / cgui.scale) : 640.0f;
	x = ((screenWidth / WEAPON_SELECTOR_SCALE) - layoutWidth) * 0.5f;
	y = WEAPON_SELECTOR_Y;
	activeX = x + (activeSection - 1) * (WEAPON_SELECTOR_HEADER_IDLE + WEAPON_SELECTOR_GAP);
	for(section = 1; section <= WEAPON_SELECTOR_SECTIONS; section++) {
		count = CG_GetWeaponSectionList(section, sectionWeapons, ARRAY_LEN(sectionWeapons));
		if(section == activeSection) {
			width = WEAPON_SELECTOR_HEADER_ACTIVE_W;
			firstWeaponSelected = (count > 0 && sectionWeapons[0] == cg.weaponSelect);
			height = firstWeaponSelected ? WEAPON_SELECTOR_ROW_SELECTED_H : WEAPON_SELECTOR_HEADER_ACTIVE_H;
			if(firstWeaponSelected) {
				selectedColor[3] = 0.23f * alpha;
				CG_DrawWeaponSelectorRect(x, y, width, height, 6.0f, selectedColor);
			} else {
				activeColor[3] = 0.21f * alpha;
				CG_DrawWeaponSelectorRect(x, y, width, height, 6.0f, activeColor);
			}
		} else {
			width = WEAPON_SELECTOR_HEADER_IDLE;
			height = WEAPON_SELECTOR_HEADER_IDLE;
			inactiveColor[3] = 0.15f * alpha;
			CG_DrawWeaponSelectorRect(x, y, width, height, 6.0f, inactiveColor);
		}

		{
			char number[2];
			number[0] = '0' + section;
			number[1] = '\0';
			numberColor[3] = alpha;
			CG_DrawWeaponSelectorText(x + 9.0f, y + 8.0f, number, FONTSTYLE_BOLD, numberColor, 0.85f);
		}

		if(section == activeSection && count > 0) {
			const char* name;
			weapon = sectionWeapons[0];
			name = cg_weapons[weapon].item->pickup_name;
			if(weapon == cg.weaponSelect) {
				CG_RegisterWeapon(weapon);
				CG_DrawWeaponSelectorIcon(weapon, x + 35.0f, y + 39.0f, 200.0f, 100.0f, alpha);
				weaponNameColor[3] = alpha;
				CG_DrawWeaponSelectorText(x + (width * 0.5f), y + height - 20.0f, name, FONTSTYLE_BOLD | FONTSTYLE_CENTER, weaponNameColor, 0.65f);
			} else {
				weaponNameColor[3] = alpha;
				CG_DrawWeaponSelectorText(x + (width * 0.5f), y + 30.0f, name, FONTSTYLE_BOLD | FONTSTYLE_CENTER, weaponNameColor, 0.65f);
			}
		}

		x += width + WEAPON_SELECTOR_GAP;
	}

	// The first weapon in the active section is represented by the expanded
	// header. Remaining weapons are listed below; the selected row is tall and
	// displays the rectangular item icon.
	count = CG_GetWeaponSectionList(activeSection, sectionWeapons, ARRAY_LEN(sectionWeapons));
	x = activeX;
	/* If weapon #0 is selected, its expanded header occupies the first card. */
	firstWeaponSelected = (count > 0 && sectionWeapons[0] == cg.weaponSelect);
	y += firstWeaponSelected ? (WEAPON_SELECTOR_ROW_SELECTED_H + WEAPON_SELECTOR_GAP) : (WEAPON_SELECTOR_HEADER_ACTIVE_H + WEAPON_SELECTOR_GAP);
	for(i = 1; i < count; i++) {
		weapon = sectionWeapons[i];
		bool selected = (weapon == cg.weaponSelect);
		const char* name;

		name = cg_weapons[weapon].item->pickup_name;
		height = selected ? WEAPON_SELECTOR_ROW_SELECTED_H : WEAPON_SELECTOR_ROW_H;

		if(selected) {
			selectedColor[3] = 0.23f * alpha;
			CG_DrawWeaponSelectorRect(x, y, WEAPON_SELECTOR_HEADER_ACTIVE_W, height, 6.0f, selectedColor);
			CG_DrawWeaponSelectorIcon(weapon, x + 35.0f, y + 39.0f, 200.0f, 100.0f, alpha);
			weaponNameColor[3] = alpha;
			CG_DrawWeaponSelectorText(x + (WEAPON_SELECTOR_HEADER_ACTIVE_W * 0.5f), y + height - 20.0f, name, FONTSTYLE_BOLD | FONTSTYLE_CENTER, weaponNameColor, 0.65f);
		} else {
			inactiveColor[3] = 0.16f * alpha;
			CG_DrawWeaponSelectorRect(x, y, WEAPON_SELECTOR_HEADER_ACTIVE_W, height, 6.0f, inactiveColor);
			weaponNameColor[3] = alpha;
			CG_DrawWeaponSelectorText(x + (WEAPON_SELECTOR_HEADER_ACTIVE_W * 0.5f), y + 30.0f, name, FONTSTYLE_BOLD | FONTSTYLE_CENTER, weaponNameColor, 0.65f);
		}

		y += height + WEAPON_SELECTOR_GAP;
	}
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

static bool n_tip1 = false;
static bool n_tip2 = false;

#define FPS_FRAMES 4
static void CG_DrawCounters(void) {
	int y;
	int value = 0;
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

	CG_ConsoleUpdateIdx(console, maxlines);

	j = 0;
	for(i = console->displayIdx; i < console->insertIdx; ++i) {
		if(console->msgTimes[i % CONSOLE_MAXHEIGHT] + time < cg.time) continue;
		drawStringAdjusted(x + 1, y + (j * (FONT_SIZE * size)), console->msgs[i % CONSOLE_MAXHEIGHT], FONTSTYLE_LEFT | FONTSTYLE_DROPSHADOW, color_white, size, 256);
		j++;
	}
}

void CG_AddToGenericConsole(const char* str, console_t* console) {
	char* p;

	p = console->msgs[console->insertIdx % CONSOLE_MAXHEIGHT];
	*p = 0;

	while(*str) {
		*p++ = *str++;
	}
	*p = 0;

	console->msgTimes[console->insertIdx % CONSOLE_MAXHEIGHT] = cg.time;
	console->insertIdx++;
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
	if(mod > 0 && mod < MOD_WEAPONS) return cg_items[ITEM_INDEX(BG_FindItemForWeapon(mod))].icon;
	if(mod == MOD_WATER) return trap_R_RegisterShaderNoMip("icons/d_water");
	if(mod == MOD_SLIME) return trap_R_RegisterShaderNoMip("icons/d_slime");
	if(mod == MOD_LAVA) return trap_R_RegisterShaderNoMip("icons/d_lava");
	if(mod == MOD_CRUSH) return trap_R_RegisterShaderNoMip("icons/d_death");
	if(mod == MOD_TELEFRAG) return trap_R_RegisterShaderNoMip("icons/teleporter");
	if(mod == MOD_FALLING) return trap_R_RegisterShaderNoMip("icons/d_fall");
	if(mod == MOD_SUICIDE) return trap_R_RegisterShaderNoMip("icons/d_death");
	if(mod == MOD_TRIGGER_HURT) return trap_R_RegisterShaderNoMip("icons/d_death");
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

	for(i = MAX_NOTIFICATIONS - 1; i > 0; i--) cg.notifications[i] = cg.notifications[i - 1];

	Q_strncpyz(cg.notifications[0].text, text, 255);
	Q_strncpyz(cg.notifications[0].picPath, picPath, 255);
	cg.notifications[0].text[255] = '\0';
	cg.notifications[0].picPath[255] = '\0';
	cg.notifications[0].type = type;
	cg.notifications[0].number = number;
	cg.notifications[0].startTime = cg.time;
	cg.notifications[0].active = true;

	if(type == NOTIFY_INFO) trap_S_StartLocalSound(cgs.media.notifySound[rand() & 3], CHAN_LOCAL_SOUND);
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

		boxWidth = stringWidth(cg.notifications[i].text, 0.50, FONTSTYLE_LEFT, 256) + (cg.notifications[i].type == NOTIFY_PLAIN ? 16 : 32);

		yOffset = startY - (20 + margin) * i;

		drawRoundedRectAdjusted(startX - boxWidth + offsetX, yOffset, boxWidth, 20, 4, backgroundColor, 0);

		trap_R_SetColor(picColor);
		if(cg.notifications[i].type == NOTIFY_INFO && cgs.media.notifyIcon) drawhShaderAdjusted(startX - boxWidth + offsetX + 5, yOffset + 2.5, 16, 16, cgs.media.notifyIcon);
		if(cg.notifications[i].type == NOTIFY_UNDO && cgs.media.undoIcon) drawhShaderAdjusted(startX - boxWidth + offsetX + 5, yOffset + 2.5, 16, 16, cgs.media.undoIcon);
		if(cg.notifications[i].type == NOTIFY_ITEM) drawhShaderAdjusted(startX - boxWidth + offsetX + 5, yOffset + 2.5, 16, 16, cg_items[cg.notifications[i].number].icon);
		if(cg.notifications[i].type == NOTIFY_KILL) drawhShaderAdjusted(startX - boxWidth + offsetX + 5, yOffset + 2.5, 16, 16, CG_FindModImage(cg.notifications[i].number));
		if(cg.notifications[i].type == NOTIFY_CUSTOM) drawShaderAdjusted(startX - boxWidth + offsetX + 5, yOffset + 2.5, 16, 16, cg.notifications[i].picPath);
		trap_R_SetColor(NULL);

		drawStringAdjusted(startX - boxWidth + offsetX + (cg.notifications[i].type == NOTIFY_PLAIN ? 8 : 26), yOffset + 5, cg.notifications[i].text, FONTSTYLE_LEFT | FONTSTYLE_DROPSHADOW, textColor, 0.50, 256);
	}
}

static void CG_Draw2D(void) {
	int catcher = trap_Key_GetCatcher();

	if(cg_draw2D.integer == 0) return;

	CG_DrawGenericConsole(&cgs.chat, 5, 10000, 0 - cgui.wideoffset, 350, 0.50);

	CG_DrawCrosshair3D();
	CG_DrawCounters();

	if(!(catcher & KEYCATCH_UI)) {
		CG_Notify();
		CG_NSErrors();
	}

	CG_UpdateWeaponStatus();
	CG_DrawWeaponSelect();

	if(!(catcher & KEYCATCH_UI)) CG_DrawScoreboard();
}

void CG_DrawActive(void) {
	if(!cg.snap) {
		CG_DrawInformation();
		return;
	}

	trap_R_RenderScene(&cg.refdef);  // draw 3D view
	CG_Draw3DStringQueue();          // draw 3D text
	CG_Draw2D();                     // draw C elements
	JS_HUDDraw();                    // draw JS elements
}
