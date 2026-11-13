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
// tr_sky.c
#include "tr_local.h"

#define SKY_SUBDIVISIONS 8
#define HALF_SKY_SUBDIVISIONS (SKY_SUBDIVISIONS / 2)

static float s_cloudTexCoords[6][SKY_SUBDIVISIONS + 1][SKY_SUBDIVISIONS + 1][2];
static float s_cloudTexP[6][SKY_SUBDIVISIONS + 1][SKY_SUBDIVISIONS + 1];

/*
===================================================================================

POLYGON TO BOX SIDE PROJECTION

===================================================================================
*/

static vec3_t sky_clip[6] = {{1, 1, 0}, {1, -1, 0}, {0, -1, 1}, {0, 1, 1}, {1, 0, 1}, {-1, 0, 1}};

static float sky_mins[2][6], sky_maxs[2][6];
static float sky_min, sky_max;

#ifdef __GNUC__
#pragma GCC diagnostic push
// AddSkyPolygon and ClipSkyPolygon both technically do
// unbounded access of their vecs parameter, though in
// practice the size of what they're passed makes it safe
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif

/*
================
AddSkyPolygon
================
*/
static void AddSkyPolygon(int nump, vec3_t vecs) {
	int i, j;
	vec3_t v, av;
	float s, t, dv;
	int axis;
	float* vp;
	// s = [0]/[2], t = [1]/[2]
	static int vec_to_st[6][3] = {
	    {-2, 3, 1},
	    {2, 3, -1},

	    {1, 3, 2},
	    {-1, 3, -2},

	    {-2, -1, 3},
	    {-2, 1, -3}

	    //	{-1,2,3},
	    //	{1,2,-3}
	};

	// decide which face it maps to
	VectorCopy(vec3_origin, v);
	for(i = 0, vp = vecs; i < nump; i++, vp += 3) {
		VectorAdd(vp, v, v);
	}
	av[0] = fabs(v[0]);
	av[1] = fabs(v[1]);
	av[2] = fabs(v[2]);
	if(av[0] > av[1] && av[0] > av[2]) {
		if(v[0] < 0)
			axis = 1;
		else
			axis = 0;
	} else if(av[1] > av[2] && av[1] > av[0]) {
		if(v[1] < 0)
			axis = 3;
		else
			axis = 2;
	} else {
		if(v[2] < 0)
			axis = 5;
		else
			axis = 4;
	}

	// project new texture coords
	for(i = 0; i < nump; i++, vecs += 3) {
		j = vec_to_st[axis][2];
		if(j > 0)
			dv = vecs[j - 1];
		else
			dv = -vecs[-j - 1];
		if(dv < 0.001) continue;  // don't divide by zero
		j = vec_to_st[axis][0];
		if(j < 0)
			s = -vecs[-j - 1] / dv;
		else
			s = vecs[j - 1] / dv;
		j = vec_to_st[axis][1];
		if(j < 0)
			t = -vecs[-j - 1] / dv;
		else
			t = vecs[j - 1] / dv;

		if(s < sky_mins[0][axis]) sky_mins[0][axis] = s;
		if(t < sky_mins[1][axis]) sky_mins[1][axis] = t;
		if(s > sky_maxs[0][axis]) sky_maxs[0][axis] = s;
		if(t > sky_maxs[1][axis]) sky_maxs[1][axis] = t;
	}
}

#define ON_EPSILON 0.1f  // point on plane side epsilon
#define MAX_CLIP_VERTS 64
/*
================
ClipSkyPolygon
================
*/
static void ClipSkyPolygon(int nump, vec3_t vecs, int stage) {
	float* norm;
	float* v;
	bool front, back;
	float d, e;
	float dists[MAX_CLIP_VERTS];
	int sides[MAX_CLIP_VERTS];
	vec3_t newv[2][MAX_CLIP_VERTS];
	int newc[2];
	int i, j;

	if(nump > MAX_CLIP_VERTS - 2) ri.Error(ERR_DROP, "ClipSkyPolygon: MAX_CLIP_VERTS");
	if(stage == 6) {  // fully clipped, so draw it
		AddSkyPolygon(nump, vecs);
		return;
	}

	front = back = false;
	norm = sky_clip[stage];
	for(i = 0, v = vecs; i < nump; i++, v += 3) {
		d = DotProduct(v, norm);
		if(d > ON_EPSILON) {
			front = true;
			sides[i] = SIDE_FRONT;
		} else if(d < -ON_EPSILON) {
			back = true;
			sides[i] = SIDE_BACK;
		} else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if(!front || !back) {  // not clipped
		ClipSkyPolygon(nump, vecs, stage + 1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy(vecs, (vecs + (i * 3)));
	newc[0] = newc[1] = 0;

	for(i = 0, v = vecs; i < nump; i++, v += 3) {
		switch(sides[i]) {
			case SIDE_FRONT:
				VectorCopy(v, newv[0][newc[0]]);
				newc[0]++;
				break;
			case SIDE_BACK:
				VectorCopy(v, newv[1][newc[1]]);
				newc[1]++;
				break;
			case SIDE_ON:
				VectorCopy(v, newv[0][newc[0]]);
				newc[0]++;
				VectorCopy(v, newv[1][newc[1]]);
				newc[1]++;
				break;
		}

		if(sides[i] == SIDE_ON || sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i]) continue;

		d = dists[i] / (dists[i] - dists[i + 1]);
		for(j = 0; j < 3; j++) {
			e = v[j] + d * (v[j + 3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon(newc[0], newv[0][0], stage + 1);
	ClipSkyPolygon(newc[1], newv[1][0], stage + 1);
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

/*
==============
ClearSkyBox
==============
*/
static void ClearSkyBox(void) {
	int i;

	for(i = 0; i < 6; i++) {
		sky_mins[0][i] = sky_mins[1][i] = 9999;
		sky_maxs[0][i] = sky_maxs[1][i] = -9999;
	}
}

/*
================
RB_ClipSkyPolygons
================
*/
void RB_ClipSkyPolygons(shaderCommands_t* input) {
	vec3_t p[5];  // need one extra point for clipping
	int i, j;

	ClearSkyBox();

	for(i = 0; i < input->numIndexes; i += 3) {
		for(j = 0; j < 3; j++) {
			VectorSubtract(input->xyz[input->indexes[i + j]], backEnd.viewParms.or.origin, p[j]);
		}
		ClipSkyPolygon(3, p[0], 0);
	}
}

/*
===================================================================================

CLOUD VERTEX GENERATION

===================================================================================
*/

/*
** MakeSkyVec
**
** Parms: s, t range from -1 to 1
*/
static void MakeSkyVec(float s, float t, int axis, float outSt[2], vec3_t outXYZ) {
	// 1 = s, 2 = t, 3 = 2048
	static int st_to_vec[6][3] = {
	    {3, -1, 2},
	    {-3, 1, 2},

	    {1, 3, 2},
	    {-1, -3, 2},

	    {-2, -1, 3},  // 0 degrees yaw, look straight up
	    {2, -1, -3}   // look straight down
	};

	vec3_t b;
	int j, k;
	float boxSize;

	boxSize = backEnd.viewParms.zFar / 1.75;  // div sqrt(3)
	b[0] = s * boxSize;
	b[1] = t * boxSize;
	b[2] = boxSize;

	for(j = 0; j < 3; j++) {
		k = st_to_vec[axis][j];
		if(k < 0) {
			outXYZ[j] = -b[-k - 1];
		} else {
			outXYZ[j] = b[k - 1];
		}
	}

	// avoid bilerp seam
	s = (s + 1) * 0.5;
	t = (t + 1) * 0.5;
	if(s < sky_min) {
		s = sky_min;
	} else if(s > sky_max) {
		s = sky_max;
	}

	if(t < sky_min) {
		t = sky_min;
	} else if(t > sky_max) {
		t = sky_max;
	}

	t = 1.0 - t;

	if(outSt) {
		outSt[0] = s;
		outSt[1] = t;
	}
}

static vec3_t s_skyPoints[SKY_SUBDIVISIONS + 1][SKY_SUBDIVISIONS + 1];
static float s_skyTexCoords[SKY_SUBDIVISIONS + 1][SKY_SUBDIVISIONS + 1][2];

enum { SKYU_MODELVIEWPROJECTION, SKYU_TOPCOLOR, SKYU_BOTTOMCOLOR, SKYU_FADEBIAS, SKYU_BRIGHTNESS, SKYU_DUSKCOLOR, SKYU_DUSKINTENSITY, SKYU_DUSKSCALE, SKYU_SUNSIZE, SKYU_SUNCOLOR, SKYU_SUNDIRECTION, SKYU_TEXTURE, SKYU_USETEXTURE, SKYU_STARS_LAYERS, SKYU_STARS_SCALE, SKYU_STARS_FADE, SKYU_STARS_SPEED, SKYU_TIME, SKYU_COUNT };

static GLint skyUniforms[SKYU_COUNT];
static bool skyUniformsInitialized;
static GLuint skyUniformProgram;
static image_t* skyStarsImage;
static char skyStarsImageName[MAX_QEXTENDEDPATH];
static void SkyUpdateStarsImage(void);

/* Reset cached GL handles when the context/program set is recreated. */
void R_ResetSkyShaderState(void) {
	skyUniformProgram = 0;
	skyUniformsInitialized = false;
	skyStarsImage = NULL;
	skyStarsImageName[0] = '\0';
	Com_Memset(skyUniforms, 0xff, sizeof(skyUniforms));
}

/*
 * Runtime sun controls are applied before RE_BeginScene copies the direction
 * and color into refdef.  -1 means "use q3map_sun" independently for yaw/pitch.
 */
void R_UpdateSkySun(void) {
	float yaw, pitch;
	vec3_t sunColor;
	bool overrideSunLight;

	SkyUpdateStarsImage();

	if(!tr.sunDirectionBase[0] && !tr.sunDirectionBase[1] && !tr.sunDirectionBase[2]) {
		VectorSet(tr.sunDirectionBase, 0.45f, 0.30f, 0.90f);
		VectorNormalize(tr.sunDirectionBase);
	}

	yaw = (r_sunAngleX && r_sunAngleX->value != -1.0f) ? r_sunAngleX->value * (M_PI / 180.0f) : atan2(tr.sunDirectionBase[1], tr.sunDirectionBase[0]);
	pitch = (r_sunAngleY && r_sunAngleY->value != -1.0f) ? r_sunAngleY->value * (M_PI / 180.0f) : asin(Com_Clamp(-1.0f, 1.0f, tr.sunDirectionBase[2]));

	tr.sunDirection[0] = cos(yaw) * cos(pitch);
	tr.sunDirection[1] = sin(yaw) * cos(pitch);
	tr.sunDirection[2] = sin(pitch);
	VectorNormalize(tr.sunDirection);

	VectorCopy(tr.sunLightBase, tr.sunLight);
	sunColor[0] = r_sunLightColorR ? r_sunLightColorR->value : 0.0f;
	sunColor[1] = r_sunLightColorG ? r_sunLightColorG->value : 0.0f;
	sunColor[2] = r_sunLightColorB ? r_sunLightColorB->value : 0.0f;
	overrideSunLight = (sunColor[0] != 0.0f || sunColor[1] != 0.0f || sunColor[2] != 0.0f);

	/* All-zero is a strict opt-out: retain the map's q3map_sun color unchanged. */
	if(overrideSunLight) {
		VectorScale(sunColor, 255.0f, tr.sunLight);
	}
}

static void SkyInitUniformLocations(void) {
	GLuint program = tr.proceduralSkyShader.program;

	if(!program) return;
	if(skyUniformProgram != program) {
		skyUniformProgram = program;
		skyUniformsInitialized = false;
		skyStarsImage = NULL;
		skyStarsImageName[0] = '\0';
	}
	if(skyUniformsInitialized) return;

	skyUniforms[SKYU_MODELVIEWPROJECTION] = qglGetUniformLocation(program, "u_ModelViewProjectionMatrix");
	skyUniforms[SKYU_TOPCOLOR] = qglGetUniformLocation(program, "u_SkyTopColor");
	skyUniforms[SKYU_BOTTOMCOLOR] = qglGetUniformLocation(program, "u_SkyBottomColor");
	skyUniforms[SKYU_FADEBIAS] = qglGetUniformLocation(program, "u_SkyFadeBias");
	skyUniforms[SKYU_BRIGHTNESS] = qglGetUniformLocation(program, "u_SkyBrightness");
	skyUniforms[SKYU_DUSKCOLOR] = qglGetUniformLocation(program, "u_DuskColor");
	skyUniforms[SKYU_DUSKINTENSITY] = qglGetUniformLocation(program, "u_DuskIntensity");
	skyUniforms[SKYU_DUSKSCALE] = qglGetUniformLocation(program, "u_DuskScale");
	skyUniforms[SKYU_SUNSIZE] = qglGetUniformLocation(program, "u_SunSize");
	skyUniforms[SKYU_SUNCOLOR] = qglGetUniformLocation(program, "u_SunColor");
	skyUniforms[SKYU_SUNDIRECTION] = qglGetUniformLocation(program, "u_SunDirection");
	skyUniforms[SKYU_TEXTURE] = qglGetUniformLocation(program, "u_SkyTexture");
	skyUniforms[SKYU_USETEXTURE] = qglGetUniformLocation(program, "u_UseSkyTexture");
	skyUniforms[SKYU_STARS_LAYERS] = qglGetUniformLocation(program, "u_StarsLayers");
	skyUniforms[SKYU_STARS_SCALE] = qglGetUniformLocation(program, "u_StarsScale");
	skyUniforms[SKYU_STARS_FADE] = qglGetUniformLocation(program, "u_StarsFade");
	skyUniforms[SKYU_STARS_SPEED] = qglGetUniformLocation(program, "u_StarsSpeed");
	skyUniforms[SKYU_TIME] = qglGetUniformLocation(program, "u_Time");
	skyUniformsInitialized = true;
}

static void SkyUpdateStarsImage(void) {
	const char* name = (r_skyStarsShader && r_skyStarsShader->string) ? r_skyStarsShader->string : "";

	if(!Q_stricmp(name, skyStarsImageName)) return;

	Q_strncpyz(skyStarsImageName, name, sizeof(skyStarsImageName));
	skyStarsImage = NULL;
	if(name[0] && Q_stricmp(name, "-") && Q_stricmp(name, "none")) {
		/* Prefer a declared shader/material. The procedural pass extracts its
		 * first color-map image; the material's blend/tcMod stages are not run. */
		shader_t* overlayShader = R_FindShader(name, LIGHTMAP_NONE, true);
		if(overlayShader && !overlayShader->defaultShader && overlayShader->numUnfoggedPasses > 0 && overlayShader->stages[0]) {
			skyStarsImage = overlayShader->stages[0]->bundle[TB_DIFFUSEMAP].image[0];
		}

		/* R_FindShader also synthesizes a simple shader for raw image paths. */
		if(!skyStarsImage) {
			skyStarsImage = R_FindImageFile((char*)name, IMGTYPE_COLORALPHA, IMGFLAG_MIPMAP);
		}
		if(!skyStarsImage) {
			ri.Printf(PRINT_WARNING, "WARNING: r_skyStarsShader material/image '%s' could not be loaded\n", name);
		}
	}
}

static void SkySetUniforms(void) {
	shaderProgram_t* program = &tr.proceduralSkyShader;
	vec3_t topColor, bottomColor, duskColor, sunColor, sunDirection;
	vec2_t starsSpeed;
	float sunSize;
	int starsLayers;
	bool useStars;

	SkyInitUniformLocations();

	topColor[0] = topColor[1] = topColor[2] = 1.0f;
	if(r_skyTopColor && r_skyTopColor->string) {
		float parsed[3];
		if(sscanf(r_skyTopColor->string, "%f %f %f", &parsed[0], &parsed[1], &parsed[2]) == 3) {
			VectorCopy(parsed, topColor);
		}
	}

	bottomColor[0] = bottomColor[1] = bottomColor[2] = 1.0f;
	if(r_skyBottomColor && r_skyBottomColor->string) {
		float parsed[3];
		if(sscanf(r_skyBottomColor->string, "%f %f %f", &parsed[0], &parsed[1], &parsed[2]) == 3) {
			VectorCopy(parsed, bottomColor);
		}
	}

	duskColor[0] = duskColor[1] = duskColor[2] = 1.0f;
	if(r_skyDuskColor && r_skyDuskColor->string) {
		float parsed[3];
		if(sscanf(r_skyDuskColor->string, "%f %f %f", &parsed[0], &parsed[1], &parsed[2]) == 3) {
			VectorCopy(parsed, duskColor);
		}
	}

	sunColor[0] = sunColor[1] = sunColor[2] = 1.0f;
	if(r_sunColor && r_sunColor->string) {
		float parsed[3];
		if(sscanf(r_sunColor->string, "%f %f %f", &parsed[0], &parsed[1], &parsed[2]) == 3) {
			VectorCopy(parsed, sunColor);
		}
	}

	VectorCopy(tr.sunDirection, sunDirection);
	VectorNormalize(sunDirection);

	starsLayers = (int)Com_Clamp(1.0f, 8.0f, r_skyStarsLayers->value);
	sunSize = MAX(0.0f, r_sunSize->value);
	starsSpeed[0] = r_skyStarsSpeedX->value;
	starsSpeed[1] = r_skyStarsSpeedY->value;
	useStars = (r_skyStars->integer != 0 && skyStarsImage != NULL && r_skyStarsFade->value > 0.0f && r_skyStarsLayers->value > 0.0f);

	if(skyUniforms[SKYU_MODELVIEWPROJECTION] != -1) qglProgramUniformMatrix4fvEXT(program->program, skyUniforms[SKYU_MODELVIEWPROJECTION], 1, GL_FALSE, glState.modelviewProjection);
	if(skyUniforms[SKYU_TOPCOLOR] != -1) qglProgramUniform3fEXT(program->program, skyUniforms[SKYU_TOPCOLOR], topColor[0], topColor[1], topColor[2]);
	if(skyUniforms[SKYU_BOTTOMCOLOR] != -1) qglProgramUniform3fEXT(program->program, skyUniforms[SKYU_BOTTOMCOLOR], bottomColor[0], bottomColor[1], bottomColor[2]);
	if(skyUniforms[SKYU_FADEBIAS] != -1) qglProgramUniform1fEXT(program->program, skyUniforms[SKYU_FADEBIAS], Com_Clamp(0.05f, 8.0f, r_skyFadeBias->value));
	if(skyUniforms[SKYU_BRIGHTNESS] != -1) qglProgramUniform1fEXT(program->program, skyUniforms[SKYU_BRIGHTNESS], MAX(0.0f, r_skyBrightness->value));
	if(skyUniforms[SKYU_DUSKCOLOR] != -1) qglProgramUniform3fEXT(program->program, skyUniforms[SKYU_DUSKCOLOR], duskColor[0], duskColor[1], duskColor[2]);
	if(skyUniforms[SKYU_DUSKINTENSITY] != -1) qglProgramUniform1fEXT(program->program, skyUniforms[SKYU_DUSKINTENSITY], MAX(0.0f, r_skyDuskIntensity->value));
	if(skyUniforms[SKYU_DUSKSCALE] != -1) qglProgramUniform1fEXT(program->program, skyUniforms[SKYU_DUSKSCALE], r_skyDuskScale->value);
	if(skyUniforms[SKYU_SUNSIZE] != -1) qglProgramUniform1fEXT(program->program, skyUniforms[SKYU_SUNSIZE], sunSize);
	if(skyUniforms[SKYU_SUNCOLOR] != -1) qglProgramUniform3fEXT(program->program, skyUniforms[SKYU_SUNCOLOR], sunColor[0], sunColor[1], sunColor[2]);
	if(skyUniforms[SKYU_SUNDIRECTION] != -1) qglProgramUniform3fEXT(program->program, skyUniforms[SKYU_SUNDIRECTION], sunDirection[0], sunDirection[1], sunDirection[2]);
	if(skyUniforms[SKYU_TEXTURE] != -1) qglProgramUniform1iEXT(program->program, skyUniforms[SKYU_TEXTURE], TB_DIFFUSEMAP);
	if(skyUniforms[SKYU_USETEXTURE] != -1) qglProgramUniform1iEXT(program->program, skyUniforms[SKYU_USETEXTURE], useStars ? 1 : 0);
	if(skyUniforms[SKYU_STARS_LAYERS] != -1) qglProgramUniform1iEXT(program->program, skyUniforms[SKYU_STARS_LAYERS], starsLayers);
	if(skyUniforms[SKYU_STARS_SCALE] != -1) qglProgramUniform1fEXT(program->program, skyUniforms[SKYU_STARS_SCALE], MAX(0.0001f, r_skyStarsScale->value));
	if(skyUniforms[SKYU_STARS_FADE] != -1) qglProgramUniform1fEXT(program->program, skyUniforms[SKYU_STARS_FADE], Com_Clamp(0.0f, 5.0f, r_skyStarsFade->value) / 5.0f);
	if(skyUniforms[SKYU_STARS_SPEED] != -1) qglProgramUniform2fEXT(program->program, skyUniforms[SKYU_STARS_SPEED], starsSpeed[0], starsSpeed[1]);
	if(skyUniforms[SKYU_TIME] != -1) qglProgramUniform1fEXT(program->program, skyUniforms[SKYU_TIME], backEnd.refdef.floatTime);
}

static void DrawSkySide(const int mins[2], const int maxs[2]) {
	int s, t;
	int firstVertex = tess.numVertexes;

	tess.firstIndex = tess.numIndexes;

	GL_Cull(CT_TWO_SIDED);

	for(t = mins[1] + HALF_SKY_SUBDIVISIONS; t <= maxs[1] + HALF_SKY_SUBDIVISIONS; t++) {
		for(s = mins[0] + HALF_SKY_SUBDIVISIONS; s <= maxs[0] + HALF_SKY_SUBDIVISIONS; s++) {
			tess.xyz[tess.numVertexes][0] = s_skyPoints[t][s][0];
			tess.xyz[tess.numVertexes][1] = s_skyPoints[t][s][1];
			tess.xyz[tess.numVertexes][2] = s_skyPoints[t][s][2];
			tess.xyz[tess.numVertexes][3] = 1.0;

			tess.texCoords[tess.numVertexes][0] = s_skyTexCoords[t][s][0];
			tess.texCoords[tess.numVertexes][1] = s_skyTexCoords[t][s][1];

			tess.numVertexes++;

			if(tess.numVertexes >= SHADER_MAX_VERTEXES) {
				ri.Error(ERR_DROP, "SHADER_MAX_VERTEXES hit in DrawSkySideVBO()");
			}
		}
	}

	for(t = 0; t < maxs[1] - mins[1]; t++) {
		for(s = 0; s < maxs[0] - mins[0]; s++) {
			if(tess.numIndexes + 6 >= SHADER_MAX_INDEXES) {
				ri.Error(ERR_DROP, "SHADER_MAX_INDEXES hit in DrawSkySideVBO()");
			}

			tess.indexes[tess.numIndexes++] = s + t * (maxs[0] - mins[0] + 1) + firstVertex;
			tess.indexes[tess.numIndexes++] = s + (t + 1) * (maxs[0] - mins[0] + 1) + firstVertex;
			tess.indexes[tess.numIndexes++] = (s + 1) + t * (maxs[0] - mins[0] + 1) + firstVertex;

			tess.indexes[tess.numIndexes++] = (s + 1) + t * (maxs[0] - mins[0] + 1) + firstVertex;
			tess.indexes[tess.numIndexes++] = s + (t + 1) * (maxs[0] - mins[0] + 1) + firstVertex;
			tess.indexes[tess.numIndexes++] = (s + 1) + (t + 1) * (maxs[0] - mins[0] + 1) + firstVertex;
		}
	}

	RB_UpdateTessVao(ATTR_POSITION);
	GLSL_BindProgram(&tr.proceduralSkyShader);
	if(skyStarsImage && r_skyStars->integer) {
		GL_BindToTMU(skyStarsImage, TB_DIFFUSEMAP);
	} else {
		GL_BindToTMU(tr.whiteImage, TB_DIFFUSEMAP);
	}
	SkySetUniforms();

	R_DrawElements(tess.numIndexes - tess.firstIndex, tess.firstIndex);

	tess.numIndexes = tess.firstIndex;
	tess.numVertexes = firstVertex;
	tess.firstIndex = 0;
}

static void DrawProceduralSky(void) {
	int i;

	sky_min = 0;
	sky_max = 1;

	Com_Memset(s_skyTexCoords, 0, sizeof(s_skyTexCoords));

	for(i = 0; i < 6; i++) {
		int sky_mins_subd[2], sky_maxs_subd[2];
		int s, t;

		sky_mins[0][i] = floor(sky_mins[0][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
		sky_mins[1][i] = floor(sky_mins[1][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[0][i] = ceil(sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[1][i] = ceil(sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;

		if((sky_mins[0][i] >= sky_maxs[0][i]) || (sky_mins[1][i] >= sky_maxs[1][i])) {
			continue;
		}

		sky_mins_subd[0] = sky_mins[0][i] * HALF_SKY_SUBDIVISIONS;
		sky_mins_subd[1] = sky_mins[1][i] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[0] = sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[1] = sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS;

		if(sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS)
			sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if(sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS)
			sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
		if(sky_mins_subd[1] < -HALF_SKY_SUBDIVISIONS)
			sky_mins_subd[1] = -HALF_SKY_SUBDIVISIONS;
		else if(sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS)
			sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;

		if(sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS)
			sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if(sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS)
			sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
		if(sky_maxs_subd[1] < -HALF_SKY_SUBDIVISIONS)
			sky_maxs_subd[1] = -HALF_SKY_SUBDIVISIONS;
		else if(sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS)
			sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;

		//
		// iterate through the subdivisions
		//
		for(t = sky_mins_subd[1] + HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1] + HALF_SKY_SUBDIVISIONS; t++) {
			for(s = sky_mins_subd[0] + HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0] + HALF_SKY_SUBDIVISIONS; s++) {
				MakeSkyVec((s - HALF_SKY_SUBDIVISIONS) / (float)HALF_SKY_SUBDIVISIONS, (t - HALF_SKY_SUBDIVISIONS) / (float)HALF_SKY_SUBDIVISIONS, i, s_skyTexCoords[t][s], s_skyPoints[t][s]);
			}
		}

		DrawSkySide(sky_mins_subd, sky_maxs_subd);
	}
}

static void FillCloudySkySide(const int mins[2], const int maxs[2], bool addIndexes) {
	int s, t;
	int vertexStart = tess.numVertexes;
	int tHeight, sWidth;

	tHeight = maxs[1] - mins[1] + 1;
	sWidth = maxs[0] - mins[0] + 1;

	for(t = mins[1] + HALF_SKY_SUBDIVISIONS; t <= maxs[1] + HALF_SKY_SUBDIVISIONS; t++) {
		for(s = mins[0] + HALF_SKY_SUBDIVISIONS; s <= maxs[0] + HALF_SKY_SUBDIVISIONS; s++) {
			VectorAdd(s_skyPoints[t][s], backEnd.viewParms.or.origin, tess.xyz[tess.numVertexes]);
			tess.texCoords[tess.numVertexes][0] = s_skyTexCoords[t][s][0];
			tess.texCoords[tess.numVertexes][1] = s_skyTexCoords[t][s][1];

			tess.numVertexes++;

			if(tess.numVertexes >= SHADER_MAX_VERTEXES) {
				ri.Error(ERR_DROP, "SHADER_MAX_VERTEXES hit in FillCloudySkySide()");
			}
		}
	}

	// only add indexes for one pass, otherwise it would draw multiple times for each pass
	if(addIndexes) {
		for(t = 0; t < tHeight - 1; t++) {
			for(s = 0; s < sWidth - 1; s++) {
				tess.indexes[tess.numIndexes] = vertexStart + s + t * (sWidth);
				tess.numIndexes++;
				tess.indexes[tess.numIndexes] = vertexStart + s + (t + 1) * (sWidth);
				tess.numIndexes++;
				tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * (sWidth);
				tess.numIndexes++;

				tess.indexes[tess.numIndexes] = vertexStart + s + (t + 1) * (sWidth);
				tess.numIndexes++;
				tess.indexes[tess.numIndexes] = vertexStart + s + 1 + (t + 1) * (sWidth);
				tess.numIndexes++;
				tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * (sWidth);
				tess.numIndexes++;
			}
		}
	}
}

static void FillCloudBox(const shader_t* shader, int stage) {
	int i;

	for(i = 0; i < 6; i++) {
		int sky_mins_subd[2], sky_maxs_subd[2];
		int s, t;
		float MIN_T;

		if(1)  // FIXME? shader->sky.fullClouds )
		{
			MIN_T = -HALF_SKY_SUBDIVISIONS;

			// still don't want to draw the bottom, even if fullClouds
			if(i == 5) continue;
		} else {
			switch(i) {
				case 0:
				case 1:
				case 2:
				case 3: MIN_T = -1; break;
				case 5:
					// don't draw clouds beneath you
					continue;
				case 4:  // top
				default: MIN_T = -HALF_SKY_SUBDIVISIONS; break;
			}
		}

		sky_mins[0][i] = floor(sky_mins[0][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
		sky_mins[1][i] = floor(sky_mins[1][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[0][i] = ceil(sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[1][i] = ceil(sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;

		if((sky_mins[0][i] >= sky_maxs[0][i]) || (sky_mins[1][i] >= sky_maxs[1][i])) {
			continue;
		}

		sky_mins_subd[0] = lrintf(sky_mins[0][i] * HALF_SKY_SUBDIVISIONS);
		sky_mins_subd[1] = lrintf(sky_mins[1][i] * HALF_SKY_SUBDIVISIONS);
		sky_maxs_subd[0] = lrintf(sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS);
		sky_maxs_subd[1] = lrintf(sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS);

		if(sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS)
			sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if(sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS)
			sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
		if(sky_mins_subd[1] < MIN_T)
			sky_mins_subd[1] = MIN_T;
		else if(sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS)
			sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;

		if(sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS)
			sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if(sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS)
			sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
		if(sky_maxs_subd[1] < MIN_T)
			sky_maxs_subd[1] = MIN_T;
		else if(sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS)
			sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;

		//
		// iterate through the subdivisions
		//
		for(t = sky_mins_subd[1] + HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1] + HALF_SKY_SUBDIVISIONS; t++) {
			for(s = sky_mins_subd[0] + HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0] + HALF_SKY_SUBDIVISIONS; s++) {
				MakeSkyVec((s - HALF_SKY_SUBDIVISIONS) / (float)HALF_SKY_SUBDIVISIONS, (t - HALF_SKY_SUBDIVISIONS) / (float)HALF_SKY_SUBDIVISIONS, i, NULL, s_skyPoints[t][s]);

				s_skyTexCoords[t][s][0] = s_cloudTexCoords[i][t][s][0];
				s_skyTexCoords[t][s][1] = s_cloudTexCoords[i][t][s][1];
			}
		}

		// only add indexes for first stage
		FillCloudySkySide(sky_mins_subd, sky_maxs_subd, (stage == 0));
	}
}

/*
** R_BuildCloudData
*/
void R_BuildCloudData(shaderCommands_t* input) {
	int i;
	shader_t* shader;

	shader = input->shader;

	assert(shader->isSky);

	sky_min = 1.0 / 256.0f;  // FIXME: not correct?
	sky_max = 255.0 / 256.0f;

	// set up for drawing
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;

	if(shader->sky.cloudHeight) {
		for(i = 0; i < MAX_SHADER_STAGES; i++) {
			if(!tess.xstages[i]) {
				break;
			}
			FillCloudBox(shader, i);
		}
	}
}

/*
** R_InitSkyTexCoords
** Called when a sky shader is parsed
*/
#define SQR(a) ((a) * (a))
void R_InitSkyTexCoords(float heightCloud) {
	int i, s, t;
	float radiusWorld = 4096;
	float p;
	float sRad, tRad;
	vec3_t skyVec;
	vec3_t v;

	// init zfar so MakeSkyVec works even though
	// a world hasn't been bounded
	backEnd.viewParms.zFar = 1024;

	for(i = 0; i < 6; i++) {
		for(t = 0; t <= SKY_SUBDIVISIONS; t++) {
			for(s = 0; s <= SKY_SUBDIVISIONS; s++) {
				// compute vector from view origin to sky side integral point
				MakeSkyVec((s - HALF_SKY_SUBDIVISIONS) / (float)HALF_SKY_SUBDIVISIONS, (t - HALF_SKY_SUBDIVISIONS) / (float)HALF_SKY_SUBDIVISIONS, i, NULL, skyVec);

				// compute parametric value 'p' that intersects with cloud layer
				p = (1.0f / (2 * DotProduct(skyVec, skyVec))) * (-2 * skyVec[2] * radiusWorld + 2 * sqrt(SQR(skyVec[2]) * SQR(radiusWorld) + 2 * SQR(skyVec[0]) * radiusWorld * heightCloud + SQR(skyVec[0]) * SQR(heightCloud) + 2 * SQR(skyVec[1]) * radiusWorld * heightCloud + SQR(skyVec[1]) * SQR(heightCloud) + 2 * SQR(skyVec[2]) * radiusWorld * heightCloud + SQR(skyVec[2]) * SQR(heightCloud)));

				s_cloudTexP[i][t][s] = p;

				// compute intersection point based on p
				VectorScale(skyVec, p, v);
				v[2] += radiusWorld;

				// compute vector from world origin to intersection point 'v'
				VectorNormalize(v);

				sRad = Q_acos(v[0]);
				tRad = Q_acos(v[1]);

				s_cloudTexCoords[i][t][s][0] = sRad;
				s_cloudTexCoords[i][t][s][1] = tRad;
			}
		}
	}
}

//======================================================================================

/*
** RB_DrawSun
*/
void RB_DrawSun(float scale, shader_t* shader) {
	float size;
	float dist;
	vec3_t origin, vec1, vec2;

	if(!backEnd.skyRenderedThisView) {
		return;
	}

	{
		// FIXME: this could be a lot cleaner
		mat4_t translation, modelview;

		Mat4Translation(backEnd.viewParms.or.origin, translation);
		Mat4Multiply(backEnd.viewParms.world.modelMatrix, translation, modelview);
		GL_SetModelviewMatrix(modelview);
	}

	dist = backEnd.viewParms.zFar / 1.75;  // div sqrt(3)
	size = dist * scale;

	VectorScale(tr.sunDirection, dist, origin);
	PerpendicularVector(vec1, tr.sunDirection);
	CrossProduct(tr.sunDirection, vec1, vec2);

	VectorScale(vec1, size, vec1);
	VectorScale(vec2, size, vec2);

	// farthest depth range
	qglDepthRange(1.0, 1.0);

	RB_BeginSurface(shader, 0, false, vec3_origin, 0.0f);

	RB_AddQuadStamp(origin, vec1, vec2, colorWhite);

	RB_EndSurface();

	// back to normal depth range
	qglDepthRange(0.0, 1.0);
}

/*
================
RB_StageIteratorSky

All of the visible sky triangles are in tess

Other things could be stuck in here, like birds in the sky, etc
================
*/
void RB_StageIteratorSky(void) {
	// go through all the polygons and project them onto
	// the sky box to see which blocks on each side need
	// to be drawn
	RB_ClipSkyPolygons(&tess);

	qglDepthRange(1.0, 1.0);

	// Draw the procedural sky into the clipped sky polygons. The sky shader
	// needs only surfaceparm sky; no stages, skyparms, or skybox textures.
	{
		mat4_t oldmodelview;
		mat4_t trans, product;

		GL_State(0);
		GL_Cull(CT_FRONT_SIDED);
		Mat4Copy(glState.modelview, oldmodelview);
		Mat4Translation(backEnd.viewParms.or.origin, trans);
		Mat4Multiply(glState.modelview, trans, product);
		GL_SetModelviewMatrix(product);

		DrawProceduralSky();

		GL_SetModelviewMatrix(oldmodelview);
	}

	// draw the inner skybox

	// back to normal depth range
	qglDepthRange(0.0, 1.0);

	// note that sky was drawn so we will draw a sun later
	backEnd.skyRenderedThisView = true;
}
