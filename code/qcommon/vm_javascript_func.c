// Copyright (C) 2023-2026 Noire's Mod — GPLv2

#include "vm_javascript.h"

void JS_ShellInit(void) {
	JS_StackClean();
	JS_Function(JS_SHELLINIT);
}

void JS_ShellDraw(void) {
	JS_StackClean();
	JS_Function(JS_SHELLDRAW);
}

void JS_ShellKey(int key, int windowID) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = key;
	jsargs.t[1] = JS_TYPE_INT;
	jsargs.v[1].i = windowID;
	JS_Function(JS_SHELLKEY);
}

void JS_ShellCallback(int windowID, int elementID, int key) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = windowID;
	jsargs.t[1] = JS_TYPE_INT;
	jsargs.v[1].i = elementID;
	jsargs.t[2] = JS_TYPE_INT;
	jsargs.v[2].i = key;
	JS_Function(JS_SHELLCALLBACK);
}

void JS_ShellUpdate(int windowID) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = windowID;
	JS_Function(JS_SHELLUPDATE);
}

void JS_ShellBackgroundUpdate(int windowID) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = windowID;
	JS_Function(JS_SHELLBACKGROUNDUPDATE);
}

void JS_ShellShutdown(int windowID) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = windowID;
	JS_Function(JS_SHELLSHUTDOWN);
}

int JS_GetApp(int id) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = id;
	JS_Function(JS_GETAPP);
	return jsresult.v.i;
}

const char* JS_GetAppName(int id) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = id;
	JS_Function(JS_GETAPPNAME);
	return jsresult.v.s;
}

const char* JS_GetAppIcon(int id) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = id;
	JS_Function(JS_GETAPPICON);
	return jsresult.v.s;
}

void JS_LaunchApp(char* nameID) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_STRING;
	StringCopy(jsargs.v[0].s, nameID, MAX_JS_STRINGSIZE);
	JS_Function(JS_LAUNCHAPP);
}

void JS_HUDInit(void) {
	JS_StackClean();
	JS_Function(JS_HUDINIT);
}

void JS_HUDDraw(void) {
	JS_StackClean();
	JS_Function(JS_HUDDRAW);
}

int JS_GetTool(int id) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = id;
	JS_Function(JS_GETTOOL);
	return jsresult.v.i;
}

const char* JS_GetToolNameID(int id) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = id;
	JS_Function(JS_GETTOOLNAMEID);
	return jsresult.v.s;
}

const char* JS_GetToolName(int id) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = id;
	JS_Function(JS_GETTOOLNAME);
	return jsresult.v.s;
}

void JS_ToolInit(int id) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = id;
	JS_Function(JS_TOOLINIT);
}

void JS_ToolCall(
    int id, int targetID, int playerID, char* arg1, char* arg2, char* arg3, char* arg4, char* arg5, char* arg6, char* arg7, char* arg8, char* arg9) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = id;
	jsargs.t[1] = JS_TYPE_INT;
	jsargs.v[1].i = targetID;
	jsargs.t[2] = JS_TYPE_INT;
	jsargs.v[2].i = playerID;
	jsargs.t[3] = JS_TYPE_STRING;
	StringCopy(jsargs.v[3].s, arg1, MAX_JS_STRINGSIZE);
	jsargs.t[4] = JS_TYPE_STRING;
	StringCopy(jsargs.v[4].s, arg2, MAX_JS_STRINGSIZE);
	jsargs.t[5] = JS_TYPE_STRING;
	StringCopy(jsargs.v[5].s, arg3, MAX_JS_STRINGSIZE);
	jsargs.t[6] = JS_TYPE_STRING;
	StringCopy(jsargs.v[6].s, arg4, MAX_JS_STRINGSIZE);
	jsargs.t[7] = JS_TYPE_STRING;
	StringCopy(jsargs.v[7].s, arg5, MAX_JS_STRINGSIZE);
	// jsargs.t[8] = JS_TYPE_STRING;
	// StringCopy(jsargs.v[8].s, arg6, MAX_JS_STRINGSIZE);
	// jsargs.t[9] = JS_TYPE_STRING;
	// StringCopy(jsargs.v[9].s, arg7, MAX_JS_STRINGSIZE);
	// jsargs.t[10] = JS_TYPE_STRING;
	// StringCopy(jsargs.v[10].s, arg8, MAX_JS_STRINGSIZE);
	// jsargs.t[11] = JS_TYPE_STRING;
	// StringCopy(jsargs.v[11].s, arg9, MAX_JS_STRINGSIZE);
	JS_Function(JS_TOOLCALL);
}

void JS_GameRunFrame(int levelTime) {
	JS_StackClean();
	jsargs.t[0] = JS_TYPE_INT;
	jsargs.v[0].i = levelTime;
	JS_Function(JS_GAMERUNFRAME);
}
