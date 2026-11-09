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
#include "../qcommon/vm_javascript.h"

/*
==================
ScoreboardMessage
==================
*/
void ScoreboardMessage(gentity_t* ent) {
	char entry[1024];
	char string[1000];
	int stringlength = 0, clientCount = 0, ping = 0;
	int i, j;

	if(ent->r.svFlags & SVF_BOT) return;

	string[0] = 0;

	for(i = 0; i < MAX_CLIENTS; i++) {
		gclient_t* cl = &level.clients[i];

		if(cl->pers.connected == CON_DISCONNECTED)
			continue;
		else if(cl->pers.connected == CON_CONNECTING)
			ping = -1;
		else
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		Com_sprintf(entry, sizeof(entry), " %i %i", cl->ps.clientNum, ping);
		j = strlen(entry);
		if(stringlength + j >= sizeof(string)) break;
		strcpy(string + stringlength, entry);
		stringlength += j;
		clientCount++;
	}

	trap_SendServerCommand(ent - g_entities, va("scores %i %s", clientCount, string));
}

void G_SendSwepWeapons(gentity_t* ent) {
	char string[4096] = "";
	int i;
	int len;

	for(i = 1; i < jsd_weaponCount; i++) {
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

	for(i = 1; i < jsd_weaponCount; i++) {
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
Cmd_Scoreboard_f

Request current scoreboard information
==================
*/
void Cmd_Scoreboard_f(gentity_t* ent) { ScoreboardMessage(ent); }

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
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f(gentity_t* ent) {
	char* name;
	item_t* it;
	int i;
	bool give_all;
	gentity_t* it_ent;
	trace_t trace;

	if(!CheatsOk(ent)) return;

	name = ConcatArgs(1);

	if(Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if(give_all || Q_stricmp(name, "weapons") == 0) {
		for(i = 1; i < jsd_weaponCount; i++) {
			ent->swep_list[i] = WS_HAVE;
			ent->swep_ammo[i] = 9999;
		}
		if(!give_all) return;
	}

	// spawn a specific item right on the player
	if(!give_all) {
		it = BG_FindItem(name);
		if(!it) return;

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
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f(gentity_t* ent) {
	if(ent->health <= 0) return;
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

static void Cmd_Altfire_Physgun_f(gentity_t* ent) {
	if(jsd_weapon[ent->client->ps.weapon].wType == WT_PHYSGUN) {
		if(ent->client->buttons & BUTTON_ATTACK) {
			if(ent->grabbedEntity) {
				ent->grabbedEntity->grabPhysicsActive = false;  // say physgun about freeze option
			}
		}
	}
}

static void Cmd_PhysgunDist_f(gentity_t* ent) {
	char mode[MAX_TOKEN_CHARS];

	trap_Argv(1, mode, sizeof(mode));

	if(jsd_weapon[ent->client->ps.weapon].wType == WT_PHYSGUN) {
		if(ent->client->buttons & BUTTON_ATTACK) {
			if(ent->grabbedEntity) {
				if(atoi(mode) == 0) {
					ent->grabDist -= 20;
					if(ent->grabDist < 100) ent->grabDist = 100;
				}
				if(atoi(mode) == 1) ent->grabDist += 20;
			}
		}
	}
}

/*
=================
Cmd_Gesture_f

Play one of the full-body IQM gestures requested by the player:
  gesture agree|disagree|becon|bow|salute|wave|cheer|dance|laugh|muscle|persistence|robot|zombie
=================
*/
static void Cmd_Gesture_f(gentity_t* ent) {
	static const struct {
		const char* name;
		int animation;
	} gestureAnimations[] = {
	    {"agree", AM_GESTURE_AGREE},
	    {"disagree", AM_GESTURE_DISAGREE},
	    {"becon", AM_GESTURE_BECON},
	    {"beckon", AM_GESTURE_BECON},
	    {"bow", AM_GESTURE_BOW},
	    {"salute", AM_GESTURE_SALUTE},
	    {"wave", AM_GESTURE_WAVE},
	    {"cheer", AM_TAUNT_CHEER},
	    {"dance", AM_TAUNT_DANCE},
	    {"laugh", AM_TAUNT_LAUGH},
	    {"muscle", AM_TAUNT_MUSCLE},
	    {"persistence", AM_TAUNT_PERSISTENCE},
	    {"robot", AM_TAUNT_ROBOT},
	    {"zombie", AM_TAUNT_ZOMBIE},
	};
	char name[MAX_TOKEN_CHARS];
	int i, animation;

	if(!ent || !ent->client || ent->health <= 0 || ent->client->ps.pm_type >= PM_DEAD) return;

	trap_Argv(1, name, sizeof(name));
	animation = -1;
	for(i = 0; i < ARRAY_LEN(gestureAnimations); i++) {
		if(!Q_stricmp(name, gestureAnimations[i].name)) {
			animation = gestureAnimations[i].animation;
			break;
		}
	}

	if(animation < 0) {
		trap_SendServerCommand(ent - g_entities, "print \"gesture: cheer dance laugh muscle persistence robot zombie agree disagree becon bow salute wave\n\"");
		return;
	}

	ent->client->ps.torsoAnim = ((ent->client->ps.torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | animation;
	ent->client->ps.torsoTimer = 3000;
	ent->client->ps.legsTimer = 0;
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
		Cmd_Scoreboard_f(ent);
		return;
	}
	if(Q_stricmp(cmd, "gesture") == 0) {
		Cmd_Gesture_f(ent);
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
	else if(Q_stricmp(cmd, "altfire_physgun") == 0)
		Cmd_Altfire_Physgun_f(ent);
	else if(Q_stricmp(cmd, "physgun_dist") == 0)
		Cmd_PhysgunDist_f(ent);
	else if(Q_stricmp(cmd, "ragdoll") == 0)
		Cmd_Ragdoll_f(ent);
	else if(Q_stricmp(cmd, "ragdoll_clear") == 0)
		Cmd_RagdollClear_f(ent);
	else if(JS_CommandRun(clientNum) != -1)
		return;
	else
		trap_SendServerCommand(clientNum, va("print \"unknown cmd %s\n\"", cmd));
}
