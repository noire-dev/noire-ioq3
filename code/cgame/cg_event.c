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
// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"

static void CG_Obituary(entityState_t* ent) {
	int mod, target, attacker;
	const char *targetInfo, *attackerInfo;
	char targetName[MAX_NETNAME], attackerName[MAX_NETNAME];

	attacker = ent->otherEntityNum;
	target = ent->otherEntityNum2;
	mod = ent->eventParm;

	if(target < 0 || target >= MAX_CLIENTS) CG_Error("CG_Obituary: target out of range");

	if(attacker < 0 || attacker >= MAX_CLIENTS)
		attackerInfo = NULL;
	else
		attackerInfo = CG_ConfigString(CS_PLAYERS + attacker);

	targetInfo = CG_ConfigString(CS_PLAYERS + target);
	if(!targetInfo) return;
	Q_strncpyz(targetName, Info_ValueForKey(targetInfo, "n"), sizeof(targetName) - 2);
	strcat(targetName, S_COLOR_WHITE);

	if(attackerInfo) {
		Q_strncpyz(attackerName, Info_ValueForKey(attackerInfo, "n"), sizeof(attackerName) - 2);
		strcat(attackerName, S_COLOR_WHITE);
	}

	if(attackerInfo)
		CG_AddNotify(va("%s -> %s", attackerName, targetName), NOTIFY_KILL, mod, "");
	else
		CG_AddNotify(targetName, NOTIFY_KILL, mod, "");
}

static void CG_ItemPickup(int itemNum) {
	CG_AddNotify(jsd_item[itemNum].pickup_name, NOTIFY_ITEM, itemNum, "");
	if(jsd_item[itemNum].giType == IT_WEAPON) cg.swep_listcl[jsd_item[itemNum].giTag] = WS_HAVE;
}

/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
void CG_EntityEvent(centity_t* cent, vec3_t position) {
	entityState_t* es;
	int event;
	vec3_t dir;
	int clientNum;
	clientInfo_t* ci;

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;

	if(!event) return;

	clientNum = es->clientNum;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS) clientNum = 0;
	ci = &cgs.clientinfo[clientNum];

	switch(event) {
		case EV_FOOTSTEP_CHAINLINK:
		case EV_FOOTSTEP_CONCRETE:
		case EV_FOOTSTEP_DIRT:
		case EV_FOOTSTEP_DUCT:
		case EV_FOOTSTEP_GRASS:
		case EV_FOOTSTEP_GRAVEL:
		case EV_FOOTSTEP_LADDER:
		case EV_FOOTSTEP_METAL:
		case EV_FOOTSTEP_METALGRATE:
		case EV_FOOTSTEP_MUD:
		case EV_FOOTSTEP_SAND:
		case EV_FOOTSTEP_SLOSH:
		case EV_FOOTSTEP_TILE:
		case EV_FOOTSTEP_WOOD:
		case EV_FOOTSTEP_WOODPANEL: trap_S_StartSound(NULL, es->number, CHAN_BODY, cgs.media.footsteps[event - EV_FOOTSTEP_CHAINLINK][rand() & 3]); break;
		case EV_FALL: trap_S_StartSound(NULL, es->number, CHAN_VOICE, cgs.media.landSound); break;
		case EV_GRAVITY_LAUNCH: trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.gravityLaunch[rand() & 3]); break;
		case EV_WATER_SPLASH: trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.waterSplash[rand() & 2]); break;
		case EV_ITEM_PICKUP: {
			item_t* item;
			int index;
			index = es->eventParm;
			if(index < 1 || index >= jsd_itemCount) break;
			item = &jsd_item[index];
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, trap_S_RegisterSound(item->pickup_sound, false));
			if(es->number == cg.snap->ps.clientNum) CG_ItemPickup(index);
		} break;
		case EV_FIRE_WEAPON: CG_FireWeapon(cent); break;
		case EV_MISSILE_BOUNCE:
			if(es->weapon > WP_NONE && es->weapon < jsd_weaponCount && jsd_weaponFx[es->weapon].missileBounceSoundHandle) {
				trap_S_StartSound(NULL, es->number, CHAN_AUTO, jsd_weaponFx[es->weapon].missileBounceSoundHandle);
			} else if(rand() & 1) {
				trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.hgrenb1aSound);
			} else {
				trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.hgrenb2aSound);
			}
			break;
		case EV_MISSILE_HIT:
			ByteToDir(es->eventParm, dir);
			CG_MissileHitPlayer(es->weapon, position, dir);
			break;
		case EV_MISSILE_MISS:
			ByteToDir(es->eventParm, dir);
			CG_MissileHitWall(es->weapon, position, dir);
			break;
		case EV_BEAM_TRAIL:
			CG_RailTrail(ci, es->origin2, es->pos.trBase, es->weapon);
			if(es->eventParm != 255) {
				ByteToDir(es->eventParm, dir);
				CG_MissileHitWall(es->weapon, position, dir);
			}
			break;
		case EV_BULLET_HIT_WORLD:
			ByteToDir(es->eventParm, dir);
			CG_Bullet(es->pos.trBase, es->otherEntityNum, dir, false, es->weapon);
			break;
		case EV_BULLET_HIT_ENTITY:
			VectorClear(dir);
			CG_Bullet(es->pos.trBase, es->otherEntityNum, dir, true, es->weapon);
			break;
		case EV_SHOTGUN: CG_ShotgunPattern(es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum, es->weapon); break;
		case EV_GENERAL_SOUND: trap_S_StartSound(NULL, es->number, CHAN_VOICE, cgs.gameSounds[es->eventParm]); break;
		case EV_GLOBAL_SOUND: trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gameSounds[es->eventParm]); break;
		case EV_OBITUARY: CG_Obituary(es); break;
		case EV_PARTICLE: CG_SpawnParticlePreset(es->eventParm, position); break;
		default:  // CG_Error("Unknown event: %i", event); break;
	}
}

/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents(centity_t* cent) {
	// check for event-only entities
	if(cent->currentState.eType > ET_EVENTS) {
		if(cent->previousEvent) {
			return;  // already fired
		}
		// if this is a player event set the entity number of the client entity number
		if(cent->currentState.eFlags & EF_PLAYER_EVENT) {
			cent->currentState.number = cent->currentState.otherEntityNum;
		}

		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	} else {
		// check for events riding with another entity
		if(cent->currentState.event == cent->previousEvent) {
			return;
		}
		cent->previousEvent = cent->currentState.event;
		if((cent->currentState.event & ~EV_EVENT_BITS) == 0) {
			return;
		}
	}

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin);
	CG_SetEntitySoundPosition(cent);

	CG_EntityEvent(cent, cent->lerpOrigin);
}

void CG_SpawnParticlePreset(int presetIdx, const vec3_t origin) {
	const particlePreset_t* p;
	vec4_t color;
	vec3_t gravity;
	qhandle_t shader;

	if(presetIdx < 0 || presetIdx >= PP_COUNT) return;
	p = &particlePresets[presetIdx];

	Vector4Copy(p->color, color);
	VectorCopy(p->gravity, gravity);
	shader = p->shader[0] ? trap_R_RegisterShader(p->shader) : 0;

	CG_AddParticles(origin, color, p->count, p->speed, p->flags, gravity, shader, p->size, p->lifetime);

	if(p->sound[0]) trap_S_StartSound(NULL, 0, CHAN_AUTO, trap_S_RegisterSound(p->sound, false));
}