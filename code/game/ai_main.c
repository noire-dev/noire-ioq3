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

/*****************************************************************************
 * name:		ai_main.c
 *
 * desc:		bot AI
 *
 *****************************************************************************/

#include "g_local.h"
#include "../qcommon/q_shared.h"
#include "../botlib/botlib.h"  //bot lib interface
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_dmnet.h"
//

// bot states
bot_state_t* botstates[MAX_CLIENTS];
// number of bots
int numbots;
// floating point time
float floattime;

vmCvar_t bot_thinktime;
vmCvar_t bot_memorydump;
vmCvar_t bot_saveroutingcache;
vmCvar_t bot_pause;
vmCvar_t bot_testsolid;
vmCvar_t bot_testclusters;
vmCvar_t bot_developer;

/*
==================
BotAI_Print
==================
*/
void QDECL BotAI_Print(int type, char* fmt, ...) {
	char str[2048];
	va_list ap;

	va_start(ap, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);

	switch(type) {
		case PRT_MESSAGE: {
			G_Printf("%s", str);
			break;
		}
		case PRT_WARNING: {
			G_Printf(S_COLOR_YELLOW "Warning: %s", str);
			break;
		}
		case PRT_ERROR: {
			G_Printf(S_COLOR_RED "Error: %s", str);
			break;
		}
		case PRT_FATAL: {
			G_Printf(S_COLOR_RED "Fatal: %s", str);
			break;
		}
		case PRT_EXIT: {
			G_Error(S_COLOR_RED "Exit: %s", str);
			break;
		}
		default: {
			G_Printf("unknown print type\n");
			break;
		}
	}
}

/*
==================
BotSkill_BFloat

Replaces the character file characteristics: every skill parameter is
linearly interpolated from worst (skill 1) to best (skill 5).
==================
*/
float BotSkill_BFloat(bot_state_t* bs, float min, float max) {
	float frac = (bs->settings.skill - 1) / 4.0f;

	if(frac < 0) frac = 0;
	if(frac > 1) frac = 1;
	return min + (max - min) * frac;
}

/*
==================
BotAI_Trace
==================
*/
void BotAI_Trace(bsp_trace_t* bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask) {
	trace_t trace;

	trap_Trace(&trace, start, mins, maxs, end, passent, contentmask);
	// copy the trace information
	bsptrace->allsolid = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy(trace.endpos, bsptrace->endpos);
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, bsptrace->plane.normal);
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = 0;
	bsptrace->surface.flags = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}

/*
==================
BotAI_GetClientState
==================
*/
int BotAI_GetClientState(int clientNum, playerState_t* state) {
	gentity_t* ent;

	ent = &g_entities[clientNum];
	if(!ent->inuse) {
		return false;
	}
	if(!ent->client) {
		return false;
	}

	memcpy(state, &ent->client->ps, sizeof(playerState_t));
	return true;
}

/*
==================
BotAI_GetEntityState
==================
*/
int BotAI_GetEntityState(int entityNum, entityState_t* state) {
	gentity_t* ent;

	ent = &g_entities[entityNum];
	memset(state, 0, sizeof(entityState_t));
	if(!ent->inuse) return false;
	if(!ent->r.linked) return false;
	if(ent->r.svFlags & SVF_NOCLIENT) return false;
	memcpy(state, &ent->s, sizeof(entityState_t));
	return true;
}

/*
==============
BotEntityInfo
==============
*/
void BotEntityInfo(int entnum, aas_entityinfo_t* info) { trap_AAS_EntityInfo(entnum, info); }

/*
==============
NumBots
==============
*/
int NumBots(void) { return numbots; }

/*
==============
AngleDifference
==============
*/
float AngleDifference(float ang1, float ang2) {
	float diff;

	diff = ang1 - ang2;
	if(ang1 > ang2) {
		if(diff > 180.0) diff -= 360.0;
	} else {
		if(diff < -180.0) diff += 360.0;
	}
	return diff;
}

/*
==============
BotChangeViewAngle
==============
*/
float BotChangeViewAngle(float angle, float ideal_angle, float speed) {
	float move;

	angle = AngleMod(angle);
	ideal_angle = AngleMod(ideal_angle);
	if(angle == ideal_angle) return angle;
	move = ideal_angle - angle;
	if(ideal_angle > angle) {
		if(move > 180.0) move -= 360.0;
	} else {
		if(move < -180.0) move += 360.0;
	}
	if(move > 0) {
		if(move > speed) move = speed;
	} else {
		if(move < -speed) move = -speed;
	}
	return AngleMod(angle + move);
}

/*
==============
BotChangeViewAngles
==============
*/
void BotChangeViewAngles(bot_state_t* bs, float thinktime) {
	float diff, factor, maxchange, anglespeed, disired_speed;
	int i;

	if(bs->ideal_viewangles[PITCH] > 180) bs->ideal_viewangles[PITCH] -= 360;
	//
	if(bs->enemy >= 0) {
		factor = BotSkill_BFloat(bs, 0.01f, 1);
		maxchange = BotSkill_BFloat(bs, 1, 1800);
	} else {
		factor = 0.05f;
		maxchange = 360;
	}
	if(maxchange < 240) maxchange = 240;
	maxchange *= thinktime;
	for(i = 0; i < 2; i++) {
		//
		if(bot_challenge.integer) {
			// smooth slowdown view model
			diff = fabs(AngleDifference(bs->viewangles[i], bs->ideal_viewangles[i]));
			anglespeed = diff * factor;
			if(anglespeed > maxchange) anglespeed = maxchange;
			bs->viewangles[i] = BotChangeViewAngle(bs->viewangles[i], bs->ideal_viewangles[i], anglespeed);
		} else {
			// over reaction view model
			bs->viewangles[i] = AngleMod(bs->viewangles[i]);
			bs->ideal_viewangles[i] = AngleMod(bs->ideal_viewangles[i]);
			diff = AngleDifference(bs->viewangles[i], bs->ideal_viewangles[i]);
			disired_speed = diff * factor;
			bs->viewanglespeed[i] += (bs->viewanglespeed[i] - disired_speed);
			if(bs->viewanglespeed[i] > 180) bs->viewanglespeed[i] = maxchange;
			if(bs->viewanglespeed[i] < -180) bs->viewanglespeed[i] = -maxchange;
			anglespeed = bs->viewanglespeed[i];
			if(anglespeed > maxchange) anglespeed = maxchange;
			if(anglespeed < -maxchange) anglespeed = -maxchange;
			bs->viewangles[i] += anglespeed;
			bs->viewangles[i] = AngleMod(bs->viewangles[i]);
			// demping
			bs->viewanglespeed[i] *= 0.45 * (1 - factor);
		}
	}
	if(bs->viewangles[PITCH] > 180) bs->viewangles[PITCH] -= 360;
	// elementary action: view
	trap_EA_View(bs->client, bs->viewangles);
}

/*
==============
BotInputToUserCommand
==============
*/
void BotInputToUserCommand(bot_input_t* bi, usercmd_t* ucmd, int delta_angles[2], int time) {
	vec3_t angles, forward, right;
	short temp;
	int j;
	float f, r, u, m;

	// clear the whole structure
	memset(ucmd, 0, sizeof(usercmd_t));
	// the duration for the user command in milli seconds
	ucmd->serverTime = time;
	//
	if(bi->actionflags & ACTION_DELAYEDJUMP) {
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}
	// set the buttons
	if(bi->actionflags & ACTION_RESPAWN) ucmd->buttons = BUTTON_ATTACK;
	if(bi->actionflags & ACTION_ATTACK) ucmd->buttons |= BUTTON_ATTACK;
	//
	ucmd->weapon = bi->weapon;
	// set the view angles
	// NOTE: the ucmd->angles are the angles WITHOUT the delta angles
	ucmd->angles[PITCH] = ANGLE2SHORT(bi->viewangles[PITCH]);
	ucmd->angles[YAW] = ANGLE2SHORT(bi->viewangles[YAW]);
	ucmd->angles[ROLL] = ANGLE2SHORT(bi->viewangles[ROLL]);
	// subtract the delta angles
	for(j = 0; j < 2; j++) {
		temp = ucmd->angles[j] - delta_angles[j];
		ucmd->angles[j] = temp;
	}
	// NOTE: movement is relative to the REAL view angles
	// get the horizontal forward and right vector
	// get the pitch in the range [-180, 180]
	if(bi->dir[2])
		angles[PITCH] = bi->viewangles[PITCH];
	else
		angles[PITCH] = 0;
	angles[YAW] = bi->viewangles[YAW];
	angles[ROLL] = 0;
	AngleVectors(angles, forward, right, NULL);
	// bot input speed is in the range [0, 400]
	bi->speed = bi->speed * 127 / 400;
	// set the view independent movement
	f = DotProduct(forward, bi->dir);
	r = DotProduct(right, bi->dir);
	u = fabs(forward[2]) * bi->dir[2];
	m = fabs(f);

	if(fabs(r) > m) {
		m = fabs(r);
	}

	if(fabs(u) > m) {
		m = fabs(u);
	}

	if(m > 0) {
		f *= bi->speed / m;
		r *= bi->speed / m;
		u *= bi->speed / m;
	}

	ucmd->forwardmove = f;
	ucmd->rightmove = r;
	ucmd->upmove = u;

	if(bi->actionflags & ACTION_MOVEFORWARD) ucmd->forwardmove = 127;
	if(bi->actionflags & ACTION_MOVEBACK) ucmd->forwardmove = -127;
	if(bi->actionflags & ACTION_MOVELEFT) ucmd->rightmove = -127;
	if(bi->actionflags & ACTION_MOVERIGHT) ucmd->rightmove = 127;
	// jump/moveup
	if(bi->actionflags & ACTION_JUMP) ucmd->upmove = 127;
	// crouch/movedown
	if(bi->actionflags & ACTION_CROUCH) ucmd->upmove = -127;
}

/*
==============
BotUpdateInput
==============
*/
void BotUpdateInput(bot_state_t* bs, int time, int elapsed_time) {
	bot_input_t bi;
	int j;

	// add the delta angles to the bot's current view angles
	for(j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	// change the bot view angles
	BotChangeViewAngles(bs, (float)elapsed_time / 1000);
	// retrieve the bot input
	trap_EA_GetInput(bs->client, (float)time / 1000, &bi);
	// respawn hack
	if(bi.actionflags & ACTION_RESPAWN) {
		if(bs->lastucmd.buttons & BUTTON_ATTACK) bi.actionflags &= ~(ACTION_RESPAWN | ACTION_ATTACK);
	}
	// convert the bot input to a usercmd
	BotInputToUserCommand(&bi, &bs->lastucmd, bs->cur_ps.delta_angles, time);
	// subtract the delta angles
	for(j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
}

/*
==============
BotAI
==============
*/
int BotAI(int client, float thinktime) {
	bot_state_t* bs;
	char buf[128];
	int j;

	trap_EA_ResetInput(client);
	//
	bs = botstates[client];
	if(!bs || !bs->inuse) {
		BotAI_Print(PRT_FATAL, "BotAI: client %d is not setup\n", client);
		return false;
	}

	// retrieve any waiting server commands and ignore them: bots don't
	// process console commands, but the reliable command queue must be
	// drained every frame or the server drops the bot with an overflow
	while(trap_BotGetServerCommand(client, buf, sizeof(buf))) {
		// ignore
	}

	// retrieve the current client state
	if(!BotAI_GetClientState(client, &bs->cur_ps)) {
		BotAI_Print(PRT_FATAL, "BotAI: failed to get player state for player %d\n", client);
		return false;
	}
	// add the delta angles to the bot's current view angles
	for(j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	// increase the local time of the bot
	bs->ltime += thinktime;
	//
	bs->thinktime = thinktime;
	// origin of the bot
	VectorCopy(bs->cur_ps.origin, bs->origin);
	// eye coordinates of the bot
	VectorCopy(bs->cur_ps.origin, bs->eye);
	bs->eye[2] += bs->cur_ps.viewheight;
	// get the area the bot is in
	bs->areanum = BotPointAreaNum(bs->origin);
	// the real AI
	BotDeathmatchAI(bs, thinktime);
	// set the weapon selection every AI frame
	trap_EA_SelectWeapon(bs->client, bs->weaponnum);
	// subtract the delta angles
	for(j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	// everything was ok
	return true;
}

/*
==================
BotScheduleBotThink
==================
*/
void BotScheduleBotThink(void) {
	int i, botnum;

	botnum = 0;

	for(i = 0; i < MAX_CLIENTS; i++) {
		if(!botstates[i] || !botstates[i]->inuse) {
			continue;
		}
		// initialize the bot think residual time
		botstates[i]->botthink_residual = bot_thinktime.integer * botnum / numbots;
		botnum++;
	}
}

/*
==============
BotAISetupClient
==============
*/
int BotAISetupClient(int client, struct bot_settings_s* settings, bool restart) {
	bot_state_t* bs;

	if(!botstates[client]) botstates[client] = G_Alloc(sizeof(bot_state_t));
	bs = botstates[client];

	if(!bs) {
		return false;
	}

	if(bs->inuse) {
		BotAI_Print(PRT_FATAL, "BotAISetupClient: client %d already setup\n", client);
		return false;
	}

	if(!trap_AAS_Initialized()) {
		BotAI_Print(PRT_FATAL, "AAS not initialized\n");
		return false;
	}

	// load the bot character
	// copy the settings
	memcpy(&bs->settings, settings, sizeof(bot_settings_t));

	bs->inuse = true;
	bs->client = client;
	bs->entitynum = client;
	bs->setupcount = 4;
	bs->entergame_time = FloatTime();
	bs->ms = trap_BotAllocMoveState();
	bs->walker = BotSkill_BFloat(bs, 1, 0);
	numbots++;

	// NOTE: reschedule the bot thinking
	BotScheduleBotThink();
	// bot has been setup successfully
	return true;
}

/*
==============
BotAIShutdownClient
==============
*/
int BotAIShutdownClient(int client, bool restart) {
	bot_state_t* bs;

	bs = botstates[client];
	if(!bs || !bs->inuse) {
		return false;
	}

	trap_BotFreeMoveState(bs->ms);
	// free the bot character
	// clear the bot state
	memset(bs, 0, sizeof(bot_state_t));
	// set the inuse flag to false
	bs->inuse = false;
	// there's one bot less
	numbots--;
	// everything went ok
	return true;
}

void BotResetState(bot_state_t* bs) {
	int client, entitynum, inuse;
	int movestate;
	bot_settings_t settings;
	playerState_t ps;  // current player state
	float entergame_time;
	float hunt_expire_time;
	int hunt_enemy;
	int sound_id;

	// save some things that should not be reset here
	memcpy(&settings, &bs->settings, sizeof(bot_settings_t));
	memcpy(&ps, &bs->cur_ps, sizeof(playerState_t));
	inuse = bs->inuse;
	client = bs->client;
	entitynum = bs->entitynum;
	movestate = bs->ms;
	entergame_time = bs->entergame_time;
	hunt_expire_time = bs->hunt_expire_time;
	hunt_enemy = bs->hunt_enemy;
	sound_id = bs->sound_id;
	// reset the whole state
	memset(bs, 0, sizeof(bot_state_t));
	// copy back some state stuff that should not be reset
	bs->ms = movestate;
	memcpy(&bs->cur_ps, &ps, sizeof(playerState_t));
	memcpy(&bs->settings, &settings, sizeof(bot_settings_t));
	bs->inuse = inuse;
	bs->client = client;
	bs->entitynum = entitynum;
	bs->entergame_time = entergame_time;
	bs->hunt_expire_time = hunt_expire_time;
	bs->hunt_enemy = hunt_enemy;
	bs->sound_id = sound_id;
	// reset several states
	if(bs->ms) trap_BotResetMoveState(bs->ms);
}

// one snapshot per second per player, 180 seconds of memory
#define BOT_TRAIL_LENGTH 180
typedef struct {
	int time;       // level time of the snapshot
	vec3_t origin;  // where the player was
	int areanum;    // AAS area of the position
} bot_trail_snapshot_t;
static bot_trail_snapshot_t bot_playertrails[MAX_CLIENTS][BOT_TRAIL_LENGTH];
static int bot_trailhead[MAX_CLIENTS];
static int bot_traillasttime[MAX_CLIENTS];

// player sounds: footsteps, falls and gunshots
#define BOT_MAX_SOUNDS 64
typedef struct {
	int client;
	vec3_t origin;
	int areanum;
} bot_framesound_t;
static bot_framesound_t bot_framesounds[BOT_MAX_SOUNDS];
static int bot_soundtotal;
static int bot_lastsoundevent[MAX_CLIENTS];
static int bot_lastfiringsound[MAX_CLIENTS];

/*
==================
BotIsSoundEvent

true for events a bot can hear: footsteps on any surface, falling,
weapon fire and water splashes
==================
*/
static int BotIsSoundEvent(int event) {
	switch(event) {
		case EV_FALL:
		case EV_WATER_SPLASH:
		case EV_FIRE_WEAPON: return true;
		default:
			// the footstep events are a contiguous block in the event enum
			return (event >= EV_FOOTSTEP_CHAINLINK && event <= EV_FOOTSTEP_WOODPANEL);
	}
}

/*
==================
BotTrailPoint

Returns the area of the position a player was at 'delay' milliseconds
ago and writes the position, 0 if nothing is known about the player.
==================
*/
int BotTrailPoint(int client, int delay, vec3_t origin) {
	int i, idx, oldest = -1, want;
	bot_trail_snapshot_t* snap;

	want = level.time - delay;
	for(i = 1; i <= BOT_TRAIL_LENGTH; i++) {
		idx = (bot_trailhead[client] - i + BOT_TRAIL_LENGTH) % BOT_TRAIL_LENGTH;
		snap = &bot_playertrails[client][idx];
		// ran past the oldest recorded point
		if(!snap->time) break;
		// stale point from before a map restart or a reused client slot
		if(snap->time < level.time - (BOT_TRAIL_LENGTH + 2) * 1000) break;
		oldest = idx;
		// the newest snapshot at or before the wanted moment
		if(snap->time <= want) break;
	}
	if(oldest < 0) return 0;
	VectorCopy(bot_playertrails[client][oldest].origin, origin);
	return bot_playertrails[client][oldest].areanum;
}

/*
==================
BotTotalSounds / BotGetSound

Access to the player sounds recorded so far, indexed by an absolute
id that keeps increasing.
==================
*/
int BotTotalSounds(void) { return bot_soundtotal; }

void BotGetSound(int id, int* client, vec3_t origin, int* areanum) {
	bot_framesound_t* snd = &bot_framesounds[id % BOT_MAX_SOUNDS];

	*client = snd->client;
	VectorCopy(snd->origin, origin);
	*areanum = snd->areanum;
}

/*
==================
BotUpdatePlayerTracking

Records the position of every player once per second (the trail bots
follow when hunting) and collects the sounds players make this frame.
==================
*/
static void BotUpdatePlayerTracking(void) {
	int i, raw, ev;
	gentity_t* ent;
	playerState_t* ps;

	for(i = 0; i < MAX_CLIENTS; i++) {
		ent = &g_entities[i];
		if(!ent->inuse || !ent->client || ent->client->pers.connected != CON_CONNECTED) continue;
		ps = &ent->client->ps;
		// trail snapshot once per second, alive players only
		if(level.time - bot_traillasttime[i] >= 1000) {
			bot_traillasttime[i] = level.time;
			if(ps->pm_type == PM_NORMAL) {
				bot_playertrails[i][bot_trailhead[i]].time = level.time;
				VectorCopy(ps->origin, bot_playertrails[i][bot_trailhead[i]].origin);
				bot_playertrails[i][bot_trailhead[i]].areanum = BotPointAreaNum(ps->origin);
				bot_trailhead[i] = (bot_trailhead[i] + 1) % BOT_TRAIL_LENGTH;
			}
		}
		// a new event on the entity: check if the bots can hear it
		raw = ent->s.event;
		if(raw != bot_lastsoundevent[i]) {
			bot_lastsoundevent[i] = raw;
			ev = raw & ~EV_EVENT_BITS;
			if(BotIsSoundEvent(ev)) {
				bot_framesounds[bot_soundtotal % BOT_MAX_SOUNDS].client = i;
				VectorCopy(ps->origin, bot_framesounds[bot_soundtotal % BOT_MAX_SOUNDS].origin);
				bot_framesounds[bot_soundtotal % BOT_MAX_SOUNDS].areanum = BotPointAreaNum(ps->origin);
				bot_soundtotal++;
			}
		}
		// continuous fire (beam like weapons) is heard as a gunshot too
		if((ent->s.eFlags & EF_FIRING) && level.time - bot_lastfiringsound[i] >= 500) {
			bot_lastfiringsound[i] = level.time;
			bot_framesounds[bot_soundtotal % BOT_MAX_SOUNDS].client = i;
			VectorCopy(ps->origin, bot_framesounds[bot_soundtotal % BOT_MAX_SOUNDS].origin);
			bot_framesounds[bot_soundtotal % BOT_MAX_SOUNDS].areanum = BotPointAreaNum(ps->origin);
			bot_soundtotal++;
		}
	}
}

/*
==============
BotAILoadMap
==============
*/
int BotAILoadMap(int restart) {
	int i;
	vmCvar_t mapname;

	if(!restart) {
		trap_Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);
		trap_BotLibLoadMap(mapname.string);
	}

	for(i = 0; i < MAX_CLIENTS; i++) {
		if(botstates[i] && botstates[i]->inuse) {
			BotResetState(botstates[i]);
			botstates[i]->setupcount = 4;
			botstates[i]->enemy = -1;
			botstates[i]->hunt_expire_time = 0;
		}
		bot_trailhead[i] = 0;
		bot_traillasttime[i] = 0;
		bot_lastsoundevent[i] = 0;
		bot_lastfiringsound[i] = 0;
	}
	memset(bot_playertrails, 0, sizeof(bot_playertrails));
	bot_soundtotal = 0;

	BotSetupDeathmatchAI();

	return true;
}

/*
==================
BotAIStartFrame
==================
*/
int BotAIStartFrame(int time) {
	int i;
	gentity_t* ent;
	bot_entitystate_t state;
	int elapsed_time, thinktime;
	static int local_time;
	static int botlib_residual;
	static int lastbotthink_time;

	trap_Cvar_Update(&bot_thinktime);
	trap_Cvar_Update(&bot_memorydump);
	trap_Cvar_Update(&bot_saveroutingcache);
	trap_Cvar_Update(&bot_pause);
	trap_Cvar_Update(&bot_predictobstacles);

	if(bot_pause.integer) {
		// execute bot user commands every frame
		for(i = 0; i < MAX_CLIENTS; i++) {
			if(!botstates[i] || !botstates[i]->inuse) {
				continue;
			}
			if(g_entities[i].client->pers.connected != CON_CONNECTED) {
				continue;
			}
			botstates[i]->lastucmd.forwardmove = 0;
			botstates[i]->lastucmd.rightmove = 0;
			botstates[i]->lastucmd.upmove = 0;
			botstates[i]->lastucmd.buttons = 0;
			botstates[i]->lastucmd.serverTime = time;
			trap_BotUserCommand(botstates[i]->client, &botstates[i]->lastucmd);
		}
		return true;
	}

	if(bot_memorydump.integer) {
		trap_BotLibVarSet("memorydump", "1");
		trap_Cvar_Set("bot_memorydump", "0");
	}
	if(bot_saveroutingcache.integer) {
		trap_BotLibVarSet("saveroutingcache", "1");
		trap_Cvar_Set("bot_saveroutingcache", "0");
	}
	// cap the bot think time
	if(bot_thinktime.integer > 200) {
		trap_Cvar_Set("bot_thinktime", "200");
	}
	// if the bot think time changed we should reschedule the bots
	if(bot_thinktime.integer != lastbotthink_time) {
		lastbotthink_time = bot_thinktime.integer;
		BotScheduleBotThink();
	}

	elapsed_time = time - local_time;
	local_time = time;

	botlib_residual += elapsed_time;

	if(elapsed_time > bot_thinktime.integer)
		thinktime = elapsed_time;
	else
		thinktime = bot_thinktime.integer;

	// update the bot library
	if(botlib_residual >= thinktime) {
		botlib_residual -= thinktime;

		trap_BotLibStartFrame((float)time / 1000);

		if(!trap_AAS_Initialized()) return false;

		// update entities in the botlib
		for(i = 0; i < MAX_GENTITIES; i++) {
			ent = &g_entities[i];
			if(!ent->inuse) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			if(!ent->r.linked) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			if(ent->r.svFlags & SVF_NOCLIENT) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			// do not update missiles
			if(ent->s.eType == ET_MISSILE) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			// do not update event only entities
			if(ent->s.eType > ET_EVENTS) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			//
			memset(&state, 0, sizeof(bot_entitystate_t));
			//
			VectorCopy(ent->r.currentOrigin, state.origin);
			if(i < MAX_CLIENTS) {
				VectorCopy(ent->s.apos.trBase, state.angles);
			} else {
				VectorCopy(ent->r.currentAngles, state.angles);
			}
			VectorCopy(ent->s.origin2, state.old_origin);
			VectorCopy(ent->r.mins, state.mins);
			VectorCopy(ent->r.maxs, state.maxs);
			state.type = ent->s.eType;
			state.flags = ent->s.eFlags;
			if(ent->r.bmodel)
				state.solid = SOLID_BSP;
			else
				state.solid = SOLID_BBOX;
			state.groundent = ent->s.groundEntityNum;
			state.modelindex = ent->s.modelindex;
			state.modelindex2 = ent->s.modelindex2;
			state.frame = ent->s.frame;
			state.event = ent->s.event;
			state.eventParm = ent->s.eventParm;
			state.legsAnim = ent->s.legsAnim;
			state.torsoAnim = ent->s.torsoAnim;
			state.weapon = ent->s.weapon;
			//
			trap_BotLibUpdateEntity(i, &state);
		}
	}

	floattime = trap_AAS_Time();

	// record player position trails and collect player sounds
	BotUpdatePlayerTracking();

	// execute scheduled bot AI
	for(i = 0; i < MAX_CLIENTS; i++) {
		if(!botstates[i] || !botstates[i]->inuse) {
			continue;
		}
		//
		botstates[i]->botthink_residual += elapsed_time;
		//
		if(botstates[i]->botthink_residual >= thinktime) {
			botstates[i]->botthink_residual -= thinktime;

			if(!trap_AAS_Initialized()) return false;

			if(g_entities[i].client->pers.connected == CON_CONNECTED) {
				BotAI(i, (float)thinktime / 1000);
			}
		}
	}

	// execute bot user commands every frame
	for(i = 0; i < MAX_CLIENTS; i++) {
		if(!botstates[i] || !botstates[i]->inuse) {
			continue;
		}
		if(g_entities[i].client->pers.connected != CON_CONNECTED) {
			continue;
		}

		BotUpdateInput(botstates[i], time, elapsed_time);
		trap_BotUserCommand(botstates[i]->client, &botstates[i]->lastucmd);
	}

	return true;
}

/*
==============
BotInitLibrary
==============
*/
int BotInitLibrary(void) {
	char buf[144];

	// set the maxclients and maxentities library variables before calling BotSetupLibrary
	Com_sprintf(buf, sizeof(buf), "%d", level.maxclients);
	trap_BotLibVarSet("maxclients", buf);
	Com_sprintf(buf, sizeof(buf), "%d", MAX_GENTITIES);
	trap_BotLibVarSet("maxentities", buf);
	// bsp checksum
	trap_Cvar_VariableStringBuffer("sv_mapChecksum", buf, sizeof(buf));
	if(strlen(buf)) trap_BotLibVarSet("sv_mapChecksum", buf);
	// maximum number of aas links
	trap_Cvar_VariableStringBuffer("max_aaslinks", buf, sizeof(buf));
	if(strlen(buf)) trap_BotLibVarSet("max_aaslinks", buf);
	// maximum number of items in a level
	trap_Cvar_VariableStringBuffer("max_levelitems", buf, sizeof(buf));
	if(strlen(buf)) trap_BotLibVarSet("max_levelitems", buf);
	// bot developer mode and log file
	trap_BotLibVarSet("bot_developer", bot_developer.string);
	// visualize jump pads
	trap_Cvar_VariableStringBuffer("bot_visualizejumppads", buf, sizeof(buf));
	if(strlen(buf)) trap_BotLibVarSet("bot_visualizejumppads", buf);
	// forced clustering calculations
	trap_Cvar_VariableStringBuffer("bot_forceclustering", buf, sizeof(buf));
	if(strlen(buf)) trap_BotLibVarSet("forceclustering", buf);
	// forced reachability calculations
	trap_Cvar_VariableStringBuffer("bot_forcereachability", buf, sizeof(buf));
	if(strlen(buf)) trap_BotLibVarSet("forcereachability", buf);
	// force writing of AAS to file
	trap_Cvar_VariableStringBuffer("bot_forcewrite", buf, sizeof(buf));
	if(strlen(buf)) trap_BotLibVarSet("forcewrite", buf);
	// no AAS optimization
	trap_Cvar_VariableStringBuffer("bot_aasoptimize", buf, sizeof(buf));
	if(strlen(buf)) trap_BotLibVarSet("aasoptimize", buf);
	//
	trap_Cvar_VariableStringBuffer("bot_saveroutingcache", buf, sizeof(buf));
	if(strlen(buf)) trap_BotLibVarSet("saveroutingcache", buf);
	// reload instead of cache bot character files
	trap_Cvar_VariableStringBuffer("bot_reloadcharacters", buf, sizeof(buf));
	if(!strlen(buf)) strcpy(buf, "0");
	trap_BotLibVarSet("bot_reloadcharacters", buf);
	// base directory
	trap_Cvar_VariableStringBuffer("fs_basepath", buf, sizeof(buf));
	if(strlen(buf)) trap_BotLibVarSet("basedir", buf);
	// game directory
	trap_Cvar_VariableStringBuffer("fs_game", buf, sizeof(buf));
	if(strlen(buf)) trap_BotLibVarSet("gamedir", buf);
	// setup the bot library
	return trap_BotLibSetup();
}

/*
==============
BotAISetup
==============
*/
int BotAISetup(int restart) {
	int errnum;

	trap_Cvar_Register(&bot_thinktime, "bot_thinktime", "100", CVAR_CHEAT);
	trap_Cvar_Register(&bot_memorydump, "bot_memorydump", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_saveroutingcache, "bot_saveroutingcache", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_pause, "bot_pause", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_testsolid, "bot_testsolid", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_testclusters, "bot_testclusters", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_developer, "bot_developer", "0", CVAR_CHEAT);

	// if the game is restarted for a tournament
	if(restart) {
		return true;
	}

	// initialize the bot states
	memset(botstates, 0, sizeof(botstates));

	errnum = BotInitLibrary();
	if(errnum != BLERR_NOERROR) return false;
	return true;
}

/*
==============
BotAIShutdown
==============
*/
int BotAIShutdown(int restart) {
	int i;

	// if the game is restarted for a tournament
	if(restart) {
		// shutdown all the bots in the botlib
		for(i = 0; i < MAX_CLIENTS; i++) {
			if(botstates[i] && botstates[i]->inuse) {
				BotAIShutdownClient(botstates[i]->client, restart);
			}
		}
		// don't shutdown the bot library
	} else {
		trap_BotLibShutdown();
	}
	return true;
}
