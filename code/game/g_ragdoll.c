// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2
//
// g_ragdoll.c - server-side ragdoll.
//
// A ragdoll is made of
//   * one ET_PHYSICS "bone" entity per bone: a bullet capsule, SVF_NOCLIENT +
//     EF_NODRAW (never sent to clients), so physgun, damage and scripts can work
//     with every single bone. entityState has ragdollID and boneID.
//   * ragdollState[ragdollID].angles[boneID]: every frame the bone entity writes
//     the angles of its body here. This is what is delta compressed and sent.
//   * one ET_RAGDOLL master entity that IS sent to clients: its origin is the
//     main bone (pelvis) joint position and ragdollID tells cgame which
//     ragdollState to draw.
//
// Ragdolls are independent from players: G_SpawnRagdoll() can be called from
// anywhere (console command, script, player death, ...). The player only matters
// when playerid >= 0, then the pose of that player's last animation frame is used.
//
// Body frame convention: the orientation of a bone's body is the orientation of
// the bone's joint frame. The capsule is rotated inside the body so it lies
// along the bone. That is why the angles of the body can be handed to the
// renderer as the joint orientation without any correction.

#include "g_local.h"

bool CheatsOk(gentity_t* ent);  // g_cmds.c

#define RAGDOLL_DENSITY 0.01f
#define RAGDOLL_MIN_MASS 1.0f    // light bones next to a heavy torso make the solver shake
#define RAGDOLL_MIN_RADIUS 1.5f  // thinner capsules tunnel through the world mesh
#define RAGDOLL_MAX_MASS 40.0f

typedef struct {
	bool inuse;
	int type;
	int masterEnt;
	int mainBone;
	int numBones;
	int spawnTime;
	int boneEnt[MAX_IQM_BONES];         // gentity number per boneID, -1 = no such bone
	vec3_t jointOffset[MAX_IQM_BONES];  // body local vector: capsule center -> joint origin
	char model[MAX_QPATH];
} ragdoll_t;

static ragdoll_t g_ragdolls[MAX_RAGDOLLS];
ragdollState_t g_ragdollState[MAX_RAGDOLLS];  // located to the engine with trap_LocateRagdollData

// ---------------------------------------------------------------------------
// math
// ---------------------------------------------------------------------------

static void RD_QuatMul(const float* a, const float* b, float* out) {
	float r[4];

	r[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
	r[1] = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
	r[2] = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
	r[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
	out[0] = r[0];
	out[1] = r[1];
	out[2] = r[2];
	out[3] = r[3];
}

static void RD_QuatRotate(const float* q, const vec3_t v, vec3_t out) {
	vec3_t t, c;

	// t = 2 * cross(q.xyz, v); v' = v + w * t + cross(q.xyz, t)
	t[0] = 2.0f * (q[1] * v[2] - q[2] * v[1]);
	t[1] = 2.0f * (q[2] * v[0] - q[0] * v[2]);
	t[2] = 2.0f * (q[0] * v[1] - q[1] * v[0]);
	c[0] = q[1] * t[2] - q[2] * t[1];
	c[1] = q[2] * t[0] - q[0] * t[2];
	c[2] = q[0] * t[1] - q[1] * t[0];
	out[0] = v[0] + q[3] * t[0] + c[0];
	out[1] = v[1] + q[3] * t[1] + c[1];
	out[2] = v[2] + q[3] * t[2] + c[2];
}

static void RD_QuatInverseRotate(const float* q, const vec3_t v, vec3_t out) {
	float conj[4];

	conj[0] = -q[0];
	conj[1] = -q[1];
	conj[2] = -q[2];
	conj[3] = q[3];
	RD_QuatRotate(conj, v, out);
}

// quaternion -> Q3 angles (pitch, yaw, roll); same convention as AnglesToAxis
static void RD_QuatToAngles(const float* q, vec3_t angles) {
	vec3_t fwd, left, in = {1, 0, 0}, in2 = {0, 1, 0};
	float cp, yaw, pitch, roll;
	float sy, cy, sp;

	RD_QuatRotate(q, in, fwd);
	RD_QuatRotate(q, in2, left);

	cp = sqrt(fwd[0] * fwd[0] + fwd[1] * fwd[1]);
	pitch = atan2(-fwd[2], cp);

	if(cp > 1e-4f) {
		vec3_t left0, up0;
		float lenL;

		yaw = atan2(fwd[1], fwd[0]);
		sy = sin(yaw);
		cy = cos(yaw);
		sp = sin(pitch);
		cp = cos(pitch);

		left0[0] = -sy;
		left0[1] = cy;
		left0[2] = 0;
		up0[0] = cy * sp;
		up0[1] = sy * sp;
		up0[2] = cp;
		lenL = DotProduct(left, left0);
		roll = atan2(DotProduct(left, up0), lenL);
	} else {
		// gimbal lock: all rotation around the vertical axis goes into yaw
		yaw = atan2(-left[0], left[1]);
		roll = 0;
	}

	angles[PITCH] = pitch * (180.0f / M_PI);
	angles[YAW] = yaw * (180.0f / M_PI);
	angles[ROLL] = roll * (180.0f / M_PI);
}

// ---------------------------------------------------------------------------
// animation.cfg (only what is needed to find the pose of a player's last frame)
// ---------------------------------------------------------------------------

typedef struct {
	int firstFrame;
	int numFrames;
	bool reversed;
	bool defined;
} rdAnim_t;

static int RD_AnimationIndexFromName(const char* name) {
	static const char* generalNames[] = {"AM_DEATH1", "AM_DEATH2", "AM_DEATH3", "AM_DEATH4", "AM_GESTURE_AGREE", "AM_GESTURE_DISAGREE", "AM_GESTURE_BECON", "AM_GESTURE_BOW", "AM_GESTURE_SALUTE", "AM_GESTURE_WAVE", "AM_TAUNT_CHEER", "AM_TAUNT_DANCE", "AM_TAUNT_LAUGH", "AM_TAUNT_MUSCLE", "AM_TAUNT_PERSISTENCE", "AM_TAUNT_ROBOT", "AM_TAUNT_ZOMBIE", "AM_MELEEATTACK", "AM_THROW"};
	static const char* moveNames[AM_MOVE_ANIMATION_COUNT] = {"AM_IDLE", "AM_SIT", "AM_WALK_E", "AM_WALK_N", "AM_WALK_NE", "AM_WALK_NW", "AM_WALK_S", "AM_WALK_SE", "AM_WALK_SW", "AM_WALK_W", "AM_RUN_E", "AM_RUN_N", "AM_RUN_NE", "AM_RUN_NW", "AM_RUN_S", "AM_RUN_SE", "AM_RUN_SW", "AM_RUN_W", "AM_CRONCHIDLE", "AM_CRONCH_E", "AM_CRONCH_N", "AM_CRONCH_NE", "AM_CRONCH_NW", "AM_CRONCH_S", "AM_CRONCH_SE", "AM_CRONCH_SW", "AM_CRONCH_W", "AM_SWIM_E", "AM_SWIM_N", "AM_SWIM_NE", "AM_SWIM_NW", "AM_SWIM_S", "AM_SWIM_SE", "AM_SWIM_SW", "AM_SWIM_W", "AM_JUMP"};
	static const char* weaponNames[AMTYPE_COUNT] = {"ALL", "AR2", "CAMERA", "CROSSBOW", "DUAL", "FIST", "GRENADE", "KNIFE", "MAGIC", "MELEE", "MELEE2", "PASSIVE", "PHYSGUN", "PISTOL", "REVOLVER", "RPG", "SHOTGUN", "SLAM", "SMG1"};
	int i, j;
	char suffixed[MAX_QPATH];

	for(i = 0; i < (int)ARRAY_LEN(generalNames); i++) {
		if(!Q_stricmp(name, generalNames[i])) return i;
	}
	for(i = 0; i < AM_MOVE_ANIMATION_COUNT; i++) {
		if(!Q_stricmp(name, moveNames[i])) return AM_IDLE + i;
		for(j = 0; j < AMTYPE_COUNT; j++) {
			Com_sprintf(suffixed, sizeof(suffixed), "%s_%s", moveNames[i], weaponNames[j]);
			if(!Q_stricmp(name, suffixed)) return AM_IDLE + i + AM_MOVE_ANIMATION_COUNT * j;
		}
	}
	return -1;
}

// same grammar as CG_ParseAnimationFile in cg_players.c
static bool RD_ParseAnimationFile(const char* filename, char* animFile, int animFileSize, rdAnim_t* anims) {
	static char text[262144];
	char* text_p;
	char* token;
	fileHandle_t f;
	int len, index;
	rdAnim_t fallback;
	int i;

	memset(anims, 0, sizeof(rdAnim_t) * MAX_TOTALANIMATIONS);
	animFile[0] = '\0';

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(len <= 0) {
		return false;
	}
	if(len >= (int)sizeof(text) - 1) {
		G_Printf("Animation file %s is too large\n", filename);
		trap_FS_FCloseFile(f);
		return false;
	}
	trap_FS_Read(text, len, f);
	text[len] = '\0';
	trap_FS_FCloseFile(f);

	text_p = text;
	while(1) {
		token = COM_Parse(&text_p);
		if(!token[0]) break;

		if(!Q_stricmp(token, "animFile")) {
			token = COM_Parse(&text_p);
			if(!token[0]) return false;
			Q_strncpyz(animFile, token, animFileSize);
			continue;
		}
		if(!Q_stricmp(token, "headsettings")) {
			COM_Parse(&text_p);
			COM_Parse(&text_p);
			continue;
		}
		if(!Q_stricmp(token, "weaponScale")) {
			COM_Parse(&text_p);
			continue;
		}
		if(!Q_stricmp(token, "jiggle")) {
			char* textSave;
			int cmdLine, param;

			COM_Parse(&text_p);  // bone
			COM_Parse(&text_p);  // scale
			cmdLine = COM_GetCurrentParseLine();
			for(param = 0; param < 3; param++) {
				textSave = text_p;
				token = COM_Parse(&text_p);
				if(!token[0] || COM_GetCurrentParseLine() != cmdLine) {
					text_p = textSave;
					break;
				}
			}
			continue;
		}

		index = RD_AnimationIndexFromName(token);
		if(index < 0 || index >= MAX_TOTALANIMATIONS) return false;

		token = COM_Parse(&text_p);
		if(!token[0]) return false;
		anims[index].firstFrame = atoi(token);
		if(anims[index].firstFrame < 0) return false;

		token = COM_Parse(&text_p);
		if(!token[0]) return false;
		anims[index].numFrames = atoi(token);
		anims[index].reversed = (anims[index].numFrames < 0);
		if(anims[index].numFrames < 0) anims[index].numFrames = -anims[index].numFrames;
		if(anims[index].numFrames <= 0) return false;

		COM_Parse(&text_p);  // loop frames
		COM_Parse(&text_p);  // fps
		anims[index].defined = true;
	}

	if(!anims[AM_IDLE].defined) {
		return false;
	}

	// undefined animations fall back like cgame does
	fallback = anims[AM_IDLE];
	for(i = 0; i < MAX_TOTALANIMATIONS; i++) {
		if(anims[i].defined) continue;
		if(i >= AM_IDLE) {
			int allIndex = AM_IDLE + ((i - AM_IDLE) % AM_MOVE_ANIMATION_COUNT);
			anims[i] = anims[allIndex].defined ? anims[allIndex] : fallback;
		} else {
			anims[i] = fallback;
		}
	}
	return true;
}

// Animation number of a player, resolved like CG_PlayerAnimationNumber() but
// always in the weapon-independent block: the weapon animType lives in cgame data
// only. (animation.cfg usually maps every weapon block to the same clips anyway.)
static int RD_PlayerAnimationNumber(const gentity_t* pl) {
	int action = pl->client->ps.torsoAnim & ~ANIM_TOGGLEBIT;
	int move = pl->client->ps.legsAnim & ~ANIM_TOGGLEBIT;

	if(action >= 0 && action < AM_IDLE) return action;  // death, gestures, taunts, ...
	if(move >= 0 && move < AM_IDLE) return move;
	if(move < AM_IDLE || move > AM_JUMP) move = AM_IDLE;
	return move;
}

// ---------------------------------------------------------------------------
// bone classification
// ---------------------------------------------------------------------------

typedef enum { RDC_OTHER, RDC_HELPER, RDC_PELVIS, RDC_SPINE, RDC_NECK, RDC_HEAD, RDC_CLAVICLE, RDC_UPPERARM, RDC_FOREARM, RDC_HAND, RDC_FINGER, RDC_THIGH, RDC_CALF, RDC_FOOT, RDC_TOE } rdClass_t;

typedef struct {
	float radius;      // capsule radius
	float leafLength;  // length when the bone has no child bone
	float swing;       // degrees, symmetric (swing1 == swing2) so it does not depend on the joint frame
	float twist;       // degrees
} rdClassInfo_t;

// First pass values. The cone-twist joint can not tell the direction a joint is allowed to
// bend in, so knees / elbows get a moderate symmetric swing and a tight twist.
static const rdClassInfo_t rdClassInfo[] = {/* RDC_OTHER    */ {0.8f, 2.0f, 15.0f, 10.0f},
                                            /* RDC_HELPER   */ {0.5f, 1.0f, 3.0f, 3.0f},
                                            /* RDC_PELVIS   */ {6.5f, 5.0f, 15.0f, 10.0f},
                                            /* RDC_SPINE    */ {6.0f, 5.0f, 18.0f, 12.0f},
                                            /* RDC_NECK     */ {2.5f, 4.0f, 28.0f, 25.0f},
                                            /* RDC_HEAD     */ {4.5f, 8.0f, 40.0f, 35.0f},
                                            /* RDC_CLAVICLE */ {2.0f, 4.0f, 12.0f, 5.0f},
                                            /* RDC_UPPERARM */ {2.2f, 10.0f, 70.0f, 40.0f},
                                            /* RDC_FOREARM  */ {1.8f, 10.0f, 40.0f, 10.0f},
                                            /* RDC_HAND     */ {1.6f, 4.0f, 40.0f, 25.0f},
                                            /* RDC_FINGER   */ {0.7f, 1.5f, 40.0f, 8.0f},
                                            /* RDC_THIGH    */ {3.8f, 16.0f, 55.0f, 25.0f},
                                            /* RDC_CALF     */ {2.8f, 16.0f, 35.0f, 8.0f},
                                            /* RDC_FOOT     */ {2.0f, 4.0f, 30.0f, 12.0f},
                                            /* RDC_TOE      */ {1.2f, 2.5f, 25.0f, 5.0f}};

static bool RD_Has(const char* lowerName, const char* part) { return strstr(lowerName, part) != NULL; }

static rdClass_t RD_ClassifyBone(const char* name) {
	char lower[MAX_RAGDOLL_BONENAME];
	int i;

	Q_strncpyz(lower, name, sizeof(lower));
	for(i = 0; lower[i]; i++) {
		if(lower[i] >= 'A' && lower[i] <= 'Z') lower[i] += 'a' - 'A';
	}

	// procedural / helper bones of the valveBiped rig
	if(RD_Has(lower, "hlp") || RD_Has(lower, "attachment") || RD_Has(lower, "forward") || RD_Has(lower, "ik_") || RD_Has(lower, "ulna") || RD_Has(lower, "wrist") || RD_Has(lower, "trapezius") || RD_Has(lower, "shoulder") || RD_Has(lower, "bicep") || RD_Has(lower, "elbow") || RD_Has(lower, "knee") || RD_Has(lower, "ankle")) {
		return RDC_HELPER;
	}
	if(RD_Has(lower, "pelvis")) return RDC_PELVIS;
	if(RD_Has(lower, "spine")) return RDC_SPINE;
	if(RD_Has(lower, "neck")) return RDC_NECK;
	if(RD_Has(lower, "head")) return RDC_HEAD;
	if(RD_Has(lower, "clavicle")) return RDC_CLAVICLE;
	if(RD_Has(lower, "upperarm")) return RDC_UPPERARM;
	if(RD_Has(lower, "forearm")) return RDC_FOREARM;
	if(RD_Has(lower, "finger")) return RDC_FINGER;
	if(RD_Has(lower, "hand")) return RDC_HAND;
	if(RD_Has(lower, "thigh")) return RDC_THIGH;
	if(RD_Has(lower, "calf")) return RDC_CALF;
	if(RD_Has(lower, "foot")) return RDC_FOOT;
	if(RD_Has(lower, "toe")) return RDC_TOE;
	return RDC_OTHER;
}

// Known chains of the valveBiped rig: which bone the capsule of a bone points to.
// A NULL next means "leaf", the capsule gets the default length of its class.
typedef struct {
	const char* name;
	const char* next;
} rdChain_t;

static const rdChain_t rdChains[] = {{"ValveBiped.Bip01_Pelvis", "ValveBiped.Bip01_Spine"}, {"ValveBiped.Bip01_Spine", "ValveBiped.Bip01_Spine1"}, {"ValveBiped.Bip01_Spine1", "ValveBiped.Bip01_Spine2"}, {"ValveBiped.Bip01_Spine2", "ValveBiped.Bip01_Spine4"}, {"ValveBiped.Bip01_Spine4", "ValveBiped.Bip01_Neck1"}, {"ValveBiped.Bip01_Neck1", "ValveBiped.Bip01_Head1"}, {"ValveBiped.Bip01_Head1", NULL}, {"ValveBiped.Bip01_L_Clavicle", "ValveBiped.Bip01_L_UpperArm"}, {"ValveBiped.Bip01_L_UpperArm", "ValveBiped.Bip01_L_Forearm"}, {"ValveBiped.Bip01_L_Forearm", "ValveBiped.Bip01_L_Hand"}, {"ValveBiped.Bip01_L_Hand", "ValveBiped.Bip01_L_Finger2"}, {"ValveBiped.Bip01_R_Clavicle", "ValveBiped.Bip01_R_UpperArm"}, {"ValveBiped.Bip01_R_UpperArm", "ValveBiped.Bip01_R_Forearm"}, {"ValveBiped.Bip01_R_Forearm", "ValveBiped.Bip01_R_Hand"}, {"ValveBiped.Bip01_R_Hand", "ValveBiped.Bip01_R_Finger2"}, {"ValveBiped.Bip01_L_Thigh", "ValveBiped.Bip01_L_Calf"}, {"ValveBiped.Bip01_L_Calf", "ValveBiped.Bip01_L_Foot"}, {"ValveBiped.Bip01_L_Foot", "ValveBiped.Bip01_L_Toe0"}, {"ValveBiped.Bip01_L_Toe0", NULL}, {"ValveBiped.Bip01_R_Thigh", "ValveBiped.Bip01_R_Calf"}, {"ValveBiped.Bip01_R_Calf", "ValveBiped.Bip01_R_Foot"}, {"ValveBiped.Bip01_R_Foot", "ValveBiped.Bip01_R_Toe0"}, {"ValveBiped.Bip01_R_Toe0", NULL}};

// ---------------------------------------------------------------------------
// building the ragdoll
// ---------------------------------------------------------------------------

typedef struct {
	bool used;
	int joint;   // joint index in the skeleton
	int parent;  // boneID of the parent bone, -1 = main bone
	int child;   // boneID the capsule points to, -1 = leaf
	int entnum;
	rdClass_t cls;
	vec3_t jointPos;  // world
	float quat[4];    // world
	vec3_t axisWorld;
	vec3_t axisLocal;
	float length;
	float radius;
	float cylLen;
	float mass;
	vec3_t center;
} rdBone_t;

static rdBone_t rdBones[MAX_IQM_BONES];
static ragdollSkeleton_t rdSkel;
static rdAnim_t rdAnims[MAX_TOTALANIMATIONS];

/*
===============
G_RagdollModelOfPlayer

Model name of a client, in the form the ragdoll spawn functions expect
(the model part of the "model" userinfo key, without skin).
===============
*/
bool G_RagdollModelOfPlayer(int clientNum, char* out, int outSize) {
	char cs[MAX_INFO_STRING];
	char* slash;

	if(clientNum < 0 || clientNum >= MAX_CLIENTS) return false;

	trap_GetConfigstring(CS_PLAYERS + clientNum, cs, sizeof(cs));
	Q_strncpyz(out, Info_ValueForKey(cs, "model"), outSize);
	slash = strchr(out, '/');
	if(slash) *slash = '\0';
	return out[0] != '\0';
}

static int RD_FindFreeSlot(void) {
	int i;

	for(i = 0; i < MAX_RAGDOLLS; i++) {
		if(!g_ragdolls[i].inuse) return i;
	}
	return -1;
}

/*
===============
G_RemoveRagdoll

Frees the master entity, all bone entities (and with them their bullet bodies and
joints) and the configstring of a ragdoll. except is an entity that is just being
freed by the caller and must not be freed again.
===============
*/
void G_RemoveRagdollEx(int id, const gentity_t* except) {
	ragdoll_t* rd;
	int b;

	if(id < 0 || id >= MAX_RAGDOLLS) return;
	rd = &g_ragdolls[id];
	if(!rd->inuse) return;

	rd->inuse = false;  // first: the free hooks below must not come back here

	for(b = 0; b < MAX_IQM_BONES; b++) {
		int n = rd->boneEnt[b];

		if(n >= 0 && &g_entities[n] != except && g_entities[n].inuse && g_entities[n].ragdollBone) {
			G_FreeEntity(&g_entities[n]);
		}
	}
	if(rd->masterEnt >= 0 && &g_entities[rd->masterEnt] != except && g_entities[rd->masterEnt].inuse && g_entities[rd->masterEnt].s.eType == ET_RAGDOLL) {
		G_FreeEntity(&g_entities[rd->masterEnt]);
	}

	trap_SetConfigstring(CS_RAGDOLLS + id, "");
	memset(rd, 0, sizeof(*rd));
	memset(&g_ragdollState[id], 0, sizeof(g_ragdollState[id]));
}

void G_RemoveRagdoll(int id) { G_RemoveRagdollEx(id, NULL); }

void G_RemoveAllRagdolls(void) {
	int i;

	for(i = 0; i < MAX_RAGDOLLS; i++) G_RemoveRagdoll(i);
}

// called by G_FreeEntity for every entity that is freed
void G_RagdollEntityFreed(gentity_t* ent) {
	if(ent->s.eType == ET_RAGDOLL) {
		// the master is gone, so is the ragdoll
		G_RemoveRagdollEx(ent->s.ragdollID, ent);
	} else if(ent->ragdollBone) {
		int id = ent->s.ragdollID;
		int b = ent->s.boneID;

		if(id >= 0 && id < MAX_RAGDOLLS && g_ragdolls[id].inuse && b >= 0 && b < MAX_IQM_BONES && g_ragdolls[id].boneEnt[b] == ent->s.number) {
			g_ragdolls[id].boneEnt[b] = -1;  // the rest of the ragdoll keeps working
		}
	}
}

/*
===============
G_SpawnRagdoll

origin:   model origin (where the player model's refEntity origin would be, i.e. the
          feet; for a player that is ps.origin + MINS_Z)
yaw:      direction the model faces
model:    player model name, same as the "model" userinfo key (models/players/<model>/)
playerid: -1 = pose is frame 0 of the animation file,
          >= 0 = pose is the last frame of the animation that client is playing
type:     RAGDOLL_SIMPLE or RAGDOLL_DETAILED

Returns the ragdollID, or -1 on failure.
===============
*/
int G_SpawnRagdoll(const vec3_t origin, float yaw, const char* model, int playerid, int type) {
	char path[MAX_QPATH];
	char cfgPath[MAX_QPATH];
	char animFile[MAX_QPATH];
	int id, frame = 0;
	int numBones, mainBone = -1;
	int jointToBone[MAX_IQM_BONES];
	int b, j, k;
	float yawQuat[4];
	ragdoll_t* rd;
	gentity_t* master;
	int groupEnts[MAX_IQM_BONES];
	int numGroup = 0;

	if(!model || !model[0]) {
		G_Printf("G_SpawnRagdoll: no model\n");
		return -1;
	}
	if(type != RAGDOLL_SIMPLE && type != RAGDOLL_DETAILED) type = RAGDOLL_SIMPLE;

	id = RD_FindFreeSlot();
	if(id < 0) {
		G_Printf("G_SpawnRagdoll: limit of %d ragdolls reached\n", MAX_RAGDOLLS);
		return -1;
	}

	// ---- which pose: animation file + frame ----
	Com_sprintf(cfgPath, sizeof(cfgPath), "models/players/%s/animation.cfg", model);
	if(!RD_ParseAnimationFile(cfgPath, animFile, sizeof(animFile), rdAnims)) {
		G_Printf("G_SpawnRagdoll: can't parse %s\n", cfgPath);
		return -1;
	}
	if(playerid >= 0 && playerid < MAX_CLIENTS && g_entities[playerid].inuse && g_entities[playerid].client) {
		const rdAnim_t* a = &rdAnims[RD_PlayerAnimationNumber(&g_entities[playerid])];

		// last frame of the clip (the first one when it plays reversed)
		frame = a->reversed ? a->firstFrame : a->firstFrame + a->numFrames - 1;
	}

	Com_sprintf(path, sizeof(path), "models/players/%s/pmodel", model);
	if(!trap_Ragdoll_LoadSkeleton(path, animFile, frame, &rdSkel)) {
		G_Printf("G_SpawnRagdoll: can't load skeleton of %s\n", path);
		return -1;
	}

	// ---- choose the bones ----
	memset(rdBones, 0, sizeof(rdBones));
	for(j = 0; j < MAX_IQM_BONES; j++) jointToBone[j] = -1;

	if(type == RAGDOLL_SIMPLE) {
		numBones = RAGDOLL_SIMPLE_NUMBONES;
		for(b = 0; b < numBones; b++) {
			for(j = 0; j < rdSkel.numJoints; j++) {
				if(!Q_stricmp(rdSkel.names[j], bg_ragdollSimpleBones[b])) {
					rdBones[b].used = true;
					rdBones[b].joint = j;
					jointToBone[j] = b;
					break;
				}
			}
		}
	} else {
		numBones = rdSkel.numJoints;
		for(b = 0; b < numBones; b++) {
			rdBones[b].used = true;
			rdBones[b].joint = b;
			jointToBone[b] = b;
		}
	}

	// main bone: the pelvis, or else the first bone that is a root
	for(b = 0; b < numBones; b++) {
		if(rdBones[b].used && RD_ClassifyBone(rdSkel.names[rdBones[b].joint]) == RDC_PELVIS) {
			mainBone = b;
			break;
		}
	}
	if(mainBone < 0) {
		for(b = 0; b < numBones; b++) {
			if(rdBones[b].used && rdSkel.parent[rdBones[b].joint] < 0) {
				mainBone = b;
				break;
			}
		}
	}
	if(mainBone < 0) {
		G_Printf("G_SpawnRagdoll: %s has no usable bones\n", model);
		return -1;
	}

	// ---- hierarchy ----
	for(b = 0; b < numBones; b++) {
		int p;

		if(!rdBones[b].used) continue;

		rdBones[b].parent = -1;
		rdBones[b].child = -1;
		rdBones[b].cls = RD_ClassifyBone(rdSkel.names[rdBones[b].joint]);

		p = rdSkel.parent[rdBones[b].joint];
		while(p >= 0 && jointToBone[p] < 0) p = rdSkel.parent[p];
		if(p >= 0) rdBones[b].parent = jointToBone[p];

		// every bone except the main bone hangs on something: orphans on the main bone
		if(b != mainBone && rdBones[b].parent < 0) rdBones[b].parent = mainBone;
	}
	rdBones[mainBone].parent = -1;

	// ---- world pose of every bone ----
	{
		float s = sin(DEG2RAD(yaw) * 0.5f);
		float c = cos(DEG2RAD(yaw) * 0.5f);

		yawQuat[0] = 0;
		yawQuat[1] = 0;
		yawQuat[2] = s;
		yawQuat[3] = c;
	}
	for(b = 0; b < numBones; b++) {
		vec3_t p;

		if(!rdBones[b].used) continue;
		j = rdBones[b].joint;

		VectorCopy(rdSkel.pos[j], p);
		RD_QuatRotate(yawQuat, p, rdBones[b].jointPos);
		VectorAdd(rdBones[b].jointPos, origin, rdBones[b].jointPos);
		RD_QuatMul(yawQuat, rdSkel.quat[j], rdBones[b].quat);
	}

	// ---- capsule direction / size ----
	for(b = 0; b < numBones; b++) {
		rdBone_t* bone = &rdBones[b];
		const rdClassInfo_t* ci;
		int next = -1;
		vec3_t dir;
		float len, volume;

		if(!bone->used) continue;
		ci = &rdClassInfo[bone->cls];

		// preferred child: the known chain, else the first child bone
		for(k = 0; k < (int)ARRAY_LEN(rdChains); k++) {
			if(!Q_stricmp(rdChains[k].name, rdSkel.names[bone->joint])) {
				if(rdChains[k].next) {
					for(j = 0; j < numBones; j++) {
						if(rdBones[j].used && !Q_stricmp(rdSkel.names[rdBones[j].joint], rdChains[k].next)) {
							next = j;
							break;
						}
					}
				}
				break;
			}
		}
		if(k == (int)ARRAY_LEN(rdChains)) {
			for(j = 0; j < numBones; j++) {
				if(j != b && rdBones[j].used && rdBones[j].parent == b && j != mainBone && rdSkel.parent[rdBones[j].joint] >= 0) {
					next = j;
					break;
				}
			}
		}

		VectorClear(dir);
		len = 0;
		if(next >= 0) {
			VectorSubtract(rdBones[next].jointPos, bone->jointPos, dir);
			len = VectorNormalize(dir);
		}
		if(next < 0 || len < 0.5f) {
			// leaf: keep pointing the way the parent bone points
			next = -1;
			len = ci->leafLength;
			if(bone->parent >= 0 && rdBones[bone->parent].length > 0.0f) {
				VectorCopy(rdBones[bone->parent].axisWorld, dir);
			} else {
				VectorSet(dir, 0, 0, 1);
			}
		}

		bone->child = next;
		bone->length = len;
		VectorCopy(dir, bone->axisWorld);
		bone->radius = ci->radius;
		if(bone->radius > len * 1.3f) bone->radius = len * 1.3f;
		if(bone->radius < RAGDOLL_MIN_RADIUS) bone->radius = RAGDOLL_MIN_RADIUS;
		bone->cylLen = len - 2.0f * bone->radius;
		if(bone->cylLen < 0.2f) bone->cylLen = 0.2f;

		volume = M_PI * bone->radius * bone->radius * bone->cylLen + (4.0f / 3.0f) * M_PI * bone->radius * bone->radius * bone->radius;
		bone->mass = volume * RAGDOLL_DENSITY;
		if(bone->mass < RAGDOLL_MIN_MASS) bone->mass = RAGDOLL_MIN_MASS;
		if(bone->mass > RAGDOLL_MAX_MASS) bone->mass = RAGDOLL_MAX_MASS;

		RD_QuatInverseRotate(bone->quat, bone->axisWorld, bone->axisLocal);
		VectorMA(bone->jointPos, len * 0.5f, bone->axisWorld, bone->center);
	}

	// ---- allocate ----
	rd = &g_ragdolls[id];
	memset(rd, 0, sizeof(*rd));
	rd->inuse = true;
	rd->type = type;
	rd->numBones = numBones;
	rd->mainBone = mainBone;
	rd->masterEnt = -1;
	rd->spawnTime = level.time;
	Q_strncpyz(rd->model, model, sizeof(rd->model));
	for(b = 0; b < MAX_IQM_BONES; b++) rd->boneEnt[b] = -1;

	memset(&g_ragdollState[id], 0, sizeof(g_ragdollState[id]));
	g_ragdollState[id].number = id;
	g_ragdollState[id].type = type;
	g_ragdollState[id].numBones = numBones;

	// ---- bone entities ----
	for(b = 0; b < numBones; b++) {
		rdBone_t* bone = &rdBones[b];
		gentity_t* e;
		physCapsuleDef_t def;
		vec3_t angles;
		float half;

		if(!bone->used) continue;

		e = G_Spawn();
		e->classname = "ragdoll_bone";
		e->s.eType = ET_PHYSICS;
		e->s.eFlags |= EF_NODRAW;
		e->r.svFlags |= SVF_NOCLIENT;  // bones are never sent, ragdollState carries them
		e->s.ragdollID = id;
		e->s.boneID = b;
		e->ragdollBone = true;
		e->sb_phys = PHYS_DYNAMIC;
		e->health = -1;  // knockback works, damage does not
		e->r.contents = CONTENTS_CORPSE;
		e->takedamage = true;
		half = bone->length * 0.5f + bone->radius;
		VectorSet(e->r.mins, -half, -half, -half);
		VectorSet(e->r.maxs, half, half, half);
		VectorCopy(bone->center, e->s.origin);
		VectorCopy(bone->center, e->s.pos.trBase);
		VectorCopy(bone->center, e->r.currentOrigin);
		bone->entnum = e->s.number;
		rd->boneEnt[b] = e->s.number;

		memset(&def, 0, sizeof(def));
		def.radius = bone->radius;
		def.cylinderLength = bone->cylLen;
		VectorCopy(bone->axisLocal, def.axis);
		VectorCopy(bone->center, def.center);
		def.quat[0] = bone->quat[0];
		def.quat[1] = bone->quat[1];
		def.quat[2] = bone->quat[2];
		def.quat[3] = bone->quat[3];
		def.mass = bone->mass;
		def.linearDamping = 0.05f;
		def.angularDamping = bone->radius < 2.5f ? 0.8f : 0.5f;
		trap_Phys_CreateCapsule(e->s.number, &def);

		// center -> joint, in the body frame
		VectorScale(bone->axisLocal, -bone->length * 0.5f, rd->jointOffset[b]);

		RD_QuatToAngles(bone->quat, angles);
		VectorCopy(angles, e->s.apos.trBase);
		VectorCopy(angles, e->r.currentAngles);
		for(k = 0; k < 3; k++) g_ragdollState[id].angles[b][k] = (short)ANGLE2SHORT(angles[k]);

		trap_LinkEntity(e);
		groupEnts[numGroup++] = e->s.number;
	}

	// ---- joints ----
	for(b = 0; b < numBones; b++) {
		rdBone_t* bone = &rdBones[b];
		physConeTwistDef_t jd;
		const rdClassInfo_t* ci;

		if(!bone->used || bone->parent < 0) continue;
		if(!rdBones[bone->parent].used || rdBones[bone->parent].entnum <= 0) continue;

		ci = &rdClassInfo[bone->cls];
		memset(&jd, 0, sizeof(jd));
		jd.entityA = rdBones[bone->parent].entnum;
		jd.entityB = bone->entnum;
		VectorCopy(bone->jointPos, jd.pivot);
		VectorCopy(bone->axisWorld, jd.axis);
		jd.swing1 = jd.swing2 = ci->swing;
		jd.twist = ci->twist;
		// bones that only hang on the main bone because they have no parent of their own are held tight
		if(rdSkel.parent[bone->joint] < 0 && b != mainBone) {
			jd.swing1 = jd.swing2 = 2.0f;
			jd.twist = 2.0f;
		}
		trap_Phys_CreateConeTwist(&jd);
	}
	trap_Phys_IgnoreCollisionGroup(groupEnts, numGroup);

	// ---- master entity (the only thing clients get) ----
	master = G_Spawn();
	master->classname = "ragdoll";
	master->s.eType = ET_RAGDOLL;
	master->s.ragdollID = id;
	master->r.contents = 0;
	VectorSet(master->r.mins, -96, -96, -96);
	VectorSet(master->r.maxs, 96, 96, 96);
	master->s.pos.trType = TR_INTERPOLATE;
	VectorCopy(rdBones[mainBone].jointPos, master->s.pos.trBase);
	VectorCopy(rdBones[mainBone].jointPos, master->s.origin);
	VectorCopy(rdBones[mainBone].jointPos, master->r.currentOrigin);
	rd->masterEnt = master->s.number;
	trap_LinkEntity(master);

	// CS_RAGDOLLS only holds the model for now
	trap_SetConfigstring(CS_RAGDOLLS + id, model);

	return id;
}

/*
===============
G_RagdollBoneUpdate

Called from G_RunPhysics for every awake ragdoll bone entity: stores the angles of
the bone's body in ragdollState[ragdollID].angles[boneID].
===============
*/
void G_RagdollBoneUpdate(gentity_t* ent, const vec3_t angles) {
	int id = ent->s.ragdollID;
	int b = ent->s.boneID;
	ragdollState_t* rs;

	if(id < 0 || id >= MAX_RAGDOLLS || !g_ragdolls[id].inuse || b < 0 || b >= MAX_IQM_BONES) return;

	rs = &g_ragdollState[id];
	rs->angles[b][0] = (short)ANGLE2SHORT(angles[0]);
	rs->angles[b][1] = (short)ANGLE2SHORT(angles[1]);
	rs->angles[b][2] = (short)ANGLE2SHORT(angles[2]);
}

/*
===============
G_RunRagdolls

Once per frame: moves the ET_RAGDOLL master entity to the joint position of the
main bone. (The bone entities themselves are run by G_RunPhysics.)
===============
*/
void G_RunRagdolls(void) {
	int id;

	for(id = 0; id < MAX_RAGDOLLS; id++) {
		ragdoll_t* rd = &g_ragdolls[id];
		gentity_t* master;
		gentity_t* bone;
		vec3_t pos, angles, axis[3], joint;
		const float* ofs;
		float* o;

		if(!rd->inuse) continue;

		master = (rd->masterEnt >= 0) ? &g_entities[rd->masterEnt] : NULL;
		bone = (rd->boneEnt[rd->mainBone] >= 0) ? &g_entities[rd->boneEnt[rd->mainBone]] : NULL;
		if(!master || !master->inuse || !bone || !bone->inuse) continue;
		if(trap_Phys_IsSleeping(bone->s.number)) continue;

		trap_Phys_GetPosition(bone->s.number, &pos[0], &pos[1], &pos[2]);
		trap_Phys_GetAngles(bone->s.number, &angles[0], &angles[1], &angles[2]);
		AnglesToAxis(angles, axis);

		// joint = center + R * (center -> joint)
		ofs = rd->jointOffset[rd->mainBone];
		o = joint;
		o[0] = pos[0] + ofs[0] * axis[0][0] + ofs[1] * axis[1][0] + ofs[2] * axis[2][0];
		o[1] = pos[1] + ofs[0] * axis[0][1] + ofs[1] * axis[1][1] + ofs[2] * axis[2][1];
		o[2] = pos[2] + ofs[0] * axis[0][2] + ofs[1] * axis[1][2] + ofs[2] * axis[2][2];

		master->s.pos.trType = TR_INTERPOLATE;
		VectorCopy(joint, master->s.pos.trBase);
		VectorCopy(joint, master->s.origin);
		if(DistanceSquared(joint, master->r.currentOrigin) > 1.0f) {
			VectorCopy(joint, master->r.currentOrigin);
			trap_LinkEntity(master);
		}
	}
}

void G_InitRagdolls(void) {
	memset(g_ragdolls, 0, sizeof(g_ragdolls));
	memset(g_ragdollState, 0, sizeof(g_ragdollState));
	trap_LocateRagdollData(g_ragdollState, MAX_RAGDOLLS);
}

/*
===============
Cmd_Ragdoll_f

ragdoll [model] [simple|detailed] [self]
Spawns a ragdoll where the crosshair points. Without a model the model of the caller
is used. "self": pose of the caller's current animation instead of frame 0.
===============
*/
void Cmd_Ragdoll_f(gentity_t* ent) {
	char arg[MAX_QPATH];
	char model[MAX_QPATH];
	int type = RAGDOLL_SIMPLE;
	int playerid = -1;
	int i, argc = trap_Argc();
	vec3_t forward, right, up, start, end, origin;
	trace_t tr;
	int id;

	if(!CheatsOk(ent)) return;

	model[0] = '\0';
	for(i = 1; i < argc; i++) {
		trap_Argv(i, arg, sizeof(arg));
		if(!Q_stricmp(arg, "simple"))
			type = RAGDOLL_SIMPLE;
		else if(!Q_stricmp(arg, "detailed"))
			type = RAGDOLL_DETAILED;
		else if(!Q_stricmp(arg, "self"))
			playerid = ent->s.number;
		else
			Q_strncpyz(model, arg, sizeof(model));
	}
	if(!model[0] && !G_RagdollModelOfPlayer(ent->s.number, model, sizeof(model))) {
		trap_SendServerCommand(ent - g_entities, "print \"ragdoll: no model\n\"");
		return;
	}

	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint(ent, forward, right, up, start);
	VectorMA(start, 256, forward, end);
	trap_Trace(&tr, start, NULL, NULL, end, ent->s.number, MASK_SOLID);
	VectorCopy(tr.endpos, origin);
	origin[2] += 2.0f;  // feet capsules must not start inside the floor

	id = G_SpawnRagdoll(origin, ent->client->ps.viewangles[YAW], model, playerid, type);
	trap_SendServerCommand(ent - g_entities, va("print \"ragdoll %s (%s): %s %d\n\"", model, type == RAGDOLL_DETAILED ? "detailed" : "simple", id >= 0 ? "id" : "failed", id));
}

void Cmd_RagdollClear_f(gentity_t* ent) {
	if(!CheatsOk(ent)) return;
	G_RemoveAllRagdolls();
}
