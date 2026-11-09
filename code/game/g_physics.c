// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2

#include "g_local.h"

typedef enum { PHYS_MAT_BODY, PHYS_MAT_CARDBOARD, PHYS_MAT_CONCRETE, PHYS_MAT_FLESH, PHYS_MAT_GLASS, PHYS_MAT_METAL, PHYS_MAT_PLASTER, PHYS_MAT_PLASTIC, PHYS_MAT_RUBBER, PHYS_MAT_WOOD, PHYS_MAT_COUNT } physicsMaterialId_t;

typedef struct {
	const char* name;
	float buoyancy;
	float waterLinearDrag;
	float waterAngularDrag;
} physicsMaterialDef_t;

// Materials are intentionally game-only. Bullet sees geometry, masses and
// impulses; gameplay chooses buoyancy and sound packages here.
static const physicsMaterialDef_t physicsMaterials[PHYS_MAT_COUNT] = {
    {"body", 1.00f, 5.0f, 5.0f},
    {"cardboard", 1.20f, 5.5f, 6.0f},
    {"concrete", 0.02f, 3.0f, 3.5f},
    {"flesh", 1.00f, 5.0f, 5.5f},
    {"glass", 0.05f, 3.2f, 3.5f},
    {"metal", 0.04f, 3.0f, 3.2f},
    {"plaster", 0.10f, 3.5f, 4.0f},
    {"plastic", 0.82f, 4.5f, 4.8f},
    {"rubber", 0.42f, 5.0f, 5.0f},
    {"wood", 1.28f, 4.8f, 5.0f},
};

static const physicsMaterialDef_t* G_PhysicsMaterial(const gentity_t* ent) {
	int id = ent ? ent->physicsMaterial : PHYS_MAT_BODY;
	if(id < 0 || id >= PHYS_MAT_COUNT) id = PHYS_MAT_BODY;
	return &physicsMaterials[id];
}

void G_SetPhysicsMaterial(gentity_t* ent, const char* materialName) {
	int i;
	if(!ent) return;
	ent->physicsMaterial = PHYS_MAT_BODY;
	if(!materialName || !materialName[0]) return;
	for(i = 0; i < PHYS_MAT_COUNT; ++i) {
		if(!Q_stricmp(materialName, physicsMaterials[i].name)) {
			ent->physicsMaterial = i;
			return;
		}
	}
}

static gentity_t* G_PhysicsEventEntity(int entityNum) {
	if(entityNum < 0 || entityNum >= level.num_entities) return NULL;
	if(!g_entities[entityNum].inuse) return NULL;
	if(g_entities[entityNum].s.eType != ET_PHYSICS) return NULL;
	return &g_entities[entityNum];
}

#define PHYS_MAX_SOUND_VARIANTS 16

typedef struct {
	int material;
	int count[2];
} physicsSoundCache_t;

static physicsSoundCache_t s_physSoundCache[32];
static int s_physSoundCacheCount = 0;

static int G_PhysicsScanSoundVariants(const char* matName, const char* impactType) {
	char path[MAX_QPATH];
	fileHandle_t f;
	int last = 0;
	int i;

	for(i = 1; i <= PHYS_MAX_SOUND_VARIANTS; ++i) {
		Com_sprintf(path, sizeof(path), "sound/physics/%s/impact_%s%i.wav", matName, impactType, i);
		if(trap_FS_FOpenFile(path, &f, FS_READ) >= 0) {
			trap_FS_FCloseFile(f);
			last = i;
		}
	}
	return last;
}

static int G_PhysicsSoundVariants(int material, const char* matName, const char* impactType) {
	int typeIdx = (impactType[0] == 'h') ? 1 : 0;
	int i;

	for(i = 0; i < s_physSoundCacheCount; ++i) {
		if(s_physSoundCache[i].material == material) return s_physSoundCache[i].count[typeIdx];
	}

	if(s_physSoundCacheCount >= (int)(sizeof(s_physSoundCache) / sizeof(s_physSoundCache[0]))) return G_PhysicsScanSoundVariants(matName, impactType);  // кэш переполнен — не кэшируем

	{
		physicsSoundCache_t* e = &s_physSoundCache[s_physSoundCacheCount++];
		e->material = material;
		e->count[0] = G_PhysicsScanSoundVariants(matName, "soft");
		e->count[1] = G_PhysicsScanSoundVariants(matName, "hard");
		return e->count[typeIdx];
	}
}

void G_PhysicsCollision(int entityA, int entityB, float speed, const vec3_t point, const vec3_t normal, float impulse) {
	gentity_t* a = G_PhysicsEventEntity(entityA);
	gentity_t* b = G_PhysicsEventEntity(entityB);
	gentity_t* player = NULL;
	gentity_t* prop = NULL;
	vec3_t effectOrigin;
	float distance;
	int i;

	if(speed < 28.0f) return;
	VectorCopy(point, effectOrigin);

	if(a && a->s.eType == ET_PHYSICS) prop = a;
	if(b && b->s.eType == ET_PHYSICS && !prop) prop = b;
	if(entityA >= 0 && entityA < level.maxclients && g_entities[entityA].client) player = &g_entities[entityA];
	if(entityB >= 0 && entityB < level.maxclients && g_entities[entityB].client) player = &g_entities[entityB];

	if(prop && prop->physicsImpactTime <= level.time) {
		const char* matName = G_PhysicsMaterial(prop)->name;
		const char* impactType = (speed >= 165.0f) ? "hard" : "soft";
		int matIdx = prop->physicsMaterial;
		int variants = G_PhysicsSoundVariants(matIdx, matName, impactType);
		char soundPath[MAX_QPATH];

		if(variants > 0) {
			int pick = 1 + (rand() % variants);
			Com_sprintf(soundPath, sizeof(soundPath), "sound/physics/%s/impact_%s%i.wav", matName, impactType, pick);
		} else {
			Com_sprintf(soundPath, sizeof(soundPath), "sound/physics/%s/impact_%s.wav", matName, impactType);
		}

		G_TempEntity(effectOrigin, EV_GENERAL_SOUND, G_SoundIndex(soundPath));
		if(speed >= 45.0f) G_TempEntity(effectOrigin, EV_PARTICLE, PP_PHYSICS_DUST);
		prop->physicsImpactTime = level.time + 120;
	}

	// A hard prop impact against the static world shakes nearby players.
	if(prop && (entityA < 0 || entityB < 0) && fabsf(normal[2]) >= 0.55f && speed >= 300.0f) {
		int strength = (int)(speed * 0.42f);
		if(strength < 1) strength = 1;
		if(strength > 255) strength = 255;
		for(i = 0; i < level.maxclients; ++i) {
			gentity_t* clientEnt = &g_entities[i];
			if(!clientEnt->inuse || !clientEnt->client || clientEnt->client->noclip || clientEnt->health <= 0) continue;
			distance = Distance(clientEnt->r.currentOrigin, effectOrigin);
			if(distance <= 900.0f) G_AddEvent(clientEnt, EV_PHYSICS_SHAKE, strength);
		}
	}

	// Dynamic props can transfer their rebound energy into a running player.
	if(player && prop && player->client && !player->client->noclip && player->health > 0 && speed >= 115.0f) {
		vec3_t direction;
		int damage = (int)((speed - 90.0f) * 0.035f + impulse * 0.002f);
		if(damage < 1) damage = 1;
		if(damage > 40) damage = 40;
		VectorSubtract(player->r.currentOrigin, prop->r.currentOrigin, direction);
		if(VectorNormalize(direction) == 0.0f) VectorCopy(normal, direction);
		G_Damage(player, prop, prop, direction, effectOrigin, damage, 0, MOD_CRUSH);
	}
}

void Phys_HoldDropStatic(gentity_t* player) {
	gentity_t* ent = player->grabbedEntity;

	if(!ent->client) {
		ent->physicsActive = false;
		trap_Phys_FreezeObject(ent->s.number);
		VectorClear(ent->s.angles2);
	} else {
		VectorClear(ent->client->ps.velocity);
	}
}

void Phys_HoldDropDynamic(gentity_t* player, bool isPhysgun) {
	gentity_t* ent = player->grabbedEntity;
	vec3_t velocity;

	if(!ent->client) {
		ent->physicsActive = true;
		VectorClear(ent->s.angles2);
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
		if(isPhysgun) ent->grabPhysicsActive = true;
		ent->physicsActive = true;
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
		if(ent->ragdollBone) {
			ent->r.svFlags &= ~SVF_NOCLIENT;
			trap_LinkEntity(ent);
		}
	} else {
		VectorClear(ent->grabOrigin);
		ent->grabAccumPitch = 0.0f;
		ent->grabAccumYaw = 0.0f;
		ent->grabAccumRoll = 0.0f;
	}

	// Send the local grab offset to the client via angles2 so the physgun
	// beam visually terminates at the grab point and rotates with the prop.
	ent->s.angles2[0] = ent->grabOrigin[0];
	ent->s.angles2[1] = ent->grabOrigin[1];
	ent->s.angles2[2] = ent->grabOrigin[2];

	// Use raw cmd input angles (see Phys_HoldFrame_Kinetic for why).
	ent->grabLastPlayerYaw = SHORT2ANGLE(player->client->pers.cmd.angles[YAW] + player->client->ps.delta_angles[YAW]);
	ent->grabLastPlayerPitch = SHORT2ANGLE(player->client->pers.cmd.angles[PITCH] + player->client->ps.delta_angles[PITCH]);
}

void Phys_HoldFrame_Kinetic(gentity_t* player, bool isPhysgun) {
	gentity_t* ent = player->grabbedEntity;
	vec3_t target, currentPos, currentVel, desiredCenter, delta, force;
	vec3_t propAngles, grabWorldOffset, newGrabWorldOffset, fwd, right, up;
	vec3_t oldGrabWorld;
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
		bool gestureHeld = (player->client->pers.cmd.buttons & BUTTON_USE) != 0;
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

	VectorSubtract(desiredCenter, currentPos, delta);

	Kp = 128.0f;
	Kd = 20.0f;

	for(i = 0; i < 3; i++) {
		float desiredAccel = Kp * delta[i] - Kd * currentVel[i];
		force[i] = ent->physicsMass * desiredAccel;
	}

	force[2] += ent->physicsMass * fabs(-800.0);

	trap_Phys_ApplyCentralForce(ent->s.number, force[0], force[1], force[2]);
}

void G_RunPhysics(gentity_t* ent) {
	vec3_t origin, angles;
	float dt;

	if(!ent) return;
	if(BG_IsItemEntity(&ent->s) && !ent->physicsItemActive) return;
	dt = (level.time - level.previousTime) * 0.001f;
	if(dt <= 0.0f || dt > 0.2f) dt = 0.05f;

	// Buoyancy is evaluated on a transformed grid inside the model's local AABB.
	// Twenty-five vertical slices smooth the old one-point/stepwise lift. The game owns
	// material values; Bullet only supplies geometry and accepts forces/torques.
	if(ent->physicsMass > 0.0f) {
		const physicsMaterialDef_t* material = G_PhysicsMaterial(ent);
		float samplePoints[225 * 4];
		vec3_t localMins, localMaxs;
		int i, wet = 0, validSamples = 0;
		bool wetSamples[225];
		float submergedFraction;
		trap_Phys_GetWaterSamples(ent->s.number, samplePoints, localMins, localMaxs);
		for(i = 0; i < 225; ++i) {
			vec3_t point;
			wetSamples[i] = false;
			if(samplePoints[i * 4 + 3] < 0.5f) continue;
			++validSamples;
			VectorSet(point, samplePoints[i * 4 + 0], samplePoints[i * 4 + 1], samplePoints[i * 4 + 2]);
			wetSamples[i] = (trap_PointContents(point, -1) & CONTENTS_WATER) != 0;
			if(wetSamples[i]) ++wet;
		}
		if(validSamples > 0 && wet > 0 && material->buoyancy > 0.0f) {
			float force = ent->physicsMass * fabsf(g_gravity.value) * material->buoyancy / (float)validSamples;
			for(i = 0; i < 225; ++i) {
				if(!wetSamples[i]) continue;
				trap_Phys_ApplyForceAtPosition(ent->s.number, 0.0f, 0.0f, force, samplePoints[i * 4 + 0], samplePoints[i * 4 + 1], samplePoints[i * 4 + 2]);
			}
		}
		submergedFraction = validSamples > 0 ? (float)wet / (float)validSamples : 0.0f;
		if(submergedFraction > 0.0f) {
			float velocity[3], angular[3], drag;
			float angularDamping;
			float dimensions[3], axes[3][3];
			float longest;

			trap_Phys_GetLinearVelocity(ent->s.number, &velocity[0], &velocity[1], &velocity[2]);
			drag = material->waterLinearDrag * ent->physicsMass * submergedFraction;
			trap_Phys_ApplyCentralForce(ent->s.number, -velocity[0] * drag, -velocity[1] * drag, -velocity[2] * drag);

			trap_Phys_GetAngularVelocity(ent->s.number, &angular[0], &angular[1], &angular[2]);
			angularDamping = expf(-material->waterAngularDrag * submergedFraction * dt);
			VectorScale(angular, angularDamping, angular);
			trap_Phys_SetAngularVelocity(ent->s.number, angular[0], angular[1], angular[2]);

			// Long floating props should settle with their broad face horizontal.
			// Align the thinnest local AABB axis (the face normal) with world up;
			// this also makes tall fence-like props tip over instead of floating upright.
			VectorSet(dimensions, localMaxs[0] - localMins[0], localMaxs[1] - localMins[1], localMaxs[2] - localMins[2]);
			trap_Phys_GetAngles(ent->s.number, &angles[0], &angles[1], &angles[2]);
			AngleVectors(angles, axes[0], axes[1], axes[2]);
			VectorScale(axes[1], -1.0f, axes[1]);  // Q3 right is opposite local +Y
			longest = dimensions[0];
			if(dimensions[1] > longest) longest = dimensions[1];
			if(dimensions[2] > longest) longest = dimensions[2];
			if(material->buoyancy >= 0.35f && longest > 1.0f) {
				int thinAxis = 0;
				float thinnest = dimensions[0];
				if(dimensions[1] < thinnest) {
					thinAxis = 1;
					thinnest = dimensions[1];
				}
				if(dimensions[2] < thinnest) {
					thinAxis = 2;
					thinnest = dimensions[2];
				}
				if(thinnest > 0.01f && longest > thinnest * 1.25f) {
					vec3_t faceNormal, worldUp, torqueAxis, torque;
					float torqueScale, dampingScale;
					VectorCopy(axes[thinAxis], faceNormal);
					if(faceNormal[2] < 0.0f) VectorScale(faceNormal, -1.0f, faceNormal);
					VectorSet(worldUp, 0.0f, 0.0f, 1.0f);
					CrossProduct(faceNormal, worldUp, torqueAxis);
					torqueScale = ent->physicsMass * fabsf(g_gravity.value) * longest * 0.045f * material->buoyancy * submergedFraction;
					dampingScale = ent->physicsMass * longest * longest * 0.020f * submergedFraction;
					VectorScale(torqueAxis, torqueScale, torque);
					VectorMA(torque, -dampingScale, angular, torque);
					trap_Phys_ApplyTorque(ent->s.number, torque[0], torque[1], torque[2]);
				}
			}
		}
	}

	if(!trap_Phys_IsSleeping(ent->s.number)) {
		trap_Phys_GetPosition(ent->s.number, &origin[0], &origin[1], &origin[2]);
		trap_Phys_GetAngles(ent->s.number, &angles[0], &angles[1], &angles[2]);
		if(origin[2] < -65535) {
			trap_Phys_SetPosition(ent->s.number, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2]);
			trap_Phys_GetPosition(ent->s.number, &origin[0], &origin[1], &origin[2]);
			trap_Phys_GetAngles(ent->s.number, &angles[0], &angles[1], &angles[2]);
		}

		if(ent->ragdollBone) G_RagdollBoneUpdate(ent, angles);  // ragdollState[ragdollID].angles[boneID]

		ent->s.pos.trType = TR_INTERPOLATE;
		ent->s.apos.trType = TR_INTERPOLATE;
		VectorCopy(origin, ent->s.pos.trBase);
		VectorCopy(angles, ent->s.apos.trBase);

		bool needLink = (DistanceSquared(origin, ent->r.currentOrigin) > 2.0f) || BG_IsItemEntity(&ent->s);

		VectorCopy(origin, ent->r.currentOrigin);
		VectorCopy(angles, ent->r.currentAngles);

		if(needLink) trap_LinkEntity(ent);
	}
}
