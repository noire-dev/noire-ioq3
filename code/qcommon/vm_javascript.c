// Copyright (C) 2023-2026 Noire's Mod [noire.dev] — GPLv2

#include "vm_javascript.h"

js_args_t vmargs;
js_result_t vmresult;
js_args_t jsargs;
js_result_t jsresult;

void JS_StackClean(void) { memset(&jsargs, 0, sizeof(jsargs)); }
void JS_Function(int func_id) {
	memset(&jsresult, 0, sizeof(jsresult));
	JSCall(func_id, &jsargs, &jsresult);
}

static int api_foundEntry = qfalse;
static int api_valueIndex = 0;
static int api_counter = 0;

void JSE_IntRead(int number) {
	api_foundEntry = qtrue;
	if(vmargs.t[api_valueIndex] != JS_TYPE_NONE) trap_Print("JSE_IntRead: read only from JS");
	vmresult.t = JS_TYPE_INT;
	vmresult.v.i = number;
}

void JSE_Int(int* number) {
	api_foundEntry = qtrue;
	if(vmargs.t[api_valueIndex] != JS_TYPE_NONE) *number = vmargs.v[api_valueIndex].i;
	vmresult.t = JS_TYPE_INT;
	vmresult.v.i = *number;
}

void JSE_Float(float* number) {
	api_foundEntry = qtrue;
	if(vmargs.t[api_valueIndex] != JS_TYPE_NONE) *number = vmargs.v[api_valueIndex].f;
	vmresult.t = JS_TYPE_FLOAT;
	vmresult.v.f = *number;
}

void JSE_String(char* string, size_t maxSize) {
	api_foundEntry = qtrue;
	if(vmargs.t[api_valueIndex] != JS_TYPE_NONE) Q_strncpyz(string, vmargs.v[api_valueIndex].s, maxSize);
	vmresult.t = JS_TYPE_STRING;
	Q_strncpyz(vmresult.v.s, string, MAX_JS_STRINGSIZE);
}

#ifdef GAME
void JSE_Entity(gentity_t* ent) {
	api_foundEntry = qtrue;
	if(vmargs.t[api_valueIndex] != JS_TYPE_NONE) trap_Print("JSE_Entity: can't modify entity pointer from JS");
	vmresult.t = JS_TYPE_INT;
	vmresult.v.i = ent - g_entities;
}
#endif

#define API_FIELD(expression)      \
	if(fieldID == api_counter++) { \
		expression;                \
		return;                    \
	}

#ifdef GAME
static void API_GameEntity(int entityID, int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = qfalse;
	api_valueIndex = 2;
	api_counter = 0;

	bounds(entityID, MAX_GENTITIES, "API_GameEntity - entityID");
}

static void GAPI_EntityDelete(int entityID) {
	if(!g_entities[entityID].client) {
		G_FreeEntity(&g_entities[entityID]);
	} else {
		DropClientSilently(g_entities[entityID].client->ps.clientNum);
	}
}
#endif

#ifdef CGAME
static void API_HUD(int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = qfalse;
	api_valueIndex = 1;
	api_counter = 0;

	API_FIELD(JSE_Float(&hud.counterW))
	API_FIELD(JSE_Float(&hud.counterH))
	API_FIELD(JSE_Float(&hud.counterCorner))
	API_FIELD(JSE_Int(&hud.counterColor))
	API_FIELD(JSE_Float(&hud.counterTextX))
	API_FIELD(JSE_Float(&hud.counterTextY))
	API_FIELD(JSE_Float(&hud.counterTextScale))
	API_FIELD(JSE_Int(&hud.counterTextColor))
	API_FIELD(JSE_Int(&hud.counterTextStyle))
	API_FIELD(JSE_Float(&hud.counterValueX))
	API_FIELD(JSE_Float(&hud.counterValueY))
	API_FIELD(JSE_Float(&hud.counterValueScale))
	API_FIELD(JSE_Int(&hud.counterValueColor))
	API_FIELD(JSE_Int(&hud.counterValueStyle))
}
static void API_ClientPlayer(int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = qfalse;
	api_valueIndex = 1;
	api_counter = 0;

	API_FIELD(bounds(i1, MAX_STATS, "API_ClientPlayer -> stats"); JSE_Int(&cg.predictedPlayerState.stats[i1]))
}
#endif

#ifndef GAME
static void API_CGUI(int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = qfalse;
	api_valueIndex = 1;
	api_counter = 0;

	API_FIELD(JSE_Int(&cgui.whiteShader))
	API_FIELD(JSE_Int(&cgui.corner))
	API_FIELD(JSE_Float(&cgui.scale))
	API_FIELD(JSE_Float(&cgui.wideoffset))
	API_FIELD(bounds(i1, CGUI_COLORCOUNT, "API_CGUI -> colors"); bounds(i2, 4, "API_CGUI -> colors"); JSE_Float(&cgui.colors[i1][i2]))
	API_FIELD(bounds(i1, MAX_KEYS, "API_CGUI -> binds"); JSE_String(cgui.binds[i1], MAX_JS_STRINGSIZE))
}

static void API_GLConfig(int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = qfalse;
	api_valueIndex = 1;
	api_counter = 0;

	API_FIELD(JSE_Int(&glconfig.vidWidth))
	API_FIELD(JSE_Int(&glconfig.vidHeight))
	API_FIELD(JSE_Int((int*)&glconfig.isFullscreen))
}
#endif

#ifdef UI
static void API_Shell(int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = qfalse;
	api_valueIndex = 1;
	api_counter = 0;

	API_FIELD(JSE_Int(&shell.cursorX))
	API_FIELD(JSE_Int(&shell.cursorY))
	API_FIELD(JSE_Int((int*)&shell.cursorIsDragging))
	API_FIELD(JSE_Int((int*)&shell.cursorIsMovingDesktop))
	API_FIELD(JSE_Int((int*)&shell.rendered))
	API_FIELD(JSE_Int((int*)&shell.debug))
	API_FIELD(JSE_Int((int*)&shell.onMap))
	API_FIELD(JSE_Int(&shell.appCount))
	API_FIELD(JSE_Int(&shell.focusedWindow))
	API_FIELD(JSE_Int(&shell.activeWindow))
	API_FIELD(bounds(i1, SHELL_MAX_WINDOWS, "API_Shell -> zOrder"); JSE_Int(&shell.zOrder[i1]))
	API_FIELD(JSE_Float(&shell.scale))
	API_FIELD(JSE_Int(&shell.windowCorner))
	API_FIELD(JSE_Int((int*)&shell.windowOutline))
	API_FIELD(JSE_Int((int*)&shell.windowColoredOutline))
}

static void API_Window(int windowID, int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = qfalse;
	api_valueIndex = 2;
	api_counter = 0;

	bounds(windowID, SHELL_MAX_WINDOWS, "API_Window - windowID");

	API_FIELD(JSE_Int((int*)&shell.window[windowID].created))
	API_FIELD(JSE_Int((int*)&shell.window[windowID].minimized))
	API_FIELD(JSE_Int((int*)&shell.window[windowID].pinned))
	API_FIELD(JSE_Int((int*)&shell.window[windowID].linked))
	API_FIELD(JSE_Int((int*)&shell.window[windowID].keyboardCapture))
	API_FIELD(JSE_Int(&shell.window[windowID].id))
	API_FIELD(JSE_String(shell.window[windowID].nameID, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_String(shell.window[windowID].name, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_String(shell.window[windowID].icon, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_Int(&shell.window[windowID].style))
	API_FIELD(JSE_Float(&shell.window[windowID].baseW))
	API_FIELD(JSE_Float(&shell.window[windowID].baseH))
	API_FIELD(JSE_Float(&shell.window[windowID].x))
	API_FIELD(JSE_Float(&shell.window[windowID].y))
	API_FIELD(JSE_Float(&shell.window[windowID].w))
	API_FIELD(JSE_Float(&shell.window[windowID].h))
	API_FIELD(JSE_Int(&shell.window[windowID].colorText))
	API_FIELD(JSE_Int(&shell.window[windowID].colorTitle))
	API_FIELD(JSE_Int(&shell.window[windowID].colorBackground))
	API_FIELD(JSE_Float(&shell.window[windowID].scaleFactor))
	API_FIELD(JSE_Int(&shell.window[windowID].focusedElement))
	API_FIELD(JSE_Int(&shell.window[windowID].keyCapture))
	API_FIELD(JSE_Float(&shell.window[windowID].worldX))
	API_FIELD(JSE_Float(&shell.window[windowID].worldY))
	API_FIELD(JSE_Float(&shell.window[windowID].worldZ))
	API_FIELD(JSE_Float(&shell.window[windowID].worldScale))
	API_FIELD(JSE_Int((int*)&shell.window[windowID].worldDisable))
	API_FIELD(JSE_Float(&shell.window[windowID].worldCursorScale))
	API_FIELD(JSE_Float(&shell.window[windowID].worldMoveDist))
}

static void API_Element(int windowID, int elementID, int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = qfalse;
	api_valueIndex = 3;
	api_counter = 0;

	bounds(windowID, SHELL_MAX_WINDOWS, "API_Element - windowID");
	bounds(elementID, WINDOW_MAX_ELEMENTS, "API_Element - elementID");

	API_FIELD(JSE_Int(&((window_s*)shell.window[windowID].element[elementID].parentWindow)->id))
	API_FIELD(JSE_Int((int*)&shell.window[windowID].element[elementID].created))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].type))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].id))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].baseX))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].baseY))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].baseW))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].baseH))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].x))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].y))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].w))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].h))
	API_FIELD(JSE_String(shell.window[windowID].element[elementID].text, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].style))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].hoverStyle))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].baseScale))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].scale))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].colorText))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].colorBackground))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].baseCorner))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].corner))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].baseMargin))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].margin))
	API_FIELD(JSE_String(shell.window[windowID].element[elementID].cvar, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].value))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].valueMod))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].min))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].max))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].mode))
	API_FIELD(JSE_String(shell.window[windowID].element[elementID].bind, MAX_JS_STRINGSIZE))
	API_FIELD(bounds(i1, ELEMENT_OPTIONSCOUNT, "API_Element -> options");
	          JSE_String(shell.window[windowID].element[elementID].options[i1], ELEMENT_OPTIONLENGTH))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].optionsCount))
	API_FIELD(JSE_String(shell.window[windowID].element[elementID].field, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].fieldPosition))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].listStyle))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].listContent))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].listType))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].listSubtype))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].scroll))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].itemW))
	API_FIELD(JSE_Float(&shell.window[windowID].element[elementID].itemH))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].col))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].row))
	API_FIELD(JSE_Int(&shell.window[windowID].element[elementID].listID))
}
#endif

void VMCall(int func_id) {
	switch(func_id) {
		case VM_CMD: trap_SendConsoleCommand(vmargs.v[0].i, vmargs.v[1].s); break;
#ifdef GAME
		case VM_APIGAMEENTITY: API_GameEntity(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[3].i, vmargs.v[4].i, vmargs.v[5].i, vmargs.v[6].i); break;
		case VM_GAPIENTITYDELETE: GAPI_EntityDelete(vmargs.v[0].i); break;
#endif
#ifndef GAME
		case VM_APICGUI: API_CGUI(vmargs.v[0].i, vmargs.v[2].i, vmargs.v[3].i, vmargs.v[4].i, vmargs.v[5].i); break;
		case VM_APIGLCONFIG: API_GLConfig(vmargs.v[0].i, vmargs.v[2].i, vmargs.v[3].i, vmargs.v[4].i, vmargs.v[5].i); break;
		case VM_DRAWSTRING:
			drawString(vmargs.v[0].f, vmargs.v[1].f, vmargs.v[2].s, vmargs.v[3].i, cgui.colors[vmargs.v[4].i], vmargs.v[5].f, vmargs.v[6].i);
			break;
		case VM_DRAWSTRINGADJUSTED:
			drawStringAdjusted(vmargs.v[0].f, vmargs.v[1].f, vmargs.v[2].s, vmargs.v[3].i, cgui.colors[vmargs.v[4].i], vmargs.v[5].f, vmargs.v[6].i);
			break;
		case VM_DRAWRECTANGLE:
			drawRoundedRect(vmargs.v[0].f, vmargs.v[1].f, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, cgui.colors[vmargs.v[5].i], vmargs.v[6].i);
			break;
		case VM_DRAWSHADER: drawColoredShader(vmargs.v[0].f, vmargs.v[1].f, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].s, cgui.colors[vmargs.v[5].i]); break;
#endif
#ifdef CGAME
		case VM_APIHUD: API_HUD(vmargs.v[0].i, vmargs.v[2].i, vmargs.v[3].i, vmargs.v[4].i, vmargs.v[5].i); break;
		case VM_APICLIENTPLAYER: API_ClientPlayer(vmargs.v[0].i, vmargs.v[2].i, vmargs.v[3].i, vmargs.v[4].i, vmargs.v[5].i); break;
		case VM_HUDCOUNTER: CG_HUDCounter(vmargs.v[0].f, vmargs.v[1].f, vmargs.v[2].s, vmargs.v[3].s); break;
		case VM_NOTIFY: CG_AddNotify(vmargs.v[0].s, vmargs.v[1].i, vmargs.v[2].i, vmargs.v[3].s); break;
#endif
#ifdef UI
		case VM_APISHELL: API_Shell(vmargs.v[0].i, vmargs.v[2].i, vmargs.v[3].i, vmargs.v[4].i, vmargs.v[5].i); break;
		case VM_APIWINDOW: API_Window(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[3].i, vmargs.v[4].i, vmargs.v[5].i, vmargs.v[6].i); break;
		case VM_APIELEMENT: API_Element(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].i, vmargs.v[4].i, vmargs.v[5].i, vmargs.v[6].i, vmargs.v[7].i); break;
		case VM_ITEMFOCUSED:
			vmresult.v.i = UI_ItemFocused(&shell.window[vmargs.v[0].i].element[vmargs.v[1].i]);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CURSORINWINDOW:
			vmresult.v.i = UI_CursorInWindowRect(&shell.window[vmargs.v[0].i], vmargs.v[1].f, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CLOSEWINDOW: UI_CloseWindow(vmargs.v[0].i); break;
		case VM_CLEARWINDOW: UI_ClearWindow(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].i); break;
		case VM_TELEPORTWINDOWTOPLAYER: UI_Move3DWindow(vmargs.v[0].i, vmargs.v[1].f); break;
		case VM_GETFREEELEMENT:
			vmresult.v.i = UI_GetElementFree(vmargs.v[0].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATEWINDOW:
			vmresult.v.i = UI_Window(vmargs.v[0].i,
			                         vmargs.v[1].s,
			                         vmargs.v[2].s,
			                         vmargs.v[3].s,
			                         vmargs.v[4].i,
			                         vmargs.v[5].f,
			                         vmargs.v[6].f,
			                         vmargs.v[7].i,
			                         vmargs.v[8].i,
			                         vmargs.v[9].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATEWINDOWBUTTON:
			vmresult.v.i = UI_WindowButton(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].f, vmargs.v[3].s, vmargs.v[4].i, vmargs.v[5].i, vmargs.v[6].f);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATEPICTURE:
			vmresult.v.i = UI_Picture(vmargs.v[0].i,
			                          vmargs.v[1].i,
			                          vmargs.v[2].f,
			                          vmargs.v[3].f,
			                          vmargs.v[4].f,
			                          vmargs.v[5].f,
			                          vmargs.v[6].s,
			                          vmargs.v[7].i,
			                          vmargs.v[8].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATEBUTTON:
			vmresult.v.i = UI_Button(vmargs.v[0].i,
			                         vmargs.v[1].i,
			                         vmargs.v[2].f,
			                         vmargs.v[3].f,
			                         vmargs.v[4].f,
			                         vmargs.v[5].f,
			                         vmargs.v[6].s,
			                         vmargs.v[7].i,
			                         vmargs.v[8].i,
			                         vmargs.v[9].f);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATECHECKBOX:
			vmresult.v.i = UI_Checkbox(vmargs.v[0].i,
			                           vmargs.v[1].i,
			                           vmargs.v[2].f,
			                           vmargs.v[3].f,
			                           vmargs.v[4].f,
			                           vmargs.v[5].f,
			                           vmargs.v[6].s,
			                           vmargs.v[7].i,
			                           vmargs.v[8].i,
			                           vmargs.v[9].f,
			                           vmargs.v[10].s);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATESLIDER:
			vmresult.v.i = UI_Slider(vmargs.v[0].i,
			                         vmargs.v[1].i,
			                         vmargs.v[2].f,
			                         vmargs.v[3].f,
			                         vmargs.v[4].f,
			                         vmargs.v[5].f,
			                         vmargs.v[6].s,
			                         vmargs.v[7].i,
			                         vmargs.v[8].i,
			                         vmargs.v[9].f,
			                         vmargs.v[10].s,
			                         vmargs.v[11].f,
			                         vmargs.v[12].f,
			                         vmargs.v[13].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATEACTION:
			vmresult.v.i = UI_Action(vmargs.v[0].i,
			                         vmargs.v[1].i,
			                         vmargs.v[2].f,
			                         vmargs.v[3].f,
			                         vmargs.v[4].f,
			                         vmargs.v[5].f,
			                         vmargs.v[6].s,
			                         vmargs.v[7].i,
			                         vmargs.v[8].i,
			                         vmargs.v[9].f,
			                         vmargs.v[10].s);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATESPIN:
			vmresult.v.i = UI_Spin(vmargs.v[0].i,
			                       vmargs.v[1].i,
			                       vmargs.v[2].f,
			                       vmargs.v[3].f,
			                       vmargs.v[4].f,
			                       vmargs.v[5].f,
			                       vmargs.v[6].s,
			                       vmargs.v[7].i,
			                       vmargs.v[8].i,
			                       vmargs.v[9].f,
			                       vmargs.v[10].s,
			                       vmargs.v[11].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATEFIELD:
			vmresult.v.i = UI_Field(vmargs.v[0].i,
			                        vmargs.v[1].i,
			                        vmargs.v[2].f,
			                        vmargs.v[3].f,
			                        vmargs.v[4].f,
			                        vmargs.v[5].f,
			                        vmargs.v[6].s,
			                        vmargs.v[7].i,
			                        vmargs.v[8].i,
			                        vmargs.v[9].f,
			                        vmargs.v[10].s);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATELIST:
			vmresult.v.i = UI_List(vmargs.v[0].i,
			                       vmargs.v[1].i,
			                       vmargs.v[2].f,
			                       vmargs.v[3].f,
			                       vmargs.v[4].f,
			                       vmargs.v[5].f,
			                       vmargs.v[6].f,
			                       vmargs.v[7].i,
			                       vmargs.v[8].i,
			                       vmargs.v[9].i,
			                       vmargs.v[10].i,
			                       vmargs.v[11].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATELISTFILES:
			vmresult.v.i = UI_ListFiles(vmargs.v[0].i,
			                            vmargs.v[1].i,
			                            vmargs.v[2].f,
			                            vmargs.v[3].f,
			                            vmargs.v[4].f,
			                            vmargs.v[5].f,
			                            vmargs.v[6].f,
			                            vmargs.v[7].i,
			                            vmargs.v[8].i,
			                            vmargs.v[9].i,
			                            vmargs.v[10].i,
			                            vmargs.v[11].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_FILLLISTFILES: UI_FillListFiles(vmargs.v[0].i, vmargs.v[1].s, vmargs.v[2].s, vmargs.v[3].s, vmargs.v[4].i); break;
		case VM_GETFILEFROMLIST:
			StringCopy(vmresult.v.s, UI_GetFileFromList(&shell.window[vmargs.v[0].i], vmargs.v[1].i, vmargs.v[2].i), MAX_JS_STRINGSIZE);
			vmresult.t = JS_TYPE_STRING;
			break;
		case VM_GETFILELISTCOUNT:
			vmresult.v.i = UI_GetFileListCount(&shell.window[vmargs.v[0].i], vmargs.v[1].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_SETMARGIN: UI_SetMargin(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, vmargs.v[5].f); break;
#endif
	}
}
