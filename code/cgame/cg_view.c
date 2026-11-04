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
// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering
#include "cg_local.h"

/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
static void CG_CalcVrect(void) {
	int size;

	if(cg_viewsize.integer < 30) {
		trap_Cvar_Set("cg_viewsize", "30");
		size = 30;
	} else if(cg_viewsize.integer > 100) {
		trap_Cvar_Set("cg_viewsize", "100");
		size = 100;
	} else {
		size = cg_viewsize.integer;
	}
	cg.refdef.width = cgs.glconfig.vidWidth * size / 100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight * size / 100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width) / 2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height) / 2;
}

//==============================================================================

/*
===============
CG_OffsetThirdPersonView

===============
*/
#define FOCUS_DISTANCE 512
static void CG_OffsetThirdPersonView(void) {
	vec3_t forward, right, up;
	vec3_t view;
	vec3_t focusAngles;
	trace_t trace;
	static vec3_t mins = {-4, -4, -4};
	static vec3_t maxs = {4, 4, 4};
	vec3_t focusPoint;
	float focusDist;
	float forwardScale, sideScale;

	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;

	VectorCopy(cg.refdefViewAngles, focusAngles);

	// if dead, look at killer
	if(cg.predictedPlayerState.stats[STAT_HEALTH] <= 0) {
		focusAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
		cg.refdefViewAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
	}

	if(focusAngles[PITCH] > 45) {
		focusAngles[PITCH] = 45;  // don't go too far overhead
	}
	AngleVectors(focusAngles, forward, NULL, NULL);

	VectorMA(cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint);

	VectorCopy(cg.refdef.vieworg, view);

	view[2] += 8;

	cg.refdefViewAngles[PITCH] *= 0.5;

	AngleVectors(cg.refdefViewAngles, forward, right, up);

	forwardScale = cos(cg_thirdPersonAngle.value / 180 * M_PI);
	sideScale = sin(cg_thirdPersonAngle.value / 180 * M_PI);
	VectorMA(view, -cg_thirdPersonRange.value * forwardScale, forward, view);
	VectorMA(view, -cg_thirdPersonRange.value * sideScale, right, view);

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

	VectorCopy(view, cg.refdef.vieworg);

	// select pitch to look at focus point from vieword
	VectorSubtract(focusPoint, cg.refdef.vieworg, focusPoint);
	focusDist = sqrt(focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1]);
	if(focusDist < 1) {
		focusDist = 1;  // should never happen
	}
	cg.refdefViewAngles[PITCH] = -180 / M_PI * atan2(focusPoint[2], focusDist);
	cg.refdefViewAngles[YAW] -= cg_thirdPersonAngle.value;
}

/*
===============
CG_OffsetBodyView

Full body awareness: the camera sits in the local player's
ValveBiped.Bip01_Head1 joint (the joint itself is collapsed by
CG_Player so it never blocks the view). Falls back to the classic
eye position when the head joint is not available.
===============
*/
static void CG_OffsetBodyView(void) {
	vec3_t headOrigin;
	vec3_t fwd;
	vec3_t angles;
	clientInfo_t* ci;

	if(CG_GetLocalPlayerHeadOrigin(headOrigin)) {
		VectorCopy(headOrigin, cg.refdef.vieworg);

		// fine-tuning: per-model headSettings (up fwd) from animation.cfg
		// plus the player cvars. Fwd pushes the camera along the horizontal
		// facing direction, up moves it up/down in world space
		ci = &cgs.clientinfo[cg.snap->ps.clientNum];
		angles[PITCH] = 0.0f;
		angles[ROLL] = 0.0f;
		angles[YAW] = cg.predictedPlayerState.viewangles[YAW];
		AngleVectors(angles, fwd, NULL, NULL);
		VectorMA(cg.refdef.vieworg, cg_firstPerson_Fwd.value + ci->headFwd, fwd, cg.refdef.vieworg);
		cg.refdef.vieworg[2] += cg_firstPerson_Up.value + ci->headUp;
	} else {
		cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;
	}
}

//======================================================================

void CG_ZoomDown_f(void) {
	if(cg.zoomed) return;
	if(jsd_weapon[cg.snap->ps.weapon].wType != WT_PHYSGUN) {
		cg.zoomed = true;
		cg.zoomTime = cg.time;
	} else {
		trap_SendConsoleCommand(EXEC_INSERT, "altfire_physgun\n");
	}
}

void CG_ZoomUp_f(void) {
	if(!cg.zoomed) {
		return;
	}
	cg.zoomed = false;
	cg.zoomTime = cg.time;
}

/*
====================
CG_CalcFov

Fixed fov, otherwise account for fov variable and zooms.
====================
*/
#define WAVE_AMPLITUDE 1
#define WAVE_FREQUENCY 0.4

static int CG_CalcFov(void) {
	float x;
	float phase;
	float v;
	int contents;
	float fov_x, fov_y;
	float zoomFov;
	float f;
	int inwater;

	// user selectable
	if(cgs.dmflags & DF_FIXED_FOV) {
		// dmflag to prevent wide fov for all clients
		fov_x = 90;
	} else {
		fov_x = cg_fov.value;
		if(fov_x < 1) {
			fov_x = 1;
		} else if(fov_x > 160) {
			fov_x = 160;
		}
	}

	// account for zooms
	zoomFov = cg_zoomFov.value;
	if(zoomFov < 1) {
		zoomFov = 1;
	} else if(zoomFov > 160) {
		zoomFov = 160;
	}

	if(cg.zoomed) {
		f = (cg.time - cg.zoomTime) / (float)ZOOM_TIME;
		if(f > 1.0) {
			fov_x = zoomFov;
		} else {
			fov_x = fov_x + f * (zoomFov - fov_x);
		}
	} else {
		f = (cg.time - cg.zoomTime) / (float)ZOOM_TIME;
		if(f <= 1.0) {
			fov_x = zoomFov + f * (fov_x - zoomFov);
		}
	}

	x = cg.refdef.width / tan(fov_x / 360 * M_PI);
	fov_y = atan2(cg.refdef.height, x);
	fov_y = fov_y * 360 / M_PI;

	// warp if underwater
	contents = CG_PointContents(cg.refdef.vieworg, -1);
	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin(phase);
		fov_x += v;
		fov_y -= v;
		inwater = true;
	} else {
		inwater = false;
	}

	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	if(!cg.zoomed) {
		cg.zoomSensitivity = 1;
	} else {
		cg.zoomSensitivity = cg.refdef.fov_y / 75.0;
	}

	return inwater;
}

/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
static int CG_CalcViewValues(void) {
	playerState_t* ps;

	memset(&cg.refdef, 0, sizeof(cg.refdef));

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;

	cg.bobcycle = (ps->bobCycle & 128) >> 7;
	cg.bobfracsin = fabs(sin((ps->bobCycle & 127) / 127.0 * M_PI));
	cg.xyspeed = sqrt(ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1]);

	VectorCopy(ps->origin, cg.refdef.vieworg);
	VectorCopy(ps->viewangles, cg.refdefViewAngles);

	if(cg_cameraOrbit.integer) {
		if(cg.time > cg.nextOrbitTime) {
			cg.nextOrbitTime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
		}
	}
	// add error decay
	if(cg_errorDecay.value > 0) {
		int t;
		float f;

		t = cg.time - cg.predictedErrorTime;
		f = (cg_errorDecay.value - t) / cg_errorDecay.value;
		if(f > 0 && f < 1) {
			VectorMA(cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg);
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	if(cg.renderingThirdPerson) {
		// back away from character
		CG_OffsetThirdPersonView();
	} else {
		// full body awareness: camera in the head joint
		CG_OffsetBodyView();
	}

	// position eye relative to origin
	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);

	if(cg.hyperspace) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}

/*
=====================
CG_AddBufferedSound
=====================
*/
void CG_AddBufferedSound(sfxHandle_t sfx) {
	if(!sfx) return;
	cg.soundBuffer[cg.soundBufferIn] = sfx;
	cg.soundBufferIn = (cg.soundBufferIn + 1) % MAX_SOUNDBUFFER;
	if(cg.soundBufferIn == cg.soundBufferOut) {
		cg.soundBufferOut++;
	}
}

/*
=====================
CG_PlayBufferedSounds
=====================
*/
static void CG_PlayBufferedSounds(void) {
	if(cg.soundTime < cg.time) {
		if(cg.soundBufferOut != cg.soundBufferIn && cg.soundBuffer[cg.soundBufferOut]) {
			trap_S_StartLocalSound(cg.soundBuffer[cg.soundBufferOut], CHAN_ANNOUNCER);
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		}
	}
}

//=========================================================================

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/
void CG_DrawActiveFrame(int serverTime, bool demoPlayback) {
	int inwater;

	cg.time = serverTime;
	cg.demoPlayback = demoPlayback;

	// update cvars
	CG_UpdateCvars();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if(cg.infoScreenValue < 0) {
		CG_DrawInformation();
		return;
	}

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds(false);

	// clear all the render lists
	trap_R_ClearScene();

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if(!cg.snap || (cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE)) {
		CG_DrawInformation();
		return;
	}

	// let the client system know what our weapon and zoom settings are
	trap_SetUserCmdValue(cg.weaponSelect, cg.zoomSensitivity);

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	// decide on third person view
	cg.renderingThirdPerson = (cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0));

	// build cg.refdef
	inwater = CG_CalcViewValues();

	// build the render lists
	if(!cg.hyperspace) {
		CG_AddPacketEntities();  // adter calcViewValues, so predicted player state is correct
		CG_AddMarks();
		CG_RenderParticles();
		CG_AddLocalEntities();
	}

	// add buffered sounds
	CG_PlayBufferedSounds();

	cg.refdef.time = cg.time;
	memcpy(cg.refdef.areamask, cg.snap->areamask, sizeof(cg.refdef.areamask));

	// update audio positions
	trap_S_Respatialize(cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater);

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	cg.frametime = cg.time - cg.oldTime;
	if(cg.frametime < 0) cg.frametime = 0;
	cg.oldTime = cg.time;
	if(cg_timescale.value != cg_timescaleFadeEnd.value) {
		if(cg_timescale.value < cg_timescaleFadeEnd.value) {
			cg_timescale.value += cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if(cg_timescale.value > cg_timescaleFadeEnd.value) cg_timescale.value = cg_timescaleFadeEnd.value;
		} else {
			cg_timescale.value -= cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if(cg_timescale.value < cg_timescaleFadeEnd.value) cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		if(cg_timescaleFadeSpeed.value) {
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value));
		}
	}

	{
		float* vel = cg.snap->ps.velocity;
		float* fwd = cg.refdef.viewaxis[0];
		float forwardSpeed = vel[0] * fwd[0] + vel[1] * fwd[1] + vel[2] * fwd[2];

		trap_Cvar_Set("r_movementBlurStrength", va("%f", forwardSpeed * 0.00005));
	}

	// actually issue the rendering calls
	CG_DrawActive();

	if(cg_stats.integer) {
		CG_Printf("cg.clientFrame:%i\n", cg.clientFrame);
	}
}
