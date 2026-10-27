/*
===========================================================================
Copyright (C) 2011 Andrei Drexler, Richard Allen, James Canete

This file is part of Reaction source code.

Reaction source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Reaction source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Reaction source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "tr_local.h"
#include "tr_dsa.h"

void RB_ToneMap(FBO_t* hdrFbo, ivec4_t hdrBox, FBO_t* ldrFbo, ivec4_t ldrBox, int autoExposure) {
	ivec4_t srcBox, dstBox;
	vec4_t color;
	static int lastFrameCount = 0;

	if(autoExposure) {
		if(lastFrameCount == 0 || tr.frameCount < lastFrameCount || tr.frameCount - lastFrameCount > 5) {
			// determine average log luminance
			FBO_t *srcFbo, *dstFbo, *tmp;
			int size = 256;

			lastFrameCount = tr.frameCount;

			VectorSet4(dstBox, 0, 0, size, size);

			FBO_Blit(hdrFbo, hdrBox, NULL, tr.textureScratchFbo[0], dstBox, &tr.calclevels4xShader, NULL, 0);

			srcFbo = tr.textureScratchFbo[0];
			dstFbo = tr.textureScratchFbo[1];

			// downscale to 1x1 texture
			while(size > 1) {
				VectorSet4(srcBox, 0, 0, size, size);
				// size >>= 2;
				size >>= 1;
				VectorSet4(dstBox, 0, 0, size, size);

				if(size == 1) dstFbo = tr.targetLevelsFbo;

				FBO_FastBlit(srcFbo, srcBox, dstFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_LINEAR);

				tmp = srcFbo;
				srcFbo = dstFbo;
				dstFbo = tmp;
			}
		}

		// blend with old log luminance for gradual change
		VectorSet4(srcBox, 0, 0, 0, 0);

		color[0] = color[1] = color[2] = 1.0f;
		if(glRefConfig.textureFloat)
			color[3] = 0.03f;
		else
			color[3] = 0.1f;

		FBO_Blit(tr.targetLevelsFbo, srcBox, NULL, tr.calcLevelsFbo, NULL, NULL, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}

	// tonemap
	color[0] = color[1] = color[2] = pow(2, 1 - autoExposure);  // exp2(1);
	color[3] = 1.0f;

	if(autoExposure)
		GL_BindToTMU(tr.calcLevelsImage, TB_LEVELSMAP);
	else
		GL_BindToTMU(tr.fixedLevelsImage, TB_LEVELSMAP);

	FBO_Blit(hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &tr.tonemapShader, color, 0);
}

/*
=============
RB_BokehBlur


Blurs a part of one framebuffer to another.

Framebuffers can be identical.
=============
*/
void RB_BokehBlur(FBO_t* src, ivec4_t srcBox, FBO_t* dst, ivec4_t dstBox, float blur) {
	//	ivec4_t srcBox, dstBox;
	vec4_t color;

	blur *= 10.0f;

	if(blur < 0.004f) return;

	if(glRefConfig.framebufferObject) {
		// bokeh blur
		if(blur > 0.0f) {
			ivec4_t quarterBox;

			quarterBox[0] = 0;
			quarterBox[1] = tr.quarterFbo[0]->height;
			quarterBox[2] = tr.quarterFbo[0]->width;
			quarterBox[3] = -tr.quarterFbo[0]->height;

			// create a quarter texture
			// FBO_Blit(NULL, NULL, NULL, tr.quarterFbo[0], NULL, NULL, NULL, 0);
			FBO_FastBlit(src, srcBox, tr.quarterFbo[0], quarterBox, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}

#ifndef HQ_BLUR
		if(blur > 1.0f) {
			// create a 1/16th texture
			// FBO_Blit(tr.quarterFbo[0], NULL, NULL, tr.textureScratchFbo[0], NULL, NULL, NULL, 0);
			FBO_FastBlit(tr.quarterFbo[0], NULL, tr.textureScratchFbo[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}
#endif

		if(blur > 0.0f && blur <= 1.0f) {
			// Crossfade original with quarter texture
			VectorSet4(color, 1, 1, 1, blur);

			FBO_Blit(tr.quarterFbo[0], NULL, NULL, dst, dstBox, NULL, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		}
#ifndef HQ_BLUR
		// ok blur, but can see some pixelization
		else if(blur > 1.0f && blur <= 2.0f) {
			// crossfade quarter texture with 1/16th texture
			FBO_Blit(tr.quarterFbo[0], NULL, NULL, dst, dstBox, NULL, NULL, 0);

			VectorSet4(color, 1, 1, 1, blur - 1.0f);

			FBO_Blit(tr.textureScratchFbo[0], NULL, NULL, dst, dstBox, NULL, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		} else if(blur > 2.0f) {
			// blur 1/16th texture then replace
			int i;

			for(i = 0; i < 2; i++) {
				vec2_t blurTexScale;
				float subblur;

				subblur = ((blur - 2.0f) / 2.0f) / 3.0f * (float)(i + 1);

				blurTexScale[0] = blurTexScale[1] = subblur;

				color[0] = color[1] = color[2] = 0.5f;
				color[3] = 1.0f;

				if(i != 0)
					FBO_Blit(tr.textureScratchFbo[0], NULL, blurTexScale, tr.textureScratchFbo[1], NULL, &tr.bokehShader, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
				else
					FBO_Blit(tr.textureScratchFbo[0], NULL, blurTexScale, tr.textureScratchFbo[1], NULL, &tr.bokehShader, color, 0);
			}

			FBO_Blit(tr.textureScratchFbo[1], NULL, NULL, dst, dstBox, NULL, NULL, 0);
		}
#else  // higher quality blur, but slower
		else if(blur > 1.0f) {
			// blur quarter texture then replace
			int i;

			src = tr.quarterFbo[0];
			dst = tr.quarterFbo[1];

			VectorSet4(color, 0.5f, 0.5f, 0.5f, 1);

			for(i = 0; i < 2; i++) {
				vec2_t blurTexScale;
				float subblur;

				subblur = (blur - 1.0f) / 2.0f * (float)(i + 1);

				blurTexScale[0] = blurTexScale[1] = subblur;

				color[0] = color[1] = color[2] = 1.0f;
				if(i != 0)
					color[3] = 1.0f;
				else
					color[3] = 0.5f;

				FBO_Blit(tr.quarterFbo[0], NULL, blurTexScale, tr.quarterFbo[1], NULL, &tr.bokehShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
			}

			FBO_Blit(tr.quarterFbo[1], NULL, NULL, dst, dstBox, NULL, NULL, 0);
		}
#endif
	}
}

static void RB_RadialBlur(FBO_t* srcFbo, FBO_t* dstFbo, int passes, float stretch, float x, float y, float w, float h, float xcenter, float ycenter, float alpha) {
	ivec4_t srcBox, dstBox;
	int srcWidth, srcHeight;
	vec4_t color;
	const float inc = 1.f / passes;
	const float mul = powf(stretch, inc);
	float scale;

	alpha *= inc;
	VectorSet4(color, alpha, alpha, alpha, 1.0f);

	srcWidth = srcFbo ? srcFbo->width : glConfig.vidWidth;
	srcHeight = srcFbo ? srcFbo->height : glConfig.vidHeight;

	VectorSet4(srcBox, 0, 0, srcWidth, srcHeight);

	VectorSet4(dstBox, x, y, w, h);
	FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, 0);

	--passes;
	scale = mul;
	while(passes > 0) {
		float iscale = 1.f / scale;
		float s0 = xcenter * (1.f - iscale);
		float t0 = (1.0f - ycenter) * (1.f - iscale);

		srcBox[0] = s0 * srcWidth;
		srcBox[1] = t0 * srcHeight;
		srcBox[2] = iscale * srcWidth;
		srcBox[3] = iscale * srcHeight;

		FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

		scale *= mul;
		--passes;
	}
}

static bool RB_UpdateSunFlareVis(void) {
	GLuint sampleCount = 0;
	if(!glRefConfig.occlusionQuery) return true;

	tr.sunFlareQueryIndex ^= 1;
	if(!tr.sunFlareQueryActive[tr.sunFlareQueryIndex]) return true;

	/* debug code */
	if(0) {
		int iter;
		for(iter = 0;; ++iter) {
			GLint available = 0;
			qglGetQueryObjectiv(tr.sunFlareQuery[tr.sunFlareQueryIndex], GL_QUERY_RESULT_AVAILABLE, &available);
			if(available) break;
		}

		ri.Printf(PRINT_DEVELOPER, "Waited %d iterations\n", iter);
	}

	// Note: On desktop OpenGL this is a sample count (glRefConfig.occlusionQueryTarget == GL_SAMPLES_PASSED)
	// but on OpenGL ES this is a boolean (glRefConfig.occlusionQueryTarget == GL_ANY_SAMPLES_PASSED)
	qglGetQueryObjectuiv(tr.sunFlareQuery[tr.sunFlareQueryIndex], GL_QUERY_RESULT, &sampleCount);
	return sampleCount > 0;
}

void RB_SunRays(FBO_t* srcFbo, ivec4_t srcBox, FBO_t* dstFbo, ivec4_t dstBox) {
	vec4_t color;
	float dot;
	const float cutoff = 0.25f;
	bool colorize = true;

	//	float w, h, w2, h2;
	mat4_t mvp;
	vec4_t pos, hpos;

	dot = DotProduct(tr.sunDirection, backEnd.viewParms.or.axis[0]);
	if(dot < cutoff) return;

	if(!RB_UpdateSunFlareVis()) return;

	// From RB_DrawSun()
	{
		float dist;
		mat4_t trans, model;

		Mat4Translation(backEnd.viewParms.or.origin, trans);
		Mat4Multiply(backEnd.viewParms.world.modelMatrix, trans, model);
		Mat4Multiply(backEnd.viewParms.projectionMatrix, model, mvp);

		dist = backEnd.viewParms.zFar / 1.75;  // div sqrt(3)

		VectorScale(tr.sunDirection, dist, pos);
	}

	// project sun point
	// Mat4Multiply(backEnd.viewParms.projectionMatrix, backEnd.viewParms.world.modelMatrix, mvp);
	Mat4Transform(mvp, pos, hpos);

	// transform to UV coords
	hpos[3] = 0.5f / hpos[3];

	pos[0] = 0.5f + hpos[0] * hpos[3];
	pos[1] = 0.5f + hpos[1] * hpos[3];

	// initialize quarter buffers
	{
		float mul = 1.f;
		ivec4_t rayBox, quarterBox;
		int srcWidth = srcFbo ? srcFbo->width : glConfig.vidWidth;
		int srcHeight = srcFbo ? srcFbo->height : glConfig.vidHeight;

		VectorSet4(color, mul, mul, mul, 1);

		rayBox[0] = srcBox[0] * tr.sunRaysFbo->width / srcWidth;
		rayBox[1] = srcBox[1] * tr.sunRaysFbo->height / srcHeight;
		rayBox[2] = srcBox[2] * tr.sunRaysFbo->width / srcWidth;
		rayBox[3] = srcBox[3] * tr.sunRaysFbo->height / srcHeight;

		quarterBox[0] = 0;
		quarterBox[1] = tr.quarterFbo[0]->height;
		quarterBox[2] = tr.quarterFbo[0]->width;
		quarterBox[3] = -tr.quarterFbo[0]->height;

		// first, downsample the framebuffer
		if(colorize) {
			FBO_FastBlit(srcFbo, srcBox, tr.quarterFbo[0], quarterBox, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			FBO_Blit(tr.sunRaysFbo, rayBox, NULL, tr.quarterFbo[0], quarterBox, NULL, color, GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);
		} else {
			FBO_FastBlit(tr.sunRaysFbo, rayBox, tr.quarterFbo[0], quarterBox, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}
	}

	// radial blur passes, ping-ponging between the two quarter-size buffers
	{
		const float stretch_add = 2.f / 3.f;
		float stretch = 1.f + stretch_add;
		int i;
		for(i = 0; i < 2; ++i) {
			RB_RadialBlur(tr.quarterFbo[i & 1], tr.quarterFbo[(~i) & 1], 5, stretch, 0.f, 0.f, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height, pos[0], pos[1], 1.125f);
			stretch += stretch_add;
		}
	}

	// add result back on top of the main buffer
	{
		float mul = 1.f;

		VectorSet4(color, mul, mul, mul, 1);

		FBO_Blit(tr.quarterFbo[0], NULL, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	}
}

static void RB_BlurAxis(FBO_t* srcFbo, FBO_t* dstFbo, float strength, bool horizontal) {
	float dx, dy;
	float xmul, ymul;
	float weights[3] = {
	    0.227027027f,
	    0.316216216f,
	    0.070270270f,
	};
	float offsets[3] = {
	    0.f,
	    1.3846153846f,
	    3.2307692308f,
	};

	xmul = horizontal;
	ymul = 1.f - xmul;

	xmul *= strength;
	ymul *= strength;

	{
		ivec4_t srcBox, dstBox;
		vec4_t color;

		VectorSet4(color, weights[0], weights[0], weights[0], 1.0f);
		VectorSet4(srcBox, 0, 0, srcFbo->width, srcFbo->height);
		VectorSet4(dstBox, 0, 0, dstFbo->width, dstFbo->height);
		FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, 0);

		VectorSet4(color, weights[1], weights[1], weights[1], 1.0f);
		dx = offsets[1] * xmul;
		dy = offsets[1] * ymul;
		VectorSet4(srcBox, dx, dy, srcFbo->width, srcFbo->height);
		FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
		VectorSet4(srcBox, -dx, -dy, srcFbo->width, srcFbo->height);
		FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

		VectorSet4(color, weights[2], weights[2], weights[2], 1.0f);
		dx = offsets[2] * xmul;
		dy = offsets[2] * ymul;
		VectorSet4(srcBox, dx, dy, srcFbo->width, srcFbo->height);
		FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
		VectorSet4(srcBox, -dx, -dy, srcFbo->width, srcFbo->height);
		FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	}
}

static void RB_HBlur(FBO_t* srcFbo, FBO_t* dstFbo, float strength) { RB_BlurAxis(srcFbo, dstFbo, strength, true); }

static void RB_VBlur(FBO_t* srcFbo, FBO_t* dstFbo, float strength) { RB_BlurAxis(srcFbo, dstFbo, strength, false); }

void RB_Gamma(FBO_t* srcFbo, ivec4_t srcBox, FBO_t* dstFbo, ivec4_t dstBox) {
	vec4_t color;

	VectorSet4(color, 1.0f, 1.0f, 1.0f, 1.0f);

	GLSL_BindProgram(&tr.gammaShader);
	GLSL_SetUniformFloat(&tr.gammaShader, UNIFORM_GAMMA, r_gamma->value);

	FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, &tr.gammaShader, color, 0);
}

void RB_Bloom(FBO_t* src, ivec4_t srcBox, FBO_t* dst, ivec4_t dstBox) {
	vec4_t color;
	float blurStrength;

	if(!r_bloom->integer || !tr.bloomFbo[0] || !tr.bloomFbo[1]) return;
	if(r_bloomIntensity->value <= 0.0f) return;

	// 1) Bright-pass: только яркие пиксели, попадают в tr.bloomFbo[0]
	GLSL_SetUniformFloat(&tr.bloomShader, UNIFORM_BLOOMTHRESHOLD, r_bloomThreshold->value);
	GLSL_SetUniformFloat(&tr.bloomShader, UNIFORM_BLOOMKNEE, Com_Clamp(0.0001f, 1.0f, r_bloomKnee->value));

	VectorSet4(color, 1.0f, 1.0f, 1.0f, 1.0f);
	FBO_Blit(src, srcBox, NULL, tr.bloomFbo[0], NULL, &tr.bloomShader, color, 0);

	// 2) Разделимый гауссовский блюр: H → bloomFbo[1], V → обратно в bloomFbo[0]
	blurStrength = Com_Clamp(0.0f, 4.0f, r_bloomBlur->value);
	if(blurStrength > 0.0f) {
		RB_HBlur(tr.bloomFbo[0], tr.bloomFbo[1], blurStrength);
		RB_VBlur(tr.bloomFbo[1], tr.bloomFbo[0], blurStrength);
	}

	// 3) Аддитивная композиция обратно в целевую FBO
	color[0] = color[1] = color[2] = r_bloomIntensity->value;
	color[3] = 1.0f;

	FBO_Blit(tr.bloomFbo[0], NULL, NULL, dst, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
}

/*
===============================================================================
LUT color correction (r_lutFile + .cube files from lut/ folder)

The LUT is loaded as a 3D texture with GL_LINEAR filtering, so sampling in
the shader does hardware trilinear interpolation. The colour pipeline runs
after tone mapping, so the LUT gets LDR (0..1) colour, which is exactly what
.cube files expect.
===============================================================================
*/

static char s_loadedLUTName[MAX_QEXTENDEDPATH] = {0};

static void RB_LoadLUTFile(const char* name) {
	if(tr.lutImage) {
		qglDeleteTextures(1, &tr.lutImage->texnum);
		tr.lutImage = NULL;
	}

	if(!name || !name[0]) {
		return;
	}

	char fullpath[MAX_QEXTENDEDPATH];
	Com_sprintf(fullpath, sizeof(fullpath), "lut/%s", name);

	byte* buffer = NULL;
	int len = ri.FS_ReadFile(fullpath, (void**)&buffer);

	if(len <= 0 || !buffer) {
		ri.Printf(PRINT_WARNING, "RB_LUT: couldn't load '%s'\n", fullpath);
		return;
	}

	int lutSize = 0;
	float* data = NULL;
	int dataCount = 0;
	int dataAlloc = 0;
	bool failed = false;

	char* p = (char*)buffer;
	char* end = (char*)buffer + len;
	char line[512];

	while(p < end) {
		char* lineStart = p;

		while(p < end && *p != '\n' && *p != '\r') {
			p++;
		}

		int lineLen = (int)(p - lineStart);

		if(lineLen >= (int)sizeof(line)) {
			lineLen = (int)sizeof(line) - 1;
		}

		Com_Memcpy(line, lineStart, lineLen);
		line[lineLen] = '\0';

		while(p < end && (*p == '\n' || *p == '\r')) {
			p++;
		}

		char* s = line;

		while(*s == ' ' || *s == '\t') {
			s++;
		}

		if(!*s || *s == '#') {
			continue;
		}

		if(!Q_stricmpn(s, "TITLE", 5)) {
			continue;
		}

		if(!Q_stricmpn(s, "DOMAIN_MIN", 10)) {
			continue;
		}

		if(!Q_stricmpn(s, "DOMAIN_MAX", 10)) {
			continue;
		}

		if(!Q_stricmpn(s, "LUT_1D_SIZE", 11)) {
			ri.Printf(PRINT_WARNING, "RB_LUT: 1D LUTs are not supported ('%s')\n", fullpath);
			failed = true;
			break;
		}

		if(!Q_stricmpn(s, "LUT_3D_SIZE", 11)) {
			lutSize = atoi(s + 11);

			if(lutSize < 2 || lutSize > 256) {
				ri.Printf(PRINT_WARNING, "RB_LUT: invalid LUT_3D_SIZE %d in '%s'\n", lutSize, fullpath);
				failed = true;
				break;
			}

			dataAlloc = lutSize * lutSize * lutSize * 3;
			data = ri.Malloc(dataAlloc * sizeof(float));
			continue;
		}

		if(lutSize > 0 && data) {
			float r, g, b;

			if(sscanf(s, "%f %f %f", &r, &g, &b) == 3) {
				if(dataCount + 3 <= dataAlloc) {
					data[dataCount++] = r;
					data[dataCount++] = g;
					data[dataCount++] = b;
				}
			}
		}
	}

	if(!failed && (!lutSize || !data || dataCount != dataAlloc)) {
		ri.Printf(PRINT_WARNING, "RB_LUT: '%s' parsed %d of %d expected values\n", fullpath, dataCount, dataAlloc);
		failed = true;
	}

	if(failed) {
		if(data) {
			ri.Free(data);
		}

		ri.FS_FreeFile(buffer);
		return;
	}

	tr.lutImage = ri.Hunk_Alloc(sizeof(image_t), h_low);
	Com_Memset(tr.lutImage, 0, sizeof(*tr.lutImage));

	qglGenTextures(1, &tr.lutImage->texnum);

	qglActiveTexture(GL_TEXTURE0 + TB_LIGHTMAP);
	qglBindTexture(GL_TEXTURE_3D, tr.lutImage->texnum);
	qglTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, lutSize, lutSize, lutSize, 0, GL_RGB, GL_FLOAT, data);
	qglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	qglActiveTexture(GL_TEXTURE0);

	Q_strncpyz(tr.lutImage->imgName, fullpath, sizeof(tr.lutImage->imgName));
	tr.lutImage->width = tr.lutImage->uploadWidth = lutSize;
	tr.lutImage->height = tr.lutImage->uploadHeight = lutSize;
	tr.lutImage->flags |= IMGFLAG_NO_COMPRESSION;

	ri.Printf(PRINT_ALL, "RB_LUT: loaded '%s' (%dx%dx%d)\n", fullpath, lutSize, lutSize, lutSize);

	ri.Free(data);
	ri.FS_FreeFile(buffer);
}

void RB_LUT(FBO_t* src, ivec4_t srcBox, FBO_t* dst, ivec4_t dstBox) {
	vec4_t color;

	if(!r_lutFile || !r_lutFile->string[0]) {
		if(s_loadedLUTName[0]) {
			RB_LoadLUTFile("");
			s_loadedLUTName[0] = '\0';
		}

		return;
	}

	if(!tr.lutShader.program) {
		return;
	}

	if(Q_stricmp(s_loadedLUTName, r_lutFile->string) != 0) {
		RB_LoadLUTFile(r_lutFile->string);
		Q_strncpyz(s_loadedLUTName, r_lutFile->string, sizeof(s_loadedLUTName));
	}

	if(!tr.lutImage) {
		return;
	}

	GL_BindMultiTexture(GL_TEXTURE0 + TB_LIGHTMAP, GL_TEXTURE_3D, tr.lutImage->texnum);

	GLSL_SetUniformInt(&tr.lutShader, UNIFORM_LUTMAP, TB_LIGHTMAP);

	VectorSet4(color, 1.0f, 1.0f, 1.0f, 1.0f);

	FBO_Blit(src, srcBox, NULL, dst, dstBox, &tr.lutShader, color, 0);
}

void RB_MovementBlur(FBO_t* src, ivec4_t srcBox, FBO_t* dst, ivec4_t dstBox) {
	vec4_t color;
	float strength;

	if(!r_movementBlur->integer) {
		return;
	}

	if(!tr.movementBlurFbo || !tr.movementBlurShader.program) {
		return;
	}

	strength = Com_Clamp(-1.0f, 1.0f, r_movementBlurStrength->value);
	if(fabsf(strength) < 1e-6f) {
		return;
	}

	GLSL_SetUniformFloat(&tr.movementBlurShader, UNIFORM_MOVEMENTBLURSTRENGTH, strength);

	VectorSet4(color, 1.0f, 1.0f, 1.0f, 1.0f);

	FBO_Blit(src, srcBox, NULL, tr.movementBlurFbo, NULL, &tr.movementBlurShader, color, 0);

	FBO_Blit(tr.movementBlurFbo, NULL, NULL, dst, dstBox, NULL, color, 0);
}

void RB_CameraMotionBlur(FBO_t* src, ivec4_t srcBox, FBO_t* dst, ivec4_t dstBox) {
	vec4_t color;
	vec2_t blurDir;
	float yawDelta = 0.0f, pitchDelta = 0.0f;
	float strength;

	if(!r_cameraBlur->integer || !tr.cameraBlurShader.program) {
		return;
	}

	if(!tr.movementBlurFbo) {
		return;
	}

	strength = r_cameraBlurStrength->value;
	if(strength == 0.0f) {
		return;
	}

	if(tr.lastViewAxisValid) {
		vec3_t prevFwd, curFwd;
		float prevYaw, curYaw, prevPitch, curPitch;

		VectorCopy(tr.lastViewAxis[0], prevFwd);
		VectorCopy(backEnd.refdef.viewaxis[0], curFwd);

		// yaw в горизонтальной плоскости XY
		prevYaw = atan2f(prevFwd[1], prevFwd[0]);
		curYaw = atan2f(curFwd[1], curFwd[0]);
		yawDelta = curYaw - prevYaw;
		if(yawDelta > M_PI) yawDelta -= 2.0f * M_PI;
		if(yawDelta < -M_PI) yawDelta += 2.0f * M_PI;

		// pitch = угол над горизонтом
		prevPitch = asinf(Com_Clamp(-1.0f, 1.0f, prevFwd[2]));
		curPitch = asinf(Com_Clamp(-1.0f, 1.0f, curFwd[2]));
		pitchDelta = curPitch - prevPitch;

		// телепорт / скачок камеры — не размазываем
		if(fabsf(yawDelta) > 1.0f || fabsf(pitchDelta) > 1.0f) {
			yawDelta = 0.0f;
			pitchDelta = 0.0f;
		} else {
			// мягкий потолок для слишком быстрых фликов
			const float softCap = 0.15f;
			if(yawDelta > softCap) yawDelta = softCap;
			if(yawDelta < -softCap) yawDelta = -softCap;
			if(pitchDelta > softCap) pitchDelta = softCap;
			if(pitchDelta < -softCap) pitchDelta = -softCap;
		}
	}

	// запоминаем текущий axis для следующего кадра
	VectorCopy(backEnd.refdef.viewaxis[0], tr.lastViewAxis[0]);
	VectorCopy(backEnd.refdef.viewaxis[1], tr.lastViewAxis[1]);
	VectorCopy(backEnd.refdef.viewaxis[2], tr.lastViewAxis[2]);
	tr.lastViewAxisValid = true;

	if(fabsf(yawDelta) < 0.0001f && fabsf(pitchDelta) < 0.0001f) {
		return;
	}

	blurDir[0] = -yawDelta * strength * 0.12f;
	blurDir[1] = pitchDelta * strength * 0.12f;

	GLSL_SetUniformVec2(&tr.cameraBlurShader, UNIFORM_CAMERABLURDIR, blurDir);

	if(tr.renderDepthImage) {
		GL_BindToTMU(tr.renderDepthImage, TB_LIGHTMAP);
		GLSL_SetUniformFloat(&tr.cameraBlurShader, UNIFORM_CAMERABLURNEARTHRESHOLD, r_cameraBlurNearThreshold->value);
		GLSL_SetUniformFloat(&tr.cameraBlurShader, UNIFORM_CAMERABLURNEARDSOFTNESS, r_cameraBlurNearSoftness->value);
	} else {
		GLSL_SetUniformFloat(&tr.cameraBlurShader, UNIFORM_CAMERABLURNEARTHRESHOLD, 0.0f);
		GLSL_SetUniformFloat(&tr.cameraBlurShader, UNIFORM_CAMERABLURNEARDSOFTNESS, 0.0f);
	}

	{
		vec4_t srcBounds;
		srcBounds[0] = srcBox[0] / (float)src->width;
		srcBounds[1] = srcBox[1] / (float)src->height;
		srcBounds[2] = (srcBox[0] + srcBox[2]) / (float)src->width;
		srcBounds[3] = (srcBox[1] + srcBox[3]) / (float)src->height;
		GLSL_SetUniformVec4(&tr.cameraBlurShader, UNIFORM_CAMERABLURSRCBOUNDS, srcBounds);
	}

	VectorSet4(color, 1.0f, 1.0f, 1.0f, 1.0f);

	FBO_Blit(src, srcBox, NULL, tr.movementBlurFbo, NULL, &tr.cameraBlurShader, color, 0);
	FBO_Blit(tr.movementBlurFbo, NULL, NULL, dst, dstBox, NULL, color, 0);
}
