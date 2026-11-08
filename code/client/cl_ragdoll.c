// Copyright (C) 2026 Noire's Mod [noire.dev] — GPLv2
//
// cl_ragdoll.c - parsing of the ragdoll states that follow the packet entities
// in a snapshot message (counterpart of SV_EmitPacketRagdolls in sv_snapshot.c).
//
// Needs, in client.h:
//   clSnapshot_t:   int numRagdolls; int parseRagdollsNum;
//   clientActive_t: ragdollState_t parseRagdolls[MAX_PARSE_RAGDOLLS]; int parseRagdollsNum;
//   #define MAX_PARSE_RAGDOLLS 1024   (power of two, >= PACKET_BACKUP * MAX_SNAPSHOT_RAGDOLLS)
// and in CL_ParseSnapshot (cl_parse.c), right after CL_ParsePacketEntities():
//   CL_ParsePacketRagdolls(msg, old, &newSnap);
// See RAGDOLL_PATCH_NOTES.md.

#include "client.h"

static void CL_DeltaRagdoll(msg_t* msg, clSnapshot_t* frame, int newnum, ragdollState_t* old, bool unchanged) {
	ragdollState_t* state;

	state = &cl.parseRagdolls[cl.parseRagdollsNum & (MAX_PARSE_RAGDOLLS - 1)];

	if(unchanged) {
		*state = *old;
	} else {
		MSG_ReadDeltaRagdollState(msg, old, state, newnum);
	}

	if(state->number == RAGDOLL_NUM_NONE) {
		return;  // ragdoll was delta removed
	}

	cl.parseRagdollsNum++;
	frame->numRagdolls++;
}

/*
==================
CL_ParsePacketRagdolls

Same merge walk as CL_ParsePacketEntities: ragdolls of the old frame that are not
mentioned are unchanged, a mention is a delta (or a remove), a ragdoll that is not
in the old frame is a delta from an empty state.
==================
*/
void CL_ParsePacketRagdolls(msg_t* msg, clSnapshot_t* oldframe, clSnapshot_t* newframe) {
	int newnum;
	ragdollState_t* oldstate;
	int oldindex, oldnum;

	newframe->parseRagdollsNum = cl.parseRagdollsNum;
	newframe->numRagdolls = 0;

	oldindex = 0;
	oldstate = NULL;
	if(!oldframe) {
		oldnum = 99999;
	} else if(oldindex >= oldframe->numRagdolls) {
		oldnum = 99999;
	} else {
		oldstate = &cl.parseRagdolls[(oldframe->parseRagdollsNum + oldindex) & (MAX_PARSE_RAGDOLLS - 1)];
		oldnum = oldstate->number;
	}

	while(1) {
		newnum = MSG_ReadBits(msg, RAGDOLL_NUM_BITS);
		if(newnum == RAGDOLL_NUM_NONE) {
			break;
		}

		if(msg->readcount > msg->cursize) {
			Com_Error(ERR_DROP, "CL_ParsePacketRagdolls: end of message");
		}

		while(oldnum < newnum) {
			// one or more ragdolls from the old packet are unchanged
			CL_DeltaRagdoll(msg, newframe, oldnum, oldstate, true);

			oldindex++;
			if(oldindex >= oldframe->numRagdolls) {
				oldnum = 99999;
			} else {
				oldstate = &cl.parseRagdolls[(oldframe->parseRagdollsNum + oldindex) & (MAX_PARSE_RAGDOLLS - 1)];
				oldnum = oldstate->number;
			}
		}
		if(oldnum == newnum) {
			// delta from previous state
			CL_DeltaRagdoll(msg, newframe, newnum, oldstate, false);

			oldindex++;
			if(oldindex >= oldframe->numRagdolls) {
				oldnum = 99999;
			} else {
				oldstate = &cl.parseRagdolls[(oldframe->parseRagdollsNum + oldindex) & (MAX_PARSE_RAGDOLLS - 1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if(oldnum > newnum) {
			// new ragdoll: delta from an empty state
			CL_DeltaRagdoll(msg, newframe, newnum, NULL, false);
			continue;
		}
	}

	// any remaining ragdolls in the old frame are copied over
	while(oldnum != 99999) {
		CL_DeltaRagdoll(msg, newframe, oldnum, oldstate, true);

		oldindex++;
		if(oldindex >= oldframe->numRagdolls) {
			oldnum = 99999;
		} else {
			oldstate = &cl.parseRagdolls[(oldframe->parseRagdollsNum + oldindex) & (MAX_PARSE_RAGDOLLS - 1)];
			oldnum = oldstate->number;
		}
	}
}
