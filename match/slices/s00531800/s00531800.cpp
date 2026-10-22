// Slice s00531800: the mission party-window update (0x531800).  This is the per-frame
// driver for the party/troop exchange screen: it scrolls the two troop lists, dispatches
// clicks on the 2x2x64 slot grid, moves/merges troop stacks between the selected party
// and the current party, and finally handles the close/ok/upgrade buttons.
// Everything is a __thiscall Mission member (ret 4); the argument is unused.
// Names and layouts are provisional, reconstructed from the disassembly.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#include "rgl.h"

// ---- value source attached to the scroll widgets (vtable slot 0x48) -------------------
struct Source {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17();
    virtual void slot48(int);                                  // +0x48
    char pad[0xf0 - 4];
    float arr[0x1e];                                           // +0xf0
    float f168;                                                // +0x168
    float f16c;                                                // +0x16c
    float f170;                                                // +0x170
    int f174;                                                  // +0x174
    int f178;                                                  // +0x178
    float f17c;                                                // +0x17c
    void set_value(float v);                                   // 0x48baa0
    void on_value_changed(float v);                            // 0x4d5940
};

// ---- UI widget (vtable slot 0x44 = pressed) -------------------------------------------
struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16();
    virtual bool pressed(int);                                 // +0x44
    char pad[0x104 - 4];
    char f104;                                                 // +0x104
};

// ---- troop/agent record referenced by +0x90/+0x94 and the scroll table -----------------
struct Target {
    char pad[0xf4];
    float f_f4;                                                // +0xf4
    char pad2[0x484 - 0xf8];
    Source* src;                                               // +0x484
};

// ---- party stack (0x20 bytes, array at party+0xa8) ------------------------------------
struct Stack {
    int count;                                                 // +0x00
    int troop;                                                 // +0x04
    int flags;                                                 // +0x08
    int f0c;                                                   // +0x0c
    int f10;                                                   // +0x10
    int f14;                                                   // +0x14
    int f18;                                                   // +0x18
    int f1c;                                                   // +0x1c
};
struct Party {
    char pad0[0xa8];
    Stack stacks[64];                                          // +0xa8
    int nstacks;                                               // +0x8a8
    char pad8ac[0x8b4 - 0x8ac];
    char f8b4;                                                 // +0x8b4
    char pad8b5[0x5ac8 - 0x8b5];
    void fn_4b1d80();                                          // 0x4b1d80
    int ext_4b1700();                                          // 0x4b1700
    void remove_row(int* out, int row, int* tbl);              // 0x489540
    int add_row(int troop, int a, int b, int c, int d);        // 0x48c840
};
struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                                             // +0x138d0
    int index_of(unsigned id);                                 // 0x48ad00
    Party* get(int party, int slot, int kind);                 // 0x51ee50
};
struct Troop {
    char pad0[0x50];
    int f50;                                                   // +0x50
    int f54;                                                   // +0x54
    char pad58[0x508 - 0x58];
};
struct GameData {
    char pad0[0x78];
    int cur_party;                                             // +0x78
    char pad7c[0x8c - 0x7c];
    int cur_troop;                                             // +0x8c
    char pad90[0xf8 - 0x90];
    PartyMgr party_mgr;                                        // +0xf8
    char padp[0x13a00 - 0x139cc];
    Troop* troops;                                             // +0x13a00
    char pad13a04[0x13a20 - 0x13a04];
    int f13a20;                                                // +0x13a20
    char pad13a24[0x14058 - 0x13a24];
    int stack_ids[0x10000];                                    // +0x14058
    int fn_48a320(int troop);                                  // 0x48a320
    bool fn_435790(int a, int troop, int party);               // 0x435790
    void fn_532de0();                                          // 0x532de0
};

// ---- mission object (at 0x5e292c) -----------------------------------------------------
struct Mission {
    char pad0[4];
    int state;                                                 // +0x04
    char pad8[8];
    int sel_party;                                             // +0x10
    char pad14[4];
    int f18;                                                   // +0x18
    char pad1c[0x30 - 0x1c];
    Widget* w30;                                               // +0x30
    Widget* w34;                                               // +0x34
    char pad38[0x44 - 0x38];
    Widget* w44;                                               // +0x44
    Widget* w48;                                               // +0x48
    Widget* w4c;                                               // +0x4c
    Widget* w50;                                               // +0x50
    Widget* w54;                                               // +0x54
    Widget* w58;                                               // +0x58
    char pad5c[0x90 - 0x5c];
    Target* f90;                                               // +0x90
    Target* f94;                                               // +0x94
    char pad98[0xb8 - 0x98];
    Widget* grid[2][2][64];                                    // +0xb8
    char pad4b8[0x4c8 - 0x4b8];
    int slots[256];                                            // +0x4c8
    int f8c8;                                                  // +0x8c8
    char pad8cc[0x8dc - 0x8cc];
    int f8dc;                                                  // +0x8dc
    int f8e0;                                                  // +0x8e0
    int f8e4;                                                  // +0x8e4
    char pad8e8[0x3908 - 0x8e8];
    char f3908;                                                // +0x3908

    void fn_531800(int);                                       // 0x531800
    void refresh_party_window();                               // 0x5063e0
    void fn_503ed0(int);                                       // 0x503ed0
    void fn_503a50();                                          // 0x503a50
    void fn_5324d0();                                          // 0x5324d0
    void fn_532520(int p, int r, int c, int* a, int* b);       // 0x532520
    void fn_532560(int);                                       // 0x532560
    void fn_5325b0(int);                                       // 0x5325b0
    void fn_532740(int);                                       // 0x532740
    void fn_532830(int a, int b, int c, int* x, int* y);       // 0x532830
    void fn_533020(int a, int b, int c);                       // 0x533020
};

// ---- other globals --------------------------------------------------------------------
struct MainMenu {
    void show(int);                                            // 0x495700
};
struct Screen {
    void fn(int a, int b, int c, int d);                       // 0x514890
};
struct Flags {
    void commit(void* out);                                    // 0x513710
};

extern GameData* g_game;                                       // 0x5e2930
extern MainMenu g_main_menu;                                   // 0xa73980
extern Screen g_screen;                                        // 0xa73300
extern Flags g_flags;                                          // 0x5e291c
extern int* g_menu_data;                                       // 0xa739c0
extern float g_ui_alpha;                                       // 0x5d7094
extern int g_world_time;                                       // 0x5d8790
extern float g_alpha2;                                         // 0x5d7098
extern float g_033;                                            // 0x54b894
extern float g_scroll;                                         // 0x590d7c
extern float g_half;                                           // 0x549d70
extern int g_trial_mode;                                       // 0x5e29c4
extern int g_a17148;                                           // 0xa17148
extern int g_a17348;                                           // 0xa17348
extern int g_a17188;                                           // 0xa17188
extern int g_a17588;                                           // 0xa17588
extern int g_a5a9fbc;                                          // 0x5a9fbc
extern int g_a170d8;                                           // 0xa170d8
extern int g_a174d8;                                           // 0xa174d8
extern int g_a735c4;                                           // 0xa735c4
extern int g_a70458;                                           // 0xa70458
extern int g_a7045c;                                           // 0xa7045c

inline Party* get_party(int id)
{
    PartyMgr* m = (PartyMgr*)((char*)g_game + 0xf8);
    int i = m->index_of(id);
    return &m->pages[i >> 4][i & 0xf];
}

// @ 0x531800
void Mission::fn_531800(int)
{
    int extra = 0;
    int slot;
    int pid;
    int cmp_party;
    int count;
    int flag;
    int out;
    int i;
    int troop;
    int v;
    bool c48;
    bool c4c;
    bool c54;
    bool c58;
    int rows;
    int r;
    int c;
    int found;
    int limit;
    int off;
    int j;
    int nfree;
    int old10;
    int rnd;
    bool overflow;
    int tmp[2];
    Stack* sel;
    Troop* tsel;
    int a;
    int b;
    int action;
    char commit_out[4];

    if (f94 != 0 && f94->f_f4 < f90->f_f4) {
        if (g_ui_alpha < g_half) {
            f94->src->on_value_changed((float)g_world_time);
            f94->src->slot48(0x10000);
            goto after_scroll;
        }
    } else {
        if (g_ui_alpha < g_half && f8c8 == 2)
            extra = 1;
    }

    slot = 0x26 + extra * 2;
    if (g_alpha2 < g_033)
        slot += 1;
    if (g_world_time != 0) {
        Target** pt = &((Target**)this)[slot];
        Source* s = (*pt)->src;
        float* pe = &s->arr[s->f178];
        float ratio = (s->f168 - s->f16c) / (s->f170 - s->f16c);
        float fv = (*pe - s->f17c) * ratio;
        float t = (float)g_world_time / *pe;
        t = t * s->f17c;
        t = t * g_scroll;
        fv += t;
        s->set_value(fv);
        (*pt)->src->slot48(0x10000);
    }

after_scroll:
    if (f8e4 >= 0 && (g_a17148 || g_a17348) && g_a17188 &&
        !g_a17588 && g_a5a9fbc && g_trial_mode) {
        slot = slots[(f8e0 + f8dc * 2) * 0x40 + f8e4];
        pid = f8dc != 0 ? sel_party : g_game->cur_party;
        Party* p = g_game->party_mgr.get(pid, slot, 0xd2);
        p->fn_4b1d80();
        refresh_party_window();
    }

    rows = 2;
    if (state == 0 || state == 5)
        rows = 1;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < 2; c++) {
            for (i = 0; i < 64; i++) {
                if (grid[r][c][i]->pressed(0x10000))
                    fn_533020(r, c, i);
            }
        }
    }

    if (w50->pressed(0x10000)) {
        slot = slots[(f8e0 + f8dc * 2) * 0x40 + f8e4];
        pid = f8dc != 0 ? sel_party : g_game->cur_party;
        cmp_party = f8dc != 0 ? g_game->cur_party : sel_party;
        count = 1;
        if (g_a17148 || g_a17348)
            count = get_party(pid)->stacks[slot].count;
        flag = 0;
        for (i = 0; i < count; i++) {
            if (w50->f104 & 3)
                break;
            troop = get_party(pid)->stacks[slot].troop;
            v = 0;
            if (state == 3) {
                v = g_game->fn_48a320(troop);
            } else if (state == 2 && troop >= 0) {
                g_a70458 = 0;
                g_a7045c = 0;
                bool ok = g_game->fn_435790(10, troop, g_game->cur_party);
                v = g_a70458;
                if (!ok)
                    v = g_game->f13a20;
            }
            if (cmp_party == g_game->cur_troop)
                v = -v;
            if (state == 3 || state == 2)
                fn_532560(v);
            fn_532830(pid, slot, cmp_party, &out, &flag);
            refresh_party_window();
            if (flag)
                fn_5324d0();
            fn_503ed0(0);
            fn_503ed0(1);
        }
        refresh_party_window();
        if (flag)
            fn_5324d0();
        fn_503ed0(0);
        fn_503ed0(1);
        fn_503a50();
    }

    c48 = w48->pressed(0x10000);
    c4c = w4c->pressed(0x10000);
    if (c48 || c4c) {
        slot = slots[(f8e0 + f8dc * 2) * 0x40 + f8e4];
        pid = f8dc != 0 ? sel_party : g_game->cur_party;
        sel = &get_party(pid)->stacks[slot];
        tsel = &g_game->troops[sel->troop];
        found = 0;
        limit = 1;
        off = 0;
        j = 0;
        overflow = false;

        for (;;) {
            Party* p = get_party(pid);
            int n = p->nstacks;
            int k = 0;
            nfree = 0;
            if (n > 0) {
                char* pf = (char*)p + 0xb0;
                do {
                    if (*pf & 1)
                        break;
                    nfree++;
                    k++;
                    pf += 0x20;
                } while (k < n);
            }
            if (j >= nfree)
                break;

            if (c48) {
                if (*(int*)((char*)get_party(pid) + off + 0xac) == tsel->f50)
                    found = 1;
            }
            if (c4c) {
                if (*(int*)((char*)get_party(pid) + off + 0xac) == tsel->f54)
                    found = 1;
            }
            off += 0x20;
            j++;
        }

        if ((sel->flags & 1) || sel->f14 < sel->count) {
            if (!found)
                goto merge;
        } else {
            if (found)
                goto overflow_path;
            {
                Party* p = get_party(pid);
                int n = p->nstacks;
                int k = 0;
                if (n > 0) {
                    char* pf = (char*)p + 0xb0;
                    do {
                        if (*pf & 1)
                            break;
                        k++;
                        pf += 0x20;
                    } while (k < n);
                }
                if (k == get_party(pid)->ext_4b1700()) {
                    limit = sel->f14;
                    if (limit > sel->count)
                        limit = sel->count;
                }
            }
        }

merge:
        if ((sel->flags & 1) || limit < sel->count)
            goto overflow_path;
        if (c48)
            sel->troop = tsel->f50;
        else
            sel->troop = tsel->f54;
        sel->f14 = 0;
        sel->f18 = 0;
        goto tail;

overflow_path:
        if (limit >= sel->count)
            overflow = true;
        int base = tsel->f50;
        if (c4c)
            base = tsel->f54;
        for (int k = 0; k < limit; k++) {
            if (f3908)
                break;
            sel->f14--;
            Party* p = get_party(pid);
            old10 = p->stacks[slot].f10;
            rnd = rand();
            p = get_party(pid);
            if (p->stacks[slot].count < 0x20)
                rnd = g_game->stack_ids[get_party(g_game->cur_party)->stacks[slot].count +
                                        slot * 0x20];
            p = get_party(pid);
            int cc = p->stacks[slot].count - 1;
            p->stacks[slot].count = cc;
            if (cc < p->stacks[slot].f10)
                p->stacks[slot].f10 = cc;
            if (cc < p->stacks[slot].f14)
                p->stacks[slot].f14 = cc;
            if (p->stacks[slot].count < 1)
                p->remove_row(tmp, slot, &g_game->stack_ids[1]);
            p->f8b4 = 1;
            p = get_party(pid);
            int after = p->stacks[slot].f10;
            p = get_party(pid);
            int ni = p->add_row(base, 1, 0, 0, 1);
            p = get_party(pid);
            if (after < old10)
                p->stacks[ni].f10++;
            p = get_party(pid);
            if (p->stacks[ni].count < 0x20)
                g_game->stack_ids[ni * 0x20 + p->stacks[ni].count] = rnd;
            refresh_party_window();
        }
        if (overflow)
            fn_5324d0();

tail:
        refresh_party_window();
        fn_503ed0(0);
        fn_503a50();
    }

    c54 = w54->pressed(0x10000);
    c58 = w58->pressed(0x10000);
    if (c54 || c58) {
        int d = c54 ? -1 : 1;
        fn_532740(d);
        fn_533020(f8dc, f8e0, f8e4 + d);
        fn_503ed0(0);
        refresh_party_window();
    }

    if (!w30->pressed(0x10000) && (g_a170d8 != 0 || g_a174d8 == 0)) {
        if (w34->pressed(0x10000)) {
            fn_5325b0(0);
            fn_5325b0(1);
            fn_5324d0();
            fn_503ed0(0);
            fn_503ed0(1);
            refresh_party_window();
            fn_503a50();
            return;
        }
        if (!w44->pressed(0x10000))
            goto close_menu;
        if (f8e4 < 0)
            return;
        fn_532520(f8dc, f8e0, f8e4, &a, &b);
        action = 0xd;
        if (get_party(a)->stacks[b].flags & 1)
            action = 0xe;
        troop = get_party(a)->stacks[b].troop;
        g_screen.fn(troop, g_game->stack_ids[1 + b * 0x80],
                    g_game->cur_party, action);
        g_menu_data[6] = 0xc;
        g_main_menu.show(0x10);
        return;
    }

close_menu:
    if (state == 4 && g_a735c4 < 1)
        g_game->fn_532de0();
    if (f18 == 100) {
        g_flags.commit(commit_out + 3);
        return;
    }
    g_main_menu.show(f18);
}
