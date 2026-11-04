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
// bg_misc.c -- both games misc functions, all completely stateless

#include "../qcommon/q_shared.h"
#include "bg_public.h"

item_t jsd_item[MAX_ITEMS];
int jsd_itemCount = 0;

// clang-format off
weaponProperties_t jsd_weapon[] = {
// 	wType 				wAnimType		classname 		delay 	count 	dmg 	sDmg 	sRadius range 	speed	sRandom spread	timeout gravity	bounce	bMod 	zOffset
	{ WT_NONE, 			WAT_NONE, 		NULL, 			0, 		0,		0, 		0, 		0,		0,		0, 		0, 		0, 		0, 		false,	false,	0.0, 	0.0 },
	{ WT_MELEE, 		WAT_MELEE, 		NULL, 			400,	1, 		50, 	0, 		0,		64,		0, 		0, 		0, 		0, 		false,	false,	0.0, 	0.0 },
	{ WT_BULLET, 		WAT_NONE, 		NULL, 			100, 	1, 		7, 		0, 		0, 		131072, 0, 		0, 		200, 	0, 		false,	false,	0.0, 	0.0 },
	{ WT_SHOTGUN, 		WAT_NONE, 		NULL, 			1000, 	11, 	10, 	0, 		0,		8192,	0, 		0, 		700, 	0, 		false,	false,	0.0, 	0.0 },
	{ WT_MISSILE, 		WAT_GRENADE, 	"grenade", 		800, 	1, 		100, 	100, 	150,	0,		700, 	0, 		0, 		2500, 	true,	true,	0.65,	0.25 },
	{ WT_MISSILE, 		WAT_NONE, 		"rocket", 		800, 	1, 		100, 	100, 	120,	0,		900, 	0, 		0, 		15000,	false,	false,	0.0, 	0.0 },
	{ WT_BEAM, 			WAT_NONE, 		NULL, 			50, 	1, 		8, 		0, 		0,		768,	0, 		0, 		0, 		0, 		false,	false,	0.0, 	0.0 },
	{ WT_BEAM, 			WAT_NONE, 		NULL, 			1500, 	1, 		100, 	0, 		0,		16384,	0, 		0, 		0, 		0, 		false,	false,	0.0, 	0.0 },
	{ WT_MISSILE, 		WAT_NONE, 		"plasma", 		100, 	1, 		20, 	15, 	20,		0,		2000, 	0, 		0, 		10000, 	false,	false,	0.0, 	0.0 },
	{ WT_MISSILE, 		WAT_NONE, 		"bfg", 			200, 	1, 		100, 	100, 	120,	0,		2000, 	0, 		0, 		10000, 	false,	false,	0.0, 	0.0 },
	{ WT_PHYSGUN, 		WAT_NONE, 		NULL, 			100, 	1, 		0, 		0, 		0,		8192,	0, 		0, 		0, 		0, 		false,	false,	0.0, 	0.0 },
	{ WT_GRAVITYGUN,	WAT_NONE, 		NULL, 			100, 	1, 		0, 		0, 		0,		1024,	0, 		0, 		0, 		0, 		false,	false,	0.0, 	0.0 },
	{ WT_TOOLGUN, 		WAT_NONE, 		NULL, 			200, 	1, 		0, 		0, 		0,		4096,	0, 		0, 		0, 		0, 		false,	false,	0.0, 	0.0 },
};
int	jsd_weaponCount = ARRAY_LEN(jsd_weapon);
// clang-format on

item_t* BG_FindItemForWeapon(weapon_t weapon) {
	for(int i = 0; i < jsd_itemCount; i++) {
		if(jsd_item[i].giType == IT_WEAPON && jsd_item[i].giTag == weapon) return &jsd_item[i];
	}

	Com_Error(ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

item_t* BG_FindItem(const char* pickupName) {
	for(int i = 0; i < jsd_itemCount; i++) {
		if(!Q_stricmp(jsd_item[i].pickup_name, pickupName)) return &jsd_item[i];
	}

	return NULL;
}

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
bool BG_PlayerTouchesItem(playerState_t* ps, entityState_t* item, int atTime) {
	vec3_t origin;

	BG_EvaluateTrajectory(&item->pos, atTime, origin);

	// we are ignoring ducked differences here
	if(ps->origin[0] - origin[0] > 44 || ps->origin[0] - origin[0] < -50 || ps->origin[1] - origin[1] > 36 || ps->origin[1] - origin[1] < -36 || ps->origin[2] - origin[2] > 36 || ps->origin[2] - origin[2] < -36) {
		return false;
	}

	return true;
}

/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
bool BG_CanItemBeGrabbed(const entityState_t* ent, const playerState_t* ps) {
	item_t* item;

	if(ent->modelindex < 1 || ent->modelindex >= jsd_itemCount) Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: index out of range");

	item = &jsd_item[ent->modelindex];

	switch(item->giType) {
		case IT_WEAPON: return true;  // weapons are always picked up
		case IT_BAD: Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD");
		default: Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n", item->giType); break;
	}

	return false;
}

//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory(const trajectory_t* tr, int atTime, vec3_t result) {
	float deltaTime;
	float phase;

	switch(tr->trType) {
		case TR_STATIONARY:
		case TR_INTERPOLATE: VectorCopy(tr->trBase, result); break;
		case TR_LINEAR:
			deltaTime = (atTime - tr->trTime) * 0.001;  // milliseconds to seconds
			VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
			break;
		case TR_SINE:
			deltaTime = (atTime - tr->trTime) / (float)tr->trDuration;
			phase = sin(deltaTime * M_PI * 2);
			VectorMA(tr->trBase, phase, tr->trDelta, result);
			break;
		case TR_LINEAR_STOP:
			if(atTime > tr->trTime + tr->trDuration) {
				atTime = tr->trTime + tr->trDuration;
			}
			deltaTime = (atTime - tr->trTime) * 0.001;  // milliseconds to seconds
			if(deltaTime < 0) {
				deltaTime = 0;
			}
			VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
			break;
		case TR_GRAVITY:
			deltaTime = (atTime - tr->trTime) * 0.001;  // milliseconds to seconds
			VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
			result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;  // FIXME: local gravity...
			break;
		default: Com_Error(ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trType); break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta(const trajectory_t* tr, int atTime, vec3_t result) {
	float deltaTime;
	float phase;

	switch(tr->trType) {
		case TR_STATIONARY:
		case TR_INTERPOLATE: VectorClear(result); break;
		case TR_LINEAR: VectorCopy(tr->trDelta, result); break;
		case TR_SINE:
			deltaTime = (atTime - tr->trTime) / (float)tr->trDuration;
			phase = cos(deltaTime * M_PI * 2);  // derivative of sin = cos
			phase *= 0.5;
			VectorScale(tr->trDelta, phase, result);
			break;
		case TR_LINEAR_STOP:
			if(atTime > tr->trTime + tr->trDuration) {
				VectorClear(result);
				return;
			}
			VectorCopy(tr->trDelta, result);
			break;
		case TR_GRAVITY:
			deltaTime = (atTime - tr->trTime) * 0.001;  // milliseconds to seconds
			VectorCopy(tr->trDelta, result);
			result[2] -= DEFAULT_GRAVITY * deltaTime;  // FIXME: local gravity...
			break;
		default: Com_Error(ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trType); break;
	}
}

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void trap_Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize);

void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t* ps) {
	ps->events[ps->eventSequence & (MAX_PS_EVENTS - 1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS - 1)] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState(playerState_t* ps, entityState_t* s, bool snap) {
	if(ps->stats[STAT_HEALTH] <= GIB_HEALTH) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy(ps->origin, s->pos.trBase);
	if(snap) {
		SnapVector(s->pos.trBase);
	}
	// set the trDelta for flag direction
	VectorCopy(ps->velocity, s->pos.trDelta);

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy(ps->viewangles, s->apos.trBase);
	if(snap) {
		SnapVector(s->apos.trBase);
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;  // ET_PLAYER looks here instead of at number
	                               // so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if(ps->stats[STAT_HEALTH] <= 0) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if(ps->externalEvent) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if(ps->entityEventSequence < ps->eventSequence) {
		int seq;

		if(ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS - 1);
		s->event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[seq];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate(playerState_t* ps, entityState_t* s, int time, bool snap) {
	if(ps->stats[STAT_HEALTH] <= GIB_HEALTH) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy(ps->origin, s->pos.trBase);
	if(snap) {
		SnapVector(s->pos.trBase);
	}
	// set the trDelta for flag direction and linear prediction
	VectorCopy(ps->velocity, s->pos.trDelta);
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 33;  // 1000 / sv_fps (default = 30)

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy(ps->viewangles, s->apos.trBase);
	if(snap) {
		SnapVector(s->apos.trBase);
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;  // ET_PLAYER looks here instead of at number
	                               // so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if(ps->stats[STAT_HEALTH] <= 0) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if(ps->externalEvent) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if(ps->entityEventSequence < ps->eventSequence) {
		int seq;

		if(ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS - 1);
		s->event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[seq];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}
