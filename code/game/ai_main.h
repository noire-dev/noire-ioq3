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
 * name:		ai_main.h
 *
 * desc:		bot AI
 *
 *****************************************************************************/

// bot flags
#define BFL_AVOIDRIGHT 16     // avoid obstacles by going to the right
#define BFL_IDEALVIEWSET 32   // bot has ideal view angles set
#define BFL_FIGHTSUICIDAL 64  // bot is in a suicidal fight

// copied from the aas file header
#define PRESENCE_NONE 1
#define PRESENCE_NORMAL 2
#define PRESENCE_CROUCH 4

#define MAX_ACTIVATESTACK 8
#define MAX_ACTIVATEAREAS 32

typedef struct bot_activategoal_s {
	int inuse;
	bot_goal_t goal;                  // goal to activate (buttons etc.)
	float time;                       // time to activate something
	float start_time;                 // time starting to activate something
	float justused_time;              // time the goal was used
	int shoot;                        // true if bot has to shoot to activate
	int weapon;                       // weapon to be used for activation
	vec3_t target;                    // target to shoot at to activate something
	vec3_t origin;                    // origin of the blocking entity to activate
	int areas[MAX_ACTIVATEAREAS];     // routing areas disabled by blocking entity
	int numareas;                     // number of disabled routing areas
	int areasdisabled;                // true if the areas are disabled for the routing
	struct bot_activategoal_s* next;  // next activate goal on stack
} bot_activategoal_t;

// bot state
typedef struct bot_state_s {
	int inuse;                           // true if this state is used by a bot client
	int botthink_residual;               // residual for the bot thinks
	int client;                          // client number of the bot
	int entitynum;                       // entity number of the bot
	playerState_t cur_ps;                // current player state
	int last_eFlags;                     // last ps flags
	usercmd_t lastucmd;                  // usercmd from last frame
	//
	bot_settings_t settings;                // several bot settings
	int (*ainode)(struct bot_state_s* bs);  // current AI node
	float thinktime;                        // time the bot thinks this frame
	vec3_t origin;                          // origin of the bot
	vec3_t eye;                             // eye coordinates of the bot
	int areanum;                            // the number of the area the bot is in
	int tfl;                                // the travel flags the bot uses
	int flags;                              // several flags
	int respawn_wait;                       // wait until respawned
	int lasthealth;                         // health value previous frame
	int botsuicide;                         // true when the bot suicides
	int setupcount;                         // true when the bot has just been setup
	int num_deaths;                         // number of time this bot died
	int num_kills;                          // number of kills of this bot
	float walker;                           // walker characteristic
	float ltime;                            // local bot time
	float entergame_time;                   // time the bot entered the game
	float ltg_time;                         // long term goal time
	float nbg_time;                         // nearby goal time
	float respawn_time;                     // time the bot takes to respawn
	float chase_time;                       // time the bot will chase the enemy
	float enemyvisible_time;                // time the enemy was last visible
	float check_time;                       // time to check for nearby items
	float stand_time;                       // time the bot is standing still
	float standfindenemy_time;              // time to find enemy while standing
	float attackstrafe_time;                // time the bot is strafing in one dir
	float attackcrouch_time;                // time the bot will stop crouching
	float attackjump_time;                  // time the bot jumped during attack
	float enemysight_time;                  // time before reacting to enemy
	float enemydeath_time;                  // time the enemy died
	float enemyposition_time;               // time the position and velocity of the enemy were stored
	float lastair_time;                     // last time the bot had air
	float teleport_time;                    // last time the bot teleported
	float weaponchange_time;                // time the bot started changing weapons
	float firethrottlewait_time;            // amount of time to wait
	float firethrottleshoot_time;           // amount of time to shoot
	float notblocked_time;                  // last time the bot was not blocked
	float blockedbyavoidspot_time;          // time blocked by an avoid spot
	float predictobstacles_time;            // last time the bot predicted obstacles
	int predictobstacles_goalareanum;       // last goal areanum the bot predicted obstacles for
	vec3_t aimtarget;
	vec3_t enemyvelocity;  // enemy velocity 0.5 secs ago during battle
	vec3_t enemyorigin;    // enemy origin 0.5 secs ago during battle
	//
	// hunting: the bot remembers a player for 180 seconds and tracks
	// them down following the trail of recorded player positions
	float hunt_expire_time;   // time the memory of the hunt target expires
	int hunt_enemy;           // client number of the tracked player
	int hunt_traildelay;      // current trail delay in milliseconds
	float hunt_trail_time;    // time to take the next trail point
	float hunt_probe_time;    // time of the next one-minute-old probe
	vec3_t hunt_goalorigin;   // current hunt destination
	int hunt_goalareanum;     // area of the current hunt destination
	int hunt_havegoal;        // true when hunt_goal* is valid
	//
	// sounds: footsteps, falls and gunshots of players
	int sound_id;             // last processed global sound index
	float sound_time;         // time the last player sound was heard
	vec3_t sound_origin;      // where the sound came from
	int sound_areanum;        // area of the sound origin
	//
	// combat position: Source style fighting, the bot occupies a
	// position with a line of fire to the enemy and shoots from it
	vec3_t combatspot_origin;       // the position to fight from
	int combatspot_areanum;         // AAS area of the position
	int combatspot_valid;           // true when the position is usable
	float combatspot_check_time;    // time to revalidate the position
	vec3_t combatspot_enemyorigin;  // enemy origin when the position was picked
	//
	int ms;         // move state of the bot
	//
	int enemy;                // enemy entity number
	int lastenemyareanum;     // last reachability area the enemy was in
	vec3_t lastenemyorigin;   // last origin of the enemy in the reachability area
	int weaponnum;            // current weapon number
	vec3_t viewangles;        // current view angles
	vec3_t ideal_viewangles;  // ideal view angles
	vec3_t viewanglespeed;
	//
	// the current long term and nearby item goals
	bot_goal_t ltg_goal;
	bot_goal_t nbg_goal;
	//
	bot_activategoal_t* activatestack;                       // first activate goal on the stack
	bot_activategoal_t activategoalheap[MAX_ACTIVATESTACK];  // activate goal heap
} bot_state_t;

// linearly interpolates a skill parameter between min (skill 1) and max (skill 5)
float BotSkill_BFloat(bot_state_t* bs, float min, float max);
// position of a player 'delay' milliseconds ago from the recorded trail,
// returns the area number or 0 if nothing is known
int BotTrailPoint(int client, int delay, vec3_t origin);
// total number of player sounds recorded this map
int BotTotalSounds(void);
// sound number 'id': client who made it, where it came from and its area
void BotGetSound(int id, int* client, vec3_t origin, int* areanum);


// resets the whole bot state
void BotResetState(bot_state_t* bs);
// returns the number of bots in the game
int NumBots(void);
// returns info about the entity
void BotEntityInfo(int entnum, aas_entityinfo_t* info);

extern float floattime;
#define FloatTime() floattime

// from the game source
void QDECL BotAI_Print(int type, char* fmt, ...) Q_PRINTF_FUNC(2, 3);
void BotAI_Trace(bsp_trace_t* bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask);
int BotAI_GetClientState(int clientNum, playerState_t* state);
int BotAI_GetEntityState(int entityNum, entityState_t* state);
