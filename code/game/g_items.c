// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2

#include "g_local.h"

void Set_Weapon(gentity_t* ent, int weapon, int status) { ent->swep_list[weapon] = status; }

void Add_Ammo(gentity_t* ent, int weapon, int count) {
	if(count == 9999) {
		ent->swep_ammo[weapon] = 9999;
	} else {
		ent->swep_ammo[weapon] += count;
		if(ent->swep_ammo[weapon] > 9000) {
			ent->swep_ammo[weapon] = 9000;
		}
	}
}

void Set_Ammo(gentity_t* ent, int weapon, int count) { ent->swep_ammo[weapon] = count; }

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
	if(!ent) {
		return;
	}

	// randomly select from teamed entities
	if(ent->team) {
		gentity_t* master;
		int count;
		int choice;

		if(!ent->teammaster) {
			G_Error("RespawnItem: bad teammaster");
		}
		master = ent->teammaster;

		for(count = 0, ent = master; ent; ent = ent->teamchain, count++);

		choice = rand() % count;

		for(count = 0, ent = master; ent && count < choice; ent = ent->teamchain, count++);
	}

	if(!ent) {
		return;
	}

	ent->r.contents = CONTENTS_TRIGGER;
	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity(ent);

	// play the normal respawn sound only to nearby clients
	G_AddEvent(ent, EV_ITEM_RESPAWN, 0);

	ent->nextthink = 0;
}

void Touch_Item(gentity_t* ent, gentity_t* other, trace_t* trace) {
	int respawn;

	if(!other->client) return;
	if(other->health < 1) return;  // dead people can't pickup

	// the same pickup rules are used for client side and server side
	if(!BG_CanItemBeGrabbed(&ent->s, &other->client->ps)) return;

	// call the item-specific pickup function
	switch(ent->item->giType) {
		case IT_WEAPON: respawn = Pickup_Weapon(ent, other); break;
		default: return;
	}

	if(!respawn) return;

	// play the normal pickup sound
	G_AddPredictableEvent(other, EV_ITEM_PICKUP, ent->s.modelindex);

	// fire item targets
	G_UseTargets(ent, other);

	// wait of -1 will not respawn
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
		if(respawn < 1) {
			respawn = 1;
		}
	}

	if(ent->flags & FL_DROPPED_ITEM) ent->freeAfterEvent = true;

	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eFlags |= EF_NODRAW;
	ent->r.contents = 0;
	ent->nextthink = level.time + respawn * 1000;
	ent->think = RespawnItem;
	trap_LinkEntity(ent);
}

gentity_t* LaunchItem(gitem_t* item, vec3_t origin, vec3_t velocity) {
	gentity_t* dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist;  // store item number in modelindex
	dropped->s.modelindex2 = 1;                  // This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet(dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet(dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin(dropped, origin);
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy(velocity, dropped->s.pos.trDelta);

	dropped->s.eFlags |= EF_BOUNCE_HALF;
	dropped->think = G_FreeEntity;
	dropped->nextthink = level.time + 30000;

	dropped->flags = FL_DROPPED_ITEM;

	trap_LinkEntity(dropped);

	return dropped;
}

gentity_t* Drop_Item(gentity_t* ent, gitem_t* item) {
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

	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;  // store item number in modelindex
	ent->s.modelindex2 = 0;                       // zero indicates this isn't a dropped item

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;
	// using an item causes it to respawn
	ent->use = Use_Item;

	if(ent->spawnflags & 1) {
		// suspended
		G_SetOrigin(ent, ent->s.origin);
	} else {
		// drop to floor
		VectorSet(dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096);
		trap_Trace(&tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID);
		if(tr.startsolid) {
			G_Printf("FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
			G_FreeEntity(ent);
			return;
		}

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin(ent, tr.endpos);
	}

	// team members and targeted items aren't present at start
	if((ent->flags & FL_TEAMMEMBER) || ent->targetname) {
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	trap_LinkEntity(ent);
}

void G_SpawnItem(gentity_t* ent, gitem_t* item) {
	G_SpawnFloat("random", "0", &ent->random);
	G_SpawnFloat("wait", "0", &ent->wait);

	ent->item = item;
	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FinishSpawningItem;

	ent->physicsBounce = 0.50;  // items are bouncy
}

void G_BounceItem(gentity_t* ent, trace_t* trace) {
	vec3_t velocity;
	float dot;
	int hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + (level.time - level.previousTime) * trace->fraction;
	BG_EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity);
	dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta);

	// cut the velocity to keep from bouncing forever
	VectorScale(ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta);

	// check for stop
	if(trace->plane.normal[2] > 0 && ent->s.pos.trDelta[2] < 40) {
		trace->endpos[2] += 1.0;  // make sure it is off ground
		SnapVector(trace->endpos);
		G_SetOrigin(ent, trace->endpos);
		ent->s.groundEntityNum = trace->entityNum;
		return;
	}

	VectorAdd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;
}

void G_RunItem(gentity_t* ent) {
	vec3_t origin;
	trace_t tr;
	int contents;
	int mask;

	// if its groundentity has been set to none, it may have been pushed off an edge
	if(ent->s.groundEntityNum == ENTITYNUM_NONE) {
		if(ent->s.pos.trType != TR_GRAVITY) {
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	if(ent->s.pos.trType == TR_STATIONARY) {
		// check think function
		G_RunThink(ent);
		return;
	}

	// get current position
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);

	// trace a line from the previous position to the current position
	if(ent->clipmask) {
		mask = ent->clipmask;
	} else {
		mask = MASK_PLAYERSOLID & ~CONTENTS_BODY;  // MASK_SOLID;
	}
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, ent->r.ownerNum, mask);

	VectorCopy(tr.endpos, ent->r.currentOrigin);

	if(tr.startsolid) {
		tr.fraction = 0;
	}

	trap_LinkEntity(ent);  // FIXME: avoid this for stationary?

	// check think function
	G_RunThink(ent);

	if(tr.fraction == 1) {
		return;
	}

	// if it is in a nodrop volume, remove it
	contents = trap_PointContents(ent->r.currentOrigin, -1);
	if(contents & CONTENTS_NODROP) {
		G_FreeEntity(ent);
		return;
	}

	G_BounceItem(ent, &tr);
}
