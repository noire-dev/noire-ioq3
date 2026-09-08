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
// cg_syscalls.c -- this file is only included when building a dll
// cg_syscalls.asm is included instead when building a qvm

#include "cg_local.h"

static intptr_t(QDECL* syscall)(intptr_t arg, ...) = (intptr_t(QDECL*)(intptr_t, ...)) - 1;

Q_EXPORT void dllEntry(intptr_t(QDECL* syscallptr)(intptr_t arg, ...)) { syscall = syscallptr; }

int PASSFLOAT(float x) {
	floatint_t fi;
	fi.f = x;
	return fi.i;
}

void trap_AddCommand(const char* cmdName) { syscall(CG_ADDCOMMAND, cmdName); }
void trap_RemoveCommand(const char* cmdName) { syscall(CG_REMOVECOMMAND, cmdName); }
void trap_SendClientCommand(const char* s) { syscall(CG_SENDCLIENTCOMMAND, s); }
void trap_CM_LoadMap(const char* mapname) { syscall(CG_CM_LOADMAP, mapname); }
int trap_CM_NumInlineModels(void) { return syscall(CG_CM_NUMINLINEMODELS); }
clipHandle_t trap_CM_InlineModel(int index) { return syscall(CG_CM_INLINEMODEL, index); }
clipHandle_t trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs) { return syscall(CG_CM_TEMPBOXMODEL, mins, maxs); }
clipHandle_t trap_CM_TempCapsuleModel(const vec3_t mins, const vec3_t maxs) { return syscall(CG_CM_TEMPCAPSULEMODEL, mins, maxs); }
int trap_CM_MarkFragments(int numPoints, const vec3_t* points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t* fragmentBuffer) { return syscall(CG_CM_MARKFRAGMENTS, numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer); }
void trap_S_StartSound(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx) { syscall(CG_S_STARTSOUND, origin, entityNum, entchannel, sfx); }
void trap_S_ClearLoopingSounds(bool killall) { syscall(CG_S_CLEARLOOPINGSOUNDS, killall); }
void trap_S_AddLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx) { syscall(CG_S_ADDLOOPINGSOUND, entityNum, origin, velocity, sfx); }
void trap_S_AddRealLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx) { syscall(CG_S_ADDREALLOOPINGSOUND, entityNum, origin, velocity, sfx); }
void trap_S_StopLoopingSound(int entityNum) { syscall(CG_S_STOPLOOPINGSOUND, entityNum); }
void trap_S_UpdateEntityPosition(int entityNum, const vec3_t origin) { syscall(CG_S_UPDATEENTITYPOSITION, entityNum, origin); }
void trap_S_Respatialize(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater) { syscall(CG_S_RESPATIALIZE, entityNum, origin, axis, inwater); }
void trap_S_StartBackgroundTrack(const char* intro, const char* loop) { syscall(CG_S_STARTBACKGROUNDTRACK, intro, loop); }
void trap_R_LoadWorldMap(const char* mapname) { syscall(CG_R_LOADWORLDMAP, mapname); }
void trap_R_RegisterFont(const char* fontName, int pointSize, fontInfo_t* font) { syscall(CG_R_REGISTERFONT, fontName, pointSize, font); }
void trap_R_AddPolysToScene(qhandle_t hShader, int numVerts, const polyVert_t* verts, int num) { syscall(CG_R_ADDPOLYSTOSCENE, hShader, numVerts, verts, num); }
int trap_R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir) { return syscall(CG_R_LIGHTFORPOINT, point, ambientLight, directedLight, lightDir); }
void trap_R_AddAdditiveLightToScene(const vec3_t org, float intensity, float r, float g, float b) { syscall(CG_R_ADDADDITIVELIGHTTOSCENE, org, PASSFLOAT(intensity), PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b)); }
void trap_GetGameState(gameState_t* gamestate) { syscall(CG_GETGAMESTATE, gamestate); }
void trap_GetCurrentSnapshotNumber(int* snapshotNumber, int* serverTime) { syscall(CG_GETCURRENTSNAPSHOTNUMBER, snapshotNumber, serverTime); }
bool trap_GetSnapshot(int snapshotNumber, snapshot_t* snapshot) { return syscall(CG_GETSNAPSHOT, snapshotNumber, snapshot); }
bool trap_GetServerCommand(int serverCommandNumber) { return syscall(CG_GETSERVERCOMMAND, serverCommandNumber); }
int trap_GetCurrentCmdNumber(void) { return syscall(CG_GETCURRENTCMDNUMBER); }
bool trap_GetUserCmd(int cmdNumber, usercmd_t* ucmd) { return syscall(CG_GETUSERCMD, cmdNumber, ucmd); }
void trap_SetUserCmdValue(int stateValue, float sensitivityScale) { syscall(CG_SETUSERCMDVALUE, stateValue, PASSFLOAT(sensitivityScale)); }
int trap_MemoryRemaining(void) { return syscall(CG_MEMORY_REMAINING); }
void trap_Key_SetCatcher(int catcher) { syscall(CG_KEY_SETCATCHER, catcher); }
int trap_PC_AddGlobalDefine(char* define) { return syscall(CG_PC_ADD_GLOBAL_DEFINE, define); }
int trap_PC_LoadSource(const char* filename) { return syscall(CG_PC_LOAD_SOURCE, filename); }
int trap_PC_FreeSource(int handle) { return syscall(CG_PC_FREE_SOURCE, handle); }
int trap_PC_ReadToken(int handle, pc_token_t* pc_token) { return syscall(CG_PC_READ_TOKEN, handle, pc_token); }
int trap_PC_SourceFileAndLine(int handle, char* filename, int* line) { return syscall(CG_PC_SOURCE_FILE_AND_LINE, handle, filename, line); }
void trap_S_StopBackgroundTrack(void) { syscall(CG_S_STOPBACKGROUNDTRACK); }
int trap_CIN_PlayCinematic(const char* arg0, int xpos, int ypos, int width, int height, int bits) { return syscall(CG_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits); }
e_status trap_CIN_StopCinematic(int handle) { return syscall(CG_CIN_STOPCINEMATIC, handle); }
e_status trap_CIN_RunCinematic(int handle) { return syscall(CG_CIN_RUNCINEMATIC, handle); }
void trap_CIN_DrawCinematic(int handle) { syscall(CG_CIN_DRAWCINEMATIC, handle); }
void trap_CIN_SetExtents(int handle, int x, int y, int w, int h) { syscall(CG_CIN_SETEXTENTS, handle, x, y, w, h); }
bool trap_GetEntityToken(char* buffer, int bufferSize) { return syscall(CG_GET_ENTITY_TOKEN, buffer, bufferSize); }
bool trap_R_inPVS(const vec3_t p1, const vec3_t p2) { return syscall(CG_R_INPVS, p1, p2); }

#include "../qcommon/vm_sharedsyscalls.inc"
#include "../qcommon/vm_sharedsyscalls_client.inc"
