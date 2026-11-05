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
 * name:		botlib.h
 *
 * desc:		bot AI library
 *
 * The item config, weapon config, chat and genetic machinery has been
 * removed: items and weapons are fully dynamic and come from the mod.
 *
 *****************************************************************************/

#define BOTLIB_API_VERSION 2

struct aas_clientmove_s;
struct aas_entityinfo_s;
struct aas_areainfo_s;
struct aas_predictroute_s;
struct bot_goal_s;
struct bot_moveresult_s;
struct bot_initmove_s;

// debug line colors
#define LINECOLOR_NONE -1
#define LINECOLOR_RED 1
#define LINECOLOR_GREEN 2
#define LINECOLOR_BLUE 3
#define LINECOLOR_YELLOW 4
#define LINECOLOR_ORANGE 5

// Print types
#define PRT_MESSAGE 1
#define PRT_WARNING 2
#define PRT_ERROR 3
#define PRT_FATAL 4
#define PRT_EXIT 5

// botlib error codes
#define BLERR_NOERROR 0              // no error
#define BLERR_LIBRARYNOTSETUP 1      // library not setup
#define BLERR_INVALIDENTITYNUMBER 2  // invalid entity number
#define BLERR_NOAASFILE 3            // no AAS file available
#define BLERR_CANNOTOPENAASFILE 4    // cannot open AAS file
#define BLERR_WRONGAASFILEID 5       // incorrect AAS file id
#define BLERR_WRONGAASFILEVERSION 6  // incorrect AAS file version
#define BLERR_CANNOTREADAASLUMP 7    // cannot read AAS file lump

// action flags
#define ACTION_ATTACK 0x00000001
#define ACTION_USE 0x00000002
#define ACTION_RESPAWN 0x00000008
#define ACTION_JUMP 0x00000010
#define ACTION_MOVEUP 0x00000020
#define ACTION_CROUCH 0x00000080
#define ACTION_MOVEDOWN 0x00000100
#define ACTION_MOVEFORWARD 0x00000200
#define ACTION_MOVEBACK 0x00000800
#define ACTION_MOVELEFT 0x00001000
#define ACTION_MOVERIGHT 0x00002000
#define ACTION_DELAYEDJUMP 0x00008000
#define ACTION_TALK 0x00010000
#define ACTION_GESTURE 0x00020000
#define ACTION_WALK 0x00080000
#define ACTION_JUMPEDLASTFRAME 0x10000000

// the bot input, will be converted to a usercmd_t
typedef struct bot_input_s {
	float thinktime;    // time since last output (in seconds)
	vec3_t dir;         // movement direction
	float speed;        // speed in the range [0, 400]
	vec3_t viewangles;  // the view angles
	int actionflags;    // one of the ACTION_? flags
	int weapon;         // weapon to use
} bot_input_t;

#ifndef BSPTRACE

#define BSPTRACE

// bsp_trace_t hit surface
typedef struct bsp_surface_s {
	char name[16];
	int flags;
	int value;
} bsp_surface_t;

// a trace is returned when a box is swept through the world
typedef struct bsp_trace_s {
	bool allsolid;          // if true, plane is not valid
	bool startsolid;        // if true, the initial point was in a solid area
	float fraction;         // time completed, 1.0 = didn't hit anything
	vec3_t endpos;          // final position
	cplane_t plane;         // surface normal at impact
	float exp_dist;         // expanded plane distance
	int sidenum;            // number of the brush side hit
	bsp_surface_t surface;  // the hit point surface
	int contents;           // contents on other side of surface hit
	int ent;                // number of entity hit
} bsp_trace_t;

#endif  // BSPTRACE

// entity state
typedef struct bot_entitystate_s {
	int type;           // entity type
	int flags;          // entity flags
	vec3_t origin;      // origin of the entity
	vec3_t angles;      // angles of the model
	vec3_t old_origin;  // for lerping
	vec3_t mins;        // bounding box minimums
	vec3_t maxs;        // bounding box maximums
	int groundent;      // ground entity
	int solid;          // solid type
	int modelindex;     // model used
	int modelindex2;    // weapons, CTF flags, etc
	int frame;          // model frame number
	int event;          // impulse events -- muzzle flashes, footsteps, etc
	int eventParm;      // even parameter
	int weapon;         // determines weapon and flash model, etc
	int legsAnim;       // mask off ANIM_TOGGLEBIT
	int torsoAnim;      // mask off ANIM_TOGGLEBIT
} bot_entitystate_t;

// bot AI library imported functions
typedef struct botlib_import_s {
	// print messages from the bot library
	void(QDECL* Print)(int type, char* fmt, ...) Q_PRINTF_FUNC(2, 3);
	// trace a bbox through the world
	void (*Trace)(bsp_trace_t* trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask);
	// trace a bbox against a specific entity
	void (*EntityTrace)(bsp_trace_t* trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int entnum, int contentmask);
	// retrieve the contents at the given point
	int (*PointContents)(vec3_t point);
	// check if the point is in potential visible sight
	int (*inPVS)(vec3_t p1, vec3_t p2);
	// retrieve the BSP entity data lump
	char* (*BSPEntityData)(void);
	//
	void (*BSPModelMinsMaxsOrigin)(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin);
	// send a bot client command
	void (*BotClientCommand)(int client, char* command);
	// memory allocation
	void* (*GetMemory)(int size);   // allocate from Zone
	void (*FreeMemory)(void* ptr);  // free memory from Zone
	int (*AvailableMemory)(void);   // available Zone memory
	void* (*HunkAlloc)(int size);   // allocate from hunk
	// file system access
	int (*FS_FOpenFile)(const char* qpath, fileHandle_t* file, fsMode_t mode);
	int (*FS_Read)(void* buffer, int len, fileHandle_t f);
	int (*FS_Write)(const void* buffer, int len, fileHandle_t f);
	void (*FS_FCloseFile)(fileHandle_t f);
	int (*FS_Seek)(fileHandle_t f, long offset, int origin);
	// debug visualisation stuff
	int (*DebugLineCreate)(void);
	void (*DebugLineDelete)(int line);
	void (*DebugLineShow)(int line, vec3_t start, vec3_t end, int color);
	//
	int (*DebugPolygonCreate)(int color, int numPoints, vec3_t* points);
	void (*DebugPolygonDelete)(int id);
} botlib_import_t;

typedef struct aas_export_s {
	//-----------------------------------
	// be_aas_entity.h
	//-----------------------------------
	void (*AAS_EntityInfo)(int entnum, struct aas_entityinfo_s* info);
	//-----------------------------------
	// be_aas_main.h
	//-----------------------------------
	int (*AAS_Initialized)(void);
	void (*AAS_PresenceTypeBoundingBox)(int presencetype, vec3_t mins, vec3_t maxs);
	float (*AAS_Time)(void);
	//--------------------------------------------
	// be_aas_sample.c
	//--------------------------------------------
	int (*AAS_PointAreaNum)(vec3_t point);
	int (*AAS_TraceAreas)(vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas);
	int (*AAS_BBoxAreas)(vec3_t absmins, vec3_t absmaxs, int* areas, int maxareas);
	int (*AAS_AreaInfo)(int areanum, struct aas_areainfo_s* info);
	//--------------------------------------------
	// be_aas_bspq3.c
	//--------------------------------------------
	int (*AAS_PointContents)(vec3_t point);
	int (*AAS_NextBSPEntity)(int ent);
	int (*AAS_ValueForBSPEpairKey)(int ent, char* key, char* value, int size);
	int (*AAS_VectorForBSPEpairKey)(int ent, char* key, vec3_t v);
	int (*AAS_FloatForBSPEpairKey)(int ent, char* key, float* value);
	int (*AAS_IntForBSPEpairKey)(int ent, char* key, int* value);
	//--------------------------------------------
	// be_aas_reach.c
	//--------------------------------------------
	int (*AAS_AreaReachability)(int areanum);
	//--------------------------------------------
	// be_aas_route.c
	//--------------------------------------------
	int (*AAS_AreaTravelTimeToGoalArea)(int areanum, vec3_t origin, int goalareanum, int travelflags);
	int (*AAS_EnableRoutingArea)(int areanum, int enable);
	int (*AAS_PredictRoute)(struct aas_predictroute_s* route, int areanum, vec3_t origin, int goalareanum, int travelflags, int maxareas, int maxtime, int stopevent, int stopcontents, int stoptfl, int stopareanum);
	//--------------------------------------------
	// be_aas_move.c
	//--------------------------------------------
	int (*AAS_Swimming)(vec3_t origin);
	int (*AAS_PredictClientMovement)(struct aas_clientmove_s* move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize);
} aas_export_t;

typedef struct ea_export_s {
	void (*EA_Attack)(int client);
	void (*EA_Respawn)(int client);
	//
	void (*EA_SelectWeapon)(int client, int weapon);
	void (*EA_View)(int client, vec3_t viewangles);
	// retrieve the accumulated input
	void (*EA_GetInput)(int client, float thinktime, bot_input_t* input);
	void (*EA_ResetInput)(int client);
} ea_export_t;

typedef struct ai_export_s {
	//-----------------------------------
	// be_ai_move.h
	//-----------------------------------
	void (*BotResetMoveState)(int movestate);
	void (*BotMoveToGoal)(struct bot_moveresult_s* result, int movestate, struct bot_goal_s* goal, int travelflags);
	int (*BotMoveInDirection)(int movestate, vec3_t dir, float speed, int type);
	void (*BotResetAvoidReach)(int movestate);
	void (*BotResetLastAvoidReach)(int movestate);
	int (*BotMovementViewTarget)(int movestate, struct bot_goal_s* goal, int travelflags, float lookahead, vec3_t target);
	int (*BotPredictVisiblePosition)(vec3_t origin, int areanum, struct bot_goal_s* goal, int travelflags, vec3_t target);
	int (*BotAllocMoveState)(void);
	void (*BotFreeMoveState)(int handle);
	void (*BotInitMoveState)(int handle, struct bot_initmove_s* initmove);
	void (*BotAddAvoidSpot)(int movestate, vec3_t origin, float radius, int type);
	int (*BotTouchingGoal)(vec3_t origin, struct bot_goal_s* goal);
} ai_export_t;

// bot AI library exported functions
typedef struct botlib_export_s {
	// Area Awareness System functions
	aas_export_t aas;
	// Elementary Action functions
	ea_export_t ea;
	// AI functions
	ai_export_t ai;
	// setup the bot library, returns BLERR_
	int (*BotLibSetup)(void);
	// shutdown the bot library, returns BLERR_
	int (*BotLibShutdown)(void);
	// sets a library variable returns BLERR_
	int (*BotLibVarSet)(const char* var_name, const char* value);
	// start a frame in the bot library
	int (*BotLibStartFrame)(float time);
	// load a new map in the bot library
	int (*BotLibLoadMap)(const char* mapname);
	// entity updates
	int (*BotLibUpdateEntity)(int ent, bot_entitystate_t* state);
} botlib_export_t;

// linking of bot library
botlib_export_t* GetBotLibAPI(int apiVersion, botlib_import_t* import);

/* Library variables:

name:						default:			module(s):			description:

"basedir"					""					-					base directory
"gamedir"					""					be_interface.c		mod game directory
"log"						"0"					l_log.c				enable/disable creating a log file
"maxclients"				"128"				be_interface.c		maximum number of clients
"maxentities"				"4096"				be_interface.c		maximum number of entities
"bot_developer"				"0"					be_interface.c		bot developer mode

"phys_friction"				"6"					be_aas_move.c		ground friction
"phys_stopspeed"			"100"				be_aas_move.c		stop speed
"phys_gravity"				"800"				be_aas_move.c		gravity value
"phys_waterfriction"		"1"					be_aas_move.c		water friction
"phys_watergravity"			"400"				be_aas_move.c		gravity in water
"phys_maxvelocity"			"320"				be_aas_move.c		maximum velocity
"phys_maxwalkvelocity"		"320"				be_aas_move.c		maximum walk velocity
"phys_maxcrouchvelocity"	"100"				be_aas_move.c		maximum crouch velocity
"phys_maxswimvelocity"		"150"				be_aas_move.c		maximum swim velocity
"phys_walkaccelerate"		"10"				be_aas_move.c		walk acceleration
"phys_airaccelerate"		"1"					be_aas_move.c		air acceleration
"phys_swimaccelerate"		"4"					be_aas_move.c		swim acceleration
"phys_maxstep"				"18"				be_aas_move.c		maximum step height
"phys_maxsteepness"			"0.7"				be_aas_move.c		maximum floor steepness
"phys_maxbarrier"			"32"				be_aas_move.c		maximum barrier height
"phys_maxwaterjump"			"19"				be_aas_move.c		maximum waterjump height
"phys_jumpvel"				"270"				be_aas_move.c		jump z velocity
"phys_falldelta5"			"40"				be_aas_move.c
"phys_falldelta10"			"60"				be_aas_move.c
"rs_waterjump"				"400"				be_aas_move.c
"rs_teleport"				"50"				be_aas_move.c
"rs_barrierjump"			"100"				be_aas_move.c
"rs_startcrouch"			"300"				be_aas_move.c
"rs_startgrapple"			"500"				be_aas_move.c
"rs_startwalkoffledge"		"70"				be_aas_move.c
"rs_startjump"				"300"				be_aas_move.c
"rs_rocketjump"				"500"				be_aas_move.c
"rs_bfgjump"				"500"				be_aas_move.c
"rs_jumppad"				"250"				be_aas_move.c
"rs_aircontrolledjumppad"	"300"				be_aas_move.c
"rs_funcbob"				"300"				be_aas_move.c
"rs_startelevator"			"50"				be_aas_move.c
"rs_falldamage5"			"300"				be_aas_move.c
"rs_falldamage10"			"500"				be_aas_move.c
"rs_maxjumpfallheight"		"450"				be_aas_move.c

"max_aaslinks"				"4096"				be_aas_sample.c		maximum links in the AAS
"max_routingcache"			"4096"				be_aas_route.c		maximum routing cache size in KB
"forceclustering"			"0"					be_aas_main.c		force recalculation of clusters
"forcereachability"			"0"					be_aas_main.c		force recalculation of reachabilities
"forcewrite"				"0"					be_aas_main.c		force writing of aas file
"aasoptimize"				"0"					be_aas_main.c		enable aas optimization
"sv_mapChecksum"			"0"					be_aas_main.c		BSP file checksum
"bot_visualizejumppads"		"0"					be_aas_reach.c		visualize jump pads
*/
