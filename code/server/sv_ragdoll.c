// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2
//
// sv_ragdoll.c - server side IQM skeleton loader for ragdolls.
//
// The game module can not read multi-megabyte IQM files itself, so it asks the
// engine (trap_Ragdoll_LoadSkeleton) for the skeleton of a model in a given
// animation pose: joint names, hierarchy and the model space position/rotation
// of every joint.
//
// The pose is built with exactly the same rules as the renderer
// (IQM_BuildRelativeJoints in tr_model_iqm.c) so the ragdoll starts in the pose
// the player model was drawn in:
//   * rotations come from the external animation IQM (matched BY NAME),
//   * translations and scale always come from the model's own bind pose,
//   * except for roots / pelvis, which get the animation's movement relative to
//     the animation's bind pose, scaled by the ratio of both bone lengths.

#include "server.h"
#include "../renderercommon/iqm.h"

#define SVR_MAX_CACHED_MODELS 8

typedef struct {
	float translate[3];
	float rotate[4];  // x y z w
	float scale[3];
} svrTransform_t;

typedef struct {
	char path[MAX_QPATH];
	int numJoints;
	char names[MAX_IQM_BONES][MAX_RAGDOLL_BONENAME];
	int parent[MAX_IQM_BONES];
	svrTransform_t bind[MAX_IQM_BONES];
} svrModelSkeleton_t;

static svrModelSkeleton_t svr_models[SVR_MAX_CACHED_MODELS];
static int svr_numModels = 0;
static int svr_nextModel = 0;

// ---------------------------------------------------------------------------
// small math helpers (3x4 row major matrices, quaternions as x y z w)
// ---------------------------------------------------------------------------

static void SVR_QuatNormalize(const float* in, float* out) {
	float len = sqrtf(in[0] * in[0] + in[1] * in[1] + in[2] * in[2] + in[3] * in[3]);

	if(len < 1e-8f) {
		out[0] = out[1] = out[2] = 0.0f;
		out[3] = 1.0f;
		return;
	}
	len = 1.0f / len;
	out[0] = in[0] * len;
	out[1] = in[1] * len;
	out[2] = in[2] * len;
	out[3] = in[3] * len;
}

static void SVR_Matrix34Multiply(const float* a, const float* b, float* out) {
	out[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8];
	out[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9];
	out[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10];
	out[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3];
	out[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8];
	out[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9];
	out[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10];
	out[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7];
	out[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8];
	out[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9];
	out[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10];
	out[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11];
}

// same layout as JointToMatrix() in tr_model_iqm.c
static void SVR_JointToMatrix(const float* rot, const float* scale, const float* trans, float* mat) {
	float xx = 2.0f * rot[0] * rot[0];
	float yy = 2.0f * rot[1] * rot[1];
	float zz = 2.0f * rot[2] * rot[2];
	float xy = 2.0f * rot[0] * rot[1];
	float xz = 2.0f * rot[0] * rot[2];
	float yz = 2.0f * rot[1] * rot[2];
	float wx = 2.0f * rot[3] * rot[0];
	float wy = 2.0f * rot[3] * rot[1];
	float wz = 2.0f * rot[3] * rot[2];

	mat[0] = scale[0] * (1.0f - (yy + zz));
	mat[1] = scale[0] * (xy - wz);
	mat[2] = scale[0] * (xz + wy);
	mat[3] = trans[0];
	mat[4] = scale[1] * (xy + wz);
	mat[5] = scale[1] * (1.0f - (xx + zz));
	mat[6] = scale[1] * (yz - wx);
	mat[7] = trans[1];
	mat[8] = scale[2] * (xz - wy);
	mat[9] = scale[2] * (yz + wx);
	mat[10] = scale[2] * (1.0f - (xx + yy));
	mat[11] = trans[2];
}

// rotation of a 3x4 matrix as a quaternion (rows are normalized first, so a
// per joint scale does not matter)
static void SVR_MatrixToQuat(const float* m, float* q) {
	float r[9];
	float trace, s;
	int i;

	for(i = 0; i < 3; i++) {
		const float* row = &m[i * 4];
		float len = sqrtf(row[0] * row[0] + row[1] * row[1] + row[2] * row[2]);

		if(len < 1e-8f) len = 1.0f;
		r[i * 3 + 0] = row[0] / len;
		r[i * 3 + 1] = row[1] / len;
		r[i * 3 + 2] = row[2] / len;
	}

	trace = r[0] + r[4] + r[8];
	if(trace > 0.0f) {
		s = sqrtf(trace + 1.0f) * 2.0f;
		q[3] = 0.25f * s;
		q[0] = (r[7] - r[5]) / s;
		q[1] = (r[2] - r[6]) / s;
		q[2] = (r[3] - r[1]) / s;
	} else if(r[0] > r[4] && r[0] > r[8]) {
		s = sqrtf(1.0f + r[0] - r[4] - r[8]) * 2.0f;
		q[3] = (r[7] - r[5]) / s;
		q[0] = 0.25f * s;
		q[1] = (r[1] + r[3]) / s;
		q[2] = (r[2] + r[6]) / s;
	} else if(r[4] > r[8]) {
		s = sqrtf(1.0f + r[4] - r[0] - r[8]) * 2.0f;
		q[3] = (r[2] - r[6]) / s;
		q[0] = (r[1] + r[3]) / s;
		q[1] = 0.25f * s;
		q[2] = (r[5] + r[7]) / s;
	} else {
		s = sqrtf(1.0f + r[8] - r[0] - r[4]) * 2.0f;
		q[3] = (r[3] - r[1]) / s;
		q[0] = (r[2] + r[6]) / s;
		q[1] = (r[5] + r[7]) / s;
		q[2] = 0.25f * s;
	}
	SVR_QuatNormalize(q, q);
}

static bool SVR_NameContainsNoCase(const char* name, const char* part) {
	int n = (int)strlen(name);
	int p = (int)strlen(part);
	int i;

	for(i = 0; i + p <= n; i++) {
		if(!Q_stricmpn(name + i, part, p)) {
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// IQM access
// ---------------------------------------------------------------------------

static bool SVR_RangeOk(const iqmHeader_t* header, unsigned int offset, unsigned int count, unsigned int size) {
	if(count == 0) return true;
	if(offset > header->filesize) return false;
	if(count > header->filesize) return false;
	if((unsigned long long)offset + (unsigned long long)count * size > header->filesize) return false;
	return true;
}

// reads an IQM into memory and validates the header. Caller frees with FS_FreeFile.
static iqmHeader_t* SVR_ReadIQM(const char* path, void** buffer) {
	char withExt[MAX_QPATH];
	int len;
	iqmHeader_t* header;
	void* buf = NULL;

	*buffer = NULL;

	len = FS_ReadFile(path, &buf);
	if((len <= 0 || !buf) && !strstr(path, ".iqm")) {
		Com_sprintf(withExt, sizeof(withExt), "%s.iqm", path);
		len = FS_ReadFile(withExt, &buf);
	}
	if(len <= 0 || !buf) {
		Com_Printf(S_COLOR_YELLOW "SV_Ragdoll: couldn't load '%s'\n", path);
		return NULL;
	}

	if(len < (int)sizeof(iqmHeader_t)) {
		Com_Printf(S_COLOR_YELLOW "SV_Ragdoll: '%s' is too small\n", path);
		FS_FreeFile(buf);
		return NULL;
	}

	header = (iqmHeader_t*)buf;
	if(Q_strncmp(header->magic, IQM_MAGIC, sizeof(header->magic))) {
		Com_Printf(S_COLOR_YELLOW "SV_Ragdoll: '%s' is not an IQM file\n", path);
		FS_FreeFile(buf);
		return NULL;
	}

	header->version = LittleLong(header->version);
	header->filesize = LittleLong(header->filesize);
	if(header->version != IQM_VERSION || header->filesize > (unsigned int)len) {
		Com_Printf(S_COLOR_YELLOW "SV_Ragdoll: '%s' has unsupported version or invalid size\n", path);
		FS_FreeFile(buf);
		return NULL;
	}

	header->num_text = LittleLong(header->num_text);
	header->ofs_text = LittleLong(header->ofs_text);
	header->num_joints = LittleLong(header->num_joints);
	header->ofs_joints = LittleLong(header->ofs_joints);
	header->num_poses = LittleLong(header->num_poses);
	header->ofs_poses = LittleLong(header->ofs_poses);
	header->num_frames = LittleLong(header->num_frames);
	header->num_framechannels = LittleLong(header->num_framechannels);
	header->ofs_frames = LittleLong(header->ofs_frames);

	*buffer = buf;
	return header;
}

static bool SVR_ReadJoints(const iqmHeader_t* header, const char* path, int* numJoints, char names[][MAX_RAGDOLL_BONENAME], int* parent, svrTransform_t* bind) {
	const iqmJoint_t* joint;
	int i, k;

	if(header->num_joints == 0 || header->num_joints > MAX_IQM_BONES) {
		Com_Printf(S_COLOR_YELLOW "SV_Ragdoll: '%s' has %u joints (1..%d supported)\n", path, header->num_joints, MAX_IQM_BONES);
		return false;
	}
	if(!SVR_RangeOk(header, header->ofs_joints, header->num_joints, sizeof(iqmJoint_t)) || header->ofs_text + header->num_text > header->filesize) {
		Com_Printf(S_COLOR_YELLOW "SV_Ragdoll: '%s' has invalid joint data\n", path);
		return false;
	}

	joint = (const iqmJoint_t*)((const byte*)header + header->ofs_joints);
	for(i = 0; i < (int)header->num_joints; i++, joint++) {
		unsigned int nameOfs = LittleLong(joint->name);

		if(nameOfs >= header->num_text) {
			Com_Printf(S_COLOR_YELLOW "SV_Ragdoll: '%s' has an invalid joint name index\n", path);
			return false;
		}
		Q_strncpyz(names[i], (const char*)header + header->ofs_text + nameOfs, MAX_RAGDOLL_BONENAME);
		parent[i] = LittleLong(joint->parent);

		if(bind) {
			float q[4];

			for(k = 0; k < 3; k++) {
				bind[i].translate[k] = LittleFloat(joint->translate[k]);
				bind[i].scale[k] = LittleFloat(joint->scale[k]);
			}
			for(k = 0; k < 4; k++) {
				q[k] = LittleFloat(joint->rotate[k]);
			}
			SVR_QuatNormalize(q, bind[i].rotate);
		}
	}

	*numJoints = (int)header->num_joints;
	return true;
}

static svrModelSkeleton_t* SVR_GetModelSkeleton(const char* modelPath) {
	svrModelSkeleton_t* sk;
	iqmHeader_t* header;
	void* buffer;
	int i;

	for(i = 0; i < svr_numModels; i++) {
		if(!Q_stricmp(svr_models[i].path, modelPath)) {
			return &svr_models[i];
		}
	}

	header = SVR_ReadIQM(modelPath, &buffer);
	if(!header) {
		return NULL;
	}

	if(svr_numModels < SVR_MAX_CACHED_MODELS) {
		sk = &svr_models[svr_numModels++];
	} else {
		sk = &svr_models[svr_nextModel];
		svr_nextModel = (svr_nextModel + 1) % SVR_MAX_CACHED_MODELS;
	}

	Com_Memset(sk, 0, sizeof(*sk));
	Q_strncpyz(sk->path, modelPath, sizeof(sk->path));

	if(!SVR_ReadJoints(header, modelPath, &sk->numJoints, sk->names, sk->parent, sk->bind)) {
		sk->path[0] = '\0';
		sk->numJoints = 0;
		FS_FreeFile(buffer);
		return NULL;
	}

	FS_FreeFile(buffer);
	return sk;
}

// decodes one frame of the animation IQM into per pose transforms
static bool SVR_DecodeFrame(const iqmHeader_t* header, const char* path, int frame, svrTransform_t* out) {
	const iqmPose_t* pose;
	const unsigned short* framedata;
	int j, k;

	if(header->num_poses == 0 || header->num_frames == 0 || header->num_poses > MAX_IQM_BONES) {
		Com_Printf(S_COLOR_YELLOW "SV_Ragdoll: '%s' has no usable pose data\n", path);
		return false;
	}
	if(!SVR_RangeOk(header, header->ofs_poses, header->num_poses, sizeof(iqmPose_t)) || !SVR_RangeOk(header, header->ofs_frames, header->num_frames * header->num_framechannels, sizeof(unsigned short))) {
		Com_Printf(S_COLOR_YELLOW "SV_Ragdoll: '%s' has invalid pose data\n", path);
		return false;
	}

	if(frame < 0 || frame >= (int)header->num_frames) {
		frame = 0;
	}

	pose = (const iqmPose_t*)((const byte*)header + header->ofs_poses);
	framedata = (const unsigned short*)((const byte*)header + header->ofs_frames) + (size_t)frame * header->num_framechannels;

	for(j = 0; j < (int)header->num_poses; j++, pose++) {
		unsigned int mask = LittleLong(pose->mask);
		float t[3], r[4], s[3];
		float offs[10], scl[10];

		for(k = 0; k < 10; k++) {
			offs[k] = LittleFloat(pose->channeloffset[k]);
			scl[k] = LittleFloat(pose->channelscale[k]);
		}

		for(k = 0; k < 10; k++) {
			float v = offs[k];

			if(mask & (1u << k)) {
				v += (float)LittleShort(*framedata++) * scl[k];
			}
			if(k < 3) t[k] = v;
			else if(k < 7) r[k - 3] = v;
			else s[k - 7] = v;
		}

		VectorCopy(t, out[j].translate);
		SVR_QuatNormalize(r, out[j].rotate);
		VectorCopy(s, out[j].scale);
	}

	return true;
}

/*
==================
SV_RagdollLoadSkeleton

modelPath: model without extension, e.g. "models/players/alyx/pmodel"
animPath:  external animation IQM (animFile of animation.cfg), may be empty:
           the model is then posed with its own embedded animation / bind pose
frame:     absolute frame of the animation IQM

Fills the skeleton with joint names, parents and the model space pose.
Returns 1 on success.
==================
*/
int SV_RagdollLoadSkeleton(const char* modelPath, const char* animPath, int frame, ragdollSkeleton_t* out) {
	svrModelSkeleton_t* model;
	svrTransform_t rel[MAX_IQM_BONES];
	float world[MAX_IQM_BONES][12];
	int i;

	if(!modelPath || !modelPath[0] || !out) {
		return 0;
	}

	Com_Memset(out, 0, sizeof(*out));

	model = SVR_GetModelSkeleton(modelPath);
	if(!model) {
		return 0;
	}

	// every joint starts in the model's own bind pose
	for(i = 0; i < model->numJoints; i++) {
		rel[i] = model->bind[i];
	}

	if(animPath && animPath[0]) {
		void* animBuffer;
		iqmHeader_t* aheader = SVR_ReadIQM(animPath, &animBuffer);
		svrTransform_t poses[MAX_IQM_BONES];

		if(aheader && SVR_DecodeFrame(aheader, animPath, frame, poses)) {
			char animNames[MAX_IQM_BONES][MAX_RAGDOLL_BONENAME];
			int animParent[MAX_IQM_BONES];
			svrTransform_t animBind[MAX_IQM_BONES];
			int numAnimJoints = 0;
			bool haveAnimBind;
			int p, nposes = (int)aheader->num_poses;

			haveAnimBind = aheader->num_joints == aheader->num_poses && SVR_ReadJoints(aheader, animPath, &numAnimJoints, animNames, animParent, animBind);

			if(!haveAnimBind) {
				// without joint names we can only assume identical skeletons
				if(nposes != model->numJoints) {
					Com_Printf(S_COLOR_YELLOW "SV_Ragdoll: '%s' has no joint names and %d poses for %d joints, using bind pose\n", animPath, nposes, model->numJoints);
					nposes = 0;
				}
			}

			for(p = 0; p < nposes; p++) {
				int dst = -1;
				bool motionJoint;
				svrTransform_t* out_t;
				const svrTransform_t* tgt;

				if(haveAnimBind) {
					int j;

					for(j = 0; j < model->numJoints; j++) {
						if(!Q_stricmp(model->names[j], animNames[p])) {
							dst = j;
							break;
						}
					}
				} else {
					dst = p;
				}
				if(dst < 0 || dst >= model->numJoints) {
					continue;
				}

				tgt = &model->bind[dst];
				out_t = &rel[dst];

				// rotation comes from the animation
				Com_Memcpy(out_t->rotate, poses[p].rotate, sizeof(out_t->rotate));

				// bone length always comes from the model
				VectorCopy(tgt->translate, out_t->translate);

				motionJoint = (model->parent[dst] < 0) || SVR_NameContainsNoCase(model->names[dst], "pelvis") || SVR_NameContainsNoCase(model->names[dst], "hips");
				if(motionJoint) {
					if(haveAnimBind) {
						const svrTransform_t* ab = &animBind[p];
						float lenA = VectorLength(ab->translate);
						float lenT = VectorLength(tgt->translate);
						float ratio = 1.0f;
						int k;

						if(lenA > 1e-3f && lenT > 1e-3f) {
							ratio = lenT / lenA;
							if(ratio < 0.1f) ratio = 0.1f;
							if(ratio > 10.0f) ratio = 10.0f;
						}
						for(k = 0; k < 3; k++) {
							out_t->translate[k] = tgt->translate[k] + (poses[p].translate[k] - ab->translate[k]) * ratio;
						}
					} else {
						VectorCopy(poses[p].translate, out_t->translate);
					}
				}

				// scale never changes
				VectorCopy(tgt->scale, out_t->scale);
			}
		}

		if(aheader) {
			FS_FreeFile(animBuffer);
		}
	}

	// model space matrices
	for(i = 0; i < model->numJoints; i++) {
		float local[12];

		SVR_JointToMatrix(rel[i].rotate, rel[i].scale, rel[i].translate, local);
		if(model->parent[i] >= 0 && model->parent[i] < i) {
			SVR_Matrix34Multiply(world[model->parent[i]], local, world[i]);
		} else {
			Com_Memcpy(world[i], local, sizeof(local));
		}
	}

	out->numJoints = model->numJoints;
	for(i = 0; i < model->numJoints; i++) {
		Q_strncpyz(out->names[i], model->names[i], sizeof(out->names[i]));
		out->parent[i] = model->parent[i];
		out->pos[i][0] = world[i][3];
		out->pos[i][1] = world[i][7];
		out->pos[i][2] = world[i][11];
		SVR_MatrixToQuat(world[i], out->quat[i]);
	}

	return 1;
}
