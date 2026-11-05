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
// g_bot.c

#include "g_local.h"

/*
===============
G_BotConnect
===============
*/
bool G_BotConnect(int clientNum, bool restart) {
	bot_settings_t settings;
	char userinfo[MAX_INFO_STRING];

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	settings.skill = atof(Info_ValueForKey(userinfo, "skill"));

	if(!BotAISetupClient(clientNum, &settings, restart)) {
		trap_DropClient(clientNum, "BotAISetupClient failed");
		return false;
	}

	return true;
}

/*
===============
G_AddBot

Any name, any model, skill 1-5.
===============
*/
static void G_AddBot(const char* name, const char* model, float skill) {
	int clientNum;
	char userinfo[MAX_INFO_STRING];

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if(clientNum == -1) {
		G_Printf(S_COLOR_RED "Unable to add bot. All player slots are in use.\n");
		G_Printf(S_COLOR_RED "Start server with more 'open' slots (or check setting of sv_maxclients cvar).\n");
		return;
	}

	// create the bot's userinfo
	userinfo[0] = '\0';

	Info_SetValueForKey(userinfo, "name", name);
	Info_SetValueForKey(userinfo, "model", model);
	Info_SetValueForKey(userinfo, "rate", "25000");
	Info_SetValueForKey(userinfo, "snaps", "20");
	Info_SetValueForKey(userinfo, "skill", va("%.2f", skill));

	// register the userinfo
	trap_SetUserinfo(clientNum, userinfo);

	// have it connect to the game as a normal client
	if(ClientConnect(clientNum, true, true)) {
		return;
	}

	ClientBegin(clientNum);
}

/*
===============
Svcmd_AddBot_f

addbot <name> <model> [skill 1-5]
===============
*/
void Svcmd_AddBot_f(void) {
	float skill;
	char name[MAX_TOKEN_CHARS];
	char model[MAX_TOKEN_CHARS];
	char string[MAX_TOKEN_CHARS];

	// are bots enabled?
	if(!trap_Cvar_VariableIntegerValue("bot_enable")) {
		return;
	}

	// name
	trap_Argv(1, name, sizeof(name));
	// model
	trap_Argv(2, model, sizeof(model));
	if(!name[0]) {
		trap_Print("Usage: addbot <name> <model> [skill 1-5]\n");
		return;
	}
	// no model given: fall back to the name, like the old addbot <name/model>
	if(!model[0]) {
		Q_strncpyz(model, name, sizeof(model));
	}

	// skill
	trap_Argv(3, string, sizeof(string));
	if(!string[0]) {
		skill = 4;
	} else {
		skill = Com_Clamp(1, 5, atof(string));
	}

	G_AddBot(name, model, skill);

	// if this was issued during gameplay and we are playing locally,
	// go ahead and load the bot's media immediately
	if(level.time - level.startTime > 1000 && trap_Cvar_VariableIntegerValue("cl_running")) {
		trap_SendServerCommand(-1, "loaddefered\n");
	}
}
