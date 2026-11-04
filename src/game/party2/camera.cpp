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
#define MB_CAMERA_TU
#include "../../include/image_addrs.h"
#include "../animator.h"      // Animator g_anim (0x8ed4f60)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include <new>
#include <string.h>
#include "../../rgl/rgl_math.h"
#include "../../include/crt.h"      // canonical CRT declarations (listed directly: the resolution audit reads only the source's own includes)
#include "../game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)
#include <vector>
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <stddef.h>
#include "../tstring.h"
#include "../font.h"
#include "../../include/game_data.h"
#include "../../include/fx.h"          // FxDef (the effect definition of a scene entity)
#include "../../include/dialog_rec.h"
#include "../../rgl/mesh.h"
#include "../../rgl/resource.h"             // resmgr g_resmgr (0x5d6fa0)
#include "../../include/party.h"
#include "../terrain_b/terrain_b.h"
#include "./camera.h"
#include "../mesh_slot/mesh_build.h"   // build_appearance (0x482240), setup_item_meshes (0x47cfe0)
#include "../skeleton.h"                // node_groups / node_list (cap_all 0x438f90)
#include "./party_2.h"
#include "./camera_class.h"      // Camera (the one declaration of the 0x4a8-byte view object)
#include "../../include/sound_mgr.h"      // g_sound_mgr 0x5aa128 (listener at +0x40 = 0x5aa168)
#include "../str_table.h"                  // StrTable::lookup 0x43a1e0 (the canonical class the views below forward to)
#include "../ui/container.h"                 // ContainerBase (ui/mission_ui.h)
#include "../rec98.h"                         // Rec98: the 0x98-byte army/settlement record (pages at mission+0xe4, 0x49bbb0)
#include "../strat/gui_manager.h"       // gui_manager (set_font_mesh 0x49b630 is defined here)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/game_recs.h"
#include "../../include/level_global.h"      // g_level (0xa732b4)
#include "../mode_rec.h"   // g_mission_templates 0x5e2a00
#include "../float_consts.h"   // shared float constants kept in memory (0x549cdc ...)
#include "../animator.h"
#include "../../include/clock_table.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "../../rgl/text_stream.h"

// ---- data this module owns (docs/standalone_data.md) ----

extern Font* g_font;                                // 0x5e2934

// ---- the canonical owners of the callees ---------------------------------------------------------------------------------------
// The namespaced views below (one namespace per slice) declare the engine functions they call under their own class names, which
// the linker cannot resolve: the mangled name has to carry the class that DEFINES the function. These are member-only stubs under
// the defining name (only their `// 0x...` callee is declared); the views forward to them with `this` cast, which compiles to the
// same call. Each comment names the defining source.
#include "../../include/vec_xlen.h"                                                // 0x43e2a0 std::vector<int>::_Xlen (agent3/agent_combat.cpp)
struct Mission3 {
    void add_amount(int idx, unsigned flags);   // 0x4677a0 terrain_a/terrain_a.cpp (ret 8)
};
struct MissionW {                                                                   // 0x492230 / 0x492350 world_data/world_data.cpp (ret 8)
    void set_weapon_order_1(int team, unsigned value);
    void set_weapon_order_2(int team, unsigned value);
};
struct Sel {                                                                        // ui/mission_ui.cpp: the order leader's selection record
    void set_v(int v);                                                              // 0x489860
    void set_vec(const int* a, const int* b, const int* c);                         // 0x4898e0
    void bump(int d);                                                               // 0x4899e0
};
struct ForceLabel {
    void make_label(char* out);   // 0x48d940 mission_ui2/mission_3.cpp
};
// level::nearest_face 0x439a50 (agent2/agent_stats.cpp): terrain_b/cell_grid.h
struct PartyMgrW {
    int add_slot();   // 0x493960 world_data/world_data.cpp
};
struct HudOuter {
    void scroll_refresh(rgl::vector4* arg);   // 0x498420 world_data/mesh_hud.cpp
};
struct FieldPanel {                                                                 // mission_ui2/mission_3.cpp
    void update();                                                                  // 0x48d470
    void set_color(unsigned rgb);                                                   // 0x48d350
    void set_alpha(unsigned a);                                                     // 0x48d3c0
};
struct SphereC {
    SphereC(rgl::vector4 c, const float* r);   // 0x4332a0 rgl/xmlcfg/game_util.cpp (named `sphere` there)
};
struct Anim;
struct Curve {
    void reset_rec(int i);   // 0x4339d0 agent/curve.cpp (FxDef::reset_rec in include/fx.h)
};
struct ColourOwner;
rgl::mesh* __stdcall build_legend_mesh(ColourOwner* parent);                               // 0x495b20 world_data/mesh_hud.cpp
namespace rgl { struct matrix; } typedef rgl::matrix pos;
__int64 __cdecl pick_slot_item(int troop, int slot, int mask, int n, int* cand, char remove, char* found);   // 0x46b190 terrain_a/terrain_a.cpp
struct CarryBody; struct CarryRef; struct CarryCtx;
void __cdecl place_scene_agents(CarryBody* holder, CarryRef* table, CarryCtx* ctx);   // 0x495430 world_data/world_data.cpp
struct MissionG {
    int create(const rgl::matrix* m, const struct ItemStackG* it, int troop);   // 0x4836e0 mesh_slot/mesh_game.cpp (ret 0xc)
    int spawn_full(const rgl::matrix* m, int troop, int a, int b, int c, int d, int e, int f, int cnt, int* arr);   // 0x483870 mesh_slot/mesh_game.cpp (ret 0x28)
};


// camera.h's views of the interpreter's callees forward to the defining classes
template<> void std::vector<int>::push_back(const int&);                           // 0x4936b0 world_data/world_data.cpp (out-of-line instance: only declared)
template<> size_t std::vector<int>::size() const;                                   // 0x40f010 rgl/particles/mesh_obj.cpp (out-of-line instance: only declared)
__forceinline void ScriptStack::push(int line, int val) { ((PairStack*)this)->push(line, val); }                // 0x488e60

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

struct Fadeable { void fade_scale(vector4 v); };                    // 0x47af30

struct R8 { int a, b; };                        // an 8-byte returned rect (2 floats)
struct E16 { int a, b, c, d; };                 // one 16-byte text-row record

// ---- external callees ----------------------------------------------------------------------------
void __fastcall base_ctor_48ae20(void* o);      // 0x48ae20 (ecx = object)
void __fastcall dtor_common(void* o);      // 0x490520 (ecx = object)

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
    void _Xlen() const { VEC_XLEN_CALL(this); }      // 0x43e2a0
};

// 0x435a10 is the int* instance of std::_Copy_opt (agent/agent.cpp; trivial _Uninit_copy = memmove). cl 13.10 takes an ICE on an explicit
// specialization declaration of it, so the instance is the library's own (same mangled name)
inline E4* uninit_copy(E4* f, E4* l, E4* d, PVec*, ptr_tag)
{
    return (E4*)std::_Copy_opt((int*)f, (int*)l, (int*)d, std::_Scalar_ptr_iterator_tag());
}

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

// 0x49b4f0 is MissionUi::get_xy (config/widget_mgr.cpp): the base class's text-rect slot, not this panel's

} // namespace c49aca0

// @ 0x49b4b0
// Mission::place: place the prop `item` (an ItemRef: id, modifier word) at the trigger block's frame.
int Mission::place(const int* p)
{
    int a = -1;
    int b = 6;
    return add_placed(&b, p, p + 1, (const rgl::matrix*)::g_game->trigger_frame, &a);
}

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

struct SoundMgr { int play(int idx, int flags, const float* pos, float freq, int* out); };  // 0x442650


struct TickPair {                                   // g_ticks[i].lo + g_ticks[i].hi * 2^32
    int lo, hi;
    operator __int64() const { return ((long long)hi << 32) | (unsigned)lo; }
};

extern "C" int __cdecl sprintf(char*, const char*, ...);        // 0x536f8f

// ---- scene objects -------------------------------------------------------------------------------
struct Shape { virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
               virtual void v4(); virtual void v5(); virtual void v6(); };
struct SphereMem {
    Shape* construct(vector4 pos, const float* radius)   // 0x4332a0 (thiscall, ret 0x14, ecx = raw memory): the constructor of ::sphere (game_util.cpp)
    {
        rgl::vector4 c;
        c.x = pos.x; c.y = pos.y; c.z = pos.z; c.w = pos.w;
        new ((void*)this) ::sphere(c, radius);
        return (Shape*)this;
    }
};

struct RotFrame { void rotate_about_u(const float* angle) { ((rgl::matrix*)this)->rotate_about_u(angle); } };   // 0x40e770 (thiscall): rotates the side/forward rows

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
    void add_extra(mesh* m) { ((Poser*)this)->add_extra((int)m); }                         // 0x442110 (thiscall, ret 4)
};
struct EntityHost { EntityFull* create_entity(int type) { return (EntityFull*)((rgl::world*)this)->create_entity(type); } };   // 0x428440 (ret 4)


// ---- records ---------------------------------------------------------------------------------------
// 0x98-byte army/settlement record (mission+0xe4 page table; see 0x49bbb0)
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
    int     index_of(int id) { return ((Map10007*)this)->lookup(id); }   // 0x48ad00 (ret 4)
    Rec5ac8* get(int id) { int a = index_of(id); return (Rec5ac8*)((a & 0xf) * 0x5ac8 + *(pages.begin() + (a >> 4))); }
};

// ---- Mission -------------------------------------------------------------------------------------
struct Mission {
    char        pad000[0xe0];
    std::vector<char*> f0e0;                        // +0xe0  page table of Rec98 records
    char        pad0f0[0xf8 - 0xf0];
    AgentTable  agents;                             // +0xf8 (0x138d4; pages at +0x139c8)
    // (the HudRoot fields at +0x132c/+0x1480/+0x158c and the +0x13a00.. records belong to other objects and were never
    //  accessed through this struct; they were mis-placed here and removed so the layout stays truthful)

    tstring string_value(int index);                // 0x48e9e0 (ret 4)
    mesh* build_legend_mesh(EntityFull* e) { return ::build_legend_mesh((ColourOwner*)e); }                // 0x495b20 (ret 4)

    void spawn_battle_icon(unsigned id);                                    // 0x49bbb0
};

// @ 0x49bbb0
void Mission::spawn_battle_icon(unsigned id)
{
    Rec98* rec = (Rec98*)((id & 0xf) * 0x98 + *(f0e0.begin() + (id >> 4)));
    EntityFull* e = ((EntityHost*)g_script.level)->create_entity(9);
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
    ((BattleRec*)rec)->bind_host();

    if (rec->f10 == 1) {
        agents.get(rec->f54)->icon_type = 7;
        agents.get(rec->f58)->icon_type = 7;
        agents.get(rec->f54)->pmid = id;
        agents.get(rec->f58)->pmid = id;
    }

    if (rec->f10 == 1) {
        {
            tstring nm("battle_icon");
            e->add_extra(((::resmgr*)&::g_resmgr)->get_object(&nm)->clone());
        }
        {
            float angle = 1.5707964f;                       // 0x3fc90fdb (1.5707963f rounds to 0x3fc90fda)
            ((RotFrame*)((char*)e->f11c[0] + 0xcc))->rotate_about_u(&angle);
        }

        mesh* label = build_legend_mesh(e);

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

// @ 0x49b630
void gui_manager::set_font_mesh(const tstring* text, int arg)
{
    {
        tstring name("ui_font");
        m1480.res = (skin*)g_resmgr.get_material(name);
    }
    m1480.clear_geometry();
    if (text->len <= 0) {
        m1480.mask = 0;
        m1220.mask = 0;
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
    g_font->layout_text(out, text, &pos, scale, &m1480, (int)&arg, &xmin, &xmax, 0x100);
    m1480.mask = 1;
    m1220.mask = 1;
}


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


struct vector4 { float x, y, z, w; };
struct matrix { float m[4][4]; };
inline matrix* identity_pos(matrix* out) { return (matrix*)rgl::identity_pos((rgl::matrix*)out); }   // 0x40cd00 (cdecl; returns out)

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
    vector4 m20;                                    // +0x20 (two vector4s: the centre at +0x20 and a copy at +0x30, 0x4332a0)
    vector4 m30;                                    // +0x30
    int m40;                                        // +0x40 (the float* arg, copied)
};
typedef char Shape_size_check[sizeof(Shape) == 0x44 ? 1 : -1];
__forceinline Shape* sphere_shape(Shape* mem, rgl::vector4 pos, const float* radius)   // 0x4332a0 (ret 0x14, ECX=mem): the constructor of ::sphere on the raw memory
{
    new ((void*)mem) ::sphere(pos, radius);
    return mem;
}

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
    void update_flags() { ((::Party*)this)->update_flags(); }                                 // 0x4b7840
};

struct OrderEntry {                                 // 0x5ac8-byte order/agent record
    char pad0[0xc];
    tstring name;                                   // +0xc..0x1c an *inline* name tstring: the original passes
                                                    // `lea esi,[ebx+0xc]` as the string (0x49c04a), not a pointer.
                                                    // Declaring it `char*` (plus a pad) also put flag1c at +0x10,
                                                    // so every later field was 0xc low (obj at 0x59b4, not 0x59c0)
    char flag1c;                                    // +0x1c
    char pad1d[3];
    tstring text;                                   // +0x20
    char pad30[0xa0 - 0x30];
    unsigned flags;                                 // +0xa0 (0x400 normal, 0x80000 hidden, 0x3000 style)
    int f_a4;                                       // +0xa4 (vertical offset)
    char pada8[0x59c0 - 0xa8];
    OrderObj* obj;                                  // +0x59c0 (== Party::model)
    char pad59c4[0x59e8 - 0x59c4];
    Sprite* label;                                  // +0x59e8 (text mesh)
    char pad59ec[0x5ac8 - 0x59ec];
    void place_on_map() { ((::Party*)this)->place_on_map(); }                                // 0x4b2880
    void update_flags() { ((::Party*)this)->update_flags(); }                                // 0x4b7840 (the record is a Party)
};
typedef char chk_ordentry_flags[offsetof(OrderEntry, flags) == 0xa0 ? 1 : -1];
typedef char chk_ordentry_obj[offsetof(OrderEntry, obj) == 0x59c0 ? 1 : -1];
typedef char chk_ordentry_label[offsetof(OrderEntry, label) == 0x59e8 ? 1 : -1];
typedef char OrderEntry_size_check[sizeof(OrderEntry) == 0x5ac8 ? 1 : -1];

struct Reinf {                                      // 0x4c-byte reinforcement record
    int idx;                                        // +0 (unit index)
    int flags;                                      // +4
    char pad8[0x4c - 8];
};

struct Unit {                                       // 0x130-byte leader/unit record inside Mission
    char pad0[0x6b0];
    char m6b0[0x40];                                // +0x6b0 (frame copy of +0x6f0)
    char m6f0[0x40];                                // +0x6f0
    char pad730[0x770 - 0x730];
    int f770;                                       // +0x770
    int f774;                                       // +0x774
    int pad778;
};   // (offsets are relative to the Mission; the code addresses them as this + idx*0x130 + off)

// the mission's agent table (member of the object 0x49bea0 is called on)
struct AgentTable {
    struct Pair { int id; int slot; } direct[0x2717];   // +0 (id -> slot, direct-mapped)
    char pad138b8[0x138d0 - 0x138b8];
    char** pages;                                   // +0x138d0 (page table of records)
    int index_of(int id) { return ((Map10007*)this)->lookup(id); }                           // 0x48ad00 (ret 4)
};

struct MissionAgents {
    char pad0[0xf8];
    AgentTable table;                               // +0xf8
    void add_order_arrow(int id);                   // 0x49bea0 (ret 4)
};

struct OrderObjHost {
    virtual OrderObj* create_entity(int type);       // 0x428440 (ret 4)
};

struct Mission {
    char pad0[0x527c];
    char orders[8 * 0x16c];                         // +0x527c (order groups, 0x16c each)
    char pad5df0[0x5df0 - 0x527c - 8 * 0x16c];
    int cur_set;                                    // +0x5df0 (index into g_mission_templates)
    char pad5df4[0x6214 - 0x5df4];
    int team_b;                                     // +0x6214
    int team_a;                                     // +0x6218
    char pad621c[0x6668 - 0x621c];
    char slot_a;                                    // +0x6668
    char slot_b;                                    // +0x6669
    char pad666a[2];
    Unit units[1];                                  // +0x666c (0x130 each)

    void add_amount(int a, int b) { ((Mission3*)this)->add_amount(a, (unsigned)b); }                 // 0x4677a0 (ret 8)
    int label_text(Sprite* label, const tstring* name, int a, int b, const float* h)   // 0x49baa0 (ret 0x14)
    {
        ((::GameData*)this)->label_text((rgl::mesh*)label, name, (char)a, b, h);
        return 0;
    }
    Sprite* build_legend_mesh(OrderObj* o) { return (Sprite*)::build_legend_mesh((ColourOwner*)o); }                // 0x495b20 (ret 4)
    void check_slot(int id, unsigned bits);         // 0x49c170 (ret 8)
};

// 0xa732b4 is g_level; the original calls the *direct* create_entity (0x49bea0: mov ecx,[0xa732b4]; push 9;
// call 0x428440), not a virtual slot - the object at g_level is not polymorphic (its first dword is data).
static OrderObj* order_create_entity(int type)
{
    return (OrderObj*)((rgl::world*)g_script.level)->create_entity(type);
}

// @ 0x49bea0
void MissionAgents::add_order_arrow(int id)
{
    rgl::vector4 zero(0.0f, 0.0f, 0.0f);
    float radius = 0.3f;
    float h;
    int idx = table.index_of(id);
    OrderEntry* e = (OrderEntry*)(table.pages[idx >> 4] + (idx & 0xf) * 0x5ac8);
    OrderObj* o = order_create_entity(9);
    Shape* shape = (Shape*)operator new(0x44);
    if (shape != 0)
        shape = sphere_shape(shape, zero, &radius);
    o->shape = shape;
    e->obj = o;
    {
        matrix tmp;
        o->frame = *identity_pos(&tmp);
    }
    e->update_flags();                                // 0x49bf6d: the original calls it on the record (ecx = ebx), not e->obj
    e->label = ((Mission*)g_game)->build_legend_mesh(e->obj);
    // the original keeps h (+0x10, reused from the radius), scale (+0x14 = 1.0, passed by pointer) and a dead copy of f_a4
    float scale = 1.0f;
    {
        if (e->flags & 0x80000) {
            tstring s("");
            ((Mission*)g_game)->label_text(e->label, &s, 0, 1, &scale);
        } else {
            const tstring* nm;
            if ((e->flags & 0x3000) == 0)
                h = 0.5f;
            else if ((e->flags & 0x3000) == 0x1000)
                h = 0.75f;
            else
                h = 1.0f;
            if (g_module_settings.show_party_ids_instead_of_names) {
                nm = &e->text;
            } else {
                // the found tstring* (or the record's own name tstring at +0xc) goes straight to label_text
                const tstring* f = 0;
                if (e->flag1c == 0)
                    f = g_strtable.find(&e->text);
                nm = f ? f : &e->name;
            }
            if (e->flags & 0x400) {
                ((Mission*)g_game)->label_text(e->label, nm, 1, (int)h, &scale);
                tstring mat_name("map_legend_outline");
                Sprite* mat = (Sprite*)g_resmgr.get_material(mat_name);
                Sprite* old = ((RefSlot*)e->label)->sprite;
                if (old) old->refs--;
                ((RefSlot*)e->label)->sprite = mat;
                if (mat) mat->refs++;
            } else {
                ((Mission*)g_game)->label_text(e->label, nm, 0, (int)h, &scale);
            }
        }
    }
    o->mode = id;
    o->fade = 1;
    if (!(o->param() > 0.0f)) {
        id = 0x3f666666;                            // the original reuses the argument slot for the 0.9f
        o->set_param((const float*)&id);
    }
    e->place_on_map();
}

// @ 0x49c170
void Mission::check_slot(int id, unsigned bits)
{
    if (id >= 0 && id < g_mission_templates[cur_set].n_spawns) {
        Reinf* r = (Reinf*)&g_mission_templates[cur_set].recs[id];
        int eax_, ebx_;

        memcpy((char*)this + r->idx * 0x130 + 0x6b0, (char*)this + r->idx * 0x130 + 0x6f0, 0x40);
        *(int*)((char*)this + r->idx * 0x130 + 0x770) = 0;     // r->idx is re-read: the stores may alias it
        *(int*)((char*)this + r->idx * 0x130 + 0x774) = 0;
        add_amount(id, bits);
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
        g_gui.add(tstring("Invalid entry no for add_reinforcements"), -1);
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


struct vector4 { float x, y, z, w; };

// an order slot inside a leader record (0x6c bytes)
struct SubOrder {
    int     state;          // +0x00  armed flag (written by the hold/follow/charge/advance orders)
    char    pad04[0xc];
    vector4 target;         // +0x10  move target
    vector4 extra;          // +0x20  (zeroed, w = order group)
    int     face;           // +0x30  nearest face
    float   ox, oy, oz;     // +0x34  per-slot offset
    char    pad40[4];
    int     mode;           // +0x44  sub-order mode (0 = unused)
    char    pad48[0x14];
    vector4 home;           // +0x5c  the slot's resting target (fall back restores it when the slot is unarmed)
};                          // 0x6c bytes (the Leader stride below)

struct Leader {             // this + 0x527c + index*0x16c
    int      pad0;
    int      agent_id;      // +4
    SubOrder sub[3];        // +8, stride 0x6c
    char     pad14c[0x20];
};
typedef char SubOrder_size_check[sizeof(SubOrder) == 0x6c && sizeof(Leader) == 0x16c ? 1 : -1];

struct AgentTable {         // mission + 4
    unsigned count;                         // +0
    unsigned first_used() { return ((::AgentTable*)this)->first_used(); }                  // 0x43b1b0
    void     next_used(unsigned* id) { ((::AgentTable*)this)->next_used(id); }       // 0x43b160
};

struct Agent {
    char    pad0[0x50];
    vector4 pos;            // +0x50
    char    pad60[0x1c4 - 0x60];
    int     slot_no;        // +0x1c4  order group (clamped to 7)
    char    pad1c8[0x2b0 - 0x1c8];
    int     unit;           // +0x2b0  unit/order-table index
    char    pad2b4[4];
    int     form_a;         // +0x2b8
    int     form_b;         // +0x2bc
    float   form_delay;     // +0x2c0
    char    pad2c4[0x56b4 - 0x2c4];
    vector4 alt;            // +0x56b4
    char    pad56c4[0x5928 - 0x56c4];
    int     update_nav_face() { return ((::Agent*)this)->update_nav_face(); }     // 0x43cb30 (thiscall) -> eax
};
typedef char Agent_offset_check[offsetof(Agent, form_a) == 0x2b8 && offsetof(Agent, alt) == 0x56b4 ? 1 : -1];

struct Level { int nearest_face(vector4* p) { return ((::level*)this)->nearest_face((const float*)p); } };     // 0x439a50 (thiscall) -> face index

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

    int  point_in_rect(vector4* p) { return ((::Mission*)this)->point_in_rect((const float*)p); }           // 0x48a990 (thiscall) -> al
    void set_weapon_order_1(int index, int on) { ((MissionW*)this)->set_weapon_order_1(index, (unsigned)on); }      // 0x492230 (ret 8)
    void set_weapon_order_2(int index, int mode) { ((MissionW*)this)->set_weapon_order_2(index, (unsigned)mode); }     // 0x492350 (ret 8)
    void order(int index, int order);
};
extern Mission* g_mission;                  // 0x5e292c

// order-leader helpers (all thiscall on the Leader record)
// 0x4898e0: every armed slot gets target = *pos, face = *face, extra = *dir
inline void set_vec(Leader* L, vector4* pos, int* face, vector4* dir) { ((Sel*)L)->set_vec((const int*)pos, face, (const int*)dir); }
inline void set_v(Leader* L, int v) { ((Sel*)L)->set_v(v); }                                     // 0x489860
inline void bump(Leader* L, int v) { ((Sel*)L)->bump(v); }                                    // 0x4899e0
inline void make_label(Leader* L, char* buf) { ((ForceLabel*)L)->make_label(buf); }                                      // 0x48d940

struct TickPair { int a; int b; };
extern "C" int __cdecl sprintf(char*, const char*, ...);   // 0x536f8f

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
            break;                              // still informs and re-seeds the group (0x49c2fe -> 0x49cafa)
        Agent* a = agent_by_id(this, L->agent_id);
        vector4 zero = { 0, 0, 0, 0 };
        int f = a->update_nav_face();
        set_vec(L, &a->pos, &f, &zero);
        if (L->sub[0].mode) L->sub[0].state = 1;
        if (L->sub[1].mode) L->sub[1].state = 1;
        if (L->sub[2].mode) L->sub[2].state = 1;
        strcpy(ordstr, order_string("ui_hold_this_position_e_"));
        break;
    }
    case 1: {                                   // follow me
        if (L->agent_id == -1)
            break;                              // still informs and re-seeds the group (0x49c3e5 -> 0x49cafa)
        if (L->sub[0].mode) L->sub[0].state = 2;
        if (L->sub[1].mode) L->sub[1].state = 2;
        if (L->sub[2].mode) L->sub[2].state = 2;
        Agent* a = agent_by_id(this, L->agent_id);
        int f = a->update_nav_face();
        set_vec(L, &a->pos, &f, &a->alt);
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
        set_v(L, 1);
        strcpy(ordstr, order_string("ui_mount_horses_e_"));
        break;
    }
    case 4: {                                   // dismount
        set_v(L, 2);
        strcpy(ordstr, order_string("ui_dismount_e_"));
        break;
    }
    case 7: {                                   // stand closer
        bump(L, -1);
        strcpy(ordstr, order_string("ui_stand_closer_e_"));
        break;
    }
    case 8: {                                   // spread out
        bump(L, 1);
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
            if (!point_in_rect(&p))
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
            if (s->state == 0)
                s->target = s->home;
            vector4 p;
            p.x = s->target.x - s->ox * 7.0f;
            p.y = s->target.y - s->oy * 7.0f;
            p.z = s->target.z - s->oz * 7.0f;
            p.w = 0.0f;
            if (!point_in_rect(&p))
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
        set_weapon_order_1(index, 1);
        strcpy(ordstr, order_string("ui_use_blunt_weapons_e_"));
        break;
    }
    case 10: {                                  // use any weapon
        set_weapon_order_1(index, 0);
        strcpy(ordstr, order_string("ui_use_any_weapon_e_"));
        break;
    }
    case 12: {                                  // hold fire
        set_weapon_order_2(index, 1);
        strcpy(ordstr, order_string("ui_hold_fire_e_"));
        break;
    }
    case 13: {                                  // fire at will
        set_weapon_order_2(index, 0);
        strcpy(ordstr, order_string("ui_fire_at_will_e_"));
        break;
    }
    default:                                    // an unknown order still informs (empty caption) and re-seeds
        break;
    }

    // inform the local agent, but only about its own order group
    if (local_id >= 0) {
        Agent* la = agent_by_id(this, local_id);
        int g = la->slot_no;
        if (g < 0 || g >= 8)
            g = 7;
        if (g == index) {
            make_label(L, ordtxt);
            sprintf(msg, ordstr, ordtxt);
            tstring line(msg);                  // copied into the by-value argument (0x42c110), destroyed after the call
            g_gui.add(line, 0xfffffefb);
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
            a->form_a = ((int*)&g_ticks[a->unit])[0];
            a->form_b = ((int*)&g_ticks[a->unit])[1];
            a->form_delay = (float)(rand() % 0x3dc9) * 6.3223117e-05f + 1.0f;
        }
        tbl->next_used(&id);
    }
}

} // namespace c49c2c0


// ======================================================================================
// slice s0049deb0 (the view object Camera: camera_class.h)
// ======================================================================================
// Slice s0049deb0: the mission "view" object.  It is an engine object (base ctor 0x48ae20, base dtor
// 0x490520, descriptor 0x591c80) with a scene subobject at +0x188, a box frustum at +0x360 (0x120
// bytes) and the per-frame shake/colour/zoom fields that update() drives.  The view's own descriptor
// is 0x5931a8 and its size is 0x4a8.
//
// ctor 0x49deb0, type 0x49df30, mask 0x49df40, move 0x49df60, update 0x49dfe0,
// scalar deleting dtor 0x49e580, render 0x49e5b0.
namespace c49deb0 {   // the slice's own callee / data declarations (their link names carry this namespace)
using rgl::vector4;

// ---- callees that live in other slices -----------------------------------------------------------
inline void identity_pos(float* m) { rgl::identity_pos((rgl::matrix*)m); }                   // 0x40cd00  out-only 4x4 identity (cdecl)

// ---- float / int constants (named by address, matching the game's own tables) ---------------------
extern float g_tick_scale;                     // 0x54a6d0  1e-05f  (tick -> seconds); defined once, in mesh_slot/slot.cpp
extern const float g_549cd8;                    // 0x549cd8  4294967296.0f (unsigned->float bias)
extern const float g_k002;                    // 0x590cd0  0.02f

} // namespace c49deb0

// tick counters, 8 bytes each; timers index this array through their own key field.
// ---- stub interface for the virtual calls (the other interfaces are in camera_class.h) ----



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
    shake_x = 0;
    shake_y = 0;
    follow[0] = 0;
    follow[1] = 0;
    target = 0;
    tail = float2(0, 0);
}

// @ 0x49df30
int Camera::kind()
{
    return 7;
}

// @ 0x49df40
unsigned Camera::mask(int a)
{
    if (a == 0)
        return f104 & 0x10;
    return f104 & 0x20;
}

// @ 0x49df60
// Place the followed object at the view position (plus its own displacement), undo the current shake
// if it is the tracked object, then tell it it moved.
void Camera::on_child_moved(MissionUi* e)
{
    *(float2*)&e->fE0 = float2(e->fE8 + fE0, e->fEC + fE4);
    if (e == target) {
        e->fE0 = e->fE0 - shake_x;
        e->fE4 = e->fE4 - shake_y;
    }
    e->on_moved();
}

// @ 0x49dfe0
// Per-frame view update: the two shake axes follow the tracked object while its follow objects report
// a weight, then the four fades (zoom, colour, level, shake) are advanced.
void Camera::animate(int)
{
    float2 v;

    shake_x = target->screen[2];
    shake_y = target->screen[3];
    for (int i = 2; i != 0; i--) {
        int k = 2 - i;
        if (follow[k] && slot_on[k]) {
            float r = follow[k]->get168();
            (&shake_x)[k] = (target->screen[k] - target->screen[2 + k] - screen[k]) * r + target->screen[2 + k];
        }
    }
    on_child_moved(target);

    float d0 = elapsed(0);    // colour
    float d1 = elapsed(1);    // level
    float d4 = elapsed(4);    // zoom
    float d5 = elapsed(5);    // shake

    // zoom: blend the target's zoom from the near weights toward the far ones
    if (d4 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - d4;
        float xn = d4 * f88 + rest * a8;
        // (the original spills both y terms and their sum to float slots before scaling; as plain locals they stay in x87
        // registers: the same value except that denormal results are not rounded to float in between)
        float yb = d4 * f8c;
        float yr = rest * ac;
        float yn = yr + yb;
        float x = xn * inv;
        float y = yn * inv;
        v.x = x * 0.02f;
        v.y = y * 0.02f;
        target->kids[0]->set_size((float*)&v);
        fD8 = x;
        fDC = y;
        ((FieldPanel*)this)->update();
    } else if (c2) {
        c2 = 0;
        ((WItem*)this)->terrain_refresh(&f88);
    }

    // colour: the three level bytes blended from the target levels and packed under the inherited
    // alpha byte, then the target sub-object's colour slot
    if (d0 < dur(0)) {
        int i0 = blend_u(dur(0), d0, srcA & 0xff, valA & 0xff);
        int i1 = blend_i(dur(0), d0, ((unsigned char*)&srcA)[1], ((unsigned char*)&valA)[1]);
        int i2 = blend_i(dur(0), d0, ((unsigned char*)&srcA)[2], ((unsigned char*)&valA)[2]);
        unsigned col = (color14c | target->kids[0]->color14c) & 0xff000000u;
        col |= (unsigned)((i2 << 16) + (i1 << 8) + i0);
        target->kids[0]->set_color(&col);
        target->kids[0]->get_xy((float*)&v);
    } else if (c0) {
        c0 = 0;
        ((FieldPanel*)this)->set_color(color14c ^ ((color14c ^ valA) & 0xffffff));
    }

    // level
    if (d1 < dur(1)) {
        ((FieldPanel*)this)->set_alpha(blend_i(dur(1), d1, ((unsigned char*)&srcA)[7], ((unsigned char*)&valA)[7]));
    } else if (c1) {
        c1 = 0;
        ((FieldPanel*)this)->set_alpha(((unsigned char*)&valA)[7]);
    }

    // shake: blend toward the far weights and hand the offset to the object
    if (d5 < dur(5)) {
        float inv = g_one / dur(5);
        float rest = dur(5) - d5;
        v.x = (d5 * f80 + rest * a0) * inv;
        v.y = (d5 * f84 + rest * a4) * inv;
        move_a((float*)&v);
        ((FieldPanel*)this)->update();
    } else if (c3) {
        move_a(&f80);
        ((FieldPanel*)this)->update();
        c3 = 0;
    }
}

// @ 0x49e580  (scalar deleting destructor)
// (the scalar deleting destructor of the implicit destructor is generated with the vtable, above)

// @ 0x49e5b0
// Place the view: size the scene viewport from the view box, rebuild the box frustum, install it as
// the scene's volume and the view matrix, then hand the frame to the scene.  The view matrix is the
// identity with its translation row set from a position vector whose w is never initialised (the
// original copies that stack slot into m[15]).
// the box setter (inline in the original): left, right, bottom, top, near, far
static __forceinline void set_box(rgl::frustum* f, float l, float r, float b, float t, float n, float fa)
{
    f->perspective = 0;
    f->left = l;
    f->right = r;
    f->bottom = b;
    f->top = t;
    f->nearz = n;
    f->farz = fa;
}
struct V4 { float x, y, z, w; };
void Camera::render(rgl::scene* a, float* pos, int c, int d)
{
    scene.reset();
    scene.f0 = (int)((float)g_gui.screen_w * fE0);
    scene.f4 = (int)((0.75f - (fE4 + screen[1])) * (float)g_gui.screen_h * 1.3333334f);
    scene.f8 = (int)((float)g_gui.screen_w * screen[0]);
    scene.fc = (int)((float)g_gui.screen_h * screen[1] * 1.3333334f);
    scene.f14 = 1.0f;
    scene.f10 = 0;
    scene.flags |= 0x1000;
    rgl::frustum* f = &fr;
    set_box(f, screen[0] * -0.5f, screen[0] * 0.5f, screen[1] * -0.5f, screen[1] * 0.5f, 0, 501.0f);
    f->build_planes();
    float m[16];
    c49deb0::identity_pos(m);
    V4 v;
    v.z = 400.0f;
    v.x = screen[0] * 0.5f + fE0;
    v.y = screen[1] * 0.5f + fE4;
    *(V4*)&m[12] = v;
    memcpy(&f->m, m, 0x40);
    f->build_planes();
    memcpy(&scene.fr, f, 0x120);
    *pos = *pos + 1.0f;
    ContainerBase::render(&scene, pos, c, d);
}


// ======================================================================================
// slice s0049e780 (namespace c49e780)
// ======================================================================================
namespace c49e780 {
// Slice s0049e780: view construction (0x49e780) and the "party sprite" wrappers that hang a party's
// character record on the view's scene graph (0x49e9c0..0x49f020).
//
//   0x49e780  View::create_text_overlay(int arg, int flags)   -> index of the new child in the view's list
//   0x49e9c0  Party::attach_party_center(float2*, int, int)     -> sprite record id
//   0x49ebd0  Party::add_sprite(int x, int y, int troop)  -> sprite record id
//   0x49ef00  Party::create_companion_party(int slot, int x, int y, int a) -> sprite record id
//   0x49f020  Party::spawn_around(int x, int y, int troop, int a)
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
struct scene_api;                                        // 0x498420 (thiscall scene position, below)


// The global string table (this = 0x5e2a18) and its lookup (0 when absent).


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
    virtual void attach(void* v, void* n);               // +0xa4
    virtual void s42(); virtual void s43(); virtual void s44(); virtual void s45();
    virtual void s46(); virtual void s47(); virtual void s48(); virtual void s49();
    virtual void add_child(void* v);                     // +0xc8
};
// the constructors work on the raw memory and return it (explicit constructor calls of the defining classes: no null test, no allocation)
inline scene_obj* base_ctor(scene_obj* p) { ((MissionUi*)p)->MissionUi::MissionUi(); return p; }      // 0x48ae20 (MissionUi::MissionUi)
inline scene_obj* ps_ctor(scene_obj* p) { ((UiD*)p)->UiD::UiD(); return p; }                        // 0x48b890 (UiD::UiD)
inline scene_obj* cam_ctor(scene_obj* p) { new ((void*)p) Camera(); return p; }             // 0x49deb0 (Camera::Camera, above)

// 0x498420: a free thiscall position setter (argument in ecx, one pointer argument).
struct scene_api { void set_pos(float2* p) { ((HudOuter*)this)->scroll_refresh((rgl::vector4*)p); } };   // (HudOuter::scroll_refresh)

// The view (`this`) seen from here: the child list object at +0x2c and its own vector at +4/+8.
struct View {
    char pad0[4];
    int* begin;                                          // +4
    int* end;                                            // +8
    char pad_c[0x2c - 0xc];
    scene_obj* list;                                     // +0x2c

    int create_text_overlay(int arg, int flags);                      // 0x49e780
    void push(void** obj) { ((std::vector<int>*)this)->push_back(*(const int*)obj); }                               // 0x4936b0 (thiscall push_back)
};

// ---- @ 0x49e780 -----------------------------------------------------------------------------------
// Build the scene object for `flags` (attribute bits flags&0x381c | 0x700, scale 0.9 x 0.75) and, for
// the "3d" flag 0x2000, the particle system + camera + group around it.
int View::create_text_overlay(int arg, int flags)
{
    scene_obj* o = (scene_obj*)operator new(0x178);
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
    o->attach((void*)&r, (void*)arg);                    // +0xa4 (the original passes &r first, then arg)
    if (flags & 0x2000) {
        scene_obj* ps = (scene_obj*)operator new(0x188);
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
        scene_obj* cam = (scene_obj*)operator new(0x4a8);
        if (cam)
            cam = cam_ctor(cam);                         // 0x49deb0
        else
            cam = 0;
        *(int*)((char*)cam + 0x104) |= 0x20;
        *(scene_obj**)((char*)cam + 0x484) = ps;
        scene_obj* grp = (scene_obj*)new ContainerBase;
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
    return (this->end - this->begin) - 1;
}

} // namespace c49e780

// ======================================================================================
// GameData members of slice s0049e780 (the slice called the game-data object `Party`): the battle records (0x98 bytes, the pages
// at +0xe4) and the parties that stand for them (0x5ac8 bytes, the pages at +0x138d0).
// ======================================================================================
// the battle record `id` (16 to a page)
static __forceinline Rec98* battle_rec(GameData* g, unsigned id) { return (Rec98*)g->agent_pages[id >> 4] + (id & 0xf); }

// @ 0x49e9c0
// Build a battle record for the parties `a` and `b` at `pt`: the record pool hands out the id, the record takes the
// position, both party ids and the summed unit counts, and both parties point at it.
int GameData::attach_party_center(const float* pt, int a, int b)
{
    int id = ((RecPool*)((char*)this + 0xcc))->alloc_record();   // 0x493700
    battle_rec(this, id)->f10 = 1;
    battle_rec(this, id)->f24 = pt[0];
    battle_rec(this, id)->f28 = pt[1];
    battle_rec(this, id)->f10 = 1;
    battle_rec(this, id)->f54 = a;
    battle_rec(this, id)->f58 = b;
    battle_rec(this, id)->f78 = 0;
    if (a >= 0) {
        int d[4] = { 0, 0, 0, 0 };
        ((GameW*)this)->count_units(a, &d[0], &d[3], &d[2], &d[1]);       // 0x490ce0
        battle_rec(this, id)->f78 += d[0];
    }
    if (b >= 0) {
        int d[4] = { 0, 0, 0, 0 };
        ((GameW*)this)->count_units(b, &d[0], &d[3], &d[2], &d[1]);       // 0x490ce0
        battle_rec(this, id)->f78 += d[0];
    }
    parties.get_party(a)->action = 7;
    parties.get_party(b)->action = 7;
    parties.get_party(a)->f88 = id;
    parties.get_party(b)->f88 = id;
    ((c49b630::Mission*)this)->spawn_battle_icon(id);          // 0x49bbb0
    return id;
}

// @ 0x49ebd0
// Add a party of party template `tmpl` at (x, y): the last party record is filled from the template record, named after
// it, randomly rotated, and given a fixture-following behaviour.
int GameData::add_sprite(float x, float y, int tmpl)
{
    PartyTemplate* tr = &g_party_templates[tmpl];       // 0x5e29f8 + 0xb0 * tmpl
    int id = ((PartyMgrW*)&parties)->add_slot();         // 0x493960
    parties.get_party(id)->init();                       // 0x48f180
    parties.get_party(id)->template_id = tmpl;
    parties.get_party(id)->pos.x = x;
    parties.get_party(id)->pos.y = y;
    parties.get_party(id)->dirx = 1.0f;
    parties.get_party(id)->diry = 0;
    float ang = (float)(rand() % 0x3dc9) * 0.00039724255f; // 0x536a9f / 0x54a6e0
    float c = (float)cos(ang);
    float s = (float)sin(ang);
    Party* ro = parties.get_party(id);
    float oy = ro->diry;
    float ox = ro->dirx;
    ro->diry = c * oy + s * ox;
    ro->dirx = c * ox - s * oy;
    tstring* n = g_strtable.find(&tr->key);     // 0x4c6010
    if (n == 0)
        n = &tr->name;
    parties.get_party(id)->name = *n;
    parties.get_party(id)->flag = 1;
    parties.get_party(id)->key = tr->key;
    parties.get_party(id)->flags_hi = tr->f24;
    parties.get_party(id)->flags_lo = tr->f20;
    parties.get_party(id)->f90 = tr->f28;
    parties.get_party(id)->f70 = tr->f2c;
    parties.get_party(id)->f74 = tr->f30;
    parties.get_party(id)->state = 2;
    parties.get_party(id)->ai_target_x = x;
    parties.get_party(id)->ai_target_y = y;
    add_template(id, tmpl, 0);                            // 0x48fbd0
    ((c49bea0::MissionAgents*)this)->add_order_arrow(id);  // 0x49bea0
    tr->num_created += 1;
    return id;
}

// @ 0x49ef00
// Add the party for troop `troop` at (x, y): named after the troop (its translation when the string table has one).
int GameData::create_companion_party(int troop, float x, float y, float z)
{
    int id = add_sprite(x, y, 3);                        // 0x49ebd0
    tstring* n = g_strtable.find(&chars[troop].key);     // 0x4c6010
    if (n == 0)
        n = &chars[troop].key;
    parties.get_party(id)->name = *n;
    parties.get_party(id)->flag = 1;
    parties.get_party(id)->f70 = chars[troop].faction;
    parties.get_party(id)->add_members((int)n, 1, 0, 0, 0);            // 0x48c840
    return id;
}

// @ 0x49f020
// Pick a position around (x, y) (x and y are adjusted in place) and add a party of template `tmpl` there.
int GameData::spawn_around(float x, float y, int tmpl, float radius)
{
    Vec2 p(x, y);
    ((GameW*)this)->random_position_around(p, radius, &p);                                   // 0x490030
    return add_sprite(p.x, p.y, tmpl);                         // 0x49ebd0
}

// ======================================================================================
// slice s0049f060 (namespace c49f060)
// ======================================================================================
namespace c49f060 {
// Slice s0049f060: party bookkeeping of the mission object at 0x5e2930. 0x49f060 decides whether party `a` can
// attach/dismount onto party `b` (and where the join point is); 0x49f3d0 spawns the scene object of a party for a
// party trigger; 0x49f7c0 pops/replaces the party on top of the trigger stack; 0x49f980 and 0x49f9e0 enter a
// trigger block. Module System names: parties, party_tpl, triggers. Names provisional.

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
    struct FxDef* skel;                                                          // +0x304
    char pad308[0x31c - 0x308];
};
// (the effect definition 0x7c bytes created by world::create_entity(2) is FxDef of include/fx.h)
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
    SceneObj* create_entity(int kind) { return (SceneObj*)((rgl::world*)this)->create_entity(kind); }      // 0x428440 (ret 4)
};

// appearance / agent builders (other slices; the argument lists are taken from the disassembly)
inline __int64 pick_slot_item(int a, int b, int c, int d, int* e, char f, char* g) { return ::pick_slot_item(a, b, c, d, e, f, g); }       // 0x46b190 (cdecl, ret 0x1c)
inline void place_scene_agents(SceneObj* e, ItemRef* items, void* b8) { ::place_scene_agents((CarryBody*)e, (CarryRef*)items, (CarryCtx*)b8); }   // 0x495430 (ret 0xc)

// @ 0x49f3d0
// Spawn the scene object of party `party` (negative = its horse) and build its full appearance: create the
// entity in the current world, copy the 0x40-byte frame matrix into it, resolve the skeleton (a skin for a
// troop, "skel_horse" for a horse), fill the ten item slots from the party's chosen troop, run the
// appearance/agent builders, link the animation groups and set the position. The builders themselves
// (0x482240, 0x47cfe0, 0x495430, 0x46b190, 0x438f90, 0x439fc0, 0x4339d0, 0x421df0, 0x428440, 0x428820,
// 0x4288c0, 0x46b9d0) live in other slices.
void spawn_party_scene(int party, int kind, int* frame, int* group, int* args)
{
    if (g_tableaus.worlds._Myfirst == 0 || g_tableaus.worlds.size() == 0)
        return;
    World* world = (World*)g_tableaus.worlds[g_tableaus.worlds.size() - 1];

    SceneObj* e = world->create_entity(9);
    memcpy(e->m, frame, 0x40);

    ItemRef s;
    s.item = kind;
    s.m4 = 0;
    s.reduce(0);

    SpawnArea area;
    float dist = 0.0f;
    char scratch = 0;

    FxDef* def = new FxDef(2);
    e->skel = def;
    def->set_env_count(2);
    def->f5c = 0;

    ::ItemRef items[10];
    int count;
    if (party < 0) {
        def->init((FxRes*)::g_resmgr.get_object70(tstring("skel_horse")));
        count = 1;
    } else {
        unsigned t = (unsigned)::g_game->chars[party].flags & 0xf;
        def->init((FxRes*)(t < (unsigned)g_anim.char_count
                              ? *(void**)((char*)((int*)g_anim.skin_vec.chars) + 0x1e8 + t * 0x1f8) : 0));
        int level = ::g_game->chars[party].level48;
        for (int i = 0; i < 10; i++)
            *(unsigned __int64*)&items[i] = pick_slot_item(party, i, world->f344, world->f348,
                                                           &world->f34c, 0, &scratch);
        count = level;
    }
    char name[0x10];
    ::build_appearance((rgl::strategic_entity*)e, count, party, *args, &s, (rgl::lod_mesh2**)&area.b8, (int*)name, (rgl::lod_mesh2**)&area.f60,
                       (int*)&dist, (rgl::lod_mesh2**)&area.bc, (EquipSet*)items, -1);
    if (party >= 0) {
        ::setup_item_meshes((SkelBodyV*)e, count, party, (char*)&area.b8, (int)&dist, items, -1);
        place_scene_agents(e, items, &area.b8);
        if (area.f74 != 0) {
            dist = 20.0f;
            for (int i = 0; i < ((node_groups*)area.f74)->nlists; i++)
                ((node_groups*)area.f74)->lists[i].cap_all(&dist);
        }
        if (area.f70 != 0) {
            dist = 20.0f;
            for (int i = 0; i < ((node_groups*)area.f70)->nlists; i++)
                ((node_groups*)area.f70)->lists[i].cap_all(&dist);
        }
    }
    Group* g = &((Group*)g_anim_set.data)[*group];
    if (g->count > 0) {
        int* src = g->src;
        ((Skeleton*)e->skel)->start_anim(0, (Anim*)src[0], src[4], src[5]);
        ((Curve*)e->skel)->reset_rec(0);
        *(int*)e->skel->env = 0x3f800000;
        e->skel->update(0.1f, (const rgl::matrix*)e->m);     // 0x3dcccccd (was 0.05f)
    }
    *(V4c*)&e->f_c4 = V4c(0.0f, 0.0f, 1.1f);
    dist = 100000.0f;
    e->set_pos(&dist);
}

} // namespace c49f060

// the 0x98-byte scene object record of index `idx` (the pages at game+0xe4, 16 records each)
static __forceinline Rec98* agent_rec(GameData* g, unsigned idx) { return (Rec98*)((idx & 0xf) * 0x98 + ((char**)g->agent_pages)[idx >> 4]); }
static __forceinline void attach_center(GameData* g, float x, float y, int a, int b)   // 0x49e9c0 (ret 0xc)
{
    Vec2 p(x, y);
    g->attach_party_center(&p.x, a, b);
}

// @ 0x49f060
// Decide whether party `a` can be attached to `b`, and where the join point is. When the leader of `b` is not
// deployed yet the two parties join at the midpoint of their positions (the player's party is always the first
// one handed to attach_party_center); otherwise `a` adopts the leader's scene object (the player) or attaches to
// the party standing next to it.
bool GameData::deploy_party(int a, int b)
{
    int lead = battle_root(b);
    if (parties.get_party(lead)->f88 >= 0) {
        if (a == party_id_78) {
            attach_party = agent_rec(this, parties.get_party(lead)->f88)->f58;
            attach_scene = agent_rec(this, parties.get_party(lead)->f88)->f54;
            attach_flip = 0;
            parties.get_party(a)->f88 = parties.get_party(lead)->f88;
            return 1;
        }
        Rec98* nd = agent_rec(this, parties.get_party(lead)->f88);
        int x = nd->f58;
        if (x == lead)
            x = nd->f54;
        if (relation(x, a) < -0.01f)
            return 0;
        ((GameW*)this)->set_objective(a, x);
        parties.get_party(a)->f8cc = 1;
        return 1;
    }
    if (a == party_id_78) {
        lead = battle_root(b);
        attach_party = lead;
        attach_scene = -100000;
        attach_flip = 0;
        ::Party* pl = parties.get_party(lead);
        Party* pa = parties.get_party(a);
        attach_center(this, (pa->pos.x + pl->pos.x) * 0.5f, (pl->pos.y + pa->pos.y) * 0.5f, a, lead);
        return 1;
    }
    if (b == party_id_78) {
        if (0.0f < f13ad0f && f13ad4 != 0)
            return 0;
        int la = battle_root(a);
        attach_party = la;
        attach_scene = -100000;
        attach_flip = 1;
        Party* p1 = parties.get_party(la);
        Party* p2 = parties.get_party(b);
        attach_center(this, (p1->pos.x + p2->pos.x) * 0.5f, (p2->pos.y + p1->pos.y) * 0.5f, b, a);
        return 1;
    }
    Party* p1 = parties.get_party(a);
    Party* p2 = parties.get_party(lead);
    attach_center(this, (p2->pos.x + p1->pos.x) * 0.5f, (p2->pos.y + p1->pos.y) * 0.5f, a, lead);
    return 1;
}

// @ 0x49f980
// Enter the trigger block `val` at statement `ip`: pack `ip` (0..0x2a) into the top byte of `val`, then
// register the entry with the trigger block object at g_game+0x14014.
int Mission::enter_trigger(int val, int ip)
{
    ItemRef s;
    s.item = val;
    if (ip < 0x2b && ip >= 0) {
        s.m4 = (s.m4 & 0xffffff) | (ip << 24);
        s.reduce(0);
    }
    return ((MissionG*)this)->create((const rgl::matrix*)g_game->trigger_frame, (const ItemStackG*)&s, 1);
}

// @ 0x49f9e0
int Mission::exec_trigger(int val)
{
    char out[0x20];
    return ((MissionG*)this)->spawn_full((const rgl::matrix*)g_game->trigger_frame, val, -1, 0, 0, -1, 0, 0, 0, (int*)out);
}

// ======================================================================================
// GameData members of slice s0049b630 (the slices called the game-data object `Mission`): the value counters, the quest
// "taken" stamp and the label text layout. Fields not in game_data.h are reached by offset.
// ======================================================================================
#include "../battle_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

// ---- shared log tail of 0x49b720 / 0x49b820 (inlined in both; the source of the original
// likely had a static helper, but cl 13.10 inlines it at /O2 regardless of `noinline`) ----------
static __forceinline void value_message(GameData* m, int amount, const char* key, const int* sound_id)
{
    char buf[0x200];
    {
        tstring s(key);
        sprintf(buf, g_strtable.lookup(s)->c_str(), m->string_value(amount).c_str());
    }
    g_gui.add(tstring(buf), -1);
    g_sound_mgr.play(*sound_id, 1, (const float*)&g_sound_mgr.frame.o, 1.0f, 0);
}

// @ 0x49b720
void GameData::add_gold(int gold, int troop)
{
    Troop* rec = troops + troop;
    rec->f3b4 += gold;                             // Troop +0x3b4 (gold)
    if (troop == 0 && f13a34 != 0 && gold != 0)
        value_message(this, gold, "ui_you_got_value", &g_sound_mgr.slot[10]);
}

// @ 0x49b820
void GameData::remove_gold(int gold, int troop)
{
    Troop* rec = troops + troop;
    rec->f3b4 -= gold;
    if (rec->f3b4 < 0)
        rec->f3b4 = 0;
    if (troop == 0 && f13a34 != 0 && gold != 0)
        value_message(this, gold, "ui_you_lost_value", &g_sound_mgr.slot[9]);
}

// @ 0x49b930
void GameData::start_quest(int quest, int giver_troop)
{
    WL_MARKER(12)
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
        g_gui.add(tstring(buf), 0xffd0f0c8);
        g_sound_mgr.play(g_sound_mgr.slot[2], 1, (const float*)&g_sound_mgr.frame.o, 1.0f, 0);
        rec->flags = 0x100;
    }
    WL_MARKER(13)
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
        pos = rgl::vector4(0.0f, 0.1f, 0.4f);              // 0x49bb06: 0x3ecccccd
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

// 0x49cc60 is a file-static function in the original translation unit (the message in ECX): a local copy of
// script_interpreter.cpp's byte-exact definition. "%s; LINE NO: %d:" to the message log and the text log, error flag up.
static __declspec(noinline) void report(const char* msg)
{
    char buf[1024];
    sprintf(buf, "%s; LINE NO: %d:", msg, g_lineno);
    g_gui.add(tstring(buf), 0xffff5555);
    stream_base& lg = g_log;
    lg.w_cstr(buf);
    g_script_error = 1;
}

// 0x49d160 is a file-static function in the original translation unit, so its pointer argument arrives in
// EAX.  A local static re-implementation reproduces that convention exactly (and hence the original
// behaviour); the party-table probe mirrors 0x49d160: max-id guard, index_of, page/base lookup, live flag.
static __declspec(noinline) bool check_party(int* id)
{
    char buf[0x200];
    int v = *id;
    PartyMgr* pm = &g_game->parties;
    if ((unsigned)v < pm->n_parties) {
        int r = ((Map10007*)pm)->lookup(v);
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
    if (((const __int64*)vals)[0] < 0) {        // 0x49d50c: test hi; jl; jg; test lo; jb (never) -- a signed 64-bit < 0
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
            g_script.reg_half[idx].lo = v[0];
            g_script.reg_half[idx].hi = v[1];
            return;
        }
    } else if (kind == 2) {
        if (bad != 0) {
        } else if (idx < 0 || idx >= 0x10000) {
            report("illegal variable no.");
            return;
        } else {
            g_script.set_var(idx, v[0]);
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
            unsigned lo = g_script.reg_half[p].lo;
            int hi = g_script.reg_half[p].hi;
            return (__int64)lo | ((__int64)hi << 32);
        }
        sprintf(buf, "Invalid register no in operation = %d, param0 = %d param1 = %d", id, ((int*)vals)[0], ((int*)vals)[2]);
        report(buf);
        return 0;
    }
    if (kind == 2) {
        if (r == 0 && (unsigned)p < 0x10000u) {
            int v = g_script.get_var(p);
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
// The original reuses its two incoming argument slots as the callee's out-variables: `stack` (as the int 1/2 operand
// selector) and `locals` (as the type out and then the loaded value).  Same here, so the frame depth matches.
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
    int* sel = (int*)&stack;
    int v;
    if (op == 6) {
        *sel = 2;
        v = (int)ins->get_operand((__int64*)locals, sel, &locals);
        return left < v;
    }
    if (op == 7) {
        *sel = 1;
        v = (int)ins->get_operand((__int64*)locals, sel, &locals);
        return left >= v;
    }
    if (op == 0xc)
        return left < g_mission->agents.count;
    if (op == 0xb)
        return left < (int)g_game->parties.n_parties;
    *sel = 1;
    if (op == 0xf) {
        v = (int)ins->get_operand((__int64*)locals, sel, &locals);
        locals = v;
        if (check_party(&locals)) {
            Party* party = g_game->parties.get(v);
            return left < (int)((std::vector<int>*)&party->attached)->size();
        }
        return left < -1;
    }
    v = (int)ins->get_operand((__int64*)locals, sel, &locals);
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
                if (depth > 0) {
                    for (int j = stack[depth - 1] + 1; j < i; j++)
                        if (items[j].f90 < 0)
                            items[j].f90 = i;
                    stack[depth - 1] = i;
                } else {
                    report("Warning: Unmatched else_try.");
                }
            } else if (op == 3) {
                if (depth > 0) {
                    for (int j = try_end[depth - 1]; j < i; j++)
                        if (items[j].f90 < 0)
                            items[j].f90 = i;
                    depth--;
                } else {
                    if (g_cfg.enable_edit_mode) report("Warning: Unmatched try_end");
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
        val = g_game->parties.get(p)->attached._Myfirst[val];
    }
    __int64 wide = val;
    items[idx].set_lvalue((__int64*)locals, wide);
    *out = idx + 1;
}

// @ 0x4b0940
char DialogBlock::run_line(int locals, int* idx, int script)
{
    int out;
    char ok = ((Operation*)(items + *idx))->execute((__int64*)locals, (int*)script, &out);
    if (out > 0) {
        char buf[512];
        _snprintf(buf, 0x200, "At %s.", text.c_str());
        g_gui.add(tstring(buf), 0xffff5555);
        stream_base& lg = g_log;
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

    g_script_error = 0;
    link();
    if (g_script_error > 0) {
        const char* q = text.c_str();
        _snprintf(msg, 0x200, "At %s.", q);
        g_gui.add(tstring(msg), 0xffff5555);
        stream_base& lg = g_log;
        lg.w_cstr(msg);
    }
    g_script_error = 0;
    buf2.sp = 0;
    buf2.pad = 0;
    last_ok = 1;
    g_or_latch = 0;
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
                g_keyboard.update(0.0f);
                if (!first) {
                    first = 1;
                    tstring s0 = renumber((int)buf, idx, 1, &pr_lo, &pr_hi);
                    set_progress_range(pr_lo, pr_hi);
                    refresh_settings_list(&s0, 1);
                }
                if ((g_keyboard.down[0x3b] && !g_keyboard.prev[0x3b]) || EMIT_AUDIO.id == 0) {
                    EMIT_AUDIO.id = 0;
                    set_debug_label();
                    tag = 0xffffffff;
                    done = 1;
                } else if (g_keyboard.down[0x3a] && !g_keyboard.prev[0x3a]) {
                    tag = 0xffffffff;
                    done = 1;
                } else if (g_keyboard.down[0x39] && !g_keyboard.prev[0x39]) {
                    int e = (int)tag >> 16;
                    if (g_scripts[e].name.equals(EMIT_AUDIO.name))
                        tag = ((unsigned)e << 16) | (unsigned)EMIT_AUDIO.tag;
                    done = 1;
                } else if (g_keyboard.down[0x38] && !g_keyboard.prev[0x38]) {
                    brk = 1;
                } else if (!(g_keyboard.down[0x37] && !g_keyboard.prev[0x37])) {
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
                buf2.push(idx, 0);
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
                buf2.push(idx, v);
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
                if (id < 0 || id >= g_scripts.count || depth >= 0x100) {
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
                else if (EMIT_AUDIO.id != 0 && g_scripts[id].name.equals(EMIT_AUDIO.name))
                    t = ((unsigned)id << 16) | (unsigned)EMIT_AUDIO.tag;
                else
                    t = 0xffffffff;
                nsub = stmt->n - 1;
                ok = g_scripts[id].body.emit(script, depth + 1, &nsub, sub, t);
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
            // the original reads `saved` into a register before the first call (which stores the block end back into the
            // slot through its last argument); both calls take the statement index, not the clobbered slot
            int at = saved;
            tstring s2 = renumber((int)buf, at, 1, &which, &saved);
            tstring s1 = renumber((int)buf, at, ok, &range_lo, &range_hi);
            set_progress_range(range_lo, range_hi);
            refresh_settings_list(&s1, 0);
        }
        if (g_script_error > 0) {
            const char* q = self->text.c_str();
            _snprintf(msg, 0x200, "At %s.", q);
            g_gui.add(tstring(msg), 0xffff5555);
            stream_base& lg = g_log;
            lg.w_cstr(msg);
        }
        if (!last_ok)
            break;
    }

    return last_ok;
}
