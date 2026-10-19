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

int r_firstSceneDrawSurf;

int r_numdlights;
int r_firstSceneDlight;

int r_numentities;
int r_firstSceneEntity;

int r_numpolys;
int r_firstScenePoly;

int r_numpolyverts;

/*
====================
R_InitNextFrame

====================
*/
void R_InitNextFrame(void) {
	backEndData->commands.used = 0;

	r_firstSceneDrawSurf = 0;

	r_numdlights = 0;
	r_firstSceneDlight = 0;

	r_numentities = 0;
	r_firstSceneEntity = 0;

	r_numpolys = 0;
	r_firstScenePoly = 0;

	r_numpolyverts = 0;
}

/*
====================
RE_ClearScene

====================
*/
void RE_ClearScene(void) {
	r_firstSceneDlight = r_numdlights;
	r_firstSceneEntity = r_numentities;
	r_firstScenePoly = r_numpolys;
}

/*
===========================================================================

DISCRETE POLYS

===========================================================================
*/

/*
=====================
R_AddPolygonSurfaces

Adds all the scene's polys into this view's drawsurf list
=====================
*/
void R_AddPolygonSurfaces(void) {
	int i;
	shader_t* sh;
	srfPoly_t* poly;
	int fogMask;

	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;
	fogMask = -((tr.refdef.rdflags & RDF_NOFOG) == 0);

	for(i = 0, poly = tr.refdef.polys; i < tr.refdef.numPolys; i++, poly++) {
		sh = R_GetShaderByHandle(poly->hShader);
		R_AddDrawSurf((void*)poly, sh, poly->fogIndex & fogMask, false, false, vec3_origin, 0.0f);
	}
}

/*
=====================
RE_AddPolyToScene

=====================
*/
void RE_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t* verts, int numPolys) {
	srfPoly_t* poly;
	int i, j;
	int fogIndex;
	fog_t* fog;
	vec3_t bounds[2];

	if(!tr.registered) {
		return;
	}

	if(!hShader) {
		// This isn't a useful warning, and an hShader of zero isn't a null shader, it's
		// the default shader.
		// ri.Printf( PRINT_WARNING, "WARNING: RE_AddPolyToScene: NULL poly shader\n");
		// return;
	}

	for(j = 0; j < numPolys; j++) {
		if(r_numpolyverts + numVerts > max_polyverts || r_numpolys >= max_polys) {
			/*
			NOTE TTimo this was initially a PRINT_WARNING
			but it happens a lot with high fighting scenes and particles
			since we don't plan on changing the const and making for room for those effects
			simply cut this message to developer only
			*/
			ri.Printf(PRINT_DEVELOPER, "WARNING: RE_AddPolyToScene: r_max_polys or r_max_polyverts reached\n");
			return;
		}

		poly = &backEndData->polys[r_numpolys];
		poly->surfaceType = SF_POLY;
		poly->hShader = hShader;
		poly->numVerts = numVerts;
		poly->verts = &backEndData->polyVerts[r_numpolyverts];

		Com_Memcpy(poly->verts, &verts[numVerts * j], numVerts * sizeof(*verts));

		// done.
		r_numpolys++;
		r_numpolyverts += numVerts;

		// if no world is loaded
		if(tr.world == NULL) {
			fogIndex = 0;
		}
		// see if it is in a fog volume
		else if(tr.world->numfogs == 1) {
			fogIndex = 0;
		} else {
			// find which fog volume the poly is in
			VectorCopy(poly->verts[0].xyz, bounds[0]);
			VectorCopy(poly->verts[0].xyz, bounds[1]);
			for(i = 1; i < poly->numVerts; i++) {
				AddPointToBounds(poly->verts[i].xyz, bounds[0], bounds[1]);
			}
			for(fogIndex = 1; fogIndex < tr.world->numfogs; fogIndex++) {
				fog = &tr.world->fogs[fogIndex];
				if(bounds[1][0] >= fog->bounds[0][0] && bounds[1][1] >= fog->bounds[0][1] && bounds[1][2] >= fog->bounds[0][2] && bounds[0][0] <= fog->bounds[1][0] && bounds[0][1] <= fog->bounds[1][1] && bounds[0][2] <= fog->bounds[1][2]) {
					break;
				}
			}
			if(fogIndex == tr.world->numfogs) {
				fogIndex = 0;
			}
		}
		poly->fogIndex = fogIndex;
	}
}

//=================================================================================

/*
=====================
RE_AddRefEntityToScene

=====================
*/
void RE_AddRefEntityToScene(const refEntity_t* ent) {
	vec3_t cross;

	if(!tr.registered) {
		return;
	}
	if(r_numentities >= MAX_REFENTITIES) {
		ri.Printf(PRINT_DEVELOPER, "RE_AddRefEntityToScene: Dropping refEntity, reached MAX_REFENTITIES\n");
		return;
	}
	if(Q_isnan(ent->origin[0]) || Q_isnan(ent->origin[1]) || Q_isnan(ent->origin[2])) {
		static bool firstTime = true;
		if(firstTime) {
			firstTime = false;
			ri.Printf(PRINT_WARNING, "RE_AddRefEntityToScene passed a refEntity which has an origin with a NaN component\n");
		}
		return;
	}
	if((int)ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE) {
		ri.Error(ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType);
	}

	backEndData->entities[r_numentities].e = *ent;
	backEndData->entities[r_numentities].lightingCalculated = false;

	CrossProduct(ent->axis[0], ent->axis[1], cross);
	backEndData->entities[r_numentities].mirrored = (DotProduct(ent->axis[2], cross) < 0.f);

	r_numentities++;
}

/*
=====================
RE_AddDynamicLightToScene
=====================
*/
void RE_AddDynamicLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b, int flags, qhandle_t hShader) {
	dlight_t* dl;

	// early out
	if(!tr.registered || r_numdlights >= MAX_DLIGHTS || radius <= 0 || intensity <= 0) {
		return;
	}

	// allow forcing some dlights under all circumstances
	if(r_dynamiclight->integer == 0 && !(flags & REF_FORCE_DLIGHT)) {
		return;
	}

	// set up a new dlight
	dl = &backEndData->dlights[r_numdlights++];
	VectorCopy(org, dl->origin);
	VectorCopy(org, dl->transformed);
	dl->radius = radius;
	dl->radiusInverseCubed = (1.0 / dl->radius);
	dl->radiusInverseCubed = dl->radiusInverseCubed * dl->radiusInverseCubed * dl->radiusInverseCubed;
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->flags = flags;
	if(hShader) {
		dl->dlshader = R_GetShaderByHandle(hShader);
	} else {
		dl->dlshader = NULL;
	}
}

void RE_AddLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader) { RE_AddDynamicLightToScene(org, radius, intensity, r, g, b, REF_GRID_DLIGHT | REF_SURFACE_DLIGHT, hShader); }

void RE_AddAdditiveLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b) { RE_AddDynamicLightToScene(org, radius, intensity, r, g, b, REF_GRID_DLIGHT | REF_SURFACE_DLIGHT | REF_ADDITIVE_DLIGHT, 0); }

void RE_AddVertexLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b) { RE_AddDynamicLightToScene(org, radius, intensity, r, g, b, REF_GRID_DLIGHT | REF_SURFACE_DLIGHT | REF_VERTEX_DLIGHT, 0); }

void RE_AddJuniorLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b) { RE_AddDynamicLightToScene(org, radius, intensity, r, g, b, REF_GRID_DLIGHT, 0); }

void RE_AddDirectedLightToScene(const vec3_t normal, float intensity, float r, float g, float b) { RE_AddDynamicLightToScene(normal, 256, intensity, r, g, b, REF_GRID_DLIGHT | REF_SURFACE_DLIGHT | REF_DIRECTED_DLIGHT, 0); }

void RE_BeginScene(const refdef_t* fd) {
	Com_Memcpy(tr.refdef.text, fd->text, sizeof(tr.refdef.text));

	tr.refdef.x = fd->x;
	tr.refdef.y = fd->y;
	tr.refdef.width = fd->width;
	tr.refdef.height = fd->height;
	tr.refdef.fov_x = fd->fov_x;
	tr.refdef.fov_y = fd->fov_y;

	VectorCopy(fd->vieworg, tr.refdef.vieworg);
	VectorCopy(fd->viewaxis[0], tr.refdef.viewaxis[0]);
	VectorCopy(fd->viewaxis[1], tr.refdef.viewaxis[1]);
	VectorCopy(fd->viewaxis[2], tr.refdef.viewaxis[2]);

	tr.refdef.time = fd->time;
	tr.refdef.rdflags = fd->rdflags;

	// copy the areamask data over and note if it has changed, which
	// will force a reset of the visible leafs even if the view hasn't moved
	tr.refdef.areamaskModified = false;
	if(!(tr.refdef.rdflags & RDF_NOWORLDMODEL)) {
		int areaDiff;
		int i;

		// compare the area bits
		areaDiff = 0;
		for(i = 0; i < MAX_MAP_AREA_BYTES / 4; i++) {
			areaDiff |= ((int*)tr.refdef.areamask)[i] ^ ((int*)fd->areamask)[i];
			((int*)tr.refdef.areamask)[i] = ((int*)fd->areamask)[i];
		}

		if(areaDiff) {
			// a door just opened or something
			tr.refdef.areamaskModified = true;
		}
	}

	tr.refdef.sunDir[3] = 0.0f;
	tr.refdef.sunCol[3] = 1.0f;
	tr.refdef.sunAmbCol[3] = 1.0f;

	VectorCopy(tr.sunDirection, tr.refdef.sunDir);
	if(tr.refdef.rdflags & RDF_NOWORLDMODEL) {
		VectorSet(tr.refdef.sunCol, 0, 0, 0);
		VectorSet(tr.refdef.sunAmbCol, 0, 0, 0);
	} else {
		float scale = (1 << 0) / 255.0f;

		VectorScale(tr.sunLight, scale * r_sunShadowLightScale->value, tr.refdef.sunCol);

		if(r_sunShadows->integer) {
			tr.refdef.sunAmbCol[0] = tr.refdef.sunAmbCol[1] = tr.refdef.sunAmbCol[2] = r_sunShadowAmbientScale->value;
		} else {
			VectorScale(tr.sunLight, scale * r_sunShadowAmbientScale->value, tr.refdef.sunAmbCol);
		}
	}

	if(r_autoExposure->integer) {
		tr.refdef.autoExposureMinMax[0] = r_autoExposureMin->value;
		tr.refdef.autoExposureMinMax[1] = r_autoExposureMax->value;
	}

	tr.refdef.toneMinAvgMaxLinear[0] = pow(2, r_toneMapMin->value);
	tr.refdef.toneMinAvgMaxLinear[1] = pow(2, r_toneMapAvg->value);
	tr.refdef.toneMinAvgMaxLinear[2] = pow(2, r_toneMapMax->value);

	// Makro - copy exta info if present
	if(fd->rdflags & RDF_EXTRA) {
		const refdefex_t* extra = (const refdefex_t*)(fd + 1);

		tr.refdef.blurFactor = extra->blurFactor;

		if(fd->rdflags & RDF_SUNLIGHT) {
			VectorCopy(extra->sunDir, tr.refdef.sunDir);
			VectorCopy(extra->sunCol, tr.refdef.sunCol);
			VectorCopy(extra->sunAmbCol, tr.refdef.sunAmbCol);
		}
	} else {
		tr.refdef.blurFactor = 0.0f;
	}

	// derived info

	tr.refdef.floatTime = tr.refdef.time * 0.001;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs = backEndData->drawSurfs;

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData->entities[r_firstSceneEntity];

	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData->dlights[r_firstSceneDlight];

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData->polys[r_firstScenePoly];

	// turn off dynamic lighting globally by clearing all the
	// dlights if it needs to be disabled or if vertex lighting is enabled
	if(r_dynamiclight->integer == 0) {
		tr.refdef.num_dlights = 0;
	}

	// a single frame may have multiple scenes draw inside it --
	// a 3D game view, 3D status bar renderings, 3D menus, etc.
	// They need to be distinguished by the light flare code, because
	// the visibility state for a given surface may be different in
	// each scene / view.
	tr.frameSceneNum++;
	tr.sceneCount++;
}

void RE_EndScene(void) {
	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
	r_firstSceneEntity = r_numentities;
	r_firstSceneDlight = r_numdlights;
	r_firstScenePoly = r_numpolys;
}

/*
@@@@@@@@@@@@@@@@@@@@@
RE_RenderScene

Draw a 3D view into a part of the window, then return
to 2D drawing.

Rendering a scene may require multiple views to be rendered
to handle mirrors,
@@@@@@@@@@@@@@@@@@@@@
*/
void RE_RenderScene(const refdef_t* fd) {
	viewParms_t parms;
	int startTime;

	if(!tr.registered) return;

	startTime = ri.Milliseconds();

	if(!tr.world && !(fd->rdflags & RDF_NOWORLDMODEL)) {
		ri.Error(ERR_DROP, "R_RenderScene: NULL worldmodel");
	}

	RE_BeginScene(fd);

	// SmileTheory: playing with shadow mapping
	if(!(fd->rdflags & RDF_NOWORLDMODEL) && tr.refdef.num_dlights && r_dynamiclight->integer >= 2) {
		R_RenderDlightCubemaps(fd);
	}

	// playing with even more shadows
	if(glRefConfig.framebufferObject && r_sunShadows->integer && !(fd->rdflags & RDF_NOWORLDMODEL) && r_sunShadows->integer) {
		if((tr.frameCount & 1) == 0) {
			if(r_shadowCascadeZFar->integer != 0) {
				if((tr.frameCount & 1) == 0) R_RenderSunShadowMaps(fd, 0);   // 1-й каскад: каждые 2 кадра
				if((tr.frameCount & 15) == 0) R_RenderSunShadowMaps(fd, 1);  // 2-й каскад: каждые 16 кадров
				if((tr.frameCount & 63) == 0) R_RenderSunShadowMaps(fd, 2);  // 3-й каскад: каждые 64 кадра
			} else {
				Mat4Zero(tr.refdef.sunShadowMvp[0]);
				Mat4Zero(tr.refdef.sunShadowMvp[1]);
				Mat4Zero(tr.refdef.sunShadowMvp[2]);
			}

			// only rerender last cascade if sun has changed position
			if(!VectorCompare(tr.refdef.sunDir, tr.lastCascadeSunDirection)) {
				VectorCopy(tr.refdef.sunDir, tr.lastCascadeSunDirection);
				R_RenderSunShadowMaps(fd, 3);
				Mat4Copy(tr.refdef.sunShadowMvp[3], tr.lastCascadeSunMvp);
			} else {
				Mat4Copy(tr.lastCascadeSunMvp, tr.refdef.sunShadowMvp[3]);
			}
		}
	}

	if(r_reflections->integer && tr.cubemap) {
		static int current_side = 0;
		if(r_reflections->integer == 1) {
			int gX, gY, gZ;

			gX = tr.refdef.vieworg[0] / r_reflectionsGrid->value;
			gY = tr.refdef.vieworg[1] / r_reflectionsGrid->value;
			gZ = tr.refdef.vieworg[2] / r_reflectionsGrid->value;
			tr.cubemap->origin[0] = (r_reflectionsGrid->value * gX) + (r_reflectionsGrid->value * 0.5);
			tr.cubemap->origin[1] = (r_reflectionsGrid->value * gY) + (r_reflectionsGrid->value * 0.5);
			tr.cubemap->origin[2] = (r_reflectionsGrid->value * gZ) + (r_reflectionsGrid->value * 0.5);
			if(gX != tr.cubemap->gridX[current_side] || gY != tr.cubemap->gridY[current_side] || gZ != tr.cubemap->gridZ[current_side]) {
				R_RenderCubemapSide(current_side, true);
				tr.cubemap->gridX[current_side] = gX;
				tr.cubemap->gridY[current_side] = gY;
				tr.cubemap->gridZ[current_side] = gZ;
			}
			current_side = (current_side + 1) % 6;
		} else if(r_reflections->integer == 2) {
			if((tr.frameCount & 1) == 0) {
				VectorCopy(tr.refdef.vieworg, tr.cubemap->origin);
				R_RenderCubemapSide(current_side, true);
				current_side = (current_side + 1) % 6;
			}
		} else if(r_reflections->integer == 3) {
			VectorCopy(tr.refdef.vieworg, tr.cubemap->origin);
			R_RenderCubemapSide(current_side, true);
			current_side = (current_side + 1) % 6;
		} else if(r_reflections->integer == 4) {
			VectorCopy(tr.refdef.vieworg, tr.cubemap->origin);
			for(int i = 0; i < 6; i++) R_RenderCubemapSide(i, true);
		}
	}

	// setup view parms for the initial view
	//
	// set up viewport
	// The refdef takes 0-at-the-top y coordinates, so
	// convert to GL's 0-at-the-bottom space
	//
	Com_Memset(&parms, 0, sizeof(parms));
	parms.viewportX = tr.refdef.x;
	parms.viewportY = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);
	parms.viewportWidth = tr.refdef.width;
	parms.viewportHeight = tr.refdef.height;
	parms.isPortal = false;

	parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	VectorCopy(fd->vieworg, parms.or.origin);
	VectorCopy(fd->viewaxis[0], parms.or.axis[0]);
	VectorCopy(fd->viewaxis[1], parms.or.axis[1]);
	VectorCopy(fd->viewaxis[2], parms.or.axis[2]);

	VectorCopy(fd->vieworg, parms.pvsOrigin);

	if(!(fd->rdflags & RDF_NOWORLDMODEL) && r_sunShadows->integer) {
		parms.flags = VPF_USESUNLIGHT;
	}

	R_RenderView(&parms);

	if(!(fd->rdflags & RDF_NOWORLDMODEL)) R_AddPostProcessCmd();

	RE_EndScene();

	tr.frontEndMsec += ri.Milliseconds() - startTime;
}
