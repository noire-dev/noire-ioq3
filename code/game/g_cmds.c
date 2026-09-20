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
#include "g_local.h"

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage(gentity_t* ent) {
	char entry[1024];
	char string[1000];
	int stringlength;
	int i, j;
	gclient_t* cl;
	int numSorted, scoreFlags, accuracy, perfect;

	// don't send scores to bots, they don't parse it
	if(ent->r.svFlags & SVF_BOT) {
		return;
	}

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	numSorted = level.numConnectedClients;

	for(i = 0; i < numSorted; i++) {
		int ping;

		cl = &level.clients[level.sortedClients[i]];

		if(cl->pers.connected == CON_CONNECTING) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		if(cl->accuracy_shots) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		} else {
			accuracy = 0;
		}
		perfect = (cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0) ? 1 : 0;

		Com_sprintf(entry, sizeof(entry), " %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i], cl->ps.persistant[PERS_SCORE], ping, (level.time - cl->pers.enterTime) / 60000, scoreFlags, g_entities[level.sortedClients[i]].s.powerups, accuracy, cl->ps.persistant[PERS_IMPRESSIVE_COUNT], cl->ps.persistant[PERS_EXCELLENT_COUNT], cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], cl->ps.persistant[PERS_DEFEND_COUNT], cl->ps.persistant[PERS_ASSIST_COUNT], perfect, cl->ps.persistant[PERS_CAPTURES]);
		j = strlen(entry);
		if(stringlength + j >= sizeof(string)) break;
		strcpy(string + stringlength, entry);
		stringlength += j;
	}

	trap_SendServerCommand(ent - g_entities, va("scores %i %i %i%s", i, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], string));
}

void G_SendSwepWeapons(gentity_t* ent) {
	char string[4096] = "";
	int i;
	int len;

	for(i = 1; i < WEAPONS_NUM; i++) {
		if(ent->swep_list[i] >= WS_HAVE) {
			if(ent->swep_ammo[i] > 0 || ent->swep_ammo[i] == -1) {
				ent->swep_list[i] = WS_HAVE;  // we have weapon and ammo
			} else {
				ent->swep_list[i] = WS_NOAMMO;  // we have weapon only
			}
		}
		if(ent->swep_list[i] == WS_HAVE) {
			Q_strcat(string, sizeof(string), va("%i ", i));
		}
		if(ent->swep_list[i] == WS_NOAMMO) {
			Q_strcat(string, sizeof(string), va("%i ", i * -1));  // use -id for send WS_NOAMMO
		}
	}
	len = strlen(string);
	if(len > 0 && string[len - 1] == ' ') {
		string[len - 1] = '\0';
	}

	trap_SendServerCommand(ent - g_entities, va("swep %s", string));
}

void G_SendSpawnSwepWeapons(gentity_t* ent) {
	char string[4096] = "";
	int i;
	int len;

	for(i = 1; i < WEAPONS_NUM; i++) {
		if(ent->swep_list[i] >= WS_HAVE) {
			if(ent->swep_ammo[i] > 0 || ent->swep_ammo[i] == -1) {
				ent->swep_list[i] = WS_HAVE;  // we have weapon and ammo
			} else {
				ent->swep_list[i] = WS_NOAMMO;  // we have weapon only
			}
		}
		if(ent->swep_list[i] == WS_HAVE) {
			Q_strcat(string, sizeof(string), va("%i ", i));
		}
		if(ent->swep_list[i] == WS_NOAMMO) {
			Q_strcat(string, sizeof(string), va("%i ", i * -1));  // use -id for send WS_NOAMMO
		}
	}
	len = strlen(string);
	if(len > 0 && string[len - 1] == ' ') {
		string[len - 1] = '\0';
	}

	trap_SendServerCommand(ent - g_entities, va("sweps %s", string));
	ClientUserinfoChanged(ent->s.clientNum);
}

/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f(gentity_t* ent) { DeathmatchScoreboardMessage(ent); }

/*
==================
CheatsOk
==================
*/
bool CheatsOk(gentity_t* ent) {
	if(!g_cheats.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"Cheats are not enabled on this server.\n\"");
		return false;
	}
	if(ent->health <= 0) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be alive to use this command.\n\"");
		return false;
	}
	return true;
}

/*
==================
ConcatArgs
==================
*/
char* ConcatArgs(int start) {
	int i, c, tlen;
	static char line[MAX_STRING_CHARS];
	int len;
	char arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for(i = start; i < c; i++) {
		trap_Argv(i, arg, sizeof(arg));
		tlen = strlen(arg);
		if(len + tlen >= MAX_STRING_CHARS - 1) {
			break;
		}
		memcpy(line + len, arg, tlen);
		len += tlen;
		if(i != c - 1) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
StringIsInteger
==================
*/
bool StringIsInteger(const char* s) {
	int i;
	int len;
	bool foundDigit;

	len = strlen(s);
	foundDigit = false;

	for(i = 0; i < len; i++) {
		if(!isdigit(s[i])) {
			return false;
		}

		foundDigit = true;
	}

	return foundDigit;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString(gentity_t* to, char* s, bool checkNums, bool checkNames) {
	gclient_t* cl;
	int idnum;
	char cleanName[MAX_STRING_CHARS];

	if(checkNums) {
		// numeric values could be slot numbers
		if(StringIsInteger(s)) {
			idnum = atoi(s);
			if(idnum >= 0 && idnum < level.maxclients) {
				cl = &level.clients[idnum];
				if(cl->pers.connected == CON_CONNECTED) {
					return idnum;
				}
			}
		}
	}

	if(checkNames) {
		// check for a name match
		for(idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++) {
			if(cl->pers.connected != CON_CONNECTED) {
				continue;
			}
			Q_strncpyz(cleanName, cl->pers.netname, sizeof(cleanName));
			Q_CleanStr(cleanName);
			if(!Q_stricmp(cleanName, s)) {
				return idnum;
			}
		}
	}

	trap_SendServerCommand(to - g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f(gentity_t* ent) {
	char* name;
	gitem_t* it;
	int i;
	bool give_all;
	gentity_t* it_ent;
	trace_t trace;

	if(!CheatsOk(ent)) {
		return;
	}

	name = ConcatArgs(1);

	if(Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if(give_all || Q_stricmp(name, "weapons") == 0) {
		ent->client->ps.stats[STAT_WEAPONS] = (1 << WEAPONS_NUM) - 1 - (1 << WP_GRAPPLING_HOOK) - (1 << WP_NONE);
		if(!give_all) return;
	}

	// spawn a specific item right on the player
	if(!give_all) {
		it = BG_FindItem(name);
		if(!it) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy(ent->r.currentOrigin, it_ent->s.origin);
		it_ent->classname = it->classname;
		G_SpawnItem(it_ent, it);
		FinishSpawningItem(it_ent);
		memset(&trace, 0, sizeof(trace));
		Touch_Item(it_ent, ent, &trace);
		if(it_ent->inuse) {
			G_FreeEntity(it_ent);
		}
	}
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f(gentity_t* ent) {
	char* msg;

	if(!CheatsOk(ent)) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if(!(ent->flags & FL_GODMODE))
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f(gentity_t* ent) {
	char* msg;

	if(!CheatsOk(ent)) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if(!(ent->flags & FL_NOTARGET))
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f(gentity_t* ent) {
	char* msg;

	if(!CheatsOk(ent)) {
		return;
	}

	if(ent->client->noclip) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

/*
==================
Cmd_TeamTask_f
==================
*/
void Cmd_TeamTask_f(gentity_t* ent) {
	char userinfo[MAX_INFO_STRING];
	char arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if(trap_Argc() != 2) {
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	task = atoi(arg);

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f(gentity_t* ent) {
	if(ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		return;
	}
	if(ent->health <= 0) {
		return;
	}
	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die(ent, ent, ent, 100000, MOD_SUICIDE);
}

/*
==================
G_Say
==================
*/

static void G_SayTo(gentity_t* ent, gentity_t* other, char* color, const char* name, const char* message) {
	if(!other) {
		return;
	}
	if(!other->inuse) {
		return;
	}
	if(!other->client) {
		return;
	}
	if(other->client->pers.connected != CON_CONNECTED) {
		return;
	}

	trap_SendServerCommand(other - g_entities, va("%s \"%s%c%s%s\"", "chat", name, Q_COLOR_ESCAPE, color, message));
}

#define EC "\x19"

void G_Say(gentity_t* ent, gentity_t* target, const char* chatText) {
	int j;
	gentity_t* other;
	char* color;
	char name[64];
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT];
	char location[64];

	Com_sprintf(name, sizeof(name), "%s%c%s" EC ": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
	color = COLOR_GREEN;

	Q_strncpyz(text, chatText, sizeof(text));

	if(target) {
		G_SayTo(ent, target, color, name, text);
		return;
	}

	// echo the text to the console
	if(g_dedicated.integer) {
		G_Printf("%s%s\n", name, text);
	}

	// send it to all the appropriate clients
	for(j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo(ent, other, color, name, text);
	}
}

static void SanitizeChatText(char* text) {
	int i;

	for(i = 0; text[i]; i++) {
		if(text[i] == '\n' || text[i] == '\r') {
			text[i] = ' ';
		}
	}
}

/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f(gentity_t* ent, bool arg0) {
	char* p;

	if(trap_Argc() < 2 && !arg0) {
		return;
	}

	if(arg0) {
		p = ConcatArgs(0);
	} else {
		p = ConcatArgs(1);
	}

	SanitizeChatText(p);

	G_Say(ent, NULL, p);
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f(gentity_t* ent) { trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", vtos(ent->r.currentOrigin))); }

/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f(gentity_t* ent) {
	vec3_t origin, angles;
	char buffer[MAX_TOKEN_CHARS];
	int i;

	if(!g_cheats.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"Cheats are not enabled on this server.\n\"");
		return;
	}
	if(trap_Argc() != 5) {
		trap_SendServerCommand(ent - g_entities, "print \"usage: setviewpos x y z yaw\n\"");
		return;
	}

	VectorClear(angles);
	for(i = 0; i < 3; i++) {
		trap_Argv(i + 1, buffer, sizeof(buffer));
		origin[i] = atof(buffer);
	}

	trap_Argv(4, buffer, sizeof(buffer));
	angles[YAW] = atof(buffer);

	TeleportPlayer(ent, origin, angles);
}

/*
=================
ClientCommand
=================
*/
void ClientCommand(int clientNum) {
	gentity_t* ent;
	char cmd[MAX_TOKEN_CHARS];

	ent = g_entities + clientNum;

	trap_Argv(0, cmd, sizeof(cmd));

	if(Q_stricmp(cmd, "say") == 0) {
		Cmd_Say_f(ent, false);
		return;
	}
	if(Q_stricmp(cmd, "score") == 0) {
		Cmd_Score_f(ent);
		return;
	}

	// ignore all other commands when at intermission
	if(level.intermissiontime) {
		Cmd_Say_f(ent, true);
		return;
	}

	if(Q_stricmp(cmd, "give") == 0)
		Cmd_Give_f(ent);
	else if(Q_stricmp(cmd, "god") == 0)
		Cmd_God_f(ent);
	else if(Q_stricmp(cmd, "notarget") == 0)
		Cmd_Notarget_f(ent);
	else if(Q_stricmp(cmd, "noclip") == 0)
		Cmd_Noclip_f(ent);
	else if(Q_stricmp(cmd, "kill") == 0)
		Cmd_Kill_f(ent);
	else if(Q_stricmp(cmd, "where") == 0)
		Cmd_Where_f(ent);
	else if(Q_stricmp(cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f(ent);
	else
		trap_SendServerCommand(clientNum, va("print \"unknown cmd %s\n\"", cmd));
}
