// mission_ui.h: the mission-UI widget base (vtable 0x591c80) and the small helper classes whose members sit
// next to it in mission_ui.cpp (0x488560..0x48b2d0). Names provisional.
//
// MissionUi is ONE class. The slices called it `Effect` (s00487c40, s004885e0: the six track setters, flag
// accessors), `MissionUi` (s00488b20, s0048add0: text placement, ctor, flag setters) and `Buffer` (s004875a0:
// its destructor 0x487c00 stores the same vtable 0x591c80); renderer.h calls the leading part `envelope`
// (0x4624c0, 0x462540 write +0xdc..+0x100). Every member below reads a field of the one layout; the offset
// comments cite a function that proves each.
//
// The slices also put members of other objects under `MissionUi` because they sit between its members; the
// disassembly shows other `this` types (see MIGRATION.md): MissionPanel (draw), ElemList (call0 / call1),
// PairStack (push), TimerChild (tick), ItemRef (sort helpers), Troop (0x4892a0..0x489370).
#ifndef GAME_UI_MISSION_UI_H
#define GAME_UI_MISSION_UI_H
#include "../tstring.h"
#include "./ui_common.h"
#include "./item_ref.h"
#include "../../include/clock_table.h"   // g_ticks (elapsed() below)
#include "../../rgl/mesh.h"


// a packed RGBA word with its channels (the animate overrides lerp them: valA.. / srcA.. below)
union Color4 {
    unsigned dword;
    struct { unsigned char r, g, b, a; };
};

struct MissionUi {
    // vtable 0x591c80, 49 slots. Names where a caller shows the signature; the rest are placeholders. Every slot is
    // defined in widget_slots.cpp: a leaf the original's slot also holds (`return 0`, `ret`), or a bare `jmp` to the
    // function the original's slot names (the comment on each: slot, its address and owner). A derived widget class
    // overrides a slot with the same name and its own jmp.
    __declspec(noinline) virtual ~MissionUi();   // +0x00 0x487c00 (mesh_slot/slot.cpp); the deleting destructor 0x502300 is the compiler's (called with push 1 by 0x488ad0)
    virtual bool is_active();                     // +0x04 "is active" (tested by 0x488720): returns 0 (0x4280e0; UiA & co. return 1: 0x48b170)
    virtual void animate(int unused);       // +0x08 0x487c40 (mesh_slot/slot.cpp: the widgets' per-frame track animator; the argument is not used)
    virtual void update_bounds();           // +0x0c 0x48d020 (mission_ui2/mission_3.cpp: the union of the four shapes' bounds)
    virtual void set_a(float a, int b);                         // +0x10 0x488560
    virtual void set_b(float a, int b);                         // +0x14 0x4885a0
    virtual void set_c(float a, int b);                         // +0x18 0x4885e0
    virtual void set_d(float a, int b);                         // +0x1c 0x488620
    virtual void set_e(float a, float* p);                      // +0x20 0x488660
    virtual void set_f(float a, float* p);                      // +0x24 0x4886c0
    virtual void render(rgl::scene* sc, float* count, int c, int d);   // +0x28 0x488720 per-child draw loop (ret 0x10; the derived widgets' update / show)
    virtual void set15c(int v);                                 // +0x2c 0x488890
    virtual int  get15c();                                      // +0x30 0x4888a0
    virtual void set168(float v);                               // +0x34 0x4888b0
    virtual float get168();                                     // +0x38 0x4888c0
    virtual bool test_c8(int m);                                // +0x3c 0x4888d0
    virtual void clear_c8();                                    // +0x40 0x4888e0
    virtual bool clear_mask(int m);                             // +0x44 0x4888f0
    virtual void set_key_flags(int m);                          // +0x48 0x488910 (0x48b200, 0x48b5a0)
    virtual void set_d0(int v);                                 // +0x4c 0x488930
    virtual void set_d4(int v);                                 // +0x50 0x488940
    virtual void or_d4(int m);                                  // +0x54 0x488950
    virtual void move_a(const float* p);                           // +0x58 0x488970: position/offset pair (0x487c40)
    virtual void on_child_moved(MissionUi* src);       // +0x5c "child moved" notification, passed this (0x488970): `ret 4` (0x408030)
    virtual void on_moved();                     // +0x60 `ret` (0x477b70)
    virtual void set_xy(const float* p);                           // +0x64 0x4889b0 (0x488b50, 0x48b090)
    virtual void move_f8(const float* p);                          // +0x68 0x4889e0
    virtual void set_size(const float* p);                      // +0x6c 0x48b090 (0x48b3a0)
    virtual void set_offset(const float* p);                       // +0x70 0x488a10 (0x48b3a0)
    virtual void set148(const int* p);                           // +0x74 0x488a40
    virtual void or148(const int* p);                            // +0x78 0x488a70
    virtual void set_mesh(int slot, rgl::mesh* m);                           // +0x7c 0x488aa0
    virtual void set_track(int i, rgl::mesh* p);                // +0x80 0x488ad0 (0x488aa0)
    virtual void normalize_uv(rgl::mesh* src);   // +0x84 0x495fa0 (world_data/mesh_hud.cpp: rescales src's uv records into the rectangle screen[] holds; 0x488aa0)
    virtual void on_track_set(void* a);              // +0x88 `ret 4` (0x408030) (0x488ad0)
    virtual char key(unsigned mask);        // +0x8c 0x42cd60 (widget_slots.cpp; the derived widgets override it with their key handler)
    virtual void refresh();                 // +0x90 0x42ce50 (widget_slots.cpp; 0x48b200)
    virtual void get_xy(float* v);          // +0x94 0x49b4f0 (config/widget_mgr.cpp: the text rect, through a hidden result pointer; 0x488b30, 0x488a40)
    virtual void set_color(unsigned* p);                          // +0x98 0x488b10 (0x487c40)
    virtual void set_target(void* p);                           // +0x9c 0x488b20 (0x487c40)
    virtual void set_pos(int p);                                // +0xa0 0x488b30
    virtual float* set_text(float* out, const tstring* s);      // +0xa4 0x488b50 (MediumButton overrides it: 0x48b3a0)
    virtual void put(tstring* out);                             // +0xa8 0x488c70
    virtual void layout_text_to(const tstring* text, const float* pos);   // +0xac 0x49abc0 (config/widget_mgr.cpp; ret 8)
    virtual void hide();                                        // +0xb0 0x488c90
    virtual void show();                                        // +0xb4 0x488ca0
    virtual void flag2_set();                                   // +0xb8 0x488cb0
    virtual void flag2_clear();                                 // +0xbc 0x488cc0
    virtual int kind();                     // +0xc0 widget type code: returns 0 (0x408020; the derived widgets return their kind)
    // 49 slots: the original vtable 0x591c80 ends here. A 50th (+0xc4, `update_children`) exists only in the classes
    // that add it, each with its own function: UiC 0x497d10, UiD 0x498390, progress_bar 0x4e3fb0, NodeA 0x48ef40.

    // six (clock index, start stamp) track pairs; the clock index selects an entry of g_ticks
    int clock0; __int64 t0;                 // +0x08 / +0x10  0x488560, 0x48ae20
    int clock1; __int64 t1;                 // +0x18 / +0x20  0x4885a0
    int clock2; __int64 t2;                 // +0x28 / +0x30  0x4885e0
    int clock3; __int64 t3;                 // +0x38 / +0x40  0x488620
    int clock4; __int64 t4;                 // +0x48 / +0x50  0x488660
    int clock5; __int64 t5;                 // +0x58 / +0x60  0x4886c0
    float periodA, periodB, periodC, periodD, periodE, periodF;   // +0x68..+0x7c  0x488560..0x4886c0 (default -1.0: 0x48ae20)
    float f80, f84;                         // +0x80 0x4886c0
    float f88, f8c;                         // +0x88 0x488660
    int valA, valB, valC, valD;             // +0x90..+0x9c  0x488560..0x488620
    float a0, a4, a8, ac;                   // +0xa0..+0xac  0x488660, 0x4886c0
    int srcA, srcB, srcC, srcD;             // +0xb0..+0xbc  0x488560..0x488620
    unsigned char c0, c1, c2, c3, c4, c5;   // +0xc0..+0xc5 track "restart" flags  0x488560..0x4886c0, 0x48ae20
    unsigned char padc6[2];
    unsigned fC8, fCC, fD0, fD4;            // +0xc8..+0xd4  0x4888d0..0x488950
    float fD8, fDC;                         // +0xd8 (1.0), +0xdc  0x48ae20, 0x488cd0
    union {
        struct { float fE0, fE4, fE8, fEC; };   // +0xe0.. 0x4886c0, 0x488970, 0x488cd0 (fE8: byte flag there, float at 0x488970; fEC smoothing)
        float e[4];                         // indexed by the slider index (0x48b5a0)
    };
    float screen[4];                        // +0xf0 text-space size / base  0x488b50, 0x4889b0 (fF0,fF4), 0x4889e0 (fF8,fFC), 0x48b4f0
    int f100;                               // +0x100 (1)  0x48ae20
    unsigned f104;                          // +0x104 flag word (bit 0 hidden, bit 1 tint)  0x488720, 0x488c90..0x488cc0
    rgl::mesh* tracks[5];                   // +0x108 child meshes (vtable 0x54a6f4); [4] is the primary one  0x488720, 0x488ad0, 0x48b4f0
    unsigned char b11c[5];                  // +0x11c..0x120 per-child "advance" flags  0x488720, 0x48ae20; [4] = 0x120 set by 0x488b50
    unsigned char b121;                     // +0x121  0x488720, 0x48b180
    unsigned char b122;                     // +0x122  0x48ae20
    unsigned char pad123;
    int f124;                               // +0x124  0x488b30
    tstring name;                           // +0x128  0x488b50, 0x488c70
    float f138, f13c, f140, f144;           // +0x138..+0x144 text metrics  0x488a10, 0x488b50, 0x48b090
    unsigned f148;                          // +0x148 option flags  0x488a40, 0x488b50
    unsigned color14c, color150;            // +0x14c / +0x150  0x488720, 0x488b10, 0x488b20
    MissionUi* p154;                        // +0x154 forwarding target  0x488970
    int f158;                               // +0x158  0x48ae20
    int v15c;                               // +0x15c  0x488890
    int f160;                               // +0x160
    int f164;                               // +0x164 (1)
    float v168;                             // +0x168 value / current  0x4888b0
    float f16c;                             // +0x16c range min
    float f170;                             // +0x170 range max (1.0)  0x48ae20
    int f174;                               // +0x174

    MissionUi();                                        // 0x48ae20
    unsigned flag1() const;                             // 0x50cc60 (inventory_panel/widget_text.cpp: the hidden bit)
    // views of the track records for the animate overrides (mesh_hud.cpp)
    float elapsed(int i) const { return (float)(g_ticks[*(const int*)((const char*)this + 8 + i * 0x10)] - *(const __int64*)((const char*)this + 0x10 + i * 0x10)) * 1e-5f; }
    float& dur(int i) { return (&periodA)[i]; }                 // +0x68 + 4i
    Color4& ca() { return *(Color4*)&valA; }                    // +0x90
    Color4& aa() { return *(Color4*)&valB; }                    // +0x94
    Color4& ca2() { return *(Color4*)&valC; }                   // +0x98
    Color4& aa2() { return *(Color4*)&valD; }                   // +0x9c
    Color4& cb() { return *(Color4*)&srcA; }                    // +0xb0
    Color4& ab() { return *(Color4*)&srcB; }                    // +0xb4
    Color4& cb2() { return *(Color4*)&srcC; }                   // +0xb8
    Color4& ab2() { return *(Color4*)&srcD; }                   // +0xbc
    void set_fog(const float* ref, const float* lo, const float* hi);   // 0x488cd0
};

// A list of 0x1dc-byte effect elements (FxElem, vtable 0x54b454) at +4; 0x488e20 / 0x488e40 forward to the
// element's virtuals 0 and 1 (slot 0 is FxElem's destructor entry, called with an int argument).
struct ElemVt { virtual void s0(int a); virtual void s1(int a); char pad[0x1dc - 4]; };
struct ElemList {
    char pad0[4];
    ElemVt* elems;                          // +4
    void call0(int i, int a);               // 0x488e20
    void call1(int i, int a);               // 0x488e40
};

// A stack of up to 0x80 (a, b) int pairs at +0 with its count at +0x400 (0x488e60).
struct PairStack {
    int pairs[0x100];                       // +0
    int count;                              // +0x400
    void push(int a, int b);                // 0x488e60
};

// A timed voice: a period, one sound channel (0x1c bytes) and a clock; 0x30 bytes (0x488e90, 0x48add0).
struct TimerChild {
    float period;                           // +0x00 seconds between plays; < -1: play when round(period) == the request
    snd_channel ch;                         // +0x04
    volatile int clock;                     // +0x20 index into g_ticks (re-read, never CSE'd: 0x488e90)
    int pad24;
    __int64 last;                           // +0x28 last play time
    void tick(int idx);                     // 0x488e90
};
#include "./timer_list.h"                 // TimerList: count +0, items +4, update 0x48add0

// free functions of the item-pair sort and the speed table
int __fastcall sort_value(ItemRef* e);                              // 0x488f80
float __stdcall speed_scale(const float* base, const float* ref, float r);   // 0x488ff0
void __fastcall clear_slots(ItemRef* e);                            // 0x489060 (sets the first 0x60 items to -1)

#endif // GAME_UI_MISSION_UI_H
