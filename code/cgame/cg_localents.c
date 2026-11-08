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

// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.

#include "cg_local.h"

#define MAX_LOCAL_ENTITIES 1024
localEntity_t cg_localEntities[MAX_LOCAL_ENTITIES];
localEntity_t cg_activeLocalEntities;  // double linked list
localEntity_t* cg_freeLocalEntities;   // single linked list

/*
===================
CG_InitLocalEntities

This is called at startup and for tournement restarts
===================
*/
void CG_InitLocalEntities(void) {
	int i;

	memset(cg_localEntities, 0, sizeof(cg_localEntities));
	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;
	for(i = 0; i < MAX_LOCAL_ENTITIES - 1; i++) {
		cg_localEntities[i].next = &cg_localEntities[i + 1];
	}
}

/*
==================
CG_FreeLocalEntity
==================
*/
static void CG_FreeLocalEntity(localEntity_t* le) {
	if(!le->prev) {
		CG_Error("CG_FreeLocalEntity: not active");
	}

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/*
===================
CG_AllocLocalEntity

Will always succeed, even if it requires freeing an old active entity
===================
*/
localEntity_t* CG_AllocLocalEntity(void) {
	localEntity_t* le;

	if(!cg_freeLocalEntities) {
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		CG_FreeLocalEntity(cg_activeLocalEntities.prev);
	}

	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;

	memset(le, 0, sizeof(*le));

	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;
	return le;
}

/*
====================================================================================

FRAGMENT PROCESSING

A fragment localentity interacts with the environment in some way (hitting walls),
or generates more localentities along a trail.

====================================================================================
*/

/*
================
CG_ReflectVelocity
================
*/
static void CG_ReflectVelocity(localEntity_t* le, trace_t* trace) {
	vec3_t velocity;
	float dot;
	int hitTime;

	// reflect the velocity on the trace plane
	hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
	BG_EvaluateTrajectoryDelta(&le->pos, hitTime, velocity);
	dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, le->pos.trDelta);

	VectorScale(le->pos.trDelta, le->bounceFactor, le->pos.trDelta);

	VectorCopy(trace->endpos, le->pos.trBase);
	le->pos.trTime = cg.time;

	// check for stop, making sure that even on low FPS systems it doesn't bobble
	if(trace->allsolid || (trace->plane.normal[2] > 0 && (le->pos.trDelta[2] < 40 || le->pos.trDelta[2] < -cg.frametime * le->pos.trDelta[2]))) {
		le->pos.trType = TR_STATIONARY;
	}
}

/*
================
CG_AddFragment
================
*/
static void CG_AddFragment(localEntity_t* le) {
	vec3_t newOrigin;
	trace_t trace;

	if(le->pos.trType == TR_STATIONARY) {
		// sink into the ground if near the removal time
		int t;
		float oldZ;

		t = le->endTime - cg.time;
		if(t < SINK_TIME) {
			// we must use an explicit lighting origin, otherwise the
			// lighting would be lost as soon as the origin went
			// into the ground
			VectorCopy(le->refEntity.origin, le->refEntity.lightingOrigin);
			le->refEntity.renderfx |= RF_LIGHTING_ORIGIN;
			oldZ = le->refEntity.origin[2];
			le->refEntity.origin[2] -= 16 * (1.0 - (float)t / SINK_TIME);
			trap_R_AddRefEntityToScene(&le->refEntity);
			le->refEntity.origin[2] = oldZ;
		} else {
			trap_R_AddRefEntityToScene(&le->refEntity);
		}

		return;
	}

	// calculate new position
	BG_EvaluateTrajectory(&le->pos, cg.time, newOrigin);

	// trace a line from previous position to new position
	CG_Trace(&trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID);
	if(trace.fraction == 1.0) {
		// still in free fall
		VectorCopy(newOrigin, le->refEntity.origin);

		if(le->leFlags & LEF_TUMBLE) {
			vec3_t angles;

			BG_EvaluateTrajectory(&le->angles, cg.time, angles);
			AnglesToAxis(angles, le->refEntity.axis);
		}

		trap_R_AddRefEntityToScene(&le->refEntity);

		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if(CG_PointContents(trace.endpos, 0) & CONTENTS_NODROP) {
		CG_FreeLocalEntity(le);
		return;
	}

	// reflect the velocity on the trace plane
	CG_ReflectVelocity(le, &trace);

	trap_R_AddRefEntityToScene(&le->refEntity);
}

/*
=====================================================================

TRIVIAL LOCAL ENTITIES

These only do simple scaling or modulation before passing to the renderer
=====================================================================
*/

/*
====================
CG_AddFadeRGB
====================
*/
static void CG_AddFadeRGB(localEntity_t* le) {
	refEntity_t* re;
	float c;

	re = &le->refEntity;

	c = (le->endTime - cg.time) * le->lifeRate;
	c *= 0xff;

	// Noire's Mod: beam local entities (no model, customShader, radius set)
	// render as a single poly quad via AddPolysToScene.
	// CG_RailTrail marks per-shot beams this way so their trailing fade
	// survives across frames while still using the uniform poly codepath.
	if(!re->hModel && re->customShader && le->radius > 0.0f) {
		vec4_t color;
		color[0] = le->color[0];
		color[1] = le->color[1];
		color[2] = le->color[2];
		color[3] = le->color[3] * (c / 255.0f);
		CG_EmitBeamPoly(re->customShader, re->origin, re->oldorigin, le->radius, color);
		return;
	}

	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;
	re->shaderRGBA[3] = le->color[3] * c;

	trap_R_AddRefEntityToScene(re);
}

/*
================
CG_AddExplosion
================
*/
static void CG_AddExplosion(localEntity_t* ex) {
	refEntity_t* ent;

	ent = &ex->refEntity;

	// add the entity
	trap_R_AddRefEntityToScene(ent);

	// add the dlight
	if(ex->light) {
		float light;

		light = (float)(cg.time - ex->startTime) / (ex->endTime - ex->startTime);
		if(light < 0.5) {
			light = 1.0;
		} else {
			light = 1.0 - (light - 0.5) * 2;
		}
		light = ex->light * light;
		trap_R_AddLightToScene(ent->origin, light, 1.00f, ex->lightColor[0], ex->lightColor[1], ex->lightColor[2], 0);
	}
}

/*
===================
CG_AddLocalEntities

===================
*/
void CG_AddLocalEntities(void) {
	localEntity_t *le, *next;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	le = cg_activeLocalEntities.prev;
	for(; le != &cg_activeLocalEntities; le = next) {
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if(cg.time >= le->endTime) {
			CG_FreeLocalEntity(le);
			continue;
		}
		switch(le->leType) {
			default: CG_Error("Bad leType: %i", le->leType); break;

			case LE_EXPLOSION: CG_AddExplosion(le); break;

			case LE_FRAGMENT:  // gibs and brass
				CG_AddFragment(le);
				break;

			case LE_FADE_RGB:  // teleporters, railtrails
				CG_AddFadeRGB(le);
				break;
		}
	}
}
