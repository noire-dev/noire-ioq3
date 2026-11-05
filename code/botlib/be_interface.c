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

/*****************************************************************************
 * name:		be_interface.c
 *
 * desc:		bot library interface
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_libvar.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"
#include "be_interface.h"

#include "be_ea.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"

// library globals in a structure
botlib_globals_t botlibglobals;

botlib_export_t be_botlib_export;
botlib_import_t botimport;
//
int botDeveloper;
// true if the library is setup
int botlibsetup = false;

//===========================================================================
//
// several functions used by the exported functions
//
//===========================================================================

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Sys_MilliSeconds(void) { return clock() * 1000 / CLOCKS_PER_SEC; }
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bool ValidClientNumber(int num, char* str) {
	if(num < 0 || num > botlibglobals.maxclients) {
		botimport.Print(PRT_ERROR, "%s: invalid client number %d, [0, %d]\n", str, num, botlibglobals.maxclients);
		return false;
	}
	return true;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bool ValidEntityNumber(int num, char* str) {
	if(num < 0 || num > botlibglobals.maxentities) {
		botimport.Print(PRT_ERROR, "%s: invalid entity number %d, [0, %d]\n", str, num, botlibglobals.maxentities);
		return false;
	}
	return true;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bool BotLibSetup(char* str) {
	if(!botlibglobals.botlibsetup) {
		botimport.Print(PRT_ERROR, "%s: bot library used before being setup\n", str);
		return false;
	}
	return true;
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibSetup(void) {
	int errnum;

	botDeveloper = LibVarGetValue("bot_developer");
	memset(&botlibglobals, 0, sizeof(botlibglobals));

	if(botDeveloper) {
		Log_Open("botlib.log");
	}

	botimport.Print(PRT_MESSAGE, "------- BotLib Initialization -------\n");

	botlibglobals.maxclients = (int)LibVarValue("maxclients", "128");
	botlibglobals.maxentities = (int)LibVarValue("maxentities", "4096");

	errnum = AAS_Setup();  // be_aas_main.c
	if(errnum != BLERR_NOERROR) return errnum;
	errnum = EA_Setup();  // be_ea.c
	if(errnum != BLERR_NOERROR) return errnum;
	errnum = BotSetupMoveAI();  // be_ai_move.c
	if(errnum != BLERR_NOERROR) return errnum;

	botlibsetup = true;
	botlibglobals.botlibsetup = true;

	return BLERR_NOERROR;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibShutdown(void) {
	if(!BotLibSetup("BotLibShutdown")) return BLERR_LIBRARYNOTSETUP;
	//
	BotShutdownMoveAI();  // be_ai_move.c
	// shut down aas
	AAS_Shutdown();
	// shut down bot elementary actions
	EA_Shutdown();
	// free all libvars
	LibVarDeAllocAll();

#ifdef DEBUG
	PrintMemoryLabels();
#endif
	// shut down library log file
	Log_Shutdown();
	//
	botlibsetup = false;
	botlibglobals.botlibsetup = false;
	//
	return BLERR_NOERROR;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibVarSet(const char* var_name, const char* value) {
	LibVarSet(var_name, value);
	return BLERR_NOERROR;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibStartFrame(float time) {
	if(!BotLibSetup("BotStartFrame")) return BLERR_LIBRARYNOTSETUP;
	return AAS_StartFrame(time);
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibLoadMap(const char* mapname) {
	int errnum;

	if(!BotLibSetup("BotLoadMap")) return BLERR_LIBRARYNOTSETUP;
	//
	botimport.Print(PRT_MESSAGE, "------------ Map Loading ------------\n");
	// startup AAS for the current map, model and sound index
	errnum = AAS_LoadMap(mapname);
	if(errnum != BLERR_NOERROR) return errnum;
	//
	BotSetBrushModelTypes();  // be_ai_move.h
	//
	botimport.Print(PRT_MESSAGE, "-------------------------------------\n");
	//
	return BLERR_NOERROR;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibUpdateEntity(int ent, bot_entitystate_t* state) {
	if(!BotLibSetup("BotUpdateEntity")) return BLERR_LIBRARYNOTSETUP;
	if(!ValidEntityNumber(ent, "BotUpdateEntity")) return BLERR_INVALIDENTITYNUMBER;

	return AAS_UpdateEntity(ent, state);
}

/*
============
Init_AAS_Export
============
*/
static void Init_AAS_Export(aas_export_t* aas) {
	//--------------------------------------------
	// be_aas_entity.c
	//--------------------------------------------
	aas->AAS_EntityInfo = AAS_EntityInfo;
	//--------------------------------------------
	// be_aas_main.c
	//--------------------------------------------
	aas->AAS_Initialized = AAS_Initialized;
	aas->AAS_PresenceTypeBoundingBox = AAS_PresenceTypeBoundingBox;
	aas->AAS_Time = AAS_Time;
	//--------------------------------------------
	// be_aas_sample.c
	//--------------------------------------------
	aas->AAS_PointAreaNum = AAS_PointAreaNum;
	aas->AAS_TraceAreas = AAS_TraceAreas;
	aas->AAS_BBoxAreas = AAS_BBoxAreas;
	aas->AAS_AreaInfo = AAS_AreaInfo;
	//--------------------------------------------
	// be_aas_bspq3.c
	//--------------------------------------------
	aas->AAS_PointContents = AAS_PointContents;
	aas->AAS_NextBSPEntity = AAS_NextBSPEntity;
	aas->AAS_ValueForBSPEpairKey = AAS_ValueForBSPEpairKey;
	aas->AAS_VectorForBSPEpairKey = AAS_VectorForBSPEpairKey;
	aas->AAS_FloatForBSPEpairKey = AAS_FloatForBSPEpairKey;
	aas->AAS_IntForBSPEpairKey = AAS_IntForBSPEpairKey;
	//--------------------------------------------
	// be_aas_reach.c
	//--------------------------------------------
	aas->AAS_AreaReachability = AAS_AreaReachability;
	//--------------------------------------------
	// be_aas_route.c
	//--------------------------------------------
	aas->AAS_AreaTravelTimeToGoalArea = AAS_AreaTravelTimeToGoalArea;
	aas->AAS_EnableRoutingArea = AAS_EnableRoutingArea;
	aas->AAS_PredictRoute = AAS_PredictRoute;
	//--------------------------------------------
	// be_aas_move.c
	//--------------------------------------------
	aas->AAS_Swimming = AAS_Swimming;
	aas->AAS_PredictClientMovement = AAS_PredictClientMovement;
}

/*
============
Init_EA_Export
============
*/
static void Init_EA_Export(ea_export_t* ea) {
	ea->EA_Attack = EA_Attack;
	ea->EA_Respawn = EA_Respawn;
	ea->EA_SelectWeapon = EA_SelectWeapon;
	ea->EA_View = EA_View;
	ea->EA_GetInput = EA_GetInput;
	ea->EA_ResetInput = EA_ResetInput;
}

/*
============
Init_AI_Export
============
*/
static void Init_AI_Export(ai_export_t* ai) {
	//-----------------------------------
	// be_ai_move.h
	//-----------------------------------
	ai->BotResetMoveState = BotResetMoveState;
	ai->BotMoveToGoal = BotMoveToGoal;
	ai->BotMoveInDirection = BotMoveInDirection;
	ai->BotResetAvoidReach = BotResetAvoidReach;
	ai->BotResetLastAvoidReach = BotResetLastAvoidReach;
	ai->BotMovementViewTarget = BotMovementViewTarget;
	ai->BotPredictVisiblePosition = BotPredictVisiblePosition;
	ai->BotAllocMoveState = BotAllocMoveState;
	ai->BotFreeMoveState = BotFreeMoveState;
	ai->BotInitMoveState = BotInitMoveState;
	ai->BotAddAvoidSpot = BotAddAvoidSpot;
	ai->BotTouchingGoal = BotTouchingGoal;
}

/*
============
GetBotLibAPI
============
*/
botlib_export_t* GetBotLibAPI(int apiVersion, botlib_import_t* import) {
	assert(import);
	botimport = *import;
	assert(botimport.Print);

	Com_Memset(&be_botlib_export, 0, sizeof(be_botlib_export));

	if(apiVersion != BOTLIB_API_VERSION) {
		botimport.Print(PRT_ERROR, "Mismatched BOTLIB_API_VERSION: expected %i, got %i\n", BOTLIB_API_VERSION, apiVersion);
		return NULL;
	}

	Init_AAS_Export(&be_botlib_export.aas);
	Init_EA_Export(&be_botlib_export.ea);
	Init_AI_Export(&be_botlib_export.ai);

	be_botlib_export.BotLibSetup = Export_BotLibSetup;
	be_botlib_export.BotLibShutdown = Export_BotLibShutdown;
	be_botlib_export.BotLibVarSet = Export_BotLibVarSet;

	be_botlib_export.BotLibStartFrame = Export_BotLibStartFrame;
	be_botlib_export.BotLibLoadMap = Export_BotLibLoadMap;
	be_botlib_export.BotLibUpdateEntity = Export_BotLibUpdateEntity;

	return &be_botlib_export;
}
