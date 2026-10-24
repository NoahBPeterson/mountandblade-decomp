// troop_ext.h: Troop members seen from outside their translation unit (an undefined declaration). The original calls
// 0x4375b0 (from Troop::adjust 0x437770) and 0x48be70 (from add_attr 0x48bef0) with `this` kept in a callee-saved
// register, as for a callee in another object; a callee defined in the same file lets cl assume its register use and
// inline part of it. The members are the ones of Troop (include/troop.h), so the casts keep the layout: no fields.
// ONE definition for src/game/agent (agent.cpp) and src/game/mission_ui2 (stat_panel_x.cpp).
#ifndef GAME_TROOP_EXT_H
#define GAME_TROOP_EXT_H

struct TroopExt {
    float falloff(float dist, float k);         // 0x4375b0
    void set_attr(int idx, int val);            // 0x48be70
};

#endif // GAME_TROOP_EXT_H
