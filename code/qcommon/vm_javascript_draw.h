// Copyright (C) 2023-2026 Noire's Mod [noire.dev] — GPLv2

#include "../qcommon/vm_javascript_core.h"

typedef enum {
	K_TAB = 9,
	K_ENTER = 13,
	K_ESCAPE = 27,
	K_SPACE = 32,
	K_QUOTE = '\'',
	K_PLUS = '+',
	K_COMMA = ',',
	K_MINUS = '-',
	K_DOT = '.',
	K_SLASH = '/',
	K_SEMICOLON = ';',
	K_EQUAL = '=',
	K_BACKSLASH = '\\',
	K_UNDERSCORE = '_',
	K_BRACKET_OPEN = '[',
	K_BRACKET_CLOSE = ']',

	K_A = 'a',
	K_B = 'b',
	K_C = 'c',
	K_D = 'd',
	K_E = 'e',
	K_F = 'f',
	K_G = 'g',
	K_H = 'h',
	K_I = 'i',
	K_J = 'j',
	K_K = 'k',
	K_L = 'l',
	K_M = 'm',
	K_N = 'n',
	K_O = 'o',
	K_P = 'p',
	K_Q = 'q',
	K_R = 'r',
	K_S = 's',
	K_T = 't',
	K_U = 'u',
	K_V = 'v',
	K_W = 'w',
	K_X = 'x',
	K_Y = 'y',
	K_Z = 'z',

	K_BACKSPACE = 127,
	K_COMMAND,
	K_CAPSLOCK,
	K_POWER,
	K_PAUSE,

	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_ALT,
	K_CTRL,
	K_SHIFT,
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_F13,
	K_F14,
	K_F15,

	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_ENTER,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_KP_MINUS,
	K_KP_PLUS,
	K_KP_NUMLOCK,
	K_KP_STAR,
	K_KP_EQUALS,

	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,
	K_MWHEELDOWN,
	K_MWHEELUP,

	K_SUPER,
	K_COMPOSE,
	K_MODE,
	K_HELP,
	K_PRINT,
	K_SYSREQ,
	K_SCROLLOCK,
	K_BREAK,
	K_MENU,
	K_EURO,
	K_UNDO,

	MAX_KEYS
} keyNum_t;

extern float color_empty[4];
extern float color_black[4];
extern float color_white[4];
extern float color_background[4];

#define CGUI_COLORCOUNT 1024

// JS shared colors
#define JSC_EMPTY 0
#define JSC_BLACK 1
#define JSC_WHITE 2
#define JSC_GREY 3
#define JSC_RED 4
#define JSC_GREEN 5
#define JSC_BLUE 6
#define JSC_DISABLED 7
#define JSC_ENABLED 8
#define JSC_CONTEXTMENU 9
#define JSC_WINDOWBUTTON 10
#define JSC_DEBUG1 11
#define JSC_DEBUG2 12
#define JSC_LOADINGISLAND 13
#define JSC_ACCENT3 597

typedef struct {
	qhandle_t whiteShader;
	qhandle_t corner;
	qhandle_t corners[5];
	float scale, bias, wideoffset;
	float colors[CGUI_COLORCOUNT][4];
	char binds[MAX_KEYS][256];
} cgui_t;
extern cgui_t cgui;

extern glconfig_t glconfig;

// Element style
// Fonts
#define FONTSTYLE_LEFT 0
#define FONTSTYLE_CENTER 1
#define FONTSTYLE_RIGHT 2
#define FONTSTYLE_BOLD 4
#define FONTSTYLE_ITALIC 8
#define FONTSTYLE_UNDERLINE 16
#define FONTSTYLE_STRIKETHROUGH 32
#define FONTSTYLE_MAGIC 64
#define FONTSTYLE_SHAKE 128
#define FONTSTYLE_LOCKSTYLE 256
#define FONTSTYLE_LOCKEFFECTS 512
#define FONTSTYLE_LOCKCOLOR 1024
#define FONTSTYLE_LOCKEMOJI 2048
#define FONTSTYLE_ACCENT 4096
#define FONTSTYLE_DROPSHADOW 8192
// Other
#define NO_TOP_LEFT 16384
#define NO_TOP_RIGHT 32768
#define NO_BOTTOM_LEFT 65536
#define NO_BOTTOM_RIGHT 131072
#define NO_DRAW 262144

// Window style
#define UI_NOTITLE 1
#define UI_NOSCALE 2
#define UI_NOZORDER 4
#define UI_NOSAVE 8

#define FONT_SIZE 24.00
#define FONT_WIDTH 0.50
#define FONT_WIDTH_CJK 0.80
#define DEFAULT_MAXCHARS 4096

void CL_Init(void);
void adjustFrom640(float* x, float* y, float* w, float* h, float* corner, float* fontScale);
void drawhShader(float x, float y, float w, float h, qhandle_t shaderID);
void drawhShaderAdjusted(float x, float y, float w, float h, qhandle_t shaderID);
void drawModel(float x, float y, float w, float h, const char* model, float scale);
void drawModelAdjusted(float x, float y, float w, float h, const char* model, float scale);
void drawShader(float x, float y, float w, float h, const char* file);
void drawShaderInFolder(float x, float y, float w, float h, const char* dir);
void drawShaderAdjusted(float x, float y, float w, float h, const char* file);
void drawColoredShader(float x, float y, float w, float h, const char* file, float* color);
void drawColoredShaderAdjusted(float x, float y, float w, float h, const char* file, float* color);
float stringWidth(const char* str, float fontScale, int style, int maxChars);
void drawString(float x, float y, const char* str, int style, float* color, float fontSize, int maxChars);
void drawStringAdjusted(float x, float y, const char* str, int style, float* color, float fontSize, int maxChars);
void drawStringField(float x, float y, const char* str, int style, float* color, float fontSize, int maxChars, int position);
void drawStringFieldAdjusted(float x, float y, const char* str, int style, float* color, float fontSize, int maxChars, int position);
void drawRoundedRect(float x, float y, float width, float height, float radius, float* color, int style);
void drawRoundedRectAdjusted(float x, float y, float width, float height, float radius, float* color, int style);
void drawOutline(float x, float y, float width, float height, float thickness, float* color);

extern float alphaRoundedRect;

typedef struct {
	float counterW;
	float counterH;
	float counterCorner;
	int counterColor;
	float counterTextX;
	float counterTextY;
	float counterTextScale;
	int counterTextColor;
	int counterTextStyle;
	float counterValueX;
	float counterValueY;
	float counterValueScale;
	int counterValueColor;
	int counterValueStyle;
} hud_s;
extern hud_s hud;

void CG_HUDInit(void);
void CG_HUDDraw(void);
void CG_HUDCounter(float x, float y, const char* value, const char* text);
