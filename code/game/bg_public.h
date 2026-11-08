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

#ifndef __BG_PUBLIC_H
#define __BG_PUBLIC_H

#define MAX_JS_STRINGSIZE 256

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

#define GAME_VERSION BASEGAME "-1"

#define DEFAULT_GRAVITY 800
#define ARMOR_PROTECTION 0.66

#define MAX_ITEMS 256

#define ITEM_RADIUS 15  // item sizes are needed for client side pickup detection

#define PLAYER_WIDTH 15
#define MINS_Z -24
#define DEFAULT_HEIGHT 48
#define DEFAULT_VIEWHEIGHT 40
#define CROUCH_HEIGHT 16
#define CROUCH_VIEWHEIGHT 12
#define DEAD_HEIGHT -8
#define DEAD_VIEWHEIGHT -16

// weapon status
#define WS_NONE 0
#define WS_HAVE 1
#define WS_NOAMMO 2

// phys type
#define PHYS_STATIC 1
#define PHYS_DYNAMIC 2
#define PHYS_SLEEPING 3

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define CS_MUSIC 2
#define CS_MESSAGE 3  // from the map worldspawn's message field

#define CS_GAME_VERSION 20
#define CS_LEVEL_START_TIME 21  // so the timer only shows the current level

#define CS_MODELS 32
#define CS_SOUNDS (CS_MODELS + MAX_MODELS)
#define CS_PLAYERS (CS_SOUNDS + MAX_SOUNDS)
#define CS_LOCATIONS (CS_PLAYERS + MAX_CLIENTS)
#define CS_PARTICLES (CS_LOCATIONS + MAX_LOCATIONS)

#define CS_RAGDOLLS (CS_PARTICLES + MAX_LOCATIONS)  // [MAX_RAGDOLLS] player model name of every ragdoll

#define CS_MAX (CS_RAGDOLLS + MAX_RAGDOLLS)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

#define MAX_NETNAME 256

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum {
	PM_NORMAL,  // can accelerate and turn
	PM_NOCLIP,  // noclip movement
	PM_DEAD,    // no acceleration or turning, but free falling
	PM_FREEZE,  // stuck in place with no control
} pmtype_t;

typedef enum { WEAPON_READY, WEAPON_RAISING, WEAPON_DROPPING, WEAPON_FIRING } weaponstate_t;

// pmove->pm_flags
#define PMF_DUCKED 1
#define PMF_JUMP_HELD 2
#define PMF_BACKWARDS_JUMP 8    // go into backwards land
#define PMF_BACKWARDS_RUN 16    // coast down to backwards run
#define PMF_TIME_LAND 32        // pm_time is time before rejump
#define PMF_TIME_KNOCKBACK 64   // pm_time is an air-accelerate only time
#define PMF_TIME_WATERJUMP 256  // pm_time is waterjump
#define PMF_RESPAWNED 512       // clear after attack and jump buttons come up

#define PMF_ALL_TIMES (PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_KNOCKBACK)

#define MAXTOUCH 32
typedef struct {
	// state (in / out)
	playerState_t* ps;

	// command (in)
	usercmd_t cmd;
	int tracemask;  // collide against these types of surfaces

	// results (out)
	int numtouch;
	int touchents[MAXTOUCH];

	vec3_t mins, maxs;  // bounding box size

	int watertype;
	int waterlevel;

	float xyspeed;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void (*trace)(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask);
	int (*pointcontents)(const vec3_t point, int passEntityNum);
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles(playerState_t* ps, const usercmd_t* cmd);
void Pmove(pmove_t* pmove);

//===================================================================================

// player_state->stats[] indexes
// NOTE: may not have more than 16
typedef enum {
	STAT_HEALTH,
	STAT_ARMOR,
	STAT_AMMO,
} statIndex_t;

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum {
	PERS_SPAWN_COUNT,  // incremented every respawn
} persEnum_t;

// entityState_t->eFlags
#define EF_DEAD 0x00000001          // don't draw a foe marker over players with EF_DEAD
#define EF_TELEPORT_BIT 0x00000004  // toggled every time the origin abruptly changes
#define EF_PLAYER_EVENT 0x00000010
#define EF_BOUNCE 0x00000010       // for missiles
#define EF_BOUNCE_HALF 0x00000020  // for missiles
#define EF_NODRAW 0x00000080       // may have an event, but no model (unspawned items)
#define EF_FIRING 0x00000100       // for lightning gun
#define EF_MOVER_STOP 0x00000400   // will push otherwise

#define WP_NONE 0
#define MAX_WEAPONS 2048

typedef enum {
	WT_NONE,

	WT_MELEE,
	WT_BULLET,
	WT_SHOTGUN,
	WT_BEAM,
	WT_PHYSGUN,
	WT_GRAVITYGUN,
	WT_TOOLGUN,
	WT_MISSILE,
	WT_EMPTY,

	WEAPONTYPES_NUM
} weaponType_t;

typedef enum {
	WAT_NONE,
	WAT_MELEE,
	WAT_GRENADE,

	WEAPONANIMTYPES_NUM
} weaponAnimType_t;

// Selects the 36-entry IQM locomotion block (AM_IDLE .. AM_JUMP) a weapon uses;
// shared by game (ragdoll pose) and cgame (player animation).
typedef enum { AMTYPE_ALL = 0, AMTYPE_AR2, AMTYPE_CAMERA, AMTYPE_CROSSBOW, AMTYPE_DUAL, AMTYPE_FIST, AMTYPE_GRENADE, AMTYPE_KNIFE, AMTYPE_MAGIC, AMTYPE_MELEE, AMTYPE_MELEE2, AMTYPE_PASSIVE, AMTYPE_PHYSGUN, AMTYPE_PISTOL, AMTYPE_REVOLVER, AMTYPE_RPG, AMTYPE_SHOTGUN, AMTYPE_SLAM, AMTYPE_SMG1, AMTYPE_COUNT } animType_t;

typedef struct weaponPropeties_s {
	weaponType_t wType;
	weaponAnimType_t wAnimType;
	animType_t animType;
	char classname[MAX_JS_STRINGSIZE];
	int delay;
	int count;
	int damage;
	int splashDamage;
	int splashRadius;
	int range;
	int speed;
	int speedRandom;
	int spread;
	int timeout;
	bool gravity;
	bool bounce;
	float bounceModifier;
	float zOffset;
} weaponProperties_t;

extern weaponProperties_t jsd_weapon[];
extern int jsd_weaponCount;

typedef struct {
	const char* shader;
	const char* sound;
	float color[4];
	int count;
	float speed;
	int flags;
	float gravity[3];
	float size;
	float lifetime;
} particlePreset_t;

enum {
	PP_SMOKE = 0,
	PP_TELEPORT_IN,
	PP_TELEPORT_OUT,
	PP_BLOOD,
	PP_SPARK,
	PP_EXPLOSION,
	PP_COUNT,
};

#define PF_SMOOTH_FADE 0x01  // ПЛАВНОЕ_ЗАТУХАНИЕ: линейное затухание альфы
#define PF_GRAVITY 0x02      // ГРАВИТАЦИЯ: применять вектор ускорения

static const particlePreset_t particlePresets[PP_COUNT] = {
    /* PP_SMOKE        */ {"", "", {1, 1, 1, 1}, 8, 80.0f, PF_SMOOTH_FADE | PF_GRAVITY, {0, 0, -100}, 3.0f, 1.2f},
    /* PP_TELEPORT_IN  */ {"spark", "", {0.35f, 0.65f, 1, 1}, 64, 180.0f, PF_SMOOTH_FADE | PF_GRAVITY, {0, 0, 80}, 4.0f, 0.9f},
    /* PP_TELEPORT_OUT */ {"spark", "", {1, 1, 1, 1}, 64, 250.0f, PF_SMOOTH_FADE, {0, 0, 0}, 5.0f, 0.5f},
    /* PP_BLOOD        */ {"bloodTrail", "", {0.75f, 0, 0, 1}, 24, 200.0f, PF_SMOOTH_FADE | PF_GRAVITY, {0, 0, -400}, 4.0f, 0.8f},
    /* PP_SPARK        */ {"spark", "", {1, 0.85f, 0.3f, 1}, 16, 300.0f, PF_SMOOTH_FADE | PF_GRAVITY, {0, 0, -300}, 2.0f, 0.4f},
    /* PP_EXPLOSION    */ {"spark", "", {1, 0.5f, 0.1f, 1}, 48, 350.0f, PF_SMOOTH_FADE, {0, 0, -50}, 6.0f, 0.7f},
};

// entityState_t->event values
// entity events are for effects that take place relative
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define EV_EVENT_BIT1 0x00000100
#define EV_EVENT_BIT2 0x00000200
#define EV_EVENT_BITS (EV_EVENT_BIT1 | EV_EVENT_BIT2)

#define EVENT_VALID_MSEC 300

typedef enum {
	EV_NONE,

	EV_FOOTSTEP_CHAINLINK,
	EV_FOOTSTEP_CONCRETE,
	EV_FOOTSTEP_DIRT,
	EV_FOOTSTEP_DUCT,
	EV_FOOTSTEP_GRASS,
	EV_FOOTSTEP_GRAVEL,
	EV_FOOTSTEP_LADDER,
	EV_FOOTSTEP_METAL,
	EV_FOOTSTEP_METALGRATE,
	EV_FOOTSTEP_MUD,
	EV_FOOTSTEP_SAND,
	EV_FOOTSTEP_SLOSH,
	EV_FOOTSTEP_TILE,
	EV_FOOTSTEP_WOOD,
	EV_FOOTSTEP_WOODPANEL,

	EV_FALL,
	EV_GRAVITY_LAUNCH,
	EV_WATER_SPLASH,
	EV_ITEM_PICKUP,
	EV_FIRE_WEAPON,
	EV_MISSILE_BOUNCE,
	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,
	EV_BULLET_HIT_ENTITY,
	EV_BULLET_HIT_WORLD,
	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_BEAM_TRAIL,
	EV_SHOTGUN,
	EV_OBITUARY,
	EV_PARTICLE,
} entity_event_t;

// IQM player animation ids. The first 19 entries are full-body actions.
// The next 36 entries describe locomotion; each weapon animType selects a
// 36-entry block starting at AM_IDLE.
typedef enum {
	// General / full-body actions
	AM_DEATH1 = 0,
	AM_DEATH2,
	AM_DEATH3,
	AM_DEATH4,
	AM_GESTURE_AGREE,
	AM_GESTURE_DISAGREE,
	AM_GESTURE_BECON,
	AM_GESTURE_BOW,
	AM_GESTURE_SALUTE,
	AM_GESTURE_WAVE,
	AM_TAUNT_CHEER,
	AM_TAUNT_DANCE,
	AM_TAUNT_LAUGH,
	AM_TAUNT_MUSCLE,
	AM_TAUNT_PERSISTENCE,
	AM_TAUNT_ROBOT,
	AM_TAUNT_ZOMBIE,
	AM_MELEEATTACK,
	AM_THROW,

	// Locomotion block (36 states per weapon animType)
	AM_IDLE,
	AM_SIT,
	AM_WALK_E,
	AM_WALK_N,
	AM_WALK_NE,
	AM_WALK_NW,
	AM_WALK_S,
	AM_WALK_SE,
	AM_WALK_SW,
	AM_WALK_W,
	AM_RUN_E,
	AM_RUN_N,
	AM_RUN_NE,
	AM_RUN_NW,
	AM_RUN_S,
	AM_RUN_SE,
	AM_RUN_SW,
	AM_RUN_W,
	AM_CRONCHIDLE,
	AM_CRONCH_E,
	AM_CRONCH_N,
	AM_CRONCH_NE,
	AM_CRONCH_NW,
	AM_CRONCH_S,
	AM_CRONCH_SE,
	AM_CRONCH_SW,
	AM_CRONCH_W,
	AM_SWIM_E,
	AM_SWIM_N,
	AM_SWIM_NE,
	AM_SWIM_NW,
	AM_SWIM_S,
	AM_SWIM_SE,
	AM_SWIM_SW,
	AM_SWIM_W,
	AM_JUMP,

	AM_MOVE_ANIMATION_COUNT = 36,
	AM_WEAPON_TYPE_COUNT = 19,
	MAX_TOTALANIMATIONS = AM_IDLE + AM_MOVE_ANIMATION_COUNT * AM_WEAPON_TYPE_COUNT
} animNumber_t;

typedef struct animation_s {
	int firstFrame;
	int numFrames;
	int loopFrames;   // 0 to numFrames
	int frameLerp;    // msec between frames
	int initialLerp;  // msec to get to first frame
	int reversed;     // true if animation is reversed
	int flipflop;     // true if animation should flipflop back to base
} animation_t;

// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define ANIM_TOGGLEBIT 0x40000000

// means of death
typedef enum {
	MOD_UNKNOWN,
	MOD_WEAPONS = MAX_WEAPONS,  // Weapons (check weapon_t)

	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TRIGGER_HURT,
} meansOfDeath_t;

//---------------------------------------------------------

// item_t->type
typedef enum {
	IT_BAD,
	IT_WEAPON,  // EFX: rotate + upscale + minlight
} itemType_t;

typedef struct item_s {
	char classname[MAX_JS_STRINGSIZE];
	char pickup_sound[MAX_JS_STRINGSIZE];
	char pickup_name[MAX_JS_STRINGSIZE];
	char model[MAX_JS_STRINGSIZE];
	char icon[MAX_JS_STRINGSIZE];
	int quantity;
	itemType_t giType;
	int giTag;
	char precaches[MAX_JS_STRINGSIZE];
	char sounds[MAX_JS_STRINGSIZE];
} item_t;

extern item_t jsd_item[MAX_ITEMS];
extern int jsd_itemCount;

item_t* BG_FindItem(const char* pickupName);
item_t* BG_FindItemForWeapon(int weapon);
#define ITEM_INDEX(x) ((x) - jsd_item)

bool BG_CanItemBeGrabbed(const entityState_t* ent, const playerState_t* ps);

// content masks
#define MASK_ALL (-1)
#define MASK_SOLID (CONTENTS_SOLID)
#define MASK_SELECT (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER)
#define MASK_PLAYERSOLID (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY)
#define MASK_DEADSOLID (CONTENTS_SOLID | CONTENTS_PLAYERCLIP)
#define MASK_WATER (CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME)
#define MASK_OPAQUE (CONTENTS_SOLID | CONTENTS_SLIME | CONTENTS_LAVA)
#define MASK_SHOT (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE)

//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PHYSICS,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_PORTAL,
	ET_SPEAKER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_RAGDOLL,  // master entity of a server-side ragdoll: origin = main bone, ragdollID selects the ragdollState

	ET_EVENTS  // any of the EV_* events can be added freestanding
	           // by setting eType to ET_EVENTS + eventNum
	           // this avoids having to set eFlags and eventNum
} entityType_t;

void BG_EvaluateTrajectory(const trajectory_t* tr, int atTime, vec3_t result);
void BG_EvaluateTrajectoryDelta(const trajectory_t* tr, int atTime, vec3_t result);

void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t* ps);

void BG_PlayerStateToEntityState(playerState_t* ps, entityState_t* s, bool snap);
void BG_PlayerStateToEntityStateExtraPolate(playerState_t* ps, entityState_t* s, int time, bool snap);

bool BG_PlayerTouchesItem(playerState_t* ps, entityState_t* item, int atTime);


//
// Server-side ragdoll, simplified skeleton.
// The simplified ragdoll only simulates and sends these valveBiped bones, in this
// order. The order is the same on server and client; boneID == index in this list.
// (The detailed ragdoll uses every bone of the IQM, boneID == IQM joint index.)
//
#define RAGDOLL_SIMPLE_NUMBONES 23
extern const char* const bg_ragdollSimpleBones[RAGDOLL_SIMPLE_NUMBONES];

#endif
