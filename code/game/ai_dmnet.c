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
 * name:		ai_dmnet.c
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

int numnodeswitches;
char nodeswitch[MAX_NODESWITCHES + 1][144];

/*
==================
BotResetNodeSwitches
==================
*/
void BotResetNodeSwitches(void) { numnodeswitches = 0; }

/*
==================
BotDumpNodeSwitches
==================
*/
void BotDumpNodeSwitches(bot_state_t* bs) {
	int i;

	BotAI_Print(PRT_MESSAGE, "switched more than %d AI nodes\n", MAX_NODESWITCHES);
	for(i = 0; i < numnodeswitches; i++) {
		BotAI_Print(PRT_MESSAGE, "%s", nodeswitch[i]);
	}
	BotAI_Print(PRT_FATAL, "");
}

/*
==================
BotRecordNodeSwitch
==================
*/
void BotRecordNodeSwitch(bot_state_t* bs, char* node, char* str, char* s) {
	Com_sprintf(nodeswitch[numnodeswitches], 144, "at %2.1f entered %s: %s from %s\n", FloatTime(), node, str, s);
	numnodeswitches++;
}

/*
==================
BotGetAirGoal
==================
*/
static int BotGetAirGoal(bot_state_t* bs, bot_goal_t* goal) {
	bsp_trace_t bsptrace;
	vec3_t end, mins = {-15, -15, -2}, maxs = {15, 15, 2};
	int areanum;

	// trace up until we hit solid
	VectorCopy(bs->origin, end);
	end[2] += 1000;
	BotAI_Trace(&bsptrace, bs->origin, mins, maxs, end, bs->entitynum, CONTENTS_SOLID | CONTENTS_PLAYERCLIP);
	// trace down until we hit water
	VectorCopy(bsptrace.endpos, end);
	BotAI_Trace(&bsptrace, end, mins, maxs, bs->origin, bs->entitynum, CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA);
	// if we found the water surface
	if(bsptrace.fraction > 0) {
		areanum = BotPointAreaNum(bsptrace.endpos);
		if(areanum) {
			VectorCopy(bsptrace.endpos, goal->origin);
			goal->origin[2] -= 2;
			goal->areanum = areanum;
			goal->mins[0] = -15;
			goal->mins[1] = -15;
			goal->mins[2] = -1;
			goal->maxs[0] = 15;
			goal->maxs[1] = 15;
			goal->maxs[2] = 1;
			goal->flags = GFL_AIR;
			goal->number = 0;
			goal->iteminfo = 0;
			goal->entitynum = 0;
			return true;
		}
	}
	return false;
}

/*
==================
BotGoForAir

Sets a nearby air goal as the nearby goal, the caller switches to the
NBG node when this returns true.
==================
*/
static int BotGoForAir(bot_state_t* bs) {
	bot_goal_t goal;

	// if the bot doesn't need air
	if(bs->lastair_time >= FloatTime() - 6) return false;
	// if we can find an air goal
	if(BotGetAirGoal(bs, &goal)) {
		memcpy(&bs->nbg_goal, &goal, sizeof(bot_goal_t));
		bs->nbg_time = FloatTime() + 8;
		return true;
	}
	return false;
}

/*
==================
BotItemGoalValid

Returns true if the item entity the goal points to is still in the level.
==================
*/
static bool BotItemGoalValid(bot_goal_t* goal) {
	gentity_t* ent;

	if(goal->entitynum < MAX_CLIENTS || goal->entitynum >= level.num_entities) return false;
	ent = &g_entities[goal->entitynum];
	if(!ent->inuse) return false;
	if(!BG_IsItemEntity(&ent->s) && ent->s.eType != ET_ITEM) return false;
	// the item was picked up and is waiting for its respawn
	if(!ent->r.linked || (ent->r.svFlags & SVF_NOCLIENT)) return false;
	return true;
}

/*
==================
BotPickItemGoal

Finds the item with the least travel time towards it. Items are dynamic
and come from JS, so every item is treated exactly the same: the closest
reachable one wins. If maxtravel is 0 the travel time is unlimited.
==================
*/
static bool BotPickItemGoal(bot_state_t* bs, bot_goal_t* goal, int tfl, float maxtravel) {
	gentity_t* ent;
	bot_goal_t bestgoal;
	int i, areanum, travel, besttravel;
	bool found;

	found = false;
	besttravel = maxtravel > 0 ? (int)maxtravel : 999999;
	for(i = MAX_CLIENTS; i < level.num_entities; i++) {
		ent = &g_entities[i];
		if(!ent->inuse) continue;
		if(!BG_IsItemEntity(&ent->s) && ent->s.eType != ET_ITEM) continue;
		// items waiting for their respawn are not in the level
		if(!ent->r.linked || (ent->r.svFlags & SVF_NOCLIENT)) continue;
		//
		areanum = BotPointAreaNum(ent->r.currentOrigin);
		if(!areanum || !trap_AAS_AreaReachability(areanum)) continue;
		travel = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, areanum, tfl);
		if(!travel || travel >= besttravel) continue;
		besttravel = travel;
		//
		memset(&bestgoal, 0, sizeof(bot_goal_t));
		bestgoal.entitynum = i;
		bestgoal.number = i;
		bestgoal.areanum = areanum;
		VectorCopy(ent->r.currentOrigin, bestgoal.origin);
		VectorSet(bestgoal.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
		VectorSet(bestgoal.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
		found = true;
	}
	if(!found) return false;
	memcpy(goal, &bestgoal, sizeof(bot_goal_t));
	return true;
}

/*
==================
BotNearbyGoal
==================
*/
int BotNearbyGoal(bot_state_t* bs, int tfl, bot_goal_t* ltg, float range) {
	bot_goal_t goal;

	// check if the bot should go for air
	if(BotGoForAir(bs)) return true;
	// grab a nearby item if there is one the bot is not already going for
	if(!BotPickItemGoal(bs, &goal, tfl, range)) return false;
	if(goal.entitynum == ltg->entitynum) return false;
	memcpy(&bs->nbg_goal, &goal, sizeof(bot_goal_t));
	bs->nbg_time = FloatTime() + 4 + range * 0.01;
	return true;
}

/*
==================
BotReachedGoal
==================
*/
int BotReachedGoal(bot_state_t* bs, bot_goal_t* goal) {
	// if touching the goal
	if(trap_BotTouchingGoal(bs->origin, goal)) return true;
	if(goal->flags & GFL_AIR) {
		// if the bot got air
		if(bs->lastair_time > FloatTime() - 1) return true;
	} else {
		// if in the goal area and within the goal bounds and not swimming
		if(bs->areanum == goal->areanum) {
			if(bs->origin[0] > goal->origin[0] + goal->mins[0] && bs->origin[0] < goal->origin[0] + goal->maxs[0]) {
				if(bs->origin[1] > goal->origin[1] + goal->mins[1] && bs->origin[1] < goal->origin[1] + goal->maxs[1]) {
					if(!trap_AAS_Swimming(bs->origin)) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

/*
==================
BotGetItemLongTermGoal

All items are equal, so the long term goal is simply the reachable item
with the least travel time. The goal is kept until it is picked up, it
disappears or the bot looks for a new one.
==================
*/
int BotGetItemLongTermGoal(bot_state_t* bs, int tfl, bot_goal_t* goal) {
	// if the current goal item is still there and not reached yet
	if(BotItemGoalValid(&bs->ltg_goal) && !BotReachedGoal(bs, &bs->ltg_goal)) {
		// if it is not time to look for a new goal yet, keep going for it
		if(bs->ltg_time > FloatTime()) {
			memcpy(goal, &bs->ltg_goal, sizeof(bot_goal_t));
			return true;
		}
	}
	// choose a new long term goal item
	if(BotPickItemGoal(bs, &bs->ltg_goal, tfl, 0)) {
		bs->ltg_time = FloatTime() + 20;
		memcpy(goal, &bs->ltg_goal, sizeof(bot_goal_t));
		return true;
	}
	// no reachable item right now, retry soon
	bs->ltg_time = FloatTime() + 1;
	return false;
}

/*
==================
AIEnter_Respawn
==================
*/
void AIEnter_Respawn(bot_state_t* bs, char* s) {
	BotRecordNodeSwitch(bs, "respawn", "", s);
	// reset some states
	trap_BotResetMoveState(bs->ms);
	trap_BotResetAvoidReach(bs->ms);
	//
	bs->respawn_time = FloatTime() + 1 + random();
	// set respawn state
	bs->respawn_wait = false;
	bs->ainode = AINode_Respawn;
}

/*
==================
AINode_Respawn
==================
*/
int AINode_Respawn(bot_state_t* bs) {
	// if waiting for the actual respawn
	if(bs->respawn_wait) {
		if(!BotIsDead(bs)) {
			AIEnter_Seek_LTG(bs, "respawn: respawned");
		} else {
			trap_EA_Respawn(bs->client);
		}
	} else if(bs->respawn_time < FloatTime()) {
		// wait until respawned
		bs->respawn_wait = true;
		// elementary action respawn
		trap_EA_Respawn(bs->client);
	}
	//
	return true;
}

/*
==================
BotClearPath

 try to deactivate obstacles like proximity mines on the bot's path
==================
*/
void BotClearPath(bot_state_t* bs, bot_moveresult_t* moveresult) {
	if(moveresult->flags & MOVERESULT_BLOCKEDBYAVOIDSPOT) {
		bs->blockedbyavoidspot_time = FloatTime() + 5;
	}
}

/*
==================
AIEnter_Seek_ActivateEntity
==================
*/
void AIEnter_Seek_ActivateEntity(bot_state_t* bs, char* s) {
	BotRecordNodeSwitch(bs, "activate entity", "", s);
	bs->ainode = AINode_Seek_ActivateEntity;
}

/*
==================
AINode_Seek_ActivateEntity
==================
*/
int AINode_Seek_ActivateEntity(bot_state_t* bs) {
	bot_goal_t* goal;
	vec3_t target, dir, ideal_viewangles;
	bot_moveresult_t moveresult;
	int targetvisible;
	bsp_trace_t bsptrace;
	aas_entityinfo_t entinfo;

	// respawn if dead
	if(BotIsDead(bs)) {
		BotClearActivateGoalStack(bs);
		AIEnter_Respawn(bs, "activate entity: bot dead");
		return false;
	}
	//
	bs->tfl = TFL_DEFAULT;
	// if in lava or slime the bot should be able to get out
	if(BotInLavaOrSlime(bs)) bs->tfl |= TFL_LAVA | TFL_SLIME;
	// no enemy
	bs->enemy = -1;
	// if the bot has no activate goal
	if(!bs->activatestack) {
		BotClearActivateGoalStack(bs);
		AIEnter_Seek_NBG(bs, "activate entity: no goal");
		return false;
	}
	//
	goal = &bs->activatestack->goal;
	// initialize target being visible to false
	targetvisible = false;
	// if the bot has to shoot at a target to activate something
	if(bs->activatestack->shoot) {
		//
		BotAI_Trace(&bsptrace, bs->eye, NULL, NULL, bs->activatestack->target, bs->entitynum, MASK_SHOT);
		// if the shootable entity is visible from the current position
		if(bsptrace.fraction >= 1.0 || bsptrace.ent == goal->entitynum) {
			targetvisible = true;
			// if holding the right weapon
			if(bs->cur_ps.weapon == bs->activatestack->weapon) {
				VectorSubtract(bs->activatestack->target, bs->eye, dir);
				vectoangles(dir, ideal_viewangles);
				// if the bot is pretty close with its aim
				if(InFieldOfVision(bs->viewangles, 20, ideal_viewangles)) {
					trap_EA_Attack(bs->client);
				}
			}
		}
	}
	// if the shoot target is visible
	if(targetvisible) {
		// get the entity info of the entity the bot is shooting at
		BotEntityInfo(goal->entitynum, &entinfo);
		// if the entity the bot shoots at moved
		if(!VectorCompare(bs->activatestack->origin, entinfo.origin)) {
			bs->activatestack->time = 0;
		}
		// if the activate goal has been activated or the bot takes too long
		if(bs->activatestack->time < FloatTime()) {
			BotPopFromActivateGoalStack(bs);
			// if there are more activate goals on the stack
			if(bs->activatestack) {
				bs->activatestack->time = FloatTime() + 10;
				return false;
			}
			AIEnter_Seek_NBG(bs, "activate entity: time out");
			return false;
		}
		memset(&moveresult, 0, sizeof(bot_moveresult_t));
	} else {
		// if the bot does not have a shoot goal
		if(!bs->activatestack->shoot) {
			// if the bot touches the current goal
			if(trap_BotTouchingGoal(bs->origin, goal)) {
				bs->activatestack->time = 0;
			}
		}
		// if the activate goal has been activated or the bot takes too long
		if(bs->activatestack->time < FloatTime()) {
			BotPopFromActivateGoalStack(bs);
			// if there are more activate goals on the stack
			if(bs->activatestack) {
				bs->activatestack->time = FloatTime() + 10;
				return false;
			}
			AIEnter_Seek_NBG(bs, "activate entity: activated");
			return false;
		}
		// predict obstacles
		if(BotAIPredictObstacles(bs, goal)) return false;
		// initialize the movement state
		BotSetupForMovement(bs);
		// move towards the goal
		trap_BotMoveToGoal(&moveresult, bs->ms, goal, bs->tfl);
		// if the movement failed
		if(moveresult.failure) {
			// reset the avoid reach, otherwise bot is stuck in current area
			trap_BotResetAvoidReach(bs->ms);
			//
			bs->activatestack->time = 0;
		}
		// check if the bot is blocked
		BotAIBlocked(bs, &moveresult, true);
	}
	//
	BotClearPath(bs, &moveresult);
	// if the bot has to shoot to activate
	if(bs->activatestack->shoot) {
		// if the view angles aren't yet used for the movement
		if(!(moveresult.flags & MOVERESULT_MOVEMENTVIEW)) {
			VectorSubtract(bs->activatestack->target, bs->eye, dir);
			vectoangles(dir, moveresult.ideal_viewangles);
			moveresult.flags |= MOVERESULT_MOVEMENTVIEW;
		}
		// if there's no weapon yet used for the movement
		if(!(moveresult.flags & MOVERESULT_MOVEMENTWEAPON)) {
			moveresult.flags |= MOVERESULT_MOVEMENTWEAPON;
			//
			bs->activatestack->weapon = BotBestWeapon(bs);
			moveresult.weapon = bs->activatestack->weapon;
		}
	}
	// if the ideal view angles are set for movement
	if(moveresult.flags & (MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
	}
	// if waiting for something
	else if(moveresult.flags & MOVERESULT_WAITING) {
		if(random() < bs->thinktime * 0.8) {
			BotRoamGoal(bs, target);
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else if(!(bs->flags & BFL_IDEALVIEWSET)) {
		if(trap_BotMovementViewTarget(bs->ms, goal, bs->tfl, 300, target)) {
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
		} else {
			vectoangles(moveresult.movedir, bs->ideal_viewangles);
		}
		bs->ideal_viewangles[2] *= 0.5;
	}
	// if the weapon is used for the bot movement
	if(moveresult.flags & MOVERESULT_MOVEMENTWEAPON) bs->weaponnum = moveresult.weapon;
	// if there is an enemy
	if(BotFindEnemy(bs, -1)) {
		if(BotWantsToRetreat(bs)) {
			// keep the current long term goal and retreat
			AIEnter_Battle_NBG(bs, "activate entity: found enemy");
		} else {
			trap_BotResetLastAvoidReach(bs->ms);
			// go fight
			AIEnter_Battle_Fight(bs, "activate entity: found enemy");
		}
		BotClearActivateGoalStack(bs);
	}
	return true;
}

/*
==================
AIEnter_Seek_NBG
==================
*/
void AIEnter_Seek_NBG(bot_state_t* bs, char* s) {
	BotRecordNodeSwitch(bs, "seek NBG", "item", s);
	bs->ainode = AINode_Seek_NBG;
}

/*
==================
AINode_Seek_NBG
==================
*/
int AINode_Seek_NBG(bot_state_t* bs) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;

	// respawn if dead
	if(BotIsDead(bs)) {
		AIEnter_Respawn(bs, "seek nbg: bot dead");
		return false;
	}
	//
	bs->tfl = TFL_DEFAULT;
	// if in lava or slime the bot should be able to get out
	if(BotInLavaOrSlime(bs)) bs->tfl |= TFL_LAVA | TFL_SLIME;
	// no enemy
	bs->enemy = -1;
	//
	memcpy(&goal, &bs->nbg_goal, sizeof(bot_goal_t));
	// if the goal item is gone or touched, stop going for it
	if(goal.flags & GFL_AIR) {
		if(BotReachedGoal(bs, &goal)) bs->nbg_time = 0;
	} else {
		if(!BotItemGoalValid(&goal) || BotReachedGoal(bs, &goal)) bs->nbg_time = 0;
	}
	//
	if(bs->nbg_time < FloatTime()) {
		// go back to seek ltg
		AIEnter_Seek_LTG(bs, "seek nbg: time out");
		return false;
	}
	// predict obstacles
	if(BotAIPredictObstacles(bs, &goal)) return false;
	// initialize the movement state
	BotSetupForMovement(bs);
	// move towards the goal
	trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	// if the movement failed
	if(moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		bs->nbg_time = 0;
	}
	// check if the bot is blocked
	BotAIBlocked(bs, &moveresult, true);
	//
	BotClearPath(bs, &moveresult);
	// if the viewangles are used for the movement
	if(moveresult.flags & (MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
	}
	// if waiting for something
	else if(moveresult.flags & MOVERESULT_WAITING) {
		if(random() < bs->thinktime * 0.8) {
			BotRoamGoal(bs, target);
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else if(!(bs->flags & BFL_IDEALVIEWSET)) {
		if(trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, 300, target)) {
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
		} else {
			vectoangles(moveresult.movedir, bs->ideal_viewangles);
		}
		bs->ideal_viewangles[2] *= 0.5;
	}
	// if the weapon is used for the bot movement
	if(moveresult.flags & MOVERESULT_MOVEMENTWEAPON) bs->weaponnum = moveresult.weapon;
	// if there is an enemy
	if(BotFindEnemy(bs, -1)) {
		if(BotWantsToRetreat(bs)) {
			// keep the current long term goal and retreat
			AIEnter_Battle_NBG(bs, "seek nbg: found enemy");
		} else {
			trap_BotResetLastAvoidReach(bs->ms);
			// go fight
			AIEnter_Battle_Fight(bs, "seek nbg: found enemy");
		}
	}
	return true;
}

/*
==================
AIEnter_Seek_LTG
==================
*/
void AIEnter_Seek_LTG(bot_state_t* bs, char* s) {
	BotRecordNodeSwitch(bs, "seek LTG", "item", s);
	bs->ainode = AINode_Seek_LTG;
}

/*
==================
AINode_Seek_LTG
==================
*/
int AINode_Seek_LTG(bot_state_t* bs) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	int range;

	// respawn if dead
	if(BotIsDead(bs)) {
		AIEnter_Respawn(bs, "seek ltg: bot dead");
		return false;
	}
	// resume hunting a remembered player
	if(bs->hunt_expire_time > FloatTime() && BotHuntTargetAlive(bs)) {
		AIEnter_Battle_Hunt(bs, "seek ltg: hunt target");
		return false;
	}

	bs->tfl = TFL_DEFAULT;
	// if in lava or slime the bot should be able to get out
	if(BotInLavaOrSlime(bs)) bs->tfl |= TFL_LAVA | TFL_SLIME;
	// no enemy
	bs->enemy = -1;
	// if there is an enemy
	if(BotFindEnemy(bs, -1)) {
		if(BotWantsToRetreat(bs)) {
			// keep the current long term goal and retreat
			AIEnter_Battle_Retreat(bs, "seek ltg: found enemy");
			return false;
		} else {
			trap_BotResetLastAvoidReach(bs->ms);
			// go fight
			AIEnter_Battle_Fight(bs, "seek ltg: found enemy");
			return false;
		}
	}
	// get the current long term goal
	if(!BotGetItemLongTermGoal(bs, bs->tfl, &goal)) {
		return true;
	}
	// check for nearby goals periodicly
	if(bs->check_time < FloatTime()) {
		bs->check_time = FloatTime() + 0.5;
		range = 150;
		if(BotNearbyGoal(bs, bs->tfl, &goal, range)) {
			trap_BotResetLastAvoidReach(bs->ms);
			// time the bot gets to pick up the nearby goal item
			AIEnter_Seek_NBG(bs, "ltg seek: nbg");
			return false;
		}
	}
	// predict obstacles
	if(BotAIPredictObstacles(bs, &goal)) return false;
	// initialize the movement state
	BotSetupForMovement(bs);
	// move towards the goal
	trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	// if the movement failed
	if(moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		bs->ltg_time = 0;
	}
	//
	BotAIBlocked(bs, &moveresult, true);
	//
	BotClearPath(bs, &moveresult);
	// if the viewangles are used for the movement
	if(moveresult.flags & (MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
	}
	// if waiting for something
	else if(moveresult.flags & MOVERESULT_WAITING) {
		if(random() < bs->thinktime * 0.8) {
			BotRoamGoal(bs, target);
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else if(!(bs->flags & BFL_IDEALVIEWSET)) {
		if(trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, 300, target)) {
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
		} else if(VectorLengthSquared(moveresult.movedir)) {
			vectoangles(moveresult.movedir, bs->ideal_viewangles);
		} else if(random() < bs->thinktime * 0.8) {
			BotRoamGoal(bs, target);
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
			bs->ideal_viewangles[2] *= 0.5;
		}
		bs->ideal_viewangles[2] *= 0.5;
	}
	// if the weapon is used for the bot movement
	if(moveresult.flags & MOVERESULT_MOVEMENTWEAPON) bs->weaponnum = moveresult.weapon;
	//
	return true;
}

/*
==================
AIEnter_Battle_Fight
==================
*/
void AIEnter_Battle_Fight(bot_state_t* bs, char* s) {
	BotRecordNodeSwitch(bs, "battle fight", "", s);
	trap_BotResetLastAvoidReach(bs->ms);
	bs->ainode = AINode_Battle_Fight;
	bs->flags &= ~BFL_FIGHTSUICIDAL;
}

/*
==================
AIEnter_Battle_SuicidalFight
==================
*/
void AIEnter_Battle_SuicidalFight(bot_state_t* bs, char* s) {
	BotRecordNodeSwitch(bs, "battle fight", "", s);
	trap_BotResetLastAvoidReach(bs->ms);
	bs->ainode = AINode_Battle_Fight;
	bs->flags |= BFL_FIGHTSUICIDAL;
}

/*
==================
AINode_Battle_Fight
==================
*/
int AINode_Battle_Fight(bot_state_t* bs) {
	int areanum;
	vec3_t target;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;

	// respawn if dead
	if(BotIsDead(bs)) {
		AIEnter_Respawn(bs, "battle fight: bot dead");
		return false;
	}
	// if there is another better enemy
	BotFindEnemy(bs, bs->enemy);
	// if no enemy
	if(bs->enemy < 0) {
		AIEnter_Seek_LTG(bs, "battle fight: no enemy");
		return false;
	}
	//
	BotEntityInfo(bs->enemy, &entinfo);
	// if the enemy is dead
	if(bs->enemydeath_time) {
		if(bs->enemydeath_time < FloatTime() - 1.0) {
			bs->enemydeath_time = 0;
			bs->ltg_time = 0;
			AIEnter_Seek_LTG(bs, "battle fight: enemy dead");
			return false;
		}
	} else {
		if(EntityIsDead(&entinfo)) {
			bs->enemydeath_time = FloatTime();
		}
	}
	// fighting the hunted player refreshes the memory
	if(bs->enemy == bs->hunt_enemy) {
		bs->hunt_expire_time = FloatTime() + 180;
	}
	//
	VectorCopy(entinfo.origin, target);
	// update the reachability area and origin if possible
	areanum = BotPointAreaNum(target);
	if(areanum && trap_AAS_AreaReachability(areanum)) {
		VectorCopy(target, bs->lastenemyorigin);
		bs->lastenemyareanum = areanum;
	}

	// if the enemy is not visible
	if(!BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy)) {
		// remember the enemy even if the bot does not want to chase now
		BotStartHunt(bs, bs->enemy);
		if(BotWantsToChase(bs)) {
			AIEnter_Battle_Chase(bs, "battle fight: enemy out of sight");
			return false;
		} else {
			AIEnter_Seek_LTG(bs, "battle fight: enemy out of sight");
			return false;
		}
	}
	//
	bs->tfl = TFL_DEFAULT;
	// if in lava or slime the bot should be able to get out
	if(BotInLavaOrSlime(bs)) bs->tfl |= TFL_LAVA | TFL_SLIME;
	// choose the best weapon to fight with
	BotChooseWeapon(bs);
	// combat movement: occupy a position and shoot from it
	moveresult = BotCombatMove(bs, bs->tfl);
	// if the movement failed
	if(moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		bs->ltg_time = 0;
	}
	//
	BotAIBlocked(bs, &moveresult, false);
	// aim at the enemy
	BotAimAtEnemy(bs);
	// attack the enemy if possible
	BotCheckAttack(bs);
	// if the bot wants to retreat
	if(!(bs->flags & BFL_FIGHTSUICIDAL)) {
		if(BotWantsToRetreat(bs)) {
			AIEnter_Battle_Retreat(bs, "battle fight: wants to retreat");
			return true;
		}
	}
	return true;
}

/*
==================
AIEnter_Battle_Chase
==================
*/
void AIEnter_Battle_Chase(bot_state_t* bs, char* s) {
	BotRecordNodeSwitch(bs, "battle chase", "", s);
	bs->chase_time = FloatTime();
	bs->ainode = AINode_Battle_Chase;
}

/*
==================
AINode_Battle_Chase
==================
*/
int AINode_Battle_Chase(bot_state_t* bs) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	float range;

	// respawn if dead
	if(BotIsDead(bs)) {
		AIEnter_Respawn(bs, "battle chase: bot dead");
		return false;
	}
	// if no enemy
	if(bs->enemy < 0) {
		AIEnter_Seek_LTG(bs, "battle chase: no enemy");
		return false;
	}
	// if the enemy is visible
	if(BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy)) {
		AIEnter_Battle_Fight(bs, "battle chase");
		return false;
	}
	// if there is another enemy
	if(BotFindEnemy(bs, -1)) {
		AIEnter_Battle_Fight(bs, "battle chase: better enemy");
		return false;
	}
	// chasing the hunted player refreshes the memory
	if(bs->enemy == bs->hunt_enemy) {
		bs->hunt_expire_time = FloatTime() + 180;
	}
	// there is no last enemy area
	if(!bs->lastenemyareanum) {
		BotStartHunt(bs, bs->enemy);
		AIEnter_Battle_Hunt(bs, "battle chase: no enemy area");
		return false;
	}
	//
	bs->tfl = TFL_DEFAULT;
	// if in lava or slime the bot should be able to get out
	if(BotInLavaOrSlime(bs)) bs->tfl |= TFL_LAVA | TFL_SLIME;
	// create the chase goal
	goal.entitynum = bs->enemy;
	goal.areanum = bs->lastenemyareanum;
	VectorCopy(bs->lastenemyorigin, goal.origin);
	VectorSet(goal.mins, -8, -8, -8);
	VectorSet(goal.maxs, 8, 8, 8);
	// if the last seen enemy spot is reached the enemy could not be found
	if(trap_BotTouchingGoal(bs->origin, &goal)) bs->chase_time = 0;
	// if there's no chase time left, remember the player and start hunting
	if(!bs->chase_time || bs->chase_time < FloatTime() - 10) {
		BotStartHunt(bs, bs->enemy);
		AIEnter_Battle_Hunt(bs, "battle chase: time out");
		return false;
	}
	// check for nearby goals periodicly
	if(bs->check_time < FloatTime()) {
		bs->check_time = FloatTime() + 1;
		range = 150;
		//
		if(BotNearbyGoal(bs, bs->tfl, &goal, range)) {
			trap_BotResetLastAvoidReach(bs->ms);
			AIEnter_Battle_NBG(bs, "battle chase: nbg");
			return false;
		}
	}
	// initialize the movement state
	BotSetupForMovement(bs);
	// move towards the goal
	trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	// if the movement failed
	if(moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		bs->ltg_time = 0;
	}
	//
	BotAIBlocked(bs, &moveresult, false);
	//
	if(moveresult.flags & (MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
	} else if(!(bs->flags & BFL_IDEALVIEWSET)) {
		if(bs->chase_time > FloatTime() - 2) {
			BotAimAtEnemy(bs);
		} else {
			if(trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, 300, target)) {
				VectorSubtract(target, bs->origin, dir);
				vectoangles(dir, bs->ideal_viewangles);
			} else {
				vectoangles(moveresult.movedir, bs->ideal_viewangles);
			}
		}
		bs->ideal_viewangles[2] *= 0.5;
	}
	// if the weapon is used for the bot movement
	if(moveresult.flags & MOVERESULT_MOVEMENTWEAPON) bs->weaponnum = moveresult.weapon;
	// if the bot is in the area the enemy was last seen in
	if(bs->areanum == bs->lastenemyareanum) bs->chase_time = 0;
	// if the bot wants to retreat (the bot could have been damage during the chase)
	if(BotWantsToRetreat(bs)) {
		AIEnter_Battle_Retreat(bs, "battle chase: wants to retreat");
		return true;
	}
	return true;
}

/*
==================
AIEnter_Battle_Hunt
==================
*/
void AIEnter_Battle_Hunt(bot_state_t* bs, char* s) {
	BotRecordNodeSwitch(bs, "battle hunt", "", s);
	trap_BotResetLastAvoidReach(bs->ms);
	bs->ainode = AINode_Battle_Hunt;
}

/*
==================
AINode_Battle_Hunt

The bot remembers a player for 180 seconds and tracks them down: it
follows the trail of recorded player positions starting 30 seconds
behind and closing in with every point reached, once a minute it also
checks the spot the player was at a whole minute ago, and any fresh
sound pulls it to the place the sound came from. All movement is AAS
routing.
==================
*/
int AINode_Battle_Hunt(bot_state_t* bs) {
	bot_goal_t goal;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	vec3_t target, dir;
	int area;

	// respawn if dead
	if(BotIsDead(bs)) {
		AIEnter_Respawn(bs, "battle hunt: bot dead");
		return false;
	}
	// if the memory of the hunted player expired
	if(bs->hunt_expire_time < FloatTime()) {
		bs->hunt_expire_time = 0;
		AIEnter_Seek_LTG(bs, "battle hunt: memory expired");
		return false;
	}
	// if the hunted player left the game or is dead (it will respawn,
	// the memory is kept and the hunt resumes then)
	if(!BotHuntTargetAlive(bs)) {
		AIEnter_Seek_LTG(bs, "battle hunt: target gone");
		return false;
	}
	// if there is any other enemy visible, fight it
	if(BotFindEnemy(bs, -1)) {
		AIEnter_Battle_Fight(bs, "battle hunt: enemy");
		return false;
	}
	// if the hunted player is visible, fight him
	if(BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->hunt_enemy)) {
		bs->enemy = bs->hunt_enemy;
		bs->enemysight_time = FloatTime();
		bs->enemydeath_time = 0;
		AIEnter_Battle_Fight(bs, "battle hunt: found target");
		return false;
	}
	//
	bs->tfl = TFL_DEFAULT;
	// if in lava or slime the bot should be able to get out
	if(BotInLavaOrSlime(bs)) bs->tfl |= TFL_LAVA | TFL_SLIME;
	// if there is no hunt destination yet, take one from the trail
	if(!bs->hunt_havegoal) {
		// the position of the player 'hunt_traildelay' milliseconds ago,
		// closing in on the player with every trail point reached
		area = BotTrailPoint(bs->hunt_enemy, bs->hunt_traildelay, bs->hunt_goalorigin);
		if(area) {
			bs->hunt_goalareanum = area;
			bs->hunt_havegoal = true;
		}
		bs->hunt_trail_time = FloatTime() + 8 + random() * 5;
		// no trail data at all: try again a bit later from the seek node
		if(!bs->hunt_havegoal) {
			bs->hunt_trail_time = FloatTime() + 5;
			AIEnter_Seek_LTG(bs, "battle hunt: no trail");
			return false;
		}
	}
	// once a minute also check where the player was a whole minute ago
	if(bs->hunt_probe_time < FloatTime()) {
		bs->hunt_probe_time = FloatTime() + 60;
		area = BotTrailPoint(bs->hunt_enemy, 60000, target);
		if(area) {
			VectorCopy(target, bs->hunt_goalorigin);
			bs->hunt_goalareanum = area;
			bs->hunt_havegoal = true;
		}
	}
	// build the hunt goal
	goal.entitynum = bs->hunt_enemy;
	goal.areanum = bs->hunt_goalareanum;
	VectorCopy(bs->hunt_goalorigin, goal.origin);
	VectorSet(goal.mins, -25, -25, -25);
	VectorSet(goal.maxs, 25, 25, 25);
	// reached the trail point: stand here for a moment, then follow
	// fresher traces
	if(trap_BotTouchingGoal(bs->origin, &goal)) {
		if(bs->hunt_traildelay > 5000) bs->hunt_traildelay -= 5000;
		bs->hunt_havegoal = false;
		bs->hunt_trail_time = FloatTime() + 2;
		return true;
	}
	// initialize the movement state
	BotSetupForMovement(bs);
	// move towards the trail point
	trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	// if the movement failed
	if(moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		// the point may be unreachable: take a fresh one
		bs->hunt_havegoal = false;
	}
	//
	BotAIBlocked(bs, &moveresult, false);
	//
	if(moveresult.flags & (MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
	} else if(!(bs->flags & BFL_IDEALVIEWSET)) {
		if(trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, 300, target)) {
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
		} else {
			vectoangles(moveresult.movedir, bs->ideal_viewangles);
		}
		bs->ideal_viewangles[2] *= 0.5;
	}
	return true;
}

/*
==================
AIEnter_Battle_Retreat
==================
*/
void AIEnter_Battle_Retreat(bot_state_t* bs, char* s) {
	BotRecordNodeSwitch(bs, "battle retreat", "", s);
	bs->ainode = AINode_Battle_Retreat;
}

/*
==================
AINode_Battle_Retreat
==================
*/
int AINode_Battle_Retreat(bot_state_t* bs) {
	bot_goal_t goal;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	vec3_t target, dir;
	float attack_skill, range;
	int areanum;

	// respawn if dead
	if(BotIsDead(bs)) {
		AIEnter_Respawn(bs, "battle retreat: bot dead");
		return false;
	}
	// if no enemy
	if(bs->enemy < 0) {
		AIEnter_Seek_LTG(bs, "battle retreat: no enemy");
		return false;
	}
	//
	BotEntityInfo(bs->enemy, &entinfo);
	if(EntityIsDead(&entinfo)) {
		AIEnter_Seek_LTG(bs, "battle retreat: enemy dead");
		return false;
	}
	// if there is another better enemy
	BotFindEnemy(bs, bs->enemy);
	//
	bs->tfl = TFL_DEFAULT;
	// if in lava or slime the bot should be able to get out
	if(BotInLavaOrSlime(bs)) bs->tfl |= TFL_LAVA | TFL_SLIME;
	// if the bot doesn't want to retreat anymore... probably picked up some nice items
	if(BotWantsToChase(bs)) {
		// go chase the enemy
		AIEnter_Battle_Chase(bs, "battle retreat: wants to chase");
		return false;
	}
	// update the last time the enemy was visible
	if(BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy)) {
		bs->enemyvisible_time = FloatTime();
		VectorCopy(entinfo.origin, target);
		// update the reachability area and origin if possible
		areanum = BotPointAreaNum(target);
		if(areanum && trap_AAS_AreaReachability(areanum)) {
			VectorCopy(target, bs->lastenemyorigin);
			bs->lastenemyareanum = areanum;
		}
	}
	// if the enemy is NOT visible for 4 seconds
	if(bs->enemyvisible_time < FloatTime() - 4) {
		BotStartHunt(bs, bs->enemy);
		AIEnter_Battle_Hunt(bs, "battle retreat: lost enemy");
		return false;
	}
	// else if the enemy is NOT visible
	else if(bs->enemyvisible_time < FloatTime()) {
		// if there is another enemy
		if(BotFindEnemy(bs, -1)) {
			AIEnter_Battle_Fight(bs, "battle retreat: another enemy");
			return false;
		}
	}
	// get the current long term goal while retreating
	if(!BotGetItemLongTermGoal(bs, bs->tfl, &goal)) {
		AIEnter_Battle_SuicidalFight(bs, "battle retreat: no way out");
		return false;
	}
	// check for nearby goals periodicly
	if(bs->check_time < FloatTime()) {
		bs->check_time = FloatTime() + 1;
		range = 150;
		if(BotNearbyGoal(bs, bs->tfl, &goal, range)) {
			trap_BotResetLastAvoidReach(bs->ms);
			AIEnter_Battle_NBG(bs, "battle retreat: nbg");
			return false;
		}
	}
	// initialize the movement state
	BotSetupForMovement(bs);
	// move towards the goal
	trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	// if the movement failed
	if(moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		bs->ltg_time = 0;
	}
	//
	BotAIBlocked(bs, &moveresult, false);
	// choose the best weapon to fight with
	BotChooseWeapon(bs);
	// if the view is fixed for the movement
	if(moveresult.flags & (MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
	} else if(!(moveresult.flags & MOVERESULT_MOVEMENTVIEWSET) && !(bs->flags & BFL_IDEALVIEWSET)) {
		attack_skill = BotSkill_BFloat(bs, 0, 1);
		// if the bot is skilled enough
		if(attack_skill > 0.3) {
			BotAimAtEnemy(bs);
		} else {
			if(trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, 300, target)) {
				VectorSubtract(target, bs->origin, dir);
				vectoangles(dir, bs->ideal_viewangles);
			} else {
				vectoangles(moveresult.movedir, bs->ideal_viewangles);
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	// if the weapon is used for the bot movement
	if(moveresult.flags & MOVERESULT_MOVEMENTWEAPON) bs->weaponnum = moveresult.weapon;
	// attack the enemy if possible
	BotCheckAttack(bs);
	//
	return true;
}

/*
==================
AIEnter_Battle_NBG
==================
*/
void AIEnter_Battle_NBG(bot_state_t* bs, char* s) {
	BotRecordNodeSwitch(bs, "battle NBG", "", s);
	bs->ainode = AINode_Battle_NBG;
}

/*
==================
AINode_Battle_NBG
==================
*/
int AINode_Battle_NBG(bot_state_t* bs) {
	int areanum;
	bot_goal_t goal;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	float attack_skill;
	vec3_t target, dir;

	// respawn if dead
	if(BotIsDead(bs)) {
		AIEnter_Respawn(bs, "battle nbg: bot dead");
		return false;
	}
	// if no enemy
	if(bs->enemy < 0) {
		AIEnter_Seek_NBG(bs, "battle nbg: no enemy");
		return false;
	}
	//
	BotEntityInfo(bs->enemy, &entinfo);
	if(EntityIsDead(&entinfo)) {
		AIEnter_Seek_NBG(bs, "battle nbg: enemy dead");
		return false;
	}
	//
	bs->tfl = TFL_DEFAULT;
	// if in lava or slime the bot should be able to get out
	if(BotInLavaOrSlime(bs)) bs->tfl |= TFL_LAVA | TFL_SLIME;
	// update the last time the enemy was visible
	if(BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy)) {
		bs->enemyvisible_time = FloatTime();
		VectorCopy(entinfo.origin, target);
		// update the reachability area and origin if possible
		areanum = BotPointAreaNum(target);
		if(areanum && trap_AAS_AreaReachability(areanum)) {
			VectorCopy(target, bs->lastenemyorigin);
			bs->lastenemyareanum = areanum;
		}
	}
	//
	memcpy(&goal, &bs->nbg_goal, sizeof(bot_goal_t));
	// if the goal item is gone or touched
	if(!(goal.flags & GFL_AIR) && (!BotItemGoalValid(&goal) || BotReachedGoal(bs, &goal))) {
		bs->nbg_time = 0;
	}
	//
	if(bs->nbg_time < FloatTime()) {
		AIEnter_Battle_Retreat(bs, "battle nbg: time out");
		return false;
	}
	// initialize the movement state
	BotSetupForMovement(bs);
	// move towards the goal
	trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	// if the movement failed
	if(moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		bs->nbg_time = 0;
	}
	//
	BotAIBlocked(bs, &moveresult, false);
	// choose the best weapon to fight with
	BotChooseWeapon(bs);
	// if the view is fixed for the movement
	if(moveresult.flags & (MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
	} else if(!(moveresult.flags & MOVERESULT_MOVEMENTVIEWSET) && !(bs->flags & BFL_IDEALVIEWSET)) {
		attack_skill = BotSkill_BFloat(bs, 0, 1);
		// if the bot is skilled enough
		if(attack_skill > 0.3) {
			BotAimAtEnemy(bs);
		} else {
			if(trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, 300, target)) {
				VectorSubtract(target, bs->origin, dir);
				vectoangles(dir, bs->ideal_viewangles);
			} else {
				vectoangles(moveresult.movedir, bs->ideal_viewangles);
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	// if the weapon is used for the bot movement
	if(moveresult.flags & MOVERESULT_MOVEMENTWEAPON) bs->weaponnum = moveresult.weapon;
	// attack the enemy if possible
	BotCheckAttack(bs);
	//
	return true;
}
