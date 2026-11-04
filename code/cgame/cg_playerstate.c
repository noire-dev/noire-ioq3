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
// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities

#include "cg_local.h"

/*
================
CG_Respawn

A respawn happened this snapshot
================
*/
void CG_Respawn(void) {
	int i;

	// no error decay on player movement
	cg.thisFrameTeleport = true;

	for(i = 1; i < jsd_weaponCount; i++) cg.swep_listcl[i] = cg.swep_spawncl[i];  // Noire's Mod weapon predict

	// display weapons available
	cg.weaponSelectTime = cg.time;

	// select the weapon the server says we are using
	cg.weaponSelect = cg.snap->ps.weapon;
}

/*
==============
CG_CheckPlayerstateEvents
==============
*/
static void CG_CheckPlayerstateEvents(playerState_t* ps, playerState_t* ops) {
	int i;
	int event;
	centity_t* cent;

	if(ps->externalEvent && ps->externalEvent != ops->externalEvent) {
		cent = &cg_entities[ps->clientNum];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent(cent, cent->lerpOrigin);
	}

	cent = &cg.predictedPlayerEntity;  // cg_entities[ ps->clientNum ];
	// go through the predictable events buffer
	for(i = ps->eventSequence - MAX_PS_EVENTS; i < ps->eventSequence; i++) {
		// if we have a new predictable event
		if(i >= ops->eventSequence
		   // or the server told us to play another event instead of a predicted event we already issued
		   // or something the server told us changed our prediction causing a different event
		   || (i > ops->eventSequence - MAX_PS_EVENTS && ps->events[i & (MAX_PS_EVENTS - 1)] != ops->events[i & (MAX_PS_EVENTS - 1)])) {
			event = ps->events[i & (MAX_PS_EVENTS - 1)];
			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[i & (MAX_PS_EVENTS - 1)];
			CG_EntityEvent(cent, cent->lerpOrigin);

			cg.predictableEvents[i & (MAX_PREDICTED_EVENTS - 1)] = event;

			cg.eventSequence++;
		}
	}
}

/*
==================
CG_CheckLocalSounds
==================
*/
static void CG_CheckLocalSounds(playerState_t* ps, playerState_t* ops) {
	int highScore;

	// hit changes
	if(ps->persistant[PERS_HITS] > ops->persistant[PERS_HITS]) {
		trap_S_StartLocalSound(cgs.media.hitSound, CHAN_LOCAL_SOUND);
	}
}

/*
===============
CG_TransitionPlayerState

===============
*/
void CG_TransitionPlayerState(playerState_t* ps, playerState_t* ops) {
	// check for changing follow mode
	if(ps->clientNum != ops->clientNum) {
		cg.thisFrameTeleport = true;
		// make sure we don't get any unwanted transition effects
		*ops = *ps;
	}

	// respawning
	if(ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT]) {
		CG_Respawn();
	}

	if(cg.mapRestart) {
		CG_Respawn();
		cg.mapRestart = false;
	}

	CG_CheckLocalSounds(ps, ops);

	// run events
	CG_CheckPlayerstateEvents(ps, ops);
}
