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

#include "g_local.h"

void SP_info_camp(gentity_t* self) { G_SetOrigin(self, self->s.origin); }

void SP_info_null(gentity_t* self) { G_FreeEntity(self); }

void SP_info_notnull(gentity_t* self) { G_SetOrigin(self, self->s.origin); }

void SP_light(gentity_t* self) { G_FreeEntity(self); }

void TeleportPlayer(gentity_t* player, vec3_t origin, vec3_t angles) {
	gentity_t* tent;
	bool noAngles;

	noAngles = (angles[0] > 999999.0);
	// use temp events at source and destination to prevent the effect
	// from getting dropped by a second player event
	if(player->client->sess.sessionTeam != TEAM_SPECTATOR) {
		tent = G_TempEntity(player->client->ps.origin, EV_PLAYER_TELEPORT_OUT);
		tent->s.clientNum = player->s.clientNum;

		tent = G_TempEntity(origin, EV_PLAYER_TELEPORT_IN);
		tent->s.clientNum = player->s.clientNum;
	}

	// unlink to make sure it can't possibly interfere with G_KillBox
	trap_UnlinkEntity(player);

	VectorCopy(origin, player->client->ps.origin);
	player->client->ps.origin[2] += 1;
	if(!noAngles) {
		// spit the player out
		AngleVectors(angles, player->client->ps.velocity, NULL, NULL);
		VectorScale(player->client->ps.velocity, 400, player->client->ps.velocity);
		player->client->ps.pm_time = 160;  // hold time
		player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		// set angles
		SetClientViewAngle(player, angles);
	}
	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;
	// kill anything at the destination
	if(player->client->sess.sessionTeam != TEAM_SPECTATOR) {
		G_KillBox(player);
	}

	// save results of pmove
	BG_PlayerStateToEntityState(&player->client->ps, &player->s, true);

	// use the precise origin for linking
	VectorCopy(player->client->ps.origin, player->r.currentOrigin);

	if(player->client->sess.sessionTeam != TEAM_SPECTATOR) {
		trap_LinkEntity(player);
	}
}

void SP_misc_teleporter_dest(gentity_t* ent) {}

void SP_misc_model(gentity_t* ent) { G_FreeEntity(ent); }

void locateCamera(gentity_t* ent) {
	vec3_t dir;
	gentity_t* target;
	gentity_t* owner;

	owner = G_PickTarget(ent->target);
	if(!owner) {
		G_Printf("Couldn't find target for misc_partal_surface\n");
		G_FreeEntity(ent);
		return;
	}
	ent->r.ownerNum = owner->s.number;

	// frame holds the rotate speed
	if(owner->spawnflags & 1) {
		ent->s.frame = 25;
	} else if(owner->spawnflags & 2) {
		ent->s.frame = 75;
	}

	// swing camera ?
	if(owner->spawnflags & 4) {
		// set to 0 for no rotation at all
		ent->s.powerups = 0;
	} else {
		ent->s.powerups = 1;
	}

	// clientNum holds the rotate offset
	ent->s.clientNum = owner->s.clientNum;

	VectorCopy(owner->s.origin, ent->s.origin2);

	// see if the portal_camera has a target
	target = G_PickTarget(owner->target);
	if(target) {
		VectorSubtract(target->s.origin, owner->s.origin, dir);
		VectorNormalize(dir);
	} else {
		G_SetMovedir(owner->s.angles, dir);
	}

	ent->s.eventParm = DirToByte(dir);
}

void SP_misc_portal_surface(gentity_t* ent) {
	VectorClear(ent->r.mins);
	VectorClear(ent->r.maxs);
	trap_LinkEntity(ent);

	ent->r.svFlags = SVF_PORTAL;
	ent->s.eType = ET_PORTAL;

	if(!ent->target) {
		VectorCopy(ent->s.origin, ent->s.origin2);
	} else {
		ent->think = locateCamera;
		ent->nextthink = level.time + 100;
	}
}

void SP_misc_portal_camera(gentity_t* ent) {
	float roll;

	VectorClear(ent->r.mins);
	VectorClear(ent->r.maxs);
	trap_LinkEntity(ent);

	G_SpawnFloat("roll", "0", &roll);

	ent->s.clientNum = roll / 360.0 * 256;
}
