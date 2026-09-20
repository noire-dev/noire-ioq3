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

void Add_Ammo(gentity_t* ent, int weapon, int count) {
	ent->client->ps.ammo[weapon] += count;
	if(ent->client->ps.ammo[weapon] > 200) {
		ent->client->ps.ammo[weapon] = 200;
	}
}

int Pickup_Weapon(gentity_t* ent, gentity_t* other) {
	int quantity;

	if(ent->count < 0) {
		quantity = 0;  // None for you, sir!
	} else {
		if(ent->count) {
			quantity = ent->count;
		} else {
			quantity = ent->item->quantity;
		}

		// dropped items and teamplay weapons always have full ammo
		if(!(ent->flags & FL_DROPPED_ITEM)) {
			// respawning rules
			// drop the quantity if the already have over the minimum
			if(other->client->ps.ammo[ent->item->giTag] < quantity) {
				quantity = quantity - other->client->ps.ammo[ent->item->giTag];
			} else {
				quantity = 1;  // only add a single shot
			}
		}
	}

	// add the weapon
	other->client->ps.stats[STAT_WEAPONS] |= (1 << ent->item->giTag);

	Add_Ammo(other, ent->item->giTag, quantity);

	if(ent->item->giTag == WP_GRAPPLING_HOOK) other->client->ps.ammo[ent->item->giTag] = -1;  // unlimited ammo

	return g_weaponRespawn.integer;
}

/*
===============
RespawnItem
===============
*/
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

/*
===============
Touch_Item
===============
*/
void Touch_Item(gentity_t* ent, gentity_t* other, trace_t* trace) {
	int respawn;
	bool predict;

	if(!other->client) return;
	if(other->health < 1) return;  // dead people can't pickup

	// the same pickup rules are used for client side and server side
	if(!BG_CanItemBeGrabbed(&ent->s, &other->client->ps)) {
		return;
	}

	predict = other->client->pers.predictItemPickup;

	// call the item-specific pickup function
	switch(ent->item->giType) {
		case IT_WEAPON:
			respawn = Pickup_Weapon(ent, other);
			//		predict = false;
			break;
		default: return;
	}

	if(!respawn) {
		return;
	}

	// play the normal pickup sound
	if(predict) {
		G_AddPredictableEvent(other, EV_ITEM_PICKUP, ent->s.modelindex);
	} else {
		G_AddEvent(other, EV_ITEM_PICKUP, ent->s.modelindex);
	}

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

	// non zero wait overrides respawn time
	if(ent->wait) {
		respawn = ent->wait;
	}

	// random can be used to vary the respawn time
	if(ent->random) {
		respawn += crandom() * ent->random;
		if(respawn < 1) {
			respawn = 1;
		}
	}

	// dropped items will not respawn
	if(ent->flags & FL_DROPPED_ITEM) {
		ent->freeAfterEvent = true;
	}

	// picked up items still stay around, they just don't
	// draw anything.  This allows respawnable items
	// to be placed on movers.
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eFlags |= EF_NODRAW;
	ent->r.contents = 0;

	// ZOID
	// A negative respawn times means to never respawn this item (but don't
	// delete it).  This is used by items that are respawned by third party
	// events such as ctf flags
	if(respawn <= 0) {
		ent->nextthink = 0;
		ent->think = 0;
	} else {
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
	}
	trap_LinkEntity(ent);
}

//======================================================================

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
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

/*
================
Drop_Item

Spawns an item and tosses it forward
================
*/
gentity_t* Drop_Item(gentity_t* ent, gitem_t* item, float angle) {
	vec3_t velocity;
	vec3_t angles;

	VectorCopy(ent->s.apos.trBase, angles);
	angles[YAW] += angle;
	angles[PITCH] = 0;  // always forward

	AngleVectors(angles, velocity, NULL, NULL);
	VectorScale(velocity, 150, velocity);
	velocity[2] += 200 + crandom() * 50;

	return LaunchItem(item, ent->s.pos.trBase, velocity);
}

/*
================
Use_Item

Respawn the item
================
*/
void Use_Item(gentity_t* ent, gentity_t* other, gentity_t* activator) { RespawnItem(ent); }

//======================================================================

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
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

bool itemRegistered[MAX_ITEMS];

/*
==============
ClearRegisteredItems
==============
*/
void ClearRegisteredItems(void) {
	memset(itemRegistered, 0, sizeof(itemRegistered));

	// players always start with the base weapon
	RegisterItem(BG_FindItemForWeapon(WP_MACHINEGUN));
	RegisterItem(BG_FindItemForWeapon(WP_GAUNTLET));
}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void RegisterItem(gitem_t* item) {
	if(!item) {
		G_Error("RegisterItem: NULL");
	}
	itemRegistered[item - bg_itemlist] = true;
}

/*
===============
SaveRegisteredItems

Write the needed items to a config string
so the client will know which ones to precache
===============
*/
void SaveRegisteredItems(void) {
	char string[MAX_ITEMS + 1];
	int i;
	int count;

	count = 0;
	for(i = 0; i < bg_numItems; i++) {
		if(itemRegistered[i]) {
			count++;
			string[i] = '1';
		} else {
			string[i] = '0';
		}
	}
	string[bg_numItems] = 0;

	G_Printf("%i items registered\n", count);
	trap_SetConfigstring(CS_ITEMS, string);
}

/*
============
G_ItemDisabled
============
*/
int G_ItemDisabled(gitem_t* item) {
	char name[128];

	Com_sprintf(name, sizeof(name), "disable_%s", item->classname);
	return trap_Cvar_VariableIntegerValue(name);
}

/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnItem(gentity_t* ent, gitem_t* item) {
	G_SpawnFloat("random", "0", &ent->random);
	G_SpawnFloat("wait", "0", &ent->wait);

	RegisterItem(item);
	if(G_ItemDisabled(item)) return;

	ent->item = item;
	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FinishSpawningItem;

	ent->physicsBounce = 0.50;  // items are bouncy
}

/*
================
G_BounceItem

================
*/
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

/*
================
G_RunItem

================
*/
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
