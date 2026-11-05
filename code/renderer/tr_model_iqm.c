/*
===========================================================================
Copyright (C) 2011 Thilo Schulz <thilo@tjps.eu>
Copyright (C) 2011 Matthias Bentrup <matthias.bentrup@googlemail.com>
Copyright (C) 2011-2019 Zack Middleton <zturtleman@gmail.com>

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

#include "tr_local.h"

#define LL(x) x = LittleLong(x)

// 3x4 identity matrix
static float identityMatrix[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

static bool IQM_CheckRange(iqmHeader_t* header, int offset, int count, int size) {
	// return true if the range specified by offset, count and size
	// doesn't fit into the file
	return (count <= 0 || offset <= 0 || offset > header->filesize || offset + count * size < 0 || offset + count * size > header->filesize);
}
// "multiply" 3x4 matrices, these are assumed to be the top 3 rows
// of a 4x4 matrix with the last row = (0 0 0 1)
static void Matrix34Multiply(const float* a, const float* b, float* out) {
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
static void JointToMatrix(const quat_t rot, const vec3_t scale, const vec3_t trans, float* mat) {
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
static void Matrix34Invert(const float* inMat, float* outMat) {
	vec3_t trans;
	float invSqrLen, *v;

	outMat[0] = inMat[0];
	outMat[1] = inMat[4];
	outMat[2] = inMat[8];
	outMat[4] = inMat[1];
	outMat[5] = inMat[5];
	outMat[6] = inMat[9];
	outMat[8] = inMat[2];
	outMat[9] = inMat[6];
	outMat[10] = inMat[10];

	v = outMat + 0;
	invSqrLen = 1.0f / DotProduct(v, v);
	VectorScale(v, invSqrLen, v);
	v = outMat + 4;
	invSqrLen = 1.0f / DotProduct(v, v);
	VectorScale(v, invSqrLen, v);
	v = outMat + 8;
	invSqrLen = 1.0f / DotProduct(v, v);
	VectorScale(v, invSqrLen, v);

	trans[0] = inMat[3];
	trans[1] = inMat[7];
	trans[2] = inMat[11];

	outMat[3] = -DotProduct(outMat + 0, trans);
	outMat[7] = -DotProduct(outMat + 4, trans);
	outMat[11] = -DotProduct(outMat + 8, trans);
}

static vec_t QuatNormalize2(const quat_t v, quat_t out) {
	float length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3];

	if(length) {
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		ilength = 1 / (float)sqrt(length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length *= ilength;
		out[0] = v[0] * ilength;
		out[1] = v[1] * ilength;
		out[2] = v[2] * ilength;
		out[3] = v[3] * ilength;
	} else {
		out[0] = out[1] = out[2] = 0;
		out[3] = -1;
	}

	return length;
}

static void Matrix34Decompose(const float* mat, vec3_t trans, quat_t rot, vec3_t scale) {
	trans[0] = mat[3];
	trans[1] = mat[7];
	trans[2] = mat[11];

	// row lengths of 3x3 = scales (IQM stores scale by rows)
	{
		float sx = sqrtf(mat[0] * mat[0] + mat[1] * mat[1] + mat[2] * mat[2]);
		float sy = sqrtf(mat[4] * mat[4] + mat[5] * mat[5] + mat[6] * mat[6]);
		float sz = sqrtf(mat[8] * mat[8] + mat[9] * mat[9] + mat[10] * mat[10]);

		float r00 = sx > 1e-6f ? mat[0] / sx : 1.0f;
		float r01 = sx > 1e-6f ? mat[1] / sx : 0.0f;
		float r02 = sx > 1e-6f ? mat[2] / sx : 0.0f;
		float r10 = sy > 1e-6f ? mat[4] / sy : 0.0f;
		float r11 = sy > 1e-6f ? mat[5] / sy : 1.0f;
		float r12 = sy > 1e-6f ? mat[6] / sy : 0.0f;
		float r20 = sz > 1e-6f ? mat[8] / sz : 0.0f;
		float r21 = sz > 1e-6f ? mat[9] / sz : 0.0f;
		float r22 = sz > 1e-6f ? mat[10] / sz : 1.0f;

		float trace = r00 + r11 + r22;

		scale[0] = sx;
		scale[1] = sy;
		scale[2] = sz;

		if(trace > 0.0f) {
			float s = sqrtf(trace + 1.0f) * 2.0f;
			rot[3] = 0.25f * s;
			rot[0] = (r21 - r12) / s;
			rot[1] = (r02 - r20) / s;
			rot[2] = (r10 - r01) / s;
		} else if(r00 > r11 && r00 > r22) {
			float s = sqrtf(1.0f + r00 - r11 - r22) * 2.0f;
			rot[3] = (r21 - r12) / s;
			rot[0] = 0.25f * s;
			rot[1] = (r01 + r10) / s;
			rot[2] = (r02 + r20) / s;
		} else if(r11 > r22) {
			float s = sqrtf(1.0f + r11 - r00 - r22) * 2.0f;
			rot[3] = (r02 - r20) / s;
			rot[0] = (r01 + r10) / s;
			rot[1] = 0.25f * s;
			rot[2] = (r12 + r21) / s;
		} else {
			float s = sqrtf(1.0f + r22 - r00 - r11) * 2.0f;
			rot[3] = (r10 - r01) / s;
			rot[0] = (r02 + r20) / s;
			rot[1] = (r12 + r21) / s;
			rot[2] = 0.25f * s;
		}

		QuatNormalize2(rot, rot);
	}
}
static void QuatSlerp(const quat_t from, const quat_t _to, float fraction, quat_t out) {
	float angle, cosAngle, sinAngle, backlerp, lerp;
	quat_t to;

	// cos() of angle
	cosAngle = from[0] * _to[0] + from[1] * _to[1] + from[2] * _to[2] + from[3] * _to[3];

	// negative handling is needed for taking shortest path (required for model joints)
	if(cosAngle < 0.0f) {
		cosAngle = -cosAngle;
		to[0] = -_to[0];
		to[1] = -_to[1];
		to[2] = -_to[2];
		to[3] = -_to[3];
	} else {
		QuatCopy(_to, to);
	}

	if(cosAngle < 0.999999f) {
		// spherical lerp (slerp)
		angle = acosf(cosAngle);
		sinAngle = sinf(angle);
		backlerp = sinf((1.0f - fraction) * angle) / sinAngle;
		lerp = sinf(fraction * angle) / sinAngle;
	} else {
		// linear lerp
		backlerp = 1.0f - fraction;
		lerp = fraction;
	}

	out[0] = from[0] * backlerp + to[0] * lerp;
	out[1] = from[1] * backlerp + to[1] * lerp;
	out[2] = from[2] * backlerp + to[2] * lerp;
	out[3] = from[3] * backlerp + to[3] * lerp;
}

// Hamilton product a * b for (x, y, z, w) quaternions. As rotations this is
// "apply b first, then a", i.e. R(a * b) = R(a) * R(b).
static void IQM_QuatMultiply(const quat_t a, const quat_t b, quat_t out) {
	quat_t r;

	r[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
	r[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
	r[1] = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
	r[2] = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];

	out[0] = r[0];
	out[1] = r[1];
	out[2] = r[2];
	out[3] = r[3];
}

static void IQM_QuatConjugate(const quat_t q, quat_t out) {
	out[0] = -q[0];
	out[1] = -q[1];
	out[2] = -q[2];
	out[3] = q[3];
}

/*
===============================================================================
External IQM animation loader.

Loads only pose/frame/bounds data from an IQM file and caches it by name.
The bind pose and joint hierarchy are always taken from the model, so
compatible skeletons (for example valveBiped) can share animations freely.
===============================================================================
*/
iqmAnimation_t* R_LoadIQMAnimation(const char* name) {
	union {
		unsigned* u;
		void* v;
	} buf;
	int filesize;
	iqmHeader_t* header;
	iqmPose_t* pose;
	iqmBounds_t* bounds;
	iqmJoint_t* joint;
	unsigned short* framedata;
	iqmTransform_t* transform;
	iqmAnimation_t* anim;
	int i, j;
	int posesSize, boundsSize, jointNamesSize;

	if(!name || !name[0]) {
		return NULL;
	}

	for(i = 0; i < tr.numIQMAnimations; i++) {
		if(!Q_stricmp(tr.iqmAnimations[i]->name, name)) {
			return tr.iqmAnimations[i];
		}
	}

	if(tr.numIQMAnimations >= MAX_IQM_ANIMATIONS) {
		ri.Printf(PRINT_WARNING, "R_LoadIQMAnimation: MAX_IQM_ANIMATIONS hit (%d)\n", MAX_IQM_ANIMATIONS);
		return NULL;
	}

	filesize = ri.FS_ReadFile(name, (void**)&buf.v);
	if(!buf.u) {
		ri.Printf(PRINT_WARNING, "R_LoadIQMAnimation: couldn't load '%s'\n", name);
		return NULL;
	}

	if(filesize < (int)sizeof(iqmHeader_t)) {
		ri.Printf(PRINT_WARNING, "R_LoadIQMAnimation: '%s' is too small\n", name);
		ri.FS_FreeFile(buf.v);
		return NULL;
	}

	header = (iqmHeader_t*)buf.u;
	if(Q_strncmp(header->magic, IQM_MAGIC, sizeof(header->magic))) {
		ri.Printf(PRINT_WARNING, "R_LoadIQMAnimation: '%s' is not an IQM file\n", name);
		ri.FS_FreeFile(buf.v);
		return NULL;
	}

	LL(header->version);
	if(header->version != IQM_VERSION) {
		ri.Printf(PRINT_WARNING, "R_LoadIQMAnimation: '%s' has unsupported IQM version %d\n", name, header->version);
		ri.FS_FreeFile(buf.v);
		return NULL;
	}

	LL(header->filesize);
	if(header->filesize > filesize || header->filesize > 128 << 20) {
		ri.Printf(PRINT_WARNING, "R_LoadIQMAnimation: '%s' has invalid filesize\n", name);
		ri.FS_FreeFile(buf.v);
		return NULL;
	}

	LL(header->num_text);
	LL(header->ofs_text);
	LL(header->num_meshes);
	LL(header->ofs_meshes);
	LL(header->num_vertexarrays);
	LL(header->num_vertexes);
	LL(header->ofs_vertexarrays);
	LL(header->num_triangles);
	LL(header->ofs_triangles);
	LL(header->ofs_adjacency);
	LL(header->num_joints);
	LL(header->ofs_joints);
	LL(header->num_poses);
	LL(header->ofs_poses);
	LL(header->num_anims);
	LL(header->ofs_anims);
	LL(header->num_frames);
	LL(header->num_framechannels);
	LL(header->ofs_frames);
	LL(header->ofs_bounds);

	if(header->num_poses == 0 || header->num_frames == 0) {
		ri.Printf(PRINT_WARNING, "R_LoadIQMAnimation: '%s' has no pose or frame data\n", name);
		ri.FS_FreeFile(buf.v);
		return NULL;
	}

	if(header->num_poses > IQM_MAX_JOINTS) {
		ri.Printf(PRINT_WARNING, "R_LoadIQMAnimation: '%s' has too many poses (%d)\n", name, header->num_poses);
		ri.FS_FreeFile(buf.v);
		return NULL;
	}

	if(IQM_CheckRange(header, header->ofs_poses, header->num_poses, sizeof(iqmPose_t)) || IQM_CheckRange(header, header->ofs_frames, header->num_frames * header->num_framechannels, sizeof(unsigned short)) || (header->ofs_bounds && IQM_CheckRange(header, header->ofs_bounds, header->num_frames, sizeof(iqmBounds_t)))) {
		ri.Printf(PRINT_WARNING, "R_LoadIQMAnimation: '%s' has invalid data ranges\n", name);
		ri.FS_FreeFile(buf.v);
		return NULL;
	}

	// joint metadata is optional (older files may lack ofs_joints), but if
	// present it lets us retarget by name against the base model.
	if(header->num_joints && header->ofs_joints) {
		if(IQM_CheckRange(header, header->ofs_joints, header->num_joints, sizeof(iqmJoint_t))) {
			ri.Printf(PRINT_WARNING, "R_LoadIQMAnimation: '%s' has invalid joint data\n", name);
			ri.FS_FreeFile(buf.v);
			return NULL;
		}

		jointNamesSize = 0;
		joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
		for(i = 0; i < header->num_joints; i++, joint++) {
			LL(joint->name);
			LL(joint->parent);
			if(joint->name >= header->num_text) {
				ri.Printf(PRINT_WARNING, "R_LoadIQMAnimation: '%s' has invalid joint name index\n", name);
				ri.FS_FreeFile(buf.v);
				return NULL;
			}
			jointNamesSize += strlen((char*)header + header->ofs_text + joint->name) + 1;
		}
	} else {
		jointNamesSize = 0;
	}

	posesSize = header->num_poses * header->num_frames * sizeof(iqmTransform_t);
	boundsSize = header->ofs_bounds ? header->num_frames * 6 * sizeof(float) : 0;

	// Memory layout (everything 4-byte aligned, names last because they have
	// arbitrary length):
	//   anim | poses | bounds | jointBindLocal | jointParents | jointNames
	// NOTE: the joint data must start AFTER the bounds array. It used to start
	// at the bounds pointer, so copying the bounds below overwrote the joint
	// parents / names / bind pose of the animation.
	anim = (iqmAnimation_t*)ri.Hunk_Alloc(sizeof(*anim) + posesSize + boundsSize + (header->num_joints ? header->num_joints * (sizeof(iqmTransform_t) + sizeof(int)) : 0) + jointNamesSize, h_low);
	Q_strncpyz(anim->name, name, sizeof(anim->name));
	anim->num_poses = header->num_poses;
	anim->num_frames = header->num_frames;
	anim->poses = (iqmTransform_t*)((byte*)anim + sizeof(*anim));
	anim->bounds = header->ofs_bounds ? (float*)((byte*)anim + sizeof(*anim) + posesSize) : NULL;

	anim->hasBindPose = false;
	anim->jointBindLocal = NULL;

	if(header->num_joints && header->ofs_joints) {
		byte* tail = (byte*)anim + sizeof(*anim) + posesSize + boundsSize;

		anim->num_joints = header->num_joints;

		anim->jointBindLocal = (iqmTransform_t*)tail;
		tail += header->num_joints * sizeof(iqmTransform_t);

		anim->jointParents = (int*)tail;
		tail += header->num_joints * sizeof(int);

		anim->jointNames = (char*)tail;
		tail += jointNamesSize;

		anim->hasBindPose = true;

		joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
		{
			char* namePtr = anim->jointNames;
			for(i = 0; i < header->num_joints; i++, joint++) {
				char* srcName = (char*)header + header->ofs_text + joint->name;
				int len = strlen(srcName) + 1;
				Com_Memcpy(namePtr, srcName, len);
				namePtr += len;
				anim->jointParents[i] = joint->parent;

				// local bind pose of the animation skeleton (TRS, no matrices:
				// retargeting works on rotations only)
				VectorCopy(joint->translate, anim->jointBindLocal[i].translate);
				QuatNormalize2(joint->rotate, anim->jointBindLocal[i].rotate);
				VectorCopy(joint->scale, anim->jointBindLocal[i].scale);
			}
		}
	} else {
		anim->num_joints = 0;
		anim->jointNames = NULL;
		anim->jointParents = NULL;
	}

	pose = (iqmPose_t*)((byte*)header + header->ofs_poses);
	for(i = 0; i < header->num_poses; i++, pose++) {
		LL(pose->parent);
		LL(pose->mask);
		for(j = 0; j < 10; j++) {
			LL(pose->channeloffset[j]);
			LL(pose->channelscale[j]);
		}
	}

	if(header->ofs_bounds) {
		bounds = (iqmBounds_t*)((byte*)header + header->ofs_bounds);
		for(i = 0; i < header->num_frames; i++, bounds++) {
			LL(bounds->bbmin[0]);
			LL(bounds->bbmin[1]);
			LL(bounds->bbmin[2]);
			LL(bounds->bbmax[0]);
			LL(bounds->bbmax[1]);
			LL(bounds->bbmax[2]);
		}
	}

	transform = anim->poses;
	framedata = (unsigned short*)((byte*)header + header->ofs_frames);
	for(i = 0; i < header->num_frames; i++) {
		pose = (iqmPose_t*)((byte*)header + header->ofs_poses);
		for(j = 0; j < header->num_poses; j++, pose++, transform++) {
			vec3_t translate;
			quat_t rotate;
			vec3_t scale;

			translate[0] = pose->channeloffset[0];
			if(pose->mask & 0x001) translate[0] += *framedata++ * pose->channelscale[0];
			translate[1] = pose->channeloffset[1];
			if(pose->mask & 0x002) translate[1] += *framedata++ * pose->channelscale[1];
			translate[2] = pose->channeloffset[2];
			if(pose->mask & 0x004) translate[2] += *framedata++ * pose->channelscale[2];

			rotate[0] = pose->channeloffset[3];
			if(pose->mask & 0x008) rotate[0] += *framedata++ * pose->channelscale[3];
			rotate[1] = pose->channeloffset[4];
			if(pose->mask & 0x010) rotate[1] += *framedata++ * pose->channelscale[4];
			rotate[2] = pose->channeloffset[5];
			if(pose->mask & 0x020) rotate[2] += *framedata++ * pose->channelscale[5];
			rotate[3] = pose->channeloffset[6];
			if(pose->mask & 0x040) rotate[3] += *framedata++ * pose->channelscale[6];

			scale[0] = pose->channeloffset[7];
			if(pose->mask & 0x080) scale[0] += *framedata++ * pose->channelscale[7];
			scale[1] = pose->channeloffset[8];
			if(pose->mask & 0x100) scale[1] += *framedata++ * pose->channelscale[8];
			scale[2] = pose->channeloffset[9];
			if(pose->mask & 0x200) scale[2] += *framedata++ * pose->channelscale[9];

			VectorCopy(translate, transform->translate);
			QuatNormalize2(rotate, transform->rotate);
			VectorCopy(scale, transform->scale);
		}
	}

	if(header->ofs_bounds) {
		bounds = (iqmBounds_t*)((byte*)header + header->ofs_bounds);
		for(i = 0; i < header->num_frames; i++, bounds++) {
			anim->bounds[i * 6 + 0] = bounds->bbmin[0];
			anim->bounds[i * 6 + 1] = bounds->bbmin[1];
			anim->bounds[i * 6 + 2] = bounds->bbmin[2];
			anim->bounds[i * 6 + 3] = bounds->bbmax[0];
			anim->bounds[i * 6 + 4] = bounds->bbmax[1];
			anim->bounds[i * 6 + 5] = bounds->bbmax[2];
		}
	}

	ri.FS_FreeFile(buf.v);

	tr.iqmAnimations[tr.numIQMAnimations++] = anim;

	ri.Printf(PRINT_DEVELOPER, "R_LoadIQMAnimation: loaded '%s' (%d poses x %d frames, %d joints)\n", name, anim->num_poses, anim->num_frames, anim->num_joints);

	return anim;
}

/*
=================
R_LoadIQM

Load an IQM model and compute the joint matrices for every frame.
=================
*/
bool R_LoadIQM(model_t* mod, void* buffer, int filesize, const char* mod_name) {
	iqmHeader_t* header;
	iqmVertexArray_t* vertexarray;
	iqmTriangle_t* triangle;
	iqmMesh_t* mesh;
	iqmJoint_t* joint;
	iqmPose_t* pose;
	iqmBounds_t* bounds;
	unsigned short* framedata;
	char* str;
	int i, j, k;
	iqmTransform_t* transform;
	float *mat, *matInv;
	size_t size, joint_names;
	byte* dataPtr;
	iqmData_t* iqmData;
	srfIQModel_t* surface;
	char meshName[MAX_QEXTENDEDPATH];
	int vertexArrayFormat[IQM_COLOR + 1];
	int allocateInfluences;
	byte* blendIndexes;
	union {
		byte* b;
		float* f;
	} blendWeights;

	if(filesize < sizeof(iqmHeader_t)) {
		return false;
	}

	header = (iqmHeader_t*)buffer;
	if(Q_strncmp(header->magic, IQM_MAGIC, sizeof(header->magic))) {
		return false;
	}

	LL(header->version);
	if(header->version != IQM_VERSION) {
		ri.Printf(PRINT_WARNING, "R_LoadIQM: %s is a unsupported IQM version (%d), only version %d is supported.\n", mod_name, header->version, IQM_VERSION);
		return false;
	}

	LL(header->filesize);
	if(header->filesize > filesize || header->filesize > 128 << 20) {
		return false;
	}

	LL(header->flags);
	LL(header->num_text);
	LL(header->ofs_text);
	LL(header->num_meshes);
	LL(header->ofs_meshes);
	LL(header->num_vertexarrays);
	LL(header->num_vertexes);
	LL(header->ofs_vertexarrays);
	LL(header->num_triangles);
	LL(header->ofs_triangles);
	LL(header->ofs_adjacency);
	LL(header->num_joints);
	LL(header->ofs_joints);
	LL(header->num_poses);
	LL(header->ofs_poses);
	LL(header->num_anims);
	LL(header->ofs_anims);
	LL(header->num_frames);
	LL(header->num_framechannels);
	LL(header->ofs_frames);
	LL(header->ofs_bounds);
	LL(header->num_comment);
	LL(header->ofs_comment);
	LL(header->num_extensions);
	LL(header->ofs_extensions);

	// check ioq3 joint limit
	if(header->num_joints > IQM_MAX_JOINTS) {
		ri.Printf(PRINT_WARNING, "R_LoadIQM: %s has more than %d joints (%d).\n", mod_name, IQM_MAX_JOINTS, header->num_joints);
		return false;
	}

	for(i = 0; i < ARRAY_LEN(vertexArrayFormat); i++) {
		vertexArrayFormat[i] = -1;
	}

	blendIndexes = NULL;
	blendWeights.b = NULL;

	allocateInfluences = 0;

	if(header->num_meshes) {
		// check and swap vertex arrays
		if(IQM_CheckRange(header, header->ofs_vertexarrays, header->num_vertexarrays, sizeof(iqmVertexArray_t))) {
			return false;
		}
		vertexarray = (iqmVertexArray_t*)((byte*)header + header->ofs_vertexarrays);
		for(i = 0; i < header->num_vertexarrays; i++, vertexarray++) {
			int n, *intPtr;

			if(vertexarray->size <= 0 || vertexarray->size > 4) {
				return false;
			}

			// total number of values
			n = header->num_vertexes * vertexarray->size;

			switch(vertexarray->format) {
				case IQM_BYTE:
				case IQM_UBYTE:
					// 1 byte, no swapping necessary
					if(IQM_CheckRange(header, vertexarray->offset, n, sizeof(byte))) {
						return false;
					}
					break;
				case IQM_INT:
				case IQM_UINT:
				case IQM_FLOAT:
					// 4-byte swap
					if(IQM_CheckRange(header, vertexarray->offset, n, sizeof(float))) {
						return false;
					}
					intPtr = (int*)((byte*)header + vertexarray->offset);
					for(j = 0; j < n; j++, intPtr++) {
						LL(*intPtr);
					}
					break;
				default:
					// not supported
					return false;
					break;
			}

			if(vertexarray->type < ARRAY_LEN(vertexArrayFormat)) {
				vertexArrayFormat[vertexarray->type] = vertexarray->format;
			}

			switch(vertexarray->type) {
				case IQM_POSITION:
				case IQM_NORMAL:
					if(vertexarray->format != IQM_FLOAT || vertexarray->size != 3) {
						return false;
					}
					break;
				case IQM_TANGENT:
					if(vertexarray->format != IQM_FLOAT || vertexarray->size != 4) {
						return false;
					}
					break;
				case IQM_TEXCOORD:
					if(vertexarray->format != IQM_FLOAT || vertexarray->size != 2) {
						return false;
					}
					break;
				case IQM_BLENDINDEXES:
					if((vertexarray->format != IQM_INT && vertexarray->format != IQM_UBYTE) || vertexarray->size != 4) {
						return false;
					}
					blendIndexes = (byte*)header + vertexarray->offset;
					break;
				case IQM_BLENDWEIGHTS:
					if((vertexarray->format != IQM_FLOAT && vertexarray->format != IQM_UBYTE) || vertexarray->size != 4) {
						return false;
					}
					if(vertexarray->format == IQM_FLOAT) {
						blendWeights.f = (float*)((byte*)header + vertexarray->offset);
					} else {
						blendWeights.b = (byte*)header + vertexarray->offset;
					}
					break;
				case IQM_COLOR:
					if(vertexarray->format != IQM_UBYTE || vertexarray->size != 4) {
						return false;
					}
					break;
			}
		}

		// check for required vertex arrays
		if(vertexArrayFormat[IQM_POSITION] == -1 || vertexArrayFormat[IQM_NORMAL] == -1 || vertexArrayFormat[IQM_TEXCOORD] == -1) {
			ri.Printf(PRINT_WARNING, "R_LoadIQM: %s is missing IQM_POSITION, IQM_NORMAL, and/or IQM_TEXCOORD array.\n", mod_name);
			return false;
		}

		if(header->num_joints) {
			if(vertexArrayFormat[IQM_BLENDINDEXES] == -1 || vertexArrayFormat[IQM_BLENDWEIGHTS] == -1) {
				ri.Printf(PRINT_WARNING, "R_LoadIQM: %s is missing IQM_BLENDINDEXES and/or IQM_BLENDWEIGHTS array.\n", mod_name);
				return false;
			}
		} else {
			// ignore blend arrays if present
			vertexArrayFormat[IQM_BLENDINDEXES] = -1;
			vertexArrayFormat[IQM_BLENDWEIGHTS] = -1;
		}

		// opengl2 renderer requires tangents
		if(vertexArrayFormat[IQM_TANGENT] == -1) {
			ri.Printf(PRINT_WARNING, "R_LoadIQM: %s is missing IQM_TANGENT array.\n", mod_name);
			return false;
		}

		// check and swap triangles
		if(IQM_CheckRange(header, header->ofs_triangles, header->num_triangles, sizeof(iqmTriangle_t))) {
			return false;
		}
		triangle = (iqmTriangle_t*)((byte*)header + header->ofs_triangles);
		for(i = 0; i < header->num_triangles; i++, triangle++) {
			LL(triangle->vertex[0]);
			LL(triangle->vertex[1]);
			LL(triangle->vertex[2]);

			if(triangle->vertex[0] > header->num_vertexes || triangle->vertex[1] > header->num_vertexes || triangle->vertex[2] > header->num_vertexes) {
				return false;
			}
		}

		// check and swap meshes
		if(IQM_CheckRange(header, header->ofs_meshes, header->num_meshes, sizeof(iqmMesh_t))) {
			return false;
		}
		mesh = (iqmMesh_t*)((byte*)header + header->ofs_meshes);
		for(i = 0; i < header->num_meshes; i++, mesh++) {
			LL(mesh->name);
			LL(mesh->material);
			LL(mesh->first_vertex);
			LL(mesh->num_vertexes);
			LL(mesh->first_triangle);
			LL(mesh->num_triangles);

			if(mesh->name < header->num_text) {
				Q_strncpyz(meshName, (char*)header + header->ofs_text + mesh->name, sizeof(meshName));
			} else {
				meshName[0] = '\0';
			}

			// check ioq3 limits
			if(mesh->num_vertexes >= SHADER_MAX_VERTEXES) {
				ri.Printf(PRINT_WARNING, "R_LoadIQM: %s has more than %i verts on %s (%i).\n", mod_name, SHADER_MAX_VERTEXES - 1, meshName[0] ? meshName : "a surface", mesh->num_vertexes);
				return false;
			}
			if(mesh->num_triangles * 3 >= SHADER_MAX_INDEXES) {
				ri.Printf(PRINT_WARNING, "R_LoadIQM: %s has more than %i triangles on %s (%i).\n", mod_name, (SHADER_MAX_INDEXES / 3) - 1, meshName[0] ? meshName : "a surface", mesh->num_triangles);
				return false;
			}

			if(mesh->first_vertex >= header->num_vertexes || mesh->first_vertex + mesh->num_vertexes > header->num_vertexes || mesh->first_triangle >= header->num_triangles || mesh->first_triangle + mesh->num_triangles > header->num_triangles || mesh->name >= header->num_text || mesh->material >= header->num_text) {
				return false;
			}

			// find number of unique blend influences per mesh
			if(header->num_joints) {
				for(j = 0; j < mesh->num_vertexes; j++) {
					int vtx = mesh->first_vertex + j;

					for(k = 0; k < j; k++) {
						int influence = mesh->first_vertex + k;

						if(*(int*)&blendIndexes[4 * influence] != *(int*)&blendIndexes[4 * vtx]) {
							continue;
						}

						if(vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT) {
							if(blendWeights.f[4 * influence + 0] == blendWeights.f[4 * vtx + 0] && blendWeights.f[4 * influence + 1] == blendWeights.f[4 * vtx + 1] && blendWeights.f[4 * influence + 2] == blendWeights.f[4 * vtx + 2] && blendWeights.f[4 * influence + 3] == blendWeights.f[4 * vtx + 3]) {
								break;
							}
						} else {
							if(*(int*)&blendWeights.b[4 * influence] == *(int*)&blendWeights.b[4 * vtx]) {
								break;
							}
						}
					}

					if(k == j) {
						allocateInfluences++;
					}
				}
			}
		}
	}

	if(header->num_poses != header->num_joints && header->num_poses != 0) {
		ri.Printf(PRINT_WARNING, "R_LoadIQM: %s has %d poses and %d joints, must have the same number or 0 poses\n", mod_name, header->num_poses, header->num_joints);
		return false;
	}

	joint_names = 0;

	if(header->num_joints) {
		// check and swap joints
		if(IQM_CheckRange(header, header->ofs_joints, header->num_joints, sizeof(iqmJoint_t))) {
			return false;
		}
		joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
		for(i = 0; i < header->num_joints; i++, joint++) {
			LL(joint->name);
			LL(joint->parent);
			LL(joint->translate[0]);
			LL(joint->translate[1]);
			LL(joint->translate[2]);
			LL(joint->rotate[0]);
			LL(joint->rotate[1]);
			LL(joint->rotate[2]);
			LL(joint->rotate[3]);
			LL(joint->scale[0]);
			LL(joint->scale[1]);
			LL(joint->scale[2]);

			if(joint->parent < -1 || joint->parent >= (int)header->num_joints || joint->name >= (int)header->num_text) {
				return false;
			}
			joint_names += strlen((char*)header + header->ofs_text + joint->name) + 1;
		}
	}

	if(header->num_poses) {
		// check and swap poses
		if(IQM_CheckRange(header, header->ofs_poses, header->num_poses, sizeof(iqmPose_t))) {
			return false;
		}
		pose = (iqmPose_t*)((byte*)header + header->ofs_poses);
		for(i = 0; i < header->num_poses; i++, pose++) {
			LL(pose->parent);
			LL(pose->mask);
			LL(pose->channeloffset[0]);
			LL(pose->channeloffset[1]);
			LL(pose->channeloffset[2]);
			LL(pose->channeloffset[3]);
			LL(pose->channeloffset[4]);
			LL(pose->channeloffset[5]);
			LL(pose->channeloffset[6]);
			LL(pose->channeloffset[7]);
			LL(pose->channeloffset[8]);
			LL(pose->channeloffset[9]);
			LL(pose->channelscale[0]);
			LL(pose->channelscale[1]);
			LL(pose->channelscale[2]);
			LL(pose->channelscale[3]);
			LL(pose->channelscale[4]);
			LL(pose->channelscale[5]);
			LL(pose->channelscale[6]);
			LL(pose->channelscale[7]);
			LL(pose->channelscale[8]);
			LL(pose->channelscale[9]);
		}
	}

	if(header->ofs_bounds) {
		// check and swap model bounds
		if(IQM_CheckRange(header, header->ofs_bounds, header->num_frames, sizeof(*bounds))) {
			return false;
		}
		bounds = (iqmBounds_t*)((byte*)header + header->ofs_bounds);
		for(i = 0; i < header->num_frames; i++) {
			LL(bounds->bbmin[0]);
			LL(bounds->bbmin[1]);
			LL(bounds->bbmin[2]);
			LL(bounds->bbmax[0]);
			LL(bounds->bbmax[1]);
			LL(bounds->bbmax[2]);

			bounds++;
		}
	}

	// allocate the model and copy the data
	size = sizeof(iqmData_t);
	if(header->num_meshes) {
		size += header->num_meshes * sizeof(srfIQModel_t);  // surfaces
		size += header->num_triangles * 3 * sizeof(int);    // triangles
		size += header->num_vertexes * 3 * sizeof(float);   // positions
		size += header->num_vertexes * 2 * sizeof(float);   // texcoords
		size += header->num_vertexes * 3 * sizeof(float);   // normals

		if(vertexArrayFormat[IQM_TANGENT] != -1) {
			size += header->num_vertexes * 4 * sizeof(float);  // tangents
		}

		if(vertexArrayFormat[IQM_COLOR] != -1) {
			size += header->num_vertexes * 4 * sizeof(byte);  // colors
		}

		if(allocateInfluences) {
			size += header->num_vertexes * sizeof(int);     // influences
			size += allocateInfluences * 4 * sizeof(byte);  // influenceBlendIndexes

			if(vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_UBYTE) {
				size += allocateInfluences * 4 * sizeof(byte);  // influenceBlendWeights
			} else if(vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT) {
				size += allocateInfluences * 4 * sizeof(float);  // influenceBlendWeights
			}
		}
	}
	if(header->num_joints) {
		size += joint_names;                              // joint names
		size += header->num_joints * sizeof(int);         // joint parents
		size += header->num_joints * 12 * sizeof(float);  // bind joint matricies
		size += header->num_joints * 12 * sizeof(float);  // inverse bind joint matricies
	}
	if(header->num_poses) {
		size += header->num_poses * header->num_frames * sizeof(iqmTransform_t);  // pose transforms
	}
	if(header->ofs_bounds) {
		size += header->num_frames * 6 * sizeof(float);  // model bounds
	} else if(header->num_meshes && header->num_frames == 0) {
		size += 6 * sizeof(float);  // model bounds
	}

	mod->type = MOD_IQM;
	iqmData = (iqmData_t*)ri.Hunk_Alloc(size, h_low);
	mod->modelData = iqmData;

	iqmData->anim = NULL;
	iqmData->animPoseMap = NULL;
	iqmData->animMotionMask = NULL;
	iqmData->bindLocal = NULL;

	// fill header
	iqmData->num_vertexes = (header->num_meshes > 0) ? header->num_vertexes : 0;
	iqmData->num_triangles = (header->num_meshes > 0) ? header->num_triangles : 0;
	iqmData->num_frames = header->num_frames;
	iqmData->num_surfaces = header->num_meshes;
	iqmData->num_joints = header->num_joints;
	iqmData->num_poses = header->num_poses;
	iqmData->blendWeightsType = vertexArrayFormat[IQM_BLENDWEIGHTS];

	dataPtr = (byte*)iqmData + sizeof(iqmData_t);
	if(header->num_meshes) {
		iqmData->surfaces = (struct srfIQModel_s*)dataPtr;
		dataPtr += header->num_meshes * sizeof(srfIQModel_t);

		iqmData->triangles = (int*)dataPtr;
		dataPtr += header->num_triangles * 3 * sizeof(int);  // triangles

		iqmData->positions = (float*)dataPtr;
		dataPtr += header->num_vertexes * 3 * sizeof(float);  // positions

		iqmData->texcoords = (float*)dataPtr;
		dataPtr += header->num_vertexes * 2 * sizeof(float);  // texcoords

		iqmData->normals = (float*)dataPtr;
		dataPtr += header->num_vertexes * 3 * sizeof(float);  // normals

		if(vertexArrayFormat[IQM_TANGENT] != -1) {
			iqmData->tangents = (float*)dataPtr;
			dataPtr += header->num_vertexes * 4 * sizeof(float);  // tangents
		}

		if(vertexArrayFormat[IQM_COLOR] != -1) {
			iqmData->colors = (byte*)dataPtr;
			dataPtr += header->num_vertexes * 4 * sizeof(byte);  // colors
		}

		if(allocateInfluences) {
			iqmData->influences = (int*)dataPtr;
			dataPtr += header->num_vertexes * sizeof(int);  // influences

			iqmData->influenceBlendIndexes = (byte*)dataPtr;
			dataPtr += allocateInfluences * 4 * sizeof(byte);  // influenceBlendIndexes

			if(vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_UBYTE) {
				iqmData->influenceBlendWeights.b = (byte*)dataPtr;
				dataPtr += allocateInfluences * 4 * sizeof(byte);  // influenceBlendWeights
			} else if(vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT) {
				iqmData->influenceBlendWeights.f = (float*)dataPtr;
				dataPtr += allocateInfluences * 4 * sizeof(float);  // influenceBlendWeights
			}
		}
	}
	if(header->num_joints) {
		iqmData->jointNames = (char*)dataPtr;
		dataPtr += joint_names;  // joint names

		iqmData->jointParents = (int*)dataPtr;
		dataPtr += header->num_joints * sizeof(int);  // joint parents

		iqmData->bindJoints = (float*)dataPtr;
		dataPtr += header->num_joints * 12 * sizeof(float);  // bind joint matricies

		iqmData->invBindJoints = (float*)dataPtr;
		dataPtr += header->num_joints * 12 * sizeof(float);  // inverse bind joint matricies
	}
	if(header->num_poses) {
		iqmData->poses = (iqmTransform_t*)dataPtr;
		dataPtr += header->num_poses * header->num_frames * sizeof(iqmTransform_t);  // pose transforms
	}
	if(header->ofs_bounds) {
		iqmData->bounds = (float*)dataPtr;
		dataPtr += header->num_frames * 6 * sizeof(float);  // model bounds
	} else if(header->num_meshes && header->num_frames == 0) {
		iqmData->bounds = (float*)dataPtr;
		dataPtr += 6 * sizeof(float);  // model bounds
	}

	if(header->num_meshes) {
		// register shaders
		// overwrite the material offset with the shader index
		mesh = (iqmMesh_t*)((byte*)header + header->ofs_meshes);
		surface = iqmData->surfaces;
		str = (char*)header + header->ofs_text;
		for(i = 0; i < header->num_meshes; i++, mesh++, surface++) {
			surface->surfaceType = SF_IQM;
			Q_strncpyz(surface->name, str + mesh->name, sizeof(surface->name));
			Q_strlwr(surface->name);  // lowercase the surface name so skin compares are faster
			surface->shader = R_FindShader(str + mesh->material, LIGHTMAP_NONE, true);
			if(surface->shader->defaultShader) surface->shader = tr.defaultShader;
			surface->data = iqmData;
			surface->first_vertex = mesh->first_vertex;
			surface->num_vertexes = mesh->num_vertexes;
			surface->first_triangle = mesh->first_triangle;
			surface->num_triangles = mesh->num_triangles;
		}

		// copy triangles
		triangle = (iqmTriangle_t*)((byte*)header + header->ofs_triangles);
		for(i = 0; i < header->num_triangles; i++, triangle++) {
			iqmData->triangles[3 * i + 0] = triangle->vertex[0];
			iqmData->triangles[3 * i + 1] = triangle->vertex[1];
			iqmData->triangles[3 * i + 2] = triangle->vertex[2];
		}

		// copy vertexarrays and indexes
		vertexarray = (iqmVertexArray_t*)((byte*)header + header->ofs_vertexarrays);
		for(i = 0; i < header->num_vertexarrays; i++, vertexarray++) {
			int n;

			// skip disabled arrays
			if(vertexarray->type < ARRAY_LEN(vertexArrayFormat) && vertexArrayFormat[vertexarray->type] == -1) continue;

			// total number of values
			n = header->num_vertexes * vertexarray->size;

			switch(vertexarray->type) {
				case IQM_POSITION: Com_Memcpy(iqmData->positions, (byte*)header + vertexarray->offset, n * sizeof(float)); break;
				case IQM_NORMAL: Com_Memcpy(iqmData->normals, (byte*)header + vertexarray->offset, n * sizeof(float)); break;
				case IQM_TANGENT: Com_Memcpy(iqmData->tangents, (byte*)header + vertexarray->offset, n * sizeof(float)); break;
				case IQM_TEXCOORD: Com_Memcpy(iqmData->texcoords, (byte*)header + vertexarray->offset, n * sizeof(float)); break;
				case IQM_BLENDINDEXES:
				case IQM_BLENDWEIGHTS: break;
				case IQM_COLOR: Com_Memcpy(iqmData->colors, (byte*)header + vertexarray->offset, n * sizeof(byte)); break;
			}
		}

		// find unique blend influences per mesh
		if(allocateInfluences) {
			int vtx, influence, totalInfluences = 0;

			surface = iqmData->surfaces;
			for(i = 0; i < header->num_meshes; i++, surface++) {
				surface->first_influence = totalInfluences;
				surface->num_influences = 0;

				for(j = 0; j < surface->num_vertexes; j++) {
					vtx = surface->first_vertex + j;

					for(k = 0; k < surface->num_influences; k++) {
						influence = surface->first_influence + k;

						if(*(int*)&iqmData->influenceBlendIndexes[4 * influence] != *(int*)&blendIndexes[4 * vtx]) {
							continue;
						}

						if(vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT) {
							if(iqmData->influenceBlendWeights.f[4 * influence + 0] == blendWeights.f[4 * vtx + 0] && iqmData->influenceBlendWeights.f[4 * influence + 1] == blendWeights.f[4 * vtx + 1] && iqmData->influenceBlendWeights.f[4 * influence + 2] == blendWeights.f[4 * vtx + 2] && iqmData->influenceBlendWeights.f[4 * influence + 3] == blendWeights.f[4 * vtx + 3]) {
								break;
							}
						} else {
							if(*(int*)&iqmData->influenceBlendWeights.b[4 * influence] == *(int*)&blendWeights.b[4 * vtx]) {
								break;
							}
						}
					}

					iqmData->influences[vtx] = surface->first_influence + k;

					if(k == surface->num_influences) {
						influence = surface->first_influence + k;

						iqmData->influenceBlendIndexes[4 * influence + 0] = blendIndexes[4 * vtx + 0];
						iqmData->influenceBlendIndexes[4 * influence + 1] = blendIndexes[4 * vtx + 1];
						iqmData->influenceBlendIndexes[4 * influence + 2] = blendIndexes[4 * vtx + 2];
						iqmData->influenceBlendIndexes[4 * influence + 3] = blendIndexes[4 * vtx + 3];

						if(vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT) {
							iqmData->influenceBlendWeights.f[4 * influence + 0] = blendWeights.f[4 * vtx + 0];
							iqmData->influenceBlendWeights.f[4 * influence + 1] = blendWeights.f[4 * vtx + 1];
							iqmData->influenceBlendWeights.f[4 * influence + 2] = blendWeights.f[4 * vtx + 2];
							iqmData->influenceBlendWeights.f[4 * influence + 3] = blendWeights.f[4 * vtx + 3];
						} else {
							iqmData->influenceBlendWeights.b[4 * influence + 0] = blendWeights.b[4 * vtx + 0];
							iqmData->influenceBlendWeights.b[4 * influence + 1] = blendWeights.b[4 * vtx + 1];
							iqmData->influenceBlendWeights.b[4 * influence + 2] = blendWeights.b[4 * vtx + 2];
							iqmData->influenceBlendWeights.b[4 * influence + 3] = blendWeights.b[4 * vtx + 3];
						}

						totalInfluences++;
						surface->num_influences++;
					}
				}
			}
		}
	}

	if(header->num_joints) {
		// copy joint names
		str = iqmData->jointNames;
		joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
		for(i = 0; i < header->num_joints; i++, joint++) {
			char* name = (char*)header + header->ofs_text + joint->name;
			int len = strlen(name) + 1;
			Com_Memcpy(str, name, len);
			str += len;
		}

		// copy joint parents
		joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
		for(i = 0; i < header->num_joints; i++, joint++) {
			iqmData->jointParents[i] = joint->parent;
		}

		// local bind pose (TRS) of every joint. External animations are
		// retargeted against this, so bone lengths and scale always come from
		// the model itself and never from the animation.
		iqmData->bindLocal = (iqmTransform_t*)ri.Hunk_Alloc(header->num_joints * sizeof(iqmTransform_t), h_low);

		// calculate bind joint matrices and their inverses
		mat = iqmData->bindJoints;
		matInv = iqmData->invBindJoints;
		joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
		for(i = 0; i < header->num_joints; i++, joint++) {
			float baseFrame[12], invBaseFrame[12];

			QuatNormalize2(joint->rotate, joint->rotate);

			VectorCopy(joint->translate, iqmData->bindLocal[i].translate);
			QuatCopy(joint->rotate, iqmData->bindLocal[i].rotate);
			VectorCopy(joint->scale, iqmData->bindLocal[i].scale);

			JointToMatrix(joint->rotate, joint->scale, joint->translate, baseFrame);
			Matrix34Invert(baseFrame, invBaseFrame);

			if(joint->parent >= 0) {
				Matrix34Multiply(iqmData->bindJoints + 12 * joint->parent, baseFrame, mat);
				mat += 12;
				Matrix34Multiply(invBaseFrame, iqmData->invBindJoints + 12 * joint->parent, matInv);
				matInv += 12;
			} else {
				Com_Memcpy(mat, baseFrame, sizeof(baseFrame));
				mat += 12;
				Com_Memcpy(matInv, invBaseFrame, sizeof(invBaseFrame));
				matInv += 12;
			}
		}
	}

	if(header->num_poses) {
		// calculate pose transforms
		transform = iqmData->poses;
		framedata = (unsigned short*)((byte*)header + header->ofs_frames);
		for(i = 0; i < header->num_frames; i++) {
			pose = (iqmPose_t*)((byte*)header + header->ofs_poses);
			for(j = 0; j < header->num_poses; j++, pose++, transform++) {
				vec3_t translate;
				quat_t rotate;
				vec3_t scale;

				translate[0] = pose->channeloffset[0];
				if(pose->mask & 0x001) translate[0] += *framedata++ * pose->channelscale[0];
				translate[1] = pose->channeloffset[1];
				if(pose->mask & 0x002) translate[1] += *framedata++ * pose->channelscale[1];
				translate[2] = pose->channeloffset[2];
				if(pose->mask & 0x004) translate[2] += *framedata++ * pose->channelscale[2];

				rotate[0] = pose->channeloffset[3];
				if(pose->mask & 0x008) rotate[0] += *framedata++ * pose->channelscale[3];
				rotate[1] = pose->channeloffset[4];
				if(pose->mask & 0x010) rotate[1] += *framedata++ * pose->channelscale[4];
				rotate[2] = pose->channeloffset[5];
				if(pose->mask & 0x020) rotate[2] += *framedata++ * pose->channelscale[5];
				rotate[3] = pose->channeloffset[6];
				if(pose->mask & 0x040) rotate[3] += *framedata++ * pose->channelscale[6];

				scale[0] = pose->channeloffset[7];
				if(pose->mask & 0x080) scale[0] += *framedata++ * pose->channelscale[7];
				scale[1] = pose->channeloffset[8];
				if(pose->mask & 0x100) scale[1] += *framedata++ * pose->channelscale[8];
				scale[2] = pose->channeloffset[9];
				if(pose->mask & 0x200) scale[2] += *framedata++ * pose->channelscale[9];

				VectorCopy(translate, transform->translate);
				QuatNormalize2(rotate, transform->rotate);
				VectorCopy(scale, transform->scale);
			}
		}
	}

	// copy model bounds
	if(header->ofs_bounds) {
		mat = iqmData->bounds;
		bounds = (iqmBounds_t*)((byte*)header + header->ofs_bounds);
		for(i = 0; i < header->num_frames; i++) {
			mat[0] = bounds->bbmin[0];
			mat[1] = bounds->bbmin[1];
			mat[2] = bounds->bbmin[2];
			mat[3] = bounds->bbmax[0];
			mat[4] = bounds->bbmax[1];
			mat[5] = bounds->bbmax[2];

			mat += 6;
			bounds++;
		}
	} else if(header->num_meshes && header->num_frames == 0) {
		mat = iqmData->bounds;

		ClearBounds(&iqmData->bounds[0], &iqmData->bounds[3]);
		for(i = 0; i < header->num_vertexes; i++) {
			AddPointToBounds(&iqmData->positions[i * 3], &iqmData->bounds[0], &iqmData->bounds[3]);
		}
	}

	// Create VAO surfaces
	if(iqmData->num_surfaces && iqmData->num_joints <= glRefConfig.glslMaxAnimatedBones) {
		srfVaoIQModel_t* vaoSurf;
		srfIQModel_t* surf;

		iqmData->numVaoSurfaces = iqmData->num_surfaces;
		iqmData->vaoSurfaces = ri.Hunk_Alloc(sizeof(*iqmData->vaoSurfaces) * iqmData->numVaoSurfaces, h_low);

		vaoSurf = iqmData->vaoSurfaces;
		surf = iqmData->surfaces;
		for(i = 0; i < iqmData->num_surfaces; i++, vaoSurf++, surf++) {
			uint32_t offset_xyz, offset_st, offset_normal, offset_tangent;
			uint32_t offset_blendindexes = 0, offset_blendweights = 0, stride;
			uint32_t dataSize, dataOfs;
			uint8_t* data;
			glIndex_t indexes[SHADER_MAX_INDEXES];
			glIndex_t* ptr;
			int* tri;

			offset_xyz = 0;
			offset_st = offset_xyz + sizeof(float) * 3;
			offset_normal = offset_st + sizeof(float) * 2;
			offset_tangent = offset_normal + sizeof(int16_t) * 4;

			if(iqmData->num_joints) {
				offset_blendindexes = offset_tangent + sizeof(int16_t) * 4;
				offset_blendweights = offset_blendindexes + sizeof(byte) * 4;

				if(vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT) {
					stride = offset_blendweights + sizeof(float) * 4;
				} else {
					stride = offset_blendweights + sizeof(byte) * 4;
				}
			} else {
				stride = offset_tangent + sizeof(int16_t) * 4;
			}

			dataSize = surf->num_vertexes * stride;

			data = ri.Malloc(dataSize);
			dataOfs = 0;

			for(j = 0; j < surf->num_vertexes; j++) {
				int vtx = surf->first_vertex + j;

				// xyz
				memcpy(data + dataOfs, &iqmData->positions[vtx * 3], sizeof(float) * 3);
				dataOfs += sizeof(float) * 3;

				// st
				memcpy(data + dataOfs, &iqmData->texcoords[vtx * 2], sizeof(float) * 2);
				dataOfs += sizeof(float) * 2;

				// normal
				R_VaoPackNormal((int16_t*)(data + dataOfs), &iqmData->normals[vtx * 3]);
				dataOfs += sizeof(int16_t) * 4;

				// tangent
				R_VaoPackTangent((int16_t*)(data + dataOfs), &iqmData->tangents[vtx * 4]);
				dataOfs += sizeof(int16_t) * 4;

				if(iqmData->num_joints) {
					// blendindexes
					memcpy(data + dataOfs, &blendIndexes[vtx * 4], sizeof(byte) * 4);
					dataOfs += sizeof(byte) * 4;

					// blendweights
					if(vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT) {
						memcpy(data + dataOfs, &blendWeights.f[vtx * 4], sizeof(float) * 4);
						dataOfs += sizeof(float) * 4;
					} else {
						memcpy(data + dataOfs, &blendWeights.b[vtx * 4], sizeof(byte) * 4);
						dataOfs += sizeof(byte) * 4;
					}
				}
			}

			tri = iqmData->triangles + 3 * surf->first_triangle;
			ptr = indexes;

			for(j = 0; j < surf->num_triangles; j++) {
				*ptr++ = (*tri++ - surf->first_vertex);
				*ptr++ = (*tri++ - surf->first_vertex);
				*ptr++ = (*tri++ - surf->first_vertex);
			}

			vaoSurf->surfaceType = SF_VAO_IQM;
			vaoSurf->iqmData = iqmData;
			vaoSurf->iqmSurface = surf;
			vaoSurf->numIndexes = surf->num_triangles * 3;
			vaoSurf->numVerts = surf->num_vertexes;

			vaoSurf->vao = R_CreateVao(va("staticIQMMesh_VAO '%s'", surf->name), data, dataSize, (byte*)indexes, surf->num_triangles * 3 * sizeof(indexes[0]), VAO_USAGE_STATIC);

			vaoSurf->vao->attribs[ATTR_INDEX_POSITION].enabled = 1;
			vaoSurf->vao->attribs[ATTR_INDEX_POSITION].enabled = 1;
			vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].enabled = 1;
			vaoSurf->vao->attribs[ATTR_INDEX_NORMAL].enabled = 1;
			vaoSurf->vao->attribs[ATTR_INDEX_TANGENT].enabled = 1;

			vaoSurf->vao->attribs[ATTR_INDEX_POSITION].count = 3;
			vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].count = 2;
			vaoSurf->vao->attribs[ATTR_INDEX_NORMAL].count = 4;
			vaoSurf->vao->attribs[ATTR_INDEX_TANGENT].count = 4;

			vaoSurf->vao->attribs[ATTR_INDEX_POSITION].type = GL_FLOAT;
			vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].type = GL_FLOAT;
			vaoSurf->vao->attribs[ATTR_INDEX_NORMAL].type = GL_SHORT;
			vaoSurf->vao->attribs[ATTR_INDEX_TANGENT].type = GL_SHORT;

			vaoSurf->vao->attribs[ATTR_INDEX_POSITION].normalized = GL_FALSE;
			vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].normalized = GL_FALSE;
			vaoSurf->vao->attribs[ATTR_INDEX_NORMAL].normalized = GL_TRUE;
			vaoSurf->vao->attribs[ATTR_INDEX_TANGENT].normalized = GL_TRUE;

			vaoSurf->vao->attribs[ATTR_INDEX_POSITION].offset = offset_xyz;
			vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].offset = offset_st;
			vaoSurf->vao->attribs[ATTR_INDEX_NORMAL].offset = offset_normal;
			vaoSurf->vao->attribs[ATTR_INDEX_TANGENT].offset = offset_tangent;

			vaoSurf->vao->attribs[ATTR_INDEX_POSITION].stride = stride;
			vaoSurf->vao->attribs[ATTR_INDEX_TEXCOORD].stride = stride;
			vaoSurf->vao->attribs[ATTR_INDEX_NORMAL].stride = stride;
			vaoSurf->vao->attribs[ATTR_INDEX_TANGENT].stride = stride;

			if(iqmData->num_joints) {
				vaoSurf->vao->attribs[ATTR_INDEX_BONE_INDEXES].enabled = 1;
				vaoSurf->vao->attribs[ATTR_INDEX_BONE_WEIGHTS].enabled = 1;

				vaoSurf->vao->attribs[ATTR_INDEX_BONE_INDEXES].count = 4;
				vaoSurf->vao->attribs[ATTR_INDEX_BONE_WEIGHTS].count = 4;

				vaoSurf->vao->attribs[ATTR_INDEX_BONE_INDEXES].type = GL_UNSIGNED_BYTE;
				vaoSurf->vao->attribs[ATTR_INDEX_BONE_INDEXES].normalized = GL_FALSE;

				if(vertexArrayFormat[IQM_BLENDWEIGHTS] == IQM_FLOAT) {
					vaoSurf->vao->attribs[ATTR_INDEX_BONE_WEIGHTS].type = GL_FLOAT;
					vaoSurf->vao->attribs[ATTR_INDEX_BONE_WEIGHTS].normalized = GL_FALSE;
				} else {
					vaoSurf->vao->attribs[ATTR_INDEX_BONE_WEIGHTS].type = GL_UNSIGNED_BYTE;
					vaoSurf->vao->attribs[ATTR_INDEX_BONE_WEIGHTS].normalized = GL_TRUE;
				}

				vaoSurf->vao->attribs[ATTR_INDEX_BONE_INDEXES].offset = offset_blendindexes;
				vaoSurf->vao->attribs[ATTR_INDEX_BONE_WEIGHTS].offset = offset_blendweights;

				vaoSurf->vao->attribs[ATTR_INDEX_BONE_INDEXES].stride = stride;
				vaoSurf->vao->attribs[ATTR_INDEX_BONE_WEIGHTS].stride = stride;
			}

			Vao_SetVertexPointers(vaoSurf->vao);

			ri.Free(data);
		}
	}

	return true;
}

/*
=============
R_CullIQM
=============
*/
static int R_CullIQM(iqmData_t* data, trRefEntity_t* ent) {
	vec3_t bounds[2];
	vec_t *oldBounds, *newBounds, *allBounds;
	int i, nframes;

	allBounds = data->anim ? data->anim->bounds : data->bounds;
	nframes = data->anim ? data->anim->num_frames : data->num_frames;

	if(!allBounds) {
		tr.pc.c_box_cull_md3_clip++;
		return CULL_CLIP;
	}

	if(nframes > 0) {
		if(ent->e.oldframe < 0 || ent->e.oldframe >= nframes) ent->e.oldframe = 0;
		if(ent->e.frame < 0 || ent->e.frame >= nframes) ent->e.frame = 0;
	}

	// compute bounds pointers
	oldBounds = allBounds + 6 * ent->e.oldframe;
	newBounds = allBounds + 6 * ent->e.frame;

	// calculate a bounding box in the current coordinate system
	for(i = 0; i < 3; i++) {
		bounds[0][i] = oldBounds[i] < newBounds[i] ? oldBounds[i] : newBounds[i];
		bounds[1][i] = oldBounds[i + 3] > newBounds[i + 3] ? oldBounds[i + 3] : newBounds[i + 3];
	}

	switch(R_CullLocalBox(bounds)) {
		case CULL_IN: tr.pc.c_box_cull_md3_in++; return CULL_IN;
		case CULL_CLIP: tr.pc.c_box_cull_md3_clip++; return CULL_CLIP;
		case CULL_OUT:
		default: tr.pc.c_box_cull_md3_out++; return CULL_OUT;
	}
}

/*
=================
R_ComputeIQMFogNum

=================
*/
int R_ComputeIQMFogNum(iqmData_t* data, trRefEntity_t* ent) {
	int i, j;
	fog_t* fog;
	const vec_t* bounds;
	const vec_t defaultBounds[6] = {-8, -8, -8, 8, 8, 8};
	vec3_t diag, center;
	vec3_t localOrigin;
	vec_t radius;

	if(tr.refdef.rdflags & RDF_NOWORLDMODEL) {
		return 0;
	}

	// FIXME: non-normalized axis issues
	{
		const float* allBounds = data->anim ? data->anim->bounds : data->bounds;

		if(allBounds) {
			int frame = ent->e.frame;
			int nframes = data->anim ? data->anim->num_frames : data->num_frames;

			if(nframes > 0 && (frame < 0 || frame >= nframes)) frame = 0;
			bounds = allBounds + 6 * frame;
		} else {
			bounds = defaultBounds;
		}
	}
	VectorSubtract(bounds + 3, bounds, diag);
	VectorMA(bounds, 0.5f, diag, center);
	VectorAdd(ent->e.origin, center, localOrigin);
	radius = 0.5f * VectorLength(diag);

	for(i = 1; i < tr.world->numfogs; i++) {
		fog = &tr.world->fogs[i];
		for(j = 0; j < 3; j++) {
			if(localOrigin[j] - radius >= fog->bounds[1][j]) {
				break;
			}
			if(localOrigin[j] + radius <= fog->bounds[0][j]) {
				break;
			}
		}
		if(j == 3) {
			return i;
		}
	}

	return 0;
}

/*
=================
R_AddIQMSurfaces

Add all surfaces of this model
=================
*/
void R_AddIQMSurfaces(trRefEntity_t* ent) {
	iqmData_t* data;
	srfIQModel_t* surface;
	void* drawSurf;
	int i, j;
	bool personalModel;
	int cull;
	int fogNum;
	bool useCubemap = false;
	shader_t* shader;
	skin_t* skin;

	data = tr.currentModel->modelData;
	surface = data->surfaces;

	// don't add third_person objects if not in a portal
	personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !(tr.viewParms.isPortal || (tr.viewParms.flags & (VPF_SHADOWMAP | VPF_DEPTHSHADOW)));

	{
		int nframes = data->anim ? data->anim->num_frames : data->num_frames;

		if(ent->e.renderfx & RF_WRAP_FRAMES) {
			if(nframes > 0) {
				ent->e.frame %= nframes;
				ent->e.oldframe %= nframes;
			}
		}

		//
		// Validate the frames so there is no chance of a crash.
		// This will write directly into the entity structure, so
		// when the surfaces are rendered, they don't need to be
		// range checked again.
		//
		if((ent->e.frame >= nframes) || (ent->e.frame < 0) || (ent->e.oldframe >= nframes) || (ent->e.oldframe < 0)) {
			if(ent->e.frame != 0) ri.Printf(PRINT_DEVELOPER, "R_AddIQMSurfaces: no such frame %d to %d for '%s'\n", ent->e.oldframe, ent->e.frame, tr.currentModel->name);
			ent->e.frame = 0;
			ent->e.oldframe = 0;
		}
	}

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_CullIQM(data, ent);
	if(cull == CULL_OUT) {
		return;
	}

	//
	// set up lighting now that we know we aren't culled
	//
	if(!personalModel) R_SetupEntityLighting(&tr.refdef, ent);

	//
	// see if we are in a fog volume
	//
	fogNum = R_ComputeIQMFogNum(data, ent);

	for(i = 0; i < data->num_surfaces; i++) {
		if(ent->e.customShader)
			shader = R_GetShaderByHandle(ent->e.customShader);
		else if(ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins) {
			skin = R_GetSkinByHandle(ent->e.customSkin);
			shader = tr.defaultShader;

			for(j = 0; j < skin->numSurfaces; j++) {
				if(!strcmp(skin->surfaces[j].name, surface->name)) {
					shader = skin->surfaces[j].shader;
					break;
				}
			}
		} else {
			shader = surface->shader;
		}

		if(data->numVaoSurfaces) {
			drawSurf = &data->vaoSurfaces[i];
		} else {
			drawSurf = surface;
		}

		if(r_reflectionsEntities->integer) useCubemap = R_ShaderRequiresCubemap(shader);

		if(!personalModel) {
			R_AddDrawSurf(drawSurf, shader, fogNum, 0, useCubemap, vec3_origin, 0.0f);
		}

		surface++;
	}
}

/*
=================
IQM_GetBindLocal

Local bind pose (translate / rotate / scale, relative to the parent joint)
of joint i of the model.
=================
*/
static void IQM_GetBindLocal(const iqmData_t* data, int i, iqmTransform_t* out) {
	float local[12];

	if(data->bindLocal) {
		*out = data->bindLocal[i];
		return;
	}

	// fallback: rebuild it from the model-space bind matrices
	if(data->jointParents[i] >= 0) {
		float invParent[12];

		Matrix34Invert(&data->bindJoints[data->jointParents[i] * 12], invParent);
		Matrix34Multiply(invParent, &data->bindJoints[i * 12], local);
	} else {
		Com_Memcpy(local, &data->bindJoints[i * 12], sizeof(local));
	}

	Matrix34Decompose(local, out->translate, out->rotate, out->scale);
}

/*
=================
IQM_BlendPose

Pose of animation joint i, interpolated between oldframe and frame.
=================
*/
static void IQM_BlendPose(const iqmTransform_t* all_poses, int nposes, int frame, int oldframe, float backlerp, int i, iqmTransform_t* out) {
	const iqmTransform_t* pose = &all_poses[frame * nposes + i];
	const iqmTransform_t* oldpose;
	float lerp;
	int k;

	if(frame == oldframe) {
		*out = *pose;
		return;
	}

	oldpose = &all_poses[oldframe * nposes + i];
	lerp = 1.0f - backlerp;

	for(k = 0; k < 3; k++) {
		out->translate[k] = oldpose->translate[k] * backlerp + pose->translate[k] * lerp;
		out->scale[k] = oldpose->scale[k] * backlerp + pose->scale[k] * lerp;
	}
	QuatSlerp(oldpose->rotate, pose->rotate, lerp, out->rotate);
}

/*
=================
IQM_BuildRelativeJoints

Local (parent relative) pose of every joint for the interpolated frame,
including the per-joint scale overrides from trap_R_SetBoneScale. Returns
the number of joints. Shared by ComputePoseMats (backend) and the jiggle
simulation (frontend) so that both see exactly the same animated pose.
=================
*/
static int IQM_BuildRelativeJoints(iqmData_t* data, int frame, int oldframe, float backlerp, iqmTransform_t* relativeJoints, const int* scaleJoints, const vec3_t* scaleValues, int numScaleJoints) {
	iqmTransform_t bindLocal[IQM_MAX_JOINTS];
	const iqmTransform_t* all_poses;
	const iqmAnimation_t* anim = data->anim;
	const int* poseMap;
	int i, nposes, nframes, numJoints;

	all_poses = anim ? anim->poses : data->poses;
	nposes = anim ? anim->num_poses : data->num_poses;
	nframes = anim ? anim->num_frames : data->num_frames;
	numJoints = data->num_joints;
	poseMap = anim ? data->animPoseMap : NULL;

	// Clamp frames to valid range in case the caller uses frames
	// that are only valid for the base model.
	if(nframes > 0) {
		if(frame < 0 || frame >= nframes) frame = 0;
		if(oldframe < 0 || oldframe >= nframes) oldframe = 0;
	}

	if(nposes > IQM_MAX_JOINTS) nposes = IQM_MAX_JOINTS;
	if(numJoints > IQM_MAX_JOINTS) numJoints = IQM_MAX_JOINTS;

	if(!anim) {
		// embedded animation: the poses are used as they are
		for(i = 0; i < numJoints; i++) {
			relativeJoints[i].translate[0] = 0.0f;
			relativeJoints[i].translate[1] = 0.0f;
			relativeJoints[i].translate[2] = 0.0f;
			relativeJoints[i].rotate[0] = 0.0f;
			relativeJoints[i].rotate[1] = 0.0f;
			relativeJoints[i].rotate[2] = 0.0f;
			relativeJoints[i].rotate[3] = 1.0f;
			relativeJoints[i].scale[0] = 1.0f;
			relativeJoints[i].scale[1] = 1.0f;
			relativeJoints[i].scale[2] = 1.0f;
		}

		for(i = 0; i < nposes && i < numJoints; i++) {
			IQM_BlendPose(all_poses, nposes, frame, oldframe, backlerp, i, &relativeJoints[i]);
		}
	} else {
		const bool haveAnimBind = anim->hasBindPose && anim->jointBindLocal && anim->num_poses == anim->num_joints;

		// every joint starts in the model's own bind pose; joints the
		// animation doesn't drive simply stay there
		for(i = 0; i < numJoints; i++) {
			IQM_GetBindLocal(data, i, &bindLocal[i]);
			relativeJoints[i] = bindLocal[i];
		}

		for(i = 0; i < nposes; i++) {
			const iqmTransform_t* tgt;
			const iqmTransform_t* ab = haveAnimBind ? &anim->jointBindLocal[i] : NULL;
			iqmTransform_t a;
			iqmTransform_t* out;
			bool motionJoint;
			int dst = poseMap ? poseMap[i] : i;

			if(dst < 0 || dst >= numJoints) continue;

			IQM_BlendPose(all_poses, nposes, frame, oldframe, backlerp, i, &a);

			tgt = &bindLocal[dst];
			out = &relativeJoints[dst];

			QuatCopy(a.rotate, out->rotate);
			QuatNormalize2(out->rotate, out->rotate);

			// --- translation: bone length always comes from the model ---
			VectorCopy(tgt->translate, out->translate);

			motionJoint = data->animMotionMask ? (data->animMotionMask[dst] != 0) : (data->jointParents[dst] < 0);
			if(motionJoint) {
				if(ab) {
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
						out->translate[k] = tgt->translate[k] + (a.translate[k] - ab->translate[k]) * ratio;
					}
				} else {
					VectorCopy(a.translate, out->translate);
				}
			}

			// --- scale: never changes ---
			VectorCopy(tgt->scale, out->scale);
		}
	}

	// per-joint scale overrides requested from cgame (trap_R_SetBoneScale).
	// Children inherit the scale through the pose matrix chain, so scaling
	// e.g. the head to 0 collapses the whole head sub-tree.
	for(i = 0; i < numScaleJoints; i++) {
		int joint = scaleJoints[i];
		if(joint >= 0 && joint < numJoints) {
			relativeJoints[joint].scale[0] *= scaleValues[i][0];
			relativeJoints[joint].scale[1] *= scaleValues[i][1];
			relativeJoints[joint].scale[2] *= scaleValues[i][2];
		}
	}

	return numJoints;
}

/*
=================
IQM_QuatMul

out = a * b (x, y, z, w order), out may alias a or b
=================
*/
static void IQM_QuatMul(const quat_t a, const quat_t b, quat_t out) {
	quat_t r;

	r[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
	r[1] = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
	r[2] = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
	r[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];

	out[0] = r[0];
	out[1] = r[1];
	out[2] = r[2];
	out[3] = r[3];
}

/*
=================
ComputePoseMats

Builds the skinning matrices of all joints for the (interpolated) frame.

Models that use their own embedded animation behave exactly like stock ioq3.

With an external animation (RE_RegisterModelEx) only ROTATIONS are taken from
the animation. Bone offsets (translation) and scale always come from the
model's own bind pose, so the proportions of the model can never change no
matter which skeleton the animation was made for. The one exception is the
motion carrier (root / pelvis): its movement relative to the animation's
bind pose is added to the model's bind position, scaled by the ratio of the
two skeleton heights, so the feet stay at the right height on every model.

copy the local rotation of the animation (Source engine behaviour)
Modes 1 and 2 give identical results when both skeletons have the same
rest rotations and differ only if the rest poses differ (finger curl,
A-pose / T-pose).

jiggleJoints / jiggleDeltas: rotations (in the parent space of the joint)
produced by the jiggle simulation, they are multiplied onto the animated
rotation of the joint (hair / cloth), children follow through the chain.
=================
*/
static void ComputePoseMats(iqmData_t* data, int frame, int oldframe, float backlerp, float* poseMats, const int* scaleJoints, const vec3_t* scaleValues, int numScaleJoints, const int* jiggleJoints, const quat_t* jiggleDeltas, int numJiggle) {
	iqmTransform_t relativeJoints[IQM_MAX_JOINTS];
	iqmTransform_t* relativeJoint;
	const int* jointParent;
	const float* invBindMat;
	float* poseMat;
	int i, numJoints;

	numJoints = IQM_BuildRelativeJoints(data, frame, oldframe, backlerp, relativeJoints, scaleJoints, scaleValues, numScaleJoints);

	// jiggle bones: rotate the animated joint by the simulated delta
	for(i = 0; i < numJiggle; i++) {
		int joint = jiggleJoints[i];
		if(joint >= 0 && joint < numJoints) {
			quat_t q;
			IQM_QuatMul(jiggleDeltas[i], relativeJoints[joint].rotate, q);
			QuatNormalize2(q, relativeJoints[joint].rotate);
		}
	}

	// multiply by inverse of bind pose and parent 'pose mat' (bind pose transform matrix)
	relativeJoint = relativeJoints;
	jointParent = data->jointParents;
	invBindMat = data->invBindJoints;
	poseMat = poseMats;
	for(i = 0; i < numJoints; i++, relativeJoint++, jointParent++, invBindMat += 12, poseMat += 12) {
		float mat1[12], mat2[12];

		JointToMatrix(relativeJoint->rotate, relativeJoint->scale, relativeJoint->translate, mat1);

		if(*jointParent >= 0) {
			Matrix34Multiply(&data->bindJoints[(*jointParent) * 12], mat1, mat2);
			Matrix34Multiply(mat2, invBindMat, mat1);
			Matrix34Multiply(&poseMats[(*jointParent) * 12], mat1, poseMat);
		} else {
			Matrix34Multiply(mat1, invBindMat, poseMat);
		}
	}
}

static void ComputeJointMats(iqmData_t* data, int frame, int oldframe, float backlerp, float* mat) {
	float* mat1;
	int i;

	if((data->anim ? data->anim->num_poses : data->num_poses) == 0) {
		Com_Memcpy(mat, data->bindJoints, data->num_joints * 12 * sizeof(float));
		return;
	}

	ComputePoseMats(data, frame, oldframe, backlerp, mat, NULL, NULL, 0, NULL, NULL, 0);

	for(i = 0; i < data->num_joints; i++) {
		float outmat[12];
		mat1 = mat + 12 * i;

		Com_Memcpy(outmat, mat1, sizeof(outmat));

		Matrix34Multiply(outmat, data->bindJoints + 12 * i, mat1);
	}
}

// JIGGLE-BEGIN
#define JIGGLE_SUBSTEP 0.0125f     // simulation step (80 Hz)
#define JIGGLE_MAX_SUBSTEPS 8      // per scene, dt is clamped to 0.1 s anyway
#define JIGGLE_MIN_SCALE 0.00001f  // below this the spring would get too stiff for explicit integration
#define JIGGLE_MAX_SCALE 8.0f
#define JIGGLE_MAX_ANGLE 1.3963f  // 80 degrees, a joint never bends further than this

static void Jiggle_TransformPoint(const float* m, const vec3_t p, vec3_t out) {
	out[0] = m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3];
	out[1] = m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7];
	out[2] = m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11];
}

static void Jiggle_TransformDir(const float* m, const vec3_t p, vec3_t out) {
	out[0] = m[0] * p[0] + m[1] * p[1] + m[2] * p[2];
	out[1] = m[4] * p[0] + m[5] * p[1] + m[6] * p[2];
	out[2] = m[8] * p[0] + m[9] * p[1] + m[10] * p[2];
}

/*
=================
Jiggle_QuatFromTo

Shortest rotation from unit vector a to unit vector b, limited to maxAngle.
=================
*/
static void Jiggle_QuatFromTo(const vec3_t a, const vec3_t b, float maxAngle, quat_t out) {
	vec3_t axis;
	float sinA, angle, s;

	CrossProduct(a, b, axis);
	sinA = VectorLength(axis);
	angle = atan2f(sinA, DotProduct(a, b));

	if(angle < 1e-5f) {
		out[0] = out[1] = out[2] = 0.0f;
		out[3] = 1.0f;
		return;
	}

	if(sinA > 1e-5f) {
		VectorScale(axis, 1.0f / sinA, axis);
	} else {
		// opposite vectors: any axis perpendicular to a will do
		vec3_t t = {1.0f, 0.0f, 0.0f};
		if(fabsf(a[0]) > 0.9f) {
			t[0] = 0.0f;
			t[1] = 1.0f;
		}
		CrossProduct(a, t, axis);
		VectorNormalize(axis);
	}

	if(angle > maxAngle) angle = maxAngle;

	s = sinf(angle * 0.5f);
	out[0] = axis[0] * s;
	out[1] = axis[1] * s;
	out[2] = axis[2] * s;
	out[3] = cosf(angle * 0.5f);
}

/*
=================
IQM_JiggleTip

Model space bind pose position of the tip of a joint: the first child, or
for a leaf joint the continuation of the parent -> joint offset.
=================
*/
static bool IQM_JiggleTip(const iqmData_t* data, int joint, vec3_t tip) {
	const float* b = &data->bindJoints[joint * 12];
	vec3_t pos, d;
	int i, parent;

	VectorSet(pos, b[3], b[7], b[11]);

	for(i = joint + 1; i < data->num_joints; i++) {
		if(data->jointParents[i] == joint) {
			const float* c = &data->bindJoints[i * 12];

			VectorSet(tip, c[3], c[7], c[11]);
			VectorSubtract(tip, pos, d);
			if(DotProduct(d, d) > 1e-4f) {
				return true;
			}
		}
	}

	parent = data->jointParents[joint];
	if(parent >= 0) {
		const float* pb = &data->bindJoints[parent * 12];

		d[0] = pos[0] - pb[3];
		d[1] = pos[1] - pb[7];
		d[2] = pos[2] - pb[11];
		if(DotProduct(d, d) > 1e-4f) {
			VectorAdd(pos, d, tip);
			return true;
		}
	}

	return false;
}

/*
=================
R_IQMJiggleEntity

Advances the jiggle simulation of one entity to timeMs and stores the
resulting rotation of every jiggled joint in entries[].delta.
=================
*/
int R_IQMJiggleEntity(const refEntity_t* re, int sceneEntityNum, jiggleEntry_t* entries, int numEntries, jiggleState_t* st, int timeMs) {
	const model_t* mod = R_GetModelByHandle(re->hModel);
	iqmData_t* data;
	iqmTransform_t rel[IQM_MAX_JOINTS];
	float M[IQM_MAX_JOINTS * 12];  // model space matrix of every joint (jiggled chain)
	int entryOf[IQM_MAX_JOINTS];
	int scaleJoints[MAX_RENDER_BONESCALES];
	vec3_t scaleValues[MAX_RENDER_BONESCALES];
	float W[12];  // model space -> world
	float maxSpeed;
	int numScaleJoints, numJoints, nframes, frame, oldframe, i, k, stepped = 0;

	if(!mod || mod->type != MOD_IQM) return 0;
	data = (iqmData_t*)mod->modelData;
	if(!data || (data->anim ? data->anim->num_poses : data->num_poses) <= 0) return 0;

	// swing speed cap (r_jiggleMaxSpeed, units/s; <= 0 disables it)
	maxSpeed = r_jiggleMaxSpeed->value;
	if(maxSpeed < 0.0f) maxSpeed = 0.0f;

	for(i = 0; i < IQM_MAX_JOINTS; i++) entryOf[i] = -1;
	for(i = 0; i < numEntries; i++) {
		entries[i].delta[0] = entries[i].delta[1] = entries[i].delta[2] = 0.0f;
		entries[i].delta[3] = 1.0f;
		if(entries[i].jointIndex >= 0 && entries[i].jointIndex < IQM_MAX_JOINTS) {
			entryOf[entries[i].jointIndex] = i;
		}
	}

	// the same animated pose the backend is going to skin with
	numScaleJoints = R_GetEntityBoneScales(sceneEntityNum, scaleJoints, scaleValues, MAX_RENDER_BONESCALES);
	nframes = data->anim ? data->anim->num_frames : data->num_frames;
	frame = nframes ? re->frame % nframes : 0;
	oldframe = nframes ? re->oldframe % nframes : 0;
	numJoints = IQM_BuildRelativeJoints(data, frame, oldframe, re->backlerp, rel, scaleJoints, scaleValues, numScaleJoints);

	for(i = 0; i < 3; i++) {
		W[i * 4 + 0] = re->axis[0][i];
		W[i * 4 + 1] = re->axis[1][i];
		W[i * 4 + 2] = re->axis[2][i];
		W[i * 4 + 3] = re->origin[i];
	}

	for(i = 0; i < numJoints; i++) {
		const int parent = data->jointParents[i];
		const bool haveParent = (parent >= 0 && parent < i);
		float* Mi = &M[i * 12];
		float local[12];

		JointToMatrix(rel[i].rotate, rel[i].scale, rel[i].translate, local);
		if(haveParent) {
			Matrix34Multiply(&M[parent * 12], local, Mi);
		} else {
			Com_Memcpy(Mi, local, sizeof(local));
		}

		if(entryOf[i] >= 0 && haveParent && entries[entryOf[i]].scale > 0.0f) {
			jiggleEntry_t* je = &entries[entryOf[i]];
			float MW[12], PW[12], PWinv[12];
			vec3_t tipBind, tipLocal, jointW, tipW0, d0, d, a, b;
			float s, L, len, ks, c, grav;
			quat_t q;

			if(!IQM_JiggleTip(data, i, tipBind)) continue;
			Jiggle_TransformPoint(&data->invBindJoints[i * 12], tipBind, tipLocal);

			// where the animation wants the tip, in world space
			Matrix34Multiply(W, Mi, MW);
			VectorSet(jointW, MW[3], MW[7], MW[11]);
			Jiggle_TransformPoint(MW, tipLocal, tipW0);
			VectorSubtract(tipW0, jointW, d0);
			L = VectorLength(d0);
			if(L < 0.05f) continue;  // collapsed (bone scale 0), nothing to simulate

			s = je->scale;
			if(s < JIGGLE_MIN_SCALE) s = JIGGLE_MIN_SCALE;
			if(s > JIGGLE_MAX_SCALE) s = JIGGLE_MAX_SCALE;
			// per joint overrides, 0 = global cvar default
			ks = (je->stiffness > 0.0f ? je->stiffness : r_jiggleStiffness->value) / s;
			c = (je->damping > 0.0f ? je->damping : r_jiggleDamping->value) / sqrtf(s);
			grav = (je->gravity != 0.0f ? je->gravity : r_jiggleGravity->value);

			if(!st->valid[i]) {
				VectorCopy(tipW0, st->pos[i]);
				VectorClear(st->vel[i]);
				st->valid[i] = 1;
				st->jointLastTime[i] = timeMs;
			}

			// Time tracking is per joint: copies of the same entity rendered
			// at the same refdef time do not step the joint twice, and the
			// order in which the copies are processed stops mattering.
			{
				int dtMs = timeMs - st->jointLastTime[i];
				float dt = 0.0f;

				if(dtMs < 0) {
					// time went backwards for this joint (demo seek, map
					// restart): start over at the animated position with no
					// velocity
					VectorCopy(tipW0, st->pos[i]);
					VectorClear(st->vel[i]);
					st->jointLastTime[i] = timeMs;
				} else if(dtMs > 0) {
					dt = Com_Clamp(0.0f, 0.1f, dtMs * 0.001f);
				}

				if(dt > 0.0f) {
					int steps = (int)ceilf(dt / JIGGLE_SUBSTEP);
					float h, damp;

					if(steps < 1) steps = 1;
					if(steps > JIGGLE_MAX_SUBSTEPS) steps = JIGGLE_MAX_SUBSTEPS;
					h = dt / steps;
					damp = expf(-c * h);

					for(k = 0; k < steps; k++) {
						vec3_t acc, before;

						// spring towards the animated tip + gravity
						VectorSubtract(tipW0, st->pos[i], acc);
						VectorScale(acc, ks, acc);
						acc[2] += grav;

						VectorMA(st->vel[i], h, acc, st->vel[i]);
						VectorScale(st->vel[i], damp, st->vel[i]);

						// cap the swing speed so an animation snap or teleport
						// cannot whip the tip through a huge arc in one substep
						if(maxSpeed > 0.0f) {
							float vlen = VectorLength(st->vel[i]);
							if(vlen > maxSpeed) VectorScale(st->vel[i], maxSpeed / vlen, st->vel[i]);
						}

						VectorCopy(st->pos[i], before);
						VectorMA(st->pos[i], h, st->vel[i], st->pos[i]);

						// rigid rod: keep the tip at the bone length from the joint
						VectorSubtract(st->pos[i], jointW, d);
						len = VectorLength(d);
						if(len > 1e-4f) {
							VectorMA(jointW, L / len, d, st->pos[i]);
						} else {
							VectorCopy(tipW0, st->pos[i]);
						}

						// velocity follows the position that was really reached
						VectorSubtract(st->pos[i], before, st->vel[i]);
						VectorScale(st->vel[i], 1.0f / h, st->vel[i]);
					}

					st->jointLastTime[i] = timeMs;
					stepped++;
				} else {
					// no time passed for this joint (same refdef time, second
					// copy of the entity, ...): only keep the rod length for
					// the current joint position
					VectorSubtract(st->pos[i], jointW, d);
					len = VectorLength(d);
					if(len > 1e-4f) VectorMA(jointW, L / len, d, st->pos[i]);
				}
			}

			if(Q_isnan(st->pos[i][0]) || Q_isnan(st->pos[i][1]) || Q_isnan(st->pos[i][2]) || Q_isnan(st->vel[i][0]) || Q_isnan(st->vel[i][1]) || Q_isnan(st->vel[i][2])) {
				VectorCopy(tipW0, st->pos[i]);
				VectorClear(st->vel[i]);
			}

			// aim the joint at the simulated tip. The rotation is expressed in
			// the parent space because that is where the local rotation lives
			VectorSubtract(st->pos[i], jointW, d);
			Matrix34Multiply(W, &M[parent * 12], PW);
			Matrix34Invert(PW, PWinv);
			Jiggle_TransformDir(PWinv, d0, a);
			Jiggle_TransformDir(PWinv, d, b);
			if(VectorNormalize(a) < 1e-6f || VectorNormalize(b) < 1e-6f) continue;

			Jiggle_QuatFromTo(a, b, JIGGLE_MAX_ANGLE, je->delta);

			// rebuild this joint with the jiggled rotation, its children use it
			IQM_QuatMul(je->delta, rel[i].rotate, q);
			QuatNormalize2(q, rel[i].rotate);
			JointToMatrix(rel[i].rotate, rel[i].scale, rel[i].translate, local);
			Matrix34Multiply(&M[parent * 12], local, Mi);
		}
	}

	return stepped;
}
// JIGGLE-END

/*
=================
R_IQMGetBoneScales

Collect the bone scale overrides recorded for the entity currently being
skinned (tag lookups deliberately never apply them).
=================
*/
static int R_IQMGetBoneScales(int* joints, vec3_t* scales, int maxCount) {
	int entityNum = (int)(backEnd.currentEntity - backEnd.refdef.entities);

	if(entityNum < 0 || entityNum >= MAX_REFENTITIES) {
		return 0;
	}

	return R_GetEntityBoneScales(entityNum, joints, scales, maxCount);
}

/*
=================
R_IQMGetJiggles

Collect the jiggle rotations the frontend simulated for the entity currently
being skinned (tag lookups never apply them).
=================
*/
static int R_IQMGetJiggles(int* joints, quat_t* deltas, int maxCount) {
	int entityNum = (int)(backEnd.currentEntity - backEndData->entities);

	if(entityNum < 0 || entityNum >= MAX_REFENTITIES) {
		return 0;
	}

	return R_GetEntityJiggles(entityNum, joints, deltas, maxCount);
}

/*
=================
RB_AddIQMSurfaces

Compute vertices for this model surface
=================
*/
void RB_IQMSurfaceAnim(surfaceType_t* surface) {
	srfIQModel_t* surf = (srfIQModel_t*)surface;
	iqmData_t* data = surf->data;
	float poseMats[IQM_MAX_JOINTS * 12];
	float influenceVtxMat[SHADER_MAX_VERTEXES * 12];
	float influenceNrmMat[SHADER_MAX_VERTEXES * 9];
	int i;

	int scaleJoints[MAX_RENDER_BONESCALES];
	vec3_t scaleValues[MAX_RENDER_BONESCALES];
	int numScaleJoints = R_IQMGetBoneScales(scaleJoints, scaleValues, MAX_RENDER_BONESCALES);

	int jiggleJoints[MAX_ENTITY_JIGGLES];
	quat_t jiggleDeltas[MAX_ENTITY_JIGGLES];
	int numJiggle = R_IQMGetJiggles(jiggleJoints, jiggleDeltas, MAX_ENTITY_JIGGLES);

	float* xyz;
	float* normal;
	float* tangent;
	float* texCoords;
	byte* color;
	vec4_t* outXYZ;
	int16_t* outNormal;
	int16_t* outTangent;
	vec2_t* outTexCoord;
	uint16_t* outColor;

	int nframes = data->anim ? data->anim->num_frames : data->num_frames;
	int frame = nframes ? backEnd.currentEntity->e.frame % nframes : 0;
	int oldframe = nframes ? backEnd.currentEntity->e.oldframe % nframes : 0;
	float backlerp = backEnd.currentEntity->e.backlerp;

	int* tri;
	glIndex_t* ptr;
	glIndex_t base;

	RB_CHECKOVERFLOW(surf->num_vertexes, surf->num_triangles * 3);

	xyz = &data->positions[surf->first_vertex * 3];
	normal = &data->normals[surf->first_vertex * 3];
	tangent = &data->tangents[surf->first_vertex * 4];
	texCoords = &data->texcoords[surf->first_vertex * 2];

	if(data->colors) {
		color = &data->colors[surf->first_vertex * 4];
	} else {
		color = NULL;
	}

	outXYZ = &tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];
	outTangent = tess.tangent[tess.numVertexes];
	outTexCoord = &tess.texCoords[tess.numVertexes];
	outColor = tess.color[tess.numVertexes];

	if((data->anim ? data->anim->num_poses : data->num_poses) > 0) {
		// compute interpolated joint matrices
		ComputePoseMats(data, frame, oldframe, backlerp, poseMats, scaleJoints, scaleValues, numScaleJoints, jiggleJoints, jiggleDeltas, numJiggle);

		// compute vertex blend influence matricies
		for(i = 0; i < surf->num_influences; i++) {
			int influence = surf->first_influence + i;
			float* vtxMat = &influenceVtxMat[12 * i];
			float* nrmMat = &influenceNrmMat[9 * i];
			int j;
			float blendWeights[4];

			if(data->blendWeightsType == IQM_FLOAT) {
				blendWeights[0] = data->influenceBlendWeights.f[4 * influence + 0];
				blendWeights[1] = data->influenceBlendWeights.f[4 * influence + 1];
				blendWeights[2] = data->influenceBlendWeights.f[4 * influence + 2];
				blendWeights[3] = data->influenceBlendWeights.f[4 * influence + 3];
			} else {
				blendWeights[0] = (float)data->influenceBlendWeights.b[4 * influence + 0] / 255.0f;
				blendWeights[1] = (float)data->influenceBlendWeights.b[4 * influence + 1] / 255.0f;
				blendWeights[2] = (float)data->influenceBlendWeights.b[4 * influence + 2] / 255.0f;
				blendWeights[3] = (float)data->influenceBlendWeights.b[4 * influence + 3] / 255.0f;
			}

			if(blendWeights[0] <= 0.0f) {
				// no blend joint, use identity matrix.
				vtxMat[0] = identityMatrix[0];
				vtxMat[1] = identityMatrix[1];
				vtxMat[2] = identityMatrix[2];
				vtxMat[3] = identityMatrix[3];
				vtxMat[4] = identityMatrix[4];
				vtxMat[5] = identityMatrix[5];
				vtxMat[6] = identityMatrix[6];
				vtxMat[7] = identityMatrix[7];
				vtxMat[8] = identityMatrix[8];
				vtxMat[9] = identityMatrix[9];
				vtxMat[10] = identityMatrix[10];
				vtxMat[11] = identityMatrix[11];
			} else {
				// compute the vertex matrix by blending the up to
				// four blend weights
				vtxMat[0] = blendWeights[0] * poseMats[12 * data->influenceBlendIndexes[4 * influence + 0] + 0];
				vtxMat[1] = blendWeights[0] * poseMats[12 * data->influenceBlendIndexes[4 * influence + 0] + 1];
				vtxMat[2] = blendWeights[0] * poseMats[12 * data->influenceBlendIndexes[4 * influence + 0] + 2];
				vtxMat[3] = blendWeights[0] * poseMats[12 * data->influenceBlendIndexes[4 * influence + 0] + 3];
				vtxMat[4] = blendWeights[0] * poseMats[12 * data->influenceBlendIndexes[4 * influence + 0] + 4];
				vtxMat[5] = blendWeights[0] * poseMats[12 * data->influenceBlendIndexes[4 * influence + 0] + 5];
				vtxMat[6] = blendWeights[0] * poseMats[12 * data->influenceBlendIndexes[4 * influence + 0] + 6];
				vtxMat[7] = blendWeights[0] * poseMats[12 * data->influenceBlendIndexes[4 * influence + 0] + 7];
				vtxMat[8] = blendWeights[0] * poseMats[12 * data->influenceBlendIndexes[4 * influence + 0] + 8];
				vtxMat[9] = blendWeights[0] * poseMats[12 * data->influenceBlendIndexes[4 * influence + 0] + 9];
				vtxMat[10] = blendWeights[0] * poseMats[12 * data->influenceBlendIndexes[4 * influence + 0] + 10];
				vtxMat[11] = blendWeights[0] * poseMats[12 * data->influenceBlendIndexes[4 * influence + 0] + 11];

				for(j = 1; j < 4; j++) {
					if(blendWeights[j] <= 0.0f) {
						break;
					}

					vtxMat[0] += blendWeights[j] * poseMats[12 * data->influenceBlendIndexes[4 * influence + j] + 0];
					vtxMat[1] += blendWeights[j] * poseMats[12 * data->influenceBlendIndexes[4 * influence + j] + 1];
					vtxMat[2] += blendWeights[j] * poseMats[12 * data->influenceBlendIndexes[4 * influence + j] + 2];
					vtxMat[3] += blendWeights[j] * poseMats[12 * data->influenceBlendIndexes[4 * influence + j] + 3];
					vtxMat[4] += blendWeights[j] * poseMats[12 * data->influenceBlendIndexes[4 * influence + j] + 4];
					vtxMat[5] += blendWeights[j] * poseMats[12 * data->influenceBlendIndexes[4 * influence + j] + 5];
					vtxMat[6] += blendWeights[j] * poseMats[12 * data->influenceBlendIndexes[4 * influence + j] + 6];
					vtxMat[7] += blendWeights[j] * poseMats[12 * data->influenceBlendIndexes[4 * influence + j] + 7];
					vtxMat[8] += blendWeights[j] * poseMats[12 * data->influenceBlendIndexes[4 * influence + j] + 8];
					vtxMat[9] += blendWeights[j] * poseMats[12 * data->influenceBlendIndexes[4 * influence + j] + 9];
					vtxMat[10] += blendWeights[j] * poseMats[12 * data->influenceBlendIndexes[4 * influence + j] + 10];
					vtxMat[11] += blendWeights[j] * poseMats[12 * data->influenceBlendIndexes[4 * influence + j] + 11];
				}
			}

			// compute the normal matrix as transpose of the adjoint
			// of the vertex matrix
			nrmMat[0] = vtxMat[5] * vtxMat[10] - vtxMat[6] * vtxMat[9];
			nrmMat[1] = vtxMat[6] * vtxMat[8] - vtxMat[4] * vtxMat[10];
			nrmMat[2] = vtxMat[4] * vtxMat[9] - vtxMat[5] * vtxMat[8];
			nrmMat[3] = vtxMat[2] * vtxMat[9] - vtxMat[1] * vtxMat[10];
			nrmMat[4] = vtxMat[0] * vtxMat[10] - vtxMat[2] * vtxMat[8];
			nrmMat[5] = vtxMat[1] * vtxMat[8] - vtxMat[0] * vtxMat[9];
			nrmMat[6] = vtxMat[1] * vtxMat[6] - vtxMat[2] * vtxMat[5];
			nrmMat[7] = vtxMat[2] * vtxMat[4] - vtxMat[0] * vtxMat[6];
			nrmMat[8] = vtxMat[0] * vtxMat[5] - vtxMat[1] * vtxMat[4];
		}

		// transform vertexes and fill other data
		for(i = 0; i < surf->num_vertexes; i++, xyz += 3, normal += 3, tangent += 4, texCoords += 2, outXYZ++, outNormal += 4, outTangent += 4, outTexCoord++) {
			int influence = data->influences[surf->first_vertex + i] - surf->first_influence;
			float* vtxMat = &influenceVtxMat[12 * influence];
			float* nrmMat = &influenceNrmMat[9 * influence];

			(*outTexCoord)[0] = texCoords[0];
			(*outTexCoord)[1] = texCoords[1];

			(*outXYZ)[0] = vtxMat[0] * xyz[0] + vtxMat[1] * xyz[1] + vtxMat[2] * xyz[2] + vtxMat[3];
			(*outXYZ)[1] = vtxMat[4] * xyz[0] + vtxMat[5] * xyz[1] + vtxMat[6] * xyz[2] + vtxMat[7];
			(*outXYZ)[2] = vtxMat[8] * xyz[0] + vtxMat[9] * xyz[1] + vtxMat[10] * xyz[2] + vtxMat[11];

			{
				vec3_t unpackedNormal;
				vec4_t unpackedTangent;

				unpackedNormal[0] = DotProduct(&nrmMat[0], normal);
				unpackedNormal[1] = DotProduct(&nrmMat[3], normal);
				unpackedNormal[2] = DotProduct(&nrmMat[6], normal);

				R_VaoPackNormal(outNormal, unpackedNormal);

				unpackedTangent[0] = DotProduct(&nrmMat[0], tangent);
				unpackedTangent[1] = DotProduct(&nrmMat[3], tangent);
				unpackedTangent[2] = DotProduct(&nrmMat[6], tangent);
				unpackedTangent[3] = tangent[3];

				R_VaoPackTangent(outTangent, unpackedTangent);
			}
		}
	} else {
		// copy vertexes and fill other data
		for(i = 0; i < surf->num_vertexes; i++, xyz += 3, normal += 3, tangent += 4, texCoords += 2, outXYZ++, outNormal += 4, outTangent += 4, outTexCoord++) {
			(*outTexCoord)[0] = texCoords[0];
			(*outTexCoord)[1] = texCoords[1];

			(*outXYZ)[0] = xyz[0];
			(*outXYZ)[1] = xyz[1];
			(*outXYZ)[2] = xyz[2];

			R_VaoPackNormal(outNormal, normal);
			R_VaoPackTangent(outTangent, tangent);
		}
	}

	if(color) {
		for(i = 0; i < surf->num_vertexes; i++, color += 4, outColor += 4) {
			outColor[0] = color[0] * 257;
			outColor[1] = color[1] * 257;
			outColor[2] = color[2] * 257;
			outColor[3] = color[3] * 257;
		}
	} else {
		for(i = 0; i < surf->num_vertexes; i++, outColor += 4) {
			outColor[0] = 0;
			outColor[1] = 0;
			outColor[2] = 0;
			outColor[3] = 0;
		}
	}

	tri = data->triangles + 3 * surf->first_triangle;
	ptr = &tess.indexes[tess.numIndexes];
	base = tess.numVertexes;

	for(i = 0; i < surf->num_triangles; i++) {
		*ptr++ = base + (*tri++ - surf->first_vertex);
		*ptr++ = base + (*tri++ - surf->first_vertex);
		*ptr++ = base + (*tri++ - surf->first_vertex);
	}

	tess.numIndexes += 3 * surf->num_triangles;
	tess.numVertexes += surf->num_vertexes;
}

/*
=================
RB_IQMSurfaceAnimVao
=================
*/
void RB_IQMSurfaceAnimVao(srfVaoIQModel_t* surface) {
	iqmData_t* data = surface->iqmData;
	int activePoses;
	int i;

	int scaleJoints[MAX_RENDER_BONESCALES];
	vec3_t scaleValues[MAX_RENDER_BONESCALES];
	int numScaleJoints;

	int jiggleJoints[MAX_ENTITY_JIGGLES];
	quat_t jiggleDeltas[MAX_ENTITY_JIGGLES];
	int numJiggle;

	if(ShaderRequiresCPUDeforms(tess.shader)) {
		RB_IQMSurfaceAnim((surfaceType_t*)surface->iqmSurface);
		return;
	}

	if(!surface->vao) return;

	// RB_CheckVao(surface->vao);
	RB_EndSurface();
	RB_BeginSurface(tess.shader, tess.fogNum, tess.useCubemap, tess.cubemapOrigin, tess.cubemapBound);

	R_BindVao(surface->vao);

	tess.useInternalVao = false;

	tess.numIndexes = surface->numIndexes;
	tess.numVertexes = surface->numVerts;

	activePoses = data->anim ? data->anim->num_poses : data->num_poses;
	glState.boneAnimation = 0;

	numScaleJoints = R_IQMGetBoneScales(scaleJoints, scaleValues, MAX_RENDER_BONESCALES);
	numJiggle = R_IQMGetJiggles(jiggleJoints, jiggleDeltas, MAX_ENTITY_JIGGLES);

	if(activePoses) {
		float jointMats[IQM_MAX_JOINTS * 12];
		int nframes = data->anim ? data->anim->num_frames : data->num_frames;
		int frame = nframes ? backEnd.currentEntity->e.frame % nframes : 0;
		int oldframe = nframes ? backEnd.currentEntity->e.oldframe % nframes : 0;
		float backlerp = backEnd.currentEntity->e.backlerp;

		ComputePoseMats(surface->iqmData, frame, oldframe, backlerp, jointMats, scaleJoints, scaleValues, numScaleJoints, jiggleJoints, jiggleDeltas, numJiggle);

		for(i = 0; i < data->num_joints; i++) {
			glState.boneMatrix[i][0] = jointMats[i * 12 + 0];
			glState.boneMatrix[i][1] = jointMats[i * 12 + 4];
			glState.boneMatrix[i][2] = jointMats[i * 12 + 8];
			glState.boneMatrix[i][3] = 0.0f;
			glState.boneMatrix[i][4] = jointMats[i * 12 + 1];
			glState.boneMatrix[i][5] = jointMats[i * 12 + 5];
			glState.boneMatrix[i][6] = jointMats[i * 12 + 9];
			glState.boneMatrix[i][7] = 0.0f;
			glState.boneMatrix[i][8] = jointMats[i * 12 + 2];
			glState.boneMatrix[i][9] = jointMats[i * 12 + 6];
			glState.boneMatrix[i][10] = jointMats[i * 12 + 10];
			glState.boneMatrix[i][11] = 0.0f;
			glState.boneMatrix[i][12] = jointMats[i * 12 + 3];
			glState.boneMatrix[i][13] = jointMats[i * 12 + 7];
			glState.boneMatrix[i][14] = jointMats[i * 12 + 11];
			glState.boneMatrix[i][15] = 1.0f;
		}

		// Upload ALL model joints to u_BoneMatrix, not just the animation
		// poses. attr_BoneIndexes for finger vertices references joints
		// 56..71; without their matrices the fingers collapse to origin.
		glState.boneAnimation = data->num_joints;
	}

	RB_EndSurface();

	glState.boneAnimation = 0;
}

bool R_IQMLerpTag(orientation_t* tag, iqmData_t* data, int startFrame, int endFrame, float frac, const char* tagName) {
	float jointMats[IQM_MAX_JOINTS * 12];
	int joint;
	const char* names;
	int nframes;

	if(!tag || !data || !tagName || !tagName[0]) {
		AxisClear(tag->axis);
		VectorClear(tag->origin);
		return false;
	}

	if(!data->jointNames || data->num_joints <= 0) {
		ri.Printf(PRINT_DEVELOPER, "R_IQMLerpTag: model has no joints (num_joints=%d), tag '%s' not available\n", data->num_joints, tagName);
		AxisClear(tag->axis);
		VectorClear(tag->origin);
		return false;
	}

	/* Clamp frames - callers routinely pass MD3-style frames that are
	   out of range for an IQM or use -1 for "current". */
	nframes = data->anim ? data->anim->num_frames : data->num_frames;
	if(nframes > 0) {
		if(startFrame < 0) startFrame = 0;
		if(startFrame >= nframes) startFrame = nframes - 1;
		if(endFrame < 0) endFrame = 0;
		if(endFrame >= nframes) endFrame = nframes - 1;
	} else {
		startFrame = 0;
		endFrame = 0;
	}
	if(frac < 0.0f) frac = 0.0f;
	if(frac > 1.0f) frac = 1.0f;

	/* ---- lookup pass 1: exact match ---- */
	names = data->jointNames;
	for(joint = 0; joint < data->num_joints; joint++) {
		if(!strcmp(tagName, names)) break;
		names += strlen(names) + 1;
	}

	/* ---- lookup pass 2: case-insensitive ---- */
	if(joint >= data->num_joints) {
		names = data->jointNames;
		for(joint = 0; joint < data->num_joints; joint++) {
			if(!Q_stricmp(tagName, names)) break;
			names += strlen(names) + 1;
		}
	}

	/* ---- lookup pass 3: strip a leading "tag_" prefix and retry case-insensitive ---- */
	if(joint >= data->num_joints && !Q_stricmpn(tagName, "tag_", 4)) {
		const char* bare = tagName + 4;
		names = data->jointNames;
		for(joint = 0; joint < data->num_joints; joint++) {
			if(!Q_stricmp(bare, names)) break;
			names += strlen(names) + 1;
		}
	}

	/* ---- lookup pass 4: match ignoring spaces / underscores ---- */
	if(joint >= data->num_joints) {
		names = data->jointNames;
		for(joint = 0; joint < data->num_joints; joint++) {
			const char* a = tagName;
			const char* b = names;
			while(*a && *b) {
				while(*a == ' ' || *a == '_' || *a == '.') a++;
				while(*b == ' ' || *b == '_' || *b == '.') b++;
				if(tolower((unsigned char)*a) != tolower((unsigned char)*b)) break;
				a++;
				b++;
			}
			/* both strings exhausted -> match */
			while(*a == ' ' || *a == '_' || *a == '.') a++;
			while(*b == ' ' || *b == '_' || *b == '.') b++;
			if(!*a && !*b) break;
			names += strlen(names) + 1;
		}
	}

	if(joint >= data->num_joints) {
		ri.Printf(PRINT_DEVELOPER, "R_IQMLerpTag: joint '%s' not found (%d joints)\n", tagName, data->num_joints);
		AxisClear(tag->axis);
		VectorClear(tag->origin);
		return false;
	}

	ComputeJointMats(data, startFrame, endFrame, frac, jointMats);

	/* Layout mirrors what MD3 code returns through orientation_t:
	   axis[i][j] = row i, column j of the model-space rotation,
	   origin    = model-space translation. */
	tag->axis[0][0] = jointMats[12 * joint + 0];
	tag->axis[1][0] = jointMats[12 * joint + 1];
	tag->axis[2][0] = jointMats[12 * joint + 2];
	tag->origin[0] = jointMats[12 * joint + 3];

	tag->axis[0][1] = jointMats[12 * joint + 4];
	tag->axis[1][1] = jointMats[12 * joint + 5];
	tag->axis[2][1] = jointMats[12 * joint + 6];
	tag->origin[1] = jointMats[12 * joint + 7];

	tag->axis[0][2] = jointMats[12 * joint + 8];
	tag->axis[1][2] = jointMats[12 * joint + 9];
	tag->axis[2][2] = jointMats[12 * joint + 10];
	tag->origin[2] = jointMats[12 * joint + 11];

	return true;
}
