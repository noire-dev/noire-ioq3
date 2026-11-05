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
// cg_main.c -- initialization and primary entry point for cgame
#include "../qcommon/vm_javascript.h"

static int forceModelModificationCount = -1;

void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum);
void CG_Shutdown(void);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	switch(command) {
		case CG_INIT: CG_Init(arg0, arg1, arg2); return 0;
		case CG_SHUTDOWN: CG_Shutdown(); return 0;
		case CG_CONSOLE_COMMAND: return CG_ConsoleCommand();
		case CG_DRAW_ACTIVE_FRAME: CG_DrawActiveFrame(arg0, arg1); return 0;

		case GETVMCONTEXT: VMContext(&vmargs, &vmresult); return 0;
		case VMCALL: VMCall(arg0); return 0;
		default: trap_Error("cgame.vm: unknown command"); break;
	}
	return -1;
}

cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
weaponInfo_t cg_weapons[MAX_WEAPONS];
itemInfo_t cg_items[MAX_ITEMS];

vmCvar_t cg_drawFPS;
vmCvar_t cg_drawCrosshair;
vmCvar_t cg_crosshairScale;
vmCvar_t cg_draw2D;
vmCvar_t cg_animSpeed;
vmCvar_t cg_errorDecay;
vmCvar_t cg_noPlayerAnims;
vmCvar_t cg_showmiss;
vmCvar_t cg_addMarks;
vmCvar_t cg_brassTime;
vmCvar_t cg_viewsize;
vmCvar_t cg_fov;
vmCvar_t cg_zoomFov;
vmCvar_t cg_thirdPerson;
vmCvar_t cg_thirdPersonRange;
vmCvar_t cg_thirdPersonAngle;
vmCvar_t cg_thirdPersonOffsetX;
vmCvar_t cg_thirdPersonOffsetY;
vmCvar_t cg_thirdPersonOffsetZ;
vmCvar_t cg_thirdPersonSmoothing;
vmCvar_t cg_firstPerson_Fwd;
vmCvar_t cg_firstPerson_Up;
vmCvar_t cg_stats;
vmCvar_t cg_buildScript;
vmCvar_t cg_forceModel;
vmCvar_t cg_deferPlayers;
vmCvar_t cg_cameraOrbit;
vmCvar_t cg_cameraOrbitDelay;
vmCvar_t cg_timescaleFadeEnd;
vmCvar_t cg_timescaleFadeSpeed;
vmCvar_t cg_timescale;
vmCvar_t cg_trueLightning;

typedef struct {
	vmCvar_t* vmCvar;
	char* cvarName;
	char* defaultString;
	int cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] = {
    {&cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE},
    {&cg_fov, "cg_fov", "90", CVAR_ARCHIVE},
    {&cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE},
    {&cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE},
    {&cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE},
    {&cg_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE},
    {&cg_crosshairScale, "cg_crosshairScale", "24", CVAR_ARCHIVE},
    {&cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE},
    {&cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE},
    {&cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT},
    {&cg_errorDecay, "cg_errordecay", "100", 0},
    {&cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT},
    {&cg_showmiss, "cg_showmiss", "0", 0},
    {&cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT},
    {&cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT},
    {&cg_thirdPersonOffsetX, "cg_thirdPersonOffsetX", "0", CVAR_ARCHIVE},
    {&cg_thirdPersonOffsetY, "cg_thirdPersonOffsetY", "0", CVAR_ARCHIVE},
    {&cg_thirdPersonOffsetZ, "cg_thirdPersonOffsetZ", "0", CVAR_ARCHIVE},
    {&cg_thirdPersonSmoothing, "cg_thirdPersonSmoothing", "0", CVAR_ARCHIVE},
    {&cg_thirdPerson, "cg_thirdPerson", "0", 0},
    {&cg_firstPerson_Fwd, "cg_firstPerson_Fwd", "0", CVAR_CHEAT},
    {&cg_firstPerson_Up, "cg_firstPerson_Up", "0", CVAR_CHEAT},
    {&cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE},
    {&cg_deferPlayers, "cg_deferPlayers", "0", CVAR_ARCHIVE},

    {&cg_stats, "cg_stats", "0", 0},
    // the following variables are created in other parts of the system,
    // but we also reference them here
    {&cg_buildScript, "com_buildScript", "0", 0},  // force loading of all possible data amd error on failures
    {&cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT},
    {&cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
    {&cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
    {&cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
    {&cg_timescale, "timescale", "1", 0},
    {&cg_trueLightning, "cg_trueLightning", "0.0", CVAR_ARCHIVE},
};

static int cvarTableSize = ARRAY_LEN(cvarTable);

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars(void) {
	int i;
	cvarTable_t* cv;

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++) {
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
	}

	forceModelModificationCount = cg_forceModel.modificationCount;

	trap_Cvar_Register(NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);

	// Global fallback hands model used when a weapon has no <model>_hand.md3.
	// Configurable so no per-weapon hardcoded path remains in the C code.
	trap_Cvar_Register(NULL, "cg_defaultHandsModel", "models/weapons2/shotgun/shotgun_hand.md3", CVAR_ARCHIVE);
}

/*
===================
CG_ForceModelChange
===================
*/
static void CG_ForceModelChange(void) {
	int i;

	for(i = 0; i < MAX_CLIENTS; i++) {
		const char* clientInfo;

		clientInfo = CG_ConfigString(CS_PLAYERS + i);
		if(!clientInfo[0]) {
			continue;
		}
		CG_NewClientInfo(i);
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars(void) {
	int i;
	cvarTable_t* cv;

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++) {
		trap_Cvar_Update(cv->vmCvar);
	}

	// if force model changed
	if(forceModelModificationCount != cg_forceModel.modificationCount) {
		forceModelModificationCount = cg_forceModel.modificationCount;
		CG_ForceModelChange();
	}
}

void QDECL CG_Printf(const char* msg, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	CG_AddToGenericConsole(text, &cgs.chat);

	trap_Print(text);
}

void QDECL CG_Error(const char* msg, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL Com_Error(int level, const char* error, ...) {
	va_list argptr;
	char text[1024];

	(void)level;
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

/*
================
CG_Argv
================
*/
const char* CG_Argv(int arg) {
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds(int itemNum) {
	item_t* item;
	char data[MAX_QPATH];
	char *s, *start;
	int len;

	item = &jsd_item[itemNum];

	if(item->pickup_sound) {
		trap_S_RegisterSound(item->pickup_sound, false);
	}

	// parse the space separated precache string for other media
	s = item->sounds;
	if(!s || !s[0]) return;

	while(*s) {
		start = s;
		while(*s && *s != ' ') {
			s++;
		}

		len = s - start;
		if(len >= MAX_QPATH || len < 5) {
			CG_Error("PrecacheItem: %s has bad precache string", item->classname);
			return;
		}
		memcpy(data, start, len);
		data[len] = 0;
		if(*s) {
			s++;
		}

		if(!strcmp(data + len - 3, "wav")) {
			trap_S_RegisterSound(data, false);
		}
	}
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds(void) {
	int i;
	char name[MAX_QPATH];
	const char* soundName;

	cgs.media.oneMinuteSound = trap_S_RegisterSound("sound/feedback/1_minute.wav", true);
	cgs.media.fiveMinuteSound = trap_S_RegisterSound("sound/feedback/5_minute.wav", true);
	cgs.media.suddenDeathSound = trap_S_RegisterSound("sound/feedback/sudden_death.wav", true);
	cgs.media.oneFragSound = trap_S_RegisterSound("sound/feedback/1_frag.wav", true);
	cgs.media.twoFragSound = trap_S_RegisterSound("sound/feedback/2_frags.wav", true);
	cgs.media.threeFragSound = trap_S_RegisterSound("sound/feedback/3_frags.wav", true);

	cgs.media.selectSound = trap_S_RegisterSound("sound/weapons/wpn_moveselect", false);

	cgs.media.talkSound = trap_S_RegisterSound("sound/player/talk.wav", false);
	cgs.media.fallPain = trap_S_RegisterSound("sound/player/pl_fallpain3.wav", false);

	cgs.media.hitSound = trap_S_RegisterSound("sound/feedback/hit.wav", false);

	for(i = 0; i < 4; i++) {
		Com_sprintf(name, sizeof(name), "sound/weapons/physcannon/superphys_launch%i.wav", i + 1);
		cgs.media.gravityLaunch[i] = trap_S_RegisterSound(name, false);
	}

	for(i = 0; i < 3; i++) {
		Com_sprintf(name, sizeof(name), "sound/ambient/water/water_splash%i.wav", i + 1);
		cgs.media.waterSplash[i] = trap_S_RegisterSound(name, false);
	}

	for(i = 0; i < 4; i++) {
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/chainlink%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_CHAINLINK][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/concrete%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_CONCRETE][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/dirt%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_DIRT][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/duct%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_DUCT][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/grass%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_GRASS][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/gravel%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_GRAVEL][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/ladder%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_LADDER][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/metal%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/metalgrate%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_METALGRATE][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/mud%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_MUD][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/sand%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SAND][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/slosh%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SLOSH][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/tile%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_TILE][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/wood%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_WOOD][i] = trap_S_RegisterSound(name, false);
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/woodpanel%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_WOODPANEL][i] = trap_S_RegisterSound(name, false);
	}

	for(i = 0; i < jsd_itemCount; i++) CG_RegisterItemSounds(i);

	for(i = 1; i < MAX_SOUNDS; i++) {
		soundName = CG_ConfigString(CS_SOUNDS + i);
		if(!soundName[0]) {
			break;
		}
		if(soundName[0] == '*') {
			continue;  // custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound(soundName, false);
	}

	cgs.media.hgrenb1aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb1a.wav", false);
	cgs.media.hgrenb2aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb2a.wav", false);
}

//===================================================================================

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics(void) {
	int i;

	// clear any references to old media
	memset(&cg.refdef, 0, sizeof(cg.refdef));
	trap_R_ClearScene();

	CG_LoadingString(cgs.mapname, 0.40);

	trap_R_LoadWorldMap(cgs.mapname);

	CG_LoadingString("game media", 0.50);

	cgs.media.deferShader = trap_R_RegisterShaderNoMip("gfx/2d/defer.tga");

	cgs.media.smokePuffShader = trap_R_RegisterShader("spark");

	for(i = 0; i < NUM_CROSSHAIRS; i++) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShader(va("gfx/2d/crosshair%c", 'a' + i));
	}

	cgs.media.noammoShader = trap_R_RegisterShader("icons/noammo");

	// Assets Noire's Mod
	cgs.media.errIcon = trap_R_RegisterShaderNoMip("menu/erricon");
	cgs.media.notifyIcon = trap_R_RegisterShaderNoMip("menu/notifyicon");
	cgs.media.undoIcon = trap_R_RegisterShaderNoMip("menu/undoicon");
	cgs.media.undoSound = trap_S_RegisterSound("sound/undo.wav", false);

	for(i = 0; i < 4; i++) {
		char name[MAX_QPATH];
		Com_sprintf(name, sizeof(name), "sound/ambient/water/drip%i.wav", i + 1);
		cgs.media.notifySound[i] = trap_S_RegisterSound(name, false);
	}

	memset(cg_items, 0, sizeof(cg_items));
	memset(cg_weapons, 0, sizeof(cg_weapons));

	for(i = 0; i < jsd_itemCount; i++) {
		CG_LoadingItem(i);
		CG_RegisterItemVisuals(i);
	}

	// wall marks
	cgs.media.burnMarkShader = trap_R_RegisterShader("gfx/damage/burn_med_mrk");
	cgs.media.bloodMarkShader = trap_R_RegisterShader("bloodMark");

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for(i = 1; i < cgs.numInlineModels; i++) {
		char name[10];
		vec3_t mins, maxs;
		int j;

		Com_sprintf(name, sizeof(name), "*%i", i);
		cgs.inlineDrawModel[i] = trap_R_RegisterModel(name);
		trap_R_ModelBounds(cgs.inlineDrawModel[i], mins, maxs);
		for(j = 0; j < 3; j++) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
		}
	}

	// register all the server specified models
	for(i = 1; i < MAX_MODELS; i++) {
		const char* modelName;

		modelName = CG_ConfigString(CS_MODELS + i);
		if(!modelName[0]) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel(modelName);
	}

	CG_ClearParticles();
}

/*
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients(void) {
	int i;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum);

	for(i = 0; i < MAX_CLIENTS; i++) {
		const char* clientInfo;

		if(cg.clientNum == i) {
			continue;
		}

		clientInfo = CG_ConfigString(CS_PLAYERS + i);
		if(!clientInfo[0]) {
			continue;
		}
		CG_LoadingClient(i);
		CG_NewClientInfo(i);
	}
}

static char modelImporterStorage[65536];
static void CG_ImportModelsOBJ(void) {
	int numfiles = trap_FS_GetFileList("props", ".obj", modelImporterStorage, sizeof(modelImporterStorage));
	int i;
	char* file;

	file = modelImporterStorage;
	for(i = 0; i < numfiles; i++) {
		char nameWithoutExt[256];
		COM_StripExtension(va("props/%s", file), nameWithoutExt, 256);
		trap_ImportOBJ(nameWithoutExt);
		CG_LoadingString(va("import OBJ: %s", nameWithoutExt), 0.80);
		file += strlen(file) + 1;
	}
}

/*
=================
CG_ConfigString
=================
*/
const char* CG_ConfigString(int index) {
	if(index < 0 || index >= MAX_CONFIGSTRINGS) {
		CG_Error("CG_ConfigString: bad index: %i", index);
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

//==================================================================

/*
======================
CG_StartMusic
======================
*/
void CG_StartMusic(void) {
	char* s;
	char parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char*)CG_ConfigString(CS_MUSIC);
	Q_strncpyz(parm1, COM_Parse(&s), sizeof(parm1));
	Q_strncpyz(parm2, COM_Parse(&s), sizeof(parm2));

	trap_S_StartBackgroundTrack(parm1, parm2);
}

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum) {
	const char* s;

	// clear everything
	memset(&cgs, 0, sizeof(cgs));
	memset(&cg, 0, sizeof(cg));
	memset(cg_entities, 0, sizeof(cg_entities));
	memset(cg_weapons, 0, sizeof(cg_weapons));
	memset(cg_items, 0, sizeof(cg_items));

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.whiteShader = trap_R_RegisterShader("white");

	CG_RegisterCvars();

	JS_SystemInit(VM_CGAME);

	CL_UIInit();
	JS_HUDInit();

	CG_InitConsoleCommands();

	cg.weaponSelect = 0;

	// get the rendering configuration from the client system
	trap_GetGlconfig(&cgs.glconfig);

	// get the gamestate from the client system
	trap_GetGameState(&cgs.gameState);

	// check version
	s = CG_ConfigString(CS_GAME_VERSION);
	if(strcmp(s, GAME_VERSION)) {
		CG_Error("Client/Server game mismatch: %s/%s", GAME_VERSION, s);
	}

	s = CG_ConfigString(CS_LEVEL_START_TIME);
	cgs.levelStartTime = atoi(s);

	CG_ParseServerinfo();

	// load the new map
	CG_LoadingString("collision map", 0.10);

	trap_CM_LoadMap(cgs.mapname);

	cg.loading = true;  // force players to load instead of defer

	CG_LoadingString("sounds", 0.20);

	CG_RegisterSounds();

	CG_LoadingString("graphics", 0.30);

	CG_RegisterGraphics();

	CG_LoadingString("clients", 0.60);

	CG_RegisterClients();  // if low on memory, some clients will be deferred

	cg.loading = false;  // future players will be deferred

	CG_LoadingString("entities", 0.70);

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_ImportModelsOBJ();

	CG_StartMusic();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	CG_LoadingString("", -1);

	trap_S_ClearLoopingSounds(true);
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown(void) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}
