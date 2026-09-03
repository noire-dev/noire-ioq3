// Copyright (C) 2023-2026 Noire's Mod — GPLv2

#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#define MAX_JS_ARGS 16
#define MAX_JS_STRINGSIZE 256

#include "../qcommon/q_shared.h"
#include "../qcommon/vm_sharedsyscalls.h"
#include "../renderercommon/tr_types.h"
#include "ui_public.h"
#include "../game/bg_public.h"

#define SHELL_STATE_FILE "nmre_state.bin"
#define UI_WINDOW_TITLE_HEIGHT (40.0 * shell.scale)

#define ETYPE_NULL 0
#define ETYPE_PICTURE 1
#define ETYPE_BUTTON 2
#define ETYPE_CHECKBOX 3
#define ETYPE_SLIDER 4
#define ETYPE_ACTION 5
#define ETYPE_SPIN 6
#define ETYPE_FIELD 7
#define ETYPE_LIST 8
#define ETYPE_LISTFILES 9
#define ETYPE_WINDOWBUTTON 10

#define EMODE_NONE 0
#define EMODE_INT 1
#define EMODE_FLOAT 2

#define EMODE_NUMBER 1
#define EMODE_STRING 2

#define LSTYLE_NONE 0
#define LSTYLE_GRID 1
#define LSTYLE_LIST 2

#define LCONTENT_NONE 0
#define LCONTENT_SHADER 1
#define LCONTENT_MODEL 2

#define LTYPE_NONE 0
#define LTYPE_APPS 1
#define LTYPE_MAPS 2
#define LTYPE_BOTS 3
#define LTYPE_PLAYERMODELS 4
#define LTYPE_GAMEITEMS 5
#define LTYPE_TOOLS 6

#define ELEMENT_OPTIONLENGTH 64
#define ELEMENT_OPTIONSCOUNT 64
typedef struct {
	void* parentWindow;
	bool created;
	int type;
	int id;
	float baseX, baseY, baseW, baseH, x, y, w, h;
	char text[MAX_JS_STRINGSIZE];
	int style;
	int hoverStyle;
	float baseScale, scale;
	int colorText, colorBackground;
	int baseCorner, corner;
	int baseMargin, margin;
	char cvar[MAX_JS_STRINGSIZE];

	float value;
	float valueMod;

	float min, max;
	int mode;

	char bind[MAX_JS_STRINGSIZE];

	char options[ELEMENT_OPTIONSCOUNT][ELEMENT_OPTIONLENGTH];
	int optionsCount;

	char field[MAX_JS_STRINGSIZE];
	int fieldPosition;

	int listStyle;
	int listContent;
	int listType;
	int listSubtype;
	int scroll;
	float itemW, itemH;
	int col, row;

	int listID;

	float baseMargin4[4], margin4[4];
} element_s;

#define FILELIST_MAX_LENGTH (1024 * 512)
typedef struct {
	char list[FILELIST_MAX_LENGTH];
	int count;
	char folder[MAX_JS_STRINGSIZE];
	char ext[MAX_JS_STRINGSIZE];
	char drawDir[MAX_JS_STRINGSIZE];
} fileList_s;

#define WINDOW_MAX_ELEMENTS 512
#define WINDOW_MAX_LISTS 2
typedef struct {
	bool created;
	bool minimized;
	bool pinned;
	bool linked;
	bool keyboardCapture;
	int id;
	char nameID[MAX_JS_STRINGSIZE];
	char name[MAX_JS_STRINGSIZE];
	char icon[MAX_JS_STRINGSIZE];
	int style;
	float baseW, baseH, x, y, w, h;
	int colorText, colorTitle, colorBackground;
	float scaleFactor;
	int focusedElement;
	int keyCapture;
	element_s element[WINDOW_MAX_ELEMENTS];
	fileList_s fileList[WINDOW_MAX_LISTS];
	float worldX, worldY, worldZ;
	float worldScale;
	bool worldDisable;
	float worldCursorScale;
	float worldMoveDist;
} window_s;

typedef struct {
	char name[MAX_JS_STRINGSIZE];
	char icon[MAX_JS_STRINGSIZE];
} app_s;

typedef struct {
	char nameID[MAX_JS_STRINGSIZE];
	char name[MAX_JS_STRINGSIZE];
} tool_s;

typedef struct {
	bool saved;
	char nameID[MAX_JS_STRINGSIZE];
	float x, y;
	float scaleFactor;
} saveState_s;

#define SHELL_MAX_APPS 4096
#define SHELL_MAX_TOOLS 4096
#define SHELL_MAX_WINDOWS 16
typedef struct {
	int cursorX, cursorY;
	bool cursorIsDragging;
	bool cursorIsMovingDesktop;
	bool rendered;
	int debug;
	bool onMap;
	app_s app[SHELL_MAX_APPS];
	int appCount;
	int focusedWindow;
	int activeWindow;
	int zOrder[SHELL_MAX_WINDOWS];
	window_s window[SHELL_MAX_WINDOWS];
	saveState_s saveState[SHELL_MAX_WINDOWS];
	float scale;
	int windowCorner;
	bool windowOutline;
	bool windowColoredOutline;

	tool_s tool[SHELL_MAX_TOOLS];
	int toolCount;
} shell_s;
extern shell_s shell;

// Функции вызываемые из движка
int UI_Init(void);
int UI_Shutdown(void);
int UI_KeyEvent(int key, int down, int isChar);
int UI_MouseEvent(int dx, int dy);
int UI_Refresh(void);
int UI_IsFullscreen(void);
int UI_SetActiveMenu(uiMenuCommand_t cmd);
bool UI_ConsoleCommand(void);
int UI_DrawConnectScreen(void);

// Функции подключенные к JS
bool UI_ItemFocused(element_s* element);
bool UI_CursorInWindowRect(window_s* window, float x, float y, float w, float h);
void UI_Move3DWindow(int windowID, float distance);
void UI_CloseWindow(int windowID);
void UI_ClearWindow(int windowID, int min, int max);
int UI_GetElementFree(int windowID);
int UI_Window(int windowID, char* nameid, char* name, char* icon, int style, float w, float h, int colorText, int colorTitle, int colorBackground);
int UI_WindowButton(int windowID, int elementID, float x, char* text, int style, int color, float scale);
int UI_Picture(int windowID, int elementID, float x, float y, float w, float h, char* picture, int style, int color);
int UI_Button(int windowID, int elementID, float x, float y, float w, float h, char* text, int style, int color, float scale);
int UI_Checkbox(int windowID, int elementID, float x, float y, float w, float h, char* text, int style, int color, float scale, char* cvar);
int UI_Slider(int windowID, int elementID, float x, float y, float w, float h, char* text, int style, int color, float scale, char* cvar, float min, float max, int mode);
int UI_Action(int windowID, int elementID, float x, float y, float w, float h, char* text, int style, int color, float scale, char* cvar);
int UI_Spin(int windowID, int elementID, float x, float y, float w, float h, char* text, int style, int color, float scale, char* cvar, int mode);
int UI_Field(int windowID, int elementID, float x, float y, float w, float h, char* text, int style, int color, float scale, char* cvar);
int UI_List(int windowID, int elementID, float x, float y, float w, float h, float scale, int col, int row, int listStyle, int listType, int listSubtype);
int UI_ListFiles(int windowID, int elementID, float x, float y, float w, float h, float scale, int col, int row, int listStyle, int listContent, int listID);
void UI_FillListFiles(int windowID, char* folder, char* ext, char* drawDir, int listID);
char* UI_GetFileFromList(window_s* window, int listID, int index);
int UI_GetFileListCount(window_s* window, int listID);
void UI_SetMargin(int windowID, int elementID, float x, float y, float w, float h);

// Вызовы движка
qhandle_t trap_R_RegisterModel(const char* name);
qhandle_t trap_R_RegisterSkin(const char* name);
qhandle_t trap_R_RegisterShaderNoMip(const char* name);
void trap_R_ClearScene(void);
void trap_R_AddRefEntityToScene(const refEntity_t* re);
void trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t* verts);
void trap_R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b);
void trap_R_RenderScene(const refdef_t* fd);
void trap_R_SetColor(const float* rgba);
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader);
void trap_UpdateScreen(void);
int trap_CM_LerpTag(orientation_t* tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char* tagName);
void trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum);
sfxHandle_t trap_S_RegisterSound(const char* sample, bool compressed);
void trap_Key_KeynumToStringBuf(int keynum, char* buf, int buflen);
void trap_Key_GetBindingBuf(int keynum, char* buf, int buflen);
void trap_Key_SetBinding(int keynum, const char* binding);
bool trap_Key_IsDown(int keynum);
bool trap_Key_GetOverstrikeMode(void);
void trap_Key_SetOverstrikeMode(bool state);
void trap_Key_ClearStates(void);
int trap_Key_GetCatcher(void);
void trap_Key_SetCatcher(int catcher);
void trap_GetClipboardData(char* buf, int bufsize);
void trap_GetClientState(uiClientState_t* state);
void trap_GetGlconfig(glconfig_t* glconfig);
int trap_GetConfigString(int index, char* buff, int buffsize);
int trap_LAN_GetServerCount(int source);
void trap_LAN_GetServerAddressString(int source, int n, char* buf, int buflen);
void trap_LAN_GetServerInfo(int source, int n, char* buf, int buflen);
int trap_LAN_GetPingQueueCount(void);
int trap_LAN_ServerStatus(const char* serverAddress, char* serverStatus, int maxLen);
void trap_LAN_ClearPing(int n);
void trap_LAN_GetPing(int n, char* buf, int buflen, int* pingtime);
void trap_LAN_GetPingInfo(int n, char* buf, int buflen);
int trap_MemoryRemaining(void);
void trap_GetCDKey(char* buf, int buflen);
void trap_SetCDKey(char* buf);
bool trap_VerifyCDKey(const char* key, const char* chksum);
void trap_SetPbClStatus(int status);
#endif
