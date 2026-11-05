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
 * name:		ai_dmq3.h
 *
 * desc:		bot AI
 *
 *****************************************************************************/

// setup the deathmatch AI
void BotSetupDeathmatchAI(void);
// shutdown the deathmatch AI
void BotShutdownDeathmatchAI(void);
// let the bot live within its deathmatch AI net
void BotDeathmatchAI(bot_state_t* bs, float thinktime);
// remember a player for 180 seconds and track them down
void BotStartHunt(bot_state_t* bs, int client);
// true if the remembered hunt target is still connected and alive
bool BotHuntTargetAlive(bot_state_t* bs);
// react to the sounds players made: footsteps, falls and gunshots
void BotListenSounds(bot_state_t* bs);
// damage types
#define DAMAGETYPE_IMPACT 1   // damage on impact
#define DAMAGETYPE_RADIAL 2   // radial damage
#define DAMAGETYPE_VISIBLE 4  // damage to all entities visible to the projectile

// aiming information of a dynamic weapon, filled from jsd_weapon
typedef struct weaponinfo_s {
	int number;  // number of the weapon
	vec3_t offset;
	float hspread;
	float vspread;
	float speed;
	struct {
		int damagetype;
		int damage;
		float radius;
	} proj;
} weaponinfo_t;

// choose a weapon
void BotChooseWeapon(bot_state_t* bs);
// returns the number of the best weapon the bot can fight with, 0 if none
int BotBestWeapon(bot_state_t* bs);
// fills the aiming info of a weapon from the dynamic jsd_weapon data
void BotGetWeaponInfo(int weapon, weaponinfo_t* wi);
// setup movement stuff
void BotSetupForMovement(bot_state_t* bs);
// return true if the bot is dead
bool BotIsDead(bot_state_t* bs);
// returns true if the bot is in lava or slime
bool BotInLavaOrSlime(bot_state_t* bs);
// returns true if the entity is dead
bool EntityIsDead(aas_entityinfo_t* entinfo);
// returns true if the entity is shooting
bool EntityIsShooting(aas_entityinfo_t* entinfo);
// returns the name of the client
char* ClientName(int client, char* name, int size);
// returns the aggression of the bot in the range [0, 100]
float BotAggression(bot_state_t* bs);
// returns true if the bot wants to retreat
int BotWantsToRetreat(bot_state_t* bs);
// returns true if the bot wants to chase
int BotWantsToChase(bot_state_t* bs);
// returns true if the bot can and wants to rocketjump
// the bot will perform attack movements
bot_moveresult_t BotCombatMove(bot_state_t* bs, int tfl);
// returns true if within the field of vision for the given angles
bool InFieldOfVision(vec3_t viewangles, float fov, vec3_t angles);
// returns true and sets the .enemy field when an enemy is found
int BotFindEnemy(bot_state_t* bs, int curenemy);
// returns a roam goal
void BotRoamGoal(bot_state_t* bs, vec3_t goal);
// returns entity visibility in the range [0, 1]
float BotEntityVisible(int viewer, vec3_t eye, vec3_t viewangles, float fov, int ent);
// the bot will aim at the current enemy
void BotAimAtEnemy(bot_state_t* bs);
// check if the bot should attack
void BotCheckAttack(bot_state_t* bs);
// AI when the bot is blocked
void BotAIBlocked(bot_state_t* bs, bot_moveresult_t* moveresult, int activate);
// AI to predict obstacles
int BotAIPredictObstacles(bot_state_t* bs, bot_goal_t* goal);
// enable or disable the areas the blocking entity is in
void BotEnableActivateGoalAreas(bot_activategoal_t* activategoal, int enable);
// pop an activate goal from the stack
int BotPopFromActivateGoalStack(bot_state_t* bs);
// clear the activate goal stack
void BotClearActivateGoalStack(bot_state_t* bs);
//
int BotPointAreaNum(vec3_t origin);

extern int max_bspmodelindex;  // maximum BSP model index

extern vmCvar_t bot_challenge;
extern vmCvar_t bot_predictobstacles;
