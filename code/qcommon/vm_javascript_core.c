// Copyright (C) 2023-2026 Noire's Mod — GPLv2

#include "vm_javascript.h"

char cvar_strings[MAX_CVARS][MAX_CVAR_VALUE_STRING];

char* trap_Cvar_VariableString(const char* var_name) {
	char cvarValueStringBuffer[MAX_CVAR_VALUE_STRING];
	int cvarID = trap_Cvar_VariableStringBuffer(var_name, cvarValueStringBuffer, MAX_CVAR_VALUE_STRING);

	if(cvarID != -1) {
		strcpy(cvar_strings[cvarID], cvarValueStringBuffer);
		return cvar_strings[cvarID];
	} else {
		return "String not found!";
	}
}

void StringCopy(char* dest, const char* source, int destsize) {
	if(!dest) {
		trap_Print("StringCopy: NULL dest \n");
		return;
	}
	if(!source) {
		trap_Print("StringCopy: NULL source \n");
		return;
	}
	if(destsize < 1) {
		trap_Print("StringCopy: destsize < 1 \n");
		return;
	}

	strncpy(dest, source, destsize - 1);
	dest[destsize - 1] = 0;
}
