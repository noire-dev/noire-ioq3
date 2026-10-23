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
// tr_init.c -- functions that are not called every frame

#include "tr_local.h"

#include "tr_dsa.h"

glconfig_t glConfig;
glRefConfig_t glRefConfig;
bool textureFilterAnisotropic = false;
int maxAnisotropy = 0;
float displayAspect = 0.0f;
bool haveClampToEdge = false;

glstate_t glState;

static void GfxInfo_f(void);
static void GfxMemInfo_f(void);

#ifdef USE_RENDERER_DLOPEN
cvar_t* com_altivec;
#endif

cvar_t* r_ignore;

cvar_t* r_dynamiclight;

cvar_t* r_lodbias;

cvar_t* r_speeds;

cvar_t* r_ext_texture_filter_anisotropic;
cvar_t* r_ext_max_anisotropy;

cvar_t* r_ext_framebuffer_multisample;

cvar_t* r_cameraExposure;

cvar_t* r_externalGLSL;

cvar_t* r_toneMapMin;
cvar_t* r_toneMapAvg;
cvar_t* r_toneMapMax;

cvar_t* r_autoExposure;
cvar_t* r_autoExposureMin;
cvar_t* r_autoExposureMax;

cvar_t* r_parallaxMapping;
cvar_t* r_parallaxMapShadows;
cvar_t* r_reflections;
cvar_t* r_reflectionsResolution;
cvar_t* r_reflectionsGrid;
cvar_t* r_reflectionsEnd;
cvar_t* r_reflectionsDetail;
cvar_t* r_deluxeSpecular;
cvar_t* r_detailLevel;
cvar_t* r_shadows;
cvar_t* r_shadowsUpdate;
cvar_t* r_shadowResolution;
cvar_t* r_shadowDistance;
cvar_t* r_shadowLightScale;
cvar_t* r_shadowAmbientScale;
cvar_t* r_shadowFilter;

cvar_t* r_fogDensity;
cvar_t* r_fogColorR;
cvar_t* r_fogColorG;
cvar_t* r_fogColorB;
cvar_t* r_fogStart;
cvar_t* r_fogEnd;

cvar_t* r_skyColorR;
cvar_t* r_skyColorG;
cvar_t* r_skyColorB;

cvar_t* r_resolution;
cvar_t* r_picmip;
cvar_t* r_showtris;
cvar_t* r_swapInterval;
cvar_t* r_textureMode;
cvar_t* r_gamma;
cvar_t* r_lockpvs;

cvar_t* r_subdivisions;

cvar_t* r_fullscreen;
cvar_t* r_noborder;

cvar_t* r_ambientScale;
cvar_t* r_directedScale;
cvar_t* r_debugSort;

int max_polys;
int max_polyverts;

/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL(void) {
	//
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_resolution
	//		- r_(color|depth|stencil)bits
	//

	if(glConfig.vidWidth == 0) {
		GLint temp;

		GLimp_Init(false);
		GLimp_InitExtraExtensions();

		glConfig.textureEnvAddAvailable = true;

		// OpenGL driver constants
		qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &temp);
		glConfig.maxTextureSize = temp;

		// stubbed or broken drivers may have reported 0...
		if(glConfig.maxTextureSize <= 0) {
			glConfig.maxTextureSize = 0;
		}

		qglGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &temp);
		glConfig.numTextureUnits = temp;

		qglGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &temp);
		glRefConfig.maxVertexAttribs = temp;

		// reserve 160 components for other uniforms
		if(qglesMajorVersion) {
			qglGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &temp);
			temp *= 4;
		} else {
			qglGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &temp);
		}
		glRefConfig.glslMaxAnimatedBones = Com_Clamp(0, IQM_MAX_JOINTS, (temp - 160) / 16);
		if(glRefConfig.glslMaxAnimatedBones < 12) {
			glRefConfig.glslMaxAnimatedBones = 0;
		}
	}

	// check for GLSL function textureCubeLod()
	if(r_reflections->integer && !QGL_VERSION_ATLEAST(3, 0)) {
		ri.Printf(PRINT_WARNING, "WARNING: Disabled r_reflections because it requires OpenGL 3.0\n");
		ri.Cvar_Set("r_reflections", "0");
	}

	// set default state
	GL_SetDefaultState();
}

/*
==============================================================================

                        SCREEN SHOTS

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homedatapath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

==============================================================================
*/

/*
==================
RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.

Reads a minimum offset for where the RGB data starts in the image from
integer stored at pointer offset. When the function has returned the actual
offset was written back to address offset. This address will always have an
alignment of packAlign to ensure efficient copying.

Stores the length of padding after a line of pixels to address padlen

Return value must be freed with ri.Hunk_FreeTempMemory()
==================
*/

byte* RB_ReadPixels(int x, int y, int width, int height, size_t* offset, int* padlen) {
	byte *buffer, *bufstart;
	int padwidth, linelen, bytesPerPixel;
	int yin, xin, xout;
	GLint packAlign, format;

	// OpenGL ES is only required to support reading GL_RGBA
	if(qglesMajorVersion >= 1) {
		format = GL_RGBA;
		bytesPerPixel = 4;
	} else {
		format = GL_RGB;
		bytesPerPixel = 3;
	}

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen = width * bytesPerPixel;
	padwidth = PAD(linelen, packAlign);

	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = ri.Hunk_AllocateTempMemory(padwidth * height + *offset + packAlign - 1);

	bufstart = PADP((intptr_t)buffer + *offset, packAlign);
	qglReadPixels(x, y, width, height, format, GL_UNSIGNED_BYTE, bufstart);

	linelen = width * 3;

	// Convert RGBA to RGB, in place, line by line
	if(format == GL_RGBA) {
		for(yin = 0; yin < height; yin++) {
			for(xin = 0, xout = 0; xout < linelen; xin += 4, xout += 3) {
				bufstart[yin * padwidth + xout + 0] = bufstart[yin * padwidth + xin + 0];
				bufstart[yin * padwidth + xout + 1] = bufstart[yin * padwidth + xin + 1];
				bufstart[yin * padwidth + xout + 2] = bufstart[yin * padwidth + xin + 2];
			}
		}
	}

	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;

	return buffer;
}

/*
==================
RB_TakeScreenshot
==================
*/
void RB_TakeScreenshot(int x, int y, int width, int height, char* fileName) {
	byte *allbuf, *buffer;
	byte *srcptr, *destptr;
	byte *endline, *endmem;
	byte temp;

	int linelen, padlen;
	size_t offset = 18, memcount;

	allbuf = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	buffer = allbuf + offset - 18;

	Com_Memset(buffer, 0, 18);
	buffer[2] = 2;  // uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;  // pixel size

	// swap rgb to bgr and remove padding from line endings
	linelen = width * 3;

	srcptr = destptr = allbuf + offset;
	endmem = srcptr + (linelen + padlen) * height;

	while(srcptr < endmem) {
		endline = srcptr + linelen;

		while(srcptr < endline) {
			temp = srcptr[0];
			*destptr++ = srcptr[2];
			*destptr++ = srcptr[1];
			*destptr++ = temp;

			srcptr += 3;
		}

		// Skip the pad
		srcptr += padlen;
	}

	memcount = linelen * height;

	ri.FS_WriteFile(fileName, buffer, memcount + 18);

	ri.Hunk_FreeTempMemory(allbuf);
}

/*
==================
RB_TakeScreenshotJPEG
==================
*/

void RB_TakeScreenshotJPEG(int x, int y, int width, int height, char* fileName) {
	byte* buffer;
	size_t offset = 0;
	int padlen;

	buffer = RB_ReadPixels(x, y, width, height, &offset, &padlen);

	RE_SaveJPG(fileName, 100, width, height, buffer + offset, padlen);
	ri.Hunk_FreeTempMemory(buffer);
}

/*
==================
RB_TakeScreenshotCmd
==================
*/
const void* RB_TakeScreenshotCmd(const void* data) {
	const screenshotCommand_t* cmd;

	cmd = (const screenshotCommand_t*)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes) RB_EndSurface();

	if(cmd->jpeg)
		RB_TakeScreenshotJPEG(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	else
		RB_TakeScreenshot(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);

	return (const void*)(cmd + 1);
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot(int x, int y, int width, int height, char* name, bool jpeg) {
	static char fileName[MAX_OSPATH];  // bad things if two screenshots per frame?
	screenshotCommand_t* cmd;

	cmd = R_GetCommandBuffer(sizeof(*cmd));
	if(!cmd) {
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	Q_strncpyz(fileName, name, sizeof(fileName));
	cmd->fileName = fileName;
	cmd->jpeg = jpeg;
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilename(int lastNumber, char* fileName) {
	int a, b, c, d;

	if(lastNumber < 0 || lastNumber > 9999) {
		Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot9999.tga");
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a * 1000;
	b = lastNumber / 100;
	lastNumber -= b * 100;
	c = lastNumber / 10;
	lastNumber -= c * 10;
	d = lastNumber;

	Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga", a, b, c, d);
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilenameJPEG(int lastNumber, char* fileName) {
	int a, b, c, d;

	if(lastNumber < 0 || lastNumber > 9999) {
		Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot9999.jpg");
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a * 1000;
	b = lastNumber / 100;
	lastNumber -= b * 100;
	c = lastNumber / 10;
	lastNumber -= c * 10;
	d = lastNumber;

	Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg", a, b, c, d);
}

/*
==================
R_ScreenShot_f

screenshot
screenshot [silent]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
==================
*/
void R_ScreenShot_f(void) {
	char checkname[MAX_OSPATH];
	static int lastNumber = -1;
	bool silent;

	if(!strcmp(ri.Cmd_Argv(1), "silent")) {
		silent = true;
	} else {
		silent = false;
	}

	if(ri.Cmd_Argc() == 2 && !silent) {
		// explicit filename
		Com_sprintf(checkname, MAX_OSPATH, "screenshots/%s.tga", ri.Cmd_Argv(1));
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if(lastNumber == -1) {
			lastNumber = 0;
		}
		// scan for a free number
		for(; lastNumber <= 9999; lastNumber++) {
			R_ScreenshotFilename(lastNumber, checkname);

			if(!ri.FS_FileExists(checkname)) {
				break;  // file doesn't exist
			}
		}

		if(lastNumber >= 9999) {
			ri.Printf(PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}

	R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, false);

	if(!silent) {
		ri.Printf(PRINT_ALL, "Wrote %s\n", checkname);
	}
}

void R_ScreenShotJPEG_f(void) {
	char checkname[MAX_OSPATH];
	static int lastNumber = -1;
	bool silent;

	if(!strcmp(ri.Cmd_Argv(1), "silent")) {
		silent = true;
	} else {
		silent = false;
	}

	if(ri.Cmd_Argc() == 2 && !silent) {
		// explicit filename
		Com_sprintf(checkname, MAX_OSPATH, "screenshots/%s.jpg", ri.Cmd_Argv(1));
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if(lastNumber == -1) {
			lastNumber = 0;
		}
		// scan for a free number
		for(; lastNumber <= 9999; lastNumber++) {
			R_ScreenshotFilenameJPEG(lastNumber, checkname);

			if(!ri.FS_FileExists(checkname)) {
				break;  // file doesn't exist
			}
		}

		if(lastNumber == 10000) {
			ri.Printf(PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}

	R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, true);

	if(!silent) {
		ri.Printf(PRINT_ALL, "Wrote %s\n", checkname);
	}
}

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void* RB_TakeVideoFrameCmd(const void* data) {
	const videoFrameCommand_t* cmd;
	byte* cBuf;
	size_t memcount, bytesPerPixel, linelen, avilinelen;
	int padwidth, avipadwidth, padlen, avipadlen;
	int yin, xin, xout;
	GLint packAlign, format;

	// finish any 2D drawing if needed
	if(tess.numIndexes) RB_EndSurface();

	cmd = (const videoFrameCommand_t*)data;

	// OpenGL ES is only required to support reading GL_RGBA
	if(qglesMajorVersion >= 1) {
		format = GL_RGBA;
		bytesPerPixel = 4;
	} else {
		format = GL_RGB;
		bytesPerPixel = 3;
	}

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen = cmd->width * bytesPerPixel;

	// Alignment stuff for glReadPixels
	padwidth = PAD(linelen, packAlign);
	padlen = padwidth - linelen;

	avilinelen = cmd->width * 3;

	// AVI line padding
	avipadwidth = PAD(avilinelen, AVI_LINE_PADDING);
	avipadlen = avipadwidth - avilinelen;

	cBuf = PADP(cmd->captureBuffer, packAlign);

	qglReadPixels(0, 0, cmd->width, cmd->height, format, GL_UNSIGNED_BYTE, cBuf);

	memcount = padwidth * cmd->height;

	if(cmd->motionJpeg) {
		// Convert RGBA to RGB, in place, line by line
		if(format == GL_RGBA) {
			linelen = cmd->width * 3;
			padlen = padwidth - linelen;

			for(yin = 0; yin < cmd->height; yin++) {
				for(xin = 0, xout = 0; xout < linelen; xin += 4, xout += 3) {
					cBuf[yin * padwidth + xout + 0] = cBuf[yin * padwidth + xin + 0];
					cBuf[yin * padwidth + xout + 1] = cBuf[yin * padwidth + xin + 1];
					cBuf[yin * padwidth + xout + 2] = cBuf[yin * padwidth + xin + 2];
				}
			}
		}

		memcount = RE_SaveJPGToBuffer(cmd->encodeBuffer, avilinelen * cmd->height, 100, cmd->width, cmd->height, cBuf, padlen);
		ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, memcount);
	} else {
		byte *lineend, *memend;
		byte *srcptr, *destptr;

		srcptr = cBuf;
		destptr = cmd->encodeBuffer;
		memend = srcptr + memcount;

		// swap R and B and remove line paddings
		while(srcptr < memend) {
			lineend = srcptr + linelen;
			while(srcptr < lineend) {
				*destptr++ = srcptr[2];
				*destptr++ = srcptr[1];
				*destptr++ = srcptr[0];
				srcptr += bytesPerPixel;
			}

			Com_Memset(destptr, '\0', avipadlen);
			destptr += avipadlen;

			srcptr += padlen;
		}

		ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, avipadwidth * cmd->height);
	}

	return (const void*)(cmd + 1);
}

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState(void) {
	qglClearDepth(1.0f);

	qglCullFace(GL_FRONT);

	GL_BindNullTextures();

	if(glRefConfig.framebufferObject) GL_BindNullFramebuffers();

	GL_TextureMode(r_textureMode->string);

	// qglShadeModel( GL_SMOOTH );
	qglDepthFunc(GL_LEQUAL);

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;
	glState.storedGlState = 0;
	glState.faceCulling = CT_TWO_SIDED;
	glState.faceCullFront = true;

	GL_BindNullProgram();

	if(glRefConfig.vertexArrayObject) qglBindVertexArray(0);

	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glState.currentVao = NULL;
	glState.vertexAttribsEnabled = 0;

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask(GL_TRUE);
	qglDisable(GL_DEPTH_TEST);
	qglEnable(GL_SCISSOR_TEST);
	qglDisable(GL_CULL_FACE);
	qglDisable(GL_BLEND);

	if(glRefConfig.seamlessCubeMap) qglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	// GL_POLYGON_OFFSET_FILL will be glEnable()d when this is used
	qglPolygonOffset(-1, -2);

	qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // FIXME: get color of sky
}

/*
================
R_PrintLongString

Workaround for ri.Printf's 1024 characters buffer limit.
================
*/
void R_PrintLongString(const char* string) {
	char buffer[1024];
	const char* p;
	int size = strlen(string);

	p = string;
	while(size > 0) {
		Q_strncpyz(buffer, p, sizeof(buffer));
		ri.Printf(PRINT_ALL, "%s", buffer);
		p += 1023;
		size -= 1023;
	}
}

/*
================
GfxInfo_f
================
*/
void GfxInfo_f(void) {
	const char* enablestrings[] = {"disabled", "enabled"};
	const char* fsstrings[] = {"windowed", "fullscreen"};

	ri.Printf(PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string);
	ri.Printf(PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string);
	ri.Printf(PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string);
	ri.Printf(PRINT_ALL, "GL_EXTENSIONS: ");
	// glConfig.extensions_string is a limited length so get the full list directly
	if(qglGetStringi) {
		GLint numExtensions;
		int i;

		qglGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
		for(i = 0; i < numExtensions; i++) {
			ri.Printf(PRINT_ALL, "%s ", qglGetStringi(GL_EXTENSIONS, i));
		}
	} else {
		R_PrintLongString((char*)qglGetString(GL_EXTENSIONS));
	}
	ri.Printf(PRINT_ALL, "\n");
	ri.Printf(PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize);
	ri.Printf(PRINT_ALL, "GL_MAX_TEXTURE_IMAGE_UNITS: %d\n", glConfig.numTextureUnits);
	ri.Printf(PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);
	ri.Printf(PRINT_ALL, "MODE: %d x %d %s hz:", glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1]);
	if(glConfig.displayFrequency) {
		ri.Printf(PRINT_ALL, "%d\n", glConfig.displayFrequency);
	} else {
		ri.Printf(PRINT_ALL, "N/A\n");
	}

	ri.Printf(PRINT_ALL, "texturemode: %s\n", r_textureMode->string);
	ri.Printf(PRINT_ALL, "picmip: %d\n", r_picmip->integer);
	ri.Printf(PRINT_ALL, "texture bits: %d\n", 32);
	ri.Printf(PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0]);
	ri.Printf(PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression != TC_NONE]);
}

/*
================
GfxMemInfo_f
================
*/
void GfxMemInfo_f(void) {
	switch(glRefConfig.memInfo) {
		case MI_NONE: {
			ri.Printf(PRINT_ALL, "No extension found for GPU memory info.\n");
		} break;
		case MI_NVX: {
			int value;

			qglGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &value);
			ri.Printf(PRINT_ALL, "GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX: %ikb\n", value);

			qglGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &value);
			ri.Printf(PRINT_ALL, "GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX: %ikb\n", value);

			qglGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &value);
			ri.Printf(PRINT_ALL, "GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX: %ikb\n", value);

			qglGetIntegerv(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX, &value);
			ri.Printf(PRINT_ALL, "GPU_MEMORY_INFO_EVICTION_COUNT_NVX: %i\n", value);

			qglGetIntegerv(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, &value);
			ri.Printf(PRINT_ALL, "GPU_MEMORY_INFO_EVICTED_MEMORY_NVX: %ikb\n", value);
		} break;
		case MI_ATI: {
			// GL_ATI_meminfo
			int value[4];

			qglGetIntegerv(GL_VBO_FREE_MEMORY_ATI, &value[0]);
			ri.Printf(PRINT_ALL, "VBO_FREE_MEMORY_ATI: %ikb total %ikb largest aux: %ikb total %ikb largest\n", value[0], value[1], value[2], value[3]);

			qglGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, &value[0]);
			ri.Printf(PRINT_ALL, "TEXTURE_FREE_MEMORY_ATI: %ikb total %ikb largest aux: %ikb total %ikb largest\n", value[0], value[1], value[2], value[3]);

			qglGetIntegerv(GL_RENDERBUFFER_FREE_MEMORY_ATI, &value[0]);
			ri.Printf(PRINT_ALL, "RENDERBUFFER_FREE_MEMORY_ATI: %ikb total %ikb largest aux: %ikb total %ikb largest\n", value[0], value[1], value[2], value[3]);
		} break;
	}
}

/*
===============
R_Register
===============
*/
void R_Register(void) {
#ifdef USE_RENDERER_DLOPEN
	com_altivec = ri.Cvar_Get("com_altivec", "1", CVAR_ARCHIVE);
#endif

	// latched and archived variables
	r_ext_framebuffer_multisample = ri.Cvar_Get("r_ext_framebuffer_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH);

	r_ext_texture_filter_anisotropic = ri.Cvar_Get("r_ext_texture_filter_anisotropic", "16", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_max_anisotropy = ri.Cvar_Get("r_ext_max_anisotropy", "16", CVAR_ARCHIVE | CVAR_LATCH);

	r_picmip = ri.Cvar_Get("r_picmip", "0", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_CheckRange(r_picmip, 0, 16, true);
	r_resolution = ri.Cvar_Get("r_resolution", "640x480", CVAR_ARCHIVE | CVAR_LATCH);
	r_fullscreen = ri.Cvar_Get("r_fullscreen", "2", CVAR_ARCHIVE);
	r_noborder = ri.Cvar_Get("r_noborder", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_subdivisions = ri.Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);

	r_externalGLSL = ri.Cvar_Get("r_externalGLSL", "0", CVAR_LATCH);

	r_toneMapMin = ri.Cvar_Get("r_toneMapMin", "-8.00", CVAR_CHEAT);
	r_toneMapAvg = ri.Cvar_Get("r_toneMapAvg", "-2.48", CVAR_CHEAT);
	r_toneMapMax = ri.Cvar_Get("r_toneMapMax", "-1.00", CVAR_CHEAT);

	r_autoExposure = ri.Cvar_Get("r_autoExposure", "1", CVAR_ARCHIVE);
	r_autoExposureMin = ri.Cvar_Get("r_autoExposureMin", "-2.0", CVAR_CHEAT);
	r_autoExposureMax = ri.Cvar_Get("r_autoExposureMax", "2.0", CVAR_CHEAT);
	r_cameraExposure = ri.Cvar_Get("r_cameraExposure", "1.08", CVAR_CHEAT);

	r_parallaxMapping = ri.Cvar_Get("r_parallaxMapping", "2", CVAR_ARCHIVE | CVAR_LATCH);
	r_parallaxMapShadows = ri.Cvar_Get("r_parallaxMapShadows", "1", CVAR_ARCHIVE | CVAR_LATCH);

	r_reflections = ri.Cvar_Get("r_reflections", "3", CVAR_ARCHIVE | CVAR_LATCH);
	r_reflectionsResolution = ri.Cvar_Get("r_reflectionsResolution", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	r_reflectionsGrid = ri.Cvar_Get("r_reflectionsGrid", "64", CVAR_ARCHIVE | CVAR_LATCH);
	r_reflectionsEnd = ri.Cvar_Get("r_reflectionsEnd", "16384", CVAR_ARCHIVE);
	r_reflectionsDetail = ri.Cvar_Get("r_reflectionsDetail", "5", CVAR_ARCHIVE);

	r_deluxeSpecular = ri.Cvar_Get("r_deluxeSpecular", "0.3", CVAR_ARCHIVE | CVAR_LATCH);
	r_detailLevel = ri.Cvar_Get("r_detailLevel", "5", CVAR_ARCHIVE);
	ri.Cvar_CheckRange(r_detailLevel, 0, 5, true);

	r_shadows = ri.Cvar_Get("r_shadows", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_shadowsUpdate = ri.Cvar_Get("r_shadowsUpdate", "5", CVAR_ARCHIVE | CVAR_LATCH);
	r_shadowResolution = ri.Cvar_Get("r_shadowResolution", "4096", CVAR_ARCHIVE | CVAR_LATCH);
	r_shadowDistance = ri.Cvar_Get("r_shadowDistance", "8192", CVAR_ARCHIVE | CVAR_LATCH);
	r_shadowLightScale = ri.Cvar_Get("r_shadowLightScale", "0.50", CVAR_ARCHIVE);
	r_shadowAmbientScale = ri.Cvar_Get("r_shadowAmbientScale", "0.32", CVAR_ARCHIVE);
	r_shadowFilter = ri.Cvar_Get("r_shadowFilter", "1", CVAR_ARCHIVE | CVAR_LATCH);

	r_fogDensity = ri.Cvar_Get("r_fogDensity", "1.0", CVAR_ARCHIVE);
	r_fogColorR = ri.Cvar_Get("r_fogColorR", "0.5", CVAR_ARCHIVE);
	r_fogColorG = ri.Cvar_Get("r_fogColorG", "0.5", CVAR_ARCHIVE);
	r_fogColorB = ri.Cvar_Get("r_fogColorB", "0.5", CVAR_ARCHIVE);
	r_fogStart = ri.Cvar_Get("r_fogStart", "0", CVAR_ARCHIVE);
	r_fogEnd = ri.Cvar_Get("r_fogEnd", "131072", CVAR_ARCHIVE);

	r_skyColorR = ri.Cvar_Get("r_skyColorR", "0.5", CVAR_ARCHIVE);
	r_skyColorG = ri.Cvar_Get("r_skyColorG", "0.5", CVAR_ARCHIVE);
	r_skyColorB = ri.Cvar_Get("r_skyColorB", "0.5", CVAR_ARCHIVE);

	//
	// archived variables that can change at any time
	//
	r_lodbias = ri.Cvar_Get("r_lodbias", "0", CVAR_ARCHIVE);
	r_dynamiclight = ri.Cvar_Get("r_dynamiclight", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_textureMode = ri.Cvar_Get("r_textureMode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE);
#ifdef __EMSCRIPTEN__
	// Under Emscripten we don't throttle framerate with com_maxfps by default, so enable
	// vsync by default instead.
	r_swapInterval = ri.Cvar_Get("r_swapInterval", "1", CVAR_ARCHIVE | CVAR_LATCH);
#else
	r_swapInterval = ri.Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE | CVAR_LATCH);
#endif
	r_gamma = ri.Cvar_Get("r_gamma", "1", CVAR_ARCHIVE);

	r_ambientScale = ri.Cvar_Get("r_ambientScale", "0.32", CVAR_CHEAT);
	r_directedScale = ri.Cvar_Get("r_directedScale", "0.72", CVAR_CHEAT);

	// temporary variables that can change at any time
	r_debugSort = ri.Cvar_Get("r_debugSort", "0", CVAR_CHEAT);
	r_ignore = ri.Cvar_Get("r_ignore", "1", CVAR_CHEAT);
	r_speeds = ri.Cvar_Get("r_speeds", "0", CVAR_CHEAT);
	r_showtris = ri.Cvar_Get("r_showtris", "0", CVAR_CHEAT);
	r_lockpvs = ri.Cvar_Get("r_lockpvs", "0", CVAR_CHEAT);

	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri.Cmd_AddCommand("imagelist", R_ImageList_f);
	ri.Cmd_AddCommand("shaderlist", R_ShaderList_f);
	ri.Cmd_AddCommand("skinlist", R_SkinList_f);
	ri.Cmd_AddCommand("modellist", R_Modellist_f);
	ri.Cmd_AddCommand("screenshot", R_ScreenShot_f);
	ri.Cmd_AddCommand("screenshotJPEG", R_ScreenShotJPEG_f);
	ri.Cmd_AddCommand("gfxinfo", GfxInfo_f);
	ri.Cmd_AddCommand("minimize", GLimp_Minimize);
	ri.Cmd_AddCommand("gfxmeminfo", GfxMemInfo_f);
	ri.Cmd_AddCommand("vaoList", R_VaoList_f);
}

void R_InitQueries(void) {
	if(!glRefConfig.occlusionQuery) return;
}

void R_ShutDownQueries(void) {
	if(!glRefConfig.occlusionQuery) return;
}

/*
===============
R_Init
===============
*/
void R_Init(void) {
	int err;
	int i;
	byte* ptr;

	ri.Printf(PRINT_ALL, "----- R_Init -----\n");

	// clear all our internal state
	Com_Memset(&tr, 0, sizeof(tr));
	Com_Memset(&backEnd, 0, sizeof(backEnd));
	Com_Memset(&tess, 0, sizeof(tess));

	if(sizeof(glconfig_t) != 11312) ri.Error(ERR_FATAL, "Mod ABI incompatible: sizeof(glconfig_t) == %u != 11312", (unsigned int)sizeof(glconfig_t));

	//	Swap_Init();

	if((intptr_t)tess.xyz & 15) {
		ri.Printf(PRINT_WARNING, "tess.xyz not 16 byte aligned\n");
	}

	//
	// init function tables
	//
	for(i = 0; i < FUNCTABLE_SIZE; i++) {
		tr.sinTable[i] = sin(DEG2RAD(i * 360.0f / ((float)(FUNCTABLE_SIZE - 1))));
		tr.squareTable[i] = (i < FUNCTABLE_SIZE / 2) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if(i < FUNCTABLE_SIZE / 2) {
			if(i < FUNCTABLE_SIZE / 4) {
				tr.triangleTable[i] = (float)i / (FUNCTABLE_SIZE / 4);
			} else {
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
			}
		} else {
			tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
		}
	}

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	max_polys = MAX_POLYS;
	max_polyverts = MAX_POLYVERTS;

	ptr = ri.Hunk_Alloc(sizeof(*backEndData) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
	backEndData = (backEndData_t*)ptr;
	backEndData->polys = (srfPoly_t*)((char*)ptr + sizeof(*backEndData));
	backEndData->polyVerts = (polyVert_t*)((char*)ptr + sizeof(*backEndData) + sizeof(srfPoly_t) * max_polys);
	R_InitNextFrame();

	InitOpenGL();

	R_InitImages();

	if(glRefConfig.framebufferObject) FBO_Init();

	GLSL_InitGPUShaders();

	R_InitVaos();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitQueries();

	err = qglGetError();
	if(err != GL_NO_ERROR) ri.Printf(PRINT_ALL, "glGetError() = 0x%x\n", err);

	// print info
	GfxInfo_f();
	ri.Printf(PRINT_ALL, "----- finished R_Init -----\n");
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown(bool destroyWindow) {
	ri.Printf(PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow);

	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("shaderlist");
	ri.Cmd_RemoveCommand("skinlist");
	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("modelist");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("screenshotJPEG");
	ri.Cmd_RemoveCommand("gfxinfo");
	ri.Cmd_RemoveCommand("minimize");
	ri.Cmd_RemoveCommand("gfxmeminfo");
	ri.Cmd_RemoveCommand("vaoList");

	if(tr.registered) {
		R_IssuePendingRenderCommands();
		R_ShutDownQueries();
		if(glRefConfig.framebufferObject) FBO_Shutdown();
		R_DeleteTextures();
		R_ShutdownVaos();
		GLSL_ShutdownGPUShaders();
	}

	// shut down platform specific OpenGL stuff
	if(destroyWindow) {
		GLimp_Shutdown();

		Com_Memset(&glConfig, 0, sizeof(glConfig));
		Com_Memset(&glRefConfig, 0, sizeof(glRefConfig));
		textureFilterAnisotropic = false;
		maxAnisotropy = 0;
		displayAspect = 0.0f;
		haveClampToEdge = false;

		Com_Memset(&glState, 0, sizeof(glState));
	}

	tr.registered = false;
}

/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration(void) { R_IssuePendingRenderCommands(); }

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
#ifdef USE_RENDERER_DLOPEN
Q_EXPORT refexport_t* QDECL GetRefAPI(int apiVersion, refimport_t* rimp) {
#else
refexport_t* GetRefAPI(int apiVersion, refimport_t* rimp) {
#endif

	static refexport_t re;

	ri = *rimp;

	Com_Memset(&re, 0, sizeof(re));

	if(apiVersion != REF_API_VERSION) {
		ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, apiVersion);
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel = RE_RegisterModel;
	re.RegisterSkin = RE_RegisterSkin;
	re.RegisterShader = RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.LoadWorld = RE_LoadWorldMap;
	re.SetWorldVisData = RE_SetWorldVisData;
	re.EndRegistration = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame = RE_EndFrame;

	re.MarkFragments = R_MarkFragments;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	re.ClearScene = RE_ClearScene;
	re.AddRefEntityToScene = RE_AddRefEntityToScene;
	re.AddPolyToScene = RE_AddPolyToScene;
	re.LightForPoint = R_LightForPoint;
	re.AddLightToScene = RE_AddLightToScene;
	re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;
	re.AddVertexLightToScene = RE_AddVertexLightToScene;
	re.AddJuniorLightToScene = RE_AddJuniorLightToScene;
	re.AddDirectedLightToScene = RE_AddDirectedLightToScene;
	re.RenderScene = RE_RenderScene;

	re.SetColor = RE_SetColor;
	re.DrawStretchPic = RE_StretchPic;
	re.DrawStretchRaw = RE_StretchRaw;
	re.UploadCinematic = RE_UploadCinematic;

	re.RemapShader = R_RemapShader;
	re.GetEntityToken = R_GetEntityToken;
	re.inPVS = R_inPVS;

	re.TakeVideoFrame = RE_TakeVideoFrame;

	return &re;
}
