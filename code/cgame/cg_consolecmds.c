// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2

#include "cg_local.h"

static void CG_ScoresDown_f(void) {
	if(cg.scoresRequestTime + 2000 < cg.time) {
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand("score");

		if(!cg.showScores) {
			cg.showScores = true;
			cg.numScores = 0;
		}
	} else {
		cg.showScores = true;
	}
}

static void CG_ScoresUp_f(void) {
	if(cg.showScores) cg.showScores = false;
}

typedef struct {
	char* cmd;
	void (*function)(void);
} consoleCommand_t;

static consoleCommand_t commands[] = {
    {"+scores", CG_ScoresDown_f},
    {"-scores", CG_ScoresUp_f},
    {"+zoom", CG_ZoomDown_f},
    {"-zoom", CG_ZoomUp_f},
    {"weapnext", CG_NextWeapon_f},
    {"weapprev", CG_PrevWeapon_f},
    {"weapon", CG_Weapon_f},
    {"loaddeferred", CG_LoadDeferredPlayers},
};

bool CG_ConsoleCommand(void) {
	const char* cmd;
	int i;

	cmd = CG_Argv(0);

	for(i = 0; i < ARRAY_LEN(commands); i++) {
		if(!Q_stricmp(cmd, commands[i].cmd)) {
			commands[i].function();
			return true;
		}
	}

	return false;
}

void CG_InitConsoleCommands(void) {
	int i;

	for(i = 0; i < ARRAY_LEN(commands); i++) {
		trap_AddCommand(commands[i].cmd);
	}

	trap_AddCommand("kill");
	trap_AddCommand("say");
	trap_AddCommand("give");
	trap_AddCommand("god");
	trap_AddCommand("notarget");
	trap_AddCommand("noclip");
	trap_AddCommand("where");
	trap_AddCommand("addbot");
	trap_AddCommand("setviewpos");
	trap_AddCommand("stats");
	trap_AddCommand("loaddefered");
}
