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
                .fireSound = {"sound/weapons/melee/fstatck.wav"},
                .continuousFireSound = "sound/weapons/melee/fstrun.wav",
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
                .tracerLength = 300.0f,
                .tracerWidth = 1.0f,
                .tracerParticleShader = "",
                .tracerParticleColor = {1.0f, 1.0f, 0.7f, 1.0f},
                .tracerParticleCount = 10,
                .tracerParticleSpeed = 0.0f,
                .tracerParticleFlags = PF_SMOOTH_FADE,
                .tracerParticleGravity = {0.0f, 0.0f, 0.0f},
                .tracerParticleSize = 4.0f,
                .tracerParticleLifetime = 0.25f,
                .tracerSound = "sound/weapons/machinegun/buletby1.wav",
                .fireSound = {"sound/weapons/machinegun/machgf1b.wav", "sound/weapons/machinegun/machgf2b.wav", "sound/weapons/machinegun/machgf3b.wav", "sound/weapons/machinegun/machgf4b.wav"},
                .brassModel = "models/weapons2/shells/m_shell.md3",
                .brassCount = 1,
                .brassOffset = {8.0f, -4.0f, 24.0f},
                .brassVelocity = {0.0f, -50.0f, 100.0f},
                .brassVelocitySpread = {0.0f, 40.0f, 50.0f},
                .brassBounceFactor = 0.4f,
                .brassLifetimeMult = 1,
                .brassSpinDelta = {2.0f, 1.0f, 0.0f},
                .barrelSpins = true,
                .barrelSpinSpeed = 0.9f,
                .barrelCoastTime = 1000,
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
                .tracerLength = 300.0f,
                .tracerWidth = 1.0f,
                .tracerParticleShader = "",
                .tracerParticleColor = {1.0f, 1.0f, 0.7f, 1.0f},
                .tracerParticleCount = 10,
                .tracerParticleSpeed = 0.0f,
                .tracerParticleFlags = PF_SMOOTH_FADE,
                .tracerParticleGravity = {0.0f, 0.0f, 0.0f},
                .tracerParticleSize = 4.0f,
                .tracerParticleLifetime = 0.25f,
                .fireSound = {"sound/weapons/shotgun/sshotf1b.wav"},
                .muzzleSmokeShader = "shotgunSmokePuff",
                .brassModel = "models/weapons2/shells/s_shell.md3",
                .brassCount = 2,
                .brassOffset = {8.0f, 0.0f, 24.0f},
                .brassVelocity = {60.0f, 0.0f, 100.0f},
                .brassVelocitySpread = {60.0f, 40.0f, 50.0f},
                .brassBounceFactor = 0.3f,
                .brassLifetimeMult = 3,
                .brassSpinDelta = {1.0f, 0.5f, 0.0f},
            },
        [WP_GRENADE_LAUNCHER] =
            {
                .explodeOnFleshHit = true,
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
                .missileModel = "models/ammo/grenade1.md3",
                .fireSound = {"sound/weapons/grenade/grenlf1a.wav"},
            },
        [WP_ROCKET_LAUNCHER] =
            {
                .explodeOnFleshHit = true,
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
                .missileModel = "models/ammo/rocket/rocket.md3",
                .missileFlySound = "sound/weapons/rocket/rockfly.wav",
                .fireSound = {"sound/weapons/rocket/rocklf1a.wav"},
            },
        [WP_LIGHTNING] =
            {
                .isBeam = true,
                .beamColor = {1.0f, 1.0f, 1.0f, 1.0f},
                .continuousFire = true,
                .suppressRepeatFire = true,
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
                .beamShader = "lightningBoltNew",
                .readySound = "sound/weapons/melee/fsthum.wav",
                .fireSound = {"sound/weapons/lightning/lg_fire.wav"},
                .continuousFireSound = "sound/weapons/lightning/lg_hum.wav",
            },
        [WP_RAILGUN] =
            {
                .skipLightningBolt = true,
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
                .isBeam = true,
                .beamShader = "railCore",
                .beamColor = {0.0f, 1.0f, 0.0f, 1.0f},
                .beamStartOffset = {0.0f, 0.0f, -12.0f},
                .beamEndOffset = {0.0f, 0.0f, -8.0f},
                .readySound = "sound/weapons/railgun/rg_hum.wav",
                .fireSound = {"sound/weapons/railgun/railgf1a.wav"},
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
                .missileSprite = "sprites/plasma1",
                .fireSound = {"sound/weapons/plasma/hyprbf1a.wav"},
                .missileFlySound = "sound/weapons/plasma/lasfly.wav",
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
                .missileModel = "models/weaphits/bfg.md3",
                .missileFlySound = "sound/weapons/rocket/rockfly.wav",
                .readySound = "sound/weapons/bfg/bfg_hum.wav",
                .fireSound = {"sound/weapons/bfg/bfg_fire.wav"},
            },
        [WP_PHYSGUN] =
            {
                .continuousFire = true,
                .modelColorSource = 1,
                .hitLightColor = {0.6f, 0.6f, 1.0f},
                .hitLightRadius = 5.0f,
                .hitLightTime = 50,
                .hitParticleColor = {0.7f, 0.7f, 1.0f, 1.0f},
                .hitParticleCount = 0,
                .hitParticleSpeed = 100.0f,
                .hitParticleFlags = PF_SMOOTH_FADE,
                .hitParticleGravity = {0.0f, 0.0f, 0.0f},
                .fireLightColor = {0.6f, 0.6f, 1.0f},
                .fireLightRadius = 0.0f,
                .fireLightTime = 50,
                .fireParticleColor = {0.7f, 0.7f, 1.0f, 1.0f},
                .fireParticleCount = 0,
                .fireParticleSpeed = 60.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .isBeam = true,
                .beamShader = "gfx/beams/core",
                .beamColorizeByClient = true,
                .fireSound = {"sound/misc/silence.wav"},
            },
        [WP_GRAVITYGUN] =
            {
                .continuousFire = true,
                .modelColorSource = 2,
                .modelColor = {160.0f / 255.0f, 80.0f / 255.0f, 0.0f},
                .hitLightColor = {1.0f, 0.5f, 0.0f},
                .hitLightRadius = 0.0f,
                .hitLightTime = 50,
                .hitParticleColor = {1.0f, 0.5f, 0.0f, 1.0f},
                .hitParticleCount = 0,
                .hitParticleSpeed = 100.0f,
                .hitParticleFlags = PF_SMOOTH_FADE,
                .hitParticleGravity = {0.0f, 0.0f, 0.0f},
                .fireLightColor = {1.0f, 0.5f, 0.0f},
                .fireLightRadius = 0.0f,
                .fireLightTime = 50,
                .fireParticleColor = {1.0f, 0.5f, 0.0f, 1.0f},
                .fireParticleCount = 0,
                .fireParticleSpeed = 60.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .isBeam = true,
                .beamShader = "gfx/beams/core",
                .beamColor = {120.0f / 255.0f, 60.0f / 255.0f, 0.0f, 1.0f},
                .fireSound = {"sound/misc/silence.wav"},
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
                .fireParticleCount = 0,
                .fireParticleSpeed = 60.0f,
                .fireParticleFlags = PF_SMOOTH_FADE,
                .fireParticleGravity = {0.0f, 0.0f, 0.0f},
                .isBeam = true,
                .beamColor = {1.0f, 1.0f, 1.0f, 1.0f},
                .beamShader = "lightningBoltNew",
                .fireSound = {"sound/weapons/toolgun/fire1.wav", "sound/weapons/toolgun/fire2.wav"},
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
		CG_AddParticles(sprOrg, fx->hitParticleColor, fx->hitParticleCount, fx->hitParticleSpeed, fx->hitParticleFlags, fx->hitParticleGravity, fx->hitParticleShader ? fx->hitParticleShader : "", fx->hitParticleSize, fx->hitParticleLifetime);
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
		CG_AddParticles(origin, fx->fireParticleColor, fx->fireParticleCount, fx->fireParticleSpeed, fx->fireParticleFlags, fx->fireParticleGravity, fx->fireParticleShader ? fx->fireParticleShader : "", fx->fireParticleSize, fx->fireParticleLifetime);
	}
	if(fx->fireLightRadius > 0.0f) {
		CG_SpawnWeaponLight(origin, fx->fireLightRadius, fx->fireLightTime, fx->fireLightColor);
	}
}
/*
==========================
CG_EjectBrass

Fully data-driven shell ejection: model, count, origin offset, velocity,
spread, bounce factor, lifetime multiplier, and spin are read from the
weapon's jsd_weaponFx entry.
==========================
*/
static void CG_EjectBrass(centity_t* cent) {
	weaponFx_t* fx;
	localEntity_t* le;
	refEntity_t* re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	vec3_t v[3];
	int i;

	if(cg_brassTime.integer <= 0) return;

	fx = &jsd_weaponFx[cent->currentState.weapon];
	if(!fx->brassModelHandle || fx->brassCount <= 0) return;

	for(i = 0; i < fx->brassCount; i++) {
		float waterScale = 1.0f;

		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		velocity[0] = fx->brassVelocity[0] + crandom() * fx->brassVelocitySpread[0];
		velocity[1] = fx->brassVelocity[1] + crandom() * fx->brassVelocitySpread[1];
		velocity[2] = fx->brassVelocity[2] + crandom() * fx->brassVelocitySpread[2];

		le->leType = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime = le->startTime + cg_brassTime.integer * fx->brassLifetimeMult + (cg_brassTime.integer * fx->brassLifetimeMult / 4) * random();

		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time - (rand() & 15);

		AnglesToAxis(cent->lerpAngles, v);

		offset[0] = fx->brassOffset[0];
		offset[1] = fx->brassOffset[1];
		offset[2] = fx->brassOffset[2];

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
		re->hModel = fx->brassModelHandle;
		le->bounceFactor = fx->brassBounceFactor * waterScale;

		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand() & 31;
		le->angles.trBase[1] = rand() & 31;
		le->angles.trBase[2] = rand() & 31;
		le->angles.trDelta[0] = fx->brassSpinDelta[0];
		le->angles.trDelta[1] = fx->brassSpinDelta[1];
		le->angles.trDelta[2] = fx->brassSpinDelta[2];

		le->leFlags = LEF_TUMBLE;
		le->leBounceSoundType = LEBS_BRASS;
		le->leMarkType = LEMT_NONE;
	}
}

// Noire's Mod: "rubbery tension" state for the physgun beam.
// We track camera angular velocity and integrate a damped spring so the beam
// bows like an elastic band under load, then wobbles back to perfectly straight
// once the player stops turning. No constant sag.
static float physBeamPrevYaw = 0.0f;
static float physBeamPrevPitch = 0.0f;
static float physBeamYawVel = 0.0f;
static float physBeamPitchVel = 0.0f;
static int physBeamLastTime = 0;

// View-space bow offset (right/up components) and its spring velocity.
static float physBeamBendRight = 0.0f;
static float physBeamBendUp = 0.0f;
static float physBeamBendRightVel = 0.0f;
static float physBeamBendUpVel = 0.0f;

// Noire's Mod: uniform beam rendering helper. Emits one poly quad (1 sector)
// with a caller-selected shader, billboarded perpendicular to the beam axis in
// view space.
//
// UV convention matches the physgun whip strip: U runs along the beam length
// (0..1), V runs across the beam width (0..1). Per-weapon textures authored
// rotated 90 degrees work with this layout.
#define BEAM_HALFWIDTH 2.0f

void CG_EmitBeamPoly(qhandle_t shader, const vec3_t start, const vec3_t end, float halfWidth, const vec4_t color) {
	vec3_t dir, viewDir, side;
	polyVert_t verts[4];
	byte mod[4];
	int i;

	if(!shader) return;

	mod[0] = (byte)(color[0] * 255);
	mod[1] = (byte)(color[1] * 255);
	mod[2] = (byte)(color[2] * 255);
	mod[3] = (byte)(color[3] * 255);

	VectorSubtract(end, start, dir);
	if(VectorNormalize(dir) == 0) return;
	VectorSubtract(start, cg.refdef.vieworg, viewDir);
	VectorNormalize(viewDir);
	CrossProduct(dir, viewDir, side);
	if(VectorLengthSquared(side) < 1e-6f) {
		// Beam axis points straight at the camera: use view right so the quad
		// stays visible and doesn't collapse to zero width.
		VectorCopy(cg.refdef.viewaxis[1], side);
	}
	VectorNormalize(side);
	VectorScale(side, halfWidth, side);

	verts[0].xyz[0] = start[0] - side[0];
	verts[0].xyz[1] = start[1] - side[1];
	verts[0].xyz[2] = start[2] - side[2];
	verts[0].st[0] = 0.0f;
	verts[0].st[1] = 0.0f;

	verts[1].xyz[0] = start[0] + side[0];
	verts[1].xyz[1] = start[1] + side[1];
	verts[1].xyz[2] = start[2] + side[2];
	verts[1].st[0] = 0.0f;
	verts[1].st[1] = 1.0f;

	verts[2].xyz[0] = end[0] + side[0];
	verts[2].xyz[1] = end[1] + side[1];
	verts[2].xyz[2] = end[2] + side[2];
	verts[2].st[0] = 1.0f;
	verts[2].st[1] = 1.0f;

	verts[3].xyz[0] = end[0] - side[0];
	verts[3].xyz[1] = end[1] - side[1];
	verts[3].xyz[2] = end[2] - side[2];
	verts[3].st[0] = 1.0f;
	verts[3].st[1] = 0.0f;

	for(i = 0; i < 4; i++) {
		verts[i].modulate[0] = mod[0];
		verts[i].modulate[1] = mod[1];
		verts[i].modulate[2] = mod[2];
		verts[i].modulate[3] = mod[3];
	}

	trap_R_AddPolysToScene(shader, 4, verts, 1);
}

void CG_RailTrail(clientInfo_t* ci, vec3_t start, vec3_t end, int weapon) {
	weaponFx_t* fx;
	localEntity_t* le;
	refEntity_t* re;
	vec3_t s, e;
	bool physgunActive;
	int i;

	if(weapon <= WP_NONE || weapon >= WEAPONS_NUM) return;
	fx = &jsd_weaponFx[weapon];
	if(!fx->isBeam) return;

	VectorCopy(start, s);
	VectorCopy(end, e);

	for(i = 0; i < 3; i++) {
		s[i] += fx->beamStartOffset[i];
		e[i] += fx->beamEndOffset[i];
	}

	physgunActive = (fx->continuousFire && weapon == WP_PHYSGUN && cg.grabbedEntityNum > 0 && cg.grabbedEntityNum < MAX_GENTITIES);

	// No active grab -> no tension. Wipe spring state so the next grab starts
	// perfectly straight instead of inheriting stale bend.
	if(!physgunActive) {
		physBeamPrevYaw = cg.refdefViewAngles[YAW];
		physBeamPrevPitch = cg.refdefViewAngles[PITCH];
		physBeamYawVel = 0.0f;
		physBeamPitchVel = 0.0f;
		physBeamBendRight = 0.0f;
		physBeamBendUp = 0.0f;
		physBeamBendRightVel = 0.0f;
		physBeamBendUpVel = 0.0f;
		physBeamLastTime = 0;
	}

	// Noire's Mod: physgun beam bends (32 sectors) toward the grabbed entity,
	// because the grabbed prop is server-authoritative and client traces miss it.
	if(physgunActive) {
		centity_t* grabCent = &cg_entities[cg.grabbedEntityNum];
		vec3_t grabOrigin;
		vec3_t viewRight, viewUp, dummyForward;
		vec4_t beamColor;
		byte modulate[4];
		float bendRight, bendUp;
		float frameDt;
		int seg, i;
#define PHY_BEAM_SEGMENTS 32
#define PHY_BEAM_BEND_SCALE 0.16f
#define PHY_BEAM_BEND_MAXVEL 600.0f
#define PHY_BEAM_STIFFNESS 55.0f
#define PHY_BEAM_DAMPING 11.0f
#define PHY_BEAM_HALFWIDTH 2.0f
		vec3_t points[PHY_BEAM_SEGMENTS + 1];
		vec3_t sides[PHY_BEAM_SEGMENTS + 1];
		static polyVert_t stripVerts[PHY_BEAM_SEGMENTS * 4];

		// Endpoint is the grab point on the prop, not its center. angles2
		// stores the local offset; rotate it into world space with the
		// entity's interpolated angles so it follows the prop's rotation.
		{
			vec3_t localOff, grabAxis[3];
			int gi;
			VectorCopy(grabCent->currentState.angles2, localOff);
			AnglesToAxis(grabCent->lerpAngles, grabAxis);
			for(gi = 0; gi < 3; gi++) {
				grabOrigin[gi] = grabCent->lerpOrigin[gi]
					+ localOff[0] * grabAxis[0][gi]
					+ localOff[1] * grabAxis[1][gi]
					+ localOff[2] * grabAxis[2][gi];
			}
		}

		frameDt = (float)cg.frametime * 0.001f;
		if(frameDt <= 0.0f || frameDt > 0.1f) frameDt = 1.0f / 60.0f;

		// --- Camera angular velocity tracking ---
		{
			float camYaw = cg.refdefViewAngles[YAW];
			float camPitch = cg.refdefViewAngles[PITCH];
			float rawYawVel = 0.0f, rawPitchVel = 0.0f;

			if(physBeamLastTime > 0) {
				int nowDt = cg.time - physBeamLastTime;
				if(nowDt > 0 && nowDt < 200) {
					float yawDelta = camYaw - physBeamPrevYaw;
					float pitchDelta = camPitch - physBeamPrevPitch;
					while(yawDelta > 180.0f) yawDelta -= 360.0f;
					while(yawDelta < -180.0f) yawDelta += 360.0f;
					while(pitchDelta > 180.0f) pitchDelta -= 360.0f;
					while(pitchDelta < -180.0f) pitchDelta += 360.0f;
					rawYawVel = yawDelta * 1000.0f / (float)nowDt;
					rawPitchVel = pitchDelta * 1000.0f / (float)nowDt;
				}
			}

			// Low-pass filter so mouse jitter doesn't jolt the beam.
			physBeamYawVel += (rawYawVel - physBeamYawVel) * 0.35f;
			physBeamPitchVel += (rawPitchVel - physBeamPitchVel) * 0.35f;

			physBeamPrevYaw = camYaw;
			physBeamPrevPitch = camPitch;
			physBeamLastTime = cg.time;

			if(physBeamYawVel > PHY_BEAM_BEND_MAXVEL) physBeamYawVel = PHY_BEAM_BEND_MAXVEL;
			if(physBeamYawVel < -PHY_BEAM_BEND_MAXVEL) physBeamYawVel = -PHY_BEAM_BEND_MAXVEL;
			if(physBeamPitchVel > PHY_BEAM_BEND_MAXVEL) physBeamPitchVel = PHY_BEAM_BEND_MAXVEL;
			if(physBeamPitchVel < -PHY_BEAM_BEND_MAXVEL) physBeamPitchVel = -PHY_BEAM_BEND_MAXVEL;
		}

		// --- Damped spring: rubbery lag, opposite sign to camera turn ---
		// Heavy prop drags on the beam. When the player turns, the beam's mid
		// bows against the turn (the tension in the other direction), the
		// spring overshoots, wobbles, then settles back to a straight line.
		{
			float targetRight = -physBeamYawVel * PHY_BEAM_BEND_SCALE;
			float targetUp = -physBeamPitchVel * PHY_BEAM_BEND_SCALE;

			physBeamBendRightVel += (targetRight - physBeamBendRight) * PHY_BEAM_STIFFNESS * frameDt;
			physBeamBendRightVel -= physBeamBendRightVel * PHY_BEAM_DAMPING * frameDt;
			physBeamBendRight += physBeamBendRightVel * frameDt;

			physBeamBendUpVel += (targetUp - physBeamBendUp) * PHY_BEAM_STIFFNESS * frameDt;
			physBeamBendUpVel -= physBeamBendUpVel * PHY_BEAM_DAMPING * frameDt;
			physBeamBendUp += physBeamBendUpVel * frameDt;
		}

		bendRight = physBeamBendRight;
		bendUp = physBeamBendUp;

		AngleVectors(cg.refdefViewAngles, dummyForward, viewRight, viewUp);

		if(fx->beamColorizeByClient && ci) {
			beamColor[0] = ci->physR / 255.0f;
			beamColor[1] = ci->physG / 255.0f;
			beamColor[2] = ci->physB / 255.0f;
			beamColor[3] = 1.0f;
		} else {
			Vector4Copy(fx->beamColor, beamColor);
		}
		modulate[0] = (byte)(beamColor[0] * 255);
		modulate[1] = (byte)(beamColor[1] * 255);
		modulate[2] = (byte)(beamColor[2] * 255);
		modulate[3] = (byte)(beamColor[3] * 255);

		// Noire's Mod: render the physgun beam as a single continuous mesh
		// (trap_R_AddPolysToScene).
		// Per-segment billboards produced two artifacts:
		//   1. Each segment's cross-section was oriented by its own local
		//      tangent, so segment joins had tiny geometric gaps/overlaps.
		//   2. Every segment restarted UV V at 0..1, so a length-wise
		//      textured shader showed a stripe at every join.
		// Both are fixed by (a) sharing a per-vertex screen-space side
		// vector between adjacent quads (boundary vertices become bit-
		// identical, gap = 0) and (b) using a single continuous V across
		// the whole beam.

		// 1. Sample curve points. Straight line from muzzle to grab origin,
		//    plus the spring-driven lateral bow in view space.
		for(seg = 0; seg <= PHY_BEAM_SEGMENTS; seg++) {
			float t = (float)seg / (float)PHY_BEAM_SEGMENTS;
			float profile = sin(t * M_PI);
			points[seg][0] = s[0] + (grabOrigin[0] - s[0]) * t;
			points[seg][1] = s[1] + (grabOrigin[1] - s[1]) * t;
			points[seg][2] = s[2] + (grabOrigin[2] - s[2]) * t;
			points[seg][0] += (viewRight[0] * bendRight + viewUp[0] * bendUp) * profile;
			points[seg][1] += (viewRight[1] * bendRight + viewUp[1] * bendUp) * profile;
			points[seg][2] += (viewRight[2] * bendRight + viewUp[2] * bendUp) * profile;
		}

		// 2. Compute per-vertex screen-space side vectors. Central-difference
		//    tangent, then billboard perpendicular = normalize(cross(T, viewDir)).
		//    Adjacent quads share these side vectors, so their boundary verts
		//    match exactly and the seam is mathematically zero.
		for(seg = 0; seg <= PHY_BEAM_SEGMENTS; seg++) {
			vec3_t tangent, viewDir;
			if(seg == 0) {
				VectorSubtract(points[1], points[0], tangent);
			} else if(seg == PHY_BEAM_SEGMENTS) {
				VectorSubtract(points[seg], points[seg - 1], tangent);
			} else {
				VectorSubtract(points[seg + 1], points[seg - 1], tangent);
			}
			VectorSubtract(points[seg], cg.refdef.vieworg, viewDir);
			VectorNormalize(tangent);
			VectorNormalize(viewDir);
			CrossProduct(tangent, viewDir, sides[seg]);
			if(VectorLengthSquared(sides[seg]) < 1e-6f) {
				// Degenerate tangent (view looking straight along the beam
				// axis): fall back to view-right so the quad stays on-screen.
				VectorCopy(viewRight, sides[seg]);
			}
			VectorNormalize(sides[seg]);
			VectorScale(sides[seg], PHY_BEAM_HALFWIDTH, sides[seg]);
		}

		// 3. Emit one quad per segment. V spans [seg/N .. (seg+1)/N] over
		//    the full beam (continuous, no resets). Winding TL->TR->BR->BL
		//    matches cg_particles.c; the beam shader has cull none so front/
		//    back orientation is irrelevant.
		// Noire's Mod: texture asset was authored rotated 90 degrees, so the
		// beam-length axis of the strip maps to texture U and the beam-width
		// axis maps to texture V. (Swap of the previous mapping.)
		for(seg = 0; seg < PHY_BEAM_SEGMENTS; seg++) {
			float v0 = (float)seg / (float)PHY_BEAM_SEGMENTS;
			float v1 = (float)(seg + 1) / (float)PHY_BEAM_SEGMENTS;
			polyVert_t* q = &stripVerts[seg * 4];

			q[0].xyz[0] = points[seg][0] - sides[seg][0];
			q[0].xyz[1] = points[seg][1] - sides[seg][1];
			q[0].xyz[2] = points[seg][2] - sides[seg][2];
			q[0].st[0] = v0;
			q[0].st[1] = 0.0f;

			q[1].xyz[0] = points[seg][0] + sides[seg][0];
			q[1].xyz[1] = points[seg][1] + sides[seg][1];
			q[1].xyz[2] = points[seg][2] + sides[seg][2];
			q[1].st[0] = v0;
			q[1].st[1] = 1.0f;

			q[2].xyz[0] = points[seg + 1][0] + sides[seg + 1][0];
			q[2].xyz[1] = points[seg + 1][1] + sides[seg + 1][1];
			q[2].xyz[2] = points[seg + 1][2] + sides[seg + 1][2];
			q[2].st[0] = v1;
			q[2].st[1] = 1.0f;

			q[3].xyz[0] = points[seg + 1][0] - sides[seg + 1][0];
			q[3].xyz[1] = points[seg + 1][1] - sides[seg + 1][1];
			q[3].xyz[2] = points[seg + 1][2] - sides[seg + 1][2];
			q[3].st[0] = v1;
			q[3].st[1] = 0.0f;

			for(i = 0; i < 4; i++) {
				q[i].modulate[0] = modulate[0];
				q[i].modulate[1] = modulate[1];
				q[i].modulate[2] = modulate[2];
				q[i].modulate[3] = modulate[3];
			}
		}

		// One batched call instead of 32 individual AddRefEntityToScene calls.
		trap_R_AddPolysToScene(fx->beamHandle, 4, stripVerts, PHY_BEAM_SEGMENTS);
		return;
	}

	// Continuous beam weapons (physgun, gravitygun, lightning): one poly quad
	// per frame, no fading. Uniform AddPolysToScene path.
	if(fx->continuousFire) {
		vec4_t localColor;

		if(fx->beamColorizeByClient && ci) {
			localColor[0] = ci->physR / 255.0f;
			localColor[1] = ci->physG / 255.0f;
			localColor[2] = ci->physB / 255.0f;
			localColor[3] = 1.0f;
		} else {
			Vector4Copy(fx->beamColor, localColor);
		}

		CG_EmitBeamPoly(fx->beamHandle, s, e, BEAM_HALFWIDTH, localColor);
		return;
	}

	// Per-shot beams (railgun, toolgun): fading trail via LE_FADE_RGB.
	// CG_AddFadeRGB in cg_localents.c detects beam local entities by
	// (hModel == 0 && customShader != 0 && radius > 0) and emits a poly quad
	// via CG_EmitBeamPoly. Fade is driven by le->color
	// scaled by le->lifeRate over le->endTime.
	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + 400;
	le->lifeRate = 1.0f / (le->endTime - le->startTime);
	le->radius = BEAM_HALFWIDTH;

	VectorCopy(s, re->origin);
	VectorCopy(e, re->oldorigin);
	re->customShader = fx->beamHandle;

	if(fx->beamColorizeByClient && ci) {
		le->color[0] = ci->physR / 255.0f;
		le->color[1] = ci->physG / 255.0f;
		le->color[2] = ci->physB / 255.0f;
		le->color[3] = 1.0f;
	} else {
		Vector4Copy(fx->beamColor, le->color);
	}
}

/*
==========================
CG_MissileTrail
Data-driven particle trail for missiles. Fully table-driven; called from
CG_Missile unconditionally (no-op when trailCount <= 0).
==========================
*/
void CG_MissileTrail(centity_t* ent) {
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

	CG_AddParticles(origin, fx->trailColor, fx->trailCount, fx->trailSpeed, fx->trailFlags, fx->trailGravity, fx->trailShader ? fx->trailShader : "", fx->trailSize, fx->trailLifetime);
}

void CG_RegisterWeapon(int weaponNum) {
	weaponInfo_t* weaponInfo;
	weaponFx_t* fx;
	item_t* weaponItem = NULL;
	char path[MAX_QPATH];
	char fallback[MAX_QPATH];
	vec3_t mins, maxs;
	int i;
	int itemIdx = -1;

	if(weaponNum <= 0 || weaponNum >= WEAPONS_NUM) return;

	fx = &jsd_weaponFx[weaponNum];
	if(fx->registered) return;

	weaponInfo = &cg_weapons[weaponNum];
	memset(weaponInfo, 0, sizeof(*weaponInfo));

	// Mark registered early to break the recursion through CG_RegisterItemVisuals.
	fx->registered = true;

	// Register every data-driven shader / model / sound handle from the table.
	if(fx->hitParticleShader && fx->hitParticleShader[0]) fx->hitParticleHandle = trap_R_RegisterShader(fx->hitParticleShader);
	if(fx->fireParticleShader && fx->fireParticleShader[0]) fx->fireParticleHandle = trap_R_RegisterShader(fx->fireParticleShader);
	if(fx->trailShader && fx->trailShader[0]) fx->trailHandle = trap_R_RegisterShader(fx->trailShader);
	if(fx->tracerParticleShader && fx->tracerParticleShader[0]) fx->tracerHandle = trap_R_RegisterShader(fx->tracerParticleShader);
	if(fx->beamShader && fx->beamShader[0]) fx->beamHandle = trap_R_RegisterShader(fx->beamShader);
	if(fx->muzzleSmokeShader && fx->muzzleSmokeShader[0]) fx->muzzleSmokeHandle = trap_R_RegisterShader(fx->muzzleSmokeShader);
	if(fx->readySound && fx->readySound[0]) fx->readySoundHandle = trap_S_RegisterSound(fx->readySound, false);
	for(i = 0; i < 4; i++) {
		if(fx->fireSound[i] && fx->fireSound[i][0]) fx->fireSoundHandle[i] = trap_S_RegisterSound(fx->fireSound[i], false);
	}
	if(fx->continuousFireSound && fx->continuousFireSound[0]) fx->continuousFireSoundHandle = trap_S_RegisterSound(fx->continuousFireSound, false);
	if(fx->missileFlySound && fx->missileFlySound[0]) fx->missileFlySoundHandle = trap_S_RegisterSound(fx->missileFlySound, false);
	if(fx->missileBounceSound && fx->missileBounceSound[0]) fx->missileBounceSoundHandle = trap_S_RegisterSound(fx->missileBounceSound, false);
	if(fx->missileExplodeSound && fx->missileExplodeSound[0]) fx->missileExplodeSoundHandle = trap_S_RegisterSound(fx->missileExplodeSound, false);
	if(fx->tracerSound && fx->tracerSound[0]) fx->tracerSoundHandle = trap_S_RegisterSound(fx->tracerSound, false);
	if(fx->missileModel && fx->missileModel[0]) fx->missileModelHandle = trap_R_RegisterModel(fx->missileModel);
	if(fx->missileSprite && fx->missileSprite[0]) fx->missileSpriteHandle = trap_R_RegisterShader(fx->missileSprite);
	if(fx->brassModel && fx->brassModel[0]) fx->brassModelHandle = trap_R_RegisterModel(fx->brassModel);
	if(fx->barrelModel && fx->barrelModel[0]) fx->barrelModelHandle = trap_R_RegisterModel(fx->barrelModel);
	if(fx->flashModel && fx->flashModel[0]) fx->flashModelHandle = trap_R_RegisterModel(fx->flashModel);
	if(fx->handsModel && fx->handsModel[0]) fx->handsModelHandle = trap_R_RegisterModel(fx->handsModel);

	// Find the corresponding item.
	for(i = 0; i < jsd_itemCount; i++) {
		if(jsd_item[i].giType == IT_WEAPON && jsd_item[i].giTag == weaponNum) {
			weaponItem = &jsd_item[i];
			weaponInfo->item = weaponItem;
			itemIdx = i;
			break;
		}
	}
	if(!weaponItem) CG_Error("Couldn't find weapon %i", weaponNum);

	CG_RegisterItemVisuals(itemIdx);

	// Cache model + midpoint for the weapon model.
	weaponInfo->weaponModel = trap_R_RegisterModel(weaponItem->model);
	weaponInfo->weaponIcon = cg_items[itemIdx].icon;

	trap_R_ModelBounds(weaponInfo->weaponModel, mins, maxs);
	for(i = 0; i < 3; i++) weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * (maxs[i] - mins[i]);

	// Derive flash/barrel/hands models from the weapon model path if the
	// table didn't specify explicit paths.
	if(!fx->flashModelHandle) {
		COM_StripExtension(weaponItem->model, path, sizeof(path));
		Q_strcat(path, sizeof(path), "_flash.md3");
		fx->flashModelHandle = trap_R_RegisterModel(path);
	}
	if(!fx->barrelModelHandle) {
		COM_StripExtension(weaponItem->model, path, sizeof(path));
		Q_strcat(path, sizeof(path), "_barrel.md3");
		fx->barrelModelHandle = trap_R_RegisterModel(path);
	}
	if(!fx->handsModelHandle) {
		COM_StripExtension(weaponItem->model, path, sizeof(path));
		Q_strcat(path, sizeof(path), "_hand.md3");
		fx->handsModelHandle = trap_R_RegisterModel(path);
	}
	if(!fx->handsModelHandle) {
		// Global default, configurable via cvar (no per-weapon hardcode).
		fallback[0] = '\0';
		trap_Cvar_VariableStringBuffer("cg_defaultHandsModel", fallback, sizeof(fallback));
		if(fallback[0]) fx->handsModelHandle = trap_R_RegisterModel(fallback);
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
	vec3_t forward;
	vec3_t muzzlePoint, endPoint;
	int anim;
	clientInfo_t* ci;
	weaponFx_t* fx;

	if(cent->currentState.weapon <= WP_NONE || cent->currentState.weapon >= WEAPONS_NUM) return;

	fx = &jsd_weaponFx[cent->currentState.weapon];
	if(!fx->isBeam) return;

	// Beams flagged skipLightningBolt are already drawn by another event
	// (e.g. EV_RAILTRAIL for the railgun); rendering here would duplicate them.
	if(fx->skipLightningBolt) return;

	ci = &cgs.clientinfo[cent->currentState.clientNum];

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

	// Noire's Mod: route all beam weapons through CG_RailTrail for a single
	// uniform AddPolysToScene emission path.
	CG_RailTrail(ci, origin, trace.endpos, cent->currentState.weapon);

	if(trace.fraction < 1.0f) {
		vec3_t dir;
		VectorSubtract(trace.endpos, origin, dir);
		VectorNormalize(dir);
		CG_EmitWeaponHitFx(cent->currentState.weapon, trace.endpos, dir);
	}
}

/*
======================
CG_MachinegunSpinAngle
======================
*/
static float CG_MachinegunSpinAngle(centity_t* cent) {
	weaponFx_t* fx = &jsd_weaponFx[cent->currentState.weapon];
	int delta;
	float angle;
	float speed;
	float spinSpeed = fx->barrelSpinSpeed > 0.0f ? fx->barrelSpinSpeed : 0.9f;
	int coastTime = fx->barrelCoastTime > 0 ? fx->barrelCoastTime : 1000;

	delta = cg.time - cent->pe.barrelTime;
	if(cent->pe.barrelSpinning) {
		angle = cent->pe.barrelAngle + delta * spinSpeed;
	} else {
		if(delta > coastTime) {
			delta = coastTime;
		}

		speed = 0.5 * (spinSpeed + (float)(coastTime - delta) / coastTime);
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

	{
		weaponFx_t* fx = &jsd_weaponFx[weaponNum];

		// add the weapon
		memset(&gun, 0, sizeof(gun));
		VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
		gun.renderfx = parent->renderfx;

		if(fx->modelColorSource == 1) {
			gun.shaderRGBA[0] = ci->physR;
			gun.shaderRGBA[1] = ci->physG;
			gun.shaderRGBA[2] = ci->physB;
			gun.shaderRGBA[3] = 255;
		} else if(fx->modelColorSource == 2) {
			gun.shaderRGBA[0] = fx->modelColor[0] * 255;
			gun.shaderRGBA[1] = fx->modelColor[1] * 255;
			gun.shaderRGBA[2] = fx->modelColor[2] * 255;
			gun.shaderRGBA[3] = 255;
		}

		gun.hModel = weapon->weaponModel;
		if(!gun.hModel) {
			return;
		}

		if(!ps) {
			// add weapon ready/looping sound (fully data-driven)
			sfxHandle_t firingSnd = fx->continuousFireSoundHandle;
			sfxHandle_t readySnd = fx->readySoundHandle;
			cent->pe.lightningFiring = false;
			if((cent->currentState.eFlags & EF_FIRING) && firingSnd) {
				trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, firingSnd);
				cent->pe.lightningFiring = true;
			} else if(readySnd) {
				trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, readySnd);
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
		if(fx->barrelModelHandle) {
			memset(&barrel, 0, sizeof(barrel));
			VectorCopy(parent->lightingOrigin, barrel.lightingOrigin);
			barrel.renderfx = parent->renderfx;

			barrel.hModel = fx->barrelModelHandle;
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
		if(fx->continuousFire && (nonPredictedCent->currentState.eFlags & EF_FIRING)) {
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

		flash.hModel = fx->flashModelHandle;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = crandom() * 10;
		AnglesToAxis(angles, flash.axis);

		CG_PositionRotatedEntityOnTag(&flash, &gun, weapon->weaponModel, "tag_flash");
		if(flash.hModel) {
			trap_R_AddRefEntityToScene(&flash);
		}
	}

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
						CG_AddParticles(flash.origin, fx->fireParticleColor, fx->fireParticleCount, fx->fireParticleSpeed, fx->fireParticleFlags, fx->fireParticleGravity, fx->fireParticleShader ? fx->fireParticleShader : "", fx->fireParticleSize, fx->fireParticleLifetime);
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

	hand.hModel = jsd_weaponFx[ps->weapon].handsModelHandle;
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

	{
		weaponFx_t* fx = &jsd_weaponFx[ent->weapon];

		// mark the entity as muzzle flashing
		cent->muzzleFlashTime = cg.time;

		// hold-to-fire suppression (lightning etc.), data-driven per weapon
		if(fx->suppressRepeatFire && cent->pe.lightningFiring) {
			return;
		}

		// play a sound (fully data-driven)
		{
			int numSounds = 0;
			for(c = 0; c < 4; c++) {
				if(fx->fireSoundHandle[c]) numSounds = c + 1;
			}
			if(numSounds > 0) {
				c = rand() % numSounds;
				trap_S_StartSound(NULL, ent->number, CHAN_WEAPON, fx->fireSoundHandle[c]);
			}
		}

		// Fire FX are emitted from CG_AddPlayerWeapon where tag_flash is available.
		cent->pe.fireFxPending = true;

		// brass ejection (data-driven; no-op if brassModel not set)
		if(cg_brassTime.integer > 0) {
			CG_EjectBrass(cent);
		}
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
	if(weapon > WP_NONE && weapon < WEAPONS_NUM && jsd_weaponFx[weapon].explodeOnFleshHit) {
		vec4_t bloodColor = {0.75f, 0.0f, 0.0f, 1.0f};
		vec3_t bloodGravity = {0.0f, 0.0f, -600.0f};

		CG_AddParticles(origin, bloodColor, 32, 300.0f, PF_SMOOTH_FADE | PF_GRAVITY, bloodGravity, "", 5.0f, 0.7f);
		CG_MissileHitWall(weapon, 0, origin, dir, IMPACTSOUND_FLESH);
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

	{
		weaponFx_t* fx = &jsd_weaponFx[weapon];
		contents = CG_PointContents(es->pos.trBase, 0);
		if(!(contents & CONTENTS_WATER) && fx->muzzleSmokeHandle) {
			VectorSet(up, 0, 0, 8);
			CG_SmokePuff(v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, fx->muzzleSmokeHandle);
		}
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
	end = begin + (fx->tracerLength > 0.0f ? fx->tracerLength : 300.0f);
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

		CG_AddParticlesEx(pos, fx->tracerParticleColor, 1, fx->tracerParticleSpeed, fx->tracerParticleFlags, fx->tracerParticleGravity, fx->tracerParticleShader ? fx->tracerParticleShader : "", (fx->tracerParticleLifetime > 0.0f ? fx->tracerParticleLifetime : 0.25f) + crandom() * 0.10f, fx->tracerParticleSize);
	}

	if(fx->tracerSoundHandle) {
		vec3_t midpoint;
		midpoint[0] = (source[0] + dest[0]) * 0.5f;
		midpoint[1] = (source[1] + dest[1]) * 0.5f;
		midpoint[2] = (source[2] + dest[2]) * 0.5f;
		trap_S_StartSound(midpoint, ENTITYNUM_WORLD, CHAN_AUTO, fx->tracerSoundHandle);
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
		CG_MissileHitPlayer(weapon, end, normal, fleshEntityNum);
	else
		CG_MissileHitWall(weapon, 0, end, normal, IMPACTSOUND_DEFAULT);
}
