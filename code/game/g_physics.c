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
	vec3_t propOrigin, hitPos, delta, fwd, right, up, angles;

	ent->isGrabbed = true;
	if(!ent->client) {
		if(isPhysgun) ent->grabNewPhys = PHYS_DYNAMIC;
		ent->sb_phys = PHYS_DYNAMIC;
		trap_Phys_UnfreezeObject(ent->s.number);

		// Compute local grab point (where the crosshair hit the prop)
		trap_Phys_GetPosition(ent->s.number, &propOrigin[0], &propOrigin[1], &propOrigin[2]);
		VectorSubtract(propOrigin, player->grabOffset, hitPos);
		VectorSubtract(hitPos, propOrigin, delta);
		trap_Phys_GetAngles(ent->s.number, &angles[0], &angles[1], &angles[2]);
		AngleVectors(angles, fwd, right, up);
		ent->grabOrigin[0] = DotProduct(delta, fwd);
		ent->grabOrigin[1] = -DotProduct(delta, right);
		ent->grabOrigin[2] = DotProduct(delta, up);
		ent->grabAccumPitch = angles[0];
		ent->grabAccumYaw = angles[1];
		ent->grabAccumRoll = angles[2];
	} else {
		VectorClear(ent->grabOrigin);
		ent->grabAccumPitch = 0.0f;
		ent->grabAccumYaw = 0.0f;
		ent->grabAccumRoll = 0.0f;
	}
	// Use raw cmd input angles (see Phys_HoldFrame_Kinetic for why).
	ent->grabLastPlayerYaw = SHORT2ANGLE(player->client->pers.cmd.angles[YAW] + player->client->ps.delta_angles[YAW]);
	ent->grabLastPlayerPitch = SHORT2ANGLE(player->client->pers.cmd.angles[PITCH] + player->client->ps.delta_angles[PITCH]);
}

void Phys_HoldFrame_Kinetic(gentity_t* player, bool isPhysgun) {
	gentity_t* ent = player->grabbedEntity;
	vec3_t target, currentPos, currentVel, desiredCenter, delta, force;
	vec3_t propAngles, grabWorldOffset, newGrabWorldOffset, fwd, right, up;
	vec3_t oldGrabWorld;
	float mass;
	float Kp, Kd;
	int i;

	CrosshairPointPhys(player, player->grabDist, target);

	trap_Phys_GetPosition(ent->s.number, &currentPos[0], &currentPos[1], &currentPos[2]);
	trap_Phys_GetLinearVelocity(ent->s.number, &currentVel[0], &currentVel[1], &currentVel[2]);
	trap_Phys_GetAngles(ent->s.number, &propAngles[0], &propAngles[1], &propAngles[2]);

	// Rotation around grabOrigin (physgun only)
	if(isPhysgun) {
		// PM_UpdateViewAngles early-returns while attack+gesture+physgun, so
		// ps.viewangles stays frozen and its delta would be zero. Read the
		// raw cmd input (cmd.angles + delta_angles) to get the real per-frame
		// mouse delta regardless of the view freeze.
		bool gestureHeld = (player->client->pers.cmd.buttons & BUTTON_GESTURE) != 0;
		float rawYaw = SHORT2ANGLE(player->client->pers.cmd.angles[YAW] + player->client->ps.delta_angles[YAW]);
		float rawPitch = SHORT2ANGLE(player->client->pers.cmd.angles[PITCH] + player->client->ps.delta_angles[PITCH]);
		float yawDelta = rawYaw - ent->grabLastPlayerYaw;
		float pitchDelta = rawPitch - ent->grabLastPlayerPitch;
		while(yawDelta > 180.0f) yawDelta -= 360.0f;
		while(yawDelta < -180.0f) yawDelta += 360.0f;
		while(pitchDelta > 180.0f) pitchDelta -= 360.0f;
		while(pitchDelta < -180.0f) pitchDelta += 360.0f;
		float rotYaw = 0.0f;
		float rotPitch = 0.0f;

		if(gestureHeld) {
			// Gesture mode: mouse up/down -> object pitch, mouse left/right -> object yaw.
			rotPitch = pitchDelta * 64.0f;
			rotYaw = yawDelta * 64.0f;
		} else {
			// Normal mode: mouse yaw -> object yaw (1:1).
			rotYaw = yawDelta;
		}

		if(fabs(rotYaw) > 0.1f || (fabs(rotPitch) > 0.0001f && gestureHeld)) {
			// Current world grab point (before rotation)
			AngleVectors(propAngles, fwd, right, up);
			grabWorldOffset[0] = ent->grabOrigin[0] * fwd[0] - ent->grabOrigin[1] * right[0] + ent->grabOrigin[2] * up[0];
			grabWorldOffset[1] = ent->grabOrigin[0] * fwd[1] - ent->grabOrigin[1] * right[1] + ent->grabOrigin[2] * up[1];
			grabWorldOffset[2] = ent->grabOrigin[0] * fwd[2] - ent->grabOrigin[1] * right[2] + ent->grabOrigin[2] * up[2];
			VectorAdd(currentPos, grabWorldOffset, oldGrabWorld);

			// Accumulate true (unbounded) rotation separately from the physics
			// engine's Euler angles so we never feed the backend pitch > 90°
			// (which triggers gimbal-lock / clamp in Euler<->quat conversion).
			ent->grabAccumPitch += rotPitch;
			ent->grabAccumYaw += rotYaw;

			// Derive a displayable Euler representation that keeps pitch inside
			// [-90, 90] using the standard Euler equivalence:
			//   (p, y, r) == (180 - p, y + 180, r + 180)   for p > 90
			//   (p, y, r) == (-180 - p, y + 180, r + 180)  for p < -90
			// The full world orientation is preserved, so visuals, grab offset
			// and subsequent accumulated rotation all stay continuous.
			float dPitch = ent->grabAccumPitch;
			float dYaw = ent->grabAccumYaw;
			float dRoll = ent->grabAccumRoll;
			while(dPitch > 180.0f) dPitch -= 360.0f;
			while(dPitch < -180.0f) dPitch += 360.0f;
			if(dPitch > 90.0f) {
				dPitch = 180.0f - dPitch;
				dYaw += 180.0f;
				dRoll += 180.0f;
			}
			if(dPitch < -90.0f) {
				dPitch = -180.0f - dPitch;
				dYaw += 180.0f;
				dRoll += 180.0f;
			}
			propAngles[0] = dPitch;
			propAngles[1] = dYaw;
			propAngles[2] = dRoll;

			// New grab offset with rotated angles (center unchanged)
			AngleVectors(propAngles, fwd, right, up);
			newGrabWorldOffset[0] = ent->grabOrigin[0] * fwd[0] - ent->grabOrigin[1] * right[0] + ent->grabOrigin[2] * up[0];
			newGrabWorldOffset[1] = ent->grabOrigin[0] * fwd[1] - ent->grabOrigin[1] * right[1] + ent->grabOrigin[2] * up[1];
			newGrabWorldOffset[2] = ent->grabOrigin[0] * fwd[2] - ent->grabOrigin[1] * right[2] + ent->grabOrigin[2] * up[2];

			// Shift center so the grab point stays at the same world position (rotate around grabOrigin).
			// Physics engine resolves against walls afterwards, so placement stays comfortable.
			VectorSubtract(oldGrabWorld, newGrabWorldOffset, currentPos);

			trap_Phys_SetPosition(ent->s.number, currentPos[0], currentPos[1], currentPos[2]);
			trap_Phys_SetAngles(ent->s.number, propAngles[0], propAngles[1], propAngles[2]);
		}
		trap_Phys_SetAngularVelocity(ent->s.number, 0.0f, 0.0f, 0.0f);
		ent->grabLastPlayerYaw = rawYaw;
		ent->grabLastPlayerPitch = rawPitch;
	}

	// Recompute grab world offset with current rotation
	AngleVectors(propAngles, fwd, right, up);
	grabWorldOffset[0] = ent->grabOrigin[0] * fwd[0] - ent->grabOrigin[1] * right[0] + ent->grabOrigin[2] * up[0];
	grabWorldOffset[1] = ent->grabOrigin[0] * fwd[1] - ent->grabOrigin[1] * right[1] + ent->grabOrigin[2] * up[1];
	grabWorldOffset[2] = ent->grabOrigin[0] * fwd[2] - ent->grabOrigin[1] * right[2] + ent->grabOrigin[2] * up[2];

	// Desired center so grab point ends up at crosshair target
	VectorSubtract(target, grabWorldOffset, desiredCenter);

	mass = 50.0f;
	VectorSubtract(desiredCenter, currentPos, delta);

	Kp = 48.0f;
	Kd = 4.0f;

	for(i = 0; i < 3; i++) {
		float desiredAccel = Kp * delta[i] - Kd * currentVel[i];
		force[i] = mass * desiredAccel;
	}

	force[2] += mass * fabs(-800.0);

	trap_Phys_ApplyCentralForce(ent->s.number, force[0], force[1], force[2]);

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

void G_RunPhysics(gentity_t* ent) {
	vec3_t origin, angles;

	if(!trap_Phys_IsSleeping(ent->s.number)) {
		trap_Phys_GetPosition(ent->s.number, &origin[0], &origin[1], &origin[2]);
		trap_Phys_GetAngles(ent->s.number, &angles[0], &angles[1], &angles[2]);
		if(origin[2] < -65535) {
			trap_Phys_SetPosition(ent->s.number, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2]);
			trap_Phys_GetPosition(ent->s.number, &origin[0], &origin[1], &origin[2]);
			trap_Phys_GetAngles(ent->s.number, &angles[0], &angles[1], &angles[2]);
		}

		ent->s.pos.trType = TR_INTERPOLATE;
		ent->s.apos.trType = TR_INTERPOLATE;
		VectorCopy(origin, ent->s.pos.trBase);
		VectorCopy(angles, ent->s.apos.trBase);

		bool needLink = (DistanceSquared(origin, ent->r.currentOrigin) > 2.0f);

		VectorCopy(origin, ent->r.currentOrigin);
		VectorCopy(angles, ent->r.currentAngles);

		if(needLink) trap_LinkEntity(ent);
	}
}
