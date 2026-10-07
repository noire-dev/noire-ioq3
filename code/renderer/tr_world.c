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
	if(r_nocull->integer || surf->cullinfo.type == CULLINFO_NONE) {
		return false;
	}

	if(r_nocurves->integer && *surf->data == SF_GRID) {
		return true;
	}

	if(surf->cullinfo.type & CULLINFO_PLANE) {
		// Only true for SF_FACE, so treat like its own function
		float d;
		cullType_t ct;

		if(!r_facePlaneCull->integer) {
			return false;
		}

		ct = surf->shader->cullType;

		if(ct == CT_TWO_SIDED) {
			return false;
		}

		// shadowmaps draw back surfaces
		if(tr.viewParms.flags & (VPF_DEPTHSHADOW)) {
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

	if(surf->cullinfo.type & CULLINFO_SPHERE) {
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

	if(surf->cullinfo.type & CULLINFO_BOX) {
		int boxCull;

		if(tr.currentEntityNum != REFENTITYNUM_WORLD) {
			boxCull = R_CullLocalBox(surf->cullinfo.bounds);
		} else {
			boxCull = R_CullBox(surf->cullinfo.bounds);
		}

		if(boxCull == CULL_OUT) {
			return true;
		}
	}

	return false;
}

/*
====================
R_DlightSurface

The given surface is going to be drawn, and it touches a leaf
that is touched by one or more dlights, so try to throw out
more dlights if possible.
====================
*/
static int R_DlightSurface(msurface_t* surf, int dlightBits) {
	float d;
	int i;
	dlight_t* dl;

	if(surf->cullinfo.type & CULLINFO_PLANE) {
		for(i = 0; i < tr.refdef.num_dlights; i++) {
			if(!(dlightBits & (1 << i))) {
				continue;
			}
			dl = &tr.refdef.dlights[i];
			d = DotProduct(dl->origin, surf->cullinfo.plane.normal) - surf->cullinfo.plane.dist;
			if(d < -dl->radius || d > dl->radius) {
				// dlight doesn't reach the plane
				dlightBits &= ~(1 << i);
			}
		}
	}

	if(surf->cullinfo.type & CULLINFO_BOX) {
		for(i = 0; i < tr.refdef.num_dlights; i++) {
			if(!(dlightBits & (1 << i))) {
				continue;
			}
			dl = &tr.refdef.dlights[i];
			if(dl->origin[0] - dl->radius > surf->cullinfo.bounds[1][0] || dl->origin[0] + dl->radius < surf->cullinfo.bounds[0][0] || dl->origin[1] - dl->radius > surf->cullinfo.bounds[1][1] || dl->origin[1] + dl->radius < surf->cullinfo.bounds[0][1] || dl->origin[2] - dl->radius > surf->cullinfo.bounds[1][2] || dl->origin[2] + dl->radius < surf->cullinfo.bounds[0][2]) {
				// dlight doesn't reach the bounds
				dlightBits &= ~(1 << i);
			}
		}
	}

	if(surf->cullinfo.type & CULLINFO_SPHERE) {
		for(i = 0; i < tr.refdef.num_dlights; i++) {
			if(!(dlightBits & (1 << i))) {
				continue;
			}
			dl = &tr.refdef.dlights[i];
			if(!SpheresIntersect(dl->origin, dl->radius, surf->cullinfo.localOrigin, surf->cullinfo.radius)) {
				// dlight doesn't reach the bounds
				dlightBits &= ~(1 << i);
			}
		}
	}

	switch(*surf->data) {
		case SF_FACE:
		case SF_GRID:
		case SF_TRIANGLES: ((srfBspSurface_t*)surf->data)->dlightBits = dlightBits; break;

		default: dlightBits = 0; break;
	}

	if(dlightBits) {
		tr.pc.c_dlightSurfaces++;
	} else {
		tr.pc.c_dlightSurfacesCulled++;
	}

	return dlightBits;
}

/*
======================
R_AddWorldSurface
======================
*/
static void R_AddWorldSurface(msurface_t* surf, int dlightBits) {
	// FIXME: bmodel fog?

	// try to cull before dlighting or adding
	if(!(tr.viewParms.targetFbo == tr.dlightShadowFbo && (tr.viewParms.flags & VPF_DEPTHSHADOW))) {
		if(R_CullSurface(surf)) {
			return;
		}
	}

	// check for dlighting
	/*if ( dlightBits ) */ {
		dlightBits = R_DlightSurface(surf, dlightBits);
		dlightBits = (dlightBits != 0);
	}

	R_AddDrawSurf(surf->data, surf->shader, surf->fogIndex, dlightBits, surf->useCubemap, surf->cubemapOrigin);
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
			R_AddWorldSurface(tr.world->surfaces + surf, tr.currentEntity->needDlights);
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

	if(!(tr.viewParms.flags & VPF_DLIGHTSHADOW)) return false;

	index = tr.viewParms.targetFboCubemapIndex;

	if(index < 0 || index >= tr.refdef.num_dlights) return true;

	dl = &tr.refdef.dlights[index];

	if(node->surfMaxs[0] <= dl->origin[0] - dl->radius || node->surfMins[0] >= dl->origin[0] + dl->radius || node->surfMaxs[1] <= dl->origin[1] - dl->radius || node->surfMins[1] >= dl->origin[1] + dl->radius || node->surfMaxs[2] <= dl->origin[2] - dl->radius || node->surfMins[2] >= dl->origin[2] + dl->radius) {
		return true;
	}

	return false;
}

static void R_RecursiveWorldNode(mnode_t* node, uint32_t planeBits, uint32_t dlightBits) {
	int i, r;
	dlight_t* dl;

	do {
		/*
		 * Dlight shadow uses light volume + frustum.
		 * Do not use PVS here.
		 */
		if(tr.viewParms.flags & VPF_DLIGHTSHADOW) {
			if(R_CullDlightShadowNode(node)) return;
		} else if(!(tr.viewParms.flags & VPF_DEPTHSHADOW)) {
			if(node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex]) return;
		}

		/*
		 * Normal view frustum culling.
		 */
		if(!r_nocull->integer) {
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
		}

		/*
		 * Normal dynamic light culling.
		 * Not used by dlight shadow views.
		 */
		if(dlightBits) {
			for(i = 0; i < tr.refdef.num_dlights; i++) {
				if(dlightBits & (1 << i)) {
					if(tr.refdef.dlights[i].flags & REF_DIRECTED_DLIGHT) continue;

					dl = &tr.refdef.dlights[i];

					if(node->surfMins[0] >= (dl->origin[0] + dl->radius) || node->surfMaxs[0] <= (dl->origin[0] - dl->radius) || node->surfMins[1] >= (dl->origin[1] + dl->radius) || node->surfMaxs[1] <= (dl->origin[1] - dl->radius) || node->surfMins[2] >= (dl->origin[2] + dl->radius) || node->surfMaxs[2] <= (dl->origin[2] - dl->radius)) {
						dlightBits &= ~(1 << i);
					}
				}
			}
		}

		if(node->isLeaf) break;

		R_RecursiveWorldNode(node->children[0], planeBits, dlightBits);

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

					R_AddWorldSurface(msurf, 0);
				} else {
					tr.world->surfacesDlightBits[surf] = dlightBits;
				}
			} else if(!(tr.viewParms.flags & VPF_DLIGHTSHADOW)) {
				tr.world->surfacesDlightBits[surf] |= dlightBits;
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
		// if the areamask or r_showcluster was modified, invalidate all visclusters
		// this caused doors to open into undrawn areas
		if(tr.refdef.areamaskModified || r_showcluster->modified) {
			tr.visClusters[i] = -2;
		} else if(tr.visClusters[i] == cluster) {
			if(tr.visClusters[i] != tr.visClusters[tr.visIndex] && r_showcluster->integer) {
				ri.Printf(PRINT_ALL, "found cluster:%i  area:%i  index:%i\n", cluster, leaf->area, i);
			}
			tr.visIndex = i;
			return;
		}
	}

	tr.visIndex = (tr.visIndex + 1) % MAX_VISCOUNTS;
	tr.visCounts[tr.visIndex]++;
	tr.visClusters[tr.visIndex] = cluster;

	if(r_showcluster->modified || r_showcluster->integer) {
		r_showcluster->modified = false;
		if(r_showcluster->integer) {
			ri.Printf(PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area);
		}
	}

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
	uint32_t planeBits, dlightBits;

	if(!r_drawworld->integer) {
		return;
	}

	if(tr.refdef.rdflags & RDF_NOWORLDMODEL) {
		return;
	}

	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

	// determine which leaves are in the PVS / areamask
	if(!(tr.viewParms.flags & VPF_DEPTHSHADOW)) R_MarkLeaves();

	// clear out the visible min/max
	ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);

	// perform frustum culling and flag all the potentially visible surfaces
	if(tr.refdef.num_dlights > MAX_DLIGHTS) {
		tr.refdef.num_dlights = MAX_DLIGHTS;
	}

	planeBits = (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 31 : 15;

	if(tr.viewParms.flags & VPF_DEPTHSHADOW) {
		dlightBits = 0;
	} else {
		dlightBits = (1ULL << tr.refdef.num_dlights) - 1;
	}

	R_RecursiveWorldNode(tr.world->nodes, planeBits, dlightBits);

	// now add all the potentially visible surfaces
	// also mask invisible dlights for next frame
	{
		int i;

		tr.refdef.dlightMask = 0;

		for(i = 0; i < tr.world->numWorldSurfaces; i++) {
			if(tr.world->surfacesViewCount[i] != tr.viewCount) continue;

			R_AddWorldSurface(tr.world->surfaces + i, tr.world->surfacesDlightBits[i]);
			tr.refdef.dlightMask |= tr.world->surfacesDlightBits[i];
		}

		tr.refdef.dlightMask = ~tr.refdef.dlightMask;
	}
}
