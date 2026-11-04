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
// cg_players.c -- handle the media and animation for player entities
#include "../qcommon/vm_javascript.h"

/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
======================
CG_AnimationIndexFromName

Animation files use named IQM clips. Movement clips may optionally carry a
weapon suffix, e.g. AM_RUN_N_SMG1. Unsuffixed movement clips are the ALL set.
======================
*/
static int CG_AnimationIndexFromName(const char* name) {
	static const char* generalNames[] = {"AM_DEATH1", "AM_DEATH2", "AM_DEATH3", "AM_DEATH4", "AM_GESTURE_AGREE", "AM_GESTURE_DISAGREE", "AM_GESTURE_BECON", "AM_GESTURE_BOW", "AM_GESTURE_SALUTE", "AM_GESTURE_WAVE", "AM_TAUNT_CHEER", "AM_TAUNT_DANCE", "AM_TAUNT_LAUGH", "AM_TAUNT_MUSCLE", "AM_TAUNT_PERSISTENCE", "AM_TAUNT_ROBOT", "AM_TAUNT_ZOMBIE", "AM_MELEEATTACK", "AM_THROW"};
	static const char* moveNames[AM_MOVE_ANIMATION_COUNT] = {"AM_IDLE", "AM_SIT", "AM_WALK_E", "AM_WALK_N", "AM_WALK_NE", "AM_WALK_NW", "AM_WALK_S", "AM_WALK_SE", "AM_WALK_SW", "AM_WALK_W", "AM_RUN_E", "AM_RUN_N", "AM_RUN_NE", "AM_RUN_NW", "AM_RUN_S", "AM_RUN_SE", "AM_RUN_SW", "AM_RUN_W", "AM_CRONCHIDLE", "AM_CRONCH_E", "AM_CRONCH_N", "AM_CRONCH_NE", "AM_CRONCH_NW", "AM_CRONCH_S", "AM_CRONCH_SE", "AM_CRONCH_SW", "AM_CRONCH_W", "AM_SWIM_E", "AM_SWIM_N", "AM_SWIM_NE", "AM_SWIM_NW", "AM_SWIM_S", "AM_SWIM_SE", "AM_SWIM_SW", "AM_SWIM_W", "AM_JUMP"};
	static const char* weaponNames[AMTYPE_COUNT] = {"ALL", "AR2", "CAMERA", "CROSSBOW", "DUAL", "FIST", "GRENADE", "KNIFE", "MAGIC", "MELEE", "MELEE2", "PASSIVE", "PHYSGUN", "PISTOL", "REVOLVER", "RPG", "SHOTGUN", "SLAM", "SMG1"};
	int i, j;
	char suffixed[MAX_QPATH];

	for(i = 0; i < ARRAY_LEN(generalNames); i++) {
		if(!Q_stricmp(name, generalNames[i])) {
			return i;
		}
	}

	for(i = 0; i < AM_MOVE_ANIMATION_COUNT; i++) {
		if(!Q_stricmp(name, moveNames[i])) {
			return AM_IDLE + i;
		}
		for(j = 0; j < AMTYPE_COUNT; j++) {
			Com_sprintf(suffixed, sizeof(suffixed), "%s_%s", moveNames[i], weaponNames[j]);
			if(!Q_stricmp(name, suffixed)) {
				return AM_IDLE + i + AM_MOVE_ANIMATION_COUNT * j;
			}
		}
	}

	return -1;
}

/*
======================
CG_ParseAnimationFile

The IQM model is a single skeletal mesh. animation.cfg maps named clips to
frame ranges in that model. Each non-empty row is:
NAME FIRST_FRAME NUM_FRAMES LOOP_FRAMES FPS
======================
*/
static bool CG_ParseAnimationFile(const char* filename, clientInfo_t* ci, char* animFile, int animFileSize) {
	char text[262144];
	char* text_p;
	char* token;
	fileHandle_t f;
	int len, index, i;
	float fps;
	bool defined[MAX_TOTALANIMATIONS];
	animation_t fallback;

	memset(ci->animations, 0, sizeof(ci->animations));
	memset(defined, 0, sizeof(defined));
	ci->headUp = 0.0f;
	ci->headFwd = 0.0f;
	ci->weaponScale = 1.0f;

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(len <= 0) {
		return false;
	}
	if(len >= sizeof(text) - 1) {
		CG_Printf("Animation file %s is too large\n", filename);
		trap_FS_FCloseFile(f);
		return false;
	}
	trap_FS_Read(text, len, f);
	text[len] = '\0';
	trap_FS_FCloseFile(f);

	text_p = text;
	while(1) {
		token = COM_Parse(&text_p);
		if(!token[0]) {
			break;
		}

		if(!Q_stricmp(token, "animFile")) {
			token = COM_Parse(&text_p);
			if(!token[0]) {
				CG_Printf("Missing value for animFile in %s\n", filename);
				return false;
			}
			Q_strncpyz(animFile, token, animFileSize);
			continue;
		}

		if(!Q_stricmp(token, "headsettings")) {
			// first-person camera fine-tuning for this model:
			// headSettings up fwd (same semantics as cg_firstPerson_Up / _Fwd)
			token = COM_Parse(&text_p);
			if(!token[0]) {
				CG_Printf("Missing up value for headSettings in %s\n", filename);
				return false;
			}
			ci->headUp = atof(token);

			token = COM_Parse(&text_p);
			if(!token[0]) {
				CG_Printf("Missing fwd value for headSettings in %s\n", filename);
				return false;
			}
			ci->headFwd = atof(token);
			continue;
		}

		if(!Q_stricmp(token, "weaponScale")) {
			// per-model size of the weapon held in the hand (1 = full size,
			// as authored for the standard valveBiped hand)
			token = COM_Parse(&text_p);
			if(!token[0]) {
				CG_Printf("Missing value for weaponScale in %s\n", filename);
				return false;
			}
			ci->weaponScale = atof(token);
			if(ci->weaponScale <= 0.0f) {
				CG_Printf("Invalid weaponScale %s in %s, using 1\n", token, filename);
				ci->weaponScale = 1.0f;
			}
			continue;
		}

		index = CG_AnimationIndexFromName(token);
		if(index < 0 || index >= MAX_TOTALANIMATIONS) {
			CG_Printf("Unknown IQM animation '%s' in %s\n", token, filename);
			return false;
		}

		token = COM_Parse(&text_p);
		if(!token[0]) return false;
		ci->animations[index].firstFrame = atoi(token);
		if(ci->animations[index].firstFrame < 0) {
			CG_Printf("Negative first frame for %s in %s\n", token, filename);
			return false;
		}

		token = COM_Parse(&text_p);
		if(!token[0]) return false;
		ci->animations[index].numFrames = atoi(token);
		ci->animations[index].reversed = (ci->animations[index].numFrames < 0);
		if(ci->animations[index].numFrames < 0) {
			ci->animations[index].numFrames = -ci->animations[index].numFrames;
		}
		if(ci->animations[index].numFrames <= 0) {
			CG_Printf("Invalid frame count for animation %s in %s\n", token, filename);
			return false;
		}

		token = COM_Parse(&text_p);
		if(!token[0]) return false;
		ci->animations[index].loopFrames = atoi(token);
		if(ci->animations[index].loopFrames < 0 || ci->animations[index].loopFrames > ci->animations[index].numFrames) {
			CG_Printf("Invalid loop frame count for animation %s in %s\n", token, filename);
			return false;
		}

		token = COM_Parse(&text_p);
		if(!token[0]) return false;
		fps = (float)atof(token);
		if(fps <= 0.0f) {
			CG_Printf("Invalid FPS for animation %s in %s\n", token, filename);
			return false;
		}
		ci->animations[index].frameLerp = (int)(1000.0f / fps);
		if(ci->animations[index].frameLerp < 1) ci->animations[index].frameLerp = 1;
		ci->animations[index].initialLerp = ci->animations[index].frameLerp;
		defined[index] = true;
	}

	if(!defined[AM_IDLE]) {
		CG_Printf("%s must define AM_IDLE\n", filename);
		return false;
	}

	fallback = ci->animations[AM_IDLE];
	for(i = 0; i < MAX_TOTALANIMATIONS; i++) {
		if(defined[i]) continue;
		if(i >= AM_IDLE) {
			int allIndex = AM_IDLE + ((i - AM_IDLE) % AM_MOVE_ANIMATION_COUNT);
			ci->animations[i] = defined[allIndex] ? ci->animations[allIndex] : fallback;
		} else {
			ci->animations[i] = fallback;
		}
	}

	return true;
}

/*
==========================
CG_FileExists
==========================
*/
static bool CG_FileExists(const char* filename) { return trap_FS_FOpenFile(filename, NULL, FS_READ) > 0; }

/*
==========================
CG_FindIQMSkinFile

A skin is a normal renderer skin remap file with one entry per IQM surface.
==========================
*/
static bool CG_FindIQMSkinFile(char* filename, int length, const char* modelName, const char* skinName) {
	Com_sprintf(filename, length, "models/players/%s/%s.skin", modelName, skinName);
	if(CG_FileExists(filename)) return true;

	Com_sprintf(filename, length, "models/players/characters/%s/%s.skin", modelName, skinName);
	if(CG_FileExists(filename)) return true;

	Com_sprintf(filename, length, "models/players/%s/default.skin", modelName);
	if(CG_FileExists(filename)) return true;

	Com_sprintf(filename, length, "models/players/characters/%s/default.skin", modelName);
	return CG_FileExists(filename);
}

/*
==========================
CG_RegisterClientModelname

Loads one IQM model, one skin remap and its animation map.
==========================
*/
static bool CG_RegisterClientModelname(clientInfo_t* ci, const char* modelName, const char* skinName) {
	char filename[MAX_QPATH];
	char animFile[MAX_QPATH];
	bool foundModel;

	animFile[0] = '\0';

	Com_sprintf(filename, sizeof(filename), "models/players/%s/animation.cfg", modelName);
	if(!CG_ParseAnimationFile(filename, ci, animFile, sizeof(animFile))) {
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/animation.cfg", modelName);
		if(!CG_ParseAnimationFile(filename, ci, animFile, sizeof(animFile))) {
			Com_Printf("Failed to load IQM animation map %s\n", filename);
			return false;
		}
	}

	if(!animFile[0]) Q_strncpyz(animFile, "models/m_anm.iqm", sizeof(animFile));
	Q_strncpyz(ci->animFile, animFile, sizeof(ci->animFile));

	Com_sprintf(filename, sizeof(filename), "models/players/%s/pmodel", modelName);
	ci->model = trap_R_RegisterModelEx(filename, animFile);
	foundModel = (ci->model != 0);
	if(!foundModel) {
		Com_Printf("Failed to load IQM player model: models/players/%s/pmodel\n", modelName);
		return false;
	}

	ci->modelSkin = 0;
	if(CG_FindIQMSkinFile(filename, sizeof(filename), modelName, skinName)) {
		ci->modelSkin = trap_R_RegisterSkin(filename);
		if(!ci->modelSkin) {
			Com_Printf("Failed to load IQM skin %s\n", filename);
		}
	}

	ci->modelIcon = 0;
	Com_sprintf(filename, sizeof(filename), "models/players/%s/icon_%s.tga", modelName, skinName);
	ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
	if(!ci->modelIcon) {
		Com_sprintf(filename, sizeof(filename), "models/players/%s/icon_default.tga", modelName);
		ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
	}
	if(!ci->modelIcon) {
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/icon_%s.tga", modelName, skinName);
		ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
	}
	if(!ci->modelIcon) {
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/icon_default.tga", modelName);
		ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
	}

	return true;
}

/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString(const char* v, vec3_t color) {
	int val;

	VectorClear(color);

	val = atoi(v);

	if(val < 1 || val > 7) {
		VectorSet(color, 1, 1, 1);
		return;
	}

	if(val & 1) {
		color[2] = 1.0f;
	}
	if(val & 2) {
		color[1] = 1.0f;
	}
	if(val & 4) {
		color[0] = 1.0f;
	}
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo(int clientNum, clientInfo_t* ci) {
	int i;

	if(!CG_RegisterClientModelname(ci, ci->modelName, ci->skinName)) {
		if(cg_buildScript.integer) {
			CG_Error("CG_RegisterClientModelname( %s, %s ) failed", ci->modelName, ci->skinName);
		}
		if(!CG_RegisterClientModelname(ci, DEFAULT_MODEL, "default")) {
			CG_Error("DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL);
		}
	}

	ci->deferred = false;
	for(i = 0; i < MAX_GENTITIES; i++) {
		if(cg_entities[i].currentState.clientNum == clientNum && cg_entities[i].currentState.eType == ET_PLAYER) {
			CG_ResetPlayerEntity(&cg_entities[i]);
		}
	}
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel(clientInfo_t* from, clientInfo_t* to) {
	to->gender = from->gender;
	to->model = from->model;
	to->modelSkin = from->modelSkin;
	to->modelIcon = from->modelIcon;
	Q_strncpyz(to->animFile, from->animFile, sizeof(to->animFile));
	memcpy(to->animations, from->animations, sizeof(to->animations));
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static bool CG_ScanForExistingClientInfo(clientInfo_t* ci) {
	int i;
	clientInfo_t* match;

	for(i = 0; i < cgs.maxclients; i++) {
		match = &cgs.clientinfo[i];
		if(!match->infoValid) {
			continue;
		}
		if(match->deferred) {
			continue;
		}
		if(!Q_stricmp(ci->modelName, match->modelName) && !Q_stricmp(ci->skinName, match->skinName) && !Q_stricmp(ci->blueTeam, match->blueTeam) && !Q_stricmp(ci->redTeam, match->redTeam)) {
			// this clientinfo is identical, so use its handles

			ci->deferred = false;

			CG_CopyClientInfoModel(match, ci);

			return true;
		}
	}

	// nothing matches, so defer the load
	return false;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo(int clientNum, clientInfo_t* ci) {
	int i;
	clientInfo_t* match;

	// if someone else is already the same models and skins we
	// can just load the client info
	for(i = 0; i < cgs.maxclients; i++) {
		match = &cgs.clientinfo[i];
		if(!match->infoValid || match->deferred) {
			continue;
		}
		if(Q_stricmp(ci->skinName, match->skinName) || Q_stricmp(ci->modelName, match->modelName)) {
			continue;
		}
		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo(clientNum, ci);
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for(i = 0; i < cgs.maxclients; i++) {
		match = &cgs.clientinfo[i];
		if(!match->infoValid) {
			continue;
		}

		ci->deferred = true;
		CG_CopyClientInfoModel(match, ci);
		return;
	}

	// we should never get here...
	CG_Printf("CG_SetDeferredClientInfo: no valid clients!\n");

	CG_LoadClientInfo(clientNum, ci);
}

/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo(int clientNum) {
	clientInfo_t* ci;
	clientInfo_t newInfo;
	const char* configstring;
	const char* v;
	char* slash;

	ci = &cgs.clientinfo[clientNum];

	configstring = CG_ConfigString(clientNum + CS_PLAYERS);
	if(!configstring[0]) {
		memset(ci, 0, sizeof(*ci));
		return;  // player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset(&newInfo, 0, sizeof(newInfo));

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz(newInfo.name, v, sizeof(newInfo.name));

	// colors
	v = Info_ValueForKey(configstring, "c1");
	CG_ColorFromString(v, newInfo.color1);

	newInfo.c1RGBA[0] = 255 * newInfo.color1[0];
	newInfo.c1RGBA[1] = 255 * newInfo.color1[1];
	newInfo.c1RGBA[2] = 255 * newInfo.color1[2];
	newInfo.c1RGBA[3] = 255;

	v = Info_ValueForKey(configstring, "c2");
	CG_ColorFromString(v, newInfo.color2);

	newInfo.c2RGBA[0] = 255 * newInfo.color2[0];
	newInfo.c2RGBA[1] = 255 * newInfo.color2[1];
	newInfo.c2RGBA[2] = 255 * newInfo.color2[2];
	newInfo.c2RGBA[3] = 255;

	v = Info_ValueForKey(configstring, "pr");
	newInfo.physR = atoi(v);

	v = Info_ValueForKey(configstring, "pg");
	newInfo.physG = atoi(v);

	v = Info_ValueForKey(configstring, "pb");
	newInfo.physB = atoi(v);

	// model
	v = Info_ValueForKey(configstring, "model");
	if(cg_forceModel.integer) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char* skin;

		trap_Cvar_VariableStringBuffer("model", modelStr, sizeof(modelStr));
		if((skin = strchr(modelStr, '/')) == NULL) {
			skin = "default";
		} else {
			*skin++ = 0;
		}

		Q_strncpyz(newInfo.skinName, skin, sizeof(newInfo.skinName));
		Q_strncpyz(newInfo.modelName, modelStr, sizeof(newInfo.modelName));

	} else {
		Q_strncpyz(newInfo.modelName, v, sizeof(newInfo.modelName));

		slash = strchr(newInfo.modelName, '/');
		if(!slash) {
			// modelName didn not include a skin name
			Q_strncpyz(newInfo.skinName, "default", sizeof(newInfo.skinName));
		} else {
			Q_strncpyz(newInfo.skinName, slash + 1, sizeof(newInfo.skinName));
			// truncate modelName
			*slash = 0;
		}
	}

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if(!CG_ScanForExistingClientInfo(&newInfo)) {
		bool forceDefer;

		forceDefer = trap_MemoryRemaining() < 4000000;

		// if we are defering loads, just have it pick the first valid
		if(forceDefer || (cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading)) {
			// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo(clientNum, &newInfo);
			// if we are low on memory, leave them with this model
			if(forceDefer) {
				CG_Printf("Memory is low. Using deferred model.\n");
				newInfo.deferred = false;
			}
		} else {
			CG_LoadClientInfo(clientNum, &newInfo);
		}
	}

	// replace whatever was there with the new one
	newInfo.infoValid = true;
	*ci = newInfo;
}

/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers(void) {
	int i;
	clientInfo_t* ci;

	// scan for a deferred player to load
	for(i = 0, ci = cgs.clientinfo; i < cgs.maxclients; i++, ci++) {
		if(ci->infoValid && ci->deferred) {
			// if we are low on memory, leave it deferred
			if(trap_MemoryRemaining() < 4000000) {
				CG_Printf("Memory is low. Using deferred model.\n");
				ci->deferred = false;
				continue;
			}
			CG_LoadClientInfo(i, ci);
			//			break;
		}
	}
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/

/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation(clientInfo_t* ci, lerpFrame_t* lf, int newAnimation) {
	animation_t* anim;

#if ANIM_SMOOTHING
	if(lf->animation) {
		// start a crossfade from the pose currently on screen. The blend
		// source must be a single frame, so pick whichever endpoint of the
		// active frame lerp is closest to the displayed pose (backlerp is
		// the weight still on oldFrame). If a blend is already running and
		// still mostly showing its source, keep that source so a rapid
		// state change does not pop.
		if(lf->blendFromAnimation && lf->backlerp > 0.5f) {
			// keep current blend source
		} else if(lf->backlerp > 0.5f) {
			lf->blendFromFrame = lf->oldFrame;
		} else {
			lf->blendFromFrame = lf->frame;
		}
		lf->blendFromAnimation = lf->animation;
		lf->blendStartTime = cg.time;
	}
#endif

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if(newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS) {
		CG_Error("Bad animation number: %i", newAnimation);
	}

	anim = &ci->animations[newAnimation];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

#if ANIM_SMOOTHING
	if(lf->blendFromAnimation) {
		// the crossfade morphs into the new animation's first frame, which
		// is held until the blend completes (CG_RunLerpFrame pins frameTime)
		lf->frame = anim->firstFrame;
	}
#endif

	if(cg_debugAnim.integer) {
		CG_Printf("Anim: %i\n", newAnimation);
	}
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunLerpFrame(clientInfo_t* ci, lerpFrame_t* lf, int newAnimation, float speedScale) {
	int f, numFrames;
	animation_t* anim;

	// debugging tool to get no animations
	if(cg_animSpeed.integer == 0) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if(newAnimation != lf->animationNumber || !lf->animation) {
		CG_SetLerpFrameAnimation(ci, lf, newAnimation);
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if(cg.time >= lf->frameTime) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if(!anim->frameLerp) {
			return;  // shouldn't happen
		}
		if(cg.time < lf->animationTime) {
			lf->frameTime = lf->animationTime;  // initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		f *= speedScale;  // adjust for haste, etc

		numFrames = anim->numFrames;
		if(anim->flipflop) {
			numFrames *= 2;
		}
		if(f >= numFrames) {
			f -= numFrames;
			if(anim->loopFrames) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		if(anim->reversed) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		} else if(anim->flipflop && f >= anim->numFrames) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - (f % anim->numFrames);
		} else {
			lf->frame = anim->firstFrame + f;
		}
		if(cg.time > lf->frameTime) {
			lf->frameTime = cg.time;
			if(cg_debugAnim.integer) {
				CG_Printf("Clamp lf->frameTime\n");
			}
		}
	}

	if(lf->frameTime > cg.time + 200) {
		lf->frameTime = cg.time;
	}

	if(lf->oldFrameTime > cg.time) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if(lf->frameTime == lf->oldFrameTime) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}

#if ANIM_SMOOTHING
	// while a crossfade is running, render the pose of the previous
	// animation frozen at the switch morphing into the new animation's
	// current frame: the frozen pose goes to the oldframe slot and the
	// blend weight rides on backlerp, exactly the lerp the renderer
	// computes between oldframe and frame. frameTime is pinned to the
	// blend end, which holds the new clip's first frame until the morph
	// finishes (same mechanism as the vanilla initialLerp hold) and
	// keeps the frame stepping state above coherent.
	if(lf->blendFromAnimation) {
		float blendT;

		blendT = (cg.time - lf->blendStartTime) / (float)ANIM_SMOOTHING_TIME;
		if(blendT >= 1.0f) {
			lf->blendFromAnimation = NULL;  // blend finished, normal output resumes
			// restart the new clip's frame clock cleanly at the blend end
			lf->animationTime = lf->frameTime;
		} else {
			if(blendT < 0.0f) {
				blendT = 0.0f;  // time jumped backwards, hold the source pose
			}
			lf->oldFrame = lf->blendFromFrame;
			lf->oldFrameTime = lf->blendStartTime;
			lf->frameTime = lf->blendStartTime + ANIM_SMOOTHING_TIME;
			lf->backlerp = 1.0f - blendT;
		}
	}
#endif
}

/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame(clientInfo_t* ci, lerpFrame_t* lf, int animationNumber) {
#if ANIM_SMOOTHING
	// resets (spawn, respawn, teleport, coming into view) snap straight
	// to the first frame: no crossfade out of a stale pose
	lf->blendFromAnimation = NULL;
	lf->animation = NULL;
#endif
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation(ci, lf, animationNumber);
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}

/*
===============
CG_PlayerAnimation
===============
*/
static int CG_PlayerAnimationNumber(const centity_t* cent) {
	int actionState;
	int movementState;
	int actionAnim;
	int movementAnim;
	int movementOffset;
	int weapon;
	animType_t animType;

	actionState = cent->currentState.torsoAnim;
	movementState = cent->currentState.legsAnim;
	actionAnim = actionState & ~ANIM_TOGGLEBIT;
	movementAnim = movementState & ~ANIM_TOGGLEBIT;

	// Keep the toggle bit so repeating the same gesture/taunt restarts its clip.
	// Full-body actions (death, gesture, taunt, melee and throw) take priority.
	if(actionAnim >= 0 && actionAnim < AM_IDLE) {
		return actionAnim | (actionState & ANIM_TOGGLEBIT);
	}
	if(movementAnim >= 0 && movementAnim < AM_IDLE) {
		return movementAnim | (movementState & ANIM_TOGGLEBIT);
	}

	if(movementAnim < AM_IDLE || movementAnim > AM_JUMP) {
		movementAnim = AM_IDLE;
	}
	movementOffset = movementAnim - AM_IDLE;

	weapon = cent->currentState.weapon;
	if(weapon < WP_NONE || weapon >= WEAPONS_NUM) {
		weapon = WP_NONE;
	}
	animType = jsd_weaponFx[weapon].animType;
	if(animType < AMTYPE_ALL || animType >= AMTYPE_COUNT) {
		animType = AMTYPE_ALL;
	}

	return (AM_IDLE + movementOffset + AM_MOVE_ANIMATION_COUNT * animType) | (movementState & ANIM_TOGGLEBIT);
}

/*
===============
CG_PlayerAnimation

Runs one full-body IQM animation instead of separate legs/torso frames.
===============
*/
static void CG_PlayerAnimation(centity_t* cent, int* oldFrame, int* frame, float* backlerp) {
	clientInfo_t* ci;
	int animationNumber;

	ci = &cgs.clientinfo[cent->currentState.clientNum];
	if(cg_noPlayerAnims.integer) {
		*oldFrame = *frame = 0;
		*backlerp = 0.0f;
		return;
	}

	animationNumber = CG_PlayerAnimationNumber(cent);
	CG_RunLerpFrame(ci, &cent->pe.legs, animationNumber, 1.0f);
	*oldFrame = cent->pe.legs.oldFrame;
	*frame = cent->pe.legs.frame;
	*backlerp = cent->pe.legs.backlerp;
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
===============
CG_PlayerAngles

Sets the orientation of the single full-body IQM model.
===============
*/
static void CG_PlayerAngles(centity_t* cent, vec3_t axis[3]) {
	vec3_t angles;

	// A single IQM skeleton owns the complete pose; no independent head/torso
	// yaw chain is applied. Directional locomotion comes from the selected clip.
	VectorCopy(cent->lerpAngles, angles);
	angles[PITCH] = 0.0f;
	angles[ROLL] = 0.0f;
	AnglesToAxis(angles, axis);
}

//==========================================================================

/*
===============
CG_TrailItem
===============
*/
static void CG_TrailItem(centity_t* cent, qhandle_t hModel) {
	refEntity_t ent;
	vec3_t angles;
	vec3_t axis[3];

	VectorCopy(cent->lerpAngles, angles);
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis(angles, axis);

	memset(&ent, 0, sizeof(ent));
	VectorMA(cent->lerpOrigin, -16, axis[0], ent.origin);
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis(angles, ent.axis);

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_PlayerFlag
===============
*/

/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite(centity_t* cent, qhandle_t shader) {
	int rf;
	refEntity_t ent;

	if(cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson) {
		rf = RF_THIRD_PERSON;  // only show in mirrors
	} else {
		rf = 0;
	}

	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene(&ent);
}

/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts(vec3_t normal, int numVerts, polyVert_t* verts) {
	int i, j;
	float incoming;
	vec3_t ambientLight;
	vec3_t lightDir;
	vec3_t directedLight;

	trap_R_LightForPoint(verts[0].xyz, ambientLight, directedLight, lightDir);

	for(i = 0; i < numVerts; i++) {
		incoming = DotProduct(normal, lightDir);
		if(incoming <= 0) {
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		}
		j = (ambientLight[0] + incoming * directedLight[0]);
		if(j > 255) {
			j = 255;
		}
		verts[i].modulate[0] = j;

		j = (ambientLight[1] + incoming * directedLight[1]);
		if(j > 255) {
			j = 255;
		}
		verts[i].modulate[1] = j;

		j = (ambientLight[2] + incoming * directedLight[2]);
		if(j > 255) {
			j = 255;
		}
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return true;
}

/*
===============
CG_GetLocalPlayerHeadOrigin

World space position of the local player's ValveBiped.Bip01_Head1 joint,
computed exactly like CG_Player builds the body entity (same axis, frames
and MINS_Z offset). Used to place the camera in full body awareness mode.
Returns false if the tag is not available.
===============
*/
bool CG_GetLocalPlayerHeadOrigin(vec3_t origin) {
	centity_t* cent;
	clientInfo_t* ci;
	orientation_t tag;
	vec3_t axis[3];
	vec3_t angles;
	int oldframe, frame;
	float backlerp;

	if(!cg.snap) {
		return false;
	}

	cent = &cg.predictedPlayerEntity;
	ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	if(!ci->infoValid || !ci->model) {
		return false;
	}

	// the predicted player entity is rebuilt from the playerstate every
	// frame, so the pose matches what CG_Player will use
	angles[PITCH] = 0.0f;
	angles[ROLL] = 0.0f;
	angles[YAW] = cg.predictedPlayerState.viewangles[YAW];
	AnglesToAxis(angles, axis);

	CG_PlayerAnimation(cent, &oldframe, &frame, &backlerp);

	memset(&tag, 0, sizeof(tag));
	AxisClear(tag.axis);
	if(!trap_CM_LerpTag(&tag, ci->model, oldframe, frame, 1.0f - backlerp, "ValveBiped.Bip01_Head1")) {
		return false;
	}

	VectorCopy(cg.predictedPlayerState.origin, origin);
	origin[2] += MINS_Z * 0.50f;
	VectorMA(origin, tag.origin[0], axis[0], origin);
	VectorMA(origin, tag.origin[1], axis[1], origin);
	VectorMA(origin, tag.origin[2], axis[2], origin);
	return true;
}

/*
===============
CG_Player
===============
*/
void CG_Player(centity_t* cent) {
	clientInfo_t* ci;
	refEntity_t player;
	int clientNum;
	int renderfx;

	clientNum = cent->currentState.clientNum;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS) {
		CG_Error("Bad clientNum on player entity");
	}
	ci = &cgs.clientinfo[clientNum];
	if(!ci->infoValid) {
		return;
	}

	renderfx = 0;

	memset(&player, 0, sizeof(player));
	CG_PlayerAngles(cent, player.axis);
	CG_PlayerAnimation(cent, &player.oldframe, &player.frame, &player.backlerp);

	player.hModel = ci->model;
	player.customSkin = ci->modelSkin;
	VectorCopy(cent->lerpOrigin, player.origin);
	VectorCopy(cent->lerpOrigin, player.lightingOrigin);
	VectorCopy(player.origin, player.oldorigin);
	player.origin[2] += MINS_Z;
	player.lightingOrigin[2] += MINS_Z;
	player.oldorigin[2] += MINS_Z;
	player.renderfx = renderfx | RF_LIGHTING_ORIGIN;
	if(!player.hModel) return;

	// full body awareness: the body is always drawn in the main view;
	// in first person the head joint is collapsed so it does not block
	// the camera sitting in it. The renderer walks the same refEntity
	// list for the main pass and for every mirror/portal pass, so the
	// local body is added TWICE with opposite pass flags: a collapsed
	// copy that only the main view sees (RF_FIRST_PERSON) and a full
	// copy that only mirror/portal passes see (RF_THIRD_PERSON).
	if(cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson) {
		vec3_t hideScale = {0.0f, 0.0f, 0.0f};
		vec3_t normalScale = {1.0f, 1.0f, 1.0f};

		// main view: neck+head collapsed, visible only through the eyes
		trap_R_SetBoneScale("ValveBiped.Bip01_Neck1", hideScale);
		trap_R_SetBoneScale("ValveBiped.Bip01_Head1", hideScale);
		player.renderfx |= RF_FIRST_PERSON;
		trap_R_AddRefEntityToScene(&player);

		// mirrors and portals: full skeleton, visible only in reflection passes
		trap_R_SetBoneScale("ValveBiped.Bip01_Neck1", normalScale);
		trap_R_SetBoneScale("ValveBiped.Bip01_Head1", normalScale);
		player.renderfx &= ~RF_FIRST_PERSON;
		player.renderfx |= RF_THIRD_PERSON;
		trap_R_AddRefEntityToScene(&player);

		// the weapon rides on the hand, which is identical in both bodies;
		// drop the pass flags so it draws in the main view and in mirrors
		player.renderfx &= ~RF_THIRD_PERSON;
	} else {
		trap_R_AddRefEntityToScene(&player);
	}

	if(cent->currentState.number != cg.snap->ps.clientNum) {
		CG_Add3DString(cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2] + 48, ci->name, FONTSTYLE_DROPSHADOW, color_white, 0.50, 512, 1536, true);
	}

	CG_AddPlayerWeapon(&player, cent, ci);
}

//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity(centity_t* cent) {
	cent->errorTime = -99999;  // guarantee no error decay added
	cent->extrapolated = false;

	CG_ClearLerpFrame(&cgs.clientinfo[cent->currentState.clientNum], &cent->pe.legs, AM_IDLE);

	BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, cent->lerpOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles);

	VectorCopy(cent->lerpOrigin, cent->rawOrigin);
	VectorCopy(cent->lerpAngles, cent->rawAngles);

	// CG_ClearLerpFrame above reset the one full-body IQM animation state.
}
