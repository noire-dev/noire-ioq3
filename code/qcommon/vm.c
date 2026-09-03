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
// vm.c -- virtual machine

/*


intermix code and data
symbol table

a dll has one imported function: VM_SystemCall
and one exported function: Perform


*/

#include "vm_local.h"

vm_t* currentVM = NULL;
vm_t* lastVM = NULL;

// used by Com_Error to get rid of running vm's before longjmp
static int forced_unload;

#define MAX_VM 3
vm_t vmTable[MAX_VM];

void VM_Init(void) { Com_Memset(vmTable, 0, sizeof(vmTable)); }

intptr_t QDECL VM_DllSyscall(intptr_t arg, ...) {
#if !id386 || defined __clang__
	// rcg010206 - see commentary above
	intptr_t args[MAX_VMSYSCALL_ARGS];
	int i;
	va_list ap;

	args[0] = arg;

	va_start(ap, arg);
	for(i = 1; i < ARRAY_LEN(args); i++) args[i] = va_arg(ap, intptr_t);
	va_end(ap);

	return currentVM->systemCall(args);
#else  // original id code
	return currentVM->systemCall(&arg);
#endif
}

vm_t* VM_Restart(vm_t* vm) {
	vmHeader_t* header;

	if(vm->dllHandle) {
		char name[MAX_QPATH];
		intptr_t (*systemCall)(intptr_t* parms);

		systemCall = vm->systemCall;
		Q_strncpyz(name, vm->name, sizeof(name));

		VM_Free(vm);

		vm = VM_Create(name, systemCall);
		return vm;
	}

	return NULL;
}

vm_t* VM_Create(const char* module, intptr_t (*systemCalls)(intptr_t*)) {
	vm_t* vm;
	int i, retval;
	char filename[MAX_OSPATH];
	void* startSearch = NULL;

	if(!module || !module[0] || !systemCalls) Com_Error(ERR_FATAL, "VM_Create: bad parms");

	// see if we already have the VM
	for(i = 0; i < MAX_VM; i++) {
		if(!Q_stricmp(vmTable[i].name, module)) {
			vm = &vmTable[i];
			return vm;
		}
	}

	// find a free vm
	for(i = 0; i < MAX_VM; i++) {
		if(!vmTable[i].name[0]) {
			break;
		}
	}

	if(i == MAX_VM) {
		Com_Error(ERR_FATAL, "VM_Create: no free vm_t");
	}

	vm = &vmTable[i];

	Q_strncpyz(vm->name, module, sizeof(vm->name));

	do {
		retval = FS_FindVM(&startSearch, filename, sizeof(filename), module);

		if(retval == VMI_NATIVE) {
			Com_Printf("Try loading dll file %s\n", filename);

			vm->dllHandle = Sys_LoadGameDll(filename, &vm->entryPoint, VM_DllSyscall);

			if(vm->dllHandle) {
				vm->systemCall = systemCalls;
				return vm;
			}

			Com_Printf("Failed loading dll\n");
		}
	} while(retval >= 0);

	return NULL;
}

void VM_Free(vm_t* vm) {
	if(!vm) return;

	if(vm->callLevel) {
		if(!forced_unload) {
			Com_Error(ERR_FATAL, "VM_Free(%s) on running vm", vm->name);
			return;
		} else {
			Com_Printf("forcefully unloading %s vm\n", vm->name);
		}
	}

	if(vm->destroy) vm->destroy(vm);

	if(vm->dllHandle) {
		Sys_UnloadDll(vm->dllHandle);
		Com_Memset(vm, 0, sizeof(*vm));
	}
	Com_Memset(vm, 0, sizeof(*vm));

	currentVM = NULL;
	lastVM = NULL;
}

void VM_Clear(void) {
	int i;
	for(i = 0; i < MAX_VM; i++) {
		VM_Free(&vmTable[i]);
	}
}

void VM_Forced_Unload_Start(void) { forced_unload = 1; }

void VM_Forced_Unload_Done(void) { forced_unload = 0; }

void* VM_ArgPtr(intptr_t intValue) {
	if(!intValue) return NULL;

	// currentVM is missing on reconnect
	if(currentVM == NULL) return NULL;

	if(currentVM->entryPoint) return (void*)(currentVM->dataBase + intValue);
	return NULL;
}

void* VM_ExplicitArgPtr(vm_t* vm, intptr_t intValue) {
	if(!intValue) return NULL;

	// currentVM is missing on reconnect here as well?
	if(currentVM == NULL) return NULL;

	if(vm->entryPoint) return (void*)(vm->dataBase + intValue);
	return NULL;
}

intptr_t QDECL VM_Call(vm_t* vm, int callnum, ...) {
	vm_t* oldVM;
	intptr_t r;
	int i;

	if(!vm || !vm->name[0]) Com_Error(ERR_FATAL, "VM_Call with NULL vm");

	oldVM = currentVM;
	currentVM = vm;
	lastVM = vm;

	++vm->callLevel;
	if(vm->entryPoint) {
		int args[MAX_VMMAIN_ARGS - 1];
		va_list ap;
		va_start(ap, callnum);
		for(i = 0; i < ARRAY_LEN(args); i++) {
			args[i] = va_arg(ap, int);
		}
		va_end(ap);

		r = vm->entryPoint(callnum, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11]);
	}
	--vm->callLevel;

	if(oldVM != NULL) currentVM = oldVM;
	return r;
}
