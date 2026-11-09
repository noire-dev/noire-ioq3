// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2

#include "../qcommon/vm_javascript.h"

// Item entities are ET_PHYSICS from spawn to pickup. modelindex is the
// renderer/Bullet model index; modelindex2 stores the game item ID.
static bool G_CreateItemPhysics(gentity_t* ent, const vec3_t origin, const vec3_t velocity, float mass) {
	char modelPath[MAX_QPATH];
	char* extension;
	int modelIndex;
	bool shapeReady;
	if(!ent || !ent->item || !ent->item->model || !ent->item->model[0]) return false;

	Q_strncpyz(modelPath, ent->item->model, sizeof(modelPath));
	extension = strrchr(modelPath, '.');
	if(!extension) {
		if(strlen(modelPath) + 5 >= sizeof(modelPath)) return false;
		Q_strcat(modelPath, sizeof(modelPath), ".md3");
		extension = strrchr(modelPath, '.');
	}

	// Prefer the declared model. If an MD3 is missing or malformed, attempt
	// the IQM sibling before leaving the entity as a non-simulated pickup.
	modelIndex = G_ModelIndex(modelPath);
	shapeReady = modelIndex > 0 && trap_Phys_CreateShape(modelIndex, modelPath, true);
	if(!shapeReady && extension && !Q_stricmp(extension, ".md3")) {
		*extension = '\0';
		Q_strcat(modelPath, sizeof(modelPath), ".iqm");
		modelIndex = G_ModelIndex(modelPath);
		shapeReady = modelIndex > 0 && trap_Phys_CreateShape(modelIndex, modelPath, true);
	}

	ent->s.eType = ET_PHYSICS;
	ent->s.modelindex = modelIndex;
	ent->s.modelindex2 = ITEM_INDEX(ent->item);
	ent->s.frame = 0;
	ent->physicsItemActive = false;
	if(!shapeReady) return false;

	ent->physicsItemActive = true;
	ent->physicsMass = mass;
	trap_Phys_CreateModel(ent->s.number, modelIndex, origin[0], origin[1], origin[2], mass);
	trap_Phys_SetAngles(ent->s.number, ent->s.angles[0], ent->s.angles[1], ent->s.angles[2]);
	trap_Phys_SetLinearVelocity(ent->s.number, velocity[0], velocity[1], velocity[2]);
	ent->s.pos.trType = TR_INTERPOLATE;
	ent->s.apos.trType = TR_INTERPOLATE;
	VectorCopy(origin, ent->s.pos.trBase);
	VectorCopy(origin, ent->r.currentOrigin);
	VectorCopy(velocity, ent->s.pos.trDelta);
	return true;
}

static void G_DeleteItemPhysics(gentity_t* ent) {
	if(!ent || !ent->physicsItemActive) return;
	trap_Phys_DeleteObject(ent->s.number);
	ent->physicsItemActive = false;
}

void Set_Weapon(gentity_t* ent, int weapon, int status) { ent->swep_list[weapon] = status; }

void Add_Ammo(gentity_t* ent, int weapon, int count) {
	if(!ent || weapon < 0 || weapon >= MAX_WEAPONS) return;
	// -1 is the sole infinite-ammo sentinel. 9999 is an ordinary capped value.
	if(count == -1 || ent->swep_ammo[weapon] == -1) {
		ent->swep_ammo[weapon] = -1;
		return;
	}
	ent->swep_ammo[weapon] += count;
	if(ent->swep_ammo[weapon] > 9999) ent->swep_ammo[weapon] = 9999;
	if(ent->swep_ammo[weapon] < 0) ent->swep_ammo[weapon] = 0;
}

static int Pickup_Weapon(gentity_t* ent, gentity_t* other) {
	int quantity;

	if(ent->count)
		quantity = ent->count;
	else
		quantity = ent->item->quantity;

	Set_Weapon(other, ent->item->giTag, 1);
	Add_Ammo(other, ent->item->giTag, quantity);

	return 5;
}

void RespawnItem(gentity_t* ent) {
	if(!ent) return;

	if(ent->team) {
		gentity_t* master;
		int count;
		int choice;

		if(!ent->teammaster) G_Error("RespawnItem: bad teammaster");
		master = ent->teammaster;

		for(count = 0, ent = master; ent; ent = ent->teamchain, count++);

		choice = rand() % count;

		for(count = 0, ent = master; ent && count < choice; ent = ent->teamchain, count++);
	}

	if(!ent) return;

	if(!ent->physicsItemActive) {
		vec3_t zeroVelocity = {0, 0, 0};
		G_CreateItemPhysics(ent, ent->s.pos.trBase, zeroVelocity, (ent->spawnflags & 1) ? 0.0f : 1.0f);
	}
	ent->r.contents = CONTENTS_TRIGGER;
	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity(ent);

	ent->nextthink = 0;
}

void Touch_Item(gentity_t* ent, gentity_t* other, trace_t* trace) {
	int respawn;

	if(!other->client) return;
	if(other->health < 1) return;  // dead people can't pickup

	if(!BG_CanItemBeGrabbed(&ent->s, &other->client->ps)) return;

	switch(ent->item->giType) {
		case IT_WEAPON: respawn = Pickup_Weapon(ent, other); break;
		default: return;
	}

	if(!respawn) return;

	// js hook: itemPickup(player, itemID)
	JS_HookRun("itemPickup", other->s.number, ITEM_INDEX(ent->item), 0);

	G_AddPredictableEvent(other, EV_ITEM_PICKUP, ent->s.modelindex2);

	G_UseTargets(ent, other);

	// Remove the Bullet body while the item is hidden/awaiting respawn.
	G_DeleteItemPhysics(ent);

	if(ent->wait == -1) {
		ent->r.svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->unlinkAfterEvent = true;
		return;
	}

	if(ent->wait) respawn = ent->wait;

	if(ent->random) {
		respawn += crandom() * ent->random;
		if(respawn < 1) respawn = 1;
	}

	if(ent->flags & FL_DROPPED_ITEM) ent->freeAfterEvent = true;

	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eFlags |= EF_NODRAW;
	ent->r.contents = 0;
	ent->nextthink = level.time + respawn * 1000;
	ent->think = RespawnItem;
	trap_LinkEntity(ent);
}

gentity_t* LaunchItem(item_t* item, vec3_t origin, vec3_t velocity) {
	gentity_t* dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_PHYSICS;
	dropped->s.modelindex = 0;                  // assigned by G_CreateItemPhysics
	dropped->s.modelindex2 = ITEM_INDEX(item);  // game item ID; ET_PHYSICS renderer uses it

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet(dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet(dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin(dropped, origin);
	if(!G_CreateItemPhysics(dropped, origin, velocity, 1.0f)) {
		// Keep one network/visual/pickup representation (ET_PHYSICS). The item
		// remains usable but will not move until a model collision can be built.
		G_Printf("LaunchItem: Bullet shape unavailable for %s\n", item->model);
		VectorCopy(origin, dropped->s.pos.trBase);
		VectorClear(dropped->s.pos.trDelta);
	}

	dropped->s.eFlags |= EF_BOUNCE_HALF;
	dropped->think = G_FreeEntity;
	dropped->nextthink = level.time + 30000;

	dropped->flags = FL_DROPPED_ITEM;

	trap_LinkEntity(dropped);

	return dropped;
}

gentity_t* Drop_Item(gentity_t* ent, item_t* item) {
	vec3_t velocity;
	vec3_t angles;

	VectorCopy(ent->s.apos.trBase, angles);
	angles[YAW] += random() * 360.0f;
	angles[PITCH] = 0;  // always forward

	AngleVectors(angles, velocity, NULL, NULL);
	VectorScale(velocity, 150, velocity);
	velocity[2] += 200 + crandom() * 50;

	return LaunchItem(item, ent->s.pos.trBase, velocity);
}

void Use_Item(gentity_t* ent, gentity_t* other, gentity_t* activator) { RespawnItem(ent); }

void FinishSpawningItem(gentity_t* ent) {
	trace_t tr;
	vec3_t dest;

	VectorSet(ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet(ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);

	ent->s.eType = ET_PHYSICS;
	ent->s.modelindex = 0;  // assigned to the registered MD3/IQM model by G_CreateItemPhysics
	ent->s.modelindex2 = ITEM_INDEX(ent->item);

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;
	ent->use = Use_Item;

	if(ent->spawnflags & 1) {
		G_SetOrigin(ent, ent->s.origin);  // suspended
	} else {
		// drop to floor
		VectorSet(dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096);
		trap_Trace(&tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID);
		if(tr.startsolid) {
			G_Printf("FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
			G_FreeEntity(ent);
			return;
		}

		ent->s.groundEntityNum = tr.entityNum;  // allow to ride movers

		G_SetOrigin(ent, tr.endpos);
	}

	// Team members and targeted items stay hidden until activated.
	if((ent->flags & FL_TEAMMEMBER) || ent->targetname) {
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	{
		vec3_t zeroVelocity = {0, 0, 0};
		float mass = (ent->spawnflags & 1) ? 0.0f : 1.0f;
		if(!G_CreateItemPhysics(ent, ent->s.pos.trBase, zeroVelocity, mass)) {
			G_Printf("FinishSpawningItem: no Bullet shape for %s (%s); keeping ET_PHYSICS pickup\n", ent->classname, ent->item->model);
		}
	}
	trap_LinkEntity(ent);
}

void G_SpawnItem(gentity_t* ent, item_t* item) {
	G_SpawnFloat("random", "0", &ent->random);
	G_SpawnFloat("wait", "0", &ent->wait);

	ent->item = item;
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FinishSpawningItem;

	ent->physicsBounce = 0.50;  // items are bouncy
}
