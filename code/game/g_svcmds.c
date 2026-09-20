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

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"

/*
===================
Svcmd_EntityList_f
===================
*/
void Svcmd_EntityList_f(void) {
	int e;
	gentity_t* check;

	check = g_entities;
	for(e = 0; e < level.num_entities; e++, check++) {
		if(!check->inuse) {
			continue;
		}
		G_Printf("%3i:", e);
		switch(check->s.eType) {
			case ET_GENERAL: G_Printf("ET_GENERAL          "); break;
			case ET_PLAYER: G_Printf("ET_PLAYER           "); break;
			case ET_ITEM: G_Printf("ET_ITEM             "); break;
			case ET_MISSILE: G_Printf("ET_MISSILE          "); break;
			case ET_MOVER: G_Printf("ET_MOVER            "); break;
			case ET_BEAM: G_Printf("ET_BEAM             "); break;
			case ET_PORTAL: G_Printf("ET_PORTAL           "); break;
			case ET_SPEAKER: G_Printf("ET_SPEAKER          "); break;
			case ET_PUSH_TRIGGER: G_Printf("ET_PUSH_TRIGGER     "); break;
			case ET_TELEPORT_TRIGGER: G_Printf("ET_TELEPORT_TRIGGER "); break;
			case ET_INVISIBLE: G_Printf("ET_INVISIBLE        "); break;
			default: G_Printf("%3i                 ", check->s.eType); break;
		}

		if(check->classname) {
			G_Printf("%s", check->classname);
		}
		G_Printf("\n");
	}
}

gclient_t* ClientForString(const char* s) {
	gclient_t* cl;
	int i;
	int idnum;

	// numeric values are just slot numbers
	if(s[0] >= '0' && s[0] <= '9') {
		idnum = atoi(s);
		if(idnum < 0 || idnum >= level.maxclients) {
			Com_Printf("Bad client slot: %i\n", idnum);
			return NULL;
		}

		cl = &level.clients[idnum];
		if(cl->pers.connected == CON_DISCONNECTED) {
			G_Printf("Client %i is not connected\n", idnum);
			return NULL;
		}
		return cl;
	}

	// check for a name match
	for(i = 0; i < level.maxclients; i++) {
		cl = &level.clients[i];
		if(cl->pers.connected == CON_DISCONNECTED) {
			continue;
		}
		if(!Q_stricmp(cl->pers.netname, s)) {
			return cl;
		}
	}

	G_Printf("User %s is not on the server\n", s);

	return NULL;
}

char* ConcatArgs(int start);

/*
=================
ConsoleCommand

=================
*/
bool ConsoleCommand(void) {
	char cmd[MAX_TOKEN_CHARS];

	trap_Argv(0, cmd, sizeof(cmd));

	if(Q_stricmp(cmd, "entitylist") == 0) {  // Перенести на JS
		Svcmd_EntityList_f();
		return true;
	}

	if(Q_stricmp(cmd, "addbot") == 0) {
		Svcmd_AddBot_f();
		return true;
	}

	if(g_dedicated.integer) {
		if(Q_stricmp(cmd, "say") == 0) {
			trap_SendServerCommand(-1, va("print \"server: %s\n\"", ConcatArgs(1)));
			return true;
		}
		// everything else will also be printed as a say command
		trap_SendServerCommand(-1, va("print \"server: %s\n\"", ConcatArgs(0)));
		return true;
	}

	return false;
}
