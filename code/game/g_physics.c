// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2

#include "g_local.h"

void Phys_HoldDropStatic(gentity_t* player, vec3_t velocity) {
	gentity_t* ent = player->grabbedEntity;

	if(!ent->client) {
		ent->sb_phys = PHYS_STATIC;
		trap_Phys_FreezeObject(ent->s.number);
		// if(ent->s.eType == ET_ITEM) ent->spawnflags = 1; // for items phys
	} else {
		VectorClear(ent->client->ps.velocity);
	}
}

void Phys_HoldDropDynamic(gentity_t* player, vec3_t velocity, bool isPhysgun) {
	gentity_t* ent = player->grabbedEntity;

	if(!ent->client) {
		ent->sb_phys = PHYS_DYNAMIC;
		// if(ent->s.eType == ET_ITEM) ent->spawnflags = 0; // for items phys
		if(!isPhysgun) {
			VectorSubtract(ent->r.currentOrigin, player->r.currentOrigin, velocity);
			VectorScale(velocity, 10, velocity);
			trap_Phys_SetLinearVelocity(ent->s.number, velocity[0], velocity[1], velocity[2]);
		}
		trap_Phys_UnfreezeObject(ent->s.number);
	} else {
		if(!isPhysgun) {
			VectorSubtract(ent->r.currentOrigin, player->r.currentOrigin, velocity);
			VectorScale(velocity, 10, velocity);
			VectorCopy(velocity, ent->client->ps.velocity);
		}
	}
}

void Phys_HoldSetup(gentity_t* player, bool isPhysgun) {
	gentity_t* ent = player->grabbedEntity;

	ent->isGrabbed = true;
	if(!ent->client) {
		if(isPhysgun) ent->grabNewPhys = PHYS_DYNAMIC;
		ent->sb_phys = PHYS_DYNAMIC;
		trap_Phys_UnfreezeObject(ent->s.number);
	}
}

void Phys_HoldFrame_Kinetic(gentity_t* player, bool isPhysgun) {
	gentity_t* ent = player->grabbedEntity;
	vec3_t target, currentPos, currentVel, delta, force;
	float mass, dt = (level.time - level.previousTime) * 0.001;
	float Kp;
	float Kd;
	int i;

	CrosshairPointPhys(player, player->grabDist, target);
	VectorClear(player->grabOffset);
	VectorAdd(target, player->grabOffset, target);

	trap_Phys_GetPosition(ent->s.number, &currentPos[0], &currentPos[1], &currentPos[2]);
	trap_Phys_GetLinearVelocity(ent->s.number, &currentVel[0], &currentVel[1], &currentVel[2]);

	mass = 50.0f;

	VectorSubtract(target, currentPos, delta);

	Kp = 192.0f;
	Kd = 24.0f;

	for(i = 0; i < 3; i++) {
		float desiredAccel = Kp * delta[i] - Kd * currentVel[i];
		force[i] = mass * desiredAccel;
	}

	force[2] += mass * fabs(-800.0);

	trap_Phys_ApplyCentralForce(ent->s.number, force[0], force[1], force[2]);
	if(isPhysgun) trap_Phys_SetAngularVelocity(ent->s.number, 0.00f, 0.00f, 0.00f);
	if(player->client->pers.cmd.buttons & BUTTON_GESTURE && isPhysgun) trap_Phys_SetAngles(ent->s.number, player->client->pers.cmd.angles[0], player->client->pers.cmd.angles[1], player->client->pers.cmd.angles[0] - player->client->pers.cmd.angles[1]);  // rotate

	VectorCopy(currentPos, ent->grabOldOrigin);
	ent->lastPlayer = player;
}

void Phys_HoldFrame(gentity_t* player, vec3_t velocity, bool isPhysgun) {
	gentity_t* ent = player->grabbedEntity;
	vec3_t end;

	trap_UnlinkEntity(ent);
	if(isPhysgun)
		CrosshairPointPhys(player, player->grabDist, end);
	else
		CrosshairPointGravity(player, 128, end);
	trap_LinkEntity(ent);
	if(isPhysgun) {
		VectorAdd(end, player->grabOffset, end);        // physgun offset
		VectorCopy(ent->s.origin, ent->grabOldOrigin);  // save old frame for speed apply
	}
	ent->lastPlayer = player;  // save attacker
	if(!ent->client) {
		trap_Phys_SetPosition(ent->s.number, end[0], end[1], end[2]);
		trap_Phys_SetLinearVelocity(ent->s.number, velocity[0], velocity[1], velocity[2]);
		trap_Phys_SetAngularVelocity(ent->s.number, 0.00f, 0.00f, 0.00f);
		if(player->client->pers.cmd.buttons & BUTTON_GESTURE && isPhysgun) trap_Phys_SetAngles(ent->s.number, player->client->pers.cmd.angles[0], player->client->pers.cmd.angles[1], player->client->pers.cmd.angles[0] - player->client->pers.cmd.angles[1]);  // rotate
	} else {
		VectorClear(ent->client->ps.velocity);
		VectorCopy(end, ent->client->ps.origin);
		if(isPhysgun) end[2] += 1;  // player not stuck
		VectorCopy(end, ent->r.currentOrigin);
		if(isPhysgun) {
			VectorSubtract(ent->r.currentOrigin, ent->grabOldOrigin, velocity);
			VectorScale(velocity, 15, velocity);
			VectorCopy(velocity, ent->client->ps.velocity);
		}
	}
}

void G_RunPhysics(gentity_t* ent, int i) {
	if(!trap_Phys_IsSleeping(ent->s.number)) {
		vec3_t origin, angles;
		trap_Phys_GetPosition(ent->s.number, &origin[0], &origin[1], &origin[2]);
		trap_Phys_GetAngles(ent->s.number, &angles[0], &angles[1], &angles[2]);
		if(origin[2] < -65535) trap_Phys_SetPosition(ent->s.number, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2]);
		// trap_Phys_GetPosition(ent->s.number, &ent->s.origin[0], &ent->s.origin[1], &ent->s.origin[2]);
		// trap_Phys_GetAngles(ent->s.number, &ent->s.angles[0], &ent->s.angles[1], &ent->s.angles[2]);
		if(i >= level.physEntNextFrame && level.physEntCount < PHYSICS_UPDATE_LIMIT) {  // Курсор для начала чтения массива + лимит по сети
			ent->s.pos.trType = TR_INTERPOLATE;
			ent->s.apos.trType = TR_INTERPOLATE;
			VectorCopy(origin, ent->s.pos.trBase);
			VectorCopy(angles, ent->s.apos.trBase);
			level.physEntCount += 1;
		} else if(i >= level.physEntNextFrame) {  // Достигли сетевого лимита, ждем следующий снапшот
			if(!level.physEntNeedsSnapshot) level.physEntNextFrame = i;
			level.physEntNeedsSnapshot = true;
		}
		VectorCopy(origin, ent->r.currentOrigin);
		VectorCopy(angles, ent->r.currentAngles);
		trap_LinkEntity(ent);
	} else {
		if(i >= level.physEntNextFrame && level.physEntCount < PHYSICS_UPDATE_LIMIT) {  // Курсор для начала чтения массива + лимит по сети
			if(!VectorCompare(ent->r.currentOrigin, ent->s.pos.trBase)) {
				VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
				VectorCopy(ent->r.currentAngles, ent->s.apos.trBase);
				level.physEntCount += 1;
			}
		} else if(i >= level.physEntNextFrame) {  // Достигли сетевого лимита, ждем следующий снапшот
			if(!level.physEntNeedsSnapshot) level.physEntNextFrame = i;
			level.physEntNeedsSnapshot = true;
		}
	}
}
