// Copyright (C) 2023-2026 Noire's Mod [noire.dev] — GPLv2

#ifndef __JAVASCRIPT_H
#define __JAVASCRIPT_H

#define MAX_JS_ARGS 16
#define MAX_JS_STRINGSIZE 256
#define GETVMCONTEXT 1000
#define VMCALL 1001

#ifdef GAME
#include "../game/g_local.h"
#endif
#ifdef CGAME
#include "../cgame/cg_local.h"
#include "vm_javascript_draw.h"
#endif
#ifdef UI
#include "../ui/ui_local.h"
#include "vm_javascript_draw.h"
#endif

typedef enum {
	JS_SHELLINIT,
	JS_SHELLDRAW,
	JS_SHELLKEY,
	JS_SHELLCALLBACK,
	JS_SHELLUPDATE,
	JS_SHELLBACKGROUNDUPDATE,
	JS_SHELLSHUTDOWN,
	JS_GETAPP,
	JS_GETAPPNAME,
	JS_GETAPPICON,
	JS_LAUNCHAPP,
	JS_HUDINIT,
	JS_HUDDRAW,
	JS_GETTOOL,
	JS_GETTOOLNAMEID,
	JS_GETTOOLNAME,
	JS_TOOLINIT,
	JS_TOOLCALL,
	JS_GAMERUNFRAME,
} jscall_t;

typedef enum {
	// General
	VM_CMD,

	// game.qvm
	VM_APIGAMEENTITY,
	VM_GAPIENTITYDELETE,

	// cgame.qvm + ui.qvm
	VM_APICGUI,
	VM_APIGLCONFIG,
	VM_DRAWSTRING,
	VM_DRAWSTRINGADJUSTED,
	VM_DRAWRECTANGLE,
	VM_DRAWSHADER,

	// cgame.qvm
	VM_APIHUD,
	VM_APICLIENTPLAYER,
	VM_HUDCOUNTER,
	VM_NOTIFY,

	// ui.qvm
	VM_APISHELL,
	VM_APIWINDOW,
	VM_APIELEMENT,
	VM_ITEMFOCUSED,
	VM_CURSORINWINDOW,
	VM_CLOSEWINDOW,
	VM_CLEARWINDOW,
	VM_TELEPORTWINDOWTOPLAYER,
	VM_GETFREEELEMENT,
	VM_CREATEWINDOW,
	VM_CREATEWINDOWBUTTON,
	VM_CREATEPICTURE,
	VM_CREATEBUTTON,
	VM_CREATECHECKBOX,
	VM_CREATESLIDER,
	VM_CREATEACTION,
	VM_CREATESPIN,
	VM_CREATEFIELD,
	VM_CREATELIST,
	VM_CREATELISTFILES,
	VM_FILLLISTFILES,
	VM_GETFILEFROMLIST,
	VM_GETFILELISTCOUNT,
	VM_SETMARGIN,
} vmcall_t;

void VMCall(int func_id);

typedef enum { JS_TYPE_NONE, JS_TYPE_INT, JS_TYPE_FLOAT, JS_TYPE_STRING } js_type_t;

typedef struct {
	int i;
	float f;
	char s[MAX_JS_STRINGSIZE];
} js_value_t;

typedef struct {
	js_type_t t[MAX_JS_ARGS];
	js_value_t v[MAX_JS_ARGS];
} js_args_t;

typedef struct {
	js_type_t t;
	js_value_t v;
} js_result_t;

void JS_StackClean(void);
void JS_Function(int func_id);

// JS Functions
void JS_ShellInit(void);
void JS_ShellDraw(void);
void JS_ShellKey(int key, int windowID);
void JS_ShellCallback(int windowID, int elementID, int key);
void JS_ShellUpdate(int windowID);
void JS_ShellBackgroundUpdate(int windowID);
void JS_ShellShutdown(int windowID);
int JS_GetApp(int id);
const char* JS_GetAppName(int id);
const char* JS_GetAppIcon(int id);
void JS_LaunchApp(char* nameID);
void JS_HUDInit(void);
void JS_HUDDraw(void);
int JS_GetTool(int id);
const char* JS_GetToolNameID(int id);
const char* JS_GetToolName(int id);
void JS_ToolInit(int id);
void JS_ToolCall(
    int id, int targetID, int playerID, char* arg1, char* arg2, char* arg3, char* arg4, char* arg5, char* arg6, char* arg7, char* arg8, char* arg9);
void JS_GameRunFrame(int levelTime);

extern js_args_t vmargs;
extern js_result_t vmresult;
extern js_args_t jsargs;
extern js_result_t jsresult;

// Engine Calls
void VMContext(js_args_t* args, js_result_t* result);
qboolean JSOpenFile(const char* filename);
void JSLoadScripts(const char* path, const char* name);
qboolean JSEval(const char* code, js_result_t* result);
qboolean JSCall(int id, js_args_t* args, js_result_t* result);

#endif
