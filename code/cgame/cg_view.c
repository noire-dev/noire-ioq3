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
static void CG_OffsetThirdPersonView(void) {
	vec3_t forward, right, up;
	vec3_t view, pivot, target;
	vec3_t traceMins = {-1.0f, -1.0f, -1.0f};
	vec3_t traceMaxs = {1.0f, 1.0f, 1.0f};
	trace_t trace;
	float pitch, yaw, cp, sp, cy, sy;
	float frameTime;
	vec3_t dir;

	// Genshin-style third-person camera.
	// The orbit pivot is the local character's head joint: the camera
	// travels on a sphere centered on the head so the whole body stays
	// in frame. Falls back to the classic eye position if the head joint
	// is unavailable.
	if(!CG_GetLocalPlayerHeadOrigin(pivot)) {
		VectorCopy(cg.refdef.vieworg, pivot);
		pivot[2] += cg.predictedPlayerState.viewheight;
	}

	pitch = cg.refdefViewAngles[PITCH];
	yaw = cg.refdefViewAngles[YAW] + cg_thirdPersonAngle.value;

	// Clamp pitch so the sphere point never ends up directly below the
	// character at extreme look angles.
	if(pitch > 75.0f) pitch = 75.0f;
	if(pitch < -75.0f) pitch = -75.0f;

	// Direction from camera toward pivot (the camera's look direction).
	cp = cos(pitch * M_PI / 180.0f);
	sp = sin(pitch * M_PI / 180.0f);
	cy = cos(yaw * M_PI / 180.0f);
	sy = sin(yaw * M_PI / 180.0f);
	dir[0] = cp * cy;
	dir[1] = cp * sy;
	dir[2] = -sp;

	// Sphere point at radius cg_thirdPersonRange behind the head pivot.
	VectorCopy(pivot, target);
	VectorMA(target, -cg_thirdPersonRange.value, dir, target);

	// Camera-local right/up axes for the offset (they orbit with the
	// camera, so cg_thirdPersonOffsetX is always "right shoulder" of the
	// current view, not a fixed world direction).
	{
		vec3_t angles;
		angles[PITCH] = pitch;
		angles[YAW] = yaw;
		angles[ROLL] = 0.0f;
		AngleVectors(angles, forward, right, up);
	}

	// Apply shoulder / vertical / depth offset.
	//   X = right shoulder (positive = camera to the right of view)
	//   Y = up            (positive = camera above view)
	//   Z = back          (positive = camera pulled further behind pivot)
	VectorMA(target, cg_thirdPersonOffsetX.value, right, target);
	VectorMA(target, cg_thirdPersonOffsetY.value, up, target);
	VectorMA(target, -cg_thirdPersonOffsetZ.value, forward, target);

	// Sphere collision with a 2x2x2 box: trace from the head pivot to the
	// (offset) sphere point. On hit, snap to the hit point so the camera
	// cannot clip through walls while orbiting. The local player is
	// skipped so their own body never blocks the orbit.
	CG_Trace(&trace, pivot, traceMins, traceMaxs, target, cg.predictedPlayerState.clientNum, MASK_PLAYERSOLID);
	if(trace.fraction < 1.0f) {
		VectorCopy(trace.endpos, target);
	}

	// Camera smoothing: exponential lag toward the target position.
	// cg_thirdPersonSmoothing 0 = instant, 100 = very smooth.
	// Also snap on teleport / first frame so the camera never flies
	// across the map when the pivot jumps.
	frameTime = (float)(cg.time - cg.oldTime) * 0.001f;
	if(frameTime <= 0.0f || frameTime > 0.1f) frameTime = 1.0f / 60.0f;

	{
		bool snap = !cg.thirdPersonCamValid || DistanceSquared(pivot, cg.thirdPersonCamLastPivot) > 200.0f * 200.0f;

		if(snap) {
			VectorCopy(target, cg.thirdPersonCamPos);
		} else {
			float smoothing = cg_thirdPersonSmoothing.value;
			if(smoothing < 0.0f) smoothing = 0.0f;
			if(smoothing > 100.0f) smoothing = 100.0f;

			if(smoothing < 0.5f) {
				VectorCopy(target, cg.thirdPersonCamPos);
			} else {
				// Exponential smoothing: time constant grows with the
				// slider. 1.0 = very responsive (~20ms), 100 = floaty
				// (~500ms). The exp form is framerate-independent.
				float timeConst = (smoothing / 100.0f) * 0.5f;
				float rate = 1.0f / timeConst;
				float alpha = 1.0f - exp(-rate * frameTime);
				if(alpha > 1.0f) alpha = 1.0f;
				if(alpha < 0.0f) alpha = 0.0f;

				VectorSubtract(target, cg.thirdPersonCamPos, view);
				VectorMA(cg.thirdPersonCamPos, alpha, view, cg.thirdPersonCamPos);
			}
		}

		VectorCopy(pivot, cg.thirdPersonCamLastPivot);
		cg.thirdPersonCamValid = true;
	}

	// Re-collide the smoothed position so the camera cannot clip while
	// smoothly lagging behind a wall (e.g. sliding along geometry).
	CG_Trace(&trace, pivot, traceMins, traceMaxs, cg.thirdPersonCamPos, cg.predictedPlayerState.clientNum, MASK_PLAYERSOLID);
	if(trace.fraction < 1.0f) {
		VectorCopy(trace.endpos, cg.thirdPersonCamPos);
	}

	VectorCopy(cg.thirdPersonCamPos, cg.refdef.vieworg);

	// Look at the pivot from the (possibly offset/smoothed/collision-
	// corrected) camera position so the character stays centered
	// regardless of where the camera ended up.
	VectorSubtract(pivot, cg.refdef.vieworg, view);
	if(VectorLength(view) > 0.01f) {
		vectoangles(view, cg.refdefViewAngles);
	} else {
		cg.refdefViewAngles[PITCH] = pitch;
		cg.refdefViewAngles[YAW] = yaw;
		cg.refdefViewAngles[ROLL] = 0.0f;
	}
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

	// first person: invalidate the smoothed third-person camera so that
	// returning to third person snaps to the sphere instead of flying in
	// from the last eye/head position
	cg.thirdPersonCamValid = false;
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
	fov_x = cg_fov.value;
	if(fov_x < 1) {
		fov_x = 1;
	} else if(fov_x > 160) {
		fov_x = 160;
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
static float cg_earthquakeIntensity;
static int cg_earthquakeEndTime;
static int cg_earthquakeStartTime;

void CG_StartEarthquake(float intensity, int duration) {
	if(intensity < 0.0f) intensity = 0.0f;
	if(intensity > 1.0f) intensity = 1.0f;
	if(duration < 1) duration = 1;
	if(cg.time >= cg_earthquakeEndTime || intensity >= cg_earthquakeIntensity) {
		cg_earthquakeIntensity = intensity;
		cg_earthquakeStartTime = cg.time;
		cg_earthquakeEndTime = cg.time + duration;
	}
}

static void CG_ApplyEarthquake(void) {
	float remain, duration, strength;
	if(cg_earthquakeIntensity <= 0.0f || cg.time >= cg_earthquakeEndTime) {
		cg_earthquakeIntensity = 0.0f;
		return;
	}
	duration = (float)(cg_earthquakeEndTime - cg_earthquakeStartTime);
	remain = (float)(cg_earthquakeEndTime - cg.time) / duration;
	strength = cg_earthquakeIntensity * remain;
	cg.refdef.vieworg[0] += sinf(cg.time * 0.071f) * strength * 7.0f;
	cg.refdef.vieworg[1] += cosf(cg.time * 0.083f) * strength * 5.0f;
	cg.refdef.vieworg[2] += sinf(cg.time * 0.109f) * strength * 3.0f;
	cg.refdefViewAngles[PITCH] += sinf(cg.time * 0.061f) * strength * 2.8f;
	cg.refdefViewAngles[YAW] += cosf(cg.time * 0.077f) * strength * 2.2f;
	cg.refdefViewAngles[ROLL] += sinf(cg.time * 0.097f) * strength * 1.8f;
}

static int CG_CalcViewValues(void) {
	playerState_t* ps;

	memset(&cg.refdef, 0, sizeof(cg.refdef));

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;

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

	// Nearby heavy prop impacts perturb the camera briefly.
	CG_ApplyEarthquake();

	// position eye relative to origin
	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);

	if(cg.hyperspace) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
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

	// Genshin-style third-person camera: the camera (viewangles) is fully
	// decoupled from the character facing. While idle in third person the
	// body keeps its last heading even as the camera orbits; it turns toward
	// the camera yaw only while moving or firing. In first person (full body
	// awareness) facing is snapped to viewangles so head-joint tracking stays
	// consistent. Follow mode is ignored because the followed player's body
	// is not under our control.
	if(cg.renderingThirdPerson) {
		playerState_t* cps = &cg.predictedPlayerState;

		if(cps->stats[STAT_HEALTH] > 0) {
			float velSq = cps->velocity[0] * cps->velocity[0] + cps->velocity[1] * cps->velocity[1];
			bool turning = (velSq > 100.0f) || (cps->eFlags & EF_FIRING);

			if(turning) {
				float targetYaw = cps->viewangles[YAW];
				float delta = targetYaw - cg.charYaw;
				float frameTime = (float)(cg.time - cg.oldTime) * 0.001f;
				float rate;

				while(delta > 180.0f) delta -= 360.0f;
				while(delta < -180.0f) delta += 360.0f;

				if(frameTime <= 0.0f || frameTime > 0.1f) frameTime = 1.0f / 60.0f;
				rate = 20.0f * frameTime;
				if(rate > 1.0f) rate = 1.0f;

				cg.charYaw += delta * rate;

				while(cg.charYaw > 360.0f) cg.charYaw -= 360.0f;
				while(cg.charYaw < 0.0f) cg.charYaw += 360.0f;
			}
		}
	} else {
		cg.charYaw = cg.predictedPlayerState.viewangles[YAW];
	}

	// build cg.refdef
	inwater = CG_CalcViewValues();

	// build the render lists
	if(!cg.hyperspace) {
		CG_AddPacketEntities();  // adter calcViewValues, so predicted player state is correct
		CG_AddMarks();
		CG_RenderParticles();
		CG_AddLocalEntities();
	}

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
