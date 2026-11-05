// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2

#include "vm_javascript.h"
#include "q_shared.h"

js_args_t vmargs;
js_result_t vmresult;
js_args_t jsargs;
js_result_t jsresult;

void JS_StackClean(void) { memset(&jsargs, 0, sizeof(jsargs)); }
void JS_Function(int func_id) {
	memset(&jsresult, 0, sizeof(jsresult));
	JSCall(func_id, &jsargs, &jsresult);
}

static int api_foundEntry = false;
static int api_valueIndex = 0;
static int api_counter = 0;

void JSE_IntRead(int number) {
	api_foundEntry = true;
	if(vmargs.t[api_valueIndex] != JS_TYPE_NONE) trap_Print("JSE_IntRead: read only from JS");
	vmresult.t = JS_TYPE_INT;
	vmresult.v.i = number;
}

void JSE_Int(int* number) {
	api_foundEntry = true;
	if(vmargs.t[api_valueIndex] != JS_TYPE_NONE) *number = vmargs.v[api_valueIndex].i;
	vmresult.t = JS_TYPE_INT;
	vmresult.v.i = *number;
}

void JSE_Float(float* number) {
	api_foundEntry = true;
	if(vmargs.t[api_valueIndex] != JS_TYPE_NONE) *number = vmargs.v[api_valueIndex].f;
	vmresult.t = JS_TYPE_FLOAT;
	vmresult.v.f = *number;
}

void JSE_String(char* string, size_t maxSize) {
	api_foundEntry = true;
	if(vmargs.t[api_valueIndex] != JS_TYPE_NONE) Q_strncpyz(string, vmargs.v[api_valueIndex].s, maxSize);
	vmresult.t = JS_TYPE_STRING;
	Q_strncpyz(vmresult.v.s, string, MAX_JS_STRINGSIZE);
}

void JSE_Bool(bool* value) {
	api_foundEntry = true;
	if(vmargs.t[api_valueIndex] != JS_TYPE_NONE) *value = (vmargs.v[api_valueIndex].i != 0);
	vmresult.t = JS_TYPE_INT;
	vmresult.v.i = *value;
}

#ifdef GAME
void JSE_Entity(gentity_t* ent) {
	api_foundEntry = true;
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

void JS_SystemInit(int vmIndex) {
	if(vmIndex == VM_GAME) trap_Print("JS_SystemInit for game\n");
	if(vmIndex == VM_CGAME) trap_Print("JS_SystemInit for cgame\n");
	if(vmIndex == VM_UI) trap_Print("JS_SystemInit for ui\n");

	// Weapons first, so js/items/*.js can link pickups to weapon.xxx ids
	JS_WeaponInit(vmIndex);
	JS_ItemInit(vmIndex);
}

static void API_Item(int itemID, int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = false;
	api_valueIndex = 2;
	api_counter = 0;

	bounds(itemID, MAX_ITEMS, "API_Item - itemID");

	API_FIELD(JSE_String(jsd_item[itemID].classname, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_String(jsd_item[itemID].pickup_sound, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_String(jsd_item[itemID].pickup_name, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_String(jsd_item[itemID].model, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_String(jsd_item[itemID].icon, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_Int(&jsd_item[itemID].quantity))
	API_FIELD(JSE_Int((int*)&jsd_item[itemID].giType))
	API_FIELD(JSE_Int(&jsd_item[itemID].giTag))
	API_FIELD(JSE_String(jsd_item[itemID].precaches, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_String(jsd_item[itemID].sounds, MAX_JS_STRINGSIZE))

	API_FIELD(JSE_Int(&jsd_itemCount))
}

static void API_Weapon(int weaponID, int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = false;
	api_valueIndex = 2;
	api_counter = 0;

	bounds(weaponID, MAX_WEAPONS, "API_Weapon - weaponID");

	API_FIELD(JSE_Int((int*)&jsd_weapon[weaponID].wType))
	API_FIELD(JSE_Int((int*)&jsd_weapon[weaponID].wAnimType))
	API_FIELD(JSE_String(jsd_weapon[weaponID].classname, MAX_JS_STRINGSIZE))
	API_FIELD(JSE_Int(&jsd_weapon[weaponID].delay))
	API_FIELD(JSE_Int(&jsd_weapon[weaponID].count))
	API_FIELD(JSE_Int(&jsd_weapon[weaponID].damage))
	API_FIELD(JSE_Int(&jsd_weapon[weaponID].splashDamage))
	API_FIELD(JSE_Int(&jsd_weapon[weaponID].splashRadius))
	API_FIELD(JSE_Int(&jsd_weapon[weaponID].range))
	API_FIELD(JSE_Int(&jsd_weapon[weaponID].speed))
	API_FIELD(JSE_Int(&jsd_weapon[weaponID].speedRandom))
	API_FIELD(JSE_Int(&jsd_weapon[weaponID].spread))
	API_FIELD(JSE_Int(&jsd_weapon[weaponID].timeout))
	API_FIELD(JSE_Bool(&jsd_weapon[weaponID].gravity))
	API_FIELD(JSE_Bool(&jsd_weapon[weaponID].bounce))
	API_FIELD(JSE_Float(&jsd_weapon[weaponID].bounceModifier))
	API_FIELD(JSE_Float(&jsd_weapon[weaponID].zOffset))

	API_FIELD(JSE_Int(&jsd_weaponCount))
}

#ifdef GAME
static void GAPI_EntityDelete(gentity_t* entity) {
	if(!entity) return;

	if(entity->client) {
		// bots (npc) can be removed, real players can't
		if(entity->r.svFlags & SVF_BOT) trap_DropClient(entity->s.number, "NPC removed");
		return;
	}

	G_FreeEntity(entity);
}

static float GAPI_PlayerCrosshairPoint(gentity_t* player, float distance, int xyz) {
	vec3_t end, start, forward, up, right;
	trace_t tr;

	bounds(xyz, 3, "GAPI_PlayerCrosshairPoint - xyz");

	// Set Aiming Directions
	AngleVectors(player->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint(player, forward, right, up, start);
	VectorMA(start, distance, forward, end);

	// Trace Position
	trap_Trace(&tr, start, NULL, NULL, end, player->s.number, MASK_SELECT);

	return tr.endpos[xyz];
}

static void GAPI_SetEntityModel(gentity_t* entity, char* modelName) {
	int len;

	if(!entity) {
		Com_Printf("GAPI_SetEntityModel: entity is NULL\n");
		return;
	}

	len = strlen(modelName);
	if(!len) {
		Com_Printf("GAPI_SetEntityModel: modelName is empty\n");
		return;
	}

	entity->s.modelindex = G_ModelIndex(modelName);
	CopyAlloc(entity->model, modelName);

	if(len >= 4 && !Q_stricmp(entity->model + len - 4, ".md3")) entity->model[len - 4] = '\0';

	trap_Phys_CreateShape(entity->s.modelindex, va("%s.md3", entity->model));
}

static void GAPI_SetEntityPosition(gentity_t* entity, float x, float y, float z) {
	if(!entity) {
		Com_Printf("GAPI_SetEntityPosition: entity is NULL\n");
		return;
	}

	entity->s.origin[0] = x;
	entity->s.origin[1] = y;
	entity->s.origin[2] = z;
}

static void GAPI_SetEntityAngles(gentity_t* entity, float x, float y, float z) {
	if(!entity) {
		Com_Printf("GAPI_SetEntityAngles: entity is NULL\n");
		return;
	}

	entity->s.angles[0] = x;
	entity->s.angles[1] = y;
	entity->s.angles[2] = z;
}

static int GAPI_EntitySpawn(float x, float y, float z, float ax, float ay, float az, char* modelName) {
	gentity_t* entity;
	entity = G_Spawn();
	entity->s.eType = ET_PHYSICS;
	entity->takedamage = true;
	entity->s.pos.trType = TR_INTERPOLATE;
	GAPI_SetEntityModel(entity, modelName);
	GAPI_SetEntityPosition(entity, x, y, z);
	GAPI_SetEntityAngles(entity, ax, ay, az);
	trap_Phys_CreateModel(entity->s.number, entity->s.modelindex, entity->s.origin[0], entity->s.origin[1], entity->s.origin[2], 10);
	trap_Phys_SetAngles(entity->s.number, entity->s.angles[0], entity->s.angles[1], entity->s.angles[2]);
	trap_Phys_UnfreezeObject(entity->s.number);

	return entity->s.number;
}

/*
================
GAPI_NPCSpawn

Spawns an npc (bot) and teleports it to the given position.
Everything is addressed by entity number: for clients the entity
number is the same as the client number.
Returns the npc entity number or -1 on failure.
================
*/
static int GAPI_NPCSpawn(char* npcName, char* npcModel, float skill, float x, float y, float z, float yaw) {
	int clientNum;
	char userinfo[MAX_INFO_STRING];
	gentity_t* entity;
	vec3_t angles;

	if(!trap_Cvar_VariableIntegerValue("bot_enable")) {
		trap_Print("GAPI_NPCSpawn: bot_enable is 0, run 'bot_enable 1' and restart the map\n");
		return -1;
	}

	if(!npcName || !npcName[0]) npcName = "NPC";
	if(!npcModel || !npcModel[0]) npcModel = "npc/default";

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if(clientNum == -1) {
		trap_Print("GAPI_NPCSpawn: no free client slots, raise sv_maxclients\n");
		return -1;
	}

	// create the npc's userinfo
	userinfo[0] = '\0';
	Info_SetValueForKey(userinfo, "name", npcName);
	Info_SetValueForKey(userinfo, "model", npcModel);
	Info_SetValueForKey(userinfo, "rate", "25000");
	Info_SetValueForKey(userinfo, "snaps", "20");
	Info_SetValueForKey(userinfo, "skill", va("%.2f", skill));
	trap_SetUserinfo(clientNum, userinfo);

	// have it connect to the game as a normal client
	if(ClientConnect(clientNum, true, true)) {
		trap_Print("GAPI_NPCSpawn: ClientConnect failed\n");
		return -1;
	}

	ClientBegin(clientNum);

	// teleport the npc to the requested position
	entity = &g_entities[clientNum];

	trap_UnlinkEntity(entity);

	entity->client->ps.origin[0] = x;
	entity->client->ps.origin[1] = y;
	entity->client->ps.origin[2] = z;

	VectorSet(angles, 0, yaw, 0);
	SetClientViewAngle(entity, angles);

	VectorCopy(entity->client->ps.origin, entity->r.currentOrigin);
	G_SetOrigin(entity, entity->client->ps.origin);

	trap_LinkEntity(entity);

	return clientNum;
}

/*
================
Entity API

All entity / player / npc functions take an entity number
(g_entities index), so one call works for players, npcs and props.
The JSE_ helpers read the value from JS when JS passes one,
otherwise they only return the current value.
================
*/

static gentity_t* GAPI_Entity(int entityID, char* apiName) {
	gentity_t* entity;

	bounds(entityID, MAX_GENTITIES, apiName);

	entity = &g_entities[entityID];
	if(!entity->inuse || !entity->client) {
		trap_Print(va("#ff5%s: entity %d is not a client\n", apiName, entityID));
		return NULL;
	}

	return entity;
}

// safe default for the value-returning entity getters, so an error
// never leaks a stale result back into JS
static void GAPI_EntityResultDefault(void) {
	vmresult.t = JS_TYPE_INT;
	vmresult.v.i = 0;
}

static void GAPI_EntityStat(int entityID, int statID) {
	gentity_t* entity;

	api_foundEntry = false;
	api_valueIndex = 2;
	GAPI_EntityResultDefault();

	entity = GAPI_Entity(entityID, "GAPI_EntityStat - entityID");
	if(!entity) return;

	bounds(statID, MAX_STATS, "GAPI_EntityStat - statID");

	JSE_Int(&entity->client->ps.stats[statID]);

	// health is the damage/death source of truth, ps.stats only mirrors it for
	// the hud, so keep ent->health in sync when js writes STAT_HEALTH
	if(statID == STAT_HEALTH && vmargs.t[api_valueIndex] != JS_TYPE_NONE) {
		entity->health = entity->client->ps.stats[STAT_HEALTH];
	}
}

static void GAPI_EntitySpeed(int entityID) {
	gentity_t* entity;

	api_foundEntry = false;
	api_valueIndex = 2;
	GAPI_EntityResultDefault();

	entity = GAPI_Entity(entityID, "GAPI_EntitySpeed - entityID");
	if(!entity) return;
}

static void GAPI_EntityAmmo(int entityID, int weaponID) {
	gentity_t* entity;

	api_foundEntry = false;
	api_valueIndex = 2;
	GAPI_EntityResultDefault();

	entity = GAPI_Entity(entityID, "GAPI_EntityAmmo - entityID");
	if(!entity) return;

	bounds(weaponID, MAX_WEAPONS, "GAPI_EntityAmmo - weaponID");
	JSE_Int(&entity->swep_ammo[weaponID]);
}

static void GAPI_EntityWeapon(int entityID, int weaponID) {
	gentity_t* entity;

	api_foundEntry = false;
	api_valueIndex = 2;
	GAPI_EntityResultDefault();

	entity = GAPI_Entity(entityID, "GAPI_EntityWeapon - entityID");
	if(!entity) return;

	bounds(weaponID, MAX_WEAPONS, "GAPI_EntityWeapon - weaponID");
	// swep_list status: WS_NONE 0, WS_HAVE 1, WS_NOAMMO 2
	JSE_Int(&entity->swep_list[weaponID]);
}

static void GAPI_EntityActiveWeapon(int entityID) {
	gentity_t* entity;

	api_foundEntry = false;
	api_valueIndex = 2;
	GAPI_EntityResultDefault();

	entity = GAPI_Entity(entityID, "GAPI_EntityActiveWeapon - entityID");
	if(!entity) return;

	JSE_Int(&entity->client->ps.weapon);
}

static void GAPI_EntityInventory(int entityID) {
	gentity_t* entity;
	char inventory[MAX_JS_STRINGSIZE];
	int weaponID;

	api_foundEntry = true;
	api_valueIndex = 2;

	vmresult.t = JS_TYPE_STRING;
	vmresult.v.s[0] = '\0';

	entity = GAPI_Entity(entityID, "GAPI_EntityInventory - entityID");
	if(!entity) return;

	inventory[0] = '\0';
	for(weaponID = 1; weaponID < jsd_weaponCount && weaponID < MAX_WEAPONS; weaponID++) {
		if(entity->swep_list[weaponID] < WS_HAVE) continue;

		if(inventory[0]) Q_strcat(inventory, sizeof(inventory), ",");
		Q_strcat(inventory, sizeof(inventory), va("%i:%i", weaponID, entity->swep_ammo[weaponID]));
	}

	StringCopy(vmresult.v.s, inventory, MAX_JS_STRINGSIZE);
}

static int GAPI_EntityIsClient(int entityID) {
	gentity_t* entity;

	bounds(entityID, MAX_GENTITIES, "GAPI_EntityIsClient - entityID");

	entity = &g_entities[entityID];
	if(!entity->inuse || !entity->client) return 0;
	if(entity->client->pers.connected != CON_CONNECTED) return 0;

	return 1;
}
#endif

#ifdef CGAME
static void API_HUD(int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = false;
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
	api_foundEntry = false;
	api_valueIndex = 1;
	api_counter = 0;

	API_FIELD(bounds(i1, MAX_STATS, "API_ClientPlayer -> stats"); JSE_Int(&cg.predictedPlayerState.stats[i1]))
}

static void API_WeaponFX(int weaponID, int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = false;
	api_valueIndex = 2;
	api_counter = 0;

	bounds(weaponID, MAX_WEAPONS, "API_WeaponFX - weaponID");
	weaponFx_t* fx = &jsd_weaponFx[weaponID];

	API_FIELD(JSE_Int((int*)&fx->animType))
	API_FIELD(JSE_Int(&fx->weaponSection))
	API_FIELD(JSE_Int(&fx->sectionOrder))
	API_FIELD(JSE_Bool(&fx->colorizeItemIcon))

	API_FIELD(bounds(i1, 3, "API_WeaponFX -> hitLightColor"); JSE_Float(&fx->hitLightColor[i1]))
	API_FIELD(JSE_Float(&fx->hitLightRadius))
	API_FIELD(JSE_Int(&fx->hitLightTime))
	API_FIELD(bounds(i1, 4, "API_WeaponFX -> hitParticleColor"); JSE_Float(&fx->hitParticleColor[i1]))
	API_FIELD(JSE_Int(&fx->hitParticleCount))
	API_FIELD(JSE_Float(&fx->hitParticleSpeed))
	API_FIELD(JSE_Int(&fx->hitParticleFlags))
	API_FIELD(bounds(i1, 3, "API_WeaponFX -> hitParticleGravity"); JSE_Float(&fx->hitParticleGravity[i1]))
	API_FIELD(JSE_Float(&fx->hitParticleSize))
	API_FIELD(JSE_Float(&fx->hitParticleLifetime))
	API_FIELD(JSE_String(fx->hitParticleShader, MAX_QPATH))

	API_FIELD(JSE_Bool(&fx->continuousFire))
	API_FIELD(JSE_Bool(&fx->suppressRepeatFire))
	API_FIELD(bounds(i1, 3, "API_WeaponFX -> fireLightColor"); JSE_Float(&fx->fireLightColor[i1]))
	API_FIELD(JSE_Float(&fx->fireLightRadius))
	API_FIELD(JSE_Int(&fx->fireLightTime))
	API_FIELD(bounds(i1, 4, "API_WeaponFX -> fireParticleColor"); JSE_Float(&fx->fireParticleColor[i1]))
	API_FIELD(JSE_Int(&fx->fireParticleCount))
	API_FIELD(JSE_Float(&fx->fireParticleSpeed))
	API_FIELD(JSE_Int(&fx->fireParticleFlags))
	API_FIELD(bounds(i1, 3, "API_WeaponFX -> fireParticleGravity"); JSE_Float(&fx->fireParticleGravity[i1]))
	API_FIELD(JSE_Float(&fx->fireParticleSize))
	API_FIELD(JSE_Float(&fx->fireParticleLifetime))
	API_FIELD(JSE_String(fx->fireParticleShader, MAX_QPATH))

	API_FIELD(bounds(i1, 4, "API_WeaponFX -> trailColor"); JSE_Float(&fx->trailColor[i1]))
	API_FIELD(JSE_Int(&fx->trailCount))
	API_FIELD(JSE_Float(&fx->trailSpeed))
	API_FIELD(JSE_Int(&fx->trailFlags))
	API_FIELD(bounds(i1, 3, "API_WeaponFX -> trailGravity"); JSE_Float(&fx->trailGravity[i1]))
	API_FIELD(JSE_Float(&fx->trailSize))
	API_FIELD(JSE_Float(&fx->trailLifetime))
	API_FIELD(JSE_String(fx->trailShader, MAX_QPATH))
	API_FIELD(JSE_Int(&fx->trailInterval))

	API_FIELD(bounds(i1, 3, "API_WeaponFX -> missileLightColor"); JSE_Float(&fx->missileLightColor[i1]))
	API_FIELD(JSE_Float(&fx->missileLightRadius))

	API_FIELD(JSE_Float(&fx->tracerChance))
	API_FIELD(JSE_Float(&fx->tracerLength))
	API_FIELD(JSE_String(fx->tracerParticleShader, MAX_QPATH))
	API_FIELD(bounds(i1, 4, "API_WeaponFX -> tracerParticleColor"); JSE_Float(&fx->tracerParticleColor[i1]))
	API_FIELD(JSE_Int(&fx->tracerParticleCount))
	API_FIELD(JSE_Float(&fx->tracerParticleSpeed))
	API_FIELD(JSE_Int(&fx->tracerParticleFlags))
	API_FIELD(bounds(i1, 3, "API_WeaponFX -> tracerParticleGravity"); JSE_Float(&fx->tracerParticleGravity[i1]))
	API_FIELD(JSE_Float(&fx->tracerParticleSize))
	API_FIELD(JSE_Float(&fx->tracerParticleLifetime))
	API_FIELD(JSE_String(fx->tracerSound, MAX_QPATH))

	API_FIELD(JSE_Bool(&fx->isBeam))
	API_FIELD(JSE_String(fx->beamShader, MAX_QPATH))
	API_FIELD(bounds(i1, 4, "API_WeaponFX -> beamColor"); JSE_Float(&fx->beamColor[i1]))
	API_FIELD(JSE_Bool(&fx->beamColorizeByClient))
	API_FIELD(bounds(i1, 3, "API_WeaponFX -> beamStartOffset"); JSE_Float(&fx->beamStartOffset[i1]))
	API_FIELD(bounds(i1, 3, "API_WeaponFX -> beamEndOffset"); JSE_Float(&fx->beamEndOffset[i1]))

	API_FIELD(JSE_String(fx->readySound, MAX_QPATH))
	API_FIELD(bounds(i1, 4, "API_WeaponFX -> fireSound"); JSE_String(fx->fireSound[i1], MAX_QPATH))
	API_FIELD(JSE_String(fx->continuousFireSound, MAX_QPATH))
	API_FIELD(JSE_String(fx->missileFlySound, MAX_QPATH))
	API_FIELD(JSE_String(fx->missileBounceSound, MAX_QPATH))
	API_FIELD(JSE_String(fx->missileExplodeSound, MAX_QPATH))

	API_FIELD(JSE_String(fx->missileModel, MAX_QPATH))
	API_FIELD(JSE_String(fx->missileSprite, MAX_QPATH))
	API_FIELD(JSE_Int(&fx->missileRenderfx))

	API_FIELD(JSE_Int(&fx->modelColorSource))
	API_FIELD(bounds(i1, 3, "API_WeaponFX -> modelColor"); JSE_Float(&fx->modelColor[i1]))
	API_FIELD(JSE_Bool(&fx->explodeOnFleshHit))

	API_FIELD(JSE_String(fx->flashModel, MAX_QPATH))
	API_FIELD(JSE_String(fx->muzzleBone, MAX_QPATH))

	API_FIELD(JSE_String(fx->brassModel, MAX_QPATH))
	API_FIELD(JSE_Int(&fx->brassCount))
	API_FIELD(bounds(i1, 3, "API_WeaponFX -> brassOffset"); JSE_Float(&fx->brassOffset[i1]))
	API_FIELD(bounds(i1, 3, "API_WeaponFX -> brassVelocity"); JSE_Float(&fx->brassVelocity[i1]))
	API_FIELD(bounds(i1, 3, "API_WeaponFX -> brassVelocitySpread"); JSE_Float(&fx->brassVelocitySpread[i1]))
	API_FIELD(JSE_Float(&fx->brassBounceFactor))
	API_FIELD(JSE_Int(&fx->brassLifetimeMult))
	API_FIELD(bounds(i1, 3, "API_WeaponFX -> brassSpinDelta"); JSE_Float(&fx->brassSpinDelta[i1]))
}
#endif

#ifndef GAME
static void API_CGUI(int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = false;
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
	api_foundEntry = false;
	api_valueIndex = 1;
	api_counter = 0;

	API_FIELD(JSE_Int(&glconfig.vidWidth))
	API_FIELD(JSE_Int(&glconfig.vidHeight))
	API_FIELD(JSE_Int((int*)&glconfig.isFullscreen))
}
#endif

#ifdef UI
static void API_Shell(int fieldID, int i1, int i2, int i3, int i4) {
	api_foundEntry = false;
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
	api_foundEntry = false;
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
	api_foundEntry = false;
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
	API_FIELD(bounds(i1, ELEMENT_OPTIONSCOUNT, "API_Element -> options"); JSE_String(shell.window[windowID].element[elementID].options[i1], ELEMENT_OPTIONLENGTH))
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
		case VM_APIITEM: API_Item(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[3].i, vmargs.v[4].i, vmargs.v[5].i, vmargs.v[6].i); break;
		case VM_CMD: trap_SendConsoleCommand(vmargs.v[0].i, vmargs.v[1].s); break;
		case VM_APIWEAPON: API_Weapon(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[3].i, vmargs.v[4].i, vmargs.v[5].i, vmargs.v[6].i); break;
#ifdef GAME
		case VM_GAPIENTITYDELETE: GAPI_EntityDelete(&g_entities[vmargs.v[0].i]); break;
		case VM_GAPIENTITYSPAWN:
			vmresult.v.i = GAPI_EntitySpawn(vmargs.v[0].f, vmargs.v[1].f, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, vmargs.v[5].f, vmargs.v[6].s);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_GAPIPLAYERCROSHAIRPOINT:
			vmresult.v.f = GAPI_PlayerCrosshairPoint(&g_entities[vmargs.v[0].i], vmargs.v[1].f, vmargs.v[2].i);
			vmresult.t = JS_TYPE_FLOAT;
			break;
		case VM_GAPINPCSPAWN:
			vmresult.v.i = GAPI_NPCSpawn(vmargs.v[0].s, vmargs.v[1].s, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, vmargs.v[5].f, vmargs.v[6].f);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_GAPIENTITYSTAT: GAPI_EntityStat(vmargs.v[0].i, vmargs.v[1].i); break;
		case VM_GAPIENTITYSPEED: GAPI_EntitySpeed(vmargs.v[0].i); break;
		case VM_GAPIENTITYAMMO: GAPI_EntityAmmo(vmargs.v[0].i, vmargs.v[1].i); break;
		case VM_GAPIENTITYWEAPON: GAPI_EntityWeapon(vmargs.v[0].i, vmargs.v[1].i); break;
		case VM_GAPIENTITYACTIVEWEAPON: GAPI_EntityActiveWeapon(vmargs.v[0].i); break;
		case VM_GAPIENTITYINVENTORY: GAPI_EntityInventory(vmargs.v[0].i); break;
		case VM_GAPIENTITYISCLIENT:
			vmresult.v.i = GAPI_EntityIsClient(vmargs.v[0].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_GAPISPAWNINT: {
			int out = 0;
			G_SpawnInt(vmargs.v[0].s, vmargs.v[1].s, &out);
			vmresult.v.i = out;
			vmresult.t = JS_TYPE_INT;
			break;
		}
		case VM_GAPISPAWNFLOAT: {
			float out = 0.0f;
			G_SpawnFloat(vmargs.v[0].s, vmargs.v[1].s, &out);
			vmresult.v.f = out;
			vmresult.t = JS_TYPE_FLOAT;
			break;
		}
		case VM_GAPISPAWNSTRING: {
			char* out = NULL;
			G_SpawnString(vmargs.v[0].s, vmargs.v[1].s, &out);
			StringCopy(vmresult.v.s, out ? out : "", MAX_JS_STRINGSIZE);
			vmresult.t = JS_TYPE_STRING;
			break;
		}
		case VM_GAPISPAWNVECTOR: {
			vec3_t out;
			G_SpawnVector(vmargs.v[0].s, vmargs.v[1].s, out);
			StringCopy(vmresult.v.s, va("%f %f %f", out[0], out[1], out[2]), MAX_JS_STRINGSIZE);
			vmresult.t = JS_TYPE_STRING;
			break;
		}
#endif
#ifndef GAME
		case VM_APICGUI: API_CGUI(vmargs.v[0].i, vmargs.v[2].i, vmargs.v[3].i, vmargs.v[4].i, vmargs.v[5].i); break;
		case VM_APIGLCONFIG: API_GLConfig(vmargs.v[0].i, vmargs.v[2].i, vmargs.v[3].i, vmargs.v[4].i, vmargs.v[5].i); break;
		case VM_DRAWSTRING: drawString(vmargs.v[0].f, vmargs.v[1].f, vmargs.v[2].s, vmargs.v[3].i, cgui.colors[vmargs.v[4].i], vmargs.v[5].f, vmargs.v[6].i); break;
		case VM_DRAWSTRINGADJUSTED: drawStringAdjusted(vmargs.v[0].f, vmargs.v[1].f, vmargs.v[2].s, vmargs.v[3].i, cgui.colors[vmargs.v[4].i], vmargs.v[5].f, vmargs.v[6].i); break;
		case VM_DRAWRECTANGLE: drawRoundedRect(vmargs.v[0].f, vmargs.v[1].f, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, cgui.colors[vmargs.v[5].i], vmargs.v[6].i); break;
		case VM_DRAWSHADER: drawColoredShader(vmargs.v[0].f, vmargs.v[1].f, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].s, cgui.colors[vmargs.v[5].i]); break;
#endif
#ifdef CGAME
		case VM_APIWEAPONFX: API_WeaponFX(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[3].i, vmargs.v[4].i, vmargs.v[5].i, vmargs.v[6].i); break;
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
			vmresult.v.i = UI_Window(vmargs.v[0].i, vmargs.v[1].s, vmargs.v[2].s, vmargs.v[3].s, vmargs.v[4].i, vmargs.v[5].f, vmargs.v[6].f, vmargs.v[7].i, vmargs.v[8].i, vmargs.v[9].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATEWINDOWBUTTON:
			vmresult.v.i = UI_WindowButton(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].f, vmargs.v[3].s, vmargs.v[4].i, vmargs.v[5].i, vmargs.v[6].f);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATEPICTURE:
			vmresult.v.i = UI_Picture(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, vmargs.v[5].f, vmargs.v[6].s, vmargs.v[7].i, vmargs.v[8].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATEBUTTON:
			vmresult.v.i = UI_Button(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, vmargs.v[5].f, vmargs.v[6].s, vmargs.v[7].i, vmargs.v[8].i, vmargs.v[9].f);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATECHECKBOX:
			vmresult.v.i = UI_Checkbox(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, vmargs.v[5].f, vmargs.v[6].s, vmargs.v[7].i, vmargs.v[8].i, vmargs.v[9].f, vmargs.v[10].s);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATESLIDER:
			vmresult.v.i = UI_Slider(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, vmargs.v[5].f, vmargs.v[6].s, vmargs.v[7].i, vmargs.v[8].i, vmargs.v[9].f, vmargs.v[10].s, vmargs.v[11].f, vmargs.v[12].f, vmargs.v[13].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATEACTION:
			vmresult.v.i = UI_Action(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, vmargs.v[5].f, vmargs.v[6].s, vmargs.v[7].i, vmargs.v[8].i, vmargs.v[9].f, vmargs.v[10].s);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATESPIN:
			vmresult.v.i = UI_Spin(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, vmargs.v[5].f, vmargs.v[6].s, vmargs.v[7].i, vmargs.v[8].i, vmargs.v[9].f, vmargs.v[10].s, vmargs.v[11].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATEFIELD:
			vmresult.v.i = UI_Field(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, vmargs.v[5].f, vmargs.v[6].s, vmargs.v[7].i, vmargs.v[8].i, vmargs.v[9].f, vmargs.v[10].s);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATELIST:
			vmresult.v.i = UI_List(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, vmargs.v[5].f, vmargs.v[6].f, vmargs.v[7].i, vmargs.v[8].i, vmargs.v[9].i, vmargs.v[10].i, vmargs.v[11].i);
			vmresult.t = JS_TYPE_INT;
			break;
		case VM_CREATELISTFILES:
			vmresult.v.i = UI_ListFiles(vmargs.v[0].i, vmargs.v[1].i, vmargs.v[2].f, vmargs.v[3].f, vmargs.v[4].f, vmargs.v[5].f, vmargs.v[6].f, vmargs.v[7].i, vmargs.v[8].i, vmargs.v[9].i, vmargs.v[10].i, vmargs.v[11].i);
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
