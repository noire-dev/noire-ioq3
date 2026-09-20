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
//
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "../qcommon/vm_javascript.h"
#include "../qcommon/vm_javascript_core.h"

static float scoreboardBG[4] = {0.00, 0.00, 0.00, 0.50};
static float scoreboardOutlineBG[4] = {0.10, 0.10, 0.10, 0.50};
static float scoreboardFieldBG[4] = {0.40, 0.40, 0.40, 0.50};

#define SCOREB_WIDTH 400.0
#define SCOREB_X (320.0 - (SCOREB_WIDTH / 2))
#define SCOREB_Y 42
#define SB_INTER_HEIGHT 16

static void CG_DrawClientScore(int y, score_t* score) {
	char nameText[256];
	char scoreText[256];
	clientInfo_t* ci;

	if(score->client < 0 || score->client >= cgs.maxclients) return;

	if(score->client == cg.snap->ps.clientNum) {
		float hcolor[4];
		int rank;

		rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;

		drawRoundedRectAdjusted(SCOREB_X, y, SCOREB_WIDTH - 4, ICON_SIZE, 3, scoreboardFieldBG, NO_TOP_LEFT | NO_BOTTOM_LEFT);
	}

	ci = &cgs.clientinfo[score->client];
	CG_DrawHead(SCOREB_X + 4, y, ICON_SIZE, ICON_SIZE, score->client);

	// draw the score line
	Com_sprintf(nameText, sizeof(nameText), "%s", ci->name);
	Com_sprintf(scoreText, sizeof(scoreText), "⇄ %i ▶ %i", score->ping, score->score);

	drawStringAdjusted(SCOREB_X + 8 + ICON_SIZE, y + 2, nameText, FONTSTYLE_LEFT | FONTSTYLE_DROPSHADOW, color_white, 0.56, 256);
	drawStringAdjusted((SCOREB_X + 8 + ICON_SIZE) + ((FONT_SIZE * FONT_WIDTH) * 54) * 0.56, y + 2, scoreText, FONTSTYLE_RIGHT | FONTSTYLE_DROPSHADOW, color_white, 0.56, 256);
}

static int CG_TeamScoreboard(int y, team_t team, int maxClients, int lineHeight) {
	int i, count;
	score_t* score;
	bool localFinded;
	clientInfo_t* ci;

	count = 0;
	localFinded = false;
	for(i = 0; i < cg.numScores && count < maxClients; i++) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[score->client];
		if(team != ci->team) continue;
		if(score->client == 0) {
			if(localFinded) continue;
			localFinded = true;
		}
		CG_DrawClientScore(y + lineHeight * count, score);
		count++;
	}

	return count;
}

void CG_DrawScoreboard(void) {
	int y, n1, n2;
	int maxClients;
	int lineHeight;

	if(!(cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD)) return;

	drawRoundedRectAdjusted(320 - (SCOREB_WIDTH * 0.5), SCOREB_Y, SCOREB_WIDTH, SB_INTER_HEIGHT * 26, 4, scoreboardBG, 0);
	drawRoundedRectAdjusted(320 - (SCOREB_WIDTH * 0.5) + 1, SCOREB_Y + 1, SCOREB_WIDTH, SB_INTER_HEIGHT * 26, 4, scoreboardOutlineBG, 0);

	y = 50;
	lineHeight = SB_INTER_HEIGHT;
	maxClients = 25;
	n1 = CG_TeamScoreboard(y, TEAM_FREE, maxClients, lineHeight);
	y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
}
