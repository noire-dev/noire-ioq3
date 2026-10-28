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
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"

weaponFx_t jsd_weaponFx[WEAPONS_NUM] =
    {
        [WP_GAUNTLET] =
            {
                .continuousFire = true,
                .hitLightColor = {0.6f, 0.6f, 1.0f},
                .hitLightRadius = 80.0f,
                .hitLightTime = 80,
                .hitParticleColor = {0.7f, 0.7f, 1.0f, 1.0f},
                .hitParticleCount = 8,
                .hitParticleSpeed = 120.0f,
                .hitParticleFlags = PF_SMOOTH_FADE,
                .hitParticleGravity = {0.0f, 0.0f, -200.0f},
                .fireLightColor = {0.6f, 0.6f, 1.0f},
                .fireLightRadius = 80.0f,
                .fireLightTime = 50,
                .fireParticleColor = {0.7f, 0.7f, 1.0f, 1.0f},
                .fireParticleCount = 4,
                .fireParticleSpeed = 60.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
            },
        [WP_MACHINEGUN] =
            {
                .hitLightColor = {1.0f, 1.0f, 0.0f},
                .hitLightRadius = 100.0f,
                .hitLightTime = 50,
                .hitParticleColor = {1.0f, 0.9f, 0.5f, 1.0f},
                .hitParticleCount = 6,
                .hitParticleSpeed = 150.0f,
                .hitParticleFlags = PF_SMOOTH_FADE | PF_GRAVITY,
                .hitParticleGravity = {0.0f, 0.0f, -600.0f},
                .fireLightColor = {1.0f, 1.0f, 0.0f},
                .fireLightRadius = 150.0f,
                .fireLightTime = 50,
                .fireParticleColor = {1.0f, 0.9f, 0.5f, 1.0f},
                .fireParticleCount = 3,
                .fireParticleSpeed = 80.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .tracerChance = 0.4f,
                .tracerLength = 100.0f,
                .tracerWidth = 1.0f,
                .tracerShader = "",
                .tracerParticleColor = {1.0f, 1.0f, 0.7f, 1.0f},
                .tracerParticleCount = 10,
                .tracerParticleSpeed = 0.0f,
            },
        [WP_SHOTGUN] =
            {
                .hitLightColor = {1.0f, 1.0f, 0.0f},
                .hitLightRadius = 100.0f,
                .hitLightTime = 50,
                .hitParticleColor = {1.0f, 0.9f, 0.5f, 1.0f},
                .hitParticleCount = 6,
                .hitParticleSpeed = 150.0f,
                .hitParticleFlags = PF_SMOOTH_FADE | PF_GRAVITY,
                .hitParticleGravity = {0.0f, 0.0f, -600.0f},
                .fireLightColor = {1.0f, 1.0f, 0.0f},
                .fireLightRadius = 150.0f,
                .fireLightTime = 50,
                .fireParticleColor = {1.0f, 0.9f, 0.5f, 1.0f},
                .fireParticleCount = 6,
                .fireParticleSpeed = 100.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .tracerChance = 0.4f,
                .tracerLength = 100.0f,
                .tracerWidth = 1.0f,
                .tracerShader = "",
                .tracerParticleColor = {1.0f, 1.0f, 0.7f, 1.0f},
                .tracerParticleCount = 10,
                .tracerParticleSpeed = 0.0f,
            },
        [WP_GRENADE_LAUNCHER] =
            {
                .hitLightColor = {1.0f, 0.7f, 0.0f},
                .hitLightRadius = 300.0f,
                .hitLightTime = 400,
                .hitParticleColor = {1.0f, 0.7f, 0.3f, 1.0f},
                .hitParticleCount = 48,
                .hitParticleSpeed = 300.0f,
                .hitParticleFlags = PF_SMOOTH_FADE | PF_GRAVITY,
                .hitParticleGravity = {0.0f, 0.0f, -800.0f},
                .fireLightColor = {1.0f, 0.7f, 0.0f},
                .fireLightRadius = 150.0f,
                .fireLightTime = 100,
                .fireParticleColor = {1.0f, 0.7f, 0.3f, 1.0f},
                .fireParticleCount = 8,
                .fireParticleSpeed = 100.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .trailColor = {0.6f, 0.6f, 0.6f, 0.5f},
                .trailCount = 2,
                .trailSpeed = 30.0f,
                .trailFlags = PF_SMOOTH_FADE,
                .trailGravity = {0.0f, 0.0f, 30.0f},
                .trailInterval = 50,
                .missileLightColor = {1.0f, 0.7f, 0.0f},
                .missileLightRadius = 150.0f,
            },
        [WP_ROCKET_LAUNCHER] =
            {
                .hitLightColor = {1.0f, 0.75f, 0.0f},
                .hitLightRadius = 400.0f,
                .hitLightTime = 400,
                .hitParticleColor = {1.0f, 0.7f, 0.3f, 1.0f},
                .hitParticleCount = 64,
                .hitParticleSpeed = 400.0f,
                .hitParticleFlags = PF_SMOOTH_FADE | PF_GRAVITY,
                .hitParticleGravity = {0.0f, 0.0f, -800.0f},
                .fireLightColor = {1.0f, 0.75f, 0.0f},
                .fireLightRadius = 200.0f,
                .fireLightTime = 100,
                .fireParticleColor = {1.0f, 0.7f, 0.3f, 1.0f},
                .fireParticleCount = 10,
                .fireParticleSpeed = 120.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .trailColor = {0.7f, 0.7f, 0.7f, 0.5f},
                .trailCount = 2,
                .trailSpeed = 30.0f,
                .trailFlags = PF_SMOOTH_FADE,
                .trailGravity = {0.0f, 0.0f, 20.0f},
                .trailInterval = 50,
                .missileLightColor = {1.0f, 0.75f, 0.0f},
                .missileLightRadius = 200.0f,
            },
        [WP_LIGHTNING] =
            {
                .continuousFire = true,
                .hitLightColor = {0.6f, 0.6f, 1.0f},
                .hitLightRadius = 150.0f,
                .hitLightTime = 100,
                .hitParticleColor = {0.7f, 0.7f, 1.0f, 1.0f},
                .hitParticleCount = 12,
                .hitParticleSpeed = 200.0f,
                .hitParticleFlags = PF_SMOOTH_FADE,
                .hitParticleGravity = {0.0f, 0.0f, 0.0f},
                .fireLightColor = {0.6f, 0.6f, 1.0f},
                .fireLightRadius = 200.0f,
                .fireLightTime = 50,
                .fireParticleColor = {0.7f, 0.7f, 1.0f, 1.0f},
                .fireParticleCount = 4,
                .fireParticleSpeed = 80.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .beamType = RT_LIGHTNING,
                .beamShader = "lightningBoltNew",
            },
        [WP_RAILGUN] =
            {
                .hitLightColor = {1.0f, 0.5f, 0.0f},
                .hitLightRadius = 200.0f,
                .hitLightTime = 200,
                .hitParticleColor = {1.0f, 0.5f, 0.0f, 1.0f},
                .hitParticleCount = 24,
                .hitParticleSpeed = 250.0f,
                .hitParticleFlags = PF_SMOOTH_FADE,
                .hitParticleGravity = {0.0f, 0.0f, 0.0f},
                .fireLightColor = {1.0f, 0.5f, 0.0f},
                .fireLightRadius = 200.0f,
                .fireLightTime = 100,
                .fireParticleColor = {1.0f, 0.5f, 0.0f, 1.0f},
                .fireParticleCount = 6,
                .fireParticleSpeed = 100.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .beamType = RT_RAIL_CORE,
                .beamShader = "railCore",
            },
        [WP_PLASMAGUN] =
            {
                .hitLightColor = {0.6f, 0.6f, 1.0f},
                .hitLightRadius = 150.0f,
                .hitLightTime = 100,
                .hitParticleColor = {0.7f, 0.7f, 1.0f, 1.0f},
                .hitParticleCount = 16,
                .hitParticleSpeed = 200.0f,
                .hitParticleFlags = PF_SMOOTH_FADE,
                .hitParticleGravity = {0.0f, 0.0f, 0.0f},
                .fireLightColor = {0.6f, 0.6f, 1.0f},
                .fireLightRadius = 150.0f,
                .fireLightTime = 50,
                .fireParticleColor = {0.7f, 0.7f, 1.0f, 1.0f},
                .fireParticleCount = 4,
                .fireParticleSpeed = 80.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .trailColor = {0.6f, 0.6f, 1.0f, 0.5f},
                .trailCount = 2,
                .trailSpeed = 30.0f,
                .trailFlags = PF_SMOOTH_FADE,
                .trailGravity = {0.0f, 0.0f, 0.0f},
                .trailInterval = 30,
                .missileLightColor = {0.6f, 0.6f, 1.0f},
                .missileLightRadius = 100.0f,
            },
        [WP_BFG] =
            {
                .hitLightColor = {1.0f, 0.7f, 1.0f},
                .hitLightRadius = 500.0f,
                .hitLightTime = 500,
                .hitParticleColor = {1.0f, 0.7f, 1.0f, 1.0f},
                .hitParticleCount = 80,
                .hitParticleSpeed = 500.0f,
                .hitParticleFlags = PF_SMOOTH_FADE,
                .hitParticleGravity = {0.0f, 0.0f, 0.0f},
                .fireLightColor = {1.0f, 0.7f, 1.0f},
                .fireLightRadius = 300.0f,
                .fireLightTime = 150,
                .fireParticleColor = {1.0f, 0.7f, 1.0f, 1.0f},
                .fireParticleCount = 12,
                .fireParticleSpeed = 150.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .trailColor = {1.0f, 0.7f, 1.0f, 0.6f},
                .trailCount = 4,
                .trailSpeed = 50.0f,
                .trailFlags = PF_SMOOTH_FADE,
                .trailGravity = {0.0f, 0.0f, 0.0f},
                .trailInterval = 30,
                .missileLightColor = {1.0f, 0.7f, 1.0f},
                .missileLightRadius = 300.0f,
            },
        [WP_PHYSGUN] =
            {
                .continuousFire = true,
                .hitLightColor = {0.6f, 0.6f, 1.0f},
                .hitLightRadius = 100.0f,
                .hitLightTime = 50,
                .hitParticleColor = {0.7f, 0.7f, 1.0f, 1.0f},
                .hitParticleCount = 4,
                .hitParticleSpeed = 100.0f,
                .hitParticleFlags = PF_SMOOTH_FADE,
                .hitParticleGravity = {0.0f, 0.0f, 0.0f},
                .fireLightColor = {0.6f, 0.6f, 1.0f},
                .fireLightRadius = 100.0f,
                .fireLightTime = 50,
                .fireParticleColor = {0.7f, 0.7f, 1.0f, 1.0f},
                .fireParticleCount = 3,
                .fireParticleSpeed = 60.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .beamType = RT_RAIL_CORE,
                .beamShader = "railCore",
            },
        [WP_GRAVITYGUN] =
            {
                .continuousFire = true,
                .hitLightColor = {1.0f, 0.5f, 0.0f},
                .hitLightRadius = 100.0f,
                .hitLightTime = 50,
                .hitParticleColor = {1.0f, 0.5f, 0.0f, 1.0f},
                .hitParticleCount = 4,
                .hitParticleSpeed = 100.0f,
                .hitParticleFlags = PF_SMOOTH_FADE,
                .hitParticleGravity = {0.0f, 0.0f, 0.0f},
                .fireLightColor = {1.0f, 0.5f, 0.0f},
                .fireLightRadius = 100.0f,
                .fireLightTime = 50,
                .fireParticleColor = {1.0f, 0.5f, 0.0f, 1.0f},
                .fireParticleCount = 3,
                .fireParticleSpeed = 60.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .beamType = RT_RAIL_CORE,
                .beamShader = "railCore",
            },
        [WP_TOOLGUN] =
            {
                .hitLightColor = {0.5f, 0.5f, 0.5f},
                .hitLightRadius = 100.0f,
                .hitLightTime = 50,
                .hitParticleColor = {0.5f, 0.5f, 0.5f, 1.0f},
                .hitParticleCount = 4,
                .hitParticleSpeed = 100.0f,
                .hitParticleFlags = PF_SMOOTH_FADE,
                .hitParticleGravity = {0.0f, 0.0f, 0.0f},
                .fireLightColor = {0.5f, 0.5f, 0.5f},
                .fireLightRadius = 100.0f,
                .fireLightTime = 50,
                .fireParticleColor = {0.5f, 0.5f, 0.5f, 1.0f},
                .fireParticleCount = 3,
                .fireParticleSpeed = 60.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .beamType = RT_LIGHTNING,
                .beamShader = "lightningBoltNew",
            },
};

// Spawns a fading light using a lightweight local entity.
static void CG_SpawnWeaponLight(vec3_t origin, float radius, int duration, const float* color) {
	localEntity_t* le;
	if(radius <= 0.0f) return;
	if(duration <= 0) {
		trap_R_AddLightToScene(origin, radius, 1.00f, color[0], color[1], color[2], 0);
		return;
	}
	le = CG_AllocLocalEntity();
	le->leType = LE_EXPLOSION;
	le->startTime = cg.time;
	le->endTime = cg.time + duration;
	le->light = radius;
	VectorCopy(color, le->lightColor);
	VectorCopy(origin, le->refEntity.origin);
}

// Emit a full weapon impact effect from the table.
static void CG_EmitWeaponHitFx(int weapon, vec3_t origin, vec3_t dir) {
	weaponFx_t* fx;
	vec3_t sprOrg;
	if(weapon <= WP_NONE || weapon >= WEAPONS_NUM) return;
	fx = &jsd_weaponFx[weapon];
	if(fx->hitParticleCount > 0) {
		VectorMA(origin, 24, dir, sprOrg);
		CG_AddParticles(sprOrg, fx->hitParticleColor, fx->hitParticleCount, fx->hitParticleSpeed, fx->hitParticleFlags, fx->hitParticleGravity, fx->hitParticleShader ? fx->hitParticleShader : "");
	}
	if(fx->hitLightRadius > 0.0f) {
		CG_SpawnWeaponLight(origin, fx->hitLightRadius, fx->hitLightTime, fx->hitLightColor);
	}
}

// Emit muzzle fire effects from the table.
static void CG_EmitWeaponFireFx(int weapon, vec3_t origin) {
	weaponFx_t* fx;
	if(weapon <= WP_NONE || weapon >= WEAPONS_NUM) return;
	fx = &jsd_weaponFx[weapon];
	if(fx->fireParticleCount > 0) {
		CG_AddParticles(origin, fx->fireParticleColor, fx->fireParticleCount, fx->fireParticleSpeed, fx->fireParticleFlags, fx->fireParticleGravity, fx->fireParticleShader ? fx->fireParticleShader : "");
	}
	if(fx->fireLightRadius > 0.0f) {
		CG_SpawnWeaponLight(origin, fx->fireLightRadius, fx->fireLightTime, fx->fireLightColor);
	}
}

/*
==========================
CG_MachineGunEjectBrass
==========================
*/
static void CG_MachineGunEjectBrass(centity_t* cent) {
	localEntity_t* le;
	refEntity_t* re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	float waterScale = 1.0f;
	vec3_t v[3];

	if(cg_brassTime.integer <= 0) {
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 0;
	velocity[1] = -50 + 40 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + (cg_brassTime.integer / 4) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand() & 15);

	AnglesToAxis(cent->lerpAngles, v);

	offset[0] = 8;
	offset[1] = -4;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd(cent->lerpOrigin, xoffset, re->origin);

	VectorCopy(re->origin, le->pos.trBase);

	if(CG_PointContents(re->origin, -1) & CONTENTS_WATER) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);
	re->hModel = cgs.media.machinegunBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand() & 31;
	le->angles.trBase[1] = rand() & 31;
	le->angles.trBase[2] = rand() & 31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_ShotgunEjectBrass
==========================
*/
static void CG_ShotgunEjectBrass(centity_t* cent) {
	localEntity_t* le;
	refEntity_t* re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	vec3_t v[3];
	int i;

	if(cg_brassTime.integer <= 0) {
		return;
	}

	for(i = 0; i < 2; i++) {
		float waterScale = 1.0f;

		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		velocity[0] = 60 + 60 * crandom();
		if(i == 0) {
			velocity[1] = 40 + 10 * crandom();
		} else {
			velocity[1] = -40 + 10 * crandom();
		}
		velocity[2] = 100 + 50 * crandom();

		le->leType = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime = le->startTime + cg_brassTime.integer * 3 + cg_brassTime.integer * random();

		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time;

		AnglesToAxis(cent->lerpAngles, v);

		offset[0] = 8;
		offset[1] = 0;
		offset[2] = 24;

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
		VectorAdd(cent->lerpOrigin, xoffset, re->origin);
		VectorCopy(re->origin, le->pos.trBase);
		if(CG_PointContents(re->origin, -1) & CONTENTS_WATER) {
			waterScale = 0.10f;
		}

		xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
		xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
		xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
		VectorScale(xvelocity, waterScale, le->pos.trDelta);

		AxisCopy(axisDefault, re->axis);
		re->hModel = cgs.media.shotgunBrassModel;
		le->bounceFactor = 0.3f;

		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand() & 31;
		le->angles.trBase[1] = rand() & 31;
		le->angles.trBase[2] = rand() & 31;
		le->angles.trDelta[0] = 1;
		le->angles.trDelta[1] = 0.5;
		le->angles.trDelta[2] = 0;

		le->leFlags = LEF_TUMBLE;
		le->leBounceSoundType = LEBS_BRASS;
		le->leMarkType = LEMT_NONE;
	}
}

void CG_RailTrail(clientInfo_t* ci, vec3_t start, vec3_t end, int weapon) {
	weaponFx_t* fx;
	refEntity_t direct;
	localEntity_t* le;
	refEntity_t* re;
	vec3_t s, e;

	if(weapon <= WP_NONE || weapon >= WEAPONS_NUM) return;
	fx = &jsd_weaponFx[weapon];
	if(fx->beamType != RT_RAIL_CORE) return;

	VectorCopy(start, s);
	VectorCopy(end, e);

	if(weapon == WP_RAILGUN) {
		s[2] -= 4;
	}

	// Continuous beam weapons (physgun, gravitygun) re-trace every frame while
	// fire is held. If we spawned a fading local entity for each trace, several
	// beams from the last ~60 ms would stack on screen — that's the "2-5 rays
	// per frame" bug. Render them directly to the scene instead: exactly one
	// beam per frame, no accumulation.
	if(fx->continuousFire) {
		memset(&direct, 0, sizeof(direct));
		direct.reType = RT_RAIL_CORE;
		direct.customShader = fx->beamHandle ? fx->beamHandle : cgs.media.railCoreShader;
		VectorCopy(s, direct.origin);
		VectorCopy(e, direct.oldorigin);
		AxisClear(direct.axis);

		if(weapon == WP_PHYSGUN) {
			direct.shaderRGBA[0] = ci->physR;
			direct.shaderRGBA[1] = ci->physG;
			direct.shaderRGBA[2] = ci->physB;
		} else if(weapon == WP_GRAVITYGUN) {
			direct.shaderRGBA[0] = 120;
			direct.shaderRGBA[1] = 60;
			direct.shaderRGBA[2] = 0;
		}
		direct.shaderRGBA[3] = 255;

		trap_R_AddRefEntityToScene(&direct);
		return;
	}

	// Per-shot beams (railgun): use a fading local entity so the classic trail
	// lingers for a moment after the shot.
	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;

	if(weapon == WP_RAILGUN) {
		le->endTime = cg.time + 400;
		re->shaderTime = cg.time / 1000.0f;
	} else {
		le->endTime = cg.time + 60;
		re->shaderTime = cg.time / 60.0f;
	}

	le->lifeRate = 1.0f / (le->endTime - le->startTime);

	re->reType = RT_RAIL_CORE;
	re->customShader = fx->beamHandle ? fx->beamHandle : cgs.media.railCoreShader;

	VectorCopy(s, re->origin);
	VectorCopy(e, re->oldorigin);

	switch(weapon) {
		case WP_RAILGUN:
			re->shaderRGBA[0] = 0;
			re->shaderRGBA[1] = 255;
			re->shaderRGBA[2] = 0;
			re->shaderRGBA[3] = 255;
			le->color[0] = 0.0f;
			le->color[1] = 1.0f;
			le->color[2] = 0.0f;
			le->color[3] = 1.0f;
			re->origin[2] -= 8;
			re->oldorigin[2] -= 8;
			break;

		case WP_PHYSGUN:
			re->shaderRGBA[0] = ci->physR;
			re->shaderRGBA[1] = ci->physG;
			re->shaderRGBA[2] = ci->physB;
			re->shaderRGBA[3] = 255;
			le->color[0] = ci->physR / 255.0f;
			le->color[1] = ci->physG / 255.0f;
			le->color[2] = ci->physB / 255.0f;
			le->color[3] = 1.0f;
			break;

		case WP_GRAVITYGUN:
			re->shaderRGBA[0] = 120;
			re->shaderRGBA[1] = 60;
			re->shaderRGBA[2] = 0;
			re->shaderRGBA[3] = 255;
			le->color[0] = 120.0f / 255.0f;
			le->color[1] = 60.0f / 255.0f;
			le->color[2] = 0.0f;
			le->color[3] = 1.0f;
			break;
	}

	AxisClear(re->axis);
}

/*
==========================
CG_MissileTrail
Data-driven particle trail for missiles.
==========================
*/
static void CG_MissileTrail(centity_t* ent, const weaponInfo_t* wi) {
	weaponFx_t* fx;
	vec3_t origin;
	int contents;

	if(cg_noProjectileTrail.integer) return;
	if(ent->currentState.weapon <= WP_NONE || ent->currentState.weapon >= WEAPONS_NUM) return;
	fx = &jsd_weaponFx[ent->currentState.weapon];
	if(fx->trailCount <= 0) return;

	if(ent->currentState.pos.trType == TR_STATIONARY) {
		ent->trailTime = cg.time;
		return;
	}

	if(fx->trailInterval > 0 && (cg.time - ent->trailTime) < fx->trailInterval) return;

	BG_EvaluateTrajectory(&ent->currentState.pos, cg.time, origin);
	contents = CG_PointContents(origin, -1);
	ent->trailTime = cg.time;

	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) return;

	CG_AddParticles(origin, fx->trailColor, fx->trailCount, fx->trailSpeed, fx->trailFlags, fx->trailGravity, fx->trailShader ? fx->trailShader : "");
}

static void CG_RocketTrail(centity_t* ent, const weaponInfo_t* wi) { CG_MissileTrail(ent, wi); }

/*
==========================
CG_PlasmaTrail
==========================
*/
static void CG_PlasmaTrail(centity_t* cent, const weaponInfo_t* wi) { CG_MissileTrail(cent, wi); }

/*
==========================
CG_GrenadeTrail
==========================
*/
static void CG_GrenadeTrail(centity_t* ent, const weaponInfo_t* wi) { CG_RocketTrail(ent, wi); }

void CG_RegisterWeapon(int weaponNum) {
	weaponInfo_t* weaponInfo;
	item_t* weaponItem;
	char path[MAX_QPATH];
	vec3_t mins, maxs;
	int i;

	weaponInfo = &cg_weapons[weaponNum];

	if(weaponNum == 0) return;
	if(weaponInfo->registered) return;

	memset(weaponInfo, 0, sizeof(*weaponInfo));
	weaponInfo->registered = true;

	// Register data-driven effect shaders for this weapon
	{
		weaponFx_t* fx = &jsd_weaponFx[weaponNum];
		if(fx->hitParticleShader && fx->hitParticleShader[0]) fx->hitParticleHandle = trap_R_RegisterShader(fx->hitParticleShader);
		if(fx->fireParticleShader && fx->fireParticleShader[0]) fx->fireParticleHandle = trap_R_RegisterShader(fx->fireParticleShader);
		if(fx->trailShader && fx->trailShader[0]) fx->trailHandle = trap_R_RegisterShader(fx->trailShader);
		if(fx->tracerShader && fx->tracerShader[0]) fx->tracerHandle = trap_R_RegisterShader(fx->tracerShader);
		if(fx->beamShader && fx->beamShader[0]) fx->beamHandle = trap_R_RegisterShader(fx->beamShader);
	}

	for(i = 0; i < jsd_itemCount; i++) {
		if(jsd_item[i].giType == IT_WEAPON && jsd_item[i].giTag == weaponNum) {
			weaponItem = &jsd_item[i];
			weaponInfo->item = weaponItem;
			break;
		}
	}
	if(!strlen(weaponItem->classname)) CG_Error("Couldn't find weapon %i", weaponNum);

	CG_RegisterItemVisuals(weaponItem - jsd_item);

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel(weaponItem->model);

	// calc midpoint for rotation
	trap_R_ModelBounds(weaponInfo->weaponModel, mins, maxs);
	for(i = 0; i < 3; i++) weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * (maxs[i] - mins[i]);

	weaponInfo->weaponIcon = trap_R_RegisterShader(weaponItem->icon);

	COM_StripExtension(weaponItem->model, path, sizeof(path));
	Q_strcat(path, sizeof(path), "_flash.md3");
	weaponInfo->flashModel = trap_R_RegisterModel(path);

	COM_StripExtension(weaponItem->model, path, sizeof(path));
	Q_strcat(path, sizeof(path), "_barrel.md3");
	weaponInfo->barrelModel = trap_R_RegisterModel(path);

	COM_StripExtension(weaponItem->model, path, sizeof(path));
	Q_strcat(path, sizeof(path), "_hand.md3");
	weaponInfo->handsModel = trap_R_RegisterModel(path);

	if(!weaponInfo->handsModel) weaponInfo->handsModel = trap_R_RegisterModel("models/weapons2/shotgun/shotgun_hand.md3");

	switch(weaponNum) {
		case WP_GAUNTLET:
			MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
			weaponInfo->firingSound = trap_S_RegisterSound("sound/weapons/melee/fstrun.wav", false);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/melee/fstatck.wav", false);
			break;

		case WP_LIGHTNING:
			MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
			weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/melee/fsthum.wav", false);
			weaponInfo->firingSound = trap_S_RegisterSound("sound/weapons/lightning/lg_hum.wav", false);

			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/lightning/lg_fire.wav", false);
			cgs.media.lightningShader = trap_R_RegisterShader("lightningBoltNew");
			cgs.media.lightningExplosionModel = trap_R_RegisterModel("models/weaphits/crackle.md3");
			cgs.media.sfx_lghit1 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit.wav", false);
			cgs.media.sfx_lghit2 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit2.wav", false);
			cgs.media.sfx_lghit3 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit3.wav", false);

			break;

		case WP_MACHINEGUN:
			MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/machinegun/machgf1b.wav", false);
			weaponInfo->flashSound[1] = trap_S_RegisterSound("sound/weapons/machinegun/machgf2b.wav", false);
			weaponInfo->flashSound[2] = trap_S_RegisterSound("sound/weapons/machinegun/machgf3b.wav", false);
			weaponInfo->flashSound[3] = trap_S_RegisterSound("sound/weapons/machinegun/machgf4b.wav", false);
			weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
			cgs.media.bulletExplosionShader = trap_R_RegisterShader("bulletExplosion");
			break;

		case WP_SHOTGUN:
			MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/shotgun/sshotf1b.wav", false);
			weaponInfo->ejectBrassFunc = CG_ShotgunEjectBrass;
			break;

		case WP_ROCKET_LAUNCHER:
			weaponInfo->missileModel = trap_R_RegisterModel("models/ammo/rocket/rocket.md3");
			weaponInfo->missileSound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", false);
			weaponInfo->missileTrailFunc = CG_RocketTrail;
			weaponInfo->missileDlight = 200;
			weaponInfo->wiTrailTime = 2000;
			weaponInfo->trailRadius = 64;

			MAKERGB(weaponInfo->missileDlightColor, 1, 0.75f, 0);
			MAKERGB(weaponInfo->flashDlightColor, 1, 0.75f, 0);

			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", false);
			cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");
			break;

		case WP_GRENADE_LAUNCHER:
			weaponInfo->missileModel = trap_R_RegisterModel("models/ammo/grenade1.md3");
			weaponInfo->missileTrailFunc = CG_GrenadeTrail;
			weaponInfo->wiTrailTime = 700;
			weaponInfo->trailRadius = 32;
			MAKERGB(weaponInfo->flashDlightColor, 1, 0.70f, 0);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/grenade/grenlf1a.wav", false);
			cgs.media.grenadeExplosionShader = trap_R_RegisterShader("grenadeExplosion");
			break;

		case WP_PLASMAGUN:
			weaponInfo->missileTrailFunc = CG_PlasmaTrail;
			weaponInfo->missileSound = trap_S_RegisterSound("sound/weapons/plasma/lasfly.wav", false);
			MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/plasma/hyprbf1a.wav", false);
			cgs.media.plasmaExplosionShader = trap_R_RegisterShader("plasmaExplosion");
			cgs.media.railRingsShader = trap_R_RegisterShader("railDisc");
			break;

		case WP_RAILGUN:
			weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/railgun/rg_hum.wav", false);
			MAKERGB(weaponInfo->flashDlightColor, 1, 0.5f, 0);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/railgun/railgf1a.wav", false);
			cgs.media.railExplosionShader = trap_R_RegisterShader("railExplosion");
			cgs.media.railRingsShader = trap_R_RegisterShader("railDisc");
			cgs.media.railCoreShader = trap_R_RegisterShader("railCore");
			break;

		case WP_BFG:
			weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/bfg/bfg_hum.wav", false);
			MAKERGB(weaponInfo->flashDlightColor, 1, 0.7f, 1);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/bfg/bfg_fire.wav", false);
			cgs.media.bfgExplosionShader = trap_R_RegisterShader("bfgExplosion");
			weaponInfo->missileModel = trap_R_RegisterModel("models/weaphits/bfg.md3");
			weaponInfo->missileSound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", false);
			break;

		case WP_TOOLGUN:
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/toolgun/fire1.wav", false);
			weaponInfo->flashSound[1] = trap_S_RegisterSound("sound/weapons/toolgun/fire2.wav", false);
			cgs.media.plasmaExplosionShader = trap_R_RegisterShader("plasmaExplosion");
			cgs.media.lightningShader = trap_R_RegisterShader("lightningBoltNew");
			break;

		case WP_PHYSGUN:
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/misc/silence.wav", false);
			cgs.media.plasmaExplosionShader = trap_R_RegisterShader("plasmaExplosion");
			cgs.media.lightningShader = trap_R_RegisterShader("lightningBoltNew");
			break;

		case WP_GRAVITYGUN:
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/misc/silence.wav", false);
			cgs.media.plasmaExplosionShader = trap_R_RegisterShader("plasmaExplosion");
			cgs.media.lightningShader = trap_R_RegisterShader("lightningBoltNew");
			break;

		default:
			MAKERGB(weaponInfo->flashDlightColor, 1, 1, 1);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", false);
			break;
	}
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals(int itemNum) {
	itemInfo_t* itemInfo;
	item_t* item;

	if(itemNum < 0 || itemNum >= jsd_itemCount) {
		CG_Error("CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, jsd_itemCount - 1);
	}

	itemInfo = &cg_items[itemNum];
	if(itemInfo->registered) {
		return;
	}

	item = &jsd_item[itemNum];

	memset(itemInfo, 0, sizeof(*itemInfo));
	itemInfo->registered = true;

	itemInfo->models[0] = trap_R_RegisterModel(item->model);

	itemInfo->icon = trap_R_RegisterShader(item->icon);

	if(item->giType == IT_WEAPON) CG_RegisterWeapon(item->giTag);
}

/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame(clientInfo_t* ci, int frame) {
	// change weapon
	if(frame >= ci->animations[TORSO_DROP].firstFrame && frame < ci->animations[TORSO_DROP].firstFrame + 9) {
		return frame - ci->animations[TORSO_DROP].firstFrame + 6;
	}

	// stand attack
	if(frame >= ci->animations[TORSO_ATTACK].firstFrame && frame < ci->animations[TORSO_ATTACK].firstFrame + 6) {
		return 1 + frame - ci->animations[TORSO_ATTACK].firstFrame;
	}

	// stand attack 2
	if(frame >= ci->animations[TORSO_ATTACK2].firstFrame && frame < ci->animations[TORSO_ATTACK2].firstFrame + 6) {
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstFrame;
	}

	return 0;
}

/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition(vec3_t origin, vec3_t angles) {
	float scale;
	int delta;
	float fracsin;

	VectorCopy(cg.refdef.vieworg, origin);
	VectorCopy(cg.refdefViewAngles, angles);

	// on odd legs, invert some angles
	if(cg.bobcycle & 1) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if(delta < LAND_DEFLECT_TIME) {
		origin[2] += cg.landChange * 0.25 * delta / LAND_DEFLECT_TIME;
	} else if(delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME) {
		origin[2] += cg.landChange * 0.25 * (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin(cg.time * 0.001);
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}

/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
static void CG_LightningBolt(centity_t* cent, vec3_t origin) {
	trace_t trace;
	refEntity_t beam;
	vec3_t forward;
	vec3_t muzzlePoint, endPoint;
	int anim;
	clientInfo_t* ci;
	weaponFx_t* fx;

	if(cent->currentState.weapon <= WP_NONE || cent->currentState.weapon >= WEAPONS_NUM) return;

	// Railgun beams are already drawn by the EV_RAILTRAIL handler. Calling
	// CG_RailTrail from here too would render every rail shot twice
	// (one predicted beam + one server-authoritative beam).
	if(cent->currentState.weapon == WP_RAILGUN) return;

	fx = &jsd_weaponFx[cent->currentState.weapon];
	if(fx->beamType == 0) return;

	ci = &cgs.clientinfo[cent->currentState.clientNum];

	memset(&beam, 0, sizeof(beam));

	// CPMA "true" lightning
	if((cent->currentState.number == cg.predictedPlayerState.clientNum) && (cg_trueLightning.value != 0)) {
		vec3_t angle;
		int i;

		for(i = 0; i < 3; i++) {
			float a = cent->lerpAngles[i] - cg.refdefViewAngles[i];
			if(a > 180) a -= 360;
			if(a < -180) a += 360;

			angle[i] = cg.refdefViewAngles[i] + a * (1.0 - cg_trueLightning.value);
			if(angle[i] < 0) angle[i] += 360;
			if(angle[i] > 360) angle[i] -= 360;
		}

		AngleVectors(angle, forward, NULL, NULL);
		VectorCopy(cent->lerpOrigin, muzzlePoint);
	} else {
		AngleVectors(cent->lerpAngles, forward, NULL, NULL);
		VectorCopy(cent->lerpOrigin, muzzlePoint);
	}

	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if(anim == LEGS_WALKCR || anim == LEGS_IDLECR) {
		muzzlePoint[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzlePoint[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA(muzzlePoint, 14, forward, muzzlePoint);
	VectorMA(muzzlePoint, jsd_weapon[cent->currentState.weapon].range, forward, endPoint);

	CG_Trace(&trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, cent->currentState.number, MASK_SHOT);

	VectorCopy(trace.endpos, beam.oldorigin);
	VectorCopy(origin, beam.origin);

	if(fx->beamType == RT_RAIL_CORE) {
		CG_RailTrail(ci, origin, trace.endpos, cent->currentState.weapon);
		if(trace.fraction < 1.0f) {
			vec3_t invDir;
			VectorNegate(trace.plane.normal, invDir);
			CG_EmitWeaponHitFx(cent->currentState.weapon, trace.endpos, invDir);
		}
		return;
	}

	// Lightning-style beam
	beam.reType = RT_LIGHTNING;
	beam.customShader = fx->beamHandle ? fx->beamHandle : cgs.media.lightningShader;
	trap_R_AddRefEntityToScene(&beam);

	if(trace.fraction < 1.0f) {
		vec3_t dir;
		VectorSubtract(beam.oldorigin, beam.origin, dir);
		VectorNormalize(dir);
		CG_EmitWeaponHitFx(cent->currentState.weapon, trace.endpos, dir);
	}
}

/*
======================
CG_MachinegunSpinAngle
======================
*/
#define SPIN_SPEED 0.9
#define COAST_TIME 1000
static float CG_MachinegunSpinAngle(centity_t* cent) {
	int delta;
	float angle;
	float speed;

	delta = cg.time - cent->pe.barrelTime;
	if(cent->pe.barrelSpinning) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if(delta > COAST_TIME) {
			delta = COAST_TIME;
		}

		speed = 0.5 * (SPIN_SPEED + (float)(COAST_TIME - delta) / COAST_TIME);
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if(cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING)) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod(angle);
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
	}

	return angle;
}

/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void CG_AddPlayerWeapon(refEntity_t* parent, playerState_t* ps, centity_t* cent, clientInfo_t* ci) {
	refEntity_t gun;
	refEntity_t barrel;
	refEntity_t flash;
	vec3_t angles;
	weapon_t weaponNum;
	weaponInfo_t* weapon;
	centity_t* nonPredictedCent;
	orientation_t lerped;

	weaponNum = cent->currentState.weapon;

	CG_RegisterWeapon(weaponNum);
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset(&gun, 0, sizeof(gun));
	VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if(weaponNum == WP_RAILGUN) {
		clientInfo_t* ci = &cgs.clientinfo[cent->currentState.clientNum];
		if(cent->pe.railFireTime + 1500 > cg.time) {
			int scale = 255 * (cg.time - cent->pe.railFireTime) / 1500;
			gun.shaderRGBA[0] = (ci->c1RGBA[0] * scale) >> 8;
			gun.shaderRGBA[1] = (ci->c1RGBA[1] * scale) >> 8;
			gun.shaderRGBA[2] = (ci->c1RGBA[2] * scale) >> 8;
			gun.shaderRGBA[3] = 255;
		} else {
			Byte4Copy(ci->c1RGBA, gun.shaderRGBA);
		}
	}

	if(weaponNum == WP_PHYSGUN) {
		gun.shaderRGBA[0] = ci->physR;
		gun.shaderRGBA[1] = ci->physG;
		gun.shaderRGBA[2] = ci->physB;
		gun.shaderRGBA[3] = 255;
	}
	if(weaponNum == WP_GRAVITYGUN) {
		gun.shaderRGBA[0] = 160;
		gun.shaderRGBA[1] = 80;
		gun.shaderRGBA[2] = 0;
		gun.shaderRGBA[3] = 255;
	}

	gun.hModel = weapon->weaponModel;
	if(!gun.hModel) {
		return;
	}

	if(!ps) {
		// add weapon ready sound
		cent->pe.lightningFiring = false;
		if((cent->currentState.eFlags & EF_FIRING) && weapon->firingSound) {
			// lightning gun and guantlet make a different sound when fire is held down
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound);
			cent->pe.lightningFiring = true;
		} else if(weapon->readySound) {
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound);
		}
	}

	trap_CM_LerpTag(&lerped, parent->hModel, parent->oldframe, parent->frame, 1.0 - parent->backlerp, "tag_weapon");
	VectorCopy(parent->origin, gun.origin);

	VectorMA(gun.origin, lerped.origin[0], parent->axis[0], gun.origin);

	// Make weapon appear left-handed for 2 and centered for 3
	if(ps && cg_drawGun.integer == 2)
		VectorMA(gun.origin, -lerped.origin[1], parent->axis[1], gun.origin);
	else if(!ps || cg_drawGun.integer != 3)
		VectorMA(gun.origin, lerped.origin[1], parent->axis[1], gun.origin);

	VectorMA(gun.origin, lerped.origin[2], parent->axis[2], gun.origin);

	MatrixMultiply(lerped.axis, ((refEntity_t*)parent)->axis, gun.axis);
	gun.backlerp = parent->backlerp;

	trap_R_AddRefEntityToScene(&gun);

	// add the spinning barrel
	if(weapon->barrelModel) {
		memset(&barrel, 0, sizeof(barrel));
		VectorCopy(parent->lightingOrigin, barrel.lightingOrigin);
		barrel.renderfx = parent->renderfx;

		barrel.hModel = weapon->barrelModel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = CG_MachinegunSpinAngle(cent);
		AnglesToAxis(angles, barrel.axis);

		CG_PositionRotatedEntityOnTag(&barrel, &gun, weapon->weaponModel, "tag_barrel");

		trap_R_AddRefEntityToScene(&barrel);
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.number];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on the single player podiums), so
	// go ahead and use the cent
	if((nonPredictedCent - cg_entities) != cent->currentState.clientNum) {
		nonPredictedCent = cent;
	}

	// add the flash
	if((weaponNum == WP_LIGHTNING || weaponNum == WP_GAUNTLET || weaponNum == WP_PHYSGUN || weaponNum == WP_GRAVITYGUN) && (nonPredictedCent->currentState.eFlags & EF_FIRING)) {
		// continuous flash
	} else {
		// impulse flash
		if(cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME) {
			return;
		}
	}

	memset(&flash, 0, sizeof(flash));
	VectorCopy(parent->lightingOrigin, flash.lightingOrigin);
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashModel;
	// if(!flash.hModel) {
	//	return;
	// }
	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis(angles, flash.axis);

	// colorize the railgun blast
	if(weaponNum == WP_RAILGUN) {
		clientInfo_t* ci;

		ci = &cgs.clientinfo[cent->currentState.clientNum];
		flash.shaderRGBA[0] = 255 * ci->color1[0];
		flash.shaderRGBA[1] = 255 * ci->color1[1];
		flash.shaderRGBA[2] = 255 * ci->color1[2];
	}

	CG_PositionRotatedEntityOnTag(&flash, &gun, weapon->weaponModel, "tag_flash");
	trap_R_AddRefEntityToScene(&flash);

	// Data-driven fire FX at tag_flash.
	//
	// In first-person we draw both the world model (for mirrors, but only
	// rendered with RF_THIRD_PERSON) and the view weapon. The world model is
	// emitted first by CG_AddPacketEntities, so without this guard it would
	// steal the pending fire event and place FX on the body instead of on the
	// view weapon. We therefore skip fire FX on the world model of the local
	// player while in first person; the view weapon (ps != NULL) is responsible
	// for it and runs later in CG_AddViewWeapon.
	{
		weaponFx_t* fx = &jsd_weaponFx[weaponNum];
		bool isViewWeapon = (ps != NULL);
		bool isLocalFirstPerson = (!isViewWeapon && cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson);
		bool shouldEmit = isViewWeapon || !isLocalFirstPerson;

		if(shouldEmit) {
			if(fx->continuousFire) {
				// Hold-to-fire weapon: light is re-added every frame (no flicker);
				// particles are throttled to avoid overspawning the pool.
				if(nonPredictedCent->currentState.eFlags & EF_FIRING) {
					if(fx->fireLightRadius > 0.0f) {
						trap_R_AddLightToScene(flash.origin, fx->fireLightRadius, 1.00f, fx->fireLightColor[0], fx->fireLightColor[1], fx->fireLightColor[2], 0);
					}
					if(fx->fireParticleCount > 0 && (cg.time - cent->pe.lastFireFxTime) >= 30) {
						CG_AddParticles(flash.origin, fx->fireParticleColor, fx->fireParticleCount, fx->fireParticleSpeed, fx->fireParticleFlags, fx->fireParticleGravity, fx->fireParticleShader ? fx->fireParticleShader : "");
						cent->pe.lastFireFxTime = cg.time;
					}
				}
				cent->pe.fireFxPending = false;
			} else if(cent->pe.fireFxPending) {
				// Per-shot weapon: one fading light + one particle burst per shot.
				CG_EmitWeaponFireFx(weaponNum, flash.origin);
				cent->pe.fireFxPending = false;
			}
		}
	}

	if(ps || cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum) {
		// add lightning bolt / rail trail for beam weapons
		CG_LightningBolt(nonPredictedCent, flash.origin);
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon(playerState_t* ps) {
	refEntity_t hand;
	centity_t* cent;
	clientInfo_t* ci;
	float fovOffset;
	vec3_t angles;
	weaponInfo_t* weapon;

	// no gun if in third person view or a camera is active
	// if ( cg.renderingThirdPerson || cg.cameraMode) {
	if(cg.renderingThirdPerson) {
		return;
	}

	// allow the gun to be completely removed
	if(!cg_drawGun.integer) {
		vec3_t origin;

		if(cg.predictedPlayerState.eFlags & EF_FIRING) {
			// special hack for lightning gun...
			VectorCopy(cg.refdef.vieworg, origin);
			VectorMA(origin, -8, cg.refdef.viewaxis[2], origin);
			CG_LightningBolt(&cg_entities[ps->clientNum], origin);
		}
		return;
	}

	// don't draw if testing a gun model
	if(cg.testGun) {
		return;
	}

	// drop gun lower at higher fov
	if(cg_fov.integer > 90) {
		fovOffset = -0.2 * (cg_fov.integer - 90);
	} else {
		fovOffset = 0;
	}

	cent = &cg.predictedPlayerEntity;  // &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon(ps->weapon);
	weapon = &cg_weapons[ps->weapon];

	memset(&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition(hand.origin, angles);

	VectorMA(hand.origin, 5, cg.refdef.viewaxis[0], hand.origin);
	VectorMA(hand.origin, -1, cg.refdef.viewaxis[1], hand.origin);
	VectorMA(hand.origin, (-1 + fovOffset), cg.refdef.viewaxis[2], hand.origin);

	AnglesToAxis(angles, hand.axis);

	ci = &cgs.clientinfo[cent->currentState.clientNum];
	hand.frame = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.frame);
	hand.oldframe = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.oldFrame);
	hand.backlerp = cent->pe.torso.backlerp;

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon(&hand, ps, &cg.predictedPlayerEntity, ci);
}

void CG_NextWeapon_f(void) {
	bool weaponFound = false;
	int original, i;

	if(!cg.snap || cg.snap->ps.pm_flags & PMF_FOLLOW) return;

	if(cg.snap->ps.weapon == WP_PHYSGUN && cg.snap->ps.eFlags & EF_FIRING) {
		trap_SendConsoleCommand(EXEC_INSERT, "physgun_dist 0\n");
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for(i = 1; i < WEAPONS_NUM; i++) {
		cg.weaponSelect++;
		if(cg.weaponSelect >= WEAPONS_NUM) cg.weaponSelect = 1;

		if(cg.swep_listcl[cg.weaponSelect]) {
			weaponFound = true;
			break;
		}
	}

	if(!weaponFound) cg.weaponSelect = original;
}

void CG_PrevWeapon_f(void) {
	bool weaponFound = false;
	int original, i;

	if(!cg.snap || cg.snap->ps.pm_flags & PMF_FOLLOW) return;

	if(cg.snap->ps.weapon == WP_PHYSGUN && cg.snap->ps.eFlags & EF_FIRING) {
		trap_SendConsoleCommand(EXEC_INSERT, "physgun_dist 1\n");
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for(i = 1; i < WEAPONS_NUM; i++) {
		cg.weaponSelect--;
		if(cg.weaponSelect <= 0) cg.weaponSelect = WEAPONS_NUM;

		if(cg.swep_listcl[cg.weaponSelect]) {
			weaponFound = true;
			break;
		}
	}

	if(!weaponFound) cg.weaponSelect = original;
}

void CG_Weapon_f(void) {
	int num;

	if(!cg.snap || cg.snap->ps.pm_flags & PMF_FOLLOW) return;

	num = atoi(CG_Argv(1));

	if(!cg.swep_listcl[num]) return;

	cg.weaponSelect = num;
}

/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon(centity_t* cent) {
	entityState_t* ent;
	int c;
	weaponInfo_t* weap;

	ent = &cent->currentState;
	if(ent->weapon == WP_NONE) {
		return;
	}
	if(ent->weapon >= WEAPONS_NUM) {
		CG_Error("CG_FireWeapon: ent->weapon >= WEAPONS_NUM");
		return;
	}
	weap = &cg_weapons[ent->weapon];

	// mark the entity as muzzle flashing
	cent->muzzleFlashTime = cg.time;

	// lightning gun only does this on initial press
	if(ent->weapon == WP_LIGHTNING) {
		if(cent->pe.lightningFiring) {
			return;
		}
	}

	if(ent->weapon == WP_RAILGUN) {
		cent->pe.railFireTime = cg.time;
	}

	// play a sound
	for(c = 0; c < 4; c++) {
		if(!weap->flashSound[c]) {
			break;
		}
	}
	if(c > 0) {
		c = rand() % c;
		if(weap->flashSound[c]) {
			trap_S_StartSound(NULL, ent->number, CHAN_WEAPON, weap->flashSound[c]);
		}
	}

	// Fire FX are emitted from CG_AddPlayerWeapon where tag_flash is available.
	cent->pe.fireFxPending = true;

	// brass ejection
	if(weap->ejectBrassFunc && cg_brassTime.integer > 0) {
		weap->ejectBrassFunc(cent);
	}
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType) { CG_EmitWeaponHitFx(weapon, origin, dir); }

/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir, int entityNum) {
	CG_Bleed(origin, entityNum);

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch(weapon) {
		case WP_GRENADE_LAUNCHER:
		case WP_ROCKET_LAUNCHER: CG_MissileHitWall(weapon, 0, origin, dir, IMPACTSOUND_FLESH); break;
		default: break;
	}
}

/*
============================================================================

SHOTGUN TRACING

============================================================================
*/

/*
================
CG_ShotgunPellet
================
*/
static void CG_ShotgunPellet(vec3_t start, vec3_t end, int skipNum) {
	trace_t tr;
	int sourceContentType, destContentType;

	CG_Trace(&tr, start, NULL, NULL, end, skipNum, MASK_SHOT);

	sourceContentType = CG_PointContents(start, 0);
	destContentType = CG_PointContents(tr.endpos, 0);

	// FIXME: should probably move this cruft into CG_BubbleTrail
	if(sourceContentType == destContentType) {
		if(sourceContentType & CONTENTS_WATER) {
			CG_BubbleTrail(start, tr.endpos, 32);
		}
	} else if(sourceContentType & CONTENTS_WATER) {
		trace_t trace;

		trap_CM_BoxTrace(&trace, end, start, NULL, NULL, 0, CONTENTS_WATER);
		CG_BubbleTrail(start, trace.endpos, 32);
	} else if(destContentType & CONTENTS_WATER) {
		trace_t trace;

		trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, CONTENTS_WATER);
		CG_BubbleTrail(tr.endpos, trace.endpos, 32);
	}

	if(cg_entities[tr.entityNum].currentState.eType == ET_PLAYER) {
		CG_MissileHitPlayer(WP_SHOTGUN, tr.endpos, tr.plane.normal, tr.entityNum);
	} else {
		if(tr.surfaceFlags & SURF_METAL) {
			CG_MissileHitWall(WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL);
		} else {
			CG_MissileHitWall(WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT);
		}
	}
}

/*
================
CG_ShotgunPattern

Perform the same traces the server did to locate the
hit splashes
================
*/
static void CG_ShotgunPattern(vec3_t origin, vec3_t origin2, int seed, int otherEntNum, int weapon) {
	int i;
	float r, u;
	vec3_t end;
	vec3_t forward, right, up;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2(origin2, forward);
	PerpendicularVector(right, forward);
	CrossProduct(forward, right, up);

	// generate the "random" spread pattern
	for(i = 0; i < jsd_weapon[weapon].count; i++) {
		r = Q_crandom(&seed) * jsd_weapon[weapon].spread * 16;
		u = Q_crandom(&seed) * jsd_weapon[weapon].spread * 16;
		VectorMA(origin, jsd_weapon[weapon].range * 16, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);

		CG_ShotgunPellet(origin, end, otherEntNum);
	}
}

/*
==============
CG_ShotgunFire
==============
*/
void CG_ShotgunFire(entityState_t* es, int weapon) {
	vec3_t v;
	int contents;

	VectorSubtract(es->origin2, es->pos.trBase, v);
	VectorNormalize(v);
	VectorScale(v, 32, v);
	VectorAdd(es->pos.trBase, v, v);
	// ragepro can't alpha fade, so don't even bother with smoke
	vec3_t up;

	contents = CG_PointContents(es->pos.trBase, 0);
	if(!(contents & CONTENTS_WATER)) {
		VectorSet(up, 0, 0, 8);
		CG_SmokePuff(v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader);
	}
	CG_ShotgunPattern(es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum, weapon);
}

/*
============================================================================

BULLETS

============================================================================
*/

/*
===============
CG_Tracer
===============
*/
void CG_Tracer(vec3_t source, vec3_t dest, int weapon) {
	weaponFx_t* fx;
	vec3_t dir, pos;
	float len, begin, end, f;
	int i, numParticles;

	if(weapon <= WP_NONE || weapon >= WEAPONS_NUM) return;
	fx = &jsd_weaponFx[weapon];
	if(fx->tracerParticleCount <= 0) return;

	VectorSubtract(dest, source, dir);
	len = VectorNormalize(dir);
	if(len < 100) return;

	begin = 50 + random() * (len - 60);
	end = begin + (fx->tracerLength > 0.0f ? fx->tracerLength : 100.0f);
	if(end > len) end = len;

	// Particle tracer: emit a short streak of dots along the ray that fade out.
	numParticles = fx->tracerParticleCount;
	for(i = 0; i < numParticles; i++) {
		f = (numParticles > 1) ? (float)i / (numParticles - 1) : 0.0f;
		VectorMA(source, begin + (end - begin) * f, dir, pos);

		// Small perpendicular jitter so the dots don't form a perfect line.
		pos[0] += crandom() * 1.5f;
		pos[1] += crandom() * 1.5f;
		pos[2] += crandom() * 1.5f;

		CG_AddParticlesEx(pos, fx->tracerParticleColor, 1, fx->tracerParticleSpeed, PF_SMOOTH_FADE, NULL, fx->tracerShader ? fx->tracerShader : "", 0.25f + crandom() * 0.10f);
	}

	if(cgs.media.tracerSound) {
		vec3_t midpoint;
		midpoint[0] = (source[0] + dest[0]) * 0.5f;
		midpoint[1] = (source[1] + dest[1]) * 0.5f;
		midpoint[2] = (source[2] + dest[2]) * 0.5f;
		trap_S_StartSound(midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound);
	}
}

/*
======================
CG_CalcMuzzlePoint
======================
*/
static bool CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle) {
	vec3_t forward;
	centity_t* cent;
	int anim;

	if(entityNum == cg.snap->ps.clientNum) {
		VectorCopy(cg.snap->ps.origin, muzzle);
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors(cg.snap->ps.viewangles, forward, NULL, NULL);
		VectorMA(muzzle, 14, forward, muzzle);
		return true;
	}

	cent = &cg_entities[entityNum];
	if(!cent->currentValid) {
		return false;
	}

	VectorCopy(cent->currentState.pos.trBase, muzzle);

	AngleVectors(cent->currentState.apos.trBase, forward, NULL, NULL);
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if(anim == LEGS_WALKCR || anim == LEGS_IDLECR) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA(muzzle, 14, forward, muzzle);

	return true;
}

/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet(vec3_t end, int sourceEntityNum, vec3_t normal, bool flesh, int fleshEntityNum, int weapon) {
	trace_t trace;
	int sourceContentType, destContentType;
	vec3_t start;
	weaponFx_t* fx;

	if(weapon <= WP_NONE || weapon >= WEAPONS_NUM) weapon = WP_MACHINEGUN;
	fx = &jsd_weaponFx[weapon];

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if(sourceEntityNum >= 0 && fx->tracerChance > 0.0f) {
		if(CG_CalcMuzzlePoint(sourceEntityNum, start)) {
			sourceContentType = CG_PointContents(start, 0);
			destContentType = CG_PointContents(end, 0);

			if((sourceContentType == destContentType) && (sourceContentType & CONTENTS_WATER)) {
				CG_BubbleTrail(start, end, 32);
			} else if(sourceContentType & CONTENTS_WATER) {
				trap_CM_BoxTrace(&trace, end, start, NULL, NULL, 0, CONTENTS_WATER);
				CG_BubbleTrail(start, trace.endpos, 32);
			} else if(destContentType & CONTENTS_WATER) {
				trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, CONTENTS_WATER);
				CG_BubbleTrail(trace.endpos, end, 32);
			}

			if(random() < fx->tracerChance) {
				CG_Tracer(start, end, weapon);
			}
		}
	}

	// impact splash
	if(flesh)
		CG_Bleed(end, fleshEntityNum);
	else
		CG_MissileHitWall(weapon, 0, end, normal, IMPACTSOUND_DEFAULT);
}
