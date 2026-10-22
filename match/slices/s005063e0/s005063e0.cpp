// Slice s005063e0: Mission::refresh_party_window (provisional name) -- rebuild the "party" screen for the
// party selected in the mission (g_command), the troop slot selected in it and the troop record it shows:
// enable/disable the panels, build the action button label from the command (give/take/sell/hire/…), pay the
// price and wages lines, the two upgrade buttons and the "no slot for upgrade" notifications, then push the
// whole state into the widgets. Layouts come from the neighbouring slices (Mission = the s0049f060 layout with
// the party screen fields added below); every offset is read off the disassembly.
#include <windows.h>
#include <stdio.h>

// ---- engine string -----------------------------------------------------------------------------------
struct tstring {
    int pos;                                    // +0
    char* buf;                                  // +4
    int cap;                                    // +8
    int len;                                    // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
struct StrTable { tstring* lookup(const tstring& key); };       // 0x43a1e0
extern StrTable g_strtable;                                     // 0x5e2a18

// ---- config table at 0xa44240 (0x2c-byte entries, s004bd170) -----------------------------------------
struct ini_file { float get_float(const tstring& key); };      // 0x4bd5e0
extern ini_file g_config;                                       // 0xa44240

// ---- widgets ------------------------------------------------------------------------------------------
// A screen widget: set_image(const tstring*, const vector4*) (slot 0xa4), set_text(slot 0xb4 on the
// label/quad handles), slot 0xb8 (enable), slot 0xbc (disable/hide), set_colour(slot 0x98),
// set_tag(slot 0xac).
struct Widget;
struct Widget {
    virtual void w00(); virtual void w01(); virtual void w02(); virtual void w03(); virtual void w04();
    virtual void w05(); virtual void w06(); virtual void w07(); virtual void w08(); virtual void w09();
    virtual void w10(); virtual void w11(); virtual void w12(); virtual void w13(); virtual void w14();
    virtual void w15(); virtual void w16(); virtual void w17(); virtual void w18(); virtual void w19();
    virtual void w20(); virtual void w21(); virtual void w22(); virtual void w23(); virtual void w24();
    virtual void w25(); virtual void w26(); virtual void w27(); virtual void w28(); virtual void w29();
    virtual void w30(); virtual void w31();
    virtual void s80(const void* a, const void* b);              // +0x80 (two pointers, or two ints)
    virtual void set_colour(int* c);                            // +0x84
    virtual void w34(); virtual void w35(); virtual void w36(); virtual void w37();
    virtual void w38(); virtual void w39(); virtual void w40();
    virtual void set_image(const tstring* t, const tstring* mask);   // +0xa4
    virtual void w42();
    virtual void set_number(const tstring* t, const float* pt);  // +0xac
    virtual void set_text(const void* s);                        // +0xb0
    virtual void set_text_at(Widget* w, const tstring* s);       // +0xb4
    virtual void enable();                                       // +0xb8
    virtual void disable();                                      // +0xbc
    virtual void w48(); virtual void w49();
    virtual void w50(); virtual void w51(); virtual void w52(); virtual void w53(); virtual void w54();
    virtual void w90(); virtual void w91(); virtual void w92(); virtual void w93(); virtual void w94();
    virtual void w95(); virtual void w96(); virtual void w97(); virtual void w98(); virtual void w99();
    virtual void w100(); virtual void w101(); virtual void w102(); virtual void w103(); virtual void w104();
    virtual void w105(); virtual void w106(); virtual void w107(); virtual void w108(); virtual void w109();
    virtual void w110(); virtual void w111(); virtual void w112(); virtual void w113(); virtual void w114();
    virtual void w115(); virtual void w116(); virtual void w117(); virtual void w118(); virtual void w119();
    virtual void w120(); virtual void w121(); virtual void w122(); virtual void w123(); virtual void w124();
    virtual void w125(); virtual void w126(); virtual void w127(); virtual void w128(); virtual void w129();
    virtual void w130(); virtual void w131(); virtual void w132(); virtual void w133(); virtual void w134();
    virtual void w135(); virtual void w136(); virtual void w137(); virtual void w138(); virtual void w139();
    virtual void w140(); virtual void w141(); virtual void w142(); virtual void w143(); virtual void w144();
    virtual void w145(); virtual void w146(); virtual void w147(); virtual void w148(); virtual void w149();
    virtual void w150(); virtual void w151(); virtual void w152(); virtual void w153(); virtual void w154();
    virtual void w155(); virtual void w156(); virtual void w157(); virtual void w158(); virtual void w159();
    virtual void w160(); virtual void w161(); virtual void w162(); virtual void w163(); virtual void w164();
    virtual void w165(); virtual void w166(); virtual void w167(); virtual void w168(); virtual void w169();
    virtual void w170(); virtual void w171(); virtual void w172(); virtual void w173(); virtual void w174();
    virtual void w175(); virtual void w176(); virtual void w177(); virtual void w178(); virtual void w179();
    virtual void w180(); virtual void w181(); virtual void w182(); virtual void w183(); virtual void w184();
    virtual void w185(); virtual void w186(); virtual void w187(); virtual void w188(); virtual void w189();
    virtual void w190(); virtual void w191(); virtual void w192(); virtual void w193(); virtual void w194();
    virtual void w195(); virtual void w196(); virtual void w197(); virtual void w198(); virtual void w199();
    virtual void w200(); virtual void w201(); virtual void w202(); virtual void w203(); virtual void w204();
    virtual void w205(); virtual void w206(); virtual void w207(); virtual void w208(); virtual void w209();
};

void pair_name(Widget* w, const tstring* s) { w->set_image(s, s); }     // 0x43a1c0 (visible name: 0x43a1e0)
void pair_tmp(Widget* w, const tstring* s) { w->set_image(s, s); }      // 0x43a1c0 alt

// ---- game data ----------------------------------------------------------------------------------------
struct TroopRec {                                               // 0x508 bytes
    unsigned flags;                                             // +0x30
    int f34;                                                    // +0x34
    char pad38[0x50 - 0x38];
    int f50;                                                    // +0x50
    int f54;                                                    // +0x54
    tstring* display_name();                                    // 0x43a480
};

struct Party {
    int f04;                                                    // +0x04
    char pad8[0x5ac8 - 0x8];
    int ext_463190();                                           // 0x463190 (thiscall count)
    int find_or_add(unsigned troop, unsigned flag, char force); // 0x489620
};

struct GameData {
    char pad0[0x78];
    int player_party;                                           // +0x78
    char pad7c[0x8c - 0x7c];
    int player;                                                 // +0x8c
    char pad90[0xf8 - 0x90];
    char parties[0x138d0];
    TroopRec* chars;                                            // +0x13a00
    char pad13a04[0x13a20 - 0x13a04];
    int price;                                                  // +0x13a20
    int sl_query(int kind, int troop, int arg);                 // 0x435790
    int sl_party_value(int party);                              // 0x48a290
    int sl_party_price(int party);                              // 0x48a320
};
extern GameData* g_game;                                        // 0x5e2930
extern int g_result;                                            // 0xa70458 (sl_query out slot 0)
extern int g_store1;                                            // 0xa7045c (sl_query out slot 1)

struct PartyMgr { Party* get(int id); };                        // 0x51ee50
#define PARTIES ((PartyMgr*)((char*)g_game + 0xf8))

// ---- mission ------------------------------------------------------------------------------------------
struct Mission {
    char pad00[0x04];
    int f04;                                                    // +0x04 (g_command)
    char pad08[4];
    int f0c;
    int f10;                                                    // +0x10
    char pad14[0x38 - 0x14];
    Widget* w38;                                                // +0x38
    Widget* w3c;                                                // +0x3c
    Widget* w40;                                                // +0x40
    Widget* w44;                                                // +0x44
    Widget* w48;                                                // +0x48
    Widget* w4c;                                                // +0x4c
    Widget* w50;                                                // +0x50
    Widget* w54;                                                // +0x54
    Widget* w58;                                                // +0x58
    char pad5c[0x7c - 0x5c];
    Widget* w7c;                                                // +0x7c
    char pad80[0x84 - 0x80];
    Widget* w84;                                                // +0x84
    char pad88[0x3908 - 0x88];
    char f3908;                                                 // +0x3908
    char pad3909[4];
    Widget* w390c;                                              // +0x390c
    char pad3910[0xb8];
    int slots[0x90];                                            // +0x4c8 (index (slot + 2*type) << 6; only +4 of each entry is read)
    int f8c0;
    int f8c4;
    int f8c8;
    int f8cc;                                                   // +0x8cc
    int f8d0;                                                   // +0x8d0
    int f8d4;                                                   // +0x8d4
    int f8d8;                                                   // +0x8d8
    int f8dc;                                                   // +0x8dc
    int f8e0;                                                   // +0x8e0
    int f8e4;                                                   // +0x8e4
    char pad8e8[0x5ac8 - 0x8e8];

    void refresh_party_window(int arg);                         // 0x507310
};
extern Mission* g_mission;                                      // 0x5e292c

// ---- stacks ------------------------------------------------------------------------------------------
struct ItemPair { int item; int modifier; };                    // 0x20-byte stack: ItemPair[8]
struct StackInfo {
    ItemPair items[8];                                          // +0x00
    unsigned flags;                                             // +0x20
    int f24;                                                    // +0x24
    int f28;                                                    // +0x28
    int f2c;                                                    // +0x2c
    int f30;                                                    // +0x30
    int f34;                                                    // +0x34
    int f38;                                                    // +0x38
    int f3c;                                                    // +0x3c
};                                                              // 0x40 = 8 dwords + 8

static int iabs(int v) { return v < 0 ? -v : v; }

// @ 0x5063e0
void Mission::refresh_party_window(int arg)
{
    tstring zero("");
    tstring tmp;

    w84->set_image(&zero, &zero);

    w44->enable();
    f3908 = 1;
    w50->enable();
    w50->set_text(0);
    w48->enable();
    w4c->enable();
    w48->set_text(0);
    w4c->set_text(0);
    w54->enable();
    w58->enable();
    w3c->set_text(0);
    w38->set_text(0);
    if (w390c) {
        tmp = tstring("");
        w7c->set_image(&tmp, &tmp);
        w390c->s80(0, 0);
    }

    bool action = false;
    if (f8e4 == -1 || f8dc == -1 || f8e0 == -1) {
        return;
    }
    int troop_type = f8dc;
    int p1 = troop_type != 0 ? g_game->player_party : f10;
    int slot_idx = slots[(f8e4 + troop_type * 2 + f8e0 * 0x40) * 2 + 1];   // (f8e0*2 + f8dc) << 6
    int p2 = troop_type != 0 ? f10 : g_game->player_party;
    StackInfo info = *(StackInfo*)((char*)PARTIES->get(p2) + 0xa8 + 0x20 * slot_idx);
    if (info.f24 < 0) {
        return;
    }
    TroopRec* rec = g_game->chars + info.f24;
    refresh_party_window(slot_idx);
    if (f04 == 0) {
        bool captured = f8e4 != -1 && !(f8e4 == 0 && f8e0 == 0);
        if (captured) {
            w44->disable();
        }
    }

    int cmd = f04;
    if (cmd == 0 || cmd == 5) {
        tstring k;
        if (f8dc == 0) {
            tstring s("ui_disband");
            k = s;
        } else if ((info.flags & 1) == 0) {
            tstring s("ui_take_back");
            k = s;
        } else {
            tstring s("ui_take_prisoner");
            k = s;
        }
        zero = *g_strtable.lookup(k);
        if (f8e4 != -1) {
            if (!(f8dc == 0 && f8e4 == 0 && f8e0 == 0)) {
                action = true;
            }
        }
    } else if (cmd == 1 || cmd == 6 || cmd == 7) {
        tstring k;
        if (f8dc == 0) {
            tstring s("ui_give");
            k = s;
        } else {
            tstring s("ui_take");
            k = s;
        }
        zero = *g_strtable.lookup(k);
        if (f8e4 != 0 || f8e0 != 0 || f04 == 6) {
            if (!(f8dc == 0 && f8e0 == 0 && f04 == 7)) {
                action = true;
            }
        }
    } else if (cmd == 2) {
        tstring k("ui_sell");
        zero = *g_strtable.lookup(k);
        if (f8dc == 0 && (info.flags & 1) != 0) {
            g_result = 1;
            g_store1 = 0;
            if (g_game->sl_query(0xb, info.f24, 0) != 0 && g_result == 0 && g_store1 == 0) {
                action = true;
            }
        }
    } else if (cmd == 3) {
        tstring k("ui_hire");
        zero = *g_strtable.lookup(k);
        if (f8dc != 0) {
            int diff = g_game->sl_party_price(info.f24) - f8cc;
            if (diff <= g_game->chars[g_game->player].f34) {
                action = true;
            } else {
                float y = g_config.get_float(tstring("party_info_warn_y"));
                float x = g_config.get_float(tstring("party_info_warn_x"));
                float pt[2];
                pt[0] = x;
                pt[1] = y;
                tstring msg("ui_notification_cant_hire");
                w84->set_number(g_strtable.lookup(msg), pt);
            }
        }
    } else if (cmd == 4) {
        action = true;
        tstring k;
        if (f8dc == 0) {
            tstring s("ui_uncapture");
            k = s;
        } else if ((info.flags & 1) == 0) {
            tstring s("ui_hire");
            k = s;
        } else {
            tstring s("ui_capture");
            k = s;
        }
        zero = *g_strtable.lookup(k);
    }

    if (p2 == g_game->player_party) {
        int n = PARTIES->get(g_game->player_party)->ext_463190();
        if (slot_idx > 1 && slot_idx < n) {
            w54->disable();
        }
        if (slot_idx > 0 && slot_idx < n - 1) {
            w58->disable();
        }
    }

    if (action) {
        if (PARTIES->get(p1)->find_or_add(info.f24, info.flags & 1, 1) == -1) {
            action = false;
            float y = g_config.get_float(tstring("party_info_warn_y"));
            float x = g_config.get_float(tstring("party_info_warn_x"));
            float pt[2];
            pt[0] = x;
            pt[1] = y;
            tstring msg("ui_party_capcity_reached");
            w84->set_number(g_strtable.lookup(msg), pt);
        }
    }

    if (((rec->flags & 0x10) == 0 || f8dc != 0 || f04 == 6) && (rec->flags & 0x10000000) == 0 && action) {
        w50->disable();
        f3908 = 0;
    }

    tstring price;
    {
        tstring src = zero;
        w50->set_image(&src, &price);
    }
    char price_buf[512];
    sprintf(price_buf, "");
    if (info.f24 == g_game->player || (info.flags & 1) != 0) {
        if (f04 == 2 && action) {
            int v;
            tstring k("ui_price");
            g_result = 0;
            g_store1 = 0;
            if (g_game->sl_query(10, slot_idx, g_game->player_party) != 0)
                v = g_result;
            else
                v = g_game->price;
            sprintf(price_buf, g_strtable.lookup(k)->c_str(), v);
            w3c->set_image(&tmp, &k);
            w3c->set_text(price_buf);
        }
    } else {
        char wage_buf[512];
        tstring k;
        if (f8dc == 1 && f04 == 3) {
            k = tstring("ui_joining_cost_weekly_wage");
            int v2 = g_game->sl_party_price(slot_idx);
            int v1 = g_game->sl_party_value(slot_idx);
            sprintf(wage_buf, g_strtable.lookup(k)->c_str(), v1, v2);
        } else {
            k = tstring("ui_weekly_wage");
            int v1 = g_game->sl_party_value(slot_idx);
            sprintf(wage_buf, g_strtable.lookup(k)->c_str(), v1);
        }
        w3c->set_image(&tmp, &k);
        w3c->set_text(wage_buf);
    }

    bool up1 = false;
    bool up2 = false;
    unsigned char nb = 0;
    char upgrade_buf[512];
    sprintf(upgrade_buf, "");
    int sel = info.f24;
    if (f8dc == 0 && (rec->flags & 0x10) == 0) {
        nb = info.flags & 1;
        if (nb == 0) {
            sprintf(upgrade_buf, g_strtable.lookup(tstring("ui_number_ready_to_upgrade"))->c_str(), f8e0);
            if (rec->f50 != 0) {
                up1 = true;
                {
                    tstring k("ui_upgrade_to_value");
                    sprintf(price_buf, g_strtable.lookup(k)->c_str(), g_game->chars[rec->f50].display_name()->c_str());
                    w48->set_image(&tmp, &k);
                }
                w48->set_colour(&g_strtable.lookup(tstring("party_b_member_t_c"))->len);
            }
            if (rec->f54 != 0) {
                up2 = true;
                {
                    tstring k("ui_upgrade_to_value");
                    sprintf(price_buf, g_strtable.lookup(k)->c_str(), g_game->chars[rec->f54].display_name()->c_str());
                    w4c->set_image(&tmp, &k);
                }
                w4c->set_colour(&g_strtable.lookup(tstring("party_b_member_t_c"))->len);
            }
            if (up1) {
                w48->set_text(upgrade_buf);
                w48->enable();
                if (f8e0 != 0) {
                    if (PARTIES->get(sel)->find_or_add(rec->f50, 0, 0) != -1)
                        up1 = true;
                }
            }
            if (up2) {
                w4c->set_text(upgrade_buf);
                w4c->enable();
                if (f8e0 != 0) {
                    if (PARTIES->get(sel)->find_or_add(rec->f54, 0, 0) != -1)
                        up2 = true;
                }
            }
        }
    }
    if (!up1) {
        if (nb == 0 && f8e0 <= p1) {
            w48->disable();
            up1 = true;
        }
    }
    if (up2) {
        w4c->disable();
    } else if (nb == 0 && f8e0 <= p1) {
        w4c->disable();
        up2 = true;
    }
    if (f8e0 != 0) {
        if (!up2) {
            tstring k("ui_notification_no_slot_for_upgrade");
            sprintf(price_buf, g_strtable.lookup(k)->c_str(), g_game->chars[rec->f54].display_name()->c_str());
            w4c->set_image(&tmp, &k);
            w4c->set_colour(&g_strtable.lookup(tstring("party_b_upgrade_t_c_passive"))->len);
        }
        if (nb == 0) {
            {
                tstring k("ui_notification_no_slot_for_upgrade");
                sprintf(price_buf, g_strtable.lookup(k)->c_str(), g_game->chars[rec->f50].display_name()->c_str());
                w48->set_image(&tmp, &k);
            }
            w48->set_colour(&g_strtable.lookup(tstring("party_b_upgrade_t_c_passive"))->len);
        }
        w38->set_text(upgrade_buf);
    }
    {
        tstring u(upgrade_buf);
        w38->set_image(&tmp, &u);
    }
    if (f04 == 3) {
        if (f8dc == 0) {
            w50->enable();
            f3908 = 1;
        } else {
            w50->set_text(0);
        }
    } else {
        w50->set_text(0);
    }
}
