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
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"

void CG_TargetCommand_f(void) {
	int targetNum;
	char test[4];

	targetNum = CG_CrosshairPlayer();
	if(targetNum == -1) {
		return;
	}

	trap_Argv(1, test, 4);
	trap_SendClientCommand(va("gc %i %i", targetNum, atoi(test)));
}

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

static void CG_TellTarget_f(void) {
	int clientNum;
	char command[128];
	char message[128];

	clientNum = CG_CrosshairPlayer();
	if(clientNum == -1) {
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void CG_TellAttacker_f(void) {
	int clientNum;
	char command[128];
	char message[128];

	clientNum = CG_LastAttacker();
	if(clientNum == -1) {
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f(void) {
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("developer", var, sizeof(var));
	if(!atoi(var)) {
		return;
	}
	if(cg_cameraOrbit.value != 0) {
		trap_Cvar_Set("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

typedef struct {
	char* cmd;
	void (*function)(void);
} consoleCommand_t;

static consoleCommand_t commands[] = {{"+scores", CG_ScoresDown_f},
                                      {"-scores", CG_ScoresUp_f},
                                      {"+zoom", CG_ZoomDown_f},
                                      {"-zoom", CG_ZoomUp_f},
                                      {"weapnext", CG_NextWeapon_f},
                                      {"weapprev", CG_PrevWeapon_f},
                                      {"weapon", CG_Weapon_f},
                                      {"tcmd", CG_TargetCommand_f},
                                      {"tell_target", CG_TellTarget_f},
                                      {"tell_attacker", CG_TellAttacker_f},
                                      {"startOrbit", CG_StartOrbit_f},
                                      //{ "camera", CG_Camera_f },
                                      {"loaddeferred", CG_LoadDeferredPlayers}};

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
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

/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands(void) {
	int i;

	for(i = 0; i < ARRAY_LEN(commands); i++) {
		trap_AddCommand(commands[i].cmd);
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
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
	trap_AddCommand("loaddefered");  // spelled wrong, but not changing for demo
}
