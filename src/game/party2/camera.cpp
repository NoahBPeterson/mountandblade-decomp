// camera.cpp: TU 92 of the map (work/migrate/tu_map.csv), 0x49b1c0..0x4b1160, minus the script interpreter's own range
// (0x49cc60..0x49d4b0 and 0x49fa10, src/game/script). Migrated from the staging slices s0049aca0, s0049b630, s0049bea0,
// s0049c2c0, s0049d500, s0049deb0, s0049e780, s0049f060 and s004b0940 (match/slices stays the reference). See MIGRATION.md.
//
// Layout of this file, in order:
//   - namespace c49aca0 .. c49f060: the slices' still-provisional views of objects whose canonical class is not (yet) usable
//     here (the text-list panel, the order / army records, the view camera, the scene sprites). One namespace per slice keeps
//     their same-named but different classes (Mission, Party, Scene, ...) apart; every function keeps its `// @ 0x...` marker.
//   - GameData members (add_gold, remove_gold, start_quest, label_text) on the canonical GameData (include/game_data.h).
//   - the script block runner (DialogBlock / Elem, include/dialog_rec.h, camera.h): set_lvalue, get_operand, step_less,
//     renumber, link, pop_try, run_line, emit. The slices viewed these as `Interp`, `Mission` and `ScriptHolder`.
#include <new>
#include <string.h>
#include "../../rgl/rgl_math.h"
#include <vector>
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <stddef.h>
#include "../tstring.h"
#include "../font.h"
#include "../../include/game_data.h"
#include "../../include/dialog_rec.h"
#include "../../rgl/mesh.h"
#include "../../include/party.h"
#include "../terrain_b/terrain_b.h"
#include "./camera.h"
#include "./party_2.h"
extern "C" void _ReadWriteBarrier(void); // compiler barrier (emits no code)

extern Font* g_font;                                // 0x5e2934


// ======================================================================================
// slice s0049aca0 (namespace c49aca0)
// ======================================================================================
namespace c49aca0 {
// Slice s0049aca0: a mission-UI "text list" panel.  The panel (`this` of 0x49aca0/0x49ad20/0x49b4f0, a
// MissionUi-derived widget) owns a container object (vptr 0x592848, base ctor 0x4931a0) that it builds in
// create() and a std::vector<E100> of text rows; create() also builds one flag-widget child per row (UiA,
// vptr 0x591d58, base ctor 0x48ae20).  Also here: the container's scalar deleting destructor, the vector
// fast path / insert / _Insert_n, a Mission::add_placed trampoline and the text-rect layout wrapper.
// Names and field names are provisional; every offset is read off the disassembly.

using rgl::vector4;

// ---- shared engine types -------------------------------------------------------------------------
using rgl::mesh;

struct resmgr { void* lookup(const tstring& name); };                  // 0x418a70
struct Fadeable { void fade_scale(vector4 v); };                    // 0x47af30

struct R8 { int a, b; };                        // an 8-byte returned rect (2 floats)
struct E16 { int a, b, c, d; };                 // one 16-byte text-row record

// ---- external callees ----------------------------------------------------------------------------
void __fastcall base_ctor_4931a0(void* o);      // 0x4931a0 (ecx = object)
void __fastcall base_ctor_48ae20(void* o);      // 0x48ae20 (ecx = object)
void __fastcall base_dtor_490520(void* o);      // 0x490520 (ecx = object)
void __fastcall play_sound_48b2d0(void* m);     // 0x48b2d0
extern "C" void __stdcall __CxxThrowException(void*, void*);        // 0x5368e6
extern void* g_5e2930;                          // 0x5e2930 (game-data pointer, matrix arena at +0x14014)
struct Mission { int add_placed(const int* kind, const int* id, const int* value, const void* m, int* spawn_id); }; // 0x44f870

struct ptr_tag {};                               // empty class passed by value (one slot)
struct NodeA;
struct UiA;
struct PObj;

// ---- std::vector<E100> (4-byte element, `_Myfirst` at +4) -----------------------------------------
typedef NodeA* E4;
struct PVec {
    void* f0;                                   // +0
    E4* first;                                  // +4
    E4* last;                                   // +8
    E4* end;                                    // +0xc
    __declspec(noinline) void push_back(const E4& x);   // 0x49b120
    void insert(E4** out, E4* where, const E4& x);      // 0x49b170
    void _Insert_n(E4* where, unsigned count, const E4& x);  // 0x49b1c0
    void _Xlen() const;                         // 0x43e2a0
};

E4* __cdecl uninit_copy(E4* f, E4* l, E4* d, PVec* al, ptr_tag tag);   // 0x435a10 (trivial _Uninit_copy = memmove)

// ---- class A: the panel (`this` of 0x49aca0 / 0x49ad20 / 0x49b4f0) ---------------------------------
struct PObj {
    virtual void p00(); virtual void p01(); virtual void p02(); virtual void p03(); virtual void p04();
    virtual void p05(); virtual void p06(); virtual void p07(); virtual void p08(); virtual void p09();
    virtual void p10(); virtual void p11(); virtual void p12(); virtual void p13(); virtual void p14();
    virtual void p15(); virtual void p16(); virtual void p17();
    virtual void set_key_flags(int f);          // +0x48
    virtual void p19(); virtual void p20(); virtual void p21(); virtual void p22(); virtual void p23();
    virtual void p24(); virtual void p25(); virtual void p26(); virtual void p27(); virtual void p28();
    virtual void p29(); virtual void p30();
    virtual void v31(int a, void* b);           // +0x7c
    virtual void p32(); virtual void p33(); virtual void p34(); virtual void p35();
    virtual void refresh();                     // +0x90
    virtual void p37();
    virtual void v38(void* p);                  // +0x98

    char pad0[0xd8 - 4];                        // +0x04
    float dx8;                                  // +0xd8
    float dxc;                                  // +0xdc
    float de0;                                  // +0xe0
    float de4;                                  // +0xe4
    char pade8[0xf0 - 0xe8];
    float vf0[4];                               // +0xf0  (vf0[0] = xmax, vf0[2] = xmin)
    char pad100[0x118 - 0x100];
    mesh* m118;                                 // +0x118
    char pad11c[0x122 - 0x11c];
    char f122;                                  // +0x122
    char pad123[0x124 - 0x123];
    int f124;                                   // +0x124
    tstring name;                                  // +0x128
    float f138;                                 // +0x138
    float f13c;                                 // +0x13c
    float v140[2];                              // +0x140
    unsigned f148;                              // +0x148
    int f14c;                                   // +0x14c
    char pad150[0x180 - 0x150];
    char f180;                                  // +0x180
    char pad181[0x188 - 0x181];
    NodeA* f188;                                // +0x188
    char pad18c[0x190 - 0x18c];
    E16* v190_first;                            // +0x190  (std::vector of E16 rows)
    E16* v190_last;                             // +0x194
    E16* v190_end;                              // +0x198
    char pad19c[0x1a0 - 0x19c];
    mesh* f1a0;                                 // +0x1a0
    mesh* f1a4;                                 // +0x1a4
    mesh* f1a8;                                 // +0x1a8
    int f1ac;                                   // +0x1ac (never read here)
    float f1b0, f1b4, f1b8;                     // +0x1b0
    float f1bc, f1c0, f1c4, f1c8;               // +0x1bc

    char on_event(unsigned ev);                 // 0x49aca0
    NodeA* create();                            // 0x49ad20
    void place(int* p);                         // 0x49b4b0
    R8 text_rect();                             // 0x49b4f0
};

// ---- class B: the container (vptr 0x592848, base ctor 0x4931a0, size 0x190) ------------------------
struct NodeA {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18();
    virtual void v19(int a);                    // +0x4c
    virtual void v20(int a);                    // +0x50
    virtual void v21();                         // +0x54
    virtual void v22(float* p);                 // +0x58
    virtual void v23();                         // +0x5c
    virtual void v24();                         // +0x60
    virtual void v25(float* p);                 // +0x64
    virtual void v26(); virtual void v27(); virtual void v28(); virtual void v29();
    virtual void v30(); virtual void v31(); virtual void v32(); virtual void v33(); virtual void v34();
    virtual void v35(); virtual void v36(); virtual void v37(); virtual void v38(); virtual void v39();
    virtual void v40(); virtual void v41(); virtual void v42(); virtual void v43(); virtual void v44();
    virtual void v45(); virtual void v46(); virtual void v47(); virtual void v48(); virtual void v49();
    virtual void v50(void* p);                  // +0xc8

    char pad0[0x158 - 4];
    void* f158;                                 // +0x158
    char pad15c[0x188 - 0x15c];
    char f188;                                  // +0x188
    char pad189[0x190 - 0x189];

    void* destroy(unsigned flags);              // 0x49b0f0
};

// ---- class C: the flag widget (vptr 0x591d58, base ctor 0x48ae20, size 0x188) ----------------------
struct UiA {
    virtual void u00(); virtual void u01(); virtual void u02(); virtual void u03(); virtual void u04();
    virtual void u05(); virtual void u06(); virtual void u07(); virtual void u08(); virtual void u09();
    virtual void u10(); virtual void u11(); virtual void u12(); virtual void u13(); virtual void u14();
    virtual void u15(); virtual void u16(); virtual void u17();
    virtual void set_key_flags(int f);          // +0x48
    virtual void v19(int a);                    // +0x4c
    virtual void v20(int a);                    // +0x50
    virtual void u21();                         // +0x54
    virtual void v22(float* p);                 // +0x58
    virtual void u23();                         // +0x5c
    virtual void u24();                         // +0x60
    virtual void v25(float* p);                 // +0x64
    virtual void u26();                         // +0x68
    virtual void set_size(float* p);            // +0x6c
    virtual void set_pos(float* p);             // +0x70
    virtual void u29(); virtual void u30(); virtual void u31();
    virtual void v32(int a, void* b);           // +0x80
    virtual void u33(); virtual void u34(); virtual void u35();
    virtual void refresh();                     // +0x90
    virtual void get_xy(float* p);              // +0x94
    virtual void u38(); virtual void u39(); virtual void u40();
    virtual void v41(void* a, void* b);         // +0xa4

    char pad0[0x10c - 4];
    void* f10c;                                 // +0x10c
    void* f110;                                 // +0x110
    void* f114;                                 // +0x114
    char pad118[0x121 - 0x118];
    char f121;                                  // +0x121
    char pad122[0x178 - 0x122];
    int f178;                                   // +0x178
    char pad17c[0x180 - 0x17c];
    char f180;                                  // +0x180
    char pad181[0x184 - 0x181];
    int f184;                                   // +0x184
};

extern PVec g_5d7080;                           // 0x5d7080 (the container list)

// ---- function definitions (in VA order) ----------------------------------------------------------

// byte difference shifted right by 2, as the original computes every element count (a plain pointer
// difference lets cl assume an exact division, which differs for misaligned pointers)
static inline int pdiff(E4* a, E4* b) { return (int)((char*)a - (char*)b) >> 2; }

// @ 0x49b1c0
// std::vector<T*>::_Insert_n (VC7.1).  The prefix/suffix copies are the out-of-line trivial _Uninit_copy
// (0x435a10: memmove of (last - first) / 4 words, returns dest + that); the other two branches inline it as
// memmove.  A try block guards the new buffer (catch(...) { operator delete(S); throw; }).
void PVec::_Insert_n(E4* where, unsigned count, const E4& val)
{
    E4 tmp = val;
    unsigned cap = first ? (unsigned)pdiff(end, first) : 0u;
    if (count == 0)
        return;
    unsigned size = first ? (unsigned)pdiff(last, first) : 0u;
    if (0x3fffffffu - size < count) {
        _Xlen();
        return;
    }
    if (cap < (first ? (unsigned)pdiff(last, first) : 0u) + count) {
        cap = (0x3fffffffu - cap / 2 < cap) ? 0u : cap + cap / 2;
        if (cap < (first ? (unsigned)pdiff(last, first) : 0u) + count)
            cap = (first ? (unsigned)pdiff(last, first) : 0u) + count;
        E4* newvec = (E4*)::operator new(cap * sizeof(E4));
        ptr_tag tag;
        try {
            E4* q = uninit_copy(first, where, newvec, this, tag);
            for (unsigned k = count; k > 0; --k, ++q) *q = tmp;
            uninit_copy(where, last, q, this, tag);
        } catch (...) {
            ::operator delete(newvec);
            throw;
        }
        count += first ? (unsigned)pdiff(last, first) : 0u;
        if (first)
            ::operator delete(first);
        end = newvec + cap;
        last = newvec + count;
        first = newvec;
    } else if ((unsigned)pdiff(last, where) < count) {
        E4* oldend = last;
        memmove(where + count, where, (unsigned)pdiff(oldend, where) * sizeof(E4));
        try {
            E4* q = oldend;
            for (unsigned k = count - (unsigned)pdiff(oldend, where); k > 0; --k, ++q) *q = tmp;
        } catch (...) {
            throw;
        }
        last = oldend + count;
        for (E4* q = where; q != oldend; ++q) *q = tmp;
    } else {
        E4* oldend = last;
        memmove(oldend, oldend - count, count * sizeof(E4));
        last = oldend + count;
        int n = pdiff(oldend - count, where);
        memmove(oldend - n, where, (unsigned)n * sizeof(E4));
        for (E4* q = where; q != where + count; ++q) *q = tmp;
    }
}

// 0x49b37d is not a function: it is the catch(...) funclet inside PVec::_Insert_n (0x49b1c0, whose COMDAT really spans
// 0x49b1c0..0x49b4af); see nonmatching.txt.

// @ 0x49b4b0
void PObj::place(int* p)
{
    int a = -1;
    int b = 6;
    ((Mission*)this)->add_placed(&b, p, p + 1, (const void*)((char*)g_5e2930 + 0x14014), &a);
}

// @ 0x49b4f0
R8 PObj::text_rect()
{
    if (m118 == 0) {
        mesh* m = new mesh;
        {
            tstring s("ui_font");
            *(void**)((char*)m + 0x2c) = ((resmgr*)0x5d6fa0)->lookup(s);
        }
        v31(4, m);
    }
    if (f122) {
        *(int*)((char*)m118 + 0x114) = 1;
        *(int*)((char*)m118 + 0x140) = 1;
    }

    R8 r;
    float pos[4];
    pos[1] = f13c;
    pos[2] = 0.0f;
    int arg = -1;
    pos[0] = f138;
    Font* font = f124 ? (Font*)f124 : g_font;

    m118->clear_geometry();
    font->layout_text((float*)&r, &name, (vector4*)pos, (float*)v140, m118, (int)&arg,
                      (float*)&vf0[2], (float*)&vf0[0], f148);
    v38(&f14c);
    return r;
}

} // namespace c49aca0

// ======================================================================================
// slice s0049b630 (namespace c49b630)
// ======================================================================================
namespace c49b630 {
// Slice s0049b630: six Mission methods from one translation unit.
//   0x49b630  rebuild the mission's UI "font mesh" (material lookup + layout_text)
//   0x49b720  add "value" to an army slot and, for slot 0, log the gained amount
//   0x49b820  subtract "value" from an army slot (clamped at 0) and log the lost amount
//   0x49b930  mark a quest record as taken (write +0x118/+0x11c, log ui_new_quest_taken)
//   0x49baa0  layout one text row into a mesh with the non-wrap/scale preset
//   0x49bbb0  spawn the battle icon for one 0x98-byte army record
// Names and field names are provisional; every offset is read off the disassembly.
// The class layout is shared with the neighbouring slices s0049bea0 / s0049c2c0 (Mission).
// (tstring.h inlined below)


using rgl::vector4;
using rgl::matrix;
using rgl::mesh;

// ---- engine tables / globals ---------------------------------------------------------------------
struct StrTable { tstring* lookup(const tstring& key); };   // 0x43a1e0
extern StrTable g_strtable;                         // 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };        // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                             // 0x5d7080

extern int g_sound_holder[];                        // 0x5aa128 (sound device)
extern float g_sound_origin[];                      // 0x5aa168 (position vector for play)
extern int g_snd_got_value;                         // 0x5aa1a0 (sound id)
extern int g_snd_lost_value;                        // 0x5aa19c
extern int g_snd_new_quest;                         // 0x5aa180
struct SoundMgr { int play(int idx, int flags, const float* pos, float freq, int* out); };  // 0x442650

struct ResMgr {
    void*  get_material(const tstring& name);       // 0x418a70
    mesh*  get_object(tstring* name);               // 0x419540 (ret 4)
};
extern ResMgr g_resmgr;                             // 0x5d6fa0


struct TickPair {                                   // g_ticks[i].lo + g_ticks[i].hi * 2^32
    int lo, hi;
    operator __int64() const { return ((long long)hi << 32) | (unsigned)lo; }
};
extern TickPair g_ticks[];                          // 0x5e2948

extern "C" int __cdecl sprintf(char*, const char*, ...);        // 0x536f8f
void mb_snprintf(char* buf, unsigned n, const char* fmt, ...);  // 0x415100
void* operator new(unsigned size);                  // 0x53545e

// ---- scene objects -------------------------------------------------------------------------------
struct Shape { virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
               virtual void v4(); virtual void v5(); virtual void v6(); };
struct SphereMem { Shape* construct(vector4 pos, const float* radius); };   // 0x4332a0 (thiscall, ret 0x14, ecx = raw memory)

struct RotFrame { void rotate_sf(const float* angle); };   // 0x40e770 (thiscall): rotates the side/forward rows

// a scene entity (only the offsets this slice touches are named)
struct EntityFull {
    char pad0[0xc];
    int      f0c;                                   // +0xc   (id)
    char pad10[0x58 - 0x10];
    matrix   frame;                                 // +0x58 (frame.o at +0x88 is the position)
    Shape*   shape;                                 // +0x98
    char pad9c[0x11c - 0x9c];
    void**   f11c;                                  // +0x11c (first dword -> matrix at +0xcc)
    char pad120[0x168 - 0x120];
    int      f168;                                  // +0x168  (end fall behaviour = 3)
    void add_mesh(mesh* m);                         // 0x442110 (thiscall, ret 4)
};
struct EntityHost { EntityFull* create_entity(int type); };   // 0x428440 (ret 4)
extern EntityHost* g_entity_host;                   // 0xa732b4 (points to the host object)


// ---- records ---------------------------------------------------------------------------------------
// 0x98-byte army/settlement record (mission+0xe4 page table; see 0x49bbb0)
struct Rec98 {
    char    pad0[0x10];
    int     f10;                                    // +0x10  (=1 -> icon owner present)
    char    pad14[0x24 - 0x14];
    float   f24;                                    // +0x24  (x)
    float   f28;                                    // +0x28  (y)
    char    pad2c[0x3c - 0x2c];
    float   f3c;                                    // +0x3c  (z base)
    char    pad40[0x50 - 0x40];
    int     f50;                                    // +0x50  (scene entity)
    int     f54;                                    // +0x54  (agent id A)
    int     f58;                                    // +0x58  (agent id B)
    void bind_host();                               // 0x4b2770 (thiscall)
};
// 0x138-byte quest record (mission+0x13a58)
struct Rec138 {
    char    pad0[0x40];
    tstring text;                                   // +0x40
    int     f50;                                    // +0x50  (bit 1 -> "new")
    char    pad54[0x64 - 0x54];
    int     f64;                                    // +0x64  (0x100 = already announced)
    char    pad68[0x118 - 0x68];
    int     f118;                                   // +0x118 (taken flag)
    float   f11c;                                   // +0x11c (game time when taken)
};
// 0x5ac8-byte order record: the fields 0x49bbb0 stamps on the two participating agents
struct Rec5ac8 {
    char     pad0[0x88];
    int      pmid;                                  // +0x88  (order icon id)
    char     pad8c[0xa0 - 0x8c];
    unsigned __int64 flags;                         // +0xa0  (0x400 = siege)
    char     pada8[0x918 - 0xa8];
    int      icon_type;                             // +0x918 (7 = battle icon)
    char     pad91c[0x5ac8 - 0x91c];
};

struct AgentTable {
    char    pad0[0x138cc];
    std::vector<char*> pages;                       // +0x138cc, relative to the table (page pointers, 16 records per page)
    int     index_of(int id);                       // 0x48ad00 (ret 4)
    Rec5ac8* get(int id) { int a = index_of(id); return (Rec5ac8*)((a & 0xf) * 0x5ac8 + *(pages.begin() + (a >> 4))); }
};

// ---- Mission -------------------------------------------------------------------------------------
struct Mission {
    char        pad000[0xe0];
    std::vector<char*> f0e0;                        // +0xe0  page table of Rec98 records
    char        pad0f0[0xf8 - 0xf0];
    AgentTable  agents;                             // +0xf8 (0x138d4; pages at +0x139c8)
    char        pad1[0x10000 - 0x6a0];
    int         f132c;                              // +0x132c (font mesh active)
    char        pad1b[0x13a00 - 0x132c - 4];
    char*       f13a00;                             // +0x13a00  array of 0x508 records
    char        pad2[0x13a34 - 0x13a04];
    int         f13a34;                             // +0x13a34
    char        pad3[0x13a58 - 0x13a38];
    char*       f13a58;                             // +0x13a58  array of Rec138 records
    char        pad4[0x13acc - 0x13a5c];
    float       f13acc;                             // +0x13acc
    char        pad5[0x13b18 - 0x13ad0];
    int         f13b18;                             // +0x13b18
    char        pad6[0x13b20 - 0x13b1c];
    int         f13b20;                             // +0x13b20 (clock low)
    int         f13b24;                             // +0x13b24 (clock high)
    char        pad7[0xc58];
    mesh        f1480;                              // +0x1480 (0x260)
    char        pad8[0x158c - 0x16e0 + 0x10000];
    int         f158c;                              // +0x158c (font mesh visible)

    tstring string_value(int index);                // 0x48e9e0 (ret 4)
    mesh* make_label(EntityFull* e);                // 0x495b20 (ret 4)

    void spawn_battle_icon(unsigned id);                                    // 0x49bbb0
};
extern Mission* g_mission;                          // 0x5e2930

// the UI root object (global 0x5d7080, the same object the message log hangs off): the font mesh at +0x1480 and its "on" flag
struct HudRoot {
    char pad0[0x132c];
    int font_on;                                    // +0x132c (font mesh active)
    char pad1330[0x1480 - 0x1330];
    rgl::mesh font;                                 // +0x1480 (0x260 bytes; its mask at +0x158c is the visible flag)
    void set_font_mesh(const tstring* text, int arg);   // 0x49b630
};

// @ 0x49b630
void HudRoot::set_font_mesh(const tstring* text, int arg)
{
    tstring name("ui_font");
    font.res = (skin*)g_resmgr.get_material(name);
    name.~tstring();
    font.clear_geometry();
    if (text->len <= 0) {
        font.mask = 0;
        font_on = 0;
        return;
    }
    float   out[2];
    vector4 pos;
    float   scale[2];
    float   xmin = -0.35f;
    float   xmax = 0.35f;
    arg = arg;
    pos.x = -0.35f; pos.y = 0.0f; pos.z = 0.0f; pos.w = 0.0f;
    scale[0] = 0.017f; scale[1] = 0.017f;
    g_font->layout_text(out, text, &pos, scale, &font, (int)&arg, &xmin, &xmax, 0x100);
    font.mask = 1;
    font_on = 1;
}

// @ 0x49bbb0
void Mission::spawn_battle_icon(unsigned id)
{
    Rec98* rec = (Rec98*)((id & 0xf) * 0x98 + *(f0e0.begin() + (id >> 4)));
    EntityFull* e = g_entity_host->create_entity(9);
    rec->f50 = (int)e;

    Shape* shape;
    void* mem = operator new(0x44);
    if (mem != 0) {
        float radius = 0.3f;
        vector4 zero;
        zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
        shape = ((SphereMem*)mem)->construct(zero, &radius);
    } else
        shape = 0;
    e->shape = shape;

    {
        matrix tmp;
        e->frame = *identity_pos(&tmp);
    }
    rec->bind_host();

    if (rec->f10 == 1) {
        agents.get(rec->f54)->icon_type = 7;
        agents.get(rec->f58)->icon_type = 7;
        agents.get(rec->f54)->pmid = id;
        agents.get(rec->f58)->pmid = id;
    }

    if (rec->f10 == 1) {
        {
            tstring nm("battle_icon");
            e->add_mesh(g_resmgr.get_object(&nm)->clone());
        }
        {
            float angle = 1.5707963f;
            ((RotFrame*)((char*)e->f11c[0] + 0xcc))->rotate_sf(&angle);
        }

        mesh* label = make_label(e);

        {
            float h = 1.0f;
            tstring kn((agents.get(rec->f58)->flags & 0x400) ? "ui_siege" : "ui_battle");
            ((GameData*)this)->label_text(label, g_strtable.lookup(kn), 1, 1, &h);   // 0x49baa0
        }

        e->frame.o = vector4(rec->f24, rec->f28, rec->f3c + 0.4f);
    }
    e->f0c = id;
    e->f168 = 3;
}

} // namespace c49b630

// ======================================================================================
// slice s0049bea0 (namespace c49bea0)
// ======================================================================================
namespace c49bea0 {
// Slice s0049bea0: 0x49bea0 (order arrow creation) and 0x49c170 (reinforcement slot check).
// 0x49bea0: look an agent/order slot up in the mission's agent table, create a scene object
//   (type 9) for it, give it a 0.3-radius sphere shape and an identity frame, refresh the order's
//   text entries and then attach the "ui_..." arrow label through 0x49baa0.
// 0x49c170: validate a reinforcement packet id against the current set and apply its flag bits
//   (reset the unit's copied frame, clear two counters, run 0x4677a0, then set the team slot flags).
// Names are provisional; every offset comes from these two functions and the neighbouring mission slices.

#define MB_NOTHROW throw()

struct StrTable {                                   // 0x5e2a18
    tstring* lookup(const tstring& key);                    // 0x43a1e0
    char* find(const tstring* key);                         // 0x4c6010
};
extern StrTable g_strtable;                         // 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };                    // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                             // 0x5d7080

struct vector4 { float x, y, z, w; };
struct matrix { float m[4][4]; };
matrix* identity_pos(matrix* out);                  // 0x40cd00 (cdecl; returns out)
void* operator new(unsigned size);                  // 0x53545e

// sprite (0x6c bytes, reference counted at +0x64); a "+0x2c slot" object also ref-counted
struct Sprite { char pad[0x64]; int refs; };
struct RefSlot { char pad[0x2c]; Sprite* sprite; };

// 0x44-byte sphere shape (vtable 0x54a4f0), built by 0x4332a0
struct Shape {
    virtual ~Shape();                               // 0
    virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual float radius() const;                   // 5 (+0x14)
    virtual void set_radius(const float* r);        // 6 (+0x18)
    char pad4[0x18];
    Sprite* own;                                    // +0x1c
    matrix m20;                                     // +0x20
    int m40;                                        // +0x40 (the float* arg, copied)
};
Shape* sphere_shape(Shape* mem, vector4 pos, const float* radius);  // 0x4332a0 (ret 0x14, ECX=mem)

// scene object for one order entry (the mission order contour)
struct OrderObj {
    virtual ~OrderObj();                            // 0
    virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual float param();                          // 5 (+0x14)
    virtual void set_param(const float* x);         // 6 (+0x18)
    int pad4, pad8;
    int mode;                                       // +0xc
    char* key;                                      // +0x10
    char flag14;                                    // +0x14
    char pad15[3];
    void* text;                                     // +0x18
    char pad1c[0x58 - 0x1c];
    matrix frame;                                   // +0x58 (4x4 frame)
    Shape* shape;                                   // +0x98
    char pad9c[0xa0 - 0x9c];
    unsigned flags;                                 // +0xa0
    int f_a4;                                       // +0xa4
    char pada8[0x168 - 0xa8];
    int fade;                                       // +0x168
    void refresh();                                 // 0x4b7840
};

struct OrderEntry {                                 // 0x5ac8-byte order/agent record
    char pad0[0xc];
    char* name;                                     // +0xc
    char flag1c;                                    // +0x1c
    char pad1d[3];
    tstring text;                                   // +0x20
    char pad30[0xa0 - 0x30];
    unsigned flags;                                 // +0xa0 (0x400 normal, 0x80000 hidden, 0x3000 style)
    int f_a4;                                       // +0xa4 (vertical offset)
    char pada8[0x59c0 - 0xa8];
    OrderObj* obj;                                  // +0x59c0
    char pad59c4[0x59e8 - 0x59c4];
    Sprite* label;                                  // +0x59e8 (text mesh)
    void finalize();                                // 0x4b2880
};

struct Reinf {                                      // 0x4c-byte reinforcement record
    int idx;                                        // +0 (unit index)
    int flags;                                      // +4
    char pad8[0x4c - 8];
};
struct ReinfSet {                                   // 0x58-byte reinforcement packet
    char pad0[0x34]; Reinf* recs;                   // +0x34
    int count;                                      // +0x38
    char pad3c[0x58 - 0x3c];
};
extern ReinfSet* g_reinf;                           // 0x5e2a00

struct Unit {                                       // 0x130-byte leader/unit record inside Mission
    char pad0[0x6b0];
    char m6b0[0x40];                                // +0x6b0 (frame copy of +0x6f0)
    char m6f0[0x40];                                // +0x6f0
    int f770;                                       // +0x770
    int f774;                                       // +0x774
    int pad778;
};

// the mission's agent table (member of the object 0x49bea0 is called on)
struct AgentTable {
    struct Pair { int id; int slot; } direct[0x2717];   // +0 (id -> slot, direct-mapped)
    char pad138b8[0x138d0 - 0x138b8];
    char** pages;                                   // +0x138d0 (page table of records)
    int index_of(int id);                           // 0x48ad00 (ret 4)
};

struct MissionAgents {
    char pad0[0xf8];
    AgentTable table;                               // +0xf8
    void add_order_arrow(int id, int mode);         // 0x49bea0 (ret 4)
};

struct OrderObjHost {
    virtual OrderObj* create_entity(int type);       // 0x428440 (ret 4)
};
extern void* g_obj_host;                            // 0xa732b4 (pointer to the host object)

struct Mission {
    char pad0[0x527c];
    char orders[8 * 0x16c];                         // +0x527c (order groups, 0x16c each)
    char pad5df0[0x5df0 - 0x527c - 8 * 0x16c];
    int cur_set;                                    // +0x5df0 (index into g_reinf)
    char pad5df4[0x6214 - 0x5df4];
    int team_b;                                     // +0x6214
    int team_a;                                     // +0x6218
    char pad621c[0x6668 - 0x621c];
    char slot_a;                                    // +0x6668
    char slot_b;                                    // +0x6669
    char pad666a[2];
    Unit units[1];                                  // +0x666c (0x130 each)

    void reset_group(int a, int b);                 // 0x4677a0 (ret 8)
    int add_item(Sprite* label, const char* name, int a, int b, const float* h);   // 0x49baa0 (ret 0x14)
    Sprite* make_label(OrderObj* o);                // 0x495b20 (ret 4)
    void check_slot(int id, unsigned bits);         // 0x49c170 (ret 8)
};
extern Mission* g_mission;                          // 0x5e2930
extern char g_ui_mode;                              // 0xa181b8
struct ResMgr { Sprite* material(const char* name); };  // 0x418a70
extern ResMgr g_resmgr;                             // 0x5d6fa0

// 0xa732b4 holds a pointer; its object creates the scene object for orders.
static OrderObj* order_create_entity(int type)
{
    return ((OrderObjHost*)*(void**)0xa732b4)->create_entity(type);
}

// @ 0x49bea0
void MissionAgents::add_order_arrow(int id, int mode)
{
    vector4 zero = { 0, 0, 0, 0 };
    float radius = 0.3f;
    float h;
    const char* name;
    int idx = table.index_of(id);
    OrderEntry* e = (OrderEntry*)(table.pages[idx >> 4] + (idx & 0xf) * 0x5ac8);
    OrderObj* o = order_create_entity(9);
    Shape* shape = (Shape*)operator new(0x44);
    if (shape != 0)
        shape = sphere_shape(shape, zero, &radius);
    o->shape = shape;
    e->obj = o;
    o->frame = *identity_pos(&o->frame);
    e->obj->refresh();
    e->label = g_mission->make_label(e->obj);
    h = (float)e->f_a4;
    if (e->flags & 0x80000) {
        name = "";
        g_mission->add_item(e->label, name, 0, 1, &h);
    } else if (e->flags & 0x400) {
        if ((e->flags & 0x3000) == 0)
            h = 0.5f;
        else if ((e->flags & 0x3000) == 0x1000)
            h = 0.75f;
        else
            h = 1.0f;
        if (g_ui_mode)
            name = (const char*)&e->text;
        else if (e->flag1c == 0 && (name = g_strtable.find(&e->text)) != 0)
            ;
        else
            name = e->name;
        g_mission->add_item(e->label, name, 1, (int)h, &h);
        ((RefSlot*)e->label)->sprite = g_resmgr.material("map_legend_outline");
    } else {
        if ((e->flags & 0x3000) == 0)
            h = 0.5f;
        else if ((e->flags & 0x3000) == 0x1000)
            h = 0.75f;
        else
            h = 1.0f;
        if (g_ui_mode)
            name = (const char*)&e->text;
        else if (e->flag1c == 0 && (name = g_strtable.find(&e->text)) != 0)
            ;
        else
            name = e->name;
        g_mission->add_item(e->label, name, 0, (int)h, &h);
    }
    o->mode = mode;
    o->fade = 1;
    if (!(o->param() > 0.0f))
        o->set_param(&h);
    e->finalize();
}

// @ 0x49c170
void Mission::check_slot(int id, unsigned bits)
{
    if (id >= 0 && id < g_reinf[cur_set].count) {
        Reinf* r = &g_reinf[cur_set].recs[id];
        int eax_, ebx_;

        memcpy((char*)this + r->idx * 0x130 + 0x6b0, (char*)this + r->idx * 0x130 + 0x6f0, 0x40);
        *(int*)((char*)this + r->idx * 0x130 + 0x770) = 0;     // r->idx is re-read: the stores may alias it
        *(int*)((char*)this + r->idx * 0x130 + 0x774) = 0;
        reset_group(id, bits);
        if (r->flags & 1) { slot_b = 1; return; }
        if (r->flags & 2) { slot_a = 1; return; }
        int a = r->flags & 0x40;
        if (a == 0 && (r->flags & 0x80) == 0) { slot_a = 1; return; }
        if (a != 0 && team_b == 0) { slot_a = 1; return; }
        int b = r->flags & 0x80;
        if (b != 0 && team_b == 1) { slot_a = 1; return; }
        if (a != 0 && team_b == 1) { slot_b = 1; return; }
        if (b != 0 && team_b == 0) { slot_b = 1; return; }
        if (a != 0 && team_a == 1) { slot_a = 1; return; }
        if (b != 0 && team_a == 0) { slot_a = 1; return; }
        if (a != 0 && team_a == 0) { slot_b = 1; return; }
        if (b != 0 && team_a == 1) slot_b = 1;
    } else {
        g_msglog.add(tstring("Invalid entry no for add_reinforcements"), -1);
    }
}

} // namespace c49bea0

// ======================================================================================
// slice s0049c2c0 (namespace c49c2c0)
// ======================================================================================
namespace c49c2c0 {
// Slice s0049c2c0: Mission::order (0x49c2c0, ~2.4 KB, `ret 8`). One `case` per army order: each of
// the three order slots belonging to the selected order leader is armed with a per-order state,
// the order's localised caption is fetched from the string table, and (only for the local agent's
// own order group) the caption plus the leader's order text is pushed to the message log. Finally
// every agent in the same order group gets its three order-parameter fields re-seeded from the
// global order table. `index` selects a leader (0..2), `order` the order (0..0xd).
// Names, and the two frame slots the compiler reads before writing on the advance/fall-back paths,
// are provisional.

#define MB_NOTHROW throw()

struct StrTable { tstring* lookup(const tstring& key); };   // 0x43a1e0 (lookup)
extern StrTable g_strtable;                         // 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };        // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                             // 0x5d7080

struct vector4 { float x, y, z, w; };

// an order slot inside a leader record (0x6c bytes)
struct SubOrder {
    int     state;          // +0x00  armed flag (written by the hold/follow/charge/advance orders)
    char    pad04[0xc];
    vector4 target;         // +0x10  move target
    vector4 extra;          // +0x20  (zeroed, w = order group)
    int     face;           // +0x30  nearest face
    float   ox, oy, oz;     // +0x34  per-slot offset
    int     mode;           // +0x44  sub-order mode (0 = unused)
    char    pad48[0x18];
    int     field5c;        // +0x5c
};

struct Leader {             // this + 0x527c + index*0x16c
    int      pad0;
    int      agent_id;      // +4
    SubOrder sub[3];        // +8, stride 0x6c
    char     pad14c[0x20];
};

struct AgentTable {         // mission + 4
    unsigned count;                         // +0
    unsigned first_used();                  // 0x43b1b0
    void     next_used(unsigned* id);       // 0x43b160
};

struct Agent {
    char    pad0[0x50];
    vector4 pos;            // +0x50
    char    pad60[0x1c4 - 0x60];
    int     slot_no;        // +0x1c4  order group (clamped to 7)
    char    pad1c8[0x2b0 - 0x1c8];
    int     unit;           // +0x2b0  unit/order-table index
    int     form_a;         // +0x2b8
    int     form_b;         // +0x2bc
    float   form_delay;     // +0x2c0
    char    pad2c4[0x56b4 - 0x2c4];
    vector4 alt;            // +0x56b4
    char    pad56c4[0x5928 - 0x56c4];
    int     nav_face();     // 0x43cb30 (thiscall) -> eax
};

struct Level { int nearest_face(vector4* p); };     // 0x439a50 (thiscall) -> face index

struct Mission {
    Level*      level;                      // +0x00
    AgentTable  agents;                     // +4
    char        pad8[0x1c - 8];
    void**      agent_pages;                // +0x1c  page table, dir[id >> 4] then + (id&0xf)*0x5928
    char        pad20[0x5c - 0x20];
    int         local_id;                   // +0x5c  local agent id (-1 = none)
    char        pad60[0x527c - 0x60];
    Leader      leaders[3];                 // +0x527c
    char        pad6c20[0x40];

    int  bounds_test(vector4* p);           // 0x48a990 (thiscall) -> al
    void set_order(int index, int on);      // 0x492230 (ret 8)
    void set_fire(int index, int mode);     // 0x492350 (ret 8)
    void order(int index, int order);
};
extern Mission* g_mission;                  // 0x5e292c

// order-leader helpers (all thiscall on the Leader record)
void leader_set_targets(Leader* L, vector4* pos, vector4* a, vector4* b);   // 0x4898e0
void leader_set_mode(Leader* L, int v);                                     // 0x489860
void leader_set_mode2(Leader* L, int v);                                    // 0x4899e0
void order_text(Leader* L, char* buf);                                      // 0x48d940

struct TickPair { int a; int b; };
extern TickPair g_ticks[];                  // 0x5e2948
extern "C" int __cdecl sprintf(char*, const char*, ...);   // 0x536f8f
extern "C" int __cdecl rand(void);                         // 0x536a9f

static Agent* agent_by_id(Mission* m, int id) {
    void** dir = m->agent_pages;
    return (Agent*)((char*)dir[id >> 4] + (id & 0xf) * 0x5928);
}

// look a localisation key up in the mission string table; the held buffer, or "" when absent
static const char* order_string(const char* key) {
    tstring tmp(key);
    tstring* r = g_strtable.lookup(tmp);
    return r->buf ? r->buf : (const char*)&r->buf;
}

// @ 0x49c2c0
void Mission::order(int index, int order) {
    char ordstr[0x200];                 // this+0x38: the localised order caption
    char ordtxt[0x200];                 // this+0x238: the leader's order text
    char msg[0x200];                    // this+0x438: the composed log line
    Leader* L = &leaders[index];

    ordstr[0] = 0;

    switch (order) {
    case 0: {                                   // hold this position
        if (L->agent_id == -1)
            return;
        Agent* a = agent_by_id(this, L->agent_id);
        vector4 zero = { 0, 0, 0, 0 };
        int f = a->nav_face();
        (void)f;
        leader_set_targets(L, &a->pos, &zero, &zero);
        if (L->sub[0].mode) L->sub[0].state = 1;
        if (L->sub[1].mode) L->sub[1].state = 1;
        if (L->sub[2].mode) L->sub[2].state = 1;
        strcpy(ordstr, order_string("ui_hold_this_position_e_"));
        break;
    }
    case 1: {                                   // follow me
        if (L->agent_id == -1)
            return;
        if (L->sub[0].mode) L->sub[0].state = 2;
        if (L->sub[1].mode) L->sub[1].state = 2;
        if (L->sub[2].mode) L->sub[2].state = 2;
        Agent* a = agent_by_id(this, L->agent_id);
        vector4 zero = { 0, 0, 0, 0 };
        int f = a->nav_face();
        (void)f;
        leader_set_targets(L, &a->pos, &a->alt, &zero);
        strcpy(ordstr, order_string("ui_follow_me_e_"));
        break;
    }
    case 2: {                                   // charge
        if (L->sub[0].mode) L->sub[0].state = 0;
        if (L->sub[1].mode) L->sub[1].state = 0;
        if (L->sub[2].mode) L->sub[2].state = 0;
        strcpy(ordstr, order_string("ui_charge_e_"));
        break;
    }
    case 11: {                                  // stand ground
        if (L->sub[0].mode) L->sub[0].state = 3;
        if (L->sub[1].mode) L->sub[1].state = 3;
        if (L->sub[2].mode) L->sub[2].state = 3;
        strcpy(ordstr, order_string("ui_stand_ground_e_"));
        break;
    }
    case 3: {                                   // mount horses
        leader_set_mode(L, 1);
        strcpy(ordstr, order_string("ui_mount_horses_e_"));
        break;
    }
    case 4: {                                   // dismount
        leader_set_mode(L, 2);
        strcpy(ordstr, order_string("ui_dismount_e_"));
        break;
    }
    case 7: {                                   // stand closer
        leader_set_mode2(L, -1);
        strcpy(ordstr, order_string("ui_stand_closer_e_"));
        break;
    }
    case 8: {                                   // spread out
        leader_set_mode2(L, 1);
        strcpy(ordstr, order_string("ui_spread_out_e_"));
        break;
    }
    case 5: {                                   // advance
        for (int k = 0; k < 3; ++k) {
            SubOrder* s = &L->sub[k];
            if (s->mode == 0)
                continue;
            if (s->state == 0)
                continue;
            vector4 p;
            p.x = s->ox * 7.0f + s->target.x;
            p.y = s->oy * 7.0f + s->target.y;
            p.z = s->oz * 7.0f + s->target.z;
            p.w = 0.0f;
            if (!bounds_test(&p))
                continue;
            s->target = p;
            s->target.w = 0.0f;
            s->face = level->nearest_face(&p);
            s->extra.x = 0.0f;
            s->extra.y = 0.0f;
            s->extra.z = 0.0f;
            s->extra.w = 0.0f;
            s->state = 1;
        }
        strcpy(ordstr, order_string("ui_advance_e_"));
        break;
    }
    case 6: {                                   // fall back
        for (int k = 0; k < 3; ++k) {
            SubOrder* s = &L->sub[k];
            if (s->mode == 0)
                continue;
            if (s->state == 0) {
                s->target.x = s->extra.x;
                s->target.y = s->extra.y;
                s->target.z = s->extra.z;
                s->target.w = s->extra.w;
            }
            vector4 p;
            p.x = s->target.x - s->ox * 7.0f;
            p.y = s->target.y - s->oy * 7.0f;
            p.z = s->target.z - s->oz * 7.0f;
            p.w = 0.0f;
            if (!bounds_test(&p))
                continue;
            s->target = p;
            s->target.w = 0.0f;
            s->face = level->nearest_face(&p);
            s->extra.x = 0.0f;
            s->extra.y = 0.0f;
            s->extra.z = 0.0f;
            s->extra.w = 0.0f;
            s->state = 1;
        }
        strcpy(ordstr, order_string("ui_fall_back_e_"));
        break;
    }
    case 9: {                                   // use blunt weapons
        set_order(index, 1);
        strcpy(ordstr, order_string("ui_use_blunt_weapons_e_"));
        break;
    }
    case 10: {                                  // use any weapon
        set_order(index, 0);
        strcpy(ordstr, order_string("ui_use_any_weapon_e_"));
        break;
    }
    case 12: {                                  // hold fire
        set_fire(index, 1);
        strcpy(ordstr, order_string("ui_hold_fire_e_"));
        break;
    }
    case 13: {                                  // fire at will
        set_fire(index, 0);
        strcpy(ordstr, order_string("ui_fire_at_will_e_"));
        break;
    }
    default:
        return;
    }

    // inform the local agent, but only about its own order group
    if (local_id >= 0) {
        Agent* la = agent_by_id(this, local_id);
        int g = la->slot_no;
        if (g < 0 || g >= 8)
            g = 7;
        if (g == index) {
            order_text(L, ordtxt);
            sprintf(msg, ordstr, ordtxt);
            g_msglog.add(tstring(msg), 0xfffffefb);
        }
    }

    // re-seed the order parameters of every agent in this order group
    AgentTable* tbl = &agents;
    unsigned id = tbl->first_used();
    while (id < tbl->count) {
        Agent* a = agent_by_id(this, id);
        int g = a->slot_no;
        if (g < 0 || g >= 8)
            g = 7;
        if (g == index) {
            a->form_a = g_ticks[a->unit].a;
            a->form_b = g_ticks[a->unit].b;
            a->form_delay = (float)(rand() % 0x3dc9) * 6.3223117e-05f + 1.0f;
        }
        tbl->next_used(&id);
    }
}

} // namespace c49c2c0


// ======================================================================================
// slice s0049deb0 (namespace c49deb0)
// ======================================================================================
namespace c49deb0 {
// Slice s0049deb0: the mission "view" object.  It is an engine object (base ctor 0x48ae20, base dtor
// 0x490520, descriptor 0x591c80) with a scene subobject at +0x188, a box frustum at +0x360 (0x120
// bytes) and the per-frame shake/colour/zoom fields that update() drives.  The view's own descriptor
// is 0x5931a8 and its size is 0x4a8.
//
// ctor 0x49deb0, type 0x49df30, mask 0x49df40, move 0x49df60, update 0x49dfe0,
// scalar deleting dtor 0x49e580, render 0x49e5b0.
using rgl::vector4;

// ---- callees that live in other slices -----------------------------------------------------------
void make_identity(float* m);                   // 0x40cd00  out-only 4x4 identity (cdecl)
extern "C" double _floor(double v);             // 0x536b10  lib_crt::_floor

// ---- float / int constants (named by address, matching the game's own tables) ---------------------
float g_tick_scale = 1e-5f;                    // 0x54a6d0  1e-05f  (tick -> seconds)
extern const float g_549cdc;                    // 0x549cdc  1.0f
extern const float g_549cd8;                    // 0x549cd8  4294967296.0f (unsigned->float bias)
extern const float g_549d70;                    // 0x549d70  0.5f
extern const float g_590cd0;                    // 0x590cd0  0.02f

// int globals: the view's z/w scale factors (zero in the image, set at startup by the game).
extern int g_view_z;                            // 0x5d75c8
extern int g_view_w;                            // 0x5d75cc
// tick counters, 8 bytes each; timers index this array through their own key field.
extern __int64 g_ticks[];                       // 0x5e2948
// ---- stub interfaces for the virtual calls -------------------------------------------------------
struct follow_iface {                           // the two follow objects at +0x480
    virtual void  s0(); virtual void  s1(); virtual void  s2(); virtual void  s3();
    virtual void  s4(); virtual void  s5(); virtual void  s6(); virtual void  s7();
    virtual void  s8(); virtual void  s9(); virtual void  s10(); virtual void  s11();
    virtual void  s12(); virtual void  s13(); virtual float s14(); virtual void  s15();
};                                              // s14 = [this+0x38]: the follow weight (0..1)

struct view_iface {                             // `this` seen as its own descriptor (0x5931a8)
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21();
    virtual void s22(void* v);                                          // [this+0x58] shake
    virtual void s23(void* v);                                          // [this+0x5c] follow target
};

struct sub_iface {                              // the target's sub-object (*(target+0x17c))
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23();
    virtual void s24(); virtual void s25(); virtual void s26();
    virtual void s27(void* v);                                          // [this+0x6c] zoom
    virtual void s28(); virtual void s29(); virtual void s30(); virtual void s31();
    virtual void s32(); virtual void s33(); virtual void s34(); virtual void s35();
    virtual void s36();
    virtual void s37(void* v);                                          // [this+0x94] colour reset
    virtual void s38(void* v);                                          // [this+0x98] colour set
    char pad_s[0x14c - 4];
    unsigned colour;                            // +0x14c
};

struct float2 {                                 // a 2-float vector (position/displacement)
    float x, y;
    float2() {}
    float2(float a, float b) : x(a), y(b) {}
};

struct shape_ref {                              // the object the view follows (an entity/agent)
    struct iface {                              // s24 = [this+0x60]: "position changed"
        virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
        virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
        virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
        virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
        virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
        virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23();
        virtual void s24();
    };
    char pad0[0xe0];
    float2 pos;                                 // +0xe0  world position, written by move()
    float2 disp;                                // +0xe8  displacement for this frame
    float sa[2];                                // +0xf0  shake target (near)
    float sb[2];                                // +0xf8  shake base (far)
    char pad100[0x17c - 0x100];
    sub_iface** sub;                            // +0x17c  pointer to the sub-object pointer
};

// ---- the sub-objects -----------------------------------------------------------------------------
struct Scene {                                  // the scene subobject, 0x1d8 bytes at +0x188
    int   viewport[4];                          // +0x000  x, y, w, h (pixels)
    int   f10;                                  // +0x010
    float f14;                                  // +0x014
    char  pad18[0x18c - 0x18];
    int   flags;                                // +0x18c
    char  tail[0x1d8 - 0x190];
    void ctor();                                // 0x429740
    void dtor();                                // 0x41f4e0
    void reset();                               // 0x428f30
};

struct Frustum {                                // the box frustum, 0x120 bytes at +0x360
    unsigned char first;                        // +0x000
    char  pad1[0x108 - 0x1];
    float box[6];                               // +0x108  half extents + the far plane
    void ctor();                                // 0x41ef60
    void build();                               // 0x41e4f0  recompute the 6 planes
    // the box setter (inline): left, right, bottom, top, near, far
    void set_box(float l, float r, float b, float t, float n, float f)
    {
        first = 0;
        box[0] = l;
        box[1] = r;
        box[3] = b;
        box[2] = t;
        box[4] = n;
        box[5] = f;
    }
};

struct Timer {                                  // one of six 0x10-byte timers
    int     key;                                // +0x00  index into g_ticks
    int     pad;                                // +0x04
    __int64 stamp;                              // +0x08  value of g_ticks[key] when started
};

// ---- the engine object base (0x48ae20) ------------------------------------------------------------
struct Object {
    void*   vptr;                               // +0x000  descriptor
    int     f004;                               // +0x004
    Timer   timer[6];                           // +0x008 .. +0x068
    float   lim[6];                             // +0x068  per-timer fade length (-1 = idle)
    float   w40, w41;                           // +0x080  timer 5 blend weights (near)
    float   w20, w21;                           // +0x088  timer 4 blend weights (near)
    unsigned char lv[8];                        // +0x090  colour levels (rendered)
    char    pad098[0x0a0 - 0x098];
    float   w42, w43;                           // +0x0a0  timer 5 blend weights (far)
    float   w22, w23;                           // +0x0a8  timer 4 blend weights (far)
    unsigned char le[8];                        // +0x0b0  colour levels (target)
    char    pad0b8[0x0c0 - 0x0b8];
    unsigned char flag0, flag1, f_c2, f_c3, flag2, flag3;   // +0x0c0
    char    pad0c6[0x0c8 - 0x0c6];
    int     f0c8, f0cc, f0d0, f0d4;             // +0x0c8
    float   f_d8, f_dc;                         // +0x0d8  zoom levels written by update()
    float   px, py;                             // +0x0e0  view position
    float   f_e8, f_ec;                         // +0x0e8
    float   w, h;                               // +0x0f0  view box size
    float   f_f8, f_fc;                         // +0x0f8
    int     f100;                               // +0x100
    unsigned flags;                             // +0x104  drawing flags tested by mask()
    int     arr108[5];                          // +0x108
    unsigned char b11c, b11d, b11e, b11f, b120; // +0x11c
    unsigned char f121, f122;                   // +0x121
    char    pad123[0x124 - 0x123];
    int     f124;                               // +0x124
    char    tstr[0x10];                         // +0x128  an engine string
    float   f138;                               // +0x138
    int     f13c;                               // +0x13c
    float   f140, f144;                         // +0x140
    int     f148;                               // +0x148
    unsigned colour;                            // +0x14c  view colour (AARRGGBB)
    unsigned colour2;                           // +0x150
    int     f154;                               // +0x154
    int     f158, f15c, f160;                   // +0x158
    int     f164;                               // +0x164
    int     f168, f16c;                         // +0x168
    float   f170;                               // +0x170
    int     f174, f178;                         // +0x174
    int     sub0, sub1, sub2;                   // +0x17c

    void base_ctor();                           // 0x48ae20
    void base_dtor();                           // 0x490520
    void reset_timer_a();                       // 0x48d470
    void reset_timer_b(void* p);                // 0x48f120  (p = &w20)
    void reset_timer_c(unsigned v);             // 0x48d350
    void reset_timer_d(unsigned v);             // 0x48d3c0
    void scene_render(void* scene, float* pos, void* c, void* d);   // 0x490710
};

// ---- the view itself -----------------------------------------------------------------------------
struct Camera : Object {
    Scene   scene;                              // +0x188
    Frustum fr;                                 // +0x360
    follow_iface* follow[2];                    // +0x480  the two follow objects
    void*   follow_shape[2];                    // +0x488  their shapes
    shape_ref* target;                          // +0x490  the object the view tracks
    float   shake_x;                            // +0x494
    float   shake_y;                            // +0x498
    float2  tail;                               // +0x49c
    int     f4a4;                               // +0x4a4

    Camera();                                   // 0x49deb0
    int  type();                                // 0x49df30
    unsigned mask(int a);                       // 0x49df40
    void move(shape_ref* e);                    // 0x49df60
    void update(int unused);                    // 0x49dfe0
    void* destroy(unsigned flags);              // 0x49e580
    void render(int a, float* pos, void* c, void* d);   // 0x49e5b0
};

// Blend one colour level from `a` (target) to `b` (rendered) over a fade of length `lim` that has run for
// `d`, rounded half-up.  The first colour byte is read as a masked dword (unsigned), the others as bytes.
static __forceinline int blend_u(const float& lim, const float& d, unsigned a, unsigned b)
{
    float v = ((float)a * (lim - d) + (float)b * d) * (1.0f / lim);
    float fl = (float)floor(v);
    int i = (int)fl;
    if (!(v - fl <= 0.5f))
        i++;
    return i;
}
static __forceinline int blend_i(const float& lim, const float& d, int a, int b)
{
    float v = ((lim - d) * (float)a + (float)b * d) * (1.0f / lim);
    float fl = (float)floor(v);
    int i = (int)fl;
    if (!(v - fl <= 0.5f))
        i++;
    return i;
}

// @ 0x49deb0
Camera::Camera()
{
    base_ctor();
    sub0 = 0;
    sub1 = 0;
    sub2 = 0;
    vptr = (void*)0x5931a8;                     // the view's own class descriptor
    scene.ctor();
    fr.ctor();
    shake_x = 0;
    shake_y = 0;
    follow[0] = 0;
    follow[1] = 0;
    target = 0;
    tail = float2(0, 0);
}

// @ 0x49df30
int Camera::type()
{
    return 7;
}

// @ 0x49df40
unsigned Camera::mask(int a)
{
    if (a == 0)
        return flags & 0x10;
    return flags & 0x20;
}

// @ 0x49df60
// Place the followed object at the view position (plus its own displacement), undo the current shake
// if it is the tracked object, then tell it it moved.
void Camera::move(shape_ref* e)
{
    e->pos = float2(e->disp.x + px, e->disp.y + py);
    if (e == target) {
        e->pos.x = e->pos.x - shake_x;
        e->pos.y = e->pos.y - shake_y;
    }
    ((shape_ref::iface*)e)->s24();
}

// @ 0x49dfe0
// Per-frame view update: the two shake axes follow the tracked object while its follow objects report
// a weight, then the four fades (zoom, colour, level, shake) are advanced.
void Camera::update(int)
{
    float2 v;

    shake_x = target->sb[0];
    shake_y = target->sb[1];
    for (int i = 2; i != 0; i--) {
        int k = 2 - i;
        if (follow[k] && follow_shape[k]) {
            float r = follow[k]->s14();
            (&shake_x)[k] = (target->sa[k] - target->sb[k] - (&w)[k]) * r + target->sb[k];
        }
    }
    ((view_iface*)this)->s23(target);

    float d0 = (float)(g_ticks[timer[0].key] - timer[0].stamp) * g_tick_scale;    // colour
    float d1 = (float)(g_ticks[timer[1].key] - timer[1].stamp) * g_tick_scale;    // level
    float d4 = (float)(g_ticks[timer[4].key] - timer[4].stamp) * g_tick_scale;    // zoom
    float d5 = (float)(g_ticks[timer[5].key] - timer[5].stamp) * g_tick_scale;    // shake

    // zoom: blend the target's zoom from the near weights toward the far ones
    if (d4 < lim[4]) {
        float inv = 1.0f / lim[4];
        float rest = lim[4] - d4;
        float xn = d4 * w20 + rest * w22;
        float yn = rest * w23 + d4 * w21;
        float x = xn * inv;
        float y = yn * inv;
        v.x = x * 0.02f;
        v.y = y * 0.02f;
        (*target->sub)->s27(&v);
        f_d8 = x;
        f_dc = y;
        reset_timer_a();
    } else if (flag2) {
        flag2 = 0;
        reset_timer_b(&w20);
    }

    // colour: the three level bytes blended from the target levels and packed under the inherited
    // alpha byte, then the target sub-object's colour slot
    if (d0 < lim[0]) {
        int i0 = blend_u(lim[0], d0, *(unsigned*)le & 0xff, *(unsigned*)lv & 0xff);
        int i1 = blend_i(lim[0], d0, le[1], lv[1]);
        int i2 = blend_i(lim[0], d0, le[2], lv[2]);
        unsigned col = (colour | (*target->sub)->colour) & 0xff000000u;
        col |= (unsigned)((i2 << 16) + (i1 << 8) + i0);
        (*target->sub)->s38(&col);
        (*target->sub)->s37(&v);
    } else if (flag0) {
        flag0 = 0;
        reset_timer_c(colour ^ ((colour ^ *(unsigned*)lv) & 0xffffff));
    }

    // level
    if (d1 < lim[1]) {
        reset_timer_d(blend_i(lim[1], d1, le[7], lv[7]));
    } else if (flag1) {
        flag1 = 0;
        reset_timer_d(lv[7]);
    }

    // shake: blend toward the far weights and hand the offset to the object
    if (d5 < lim[5]) {
        float inv = g_549cdc / lim[5];
        float rest = lim[5] - d5;
        v.x = (d5 * w40 + rest * w42) * inv;
        v.y = (d5 * w41 + rest * w43) * inv;
        ((view_iface*)this)->s22(&v);
        reset_timer_a();
    } else if (flag3) {
        ((view_iface*)this)->s22(&w40);
        reset_timer_a();
        flag3 = 0;
    }
}

// @ 0x49e580  (scalar deleting destructor)
void* Camera::destroy(unsigned flags)
{
    scene.dtor();
    base_dtor();
    if (flags & 1)
        ::operator delete(this);
    return this;
}

// @ 0x49e5b0
// Place the view: size the scene viewport from the view box, rebuild the box frustum, install it as
// the scene's volume and the view matrix, then hand the frame to the scene.  The view matrix is the
// identity with its translation row set from a position vector whose w is never initialised (the
// original copies that stack slot into m[15]).
struct V4 { float x, y, z, w; };
void Camera::render(int a, float* pos, void* c, void* d)
{
    scene.reset();
    scene.viewport[0] = (int)((float)g_view_z * px);
    scene.viewport[1] = (int)((0.75f - (py + h)) * (float)g_view_w * 1.3333334f);
    scene.viewport[2] = (int)((float)g_view_z * w);
    scene.viewport[3] = (int)((float)g_view_w * h * 1.3333334f);
    scene.f14 = 1.0f;
    scene.f10 = 0;
    scene.flags |= 0x1000;
    Frustum* f = &fr;
    f->set_box(w * -0.5f, w * 0.5f, h * -0.5f, h * 0.5f, 0, 501.0f);
    f->build();
    float m[16];
    make_identity(m);
    V4 v;
    v.z = 400.0f;
    v.x = w * 0.5f + px;
    v.y = h * 0.5f + py;
    *(V4*)&m[12] = v;
    memcpy((char*)f + 0xc8, m, 0x40);
    f->build();
    memcpy((char*)this + 0x1b8, f, 0x120);
    *pos = *pos + 1.0f;
    scene_render(&scene, pos, c, d);
}

} // namespace c49deb0

// ======================================================================================
// slice s0049e780 (namespace c49e780)
// ======================================================================================
namespace c49e780 {
// Slice s0049e780: view construction (0x49e780) and the "party sprite" wrappers that hang a party's
// character record on the view's scene graph (0x49e9c0..0x49f020).
//
//   0x49e780  View::create(int arg, int flags)            -> index of the new child in the view's list
//   0x49e9c0  Party::attach_sprite(float2*, int, int)     -> sprite record id
//   0x49ebd0  Party::add_sprite(int x, int y, int troop)  -> sprite record id
//   0x49ef00  Party::add_member_sprite(int slot, int x, int y, int a) -> sprite record id
//   0x49f020  Party::add_sprite_at(int x, int y, int troop, int a)
//
// The party object keeps a free-id list at +0xcc, a paged pool of 0x98-byte sprite records at +0xe4 and
// the embedded scene at +0xf8.  The scene holds paged 0x5ac8-byte object records at +0x138d0 (party
// 0x139c8) and the character table at +0x13908 (party 0x13a00, 0x508-byte records).

struct float2 {
    float x, y;
    float2() {}
    float2(float a, float b) { x = a; y = b; }
};


// ---- engine services that live in other slices -----------------------------------------------------
void* __cdecl operator_new(unsigned int n);              // 0x53545e (operator new)
int   __cdecl _rand();                                   // 0x536a9f (CRT)
struct scene_api;                                        // 0x498420 (thiscall scene position, below)

extern float g_rand_scale;                               // 0x54a6e0 (angle per rand() step)
extern void* g_troop_base;                               // 0x5e29f8 (0xb0-byte troop record table)

// The global string table (this = 0x5e2a18) and its lookup (0 when absent).
struct Golist {
    tstring* get(tstring* key);                          // 0x4c6010
};


// The 0xb0-byte troop-type record (the name at +0x10; the field offsets are relative to the record).
struct TroopRec {
    char pad0[0x10];
    tstring name;                                        // +0x10
    int f20;                                             // +0x20
    int f24;                                             // +0x24
    int f28;                                             // +0x28
    int f2c;                                             // +0x2c
    int f30;                                             // +0x30
    int f34;                                             // +0x34
    char pad38[0xb0 - 0x38];
};

// ---- the objects create() builds ---------------------------------------------------------------
// The constructors are thiscall (object in ecx) and return the object: 0x48ae20 the base scene object,
// 0x48b890 its particle-system sibling, 0x49deb0 the view camera (slice s0049deb0).
// Scene-object vtable slots used here: +0x58 set position (float2*), +0x64 set rotation (float2*),
// +0x74 set attribute (int*), +0xa4 (used around the attribute bits, see create), +0xc8 add child.
struct scene_obj {
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03();
    virtual void s04(); virtual void s05(); virtual void s06(); virtual void s07();
    virtual void s08(); virtual void s09(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21();
    virtual void set_pos(void* v);                       // +0x58
    virtual void s23(); virtual void s24();
    virtual void set_rot(void* v);                       // +0x64
    virtual void s26(); virtual void s27(); virtual void s28();
    virtual void set_attr(void* v);                      // +0x74
    virtual void s30(); virtual void s31(); virtual void s32(); virtual void s33();
    virtual void s34(); virtual void s35(); virtual void s36(); virtual void s37();
    virtual void s38(); virtual void s39(); virtual void s40();
    virtual void attach(void* v, int n);                 // +0xa4
    virtual void s42(); virtual void s43(); virtual void s44(); virtual void s45();
    virtual void s46(); virtual void s47(); virtual void s48(); virtual void s49();
    virtual void s50(); virtual void s51(); virtual void s52();
    virtual void add_child(void* v);                     // +0xc8
};
scene_obj* base_ctor(scene_obj* p);                      // 0x48ae20
scene_obj* ps_ctor(scene_obj* p);                        // 0x48b890
scene_obj* cam_ctor(scene_obj* p);                       // 0x49deb0

// 0x498420: a free thiscall position setter (argument in ecx, one pointer argument).
struct scene_api { void set_pos(float2* p); };

// The 0x5ac8-byte object record (only the fields the sprite wrappers touch).
struct ObjRec {
    char pad0[0xc];
    tstring name;                                        // +0xc
    char flag1c;                                         // +0x1c
    char pad1d[0x3];
    tstring name2;                                       // +0x20
    int f30;                                             // +0x30
    int f34;                                             // +0x34
    char pad38[0x8];
    float2 rot;                                          // +0x40
    char pad48[0x70 - 0x48];
    int f70;                                             // +0x70
    int f74;                                             // +0x74
    int id;                                              // +0x78
    char pad7c[0x88 - 0x7c];
    int sprite;                                          // +0x88  owning sprite record id (-1 = none)
    int f8c;                                             // +0x8c
    int f90;                                             // +0x90
    char pad94[0xa0 - 0x94];
    int fa0;                                             // +0xa0
    int fa4;                                             // +0xa4
    char pad_a8[0x8e4 - 0xa8];
    int f8e4;                                            // +0x8e4
    int f8ec;                                            // +0x8ec
    int f8f0;                                            // +0x8f0
    char pad8f0[0x918 - 0x8f0];
    int f918;                                            // +0x918
    char pad91c[0x5ac8 - 0x91c];

    void setup();                                        // 0x48f180 (thiscall, no args)
    void set_record(tstring* n, int a, int b, int c, int d); // 0x48c840
};

// The 0x508-byte character record.
struct CharRec {
    char pad0[0x20];
    tstring name;                                        // +0x20
    char pad30[0x4c - 0x30];
    int f4c;                                             // +0x4c
    char pad50[0x508 - 0x50];
};

// The 0x98-byte sprite record (16 to a page).
struct SpriteRec {
    char pad0[0x10];
    int alive;                                           // +0x10
    char pad14[0x24 - 0x14];
    float2 pos;                                          // +0x24
    char pad2c[0x54 - 0x2c];
    int a;                                               // +0x54
    int b;                                               // +0x58
    char pad5c[0x78 - 0x5c];
    int height;                                          // +0x78 sum of the two queried heights
    char pad7c[0x98 - 0x7c];
};

// The child-id list at party+0xcc.
struct ChildList {
    int pop_front();                                     // 0x493700 (thiscall)
};

// The scene embedded at party+0xf8.
struct Scene {
    char pad0[0x138d0];
    ObjRec** objects;                                    // +0x138d0 (party +0x139c8)
    char pad1[0x13908 - 0x138d4];
    CharRec** chars;                                     // +0x13908 (party +0x13a00)

    int obj_index(int key);                              // 0x48ad00 (thiscall, key -> object id)
    int last();                                          // 0x493960 (thiscall, back object id)
    ObjRec* obj(int key)
    {
        int id = obj_index(key);
        return objects[id >> 4] + (id & 0xf);
    }
};

// The party object the sprite wrappers work on.
struct Party {
    char pad0[0xcc];
    ChildList list;                                      // +0xcc
    char pad1[0xe4 - 0xd0];
    SpriteRec** sprites;                                 // +0xe4 paged sprite records
    char pad2[0xf8 - 0xe8];
    Scene scene;                                         // +0xf8

    CharRec* chars() { return *scene.chars; }
    SpriteRec* sprite(int id) { return sprites[id >> 4] + (id & 0xf); }

    int attach_sprite(float2* pos, int a, int b);        // 0x49e9c0
    int add_sprite(int x, int y, int troop);             // 0x49ebd0
    int add_member_sprite(int slot, int x, int y, int a);  // 0x49ef00
    void add_sprite_at(int x, int y, int troop, int a);   // 0x49f020

    void count_dims(int troop, int* h, int* d, int* w, int* o); // 0x490ce0
    void slot_setup(int id, int troop, int a);           // 0x48fbd0
    void sprite_hook(int id);                            // 0x49bbb0
    void sprite_flag(int id);                            // 0x49bea0
    void pick(int x, int y, int a, int* out);            // 0x490030
};

inline TroopRec* troop_rec(int troop) { return (TroopRec*)((char*)g_troop_base + 0xb0 * troop); }

// The view (`this`) seen from here: the child list object at +0x2c and its own vector at +4/+8.
struct View {
    char pad0[4];
    int* begin;                                          // +4
    int* end;                                            // +8
    char pad_c[0x2c - 0xc];
    scene_obj* list;                                     // +0x2c

    int create(int arg, int flags);                      // 0x49e780
    void push(void** obj);                               // 0x4936b0 (thiscall push_back)
};

// ---- @ 0x49e780 -----------------------------------------------------------------------------------
// Build the scene object for `flags` (attribute bits flags&0x381c | 0x700, scale 0.9 x 0.75) and, for
// the "3d" flag 0x2000, the particle system + camera + group around it.
int View::create(int arg, int flags)
{
    scene_obj* o = (scene_obj*)operator_new(0x178);
    if (o)
        o = base_ctor(o);                                // 0x48ae20
    else
        o = 0;
    float2 p;
    p.x = 0;
    p.y = 0;
    o->set_pos(&p);                                      // +0x58
    int attr = (flags & 0x381c) | 0x700;
    o->set_attr(&attr);                                  // +0x74
    float2 r;
    r.x = 0.9f;
    r.y = 0.75f;
    o->set_rot(&r);                                      // +0x64
    o->attach((void*)arg, *(int*)&r.y);                  // +0xa4 (operand pair of the original)
    if (flags & 0x2000) {
        scene_obj* ps = (scene_obj*)operator_new(0x188);
        if (ps)
            ps = ps_ctor(ps);                            // 0x48b890
        else
            ps = 0;
        *(int*)((char*)ps + 0x178) = 1;
        float2 z;
        z.x = 0;
        z.y = 0;
        ps->set_pos(&z);                                 // +0x58
        this->list->add_child(ps);                       // +0xc8
        scene_obj* cam = (scene_obj*)operator_new(0x4a8);
        if (cam)
            cam = cam_ctor(cam);                         // 0x49deb0
        else
            cam = 0;
        *(int*)((char*)cam + 0x104) |= 0x20;
        *(scene_obj**)((char*)cam + 0x484) = ps;
        scene_obj* grp = (scene_obj*)operator_new(0x188);
        if (grp) {
            grp = base_ctor(grp);                        // 0x48ae20
            *(void**)grp = (void*)0x5924a0;
            *(int*)((char*)grp + 0x17c) = 0;
            *(int*)((char*)grp + 0x180) = 0;
            *(int*)((char*)grp + 0x184) = 0;
        } else {
            grp = 0;
        }
        float2 z2;
        z2.x = 0;
        z2.y = 0;
        grp->set_pos(&z2);                               // +0x58
        grp->set_rot((float2*)((char*)o + 0xf0));        // +0x64
        grp->add_child(o);                               // +0xc8
        *(scene_obj**)((char*)cam + 0x490) = grp;
        cam->add_child(grp);                             // +0xc8
        float2 z3;
        z3.x = 0;
        z3.y = 0;
        cam->set_pos(&z3);                               // +0x58
        float2 cp;
        cp.x = *(float*)((char*)o + 0xf0);
        cp.y = 0.1f;
        ((scene_api*)cam)->set_pos(&cp);                 // 0x498420
        this->list->add_child(cam);                      // +0xc8
        scene_obj* t = cam;
        push((void**)&t);                                // 0x4936b0
    } else {
        this->list->add_child(o);                        // +0xc8
        scene_obj* t = o;
        push((void**)&t);                                // 0x4936b0
    }
    if (this->begin == 0)
        return -1;
    return ((this->end - this->begin) >> 2) - 1;
}

// ---- @ 0x49e9c0 -----------------------------------------------------------------------------------
// Build a sprite record for troop types `a` and `b` at `pos`: the party's free list hands out the
// record id, the record takes the position, both troop ids and the summed heights, and both troop
// objects point at it.
int Party::attach_sprite(float2* pos, int a, int b)
{
    int id = list.pop_front();                           // 0x493700
    sprite(id)->alive = 1;
    sprite(id)->pos = *pos;
    sprite(id)->alive = 1;
    sprite(id)->a = a;
    sprite(id)->b = b;
    sprite(id)->height = 0;
    if (a >= 0) {
        int d[4] = { 0, 0, 0, 0 };
        count_dims(a, &d[0], &d[3], &d[2], &d[1]);       // 0x490ce0
        sprite(id)->height += d[0];
    }
    if (b >= 0) {
        int d[4] = { 0, 0, 0, 0 };
        count_dims(b, &d[0], &d[3], &d[2], &d[1]);       // 0x490ce0
        sprite(id)->height += d[0];
    }
    scene.obj(a)->f918 = 7;
    scene.obj(b)->f918 = 7;
    scene.obj(a)->sprite = id;
    scene.obj(b)->sprite = id;
    sprite_hook(id);                                     // 0x49bbb0
    return id;
}

// ---- @ 0x49ebd0 -----------------------------------------------------------------------------------
// Add a scene object for troop type `troop` at (x, y): the scene's last object record is filled from
// the 0xb0-byte troop record, named after it, randomly rotated, and hooked to a fixture.
int Party::add_sprite(int x, int y, int troop)
{
    TroopRec* tr = troop_rec(troop);                     // 0x5e29f8 + 0xb0 * troop
    int id = scene.last();                               // 0x493960
    scene.obj(id)->setup();                              // 0x48f180
    scene.obj(id)->f8c = troop;
    scene.obj(id)->f30 = x;
    scene.obj(id)->f34 = y;
    scene.obj(id)->rot.x = 1.0f;
    scene.obj(id)->rot.y = 0;
    float ang = (float)(_rand() % 0x3dc9) * g_rand_scale; // 0x536a9f / 0x54a6e0
    float c = (float)cos(ang);
    float s = (float)sin(ang);
    ObjRec* ro = scene.obj(id);
    float oy = ro->rot.y;
    float ox = ro->rot.x;
    ro->rot.y = c * oy + s * ox;
    ro->rot.x = c * ox - s * oy;
    tstring* n = ((Golist*)0x5e2a18)->get(&tr->name);    // 0x4c6010
    if (n == 0)
        n = (tstring*)tr;
    scene.obj(id)->name = *n;
    scene.obj(id)->flag1c = 1;
    scene.obj(id)->name2 = tr->name;
    scene.obj(id)->fa0 = tr->f20;
    scene.obj(id)->fa4 = tr->f24;
    scene.obj(id)->f90 = tr->f28;
    scene.obj(id)->f70 = tr->f2c;
    scene.obj(id)->f74 = tr->f30;
    scene.obj(id)->f8e4 = 2;
    scene.obj(id)->f8ec = x;
    scene.obj(id)->f8f0 = y;
    slot_setup(id, troop, 0);                            // 0x48fbd0
    sprite_flag(id);                                     // 0x49bea0
    tr->f34 += 1;
    return id;
}

// ---- @ 0x49ef00 -----------------------------------------------------------------------------------
// Add the sprite for character record `slot` at (x, y): name it after the character and move it to the
// character's own position field.
int Party::add_member_sprite(int slot, int x, int y, int a)
{
    int id = add_sprite(x, y, 3);                        // 0x49ebd0
    tstring* n = ((Golist*)0x5e2a18)->get(&chars()[slot].name); // 0x4c6010
    if (n == 0)
        n = &chars()[slot].name;
    scene.obj(id)->name = *n;
    scene.obj(id)->flag1c = 1;
    scene.obj(id)->f70 = chars()[slot].f4c;
    scene.obj(id)->set_record(n, 1, 0, 0, 0);            // 0x48c840
    return id;
}

// ---- @ 0x49f020 -----------------------------------------------------------------------------------
// Pick the on-screen position for troop `troop` (x is adjusted in place) and add its sprite there.
void Party::add_sprite_at(int x, int y, int troop, int a)
{
    pick(x, y, a, &x);                                   // 0x490030
    add_sprite(x, y, troop);                             // 0x49ebd0
}

} // namespace c49e780

// ======================================================================================
// slice s0049f060 (namespace c49f060)
// ======================================================================================
namespace c49f060 {
// Slice s0049f060: party bookkeeping of the mission object at 0x5e2930. 0x49f060 decides whether party `a` can
// attach/dismount onto party `b` (and where the join point is); 0x49f3d0 spawns the scene object of a party for a
// party trigger; 0x49f7c0 pops/replaces the party on top of the trigger stack; 0x49f980 and 0x49f9e0 enter a
// trigger block. Module System names: parties, party_tpl, triggers. Names provisional.

// ---- the resource manager (0x5d6fa0; the slice called it the string table) ----
struct ResTable { tstring* find(const tstring* key); };                         // 0x418cf0 (ret 4, null when absent)
extern ResTable g_restable;                                                           // 0x5d6fa0

// ---- the game data ---------------------------------------------------------------------------------------
struct Party;                                                                     // 0x5ac8 bytes
struct PartyMgr {                                                                 // embedded in the game data at +0xf8
    char pad0[0x138cc];
    std::vector<char*> pages;                                                     // +0x138cc (page pointers, 16 parties per page)
    int deflt;                                                                    // +0x138dc (never read here)
    int index_of(int id);                                                         // 0x48ad00 (thiscall, ret 4)
    Party* get(int id)                                                            // 0x51ee50 (thiscall, ret 4): out of line when the caller is too big
    {
        int k = index_of(id);
        return (Party*)((k & 0xf) * 0x5ac8 + *(pages.begin() + (k >> 4)));
    }
    void next_party_id(int* p);                                                   // 0x460ca0 (thiscall, ret 4)
};
struct KindRec {                                                                  // 0x98-byte scene object record
    int kind;                                                                     // +0
    char pad4[0x98 - 4];
};
struct Party {
    char pad0[0x30];
    float x, y, z, w;                                                             // +0x30 (position)
    char pad40[0x54 - 0x40];
    int f54;                                                                      // +0x54
    char pad58[0x78 - 0x58];
    int f78;                                                                      // +0x78 (the mounted/attached party)
    char pad7c[0x88 - 0x7c];
    int f88;                                                                      // +0x88 (scene object index, < 0 = not deployed)
    char pad8c[0x8b4 - 0x8c];
    char f8b4;                                                                    // +0x8b4
    char pad8b5[0x8bc - 0x8b5];
    int* f8bc;                                                                    // +0x8bc (item count table)
    char pad8c0[0x8c8 - 0x8c0];
    int f8c8;                                                                     // +0x8c8
    char f8cc;                                                                    // +0x8cc
    char pad8cd[0x5ac8 - 0x8cd];
};
struct TroopRec {                                                                 // 0x508 bytes
    char pad0[0x30];
    unsigned __int64 flags;                                                       // +0x30 tf_*
    char pad38[0x48 - 0x38];
    int level;                                                                    // +0x48
    char pad4c[0x508 - 0x4c];
};
struct ItemRef {                                                                  // 8 bytes
    int item; union { unsigned m4; struct { unsigned char amount; char pad[2]; signed char mod; } b; };
};
struct V2 {
    float x, y;
    V2(float a, float b) : x(a), y(b) {}
};
struct Receiver {                                                                 // a 0x98-byte scene object record
    char pad0[0x54];
    int f54;                                                                      // +0x54
    int f58;                                                                      // +0x58
    char pad5c[0x98 - 0x5c];
};
struct GameData {
    char pad0[0x78];
    int f78;                                                                      // +0x78 (player party)
    char pad7c[0xe0 - 0x7c];
    std::vector<char*> nodes;                                                     // +0xe0 (scene object pages, 16 records of 0x98 bytes)
    char padf0[0xf8 - 0xf0];
    PartyMgr parties;                                                             // +0xf8
    char pad_to_chars[0x13a00 - 0xf8 - sizeof(PartyMgr)];
    TroopRec* chars;                                                              // +0x13a00
    char pad13a04[0x13a0c - 0x13a04];
    int f13a0c;                                                                   // +0x13a0c
    int f13a10;                                                                   // +0x13a10
    int f13a14;                                                                   // +0x13a14
    char pad13a18[0x13ad0 - 0x13a18];
    float f13ad0;                                                                 // +0x13ad0
    int f13ad4;                                                                   // +0x13ad4
    char pad13ad8[0x14014 - 0x13ad8];
    unsigned char f14014;                                                         // +0x14014 (trigger block object)

    Receiver* node(unsigned idx)                                                  // scene object by index
    {
        return (Receiver*)((idx & 0xf) * 0x98 + *(nodes.begin() + (idx >> 4)));
    }
    int last_leader(int id);                                                      // 0x48fce0 (ret 4)
    float party_relation(int a, int b);                                           // 0x48ffa0 (ret 8)
    void attach_party(int a, int b);                                              // 0x4949d0 (ret 8)
    void attach_party_center(const V2& pt, int a, int b);                         // 0x49e9c0 (ret 0xc)
    bool deploy_party(int a, int b);                                              // 0x49f060 (ret 8)
};
extern GameData* g_game;                                                          // 0x5e2930


// ---- the mission -----------------------------------------------------------------------------------------
struct PartyStack { int id; int val; void reduce(int n); };                       // 8 bytes, mission+0x404
// PartyStack::reduce is 0x437510 (ItemStack::reduce: take `n` off the stack).
struct TriggerStack {                                                             // the party trigger stack
    PartyStack sp[0x80];                                                          // +0 (depth i reads sp[i-1])
    int count;                                                                    // +0x400 (stack depth)
};
struct NextUsedTbl { void next_used(int* p); };                                   // 0x43b160 (thiscall, ret 4)
struct Mission {
    KindRec* table;                                                               // +0x0 (0x98-byte scene object records)
    char pad4[0x5c - 4];
    int local_id;                                                                 // +0x5c
    char pad60[0x400 - 0x60];
    int sp_count;                                                                 // +0x400 (party trigger stack depth)
    PartyStack sp[0x80];                                                          // +0x404
    char pad804[0x13a0c - 0x804];
    int f13a0c;                                                                   // +0x13a0c
    int f13a10;                                                                   // +0x13a10
    int f13a14;                                                                   // +0x13a14
    char pad13a18[0x13a48 - 0x13a18];
    float* harvest;                                                               // +0x13a48 (party x party matrix)
    char pad13a4c[0x13ad0 - 0x13a4c];
    float f13ad0;                                                                 // +0x13ad0
    int f13ad4;                                                                   // +0x13ad4

    Party* first_party(int id);                                                   // 0x48fce0 (this = mission-4; ret 4)
    float party_relation(int a, int b);                                           // 0x48ffa0 (this = mission-4; ret 8)
    void attach_party(int a, int b);                                              // 0x4949d0 (this = mission-4; ret 8)
    void attach_party_center(const float* pt, int a, int b);                      // 0x49e9c0 (this = mission-4; ret 0xc)
    void set_trigger_value(int val, long long* out);                              // 0x49d500 (this = mission-4; ret 8)
    int add_party_to_trigger(int a, int* b, int* c);                              // 0x49d650 (this = mission-4; ret 0xc)
    bool trigger_step(int a, void* t);                                            // 0x49d7f0 (ret 8)
    void enter_trigger(int val, int ip);                                          // 0x49f980 (ret 8)
    void exec_trigger(int val);                                                   // 0x49f9e0 (ret 4)
    // trigger-block builders (thiscall, the block handle is the first argument)
    void block_add(void* block, const void* pos, int one);                        // 0x4836e0 (ret 0xc)
    void block_exec(void* block, int val, int a, int b, int c, int d, int e, int f,
                    int g, char* out);                                            // 0x483870 (ret 0x28)
};
extern Mission* g_mission;                                                        // 0x5e292c

// ---- spawn helpers (other slices) ------------------------------------------------------------------------
struct SceneObj {
    virtual void f0();
    virtual void f4();
    virtual void f8();
    virtual void fc();
    virtual void f10();
    virtual void f14();
    virtual void set_pos(const float* arg);                                       // slot 6
    char pad4[0x58 - 4];
    char m[0x40];                                                                 // +0x58 (frame)
    char pad98[0xc4 - 0x98];
    int f_c4;                                                                     // +0xc4
    int f_c8;                                                                     // +0xc8
    float f_cc;                                                                   // +0xcc
    int f_d0;                                                                     // +0xd0
    char pad_d4[0x304 - 0xd4];
    struct FxDef2* skel;                                                          // +0x304
    char pad308[0x31c - 0x308];
};
struct FxDef2 {                                                                   // 0x7c bytes (created by world::create_entity(2))
    FxDef2(int kind);                                                             // 0x428820 (ret 4)
    char pad0[0x40];
    unsigned char* f40;                                                           // +0x40
    char pad44[0x5c - 0x44];
    int f5c;                                                                      // +0x5c
    char pad60[0x7c - 0x60];
    void set_env_count(int n);                                                    // 0x46b9d0 (ret 4)
    void set_skeleton(void* p);                                                   // 0x4288c0 (ret 4)
    void set_group0(int g, int id, int a, int b);                                 // 0x439fc0 (ret 0x14)
    void link_group(int g);                                                       // 0x4339d0 (ret 4)
    void set_blend(float f, const void* m);                                       // 0x421df0 (ret 8)
};
struct PickArg { char pad[0x400]; };                                              // the v[] argument of 0x482240
struct SiteRec {                                                                  // 0x1dc bytes, one per scene site
    char pad0[0x34];
    float x, y, z, w;                                                             // +0x34
    char pad44[0x1dc - 0x44];
};
struct Group {                                                                    // 0x30-byte records at 0xa76628
    char pad0[0x10];
    int* src;                                                                     // +0x10
    int count;                                                                    // +0x14
    char pad18[0x30 - 0x18];
};
extern Group* g_groups;                                                           // 0xa76628
extern int* g_body_table;                                                         // 0x8ed4f7c (vector<int>, entry ptr at +0x1e8)
extern unsigned g_body_count;                                                     // 0x8ed4f88

// @ 0x49f060
// Decide whether party `a` can be attached to `b`, and where the join point is. When the leader of `b` is not
// deployed yet the two parties join at the midpoint of their positions (the player's party is always the first
// one handed to attach_party_center); otherwise `a` adopts the leader's scene object (the player) or attaches to
// the party standing next to it.
bool GameData::deploy_party(int a, int b)
{
    int lead = last_leader(b);
    if (parties.get(lead)->f88 >= 0) {
        if (a == f78) {
            f13a0c = node(parties.get(lead)->f88)->f58;
            f13a10 = node(parties.get(lead)->f88)->f54;
            f13a14 = 0;
            parties.get(a)->f88 = parties.get(lead)->f88;
            return 1;
        }
        Receiver* nd = node(parties.get(lead)->f88);
        int x = nd->f58;
        if (x == lead)
            x = nd->f54;
        if (party_relation(x, a) < -0.01f)
            return 0;
        attach_party(a, x);
        parties.get(a)->f8cc = 1;
        return 1;
    }
    if (a == f78) {
        lead = last_leader(b);
        f13a0c = lead;
        f13a10 = -100000;
        f13a14 = 0;
        Party* pl = parties.get(lead);
        Party* pa = parties.get(a);
        attach_party_center(V2((pa->x + pl->x) * 0.5f, (pl->y + pa->y) * 0.5f), a, lead);
        return 1;
    }
    if (b == f78) {
        if (0.0f < f13ad0 && f13ad4 != 0)
            return 0;
        int la = last_leader(a);
        f13a0c = la;
        f13a10 = -100000;
        f13a14 = 1;
        Party* p1 = parties.get(la);
        Party* p2 = parties.get(b);
        attach_party_center(V2((p1->x + p2->x) * 0.5f, (p2->y + p1->y) * 0.5f), b, a);
        return 1;
    }
    Party* p1 = parties.get(a);
    Party* p2 = parties.get(lead);
    attach_party_center(V2((p2->x + p1->x) * 0.5f, (p2->y + p1->y) * 0.5f), a, lead);
    return 1;
}

// The work area 0x49f3d0 shares with the appearance/agent builders. The member names are the frame offsets of
// the original (local_bc .. local_54): `b8` is local_b8, `f60` is local_60, and `f74`/`f70` receive the mesh
// node lists the builders hand back.
struct SpawnArea {
    int bc; int b8; int b4; int b0; int ac; int a8; int a4; int a0;
    int x9c; int x98; int x94; int x90; int x8c; int x88; int x84; int x80; int x7c; int x78;
    int f74; int f70; int x6c; int x68; int x64; int f60; int x5c; int x58; int x54;
    // the original zeroes every member with its own store
    __forceinline SpawnArea()
        : bc(0), b8(0), b4(0), b0(0), ac(0), a8(0), a4(0), a0(0), x9c(0), x98(0), x94(0), x90(0), x8c(0), x88(0),
          x84(0), x80(0), x7c(0), x78(0), f74(0), f70(0), x6c(0), x68(0), x64(0), f60(0), x5c(0), x58(0), x54(0)
    {
    }
};
struct V4c {                                                                      // w is left uninitialised (a stack slot)
    float x, y, z, w;
    V4c(float a, float b, float c) : x(a), y(b), z(c) {}
};
struct World {
    char pad0[0x344];
    int f344;                                                                     // +0x344
    int f348;                                                                     // +0x348
    int f34c;                                                                     // +0x34c
    SceneObj* create_entity(int kind);                                            // 0x428440 (ret 4)
};
struct PtrVec {                                                                   // a vector of pointers (begin/end)
    void** begin;                                                                 // +0 (0xa75f54)
    void** end;                                                                   // +4 (0xa75f58)
    int size() { return (int)(end - begin); }
};
extern PtrVec g_world_list;                                                       // 0xa75f54
struct MeshNodeList { void cap_all(float* v); };                                  // 0x438f90 (ret 4)

// appearance / agent builders (other slices; the argument lists are taken from the disassembly)
__int64 pick_slot_item(int a, int b, int c, int d, int* e, char f, char* g);       // 0x46b190 (cdecl, ret 0x1c)
void build_appearance(SceneObj* e, int count, int party, int arg, PartyStack* s, void* b8, void* name,
                      void* sixty, float* dist, void* bc, ItemRef* items, int one); // 0x482240 (cdecl)
void build_agent(SceneObj* e, int count, int party, void* b8, float* dist, ItemRef* items,
                 int one);                                                        // 0x47cfe0 (cdecl, ret 0x1c)
void link_appearance(SceneObj* e, ItemRef* items, void* b8);                       // 0x495430 (ret 0xc)

// @ 0x49f3d0
// Spawn the scene object of party `party` (negative = its horse) and build its full appearance: create the
// entity in the current world, copy the 0x40-byte frame matrix into it, resolve the skeleton (a skin for a
// troop, "skel_horse" for a horse), fill the ten item slots from the party's chosen troop, run the
// appearance/agent builders, link the animation groups and set the position. The builders themselves
// (0x482240, 0x47cfe0, 0x495430, 0x46b190, 0x438f90, 0x439fc0, 0x4339d0, 0x421df0, 0x428440, 0x428820,
// 0x4288c0, 0x46b9d0) live in other slices.
void spawn_party_scene(int party, int kind, int* frame, int* group, int* args)
{
    if (g_world_list.begin == 0 || g_world_list.size() == 0)
        return;
    World* world = (World*)g_world_list.begin[g_world_list.size() - 1];

    SceneObj* e = world->create_entity(9);
    memcpy(e->m, frame, 0x40);

    PartyStack s;
    s.id = kind;
    s.val = 0;
    s.reduce(0);

    SpawnArea area;
    float dist = 0.0f;
    char scratch = 0;

    FxDef2* def = new FxDef2(2);
    e->skel = def;
    def->set_env_count(2);
    def->f5c = 0;

    ItemRef items[10];
    int count;
    if (party < 0) {
        def->set_skeleton(g_restable.find(&tstring("skel_horse")));
        count = 1;
    } else {
        unsigned t = (unsigned)g_game->chars[party].flags & 0xf;
        def->set_skeleton(t < (unsigned)g_body_count
                              ? *(void**)((char*)g_body_table + 0x1e8 + t * 0x1f8) : 0);
        int level = g_game->chars[party].level;
        for (int i = 0; i < 10; i++)
            *(unsigned __int64*)&items[i] = pick_slot_item(party, i, world->f344, world->f348,
                                                           &world->f34c, 0, &scratch);
        count = level;
    }
    char name[0x10];
    build_appearance(e, count, party, *args, &s, &area.b8, name, &area.f60, &dist, &area.bc, items, -1);
    if (party >= 0) {
        build_agent(e, count, party, &area.b8, &dist, items, -1);
        link_appearance(e, items, &area.b8);
        if (area.f74 != 0) {
            dist = 20.0f;
            for (int i = 0; i < *(int*)(area.f74 + 0x84); i++)
                ((MeshNodeList*)(area.f74 + 0x34 + i * 0x10))->cap_all(&dist);
        }
        if (area.f70 != 0) {
            dist = 20.0f;
            for (int i = 0; i < *(int*)(area.f70 + 0x84); i++)
                ((MeshNodeList*)(area.f70 + 0x34 + i * 0x10))->cap_all(&dist);
        }
    }
    Group* g = &g_groups[*group];
    if (g->count > 0) {
        int* src = g->src;
        e->skel->set_group0(0, src[0], src[4], src[5]);
        e->skel->link_group(0);
        *(int*)e->skel->f40 = 0x3f800000;
        e->skel->set_blend(0.05f, e->m);
    }
    *(V4c*)&e->f_c4 = V4c(0.0f, 0.0f, 1.1f);
    dist = 100000.0f;
    e->set_pos(&dist);
}

// @ 0x49f980
// Enter the trigger block `val` at statement `ip`: pack `ip` (0..0x2a) into the top byte of `val`, then
// register the entry with the trigger block object at g_game+0x14014.
void Mission::enter_trigger(int val, int ip)
{
    PartyStack s;
    s.id = val;
    if (ip < 0x2b && ip >= 0) {
        s.val = (s.val & 0xffffff) | (ip << 24);
        s.reduce(0);
    }
    block_add((void*)((char*)g_game + 0x14014), &s, 1);
}

// @ 0x49f9e0
void Mission::exec_trigger(int val)
{
    char out[0x20];
    block_exec((void*)((char*)g_game + 0x14014), val, -1, 0, 0, -1, 0, 0, 0, out);
}

} // namespace c49f060

// ======================================================================================
// GameData members of slice s0049b630 (the slices called the game-data object `Mission`): the value counters, the quest
// "taken" stamp and the label text layout. Fields not in game_data.h are reached by offset.
// ======================================================================================
#include "../str_table.h"
extern StrTable g_strtable;                         // 0x5e2a18
#include "../../include/msg_log.h"
extern MsgLog g_msglog;                             // 0x5d7080 (MsgLog::add 0x452c00, thiscall, ret 0x14)
extern int g_sound_holder[];                        // 0x5aa128 (sound device)
extern float g_sound_origin[];                      // 0x5aa168 (position vector for play)
extern int g_snd_got_value;                         // 0x5aa1a0 (sound id)
extern int g_snd_lost_value;                        // 0x5aa19c
extern int g_snd_new_quest;                         // 0x5aa180
struct SoundPlayer { int play(int idx, int flags, const float* pos, float freq, int* out); };  // 0x442650
void mb_snprintf(char* buf, unsigned n, const char* fmt, ...);  // 0x415100

// ---- shared log tail of 0x49b720 / 0x49b820 (inlined in both; the source of the original
// likely had a static helper, but cl 13.10 inlines it at /O2 regardless of `noinline`) ----------
static __forceinline void value_message(GameData* m, int amount, const char* key, const int* sound_id)
{
    char buf[0x200];
    {
        tstring s(key);
        sprintf(buf, g_strtable.lookup(s)->c_str(), m->string_value(amount).c_str());
    }
    g_msglog.add(tstring(buf), -1);
    ((SoundPlayer*)g_sound_holder)->play(*sound_id, 1, g_sound_origin, 1.0f, 0);
}

// @ 0x49b720
void GameData::add_gold(int gold, int troop)
{
    Troop* rec = troops + troop;
    rec->f3b4 += gold;                             // Troop +0x3b4 (gold)
    if (troop == 0 && f13a34 != 0 && gold != 0)
        value_message(this, gold, "ui_you_got_value", &g_snd_got_value);
}

// @ 0x49b820
void GameData::remove_gold(int gold, int troop)
{
    Troop* rec = troops + troop;
    rec->f3b4 -= gold;
    if (rec->f3b4 < 0)
        rec->f3b4 = 0;
    if (troop == 0 && f13a34 != 0 && gold != 0)
        value_message(this, gold, "ui_you_lost_value", &g_snd_lost_value);
}

// @ 0x49b930
void GameData::start_quest(int quest, int giver_troop)
{
    quests[quest].giver_troop = giver_troop;       // +0x118
    quests[quest].f11c = (float)(g_ticks[f13b18] - *(__int64*)&f13b20) * 1e-5f;   // +0x11c (game time when started)
    QuestRec* rec = &quests[quest];
    if (((rec->flags2 >> 1) & 1) || !rec->f64_flag) {
        tstring name(quests[quest].text);
        char buf[0x200];
        {
            tstring key("ui_new_quest_taken");
            const char* n = name.c_str();
            tstring* f = g_strtable.lookup(key);
            mb_snprintf(buf, 0x200, f->c_str(), n);
        }
        g_msglog.add(tstring(buf), 0xffd0f0c8);
        ((SoundPlayer*)g_sound_holder)->play(g_snd_new_quest, 1, g_sound_origin, 1.0f, 0);
        rec->flags = 0x100;
    }
}

// @ 0x49baa0
void GameData::label_text(rgl::mesh* m, const tstring* text, char flag, int arg, const float* h)
{
    Font* font = g_font;
    unsigned flags = 0;
    rgl::vector4 pos(0.05f, 0.1f, 0.2f);
    float   scale[2];
    scale[0] = 0.2f; scale[1] = 0.2f;
    float   xmin = 0.0f;
    float   xmax = 1.0f;

    if (flag) {
        xmax = GD(float, 0x13acc) * *h;
        xmin = -xmax;
        scale[0] = xmax * 0.6f;
        scale[1] = xmax * 0.6f;
        pos = rgl::vector4(0.0f, 0.1f, 0.2f);
        flags = 0x10;
    }
    float   out[2];
    arg = arg;
    m->clear_geometry();
    font->layout_text(out, text, &pos, scale, m, (int)&arg, &xmin, &xmax, flags);
}

// ======================================================================================
// The script block runner (canonical DialogBlock / Elem, src/include/dialog_rec.h): the mission script interpreter's
// operand accessors (0x49d500, 0x49d650), the loop test (0x49d7f0), the line renumberer (0x49d930), the else_try /
// try_end linker (0x49dd30), the try_end pop (0x49f7c0), run_line (0x4b0940) and the block runner emit (0x4b09e0).
// The slices (s0049d500, s0049f060, s004b0940) viewed these as members of `Interp`, `Mission` and `ScriptHolder`;
// all of them are DialogBlock / Elem members (items, count, done = the linked flag, text = the script id at +0xc).
// ======================================================================================

// 0x49d160 is a file-static function in the original translation unit, so its pointer argument arrives in
// EAX.  A local static re-implementation reproduces that convention exactly (and hence the original
// behaviour); the party-table probe mirrors 0x49d160: max-id guard, index_of, page/base lookup, live flag.
static __declspec(noinline) bool check_party(int* id)
{
    char buf[0x200];
    int v = *id;
    PartyMgr* pm = &g_game->parties;
    if ((unsigned)v < pm->n_parties) {
        int r = pm->index_of(v);
        if (r >= 0) {
            Party* base = pm->pages._Myfirst[r >> 4];
            if (base[r & 0xf].type == 1)
                return true;
        }
    }
    sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Party ID: %d", g_cur_opcode, v);
    report(buf);
    return false;
}

// 0x54ba40 "MyButton", 0x593164 ", ", 0x593160 " = ", 0x593158 " = %d"
// 0x593168 "Warning: Unmatched try_end", 0x593184 "Warning: Unmatched else_try."

// @ 0x49d500
void Elem::set_lvalue(__int64* locals, const __int64& value)
{
    char buf[512];
    const int* v = (const int*)&value;
    int k = ((int*)vals)[1];
    int idx = ((int*)vals)[0];
    if (k < 0 || (k == 0 && (unsigned)idx >= 0x80000000u)) {
        sprintf(buf, "Illegal lvalue in statement; Opcode = %d", g_cur_opcode, g_cur_param);
        report(buf);
        return;
    }
    int bad = 0;
    int kind = ((unsigned)k >> 24) & 0x7f;
    if (kind == 1) {
        if (bad != 0) {
        } else if (idx < 0 || idx >= 0x80) {
            report("illegal register no.");
            return;
        } else {
            g_reg_lo[idx*2] = v[0];
            g_reg_hi[idx*2] = v[1];
            return;
        }
    } else if (kind == 2) {
        if (bad != 0) {
        } else if (idx < 0 || idx >= 0x10000) {
            report("illegal variable no.");
            return;
        } else {
            g_vartab.set(idx, v[0]);
            return;
        }
    } else if (kind == 0x11) {
        if (bad != 0) {
        } else if (idx < 0 || idx >= 0x80) {
            report("illegal local variable no.");
            return;
        } else {
            ((int*)locals)[idx * 2] = v[0];
            ((int*)locals)[idx * 2 + 1] = v[1];
            return;
        }
    }
    sprintf(buf, "Illegal lvalue in statement; Opcode = %d", g_cur_opcode, g_cur_param);
    report(buf);
}

// @ 0x49d650
__int64 Elem::get_operand(__int64* locals, int* i, int* type)
{
    char buf[512];
    int idx = *i;
    int p = ((int*)vals)[idx * 2];
    int k = ((int*)vals)[idx * 2 + 1];
    if (k < 0 || (k == 0 && (unsigned)p < 0x80000000u)) {
        *type = 0;
        return ((__int64)(unsigned)k << 32) | (unsigned)p;
    }
    int kind = ((unsigned)k >> 24) & 0x7f;
    *type = kind;
    int r = k & 0xffffff;
    if (kind == 0)
        return ((__int64)(unsigned)k << 32) | (unsigned)p;
    if (kind == 1) {
        if (p >= 0 && p < 0x80) {
            unsigned lo = g_reg_lo[p*2];
            _ReadWriteBarrier();
            int hi = g_reg_hi[p*2];
            return (__int64)lo | ((__int64)hi << 32);
        }
        sprintf(buf, "Invalid register no in operation = %d, param0 = %d param1 = %d", id, ((int*)vals)[0], ((int*)vals)[2]);
        report(buf);
        return 0;
    }
    if (kind == 2) {
        if (r == 0 && (unsigned)p < 0x10000u) {
            int v = g_vartab.get(p);
            return (__int64)v;
        }
        sprintf(buf, "Invalid variable no in operation = %d, param0 = %d param1 = %d", id, ((int*)vals)[0], ((int*)vals)[2]);
        report(buf);
        return 0;
    }
    if (kind == 0x11) {
        if (r == 0 && (unsigned)p < 0x80u)
            return locals[p];
        sprintf(buf, "Invalid local variable no in operation = %d, param0 = %d param1 = %d", id, ((int*)vals)[0], ((int*)vals)[2]);
        report(buf);
        return 0;
    }
    return ((__int64)(unsigned)r << 32) | (unsigned)p;
}

// @ 0x49d7f0
bool DialogBlock::step_less(int locals, int* stack)
{
    int n = stack[0x100];
    if (n == 0)
        return false;
    int left, idx;
    if (n >= 1 && n < 0x80) {
        idx = stack[n * 2 - 2];
        left = stack[n * 2 - 1];
    } else {
        idx = locals;
        left = locals;
    }
    int op = items[idx].id;
    Elem* ins = items + idx;
    int kind;
    if (op == 6) {
        int two = 2;
        int v = (int)ins->get_operand((__int64*)locals, &two, &kind);
        return left < v;
    }
    if (op == 7) {
        int one = 1;
        int v = (int)ins->get_operand((__int64*)locals, &one, &kind);
        return left >= v;
    }
    if (op == 0xc)
        return left < g_mission->agents.count;
    if (op == 0xb)
        return left < (int)g_game->parties.n_parties;
    if (op == 0xf) {
        int one = 1;
        int v = (int)ins->get_operand((__int64*)locals, &one, &kind);
        if (check_party(&v)) {
            Party* party = g_game->parties.get(v);
            return left < container_size(&party->attached);
        }
        return left < -1;
    }
    int one = 1;
    int v = (int)ins->get_operand((__int64*)locals, &one, &kind);
    return left < v;
}

// @ 0x49d930
tstring DialogBlock::renumber(int locals, int index, int use_var, int* pvar, int* pend)
{
    tstring* src = &text;                            // the block's script id
    tstring* gs = &GDP(tstring, 0x1a080);            // the whole script text
    tstring result;
    tstring key("\"");
    key.append(src->substr(src->find(' ', 0) + 1, src->len));
    {
        tstring q("\"");
        key.append(q);
    }
    int pos = gs->find(key, 0);
    tstring value;
    if (pos > 0) {
        for (int i = index; i > 0; i--) {
            if (pos >= 2) {
                tstring sep("),");
                pos = gs->find(sep, pos) + 2;
            }
        }
        if (pos >= 2)
            pos = gs->find('(', pos) + 1;
        if (pos >= 1) {
            int end;
            {
                tstring sep("),");
                end = gs->find(sep, pos);
            }
            if (end >= 0) {
                *pvar = pos;
                *pend = end;
                tstring body = gs->substr(pos, end);
                int eq;
                {
                    tstring eqs(" = ");
                    eq = body.find(eqs, 0);
                }
                while (eq >= 0) {
                    int c = body.find(',', eq);
                    if (c < 0)
                        c = body.len;
                    body.erase(eq, c - eq);
                    tstring eqs(" = ");
                    eq = body.find(eqs, 0);
                }
                value = body;
                int c = body.find(',', 0);
                if (c >= 0) {
                    int base = c + 1;
                    body = body.substr(base, body.len);
                    int idx = 0;
                    for (;;) {
                        int n = body.find(',', 0);
                        if (n <= 0 && body.len <= 0)
                            break;
                        int endpos = (n <= 0) ? body.len : n;
                        tstring item = body.substr(0, endpos);
                        if (endpos + 1 < body.len)
                            body = body.substr(endpos + 1, body.len);
                        else
                            body.assign("");
                        item.trim();
                        bool named = false;
                        for (int k = 0; k < item.len; k++) {
                            char ch = item.buf[k];
                            if (ch != '-' && (ch < '0' || ch > '9')) {
                                named = true;
                                break;
                            }
                        }
                        if (named) {
                            int kind;
                            int v = (int)items[index].get_operand((__int64*)locals, &idx, &kind);
                            char tmp[132];
                            sprintf(tmp, " = %d", v);
                            tstring pre(tmp);
                            tstring num(tmp);
                            value.insert(base + endpos, num);
                            base += pre.len;
                        }
                        idx++;
                        base = base + 1 + endpos;
                    }
                }
            }
        }
    }
    result.append(value);
    return result;
}

// @ 0x49dd30
void DialogBlock::link()
{
    int stack[0x100];
    int try_end[0x100];
    if (done) return;
    int depth = 0;
    for (int i = 0; i < count; i++) {
        unsigned op = items[i].id;
        if ((op & 0xfffffff) < 30) {
            if (op == 4 || op == 6 || op == 7 || op == 0xb || op == 0xf || op == 0xc) {
                try_end[depth] = i;
                stack[depth++] = i;
            } else if (op == 5) {
                if (depth <= 0) {
                    report("Warning: Unmatched else_try.");
                } else {
                    for (int j = stack[depth - 1] + 1; j < i; j++)
                        if (items[j].f90 < 0)
                            items[j].f90 = i;
                    stack[depth - 1] = i;
                }
            } else if (op == 3) {
                if (depth <= 0) {
                    if (g_script_output) report("Warning: Unmatched try_end");
                } else {
                    for (int j = try_end[depth - 1]; j < i; j++)
                        if (items[j].f90 < 0)
                            items[j].f90 = i;
                    depth--;
                }
            }
        }
    }
    for (int i = 0; i < count; i++)
        if (items[i].f90 < 0)
            items[i].f90 = count;
    done = 1;
}

// @ 0x49f7c0
// Pop / replace the loop entry on top of the try stack. `n` is the depth read once at entry; the top entry's
// opcode decides what happens to its value (try_begin / else_try just unwind, for_range / for_parties ... shift it
// along one), then the loop test runs and the (possibly re-mapped) value is written back as a 64-bit script value.
void DialogBlock::pop_try(int locals, int* stack, int* out)
{
    int val;
    int idx;
    (*out)++;
    int n = stack[0x100];
    if (n == 0)
        return;
    if (n > 0 && n < 0x80) {
        idx = stack[n * 2 - 2];
        val = stack[n * 2 - 1];
    }
    int kind = items[idx].id;
    if (kind == 4 || kind == 5) {
        if (n < 1)
            return;
        stack[0x100] = n - 1;
        return;
    }
    if (kind == 6 || kind == 7 || kind == 0xb || kind == 0xf || kind == 0xc) {
        if (kind == 0xc) {
            g_mission->agents.next_used((unsigned*)&val);
        } else if (kind == 7) {
            val--;
        } else if (kind == 0xb) {
            g_game->parties.next_party_id(&val);
        } else {
            val++;
        }
    } else {
        return;
    }
    if (n > 0 && n < 0x80)
        stack[n * 2 - 1] = val;
    if (!step_less(locals, stack)) {
        int c = stack[0x100];
        if (c > 0)
            stack[0x100] = c - 1;
        return;
    }
    if (kind == 0xf) {
        int one = 1;
        int type;
        int p = (int)items[idx].get_operand((__int64*)locals, &one, &type);
        val = g_game->parties.get(p)->attached.first[val];
    }
    __int64 wide = val;
    items[idx].set_lvalue((__int64*)locals, wide);
    *out = idx + 1;
}

// @ 0x4b0940
char DialogBlock::run_line(int locals, int* idx, int script)
{
    int out;
    char ok = ((ScriptStmt*)(items + *idx))->run(locals, script, &out);
    if (out > 0) {
        char buf[512];
        _snprintf(buf, 0x200, "At %s.", text.c_str());
        g_msglog.add(tstring(buf), 0xffff5555);
        LogStreamBase& lg = g_log;
        lg.w_cstr(buf);
    }
    return ok;
}
// @ 0x4b09e0
// The mission script runner. Steps over the statements `items[idx]` of this holder. Each opcode is run
// through one of the interpreter helpers, then step_less decides whether to fall through or jump to the
// statement's skip target (f90). While the script is "inside a key frame" (the tag matches the current
// line, or the frame was opened earlier) the input frames are polled for the abort/return/continue keys.
char DialogBlock::emit(int script, int depth, int* limit, __int64* vals, unsigned tag)
{
    char last_ok = 1;                           // local_b05
    char done = 0;                              // local_b07
    char in_key = 0;                            // local_b06
    char brk = 0;                               // local_afa
    char first = 0;                             // local_af9
    char ok = 1;                                // local_af4
    int saved;                                  // local_af0
    int which;                                  // local_b04
    int idx = 0;                                // local_b00
    char advance = 1;                           // local_b08
    int pr_lo, pr_hi;                           // local_ae4 / local_ae0
    int out1;                                   // local_aec
    int out2;                                   // local_ad8
    int out3;                                   // local_ac4
    int range_lo, range_hi;                     // local_ad4 / local_adc
    int nsub;                                   // local_ae8
    long long value;                            // local_ad0 / local_acc
    DialogBlock* self = this;                   // local_af8
    int buf[256];                               // local_a10
    ScriptStack buf2;                           // local_410
    char msg[512];                              // local_610
    __declspec(align(8)) __int64 sub[16];                            // auStack_a98 + 2

    g_script_out = 0;
    link();
    if (g_script_out > 0) {
        const char* q = text.c_str();
        _snprintf(msg, 0x200, "At %s.", q);
        g_msglog.add(tstring(msg), 0xffff5555);
        LogStreamBase& lg = g_log;
        lg.w_cstr(msg);
    }
    g_script_out = 0;
    buf2.sp = 0;
    buf2.pad = 0;
    last_ok = 1;
    g_script_flag = 0;
    idx = 0;
    in_key = 0;
    done = 0;
    advance = 1;

    for (;;) {
        if (idx >= count)
            break;
        brk = 0;
        if ((tag & 0xffff) == (unsigned)idx || in_key) {
            in_key = 1;
            first = 0;
            for (;;) {
                g_keyboard.frame(0);
                if (!first) {
                    first = 1;
                    tstring s0 = renumber((int)buf, idx, 1, &pr_lo, &pr_hi);
                    set_progress_range(pr_lo, pr_hi);
                    refresh_settings_list(&s0, 1);
                }
                if ((g_key_a[0x3b] && !g_key_b[0x3b]) || EMIT_AUDIO.id == 0) {
                    EMIT_AUDIO.id = 0;
                    set_debug_label();
                    tag = 0xffffffff;
                    done = 1;
                } else if (g_key_a[0x3a] && !g_key_b[0x3a]) {
                    tag = 0xffffffff;
                    done = 1;
                } else if (g_key_a[0x39] && !g_key_b[0x39]) {
                    int e = (int)tag >> 16;
                    if (g_emitters[e].name.equals(EMIT_AUDIO.name))
                        tag = ((unsigned)e << 16) | (unsigned)EMIT_AUDIO.tag;
                    done = 1;
                } else if (g_key_a[0x38] && !g_key_b[0x38]) {
                    brk = 1;
                } else if (!(g_key_a[0x37] && !g_key_b[0x37])) {
                    pump_messages();
                    continue;
                }
                break;
            }
            pump_messages();
        }

        Elem* stmt = items + idx;
        int off  = idx * 0x98;
        int op = stmt->id & 0xfffffff;
        (void)off;
        g_lineno = idx;
        saved = idx;
        ok = 1;
        g_cur_opcode = op;

        if (op < 0x15 && op != 1) {
            if (stmt->id == 3) {
                advance = 1;
                pop_try((int)buf, buf2.data, &idx);
            } else if (stmt->id == 4) {
                advance = 1;
                buf2.goto_stmt(idx, 0);
                idx++;
            } else if (stmt->id == 5) {
                if (advance) {
                    idx = stmt->f90;
                } else {
                    idx++;
                    advance = 1;
                }
            } else {
                int v = 0;
                advance = 1;
                if (stmt->id == 6) {
                    which = 1;
                    v = (int)stmt->get_operand((__int64*)buf, &which, &out2);
                } else if (stmt->id == 7) {
                    which = 2;
                    v = (int)stmt->get_operand((__int64*)buf, &which, &out2) - 1;
                } else if (stmt->id == 0xc) {
                    v = g_mission->agents.first_used();
                } else if (stmt->id == 0xb) {
                    v = (int)g_game->parties.first_party_id();
                } else if (stmt->id == 0xf) {
                    v = 0;
                } else {
                    mb_error("invalid opcode %d", stmt->id);
                }
                buf2.goto_stmt(idx, v);
                value = v;
                stmt->set_lvalue((__int64*)buf, value);
                if (!step_less((int)buf, buf2.data)) {
                    idx = stmt->f90;
                    goto step_end;
                }
                idx++;
            }
        } else if (op < 0x1e) {
            if (op == 1) {
                which = 0;
                int id = (int)stmt->get_operand((__int64*)buf, &which, &out1);
                if (id < 0 || id >= g_emitter_count || depth >= 0x100) {
                    ok = 0;
                    advance = 0;
                    idx = stmt->f90;
                    goto step_end;
                }
                which = 1;
                for (int i = 1; i < stmt->n; i++) {
                    which = i;
                    sub[i - 1] = stmt->get_operand((__int64*)buf, &which, &out1);
                }
                unsigned t;
                if (brk)
                    t = 0;
                else if (EMIT_AUDIO.id != 0 && g_emitters[id].name.equals(EMIT_AUDIO.name))
                    t = ((unsigned)id << 16) | (unsigned)EMIT_AUDIO.tag;
                else
                    t = 0xffffffff;
                nsub = stmt->n - 1;
                ok = g_emitters[id].script.emit(script, depth + 1, &nsub, sub, t);
                if (!ok) {
                    advance = 0;
                    idx = stmt->f90;
                } else {
                    idx++;
                }
            } else if (op == 0x15) {
                which = 0;
                if (check_script_param(&which, limit))
                    stmt->set_lvalue((__int64*)buf, vals[0]);
                idx++;
            } else if (op == 0x16) {
                which = 1;
                if (check_script_param(&which, limit))
                    stmt->set_lvalue((__int64*)buf, vals[1]);
                idx++;
            } else if (op == 0x17) {
                which = 1;
                int k = (int)stmt->get_operand((__int64*)buf, &which, &out3) - 1;
                which = k;
                if (check_script_param(&which, limit))
                    stmt->set_lvalue((__int64*)buf, vals[k]);
                idx++;
            } else {
                idx++;
            }
        } else {
            ok = run_line((int)buf, &idx, (int)&script);
            if (ok) {
                idx++;
            } else {
                advance = 0;
                idx = stmt->f90;
            }
        }

    step_end:
        if (buf2.sp == 0)
            last_ok = ok;
        if (in_key) {
            if (done) {
                in_key = 0;
                done = 0;
            }
            tstring s2 = renumber((int)buf, saved, 1, &which, &saved);
            tstring s1 = renumber((int)buf, saved, ok, &range_lo, &range_hi);
            set_progress_range(range_lo, range_hi);
            refresh_settings_list(&s1, 0);
        }
        if (g_script_out > 0) {
            const char* q = self->text.c_str();
            _snprintf(msg, 0x200, "At %s.", q);
            g_msglog.add(tstring(msg), 0xffff5555);
            LogStreamBase& lg = g_log;
            lg.w_cstr(msg);
        }
        if (!last_ok)
            break;
    }

    return last_ok;
}
