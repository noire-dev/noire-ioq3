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
#include "g_local.h"

static intptr_t(QDECL* syscall)(intptr_t arg, ...) = (intptr_t(QDECL*)(intptr_t, ...)) - 1;

Q_EXPORT void dllEntry(intptr_t(QDECL* syscallptr)(intptr_t arg, ...)) { syscall = syscallptr; }

int PASSFLOAT(float x) {
	floatint_t fi;
	fi.f = x;
	return fi.i;
}

void trap_LocateGameData(gentity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGClient) { syscall(G_LOCATE_GAME_DATA, gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGClient); }

void trap_DropClient(int clientNum, const char* reason) { syscall(G_DROP_CLIENT, clientNum, reason); }

void trap_SendServerCommand(int clientNum, const char* text) { syscall(G_SEND_SERVER_COMMAND, clientNum, text); }

void trap_SetConfigstring(int num, const char* string) { syscall(G_SET_CONFIGSTRING, num, string); }

void trap_GetConfigstring(int num, char* buffer, int bufferSize) { syscall(G_GET_CONFIGSTRING, num, buffer, bufferSize); }

void trap_GetUserinfo(int num, char* buffer, int bufferSize) { syscall(G_GET_USERINFO, num, buffer, bufferSize); }

void trap_SetUserinfo(int num, const char* buffer) { syscall(G_SET_USERINFO, num, buffer); }

void trap_GetServerinfo(char* buffer, int bufferSize) { syscall(G_GET_SERVERINFO, buffer, bufferSize); }

void trap_SetBrushModel(gentity_t* ent, const char* name) { syscall(G_SET_BRUSH_MODEL, ent, name); }

void trap_Trace(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) { syscall(G_TRACE, results, start, mins, maxs, end, passEntityNum, contentmask); }

int trap_PointContents(const vec3_t point, int passEntityNum) { return syscall(G_POINT_CONTENTS, point, passEntityNum); }

bool trap_InPVS(const vec3_t p1, const vec3_t p2) { return syscall(G_IN_PVS, p1, p2); }

bool trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2) { return syscall(G_IN_PVS_IGNORE_PORTALS, p1, p2); }

void trap_AdjustAreaPortalState(gentity_t* ent, bool open) { syscall(G_ADJUST_AREA_PORTAL_STATE, ent, open); }

bool trap_AreasConnected(int area1, int area2) { return syscall(G_AREAS_CONNECTED, area1, area2); }

void trap_LinkEntity(gentity_t* ent) { syscall(G_LINKENTITY, ent); }

void trap_UnlinkEntity(gentity_t* ent) { syscall(G_UNLINKENTITY, ent); }

int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int* list, int maxcount) { return syscall(G_ENTITIES_IN_BOX, mins, maxs, list, maxcount); }

bool trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t* ent) { return syscall(G_ENTITY_CONTACT, mins, maxs, ent); }


int trap_BotAllocateClient(void) { return syscall(G_BOT_ALLOCATE_CLIENT); }

void trap_BotFreeClient(int clientNum) { syscall(G_BOT_FREE_CLIENT, clientNum); }

void trap_GetUsercmd(int clientNum, usercmd_t* cmd) { syscall(G_GET_USERCMD, clientNum, cmd); }

bool trap_GetEntityToken(char* buffer, int bufferSize) { return syscall(G_GET_ENTITY_TOKEN, buffer, bufferSize); }

void trap_Phys_CreateShape(int modelindex, const char* objPath) { syscall(PHYS_CREATESHAPE, modelindex, objPath); }

void trap_Phys_CreateModel(int entityID, int modelindex, float x, float y, float z, float mass) { syscall(PHYS_CREATEMODEL, entityID, modelindex, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(z), PASSFLOAT(mass)); }

void trap_Phys_DeleteObject(int entityID) { syscall(PHYS_DELETEOBJECT, entityID); }

void trap_Phys_GetPosition(int entityID, float* x, float* y, float* z) { syscall(PHYS_GETPOSITION, entityID, x, y, z); }

void trap_Phys_SetPosition(int entityID, float x, float y, float z) { syscall(PHYS_SETPOSITION, entityID, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(z)); }

void trap_Phys_GetAngles(int entityID, float* pitch, float* yaw, float* roll) { syscall(PHYS_GETANGLES, entityID, pitch, yaw, roll); }

void trap_Phys_SetAngles(int entityID, float pitch, float yaw, float roll) { syscall(PHYS_SETANGLES, entityID, PASSFLOAT(pitch), PASSFLOAT(yaw), PASSFLOAT(roll)); }

void trap_Phys_GetLinearVelocity(int entityID, float* vx, float* vy, float* vz) { syscall(PHYS_GETLINEARVELOCITY, entityID, vx, vy, vz); }

void trap_Phys_SetLinearVelocity(int entityID, float vx, float vy, float vz) { syscall(PHYS_SETLINEARVELOCITY, entityID, PASSFLOAT(vx), PASSFLOAT(vy), PASSFLOAT(vz)); }

void trap_Phys_GetAngularVelocity(int entityID, float* vx, float* vy, float* vz) { syscall(PHYS_GETANGULARVELOCITY, entityID, vx, vy, vz); }

void trap_Phys_SetAngularVelocity(int entityID, float vx, float vy, float vz) { syscall(PHYS_SETANGULARVELOCITY, entityID, PASSFLOAT(vx), PASSFLOAT(vy), PASSFLOAT(vz)); }

void trap_Phys_ApplyCentralForce(int entityID, float fx, float fy, float fz) { syscall(PHYS_APPLYCENTRALFORCE, entityID, PASSFLOAT(fx), PASSFLOAT(fy), PASSFLOAT(fz)); }

void trap_Phys_FreezeObject(int entityID) { syscall(PHYS_FREEZEOBJECT, entityID); }

void trap_Phys_UnfreezeObject(int entityID) { syscall(PHYS_UNFREEZEOBJECT, entityID); }

void trap_Phys_Weld(int entA, int entB, float px, float py, float pz) { syscall(PHYS_WELD, entA, entB, PASSFLOAT(px), PASSFLOAT(py), PASSFLOAT(pz)); }

bool trap_Phys_IsSleeping(int entityID) { return syscall(PHYS_ISSLEEPING, entityID) != 0; }

void trap_Phys_CreateCapsule(int entityID, const physCapsuleDef_t* def) { syscall(PHYS_CREATECAPSULE, entityID, def); }

void trap_Phys_CreateConeTwist(const physConeTwistDef_t* def) { syscall(PHYS_CREATECONETWIST, def); }

void trap_Phys_IgnoreCollisionGroup(const int* entityIDs, int count) { syscall(PHYS_IGNORECOLLISIONGROUP, entityIDs, count); }

void trap_LocateRagdollData(ragdollState_t* states, int numStates) { syscall(G_LOCATE_RAGDOLL_DATA, states, numStates); }

bool trap_Ragdoll_LoadSkeleton(const char* modelPath, const char* animPath, int frame, ragdollSkeleton_t* out) { return syscall(G_RAGDOLL_LOADSKELETON, modelPath, animPath, frame, out) != 0; }

// BotLib traps start here
int trap_BotLibSetup(void) { return syscall(BOTLIB_SETUP); }

int trap_BotLibShutdown(void) { return syscall(BOTLIB_SHUTDOWN); }

int trap_BotLibVarSet(char* var_name, char* value) { return syscall(BOTLIB_LIBVAR_SET, var_name, value); }

int trap_BotLibStartFrame(float time) { return syscall(BOTLIB_START_FRAME, PASSFLOAT(time)); }

int trap_BotLibLoadMap(const char* mapname) { return syscall(BOTLIB_LOAD_MAP, mapname); }

int trap_BotLibUpdateEntity(int ent, void /* struct bot_updateentity_s */* bue) { return syscall(BOTLIB_UPDATENTITY, ent, bue); }

void trap_BotUserCommand(int clientNum, usercmd_t* ucmd) { syscall(BOTLIB_USER_COMMAND, clientNum, ucmd); }

void trap_AAS_EntityInfo(int entnum, void /* struct aas_entityinfo_s */* info) { syscall(BOTLIB_AAS_ENTITY_INFO, entnum, info); }

int trap_AAS_Initialized(void) { return syscall(BOTLIB_AAS_INITIALIZED); }

void trap_AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs) { syscall(BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX, presencetype, mins, maxs); }

float trap_AAS_Time(void) {
	floatint_t fi;
	fi.i = syscall(BOTLIB_AAS_TIME);
	return fi.f;
}

int trap_AAS_PointAreaNum(vec3_t point) { return syscall(BOTLIB_AAS_POINT_AREA_NUM, point); }

int trap_AAS_TraceAreas(vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas) { return syscall(BOTLIB_AAS_TRACE_AREAS, start, end, areas, points, maxareas); }

int trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int* areas, int maxareas) { return syscall(BOTLIB_AAS_BBOX_AREAS, absmins, absmaxs, areas, maxareas); }

int trap_AAS_AreaInfo(int areanum, void /* struct aas_areainfo_s */* info) { return syscall(BOTLIB_AAS_AREA_INFO, areanum, info); }

int trap_AAS_PointContents(vec3_t point) { return syscall(BOTLIB_AAS_POINT_CONTENTS, point); }

int trap_AAS_NextBSPEntity(int ent) { return syscall(BOTLIB_AAS_NEXT_BSP_ENTITY, ent); }

int trap_AAS_ValueForBSPEpairKey(int ent, char* key, char* value, int size) { return syscall(BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY, ent, key, value, size); }

int trap_AAS_VectorForBSPEpairKey(int ent, char* key, vec3_t v) { return syscall(BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY, ent, key, v); }

int trap_AAS_FloatForBSPEpairKey(int ent, char* key, float* value) { return syscall(BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY, ent, key, value); }

int trap_AAS_IntForBSPEpairKey(int ent, char* key, int* value) { return syscall(BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY, ent, key, value); }

int trap_AAS_AreaReachability(int areanum) { return syscall(BOTLIB_AAS_AREA_REACHABILITY, areanum); }

int trap_AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags) { return syscall(BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA, areanum, origin, goalareanum, travelflags); }

int trap_AAS_EnableRoutingArea(int areanum, int enable) { return syscall(BOTLIB_AAS_ENABLE_ROUTING_AREA, areanum, enable); }

int trap_AAS_PredictRoute(void /*struct aas_predictroute_s*/* route, int areanum, vec3_t origin, int goalareanum, int travelflags, int maxareas, int maxtime, int stopevent, int stopcontents, int stoptfl, int stopareanum) { return syscall(BOTLIB_AAS_PREDICT_ROUTE, route, areanum, origin, goalareanum, travelflags, maxareas, maxtime, stopevent, stopcontents, stoptfl, stopareanum); }

int trap_AAS_Swimming(vec3_t origin) { return syscall(BOTLIB_AAS_SWIMMING, origin); }

int trap_AAS_PredictClientMovement(void /* struct aas_clientmove_s */* move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize) { return syscall(BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT, move, entnum, origin, presencetype, onground, velocity, cmdmove, cmdframes, maxframes, PASSFLOAT(frametime), stopevent, stopareanum, visualize); }

void trap_EA_Attack(int client) { syscall(BOTLIB_EA_ATTACK, client); }

void trap_EA_Respawn(int client) { syscall(BOTLIB_EA_RESPAWN, client); }

void trap_EA_SelectWeapon(int client, int weapon) { syscall(BOTLIB_EA_SELECT_WEAPON, client, weapon); }

void trap_EA_View(int client, vec3_t viewangles) { syscall(BOTLIB_EA_VIEW, client, viewangles); }

void trap_EA_GetInput(int client, float thinktime, void /* struct bot_input_s */* input) { syscall(BOTLIB_EA_GET_INPUT, client, PASSFLOAT(thinktime), input); }

void trap_EA_ResetInput(int client) { syscall(BOTLIB_EA_RESET_INPUT, client); }




int trap_BotGetServerCommand(int client, char* message, int size) { return syscall(BOTLIB_GET_CONSOLE_MESSAGE, client, message, size); }
int trap_BotTouchingGoal(vec3_t origin, void /* struct bot_goal_s */* goal) { return syscall(BOTLIB_AI_TOUCHING_GOAL, origin, goal); }

void trap_BotResetMoveState(int movestate) { syscall(BOTLIB_AI_RESET_MOVE_STATE, movestate); }

void trap_BotAddAvoidSpot(int movestate, vec3_t origin, float radius, int type) { syscall(BOTLIB_AI_ADD_AVOID_SPOT, movestate, origin, PASSFLOAT(radius), type); }

void trap_BotMoveToGoal(void /* struct bot_moveresult_s */* result, int movestate, void /* struct bot_goal_s */* goal, int travelflags) { syscall(BOTLIB_AI_MOVE_TO_GOAL, result, movestate, goal, travelflags); }

int trap_BotMoveInDirection(int movestate, vec3_t dir, float speed, int type) { return syscall(BOTLIB_AI_MOVE_IN_DIRECTION, movestate, dir, PASSFLOAT(speed), type); }

void trap_BotResetAvoidReach(int movestate) { syscall(BOTLIB_AI_RESET_AVOID_REACH, movestate); }

void trap_BotResetLastAvoidReach(int movestate) { syscall(BOTLIB_AI_RESET_LAST_AVOID_REACH, movestate); }

int trap_BotMovementViewTarget(int movestate, void /* struct bot_goal_s */* goal, int travelflags, float lookahead, vec3_t target) { return syscall(BOTLIB_AI_MOVEMENT_VIEW_TARGET, movestate, goal, travelflags, PASSFLOAT(lookahead), target); }

int trap_BotPredictVisiblePosition(vec3_t origin, int areanum, void /* struct bot_goal_s */* goal, int travelflags, vec3_t target) { return syscall(BOTLIB_AI_PREDICT_VISIBLE_POSITION, origin, areanum, goal, travelflags, target); }

int trap_BotAllocMoveState(void) { return syscall(BOTLIB_AI_ALLOC_MOVE_STATE); }

void trap_BotFreeMoveState(int handle) { syscall(BOTLIB_AI_FREE_MOVE_STATE, handle); }

void trap_BotInitMoveState(int handle, void /* struct bot_initmove_s */* initmove) { syscall(BOTLIB_AI_INIT_MOVE_STATE, handle, initmove); }

#include "../qcommon/vm_sharedsyscalls.inc"
