// Copyright (C) 2023-2026 Noire's Mod [noire.dev] — GPLv2

#include "../qcommon/vm_javascript_core.h"

typedef enum {
	K_TAB = 9,
	K_ENTER = 13,
	K_ESCAPE = 27,
	K_SPACE = 32,

	K_BACKSPACE = 127,

	K_COMMAND = 128,
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

	K_JOY1,
	K_JOY2,
	K_JOY3,
	K_JOY4,
	K_JOY5,
	K_JOY6,
	K_JOY7,
	K_JOY8,
	K_JOY9,
	K_JOY10,
	K_JOY11,
	K_JOY12,
	K_JOY13,
	K_JOY14,
	K_JOY15,
	K_JOY16,
	K_JOY17,
	K_JOY18,
	K_JOY19,
	K_JOY20,
	K_JOY21,
	K_JOY22,
	K_JOY23,
	K_JOY24,
	K_JOY25,
	K_JOY26,
	K_JOY27,
	K_JOY28,
	K_JOY29,
	K_JOY30,
	K_JOY31,
	K_JOY32,

	K_AUX1,
	K_AUX2,
	K_AUX3,
	K_AUX4,
	K_AUX5,
	K_AUX6,
	K_AUX7,
	K_AUX8,
	K_AUX9,
	K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
	K_AUX15,
	K_AUX16,

	K_WORLD_0,
	K_WORLD_1,
	K_WORLD_2,
	K_WORLD_3,
	K_WORLD_4,
	K_WORLD_5,
	K_WORLD_6,
	K_WORLD_7,
	K_WORLD_8,
	K_WORLD_9,
	K_WORLD_10,
	K_WORLD_11,
	K_WORLD_12,
	K_WORLD_13,
	K_WORLD_14,
	K_WORLD_15,
	K_WORLD_16,
	K_WORLD_17,
	K_WORLD_18,
	K_WORLD_19,
	K_WORLD_20,
	K_WORLD_21,
	K_WORLD_22,
	K_WORLD_23,
	K_WORLD_24,
	K_WORLD_25,
	K_WORLD_26,
	K_WORLD_27,
	K_WORLD_28,
	K_WORLD_29,
	K_WORLD_30,
	K_WORLD_31,
	K_WORLD_32,
	K_WORLD_33,
	K_WORLD_34,
	K_WORLD_35,
	K_WORLD_36,
	K_WORLD_37,
	K_WORLD_38,
	K_WORLD_39,
	K_WORLD_40,
	K_WORLD_41,
	K_WORLD_42,
	K_WORLD_43,
	K_WORLD_44,
	K_WORLD_45,
	K_WORLD_46,
	K_WORLD_47,
	K_WORLD_48,
	K_WORLD_49,
	K_WORLD_50,
	K_WORLD_51,
	K_WORLD_52,
	K_WORLD_53,
	K_WORLD_54,
	K_WORLD_55,
	K_WORLD_56,
	K_WORLD_57,
	K_WORLD_58,
	K_WORLD_59,
	K_WORLD_60,
	K_WORLD_61,
	K_WORLD_62,
	K_WORLD_63,
	K_WORLD_64,
	K_WORLD_65,
	K_WORLD_66,
	K_WORLD_67,
	K_WORLD_68,
	K_WORLD_69,
	K_WORLD_70,
	K_WORLD_71,
	K_WORLD_72,
	K_WORLD_73,
	K_WORLD_74,
	K_WORLD_75,
	K_WORLD_76,
	K_WORLD_77,
	K_WORLD_78,
	K_WORLD_79,
	K_WORLD_80,
	K_WORLD_81,
	K_WORLD_82,
	K_WORLD_83,
	K_WORLD_84,
	K_WORLD_85,
	K_WORLD_86,
	K_WORLD_87,
	K_WORLD_88,
	K_WORLD_89,
	K_WORLD_90,
	K_WORLD_91,
	K_WORLD_92,
	K_WORLD_93,
	K_WORLD_94,
	K_WORLD_95,

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

	K_PAD0_A,
	K_PAD0_B,
	K_PAD0_X,
	K_PAD0_Y,
	K_PAD0_BACK,
	K_PAD0_GUIDE,
	K_PAD0_START,
	K_PAD0_LEFTSTICK_CLICK,
	K_PAD0_RIGHTSTICK_CLICK,
	K_PAD0_LEFTSHOULDER,
	K_PAD0_RIGHTSHOULDER,
	K_PAD0_DPAD_UP,
	K_PAD0_DPAD_DOWN,
	K_PAD0_DPAD_LEFT,
	K_PAD0_DPAD_RIGHT,

	K_PAD0_LEFTSTICK_LEFT,
	K_PAD0_LEFTSTICK_RIGHT,
	K_PAD0_LEFTSTICK_UP,
	K_PAD0_LEFTSTICK_DOWN,
	K_PAD0_RIGHTSTICK_LEFT,
	K_PAD0_RIGHTSTICK_RIGHT,
	K_PAD0_RIGHTSTICK_UP,
	K_PAD0_RIGHTSTICK_DOWN,
	K_PAD0_LEFTTRIGGER,
	K_PAD0_RIGHTTRIGGER,

	K_PAD0_MISC1,
	K_PAD0_PADDLE1,
	K_PAD0_PADDLE2,
	K_PAD0_PADDLE3,
	K_PAD0_PADDLE4,
	K_PAD0_TOUCHPAD,

	K_CONSOLE,

	MAX_KEYS
} keyNum_t;

extern float color_empty[4];
extern float color_black[4];
extern float color_white[4];
extern float color_background[4];

#define CGUI_COLORCOUNT 1024 + (16 * 1024)

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
