// Slice s00532de0 (batch ds1): the party-window update pass, the grid selection setter and the
// huge key/menu event handler (0x533070).  All three are members of the embedded game object
// (the PartyMgr lives at +0xf8, the trooper table at +0x13a00).  Names are provisional.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <stdlib.h>
#include <math.h>
#include "rgl.h"

// ---- UI widget (vtable slots by byte offset: 0x2c select, 0x44 hit test, 0x48 refresh) ---------------
struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10();
    virtual void select(int flag);                       // +0x2c
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16();
    virtual bool hit(int mask);                          // +0x44
    virtual void refresh(int mask);                      // +0x48
};

struct TString {
    void operator=(const TString& s);                    // 0x42bc50
};

struct PartyMgr;
struct Party {                                           // 0x5ac8-byte party record
    char data[0x5ac8];

    int  total_value(unsigned flags);                    // 0x4b1cf0 (ret 4)
    void row_render(int a, int b, int c, int d, int e);  // 0x48c840 (ret 0x14)
};

struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                                       // +0x138d0
    int index_of(int id);                                // 0x48ad00 (ret 4)
    Party* get(int id);                                  // 0x51ee50 (ret 4)
};

struct CharRec {                                         // 0x508-byte troop record
    char pad0[0x20];
    char name[0x10];                                     // +0x20 (tstring payload)
    unsigned flags30;                                    // +0x30
    char pad34[0x508 - 0x34];
};

struct MissionSource {
    void pick(int* out);                                 // 0x4d5410 (ret 4)
};

struct MenuItem {
    char pad0[4];
    int f4;                                              // +0x04
    char pad8[0x10 - 8];
    int f10;                                             // +0x10
    int f14;                                             // +0x14
    int f18;                                             // +0x18
};

struct MainMenu {
    void show(int code);                                 // 0x495700 (ret 4)
};

struct KeyTable {
    bool down(int key);                                  // 0x48ac90 (ret 4)
};

struct FlagSet {
    void commit(int* slot);                              // 0x513710 (ret 4)
};

struct AuxObj {
    void reset();                                        // 0x513b40
};

struct StrTable {
    const void* find(const void* key);                   // 0x4c6010 (ret 4)
};

// The full layout is kept offset based below; the actual game object is opaque.
struct GameObj;
inline PartyMgr* mgr_of(GameObj* g);
inline Party* party_index(GameObj* g, int id);

struct GameObj {
    char data[0x5ac8];

    void clear_selection();                              // 0x5324d0 (ret 0)
    void update_party_sprites();                         // 0x532de0
    void set_selection(int a, int b, int c);             // 0x533020 (ret 0xc)
    void on_event(int code);                             // 0x533070 (ret 4)
    void refresh_party_window();                         // 0x5063e0 (ret 0)
    int  add_sprite(int a, int b, int kind);             // 0x49ebd0 (ret 0xc)
};

extern GameObj* g_game;                                  // 0x5e2930
extern int g_world_time;                                 // 0x5d8790
extern float g_ui_alpha;                                 // 0x5d7098
extern float g_scroll;                                   // 0x590d7c
extern int g_a181b4;                                     // 0xa181b4
extern int g_a7333c;                                     // 0xa7333c
extern AuxObj g_a73300;                                  // 0xa73300
extern FlagSet g_flags5e291c;                            // 0x5e291c
extern KeyTable g_a73730;                                // 0xa73730
extern MainMenu g_a73980;                                // 0xa73980
extern MenuItem* g_a739a0;                               // 0xa739a0
extern MenuItem* g_a739b0;                               // 0xa739b0
extern MenuItem* g_a739b4;                               // 0xa739b4
extern MenuItem* g_a739bc;                               // 0xa739bc
extern MenuItem* g_a739e4;                               // 0xa739e4
extern MenuItem* g_a739e8;                               // 0xa739e8
extern MenuItem* g_a739ec;                               // 0xa739ec
extern char* g_a5e29b0;                                  // 0x5e29b0
#define STR_TABLE ((StrTable*)0x5e2a18)

struct UiDsv {
    void set_value(float v, void* src);                  // 0x48baa0 (ret 8)
};

struct Emit {
    void emit(int arg);                                  // 0x4875e0 (ret 4)
};

extern void __fastcall flags_reset(void* self);          // 0x513b40

inline PartyMgr* mgr_of(GameObj* g) { return (PartyMgr*)((char*)g + 0xf8); }

inline Party* party_at(PartyMgr* m, int id)
{
    return &m->pages[id >> 4][id & 0xf];
}

inline Party* party_index(GameObj* g, int id)
{
    PartyMgr* m = mgr_of(g);
    return party_at(m, m->index_of(id));
}

// @ 0x532de0
void GameObj::update_party_sprites()
{
    PartyMgr* mgr = mgr_of(this);
    Party* p = party_index(this, *(int*)((char*)this + 0x7c));
    if (p->total_value(0x10) != 0) {
        Party* q = party_index(this, *(int*)((char*)this + 0x78));
        int spr = add_sprite(*(int*)((char*)q + 0x30), *(int*)((char*)q + 0x34), 1);
        Party* r = party_index(this, spr);
        int n = *(int*)((char*)p + 0x8a8);
        int i = 0;
        if (n > 0) {
            int* row = (int*)((char*)p + 0xb0);
            do {
                if ((row[0] & 1) == 0)
                    r->row_render(row[-1], row[-2], row[0], 0, 0);
                i++;
                row += 8;
            } while (i < *(int*)((char*)p + 0x8a8));
        }
    }
    int n = *(int*)((char*)p + 0x8a8);
    int i = 0;
    if (n > 0) {
        int* row = (int*)((char*)p + 0xac);
        do {
            CharRec* rec = &(*(CharRec**)((char*)this + 0x13a00))[row[0]];
            if ((rec->flags30 & 0x10) != 0) {
                int kind = (row[1] & 1) ? 2 : 1;
                Party* q = party_index(this, *(int*)((char*)this + 0x78));
                int spr = add_sprite(*(int*)((char*)q + 0x30), *(int*)((char*)q + 0x34), kind);
                Party* d = party_index(this, spr);
                CharRec* src = &(*(CharRec**)((char*)this + 0x13a00))[row[0]];
                const void* s = STR_TABLE->find((char*)src + 0x20);
                if (s == 0)
                    s = src;
                ((TString*)((char*)d + 0xc))->operator=(*(const TString*)s);
                *(char*)((char*)d + 0x1c) = 1;
                d->row_render(row[0], 1, 0, 0, 0);
            }
            i++;
            row += 8;
        } while (i < *(int*)((char*)p + 0x8a8));
    }
    Party* e = mgr->get(*(int*)((char*)this + 0x7c));
    *(int*)((char*)e + 0x8a8) = 0;
}

// @ 0x533020
void GameObj::set_selection(int a, int b, int c)
{
    clear_selection();
    *(int*)((char*)this + 0x8dc) = a;
    *(int*)((char*)this + 0x8e0) = b;
    *(int*)((char*)this + 0x8e4) = c;
    if (c != -1) {
        Widget* w = *(Widget**)((char*)this + 0xb8 + ((b + a * 2) * 0x40 + c) * 4);
        w->select(1);
    }
    refresh_party_window();
}

// @ 0x533070
void GameObj::on_event(int code)
{
    int slot = (int)this;
    ((MissionSource*)((char*)this + 0x30))->pick(&slot);

    char* rec = g_a5e29b0 + *(int*)((char*)g_game + 0x13aec) * 0x60;
    int local = -1;

    if (g_world_time != 0) {
        Widget* w;
        if (g_ui_alpha < 0.3f)
            w = *(Widget**)((char*)this + 0xec);
        else
            w = *(Widget**)((char*)this + 0xe8);
        Widget* vs = (Widget*)*(void**)((char*)w + 0x484);
        float* base = (float*)((char*)vs + 0xf0 + *(int*)((char*)vs + 0x178) * 4);
        float v = ((float)g_world_time / base[0]) * *(float*)((char*)vs + 0x17c) * g_scroll
                + (base[0] - *(float*)((char*)vs + 0x17c))
                * ((*(float*)((char*)vs + 0x168) - *(float*)((char*)vs + 0x16c))
                 / (*(float*)((char*)vs + 0x170) - *(float*)((char*)vs + 0x16c)));
        ((UiDsv*)vs)->set_value(v, vs);
        Widget* vs2 = (Widget*)*(void**)((char*)w + 0x484);
        vs2->refresh(0x10000);
    }

    unsigned k = *(unsigned*)(rec + 0x20);
    bool ctrl = (k & 0x200) == 0 && (k & 0x100) != 0;

    if (ctrl && g_a73730.down(0x10)) {
        g_a739a0->f18 = 0x17;
        g_a739a0->f4 = 0;
        g_a73980.show(8);
    }
    else if (ctrl && g_a73730.down(0x11)) {
        g_a739b0->f18 = 0x17;
        g_a739b0->f4 = 0;
        g_a73980.show(0xc);
    }
    else if (ctrl && g_a73730.down(0xf)) {
        g_a739bc->f18 = 0x17;
        g_a739bc->f4 = 0;
        g_a739bc->f14 = *(int*)((char*)g_game + 0x8c);
        g_a73980.show(0xf);
    }
    else if (ctrl && g_a73730.down(0x12)) {
        if (g_a181b4 == 0) {
            g_a739b4->f18 = 0x17;
            g_a739b4->f4 = 0;
            g_a73980.show(0xd);
        } else {
            g_a739e8->f18 = 0x17;
            *(int*)((char*)g_game + 0x13b08) = 4;
            *(int*)((char*)g_game + 0x13b0c) = 0;
            g_a73980.show(0x1a);
        }
    }
    else if ((k & 0x200) == 0 && g_a73730.down(0x13)) {
        g_a739e4->f18 = 0x17;
        g_a739e4->f4 = 0;
        g_a73980.show(0x19);
    }
    else {
        bool go = false;
        if ((k & 0x10) == 0) {
            if (*(int*)((char*)g_game + 0xc8) == 0 && *(int*)((char*)this + 0xa0) > 0) {
                int i = 0;
                Widget** ws = (Widget**)((char*)this + 0x60);
                do {
                    if (ws[i]->hit(0x10000))
                        local = i;
                    i++;
                } while (i < *(int*)((char*)this + 0xa0));
                if (local >= 0)
                    go = true;
            }
        } else {
            local = 0;
            if (*(int*)((char*)this + 0xa0) > 0)
                go = true;
        }
        if (go) {
            int sel = ((int*)((char*)this + 0xa4))[local];
            *(int*)((char*)g_game + 0x13af8) = 0;
            ((Emit*)(*(char**)(rec + 0x58) + sel * 0x68 + 0x4c))->emit(8);
        }
    }

    int st = *(int*)((char*)g_game + 0xc8);
    if (st == 0) {
        if (*(int*)((char*)this + 0x18) != 100)
            return;
        g_a7333c |= 0x400;
        g_flags5e291c.commit(&slot);
        return;
    }
    if (st == 1) {
        *(int*)((char*)g_game + 0xc8) = 0;
        if (*(int*)((char*)this + 0x18) == 100) {
            *(unsigned*)((char*)g_game + 0xa8) |= 1;
        } else {
            g_a73300.reset();
            g_a73980.show(*(int*)((char*)this + 0x18));
            return;
        }
        g_flags5e291c.commit(&slot);
        return;
    }
    if (st == 0x11) {
        *(int*)((char*)g_game + 0xc8) = 0;
        g_a73300.reset();
        g_a73980.show(7);
        return;
    }
    if (st == 0xd) {
        *(int*)((char*)g_game + 0xc8) = 0;
        g_a7333c |= 8;
        g_flags5e291c.commit(&slot);
        return;
    }
    if (st == 2) {
        *(int*)((char*)g_game + 0xc8) = 0;
        g_a7333c |= 0x80;
        g_flags5e291c.commit(&slot);
        return;
    }
    if (st == 0xe) {
        *(int*)((char*)g_game + 0xc8) = 0;
        g_a7333c |= 0x800;
        g_flags5e291c.commit(&slot);
        return;
    }
    if (st == 0xf) {
        *(int*)((char*)g_game + 0xc8) = 0;
        if (*(int*)((char*)this + 0x18) != 100) {
            g_a739ec->f18 = 0x17;
            g_a73980.show(0x1b);
            return;
        }
        g_a7333c |= 0x1000;
        g_flags5e291c.commit(&slot);
        return;
    }
    if (st == 0x10) {
        *(int*)((char*)g_game + 0xc8) = 0;
        if (*(int*)((char*)this + 0x18) != 100) {
            g_a739e8->f18 = 0x17;
            g_a73980.show(0x1a);
            return;
        }
        g_a7333c |= 0x2000;
        g_flags5e291c.commit(&slot);
        return;
    }
    if (st == 0x12) {
        *(int*)((char*)g_game + 0xc8) = 0;
        *(int*)((char*)g_game + 0x13af8) = 0;
        if (*(int*)((char*)this + 0x18) != 100) {
            g_a73980.show(0x17);
            return;
        }
        g_a7333c |= 0x400;
        g_flags5e291c.commit(&slot);
        return;
    }
    if (st == 4) {
        g_a739a0->f18 = 0x17;
        g_a739a0->f14 = *(int*)((char*)g_game + 0x13af0);
        g_a739a0->f4 = 2;
        g_a73980.show(8);
        return;
    }
    if (st == 8) {
        *(int*)((char*)g_game + 0xc8) = 0;
        g_a7333c |= 0x100;
        g_flags5e291c.commit(&slot);
        return;
    }
    if (st == 5 || st == 7 || st == 6) {
        g_a739b0->f18 = 0x17;
        g_a739b0->f10 = *(int*)((char*)g_game + 0x13b00);
        int v;
        if (st == 7)
            v = 7;
        else if (st == 6)
            v = 6;
        else if (st == 9)
            v = 2;
        else if (st == 10)
            v = 3;
        else
            v = 1;
        g_a739b0->f4 = v;
        g_a73980.show(0xc);
        *(int*)((char*)g_game + 0xc8) = 0;
        return;
    }
}
