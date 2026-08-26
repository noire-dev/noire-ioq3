// Copyright (C) 2023-2026 Noire's Mod — GPLv2

#ifndef __JAVASCRIPT_CORE_H
#define __JAVASCRIPT_CORE_H

char* trap_Cvar_VariableString(const char* var_name);
void StringCopy(char* dest, const char* source, int destsize);

#define NONE -1

#define bounds(x, max, name)                                                    			\
	do {                                                                        			\
		if((x) < 0) {                                                           			\
			trap_Print(va("#ff5Out of Bounds in %s array: %d < 0", name, (x))); 			\
			(x) = 0;                                                            			\
		} else if((x) >= (max)) {                                               			\
			trap_Print(va("#ff5Out of Bounds in %s array: %d >= %d", name, (x), (max))); 	\
			(x) = (max) - 1;                                                    			\
		}                                                                       			\
	} while(0)

#define clamp(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#endif
