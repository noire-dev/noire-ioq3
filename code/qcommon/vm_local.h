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
#include "q_shared.h"
#include "qcommon.h"

// Max number of arguments to pass from engine to vm's vmMain function.
// command number + 12 arguments
#define MAX_VMMAIN_ARGS 13

// Max number of arguments to pass from a vm to engine's syscall handler function for the vm.
// syscall number + 15 arguments
#define MAX_VMSYSCALL_ARGS 16

struct vm_s {
	intptr_t (*systemCall)(intptr_t* parms);

	char name[MAX_QPATH];

	// for dynamic linked modules
	void* dllHandle;
	vmMainProc entryPoint;
	void (*destroy)(vm_t* self);

	bool compiled;
	byte* codeBase;
	int entryOfs;
	int codeLength;

	intptr_t* instructionPointers;
	int instructionCount;

	byte* dataBase;
	int dataMask;
	int dataAlloc;  // actually allocated

	int stackBottom;  // if programStack < stackBottom, error

	int numSymbols;
	struct vmSymbol_s* symbols;

	int callLevel;      // counts recursive VM_Call
	int breakFunction;  // increment breakCount on function entry to this
	int breakCount;

	byte* jumpTableTargets;
	int numJumpTableTargets;
};

extern vm_t* currentVM;
