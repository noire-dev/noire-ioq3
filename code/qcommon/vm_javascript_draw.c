// Copyright (C) 2023-2026 Noire's Mod — GPLv2

#include "vm_javascript.h"

float color_empty[4] = {0.00f, 0.00f, 0.00f, 0.00f};
float color_black[4] = {0.00f, 0.00f, 0.00f, 1.00f};
float color_white[4] = {1.00f, 1.00f, 1.00f, 1.00f};
float color_background[4] = {0.10f, 0.10f, 0.125f, 0.70f};

cgui_t cgui;
glconfig_t glconfig;
hud_s hud;

static void getScreenProperties(void) {
	trap_GetGlconfig(&glconfig);
	cgui.scale = (glconfig.vidWidth * (1.0 / 640.0) < glconfig.vidHeight * (1.0 / 480.0)) ? glconfig.vidWidth * (1.0 / 640.0) : glconfig.vidHeight * (1.0 / 480.0);

	if(glconfig.vidWidth * 480 > glconfig.vidHeight * 640)
		cgui.bias = 0.5 * (glconfig.vidWidth - (glconfig.vidHeight * (640.0 / 480.0)));
	else
		cgui.bias = 0.0;

	if((glconfig.vidWidth / (glconfig.vidHeight / 480.0) - 640.0) * 0.5 >= 0)
		cgui.wideoffset = (glconfig.vidWidth / (glconfig.vidHeight / 480.0) - 640.0) * 0.5;
	else
		cgui.wideoffset = 0.0;
}

void CL_Init(void) {
	int i;

	getScreenProperties();

	cgui.whiteShader = trap_R_RegisterShaderNoMip("white");
	cgui.corner = trap_R_RegisterShaderNoMip("menu/corner");
	for(i = 0; i < 5; i++) cgui.corners[i] = trap_R_RegisterShaderNoMip(va("menu/corner_%i", i));
#ifndef CGAME
	for(i = 0; i < MAX_KEYS; i++) trap_Key_GetBindingBuf(i, cgui.binds[i], 256);
#endif
}

void adjustFrom640(float* x, float* y, float* w, float* h, float* corner, float* fontScale) {
	if(x) *x = *x * cgui.scale + cgui.bias;
	if(y) *y *= cgui.scale;
	if(w) *w *= cgui.scale;
	if(h) *h *= cgui.scale;
	if(corner) *corner *= cgui.scale;
	if(fontScale) *fontScale *= cgui.scale;
}

void drawhShader(float x, float y, float w, float h, qhandle_t shaderID) { trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, shaderID); }

void drawhShaderAdjusted(float x, float y, float w, float h, qhandle_t shaderID) {
	adjustFrom640(&x, &y, &w, &h, NULL, NULL);
	trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, shaderID);
}

void drawShader(float x, float y, float w, float h, const char* file) {
	qhandle_t shaderID = trap_R_RegisterShaderNoMip(file);
	if(shaderID) {
		trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, shaderID);
	} else {
		if(w > h)
			trap_R_DrawStretchPic(x + ((w - h) * 0.5), y, h, h, 0, 0, 1, 1, trap_R_RegisterShaderNoMip("menu/loading"));
		else if(w < h)
			trap_R_DrawStretchPic(x, y + ((h - w) * 0.5), w, w, 0, 0, 1, 1, trap_R_RegisterShaderNoMip("menu/loading"));
		else
			trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, trap_R_RegisterShaderNoMip("menu/loading"));
	}
}

static char sh_list[131072];

void drawShaderInFolder(float x, float y, float w, float h, const char* dir) {
	qhandle_t shaderID = 0;
	int numfiles;
	char* fileptr;
	int i;
	int filelen;

	numfiles = trap_FS_GetFileList(dir, "*", sh_list, 131072);
	fileptr = sh_list;
	for(i = 0; i < numfiles; i++, fileptr += filelen + 1) {
		filelen = strlen(fileptr);
		if(!strcmp(fileptr + filelen - 3, "png") || !strcmp(fileptr + filelen - 3, "jpg") || !strcmp(fileptr + filelen - 3, "tga")) {
			shaderID = trap_R_RegisterShaderNoMip(va("%s/%s", dir, fileptr));
			break;
		}
	}
	if(shaderID) {
		trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, shaderID);
	} else {
		if(w > h)
			trap_R_DrawStretchPic(x + ((w - h) * 0.5), y, h, h, 0, 0, 1, 1, trap_R_RegisterShaderNoMip("menu/loading"));
		else if(w < h)
			trap_R_DrawStretchPic(x, y + ((h - w) * 0.5), w, w, 0, 0, 1, 1, trap_R_RegisterShaderNoMip("menu/loading"));
		else
			trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, trap_R_RegisterShaderNoMip("menu/loading"));
	}
}

void drawShaderAdjusted(float x, float y, float w, float h, const char* file) {
	adjustFrom640(&x, &y, &w, &h, NULL, NULL);
	drawShader(x, y, w, h, file);
}

void drawColoredShader(float x, float y, float w, float h, const char* file, float* color) {
	trap_R_SetColor(color);
	drawShader(x, y, w, h, file);
	trap_R_SetColor(NULL);
}

void drawColoredShaderAdjusted(float x, float y, float w, float h, const char* file, float* color) {
	adjustFrom640(&x, &y, &w, &h, NULL, NULL);
	drawColoredShader(x, y, w, h, file, color);
}

static void drawSymbol(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t fontIndex) { trap_R_DrawStretchPic(x, y, w, h, s1, t1, s2, t2, fontIndex); }

static int getFontRes(float fontScale) {
	if(fontScale * FONT_SIZE > 128) return 4;  // 4096
	if(fontScale * FONT_SIZE > 64) return 3;   // 2048
	if(fontScale * FONT_SIZE > 32) return 2;   // 1024
	if(fontScale * FONT_SIZE > 16) return 1;   // 512
	return 0;                                  // 256 default
}

// static const int emojiPages[] = { 32, 33, 35, 36, 37, 38, 39, 41, 43, 48, 50, 496, 497, 498, 499, 500, 501, 502, 503, 505, 506, 4072, -1 };
static const int emojiPages[] = {499, 500, 501, 502, 505, 506, 4072, -1};

static qboolean isEmojiPage(int pageID) {
	int i;
	for(i = 0; emojiPages[i] != -1; i++) {
		if(emojiPages[i] == pageID) return qtrue;
	}
	return qfalse;
}

static int getUTF8Font(int code, const char* style, float fontSize) {
	int sheetIndex = code / 256;
	if(strcmp(style, "Emoji") && code >= 1280) style = "Regular";
	if(isEmojiPage(sheetIndex)) style = "Emoji";
	return trap_R_RegisterShaderNoMip(va("menu/fonts/%s_%s_%i_%i", "default", style, sheetIndex, getFontRes(fontSize)));
}

static int isHex(char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }

static int hexToInt(char c) {
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'A' && c <= 'F') return 10 + (c - 'A');
	if(c >= 'a' && c <= 'f') return 10 + (c - 'a');
	return 0;
}

static float drawChars(int x, int y, const char* str, float* color, float fontScale, int style, int maxChars, qboolean returnWidth, int position) {
	const char* s;
	float ax, ay, aw, ah;
	float frow, fcol;
	int codepoint, bytesRead;
	int fontIndex = 0, currentIndex = 0, currentDrawChar = 0;
	float glyphsPerRow = 4096.0 / 256.0;
	float glyphTexSize = 256.0 / 4096.0;
	const char* formatStyle = "Regular";
	const char* savedStyle = "Regular";
	qboolean formatUnderline = qfalse;
	qboolean formatStrikethrough = qfalse;
	qboolean formatMagic = qfalse;
	qboolean formatShake = qfalse;
	float formatColor[4] = {1.00, 1.00, 1.00, 1.00};
	float* currentColor = color;
	float debugColor[4] = {0.75, 0.75, 0.75, 0.50};
	qboolean drawCursor = qtrue;

	ax = x;
	ay = y;
	aw = FONT_SIZE * fontScale;
	ah = FONT_SIZE * fontScale;

	if(style & FONTSTYLE_BOLD) savedStyle = "Bold";
	if(style & FONTSTYLE_ITALIC) savedStyle = "Italic";
	if(style & FONTSTYLE_BOLD && style & FONTSTYLE_ITALIC) savedStyle = "BoldItalic";
	if(style & FONTSTYLE_UNDERLINE) formatUnderline = qtrue;
	if(style & FONTSTYLE_STRIKETHROUGH) formatStrikethrough = qtrue;
	if(style & FONTSTYLE_MAGIC) formatMagic = qtrue;
	if(style & FONTSTYLE_SHAKE) formatShake = qtrue;

	formatStyle = savedStyle;

	s = str;
	while(*s) {
		if(*s == '&' && *(s + 1) != '\0') {
			char fmtCode = *(s + 1);

			switch(fmtCode) {
				case 'n': formatStyle = "Regular"; break;
				case 'e': formatStyle = "Emoji"; break;
				case 'b': formatStyle = "Bold"; break;
				case 'i': formatStyle = "Italic"; break;
				case 'o': formatStyle = "BoldItalic"; break;
				case '-': formatUnderline = qtrue; break;
				case '_': formatStrikethrough = qtrue; break;
				case 'm': formatMagic = qtrue; break;
				case 's': formatShake = qtrue; break;
				case 'r':
					formatStyle = savedStyle;
					formatUnderline = style & FONTSTYLE_UNDERLINE;
					formatStrikethrough = style & FONTSTYLE_STRIKETHROUGH;
					formatMagic = style & FONTSTYLE_MAGIC;
					formatShake = style & FONTSTYLE_SHAKE;
					currentColor = color;
					break;

				default: break;
			}

			s += 2;
			currentIndex += 2;
			continue;
		}

		if(style & FONTSTYLE_LOCKSTYLE) formatStyle = savedStyle;
		if(style & FONTSTYLE_LOCKEFFECTS) {
			formatUnderline = style & FONTSTYLE_UNDERLINE;
			formatStrikethrough = style & FONTSTYLE_STRIKETHROUGH;
			formatMagic = style & FONTSTYLE_MAGIC;
			formatShake = style & FONTSTYLE_SHAKE;
		}

		if(*s == '#' && isHex(*(s + 1)) && isHex(*(s + 2)) && isHex(*(s + 3))) {
			int r = hexToInt(*(s + 1));
			int g = hexToInt(*(s + 2));
			int b = hexToInt(*(s + 3));

			if(!(style & FONTSTYLE_LOCKCOLOR)) {
				formatColor[0] = (r * 16 + r) / 255.0f;
				formatColor[1] = (g * 16 + g) / 255.0f;
				formatColor[2] = (b * 16 + b) / 255.0f;
				formatColor[3] = color[3];

				currentColor = formatColor;
			}

			s += 4;
			currentIndex += 4;
			continue;
		}

		if(style & FONTSTYLE_ACCENT) currentColor = cgui.colors[JSC_ENABLED];

		if((*s & 0x80) == 0) {
			codepoint = *s;
			bytesRead = 1;
		} else if((*s & 0xE0) == 0xC0) {
			if(*(s + 1) != '\0') {
				codepoint = ((*s & 0x1F) << 6) | (*(s + 1) & 0x3F);
				bytesRead = 2;
			} else {
				codepoint = '?';
				bytesRead = 1;
			}
		} else if((*s & 0xF0) == 0xE0) {
			if(*(s + 1) != '\0' && *(s + 2) != '\0') {
				codepoint = ((*s & 0x0F) << 12) | ((*(s + 1) & 0x3F) << 6) | (*(s + 2) & 0x3F);
				bytesRead = 3;
			} else {
				codepoint = '?';
				bytesRead = 1;
			}
		} else if((*s & 0xF8) == 0xF0) {
			if(*(s + 1) != '\0' && *(s + 2) != '\0' && *(s + 3) != '\0') {
				codepoint = ((*s & 0x07) << 18) | ((*(s + 1) & 0x3F) << 12) | ((*(s + 2) & 0x3F) << 6) | (*(s + 3) & 0x3F);
				bytesRead = 4;
			} else {
				codepoint = '?';
				bytesRead = 1;
			}
		} else {
			codepoint = '?';
			bytesRead = 1;
		}

		if(codepoint >= 0xD800 && codepoint <= 0xDBFF) {
			const char* next_s = s + bytesRead;
			int next_cp = 0;
			int next_bytes = 1;

			if(*next_s) {
				if((*next_s & 0x80) == 0) {
					next_cp = *next_s;
				} else if((*next_s & 0xE0) == 0xC0) {
					next_cp = ((*next_s & 0x1F) << 6) | (*(next_s + 1) & 0x3F);
					next_bytes = 2;
				} else if((*next_s & 0xF0) == 0xE0) {
					next_cp = ((*next_s & 0x0F) << 12) | ((*(next_s + 1) & 0x3F) << 6) | (*(next_s + 2) & 0x3F);
					next_bytes = 3;
				} else if((*next_s & 0xF8) == 0xF0) {
					next_cp = ((*next_s & 0x07) << 18) | ((*(next_s + 1) & 0x3F) << 12) | ((*(next_s + 2) & 0x3F) << 6) | (*(next_s + 3) & 0x3F);
					next_bytes = 4;
				}
			}

			if(next_cp >= 0xDC00 && next_cp <= 0xDFFF) {
				codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (next_cp - 0xDC00);
				bytesRead += next_bytes;
			}
		}

		if(formatMagic) codepoint = 912 + (rand() % (976 - 912 + 1));
		if(!returnWidth) fontIndex = getUTF8Font(codepoint, formatStyle, fontScale);

		if(fontIndex > 0 && !returnWidth) {
			float shadowColor[4];
			int glyphIndex = codepoint % 256;
			int row = glyphIndex / (int)glyphsPerRow;
			int col = glyphIndex % (int)glyphsPerRow;
			float xoff = 0.00;
			float yoff = 0.00;

			shadowColor[0] = shadowColor[1] = shadowColor[2] = 0.00;
			shadowColor[3] = currentColor[3];

			if(formatShake) {
				xoff = 2 * fontScale + (rand() % 5) - 2;
				yoff = 2 * fontScale + (rand() % 5) - 2;
			} else {
				xoff = 0.00;
				yoff = 0.00;
			}

			fcol = col * glyphTexSize;
			frow = row * glyphTexSize;

			if(codepoint < 1280) {
				trap_R_SetColor(shadowColor);
				if(style & FONTSTYLE_DROPSHADOW) drawSymbol(ax + xoff + (2 * fontScale), ay + yoff + (2 * fontScale), aw * FONT_WIDTH, ah, fcol + 0.015625, frow, (fcol + glyphTexSize) - 0.015625, frow + glyphTexSize, fontIndex);
				trap_R_SetColor(currentColor);
				drawSymbol(ax + xoff, ay + yoff, aw * FONT_WIDTH, ah, fcol + 0.015625, frow, (fcol + glyphTexSize) - 0.015625, frow + glyphTexSize, fontIndex);
				if(formatUnderline) drawRoundedRect(ax, y + (ah * 0.925), aw * FONT_WIDTH, ah * 0.10, 0.00, currentColor, 0);
				if(formatStrikethrough) drawRoundedRect(ax, y + (ah * 0.50), aw * FONT_WIDTH, ah * 0.10, 0.00, currentColor, 0);
				if(position != -1 && currentIndex >= position && drawCursor) {
					drawRoundedRect(ax, y, aw * 0.05, ah, 0.00, currentColor, 9);
					drawCursor = qfalse;
				}
			} else if((isEmojiPage(codepoint / 256) && !(style & FONTSTYLE_LOCKEMOJI)) || !isEmojiPage(codepoint / 256)) {
				trap_R_SetColor(shadowColor);
				if(style & FONTSTYLE_DROPSHADOW) drawSymbol(ax + xoff + (2 * fontScale), ay + yoff + (2 * fontScale), ah, ah, fcol, frow, fcol + glyphTexSize, frow + glyphTexSize, fontIndex);
				if(isEmojiPage(codepoint / 256) || !strcmp(formatStyle, "Emoji"))
					trap_R_SetColor(formatColor);
				else
					trap_R_SetColor(currentColor);
				drawSymbol(ax + xoff, ay + yoff, ah, ah, fcol, frow, fcol + glyphTexSize, frow + glyphTexSize, fontIndex);
				if(formatUnderline) drawRoundedRect(ax, y + (ah * 0.925), aw, ah * 0.10, 0.00, formatColor, 0);
				if(formatStrikethrough) drawRoundedRect(ax, y + (ah * 0.50), aw, ah * 0.10, 0.00, formatColor, 0);
				if(position != -1 && currentIndex >= position && drawCursor) {
					drawRoundedRect(ax, y, aw * 0.05, ah, 0.00, formatColor, 0);
					drawCursor = qfalse;
				}
			}
		}

		if(fontIndex > 0 || returnWidth) {
			if(codepoint < 1280) {
				ax += aw * FONT_WIDTH;
				currentDrawChar += 1;
			} else if((isEmojiPage(codepoint / 256) && !(style & FONTSTYLE_LOCKEMOJI)) || !isEmojiPage(codepoint / 256)) {
				ax += aw * FONT_WIDTH_CJK;
				currentDrawChar += 2;
			}
		}

		s += bytesRead;
		currentIndex += bytesRead;
		if(currentDrawChar >= maxChars) {
			trap_R_SetColor(NULL);
			return ax - x;
		}
	}

	trap_R_SetColor(NULL);

	// if(!returnWidth) drawRoundedRect(x, y, stringWidth(str, fontScale, style), FONT_SIZE * fontScale, 0.00, debugColor, 0);

	return ax - x;
}

float stringWidth(const char* str, float fontScale, int style, int maxChars) { return drawChars(0, 0, str, cgui.colors[0], fontScale, style, maxChars, qtrue, -1); }

void drawModel(float x, float y, float w, float h, const char* model, float scale) {
	refdef_t refdef;
	refEntity_t ent;
	vec3_t origin;
	vec3_t angles;

	memset(&refdef, 0, sizeof(refdef));
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear(refdef.viewaxis);

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.fov_x = 90;
	refdef.fov_y = 90;

	refdef.time = trap_Milliseconds();

	origin[0] = scale;
	origin[1] = 0;
	origin[2] = 0;

	angles[0] = 0;
	angles[1] = (trap_Milliseconds() & 4095) * 360 / 4096.0;
	angles[2] = 0;

	trap_R_ClearScene();

	memset(&ent, 0, sizeof(ent));

	AnglesToAxis(angles, ent.axis);
	ent.hModel = trap_R_RegisterModel(model);
	ent.shaderRGBA[0] = 128;
	ent.shaderRGBA[1] = 128;
	ent.shaderRGBA[2] = 128;
	ent.shaderRGBA[3] = 255;
	VectorCopy(origin, ent.origin);
	VectorCopy(origin, ent.lightingOrigin);
	ent.renderfx = RF_LIGHTING_ORIGIN;
	VectorCopy(ent.origin, ent.oldorigin);
	ent.customSkin = trap_R_RegisterSkin(va("mtr/%s/%i.skin", model, 0));

	trap_R_AddRefEntityToScene(&ent);
	trap_R_RenderScene(&refdef);
}

void drawModelAdjusted(float x, float y, float w, float h, const char* model, float scale) {
	adjustFrom640(&x, &y, &w, &h, NULL, NULL);
	drawModel(x, y, w, h, model, scale);
}

void drawString(float x, float y, const char* str, int style, float* color, float fontSize, int maxChars) {
	if(!str || !strlen(str)) return;

	if(style & FONTSTYLE_RIGHT)
		x -= stringWidth(str, fontSize, style, maxChars);
	else if(style & FONTSTYLE_CENTER)
		x -= stringWidth(str, fontSize, style, maxChars) * 0.50f;

	drawChars(x, y, str, color, fontSize, style, maxChars, qfalse, -1);
}

void drawStringAdjusted(float x, float y, const char* str, int style, float* color, float fontSize, int maxChars) {
	adjustFrom640(&x, &y, NULL, NULL, NULL, &fontSize);
	drawString(x, y, str, style, color, fontSize, maxChars);
}

void drawStringField(float x, float y, const char* str, int style, float* color, float fontSize, int maxChars, int position) {
	if(!str || !strlen(str)) return;

	if(style & FONTSTYLE_CENTER) x -= stringWidth(str, fontSize, style, maxChars) * 0.50;
	if(style & FONTSTYLE_RIGHT) x -= stringWidth(str, fontSize, style, maxChars) * 1.00;

	drawChars(x, y, str, color, fontSize, style, maxChars, qfalse, position);
}

void drawStringFieldAdjusted(float x, float y, const char* str, int style, float* color, float fontSize, int maxChars, int position) {
	adjustFrom640(&x, &y, NULL, NULL, NULL, &fontSize);
	drawStringField(x, y, str, style, color, fontSize, maxChars, position);
}

static int getCornerRes(float cornerRadius) {
	if(cornerRadius > 128) return 4;  // 256
	if(cornerRadius > 64) return 3;   // 128
	if(cornerRadius > 32) return 2;   // 64
	if(cornerRadius > 16) return 1;   // 32
	return 0;                         // 16 default
}

float alphaRoundedRect = 1.00f;

void drawRoundedRect(float x, float y, float width, float height, float radius, float* color, int style) {
	float cx, cy, cw, ch;
	float top_x, top_y, top_w, top_h;
	float bot_x, bot_y, bot_w, bot_h;
	float left_x, left_y, left_w, left_h;
	float right_x, right_y, right_w, right_h;
	int tl, tr, bl, br;
	int cornerRes = 0;
	float tempColor[4];

	if(color[3] < 0.01f) return;

	tempColor[0] = color[0];
	tempColor[1] = color[1];
	tempColor[2] = color[2];
	tempColor[3] = color[3] * alphaRoundedRect;

	if(radius * 2.0f > height) radius = height * 0.5f;
	if(radius * 2.0f > width) radius = width * 0.5f;

	cornerRes = getCornerRes(radius);

	trap_R_SetColor(tempColor);

	if(radius <= 0.0f) {
		trap_R_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, cgui.whiteShader);
		trap_R_SetColor(NULL);
		return;
	}

	tl = !(style & NO_TOP_LEFT);
	tr = !(style & NO_TOP_RIGHT);
	bl = !(style & NO_BOTTOM_LEFT);
	br = !(style & NO_BOTTOM_RIGHT);

	cx = x + radius;
	cy = y + radius;
	cw = width - 2.0f * radius;
	ch = height - 2.0f * radius;

	top_x = x + radius;
	top_y = y;
	top_w = width - (radius * 2);
	top_h = radius;

	bot_x = x + radius;
	bot_y = y + height - radius;
	bot_w = width - (radius * 2);
	bot_h = radius;

	left_x = x;
	left_y = y + (tl ? radius : 0.0f);
	left_w = radius;
	left_h = height - (tl ? radius : 0.0f) - (bl ? radius : 0.0f);

	right_x = x + width - radius;
	right_y = y + (tr ? radius : 0.0f);
	right_w = radius;
	right_h = height - (tr ? radius : 0.0f) - (br ? radius : 0.0f);

	if(cw > 0.0f && ch > 0.0f) trap_R_DrawStretchPic(cx, cy, cw, ch, 0, 0, 0, 0, cgui.whiteShader);
	if(top_w > 0.0f && top_h > 0.0f) trap_R_DrawStretchPic(top_x, top_y, top_w, top_h, 0, 0, 0, 0, cgui.whiteShader);
	if(bot_w > 0.0f && bot_h > 0.0f) trap_R_DrawStretchPic(bot_x, bot_y, bot_w, bot_h, 0, 0, 0, 0, cgui.whiteShader);
	if(left_w > 0.0f && left_h > 0.0f) trap_R_DrawStretchPic(left_x, left_y, left_w, left_h, 0, 0, 0, 0, cgui.whiteShader);
	if(right_w > 0.0f && right_h > 0.0f) trap_R_DrawStretchPic(right_x, right_y, right_w, right_h, 0, 0, 0, 0, cgui.whiteShader);

	if(tl) trap_R_DrawStretchPic(x, y, radius, radius, 1, 0, 0, 1, cgui.corners[cornerRes]);
	if(tr) trap_R_DrawStretchPic(x + width - radius, y, radius, radius, 0, 0, 1, 1, cgui.corners[cornerRes]);
	if(bl) trap_R_DrawStretchPic(x, y + height - radius, radius, radius, 1, 1, 0, 0, cgui.corners[cornerRes]);
	if(br) trap_R_DrawStretchPic(x + width - radius, y + height - radius, radius, radius, 0, 1, 1, 0, cgui.corners[cornerRes]);

	trap_R_SetColor(NULL);
}

void drawRoundedRectAdjusted(float x, float y, float width, float height, float radius, float* color, int style) {
	adjustFrom640(&x, &y, &width, &height, &radius, NULL);
	drawRoundedRect(x, y, width, height, radius, color, style);
}

void drawOutline(float x, float y, float width, float height, float thickness, float* color) {
	trap_R_SetColor(color);

	trap_R_DrawStretchPic(x, y, width, thickness, 0, 0, 0, 0, cgui.whiteShader);
	trap_R_DrawStretchPic(x, y + height - thickness, width, thickness, 0, 0, 0, 0, cgui.whiteShader);
	trap_R_DrawStretchPic(x, y, thickness, height, 0, 0, 0, 0, cgui.whiteShader);
	trap_R_DrawStretchPic(x + width - thickness, y, thickness, height, 0, 0, 0, 0, cgui.whiteShader);

	trap_R_SetColor(NULL);
}

void CG_HUDInit(void) { JS_HUDInit(); }

void CG_HUDDraw(void) { JS_HUDDraw(); }

void CG_HUDCounter(float x, float y, const char* value, const char* text) {
	float textY = y + (hud.counterH - (FONT_SIZE * hud.counterTextScale)) * hud.counterTextY;
	float valueY = y + (hud.counterH - (FONT_SIZE * hud.counterValueScale)) * hud.counterValueY;

	drawRoundedRectAdjusted(x, y, hud.counterW, hud.counterH, hud.counterCorner, cgui.colors[hud.counterColor], 0);
	drawStringAdjusted(x + (hud.counterW * hud.counterTextX), textY, text, hud.counterTextStyle, cgui.colors[hud.counterTextColor], hud.counterTextScale, 256);
	drawStringAdjusted(x + (hud.counterW * hud.counterValueX), valueY, value, hud.counterValueStyle, cgui.colors[hud.counterValueColor], hud.counterValueScale, 256);
}
