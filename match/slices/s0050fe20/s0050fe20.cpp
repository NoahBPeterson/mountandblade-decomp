// Slice s0050fe20: the inventory panel's per-frame step (0x50fe20) -- the scroll-bar follow, the trial-mode
// money top-up, the two character-strip clicks, the pending "trade" value bookkeeping (pay / receive /
// dump questions, the refund, the auto-equip retry) and the closing menu choice.
//
// `this` is the inventory panel of s00500ad0 (mode +0x4, troop +0x14, next menu +0x18, the two views at
// +0x34, view_count +0x964, the container +0x96c, total-cost label +0xa60, the character icons +0xa84/+0xa88,
// the pending value +0xa8c, its "locked" flag +0xa98, the three dialog results +0xab0/+0xab4/+0xab8 and the
// two strip specs +0xac0/+0xac4).  Widget slots: +0x44 clicked_on(int), +0x48 set_key_flags(int),
// +0xa4 set_text(tstring* out, tstring* text) (see s004eeb10).
#include <windows.h>
#include <new>
#include "rgl.h"

// ---- the engine string (0x10 bytes; ctor 0x42bbe0, dtor 0x42bc40) ----
struct tstring {
    int pos;                                            // +0x00
    char* buf;                                          // +0x04
    int cap;                                            // +0x08
    int len;                                            // +0x0c
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0 (ret 4)
    tstring(const tstring& o);                          // 0x42c110
    ~tstring();                                         // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// a tstring a callee constructs in place (World::string_value writes into its first argument)
struct tstring_buffer {
    char bytes[sizeof(tstring)];
    tstring* get() { return (tstring*)bytes; }
    void destroy() { get()->~tstring(); }
};

// ---- widgets ----
struct UiWidget {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16();
    virtual char clicked_on(int which);                 // +0x44
    virtual void set_key_flags(int flags);              // +0x48
    virtual void d19(); virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23();
    virtual void d24(); virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28();
    virtual void d29(); virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33();
    virtual void d34(); virtual void d35(); virtual void d36(); virtual void d37(); virtual void d38();
    virtual void d39(); virtual void d40();
    virtual void set_text(tstring* out, tstring* text); // +0xa4
};

// the scroll bar (0x48baa0 set_value; layout from s004eeb10)
struct UiDsv : UiWidget {
    char  pad0[0xf0 - 4];
    float stops[0x1e];                                  // +0xf0
    float f_168;                                        // +0x168 (thumb)
    float f_16c;                                        // +0x16c (track start)
    float f_170;                                        // +0x170 (track end)
    float f_174;                                        // +0x174
    int   f178;                                         // +0x178 (current stop)
    float f_17c;                                        // +0x17c
    void  set_value(float v);                           // 0x48baa0 (ret 4)
};

// the inventory view owns the scroll bar at +0x484 (s00500ad0)
struct InvView {
    char   pad0[0x484];
    UiDsv* bar;                                         // +0x484
};

// ---- game data ----
struct ItemStack {
    int id;                                             // +0x00
    int count;                                          // +0x04
    void reduce(int n);                                 // 0x437510 (ret 4)
};

struct TroopRec {
    char      pad0[0x20];
    tstring   name;                                     // +0x20 (displayed name, tstring)
    unsigned  flags;                                    // +0x30
    int       f34;                                      // +0x34
    char      pad38[0x3b4 - 0x38];
    int       money;                                    // +0x3b4
    char      pad3b8[0x4ec - 0x3b8];
    ItemStack food;                                     // +0x4ec
    char      pad4f4[0x508 - 0x4f4];
    int       slots[0x60];                              // +0xb4 (stride 8)
    TroopRec* auto_equip(int a, int b, int c, int d);   // 0x4687e0 (ret 0x10)
};

struct Item {
    char          pad0[0x20];
    tstring       name;                                 // +0x20
    char          pad30[0x1c8 - 0x30];
    unsigned char type;                                 // +0x1c8
    char          pad1c9[0x238 - 0x1c9];
};
extern Item* g_items;                                   // 0x5e29d0

struct World {
    char       pad8c[0x8c];
    int        cur_troop;                               // +0x8c
    int        hero_troop;                              // +0x90
    char       pad94[0x13a00 - 0x94];
    TroopRec*  troops;                                  // +0x13a00
    tstring*   string_value(tstring* out, int value);   // 0x48e9e0 (ret 8)
    void       add_value(int amount, int group);        // 0x49b720
    void       sub_value(int amount, int group);        // 0x49b820
};
extern World* g_game;                                   // 0x5e2930

#define TROOP(i) (g_game->troops + (i))

// ---- services ----
struct StrTable {
    tstring* lookup(tstring* key);                      // 0x43a1e0 (ret 4)
    tstring* find(tstring* key);                        // 0x4c6010 (ret 4, 0 when absent)
};
extern StrTable g_strtable;                             // 0x5e2a18

struct MsgLog { void flush(); };                        // 0x4f1390
#define MSGLOG ((MsgLog*)0x5d7080)                      // 0x5d7080

struct MainMenu {
    int  f0;                                            // +0x00
    int  f4;                                            // +0x04
    void ask(tstring* question, tstring* yes, tstring* no, int* result);   // 0x48a3d0 (ret 0x10)
    void choose(int option);                            // 0x495700 (ret 4)
};
extern MainMenu g_main_menu;                            // 0xa73980

struct SoundMgr {
    int play(int idx, int flags, const float* pos, float vol, int* out);   // 0x442650 (ret 0x14)
};
#define SOUNDMGR ((SoundMgr*)0x5aa128)                  // 0x5aa128
extern int   g_snd_id_a;                                // 0x5aa350
extern int   g_snd_id_b;                                // 0x5aa2fc
extern float g_sound_origin[];                          // 0x5aa168

extern int   g_a17148;                                  // 0xa17148
extern int   g_a17348;                                  // 0xa17348
extern int   g_a17188;                                  // 0xa17188
extern int   g_a17588;                                  // 0xa17588
extern int   g_a5a9fbc;                                 // 0x5a9fbc
extern int   g_trial_mode;                              // 0x5e29c4
extern int   g_a170d8;                                  // 0xa170d8
extern int   g_a174d8;                                  // 0xa174d8
extern float g_ui_alpha;                                // 0x5d7094
extern int   g_time;                                    // 0x5d8790 (integer tick counter)

void  mb_snprintf(char* buf, unsigned n, const char* fmt, ...);   // 0x415100
extern "C" int __cdecl sprintf(char*, const char*, ...);           // 0x536f8f

// the object at 0x5e291c takes the final flag
// the object at 0x5e291c takes the final flag
struct Flags5e291c {
    void commit(char* out);                             // 0x513710 (ret 4)
};
#define OBJ5E291C ((Flags5e291c*)0x5e291c)              // 0x5e291c

// ---- the panel ----
struct InventoryPanel {
    int  vptr;                                          // +0x00
    int  mode;                                          // +0x04
    char pad08[0x14 - 0x08];
    int  troop;                                         // +0x14
    int  f18;                                           // +0x18 (next menu option, 0x64 = none)
    char pad1c[0x34 - 0x1c];
    InvView* view[2];                                   // +0x34
    char pad3c[0x964 - 0x3c];
    int  view_count;                                    // +0x964
    int  f968;                                          // +0x968
    void* window;                                       // +0x96c
    char pad970[0xa60 - 0x970];
    UiWidget* total_cost;                               // +0xa60
    char pad_a64[0xa84 - 0xa64];
    UiWidget* f_a84;                                    // +0xa84 (page 0 character icon)
    UiWidget* f_a88;                                    // +0xa88 (page 1 character icon)
    int  f_a8c;                                         // +0xa8c (pending trade value)
    UiWidget* leave;                                    // +0xa90
    int  f_a94;                                         // +0xa94
    int  f_a98;                                         // +0xa98 (trade locked)
    int  f_a9c;                                         // +0xa9c
    int  f_aa0;                                         // +0xaa0
    int  f_aa4;                                         // +0xaa4
    int  f_aa8;                                         // +0xaa8
    int  f_aac;                                         // +0xaac
    int  f_ab0;                                         // +0xab0 (pay question result)
    int  f_ab4;                                         // +0xab4 (dump question result)
    int  f_ab8;                                         // +0xab8 (food question result)
    char pabc;                                          // +0xabc
    char padabd[0xac0 - 0xabd];
    int  f_ac0;                                         // +0xac0 (page 0 strip spec)
    int  f_ac4;                                         // +0xac4 (page 1 strip spec)

    void money_label(void* arg);                                    // 0x501940 (ret 4)
    void init_window();                                             // 0x5025a0
    void refresh_lists(int page);                                   // 0x5108d0 (ret 4)
    void slot_flags(int page, int idx, unsigned mask, char on);     // 0x510850 (ret 0x10)
    void* add_strip_icon(char* slot, const tstring& name, int troop, int kind);  // 0x5024e0 (ret 0x10)
    void refresh_transaction();                                     // 0x50fe20 (ret 4)
};

// @ 0x50fe20
void InventoryPanel::refresh_transaction()
{
    MSGLOG->flush();

    int page = 0;
    if (g_ui_alpha < 0.5f && view_count == 2)
        page = 1;

    if (g_time != 0) {
        UiDsv* bar = view[page]->bar;
        int stop = bar->f178;
        bar->set_value((g_time / bar->stops[stop]) * bar->f_17c * 0.0005f +
                       (bar->stops[stop] - bar->f_17c) *
                       ((bar->f_168 - bar->f_16c) / (bar->f_170 - bar->f_16c)));
        bar->set_key_flags(0x10000);
    }

    if ((g_a17148 != 0 || g_a17348 != 0) && g_a17188 != 0 && g_a17588 == 0 &&
        g_a5a9fbc != 0 && g_trial_mode != 0) {
        g_game->add_value(1000, g_game->cur_troop);
        money_label(0);
    }

    if (f_a84 != 0 && f_a84->clicked_on(0x10000)) {
        f_ac0 = (f_ac0 + 1) % 4;
        add_strip_icon((char*)this + 0xa84, tstring("inv_i_character"), g_game->cur_troop, f_ac0);
    }
    if (f_a88 != 0 && f_a88->clicked_on(0x10000)) {
        f_ac4 = (f_ac4 + 1) % 4;
        add_strip_icon((char*)this + 0xa88, tstring("inv_i_character2"), troop, f_ac4);
    }

    refresh_lists(0);
    if (mode == 4)
        refresh_lists(1);

    char eaten = 0;
    if (leave->clicked_on(0x10000) || (g_a170d8 == 0 && g_a174d8 != 0 && f_a8c == 0)) {
        if (f_a98 == 0) {
            int worn = 0;
            if (mode == 0 && troop == g_game->hero_troop) {
                int* it = &TROOP(troop)->slots[0];
                for (int i = 0; i < 9; i++) {
                    if (it[i * 2] >= 0)
                        worn++;
                }
            }

            if (f_a8c < 0) {
                if (-f_a8c <= TROOP(g_game->cur_troop)->money) {
                    g_game->sub_value(-f_a8c, g_game->cur_troop);
                    TROOP(troop)->money -= f_a8c;
                    eaten = 1;
                } else {
                    tstring key("ui_you_dont_have_value");
                    tstring_buffer vb;
                    tstring* val = g_game->string_value(vb.get(), -f_a8c);
                    tstring* f = g_strtable.lookup(&key);
                    char buf[0x200];
                    sprintf(buf, f->c_str(), val->c_str());
                    vb.destroy();
                    key.~tstring();
                    tstring msg(buf);
                    char out[0x10];
                    total_cost->set_text((tstring*)out, &msg);
                    msg.~tstring();
                }
            } else if (f_a8c > 0) {
                if (f_a8c <= TROOP(troop)->money) {
                    g_game->add_value(f_a8c, g_game->cur_troop);
                    TROOP(troop)->money -= f_a8c;
                    eaten = 1;
                } else {
                    f_ab0 = -1;
                    tstring key("ui_merchant_cant_afford_value");
                    tstring_buffer vb1;
                    tstring* val = g_game->string_value(vb1.get(), TROOP(this->troop)->money);
                    tstring_buffer vb2;
                    tstring* val2 = g_game->string_value(vb2.get(), f_a8c);
                    TroopRec* rec = TROOP(troop);
                    tstring* nm = g_strtable.find(&rec->name);
                    if (nm == 0)
                        nm = &rec->name;
                    tstring* f = g_strtable.lookup(&key);
                    char buf[0x200];
                    mb_snprintf(buf, 0x200, f->c_str(), nm->c_str(), val2->c_str(), val->c_str());
                    vb2.destroy();
                    vb1.destroy();
                    key.~tstring();
                    tstring think("ui_merchant_think_of_something_else");
                    tstring pay("ui_merchant_pay_whatever");
                    tstring msg(buf);
                    g_main_menu.ask(&msg, g_strtable.lookup(&pay), g_strtable.lookup(&think), &f_ab0);
                    msg.~tstring();
                    pay.~tstring();
                    think.~tstring();
                    g_main_menu.f4 = 2;
                    g_main_menu.choose(0);
                }
            } else if (worn > 0) {
                f_ab4 = -1;
                char buf[0x200];
                if (worn == 1) {
                    tstring key("ui_dumping_value_item");
                    tstring* f = g_strtable.lookup(&key);
                    mb_snprintf(buf, 0x200, f->c_str(), 1);
                    key.~tstring();
                } else {
                    tstring key("ui_dumping_value_items");
                    tstring* f = g_strtable.lookup(&key);
                    mb_snprintf(buf, 0x200, f->c_str(), worn);
                    key.~tstring();
                }
                tstring no("ui_no");
                tstring yes("ui_yes");
                tstring msg(buf);
                g_main_menu.ask(&msg, g_strtable.lookup(&yes), g_strtable.lookup(&no), &f_ab4);
                msg.~tstring();
                yes.~tstring();
                no.~tstring();
                g_main_menu.f4 = 2;
                g_main_menu.choose(0);
            } else {
                eaten = 1;
            }

            int food = g_game->troops->food.id;
            if (food >= 0 && g_items[food].type == 1) {
                f_ab8 = -1;
                tstring q("ui_question_slaughter_food_and_eat");
                Item* rec = &g_items[food];
                tstring* nm = g_strtable.find(&rec->name);
                if (nm == 0)
                    nm = &rec->name;
                tstring* f = g_strtable.lookup(&q);
                char buf[0x200];
                mb_snprintf(buf, 0x200, f->c_str(), nm->c_str());
                q.~tstring();
                tstring no("ui_no");
                tstring yes("ui_yes");
                tstring msg(buf);
                g_main_menu.ask(&msg, g_strtable.lookup(&yes), g_strtable.lookup(&no), &f_ab8);
                msg.~tstring();
                yes.~tstring();
                no.~tstring();
                g_main_menu.choose(0);
                eaten = 0;
            }
        }
    }

    if (f_ab8 == 0) {
        f_ab8 = -1;
        TroopRec* rec = TROOP(g_game->cur_troop);
        rec->food.id = 1;
        rec->food.count = 0;
        rec->food.reduce(0);
        slot_flags(2, 9, 2, 1);
        SOUNDMGR->play(g_snd_id_a, 1, g_sound_origin, 1.0f, 0);
        SOUNDMGR->play(g_snd_id_b, 1, g_sound_origin, 1.0f, 0);
        init_window();
    }

    if (!eaten) {
        if (f_ab0 != 0) {
            if (f_ab4 != 0)
                return;
        }
    } else if (f_ab0 != 0) {
        goto equip;
    }
    g_game->add_value(TROOP(troop)->money, g_game->cur_troop);
    TROOP(troop)->money = 0;

equip:
    if (troop >= 3) {
        TroopRec* rec = TROOP(troop);
        unsigned f = rec->flags;
        int spare = rec->f34;
        spare = spare;
        if ((f & 0x10) == 0 || (f & 0x1000) == 0) {
            if (mode == 4)
                rec->auto_equip(-1, 0x100e, 0, 0);
        }
    }

    if (f18 != 0x64)
        g_main_menu.choose(f18);
    else
        OBJ5E291C->commit(&eaten);
}
