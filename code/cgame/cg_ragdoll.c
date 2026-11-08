// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2
//
// cg_ragdoll.c - drawing of server-side ragdolls (ET_RAGDOLL).
//
// The server never sends the physics bodies. It sends one ET_RAGDOLL entity whose
// origin is the main bone (pelvis) and whose ragdollID selects a ragdollState_t in
// the snapshot. That state holds the world angles of every bone, which are handed
// to the renderer with trap_R_SetBoneAngles. CS_RAGDOLLS + ragdollID holds the
// player model name of the ragdoll.

#include "cg_local.h"

typedef struct {
	char name[MAX_QPATH];  // model name the handle was registered for
	qhandle_t hModel;
} cgRagdollModel_t;

static cgRagdollModel_t cg_ragdollModels[MAX_RAGDOLLS];

// bone names of detailed ragdolls = joint order of the IQM, cached per model handle
#define MAX_RAGDOLL_NAME_CACHE 8
typedef struct {
	qhandle_t hModel;
	int count;
	char names[MAX_IQM_BONES][MAX_RAGDOLL_BONENAME];
} cgRagdollNames_t;

static cgRagdollNames_t cg_ragdollNames[MAX_RAGDOLL_NAME_CACHE];
static int cg_numRagdollNames;
static int cg_nextRagdollNames;

// ---------------------------------------------------------------------------
// quaternion helpers (angles are interpolated as rotations, plain angle lerp
// goes wrong close to pitch +-90 where the Euler representation flips)
// ---------------------------------------------------------------------------

static void CGR_AnglesToQuat(const float* angles, float* q) {
	float cy = cos(DEG2RAD(angles[YAW]) * 0.5f), sy = sin(DEG2RAD(angles[YAW]) * 0.5f);
	float cp = cos(DEG2RAD(angles[PITCH]) * 0.5f), sp = sin(DEG2RAD(angles[PITCH]) * 0.5f);
	float cr = cos(DEG2RAD(angles[ROLL]) * 0.5f), sr = sin(DEG2RAD(angles[ROLL]) * 0.5f);

	q[0] = sr * cp * cy - cr * sp * sy;
	q[1] = cr * sp * cy + sr * cp * sy;
	q[2] = cr * cp * sy - sr * sp * cy;
	q[3] = cr * cp * cy + sr * sp * sy;
}

static void CGR_QuatToAngles(const float* q, vec3_t angles) {
	float x = q[0], y = q[1], z = q[2], w = q[3];
	vec3_t fwd, left;
	float cp, yaw, pitch, roll;

	fwd[0] = 1.0f - 2.0f * (y * y + z * z);
	fwd[1] = 2.0f * (x * y + w * z);
	fwd[2] = 2.0f * (x * z - w * y);
	left[0] = 2.0f * (x * y - w * z);
	left[1] = 1.0f - 2.0f * (x * x + z * z);
	left[2] = 2.0f * (y * z + w * x);

	cp = sqrt(fwd[0] * fwd[0] + fwd[1] * fwd[1]);
	pitch = atan2(-fwd[2], cp);

	if(cp > 1e-4f) {
		float sy, cy, sp;
		vec3_t left0, up0;

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
		roll = atan2(DotProduct(left, up0), DotProduct(left, left0));
	} else {
		yaw = atan2(-left[0], left[1]);
		roll = 0;
	}

	angles[PITCH] = pitch * (180.0f / M_PI);
	angles[YAW] = yaw * (180.0f / M_PI);
	angles[ROLL] = roll * (180.0f / M_PI);
}

static void CGR_LerpAngles(const float* a, const float* b, float t, vec3_t out) {
	float qa[4], qb[4], q[4], len;
	int i;

	CGR_AnglesToQuat(a, qa);
	CGR_AnglesToQuat(b, qb);
	if(qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3] < 0.0f) {
		for(i = 0; i < 4; i++) qb[i] = -qb[i];  // shortest way
	}
	for(i = 0; i < 4; i++) q[i] = qa[i] + (qb[i] - qa[i]) * t;
	len = sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
	if(len < 1e-6f) {
		VectorCopy(a, out);
		return;
	}
	for(i = 0; i < 4; i++) q[i] /= len;
	CGR_QuatToAngles(q, out);
}

// ---------------------------------------------------------------------------
// model
// ---------------------------------------------------------------------------

// reads "animFile <path>" from models/players/<model>/animation.cfg
static bool CGR_ReadAnimFile(const char* model, char* animFile, int size) {
	static char text[262144];
	char filename[MAX_QPATH];
	fileHandle_t f;
	char* text_p;
	char* token;
	int len;

	animFile[0] = '\0';
	Com_sprintf(filename, sizeof(filename), "models/players/%s/animation.cfg", model);
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(len <= 0 || len >= (int)sizeof(text) - 1) {
		if(len > 0) trap_FS_FCloseFile(f);
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
			Q_strncpyz(animFile, token, size);
			return animFile[0] != '\0';
		}
	}
	return false;
}

static qhandle_t CGR_GetModel(int id) {
	cgRagdollModel_t* rm = &cg_ragdollModels[id];
	const char* model = CG_ConfigString(CS_RAGDOLLS + id);
	char path[MAX_QPATH];
	char animFile[MAX_QPATH];

	if(!model || !model[0]) {
		rm->name[0] = '\0';
		rm->hModel = 0;
		return 0;
	}

	if(!Q_stricmp(rm->name, model)) {
		return rm->hModel;  // already registered (or failed once, don't retry every frame)
	}

	Q_strncpyz(rm->name, model, sizeof(rm->name));
	Com_sprintf(path, sizeof(path), "models/players/%s/pmodel", model);
	if(CGR_ReadAnimFile(model, animFile, sizeof(animFile))) {
		rm->hModel = trap_R_RegisterModelEx(path, animFile);
	} else {
		rm->hModel = trap_R_RegisterModel(path);
	}
	if(!rm->hModel) {
		CG_Printf("CG_Ragdoll: can't register %s\n", path);
	}
	return rm->hModel;
}

static const cgRagdollNames_t* CGR_GetNames(qhandle_t hModel) {
	cgRagdollNames_t* n;
	int i, count;

	for(i = 0; i < cg_numRagdollNames; i++) {
		if(cg_ragdollNames[i].hModel == hModel) return &cg_ragdollNames[i];
	}

	count = trap_R_GetModelBone(hModel, -1, NULL, 0);
	if(count <= 0 || count > MAX_IQM_BONES) return NULL;

	if(cg_numRagdollNames < MAX_RAGDOLL_NAME_CACHE) {
		n = &cg_ragdollNames[cg_numRagdollNames++];
	} else {
		n = &cg_ragdollNames[cg_nextRagdollNames];
		cg_nextRagdollNames = (cg_nextRagdollNames + 1) % MAX_RAGDOLL_NAME_CACHE;
	}
	n->hModel = hModel;
	n->count = count;
	for(i = 0; i < count; i++) {
		n->names[i][0] = '\0';
		trap_R_GetModelBone(hModel, i, n->names[i], sizeof(n->names[i]));
	}
	return n;
}

static const ragdollState_t* CGR_FindState(const snapshot_t* snap, int id) {
	int i;

	if(!snap) return NULL;
	for(i = 0; i < snap->numRagdolls; i++) {
		if(snap->ragdolls[i].number == id) return &snap->ragdolls[i];
	}
	return NULL;
}

/*
==================
CG_Ragdoll

Draws one ET_RAGDOLL entity: every bone of the interpolated ragdollState goes to the
renderer, then the model is added at the origin of the master entity (= main bone).
==================
*/
void CG_Ragdoll(centity_t* cent) {
	int id = cent->currentState.ragdollID;
	const ragdollState_t *a, *b;
	const cgRagdollNames_t* names = NULL;
	qhandle_t hModel;
	refEntity_t ent;
	float t = 0.0f;
	int i, numBones;

	if(id < 0 || id >= MAX_RAGDOLLS) return;

	a = CGR_FindState(cg.snap, id);
	b = (cg.nextSnap && !cg.nextFrameTeleport) ? CGR_FindState(cg.nextSnap, id) : NULL;
	if(!a) {
		a = b;
		b = NULL;
	}
	if(!a) return;  // no state received yet

	hModel = CGR_GetModel(id);
	if(!hModel) return;

	numBones = a->numBones;
	if(numBones > MAX_IQM_BONES) numBones = MAX_IQM_BONES;

	if(a->type == RAGDOLL_DETAILED) {
		names = CGR_GetNames(hModel);
		if(!names) return;
		if(numBones > names->count) numBones = names->count;
	} else if(numBones > RAGDOLL_SIMPLE_NUMBONES) {
		numBones = RAGDOLL_SIMPLE_NUMBONES;
	}

	if(b && b->type == a->type && b->numBones == a->numBones && cg.nextSnap->serverTime > cg.snap->serverTime) {
		t = cg.frameInterpolation;
		if(t < 0.0f) t = 0.0f;
		if(t > 1.0f) t = 1.0f;
	} else {
		b = NULL;
	}

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_MODEL;
	ent.hModel = hModel;
	AxisClear(ent.axis);  // the bone angles are world space
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.renderfx = RF_LIGHTING_ORIGIN;
	VectorCopy(ent.origin, ent.lightingOrigin);
	VectorCopy(ent.origin, ent.oldorigin);

	for(i = 0; i < numBones; i++) {
		const char* name = (a->type == RAGDOLL_DETAILED) ? names->names[i] : bg_ragdollSimpleBones[i];
		vec3_t angA, angB, ang;

		angA[PITCH] = SHORT2ANGLE(a->angles[i][0]);
		angA[YAW] = SHORT2ANGLE(a->angles[i][1]);
		angA[ROLL] = SHORT2ANGLE(a->angles[i][2]);

		if(b) {
			angB[PITCH] = SHORT2ANGLE(b->angles[i][0]);
			angB[YAW] = SHORT2ANGLE(b->angles[i][1]);
			angB[ROLL] = SHORT2ANGLE(b->angles[i][2]);
			CGR_LerpAngles(angA, angB, t, ang);
		} else {
			VectorCopy(angA, ang);
		}

		trap_R_SetBoneAngles(name, ang);
	}

	trap_R_AddRefEntityToScene(&ent);
}
