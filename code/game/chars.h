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

// bot characteristics, the index numbers are the bot character file format

// attack skill
//  >  0.0 && <  0.2 = don't move
//  >  0.3 && <  1.0 = aim at enemy during retreat
//  >  0.0 && <  0.4 = only move forward/backward
//  >= 0.4 && <  1.0 = circle strafing
//  >  0.7 && <  1.0 = random strafe direction change
#define CHARACTERISTIC_ATTACK_SKILL 2  // float [0, 1]
// view angle difference to angle change factor
#define CHARACTERISTIC_VIEW_FACTOR 4  // float <0, 1]
// maximum view angle change
#define CHARACTERISTIC_VIEW_MAXCHANGE 5  // float [1, 360]
// reaction time in seconds
#define CHARACTERISTIC_REACTIONTIME 6  // float [0, 5]
// accuracy when aiming
#define CHARACTERISTIC_AIM_ACCURACY 7  // float [0, 1]
// skill when aiming
//  >  0.0 && <  0.9 = aim is affected by enemy movement
//  >  0.4 && <= 0.8 = enemy linear leading
//  >  0.8 && <= 1.0 = enemy exact movement leading
//  >  0.5 && <= 1.0 = prediction shots when enemy is not visible
//  >  0.6 && <= 1.0 = splash damage by shooting nearby geometry
#define CHARACTERISTIC_AIM_SKILL 16  // float [0, 1]
// movement
// tendency to crouch
#define CHARACTERISTIC_CROUCHER 36  // float [0, 1]
// tendency to jump
#define CHARACTERISTIC_JUMPER 37  // float [0, 1]
// tendency to walk
#define CHARACTERISTIC_WALKER 48  // float [0, 1]
// tendency to jump using a weapon
#define CHARACTERISTIC_WEAPONJUMPING 38  // float [0, 1]
// how alert the bot is (view distance)
#define CHARACTERISTIC_ALERTNESS 46  // float [0, 1]
// how much the bot fires its weapon
#define CHARACTERISTIC_FIRETHROTTLE 47  // float [0, 1]
