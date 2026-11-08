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

// Weapon properties are registered from JS (js/weapons/default.js) through VM_APIWEAPON
weaponProperties_t jsd_weapon[MAX_WEAPONS];
int jsd_weaponCount = 0;

item_t* BG_FindItemForWeapon(int weapon) {
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
	s->eType = ET_PLAYER;

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
	s->clientNum = ps->clientNum;
	s->eFlags = ps->eFlags;

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
	s->eType = ET_PLAYER;

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
	s->clientNum = ps->clientNum;
	s->eFlags = ps->eFlags;

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
===============
bg_ragdollSimpleBones

Bones of the simplified ragdoll (torso, head, arms, legs). Names are the stock
valveBiped names (Garry's Mod / Half-Life 2). Shared by game and cgame.
===============
*/
const char* const bg_ragdollSimpleBones[RAGDOLL_SIMPLE_NUMBONES] = {
	"ValveBiped.Bip01_Pelvis",      // 0  main bone
	"ValveBiped.Bip01_Spine",       // 1
	"ValveBiped.Bip01_Spine1",      // 2
	"ValveBiped.Bip01_Spine2",      // 3
	"ValveBiped.Bip01_Spine4",      // 4
	"ValveBiped.Bip01_Neck1",       // 5
	"ValveBiped.Bip01_Head1",       // 6
	"ValveBiped.Bip01_L_Clavicle",  // 7
	"ValveBiped.Bip01_L_UpperArm",  // 8
	"ValveBiped.Bip01_L_Forearm",   // 9
	"ValveBiped.Bip01_L_Hand",      // 10
	"ValveBiped.Bip01_R_Clavicle",  // 11
	"ValveBiped.Bip01_R_UpperArm",  // 12
	"ValveBiped.Bip01_R_Forearm",   // 13
	"ValveBiped.Bip01_R_Hand",      // 14
	"ValveBiped.Bip01_L_Thigh",     // 15
	"ValveBiped.Bip01_L_Calf",      // 16
	"ValveBiped.Bip01_L_Foot",      // 17
	"ValveBiped.Bip01_L_Toe0",      // 18
	"ValveBiped.Bip01_R_Thigh",     // 19
	"ValveBiped.Bip01_R_Calf",      // 20
	"ValveBiped.Bip01_R_Foot",      // 21
	"ValveBiped.Bip01_R_Toe0"       // 22
};
