// Slice s00510fa0: the item / inventory tooltip panel.
//   0x510fa0  panel::draw_item_info(slot, pair, level, index)
//              Builds and draws every stat line of the selected item: name, weapon class, price,
//              weight, weapon damage / armour bonus, horse stats, requirements and 8 line slots.
// Class and member names are provisional.
#include <stdio.h>
#include <math.h>
#include <windows.h>

// ---- globals ------------------------------------------------------------------------------------------------
extern void* g_game;                    // 0x5e2930
extern int   g_sel_state;               // 0xa181a8
extern char  g_items[];                 // 0x5e29d0  (item records, 0x238 bytes each)
extern char  g_skills[];                // 0x8ee3100 (0x44 bytes each)
extern int   g_result;                  // 0xa70458
extern int   g_store1;                  // 0xa7045c
extern int   g_num_valid;               // 0xa70470

// ---- types --------------------------------------------------------------------------------------------------
struct tstring {                        // 16-byte string
    int ref;  char* buf;  int cap;  int len;
    tstring(const char* s);             // 0x42bbe0
    ~tstring();                         // 0x42bc40
    void assign(const char* s);         // 0x42bcb0
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
extern tstring g_num;                   // 0xa70464

struct Translator {
    const tstring* get(const tstring* key);     // 0x43a1e0
    const tstring* get_alt(const tstring* key); // 0x4c6010
};
extern Translator g_tr;                 // 0x5e2a18

struct Gui {
    void reset_all(int a, int b);                     // 0x42cdb0
    void set_pair(int p);                             // 0x4c5190
    void draw_string(const tstring* s, float y, int color, char b);   // 0x4e43e0
};
extern Gui g_gui;                       // 0x5d7080

struct ItemInstance {                   // the inventory slot record (arg 1)
    int item_no;                        // +0
    int amount;                         // +4
    void label(char* out);              // 0x512730
    int max_stack();                    // 0x434b70
    int requirement();                  // 0x434c50
    int value(int i);                   // 0x434cb0
    int speed_rating();                 // 0x434d70
    int hit_points();                   // 0x434df0
    int armor();                        // 0x434eb0
    int speed();                        // 0x434f60
    int maneuver();                     // 0x434fd0
    int charge();                       // 0x435040
    int head_armor();                   // 0x462960
    int leg_armor();                    // 0x462a10
};

struct Item {                           // 0x238 bytes
    char pad000[0x1c8];
    unsigned int flags;                 // +0x1c8  item type in the low byte, flags above
    char pad1cc[0x1fc - 0x1cc];
    int f1fc;                           // +0x1fc
    int f200;                           // +0x200
    int f204;                           // +0x204
    float weight;                       // +0x208
    int reach;                          // +0x20c
    char pad210[0x214 - 0x210];
    int f214;                           // +0x214  thrust damage type in bits 8..9
    int f218;                           // +0x218  swing damage type in bits 8..9
};

struct SkillRec {                       // 0x44 bytes
    char pad000[0x10];
    tstring f10;                        // +0x10
    tstring f20;                        // +0x20
};

struct Agent {
    int slot_value(int item_no);        // 0x462c00
};

struct Panel {                          // this (ecx)
    char can_buy(int level, int slot, int flag);    // 0x5126e0
    int  price(int level, int buy);                 // 0x512960
};

// ---- helpers ------------------------------------------------------------------------------------------------
void __stdcall set_agent_slot(void* agent, int slot);       // 0x437b20
void item_events(int item_no, int* a, int* b, int* c);      // 0x4ef730
void item_caption(int cls, char* out);                      // 0x512840
void weapon_class_caption(int cls, char* out);              // 0x4f04f0
void __stdcall line_slot(int op, int item_no, int n, int flag);  // 0x512620

// ------------------------------------------------------------------------------------------------------------
// @ 0x510fa0
// ------------------------------------------------------------------------------------------------------------
void __fastcall draw_item_info(Panel* self, ItemInstance* slot, int pair, int level, int index)
{
    int zero = 0;
    Item* it = (Item*)(g_items + slot->item_no * 0x238);
    g_gui.reset_all(0, 1);
    g_gui.set_pair(pair);

    char buf[512];
    slot->label(buf);
    {
        tstring s(buf);
        g_gui.draw_string(&s, 0.025f, 0xfff0e0d0, 1);
    }

    unsigned int flags = it->flags;
    unsigned int kind = flags & 0xff;
    if (kind == 2 || kind == 3 || kind == 4) {
        const char* key;
        if (kind == 4)
            key = (flags & 0x200000) ? "ui_polearm_no_shield" : "ui_polearm";
        else if (kind == 3)
            key = (flags & 0x200000) ? "ui_two_handed" : "ui_two_handed_one_handed";
        else
            key = "ui_one_handed";
        tstring k(key);
        sprintf(buf, "%s", g_tr.get(&k)->c_str());
        k.~tstring();
        {
            tstring s(buf);
            g_gui.draw_string(&s, 0.021f, 0xffc0b0e0, 1);
        }
    }

    char buyable = self->can_buy(level, index, 1);

    kind = it->flags & 0xff;
    if (kind == 10 || kind == 5 || kind == 6 || kind == 0x12) {
        int max = slot->max_stack();
        sprintf(buf, "(%d / %d)", max - (slot->amount & 0xff), slot->max_stack());
        tstring s(buf);
        g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
    } else if (kind == 7) {
        int hp = slot->hit_points();
        sprintf(buf, "(%d / %d)", slot->amount & 0xffff, hp);
        tstring s(buf);
        g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
    } else if (kind == 0xb && (it->flags & 0x2000000)) {
        int max = slot->max_stack();
        sprintf(buf, "(%d / %d)", max - (slot->amount & 0xff), slot->max_stack());
        tstring s(buf);
        g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
    }

    if (level == 1) {
        if (buyable) {
            sprintf(buf, "%s", g_tr.get(&tstring("ui_reclaim_price"))->c_str(),
                     *(int*)((char*)self + index * 4 + 0x7e4));
        } else {
            const tstring* v = g_tr.get(&tstring("ui_buying_price"));
            int p = self->price(level, 1);
            sprintf(buf, v->c_str(), p);
        }
    } else {
        if (buyable) {
            int p;
            if (level < 2)
                p = *(int*)((char*)self + (level * 0x60 + index) * 4 + 0x664);
            else
                p = *(int*)((char*)self + (index + level * 0xa) * 4 + 0x9c0);
            const tstring* v = g_tr.get(&tstring("ui_return_price"));
            sprintf(buf, v->c_str(), p);
        } else {
            int p = self->price(level, 0);
            const tstring* v = g_tr.get(&tstring("ui_sell_price"));
            sprintf(buf, v->c_str(), p);
        }
    }
    {
        tstring s(buf);
        g_gui.draw_string(&s, 0.024f, 0xfffffdf0, 1);
    }

    if (slot->item_no >= 0 && it->weight > 1e-5f) {
        float w = slot->item_no < 0 ? 0.0f : it->weight;
        const tstring* v = g_tr.get(&tstring("ui_weight"));
        sprintf(buf, v->c_str(), (double)w);
        tstring s(buf);
        g_gui.draw_string(&s, 0.011f, 0xffffffff, 1);
    }

    kind = it->flags & 0xff;
    if (kind == 12 || kind == 13 || kind == 14 || kind == 15) {
        if (slot->head_armor()) {
            const tstring* v = g_tr.get(&tstring("ui_plus_value_to_head_armor"));
            sprintf(buf, v->c_str(), slot->head_armor());
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        if (slot->armor()) {
            const tstring* v = g_tr.get(&tstring("ui_plus_value_to_body_armor"));
            sprintf(buf, v->c_str(), slot->armor());
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        if (slot->leg_armor()) {
            const tstring* v = g_tr.get(&tstring("ui_plus_value_to_leg_armor"));
            sprintf(buf, v->c_str(), slot->leg_armor());
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
    }

    kind = it->flags & 0xff;
    if (kind == 8 || kind == 9 || kind == 10 || kind == 16 || kind == 17 ||
        kind == 2 || kind == 3 || kind == 4) {
        // swing damage (type in flags2 at +0x218)
        int mask = 0;
        if (slot->value(0)) {
            unsigned int t = (it->f218 >> 8) & 3;
            const char* key;
            if (t == 0)      { key = "ui_cut_short";    mask = 1; }
            else if (t == 1) { key = "ui_pierce_short"; mask = 2; }
            else             { key = "ui_blunt_short";  mask = 4; }
            const tstring* v;
            {
                tstring k(key);
                v = g_tr.get(&k);
            }
            const tstring* dmg = g_tr.get(&tstring("ui_swing"));
            sprintf(buf, dmg->c_str(), slot->value(0), v->c_str());
            if (mask & 4) mask &= ~4;
            if (mask & 2) mask &= ~2;
            if (mask & 1) mask &= ~1;
            {
                tstring s(buf);
                g_gui.draw_string(&s, 0.024f, 0xffffb0a0, 1);
            }
        }
        // thrust damage (type in flags2 at +0x214)
        if (slot->value(1)) {
            unsigned int t = (it->f214 >> 8) & 3;
            const char* key;
            const char* fmtkey;
            if (kind == 8 || kind == 9 || kind == 10 || kind == 16 || kind == 17) {
                fmtkey = "ui_damage";
                if (t == 0)      { key = "ui_cut_short";    mask = 8; }
                else if (t == 1) { key = "ui_pierce_short"; mask = 0x10; }
                else             { key = "ui_blunt_short";  mask = 0x20; }
            } else {
                fmtkey = "ui_thrust";
                if (t == 0)      { key = "ui_cut_short";    mask = 0x40; }
                else if (t == 1) { key = "ui_pierce_short"; mask = 0x80; }
                else             { key = "ui_blunt_short";  mask = 0x100; }
            }
            const tstring* v;
            {
                tstring k(key);
                v = g_tr.get(&k);
            }
            const tstring* dmg = g_tr.get(&tstring(fmtkey));
            sprintf(buf, dmg->c_str(), slot->value(1), v->c_str());
            if (mask & 0x100) mask &= ~0x100;
            if (mask & 0x80)  mask &= ~0x80;
            if (mask & 0x40)  mask &= ~0x40;
            {
                tstring s(buf);
                g_gui.draw_string(&s, 0.024f, 0xffffb0a0, 1);
            }
        }
        if (kind == 8 || kind == 9 || kind == 16 || kind == 17) {
            int v = 100;
            if (slot->item_no >= 0) {
                v = it->f204;
                if (v == 0)
                    v = 99;
            }
            if (v > 100) v = 100;
            const tstring* p = g_tr.get(&tstring("ui_accuracy"));
            sprintf(buf, p->c_str(), v);
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffeebbfe, 1);
        }
        if (slot->speed_rating()) {
            const tstring* p = g_tr.get(&tstring("ui_speed_rating"));
            sprintf(buf, p->c_str(), slot->speed_rating());
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
    } else if (kind == 5 || kind == 6 || kind == 0x12) {
        if (slot->value(1)) {
            const tstring* p = g_tr.get(&tstring("ui_value_to_damage"));
            int v = slot->value(1);
            sprintf(buf, p->c_str(), slot->value(1), v <= 0 ? '+' : ' ');
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffffb0a0, 1);
        }
    } else if (kind == 7) {
        if (slot->armor()) {
            const tstring* p = g_tr.get(&tstring("ui_resistance"));
            sprintf(buf, p->c_str(), slot->armor());
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        {
            const tstring* p = g_tr.get(&tstring("ui_size"));
            int v = (int)((float)it->reach * 1.0f);
            sprintf(buf, p->c_str(), v);
        }
        {
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        if (slot->speed_rating()) {
            const tstring* p = g_tr.get(&tstring("ui_speed_rating"));
            sprintf(buf, p->c_str(), slot->speed_rating());
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
    } else if (kind == 0xb && g_sel_state == 0 && slot->item_no >= 0 && it->f1fc > 0) {
        const tstring* p = g_tr.get(&tstring("ui_value_to_morale"));
        float v = slot->item_no < 0 ? 0.0f : (float)it->f1fc;
        sprintf(buf, p->c_str(), (double)(v * 0.02f));
        tstring s(buf);
        g_gui.draw_string(&s, 0.024f, 0xffa0f0b0, 1);
    }

    kind = it->flags & 0xff;
    if (kind == 2 || kind == 3 || kind == 4) {
        float reach = (float)it->reach * 1.0f;
        int n = (int)floor(reach);
        if (reach - (float)n > 0.5f)
            n++;
        const tstring* p = g_tr.get(&tstring("ui_weapon_reach"));
        sprintf(buf, p->c_str(), n);
        tstring s(buf);
        g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
    }

    if (*(char*)&it->flags == 1) {
        set_agent_slot(*(void**)((char*)g_game + 0x8c), 0x18);
        {
            const tstring* p = g_tr.get(&tstring("ui_armor"));
            sprintf(buf, p->c_str(), slot->armor());
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffddddff, 1);
        }
        {
            const tstring* p = g_tr.get(&tstring("ui_speed"));
            sprintf(buf, p->c_str(), slot->speed());
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        {
            const tstring* p = g_tr.get(&tstring("ui_maneuver"));
            sprintf(buf, p->c_str(), slot->maneuver());
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        {
            const tstring* p = g_tr.get(&tstring("ui_charge"));
            sprintf(buf, p->c_str(), slot->charge());
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
        {
            const tstring* p = g_tr.get(&tstring("ui_hit_points"));
            sprintf(buf, p->c_str(), slot->hit_points(), slot->hit_points());
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, 0xffffffff, 1);
        }
    }

    if (slot->requirement()) {
        int color = 0xffee4444;
        void* game = g_game;
        int agent = *(int*)((char*)game + 0x8c);
        char* agents = *(char**)((char*)game + 0x13a00);
        int have = ((Agent*)(agents + agent * 0x508))->slot_value(slot->item_no);
        if (have >= slot->requirement())
            color = 0xff66ff44;

        char need[512];
        int a, b, c;
        item_events(slot->item_no, &a, &b, &c);
        if (a >= 0) {
            item_caption(a, need);
        } else {
            if (b < 0) {
                if (c >= 0)
                    weapon_class_caption(c, need);
            } else {
                SkillRec* sk = (SkillRec*)(g_skills + b * 0x44);
                const tstring* name = g_tr.get_alt(&sk->f20);
                if (!name)
                    name = &sk->f10;
                sprintf(need, "%s", name->c_str());
            }
        }
        {
            const tstring* p = g_tr.get(&tstring("ui_requires_value_difficulty"));
            sprintf(buf, p->c_str(), need, slot->requirement());
            tstring s(buf);
            g_gui.draw_string(&s, 0.024f, color, 1);
        }
    }

    if (it->flags & 0x4000000) {
        const tstring* p = g_tr.get(&tstring("ui_bonus_against_shields"));
        sprintf(buf, p->c_str());
        tstring s(buf);
        g_gui.draw_string(&s, 0.024f, 0xffbbff22, 1);
    }
    if (it->flags & 0x4000) {
        tstring k("ui_cant_be_used_to_block");
        g_gui.draw_string(g_tr.get(&k), 0.024f, 0xffccbb33, 1);
    }
    if (it->flags & 0x10100000) {
        tstring k("ui_cannot_be_used_on_horseback");
        g_gui.draw_string(g_tr.get(&k), 0.024f, 0xffccbb33, 1);
    }

    for (int i = 0; i < 8; i++) {
        g_result = 0;
        g_store1 = 0;
        g_num.assign("");
        line_slot(0x13, slot->item_no, i, *(signed char*)((char*)slot + 7));
        if (g_num_valid > 0) {
            unsigned int color = 0xffffffff;
            if (g_result != 0 || g_store1 != 0)
                color = g_result | 0xff000000;
            g_gui.draw_string(&g_num, 0.024f, color, 1);
        }
    }
    (void)zero;
}
