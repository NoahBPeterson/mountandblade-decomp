// Slice s004dada0: the mission / meeting screen windows for the campaign diary.
//  0x4dada0  make_hint            allocates a 0x178-byte widget, positions/scales/sizes it, sets its text and
//                                 adds it to an overlay window.
//  0x4db020  fill_single_mission sets the overlay and the f38 widget to one mission's text.
//  0x4db090  fill_mission_grid   walks the 3-D unit grid (rows x depth x columns) and adds one row per unit.
//  0x4dae40  init_mission_window resets the GUI, builds the 0x190-byte "mission_window" overlay, its mesh,
//                                 colour/scale pass and a 0x178-byte child, then fills the text.
//  0x4db1f0  init_meeting_window the same construction for "meeting_window", without the text fill.
//  0x4db440  fill_mission_row    walks one side's party and writes one row per unit into rows[side].
// Reconstruction status: complete in structure and in every field offset read off the disassembly, but not
// byte-exact.  0x4dae40 / 0x4db1f0 / 0x4db440 touch tstring temporaries whose stack slots this build does not
// reuse the way the original does, 0x4db090's inner loop keeps its row text in the manager table which this
// build reads through a helper that returns a pointer instead of an index, and 0x4dada0's six stack floats are
// passed as three pairs by this build.  See nonmatching.txt.
// flags /O2 /MT /Gy /TP /GX
#include <new>
#include <string.h>
#include <stdio.h>
#include "rgl.h"

// ---- engine string (16 bytes, see s0042a1f0/tstring.h) --------------------------------------------------
struct tstring {
    tstring& operator=(const tstring& o);       // 0x42bc50
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
};

namespace rgl { struct vector2 { float x, y; }; }

void __fastcall ctor_4931a0(void* p);          // 0x4931a0 (ecx = object)

// ---- widgets ------------------------------------------------------------------------------------------
// the 0x178-byte widget (vptr 0x591d58, base ctor 0x48ae20; see s0049aca0).  The original has 0x32 virtual
// slots before add_child at +0xc8; the names of the unused ones do not matter, only their count.
struct Widget {
    Widget();                                   // 0x48ae20
    virtual void w00(int); virtual void w01(int); virtual void w02(int); virtual void w03(int);
    virtual void w04(int); virtual void w05(int); virtual void w06(int); virtual void w07(int);
    virtual void w08(int); virtual void w09(int); virtual void w10(int); virtual void w11(int);
    virtual void w12(int); virtual void w13(int); virtual void w14(int); virtual void w15(int);
    virtual void w16(int); virtual void w17(int); virtual void w18(int); virtual void w19(int);
    virtual void w20(int); virtual void w21(int);                           // 0x00..0x54
    virtual void set_pos(const float* p);       // +0x58 (slot 22)
    virtual void w23(int); virtual void w24(int);                           // +0x5c, +0x60
    virtual void set_scale(const float* p);     // +0x64 (slot 25)
    virtual void w26(int);                                                  // +0x68
    virtual void set_size(const float* p);      // +0x6c (slot 27)
    virtual void w28(int);                                                  // +0x70
    virtual void w29(int);                                                  // +0x74
    virtual void w30(int);                                                  // +0x78
    virtual void set_mesh(int kind, void* m);   // +0x7c (slot 31)
    virtual void w32(int);                                                  // +0x80
    virtual void w33(int); virtual void w34(int); virtual void w35(int);    // +0x84..0x8c
    virtual void w36(int); virtual void w37(int);                           // +0x90, +0x94
    virtual void set_color(const int* c);       // +0x98 (slot 38)
    virtual void set_color2(const int* c);      // +0x9c (slot 39)
    virtual void w40(int);                                                  // +0xa0
    virtual void set_text(const void* a, const void* b);    // +0xa4 (slot 41)
    virtual void w42(int);                                                  // +0xa8
    virtual void set_text2(const void* a, const void* b);  // +0xac (slot 43)
    virtual void w44(int); virtual void w45(int); virtual void w46(int);    // +0xb0..0xb8
    virtual void w47(int); virtual void w48(int); virtual void w49(int);    // +0xbc..0xc4
    virtual void add_child(void* c);            // +0xc8 (slot 50)
    virtual void w51(int); virtual void w52(int); virtual void w53(int);    // +0xcc..0xd4
    virtual void w54(int); virtual void w55(int); virtual void w56(int);    // +0xd8..0xe0
    virtual void w57(int); virtual void w58(int); virtual void w59(int);    // +0xe4..0xec
    virtual void w60(int); virtual void w61(int);                           // +0xf0, +0xf4
    virtual void w62(int);                                                  // +0xf8 (slot 62)

    char pad0[0x121 - 4];
    char f121;                                  // +0x121
    char pad122[0x178 - 0x122];
};

// the 0x188-byte row widget of the mission grid (ctor 0x48b1c0)
struct Entry : Widget {
    Entry();                                    // 0x48b1c0
    char pad178[0x188 - 0x178];
};

// the 0x188-byte per-side column window (ctor 0x4931a0, vptr 0x597050)
struct Column : Widget {
    char pad178[0x188 - 0x178];
};

// the overlay window (vptr 0x592848, base ctor 0x4931a0, 0x190 bytes; see s0049aca0)
struct Overlay {
    Overlay();                                  // 0x4931a0 (base ctor)
    virtual void n00(int); virtual void n01(int); virtual void n02(int); virtual void n03(int);
    virtual void n04(int); virtual void n05(int); virtual void n06(int); virtual void n07(int);
    virtual void n08(int); virtual void n09(int); virtual void n10(int); virtual void n11(int);
    virtual void n12(int); virtual void n13(int); virtual void n14(int); virtual void n15(int);
    virtual void n16(int); virtual void n17(int); virtual void n18(int); virtual void n19(int);
    virtual void n20(int); virtual void n21(int);                           // 0x00..0x54
    virtual void set_pos(const float* p);       // +0x58 (slot 22)
    virtual void n23(int); virtual void n24(int);                           // +0x5c, +0x60
    virtual void set_scale(const float* p);     // +0x64 (slot 25)
    virtual void n26(int);                                                  // +0x68
    virtual void set_size(const float* p);      // +0x6c (slot 27)
    virtual void n28(int);                                                  // +0x70
    virtual void set_flag(const int* f);        // +0x74 (slot 29)
    virtual void n30(int);                                                  // +0x78
    virtual void set_mesh(int kind, void* m);   // +0x7c (slot 31)
    virtual void n32(int);                                                  // +0x80
    virtual void n33(int); virtual void n34(int); virtual void n35(int);    // +0x84..0x8c
    virtual void n36(int); virtual void n37(int);                           // +0x90, +0x94
    virtual void set_color(const int* c);       // +0x98 (slot 38)
    virtual void n39(int);                                                  // +0x9c
    virtual void n40(int);                                                  // +0xa0
    virtual void set_text(const void* a, const void* b);    // +0xa4 (slot 41)
    virtual void n42(int);                                                  // +0xa8
    virtual void set_text2(const void* a, const void* b);   // +0xac (slot 43)
    virtual void n44(int); virtual void n45(int); virtual void n46(int);    // +0xb0..0xb8
    virtual void n47(int); virtual void n48(int); virtual void n49(int);    // +0xbc..0xc4
    virtual void add_child(void* c);            // +0xc8 (slot 50)
    char pad0[0x190 - 4];                       // 0x190 bytes
};

// ---- the screen manager (`this` of every function in this slice) ---------------------------------------
struct Screen {
    virtual void s00(int); virtual void s01(int); virtual void s02(int); virtual void s03(int);
    virtual void s04(int); virtual void s05(int); virtual void s06(int); virtual void s07(int);
    virtual void s08(int); virtual void s09(int);
    int f4;                                     // +0x4
    char pad8[0x2c - 8];
    Overlay* overlay;                           // +0x2c
    Widget* row[16];                            // +0x30 (unit widgets of the grid; row[2] = mission text)
    Column* col[3];                             // +0x70 (per-side column windows)
    char pad7c[0x184 - 0x7c];
    int f184;                                   // +0x184
    void fill_single_mission();                                         // 0x4db020
    void fill_mission_grid();                                           // 0x4db090
    void init_mission_window();                                         // 0x4dae40
    void init_meeting_window();                                         // 0x4db1f0
    void fill_mission_row(int side);                                    // 0x4db440
};

// ---- game objects touched by 0x4db090 / 0x4db440 ------------------------------------------------------
struct E48 {                                    // one 0x48-byte string-table entry (name at +4)
    void* p0;
    void* name;                                 // +0x04
};
struct Unit {                                   // a 0x508-byte game unit (table at g_game->units)
    char pad0[0x30];
    int flags30;                                // +0x30
    char pad34[0x58 - 0x34];
    int f58;                                    // +0x58
    char pad5c[0x7a - 0x5c];
    unsigned char f7a;                          // +0x7a (health rank, low 4 bits)
    char pad7b[0xa0 - 0x7b];
    float fa0;                                  // +0xa0 (health)
    char pada4[0x508 - 0xa4];
};
struct PartyUnitRec {                           // 0x20-byte entry of a party's unit list
    int f0;                                     // +0x00
    int id;                                     // +0x04
    int f8;                                     // +0x08
    int fc;                                     // +0x0c
    int f10;                                    // +0x10 (stack size)
    char pad14[0xc];
    tstring* s20;                               // +0x20 (name)
};
struct Party {
    char pad0[0xa8];
    PartyUnitRec* first;                        // +0xa8
    char padac[0x18];
    int count();                                // 0x463190
};
struct Game {
    char pad0[0x78];
    int side_party;                             // +0x78
    char pad7c[0x139c8 - 0x7c];
    void* chunks;                               // +0x139c8 (chunk table of unit blocks)
    char pad9cc[0x13a00 - 0x139cc];
    Unit* units;                                // +0x13a00
};

struct GuiMgr { void reset(); void add_root(Overlay* const& p); };
struct ResMgr { void* get_object(const tstring& n); };
struct StrTab {                                 // 0x5e2a18
    char pad0[0xdf34];
    int count;                                  // +0xdf34
    const void* find(const void* key);          // 0x4c6010
};

#define g_gui     (*(GuiMgr*)0x5d7080)
#define g_resmgr  (*(ResMgr*)0x5d6fa0)
#define g_strtab  (*(StrTab*)0x5e2a18)
#define g_5d8788  (*(char*)0x5d8788)

extern Game* g_game;                            // 0x5e2930
extern void* g_5e292c;                          // 0x5e292c (world; +0x5df0 = selected mission)
extern void* g_5e2a00;                          // 0x5e2a00 (mission table, 0x58-byte records; +0x24 = name)
extern int g_a73314;                            // 0xa73314 (mission stage)
extern int g_a73430;                            // 0xa73430 (grid rows)
extern int g_a73428;                            // 0xa73428 (grid columns)
extern int g_a7342c;                            // 0xa7342c (grid depth)
extern int g_a73348[];                          // 0xa73348 (grid of unit indices)
extern int g_a73438;                            // 0xa73438 (selected index, -1 = none)
extern int g_a73300[];                          // 0xa73300 (selection table)
extern int g_a732fc[];                          // 0xa732fc (per-side fallback ids)
extern char g_a181b8;                           // 0xa181b8 (hide-names flag)
extern int g_a18210;                            // 0xa18210 (health rank limit)
extern float g_590cd0;                          // 0x590cd0 (row step, 0.1)
extern float g_590c60;                          // 0x590c60
extern float g_54a6f0;                          // 0x54a6f0
extern float g_54ab28;                          // 0x54ab28
extern float g_54b660;                          // 0x54b660
extern float g_54b3c8;                          // 0x54b3c8
extern float g_5a9f28[];                        // 0x5a9f28 {0.0, 0.2145}
extern int g_skill_clamp; // 0x8ee3ac4

// ---- callees -------------------------------------------------------------------------------------------
void ctor_48ae20(void* p);                      // 0x48ae20 (ecx = object)
void* __fastcall clone_41c6d0(void* m);         // 0x41c6d0 (ecx = mesh)
void __fastcall ctor_494310(void* p);           // 0x494310 (ecx = 0x5aa128: the main mission UI)
void* __fastcall party_find_48ad00(Party* p, int unused, int id);    // 0x48ad00 (ecx = party array)
int crt_sprintf(char* buf, const char* fmt, ...);   // 0x536f8f
void __fastcall dynamic_4d7e50(Screen* s, int unused);  // 0x4d7e50

// ---- widget/overlay construction shared by 0x4dae40 and 0x4db1f0 ---------------------------------------
// Both functions are the same sequence with a different overlay mesh name; kept inline in each so the
// instruction order stays visible next to the original.
#define NEW_OVERLAY_BASE(name_string)                                                   \
    g_gui.reset();                                                                      \
    ctor_494310((void*)0x5aa128);                                                       \
    Overlay* p = (Overlay*)operator new(0x190);                                         \
    overlay = p != 0 ? (ctor_4931a0(p), *(void**)p = (void*)0x592848,                   \
                        ((char*)p)[0x188] = 0, p) : 0;                                  \
    {                                                                                   \
        float v[2] = { 0.0f, 0.0f };                                                    \
        overlay->set_pos(v);                                                            \
    }                                                                                   \
    {                                                                                   \
        float v[2] = { 1.0f, 0.75f };                                                   \
        overlay->set_scale(v);                                                          \
    }                                                                                   \
    {                                                                                   \
        tstring name(name_string);                                                      \
        overlay->set_mesh(0, clone_41c6d0(g_resmgr.get_object(name)));                  \
    }                                                                                   \
    {                                                                                   \
        int c[1] = { -1 };                                                              \
        overlay->set_color(c);                                                          \
    }

#define NEW_OVERLAY(name_string)                                                        \
    NEW_OVERLAY_BASE(name_string)                                                       \
    {                                                                                   \
        float v[2] = { 0.05f, 0.05f };                                                  \
        overlay->set_size(v);                                                           \
    }                                                                                   \
    row[2] = new Widget;                                                                \
    {                                                                                   \
        float v[2] = { 0.1f, 0.2f };                                                    \
        row[2]->set_pos(v);                                                             \
    }                                                                                   \
    {                                                                                   \
        float v[2] = { 0.6f, 0.3f };                                                    \
        row[2]->set_scale(v);                                                           \
    }                                                                                   \
    {                                                                                   \
        int c[1] = { -1 };                                                              \
        row[2]->set_color(c);                                                           \
    }                                                                                   \
    {                                                                                   \
        float v[2] = { 0.03f, 0.03f };                                                  \
        row[2]->set_size(v);                                                            \
    }                                                                                   \
    overlay->add_child(row[2]);

// @ 0x4dada0
Widget* make_hint(Overlay* o, const tstring& text, float x, float y, float w, float h)
{
    Widget* r = new Widget();
    {
        float pos[2] = { x, y };
        r->set_pos(pos);
    }
    {
        float sc[2] = { w, h };
        r->set_scale(sc);
    }
    float z = h * 0.7f;
    {
        float sz[2] = { z, z };
        r->set_size(sz);
        r->set_text(sz, &text);
    }
    o->add_child(r);
    return r;
}

// @ 0x4db020
void Screen::fill_single_mission()
{
    rgl::vector2 pos;
    {
        tstring none("");
        overlay->set_text(&pos, &none);
    }
    row[2]->set_text(&pos, (const void*)(*(int*)((char*)g_5e292c + 0x5df0) * 0x58 + 0x24 + (char*)g_5e2a00));
}

// @ 0x4db090
void Screen::fill_mission_grid()
{
    float y = 0.7f;
    {
        tstring none("");
        rgl::vector2 pos;
        overlay->set_text(&pos, &none);
    }
    int kb = 0;
    for (int k = 0; k < g_a73430; k++) {
        int jb = 0;
        for (int j = 0; j < g_a7342c; j++) {
            int* cell = (int*)((char*)0xa73348 + kb + jb);
            for (int i = 0; i < g_a73428; i++) {
                Unit* u = &g_game->units[*cell];
                const void* txt = g_strtab.find((char*)u + 0x20);
                if (!txt)
                    txt = u;
                float v[2] = { 0.1f, y };
                overlay->set_text2(txt, v);
                cell++;
                y -= 0.02f;
            }
            jb += 0xc;
            y -= 0.005f;
        }
        kb += 0x38;
        y -= 0.01f;
    }
}

// @ 0x4dae40
void Screen::init_mission_window()
{
    row[4] = 0;
    NEW_OVERLAY("mission_window")
    if (g_a73314 == 2)
        fill_mission_grid();
    else if (g_a73314 == 3 || g_a73314 == 4 || g_a73314 == 5)
        fill_single_mission();
    g_gui.add_root(overlay + 0);
    g_5d8788 = 1;
}

// @ 0x4db1f0
void Screen::init_meeting_window()
{
    NEW_OVERLAY_BASE("meeting_window")
    for (int i = 0; i < 16; i++) {
        row[i] = new Entry;
        {
            float v[2] = { 0.0f, 0.41f - i * 0.05f };
            row[i]->set_pos(v);
        }
        {
            float v[2] = { 0.6f, 0.05f };
            row[i]->set_scale(v);
        }
        {
            int c[1] = { (int)0xffaaff55 };
            row[i]->set_color2(c);
        }
        overlay->add_child(row[i]);
    }
    int j = 0;
    do {
        j++;
        Column* c = (Column*)operator new(0x188);
        col[j - 1] = c != 0 ? (ctor_4931a0(c), *(void**)c = (void*)0x597050, c) : 0;
        {
            float v[2] = { 0.6f, 0.65f - j * 0.2145f };
            c->set_pos(v);
        }
        c->set_scale(g_5a9f28);
        overlay->add_child(c);
    } while (j < 3);
    g_gui.add_root(overlay + 0);
    g_5d8788 = 1;
    fill_mission_row(0);
    fill_mission_row(1);
    fill_mission_row(2);
}

// @ 0x4db440
void Screen::fill_mission_row(int side)
{
    int id;
    if (side == 0)
        id = g_game->side_party;
    else if (g_a73438 < 0)
        id = g_a732fc[side];
    else if (side == 1)
        id = g_a73300[g_a73438];
    else
        id = *(int*)(0xa73304 - g_a73438 * 4);
    if (id == -1)
        return;
    int idx = (int)party_find_48ad00((Party*)((char*)g_game + 0xf8), 0, id);
    char* chunks = *(char**)((char*)g_game + 0x139c8);
    int base = *(int*)(chunks + (idx >> 4) * 4);
    Party* party = (Party*)(base + (idx & 0xf) * 0x5ac8);

    rgl::vector2 v;
    const void* text;
    if (g_a181b8)
        text = (char*)party + 0x20;
    else if (*(char*)((char*)party + 0x1c) == 0 &&
             (text = g_strtab.find((char*)party + 0x20)) != 0)
        text = text;
    else
        text = (char*)party + 0xc;
    col[side]->set_text(&v, text);

    float y = 0.15f;
    for (int i = 0; i < party->count(); i++) {
        PartyUnitRec* rec = (PartyUnitRec*)((char*)party + 0xa8) + i;
        Unit* u = &g_game->units[rec->id];
        char buf[0x20];
        if (u->flags30 & 0x10) {
            int h = u->f7a & 0xf;
            if (h > *(int*)0x8ee3ac4)
                h = *(int*)0x8ee3ac4;
            crt_sprintf(buf, " %d%%",
                        (int)(u->fa0 * 100.0f / (u->f58 + h * 2 + 0x23) + 0.0001f));
        } else {
            crt_sprintf(buf, " (%d/%d)", rec->f0, rec->f0 - rec->f10);
        }
        const void* nm = g_strtab.find((char*)u + 0x20);
        if (!nm)
            nm = u;
        const void* nm2 = (const void*)((const char*)nm + 4);
        if (*(const void**)nm2)
            nm2 = *(const void**)nm2;
        char line[0x30];
        crt_sprintf(line, "%s %s", nm2, buf);
        {
            tstring ts(line);
            v.x = 0.01f;
            v.y = y;
            col[side]->set_text2(&ts, &v);
        }
        y -= 0.02f;
    }
}
