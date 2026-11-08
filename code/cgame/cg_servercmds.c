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
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definitely
// be a valid snapshot this frame

#include "cg_local.h"

#define NUM_DATA 2
#define FIRST_DATA 1

static void CG_ParseScores(void) {
	int i;

	cg.numScores = atoi(CG_Argv(1));
	if(cg.numScores > MAX_CLIENTS) cg.numScores = MAX_CLIENTS;

	memset(cg.scores, 0, sizeof(cg.scores));

	for(i = 0; i < cg.numScores; i++) {
		cg.scores[i].client = atoi(CG_Argv(i * NUM_DATA + FIRST_DATA + 1));
		cg.scores[i].ping = atoi(CG_Argv(i * NUM_DATA + FIRST_DATA + 2));

		if(cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS) cg.scores[i].client = 0;
	}
}

static void CG_ParseSweps(void) {
	int i;
	int weaponIndex;
	int numArgs = trap_Argc();

	for(i = 0; i < jsd_weaponCount; i++) cg.swep_listcl[i] = WS_NONE;

	for(i = 1; i < numArgs; i++) {
		weaponIndex = atoi(CG_Argv(i));

		if(weaponIndex >= -jsd_weaponCount && weaponIndex < jsd_weaponCount) {
			if(weaponIndex > 0) {
				cg.swep_listcl[weaponIndex] = WS_HAVE;
			} else if(weaponIndex < 0) {
				cg.swep_listcl[-weaponIndex] = WS_NOAMMO;
			}
		}
	}
}

static void CG_ParseSpawnSweps(void) {
	int i;
	int weaponIndex;
	int numArgs = trap_Argc();

	for(i = 0; i < jsd_weaponCount; i++) cg.swep_spawncl[i] = WS_NONE;

	for(i = 1; i < numArgs; i++) {
		weaponIndex = atoi(CG_Argv(i));

		if(weaponIndex >= -jsd_weaponCount && weaponIndex < jsd_weaponCount) {
			if(weaponIndex > 0) {
				cg.swep_spawncl[weaponIndex] = WS_HAVE;
			} else if(weaponIndex < 0) {
				cg.swep_spawncl[-weaponIndex] = WS_NOAMMO;
			}
		}
	}
}

/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo(void) {
	const char* info;
	char* mapname;

	info = CG_ConfigString(CS_SERVERINFO);
	cgs.maxclients = atoi(Info_ValueForKey(info, "sv_maxclients"));
	mapname = Info_ValueForKey(info, "mapname");
	Com_sprintf(cgs.mapname, sizeof(cgs.mapname), "maps/%s.bsp", mapname);
}

/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues(void) { cgs.levelStartTime = atoi(CG_ConfigString(CS_LEVEL_START_TIME)); }

/*
================
CG_ConfigStringModified

================
*/
static void CG_ConfigStringModified(void) {
	const char* str;
	int num;

	num = atoi(CG_Argv(1));

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState(&cgs.gameState);

	// look up the individual string that was modified
	str = CG_ConfigString(num);

	// do something with it if necessary
	if(num == CS_MUSIC) {
		CG_StartMusic();
	} else if(num == CS_SERVERINFO) {
		CG_ParseServerinfo();
	} else if(num == CS_LEVEL_START_TIME) {
		cgs.levelStartTime = atoi(str);
	} else if(num >= CS_MODELS && num < CS_MODELS + MAX_MODELS) {
		cgs.gameModels[num - CS_MODELS] = trap_R_RegisterModel(str);
	} else if(num >= CS_SOUNDS && num < CS_SOUNDS + MAX_SOUNDS) {
		if(str[0] != '*') {  // player specific sounds don't register here
			cgs.gameSounds[num - CS_SOUNDS] = trap_S_RegisterSound(str, false);
		}
	} else if(num >= CS_PLAYERS && num < CS_PLAYERS + MAX_CLIENTS) {
		CG_NewClientInfo(num - CS_PLAYERS);
	}
}

/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart(void) {
	if(cg_showmiss.integer) {
		CG_Printf("CG_MapRestart\n");
	}

	CG_InitLocalEntities();
	CG_InitMarkPolys();
	CG_ClearParticles();

	cg.mapRestart = true;

	CG_StartMusic();

	trap_S_ClearLoopingSounds(true);

	// we really should clear more parts of cg here and stop sounds
	trap_Cvar_Set("cg_thirdPerson", "0");
}

/*
=================
CG_RemoveChatEscapeChar
=================
*/
static void CG_RemoveChatEscapeChar(char* text) {
	int i, l;

	l = 0;
	for(i = 0; text[i]; i++) {
		if(text[i] == '\x19') continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
static void CG_ServerCommand(void) {
	const char* cmd;
	char text[MAX_SAY_TEXT];

	cmd = CG_Argv(0);

	if(!cmd[0]) {
		// server claimed the command
		return;
	}

	if(!strcmp(cmd, "cs")) {
		CG_ConfigStringModified();
		return;
	}

	if(!strcmp(cmd, "print")) {
		CG_Printf("%s", CG_Argv(1));
		return;
	}

	if(!strcmp(cmd, "chat")) {
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
		Q_strncpyz(text, CG_Argv(1), MAX_SAY_TEXT);
		CG_RemoveChatEscapeChar(text);
		CG_Printf("%s\n", text);
		return;
	}

	if(!strcmp(cmd, "scores")) {
		CG_ParseScores();
		return;
	}

	if(!strcmp(cmd, "swep")) {
		CG_ParseSweps();
		return;
	}

	if(!strcmp(cmd, "sweps")) {
		CG_ParseSpawnSweps();
		return;
	}

	if(!strcmp(cmd, "map_restart")) {
		CG_MapRestart();
		return;
	}

	if(Q_stricmp(cmd, "remapShader") == 0) {
		if(trap_Argc() == 4) {
			char shader1[MAX_QPATH];
			char shader2[MAX_QPATH];
			char shader3[MAX_QPATH];

			Q_strncpyz(shader1, CG_Argv(1), sizeof(shader1));
			Q_strncpyz(shader2, CG_Argv(2), sizeof(shader2));
			Q_strncpyz(shader3, CG_Argv(3), sizeof(shader3));

			trap_R_RemapShader(shader1, shader2, shader3);
		}

		return;
	}

	// loaddeferred can be both a servercmd and a consolecmd
	if(!strcmp(cmd, "loaddefered")) {  // FIXME: spelled wrong, but not changing for demo
		CG_LoadDeferredPlayers();
		return;
	}

	CG_Printf("Unknown client game command: %s\n", cmd);
}

/*
====================
CG_ExecuteNewServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void CG_ExecuteNewServerCommands(int latestSequence) {
	while(cgs.serverCommandSequence < latestSequence) {
		if(trap_GetServerCommand(++cgs.serverCommandSequence)) {
			CG_ServerCommand();
		}
	}
}
