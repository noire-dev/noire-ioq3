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
 * name:		ai_dmq3.c
 *
 * desc:		bot AI
 *
 *****************************************************************************/

#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_dmnet.h"
//

// from aasfile.h
#define AREACONTENTS_MOVER 1024
#define AREACONTENTS_MODELNUMSHIFT 24
#define AREACONTENTS_MAXMODELNUM 0xFF
#define AREACONTENTS_MODELNUM (AREACONTENTS_MAXMODELNUM << AREACONTENTS_MODELNUMSHIFT)

#define IDEAL_ATTACKDIST 140

vmCvar_t bot_challenge;
vmCvar_t bot_predictobstacles;

extern vmCvar_t bot_developer;

vec3_t lastteleport_origin;  // last teleport event origin
float lastteleport_time;     // last teleport event time
int max_bspmodelindex;       // maximum BSP model index

/*
==================
BotPointAreaNum
==================
*/
int BotPointAreaNum(vec3_t origin) {
	int areanum, numareas, areas[10];
	vec3_t end;

	areanum = trap_AAS_PointAreaNum(origin);
	if(areanum) return areanum;
	VectorCopy(origin, end);
	end[2] += 10;
	numareas = trap_AAS_TraceAreas(origin, end, areas, NULL, 10);
	if(numareas > 0) return areas[0];
	return 0;
}

/*
==================
ClientName
==================
*/
char* ClientName(int client, char* name, int size) {
	char buf[MAX_INFO_STRING];

	if(client < 0 || client >= MAX_CLIENTS) {
		BotAI_Print(PRT_ERROR, "ClientName: client out of range\n");
		return "[client out of range]";
	}
	trap_GetConfigstring(CS_PLAYERS + client, buf, sizeof(buf));
	Q_strncpyz(name, Info_ValueForKey(buf, "n"), size);
	Q_CleanStr(name);
	return name;
}

/*
==================
BotGetWeaponInfo

The weapons are dynamic and registered from JS, so instead of asking the
botlib (which loads weapon files) the aiming info is filled from jsd_weapon.
==================
*/
void BotGetWeaponInfo(int weapon, weaponinfo_t* wi) {
	memset(wi, 0, sizeof(weaponinfo_t));
	if(weapon <= 0 || weapon >= jsd_weaponCount) return;

	wi->number = weapon;
	wi->offset[2] = jsd_weapon[weapon].zOffset;

	switch(jsd_weapon[weapon].wType) {
		case WT_MISSILE: {
			// projectile weapons: prediction uses the projectile speed,
			// splash weapons aim at the ground near the enemy
			wi->speed = jsd_weapon[weapon].speed;
			if(jsd_weapon[weapon].splashDamage > 0) {
				wi->proj.damagetype |= DAMAGETYPE_RADIAL;
				wi->proj.damage = jsd_weapon[weapon].splashDamage;
				wi->proj.radius = jsd_weapon[weapon].splashRadius;
			}
			break;
		}
		case WT_BULLET:
		case WT_SHOTGUN:
		case WT_BEAM:
		case WT_MELEE: {
			wi->hspread = jsd_weapon[weapon].spread;
			wi->vspread = jsd_weapon[weapon].spread;
			break;
		}
		default: {
			break;
		}
	}
}

/*
==================
BotBestWeapon

Returns the number of the best weapon the bot can fight with (owned and
with ammo), 0 if the bot has nothing to fight with. Every weapon is judged
only by its damage, weapons are dynamic so there is nothing else to go on.
==================
*/
int BotBestWeapon(bot_state_t* bs) {
	gentity_t* ent;
	int i, power, bestweapon, bestpower;

	ent = &g_entities[bs->client];
	bestweapon = 0;
	bestpower = 0;
	for(i = 1; i < jsd_weaponCount; i++) {
		// the bot must own the weapon and have ammo for it
		if(ent->swep_list[i] < WS_HAVE) continue;
		if(!(ent->swep_ammo[i] > 0 || ent->swep_ammo[i] == -1)) continue;
		switch(jsd_weapon[i].wType) {
			case WT_MELEE:
			case WT_BULLET:
			case WT_SHOTGUN:
			case WT_BEAM:
			case WT_MISSILE: {
				break;
			}
			default: {
				// physgun, gravitygun, toolgun etc. are no fight weapons
				continue;
			}
		}
		power = jsd_weapon[i].damage + jsd_weapon[i].splashDamage;
		if(power > bestpower) {
			bestpower = power;
			bestweapon = i;
		}
	}
	return bestweapon;
}

/*
==================
BotChooseWeapon
==================
*/
void BotChooseWeapon(bot_state_t* bs) {
	int newweaponnum;

	if(bs->cur_ps.weaponstate == WEAPON_RAISING || bs->cur_ps.weaponstate == WEAPON_DROPPING) {
		trap_EA_SelectWeapon(bs->client, bs->weaponnum);
		return;
	}
	newweaponnum = BotBestWeapon(bs);
	if(newweaponnum) {
		if(bs->weaponnum != newweaponnum) bs->weaponchange_time = FloatTime();
		bs->weaponnum = newweaponnum;
	}
	trap_EA_SelectWeapon(bs->client, bs->weaponnum);
}

/*
==================
BotSetupForMovement
==================
*/
void BotSetupForMovement(bot_state_t* bs) {
	bot_initmove_t initmove;

	memset(&initmove, 0, sizeof(bot_initmove_t));
	VectorCopy(bs->cur_ps.origin, initmove.origin);
	VectorCopy(bs->cur_ps.velocity, initmove.velocity);
	VectorClear(initmove.viewoffset);
	initmove.viewoffset[2] += bs->cur_ps.viewheight;
	initmove.entitynum = bs->entitynum;
	initmove.client = bs->client;
	initmove.thinktime = bs->thinktime;
	// set the onground flag
	if(bs->cur_ps.groundEntityNum != ENTITYNUM_NONE) initmove.or_moveflags |= MFL_ONGROUND;
	// set the teleported flag
	if((bs->cur_ps.pm_flags & PMF_TIME_KNOCKBACK) && (bs->cur_ps.pm_time > 0)) {
		initmove.or_moveflags |= MFL_TELEPORTED;
	}
	// set the waterjump flag
	if((bs->cur_ps.pm_flags & PMF_TIME_WATERJUMP) && (bs->cur_ps.pm_time > 0)) {
		initmove.or_moveflags |= MFL_WATERJUMP;
	}
	// set presence type
	if(bs->cur_ps.pm_flags & PMF_DUCKED)
		initmove.presencetype = PRESENCE_CROUCH;
	else
		initmove.presencetype = PRESENCE_NORMAL;
	//
	if(bs->walker > 0.5) initmove.or_moveflags |= MFL_WALK;
	//
	VectorCopy(bs->viewangles, initmove.viewangles);
	//
	trap_BotInitMoveState(bs->ms, &initmove);
}

/*
==================
BotSetTeleportTime
==================
*/
void BotSetTeleportTime(bot_state_t* bs) {
	if((bs->cur_ps.eFlags ^ bs->last_eFlags) & EF_TELEPORT_BIT) {
		bs->teleport_time = FloatTime();
	}
	bs->last_eFlags = bs->cur_ps.eFlags;
}

/*
==================
EntityIsDead
==================
*/
bool EntityIsDead(aas_entityinfo_t* entinfo) {
	playerState_t ps;

	if(entinfo->number >= 0 && entinfo->number < MAX_CLIENTS) {
		// retrieve the current client state
		if(!BotAI_GetClientState(entinfo->number, &ps)) {
			return false;
		}

		if(ps.pm_type != PM_NORMAL) return true;
	}
	return false;
}

/*
==================
EntityIsShooting
==================
*/
bool EntityIsShooting(aas_entityinfo_t* entinfo) {
	if(entinfo->flags & EF_FIRING) {
		return true;
	}
	return false;
}

/*
==================
BotIsDead
==================
*/
bool BotIsDead(bot_state_t* bs) { return (bs->cur_ps.pm_type == PM_DEAD); }

/*
==================
BotInLavaOrSlime
==================
*/
bool BotInLavaOrSlime(bot_state_t* bs) {
	vec3_t feet;

	VectorCopy(bs->origin, feet);
	feet[2] -= 23;
	return (trap_AAS_PointContents(feet) & (CONTENTS_LAVA | CONTENTS_SLIME));
}

/*
==================
BotAggression

Returns the aggression of the bot in the range [0, 100]. Weapons are
dynamic, so aggression only depends on health and how much damage the
best available weapon does.
==================
*/
float BotAggression(bot_state_t* bs) {
	aas_entityinfo_t entinfo;
	int health, weapon, damage;

	// if the enemy is located way higher than the bot
	if(bs->enemy >= 0) {
		BotEntityInfo(bs->enemy, &entinfo);
		if(entinfo.origin[2] > bs->origin[2] + 200) return 0;
	}
	// if the bot is very low on health
	health = bs->cur_ps.stats[STAT_HEALTH];
	if(health < 60) return 0;
	// without a weapon there is nothing to be aggressive about
	weapon = BotBestWeapon(bs);
	if(!weapon) return 0;
	damage = jsd_weapon[weapon].damage + jsd_weapon[weapon].splashDamage;
	// if the bot is low on health it only keeps fighting with a strong weapon
	if(health < 80 && damage < 100) return 0;
	// the stronger the weapon, the more aggressive the bot
	if(damage >= 100) return 100;
	if(damage >= 50) return 90;
	return 50;
}

/*
==================
BotMeleeFighter

A bot without a usable weapon (none at all or a melee type weapon) has
to get right next to its enemy to do any damage, so it always chases
and never retreats.
==================
*/
bool BotMeleeFighter(bot_state_t* bs) {
	int weapon = bs->cur_ps.weapon;

	if(weapon <= WP_NONE || weapon >= jsd_weaponCount) return true;
	return (jsd_weapon[weapon].wType == WT_MELEE);
}

/*
==================
BotChaseEnemyMove

Movement for melee bots: run right up to the enemy moving exclusively
along the AAS routing (no direct steering).
==================
*/
static bot_moveresult_t BotChaseEnemyMove(bot_state_t* bs, int tfl) {
	bot_goal_t goal;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	int areanum;

	memset(&moveresult, 0, sizeof(bot_moveresult_t));
	// update the enemy area and origin
	BotEntityInfo(bs->enemy, &entinfo);
	areanum = BotPointAreaNum(entinfo.origin);
	if(areanum && trap_AAS_AreaReachability(areanum)) {
		VectorCopy(entinfo.origin, bs->lastenemyorigin);
		bs->lastenemyareanum = areanum;
	}
	if(!bs->lastenemyareanum) return moveresult;
	// the goal is the enemy itself, right up close
	goal.entitynum = bs->enemy;
	goal.areanum = bs->lastenemyareanum;
	VectorCopy(bs->lastenemyorigin, goal.origin);
	VectorSet(goal.mins, -8, -8, -8);
	VectorSet(goal.maxs, 8, 8, 8);
	// initialize the movement state
	BotSetupForMovement(bs);
	// run at the enemy along the AAS routing
	trap_BotMoveToGoal(&moveresult, bs->ms, &goal, tfl);
	return moveresult;
}

/*
==================
BotWantsToRetreat
==================
*/
int BotWantsToRetreat(bot_state_t* bs) {
	if(BotMeleeFighter(bs)) return false;
	return (BotAggression(bs) < 50);
}

/*
==================
BotWantsToChase
==================
*/
int BotWantsToChase(bot_state_t* bs) {
	if(BotMeleeFighter(bs)) return true;
	return (BotAggression(bs) > 50);
}

/*
==================
BotRoamGoal
==================
*/
void BotRoamGoal(bot_state_t* bs, vec3_t goal) {
	int pc, i;
	float len, rnd;
	vec3_t dir, bestorg, belowbestorg;
	bsp_trace_t trace;

	for(i = 0; i < 10; i++) {
		// start at the bot origin
		VectorCopy(bs->origin, bestorg);
		rnd = random();
		if(rnd > 0.25) {
			// add a random value to the x-coordinate
			if(random() < 0.5)
				bestorg[0] -= 800 * random() + 100;
			else
				bestorg[0] += 800 * random() + 100;
		}
		if(rnd < 0.75) {
			// add a random value to the y-coordinate
			if(random() < 0.5)
				bestorg[1] -= 800 * random() + 100;
			else
				bestorg[1] += 800 * random() + 100;
		}
		// add a random value to the z-coordinate (NOTE: 48 = maxjump?)
		bestorg[2] += 2 * 48 * crandom();
		// trace a line from the origin to the roam target
		BotAI_Trace(&trace, bs->origin, NULL, NULL, bestorg, bs->entitynum, MASK_SOLID);
		// direction and length towards the roam target
		VectorSubtract(trace.endpos, bs->origin, dir);
		len = VectorNormalize(dir);
		// if the roam target is far away enough
		if(len > 200) {
			// the roam target is in the given direction before walls
			VectorScale(dir, len * trace.fraction - 40, dir);
			VectorAdd(bs->origin, dir, bestorg);
			// get the coordinates of the floor below the roam target
			belowbestorg[0] = bestorg[0];
			belowbestorg[1] = bestorg[1];
			belowbestorg[2] = bestorg[2] - 800;
			BotAI_Trace(&trace, bestorg, NULL, NULL, belowbestorg, bs->entitynum, MASK_SOLID);
			//
			if(!trace.startsolid) {
				trace.endpos[2]++;
				pc = trap_PointContents(trace.endpos, bs->entitynum);
				if(!(pc & (CONTENTS_LAVA | CONTENTS_SLIME))) {
					VectorCopy(bestorg, goal);
					return;
				}
			}
		}
	}
	VectorCopy(bestorg, goal);
}

/*
==================
BotSpotSeesEnemy

true if there is a line of fire from a spot (eye height) to the enemy
==================
*/
static bool BotSpotSeesEnemy(bot_state_t* bs, vec3_t spot, aas_entityinfo_t* entinfo) {
	bsp_trace_t trace;
	vec3_t start, end;

	VectorCopy(spot, start);
	start[2] += 32;
	VectorCopy(entinfo->origin, end);
	end[2] += 8;
	BotAI_Trace(&trace, start, NULL, NULL, end, bs->entitynum, CONTENTS_SOLID | CONTENTS_PLAYERCLIP);
	return (trace.fraction >= 1 || trace.ent == entinfo->number);
}

/*
==================
BotFindCombatSpot

Finds a position to fight from along the AAS routing: reachable by the
bot, at a healthy distance from the enemy and with a line of fire to
it. Returns the area number and writes the spot origin, 0 if no usable
position was found.
==================
*/
static int BotFindCombatSpot(bot_state_t* bs, aas_entityinfo_t* entinfo, int preferred, vec3_t spot) {
	int areas[32], numareas, i, areanum, bestarea;
	float dist, score, bestscore;
	vec3_t center, mins, maxs, dir;
	aas_areainfo_t areainfo;

	bestarea = 0;
	bestscore = 0;
	VectorSet(mins, entinfo->origin[0] - 700, entinfo->origin[1] - 700, entinfo->origin[2] - 400);
	VectorSet(maxs, entinfo->origin[0] + 700, entinfo->origin[1] + 700, entinfo->origin[2] + 400);
	numareas = trap_AAS_BBoxAreas(mins, maxs, areas, 32);
	for(i = 0; i < numareas; i++) {
		areanum = areas[i];
		if(!trap_AAS_AreaReachability(areanum)) continue;
		trap_AAS_AreaInfo(areanum, &areainfo);
		VectorAdd(areainfo.mins, areainfo.maxs, center);
		VectorScale(center, 0.5, center);
		VectorSubtract(center, entinfo->origin, dir);
		dist = VectorLength(dir);
		// not too close (no knife fights) and not too far
		if(dist < 150 || dist > 1100) continue;
		// the position has to be reachable for the bot
		if(!trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, areanum, TFL_DEFAULT)) continue;
		// the position has to have a line of fire to the enemy
		if(!BotSpotSeesEnemy(bs, center, entinfo)) continue;
		// the closer to the preferred engagement distance the better
		score = 1.0 - fabs(dist - preferred) / 1100.0 + random() * 0.3;
		if(score > bestscore) {
			bestscore = score;
			bestarea = areanum;
			VectorCopy(center, spot);
		}
	}
	return bestarea;
}

/*
==================
BotCombatMove

Source style combat movement: ranged bots occupy a position with a
line of fire to the enemy (reached over the AAS routing) and shoot
from there, repositioning when it goes stale. Melee bots just chase
the enemy down. There is no strafing and no jumping around.
==================
*/
bot_moveresult_t BotCombatMove(bot_state_t* bs, int tfl) {
	bot_goal_t goal;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	weaponinfo_t wi;
	int areanum, preferred;
	vec3_t dir;

	memset(&moveresult, 0, sizeof(bot_moveresult_t));
	// a melee bot fights by getting right next to the enemy
	if(BotMeleeFighter(bs)) return BotChaseEnemyMove(bs, tfl);
	//
	BotEntityInfo(bs->enemy, &entinfo);
	// validate the combat position periodically
	if(bs->combatspot_valid && bs->combatspot_check_time < FloatTime()) {
		bs->combatspot_check_time = FloatTime() + 0.5;
		// the enemy moved away from where the position was picked for
		VectorSubtract(entinfo.origin, bs->combatspot_enemyorigin, dir);
		if(VectorLengthSquared(dir) > Square(400)) {
			bs->combatspot_valid = false;
		}
		// the position no longer has a line of fire to the enemy
		else if(!BotSpotSeesEnemy(bs, bs->combatspot_origin, &entinfo)) {
			bs->combatspot_valid = false;
		}
	}
	// pick a new combat position
	if(!bs->combatspot_valid) {
		BotGetWeaponInfo(bs->weaponnum, &wi);
		// splash weapons are fought at a longer range
		preferred = (wi.speed > 0 && (wi.proj.damagetype & DAMAGETYPE_RADIAL)) ? 700 : 450;
		areanum = BotFindCombatSpot(bs, &entinfo, preferred, bs->combatspot_origin);
		if(areanum) {
			bs->combatspot_areanum = areanum;
			bs->combatspot_valid = true;
			VectorCopy(entinfo.origin, bs->combatspot_enemyorigin);
			trap_BotResetLastAvoidReach(bs->ms);
		} else {
			// no usable position: close in on the enemy along the AAS
			return BotChaseEnemyMove(bs, tfl);
		}
	}
	// move to the position over the AAS routing, then hold it and shoot
	goal.entitynum = bs->enemy;
	goal.areanum = bs->combatspot_areanum;
	VectorCopy(bs->combatspot_origin, goal.origin);
	VectorSet(goal.mins, -15, -15, -15);
	VectorSet(goal.maxs, 15, 15, 15);
	if(!trap_BotTouchingGoal(bs->origin, &goal)) {
		BotSetupForMovement(bs);
		trap_BotMoveToGoal(&moveresult, bs->ms, &goal, tfl);
	}
	return moveresult;
}

/*
==================
InFieldOfVision
==================
*/
bool InFieldOfVision(vec3_t viewangles, float fov, vec3_t angles) {
	int i;
	float diff, angle;

	for(i = 0; i < 2; i++) {
		angle = AngleMod(viewangles[i]);
		angles[i] = AngleMod(angles[i]);
		diff = angles[i] - angle;
		if(angles[i] > angle) {
			if(diff > 180.0) diff -= 360.0;
		} else {
			if(diff < -180.0) diff += 360.0;
		}
		if(diff > 0) {
			if(diff > fov * 0.5) return false;
		} else {
			if(diff < -fov * 0.5) return false;
		}
	}
	return true;
}

/*
==================
BotEntityVisible

returns visibility in the range [0, 1] taking fog and water surfaces into account
==================
*/
float BotEntityVisible(int viewer, vec3_t eye, vec3_t viewangles, float fov, int ent) {
	int i, contents_mask, passent, hitent, infog, inwater, otherinfog, pc;
	float squaredfogdist, waterfactor, vis, bestvis;
	bsp_trace_t trace;
	aas_entityinfo_t entinfo;
	vec3_t dir, entangles, start, end, middle;

	BotEntityInfo(ent, &entinfo);
	if(!entinfo.valid) {
		return 0;
	}

	// calculate middle of bounding box
	VectorAdd(entinfo.mins, entinfo.maxs, middle);
	VectorScale(middle, 0.5, middle);
	VectorAdd(entinfo.origin, middle, middle);
	// check if entity is within field of vision
	VectorSubtract(middle, eye, dir);
	vectoangles(dir, entangles);
	if(!InFieldOfVision(viewangles, fov, entangles)) return 0;
	//
	pc = trap_AAS_PointContents(eye);
	infog = (pc & CONTENTS_FOG);
	inwater = (pc & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER));
	//
	bestvis = 0;
	for(i = 0; i < 3; i++) {
		//
		contents_mask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
		passent = viewer;
		hitent = ent;
		VectorCopy(eye, start);
		VectorCopy(middle, end);
		// if the entity is in water, lava or slime
		if(trap_AAS_PointContents(middle) & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER)) {
			contents_mask |= (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER);
		}
		// if eye is in water, lava or slime
		if(inwater) {
			if(!(contents_mask & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER))) {
				passent = ent;
				hitent = viewer;
				VectorCopy(middle, start);
				VectorCopy(eye, end);
			}
			contents_mask ^= (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER);
		}
		// trace from start to end
		BotAI_Trace(&trace, start, NULL, NULL, end, passent, contents_mask);
		// the game trace cannot report water contents, so visibility is
		// never reduced by water surfaces
		waterfactor = 1.0;
		// if a full trace or the hitent was hit
		if(trace.fraction >= 1 || trace.ent == hitent) {
			// check for fog, assuming there's only one fog brush where
			// either the viewer or the entity is in or both are in
			otherinfog = (trap_AAS_PointContents(middle) & CONTENTS_FOG);
			if(infog && otherinfog) {
				VectorSubtract(trace.endpos, eye, dir);
				squaredfogdist = VectorLengthSquared(dir);
			} else if(infog) {
				VectorCopy(trace.endpos, start);
				BotAI_Trace(&trace, start, NULL, NULL, eye, viewer, CONTENTS_FOG);
				VectorSubtract(eye, trace.endpos, dir);
				squaredfogdist = VectorLengthSquared(dir);
			} else if(otherinfog) {
				VectorCopy(trace.endpos, end);
				BotAI_Trace(&trace, eye, NULL, NULL, end, viewer, CONTENTS_FOG);
				VectorSubtract(end, trace.endpos, dir);
				squaredfogdist = VectorLengthSquared(dir);
			} else {
				// if the entity and the viewer are not in fog assume there's no fog in between
				squaredfogdist = 0;
			}
			// decrease visibility with the view distance through fog
			vis = 1 / ((squaredfogdist * 0.001) < 1 ? 1 : (squaredfogdist * 0.001));
			// if entering water visibility is reduced
			vis *= waterfactor;
			//
			if(vis > bestvis) bestvis = vis;
			// if pretty much no fog
			if(bestvis >= 0.95) return bestvis;
		}
		// check bottom and top of bounding box as well
		if(i == 0)
			middle[2] += entinfo.mins[2];
		else if(i == 1)
			middle[2] += entinfo.maxs[2] - entinfo.mins[2];
	}
	return bestvis;
}

/*
==================
BotFindEnemy
==================
*/
int BotFindEnemy(bot_state_t* bs, int curenemy) {
	int i, healthdecrease;
	float f, alertness, vis;
	float squaredist;
	aas_entityinfo_t entinfo;
	vec3_t dir, angles;

	alertness = BotSkill_BFloat(bs, 0, 1);
	// check if the health decreased
	healthdecrease = bs->lasthealth > bs->cur_ps.stats[STAT_HEALTH];
	//
	for(i = 0; i < level.maxclients; i++) {
		if(i == bs->client) continue;
		// if it's the current enemy
		if(i == curenemy) continue;
		// if the enemy has targeting disabled
		if(g_entities[i].flags & FL_NOTARGET) {
			continue;
		}
		//
		BotEntityInfo(i, &entinfo);
		//
		if(!entinfo.valid) continue;
		// if the enemy isn't dead and the enemy isn't the bot self
		if(EntityIsDead(&entinfo) || entinfo.number == bs->entitynum) continue;
		//
		if(lastteleport_time > FloatTime() - 3) {
			VectorSubtract(entinfo.origin, lastteleport_origin, dir);
			if(VectorLengthSquared(dir) < Square(70)) continue;
		}
		// calculate the distance towards the enemy
		VectorSubtract(entinfo.origin, bs->origin, dir);
		squaredist = VectorLengthSquared(dir);
		if(squaredist > Square(900.0 + alertness * 4000.0)) continue;
		// if the bot's health decreased or the enemy is shooting
		if(curenemy < 0 && (healthdecrease || EntityIsShooting(&entinfo)))
			f = 360;
		else
			f = 90 + 90 - (90 - (squaredist > Square(810) ? Square(810) : squaredist) / (810 * 9));
		// check if the enemy is visible
		vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, f, i);
		if(vis <= 0) continue;
		// if the enemy is quite far away, not shooting and the bot is not damaged
		if(curenemy < 0 && squaredist > Square(100) && !healthdecrease && !EntityIsShooting(&entinfo)) {
			// check if we can avoid this enemy
			VectorSubtract(bs->origin, entinfo.origin, dir);
			vectoangles(dir, angles);
			// if the bot isn't in the fov of the enemy
			if(!InFieldOfVision(entinfo.angles, 90, angles)) {
				// if the bot doesn't really want to fight
				if(BotWantsToRetreat(bs)) continue;
			}
		}
		// found an enemy
		bs->enemy = entinfo.number;
		if(curenemy >= 0)
			bs->enemysight_time = FloatTime() - 2;
		else
			bs->enemysight_time = FloatTime();
		bs->enemydeath_time = 0;
		bs->enemyvisible_time = FloatTime();
		return true;
	}
	return false;
}

/*
==================
BotAimAtEnemy
==================
*/
void BotAimAtEnemy(bot_state_t* bs) {
	int i, enemyvisible;
	float dist, f, aim_skill, aim_accuracy, speed, reactiontime;
	vec3_t dir, bestorigin, end, start, groundtarget, cmdmove, enemyvelocity;
	vec3_t mins = {-4, -4, -4}, maxs = {4, 4, 4};
	weaponinfo_t wi;
	aas_entityinfo_t entinfo;
	bot_goal_t goal;
	bsp_trace_t trace;
	vec3_t target;

	// if the bot has no enemy
	if(bs->enemy < 0) {
		return;
	}
	//
	aim_skill = BotSkill_BFloat(bs, 0, 1);
	aim_accuracy = BotSkill_BFloat(bs, 0, 1);
	//
	if(aim_skill > 0.95) {
		// don't aim too early
		reactiontime = 0.5 * BotSkill_BFloat(bs, 1, 0);
		if(bs->enemysight_time > FloatTime() - reactiontime) return;
		if(bs->teleport_time > FloatTime() - reactiontime) return;
	}

	// get the weapon information from the dynamic jsd_weapon data
	BotGetWeaponInfo(bs->weaponnum, &wi);
	// get the enemy entity information
	BotEntityInfo(bs->enemy, &entinfo);
	//
	VectorSubtract(entinfo.origin, entinfo.lastvisorigin, enemyvelocity);
	VectorScale(enemyvelocity, 1 / entinfo.update_time, enemyvelocity);
	// enemy origin and velocity is remembered every 0.5 seconds
	if(bs->enemyposition_time < FloatTime()) {
		//
		bs->enemyposition_time = FloatTime() + 0.5;
		VectorCopy(enemyvelocity, bs->enemyvelocity);
		VectorCopy(entinfo.origin, bs->enemyorigin);
	}
	// if not extremely skilled
	if(aim_skill < 0.9) {
		VectorSubtract(entinfo.origin, bs->enemyorigin, dir);
		// if the enemy moved a bit
		if(VectorLengthSquared(dir) > Square(48)) {
			// if the enemy changed direction
			if(DotProduct(bs->enemyvelocity, enemyvelocity) < 0) {
				// aim accuracy should be worse now
				aim_accuracy *= 0.7f;
			}
		}
	}
	// check visibility of enemy
	enemyvisible = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy);
	// if the enemy is visible
	if(enemyvisible) {
		//
		VectorCopy(entinfo.origin, bestorigin);
		bestorigin[2] += 8;
		// get the start point shooting from
		// NOTE: the x and y projectile start offsets are ignored
		VectorCopy(bs->origin, start);
		start[2] += bs->cur_ps.viewheight;
		start[2] += wi.offset[2];
		//
		BotAI_Trace(&trace, start, mins, maxs, bestorigin, bs->entitynum, MASK_SHOT);
		// if the enemy is NOT hit
		if(trace.fraction <= 1 && trace.ent != entinfo.number) {
			bestorigin[2] += 16;
		}
		// if it is not an instant hit weapon the bot might want to predict the enemy
		if(wi.speed) {
			//
			VectorSubtract(bestorigin, bs->origin, dir);
			dist = VectorLength(dir);
			VectorSubtract(entinfo.origin, bs->enemyorigin, dir);
			// if the enemy is NOT pretty far away and strafing just small steps left and right
			if(!(dist > 100 && VectorLengthSquared(dir) < Square(32))) {
				// if skilled enough do exact prediction
				if(aim_skill > 0.8 &&
				   // if the weapon is ready to fire
				   bs->cur_ps.weaponstate == WEAPON_READY) {
					aas_clientmove_t move;
					vec3_t origin;

					VectorSubtract(entinfo.origin, bs->origin, dir);
					// distance towards the enemy
					dist = VectorLength(dir);
					// direction the enemy is moving in
					VectorSubtract(entinfo.origin, entinfo.lastvisorigin, dir);
					//
					VectorScale(dir, 1 / entinfo.update_time, dir);
					//
					VectorCopy(entinfo.origin, origin);
					origin[2] += 1;
					//
					VectorClear(cmdmove);
					trap_AAS_PredictClientMovement(&move, bs->enemy, origin, PRESENCE_CROUCH, false, dir, cmdmove, 0, dist * 10 / wi.speed, 0.1f, 0, 0, false);
					VectorCopy(move.endpos, bestorigin);
				}
				// if not that skilled do linear prediction
				else if(aim_skill > 0.4) {
					VectorSubtract(entinfo.origin, bs->origin, dir);
					// distance towards the enemy
					dist = VectorLength(dir);
					// direction the enemy is moving in
					VectorSubtract(entinfo.origin, entinfo.lastvisorigin, dir);
					dir[2] = 0;
					//
					speed = VectorNormalize(dir) / entinfo.update_time;
					// best spot to aim at
					VectorMA(entinfo.origin, (dist / wi.speed) * speed, dir, bestorigin);
				}
			}
		}
		// if the projectile does radial damage
		if(aim_skill > 0.6 && wi.proj.damagetype & DAMAGETYPE_RADIAL) {
			// if the enemy isn't standing significantly higher than the bot
			if(entinfo.origin[2] < bs->origin[2] + 16) {
				// try to aim at the ground in front of the enemy
				VectorCopy(entinfo.origin, end);
				end[2] -= 64;
				BotAI_Trace(&trace, entinfo.origin, NULL, NULL, end, entinfo.number, MASK_SHOT);
				//
				VectorCopy(bestorigin, groundtarget);
				if(trace.startsolid)
					groundtarget[2] = entinfo.origin[2] - 16;
				else
					groundtarget[2] = trace.endpos[2] - 8;
				// trace a line from projectile start to ground target
				BotAI_Trace(&trace, start, NULL, NULL, groundtarget, bs->entitynum, MASK_SHOT);
				// if hitpoint is not vertically too far from the ground target
				if(fabs(trace.endpos[2] - groundtarget[2]) < 50) {
					VectorSubtract(trace.endpos, groundtarget, dir);
					// if the hitpoint is near enough the ground target
					if(VectorLengthSquared(dir) < Square(60)) {
						VectorSubtract(trace.endpos, start, dir);
						// if the hitpoint is far enough from the bot
						if(VectorLengthSquared(dir) > Square(100)) {
							// check if the bot is visible from the ground target
							trace.endpos[2] += 1;
							BotAI_Trace(&trace, trace.endpos, NULL, NULL, entinfo.origin, entinfo.number, MASK_SHOT);
							if(trace.fraction >= 1) {
								VectorCopy(groundtarget, bestorigin);
							}
						}
					}
				}
			}
		}
		bestorigin[0] += 20 * crandom() * (1 - aim_accuracy);
		bestorigin[1] += 20 * crandom() * (1 - aim_accuracy);
		bestorigin[2] += 10 * crandom() * (1 - aim_accuracy);
	} else {
		//
		VectorCopy(bs->lastenemyorigin, bestorigin);
		bestorigin[2] += 8;
		// if the bot is skilled enough
		if(aim_skill > 0.5) {
			// do prediction shots around corners
			if(jsd_weapon[bs->weaponnum].wType == WT_MISSILE) {
				// create the chase goal
				goal.entitynum = bs->client;
				goal.areanum = bs->areanum;
				VectorCopy(bs->eye, goal.origin);
				VectorSet(goal.mins, -8, -8, -8);
				VectorSet(goal.maxs, 8, 8, 8);
				//
				if(trap_BotPredictVisiblePosition(bs->lastenemyorigin, bs->lastenemyareanum, &goal, TFL_DEFAULT, target)) {
					VectorSubtract(target, bs->eye, dir);
					if(VectorLengthSquared(dir) > Square(80)) {
						VectorCopy(target, bestorigin);
						bestorigin[2] -= 20;
					}
				}
				aim_accuracy = 1;
			}
		}
	}
	//
	if(enemyvisible) {
		BotAI_Trace(&trace, bs->eye, NULL, NULL, bestorigin, bs->entitynum, MASK_SHOT);
		VectorCopy(trace.endpos, bs->aimtarget);
	} else {
		VectorCopy(bestorigin, bs->aimtarget);
	}
	// get aim direction
	VectorSubtract(bestorigin, bs->eye, dir);
	//
	if(jsd_weapon[bs->weaponnum].wType == WT_BULLET || jsd_weapon[bs->weaponnum].wType == WT_SHOTGUN || jsd_weapon[bs->weaponnum].wType == WT_BEAM) {
		// distance towards the enemy
		dist = VectorLength(dir);
		if(dist > 150) dist = 150;
		f = 0.6 + dist / 150 * 0.4;
		aim_accuracy *= f;
	}
	// add some random stuff to the aim direction depending on the aim accuracy
	if(aim_accuracy < 0.8) {
		VectorNormalize(dir);
		for(i = 0; i < 3; i++) dir[i] += 0.3 * crandom() * (1 - aim_accuracy);
	}
	// set the ideal view angles
	vectoangles(dir, bs->ideal_viewangles);
	// take the weapon spread into account for lower skilled bots
	bs->ideal_viewangles[PITCH] += 6 * wi.vspread * crandom() * (1 - aim_accuracy);
	bs->ideal_viewangles[PITCH] = AngleMod(bs->ideal_viewangles[PITCH]);
	bs->ideal_viewangles[YAW] += 6 * wi.hspread * crandom() * (1 - aim_accuracy);
	bs->ideal_viewangles[YAW] = AngleMod(bs->ideal_viewangles[YAW]);
	// if the bots should be really challenging
	if(bot_challenge.integer) {
		// if the bot is really accurate and has the enemy in view for some time
		if(aim_accuracy > 0.9 && bs->enemysight_time < FloatTime() - 1) {
			// set the view angles directly
			if(bs->ideal_viewangles[PITCH] > 180) bs->ideal_viewangles[PITCH] -= 360;
			VectorCopy(bs->ideal_viewangles, bs->viewangles);
			trap_EA_View(bs->client, bs->viewangles);
		}
	}
}

/*
==================
BotCheckAttack
==================
*/
void BotCheckAttack(bot_state_t* bs) {
	float points, reactiontime, fov, firethrottle;
	int attackentity;
	bsp_trace_t bsptrace;
	vec3_t forward, right, start, end, dir, angles;
	weaponinfo_t wi;
	bsp_trace_t trace;
	aas_entityinfo_t entinfo;
	vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};

	attackentity = bs->enemy;
	BotEntityInfo(attackentity, &entinfo);
	// skill 1 reacts slowly, skill 5 reacts almost instantly
	reactiontime = BotSkill_BFloat(bs, 1, 0);
	if(bs->enemysight_time > FloatTime() - reactiontime) return;
	if(bs->teleport_time > FloatTime() - reactiontime) return;
	// if changing weapons
	if(bs->weaponchange_time > FloatTime() - 0.1) return;
	// check fire throttle characteristic
	if(bs->firethrottlewait_time > FloatTime()) return;
	firethrottle = BotSkill_BFloat(bs, 0, 1);
	if(bs->firethrottleshoot_time < FloatTime()) {
		if(random() > firethrottle) {
			bs->firethrottlewait_time = FloatTime() + firethrottle;
			bs->firethrottleshoot_time = 0;
		} else {
			bs->firethrottleshoot_time = FloatTime() + 1 - firethrottle;
			bs->firethrottlewait_time = 0;
		}
	}
	//
	//
	VectorSubtract(bs->aimtarget, bs->eye, dir);
	//
	// without a ranged weapon only swing when right next to the enemy
	if(BotMeleeFighter(bs)) {
		if(VectorLengthSquared(dir) > Square(60)) {
			return;
		}
	}
	if(VectorLengthSquared(dir) < Square(100))
		fov = 120;
	else
		fov = 50;
	//
	vectoangles(dir, angles);
	if(!InFieldOfVision(bs->viewangles, fov, angles)) return;
	BotAI_Trace(&bsptrace, bs->eye, NULL, NULL, bs->aimtarget, bs->client, CONTENTS_SOLID | CONTENTS_PLAYERCLIP);
	if(bsptrace.fraction < 1 && bsptrace.ent != attackentity) return;

	// get the weapon info
	BotGetWeaponInfo(bs->weaponnum, &wi);
	// get the start point shooting from
	VectorCopy(bs->origin, start);
	start[2] += bs->cur_ps.viewheight;
	AngleVectors(bs->viewangles, forward, right, NULL);
	start[0] += forward[0] * wi.offset[0] + right[0] * wi.offset[1];
	start[1] += forward[1] * wi.offset[0] + right[1] * wi.offset[1];
	start[2] += forward[2] * wi.offset[0] + right[2] * wi.offset[1] + wi.offset[2];
	// end point aiming at
	VectorMA(start, 1000, forward, end);
	// a little back to make sure not inside a very close enemy
	VectorMA(start, -12, forward, start);
	BotAI_Trace(&trace, start, mins, maxs, end, bs->entitynum, MASK_SHOT);
	// if won't hit the enemy or not attacking a player
	if(trace.ent != attackentity || attackentity >= MAX_CLIENTS) {
		// if the projectile does radial damage
		if(wi.proj.damagetype & DAMAGETYPE_RADIAL) {
			if(trace.fraction * 1000 < wi.proj.radius) {
				points = (wi.proj.damage - 0.5 * trace.fraction * 1000) * 0.5;
				if(points > 0) {
					return;
				}
			}
		}
	}
	trap_EA_Attack(bs->client);
}

/*
==================
BotSetMovedir
==================
*/
static vec3_t VEC_UP = {0, -1, 0};
static vec3_t MOVEDIR_UP = {0, 0, 1};
static vec3_t VEC_DOWN = {0, -2, 0};
static vec3_t MOVEDIR_DOWN = {0, 0, -1};

void BotSetMovedir(vec3_t angles, vec3_t movedir) {
	if(VectorCompare(angles, VEC_UP)) {
		VectorCopy(MOVEDIR_UP, movedir);
	} else if(VectorCompare(angles, VEC_DOWN)) {
		VectorCopy(MOVEDIR_DOWN, movedir);
	} else {
		AngleVectors(angles, movedir, NULL, NULL);
	}
}

/*
==================
BotModelMinsMaxs

this is ugly
==================
*/
int BotModelMinsMaxs(int modelindex, int eType, int contents, vec3_t mins, vec3_t maxs) {
	gentity_t* ent;
	int i;

	ent = &g_entities[0];
	for(i = 0; i < level.num_entities; i++, ent++) {
		if(!ent->inuse) {
			continue;
		}
		if(eType && ent->s.eType != eType) {
			continue;
		}
		if(contents && ent->r.contents != contents) {
			continue;
		}
		if(ent->s.modelindex == modelindex) {
			if(mins) VectorAdd(ent->r.currentOrigin, ent->r.mins, mins);
			if(maxs) VectorAdd(ent->r.currentOrigin, ent->r.maxs, maxs);
			return i;
		}
	}
	if(mins) VectorClear(mins);
	if(maxs) VectorClear(maxs);
	return 0;
}

/*
==================
BotFuncButtonActivateGoal
==================
*/
int BotFuncButtonActivateGoal(bot_state_t* bs, int bspent, bot_activategoal_t* activategoal) {
	int i, areas[10], numareas, modelindex, entitynum;
	char model[128];
	float lip, dist, health, angle;
	vec3_t size, start, end, mins, maxs, angles, points[10];
	vec3_t movedir, origin, goalorigin, bboxmins, bboxmaxs;
	vec3_t extramins = {1, 1, 1}, extramaxs = {-1, -1, -1};
	bsp_trace_t bsptrace;

	activategoal->shoot = false;
	VectorClear(activategoal->target);
	// create a bot goal towards the button
	trap_AAS_ValueForBSPEpairKey(bspent, "model", model, sizeof(model));
	if(!*model) return false;
	modelindex = atoi(model + 1);
	if(!modelindex) return false;
	entitynum = BotModelMinsMaxs(modelindex, ET_MOVER, 0, mins, maxs);
	// get the lip of the button
	trap_AAS_FloatForBSPEpairKey(bspent, "lip", &lip);
	if(!lip) lip = 4;
	// get the move direction from the angle
	trap_AAS_FloatForBSPEpairKey(bspent, "angle", &angle);
	VectorSet(angles, 0, angle, 0);
	BotSetMovedir(angles, movedir);
	// button size
	VectorSubtract(maxs, mins, size);
	// button origin
	VectorAdd(mins, maxs, origin);
	VectorScale(origin, 0.5, origin);
	// touch distance of the button
	dist = fabs(movedir[0]) * size[0] + fabs(movedir[1]) * size[1] + fabs(movedir[2]) * size[2];
	dist *= 0.5;
	//
	trap_AAS_FloatForBSPEpairKey(bspent, "health", &health);
	// if the button is shootable
	if(health) {
		// calculate the shoot target
		VectorMA(origin, -dist, movedir, goalorigin);
		//
		VectorCopy(goalorigin, activategoal->target);
		activategoal->shoot = true;
		//
		BotAI_Trace(&bsptrace, bs->eye, NULL, NULL, goalorigin, bs->entitynum, MASK_SHOT);
		// if the button is visible from the current position
		if(bsptrace.fraction >= 1.0 || bsptrace.ent == entitynum) {
			//
			activategoal->goal.entitynum = entitynum;  // NOTE: this is the entity number of the shootable button
			activategoal->goal.number = 0;
			activategoal->goal.flags = 0;
			VectorCopy(bs->origin, activategoal->goal.origin);
			activategoal->goal.areanum = bs->areanum;
			VectorSet(activategoal->goal.mins, -8, -8, -8);
			VectorSet(activategoal->goal.maxs, 8, 8, 8);
			//
			return true;
		} else {
			// create a goal from where the button is visible and shoot at the button from there
			// add bounding box size to the dist
			trap_AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, bboxmins, bboxmaxs);
			for(i = 0; i < 3; i++) {
				if(movedir[i] < 0)
					dist += fabs(movedir[i]) * fabs(bboxmaxs[i]);
				else
					dist += fabs(movedir[i]) * fabs(bboxmins[i]);
			}
			// calculate the goal origin
			VectorMA(origin, -dist, movedir, goalorigin);
			//
			VectorCopy(goalorigin, start);
			start[2] += 24;
			VectorCopy(start, end);
			end[2] -= 512;
			numareas = trap_AAS_TraceAreas(start, end, areas, points, 10);
			//
			for(i = numareas - 1; i >= 0; i--) {
				if(trap_AAS_AreaReachability(areas[i])) {
					break;
				}
			}
			if(i < 0) {
				// FIXME: trace forward and maybe in other directions to find a valid area
			}
			if(i >= 0) {
				//
				VectorCopy(points[i], activategoal->goal.origin);
				activategoal->goal.areanum = areas[i];
				VectorSet(activategoal->goal.mins, 8, 8, 8);
				VectorSet(activategoal->goal.maxs, -8, -8, -8);
				//
				for(i = 0; i < 3; i++) {
					if(movedir[i] < 0)
						activategoal->goal.maxs[i] += fabs(movedir[i]) * fabs(extramaxs[i]);
					else
						activategoal->goal.mins[i] += fabs(movedir[i]) * fabs(extramins[i]);
				}  // end for
				//
				activategoal->goal.entitynum = entitynum;
				activategoal->goal.number = 0;
				activategoal->goal.flags = 0;
				return true;
			}
		}
		return false;
	} else {
		// add bounding box size to the dist
		trap_AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, bboxmins, bboxmaxs);
		for(i = 0; i < 3; i++) {
			if(movedir[i] < 0)
				dist += fabs(movedir[i]) * fabs(bboxmaxs[i]);
			else
				dist += fabs(movedir[i]) * fabs(bboxmins[i]);
		}
		// calculate the goal origin
		VectorMA(origin, -dist, movedir, goalorigin);
		//
		VectorCopy(goalorigin, start);
		start[2] += 24;
		VectorCopy(start, end);
		end[2] -= 100;
		numareas = trap_AAS_TraceAreas(start, end, areas, NULL, 10);
		//
		for(i = 0; i < numareas; i++) {
			if(trap_AAS_AreaReachability(areas[i])) {
				break;
			}
		}
		if(i < numareas) {
			//
			VectorCopy(origin, activategoal->goal.origin);
			activategoal->goal.areanum = areas[i];
			VectorSubtract(mins, origin, activategoal->goal.mins);
			VectorSubtract(maxs, origin, activategoal->goal.maxs);
			//
			for(i = 0; i < 3; i++) {
				if(movedir[i] < 0)
					activategoal->goal.maxs[i] += fabs(movedir[i]) * fabs(extramaxs[i]);
				else
					activategoal->goal.mins[i] += fabs(movedir[i]) * fabs(extramins[i]);
			}  // end for
			//
			activategoal->goal.entitynum = entitynum;
			activategoal->goal.number = 0;
			activategoal->goal.flags = 0;
			return true;
		}
	}
	return false;
}

/*
==================
BotFuncDoorGoal
==================
*/
int BotFuncDoorActivateGoal(bot_state_t* bs, int bspent, bot_activategoal_t* activategoal) {
	int modelindex, entitynum;
	char model[MAX_INFO_STRING];
	vec3_t mins, maxs, origin;

	// shoot at the shootable door
	trap_AAS_ValueForBSPEpairKey(bspent, "model", model, sizeof(model));
	if(!*model) return false;
	modelindex = atoi(model + 1);
	if(!modelindex) return false;
	entitynum = BotModelMinsMaxs(modelindex, ET_MOVER, 0, mins, maxs);
	// door origin
	VectorAdd(mins, maxs, origin);
	VectorScale(origin, 0.5, origin);
	VectorCopy(origin, activategoal->target);
	activategoal->shoot = true;
	//
	activategoal->goal.entitynum = entitynum;  // NOTE: this is the entity number of the shootable door
	activategoal->goal.number = 0;
	activategoal->goal.flags = 0;
	VectorCopy(bs->origin, activategoal->goal.origin);
	activategoal->goal.areanum = bs->areanum;
	VectorSet(activategoal->goal.mins, -8, -8, -8);
	VectorSet(activategoal->goal.maxs, 8, 8, 8);
	return true;
}

/*
==================
BotTriggerMultipleGoal
==================
*/
int BotTriggerMultipleActivateGoal(bot_state_t* bs, int bspent, bot_activategoal_t* activategoal) {
	int i, areas[10], numareas, modelindex, entitynum;
	char model[128];
	vec3_t start, end, mins, maxs;
	vec3_t origin, goalorigin;

	activategoal->shoot = false;
	VectorClear(activategoal->target);
	// create a bot goal towards the trigger
	trap_AAS_ValueForBSPEpairKey(bspent, "model", model, sizeof(model));
	if(!*model) return false;
	modelindex = atoi(model + 1);
	if(!modelindex) return false;
	entitynum = BotModelMinsMaxs(modelindex, 0, CONTENTS_TRIGGER, mins, maxs);
	// trigger origin
	VectorAdd(mins, maxs, origin);
	VectorScale(origin, 0.5, origin);
	VectorCopy(origin, goalorigin);
	//
	VectorCopy(goalorigin, start);
	start[2] += 24;
	VectorCopy(start, end);
	end[2] -= 100;
	numareas = trap_AAS_TraceAreas(start, end, areas, NULL, 10);
	//
	for(i = 0; i < numareas; i++) {
		if(trap_AAS_AreaReachability(areas[i])) {
			break;
		}
	}
	if(i < numareas) {
		VectorCopy(origin, activategoal->goal.origin);
		activategoal->goal.areanum = areas[i];
		VectorSubtract(mins, origin, activategoal->goal.mins);
		VectorSubtract(maxs, origin, activategoal->goal.maxs);
		//
		activategoal->goal.entitynum = entitynum;
		activategoal->goal.number = 0;
		activategoal->goal.flags = 0;
		return true;
	}
	return false;
}

/*
==================
BotPopFromActivateGoalStack
==================
*/
int BotPopFromActivateGoalStack(bot_state_t* bs) {
	if(!bs->activatestack) return false;
	BotEnableActivateGoalAreas(bs->activatestack, true);
	bs->activatestack->inuse = false;
	bs->activatestack->justused_time = FloatTime();
	bs->activatestack = bs->activatestack->next;
	return true;
}

/*
==================
BotPushOntoActivateGoalStack
==================
*/
int BotPushOntoActivateGoalStack(bot_state_t* bs, bot_activategoal_t* activategoal) {
	int i, best;
	float besttime;

	best = -1;
	besttime = FloatTime() + 9999;
	//
	for(i = 0; i < MAX_ACTIVATESTACK; i++) {
		if(!bs->activategoalheap[i].inuse) {
			if(bs->activategoalheap[i].justused_time < besttime) {
				besttime = bs->activategoalheap[i].justused_time;
				best = i;
			}
		}
	}
	if(best != -1) {
		memcpy(&bs->activategoalheap[best], activategoal, sizeof(bot_activategoal_t));
		bs->activategoalheap[best].inuse = true;
		bs->activategoalheap[best].next = bs->activatestack;
		bs->activatestack = &bs->activategoalheap[best];
		return true;
	}
	return false;
}

/*
==================
BotClearActivateGoalStack
==================
*/
void BotClearActivateGoalStack(bot_state_t* bs) {
	while(bs->activatestack) BotPopFromActivateGoalStack(bs);
}

/*
==================
BotEnableActivateGoalAreas
==================
*/
void BotEnableActivateGoalAreas(bot_activategoal_t* activategoal, int enable) {
	int i;

	if(activategoal->areasdisabled == !enable) return;
	for(i = 0; i < activategoal->numareas; i++) trap_AAS_EnableRoutingArea(activategoal->areas[i], enable);
	activategoal->areasdisabled = !enable;
}

/*
==================
BotIsGoingToActivateEntity
==================
*/
int BotIsGoingToActivateEntity(bot_state_t* bs, int entitynum) {
	bot_activategoal_t* a;
	int i;

	for(a = bs->activatestack; a; a = a->next) {
		if(a->time < FloatTime()) continue;
		if(a->goal.entitynum == entitynum) return true;
	}
	for(i = 0; i < MAX_ACTIVATESTACK; i++) {
		if(bs->activategoalheap[i].inuse) continue;
		//
		if(bs->activategoalheap[i].goal.entitynum == entitynum) {
			// if the bot went for this goal less than 2 seconds ago
			if(bs->activategoalheap[i].justused_time > FloatTime() - 2) return true;
		}
	}
	return false;
}

/*
==================
BotGetActivateGoal

  returns the number of the bsp entity to activate
  goal->entitynum will be set to the game entity to activate
==================
*/
int BotGetActivateGoal(bot_state_t* bs, int entitynum, bot_activategoal_t* activategoal) {
	int i, ent, cur_entities[10], spawnflags, modelindex, areas[MAX_ACTIVATEAREAS * 2], numareas, t;
	char model[MAX_INFO_STRING], tmpmodel[128];
	char target[128], classname[128];
	float health;
	char targetname[10][128];
	aas_entityinfo_t entinfo;
	aas_areainfo_t areainfo;
	vec3_t origin, absmins, absmaxs;

	memset(activategoal, 0, sizeof(bot_activategoal_t));
	BotEntityInfo(entitynum, &entinfo);
	Com_sprintf(model, sizeof(model), "*%d", entinfo.modelindex);
	for(ent = trap_AAS_NextBSPEntity(0); ent; ent = trap_AAS_NextBSPEntity(ent)) {
		if(!trap_AAS_ValueForBSPEpairKey(ent, "model", tmpmodel, sizeof(tmpmodel))) continue;
		if(!strcmp(model, tmpmodel)) break;
	}
	if(!ent) {
		BotAI_Print(PRT_ERROR, "BotGetActivateGoal: no entity found with model %s\n", model);
		return 0;
	}
	trap_AAS_ValueForBSPEpairKey(ent, "classname", classname, sizeof(classname));
	if(!*classname) {
		BotAI_Print(PRT_ERROR, "BotGetActivateGoal: entity with model %s has no classname\n", model);
		return 0;
	}
	// if it is a door
	if(!strcmp(classname, "func_door")) {
		if(trap_AAS_FloatForBSPEpairKey(ent, "health", &health)) {
			// if the door has health then the door must be shot to open
			if(health) {
				BotFuncDoorActivateGoal(bs, ent, activategoal);
				return ent;
			}
		}
		//
		trap_AAS_IntForBSPEpairKey(ent, "spawnflags", &spawnflags);
		// if the door starts open then just wait for the door to return
		if(spawnflags & 1) return 0;
		// get the door origin
		if(!trap_AAS_VectorForBSPEpairKey(ent, "origin", origin)) {
			VectorClear(origin);
		}
		// if the door is open or opening already
		if(!VectorCompare(origin, entinfo.origin)) return 0;
		// store all the areas the door is in
		trap_AAS_ValueForBSPEpairKey(ent, "model", model, sizeof(model));
		if(*model) {
			modelindex = atoi(model + 1);
			if(modelindex) {
				BotModelMinsMaxs(modelindex, ET_MOVER, 0, absmins, absmaxs);
				//
				numareas = trap_AAS_BBoxAreas(absmins, absmaxs, areas, MAX_ACTIVATEAREAS * 2);
				// store the areas with reachabilities first
				for(i = 0; i < numareas; i++) {
					if(activategoal->numareas >= MAX_ACTIVATEAREAS) break;
					if(!trap_AAS_AreaReachability(areas[i])) {
						continue;
					}
					trap_AAS_AreaInfo(areas[i], &areainfo);
					if(areainfo.contents & AREACONTENTS_MOVER) {
						activategoal->areas[activategoal->numareas++] = areas[i];
					}
				}
				// store any remaining areas
				for(i = 0; i < numareas; i++) {
					if(activategoal->numareas >= MAX_ACTIVATEAREAS) break;
					if(trap_AAS_AreaReachability(areas[i])) {
						continue;
					}
					trap_AAS_AreaInfo(areas[i], &areainfo);
					if(areainfo.contents & AREACONTENTS_MOVER) {
						activategoal->areas[activategoal->numareas++] = areas[i];
					}
				}
			}
		}
	}
	// if the bot is blocked by or standing on top of a button
	if(!strcmp(classname, "func_button")) {
		return 0;
	}
	// get the targetname so we can find an entity with a matching target
	if(!trap_AAS_ValueForBSPEpairKey(ent, "targetname", targetname[0], sizeof(targetname[0]))) {
		if(bot_developer.integer) {
			BotAI_Print(PRT_ERROR, "BotGetActivateGoal: entity with model \"%s\" has no targetname\n", model);
		}
		return 0;
	}
	// allow tree-like activation
	cur_entities[0] = trap_AAS_NextBSPEntity(0);
	for(i = 0; i >= 0 && i < 10;) {
		for(ent = cur_entities[i]; ent; ent = trap_AAS_NextBSPEntity(ent)) {
			if(!trap_AAS_ValueForBSPEpairKey(ent, "target", target, sizeof(target))) continue;
			if(!strcmp(targetname[i], target)) {
				cur_entities[i] = trap_AAS_NextBSPEntity(ent);
				break;
			}
		}
		if(!ent) {
			if(bot_developer.integer) {
				BotAI_Print(PRT_ERROR, "BotGetActivateGoal: no entity with target \"%s\"\n", targetname[i]);
			}
			i--;
			continue;
		}
		if(!trap_AAS_ValueForBSPEpairKey(ent, "classname", classname, sizeof(classname))) {
			if(bot_developer.integer) {
				BotAI_Print(PRT_ERROR, "BotGetActivateGoal: entity with target \"%s\" has no classname\n", targetname[i]);
			}
			continue;
		}
		// BSP button model
		if(!strcmp(classname, "func_button")) {
			//
			if(!BotFuncButtonActivateGoal(bs, ent, activategoal)) continue;
			// if the bot tries to activate this button already
			if(bs->activatestack && bs->activatestack->inuse && bs->activatestack->goal.entitynum == activategoal->goal.entitynum && bs->activatestack->time > FloatTime() && bs->activatestack->start_time < FloatTime() - 2) continue;
			// if the bot is in a reachability area
			if(trap_AAS_AreaReachability(bs->areanum)) {
				// disable all areas the blocking entity is in
				BotEnableActivateGoalAreas(activategoal, false);
				//
				t = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, activategoal->goal.areanum, bs->tfl);
				// if the button is not reachable
				if(!t) {
					continue;
				}
				activategoal->time = FloatTime() + t * 0.01 + 5;
			}
			return ent;
		}
		// invisible trigger multiple box
		else if(!strcmp(classname, "trigger_multiple")) {
			//
			if(!BotTriggerMultipleActivateGoal(bs, ent, activategoal)) continue;
			// if the bot tries to activate this trigger already
			if(bs->activatestack && bs->activatestack->inuse && bs->activatestack->goal.entitynum == activategoal->goal.entitynum && bs->activatestack->time > FloatTime() && bs->activatestack->start_time < FloatTime() - 2) continue;
			// if the bot is in a reachability area
			if(trap_AAS_AreaReachability(bs->areanum)) {
				// disable all areas the blocking entity is in
				BotEnableActivateGoalAreas(activategoal, false);
				//
				t = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, activategoal->goal.areanum, bs->tfl);
				// if the trigger is not reachable
				if(!t) {
					continue;
				}
				activategoal->time = FloatTime() + t * 0.01 + 5;
			}
			return ent;
		} else if(!strcmp(classname, "func_timer")) {
			// just skip the func_timer
			continue;
		}
		// the actual button or trigger might be linked through a target_relay or target_delay
		else if(!strcmp(classname, "target_relay") || !strcmp(classname, "target_delay")) {
			if(trap_AAS_ValueForBSPEpairKey(ent, "targetname", targetname[i + 1], sizeof(targetname[0]))) {
				i++;
				cur_entities[i] = trap_AAS_NextBSPEntity(0);
			}
		}
	}
	return 0;
}

/*
==================
BotGoForActivateGoal
==================
*/
int BotGoForActivateGoal(bot_state_t* bs, bot_activategoal_t* activategoal) {
	aas_entityinfo_t activateinfo;

	activategoal->inuse = true;
	if(!activategoal->time) activategoal->time = FloatTime() + 10;
	activategoal->start_time = FloatTime();
	BotEntityInfo(activategoal->goal.entitynum, &activateinfo);
	VectorCopy(activateinfo.origin, activategoal->origin);
	//
	if(BotPushOntoActivateGoalStack(bs, activategoal)) {
		// enter the activate entity AI node
		AIEnter_Seek_ActivateEntity(bs, "BotGoForActivateGoal");
		return true;
	} else {
		// enable any routing areas that were disabled
		BotEnableActivateGoalAreas(activategoal, true);
		return false;
	}
}

/*
==================
BotRandomMove
==================
*/
void BotRandomMove(bot_state_t* bs, bot_moveresult_t* moveresult) {
	vec3_t dir, angles;

	angles[0] = 0;
	angles[1] = random() * 360;
	angles[2] = 0;
	AngleVectors(angles, dir, NULL, NULL);

	trap_BotMoveInDirection(bs->ms, dir, 400, MOVE_WALK);

	moveresult->failure = false;
	VectorCopy(dir, moveresult->movedir);
}

/*
==================
BotAIBlocked

Very basic handling of bots being blocked by other entities.
Check what kind of entity is blocking the bot and try to activate
it. If that's not an option then try to walk around or over the entity.
Before the bot ends in this part of the AI it should predict which doors to
open, which buttons to activate etc.
==================
*/
void BotAIBlocked(bot_state_t* bs, bot_moveresult_t* moveresult, int activate) {
	int bspent;
	vec3_t hordir, sideward, angles, up = {0, 0, 1};
	aas_entityinfo_t entinfo;
	bot_activategoal_t activategoal;

	// if the bot is not blocked by anything
	if(!moveresult->blocked) {
		bs->notblocked_time = FloatTime();
		return;
	}
	// if stuck in a solid area
	if(moveresult->type == RESULTTYPE_INSOLIDAREA) {
		// move in a random direction in the hope to get out
		BotRandomMove(bs, moveresult);
		//
		return;
	}
	// get info for the entity that is blocking the bot
	BotEntityInfo(moveresult->blockentity, &entinfo);
	// if blocked by a bsp model and the bot wants to activate it
	if(activate && entinfo.modelindex > 0 && entinfo.modelindex <= max_bspmodelindex) {
		// find the bsp entity which should be activated in order to get the blocking entity out of the way
		bspent = BotGetActivateGoal(bs, entinfo.number, &activategoal);
		if(bspent) {
			//
			if(bs->activatestack && !bs->activatestack->inuse) bs->activatestack = NULL;
			// if not already trying to activate this entity
			if(!BotIsGoingToActivateEntity(bs, activategoal.goal.entitynum)) {
				//
				BotGoForActivateGoal(bs, &activategoal);
			}
			// if ontop of an obstacle or
			// if the bot is not in a reachability area it'll still
			// need some dynamic obstacle avoidance, otherwise return
			if(!(moveresult->flags & MOVERESULT_ONTOPOFOBSTACLE) && trap_AAS_AreaReachability(bs->areanum)) return;
		} else {
			// enable any routing areas that were disabled
			BotEnableActivateGoalAreas(&activategoal, true);
		}
	}
	// just some basic dynamic obstacle avoidance code
	hordir[0] = moveresult->movedir[0];
	hordir[1] = moveresult->movedir[1];
	hordir[2] = 0;
	// if no direction just take a random direction
	if(VectorNormalize(hordir) < 0.1) {
		VectorSet(angles, 0, 360 * random(), 0);
		AngleVectors(angles, hordir, NULL, NULL);
	}
	// get the sideward vector
	CrossProduct(hordir, up, sideward);
	//
	if(bs->flags & BFL_AVOIDRIGHT) VectorNegate(sideward, sideward);
	// if walking straight forward fails, strafe around the obstacle
	if(!trap_BotMoveInDirection(bs->ms, hordir, 400, MOVE_WALK)) {
		// perform the movement
		if(!trap_BotMoveInDirection(bs->ms, sideward, 400, MOVE_WALK)) {
			// flip the avoid direction flag
			bs->flags ^= BFL_AVOIDRIGHT;
			// flip the direction
			VectorMA(sideward, -1, hordir, sideward);
			// move in the other direction
			trap_BotMoveInDirection(bs->ms, sideward, 400, MOVE_WALK);
		}
	}
	//
	if(bs->notblocked_time < FloatTime() - 0.4) {
		// just reset goals and hope the bot will go into another direction?
		if(bs->ainode == AINode_Seek_NBG)
			bs->nbg_time = 0;
		else if(bs->ainode == AINode_Seek_LTG)
			bs->ltg_time = 0;
	}
}

/*
==================
BotAIPredictObstacles

Predict the route towards the goal and check if the bot
will be blocked by certain obstacles. When the bot has obstacles
on its path the bot should figure out if they can be removed
by activating certain entities.
==================
*/
int BotAIPredictObstacles(bot_state_t* bs, bot_goal_t* goal) {
	int modelnum, entitynum, bspent;
	bot_activategoal_t activategoal;
	aas_predictroute_t route;

	if(!bot_predictobstacles.integer) return false;

	// always predict when the goal change or at regular intervals
	if(bs->predictobstacles_goalareanum == goal->areanum && bs->predictobstacles_time > FloatTime() - 6) {
		return false;
	}
	bs->predictobstacles_goalareanum = goal->areanum;
	bs->predictobstacles_time = FloatTime();

	// predict at most 100 areas or 1 second ahead
	trap_AAS_PredictRoute(&route, bs->areanum, bs->origin, goal->areanum, bs->tfl, 100, 1000, RSE_USETRAVELTYPE | RSE_ENTERCONTENTS, AREACONTENTS_MOVER, TFL_BRIDGE, 0);
	// if bot has to travel through an area with a mover
	if(route.stopevent & RSE_ENTERCONTENTS) {
		// if the bot will run into a mover
		if(route.endcontents & AREACONTENTS_MOVER) {
			// NOTE: this only works with bspc 2.1 or higher
			modelnum = (route.endcontents & AREACONTENTS_MODELNUM) >> AREACONTENTS_MODELNUMSHIFT;
			if(modelnum) {
				//
				entitynum = BotModelMinsMaxs(modelnum, ET_MOVER, 0, NULL, NULL);
				if(entitynum) {
					// NOTE: BotGetActivateGoal already checks if the door is open or not
					bspent = BotGetActivateGoal(bs, entitynum, &activategoal);
					if(bspent) {
						//
						if(bs->activatestack && !bs->activatestack->inuse) bs->activatestack = NULL;
						// if not already trying to activate this entity
						if(!BotIsGoingToActivateEntity(bs, activategoal.goal.entitynum)) {
							//
							BotGoForActivateGoal(bs, &activategoal);
							return true;
						} else {
							// enable any routing areas that were disabled
							BotEnableActivateGoalAreas(&activategoal, true);
						}
					}
				}
			}
		}
	}
	return false;
}

/*
==================
BotCheckAir
==================
*/
void BotCheckAir(bot_state_t* bs) {
	if(trap_AAS_PointContents(bs->eye) & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
		return;
	}
	bs->lastair_time = FloatTime();
}

/*
==================
BotAvoidMissiles

Clears the avoid spots and adds one for every missile around so the
bot tries to dodge them.
==================
*/
void BotAvoidMissiles(bot_state_t* bs) {
	int i;
	gentity_t* ent;

	trap_BotAddAvoidSpot(bs->ms, vec3_origin, 0, AVOID_CLEAR);
	//
	for(i = 0; i < level.num_entities; i++) {
		ent = &g_entities[i];
		if(!ent->inuse) continue;
		if(ent->s.eType != ET_MISSILE) continue;
		// try to avoid the missile
		trap_BotAddAvoidSpot(bs->ms, ent->s.pos.trBase, 160, AVOID_ALWAYS);
	}
}

/*
==================
BotStartHunt

Remember a player for 180 seconds and track them down where they were
last seen or heard.
==================
*/
void BotStartHunt(bot_state_t* bs, int client) {
	bs->hunt_enemy = client;
	bs->hunt_expire_time = FloatTime() + 180;
	bs->hunt_traildelay = 30000;
	bs->hunt_trail_time = 0;
	bs->hunt_probe_time = FloatTime() + 60;
	bs->hunt_havegoal = false;
}

/*
==================
BotHuntTargetAlive

true if the remembered hunt target is still connected and alive
==================
*/
bool BotHuntTargetAlive(bot_state_t* bs) {
	aas_entityinfo_t entinfo;

	if(bs->hunt_enemy < 0 || bs->hunt_enemy >= MAX_CLIENTS) return false;
	if(g_entities[bs->hunt_enemy].client->pers.connected != CON_CONNECTED) return false;
	BotEntityInfo(bs->hunt_enemy, &entinfo);
	return (entinfo.valid && !EntityIsDead(&entinfo));
}

/*
==================
BotListenSounds

Bots hear footsteps, falls and gunshots of players within 2500 units.
A heard player is remembered and hunted; the bot immediately walks to
the place the sound came from.
==================
*/
void BotListenSounds(bot_state_t* bs) {
	int id, total, client, areanum;
	vec3_t origin, dir;

	total = BotTotalSounds();
	for(id = bs->sound_id; id < total; id++) {
		BotGetSound(id, &client, origin, &areanum);
		if(client == bs->client) continue;
		VectorSubtract(origin, bs->origin, dir);
		// too far away to hear
		if(VectorLengthSquared(dir) > Square(2500)) continue;
		// remember the sound for a couple of seconds
		bs->sound_time = FloatTime();
		VectorCopy(origin, bs->sound_origin);
		bs->sound_areanum = areanum;
		// while fighting the bot stays on its enemy, the sound is just remembered
		if(bs->enemy >= 0) continue;
		// no hunt yet: the sound starts one on this player
		if(bs->hunt_expire_time < FloatTime()) {
			BotStartHunt(bs, client);
		}
		// fresh evidence of the hunted player extends the memory
		else if(bs->hunt_enemy == client) {
			bs->hunt_expire_time = FloatTime() + 180;
		}
		// sounds from other players only pull the bot away for a moment,
		// the memory of the hunted player is kept
		VectorCopy(origin, bs->hunt_goalorigin);
		bs->hunt_goalareanum = areanum;
		bs->hunt_havegoal = true;
		bs->hunt_trail_time = FloatTime() + 10;
	}
	bs->sound_id = total;
}

/*
==================
BotDeathmatchAI
==================
*/
void BotDeathmatchAI(bot_state_t* bs, float thinktime) {
	int i;

	// if the bot has just been setup
	if(bs->setupcount > 0) {
		bs->setupcount--;
		if(bs->setupcount > 0) return;
		//
		bs->lasthealth = bs->cur_ps.stats[STAT_HEALTH];
		//
		bs->setupcount = 0;
	}
	// no ideal view set
	bs->flags &= ~BFL_IDEALVIEWSET;

	// set the teleport time
	BotSetTeleportTime(bs);
	// listen to the players: footsteps, falls and gunshots
	BotListenSounds(bs);
	// dodge missiles around the bot
	BotAvoidMissiles(bs);
	// check for air
	BotCheckAir(bs);

	// if the bot has no ai node
	if(!bs->ainode) {
		AIEnter_Seek_LTG(bs, "BotDeathmatchAI: no ai node");
	}

	// reset the node switches from the previous frame
	BotResetNodeSwitches();
	// execute AI nodes
	for(i = 0; i < MAX_NODESWITCHES; i++) {
		if(bs->ainode(bs)) break;
	}
	// if the bot removed itself :)
	if(!bs->inuse) return;
	// if the bot executed too many AI nodes
	if(i >= MAX_NODESWITCHES) {
		BotDumpNodeSwitches(bs);
		BotAI_Print(PRT_ERROR, "bot at %1.1f switched more than %d AI nodes\n", FloatTime(), MAX_NODESWITCHES);
	}
	//
	bs->lasthealth = bs->cur_ps.stats[STAT_HEALTH];
}

/*
==================
BotSetupDeathmatchAI
==================
*/
void BotSetupDeathmatchAI(void) {
	int ent, modelnum;
	char model[128];

	trap_Cvar_Register(&bot_challenge, "bot_challenge", "0", 0);
	trap_Cvar_Register(&bot_predictobstacles, "bot_predictobstacles", "1", 0);

	max_bspmodelindex = 0;
	for(ent = trap_AAS_NextBSPEntity(0); ent; ent = trap_AAS_NextBSPEntity(ent)) {
		if(!trap_AAS_ValueForBSPEpairKey(ent, "model", model, sizeof(model))) continue;
		if(model[0] == '*') {
			modelnum = atoi(model + 1);
			if(modelnum > max_bspmodelindex) max_bspmodelindex = modelnum;
		}
	}
}

/*
==================
BotShutdownDeathmatchAI
==================
*/
void BotShutdownDeathmatchAI(void) {}

