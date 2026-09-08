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
#include "ui_local.h"

static intptr_t(QDECL* syscall)(intptr_t arg, ...) = (intptr_t(QDECL*)(intptr_t, ...)) - 1;

Q_EXPORT void dllEntry(intptr_t(QDECL* syscallptr)(intptr_t arg, ...)) { syscall = syscallptr; }

int PASSFLOAT(float x) {
	floatint_t fi;
	fi.f = x;
	return fi.i;
}

void trap_R_RegisterFont(const char* fontName, int pointSize, fontInfo_t* font) { syscall(UI_R_REGISTERFONT, fontName, pointSize, font); }
void trap_Key_SetBinding(int keynum, const char* binding) { syscall(UI_KEY_SETBINDING, keynum, binding); }
void trap_Key_ClearStates(void) { syscall(UI_KEY_CLEARSTATES); }
void trap_Key_SetCatcher(int catcher) { syscall(UI_KEY_SETCATCHER, catcher); }
void trap_GetClipboardData(char* buf, int bufsize) { syscall(UI_GETCLIPBOARDDATA, buf, bufsize); }
void trap_GetClientState(uiClientState_t* state) { syscall(UI_GETCLIENTSTATE, state); }
int trap_GetConfigString(int index, char* buff, int buffsize) { return syscall(UI_GETCONFIGSTRING, index, buff, buffsize); }
int trap_LAN_GetServerCount(int source) { return syscall(UI_LAN_GETSERVERCOUNT, source); }
void trap_LAN_GetServerAddressString(int source, int n, char* buf, int buflen) { syscall(UI_LAN_GETSERVERADDRESSSTRING, source, n, buf, buflen); }
void trap_LAN_GetServerInfo(int source, int n, char* buf, int buflen) { syscall(UI_LAN_GETSERVERINFO, source, n, buf, buflen); }
int trap_LAN_GetServerPing(int source, int n) { return syscall(UI_LAN_GETSERVERPING, source, n); }
int trap_LAN_GetPingQueueCount(void) { return syscall(UI_LAN_GETPINGQUEUECOUNT); }
int trap_LAN_ServerStatus(const char* serverAddress, char* serverStatus, int maxLen) { return syscall(UI_LAN_SERVERSTATUS, serverAddress, serverStatus, maxLen); }
void trap_LAN_SaveCachedServers(void) { syscall(UI_LAN_SAVECACHEDSERVERS); }
void trap_LAN_LoadCachedServers(void) { syscall(UI_LAN_LOADCACHEDSERVERS); }
void trap_LAN_ResetPings(int n) { syscall(UI_LAN_RESETPINGS, n); }
void trap_LAN_ClearPing(int n) { syscall(UI_LAN_CLEARPING, n); }
void trap_LAN_GetPing(int n, char* buf, int buflen, int* pingtime) { syscall(UI_LAN_GETPING, n, buf, buflen, pingtime); }
void trap_LAN_GetPingInfo(int n, char* buf, int buflen) { syscall(UI_LAN_GETPINGINFO, n, buf, buflen); }
void trap_LAN_MarkServerVisible(int source, int n, bool visible) { syscall(UI_LAN_MARKSERVERVISIBLE, source, n, visible); }
int trap_LAN_ServerIsVisible(int source, int n) { return syscall(UI_LAN_SERVERISVISIBLE, source, n); }
bool trap_LAN_UpdateVisiblePings(int source) { return syscall(UI_LAN_UPDATEVISIBLEPINGS, source); }
int trap_LAN_AddServer(int source, const char* name, const char* addr) { return syscall(UI_LAN_ADDSERVER, source, name, addr); }
void trap_LAN_RemoveServer(int source, const char* addr) { syscall(UI_LAN_REMOVESERVER, source, addr); }
int trap_LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2) { return syscall(UI_LAN_COMPARESERVERS, source, sortKey, sortDir, s1, s2); }
int trap_MemoryRemaining(void) { return syscall(UI_MEMORY_REMAINING); }
void trap_GetCDKey(char* buf, int buflen) { syscall(UI_GET_CDKEY, buf, buflen); }
void trap_SetCDKey(char* buf) { syscall(UI_SET_CDKEY, buf); }
int trap_PC_AddGlobalDefine(char* define) { return syscall(UI_PC_ADD_GLOBAL_DEFINE, define); }
int trap_PC_LoadSource(const char* filename) { return syscall(UI_PC_LOAD_SOURCE, filename); }
int trap_PC_FreeSource(int handle) { return syscall(UI_PC_FREE_SOURCE, handle); }
int trap_PC_ReadToken(int handle, pc_token_t* pc_token) { return syscall(UI_PC_READ_TOKEN, handle, pc_token); }
int trap_PC_SourceFileAndLine(int handle, char* filename, int* line) { return syscall(UI_PC_SOURCE_FILE_AND_LINE, handle, filename, line); }
void trap_S_StopBackgroundTrack(void) { syscall(UI_S_STOPBACKGROUNDTRACK); }
void trap_S_StartBackgroundTrack(const char* intro, const char* loop) { syscall(UI_S_STARTBACKGROUNDTRACK, intro, loop); }
int trap_CIN_PlayCinematic(const char* arg0, int xpos, int ypos, int width, int height, int bits) { return syscall(UI_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits); }
e_status trap_CIN_StopCinematic(int handle) { return syscall(UI_CIN_STOPCINEMATIC, handle); }
e_status trap_CIN_RunCinematic(int handle) { return syscall(UI_CIN_RUNCINEMATIC, handle); }
void trap_CIN_DrawCinematic(int handle) { syscall(UI_CIN_DRAWCINEMATIC, handle); }
void trap_CIN_SetExtents(int handle, int x, int y, int w, int h) { syscall(UI_CIN_SETEXTENTS, handle, x, y, w, h); }
bool trap_VerifyCDKey(const char* key, const char* chksum) { return syscall(UI_VERIFY_CDKEY, key, chksum); }
void trap_SetPbClStatus(int status) { syscall(UI_SET_PBCLSTATUS, status); }

#include "../qcommon/vm_sharedsyscalls.inc"
#include "../qcommon/vm_sharedsyscalls_client.inc"
