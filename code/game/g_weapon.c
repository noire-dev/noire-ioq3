// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2

#include "../qcommon/vm_javascript.h"

static vec3_t forward, right, up;
static vec3_t muzzle;

// General funcs
static void G_BounceProjectile(vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout) {
	vec3_t v, newv;
	float dot;

	VectorSubtract(impact, start, v);
	dot = DotProduct(v, dir);
	VectorMA(v, -2 * dot, dir, newv);

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}

void CalcMuzzlePoint(gentity_t* ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint) {
	VectorCopy(ent->s.pos.trBase, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA(muzzlePoint, 14, forward, muzzlePoint);
	SnapVector(muzzlePoint);
}

static void CalcMuzzlePointOrigin(gentity_t* ent, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint) {
	VectorCopy(ent->s.pos.trBase, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA(muzzlePoint, 14, forward, muzzlePoint);
	SnapVector(muzzlePoint);
}

static void G_BounceMissile(gentity_t* ent, trace_t* trace) {
	vec3_t velocity;
	float dot;
	int hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + (level.time - level.previousTime) * trace->fraction;
	BG_EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity);
	dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta);

	VectorScale(ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta);
	// check for stop
	if(trace->plane.normal[2] > 0.2 && VectorLength(ent->s.pos.trDelta) < 40) {
		G_SetOrigin(ent, trace->endpos);
		return;
	}

	VectorAdd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;
}

void G_ExplodeMissile(gentity_t* ent) {
	vec3_t dir;
	vec3_t origin;

	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
	SnapVector(origin);
	G_SetOrigin(ent, origin);

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(dir));

	ent->freeAfterEvent = true;

	// splash damage
	if(ent->splashDamage) G_RadiusDamage(ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent->s.weapon);

	trap_LinkEntity(ent);
}

// Melee type
bool Melee_Fire(gentity_t* ent, int weapon) {
	trace_t tr;
	vec3_t end;
	gentity_t* tent;
	gentity_t* traceEnt;

	// set aiming directions
	AngleVectors(ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePoint(ent, forward, right, up, muzzle);
	VectorMA(muzzle, jsd_weapon[weapon].range, forward, end);
	trap_Trace(&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);

	if(tr.surfaceFlags & SURF_NOIMPACT) return false;

	traceEnt = &g_entities[tr.entityNum];

	// send blood impact
	if(traceEnt->takedamage && traceEnt->client) {
		tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte(tr.plane.normal);
		tent->s.weapon = ent->s.weapon;
	}

	if(!traceEnt->takedamage) return false;

	G_Damage(traceEnt, ent, ent, forward, tr.endpos, jsd_weapon[weapon].damage, 0, WP_GAUNTLET);

	return true;
}

// Bullet type
static void Bullet_Fire(gentity_t* ent, int weapon) {
	trace_t tr;
	vec3_t end;
	float r, u;
	gentity_t *tent, *traceEnt;
	int passent;

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * jsd_weapon[weapon].spread * 16;
	r = cos(r) * crandom() * jsd_weapon[weapon].spread * 16;

	VectorMA(muzzle, jsd_weapon[weapon].range, forward, end);
	VectorMA(end, r, right, end);
	VectorMA(end, u, up, end);

	passent = ent->s.number;
	trap_Trace(&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);

	if(tr.surfaceFlags & SURF_NOIMPACT) return;

	traceEnt = &g_entities[tr.entityNum];

	// send bullet impact
	if(traceEnt->takedamage && traceEnt->client) {
		tent = G_TempEntity(tr.endpos, EV_BULLET_HIT_FLESH);
		tent->s.eventParm = traceEnt->s.number;
		tent->s.clientNum = ent->s.clientNum;
	} else {
		tent = G_TempEntity(tr.endpos, EV_BULLET_HIT_WALL);
		tent->s.eventParm = DirToByte(tr.plane.normal);
		tent->s.clientNum = ent->s.clientNum;
	}
	tent->s.otherEntityNum = ent->s.number;

	if(traceEnt->takedamage) G_Damage(traceEnt, ent, ent, forward, tr.endpos, jsd_weapon[weapon].damage, 0, weapon);
}

// Shotgun type
static void ShotgunPellet(vec3_t start, vec3_t end, gentity_t* ent, int weapon) {
	trace_t tr;
	int passent;
	gentity_t* traceEnt;
	vec3_t tr_start, tr_end;

	passent = ent->s.number;
	VectorCopy(start, tr_start);
	VectorCopy(end, tr_end);
	trap_Trace(&tr, tr_start, NULL, NULL, tr_end, passent, MASK_SHOT);
	traceEnt = &g_entities[tr.entityNum];

	if(tr.surfaceFlags & SURF_NOIMPACT) return;

	if(traceEnt->takedamage) G_Damage(traceEnt, ent, ent, forward, tr.endpos, jsd_weapon[weapon].damage, 0, weapon);
}

static void ShotgunPattern(vec3_t origin, vec3_t origin2, int seed, gentity_t* ent, int weapon) {
	int i;
	float r, u;
	vec3_t end, forward, right, up;

	VectorNormalize2(origin2, forward);
	PerpendicularVector(right, forward);
	CrossProduct(forward, right, up);

	for(i = 0; i < jsd_weapon[weapon].count; i++) {
		r = Q_crandom(&seed) * jsd_weapon[weapon].spread * 16;
		u = Q_crandom(&seed) * jsd_weapon[weapon].spread * 16;
		VectorMA(origin, jsd_weapon[weapon].range * 16, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);

		ShotgunPellet(origin, end, ent, weapon);
	}
}

static void Shotgun_Fire(gentity_t* ent, int weapon) {
	gentity_t* tent;

	tent = G_TempEntity(muzzle, EV_SHOTGUN);
	tent->s.weapon = weapon;
	VectorScale(forward, jsd_weapon[weapon].range, tent->s.origin2);
	SnapVector(tent->s.origin2);
	tent->s.eventParm = rand() % 255;
	tent->s.otherEntityNum = ent->s.number;

	ShotgunPattern(tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent, weapon);
}

// Railgun type
#define MAX_RAIL_HITS 4
static void Railgun_Fire(gentity_t* ent, int weapon) {
	vec3_t end, impactpoint, bouncedir;
	trace_t trace;
	gentity_t *tent, *traceEnt, *unlinkedEntities[MAX_RAIL_HITS];
	int i, hits, unlinked;

	VectorMA(muzzle, jsd_weapon[weapon].range, forward, end);

	unlinked = 0;
	hits = 0;
	do {
		trap_Trace(&trace, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);
		if(trace.entityNum >= ENTITYNUM_MAX_NORMAL) break;

		traceEnt = &g_entities[trace.entityNum];
		if(traceEnt->takedamage) G_Damage(traceEnt, ent, ent, forward, trace.endpos, jsd_weapon[weapon].damage, 0, weapon);
		if(trace.contents & CONTENTS_SOLID) break;

		trap_UnlinkEntity(traceEnt);
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	} while(unlinked < MAX_RAIL_HITS);

	for(i = 0; i < unlinked; i++) trap_LinkEntity(unlinkedEntities[i]);

	tent = G_TempEntity(trace.endpos, EV_RAILTRAIL);
	tent->s.clientNum = ent->s.clientNum;

	VectorCopy(muzzle, tent->s.origin2);

	VectorMA(tent->s.origin2, 4, right, tent->s.origin2);
	VectorMA(tent->s.origin2, -1, up, tent->s.origin2);

	if(trace.surfaceFlags & SURF_NOIMPACT)
		tent->s.eventParm = 255;
	else
		tent->s.eventParm = DirToByte(trace.plane.normal);

	tent->s.clientNum = ent->s.clientNum;
}

// Lightning type
static void Lightning_Fire(gentity_t* ent, int weapon) {
	trace_t tr;
	vec3_t end, impactpoint, bouncedir;
	gentity_t *traceEnt, *tent;

	VectorMA(muzzle, jsd_weapon[weapon].range, forward, end);
	trap_Trace(&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);

	if(tr.entityNum == ENTITYNUM_NONE) return;

	traceEnt = &g_entities[tr.entityNum];

	if(traceEnt->takedamage) G_Damage(traceEnt, ent, ent, forward, tr.endpos, jsd_weapon[weapon].damage, 0, weapon);

	if(traceEnt->takedamage && traceEnt->client) {
		tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte(tr.plane.normal);
		tent->s.weapon = ent->s.weapon;
	} else if(!(tr.surfaceFlags & SURF_NOIMPACT)) {
		tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
		tent->s.eventParm = DirToByte(tr.plane.normal);
	}
}

// Missile type
static void G_MissileImpact(gentity_t* ent, trace_t* trace) {
	gentity_t* other;
	bool hitClient = false;
	vec3_t forward, impactpoint, bouncedir;
	int eFlags;
	other = &g_entities[trace->entityNum];

	// check for bounce
	if(!other->takedamage && (ent->s.eFlags & (EF_BOUNCE))) {
		G_BounceMissile(ent, trace);
		if(ent->s.weapon == WP_GRENADE_LAUNCHER) {
			G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
		}
		return;
	}

	// impact damage
	if(other->takedamage) {
		// FIXME: wrong damage direction?
		if(ent->damage) {
			vec3_t velocity;

			BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity);
			if(VectorLength(velocity) == 0) {
				velocity[2] = 1;  // stepped on a grenade
			}
			G_Damage(other, ent, &g_entities[ent->r.ownerNum], velocity, ent->s.origin, ent->damage, 0, ent->s.weapon);
		}
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if(other->takedamage && other->client) {
		G_AddEvent(ent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
		ent->s.otherEntityNum = other->s.number;
	} else if(trace->surfaceFlags & SURF_METALSTEPS) {
		G_AddEvent(ent, EV_MISSILE_MISS_METAL, DirToByte(trace->plane.normal));
	} else {
		G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(trace->plane.normal));
	}

	ent->freeAfterEvent = true;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	G_SetOrigin(ent, trace->endpos);

	// splash damage (doesn't apply to person directly hit)
	if(ent->splashDamage) G_RadiusDamage(trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, other, ent->s.weapon);

	trap_LinkEntity(ent);
}

void G_RunMissile(gentity_t* ent) {
	vec3_t origin;
	trace_t tr;
	int passent;

	// get current position
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
	passent = ent->r.ownerNum;

	// trace a line from the previous position to the current position
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask);

	if(tr.startsolid || tr.allsolid) {
		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask);
		tr.fraction = 0;
	} else {
		VectorCopy(tr.endpos, ent->r.currentOrigin);
	}

	trap_LinkEntity(ent);

	if(tr.fraction != 1) {
		G_MissileImpact(ent, &tr);
		if(ent->s.eType != ET_MISSILE) return;  // exploded
	}

	// check think function after bouncing
	G_RunThink(ent);
}

gentity_t* fire_missile(gentity_t* self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up, int weapon) {
	gentity_t* bolt;
	vec3_t dir, end;
	float r, u, scale;

	VectorNormalize(dir);

	// create missile
	bolt = G_Spawn();

	// classname
	bolt->classname = jsd_weapon[weapon].classname;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = jsd_weapon[weapon].mEffect;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->clipmask = MASK_SHOT;

	// think
	bolt->nextthink = level.time + jsd_weapon[weapon].timeout;
	bolt->think = G_ExplodeMissile;

	// damage
	bolt->damage = jsd_weapon[weapon].damage;
	bolt->splashDamage = jsd_weapon[weapon].splashDamage;
	bolt->splashRadius = jsd_weapon[weapon].splashRadius;

	// physics
	if(jsd_weapon[weapon].bounce) {
		bolt->s.eFlags = EF_BOUNCE;
		bolt->physicsBounce = jsd_weapon[weapon].bounceModifier;
	}
	if(jsd_weapon[weapon].gravity) {
		bolt->s.pos.trType = TR_GRAVITY;
	} else {
		bolt->s.pos.trType = TR_LINEAR;
	}
	bolt->s.pos.trTime = level.time - 50;
	VectorCopy(start, bolt->s.pos.trBase);

	// speed
	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * jsd_weapon[weapon].spread * 16;
	r = cos(r) * crandom() * jsd_weapon[weapon].spread * 16;
	VectorMA(start, 8192 * 16, forward, end);
	VectorMA(end, r, right, end);
	VectorMA(end, u, up, end);
	VectorSubtract(end, start, dir);
	VectorNormalize(dir);

	scale = jsd_weapon[weapon].speed + (random() * jsd_weapon[weapon].speedRandom);
	VectorScale(dir, scale, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);
	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}

static void Missile_Fire(gentity_t* ent, int weapon) {
	gentity_t* m;
	int count;

	if(weapon == WP_GRENADE_LAUNCHER) {  // extra vertical velocity
		forward[2] += 0.2f;
		VectorNormalize(forward);
	}
	for(count = 0; count < jsd_weapon[weapon].count; count++) m = fire_missile(ent, muzzle, forward, right, up, weapon);
}

// Fire Weapon
void FireWeapon(gentity_t* ent) {
	// set aiming directions
	AngleVectors(ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePointOrigin(ent, ent->client->oldOrigin, forward, right, up, muzzle);

	switch(jsd_weapon[ent->s.weapon].wType) {
		case WT_BULLET: Bullet_Fire(ent, ent->s.weapon); break;
		case WT_SHOTGUN: Shotgun_Fire(ent, ent->s.weapon); break;
		case WT_LIGHTNING: Lightning_Fire(ent, ent->s.weapon); break;
		case WT_RAILGUN: Railgun_Fire(ent, ent->s.weapon); break;
		case WT_EMPTY: break;
		case WT_TOOLGUN: break;
		case WT_MISSILE: Missile_Fire(ent, ent->s.weapon); break;
		default: break;
	}
}
