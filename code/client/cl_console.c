// Copyright (C) 2023-2026 Noire's Mod [noire.dev] — GPLv2

#include "client.h"

typedef struct {
	float displayFrac;  // aproaches finalFrac at scr_conspeed
	float finalFrac;    // 0.0 to 1.0 lines of console to display
} console_t;

console_t con;
uiconsole_t uiconsole;

int uiconsoleScroll = 0;

cvar_t* con_autoclear;

void Con_ToggleConsole_f(void) {
	if(clc.state == CA_DISCONNECTED && Key_GetCatcher() == KEYCATCH_CONSOLE) return;
	if(con_autoclear->integer) Field_Clear(&g_consoleField);

	g_consoleField.widthInChars = 32;
	Key_SetCatcher(Key_GetCatcher() ^ KEYCATCH_CONSOLE);
}

void Con_MessageMode_f(void) {
	chat_playerNum = -1;
	chat_team = qfalse;
	Field_Clear(&chatField);
	chatField.widthInChars = 32;

	Key_SetCatcher(Key_GetCatcher() ^ KEYCATCH_MESSAGE);
}

void Con_MessageMode2_f(void) {
	chat_playerNum = -1;
	chat_team = qtrue;
	Field_Clear(&chatField);
	chatField.widthInChars = 32;
	Key_SetCatcher(Key_GetCatcher() ^ KEYCATCH_MESSAGE);
}

void Con_MessageMode3_f(void) {
	chat_playerNum = VM_Call(cgvm, CG_CROSSHAIR_PLAYER);
	if(chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear(&chatField);
	chatField.widthInChars = 32;
	Key_SetCatcher(Key_GetCatcher() ^ KEYCATCH_MESSAGE);
}

void Con_Init(void) {
	int i;

	con_autoclear = Cvar_Get("con_autoclear", "1", CVAR_ARCHIVE);

	Field_Clear(&g_consoleField);
	g_consoleField.widthInChars = 32;
	for(i = 0; i < COMMAND_HISTORY; i++) {
		Field_Clear(&historyEditLines[i]);
		historyEditLines[i].widthInChars = 32;
	}
	CL_LoadConsoleHistory();

	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand("messagemode", Con_MessageMode_f);
	Cmd_AddCommand("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand("messagemode3", Con_MessageMode3_f);
}

void Con_Shutdown(void) {
	Cmd_RemoveCommand("toggleconsole");
	Cmd_RemoveCommand("messagemode");
	Cmd_RemoveCommand("messagemode2");
	Cmd_RemoveCommand("messagemode3");
}

void CL_ConsolePrint(const char* txt) {
	if(!txt) return;

	if(uiconsole.linescount >= CON_MAXLINES) {
		for(int i = 0; i < CON_MAXLINES - CON_PURGE_AMOUNT; i++) memcpy(uiconsole.lines[i], uiconsole.lines[i + CON_PURGE_AMOUNT], CON_MAXLINE);
		for(int i = CON_MAXLINES - CON_PURGE_AMOUNT; i < CON_MAXLINES; i++) uiconsole.lines[i][0] = '\0';
		uiconsole.linescount = CON_MAXLINES - CON_PURGE_AMOUNT;
	}

	strncpy(uiconsole.lines[uiconsole.linescount], txt, CON_MAXLINE - 1);
	uiconsole.linescount++;
}

static void Con_DrawNotify(void) {
	static float consoleBG[4] = {0.00, 0.00, 0.00, 0.80};
	static float consoleOutlineBG[4] = {0.10, 0.10, 0.10, 0.80};
	if(Key_GetCatcher() & (KEYCATCH_UI | KEYCATCH_CGAME)) return;

	if(Key_GetCatcher() & KEYCATCH_MESSAGE) {
		drawRoundedRect(((480 * cls.scale) + ((cls.wideoffset * cls.scale) * 0.5)) - 456 * cls.scale, 0, 912 * cls.scale, 24 * cls.scale, 6 * cls.scale, consoleBG, NO_TOP_LEFT | NO_TOP_RIGHT);
		drawRoundedRect(((480 * cls.scale) + ((cls.wideoffset * cls.scale) * 0.5)) - 456 * cls.scale, 0, (912 * cls.scale) + 2, (24 * cls.scale) + 2, 6 * cls.scale, consoleOutlineBG, NO_TOP_LEFT | NO_TOP_RIGHT);
		drawStringField((((480 * cls.scale) + ((cls.wideoffset * cls.scale) * 0.5)) - 456 * cls.scale) + 10 * cls.scale, 5 * cls.scale, chatField.buffer, FONTSTYLE_DROPSHADOW, color_white, 0.65 * cls.scale, 114, chatField.cursor);
	}
}

#define CONSOLE_LINES 18
static void Con_DrawSolidConsole(float frac) {
	static float consoleBG[4] = {0.00, 0.00, 0.00, 0.80};
	static float consoleOutlineBG[4] = {0.10, 0.10, 0.10, 0.80};
	static float consoleFieldBG[4] = {0.15, 0.15, 0.15, 0.80};
	int consoleLines = CONSOLE_LINES * frac;
	int currentLineID = (uiconsole.linescount - uiconsoleScroll) - consoleLines;

	drawRoundedRect(((480 * cls.scale) + ((cls.wideoffset * cls.scale) * 0.5)) - 456 * cls.scale, 0, 912 * cls.scale, (360 * cls.scale) * frac, 16 * cls.scale, consoleBG, NO_TOP_LEFT | NO_TOP_RIGHT);
	drawRoundedRect(((480 * cls.scale) + ((cls.wideoffset * cls.scale) * 0.5)) - 456 * cls.scale, 0, (912 * cls.scale) + 2, ((360 * cls.scale) * frac) + 2, 16 * cls.scale, consoleOutlineBG, NO_TOP_LEFT | NO_TOP_RIGHT);
	for(int i = 0; i < consoleLines; i++) {
		if(currentLineID < 0) {
			uiconsoleScroll = uiconsole.linescount - consoleLines;
		}
		if(currentLineID >= uiconsole.linescount) {
			uiconsoleScroll = 0;
			break;
		}
		drawString((((480 * cls.scale) + ((cls.wideoffset * cls.scale) * 0.5)) - 456 * cls.scale) + 10 * cls.scale, 10 * cls.scale + (i * (FONT_SIZE * (0.70 * cls.scale))), uiconsole.lines[currentLineID], FONTSTYLE_DROPSHADOW, color_white, 0.60 * cls.scale, 124);
		currentLineID += 1;
	}
	if(frac >= 0.95) {
		drawRoundedRect((((480 * cls.scale) + ((cls.wideoffset * cls.scale) * 0.5)) - 456 * cls.scale) + 8 * cls.scale, 6 * cls.scale + (19 * (FONT_SIZE * (0.70 * cls.scale))), 896 * cls.scale, 18 * cls.scale, 4 * cls.scale, consoleFieldBG, 0);
		drawStringField((((480 * cls.scale) + ((cls.wideoffset * cls.scale) * 0.5)) - 456 * cls.scale) + 16 * cls.scale, 10 * cls.scale + (19 * (FONT_SIZE * (0.70 * cls.scale))), g_consoleField.buffer, FONTSTYLE_DROPSHADOW, color_white, 0.60 * cls.scale, 122, g_consoleField.cursor);
	}
}

void Con_DrawConsole(void) {
	if(clc.state == CA_DISCONNECTED) {
		if(!(Key_GetCatcher() & (KEYCATCH_UI | KEYCATCH_CGAME))) {
			Con_DrawSolidConsole(1.0);
			return;
		}
	}

	if(con.displayFrac) {
		Con_DrawSolidConsole(con.displayFrac);
	} else {
		if(clc.state == CA_ACTIVE) Con_DrawNotify();
	}
}

void Con_RunConsole(void) {
	if(Key_GetCatcher() & KEYCATCH_CONSOLE)
		con.finalFrac = 1.0;
	else
		con.finalFrac = 0.0;

	if(con.finalFrac < con.displayFrac) {
		con.displayFrac -= 5 * cls.realFrametime * 0.001;
		if(con.finalFrac > con.displayFrac) con.displayFrac = con.finalFrac;

	} else if(con.finalFrac > con.displayFrac) {
		con.displayFrac += 5 * cls.realFrametime * 0.001;
		if(con.finalFrac < con.displayFrac) con.displayFrac = con.finalFrac;
	}
}

void Con_PageUp(void) { uiconsoleScroll += 1; }
void Con_PageDown(void) { uiconsoleScroll -= 1; }
void Con_Top(void) { uiconsoleScroll = uiconsole.linescount - CONSOLE_LINES; }
void Con_Bottom(void) { uiconsoleScroll = 0; }

void Con_Close(void) {
	if(!com_cl_running->integer) return;

	Field_Clear(&g_consoleField);
	Key_SetCatcher(Key_GetCatcher() & ~KEYCATCH_CONSOLE);
	con.finalFrac = 0.0;  // none visible
	con.displayFrac = 0.0;
}
