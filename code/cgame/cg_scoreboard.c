// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2

#include "../qcommon/vm_javascript.h"

static float scoreboardBG[4] = {0.10, 0.10, 0.10, 0.72};
static float scoreboardMeBG[4] = {0.96, 0.16, 0.32, 1.00};

#define SCORE_WIDTH 148.0
#define SCORE_HEIGHT 11.0
#define SCORE_SPACE 1.0
#define SCORE_SPACEX 2.0
#define SCORE_ICON_SIZE 9.0
#define SCORE_PING_SIZE 12.0

static void CG_DrawPing(int x, int y, int ping) {
	int pingID = 0;

	if(ping >= 80) pingID = 1;
	if(ping >= 160) pingID = 2;
	if(ping >= 240) pingID = 3;
	if(ping >= 320) pingID = 4;
	if(ping >= 400) pingID = 5;
	if(ping >= 800 || ping == -1) pingID = 6;

	drawShaderAdjusted(x, y - 3, SCORE_PING_SIZE, SCORE_PING_SIZE, va("menu/ping_%i", pingID));
}

static void CG_DrawClientScore(int x, int y, score_t* score) {
	if(score->client < 0 || score->client >= cgs.maxclients) return;

	drawRoundedRectAdjusted(x, y, SCORE_WIDTH, SCORE_HEIGHT, 0, scoreboardBG, 0);
	if(score->client == cg.snap->ps.clientNum) drawRoundedRectAdjusted(x, y + (SCORE_HEIGHT - 1), SCORE_WIDTH, 1, 0, scoreboardMeBG, 0);

	CG_DrawHead(x + 1, y + 1, SCORE_ICON_SIZE, SCORE_ICON_SIZE, score->client);

	drawStringAdjusted(x + 8 + SCORE_ICON_SIZE, y + 2, cgs.clientinfo[score->client].name, FONTSTYLE_LEFT | FONTSTYLE_DROPSHADOW, color_white, 0.35, 26);
	CG_DrawPing((x - 4) + (SCORE_WIDTH - SCORE_PING_SIZE), y, score->ping);
}

void CG_DrawScoreboard(void) {
	float x, y;
	int i, cols = (cg.numScores + 15) / 16;
	int colPosition = 0;

	if(!(cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD)) return;

	x = 320 - ((SCORE_WIDTH * 0.5) * cols) - ((cols - 1) * (SCORE_SPACEX * 0.5));
	y = 32;

	for(i = 0; i < cg.numScores; i++) {
		CG_DrawClientScore(x, y, &cg.scores[i]);
		y += SCORE_HEIGHT + SCORE_SPACE;
		colPosition++;
		if(colPosition >= 16) {
			x += SCORE_WIDTH + SCORE_SPACEX;
			y = 32;
			colPosition = 0;
		}
	}
}
