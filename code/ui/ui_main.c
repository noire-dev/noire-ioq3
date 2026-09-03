// Copyright (C) 2023-2026 Noire's Mod (noire.dev) — GPLv2

#include "../qcommon/vm_javascript.h"
#include "../qcommon/vm_javascript_core.h"
#include "../qcommon/q_shared.h"

shell_s shell;

Q_EXPORT intptr_t vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10,
                         int arg11) {  // Точка входа движка (функция должна быть первой)
	switch(command) {
		case UI_GETAPIVERSION: return UI_API_VERSION;
		case UI_INIT: return UI_Init();
		case UI_SHUTDOWN: return UI_Shutdown();
		case UI_KEY_EVENT: return UI_KeyEvent(arg0, arg1, arg2);
		case UI_MOUSE_EVENT: return UI_MouseEvent(arg0, arg1);
		case UI_REFRESH: return UI_Refresh();
		case UI_IS_FULLSCREEN: return UI_IsFullscreen();
		case UI_SET_ACTIVE_MENU: return UI_SetActiveMenu(arg0);
		case UI_CONSOLE_COMMAND: return UI_ConsoleCommand();
		case UI_DRAW_CONNECT_SCREEN: return UI_DrawConnectScreen();
		case UI_HASUNIQUECDKEY: return true;

		case GETVMCONTEXT: VMContext(&vmargs, &vmresult); return 0;
		case VMCALL: VMCall(arg0); return 0;
		default: trap_Error("ui.qvm: unknown command"); break;
	}

	return -1;
}

void QDECL Com_Error(int level, const char* error, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL Com_Printf(const char* msg, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(text);
}

#define POOLSIZE (1024 * 1024) * 4
static char memoryPool[POOLSIZE];
static int allocPoint;

void* UI_Alloc(int size) {
	char* p;

	if(allocPoint + size > POOLSIZE) return NULL;

	p = &memoryPool[allocPoint];
	allocPoint += (size + 31) & ~31;
	return p;
}

void UI_InitMemory(void) { allocPoint = 0; }

// Quake 3 Content Format
#define DIRLIST_SIZE 65536

static int ui_numBots;
static char* ui_botInfos[MAX_BOTS];

static int ui_numArenas;
static char* ui_arenaInfos[MAX_ARENAS];

static char dirlist[DIRLIST_SIZE];

static int UI_ParseInfos(char* buf, int max, char* infos[], bool arenas) {
	char* token;
	int count;
	char key[MAX_TOKEN_CHARS], info[MAX_INFO_STRING];

	count = 0;

	while(1) {
		token = COM_Parse(&buf);
		if(!token[0]) break;
		if(strcmp(token, "{")) {
			trap_Print("Missing { in info file\n");
			break;
		}
		if(count == max) {
			trap_Print("Max infos exceeded\n");
			break;
		}

		info[0] = '\0';
		while(1) {
			token = COM_ParseExt(&buf, true);
			if(!token[0]) {
				trap_Print("Unexpected end of info file\n");
				break;
			}
			if(!strcmp(token, "}")) break;
			Q_strncpyz(key, token, sizeof(key));
			token = COM_ParseExt(&buf, false);
			if(!token[0]) strcpy(token, "<NULL>");
			if(!arenas || (!strcmp(key, "map") || !strcmp(key, "type"))) Info_SetValueForKey(info, key, token);
		}
		infos[count] = UI_Alloc(strlen(info) + 1);
		if(infos[count]) {
			strcpy(infos[count], info);
			count++;
		}
	}
	return count;
}

static void UI_LoadArenasFromFile(char* filename) {
	int len;
	fileHandle_t f;
	char buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(!f) {
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}
	if(len >= MAX_ARENAS_TEXT) {
		trap_Print(va(S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT));
		trap_FS_FCloseFile(f);
		return;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	ui_numArenas += UI_ParseInfos(buf, MAX_ARENAS - ui_numArenas, &ui_arenaInfos[ui_numArenas], true);
}

static void UI_LoadArenas(void) {
	int numdirs, i, dirlen;
	char filename[128], dirlist[1024];
	char* dirptr;

	ui_numArenas = 0;

	numdirs = trap_FS_GetFileList("scripts", ".arena", dirlist, 1024);
	dirptr = dirlist;
	for(i = 0; i < numdirs; i++, dirptr += dirlen + 1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		UI_LoadArenasFromFile(filename);
	}
}

static void UI_LoadBotsFromFile(char* filename) {
	int len;
	fileHandle_t f;
	char buf[MAX_BOTS_TEXT];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(!f) {
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}
	if(len >= MAX_BOTS_TEXT) {
		trap_Print(va(S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT));
		trap_FS_FCloseFile(f);
		return;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	ui_numBots += UI_ParseInfos(buf, MAX_BOTS - ui_numBots, &ui_botInfos[ui_numBots], false);
}

static void UI_LoadBots(void) {
	int numdirs;
	char filename[128];
	char* dirptr;
	int i;
	int dirlen;

	ui_numBots = 0;

	numdirs = trap_FS_GetFileList("scripts", ".bot", dirlist, DIRLIST_SIZE);
	dirptr = dirlist;
	for(i = 0; i < numdirs; i++, dirptr += dirlen + 1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		UI_LoadBotsFromFile(filename);
	}
}

static char* UI_GetBotInfoByName(const char* name) {
	int n;
	char* value;

	for(n = 0; n < ui_numBots; n++) {
		value = Info_ValueForKey(ui_botInfos[n], "name");
		if(!Q_stricmp(value, name)) {
			return ui_botInfos[n];
		}
	}

	return NULL;
}

static bool MapHasGametype(char* typeString, const char* mode) {
	char* token;

	while(1) {
		token = COM_ParseExt(&typeString, false);
		if(!typeString || !token[0]) break;
		if(!Q_stricmp(token, mode)) return true;
	}

	return false;
}

static char mapListCache[MAX_ARENAS][64];
static int mapListCacheCount = 0;
static char mapListCacheGametype[64];

static void UI_BuildMapCache(char* gametype) {
	int i, count = 0;
	char *map, *type;

	for(i = 0; i < ui_numArenas; i++) {
		map = Info_ValueForKey(ui_arenaInfos[i], "map");
		type = Info_ValueForKey(ui_arenaInfos[i], "type");

		if(MapHasGametype(type, gametype)) {
			StringCopy(mapListCache[count], map, 64);
			count++;
		}
	}
	mapListCacheCount = count;
	StringCopy(mapListCacheGametype, gametype, 64);
}

static char* UI_MapForID(int id, char* gametype) {
	if(strcmp(mapListCacheGametype, gametype)) UI_BuildMapCache(gametype);
	if(id >= 0 && id < mapListCacheCount) return mapListCache[id];
	return "Empty";
}

static int UI_CountOfMaps(char* gametype) {
	if(strcmp(mapListCacheGametype, gametype)) UI_BuildMapCache(gametype);
	return mapListCacheCount;
}

#define MAX_PLAYERMODELS 4096
static int ui_numPlayerModels = 0;
static char ui_playerModelNames[MAX_PLAYERMODELS][MAX_QPATH];
static char ui_playerModelIcons[MAX_PLAYERMODELS][MAX_QPATH];

static char pm_dirlist[131072];
static char pm_filelist[131072];

static void UI_LoadPlayerModels(void) {
	int numdirs, numfiles;
	char skinname[512];
	char *dirptr, *fileptr;
	int i, j;
	int dirlen, filelen;

	ui_numPlayerModels = 0;

	numdirs = trap_FS_GetFileList("models/players", "/", pm_dirlist, 131072);
	dirptr = pm_dirlist;
	for(i = 0; i < numdirs && ui_numPlayerModels < MAX_PLAYERMODELS; i++, dirptr += dirlen + 1) {
		dirlen = strlen(dirptr);
		if(dirlen && dirptr[dirlen - 1] == '/') dirptr[dirlen - 1] = '\0';
		if(!strcmp(dirptr, ".") || !strcmp(dirptr, "..")) continue;

		numfiles = trap_FS_GetFileList(va("models/players/%s", dirptr), "", pm_filelist, 131072);
		fileptr = pm_filelist;
		for(j = 0; j < numfiles && ui_numPlayerModels < MAX_PLAYERMODELS; j++, fileptr += filelen + 1) {
			filelen = strlen(fileptr);
			COM_StripExtension(fileptr, skinname, sizeof(skinname));
			if(!Q_stricmpn(skinname, "icon_", 5)) {
				Com_sprintf(ui_playerModelNames[ui_numPlayerModels], sizeof(ui_playerModelNames[ui_numPlayerModels]), "%s/%s", dirptr, skinname + 5);
				Com_sprintf(ui_playerModelIcons[ui_numPlayerModels], sizeof(ui_playerModelIcons[ui_numPlayerModels]), "models/players/%s/%s", dirptr, skinname);
				ui_numPlayerModels += 1;
			}
		}
	}
}

static int UI_ListGetCount(int listType, int listSubtype) {
	if(listType == LTYPE_APPS) return shell.appCount;
	if(listType == LTYPE_MAPS) return UI_CountOfMaps(gametypes_mapnames[listSubtype]);
	if(listType == LTYPE_BOTS) return UI_CountOfMaps(gametypes_mapnames[listSubtype]);
	if(listType == LTYPE_PLAYERMODELS) return ui_numPlayerModels;
	if(listType == LTYPE_GAMEITEMS) return bg_numItems - 1;
	if(listType == LTYPE_TOOLS) return shell.toolCount;
	return 0;
}

static char* UI_ListGetCallbackName(int listType, int listSubtype, int index) {
	if(listType == LTYPE_APPS) return shell.app[index].name;
	if(listType == LTYPE_MAPS) return UI_MapForID(index, gametypes_mapnames[listSubtype]);
	if(listType == LTYPE_BOTS) return UI_MapForID(index, gametypes_mapnames[listSubtype]);
	if(listType == LTYPE_PLAYERMODELS) return ui_playerModelNames[index];
	if(listType == LTYPE_GAMEITEMS) return bg_itemlist[index + 1].pickup_name;
	if(listType == LTYPE_TOOLS) return shell.tool[index].nameID;
	return "";
}

static char* UI_ListGetName(int listType, int listSubtype, int index) {
	if(listType == LTYPE_APPS) return shell.app[index].name;
	if(listType == LTYPE_MAPS) return UI_MapForID(index, gametypes_mapnames[listSubtype]);
	if(listType == LTYPE_BOTS) return UI_MapForID(index, gametypes_mapnames[listSubtype]);
	if(listType == LTYPE_PLAYERMODELS) return ui_playerModelNames[index];
	if(listType == LTYPE_GAMEITEMS) return bg_itemlist[index + 1].pickup_name;
	if(listType == LTYPE_TOOLS) return shell.tool[index].name;
	return "";
}

static char* UI_ListGetIcon(int listType, int listSubtype, int index) {
	if(listType == LTYPE_APPS) return shell.app[index].icon;
	if(listType == LTYPE_MAPS) return va("levelshots/%s", UI_MapForID(index, gametypes_mapnames[listSubtype]));
	if(listType == LTYPE_BOTS) return va("levelshots/%s", UI_MapForID(index, gametypes_mapnames[listSubtype]));
	if(listType == LTYPE_PLAYERMODELS) return ui_playerModelIcons[index];
	if(listType == LTYPE_GAMEITEMS) return bg_itemlist[index + 1].icon;
	if(listType == LTYPE_TOOLS) return "";
	return "";
}

static void UI_Status(void) {  // Проверка статуса UI
	char svinfo[MAX_INFO_STRING];
	uiClientState_t cstate;

	trap_GetClientState(&cstate);

	trap_GetConfigString(CS_SERVERINFO, svinfo, MAX_INFO_STRING);
	shell.onMap = (strlen(svinfo) <= 0 && cstate.connState < CA_LOADING) ? false : true;
}

static void UI_OpenShell(void) {  // Открытие UI
	trap_Cvar_Set("ui.control", "0");
	UI_Status();
	if(trap_Key_GetCatcher() & KEYCATCH_UI) return;
	trap_Key_SetCatcher(KEYCATCH_UI);
}

static void UI_CloseShell(void) {  // Закрытие UI (состояние сохраняется)
	trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
	trap_Key_ClearStates();
	shell.rendered = false;
}

static void UI_ShellInit(void) {  // Инициализация Shell
	int i;

	memset(&shell, 0, sizeof(shell));
	shell.scale = trap_Cvar_VariableValue("shell.global.scale");
	shell.windowCorner = trap_Cvar_VariableValue("shell.window.corner");
	shell.windowOutline = trap_Cvar_VariableValue("shell.window.outline");
	shell.windowColoredOutline = trap_Cvar_VariableValue("shell.window.coloredOutline");
	UI_Status();
	JS_ShellInit();
	for(i = 0; i < SHELL_MAX_WINDOWS; i++) shell.zOrder[i] = i;
}

static void UI_LoadApps(void) {  // Кэширует список приложений из JS
	int i;

	for(i = 0; i < SHELL_MAX_APPS; i++) {
		shell.appCount = i;
		if(JS_GetApp(i) == NONE) break;
		StringCopy(shell.app[i].name, JS_GetAppName(i), MAX_JS_STRINGSIZE);
		StringCopy(shell.app[i].icon, JS_GetAppIcon(i), MAX_JS_STRINGSIZE);
	}
}

static void UI_LoadTools(void) {  // Кэширует список инструментов из JS
	int i;

	for(i = 0; i < SHELL_MAX_TOOLS; i++) {
		shell.toolCount = i;
		if(JS_GetTool(i) == NONE) break;
		StringCopy(shell.tool[i].nameID, JS_GetToolNameID(i), MAX_JS_STRINGSIZE);
		StringCopy(shell.tool[i].name, JS_GetToolName(i), MAX_JS_STRINGSIZE);
	}
}

static void UI_HotReload(void) {  // Перезапуск JS контекста и повторная инициализация Shell
	trap_SendConsoleCommand(EXEC_NOW, "js.restart \n");
	UI_Shutdown();
	UI_Init();
}

int UI_Init(void) {  // Инициализация UI и загрузка состояния
	fileHandle_t stateFile = 0;
	int stateLen = 0;
	int i;
	int lastWindow = 0;

	trap_Print("NMRE: init... \n");
	CL_Init();
	UI_ShellInit();
	UI_LoadApps();
	UI_LoadTools();
	UI_LoadArenas();
	UI_LoadBots();
	// UI_LoadPlayerModels();

	for(i = SHELL_MAX_WINDOWS - 1; i > 0; i--) {
		if(shell.window[i].created) {
			lastWindow = i + 1;
			break;
		}
	}

	stateLen = trap_FS_FOpenFile(SHELL_STATE_FILE, &stateFile, FS_READ);
	if(stateFile && stateLen > 0) {
		trap_Print("NMRE: loading state... \n");
		trap_FS_Read(shell.saveState, sizeof(shell.saveState), stateFile);

		for(i = 0; i < SHELL_MAX_WINDOWS; i++) {
			window_s* window = &shell.window[lastWindow];
			saveState_s* savedWindow = &shell.saveState[i];

			if(!savedWindow->saved) continue;

			JS_LaunchApp(savedWindow->nameID);

			window->x = savedWindow->x;
			window->y = savedWindow->y;
			window->scaleFactor = savedWindow->scaleFactor;
			lastWindow += 1;
		}
		trap_FS_FCloseFile(stateFile);
		trap_Print("NMRE: state loaded! \n");
	}

	return 0;
}

int UI_Shutdown(void) {  // Выключение UI и сохранение состояния
	fileHandle_t stateFile = 0;
	int i;

	trap_Print("NMRE: shutdown... \n");
	trap_Print("NMRE: saving state... \n");

	memset(&shell.saveState, 0, sizeof(shell.saveState));
	for(i = 0; i < SHELL_MAX_WINDOWS; i++) {
		window_s* window = &shell.window[shell.zOrder[i]];
		saveState_s* savedWindow = &shell.saveState[(SHELL_MAX_WINDOWS - 1) - i];

		if(!window->created) continue;
		if(window->style & UI_NOSAVE) continue;
		if(!strlen(window->nameID)) continue;
		savedWindow->saved = true;
		StringCopy(savedWindow->nameID, window->nameID, MAX_JS_STRINGSIZE);
		savedWindow->x = window->x;
		savedWindow->y = window->y;
		savedWindow->scaleFactor = window->scaleFactor;
	}
	trap_FS_FOpenFile(SHELL_STATE_FILE, &stateFile, FS_WRITE);
	trap_FS_Write(&shell.saveState, sizeof(shell.saveState), stateFile);
	trap_FS_FCloseFile(stateFile);
	trap_Print("NMRE: state saved! \n");

	for(i = 0; i < SHELL_MAX_WINDOWS; i++) {
		if(!shell.window[i].created || !strlen(shell.window[i].nameID)) continue;
		JS_ShellShutdown(i);
	}

	return 0;
}

static void UI_AdjustToWindow(element_s* element) {  // Локальная позиция внутри окна к глобальной позиции
	window_s* window = (window_s*)element->parentWindow;
	float scale = window->scaleFactor;
	int i;

	element->x = (element->baseX * scale) + window->x;
	element->y = (element->baseY * scale) + window->y;
	element->w = element->baseW * scale;
	element->h = element->baseH * scale;
	element->scale = element->baseScale * scale;
	element->corner = element->baseCorner * scale;
	element->margin = element->baseMargin * scale;
	if(element->type == ETYPE_LIST || element->type == ETYPE_LISTFILES) {
		element->w = (element->baseW * element->col) * scale;
		element->h = (element->baseH * element->row) * scale;
		element->itemW = element->baseW * scale;
		element->itemH = element->baseH * scale;
		for(i = 0; i < 4; i++) element->margin4[i] = element->baseMargin4[i] * scale;
	}
	if(element->type == ETYPE_WINDOWBUTTON) {
		float posX = window->x + window->w;
		float marginX = 8 * shell.scale;
		float buttonX = posX - ((element->baseH + marginX) * (element->baseX + 1));

		element->x = buttonX;
		element->y = element->baseY + window->y;
		element->w = element->baseW;
		element->h = element->baseH;
		element->scale = element->baseScale * shell.scale;
		element->corner = element->baseCorner;
	}
}

static void UI_ScaleToWindow(window_s* window) {  // Применение масштабирования к окну
	if(window->style & UI_NOSCALE) window->scaleFactor = 1.00;
	window->w = window->baseW * window->scaleFactor;
	window->h = window->baseH * window->scaleFactor;
}

static bool UI_ElementIsVisible(element_s* element) {  // Проверка видимости элемента
	if(element->x > glconfig.vidWidth) return false;   // проверка +X
	if(element->y > glconfig.vidHeight) return false;  // проверка +Y
	if((element->x + element->w) < 0) return false;    // проверка -X
	if((element->y + element->h) < 0) return false;    // проверка -Y
	return true;
}

static bool UI_CursorOnItem(element_s* element) {  // Проверка мыши на элементе
	window_s* window = (window_s*)element->parentWindow;

	if(shell.cursorX < element->x || shell.cursorY < element->y || shell.cursorX > element->x + element->w || shell.cursorY > element->y + element->h) return false;
	return true;
}

bool UI_ItemFocused(element_s* element) {  // Проверка мыши на элементе в окне
	window_s* window = (window_s*)element->parentWindow;

	if(shell.focusedWindow != window->id) return false;
	if(window->focusedElement != element->id) return false;

	return true;
}

static bool UI_CursorInRect(float x, float y, float w, float h) {  // Проверка мыши в области
	if(shell.cursorX < x || shell.cursorY < y || shell.cursorX > x + w || shell.cursorY > y + h) return false;
	return true;
}

bool UI_CursorInWindowRect(window_s* window, float x, float y, float w, float h) {  // Проверка мыши в координатах окна
	if(shell.activeWindow != window->id) return false;
	if(shell.cursorX < (x * window->scaleFactor) + window->x || shell.cursorY < (y * window->scaleFactor) + window->y || shell.cursorX > ((x * window->scaleFactor) + window->x) + w * window->scaleFactor || shell.cursorY > ((y * window->scaleFactor) + window->y) + h * window->scaleFactor) return false;
	return true;
}

static float UI_SliderValue(float min, float max, float x, float w) {  // Значение слайдера из позиции мыши
	float local_x = (float)shell.cursorX - x;

	if(local_x < 0.0f) local_x = 0.0f;
	if(local_x > w) local_x = w;

	return min + (local_x / w) * (max - min);
}

static void UI_GeneralCallback(element_s* e) {  // Обработка активации элемента
	switch(e->type) {
		case ETYPE_CHECKBOX:
			e->value = (e->value > 0) ? 0 : 1;
			if(strlen(e->cvar)) trap_Cvar_Set(e->cvar, va("%f", e->value));
			break;
		case ETYPE_SLIDER:
			if(UI_CursorOnItem(e)) {
				if(e->mode == EMODE_INT) e->value = (int)(UI_SliderValue(e->min, e->max, e->x + (e->w * 0.5), (e->w * 0.5) - e->margin));
				if(e->mode == EMODE_FLOAT) e->value = UI_SliderValue(e->min, e->max, e->x + (e->w * 0.5), (e->w * 0.5) - e->margin);
			}
			if(strlen(e->cvar)) trap_Cvar_Set(e->cvar, va("%f", e->value * e->valueMod));
			break;
		case ETYPE_ACTION: shell.window[shell.activeWindow].keyCapture = e->id; break;
		case ETYPE_SPIN:
			e->value = (e->value < e->optionsCount - 1) ? e->value + 1 : 0;
			if(e->mode == EMODE_NUMBER)
				trap_Cvar_Set(e->cvar, va("%f", e->value * e->valueMod));
			else if(e->mode == EMODE_STRING)
				trap_Cvar_Set(e->cvar, va("%s", e->options[(int)e->value]));
			break;
		case ETYPE_FIELD: shell.window[shell.activeWindow].keyCapture = e->id; break;
		case ETYPE_LIST: {
			int i, j;
			int currentItem = e->scroll;

			for(i = 0; i < e->row; i++) {
				for(j = 0; j < e->col; j++) {
					if(currentItem >= UI_ListGetCount(e->listType, e->listSubtype)) break;
					if(UI_CursorInRect(e->x + (e->itemW * j), e->y + (e->itemH * i), e->itemW, e->itemH)) {
						e->value = currentItem;
						StringCopy(e->field, UI_ListGetCallbackName(e->listType, e->listSubtype, currentItem), MAX_JS_STRINGSIZE);
						break;
					}
					currentItem += 1;
				}
			}
			break;
		}
		case ETYPE_LISTFILES: {
			window_s* window = (window_s*)e->parentWindow;
			int i, j, currentItem = 0;
			char* file = window->fileList[e->listID].list;
			char nameWithoutExt[MAX_JS_STRINGSIZE];
			int filelen = 0;

			for(i = 0; i < e->scroll; i++) {
				if(i >= window->fileList[e->listID].count) break;
				file += strlen(file) + 1;
			}

			for(i = 0; i < e->row; i++) {
				for(j = 0; j < e->col; j++) {
					if(currentItem >= window->fileList[e->listID].count) break;
					filelen = strlen(file);
					if(filelen && file[filelen - 1] == '/') file[filelen - 1] = ' ';
					if(!strcmp(file, "") || !strcmp(file, ".") || !strcmp(file, "..")) {
						currentItem++;
						file += strlen(file) + 1;
						j--;
						continue;
					}
					COM_StripExtension(file, nameWithoutExt, MAX_JS_STRINGSIZE);
					if(UI_CursorInRect(e->x + (e->itemW * j), e->y + (e->itemH * i), e->itemW, e->itemH)) {
						e->value = currentItem;
						if(strcmp(window->fileList[e->listID].ext, "/"))
							StringCopy(e->field, nameWithoutExt, MAX_JS_STRINGSIZE);
						else
							StringCopy(e->field, file, MAX_JS_STRINGSIZE);
						break;
					}
					currentItem += 1;
					file += strlen(file) + 1;
				}
			}
			break;
		}
		default: break;
	}
}

static void UI_SecondaryCallback(element_s* e) {  // Обработка альтернативной активации элемента
	int i;

	switch(e->type) {
		case ETYPE_CHECKBOX:
			if(strlen(e->cvar)) trap_Cvar_Set(e->cvar, NULL);
			e->value = trap_Cvar_VariableValue(e->cvar);
			break;
		case ETYPE_SLIDER:
			if(strlen(e->cvar)) trap_Cvar_Set(e->cvar, NULL);
			e->value = trap_Cvar_VariableValue(e->cvar);
			break;
		case ETYPE_SPIN:
			if(strlen(e->cvar)) trap_Cvar_Set(e->cvar, NULL);
			if(e->mode == EMODE_NUMBER) {
				e->value = trap_Cvar_VariableValue(e->cvar);
			} else if(e->mode == EMODE_STRING) {
				for(i = 0; i < e->optionsCount; i++) {
					if(!strcmp(trap_Cvar_VariableString(e->cvar), e->options[i])) e->value = i;
				}
			}
			break;
		case ETYPE_FIELD:
			if(strlen(e->cvar)) trap_Cvar_Set(e->cvar, NULL);
			StringCopy(e->field, trap_Cvar_VariableString(e->cvar), MAX_JS_STRINGSIZE);
			break;
		default: break;
	}
}

static void UI_KeyCallback(element_s* e, int key) {  // Обработка клавиш для элемента
	window_s* window = (window_s*)e->parentWindow;

	switch(e->type) {
		case ETYPE_LIST:
			if(key == K_MWHEELUP && e->scroll - e->col >= 0) e->scroll -= e->col;
			if(key == K_MWHEELDOWN && e->scroll + (e->col * e->row) < UI_ListGetCount(e->listType, e->listSubtype)) e->scroll += e->col;
			break;
		case ETYPE_LISTFILES:
			if(key == K_MWHEELUP && e->scroll - e->col >= 0) e->scroll -= e->col;
			if(key == K_MWHEELDOWN && e->scroll + (e->col * e->row) < window->fileList[e->listID].count) e->scroll += e->col;
			break;
		default: break;
	}
}

static void UI_WindowToTop(int windowID) {  // Перенос окна на пережний план
	int pos, temp, i;

	if(shell.window[windowID].style & UI_NOZORDER) return;
	for(pos = 0; pos < SHELL_MAX_WINDOWS; pos++) {
		if(shell.zOrder[pos] == windowID) break;
	}
	if(pos == SHELL_MAX_WINDOWS) return;

	temp = shell.zOrder[pos];
	for(i = pos; i < SHELL_MAX_WINDOWS - 1; i++) shell.zOrder[i] = shell.zOrder[i + 1];
	shell.zOrder[SHELL_MAX_WINDOWS - 1] = temp;
}

void UI_Move3DWindow(int windowID, float distance) {  // 3D окна в координатах мира
	window_s* window = &shell.window[windowID];
	vec3_t viewOrg;
	vec3_t viewAxis[3];
	vec3_t worldOrg;

	viewOrg[0] = trap_Cvar_VariableValue("cgame.vieworg[0]");
	viewOrg[1] = trap_Cvar_VariableValue("cgame.vieworg[1]");
	viewOrg[2] = trap_Cvar_VariableValue("cgame.vieworg[2]");

	viewAxis[0][0] = trap_Cvar_VariableValue("cgame.viewaxis[0][0]");
	viewAxis[0][1] = trap_Cvar_VariableValue("cgame.viewaxis[0][1]");
	viewAxis[0][2] = trap_Cvar_VariableValue("cgame.viewaxis[0][2]");
	viewAxis[1][0] = trap_Cvar_VariableValue("cgame.viewaxis[1][0]");
	viewAxis[1][1] = trap_Cvar_VariableValue("cgame.viewaxis[1][1]");
	viewAxis[1][2] = trap_Cvar_VariableValue("cgame.viewaxis[1][2]");
	viewAxis[2][0] = trap_Cvar_VariableValue("cgame.viewaxis[2][0]");
	viewAxis[2][1] = trap_Cvar_VariableValue("cgame.viewaxis[2][1]");
	viewAxis[2][2] = trap_Cvar_VariableValue("cgame.viewaxis[2][2]");

	if(distance < 0.01)
		VectorMA(viewOrg, window->worldMoveDist, viewAxis[0], worldOrg);
	else
		VectorMA(viewOrg, distance, viewAxis[0], worldOrg);
	window->worldX = worldOrg[0];
	window->worldY = worldOrg[1];
	window->worldZ = worldOrg[2];
}

int UI_KeyEvent(int key, int isDown, int isChar) {  // [SAFE] Обработка событий ввода UI
	element_s* element = NULL;
	window_s* activeWindow = NULL;
	window_s* focusedWindow = NULL;
	bool keyCapture = false;
	int i;

	if(!isDown) {  // Обработка поднятия клавиш
		if(key == K_MOUSE1) shell.cursorIsDragging = false;
		if(key == K_ALT) shell.cursorIsMovingDesktop = false;
		return 0;
	}

	if(key == K_MOUSE1) {  // Активация окна при нажатии мыши
		shell.cursorIsDragging = true;
		shell.activeWindow = shell.focusedWindow;
		if(shell.activeWindow != NONE) UI_WindowToTop(shell.activeWindow);
	}

	if(key == K_ALT) shell.cursorIsMovingDesktop = true;

	if(shell.activeWindow != NONE) activeWindow = &shell.window[shell.activeWindow];
	if(shell.focusedWindow != NONE) focusedWindow = &shell.window[shell.focusedWindow];

	if(focusedWindow != NULL && !(focusedWindow->style & UI_NOSCALE)) {  // Изменения размера окна колесиком мыши
		if(focusedWindow->linked) {
			if(key == K_MWHEELDOWN && shell.cursorIsMovingDesktop) {
				focusedWindow->worldScale = clamp(focusedWindow->worldScale - 0.050000, 0.50, 50.00);
				return 0;
			}
			if(key == K_MWHEELUP && shell.cursorIsMovingDesktop) {
				focusedWindow->worldScale = clamp(focusedWindow->worldScale + 0.050000, 0.50, 50.00);
				return 0;
			}
		} else {
			if(key == K_MWHEELDOWN && shell.cursorY < focusedWindow->y) {
				focusedWindow->scaleFactor = clamp(focusedWindow->scaleFactor - 0.050000, 0.50, 50.00);
				return 0;
			}
			if(key == K_MWHEELUP && shell.cursorY < focusedWindow->y) {
				focusedWindow->scaleFactor = clamp(focusedWindow->scaleFactor + 0.050000, 0.50, 50.00);
				return 0;
			}
		}
	}

	if(activeWindow != NULL && activeWindow->keyCapture != NONE && key != K_MOUSE1) {  // Выбор элемента
		element = &activeWindow->element[activeWindow->keyCapture];
		keyCapture = true;
	} else if(activeWindow != NULL) {
		if(activeWindow->focusedElement != NONE) element = &activeWindow->element[activeWindow->focusedElement];
	}

	if(keyCapture && element->type == ETYPE_ACTION) {
		if(key != K_ESCAPE) {
			for(i = 0; i < MAX_KEYS; i++) {
				if(!strcmp(element->cvar, cgui.binds[i])) trap_Key_SetBinding(i, "");
			}
			trap_Key_SetBinding(key, element->cvar);
			for(i = 0; i < MAX_KEYS; i++) {
				trap_Key_GetBindingBuf(i, cgui.binds[i], 256);
				if(!strcmp(element->cvar, cgui.binds[i])) trap_Key_KeynumToStringBuf(i, element->bind, 256);
			}
			activeWindow->keyCapture = NONE;
		}
		return 0;
	} else if(keyCapture && element->type == ETYPE_FIELD) {
		int len;
		len = strlen(element->field);

		if(isChar && (key < 0 || key >= 32) && key != 127 && len < MAX_JS_STRINGSIZE - 1) {
			char utf8[5] = {0};
			int bytes = 0;

			if(key < 0x80) {
				utf8[0] = (char)key;
				bytes = 1;
			} else if(key < 0x800) {
				utf8[0] = 0xC0 | (key >> 6);
				utf8[1] = 0x80 | (key & 0x3F);
				bytes = 2;
			} else if(key < 0x10000) {
				utf8[0] = 0xE0 | (key >> 12);
				utf8[1] = 0x80 | ((key >> 6) & 0x3F);
				utf8[2] = 0x80 | (key & 0x3F);
				bytes = 3;
			} else if(key < 0x110000) {
				utf8[0] = 0xF0 | (key >> 18);
				utf8[1] = 0x80 | ((key >> 12) & 0x3F);
				utf8[2] = 0x80 | ((key >> 6) & 0x3F);
				utf8[3] = 0x80 | (key & 0x3F);
				bytes = 4;
			}

			if(element->fieldPosition < 0) element->fieldPosition = len;
			if(element->fieldPosition <= len && len + bytes <= MAX_JS_STRINGSIZE - 1) {
				memmove(element->field + element->fieldPosition + bytes, element->field + element->fieldPosition, len - element->fieldPosition + 1);
				memcpy(element->field + element->fieldPosition, utf8, bytes);
				element->fieldPosition += bytes;
				element->field[len + bytes] = '\0';
			}
		} else if(!isChar) {
			len = strlen(element->field);
			if(element->fieldPosition < 0) element->fieldPosition = len;

			if(key == K_LEFTARROW) {
				if(element->fieldPosition > 0) {
					element->fieldPosition--;
					while(element->fieldPosition > 0 && (element->field[element->fieldPosition] & 0xC0) == 0x80) element->fieldPosition--;
				}
			} else if(key == K_RIGHTARROW) {
				if(element->fieldPosition < len) {
					element->fieldPosition++;
					while(element->fieldPosition < len && (element->field[element->fieldPosition] & 0xC0) == 0x80) element->fieldPosition++;
				}
			} else if(key == K_BACKSPACE) {
				if(element->fieldPosition > 0) {
					int start = element->fieldPosition - 1;
					while(start > 0 && (element->field[start] & 0xC0) == 0x80) start--;
					memmove(element->field + start, element->field + element->fieldPosition, len - element->fieldPosition + 1);
					element->fieldPosition = start;
				}
			} else if(key == K_HOME) {
				element->fieldPosition = 0;
			} else if(key == K_END) {
				element->fieldPosition = len;
			} else if(key == K_DEL) {
				UI_KeyEvent(K_RIGHTARROW, true, false);
				UI_KeyEvent(K_BACKSPACE, true, false);
			} else if(key == K_ENTER) {
				if(strlen(element->cvar)) trap_Cvar_Set(element->cvar, element->field);
				JS_ShellCallback(activeWindow->id, element->id, key);
			} else if(key == K_ESCAPE) {
				if(strlen(element->cvar)) trap_Cvar_Set(element->cvar, element->field);
				JS_ShellCallback(activeWindow->id, element->id, key);
			}
		}
	}

	if(activeWindow != NULL && (key == K_ESCAPE || key == K_ENTER)) activeWindow->keyCapture = NONE;

	if(!keyCapture) {
		switch(key) {  // Обработка глобальных клавиш
			case K_ESCAPE:
				if(shell.rendered) UI_CloseShell();
				break;
			case K_F5: UI_HotReload(); break;
			case K_F12: shell.debug = !shell.debug; break;
		}
	}

	if(activeWindow != NULL) {
		if((key == K_SPACE && !isChar) || (key != K_SPACE)) {
			JS_ShellKey(key, activeWindow->id);
		}
	}

	if(element != NULL) {  // Обработка нажатий по элементам
		switch(key) {
			case K_MOUSE1:
				if(UI_CursorOnItem(element)) UI_GeneralCallback(element);
				break;
			case K_MOUSE2:
				if(UI_CursorOnItem(element)) UI_SecondaryCallback(element);
				break;
			default: UI_KeyCallback(element, key); break;
		}
		if(UI_CursorOnItem(element) && element->type != ETYPE_FIELD) JS_ShellCallback(activeWindow->id, element->id, key);
	}

	return 0;
}

int UI_MouseEvent(int dx, int dy) {  // Обработка событий мыши UI
	int i;

	for(i = SHELL_MAX_WINDOWS - 1; i >= 0; i--) {  // Проверка и обработка перетаскивания окна
		window_s* window = &shell.window[shell.zOrder[i]];

		if(trap_Cvar_VariableIntegerValue("ui.control")) break;
		if(!shell.cursorIsDragging) break;
		if(!window->minimized && UI_CursorInRect(window->x, window->y, window->w, window->h)) break;
		if(window->style & UI_NOTITLE) continue;

		if(UI_CursorInRect(window->x, window->y - UI_WINDOW_TITLE_HEIGHT, window->w, UI_WINDOW_TITLE_HEIGHT)) {
			window->x = clamp(window->x + dx, 0 - (window->w * 0.5), glconfig.vidWidth - (window->w * 0.5));
			window->y = clamp(window->y + dy, 0 + UI_WINDOW_TITLE_HEIGHT, glconfig.vidHeight);
			if(window->x >= -10 * cgui.scale && window->x <= 10 * cgui.scale && abs(dx) <= 1) window->x = 0;
			if(window->x >= glconfig.vidWidth - window->w - 10 * cgui.scale && window->x <= glconfig.vidWidth - window->w + 10 * cgui.scale && abs(dx) <= 1) window->x = glconfig.vidWidth - window->w;
			if(window->y >= glconfig.vidHeight - window->h - 10 * cgui.scale && window->y <= glconfig.vidHeight - window->h + 10 * cgui.scale && abs(dy) <= 1) window->y = glconfig.vidHeight - window->h;
			break;
		}
	}

	if(shell.cursorIsMovingDesktop) {
		for(i = SHELL_MAX_WINDOWS - 1; i >= 0; i--) {
			window_s* moveWindow = &shell.window[shell.zOrder[i]];
			if(moveWindow->style & UI_NOSAVE) continue;
			moveWindow->x = moveWindow->x - dx;
			moveWindow->y = moveWindow->y - dy;
		}
	} else {
		shell.cursorX = clamp(shell.cursorX + dx, 0, glconfig.vidWidth);
		shell.cursorY = clamp(shell.cursorY + dy, 0, glconfig.vidHeight);
	}

	shell.focusedWindow = NONE;
	for(i = SHELL_MAX_WINDOWS - 1; i >= 0; i--) {  // Поиск окна под курсором
		window_s* window = &shell.window[shell.zOrder[i]];

		if(!window->created) continue;
		if(trap_Cvar_VariableIntegerValue("ui.control") && !window->linked) continue;

		if(UI_CursorInRect(window->x, window->y - UI_WINDOW_TITLE_HEIGHT, window->w, UI_WINDOW_TITLE_HEIGHT)) {
			shell.focusedWindow = window->id;
			if(trap_Cvar_VariableIntegerValue("ui.control") && window->linked) shell.activeWindow = window->id;
			break;
		}
		if(!window->minimized && UI_CursorInRect(window->x, window->y, window->w, window->h)) {
			shell.focusedWindow = window->id;
			if(trap_Cvar_VariableIntegerValue("ui.control") && window->linked) shell.activeWindow = window->id;
			break;
		}
	}

	shell.window[shell.focusedWindow].focusedElement = NONE;
	for(i = WINDOW_MAX_ELEMENTS - 1; i >= 0; i--) {  // Поиск элемента под курсором
		window_s* window = &shell.window[shell.focusedWindow];

		if(shell.focusedWindow == NONE) break;

		if(UI_CursorOnItem(&window->element[i])) {
			window->focusedElement = i;
			break;
		}
	}

	if(trap_Cvar_VariableIntegerValue("ui.control") && shell.cursorIsMovingDesktop && shell.focusedWindow != NONE) UI_Move3DWindow(shell.window[shell.focusedWindow].id, 0.00);

	return 0;
}

static void UI_DrawTip(const char* text) {  // Отрисовка подсказки
	drawRoundedRect((shell.cursorX - 5) + (9 * cgui.scale), shell.cursorY - (10 * cgui.scale), (stringWidth(text, 0.50, 0, 32) + 4) * cgui.scale, ((FONT_SIZE * 0.50) + 2) * cgui.scale, 0, cgui.colors[JSC_CONTEXTMENU], 0);
	drawString(shell.cursorX + (9 * cgui.scale), shell.cursorY - (9 * cgui.scale), text, 0, cgui.colors[JSC_WHITE], 0.50 * cgui.scale, 256);
}

#define UISTYLE_Y e->y + (e->h - (FONT_SIZE * e->scale)) * 0.5
#define UISTYLE_HOVER UI_ItemFocused(e) ? e->hoverStyle : 0
#define UISTYLE_BACKGROUND e->x, e->y, e->w, e->h, e->corner, cgui.colors[e->colorBackground], e->style | hoverStyle
#define UISTYLE_CENTER_TEXT(text) e->x + (e->w * 0.5), text_y, text, FONTSTYLE_CENTER | e->style | hoverStyle, cgui.colors[e->colorText], e->scale, 256
#define UISTYLE_RIGHT_TEXT(text) (e->x + e->w) - e->margin, text_y, text, FONTSTYLE_RIGHT | e->style | hoverStyle, cgui.colors[e->colorText], e->scale, 256
#define UISTYLE_LEFT_TEXT(text) e->x + e->margin, text_y, text, e->style | hoverStyle, cgui.colors[e->colorText], e->scale, 256

static void UI_Picture_Draw(element_s* e) {
	int hoverStyle = UISTYLE_HOVER;

	trap_R_SetColor(cgui.colors[e->colorBackground]);
	drawShader(e->x, e->y, e->w, e->h, e->text);
	trap_R_SetColor(NULL);
}

static void UI_Button_Draw(element_s* e) {
	float text_y = UISTYLE_Y;
	int hoverStyle = UISTYLE_HOVER;

	drawRoundedRect(UISTYLE_BACKGROUND);
	if(e->style & FONTSTYLE_CENTER)
		drawString(UISTYLE_CENTER_TEXT(e->text));
	else if(e->style & FONTSTYLE_RIGHT)
		drawString(UISTYLE_RIGHT_TEXT(e->text));
	else
		drawString(UISTYLE_LEFT_TEXT(e->text));
}

static void UI_Checkbox_Draw(element_s* e) {
	float text_y = UISTYLE_Y;
	int hoverStyle = UISTYLE_HOVER;

	drawRoundedRect(UISTYLE_BACKGROUND);
	if(e->value) {
		drawRoundedRect(((e->x + e->w) - e->h * 1.38) - e->margin, (e->y + (e->h * 0.5)) - e->h * 0.37, e->h * 1.38, e->h * 0.74, 999999, cgui.colors[JSC_ENABLED], 0);
		drawRoundedRect(((e->x + e->w) - e->h * 0.66) - e->margin, (e->y + (e->h * 0.5)) - e->h * 0.27, e->h * 0.54, e->h * 0.54, 999999, cgui.colors[JSC_WHITE], 0);
	} else {
		drawRoundedRect(((e->x + e->w) - e->h * 1.38) - e->margin, (e->y + (e->h * 0.5)) - e->h * 0.37, e->h * 1.38, e->h * 0.74, 999999, cgui.colors[JSC_DISABLED], 0);
		drawRoundedRect(((e->x + e->w) - e->h * 1.26) - e->margin, (e->y + (e->h * 0.5)) - e->h * 0.27, e->h * 0.54, e->h * 0.54, 999999, cgui.colors[JSC_WHITE], 0);
	}
	drawString(UISTYLE_LEFT_TEXT(e->text));
}

static float UI_SliderProgress(float value, float min, float max) {  // Расчет положения slider
	float c = clamp(value, min, max);
	float result = (c - min) / (max - min);

	return result;
}

static void UI_Slider_Draw(element_s* e) {
	float text_y = UISTYLE_Y;
	int hoverStyle = UISTYLE_HOVER;

	drawRoundedRect(UISTYLE_BACKGROUND);
	drawRoundedRect(e->x + (e->w * 0.5), (e->y + (e->h * 0.5)) - e->h * 0.15, (e->w * 0.5) - (e->margin * 1.50), e->h * 0.30, 999999, cgui.colors[JSC_DISABLED], 0);
	drawRoundedRect(e->x + (e->w * 0.5), (e->y + (e->h * 0.5)) - e->h * 0.15, ((e->w * 0.5) - (e->margin * 1.50)) * UI_SliderProgress(e->value, e->min, e->max), e->h * 0.30, 999999, cgui.colors[JSC_ENABLED], 0);
	drawRoundedRect(((e->x + (e->w * 0.5)) - (e->h * 0.32)) + (((e->w * 0.5) - (e->margin * 1.50)) * UI_SliderProgress(e->value, e->min, e->max)), (e->y + (e->h * 0.5)) - e->h * 0.32, e->h * 0.64, e->h * 0.64, 999999, cgui.colors[JSC_WHITE], 0);
	drawString(UISTYLE_LEFT_TEXT(e->text));
	if(UI_ItemFocused(e)) UI_DrawTip(va("%f", e->value));
}

static void UI_Action_Draw(element_s* e) {
	float text_y = UISTYLE_Y;
	int hoverStyle = UISTYLE_HOVER;
	char* bind = (shell.window[shell.activeWindow].keyCapture == e->id) ? ">_" : e->bind;

	drawRoundedRect(UISTYLE_BACKGROUND);
	drawString(UISTYLE_LEFT_TEXT(e->text));
	drawString(UISTYLE_RIGHT_TEXT(bind));
}

static void UI_Spin_Draw(element_s* e) {
	float text_y = UISTYLE_Y;
	int hoverStyle = UISTYLE_HOVER;

	drawRoundedRect(UISTYLE_BACKGROUND);
	drawString(UISTYLE_LEFT_TEXT(e->text));
	drawString(UISTYLE_RIGHT_TEXT(e->options[(int)e->value]));
}

static void UI_Field_Draw(element_s* e) {
	window_s* window = (window_s*)e->parentWindow;
	float text_y = UISTYLE_Y;
	int hoverStyle = UISTYLE_HOVER;
	bool keyCaptureActive = (shell.window[shell.activeWindow].keyCapture == e->id && window->id == shell.window[shell.activeWindow].id);

	if(keyCaptureActive) hoverStyle = 0;

	drawRoundedRect(UISTYLE_BACKGROUND);
	if(keyCaptureActive) {
		drawStringField(UISTYLE_LEFT_TEXT(e->field), e->fieldPosition);
	} else {
		drawString(UISTYLE_LEFT_TEXT(e->text));
		drawStringField(UISTYLE_RIGHT_TEXT(e->field), NONE);
	}
}

static void UI_List_Draw(element_s* e) {
	window_s* window = (window_s*)e->parentWindow;
	int i, j, totalItems, currentItem = e->scroll;

	totalItems = UI_ListGetCount(e->listType, e->listSubtype);
	for(i = 0; i < e->row && currentItem < totalItems; i++) {
		for(j = 0; j < e->col && currentItem < totalItems; j++) {
			if(!strlen(UI_ListGetName(e->listType, e->listSubtype, currentItem))) return;
			if(UI_CursorInRect(e->x + (e->itemW * j), e->y + (e->itemH * i), e->itemW, e->itemH) && shell.focusedWindow == window->id) drawRoundedRect(e->x + (e->itemW * j), e->y + (e->itemH * i), e->itemW, e->itemH, e->corner, cgui.colors[JSC_CONTEXTMENU], 0);
			if(e->listStyle == LSTYLE_GRID) {
				drawShader((e->x + e->margin4[0]) + (e->itemW * j), (e->y + e->margin4[1]) + (e->itemH * i), e->itemW - (e->margin4[2] + e->margin4[0]), e->itemH - (e->margin4[3] + e->margin4[1]), UI_ListGetIcon(e->listType, e->listSubtype, currentItem));
				drawString((e->x + (e->itemW * 0.50)) + (e->itemW * j), e->y + (e->itemH * i) + (e->itemH * 0.85), UI_ListGetName(e->listType, e->listSubtype, currentItem), FONTSTYLE_CENTER | e->style, cgui.colors[e->colorText], e->scale, (e->itemW / ((FONT_SIZE * FONT_WIDTH) * e->scale)) - 1);
			}
			if(e->listStyle == LSTYLE_LIST) {
				drawString((e->x + e->margin) + (e->itemW * j), e->y + (e->itemH - (FONT_SIZE * e->scale)) * 0.5 + (i * e->itemH), UI_ListGetName(e->listType, e->listSubtype, currentItem), FONTSTYLE_LEFT | e->style, cgui.colors[e->colorText], e->scale, (e->itemW / ((FONT_SIZE * FONT_WIDTH) * e->scale)) - 1);
			}
			currentItem++;
		}
	}
}

static void UI_ListFiles_Draw(element_s* e) {
	window_s* window = (window_s*)e->parentWindow;
	int i, j, currentItem = 0;
	char* file = window->fileList[e->listID].list;
	char nameWithoutExt[MAX_JS_STRINGSIZE];
	int filelen = 0;

	for(i = 0; i < e->scroll; i++) {
		if(i >= window->fileList[e->listID].count) break;
		file += strlen(file) + 1;
	}

	for(i = 0; i < e->row; i++) {
		for(j = 0; j < e->col; j++) {
			if(currentItem >= window->fileList[e->listID].count) break;
			filelen = strlen(file);
			if(filelen && file[filelen - 1] == '/') file[filelen - 1] = ' ';
			if(!strcmp(file, "") || !strcmp(file, ".") || !strcmp(file, "..")) {
				currentItem++;
				file += strlen(file) + 1;
				j--;
				continue;
			}
			COM_StripExtension(file, nameWithoutExt, MAX_JS_STRINGSIZE);
			if(UI_CursorInRect(e->x + (e->itemW * j), e->y + (e->itemH * i), e->itemW, e->itemH) && shell.focusedWindow == window->id) drawRoundedRect(e->x + (e->itemW * j), e->y + (e->itemH * i), e->itemW, e->itemH, e->corner, cgui.colors[JSC_CONTEXTMENU], 0);
			if(e->listStyle == LSTYLE_GRID) {
				if(!strcmp(window->fileList[e->listID].ext, "/")) {
					drawShaderInFolder((e->x + e->margin4[0]) + (e->itemW * j), (e->y + e->margin4[1]) + (e->itemH * i), e->itemW - (e->margin4[2] + e->margin4[0]), e->itemH - (e->margin4[3] + e->margin4[1]), va("%s%s", window->fileList[e->listID].drawDir, file));
				} else {
					if(e->listContent == LCONTENT_SHADER) drawShader((e->x + e->margin4[0]) + (e->itemW * j), (e->y + e->margin4[1]) + (e->itemH * i), e->itemW - (e->margin4[2] + e->margin4[0]), e->itemH - (e->margin4[3] + e->margin4[1]), va("%s%s", window->fileList[e->listID].drawDir, nameWithoutExt));
					if(e->listContent == LCONTENT_MODEL) drawModel((e->x + e->margin4[0]) + (e->itemW * j), (e->y + e->margin4[1]) + (e->itemH * i), e->itemW - (e->margin4[2] + e->margin4[0]), e->itemH - (e->margin4[3] + e->margin4[1]), va("%s%s", window->fileList[e->listID].drawDir, nameWithoutExt), 75.0f);
				}
				if(strcmp(window->fileList[e->listID].ext, "/"))
					drawString((e->x + (e->itemW * 0.50)) + (e->itemW * j), e->y + (e->itemH * i) + (e->itemH * 0.85), nameWithoutExt, FONTSTYLE_CENTER | e->style, cgui.colors[e->colorText], e->scale, (e->itemW / ((FONT_SIZE * FONT_WIDTH) * e->scale)) - 1);
				else
					drawString((e->x + (e->itemW * 0.50)) + (e->itemW * j), e->y + (e->itemH * i) + (e->itemH * 0.85), file, FONTSTYLE_CENTER | e->style, cgui.colors[e->colorText], e->scale, (e->itemW / ((FONT_SIZE * FONT_WIDTH) * e->scale)) - 1);
			}
			if(e->listStyle == LSTYLE_LIST) {
				if(strcmp(window->fileList[e->listID].ext, "/"))
					drawString((e->x + e->margin) + (e->itemW * j), e->y + (e->itemH - (FONT_SIZE * e->scale)) * 0.5 + (i * e->itemH), nameWithoutExt, FONTSTYLE_LEFT | e->style, cgui.colors[e->colorText], e->scale, (e->itemW / ((FONT_SIZE * FONT_WIDTH) * e->scale)) - 1);
				else
					drawString((e->x + e->margin) + (e->itemW * j), e->y + (e->itemH - (FONT_SIZE * e->scale)) * 0.5 + (i * e->itemH), file, FONTSTYLE_LEFT | e->style, cgui.colors[e->colorText], e->scale, (e->itemW / ((FONT_SIZE * FONT_WIDTH) * e->scale)) - 1);
			}
			currentItem++;
			file += strlen(file) + 1;
		}
	}
}

static void UI_WindowButton_Draw(element_s* e) {
	window_s* window = (window_s*)e->parentWindow;

	if((!(trap_Key_GetCatcher() & KEYCATCH_UI) && !window->linked) || (!(trap_Key_GetCatcher() & KEYCATCH_UI) && window->linked && window->worldDisable)) return;

	drawRoundedRect(e->x, e->y, e->w, e->h, e->corner, cgui.colors[e->colorBackground], e->style);
	drawString(e->x + (e->w * 0.456f), (e->y + (e->h * 0.525)) - ((FONT_SIZE * 0.50) * e->scale), e->text, UI_CENTER | FONTSTYLE_BOLD, cgui.colors[e->colorText], e->scale, 256);
}

static void UI_ElementRender(int windowID, int elementID) {  // Отрисовка элемента
	element_s* element = &shell.window[windowID].element[elementID];
	if(!element->type) return;
	UI_AdjustToWindow(element);
	if(!UI_ElementIsVisible(element)) return;
	if(shell.window[windowID].minimized && element->type != ETYPE_WINDOWBUTTON) return;

	if(element->style & NO_DRAW) return;
	switch(element->type) {
		case ETYPE_PICTURE: UI_Picture_Draw(element); break;
		case ETYPE_BUTTON: UI_Button_Draw(element); break;
		case ETYPE_CHECKBOX: UI_Checkbox_Draw(element); break;
		case ETYPE_SLIDER: UI_Slider_Draw(element); break;
		case ETYPE_ACTION: UI_Action_Draw(element); break;
		case ETYPE_SPIN: UI_Spin_Draw(element); break;
		case ETYPE_FIELD: UI_Field_Draw(element); break;
		case ETYPE_LIST: UI_List_Draw(element); break;
		case ETYPE_LISTFILES: UI_ListFiles_Draw(element); break;
		case ETYPE_WINDOWBUTTON: UI_WindowButton_Draw(element); break;
	}
	if(shell.debug) drawOutline(element->x, element->y, element->w, element->h, 2, cgui.colors[JSC_DEBUG1]);
}

static void UI_Draw3DWindow(float x, float y, float z, int windowID, float max) {  // 3D окна в координатах мира
	window_s* window = &shell.window[windowID];
	vec3_t dir;
	float localX, localY, localZ;
	float tanFovX, tanFovY;
	float finalx, finaly;
	float dist;
	vec3_t worldPos;
	vec3_t viewOrg;
	vec3_t viewAxis[3];
	int i;
	float fov_x, fov_y;
	float size;
	float rX, rY, rScale;
	trace_t trace;
	bool blocked = false;

	viewOrg[0] = trap_Cvar_VariableValue("cgame.vieworg[0]");
	viewOrg[1] = trap_Cvar_VariableValue("cgame.vieworg[1]");
	viewOrg[2] = trap_Cvar_VariableValue("cgame.vieworg[2]");

	viewAxis[0][0] = trap_Cvar_VariableValue("cgame.viewaxis[0][0]");
	viewAxis[0][1] = trap_Cvar_VariableValue("cgame.viewaxis[0][1]");
	viewAxis[0][2] = trap_Cvar_VariableValue("cgame.viewaxis[0][2]");
	viewAxis[1][0] = trap_Cvar_VariableValue("cgame.viewaxis[1][0]");
	viewAxis[1][1] = trap_Cvar_VariableValue("cgame.viewaxis[1][1]");
	viewAxis[1][2] = trap_Cvar_VariableValue("cgame.viewaxis[1][2]");
	viewAxis[2][0] = trap_Cvar_VariableValue("cgame.viewaxis[2][0]");
	viewAxis[2][1] = trap_Cvar_VariableValue("cgame.viewaxis[2][1]");
	viewAxis[2][2] = trap_Cvar_VariableValue("cgame.viewaxis[2][2]");

	fov_x = trap_Cvar_VariableValue("cgame.fov_x");
	fov_y = trap_Cvar_VariableValue("cgame.fov_y");

	worldPos[0] = x;
	worldPos[1] = y;
	worldPos[2] = z;

	VectorSubtract(worldPos, viewOrg, dir);

	// trap_CM_BoxTrace(&trace, viewOrg, worldPos, vec3_origin, vec3_origin, 0, CONTENTS_SOLID);
	// if(trace.fraction < 1.0f) blocked = true;

	localX = -DotProduct(dir, viewAxis[1]);
	localY = DotProduct(dir, viewAxis[2]);
	localZ = DotProduct(dir, viewAxis[0]);

	if(localZ <= 0) return;

	tanFovX = tan(DEG2RAD(fov_x * 0.5f));
	tanFovY = tan(DEG2RAD(fov_y * 0.5f));

	finalx = (localX / (localZ * tanFovX)) * glconfig.vidWidth * 0.50f + glconfig.vidWidth * 0.50f;
	finaly = (-localY / (localZ * tanFovY)) * glconfig.vidHeight * 0.50f + glconfig.vidHeight * 0.50f;

	dist = VectorLength(dir);

	if(!shell.cursorIsMovingDesktop) window->worldMoveDist = dist;

	size = window->scaleFactor * max / dist;
	if(size < 0.0f) size = 0.0f;

	rX = window->x;
	rY = window->y;
	rScale = window->scaleFactor;

	window->x = finalx;
	window->y = finaly;
	window->scaleFactor = size * window->worldScale;
	window->worldCursorScale = size * window->worldScale;
	UI_ScaleToWindow(window);
	window->x -= window->w * 0.5f;
	window->y -= window->h * 0.5f;
	if(window->h < glconfig.vidHeight * 0.15)
		window->worldDisable = true;
	else
		window->worldDisable = false;

	if(blocked)
		alphaRoundedRect = 0.25;
	else
		alphaRoundedRect = 0.90;

	if(window->y == UI_WINDOW_TITLE_HEIGHT) window->y -= UI_WINDOW_TITLE_HEIGHT;
	drawRoundedRect(window->x, window->y, window->w, window->h, shell.windowCorner * shell.scale, cgui.colors[window->colorBackground], NO_TOP_LEFT | NO_TOP_RIGHT | NO_BOTTOM_LEFT | NO_BOTTOM_RIGHT);
	if(window->h > glconfig.vidHeight * 0.025 && !blocked)
		for(i = 0; i < WINDOW_MAX_ELEMENTS; i++) UI_ElementRender(windowID, i);
	if(window->y == 0) window->y += UI_WINDOW_TITLE_HEIGHT;

	alphaRoundedRect = 1.00;

	if(window->h > glconfig.vidHeight * 0.025) {
		shell.cursorX = glconfig.vidWidth * 0.50;
		shell.cursorY = glconfig.vidHeight * 0.50;
	}

	if(window->h > glconfig.vidHeight * 0.025 && UI_CursorInRect(window->x, window->y - UI_WINDOW_TITLE_HEIGHT, window->w, window->h + UI_WINDOW_TITLE_HEIGHT)) {
		if(window->keyboardCapture)
			trap_Cvar_Set("ui.control", "2");
		else
			trap_Cvar_Set("ui.control", "1");

		shell.focusedWindow = window->id;
		shell.activeWindow = window->id;

		shell.window[shell.focusedWindow].focusedElement = NONE;
		for(i = WINDOW_MAX_ELEMENTS - 1; i >= 0; i--) {  // Поиск элемента под курсором
			if(UI_CursorOnItem(&window->element[i])) {
				window->focusedElement = i;
				break;
			}
		}

		if(trap_Cvar_VariableIntegerValue("ui.control") && shell.cursorIsMovingDesktop) UI_Move3DWindow(shell.window[shell.focusedWindow].id, 0.00);
	}

	window->x = rX;
	window->y = rY;
	window->scaleFactor = rScale;
	UI_ScaleToWindow(window);
}

static void UI_ShellDraw(void) {  // Отрисовка всех окон и подсказки
	window_s* focusedWindow = &shell.window[shell.focusedWindow];
	window_s* activeWindow = &shell.window[shell.activeWindow];
	int i, j;
	float tc = shell.windowCorner * shell.scale;
	float th = UI_WINDOW_TITLE_HEIGHT;

	for(i = 0; i < SHELL_MAX_WINDOWS; i++) {
		window_s* window = &shell.window[shell.zOrder[i]];

		if(!window->created) continue;
		UI_ScaleToWindow(window);
		if(!window->minimized) {
			float titleHeight = 0;
			if(!(window->style & UI_NOTITLE)) titleHeight = th;
			if(shell.windowOutline && shell.windowColoredOutline && window->colorBackground != JSC_EMPTY)
				drawRoundedRect(window->x - 1, (window->y - titleHeight) - 1, window->w + 2, (window->h + titleHeight) + 2, tc, cgui.colors[JSC_ENABLED], NO_BOTTOM_LEFT | NO_BOTTOM_RIGHT);
			else if(shell.windowOutline && window->colorBackground != JSC_EMPTY)
				drawRoundedRect(window->x - 1, (window->y - titleHeight) - 1, window->w + 2, (window->h + titleHeight) + 2, tc, cgui.colors[JSC_WINDOWBUTTON], NO_BOTTOM_LEFT | NO_BOTTOM_RIGHT);
			drawRoundedRect(window->x, window->y, window->w, window->h, tc, cgui.colors[window->colorBackground], NO_TOP_LEFT | NO_TOP_RIGHT | NO_BOTTOM_LEFT | NO_BOTTOM_RIGHT);
			if(shell.debug) drawOutline(window->x, window->y, window->w, window->h, 2, cgui.colors[JSC_DEBUG2]);
		}
		if(!(window->style & UI_NOTITLE)) {
			int adaptiveStyle = 0;
			if(window->x == 0 && window->y == th) adaptiveStyle += NO_TOP_LEFT;
			if(window->x + window->w == glconfig.vidWidth && window->y == th) adaptiveStyle += NO_TOP_RIGHT;
			drawRoundedRect(window->x, window->y - th, window->w, th, tc, cgui.colors[window->colorTitle], NO_BOTTOM_LEFT | NO_BOTTOM_RIGHT | adaptiveStyle);
			drawString(window->x + (window->w * 0.5), (window->y - th) + (th - FONT_SIZE * (0.75 * shell.scale)) * 0.5, window->name, FONTSTYLE_CENTER | FONTSTYLE_BOLD, cgui.colors[window->colorText], 0.75 * shell.scale, 256);
		}
		for(j = 0; j < WINDOW_MAX_ELEMENTS; j++) UI_ElementRender(shell.zOrder[i], j);
		if(!(window->style & UI_NOTITLE)) drawShader(clamp(window->x + (8 * shell.scale), 0, glconfig.vidWidth - (th * 0.70)), clamp(window->y - (th * 0.85), 0, glconfig.vidHeight - (th * 0.70)), th * 0.70, th * 0.70, window->icon);
	}

	if(activeWindow->keyCapture >= 0) UI_DrawTip("Press any key, ESC for cancel");
}

static void UI_ShellRender(void) {  // Отрисовка UI
	int i;

	JS_ShellDraw();
	for(i = 0; i < SHELL_MAX_WINDOWS; i++) {
		if(!shell.window[i].created || !strlen(shell.window[i].nameID)) continue;
		JS_ShellUpdate(i);
	}
	UI_ShellDraw();
}

int UI_Refresh(void) {  // Обновление UI (отрисовка кадра)
	UI_ShellRender();
	drawShader(shell.cursorX - (12 * cgui.scale), shell.cursorY - (12 * cgui.scale), 24 * cgui.scale, 24 * cgui.scale, "menu/cursor");
	shell.rendered = true;

	return 0;
}

int UI_IsFullscreen(void) {  // Статус полноэкранного режима UI (влияет на рендеринг карты)
	return ((trap_Key_GetCatcher() & KEYCATCH_UI) && !shell.onMap);
}

int UI_SetActiveMenu(uiMenuCommand_t cmd) {  // Управление UI из движка
	(cmd ? UI_OpenShell : UI_CloseShell)();
	return 0;
}

static char* UI_Argv(int arg) {  // Буфер аргумента
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));
	return buffer;
}

bool UI_ConsoleCommand(void) {  // Команда консоли из движка
	char* cmd;
	cmd = UI_Argv(0);

	if(Q_stricmp(cmd, "shell.open") == 0) {
		UI_OpenShell();
		return true;
	}

	if(Q_stricmp(cmd, "shell.restart") == 0) {
		UI_HotReload();
		return true;
	}

	if(Q_stricmp(cmd, "shell.close") == 0) {
		UI_CloseShell();
		return true;
	}

	return false;
}

int UI_DrawConnectScreen(void) {  // Экран подключение к серверу (до фактического подключения)
	int i, j;
	float cursorScale = 1.00f;
	uiClientState_t cstate;

	trap_GetClientState(&cstate);

	if(cstate.connState >= CA_ACTIVE && !(trap_Key_GetCatcher() & KEYCATCH_UI)) {
		float tc = shell.windowCorner * shell.scale;
		float th = 0;

		trap_Cvar_Set("ui.control", "0");

		for(i = 0; i < SHELL_MAX_WINDOWS; i++) {
			if(!shell.window[i].created || !strlen(shell.window[i].nameID) || (!shell.window[i].pinned && !shell.window[i].linked)) continue;
			JS_ShellUpdate(i);
		}

		for(i = 0; i < SHELL_MAX_WINDOWS; i++) {
			window_s* window = &shell.window[shell.zOrder[i]];
			if(!window->created) continue;
			if(window->linked) {
				UI_Draw3DWindow(window->worldX, window->worldY, window->worldZ, window->id, 128);
			} else {
				UI_ScaleToWindow(window);
				if(window->y == UI_WINDOW_TITLE_HEIGHT) window->y -= UI_WINDOW_TITLE_HEIGHT;
				if(window->pinned) {
					float titleHeight = 0;
					if(shell.windowOutline && shell.windowColoredOutline && window->colorBackground != JSC_EMPTY)
						drawRoundedRect(window->x - 1, (window->y - titleHeight) - 1, window->w + 2, (window->h + titleHeight) + 2, tc, cgui.colors[JSC_ENABLED], NO_BOTTOM_LEFT | NO_BOTTOM_RIGHT | NO_TOP_LEFT | NO_TOP_RIGHT);
					else if(shell.windowOutline && window->colorBackground != JSC_EMPTY)
						drawRoundedRect(window->x - 1, (window->y - titleHeight) - 1, window->w + 2, (window->h + titleHeight) + 2, tc, cgui.colors[JSC_WINDOWBUTTON], NO_BOTTOM_LEFT | NO_BOTTOM_RIGHT | NO_TOP_LEFT | NO_TOP_RIGHT);
					drawRoundedRect(window->x, window->y, window->w, window->h, tc, cgui.colors[window->colorBackground], NO_TOP_LEFT | NO_TOP_RIGHT | NO_BOTTOM_LEFT | NO_BOTTOM_RIGHT);
					if(shell.debug) drawOutline(window->x, window->y, window->w, window->h, 2, cgui.colors[JSC_DEBUG2]);
				}
				for(j = 0; j < WINDOW_MAX_ELEMENTS; j++)
					if(window->pinned) UI_ElementRender(shell.zOrder[i], j);
				if(window->y == 0) window->y += UI_WINDOW_TITLE_HEIGHT;
			}
		}

		if(shell.focusedWindow != NONE && shell.window[shell.focusedWindow].linked) cursorScale = shell.window[shell.focusedWindow].worldCursorScale;
		if(trap_Cvar_VariableIntegerValue("ui.control")) drawShader(shell.cursorX - (12 * cgui.scale) * cursorScale, shell.cursorY - (12 * cgui.scale) * cursorScale, (24 * cgui.scale) * cursorScale, (24 * cgui.scale) * cursorScale, "menu/cursor");
	}

	for(i = 0; i < SHELL_MAX_WINDOWS; i++) {
		if(!shell.window[i].created || !strlen(shell.window[i].nameID)) continue;
		JS_ShellBackgroundUpdate(i);
	}

	if(cstate.connState >= CA_ACTIVE || cstate.connState == CA_DISCONNECTED) return 0;

	if(cstate.connState > CA_CONNECTED)
		drawRoundedRect((glconfig.vidWidth * 0.50) - (glconfig.vidWidth * 0.08), 0, glconfig.vidWidth * 0.16, glconfig.vidWidth * 0.015, shell.windowCorner, cgui.colors[JSC_LOADINGISLAND], NO_TOP_LEFT | NO_TOP_RIGHT);
	else
		drawRoundedRect((glconfig.vidWidth * 0.50) - (glconfig.vidWidth * 0.08), 0, glconfig.vidWidth * 0.16, glconfig.vidWidth * 0.03, shell.windowCorner, cgui.colors[JSC_LOADINGISLAND], NO_TOP_LEFT | NO_TOP_RIGHT);

	drawString(glconfig.vidWidth * 0.5, glconfig.vidHeight * 0.005, "Connecting to server...", FONTSTYLE_CENTER, cgui.colors[JSC_WHITE], 0.35 * cgui.scale, 256);

	if(cstate.connState < CA_CONNECTED) drawString(glconfig.vidWidth * 0.5, glconfig.vidHeight * 0.03, cstate.messageString, FONTSTYLE_CENTER, color_white, 0.35 * cgui.scale, 256);

	switch(cstate.connState) {
		case CA_CONNECTING: drawString(glconfig.vidWidth * 0.5, glconfig.vidHeight * 0.03, va("Awaiting challenge...%i", cstate.connectPacketCount), FONTSTYLE_CENTER, color_white, 0.35 * cgui.scale, 256); break;
		case CA_CHALLENGING: drawString(glconfig.vidWidth * 0.5, glconfig.vidHeight * 0.03, va("Awaiting connection...%i", cstate.connectPacketCount), FONTSTYLE_CENTER, color_white, 0.35 * cgui.scale, 256); break;
		case CA_CONNECTED: drawString(glconfig.vidWidth * 0.5, glconfig.vidHeight * 0.03, "Awaiting gamestate...", FONTSTYLE_CENTER, color_white, 0.35 * cgui.scale, 256); break;
		case CA_UNINITIALIZED:
		case CA_DISCONNECTED:
		case CA_AUTHORIZING:
		case CA_LOADING:
		case CA_PRIMED:
		case CA_ACTIVE:
		case CA_CINEMATIC: break;
	}

	return 0;
}

void UI_CloseWindow(int windowID) {  // Закрытие окна
	if(shell.activeWindow == shell.window[windowID].id) shell.activeWindow = 0;
	memset(&shell.window[windowID], 0, sizeof(shell.window[windowID]));
}

void UI_ClearWindow(int windowID, int min, int max) {  // Очистка окна
	int i;

	for(i = min; i <= max; i++) memset(&shell.window[windowID].element[i], 0, sizeof(shell.window[windowID].element[i]));
}

int UI_GetElementFree(int windowID) {
	int id = -1;
	int i;

	for(i = 0; i < WINDOW_MAX_ELEMENTS; i++) {
		if(!shell.window[windowID].element[i].created) {
			id = i;
			break;
		}
	}

	return id;
}

int UI_Window(int windowID, char* nameID, char* name, char* icon, int style, float w, float h, int colorText, int colorTitle, int colorBackground) {
	int id = windowID;
	int i;

	if(windowID == NONE) {
		for(i = 0; i < SHELL_MAX_WINDOWS; i++) {
			if(!shell.window[i].created) {
				id = i;
				break;
			}
		}
	}

	if(id == NONE) {
		trap_Print("#f55NMRE: window limit exceeded \n");
		return id;
	}

	shell.window[id].created = true;
	shell.window[id].minimized = false;
	shell.window[id].pinned = false;
	shell.window[id].linked = false;
	shell.window[id].keyboardCapture = false;
	shell.window[id].id = id;
	StringCopy(shell.window[id].nameID, nameID, MAX_JS_STRINGSIZE);
	StringCopy(shell.window[id].name, name, MAX_JS_STRINGSIZE);
	StringCopy(shell.window[id].icon, icon, MAX_JS_STRINGSIZE);
	shell.window[id].style = style;
	shell.window[id].x = (glconfig.vidWidth - w) * 0.5;
	shell.window[id].y = (glconfig.vidHeight - h) * 0.5;
	shell.window[id].baseW = w;
	shell.window[id].baseH = h;
	shell.window[id].colorText = colorText;
	shell.window[id].colorTitle = colorTitle;
	shell.window[id].colorBackground = colorBackground;
	shell.window[id].scaleFactor = shell.scale;
	shell.window[id].keyCapture = NONE;
	shell.window[id].worldX = 0;
	shell.window[id].worldY = 0;
	shell.window[id].worldZ = -20;
	shell.window[id].worldScale = 1.00;
	shell.window[id].worldDisable = false;
	UI_WindowToTop(id);
	return id;
}

static int UI_GenericItem(int windowID, int elementID, int type, float x, float y, float w, float h, char* text, int style, int color, float scale) {
	int id = elementID;
	int i;

	if(elementID == NONE) {
		for(i = 0; i < WINDOW_MAX_ELEMENTS; i++) {
			if(!shell.window[windowID].element[i].created) {
				id = i;
				break;
			}
		}
	}

	if(id == NONE) {
		trap_Print("#f55NMRE: element limit exceeded \n");
		return id;
	}

	shell.window[windowID].element[id].created = true;
	shell.window[windowID].element[id].type = type;
	shell.window[windowID].element[id].id = id;
	shell.window[windowID].element[id].parentWindow = &shell.window[windowID];
	shell.window[windowID].element[id].baseX = x;
	shell.window[windowID].element[id].baseY = y;
	shell.window[windowID].element[id].baseW = w;
	shell.window[windowID].element[id].baseH = h;
	StringCopy(shell.window[windowID].element[id].text, text, MAX_JS_STRINGSIZE);
	shell.window[windowID].element[id].style = style;
	shell.window[windowID].element[id].hoverStyle = FONTSTYLE_UNDERLINE;
	shell.window[windowID].element[id].baseScale = scale;
	shell.window[windowID].element[id].colorText = JSC_WHITE;
	shell.window[windowID].element[id].colorBackground = color;
	shell.window[windowID].element[id].baseCorner = trap_Cvar_VariableValue("shell.element.corner");
	shell.window[windowID].element[id].baseMargin = trap_Cvar_VariableValue("shell.element.corner") * 1.50;
	shell.window[windowID].element[id].scroll = 0;
	if(!shell.window[windowID].element[id].valueMod) shell.window[windowID].element[id].valueMod = 1.00;
	return id;
}

int UI_WindowButton(int windowID, int elementID, float x, char* text, int style, int color, float scale) {
	int id = UI_GenericItem(windowID, elementID, ETYPE_WINDOWBUTTON, x, 0, 0, 0, text, style, color, scale);
	shell.window[windowID].element[id].baseY = 0 - (UI_WINDOW_TITLE_HEIGHT * 0.80);
	shell.window[windowID].element[id].baseW = UI_WINDOW_TITLE_HEIGHT * 0.60;
	shell.window[windowID].element[id].baseH = UI_WINDOW_TITLE_HEIGHT * 0.60;
	shell.window[windowID].element[id].baseCorner = 999999;
	return id;
}

int UI_Picture(int windowID, int elementID, float x, float y, float w, float h, char* picture, int style, int color) {
	int id = UI_GenericItem(windowID, elementID, ETYPE_PICTURE, x, y, w, h, picture, style, color, 1.00);
	return id;
}

int UI_Button(int windowID, int elementID, float x, float y, float w, float h, char* text, int style, int color, float scale) {
	int id = UI_GenericItem(windowID, elementID, ETYPE_BUTTON, x, y, w, h, text, style, color, scale);
	return id;
}

int UI_Checkbox(int windowID, int elementID, float x, float y, float w, float h, char* text, int style, int color, float scale, char* cvar) {
	int id = UI_GenericItem(windowID, elementID, ETYPE_CHECKBOX, x, y, w, h, text, style, color, scale);
	StringCopy(shell.window[windowID].element[id].cvar, cvar, MAX_JS_STRINGSIZE);
	shell.window[windowID].element[id].value = trap_Cvar_VariableValue(shell.window[windowID].element[id].cvar);
	return id;
}

int UI_Slider(int windowID, int elementID, float x, float y, float w, float h, char* text, int style, int color, float scale, char* cvar, float min, float max, int mode) {
	int id = UI_GenericItem(windowID, elementID, ETYPE_SLIDER, x, y, w, h, text, style, color, scale);
	StringCopy(shell.window[windowID].element[id].cvar, cvar, MAX_JS_STRINGSIZE);
	shell.window[windowID].element[id].value = trap_Cvar_VariableValue(shell.window[windowID].element[id].cvar);
	shell.window[windowID].element[id].min = min;
	shell.window[windowID].element[id].max = max;
	shell.window[windowID].element[id].mode = mode;
	return id;
}

int UI_Action(int windowID, int elementID, float x, float y, float w, float h, char* text, int style, int color, float scale, char* cvar) {
	int i;
	int id = UI_GenericItem(windowID, elementID, ETYPE_ACTION, x, y, w, h, text, style, color, scale);
	StringCopy(shell.window[windowID].element[id].cvar, cvar, MAX_JS_STRINGSIZE);
	for(i = 0; i < MAX_KEYS; i++) {
		if(!strcmp(cvar, cgui.binds[i])) trap_Key_KeynumToStringBuf(i, shell.window[windowID].element[id].bind, 256);
	}
	return id;
}

int UI_Spin(int windowID, int elementID, float x, float y, float w, float h, char* text, int style, int color, float scale, char* cvar, int mode) {
	int i;
	int id = UI_GenericItem(windowID, elementID, ETYPE_SPIN, x, y, w, h, text, style, color, scale);
	StringCopy(shell.window[windowID].element[id].cvar, cvar, MAX_JS_STRINGSIZE);
	shell.window[windowID].element[id].mode = mode;
	if(mode == EMODE_NUMBER) {
		shell.window[windowID].element[id].value = trap_Cvar_VariableValue(shell.window[windowID].element[id].cvar);
	} else if(mode == EMODE_STRING) {
		for(i = 0; i < shell.window[windowID].element[id].optionsCount; i++) {
			if(!strcmp(trap_Cvar_VariableString(shell.window[windowID].element[id].cvar), shell.window[windowID].element[id].options[i])) shell.window[windowID].element[id].value = i;
		}
	}
	return id;
}

int UI_Field(int windowID, int elementID, float x, float y, float w, float h, char* text, int style, int color, float scale, char* cvar) {
	int id = UI_GenericItem(windowID, elementID, ETYPE_FIELD, x, y, w, h, text, style, color, scale);
	StringCopy(shell.window[windowID].element[id].cvar, cvar, MAX_JS_STRINGSIZE);
	StringCopy(shell.window[windowID].element[id].field, trap_Cvar_VariableString(cvar), MAX_JS_STRINGSIZE);
	shell.window[windowID].element[id].fieldPosition = strlen(trap_Cvar_VariableString(cvar));
	return id;
}

int UI_List(int windowID, int elementID, float x, float y, float w, float h, float scale, int col, int row, int listStyle, int listType, int listSubtype) {
	int id = UI_GenericItem(windowID, elementID, ETYPE_LIST, x, y, w, h, "", 0, 0, scale);
	shell.window[windowID].element[id].listStyle = listStyle;
	shell.window[windowID].element[id].listType = listType;
	shell.window[windowID].element[id].listSubtype = listSubtype;
	shell.window[windowID].element[id].col = col;
	shell.window[windowID].element[id].row = row;
	return id;
}

int UI_ListFiles(int windowID, int elementID, float x, float y, float w, float h, float scale, int col, int row, int listStyle, int listContent, int listID) {
	int id = UI_GenericItem(windowID, elementID, ETYPE_LISTFILES, x, y, w, h, "", 0, 0, scale);
	shell.window[windowID].element[id].listStyle = listStyle;
	shell.window[windowID].element[id].listContent = listContent;
	shell.window[windowID].element[id].col = col;
	shell.window[windowID].element[id].row = row;
	shell.window[windowID].element[id].listID = listID;
	return id;
}

void UI_FillListFiles(int windowID, char* folder, char* ext, char* drawDir, int listID) {  // Заполняет список файлов
	memset(&shell.window[windowID].fileList[listID], 0, sizeof(shell.window[windowID].fileList[listID]));
	shell.window[windowID].fileList[listID].count = trap_FS_GetFileList(folder, ext, shell.window[windowID].fileList[listID].list, FILELIST_MAX_LENGTH);
	StringCopy(shell.window[windowID].fileList[listID].folder, folder, MAX_JS_STRINGSIZE);
	StringCopy(shell.window[windowID].fileList[listID].ext, ext, MAX_JS_STRINGSIZE);
	StringCopy(shell.window[windowID].fileList[listID].drawDir, drawDir, MAX_JS_STRINGSIZE);
}

char* UI_GetFileFromList(window_s* window, int listID, int index) {  // Возвращает имя файла из списка
	int i;
	char* file = window->fileList[listID].list;
	static char getFileFromList_buffer[MAX_JS_STRINGSIZE];

	for(i = 0; i < index; i++) {
		if(i >= window->fileList[listID].count) return "";
		file += strlen(file) + 1;
	}

	memcpy(getFileFromList_buffer, file, strlen(file));
	getFileFromList_buffer[strlen(file)] = '\0';
	return getFileFromList_buffer;
}

int UI_GetFileListCount(window_s* window, int listID) {  // Возвращает количество файлов в списке
	return window->fileList[listID].count;
}

void UI_SetMargin(int windowID, int elementID, float x, float y, float w, float h) {  // Устанавливает отступы
	shell.window[windowID].element[elementID].baseMargin4[0] = x;
	shell.window[windowID].element[elementID].baseMargin4[1] = y;
	shell.window[windowID].element[elementID].baseMargin4[2] = w;
	shell.window[windowID].element[elementID].baseMargin4[3] = h;
}
