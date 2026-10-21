// Slice s004532f0: Agent::shield_hit (a blow on the held shield: durability loss by damage minus the shield's
// stat and the troop's skill, shield break handling, and the "ui_shield_*" damage-state messages).
#include "agent_fx.h"

float frand();                                      // 0x40e400

#define itp_type_shield 7
// the stored copy of troop t's slot i: the 0x508-byte troop records are indexed as a flat array of ItemRef
#define STORED(t, i) (g_game->chars->items[(t) * 0xa1 + (i)])
#define imod_cracked 1                              // header_item_modifiers.py: imod_cracked = 1 (item marked broken)

// @ 0x4532f0
void Agent::shield_hit(int damage)
{
    int cur = wield_b;
    if (cur >= 0 && f23c == 0) {
        ItemRef* it = &items[cur];
        if (it && (g_items[it->item].type_flags & 0xff) == itp_type_shield) {
            damage -= it->stat_200();
            float f = 0.0f > (float)damage ? 0.0f : (float)damage;
            int skill = g_game->skill_level(troop, 0x1a);
            int d = (int)((1.0f - (float)skill * 0.08f) * (float)(int)f);

            if ((float)d > 0.0f) {
                unsigned v = (it->m4 & 0xffff) - d;
                *(short*)&it->m4 = 0;
                it->m4 |= ((int)v < 0) - 1 & v;
                if ((it->m4 & 0xffff) == 0) {
                    int old = wield_b;
                    wield_b = -1;
                    on_wield_reset();
                    set_order(1, move_anim(0), 0x1000010, 1.0f);
                    float var = 0.0f;
                    int kind = 0x5e;
                    play_sound_var(&kind, &var);
                    mat4 M = body->m * *(mat4*)body->skel->rec(0xc);
                    int mod = it->b.mod;
                    kind = 7;
                    g_mission->add_placed(&kind, &it->item, &mod, (matrix*)&M, &id);
                    if (((g_rec58[g_mission->cur_rec].flags >> 1) & 1) && old >= 0 &&
                        id == g_mission->local_id && old < 10 && b220[old]) {
                        ItemRef* s = &items[old];
                        if (s->item >= 0 && (g_items[s->item].type_flags & 0xff) == itp_type_shield) {
                            unsigned bits;
                            int m = s->b.mod;
                            if (m == 1) {
                                if (frand() < 0.2f) {
                                    { tstring key("ui_shield_broken"); g_msglog.add(*g_strtable.lookup(key), 0xddbbbb); }
                                    
                                    if (s->item == STORED(g_game->player, old).item && items[old].m4 == STORED(g_game->player, old).m4)
                                        STORED(g_game->player, old).item = -1;
                                }
                                return;
                            } else if (m == 5) {
                                if (!(frand() < 0.2f))
                                    return;
                                { tstring key("ui_shield_cracked"); g_msglog.add(*g_strtable.lookup(key), 0xddbbbb); }
                                bits = 0x1000000;
                            } else {
                                if (!(frand() < 0.2f))
                                    return;
                                { tstring key("ui_shield_deformed"); g_msglog.add(*g_strtable.lookup(key), 0xddbbbb); }
                                bits = 0x5000000;
                            }
                            
                            if (s->item == STORED(g_game->player, old).item && items[old].m4 == STORED(g_game->player, old).m4) {
                                ItemRef* r2 = &STORED(troop, old);
                                r2->b.mod = 0;
                                r2->m4 |= bits;
                                r2->reduce(0);
                            }
                            s->b.mod = 0;
                            s->m4 |= bits;
                            s->reduce(0);
                        }
                    }
                }
            }
        }
    }
}
