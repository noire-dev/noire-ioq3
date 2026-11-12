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
#include "tr_local.h"

/*
================
R_CullSurface

Tries to cull surfaces before they are lighted or
added to the sorting list.
================
*/
static bool R_CullSurface(msurface_t* surf) {
	if(surf->cullinfo.type == CULLINFO_NONE) {
		return false;
	}

	if(surf->cullinfo.type & CULLINFO_PLANE) {
		// Only true for SF_FACE, so treat like its own function
		float d;
		cullType_t ct;

		ct = surf->shader->cullType;

		if(ct == CT_TWO_SIDED) {
			return false;
		}

		// shadowmaps draw back surfaces
		if(tr.viewParms.flags & (VPF_SHADOWMAP | VPF_DEPTHSHADOW)) {
			if(ct == CT_FRONT_SIDED) {
				ct = CT_BACK_SIDED;
			} else {
				ct = CT_FRONT_SIDED;
			}
		}

		// do proper cull for orthographic projection
		if(tr.viewParms.flags & VPF_ORTHOGRAPHIC) {
			d = DotProduct(tr.viewParms.or.axis[0], surf->cullinfo.plane.normal);
			if(ct == CT_FRONT_SIDED) {
				if(d > 0) return true;
			} else {
				if(d < 0) return true;
			}
			return false;
		}

		d = DotProduct(tr.or.viewOrigin, surf->cullinfo.plane.normal);

		// don't cull exactly on the plane, because there are levels of rounding
		// through the BSP, ICD, and hardware that may cause pixel gaps if an
		// epsilon isn't allowed here
		if(ct == CT_FRONT_SIDED) {
			if(d < surf->cullinfo.plane.dist - 8) {
				return true;
			}
		} else {
			if(d > surf->cullinfo.plane.dist + 8) {
				return true;
			}
		}

		return false;
	}

	/*
	 * R_AddWorldSurface calls R_FrustumCullSurface immediately before this
	 * function. That function already checks the box when present, or the
	 * sphere when it is the only bounds type. Only check a sphere here when
	 * both bounds are present (the frustum path checks the box first).
	 */
	if((surf->cullinfo.type & (CULLINFO_BOX | CULLINFO_SPHERE)) == (CULLINFO_BOX | CULLINFO_SPHERE)) {
		int sphereCull;

		if(tr.currentEntityNum != REFENTITYNUM_WORLD) {
			sphereCull = R_CullLocalPointAndRadius(surf->cullinfo.localOrigin, surf->cullinfo.radius);
		} else {
			sphereCull = R_CullPointAndRadius(surf->cullinfo.localOrigin, surf->cullinfo.radius);
		}

		if(sphereCull == CULL_OUT) {
			return true;
		}
	}

	return false;
}

static bool R_DlightCullSurface(msurface_t* surf, dlightMask_t* dlightBits) {
	int i;
	dlight_t* dl;

	if(!DlightMask_Any(dlightBits)) {
		if(!(tr.viewParms.flags & (VPF_SHADOWMAP | VPF_DEPTHSHADOW))) {
			switch(*surf->data) {
				case SF_FACE:
				case SF_GRID:
				case SF_TRIANGLES: DlightMask_Clear(&((srfBspSurface_t*)surf->data)->dlightBits); break;
				default: break;
			}
		}
		tr.pc.c_dlightSurfacesCulled++;
		return true;
	}

	for(i = 0; i < tr.refdef.num_dlights; i++) {
		if(DlightMask_Test(dlightBits, i) && !(tr.refdef.dlights[i].flags & REF_SURFACE_DLIGHT)) {
			DlightMask_ClearBit(dlightBits, i);
		}
	}

	if(r_dynamicLightCull->integer && tr.currentEntityNum == REFENTITYNUM_WORLD) {
		if(surf->cullinfo.type & (CULLINFO_SPHERE | CULLINFO_BOX)) {
			// sqrt нужен один раз на поверхность (не на источник!)
			float surfRadius = sqrtf(surf->cullinfo.radiusSquared);

			for(i = 0; i < tr.refdef.num_dlights; i++) {
				vec3_t toLight;
				float cullDist, distSq;

				if(!DlightMask_Test(dlightBits, i)) continue;
				dl = &tr.refdef.dlights[i];

				cullDist = dl->radius * 1.5f + surfRadius;
				cullDist *= cullDist;

				VectorSubtract(dl->origin, surf->cullinfo.center, toLight);
				distSq = VectorLengthSquared(toLight);

				if(distSq > cullDist) {
					DlightMask_ClearBit(dlightBits, i);
				}
			}
		}
	}

	switch(*surf->data) {
		case SF_FACE:
		case SF_GRID:
		case SF_TRIANGLES:
			if(!(tr.viewParms.flags & (VPF_SHADOWMAP | VPF_DEPTHSHADOW))) {
				DlightMask_Copy(&((srfBspSurface_t*)surf->data)->dlightBits, dlightBits);
			}
			break;
		default: DlightMask_Clear(dlightBits); break;
	}

	if(DlightMask_Any(dlightBits)) {
		tr.pc.c_dlightSurfaces++;
		return false;
	}

	tr.pc.c_dlightSurfacesCulled++;
	return true;
}

static void R_FrustumCullSurface(msurface_t* surf) {
	int frustumCull;

	if(surf->cullinfo.type & CULLINFO_BOX) {
		if(tr.currentEntityNum != REFENTITYNUM_WORLD)
			frustumCull = R_CullLocalBox(surf->cullinfo.bounds);
		else
			frustumCull = R_CullBox(surf->cullinfo.bounds);

		if(frustumCull == CULL_OUT) {
			surf->doCull = true;
			return;
		} else {
			surf->doCull = false;
			return;
		}
	} else if(surf->cullinfo.type & CULLINFO_SPHERE) {
		if(tr.currentEntityNum != REFENTITYNUM_WORLD)
			frustumCull = R_CullLocalPointAndRadius(surf->cullinfo.localOrigin, surf->cullinfo.radius);
		else
			frustumCull = R_CullPointAndRadius(surf->cullinfo.localOrigin, surf->cullinfo.radius);

		if(frustumCull == CULL_OUT) {
			surf->doCull = true;
			return;
		} else {
			surf->doCull = false;
			return;
		}
	}
}

static void R_FogCullSurface(msurface_t* surf) {
	cullinfo_t* ci = &surf->cullinfo;

	if(surf->doCull) {
		ci->fogDistance = 0.0f;
		return;
	}

	if(ci->type & CULLINFO_BOX) {
		const float fogEndSq = r_fogEnd->value * r_fogEnd->value;
		vec3_t toCam;
		float distSq, boundSq, threshold;

		VectorSubtract(ci->center, backEnd.viewParms.or.origin, toCam);
		distSq = VectorLengthSquared(toCam);
		boundSq = ci->radiusSquared;

		threshold = sqrtf(fogEndSq) + sqrtf(boundSq);
		surf->doFogCull = distSq > threshold * threshold;

		float dist = sqrtf(distSq) - sqrtf(boundSq);
		ci->fogDistance = dist > 0.0f ? dist : 0.0f;
	} else {
		ci->fogDistance = 0.0f;
		surf->doFogCull = false;
	}
}

static void R_AddWorldSurface(msurface_t* surf, const dlightMask_t* sourceDlightBits) {
	dlightMask_t dlightBits;
	int fogIndex;

	if(sourceDlightBits)
		DlightMask_Copy(&dlightBits, sourceDlightBits);
	else
		DlightMask_Clear(&dlightBits);

	// SKYBOX geometry is not a dynamic-light receiver or shadow caster by default.
	// Keep it in the normal scene so the skybox still renders, but remove its
	// dynamic-light bits and omit it from dlight shadow cubemap views.
	if((surf->surfaceFlags & SURF_SKYBOX) && !r_dynamicLightSkybox->integer) {
		DlightMask_Clear(&dlightBits);
		if(tr.viewParms.flags & VPF_DLIGHTSHADOW) return;
	}

	if(surf->surfaceFlags & SURF_SKYBOX) {
		bool allowShadowSkybox = (tr.viewParms.flags & VPF_DLIGHTSHADOW) ? r_dynamicLightSkybox->integer : r_shadowSkybox->integer;
		if((tr.viewParms.flags & VPF_NOCUBEMAPS && !r_reflectionsSkybox->integer) || (tr.viewParms.flags & VPF_DEPTHSHADOW && !allowShadowSkybox) || !r_skybox->integer) return;
	}
	R_FrustumCullSurface(surf);
	if(r_fogDensity->value >= 0.95 && (tr.frameCount & 15) == 0) R_FogCullSurface(surf);
	if(surf->doCull || surf->doFogCull) return;

	if(!(tr.viewParms.targetFbo == tr.dlightShadowFbo && (tr.viewParms.flags & VPF_DEPTHSHADOW))) {
		if(R_CullSurface(surf)) return;
	}

	bool noDlights = R_DlightCullSurface(surf, &dlightBits);
	fogIndex = surf->hasFog ? surf->fogIndex : 0;
	R_AddDrawSurf(surf->data, surf->shader, fogIndex, !noDlights, surf->useCubemap, surf->cubemapOrigin, surf->cubemapBound);
}

/*
=============================================================

    BRUSH MODELS

=============================================================
*/

/*
=================
R_AddBrushModelSurfaces
=================
*/
void R_AddBrushModelSurfaces(trRefEntity_t* ent) {
	bmodel_t* bmodel;
	int clip;
	model_t* pModel;
	int i;

	pModel = R_GetModelByHandle(ent->e.hModel);

	bmodel = pModel->bmodel;

	clip = R_CullLocalBox(bmodel->bounds);
	if(clip == CULL_OUT) {
		return;
	}

	R_SetupEntityLighting(&tr.refdef, ent);
	R_DlightBmodel(bmodel);

	for(i = 0; i < bmodel->numSurfaces; i++) {
		int surf = bmodel->firstSurface + i;

		if(tr.world->surfacesViewCount[surf] != tr.viewCount) {
			tr.world->surfacesViewCount[surf] = tr.viewCount;
			R_AddWorldSurface(tr.world->surfaces + surf, &tr.currentEntity->needDlights);
		}
	}
}

/*
=============================================================

    WORLD MODEL

=============================================================
*/

static bool R_CullDlightShadowNode(const mnode_t* node) {
	int index;
	dlight_t* dl;

	if(!(tr.viewParms.flags & VPF_DLIGHTSHADOW) || !r_dynamicLightCull->integer) return false;

	index = tr.viewParms.targetFboCubemapIndex;

	if(index < 0 || index >= tr.refdef.num_dlights) return true;

	dl = &tr.refdef.dlights[index];

	if(node->surfMaxs[0] <= dl->origin[0] - dl->radius || node->surfMins[0] >= dl->origin[0] + dl->radius || node->surfMaxs[1] <= dl->origin[1] - dl->radius || node->surfMins[1] >= dl->origin[1] + dl->radius || node->surfMaxs[2] <= dl->origin[2] - dl->radius || node->surfMins[2] >= dl->origin[2] + dl->radius) {
		return true;
	}

	return false;
}

static void R_RecursiveWorldNode_Direct(const dlightMask_t* sourceDlightBits) {
	int s;
	dlightMask_t dlightBits;
	msurface_t* surf;

	DlightMask_Copy(&dlightBits, sourceDlightBits);

	for(s = 0; s < tr.world->numsurfaces; s++) {
		surf = &tr.world->surfaces[s];

		if(tr.world->surfacesViewCount[s] == tr.viewCount) continue;
		tr.world->surfacesViewCount[s] = tr.viewCount;

		if(tr.viewParms.flags & VPF_DLIGHTSHADOW) {
			R_AddWorldSurface(surf, NULL);
		} else {
			DlightMask_Copy(&tr.world->surfacesDlightBits[s], &dlightBits);
		}
	}

	tr.pc.c_leafs = tr.world->numsurfaces;
}

static void R_RecursiveWorldNode(mnode_t* node, uint32_t planeBits, const dlightMask_t* sourceDlightBits) {
	int i, r;
	dlight_t* dl;
	dlightMask_t dlightBits;
	DlightMask_Copy(&dlightBits, sourceDlightBits);

	do {
		/*
		 * Dlight shadow uses light volume + frustum.
		 * Do not use PVS here.
		 */
		if(tr.viewParms.flags & VPF_DLIGHTSHADOW) {
			if(R_CullDlightShadowNode(node)) return;
		} else {
			if(node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex]) return;
		}

		/*
		 * Normal view frustum culling.
		 */
		if(planeBits & 1) {
			r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[0]);

			if(r == 2) return;

			if(r == 1) planeBits &= ~1;
		}

		if(planeBits & 2) {
			r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[1]);

			if(r == 2) return;

			if(r == 1) planeBits &= ~2;
		}

		if(planeBits & 4) {
			r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[2]);

			if(r == 2) return;

			if(r == 1) planeBits &= ~4;
		}

		if(planeBits & 8) {
			r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[3]);

			if(r == 2) return;

			if(r == 1) planeBits &= ~8;
		}

		if(planeBits & 16) {
			r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[4]);

			if(r == 2) return;

			if(r == 1) planeBits &= ~16;
		}

		/*
		 * Normal dynamic light culling.
		 * Not used by dlight shadow views.
		 */
		if(r_dynamicLightCull->integer && DlightMask_Any(&dlightBits)) {
			for(i = 0; i < tr.refdef.num_dlights; i++) {
				if(DlightMask_Test(&dlightBits, i)) {
					vec3_t center, halfSize, toCenter;
					float boundRadius;
					dl = &tr.refdef.dlights[i];

					if(node->surfMins[0] >= (dl->origin[0] + dl->radius) || node->surfMaxs[0] <= (dl->origin[0] - dl->radius) || node->surfMins[1] >= (dl->origin[1] + dl->radius) || node->surfMaxs[1] <= (dl->origin[1] - dl->radius) || node->surfMins[2] >= (dl->origin[2] + dl->radius) || node->surfMaxs[2] <= (dl->origin[2] - dl->radius)) {
						DlightMask_ClearBit(&dlightBits, i);
						continue;
					}

					// The node bounds cover all surfaces below this node. For a
					// projector, conservatively reject the subtree when its bounding
					// sphere cannot intersect the light cone.
					if(dl->type == DLIGHT_PROJECTOR) {
						float forwardDistance, radialSq, coneRadius;
						VectorSet(center, (node->surfMins[0] + node->surfMaxs[0]) * 0.5f, (node->surfMins[1] + node->surfMaxs[1]) * 0.5f, (node->surfMins[2] + node->surfMaxs[2]) * 0.5f);
						VectorSet(halfSize, (node->surfMaxs[0] - node->surfMins[0]) * 0.5f, (node->surfMaxs[1] - node->surfMins[1]) * 0.5f, (node->surfMaxs[2] - node->surfMins[2]) * 0.5f);
						boundRadius = VectorLength(halfSize);
						VectorSubtract(center, dl->origin, toCenter);
						forwardDistance = DotProduct(toCenter, dl->forward);
						if(forwardDistance + boundRadius <= 0.0f || forwardDistance - boundRadius >= dl->radius) {
							DlightMask_ClearBit(&dlightBits, i);
							continue;
						}
						radialSq = VectorLengthSquared(toCenter) - forwardDistance * forwardDistance;
						coneRadius = Com_Clamp(0.0f, dl->radius, forwardDistance) * dl->coneTangent + boundRadius;
						if(radialSq > coneRadius * coneRadius) DlightMask_ClearBit(&dlightBits, i);
					}
				}
			}
		}

		if(node->isLeaf) break;

		R_RecursiveWorldNode(node->children[0], planeBits, &dlightBits);

		node = node->children[1];

	} while(1);

	{
		int c;
		int surf, *view;

		tr.pc.c_leafs++;

		if(node->mins[0] < tr.viewParms.visBounds[0][0]) tr.viewParms.visBounds[0][0] = node->mins[0];

		if(node->mins[1] < tr.viewParms.visBounds[0][1]) tr.viewParms.visBounds[0][1] = node->mins[1];

		if(node->mins[2] < tr.viewParms.visBounds[0][2]) tr.viewParms.visBounds[0][2] = node->mins[2];

		if(node->maxs[0] > tr.viewParms.visBounds[1][0]) tr.viewParms.visBounds[1][0] = node->maxs[0];

		if(node->maxs[1] > tr.viewParms.visBounds[1][1]) tr.viewParms.visBounds[1][1] = node->maxs[1];

		if(node->maxs[2] > tr.viewParms.visBounds[1][2]) tr.viewParms.visBounds[1][2] = node->maxs[2];

		view = tr.world->marksurfaces + node->firstmarksurface;
		c = node->nummarksurfaces;

		while(c--) {
			surf = *view;

			if(tr.world->surfacesViewCount[surf] != tr.viewCount) {
				tr.world->surfacesViewCount[surf] = tr.viewCount;

				if(tr.viewParms.flags & VPF_DLIGHTSHADOW) {
					msurface_t* msurf;

					msurf = &tr.world->surfaces[surf];

					R_AddWorldSurface(msurf, NULL);
				} else {
					DlightMask_Copy(&tr.world->surfacesDlightBits[surf], &dlightBits);
				}
			} else if(!(tr.viewParms.flags & VPF_DLIGHTSHADOW)) {
				DlightMask_Or(&tr.world->surfacesDlightBits[surf], &dlightBits);
			}

			view++;
		}
	}
}

/*
===============
R_PointInLeaf
===============
*/
static mnode_t* R_PointInLeaf(const vec3_t p) {
	mnode_t* node;
	float d;
	cplane_t* plane;

	if(!tr.world) {
		ri.Error(ERR_DROP, "R_PointInLeaf: bad model");
	}

	node = tr.world->nodes;
	while(1) {
		if(node->contents != -1) {
			break;
		}
		plane = node->plane;
		d = DotProduct(p, plane->normal) - plane->dist;
		if(d > 0) {
			node = node->children[0];
		} else {
			node = node->children[1];
		}
	}

	return node;
}

/*
==============
R_ClusterPVS
==============
*/
static const byte* R_ClusterPVS(int cluster) {
	if(!tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters) {
		return NULL;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

/*
=================
R_inPVS
=================
*/
bool R_inPVS(const vec3_t p1, const vec3_t p2) {
	mnode_t* leaf;
	byte* vis;

	leaf = R_PointInLeaf(p1);
	vis = ri.CM_ClusterPVS(leaf->cluster);  // why not R_ClusterPVS ??
	leaf = R_PointInLeaf(p2);

	if(!(vis[leaf->cluster >> 3] & (1 << (leaf->cluster & 7)))) {
		return false;
	}
	return true;
}

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
static void R_MarkLeaves(void) {
	const byte* vis;
	mnode_t *leaf, *parent;
	int i;
	int cluster;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if(r_lockpvs->integer) {
		return;
	}

	// current viewcluster
	leaf = R_PointInLeaf(tr.viewParms.pvsOrigin);
	cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	for(i = 0; i < MAX_VISCOUNTS; i++) {
		// if the areamask was modified, invalidate all visclusters
		// this caused doors to open into undrawn areas
		if(tr.refdef.areamaskModified) {
			tr.visClusters[i] = -2;
		} else if(tr.visClusters[i] == cluster) {
			tr.visIndex = i;
			return;
		}
	}

	tr.visIndex = (tr.visIndex + 1) % MAX_VISCOUNTS;
	tr.visCounts[tr.visIndex]++;
	tr.visClusters[tr.visIndex] = cluster;

	vis = R_ClusterPVS(tr.visClusters[tr.visIndex]);

	for(i = 0, leaf = tr.world->nodes; i < tr.world->numnodes; i++, leaf++) {
		cluster = leaf->cluster;
		if(cluster < 0 || cluster >= tr.world->numClusters) {
			continue;
		}

		// check general pvs
		if(vis && !(vis[cluster >> 3] & (1 << (cluster & 7)))) {
			continue;
		}

		// check for door connection
		if((tr.refdef.areamask[leaf->area >> 3] & (1 << (leaf->area & 7)))) {
			continue;  // not visible
		}

		parent = leaf;
		do {
			if(parent->visCounts[tr.visIndex] == tr.visCounts[tr.visIndex]) break;
			parent->visCounts[tr.visIndex] = tr.visCounts[tr.visIndex];
			parent = parent->parent;
		} while(parent);
	}
}

/*
=============
R_AddWorldSurfaces
=============
*/
void R_AddWorldSurfaces(void) {
	uint32_t planeBits;
	dlightMask_t dlightBits;

	if(tr.refdef.rdflags & RDF_NOWORLDMODEL) return;

	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

	// determine which leaves are in the PVS / areamask
	if(!(tr.viewParms.flags & VPF_DLIGHTSHADOW) && r_pvs->integer) R_MarkLeaves();

	// clear out the visible min/max
	ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);

	// perform frustum culling and flag all the potentially visible surfaces
	if(tr.refdef.num_dlights > MAX_DLIGHTS) tr.refdef.num_dlights = MAX_DLIGHTS;

	planeBits = (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 31 : 15;

	DlightMask_Clear(&dlightBits);
	if(!(tr.viewParms.flags & VPF_DEPTHSHADOW)) {
		int i;
		for(i = 0; i < tr.refdef.num_dlights; i++) DlightMask_Set(&dlightBits, i);
	}

	if(r_pvs->integer)
		R_RecursiveWorldNode(tr.world->nodes, planeBits, &dlightBits);
	else
		R_RecursiveWorldNode_Direct(&dlightBits);

	// now add all the potentially visible surfaces
	// also mask invisible dlights for next frame
	{
		int i;

		DlightMask_Clear(&tr.refdef.dlightMask);

		for(i = 0; i < tr.world->numWorldSurfaces; i++) {
			if(tr.world->surfacesViewCount[i] != tr.viewCount) continue;

			R_AddWorldSurface(tr.world->surfaces + i, &tr.world->surfacesDlightBits[i]);
			DlightMask_Or(&tr.refdef.dlightMask, &tr.world->surfacesDlightBits[i]);
		}

		DlightMask_Not(&tr.refdef.dlightMask);
	}
}
