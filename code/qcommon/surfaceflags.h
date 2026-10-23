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
// This file must be identical in the quake and utils directories

// contents flags are separate bits
// a given brush can contribute multiple content bits

// these definitions also need to be in q_shared.h!

#define CONTENTS_SOLID 1  // an eye is never valid in a solid
#define CONTENTS_LAVA 8
#define CONTENTS_SLIME 16
#define CONTENTS_WATER 32
#define CONTENTS_FOG 64

#define CONTENTS_NOTTEAM1 0x0080
#define CONTENTS_NOTTEAM2 0x0100
#define CONTENTS_NOBOTCLIP 0x0200

#define CONTENTS_AREAPORTAL 0x8000

#define CONTENTS_PLAYERCLIP 0x10000
#define CONTENTS_MONSTERCLIP 0x20000
// bot specific contents types
#define CONTENTS_TELEPORTER 0x40000
#define CONTENTS_JUMPPAD 0x80000
#define CONTENTS_CLUSTERPORTAL 0x100000
#define CONTENTS_DONOTENTER 0x200000
#define CONTENTS_BOTCLIP 0x400000
#define CONTENTS_MOVER 0x800000

#define CONTENTS_ORIGIN 0x1000000  // removed before bsping an entity

#define CONTENTS_BODY 0x2000000  // should never be on a brush, only in game
#define CONTENTS_CORPSE 0x4000000
#define CONTENTS_DETAIL 0x8000000        // brushes not used for the bsp
#define CONTENTS_STRUCTURAL 0x10000000   // brushes used for the bsp
#define CONTENTS_TRANSLUCENT 0x20000000  // don't consume surface fragments inside
#define CONTENTS_TRIGGER 0x40000000
#define CONTENTS_NODROP 0x80000000  // don't leave bodies or items (death fog, lava)

#define SURF_NODAMAGE 1          // never give falling damage
#define SURF_SLICK 2             // effects game physics
#define SURF_SKY 4               // lighting from environment map
#define SURF_NOMARKS 8           // don't leave missile marks
#define SURF_NODRAW 16           // don't generate a drawsurface at all
#define SURF_HINT 32             // make a primary bsp splitter
#define SURF_SKIP 64             // completely ignore, allowing non-closed brushes
#define SURF_NOLIGHTMAP 128      // surface doesn't need a lightmap
#define SURF_POINTLIGHT 256      // generate lighting info at vertexes
#define SURF_NONSOLID 512        // don't collide against curves with this set
#define SURF_LIGHTFILTER 1024    // act as a light filter during q3map -light
#define SURF_ALPHASHADOW 2048    // do per-pixel light shadow casting in q3map
#define SURF_CUBEMAP 4096        // reflective?
#define SURF_CUBEMAP_WATER 8192  // correct reflect?

/* --- material flags --- */
#define SURF_CHAINLINK 16384
#define SURF_CONCRETE 32768
#define SURF_DIRT 65536
#define SURF_DUCT 131072
#define SURF_GRASS 262144
#define SURF_GRAVEL 524288
#define SURF_LADDER 1048576
#define SURF_METAL 2097152
#define SURF_METALGRATE 4194304
#define SURF_MUD 8388608
#define SURF_SAND 16777216
#define SURF_SLOSH 33554432
#define SURF_TILE 67108864
#define SURF_WOOD 134217728
#define SURF_WOODPANEL 268435456

#define SURF_SKYBOX 536870912