// agent_rec.h: what terrain_b keeps beside the agent record: the record itself is Agent (game/agent/agent_obj.h; its constructor
// 0x477a30 and reset 0x472f90 are defined in poly_mesh.cpp), and EmptySlot is the out-of-line empty slot 1 / 2 of the
// IfaceB-style records (0x477b70).
#ifndef TERRAIN_B_AGENT_REC_H
#define TERRAIN_B_AGENT_REC_H
#include "engine/math/rgl_math.h"
#include "game/agent/agent_obj.h"                 // Agent
struct EmptySlot { virtual void slot(); };      // 0x477b70: the empty slot 1 / 2 of the IfaceB-style records
#endif // TERRAIN_B_AGENT_REC_H
