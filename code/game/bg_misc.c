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

#if 0
item_t jsd_item[] = {{NULL,
                          NULL,
                          {NULL, NULL, NULL, NULL},
                          /* icon */ NULL,
                          /* pickup */ NULL,
                          0,
                          0,
                          0,
                          /* precache */ "",
                          /* sounds */ ""},  // leave index 0 alone

                         //
                         // WEAPONS
                         //

                         /*QUAKED weapon_gauntlet (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
                          */
                         {"weapon_gauntlet",
                          "sound/misc/w_pkup.wav",
                          {"models/weapons2/gauntlet/gauntlet.md3", NULL, NULL, NULL},
                          /* icon */ "icons/iconw_gauntlet",
                          /* pickup */ "Gauntlet",
                          0,
                          IT_WEAPON,
                          WP_GAUNTLET,
                          /* precache */ "",
                          /* sounds */ ""},

                         /*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
                          */
                         {"weapon_shotgun",
                          "sound/misc/w_pkup.wav",
                          {"models/weapons2/shotgun/shotgun.md3", NULL, NULL, NULL},
                          /* icon */ "icons/iconw_shotgun",
                          /* pickup */ "Shotgun",
                          10,
                          IT_WEAPON,
                          WP_SHOTGUN,
                          /* precache */ "",
                          /* sounds */ ""},

                         /*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
                          */
                         {"weapon_machinegun",
                          "sound/misc/w_pkup.wav",
                          {"models/weapons2/machinegun/machinegun.md3", NULL, NULL, NULL},
                          /* icon */ "icons/iconw_machinegun",
                          /* pickup */ "Machinegun",
                          40,
                          IT_WEAPON,
                          WP_MACHINEGUN,
                          /* precache */ "",
                          /* sounds */ ""},

                         /*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
                          */
                         {"weapon_grenadelauncher",
                          "sound/misc/w_pkup.wav",
                          {"models/weapons2/grenadel/grenadel.md3", NULL, NULL, NULL},
                          /* icon */ "icons/iconw_grenade",
                          /* pickup */ "Grenade Launcher",
                          10,
                          IT_WEAPON,
                          WP_GRENADE_LAUNCHER,
                          /* precache */ "",
                          /* sounds */ "sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav"},

                         /*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
                          */
                         {"weapon_rocketlauncher",
                          "sound/misc/w_pkup.wav",
                          {"models/weapons2/rocketl/rocketl.md3", NULL, NULL, NULL},
                          /* icon */ "icons/iconw_rocket",
                          /* pickup */ "Rocket Launcher",
                          10,
                          IT_WEAPON,
                          WP_ROCKET_LAUNCHER,
                          /* precache */ "",
                          /* sounds */ ""},

                         /*QUAKED weapon_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
                          */
                         {"weapon_lightning",
                          "sound/misc/w_pkup.wav",
                          {"models/weapons2/lightning/lightning.md3", NULL, NULL, NULL},
                          /* icon */ "icons/iconw_lightning",
                          /* pickup */ "Lightning Gun",
                          100,
                          IT_WEAPON,
                          WP_LIGHTNING,
                          /* precache */ "",
                          /* sounds */ ""},

                         /*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
                          */
                         {"weapon_railgun",
                          "sound/misc/w_pkup.wav",
                          {"models/weapons2/railgun/railgun.md3", NULL, NULL, NULL},
                          /* icon */ "icons/iconw_railgun",
                          /* pickup */ "Railgun",
                          10,
                          IT_WEAPON,
                          WP_RAILGUN,
                          /* precache */ "",
                          /* sounds */ ""},

                         /*QUAKED weapon_plasmagun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
                          */
                         {"weapon_plasmagun",
                          "sound/misc/w_pkup.wav",
                          {"models/weapons2/plasma/plasma.md3", NULL, NULL, NULL},
                          /* icon */ "icons/iconw_plasma",
                          /* pickup */ "Plasma Gun",
                          50,
                          IT_WEAPON,
                          WP_PLASMAGUN,
                          /* precache */ "",
                          /* sounds */ ""},

                         /*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
                          */
                         {"weapon_bfg",
                          "sound/misc/w_pkup.wav",
                          {"models/weapons2/bfg/bfg.md3", NULL, NULL, NULL},
                          /* icon */ "icons/iconw_bfg",
                          /* pickup */ "BFG10K",
                          20,
                          IT_WEAPON,
                          WP_BFG,
                          /* precache */ "",
                          /* sounds */ ""},

                         // end of list marker
                         {NULL}};

int jsd_itemCount = ARRAY_LEN(jsd_item) - 1;
#endif

/*
===============
BG_FindItemForWeapon

===============
*/
item_t* BG_FindItemForWeapon(weapon_t weapon) {
	item_t* it;

	for(it = jsd_item + 1; it->classname; it++) {
		if(it->giType == IT_WEAPON && it->giTag == weapon) {
			return it;
		}
	}

	Com_Error(ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

/*
===============
BG_FindItem

===============
*/
item_t* BG_FindItem(const char* pickupName) {
	item_t* it;

	for(it = jsd_item + 1; it->classname; it++) {
		if(!Q_stricmp(it->pickup_name, pickupName)) return it;
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

	if(ent->modelindex < 1 || ent->modelindex >= jsd_itemCount) {
		Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: index out of range");
	}

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

char* eventnames[] = {
    "EV_NONE",

    "EV_FOOTSTEP",
    "EV_FOOTSTEP_METAL",
    "EV_FOOTSPLASH",
    "EV_FOOTWADE",
    "EV_SWIM",

    "EV_STEP_4",
    "EV_STEP_8",
    "EV_STEP_12",
    "EV_STEP_16",

    "EV_FALL_SHORT",
    "EV_FALL_MEDIUM",
    "EV_FALL_FAR",

    "EV_JUMP_PAD",  // boing sound at origin", jump sound on player

    "EV_JUMP",
    "EV_WATER_TOUCH",  // foot touches
    "EV_WATER_LEAVE",  // foot leaves
    "EV_WATER_UNDER",  // head touches
    "EV_WATER_CLEAR",  // head leaves

    "EV_ITEM_PICKUP",         // normal item pickups are predictable
    "EV_GLOBAL_ITEM_PICKUP",  // powerup / team sounds are broadcast to everyone

    "EV_NOAMMO",
    "EV_CHANGE_WEAPON",
    "EV_FIRE_WEAPON",

    "EV_ITEM_RESPAWN",
    "EV_ITEM_POP",
    "EV_PLAYER_TELEPORT_IN",
    "EV_PLAYER_TELEPORT_OUT",

    "EV_GRENADE_BOUNCE",  // eventParm will be the soundindex

    "EV_GENERAL_SOUND",
    "EV_GLOBAL_SOUND",  // no attenuation
    "EV_GLOBAL_TEAM_SOUND",

    "EV_BULLET_HIT_FLESH",
    "EV_BULLET_HIT_WALL",

    "EV_MISSILE_HIT",
    "EV_MISSILE_MISS",
    "EV_MISSILE_MISS_METAL",
    "EV_RAILTRAIL",
    "EV_SHOTGUN",
    "EV_BULLET",  // otherEntity is the shooter

    "EV_PAIN",
    "EV_DEATH1",
    "EV_DEATH2",
    "EV_DEATH3",
    "EV_OBITUARY",

    "EV_POWERUP_QUAD",
    "EV_POWERUP_BATTLESUIT",
    "EV_POWERUP_REGEN",

    "EV_GIB_PLAYER",  // gib a previously living player
    "EV_SCOREPLUM",   // score plum
};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void trap_Cvar_VariableStringBuffer(const char* var_name, char* buffer, int bufsize);

void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t* ps) {
#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if(atof(buf) != 0) {
#ifdef GAME
			Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#else
			Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS - 1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS - 1)] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_TouchJumpPad
========================
*/
void BG_TouchJumpPad(playerState_t* ps, entityState_t* jumppad) {
	vec3_t angles;
	float p;
	int effectNum;

	if(ps->pm_type != PM_NORMAL) return;

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if(ps->jumppad_ent != jumppad->number) {
		vectoangles(jumppad->origin2, angles);
		p = fabs(AngleNormalize180(angles[PITCH]));
		if(p < 45) {
			effectNum = 0;
		} else {
			effectNum = 1;
		}
		BG_AddPredictableEventToPlayerstate(EV_JUMP_PAD, effectNum, ps);
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy(jumppad->origin2, ps->velocity);
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState(playerState_t* ps, entityState_t* s, bool snap) {
	int i;

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

	s->powerups = 0;
	for(i = 0; i < MAX_POWERUPS; i++) {
		if(ps->powerups[i]) {
			s->powerups |= 1 << i;
		}
	}

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
	int i;

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
	s->pos.trDuration = 50;  // 1000 / sv_fps (default = 20)

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

	s->powerups = 0;
	for(i = 0; i < MAX_POWERUPS; i++) {
		if(ps->powerups[i]) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}
