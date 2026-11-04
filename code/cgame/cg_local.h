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
#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_types.h"
#include "../qcommon/vm_sharedsyscalls.h"
#include "../qcommon/vm_sharedsyscalls_client.h"
#include "../game/bg_public.h"
#include "cg_public.h"

// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#define ITEM_SCALEUP_TIME 1000
#define ZOOM_TIME 150
#define MUZZLE_FLASH_TIME 20
#define SINK_TIME 1000  // time for fragments to sink into ground before going away

#define MAX_VERTS_ON_POLY 10
#define MAX_MARK_POLYS 256

#define NUM_CROSSHAIRS 10

#define DEFAULT_MODEL "kleiner"

#define MAX_NOTIFICATIONS 18
#define NOTIFICATION_FADE_TIME 500

#define NOTIFY_INFO 1
#define NOTIFY_UNDO 2
#define NOTIFY_ITEM 3
#define NOTIFY_KILL 4
#define NOTIFY_CUSTOM 5

#define MAX_3D_STRING_QUEUE 512

typedef struct {
	float x, y, z;
	const char* str;
	int style;
	vec4_t color;
	float fontSize;
	float min, max;
	bool useTrace;
} queued3DString_t;

typedef enum {
	FOOTSTEP_CHAINLINK,
	FOOTSTEP_CONCRETE,
	FOOTSTEP_DIRT,
	FOOTSTEP_DUCT,
	FOOTSTEP_GRASS,
	FOOTSTEP_GRAVEL,
	FOOTSTEP_LADDER,
	FOOTSTEP_METAL,
	FOOTSTEP_METALGRATE,
	FOOTSTEP_MUD,
	FOOTSTEP_SAND,
	FOOTSTEP_SLOSH,
	FOOTSTEP_TILE,
	FOOTSTEP_WOOD,
	FOOTSTEP_WOODPANEL,

	FOOTSTEP_TOTAL
} footstep_t;

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// ============================================================
// Animation state smoothing (crossfade between different
// animation states: idle <-> run <-> jump <-> death, ...)
//
// ANIM_SMOOTHING      1 = blend the pose over ANIM_SMOOTHING_TIME
//                         milliseconds when the animation changes,
//                     0 = instant frame switch (vanilla behavior,
//                         the blend fields are compiled out)
// ANIM_SMOOTHING_TIME crossfade duration in milliseconds;
//                     0 also effectively disables the blend
// ============================================================
#define ANIM_SMOOTHING 1
#define ANIM_SMOOTHING_TIME 180

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
// With ANIM_SMOOTHING the switch additionally crossfades from the frozen
// pose of the previous animation instead of snapping to the new first frame
typedef struct {
	int oldFrame;
	int oldFrameTime;  // time when ->oldFrame was exactly on

	int frame;
	int frameTime;  // time when ->frame will be exactly on

	float backlerp;

	int animationNumber;  // may include ANIM_TOGGLEBIT
	animation_t* animation;
	int animationTime;  // time when the first frame of the animation will be exact

#if ANIM_SMOOTHING
	// crossfade state: the pose we are fading out from
	animation_t* blendFromAnimation;  // animation that was playing before the switch
	int blendFromFrame;               // frame of that animation frozen at the switch
	int blendStartTime;               // cg.time when the switch happened
#endif
} lerpFrame_t;

typedef struct {
	lerpFrame_t legs;

	int lightningFiring;

	// Data-driven fire FX state
	bool fireFxPending;  // set by CG_FireWeapon, consumed in CG_AddPlayerWeapon (burst weapons)
	int lastFireFxTime;  // throttle for continuous fire FX
} playerEntity_t;

// Notifications
typedef struct {
	char text[256];
	char picPath[256];
	int type;
	int number;
	int startTime;
	bool active;
} notification_t;

//=================================================

// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s {
	entityState_t currentState;  // from cg.frame
	entityState_t nextState;     // from cg.nextFrame, if available
	bool interpolate;            // true if next is valid to interpolate to
	bool currentValid;           // true if cg.frame holds this entity

	int muzzleFlashTime;
	int previousEvent;

	int trailTime;  // so missile trails can handle dropped initial packets
	int miscTime;

	int snapShotTime;  // last time this entity was found in a snapshot

	playerEntity_t pe;

	// exact interpolated position of entity on this frame
	vec3_t lerpOrigin;
	vec3_t lerpAngles;
} centity_t;

//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independently from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s *prevMark, *nextMark;
	int time;
	qhandle_t markShader;
	bool alphaFade;  // fade alpha instead of rgb
	float color[4];
	poly_t poly;
	polyVert_t verts[MAX_VERTS_ON_POLY];
} markPoly_t;

typedef enum {
	LE_EXPLOSION,
	LE_FRAGMENT,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
} leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE = 0x0001,  // do not scale size over time
	LEF_TUMBLE = 0x0002,           // tumble over time, used for ejecting shells
} leFlag_t;

typedef enum { LEMT_NONE, LEMT_BURN, LEMT_BLOOD } leMarkType_t;  // fragment local entities can leave marks on walls

typedef struct localEntity_s {
	struct localEntity_s *prev, *next;
	leType_t leType;
	int leFlags;

	int startTime;
	int endTime;
	int fadeInTime;

	float lifeRate;  // 1.0 / (endTime - startTime)

	trajectory_t pos;
	trajectory_t angles;

	float bounceFactor;  // 0.0 = no bounce, 1.0 = perfect

	float color[4];

	float radius;

	float light;
	vec3_t lightColor;

	leMarkType_t leMarkType;  // mark to leave on fragment impact

	refEntity_t refEntity;
} localEntity_t;

//======================================================================

typedef struct {
	int client;
	int ping;
} score_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
typedef struct {
	bool infoValid;

	char name[MAX_NETNAME];

	int physR;
	int physG;
	int physB;
	int grabbedEntityNum;

	// when clientinfo is changed, the loading of models/skins/sounds
	// can be deferred until you are dead, to prevent hitches in
	// gameplay
	char modelName[MAX_QPATH];
	char skinName[MAX_QPATH];
	bool deferred;

	// A single IQM model and one material skin replace lower/upper/head MD3s.
	qhandle_t model;
	qhandle_t modelSkin;
	qhandle_t modelIcon;

	// Indexed by animNumber_t; model/animation.cfg supplies IQM frame ranges.
	animation_t animations[MAX_TOTALANIMATIONS];

	// "headSettings up fwd" from animation.cfg: per-model fine-tuning of
	// the first-person camera, same semantics as cg_firstPerson_Up / _Fwd.
	float headUp;
	float headFwd;

	// "weaponScale s" from animation.cfg: per-model size of the weapon held
	// in the hand. Weapon models are authored for the full-size valveBiped
	// hand, so models smaller than that need e.g. "weaponScale 0.7".
	float weaponScale;

	char animFile[MAX_QPATH];
} clientInfo_t;

// Minimal runtime cache for a weapon. All presentational data now lives in
// jsd_weaponFx (see cg_local.h) so weapons can be fully described by the data
// table alone (JS migration target).
typedef struct weaponInfo_s {
	item_t* item;
	qhandle_t weaponModel;
	qhandle_t weaponIcon;
	vec3_t weaponMidpoint;
} weaponInfo_t;

// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	bool registered;
	qhandle_t models;
	qhandle_t icon;
} itemInfo_t;

//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS 16

typedef struct {
	int clientFrame;  // incremented each frame

	int clientNum;

	bool demoPlayback;
	bool loading;  // don't defer players at initial startup

	// there are only one or two snapshot_t that are relevant at a time
	int latestSnapshotNum;   // the number of snapshots the client system has received
	int latestSnapshotTime;  // the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t* snap;      // cg.snap->serverTime <= cg.time
	snapshot_t* nextSnap;  // cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t activeSnapshots[2];

	float frameInterpolation;  // (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	bool thisFrameTeleport;
	bool nextFrameTeleport;

	int frametime;  // cg.time - cg.oldTime

	int time;     // this is the time value that the client
	              // is rendering at.
	int oldTime;  // time at last frame, used for missile trails and prediction checking

	int physicsTime;  // either cg.snap->time or cg.nextSnap->time

	int timelimitWarnings;  // 5 min, 1 min, overtime
	int fraglimitWarnings;

	bool mapRestart;  // set on a map restart to set back the weapon

	bool renderingThirdPerson;  // during deaths, chasecams, etc

	// prediction state
	bool hyperspace;  // true if prediction has hit a trigger_teleport
	playerState_t predictedPlayerState;
	centity_t predictedPlayerEntity;
	bool validPPS;  // clear until the first call to CG_PredictPlayerState
	int predictedErrorTime;
	vec3_t predictedError;

	int eventSequence;
	int predictableEvents[MAX_PREDICTED_EVENTS];

	// input state sent to server
	int weaponSelect;
	int weaponSelectTime;
	int swep_listcl[MAX_WEAPONS];
	int swep_spawncl[MAX_WEAPONS];  // stores spawn weapons

	float savedSens;  // physgun

	// auto rotating items
	vec3_t autoAngles;
	vec3_t autoAxis[3];
	vec3_t autoAnglesFast;
	vec3_t autoAxisFast[3];

	// view rendering
	refdef_t refdef;
	vec3_t refdefViewAngles;  // will be converted to refdef.viewaxis

	// zoom key
	bool zoomed;
	int zoomTime;
	float zoomSensitivity;

	// information screen text during loading
	char infoScreenText[MAX_STRING_CHARS];
	float infoScreenValue;

	// scoreboard
	int scoresRequestTime;
	int numScores;
	score_t scores[MAX_CLIENTS];
	bool showScores;

	// sound buffer mainly for announcer sounds
	int soundBufferIn;
	int soundBufferOut;
	int soundTime;

	// temp working variables for player view
	int nextOrbitTime;

	notification_t notifications[MAX_NOTIFICATIONS];
} cg_t;

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t
typedef struct {
	qhandle_t whiteShader;

	qhandle_t deferShader;

	qhandle_t crosshairShader[NUM_CROSSHAIRS];
	qhandle_t noammoShader;

	qhandle_t smokePuffShader;

	// wall mark shaders
	qhandle_t bloodMarkShader;
	qhandle_t burnMarkShader;

	// Assets Noire's Mod
	qhandle_t errIcon;
	qhandle_t notifyIcon;
	qhandle_t undoIcon;
	sfxHandle_t notifySound[4];
	sfxHandle_t undoSound;

	// sounds
	sfxHandle_t selectSound;
	sfxHandle_t footsteps[FOOTSTEP_TOTAL][4];
	sfxHandle_t talkSound;
	sfxHandle_t landSound;
	sfxHandle_t fallSound;

	sfxHandle_t oneMinuteSound;
	sfxHandle_t fiveMinuteSound;
	sfxHandle_t suddenDeathSound;

	sfxHandle_t threeFragSound;
	sfxHandle_t twoFragSound;
	sfxHandle_t oneFragSound;

	sfxHandle_t hitSound;
	sfxHandle_t hitTeamSound;
	sfxHandle_t hgrenb1aSound;
	sfxHandle_t hgrenb2aSound;

	sfxHandle_t gravityLaunch[4];
	sfxHandle_t waterSplash[3];
} cgMedia_t;

#define CONSOLE_MAXHEIGHT 16
#define CONSOLE_WIDTH 512
typedef struct {
	char msgs[CONSOLE_MAXHEIGHT][CONSOLE_WIDTH];
	int msgTimes[CONSOLE_MAXHEIGHT];
	int insertIdx;
	int displayIdx;
} console_t;

// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct {
	gameState_t gameState;  // gamestate from server
	glconfig_t glconfig;    // rendering configuration

	int serverCommandSequence;  // reliable command stream counter
	int processedSnapshotNum;   // the number of snapshots cgame has requested

	int dmflags;
	int fraglimit;
	int timelimit;
	int maxclients;
	char mapname[MAX_QPATH];
	char redTeam[MAX_QPATH];
	char blueTeam[MAX_QPATH];

	int levelStartTime;

	int scores1, scores2;  // from configstrings

	//
	// locally derived information from gamestate
	//
	qhandle_t gameModels[MAX_MODELS];
	sfxHandle_t gameSounds[MAX_SOUNDS];

	int numInlineModels;
	qhandle_t inlineDrawModel[MAX_MODELS];
	vec3_t inlineModelMidpoints[MAX_MODELS];

	clientInfo_t clientinfo[MAX_CLIENTS];

	console_t chat;

	// media
	cgMedia_t media;

} cgs_t;

//==============================================================================

// Data-driven weapon effects table (see cg_weapons.c)
// String paths are fixed buffers (MAX_QPATH) so they can be written from JS through VM_APIWEAPONFX
typedef struct {
	animType_t animType;  // Selects the weapon-specific IQM locomotion block.

	// Garry's Mod-style weapon selector metadata.
	// Sections are laid out left-to-right (1..6); sectionOrder controls
	// the top-to-bottom order inside each section.
	int weaponSection;
	int sectionOrder;
	bool colorizeItemIcon;
	// Hit effects
	float hitLightColor[3];
	float hitLightRadius;
	int hitLightTime;
	float hitParticleColor[4];
	int hitParticleCount;
	float hitParticleSpeed;
	int hitParticleFlags;
	float hitParticleGravity[3];
	float hitParticleSize;
	float hitParticleLifetime;
	char hitParticleShader[MAX_QPATH];

	// Fire (muzzle) effects
	bool continuousFire;
	bool suppressRepeatFire;  // don't replay fire sound/fx on repeated EV_FIRE_WEAPON while already firing
	float fireLightColor[3];
	float fireLightRadius;
	int fireLightTime;
	float fireParticleColor[4];
	int fireParticleCount;
	float fireParticleSpeed;
	int fireParticleFlags;
	float fireParticleGravity[3];
	float fireParticleSize;
	float fireParticleLifetime;
	char fireParticleShader[MAX_QPATH];

	// Missile trail
	float trailColor[4];
	int trailCount;
	float trailSpeed;
	int trailFlags;
	float trailGravity[3];
	float trailSize;
	float trailLifetime;
	char trailShader[MAX_QPATH];
	int trailInterval;

	// Missile light (per-frame)
	float missileLightColor[3];
	float missileLightRadius;

	// Hitscan tracer (particle based)
	float tracerChance;
	float tracerLength;
	char tracerParticleShader[MAX_QPATH];
	float tracerParticleColor[4];
	int tracerParticleCount;
	float tracerParticleSpeed;
	int tracerParticleFlags;
	float tracerParticleGravity[3];
	float tracerParticleSize;
	float tracerParticleLifetime;
	char tracerSound[MAX_QPATH];

	// Beam (rail / lightning / physgun / toolgun)
	bool isBeam;
	char beamShader[MAX_QPATH];
	float beamColor[4];
	bool beamColorizeByClient;
	float beamStartOffset[3];
	float beamEndOffset[3];

	// Sounds
	char readySound[MAX_QPATH];
	char fireSound[4][MAX_QPATH];
	char continuousFireSound[MAX_QPATH];
	char missileFlySound[MAX_QPATH];
	char missileBounceSound[MAX_QPATH];
	char missileExplodeSound[MAX_QPATH];

	// Missile visual
	char missileModel[MAX_QPATH];
	char missileSprite[MAX_QPATH];
	int missileRenderfx;

	// Model color override (0 = none, 1 = client physRGB, 2 = fixed modelColor)
	int modelColorSource;
	float modelColor[3];

	// Emit red blood particles on flesh impact (grenade/rocket style)
	bool explodeOnFleshHit;

	// Weapon visual model overrides (empty = derive from item->model)
	char flashModel[MAX_QPATH];

	// Tag on the weapon model where the muzzle flash and fire FX originate
	char muzzleBone[MAX_QPATH];

	// Brass (shells)
	char brassModel[MAX_QPATH];
	int brassCount;
	float brassOffset[3];
	float brassVelocity[3];
	float brassVelocitySpread[3];
	float brassBounceFactor;
	int brassLifetimeMult;
	float brassSpinDelta[3];

	// Runtime-registered handles (populated by CG_RegisterWeapon)
	qhandle_t hitParticleHandle;
	qhandle_t fireParticleHandle;
	qhandle_t trailHandle;
	qhandle_t tracerHandle;
	qhandle_t beamHandle;
	qhandle_t missileModelHandle;
	qhandle_t missileSpriteHandle;
	qhandle_t flashModelHandle;
	qhandle_t brassModelHandle;
	sfxHandle_t readySoundHandle;
	sfxHandle_t fireSoundHandle[4];
	sfxHandle_t continuousFireSoundHandle;
	sfxHandle_t missileFlySoundHandle;
	sfxHandle_t missileBounceSoundHandle;
	sfxHandle_t missileExplodeSoundHandle;
	sfxHandle_t tracerSoundHandle;

	bool registered;
} weaponFx_t;

extern weaponFx_t jsd_weaponFx[MAX_WEAPONS];

extern cgs_t cgs;
extern cg_t cg;
extern centity_t cg_entities[MAX_GENTITIES];
extern weaponInfo_t cg_weapons[MAX_WEAPONS];
extern itemInfo_t cg_items[MAX_ITEMS];
extern markPoly_t cg_markPolys[MAX_MARK_POLYS];

extern vmCvar_t cg_centertime;
extern vmCvar_t cg_swingSpeed;
extern vmCvar_t cg_drawTimer;
extern vmCvar_t cg_drawFPS;
extern vmCvar_t cg_drawSnapshot;
extern vmCvar_t cg_draw3dIcons;
extern vmCvar_t cg_drawIcons;
extern vmCvar_t cg_drawAmmoWarning;
extern vmCvar_t cg_drawCrosshair;
extern vmCvar_t cg_drawTeamOverlay;
extern vmCvar_t cg_teamOverlayUserinfo;
extern vmCvar_t cg_crosshairScale;
extern vmCvar_t cg_drawStatus;
extern vmCvar_t cg_draw2D;
extern vmCvar_t cg_animSpeed;
extern vmCvar_t cg_debugAnim;
extern vmCvar_t cg_debugPosition;
extern vmCvar_t cg_errorDecay;
extern vmCvar_t cg_nopredict;
extern vmCvar_t cg_noPlayerAnims;
extern vmCvar_t cg_showmiss;
extern vmCvar_t cg_addMarks;
extern vmCvar_t cg_brassTime;
extern vmCvar_t cg_viewsize;
extern vmCvar_t cg_ignore;
extern vmCvar_t cg_fov;
extern vmCvar_t cg_zoomFov;
extern vmCvar_t cg_thirdPersonRange;
extern vmCvar_t cg_thirdPersonAngle;
extern vmCvar_t cg_thirdPerson;
extern vmCvar_t cg_firstPerson_Fwd;
extern vmCvar_t cg_firstPerson_Up;
extern vmCvar_t cg_drawAttacker;
extern vmCvar_t cg_synchronousClients;
extern vmCvar_t cg_teamChatTime;
extern vmCvar_t cg_teamChatHeight;
extern vmCvar_t cg_stats;
extern vmCvar_t cg_forceModel;
extern vmCvar_t cg_buildScript;
extern vmCvar_t cg_paused;
extern vmCvar_t cg_blood;
extern vmCvar_t cg_deferPlayers;
extern vmCvar_t cg_drawFriend;
extern vmCvar_t cg_teamChatsOnly;
extern vmCvar_t cg_scorePlum;
extern vmCvar_t cg_cameraOrbit;
extern vmCvar_t cg_cameraOrbitDelay;
extern vmCvar_t cg_timescaleFadeEnd;
extern vmCvar_t cg_timescaleFadeSpeed;
extern vmCvar_t cg_timescale;
extern vmCvar_t cg_noProjectileTrail;
extern vmCvar_t cg_oldRail;
extern vmCvar_t cg_oldRocket;
extern vmCvar_t cg_oldPlasma;
extern vmCvar_t cg_trueLightning;

//
// cg_main.c
//
const char* CG_ConfigString(int index);
const char* CG_Argv(int arg);

void QDECL CG_Printf(const char* msg, ...) Q_PRINTF_FUNC(1, 2);
void QDECL CG_Error(const char* msg, ...) Q_NO_RETURN Q_PRINTF_FUNC(1, 2);

void CG_StartMusic(void);

void CG_UpdateCvars(void);

// cg_view.c
void CG_ZoomDown_f(void);
void CG_ZoomUp_f(void);

void CG_DrawActiveFrame(int serverTime, bool demoPlayback);

// cg_drawtools.c
void CG_DrawProgressBar(float x, float y, float width, float height, float progress, float segmentWidth, const float* barColor, const float* bgColor);
void CG_Draw3DString(float x, float y, float z, const char* str, int style, vec4_t color, float fontSize, float min, float max, bool useTrace);

// cg_draw.c
void CG_Add3DString(float x, float y, float z, const char* str, int style, const vec4_t color, float fontSize, float min, float max, bool useTrace);
void CG_DrawHead(float x, float y, float w, float h, int clientNum);
void CG_AddToGenericConsole(const char* str, console_t* console);
void CG_AddNotify(const char* text, int type, int number, const char* picPath);
void CG_DrawActive(void);

//
// cg_player.c
//
void CG_Player(centity_t* cent);
void CG_ResetPlayerEntity(centity_t* cent);
void CG_NewClientInfo(int clientNum);

//
// cg_predict.c
//
void CG_BuildSolidList(void);
int CG_PointContents(const vec3_t point, int passEntityNum);
void CG_Trace(trace_t* result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask);
void CG_PredictPlayerState(void);
void CG_LoadDeferredPlayers(void);

//
// cg_events.c
//
void CG_CheckEvents(centity_t* cent);
void CG_EntityEvent(centity_t* cent, vec3_t position);
void CG_SpawnParticlePreset(int presetIdx, const vec3_t origin);

//
// cg_ents.c
//
void CG_SetEntitySoundPosition(centity_t* cent);
void CG_AddPacketEntities(void);
void CG_AdjustPositionForMover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t angles_in, vec3_t angles_out);

void CG_PositionRotatedEntityOnTag(refEntity_t* entity, const refEntity_t* parent, qhandle_t parentModel, char* tagName);

//
// cg_weapons.c
//
void CG_NextWeapon_f(void);
void CG_PrevWeapon_f(void);
void CG_Weapon_f(void);

void CG_EmitBeamPoly(qhandle_t shader, const vec3_t start, const vec3_t end, float halfWidth, const vec4_t color);

void CG_RegisterWeapon(int weaponNum);
void CG_RegisterItemVisuals(int itemNum);

void CG_FireWeapon(centity_t* cent);
void CG_MissileHitWall(int weapon, vec3_t origin, vec3_t dir);
void CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir);
void CG_ShotgunPattern(vec3_t origin, vec3_t origin2, int seed, int otherEntNum, int weapon);
void CG_Bullet(vec3_t origin, int sourceEntityNum, vec3_t normal, bool flesh, int weapon);

void CG_RailTrail(clientInfo_t* ci, vec3_t start, vec3_t end, int weapon);
void CG_MissileTrail(centity_t* ent);
void CG_AddPlayerWeapon(refEntity_t* parent, centity_t* cent, clientInfo_t* ci);

bool CG_GetLocalPlayerHeadOrigin(vec3_t origin);

//
// cg_marks.c
//
void CG_InitMarkPolys(void);
void CG_AddMarks(void);
void CG_ImpactMark(qhandle_t markShader, const vec3_t origin, const vec3_t dir, float orientation, float r, float g, float b, float a, bool alphaFade, float radius, bool temporary);

// cg_particles.c — упрощённая система частиц
void CG_ClearParticles(void);
void CG_AddParticles(const vec3_t origin, const vec4_t color, int count, float speed, int flags, const vec3_t gravity, qhandle_t shader, float size, float lifetime);
void CG_AddParticlesEx(const vec3_t origin, const vec4_t color, int count, float speed, int flags, const vec3_t gravity, qhandle_t shader, float lifetime, float size);
void CG_RenderParticles(void);

//
// cg_localents.c
//
void CG_InitLocalEntities(void);
localEntity_t* CG_AllocLocalEntity(void);
void CG_AddLocalEntities(void);

//
// cg_snapshot.c
//
void CG_ProcessSnapshots(void);

//
// cg_info.c
//
void CG_LoadingString(const char* s, float value);
void CG_LoadingItem(int itemNum);
void CG_LoadingClient(int clientNum);
void CG_DrawInformation(void);

// cg_scoreboard.c
void CG_DrawScoreboard(void);

//
// cg_consolecmds.c
//
bool CG_ConsoleCommand(void);
void CG_InitConsoleCommands(void);

//
// cg_servercmds.c
//
void CG_ExecuteNewServerCommands(int latestSequence);
void CG_ParseServerinfo(void);
void CG_SetConfigValues(void);
void CG_ShaderStateChanged(void);

//
// cg_playerstate.c
//
void CG_Respawn(void);
void CG_TransitionPlayerState(playerState_t* ps, playerState_t* ops);

//===============================================

// system traps
void trap_AddCommand(const char* cmdName);
void trap_RemoveCommand(const char* cmdName);
void trap_SendClientCommand(const char* s);
void trap_CM_LoadMap(const char* mapname);
int trap_CM_NumInlineModels(void);
clipHandle_t trap_CM_InlineModel(int index);  // 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs);
int trap_CM_MarkFragments(int numPoints, const vec3_t* points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t* fragmentBuffer);
void trap_S_StartSound(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx);
void trap_S_StopLoopingSound(int entnum);
void trap_S_ClearLoopingSounds(bool killall);
void trap_S_AddLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx);
void trap_S_AddRealLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx);
void trap_S_UpdateEntityPosition(int entityNum, const vec3_t origin);
void trap_S_Respatialize(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater);
void trap_S_StartBackgroundTrack(const char* intro, const char* loop);  // empty name stops music
void trap_R_LoadWorldMap(const char* mapname);
void trap_R_AddPolysToScene(qhandle_t hShader, int numVerts, const polyVert_t* verts, int numPolys);
int trap_R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);
void trap_GetGameState(gameState_t* gamestate);
void trap_GetCurrentSnapshotNumber(int* snapshotNumber, int* serverTime);
bool trap_GetSnapshot(int snapshotNumber, snapshot_t* snapshot);
bool trap_GetServerCommand(int serverCommandNumber);
int trap_GetCurrentCmdNumber(void);
bool trap_GetUserCmd(int cmdNumber, usercmd_t* ucmd);
void trap_SetUserCmdValue(int stateValue, float sensitivityScale);
int trap_MemoryRemaining(void);
