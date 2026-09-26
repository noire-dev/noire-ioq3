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
// g_local.h -- local definitions for game module

#include "../qcommon/q_shared.h"
#include "../qcommon/vm_sharedsyscalls.h"
#include "bg_public.h"
#include "g_public.h"

//==================================================================

// the "gameversion" client command will print this plus compile date
#define GAMEVERSION BASEGAME

#define BODY_QUEUE_SIZE 8

#define INFINITE 1000000

#define FRAMETIME 100  // msec
#define CARNAGE_REWARD_TIME 3000
#define REWARD_SPRITE_TIME 2000

#define INTERMISSION_DELAY_TIME 1000
#define SP_INTERMISSION_DELAY_TIME 5000

// gentity->flags
#define FL_GODMODE 0x00000010
#define FL_NOTARGET 0x00000020
#define FL_TEAMMEMBER 0x00000400  // not the first on the team
#define FL_NO_KNOCKBACK 0x00000800
#define FL_DROPPED_ITEM 0x00001000
#define FL_NO_BOTS 0x00002000        // spawn point not for bot use
#define FL_NO_HUMANS 0x00004000      // spawn point just for bots
#define FL_FORCE_GESTURE 0x00008000  // force gesture on client

// movers are things like doors, plats, buttons, etc
typedef enum { MOVER_POS1, MOVER_POS2, MOVER_1TO2, MOVER_2TO1 } moverState_t;

#define SP_PODIUM_MODEL "models/mapobjects/podium/podium4.md3"

//============================================================================

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

struct gentity_s {
	entityState_t s;   // communicated by server to clients
	entityShared_t r;  // shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gclient_s* client;  // NULL if not a client

	bool inuse;

	char* classname;  // set in QuakeEd
	int spawnflags;   // set in QuakeEd

	bool neverFree;  // if true, FreeEntity will only unlink
	                 // bodyque uses this

	int flags;  // FL_* variables

	char* model;
	char* model2;
	int freetime;  // level.time when the object was freed

	int eventTime;  // events will be cleared EVENT_VALID_MSEC after set
	bool freeAfterEvent;
	bool unlinkAfterEvent;

	bool physicsObject;   // if true, it can be pushed by movers and fall off edges
	                      // all game items are physicsObjects,
	float physicsBounce;  // 1.0 = continuous bounce, 0.0 = no bounce
	int clipmask;         // brushes with this content value will be collided against
	                      // when moving.  items and corpses do not collide against
	                      // players, for instance

	// movers
	moverState_t moverState;
	int soundPos1;
	int sound1to2;
	int sound2to1;
	int soundPos2;
	int soundLoop;
	gentity_t* parent;
	gentity_t* nextTrain;
	gentity_t* prevTrain;
	vec3_t pos1, pos2;

	int timestamp;  // body queue sinking, etc

	char* target;
	char* targetname;
	char* team;

	float speed;
	vec3_t movedir;

	int nextthink;
	void (*think)(gentity_t* self);
	void (*reached)(gentity_t* self);  // movers call this when hitting endpoint
	void (*blocked)(gentity_t* self, gentity_t* other);
	void (*touch)(gentity_t* self, gentity_t* other, trace_t* trace);
	void (*use)(gentity_t* self, gentity_t* other, gentity_t* activator);
	void (*pain)(gentity_t* self, gentity_t* attacker, int damage);
	void (*die)(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod);

	int pain_debounce_time;
	int fly_sound_debounce_time;  // wind tunnel
	int last_move_time;

	int health;

	bool takedamage;

	int damage;
	int splashDamage;  // quad will increase this without increasing radius
	int splashRadius;
	int methodOfDeath;
	int splashMethodOfDeath;

	int count;

	gentity_t* chain;
	gentity_t* enemy;
	gentity_t* activator;
	gentity_t* teamchain;   // next entity in team
	gentity_t* teammaster;  // master of the team

	int watertype;
	int waterlevel;

	int noise_index;

	// timing variables
	float wait;
	float random;

	item_t* item;  // for bonus items

	int swep_list[WEAPONS_NUM];
	int swep_ammo[WEAPONS_NUM];
};

typedef enum { CON_DISCONNECTED, CON_CONNECTING, CON_CONNECTED } clientConnected_t;

//
#define MAX_NETNAME 36
#define MAX_VOTE_COUNT 3

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct {
	clientConnected_t connected;
	usercmd_t cmd;      // we would lose angles if not persistant
	bool localClient;   // true if "ip" info key is "localhost"
	bool initialSpawn;  // the first spawn should be at a cool location
	bool pmoveFixed;    //
	char netname[MAX_NETNAME];
	int enterTime;      // level.time the client entered the game
	int voteCount;      // to prevent people from constantly calling votes
	int teamVoteCount;  // to prevent people from constantly calling votes
	bool teamInfo;      // send team overlay updates?
} clientPersistant_t;

// this structure is cleared on each ClientSpawn(),
// except for 'client->pers'
struct gclient_s {
	// ps MUST be the first element, because the server expects it
	playerState_t ps;  // communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t pers;

	bool noclip;

	int lastCmdTime;  // level.time of last usercmd_t, for EF_CONNECTION
	                  // we can't just use pers.lastCommand.time, because
	                  // of the g_sycronousclients case
	int buttons;
	int oldbuttons;
	int latched_buttons;

	vec3_t oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int damage_armor;       // damage absorbed by armor
	int damage_blood;       // damage taken out of health
	int damage_knockback;   // impact damage
	vec3_t damage_from;     // origin for vector calculation
	bool damage_fromWorld;  // if true, don't use the damage_from vector

	int accurateCount;  // for "impressive" reward sound

	int accuracy_shots;  // total number of shots
	int accuracy_hits;   // total number of hits

	//
	int lastkilled_client;  // last client that this client killed
	int lasthurt_client;    // last client that damaged this client
	int lasthurt_mod;       // type of damage the client did

	// timers
	int respawnTime;         // can respawn when time > this, force after g_forcerespwan
	int inactivityTime;      // kick players when time > this
	bool inactivityWarning;  // true if the five seoond warning has been given
	int rewardTime;          // clear the EF_AWARD_IMPRESSIVE, etc when time > this

	int airOutTime;

	int lastKillTime;  // for multiple kill rewards

	int switchTeamTime;  // time the player switched teams

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int timeResidual;

	char* areabits;
};

//
// this structure is cleared as each map is entered
//
#define MAX_SPAWN_VARS 64
#define MAX_SPAWN_VARS_CHARS 4096

typedef struct {
	struct gclient_s* clients;  // [maxclients]

	struct gentity_s* gentities;
	int gentitySize;
	int num_entities;  // MAX_CLIENTS <= num_entities <= ENTITYNUM_MAX_NORMAL

	fileHandle_t logFile;

	// store latched cvars here that we want to get at often
	int maxclients;

	int framenum;
	int time;          // in msec
	int previousTime;  // so movers can back up when blocked

	int startTime;  // level.time the map was started

	bool restarted;  // waiting for a map_restart to fire

	int numConnectedClients;
	int numNonSpectatorClients;      // includes connecting clients
	int numPlayingClients;           // connected, non-spectators
	int sortedClients[MAX_CLIENTS];  // sorted by score

	int snd_fry;  // sound index for standing in lava

	// spawn variables
	bool spawning;  // the G_Spawn*() functions are valid
	int numSpawnVars;
	char* spawnVars[MAX_SPAWN_VARS][2];  // key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	char* changemap;
	bool readyToExit;  // at least one client wants to exit
	int exitTime;

	bool locationLinked;      // target_locations get linked
	gentity_t* locationHead;  // head of the location list
	int bodyQueIndex;         // dead bodies
	gentity_t* bodyQue[BODY_QUEUE_SIZE];
} level_locals_t;

//
// g_spawn.c
//
bool G_SpawnString(const char* key, const char* defaultString, char** out);
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
bool G_SpawnFloat(const char* key, const char* defaultString, float* out);
bool G_SpawnInt(const char* key, const char* defaultString, int* out);
bool G_SpawnVector(const char* key, const char* defaultString, float* out);
void G_SpawnEntitiesFromString(void);
char* G_NewString(const char* string);

//
// g_cmds.c
//
void Cmd_Score_f(gentity_t* ent);

//
// g_items.c
//
void G_RunItem(gentity_t* ent);
void RespawnItem(gentity_t* ent);

void PrecacheItem(item_t* it);
gentity_t* Drop_Item(gentity_t* ent, item_t* item);
gentity_t* LaunchItem(item_t* item, vec3_t origin, vec3_t velocity);
void SetRespawn(gentity_t* ent, float delay);
void G_SpawnItem(gentity_t* ent, item_t* item);
void FinishSpawningItem(gentity_t* ent);
void Think_Weapon(gentity_t* ent);
int ArmorIndex(gentity_t* ent);
void Add_Ammo(gentity_t* ent, int weapon, int count);
void Touch_Item(gentity_t* ent, gentity_t* other, trace_t* trace);

//
// g_utils.c
//
int G_ModelIndex(char* name);
int G_SoundIndex(char* name);
void G_KillBox(gentity_t* ent);
gentity_t* G_Find(gentity_t* from, int fieldofs, const char* match);
gentity_t* G_PickTarget(char* targetname);
void G_UseTargets(gentity_t* ent, gentity_t* activator);
void G_SetMovedir(vec3_t angles, vec3_t movedir);

void G_InitGentity(gentity_t* e);
gentity_t* G_Spawn(void);
gentity_t* G_TempEntity(vec3_t origin, int event);
void G_Sound(gentity_t* ent, int channel, int soundIndex);
void G_FreeEntity(gentity_t* e);
bool G_EntitiesFree(void);

void G_TouchTriggers(gentity_t* ent);

float* tv(float x, float y, float z);
char* vtos(const vec3_t v);

float vectoyaw(const vec3_t vec);

void G_AddPredictableEvent(gentity_t* ent, int event, int eventParm);
void G_AddEvent(gentity_t* ent, int event, int eventParm);
void G_SetOrigin(gentity_t* ent, vec3_t origin);
void AddRemap(const char* oldShader, const char* newShader, float timeOffset);
const char* BuildShaderStateConfig(void);

//
// g_combat.c
//
bool CanDamage(gentity_t* targ, vec3_t origin);
void G_Damage(gentity_t* targ, gentity_t* inflictor, gentity_t* attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod);
bool G_RadiusDamage(vec3_t origin, gentity_t* attacker, float damage, float radius, gentity_t* ignore, int mod);
int G_InvulnerabilityEffect(gentity_t* targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir);
void body_die(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int meansOfDeath);
void TossClientItems(gentity_t* self);
void TossClientCubes(gentity_t* self);

// damage flags
#define DAMAGE_RADIUS 0x00000001         // damage was indirect
#define DAMAGE_NO_ARMOR 0x00000002       // armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK 0x00000004   // do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION 0x00000008  // armor, shields, invulnerability, and godmode have no effect

//
// g_missile.c
//
void G_RunMissile(gentity_t* ent);

gentity_t* fire_plasma(gentity_t* self, vec3_t start, vec3_t aimdir);
gentity_t* fire_grenade(gentity_t* self, vec3_t start, vec3_t aimdir);
gentity_t* fire_rocket(gentity_t* self, vec3_t start, vec3_t dir);
gentity_t* fire_bfg(gentity_t* self, vec3_t start, vec3_t dir);

//
// g_mover.c
//
void G_RunMover(gentity_t* ent);
void Touch_DoorTrigger(gentity_t* ent, gentity_t* other, trace_t* trace);

//
// g_trigger.c
//
void trigger_teleporter_touch(gentity_t* self, gentity_t* other, trace_t* trace);

//
// g_misc.c
//
void TeleportPlayer(gentity_t* player, vec3_t origin, vec3_t angles);

//
// g_weapon.c
//
bool LogAccuracyHit(gentity_t* target, gentity_t* attacker);
void CalcMuzzlePoint(gentity_t* ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint);
void SnapVectorTowards(vec3_t v, vec3_t to);
bool CheckGauntletAttack(gentity_t* ent);

//
// g_client.c
//
void SetClientViewAngle(gentity_t* ent, vec3_t angle);
gentity_t* SelectSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles, bool isbot);
void CopyToBodyQue(gentity_t* ent);
void ClientRespawn(gentity_t* ent);
void InitBodyQue(void);
void ClientSpawn(gentity_t* ent);
void player_die(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod);
bool SpotWouldTelefrag(gentity_t* spot);

//
// g_svcmds.c
//
bool ConsoleCommand(void);

//
// g_weapon.c
//
void FireWeapon(gentity_t* ent);

//
// g_cmds.c
//
void DeathmatchScoreboardMessage(gentity_t* ent);
void G_SendSwepWeapons(gentity_t* ent);
void G_SendSpawnSwepWeapons(gentity_t* ent);

//
// g_main.c
//
void G_RunThink(gentity_t* ent);
void SendScoreboardMessageToAllClients(void);
void QDECL G_Printf(const char* fmt, ...) Q_PRINTF_FUNC(1, 2);
void QDECL G_Error(const char* fmt, ...) Q_NO_RETURN Q_PRINTF_FUNC(1, 2);

//
// g_client.c
//
char* ClientConnect(int clientNum, bool firstTime, bool isBot);
void ClientUserinfoChanged(int clientNum);
void ClientDisconnect(int clientNum);
void ClientBegin(int clientNum);
void ClientCommand(int clientNum);

//
// g_active.c
//
void ClientThink(int clientNum);
void ClientEndFrame(gentity_t* ent);
void G_RunClient(gentity_t* ent);
bool G_CheckWeapon(int clientNum, int wp, int finish);
int G_CheckWeaponAmmo(int clientNum, int wp);
void PM_Add_SwepAmmo(int clientNum, int wp, int count);

//
// g_mem.c
//
void* G_Alloc(int size);
void G_InitMemory(void);

//
// g_bot.c
//
void G_InitBots(bool restart);
char* G_GetBotInfoByNumber(int num);
char* G_GetBotInfoByName(const char* name);
void G_RemoveQueuedBotBegin(int clientNum);
bool G_BotConnect(int clientNum, bool restart);
void Svcmd_AddBot_f(void);
void BotInterbreedEndMatch(void);

// ai_main.c
#define MAX_FILEPATH 144

// bot settings
typedef struct bot_settings_s {
	char characterfile[MAX_FILEPATH];
	float skill;
} bot_settings_t;

int BotAISetup(int restart);
int BotAIShutdown(int restart);
int BotAILoadMap(int restart);
int BotAISetupClient(int client, struct bot_settings_s* settings, bool restart);
int BotAIShutdownClient(int client, bool restart);
int BotAIStartFrame(int time);
void BotTestAAS(vec3_t origin);

extern level_locals_t level;
extern gentity_t g_entities[MAX_GENTITIES];

#define FOFS(x) ((size_t)&(((gentity_t*)0)->x))

extern vmCvar_t g_dedicated;
extern vmCvar_t g_cheats;
extern vmCvar_t g_maxclients;      // allow this many total, including spectators
extern vmCvar_t g_maxGameClients;  // allow this many active

extern vmCvar_t g_dmflags;
extern vmCvar_t g_fraglimit;
extern vmCvar_t g_timelimit;
extern vmCvar_t g_capturelimit;
extern vmCvar_t g_friendlyFire;
extern vmCvar_t g_password;
extern vmCvar_t g_needpass;
extern vmCvar_t g_gravity;
extern vmCvar_t g_speed;
extern vmCvar_t g_knockback;
extern vmCvar_t g_quadfactor;
extern vmCvar_t g_forcerespawn;
extern vmCvar_t g_inactivity;
extern vmCvar_t g_debugMove;
extern vmCvar_t g_debugAlloc;
extern vmCvar_t g_debugDamage;
extern vmCvar_t g_weaponRespawn;
extern vmCvar_t g_weaponTeamRespawn;
extern vmCvar_t g_synchronousClients;
extern vmCvar_t g_motd;
extern vmCvar_t g_blood;
extern vmCvar_t g_banIPs;
extern vmCvar_t g_filterBan;
extern vmCvar_t g_obeliskHealth;
extern vmCvar_t g_obeliskRegenPeriod;
extern vmCvar_t g_obeliskRegenAmount;
extern vmCvar_t g_obeliskRespawnDelay;
extern vmCvar_t g_cubeTimeout;
extern vmCvar_t g_redteam;
extern vmCvar_t g_blueteam;
extern vmCvar_t g_smoothClients;
extern vmCvar_t g_enableDust;
extern vmCvar_t g_enableBreath;

void trap_LocateGameData(gentity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* gameClients, int sizeofGameClient);
void trap_DropClient(int clientNum, const char* reason);
void trap_SendServerCommand(int clientNum, const char* text);
void trap_SetConfigstring(int num, const char* string);
void trap_GetConfigstring(int num, char* buffer, int bufferSize);
void trap_GetUserinfo(int num, char* buffer, int bufferSize);
void trap_SetUserinfo(int num, const char* buffer);
void trap_GetServerinfo(char* buffer, int bufferSize);
void trap_SetBrushModel(gentity_t* ent, const char* name);
void trap_Trace(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
int trap_PointContents(const vec3_t point, int passEntityNum);
bool trap_InPVS(const vec3_t p1, const vec3_t p2);
bool trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2);
void trap_AdjustAreaPortalState(gentity_t* ent, bool open);
bool trap_AreasConnected(int area1, int area2);
void trap_LinkEntity(gentity_t* ent);
void trap_UnlinkEntity(gentity_t* ent);
int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int* entityList, int maxcount);
bool trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t* ent);
int trap_BotAllocateClient(void);
void trap_BotFreeClient(int clientNum);
void trap_GetUsercmd(int clientNum, usercmd_t* cmd);
bool trap_GetEntityToken(char* buffer, int bufferSize);

int trap_DebugPolygonCreate(int color, int numPoints, vec3_t* points);
void trap_DebugPolygonDelete(int id);

int trap_BotLibSetup(void);
int trap_BotLibShutdown(void);
int trap_BotLibVarSet(char* var_name, char* value);
int trap_BotLibVarGet(char* var_name, char* value, int size);
int trap_BotLibDefine(char* string);
int trap_BotLibStartFrame(float time);
int trap_BotLibLoadMap(const char* mapname);
int trap_BotLibUpdateEntity(int ent, void /* struct bot_updateentity_s */* bue);
int trap_BotLibTest(int parm0, char* parm1, vec3_t parm2, vec3_t parm3);

int trap_BotGetSnapshotEntity(int clientNum, int sequence);
int trap_BotGetServerCommand(int clientNum, char* message, int size);
void trap_BotUserCommand(int client, usercmd_t* ucmd);

int trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int* areas, int maxareas);
int trap_AAS_AreaInfo(int areanum, void /* struct aas_areainfo_s */* info);
void trap_AAS_EntityInfo(int entnum, void /* struct aas_entityinfo_s */* info);

int trap_AAS_Initialized(void);
void trap_AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs);
float trap_AAS_Time(void);

int trap_AAS_PointAreaNum(vec3_t point);
int trap_AAS_PointReachabilityAreaIndex(vec3_t point);
int trap_AAS_TraceAreas(vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas);

int trap_AAS_PointContents(vec3_t point);
int trap_AAS_NextBSPEntity(int ent);
int trap_AAS_ValueForBSPEpairKey(int ent, char* key, char* value, int size);
int trap_AAS_VectorForBSPEpairKey(int ent, char* key, vec3_t v);
int trap_AAS_FloatForBSPEpairKey(int ent, char* key, float* value);
int trap_AAS_IntForBSPEpairKey(int ent, char* key, int* value);

int trap_AAS_AreaReachability(int areanum);

int trap_AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags);
int trap_AAS_EnableRoutingArea(int areanum, int enable);
int trap_AAS_PredictRoute(void /*struct aas_predictroute_s*/* route, int areanum, vec3_t origin, int goalareanum, int travelflags, int maxareas, int maxtime, int stopevent, int stopcontents, int stoptfl, int stopareanum);

int trap_AAS_AlternativeRouteGoals(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags, void /*struct aas_altroutegoal_s*/* altroutegoals, int maxaltroutegoals, int type);
int trap_AAS_Swimming(vec3_t origin);
int trap_AAS_PredictClientMovement(void /* aas_clientmove_s */* move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize);

void trap_EA_Say(int client, char* str);
void trap_EA_SayTeam(int client, char* str);
void trap_EA_Command(int client, char* command);

void trap_EA_Action(int client, int action);
void trap_EA_Gesture(int client);
void trap_EA_Talk(int client);
void trap_EA_Attack(int client);
void trap_EA_Use(int client);
void trap_EA_Respawn(int client);
void trap_EA_Crouch(int client);
void trap_EA_MoveUp(int client);
void trap_EA_MoveDown(int client);
void trap_EA_MoveForward(int client);
void trap_EA_MoveBack(int client);
void trap_EA_MoveLeft(int client);
void trap_EA_MoveRight(int client);
void trap_EA_SelectWeapon(int client, int weapon);
void trap_EA_Jump(int client);
void trap_EA_DelayedJump(int client);
void trap_EA_Move(int client, vec3_t dir, float speed);
void trap_EA_View(int client, vec3_t viewangles);

void trap_EA_EndRegular(int client, float thinktime);
void trap_EA_GetInput(int client, float thinktime, void /* struct bot_input_s */* input);
void trap_EA_ResetInput(int client);

int trap_BotLoadCharacter(char* charfile, float skill);
void trap_BotFreeCharacter(int character);
float trap_Characteristic_Float(int character, int index);
float trap_Characteristic_BFloat(int character, int index, float min, float max);
int trap_Characteristic_Integer(int character, int index);
int trap_Characteristic_BInteger(int character, int index, int min, int max);
void trap_Characteristic_String(int character, int index, char* buf, int size);

int trap_BotAllocChatState(void);
void trap_BotFreeChatState(int handle);
void trap_BotQueueConsoleMessage(int chatstate, int type, char* message);
void trap_BotRemoveConsoleMessage(int chatstate, int handle);
int trap_BotNextConsoleMessage(int chatstate, void /* struct bot_consolemessage_s */* cm);
int trap_BotNumConsoleMessages(int chatstate);
void trap_BotInitialChat(int chatstate, char* type, int mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7);
int trap_BotNumInitialChats(int chatstate, char* type);
int trap_BotReplyChat(int chatstate, char* message, int mcontext, int vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7);
int trap_BotChatLength(int chatstate);
void trap_BotEnterChat(int chatstate, int client, int sendto);
void trap_BotGetChatMessage(int chatstate, char* buf, int size);
int trap_StringContains(char* str1, char* str2, int casesensitive);
int trap_BotFindMatch(char* str, void /* struct bot_match_s */* match, unsigned long int context);
void trap_BotMatchVariable(void /* struct bot_match_s */* match, int variable, char* buf, int size);
void trap_UnifyWhiteSpaces(char* string);
void trap_BotReplaceSynonyms(char* string, unsigned long int context);
int trap_BotLoadChatFile(int chatstate, char* chatfile, char* chatname);
void trap_BotSetChatGender(int chatstate, int gender);
void trap_BotSetChatName(int chatstate, char* name, int client);
void trap_BotResetGoalState(int goalstate);
void trap_BotRemoveFromAvoidGoals(int goalstate, int number);
void trap_BotResetAvoidGoals(int goalstate);
void trap_BotPushGoal(int goalstate, void /* struct bot_goal_s */* goal);
void trap_BotPopGoal(int goalstate);
void trap_BotEmptyGoalStack(int goalstate);
void trap_BotDumpAvoidGoals(int goalstate);
void trap_BotDumpGoalStack(int goalstate);
void trap_BotGoalName(int number, char* name, int size);
int trap_BotGetTopGoal(int goalstate, void /* struct bot_goal_s */* goal);
int trap_BotGetSecondGoal(int goalstate, void /* struct bot_goal_s */* goal);
int trap_BotChooseLTGItem(int goalstate, vec3_t origin, int* inventory, int travelflags);
int trap_BotChooseNBGItem(int goalstate, vec3_t origin, int* inventory, int travelflags, void /* struct bot_goal_s */* ltg, float maxtime);
int trap_BotTouchingGoal(vec3_t origin, void /* struct bot_goal_s */* goal);
int trap_BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */* goal);
int trap_BotGetNextCampSpotGoal(int num, void /* struct bot_goal_s */* goal);
int trap_BotGetMapLocationGoal(char* name, void /* struct bot_goal_s */* goal);
int trap_BotGetLevelItemGoal(int index, char* classname, void /* struct bot_goal_s */* goal);
float trap_BotAvoidGoalTime(int goalstate, int number);
void trap_BotSetAvoidGoalTime(int goalstate, int number, float avoidtime);
void trap_BotInitLevelItems(void);
void trap_BotUpdateEntityItems(void);
int trap_BotLoadItemWeights(int goalstate, char* filename);
void trap_BotFreeItemWeights(int goalstate);
void trap_BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child);
void trap_BotSaveGoalFuzzyLogic(int goalstate, char* filename);
void trap_BotMutateGoalFuzzyLogic(int goalstate, float range);
int trap_BotAllocGoalState(int state);
void trap_BotFreeGoalState(int handle);

void trap_BotResetMoveState(int movestate);
void trap_BotMoveToGoal(void /* struct bot_moveresult_s */* result, int movestate, void /* struct bot_goal_s */* goal, int travelflags);
int trap_BotMoveInDirection(int movestate, vec3_t dir, float speed, int type);
void trap_BotResetAvoidReach(int movestate);
void trap_BotResetLastAvoidReach(int movestate);
int trap_BotReachabilityArea(vec3_t origin, int testground);
int trap_BotMovementViewTarget(int movestate, void /* struct bot_goal_s */* goal, int travelflags, float lookahead, vec3_t target);
int trap_BotPredictVisiblePosition(vec3_t origin, int areanum, void /* struct bot_goal_s */* goal, int travelflags, vec3_t target);
int trap_BotAllocMoveState(void);
void trap_BotFreeMoveState(int handle);
void trap_BotInitMoveState(int handle, void /* struct bot_initmove_s */* initmove);
void trap_BotAddAvoidSpot(int movestate, vec3_t origin, float radius, int type);

int trap_BotChooseBestFightWeapon(int weaponstate, int* inventory);
void trap_BotGetWeaponInfo(int weaponstate, int weapon, void /* struct weaponinfo_s */* weaponinfo);
int trap_BotLoadWeaponWeights(int weaponstate, char* filename);
int trap_BotAllocWeaponState(void);
void trap_BotFreeWeaponState(int weaponstate);
void trap_BotResetWeaponState(int weaponstate);

int trap_GeneticParentsAndChildSelection(int numranks, float* ranks, int* parent1, int* parent2, int* child);
