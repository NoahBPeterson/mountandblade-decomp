//-----------------------------------------------------------------------------
// File: script_interpreter.cpp  (one translation unit of the original: at least 0x49a0b0..0x4b1160; bodies in address order)
//
// Desc: the Module System operation interpreter and everything the original object holds around it:
//       - the slices' provisional views (namespaces c49aca0 .. c49f060): the text-list panel, the order / army records,
//         the view camera, the scene sprites. One namespace per slice keeps their same-named classes apart.
//       - GameData members (add_gold, remove_gold, start_quest, label_text).
//       - the validators 0x49cc60..0x49d4b0: the original passes their value pointer in ECX (most), EAX (check_party,
//         warn_tableau), EDI (warn_scene_prop_instance), EDX (warn_agent) or ECX+ESI (check_string_arg), i.e. cl's register
//         conventions for functions it can see. report and the four game-table checks are byte-exact (__fastcall, external).
//       - the operand accessors, the try linker and the Camera object (0x49d500..0x49f9e0).
//       - Operation::execute 0x49fa10: fetches the (up to 16) operand values, dispatches on the opcode (header_operations.py
//         numbering, `switch (op)` over 30..2385 with a 2356-entry jump table at 0x4ae458; 591 opcodes are implemented, the
//         rest report "Unrecognized opcode"), and applies the neg / this_or_next flags to the condition result. Case bodies
//         are in opcode order. Control opcodes below 30 are run by the block runner (DialogBlock::emit 0x4b09e0, below).
//       See NOTES.md. Flags: /O2 /MT /Gy /TP. Names provisional except the Module System ones.
//-----------------------------------------------------------------------------

#define RGL_VECTOR4_OPS_OUT_OF_LINE   // this object holds the out-of-line copy 0x407cd0 of vector4::operator* (defined below)
#include <math.h>
#include "../../include/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40; listed directly for the resolution audit)
#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): listed directly (the resolution audit reads only the source's own includes)
#include "../env_row.h"                // EnvRow, g_env_rows (0x5e28f4)
#include "../music_player.h"   // g_music (0xa73a10)
#include "../animator.h"      // Animator g_anim (0x8ed4f60)
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include <stdlib.h>
#include "./script_interpreter.h"
#include "../strat/gui_manager.h"          // gui_manager: g_gui
// the shared headers it builds on (included again here so their address annotations are read with this file)
#include "../dialog_manager.h"
#include "../agent/agent.h"
#include "../tstring.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../str_table.h"
#include "../../include/mat4.h"
#include "../../include/quaternion.h"
#include "../../include/timer.h"
#include "../../include/vec.h"
#include "../light.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../placed_obj.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../skeleton.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../slot_array.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../agent_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../item_ref.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_recs.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../blow.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../battle_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../int_vec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../prop_vec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mode_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../string_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../party_x.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/party.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../rgl/scene/world.h"      // rgl::world (the scene object at mission +0 and in g_script.level)
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128
#include "../../include/item_kind.h"   // g_item_kind_count 0x5e29d4
#include "../../include/image_addrs.h"
#include "../animator.h"      // Animator g_anim (0x8ed4f60)
#include "../../include/skin.h"      // Skin (g_anim.skin_vec.first)
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
#include "../party2/camera.h"
#include "../mesh_slot/mesh_build.h"   // build_appearance (0x482240), setup_item_meshes (0x47cfe0)
#include "../skeleton.h"                // node_groups / node_list (cap_all 0x438f90)
#include "../party2/camera_class.h"      // Camera (the one declaration of the 0x4a8-byte view object)
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
#define GD(T, off) (*(T*)((char*)this + (off)))    // a field of the game data that the shared GameData does not declare yet
#define GDP(T, off) (*(T*)((char*)g_game + (off)))
extern int g_propkind_count;                                // 0x5e29e0


#include "../shared_vars.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
Presentation* g_presentation;                               // 0x5e299c the running presentation









int g_visitor_scene;                                        // 0x5e2594 (modify_visitors_at_site 0x4a4b17)
float g_spawn_radius = 3.0f;                                // 0x5a9c18 (set_spawn_radius 0x4a2e0c)

#include "../animator.h"

#include "./script_globals.h"      // canonical declarations of the interpreter's globals (listed directly: the resolution audit reads only the source's own includes)

// ---- data this module owns (docs/standalone_data.md) ----
int g_cur_param;                                 // 0x5e2590 (.bss)
int g_lineno;                                    // 0x5e2598 (.bss) line of the statement being run
// 0x5e259c (.bss) set by report(); copied to *error at the end of execute. Every reference in the original is inside this
// translation unit's address range (0x49cc60..0x4b10d7): it is a file static there, which is what lets cl sink the store below
// the `id` loads in execute() (a static whose address is never taken cannot alias *this) and so sets execute's whole scratch
// register rotation.
static int g_script_error;
int g_cur_opcode;                                // 0x5e25a0 (.bss) opcode word of the statement being run
int g_or_latch;                                  // 0x5e25a4 (.bss) this_or_next chain already satisfied
#include "../ui_globals.h"      // UI / core globals
// ---- data this module owns (docs/standalone_data.md) ----

extern Font* g_font;                                // 0x5e2934

// ---- the canonical owners of the callees ---------------------------------------------------------------------------------------
// The namespaced views below (one namespace per slice) declare the engine functions they call under their own class names, which
// the linker cannot resolve: the mangled name has to carry the class that DEFINES the function. These are member-only stubs under
// the defining name (only their `// 0x...` callee is declared); the views forward to them with `this` cast, which compiles to the
// same call. Each comment names the defining source.
#include "../../include/vec_xlen.h"                                                // 0x43e2a0 std::vector<int>::_Xlen (agent3/agent_combat.cpp)
struct ForceLabel {
    void make_label(char* out);   // 0x48d940 mission_ui2/mission_3.cpp
};
// level::nearest_face 0x439a50 (agent2/agent_stats.cpp): terrain_b/cell_grid.h
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
struct WItem { void terrain_refresh(const float* pos); };   // 0x48f120 config/widget_mgr.h
struct Poser { void add_extra(int m); };   // 0x442110 world_data/mesh_hud.h
struct RecPool { unsigned alloc_record(); };   // 0x493700 (world_data.cpp; returns the new record id)
struct ColourOwner;
rgl::mesh* __stdcall build_legend_mesh(ColourOwner* parent);                               // 0x495b20 world_data/mesh_hud.cpp
namespace rgl { struct matrix; } typedef rgl::matrix pos;
__int64 __cdecl pick_slot_item(int troop, int slot, int mask, int n, int* cand, char remove, char* found);   // 0x46b190 terrain_a/terrain_a.cpp
struct CarryBody; struct CarryRef; struct CarryCtx;
void __cdecl place_scene_agents(CarryBody* holder, CarryRef* table, CarryCtx* ctx);   // 0x495430 world_data/world_data.cpp


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
    pos.x = -0.35f; pos.y = 0.0f; pos.z = 0.0f;
    scale[0] = 0.017f; scale[1] = 0.017f;
    g_font->layout_text(out, text, &pos, scale, &m1480, (int)&arg, &xmin, &xmax, 0x100);
    m1480.mask = 1;
    m1220.mask = 1;
}


// ======================================================================================
// slice s0049bea0 (namespace c49bea0)
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

namespace c49b630 {
using rgl::vector4;
using rgl::matrix;
using rgl::mesh;

// @ 0x49bbb0
void Mission::spawn_battle_icon(unsigned id)
{
    Rec98* rec = (Rec98*)((id & 0xf) * 0x98 + *(f0e0.begin() + (id >> 4)));
    EntityFull* e = (EntityFull*)g_script.level->create_entity(9);
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

    void add_amount(int a, unsigned b) { ((::Mission*)this)->add_amount(a, b); }                     // 0x4677a0 (terrain_a/terrain_a.cpp, ret 8)
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
    return (OrderObj*)g_script.level->create_entity(type);
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
    if (o->param() < 0.9f) {
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
        int f = r->flags;
        if (f & 1) { slot_b = 1; return; }
        if (f & 2) { slot_a = 1; return; }
        int b;
        int a = f & 0x40;
        if (((f & 0x40) == 0 && (f & 0x80) == 0) || (a != 0 && team_b == 0) || ((b = f & 0x80) != 0 && team_b == 1)) { slot_a = 1; return; }
        if ((a != 0 && team_b == 1) || (b != 0 && team_b == 0)) { slot_b = 1; return; }
        if ((a != 0 && team_a == 1) || (b != 0 && team_a == 0)) { slot_a = 1; return; }
        if ((a != 0 && team_a == 0) || (b != 0 && team_a == 1)) slot_b = 1;
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


struct Mission {
    rgl::world* level;                      // +0x00
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

static Agent* agent_by_id(Mission* m, unsigned id) {
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

    ordstr[0] = 0;

    if (order == 0) {                           // hold this position
        Leader* L = &leaders[index];
        if (L->agent_id != -1) {
            int f = agent_by_id(this, L->agent_id)->update_nav_face();
            vector4 zero = { 0, 0, 0 };
            set_vec(L, &agent_by_id(this, L->agent_id)->pos, &f, &zero);
            if (L->sub[0].mode) L->sub[0].state = 1;
            if (L->sub[1].mode) L->sub[1].state = 1;
            if (L->sub[2].mode) L->sub[2].state = 1;
            strcpy(ordstr, order_string("ui_hold_this_position_e_"));
        }
    } else if (order == 1) {                    // follow me
        Leader* L = &leaders[index];
        if (L->agent_id != -1) {
            if (L->sub[0].mode) L->sub[0].state = 2;
            if (L->sub[1].mode) L->sub[1].state = 2;
            if (L->sub[2].mode) L->sub[2].state = 2;
            int f = agent_by_id(this, L->agent_id)->update_nav_face();
            set_vec(L, &agent_by_id(this, L->agent_id)->pos, &f,
                    &agent_by_id(this, L->agent_id)->alt);
            strcpy(ordstr, order_string("ui_follow_me_e_"));
        }
    } else if (order == 2) {                    // charge
        Leader* L = &leaders[index];
        if (L->sub[0].mode) L->sub[0].state = 0;
        if (L->sub[1].mode) L->sub[1].state = 0;
        if (L->sub[2].mode) L->sub[2].state = 0;
        strcpy(ordstr, order_string("ui_charge_e_"));
    } else if (order == 11) {                   // stand ground
        Leader* L = &leaders[index];
        if (L->sub[0].mode) L->sub[0].state = 3;
        if (L->sub[1].mode) L->sub[1].state = 3;
        if (L->sub[2].mode) L->sub[2].state = 3;
        strcpy(ordstr, order_string("ui_stand_ground_e_"));
    } else if (order == 3) {                    // mount horses
        Leader* L = &leaders[index];
        set_v(L, 1);
        strcpy(ordstr, order_string("ui_mount_horses_e_"));
    } else if (order == 4) {                    // dismount
        Leader* L = &leaders[index];
        set_v(L, 2);
        strcpy(ordstr, order_string("ui_dismount_e_"));
    } else if (order == 7) {                    // stand closer
        Leader* L = &leaders[index];
        bump(L, -1);
        strcpy(ordstr, order_string("ui_stand_closer_e_"));
    } else if (order == 8) {                    // spread out
        Leader* L = &leaders[index];
        bump(L, 1);
        strcpy(ordstr, order_string("ui_spread_out_e_"));
    } else if (order == 5) {                    // advance
        Leader* L = &leaders[index];
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
            s->face = level->nearest_face(&p.x);
            s->extra.x = 0.0f;
            s->extra.y = 0.0f;
            s->extra.z = 0.0f;
            s->extra.w = 0.0f;
            s->state = 1;
        }
        strcpy(ordstr, order_string("ui_advance_e_"));
    } else if (order == 6) {                    // fall back
        Leader* L = &leaders[index];
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
            s->face = level->nearest_face(&p.x);
            s->extra.x = 0.0f;
            s->extra.y = 0.0f;
            s->extra.z = 0.0f;
            s->extra.w = 0.0f;
            s->state = 1;
        }
        strcpy(ordstr, order_string("ui_fall_back_e_"));
    } else if (order == 9) {                    // use blunt weapons
        set_weapon_order_1(index, 1);
        strcpy(ordstr, order_string("ui_use_blunt_weapons_e_"));
    } else if (order == 10) {                   // use any weapon
        set_weapon_order_1(index, 0);
        strcpy(ordstr, order_string("ui_use_any_weapon_e_"));
    } else if (order == 12) {                   // hold fire
        set_weapon_order_2(index, 1);
        strcpy(ordstr, order_string("ui_hold_fire_e_"));
    } else if (order == 13) {                   // fire at will
        set_weapon_order_2(index, 0);
        strcpy(ordstr, order_string("ui_fire_at_will_e_"));
    }

    // inform the local agent, but only about its own order group
    if (local_id >= 0) {
        Agent* la = agent_by_id(this, local_id);
        int g = la->slot_no;
        if (g < 0 || g >= 8)
            g = 7;
        if (g == index) {
            make_label(&leaders[index], ordtxt);
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
// by-reference helpers, validators (0x49cc60..0x49d4b0)
// ======================================================================================

// ==== by-reference helpers with their bodies in this object ================================================
// The original's interpreter sees these bodies (header inlines its inliner left out of line in the 70 KB execute()):
// a temporary or local whose address is passed to a callee cl can see is known not to escape, so it shares the
// low frame slots (0x10/0x20/0x24...) with the validator arguments. Passed to an opaque callee, every such object
// keeps a private slot for the whole function (frame 0x24cc -> 0x22d0, params back at 0x2c). The canonical
// definitions (and their manifest rows) stay in their own modules; tools/relink/build.py takes each duplicate
// from its row's module.
__declspec(noinline) int __cdecl clamp_index(const int* v, const int* lo, const int* hi)
{
    if (*v < *lo) return *lo;
    if (*v >= *hi) return *hi - 1;
    return *v;
}
__declspec(noinline) int __cdecl round_float(float* p)
{
    float fl = (float)floor((double)*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}
__declspec(noinline) int __cdecl rand_below(const int* n)
{
    if (*n <= 1) return 0;
    return rand() % *n;
}
__declspec(noinline) __int64 __cdecl round_i64(const float* p)
{
    float fl = (float)floor((double)*p);
    long long i = (long long)fl;
    if (!(*p - fl <= 0.5f))
        i += 1;
    return i;
}
__declspec(noinline) rgl::vector4& rgl::vector4::set(const float& x_, const float& y_, const float& z_) { x = x_; y = y_; z = z_; return *this; }
__declspec(noinline) rgl::vector4 rgl::vector4::operator*(const float& s) const { return rgl::vector4(x * s, y * s, z * s); }
inline __declspec(noinline) rgl::vector4 rgl::operator*(const float& s, const rgl::vector4& v) { return rgl::vector4(s * v.x, s * v.y, s * v.z); }

__declspec(noinline) int __cdecl imax_ref(const int* a, const int* b) { return *a > *b ? *a : *b; }
__declspec(noinline) float __cdecl fmin_ref(const float* a, const float* b) { return *a < *b ? *a : *b; }
__declspec(noinline) float __cdecl clamp(const float* x, const float* lo, const float* hi)
{
    return *x < *lo ? *lo : (*x >= *hi ? *hi : *x);
}
struct I64VecW : I64Vec { void reserve(int* _pNewsize); };   // world_data.h has the full class; same layout, no inline bodies (no COMDAT folding risk)
__declspec(noinline) __int64 SlotArray::get(const int& slot)
{
    I64Vec* v = (I64Vec*)this;
    unsigned i = slot;
    if ((int)i < 0 || !v->_Myfirst || i >= (unsigned)(v->_Mylast - v->_Myfirst))
        return 0;
    return v->_Myfirst[i];
}

// ==== error reporting and argument validators (static, same object) =========================================

// @ 0x49cc60
// "%s; LINE NO: %d:" to the on-screen message log and the text log, then raise the error flag.
static __declspec(noinline) void report(const char* msg)
{
    char buf[1024];
    sprintf(buf, "%s; LINE NO: %d:", msg, g_lineno);
    g_gui.add(tstring(buf), 0xffff5555);
    stream_base& lg = g_log;
    lg.w_cstr(buf);
    g_script_error = 1;
}

// @ 0x49ccc0
char __fastcall check_script_param(int* value, int* limit)
{
    char buf[512];
    int v = *value;
    char r = 1;
    if (v < 0 || v >= *limit) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Script Parameter ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49cd00
static __declspec(noinline) char check_register(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= 0x80) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Register ID: %d", g_cur_opcode, value);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49cd50
static __declspec(noinline) char check_presentation(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= g_presentations.count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Presentation ID: %d", g_cur_opcode, value);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49cda0
// String operand: its type selects the table: 3 = string (s_*), 0x16 = quick string, 0 = string register.
// (sequential tests of the reference itself, with the error paths leaving: type stays in ECX, its value in EDX, value in ESI)
static __declspec(noinline) char check_string_arg(const int& value, const int& type)
{
    char buf[512];
    char r = 1;
    if (type == 3) {
        if (value < 0 || value >= g_strings.count) {
            sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid String ID: %d", g_cur_opcode, value);
            report(buf);
            r = 0;
            goto done;
        }
    }
    if (type == 0x16) {
        if (value < 0 || value >= g_strings.quick_count) {
            sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Quick String ID: %d", g_cur_opcode, value);
            report(buf);
            r = 0;
            goto done;
        }
    }
    if (type == 0) {
        if (value < 0 || value >= 0x80) {
            sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid String Register ID: %d", g_cur_opcode, value);
            report(buf);
            r = 0;
            goto done;
        }
    }
done:
    return r;
}

// Range validators: statics; cl gives value the ECX register convention when the body reads value directly (a local `int v = value` moves it to EAX and rotates the scratch registers)
// @ 0x49ce40
static __declspec(noinline) char check_mesh(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= g_meshes.count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Mesh ID: %d", g_cur_opcode, value);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49ce90
static __declspec(noinline) char check_menu(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= g_menu_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Menu ID: %d", g_cur_opcode, value);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49cee0
__declspec(noinline) char __fastcall check_troop(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= game()->num_troops) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Troop ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49cf30
__declspec(noinline) char __fastcall check_quest(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= game()->num_quests) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Quest ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49cf80
__declspec(noinline) char __fastcall check_faction(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= game()->num_factions) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Faction ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49cfd0
static __declspec(noinline) char check_item_kind(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= g_item_kind_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Item Kind ID: %d", g_cur_opcode, value);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49d020
__declspec(noinline) char __fastcall check_scene(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= game()->num_scenes) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Scene ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49d070
static __declspec(noinline) char check_mission_template(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= g_mission_template_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Mission Template ID: %d", g_cur_opcode, value);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49d0c0
static __declspec(noinline) char check_party_template(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= g_party_template_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Party Template ID: %d", g_cur_opcode, value);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49d110
static __declspec(noinline) char check_map_icon(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= g_map_icon_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Map Icon ID: %d", g_cur_opcode, value);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49d160
// Party id: must be a live entry of the party table (record +4 == 1). The original takes the pointer in EAX.
static __declspec(noinline) char check_party(const int& value)
{
    char buf[512];
    PartyMgr* t = &g_game->parties;
    unsigned id = value;
    char r = 1;
    int idx;
    if (id >= (unsigned)t->deflt || (idx = t->lookup(id)) < 0 || t->atu(idx)->type != 1) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Party ID: %d", g_cur_opcode, id);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49d1f0
static __declspec(noinline) char warn_particle_system(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= g_emit_defs.count) {
        sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid particle_system ID: %d", g_cur_opcode, value);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49d240
static __declspec(noinline) char warn_scene_prop(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= g_scene_prop_count) {
        sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Scene_prop ID: %d", g_cur_opcode, value);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49d290
// Scene prop instance: the mission must exist and the id must index its 0x1c8-byte prop records (pointer in EDI).
static __declspec(noinline) char warn_scene_prop_instance(const int& value)
{
    char buf[512];
    char r = 1;
    if (!mission() || value < 0 || value >= mission()->props.size()) {
        sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Scene_prop Instance ID: %d", g_cur_opcode, value);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49d310
static __declspec(noinline) char warn_sound(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= g_sound_mgr.ngroups) {
        if (g_cfg.disable_sound == 0) {
            sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Sound ID: %d", g_cur_opcode, value);
            report(buf);
        }
        r = 0;
    }
    return r;
}

// @ 0x49d360
// Tableau: the id must index the 0x68-byte tableau records (pointer in EAX).
static __declspec(noinline) char warn_tableau(const int& value)
{
    char buf[512];
    int id = value;
    char r = 1;
    if (id < 0 || g_tableaus.recs._Myfirst == 0 || (unsigned)id >= (unsigned)g_tableaus.recs.size()) {   // jb: unsigned, as the original
        sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Tableau ID: %d", g_cur_opcode, id);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49d3d0
static __declspec(noinline) char warn_track(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= g_music.track_count) {
        if (g_cfg.disable_music == 0 && g_cfg.disable_sound == 0) {
            sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Track ID: %d", g_cur_opcode, value);
            report(buf);
        }
        r = 0;
    }
    return r;
}

// @ 0x49d430
// Agent: the mission must exist and the id must be a live entry of its agent table (pointer in EDX).
static __declspec(noinline) char warn_agent(const int& value)
{
    char buf[512];
    char r = 0;
    int id;
    unsigned u;
    if (g_mission && (id = value) >= 0 && id < mission()->agents.count &&
        (u = id, mission()->agents.pages[u >> 4][u & 0xf].live != 0)) {
        r = 1;
    } else {
        sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Agent ID: %d", g_cur_opcode, value);
        report(buf);
    }
    return r;
}

// @ 0x49d4b0
static __declspec(noinline) char check_animation(const int& value)
{
    char buf[512];
    char r = 1;
    if (value < 0 || value >= g_anim_set.count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Animation ID: %d", g_cur_opcode, value);
        report(buf);
        r = 0;
    }
    return r;
}

__declspec(noinline) void SlotArray::set(const int& slot, const __int64& value)
{
    int _n = slot + 1;
    ((I64VecW*)(void*)this)->reserve(&_n);
    ((I64Vec*)this)->_Myfirst[slot] = value;
}
__declspec(noinline) void NamedSelRec::set_sel(const int* src)
{
    int v = *src;
    if (v < 0 || v >= 8)
        return;
    *(int*)((char*)this + 0x1c4) = v;
}
__declspec(noinline) int Troop::nibble(int i)
{
    int v = (nib[i / 8] >> ((i % 8) * 4)) & 0xf;
    if (v < 0) return 0;
    int cap = g_skill_tbl[i].max_rank;
    if (v > cap) v = cap;
    return v;
}
__declspec(noinline) int Troop::inv_count()
{
    if (!(flags30 & 0x10)) return 0x5a;
    switch (nibble(12)) {
    case 0: return 0x1e;
    case 1: return 0x24;
    case 2: return 0x2a;
    case 3: return 0x30;
    case 4: return 0x36;
    case 5: return 0x3c;
    case 6: return 0x42;
    case 7: return 0x48;
    case 8: return 0x4e;
    case 9: return 0x54;
    case 10: return 0x60;
    default: return 0x60;
    }
}
__declspec(noinline) int Mission::team_count()
{
    int n = 0;
    if (0 < counts[0][1]) n = 1;
    if (0 < counts[1][1]) n++;
    if (0 < counts[2][1]) n++;
    if (0 < counts[3][1]) n++;
    if (0 < counts[4][1]) n++;
    if (0 < counts[5][1]) n++;
    if (0 < counts[6][1]) n++;
    if (0 < counts[7][1]) n++;
    return n;
}
__declspec(noinline) int Party::first_flagged()
{
    int r = 0;
    int i = 0;
    if (0 < nstacks) {
        Stack* p = stacks;
        do {
            if ((p->flags & 1) != 0)
                return r;
            r++;
            i++;
            p++;
        } while (i < nstacks);
    }
    return r;
}
__declspec(noinline) int Party::count_tail()
{
    int n = nstacks;
    int lead = 0;
    int i = 0;
    if (n > 0) {
        char* q = (char*)stacks + 8;
        do {
            if (*q & 1)
                break;
            lead++;
            i++;
            q += 0x20;
        } while (i < n);
    }
    return n - lead;
}
__declspec(noinline) int Party::sum_head()
{
    int s = 0;
    int i = 0;
    int n = nstacks;
    if (n > 0) {
        Stack* p = stacks;
        do {
            if (p->flags & 1)
                return s;
            s += p->count;
            i++;
            p++;
        } while (i < n);
    }
    return s;
}
__declspec(noinline) int Party::sum_flagged()
{
    int s = 0;
    int n = nstacks;
    if (n > 0) {
        Stack* p = stacks;
        do {
            if (p->flags & 1)
                s += p->count;
            p++;
        } while (--n);
    }
    return s;
}
__declspec(noinline) int Party::sum_all()
{
    int s = 0;
    int n = nstacks;
    if (n > 0) {
        Stack* p = stacks;
        do {
            s += p->count;
            p++;
        } while (--n);
    }
    return s;
}
__declspec(noinline) int PartyMgr::is_live(int id)
{
    if ((unsigned)id < n_parties) {
        int i = this->lookup(id);
        if (i >= 0)
            return pages[(unsigned)i >> 4][i & 0xf].type == 1;
    }
    return false;
}
__declspec(noinline) TabCurrent* TableauTbl::current()
{
    return (TabCurrent*)worlds[worlds.size() - 1];
}
__declspec(noinline) void PartyMgr::next_party_id(int* id)
{
    unsigned i = lookup(*id) + 1;
    while ((int)i < count && pages._Myfirst[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        *id = deflt;
    else
        *id = pages._Myfirst[i >> 4][i & 0xf].id;
}
__declspec(noinline) void AgentTable::next_used(unsigned* i)
{
    ++*i;
    while ((int)*i < count) {
        unsigned v = *i;
        if (pages[v >> 4][v & 0xf].used != 0) break;
        ++*i;
    }
}
__declspec(noinline) Agent* AgentTable::get(unsigned id)
{
    return &pages[id >> 4][id & 0xf];
}
__declspec(noinline) int min_int(const int& a, const int& b) { return a < b ? a : b; }
// more callees whose bodies the object sees: their by-pointer arguments share the low frame slots
struct TabCurrent { char pad0[0x36c]; rgl::frustum fr; int background; float ambient[4]; };   // as mission_ui2/mission_4.h
static __forceinline vector4 vis_mul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 vis_add(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 vis_sub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
__declspec(noinline) void rgl::matrix::rotate_about_s(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 nf, nu;
    nf = vis_add(vis_mul(f, cos((double)*angle)), vis_mul(u, sn));
    nu = vis_sub(vis_mul(u, cos((double)*angle)), vis_mul(f, sn));
    u = nu;
    f = nf;
}
__declspec(noinline) void rgl::matrix::rotate_about_f(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 ns, nu;
    ns = vis_sub(vis_mul(s, cos((double)*angle)), vis_mul(u, sn));
    nu = vis_add(vis_mul(u, cos((double)*angle)), vis_mul(s, sn));
    s = ns;
    u = nu;
}
__declspec(noinline) void rgl::matrix::rotate_about_u(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 ns, nf;
    ns = vis_add(vis_mul(s, cos((double)*angle)), vis_mul(f, sn));
    nf = vis_sub(vis_mul(f, cos((double)*angle)), vis_mul(s, sn));
    s = ns;
    f = nf;
}
__declspec(noinline) void Sel::set_vec(const int* a, const int* b, const int* c)
{
    struct B { int d[4]; };
    struct R { char p0[0x18]; B a; B c; int b; char p3c[0x4c - 0x3c]; int present; char p50[0x6c - 0x50]; };
    R* rec = (R*)this;
    if (rec[0].present) { rec[0].a = *(const B*)a; rec[0].b = *b; rec[0].c = *(const B*)c; }
    if (rec[1].present) { rec[1].a = *(const B*)a; rec[1].b = *b; rec[1].c = *(const B*)c; }
    if (rec[2].present) { rec[2].a = *(const B*)a; rec[2].b = *b; rec[2].c = *(const B*)c; }
}
__declspec(noinline) void rgl::world::set_fog(const float* ref, const float* rate, const int* color)
{
    view.fog_dirty = 1;
    view.fog_ref = *ref;
    view.fog_rate = *rate;
    view.fog_color = *color;
    float f = (1.0f / view.fog_rate) * 3.0f;
    if (f < 0.001f) {
        view.fog_blend = 0.001f;
        return;
    }
    if (f >= 0.95f) {
        view.fog_blend = 0.95f;
        return;
    }
    view.fog_blend = f;
}
__declspec(noinline) rgl::psys* EmitTable::spawn(int* idx, const vector4* pos, const int* owner)
{
    if (*idx < 0 || *idx >= count)
        return 0;
    rgl::psys* e = new rgl::psys(*owner);
    setup(e, *idx, pos);
    return e;
}
__declspec(noinline) void TableauTbl::set_camera_parameters(const char* perspective, const float* w, const float* h, const float* near_, const float* far_)
{
    float fz = *far_;
    float nz = *near_;
    float fh = *h;
    float fh2 = *h;
    float fw = *w;
    float fw2 = *w;
    TabCurrent* c = (TabCurrent*)worlds[worlds.size() - 1];
    c->fr.perspective = *perspective;
    c->fr.left = fw * -0.5f;
    c->fr.right = fw2 * 0.5f;
    c->fr.top = fh * 0.5f;
    c->fr.bottom = fh2 * -0.5f;
    c->fr.nearz = nz;
    c->fr.farz = fz;
    c->fr.build_planes();
}
extern const int g_value_max;   // 0x590a9c (agent/agent.cpp)
struct Prop { void update(int x); };   // 0x467c50 (ui/mission.h)
// Bodies this object sees so that execute's temporaries pack as in the original: a local whose address reaches a callee cl cannot see
// keeps a private frame slot (cl assumes the callee may keep the pointer), and that holds through a chain, so the whole chain has to be
// visible. unpack_color: cur_tableau_add_mesh*'s colour. MissionView::setup: prop_instance_animate_to_position's duration and zero.
// ItemRef + Mission::add_placed + init_placed: spawn_item's ItemRef. Duplicates of the canonical rows (see NOTES.md, "What the object can see").
__declspec(noinline) rgl::vector4 rgl::unpack_color(const unsigned* c)
{
    rgl::vector4 v;
    const unsigned char* b = (const unsigned char*)c;
    v.w = b[3] * (1.0f / 255.0f);
    v.x = b[2] * (1.0f / 255.0f);
    v.y = b[1] * (1.0f / 255.0f);
    v.z = (*c & 0xff) * (1.0f / 255.0f);
    return v;
}
__declspec(noinline) int ItemRef::stat_1e8()
{
    if (item >= 0) {
        int v = g_items[item].f1e8;
        if ((char)g_items[item].type_flags == itp_type_horse) {
            if (v == 0) v = 100;
        } else if (v == 0)
            return v;
        switch (b.mod) {
        case imod_cracked: v -= 0x38; break;
        case imod_battered: v -= 0x1a; break;
        case 0x19: v += 0x2f; break;
        case 0x1b: v += 0x53; break;
        case 0x1d: v += 0x9b; break;
        case imod_heavy: v += 10; break;
        case 0x20: v += 5; break;
        }
        if (v >= 1) return v;
    }
    return 1;
}
__declspec(noinline) void ItemRef::reduce(int n)
{
    if (item > 0) {
        unsigned t = g_items[item].type_flags & 0xff;
        if (t == itp_type_thrown || t == itp_type_arrows || t == itp_type_bolts || t == itp_type_pistol ||
            t == itp_type_musket || t == itp_type_crossbow || t == itp_type_bullets) {
            if (n <= 0) {
                b.amount = 0;
                return;
            }
            unsigned v = (m4 & 0xff) - n;
            b.amount = 0;
            m4 = m4 | (((int)v < 0) - 1 & v);
        } else if (t == itp_type_shield || t == itp_type_horse || t == itp_type_animal) {
            unsigned v = stat_1e8();
            *(short*)&m4 = 0;
            m4 |= v;
        }
    }
}
__declspec(noinline) void ItemRef::set_modifier(int k)
{
    if (k >= 0 && k < 0x2b) {
        ((char*)this)[7] = 0;
        m4 |= k << 24;
        reduce(0);
    }
}
__declspec(noinline) void __stdcall init_placed(PlacedObj* o, const int* kind, const int* id, const int* value, const rgl::matrix* m)
{
    tstring name;
    o->reset();
    int k = *kind;
    if (k == 2 || k == 6 || k == 7) {
        name = g_items[*id].key;
        o->value = *value;
    } else if (k == 4) {
        name = g_ptypes[*id].name;
        o->value = *value;
    } else if (k == 0) {
        name = g_propkinds[*id].name;
    } else {
        int zero = 0;
        o->value = clamp_index(value, &zero, &g_value_max);
    }
    o->m18 = *m;
    o->m98 = *m;
    o->kind = *kind;
    o->id = *id;
    o->name = name;
}
__declspec(noinline) int Mission::add_placed(const int* kind, const int* id, const int* value, const rgl::matrix* m, int* spawn_id)
{
    int cnt = 0;
    int prev = -1;
    int idx = (int)props.size();
    {
        PlacedObj rec;                                            // its inlined constructor calls reset (0x4352f0, defined as PlacedObj::reset in agent.cpp): shared rec1c8.h
        ((std::vector<PlacedObj>&)props).push_back(rec);       // 0x440b80 (the explicit instantiation of agent_combat.cpp)
        int off = idx * 0x1c8;
        ::init_placed((PlacedObj*)((char*)&props[0] + off), kind, id, value, m);
        if (*kind == 7 || *kind == 6)
            ((PlacedObj*)((char*)&props[0] + off))->m10 = 2;
        if (*kind == 1) {
            cnt = 0;
            prev = -1;
            for (int k = 0; k < 0x28; k++) {
                if (prev >= cnt)
                    break;
                prev = cnt;
                for (unsigned j = 0; j < props.size(); j++) {
                    if (((PlacedObj&)props[j]).kind == 1 && ((PlacedObj&)props[j]).value == cnt)
                        cnt++;
                }
            }
            if (cnt < 0x40)
                ((PlacedObj*)((char*)&props[0] + off))->value = cnt;
        }
        spawn_prop(&idx, spawn_id);
    }
    return idx;
}
__declspec(noinline) void MissionView::setup(void* src, int* b, int* c)
{
    char* t = (char*)this;
    ((Prop*)this)->update(0);
    memcpy(t + 0x118, t + 0x18, 0x40);
    memcpy(t + 0xd8, src, 0x40);
    *(int*)(t + 0x168) = *c;
    *(int*)(t + 0x16c) = *b;
    ((Timer*)(t + 0x170))->restamp();
    *(int*)(t + 0x180) = 0;
}
// TableauTbl::spawn_scaled / spawn_scaled2 (0x499390, 0x499540; world_data/obj_tbl.cpp owns the rows): visible duplicates. With them
// execute's cur_tableau_add_* cases share the validator temporaries as in the original (24 more identical cases, frame 0x10 smaller).
struct TableauEntityV { void* vptr; char pad4[0x58 - 4]; rgl::matrix local; };
__declspec(noinline) void TableauTbl::spawn_scaled(int index, const rgl::matrix* frame, const rgl::vector4* scale,
                              const float* alpha, const unsigned* color)
{
    TableauEntityV* e = (TableauEntityV*)((rgl::world*)(void*)worlds[worlds.size() - 1])->create_entity(9);
    rgl::mesh* m = g_meshes.placed(index);
    if (!(0.99999f <= scale->x && scale->x < 1.00001f
       && 0.99999f <= scale->y && scale->y < 1.00001f
       && 0.99999f <= scale->z && scale->z < 1.00001f))
        m->set_color(*scale);
    if (*color != 0xffffffffu) {
        rgl::vector4 c = rgl::unpack_color(color);
        if (g_use_shaders) {
            c.x = pow((double)c.x, (double)2.2f);
            c.y = pow((double)c.y, (double)2.2f);
            c.z = pow((double)c.z, (double)2.2f);
        }
        ((rgl::mesh*)(void*)m)->set_bone_index((int)rgl::pack_color(&c));
    }
    if (*alpha < 0.99f) {
        float a;
        if (*alpha < 0.0f)
            a = 0.0f;
        else if (*alpha >= 1.0f)
            a = 1.0f;
        else
            a = *alpha;
        m->set_alpha(a);
    }
    ((Poser*)(void*)e)->add_extra((int)m);
    e->local = *frame;
}
__declspec(noinline) void TableauTbl::spawn_scaled2(int index, const rgl::matrix* frame, const float* scale)
{
    TableauEntityV* e = (TableauEntityV*)((rgl::world*)(void*)worlds[worlds.size() - 1])->create_entity(9);
    rgl::mesh* m = (((rgl::mesh*)g_resmgr.get_object((rgl::string*)((char*)g_map_icons + index * 0x48 + 0x14))))->copy_of();
    if (!(0.99999f <= *scale && *scale < 1.00001f))
        m->scale(*scale);
    ((Poser*)(void*)e)->add_extra((int)m);
    e->local = *frame;
}
// PartyMgr::get and the id lookup: visible here too, as header inlines in the original. cl then knows the calls keep the
// operands in registers (esi = params[0] low, ebx = params[1] low across PartyMgr::get, 0x49fb9f..).
inline int Map10007::lookup(unsigned id)
{
    if (id < 0x2717)
        return tab[id].value;
    int h = id % 0x2717;
    int n = 0;
    if (tab[h].key != id) {
        do {
            if (n >= 1000)
                return -1;
            h = (h + 0x4f) % 0x2717;
            n++;
        } while (tab[h].key != id);
    }
    if (n < 1000)
        return tab[h].value;
    return -1;
}
__declspec(noinline) Party* PartyMgr::get(int id)
{
    int k = this->lookup(id);
    return (Party*)((k & 0xf) * 0x5ac8 + (int)pages._Myfirst[k >> 4]);
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
        if (check_party(locals)) {
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
    return (v - fl <= 0.5f) ? i : i + 1;
}

// this constructor expands the container constructor (0x49deb7); the other sites call 0x4931a0
inline ContainerBase::ContainerBase() {}

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

static inline float2 operator*(const float2& a, float k) { return float2(a.x * k, a.y * k); }
static inline float track_elapsed(const Timer& t) { return (float)(g_ticks[t.clock] - t.start) * 1e-5f; }
static inline float2 fade_mix(float s, const float2& a, float t, const float2& b)
{
    float2 l(s * a.x, s * a.y);
    float2 r(t * b.x, t * b.y);
    return float2(l.x + r.x, l.y + r.y);
}

// @ 0x49dfe0
// Per-frame view update: the two shake axes follow the tracked object while its follow objects report
// a weight, then the four fades (zoom, colour, level, shake) are advanced.
void Camera::animate(int)
{
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

    float d0 = track_elapsed(*(const Timer*)((char*)this + 8));         // colour
    float d1 = track_elapsed(*(const Timer*)((char*)this + 0x18));      // level
    float d4 = track_elapsed(*(const Timer*)((char*)this + 0x48));      // zoom
    float d5 = track_elapsed(*(const Timer*)((char*)this + 0x58));      // shake

    // zoom: blend the target's zoom from the near weights toward the far ones (track flag c4, as 0x4886ab sets it)
    if (d4 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - d4;
        float2 z = fade_mix(d4, *(float2*)&f88, rest, *(float2*)&a8) * inv;
        float2 v = z * 0.02f;
        target->kids[0]->set_size((float*)&v);
        fD8 = z.x;
        fDC = z.y;
        ((FieldPanel*)this)->update();
    } else if (c4) {
        c4 = 0;
        ((WItem*)this)->terrain_refresh(&f88);
    }

    // colour: the three level bytes blended from the target levels and packed under the inherited
    // alpha byte, then the target sub-object's colour slot
    if (d0 < dur(0)) {
        int c = blend_u(dur(0), d0, srcA & 0xff, valA & 0xff);
        c = (blend_i(dur(0), d0, ((unsigned char*)&srcA)[1], ((unsigned char*)&valA)[1]) << 8) + c;
        c = (blend_i(dur(0), d0, ((unsigned char*)&srcA)[2], ((unsigned char*)&valA)[2]) << 16) + c;
        unsigned kid_col = target->kids[0]->color14c;
        unsigned col = color14c;
        col |= kid_col;
        col &= 0xff000000u;
        col |= c;
        target->kids[0]->set_color(&col);
        float2 v;
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

    // shake: blend toward the far weights and hand the offset to the object (track flag c5, as 0x48870b sets it)
    if (d5 < dur(5)) {
        float inv = g_one / dur(5);
        float rest = dur(5) - d5;
        float2 v = fade_mix(d5, *(float2*)&f80, rest, *(float2*)&a0) * inv;
        move_a((float*)&v);
        ((FieldPanel*)this)->update();
    } else if (c5) {
        move_a(&f80);
        ((FieldPanel*)this)->update();
        c5 = 0;
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
    float2 v;
    v.x = 0;
    v.y = 0;
    o->set_pos(&v);                                      // +0x58
    int attr = (flags & 0x381c) | 0x700;
    o->set_attr(&attr);                                  // +0x74
    v.x = 0.9f;
    v.y = 0.75f;
    o->set_rot(&v);                                      // +0x64
    o->attach((void*)&v, (void*)arg);                    // +0xa4 (the original passes &r first, then arg)
    if (flags & 0x2000) {
        scene_obj* ps = (scene_obj*)operator new(0x188);
        if (ps)
            ps = ps_ctor(ps);                            // 0x48b890
        else
            ps = 0;
        *(int*)((char*)ps + 0x178) = 1;
        v.x = 0;
        v.y = 0;
        ps->set_pos(&v);                                 // +0x58
        this->list->add_child(ps);                       // +0xc8
        scene_obj* cam = (scene_obj*)operator new(0x4a8);
        if (cam)
            cam = cam_ctor(cam);                         // 0x49deb0
        else
            cam = 0;
        *(int*)((char*)cam + 0x104) |= 0x20;
        *(scene_obj**)((char*)cam + 0x484) = ps;
        scene_obj* grp = (scene_obj*)new ContainerBase;
        v.x = 0;
        v.y = 0;
        grp->set_pos(&v);                                // +0x58
        grp->set_rot((float2*)((char*)o + 0xf0));        // +0x64
        grp->add_child(o);                               // +0xc8
        *(scene_obj**)((char*)cam + 0x490) = grp;
        cam->add_child(grp);                             // +0xc8
        v.x = 0;
        v.y = 0;
        cam->set_pos(&v);                                // +0x58
        v.x = *(float*)((char*)o + 0xf0);
        v.y = 0.1f;
        ((scene_api*)cam)->set_pos(&v);                  // 0x498420
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
        count_units(a, &d[0], &d[3], &d[2], &d[1]);       // 0x490ce0
        battle_rec(this, id)->f78 += d[0];
    }
    if (b >= 0) {
        int d[4] = { 0, 0, 0, 0 };
        count_units(b, &d[0], &d[3], &d[2], &d[1]);       // 0x490ce0
        battle_rec(this, id)->f78 += d[0];
    }
    parties.get_w(a)->action = 7;
    parties.get_w(b)->action = 7;
    parties.get_w(a)->f88 = id;
    parties.get_w(b)->f88 = id;
    ((c49b630::Mission*)this)->spawn_battle_icon(id);          // 0x49bbb0
    return id;
}

// @ 0x49ebd0
// Add a party of party template `tmpl` at (x, y): the last party record is filled from the template record, named after
// it, randomly rotated, and given a fixture-following behaviour.
int GameData::add_sprite(float x, float y, int tmpl)
{
    PartyTemplate* tr = &g_party_templates[tmpl];       // 0x5e29f8 + 0xb0 * tmpl
    int id = parties.add_slot();         // 0x493960
    parties.get_w(id)->init();                       // 0x48f180
    parties.get_w(id)->template_id = tmpl;
    parties.get_w(id)->pos.x = x;
    parties.get_w(id)->pos.y = y;
    parties.get_w(id)->dirx = 1.0f;
    parties.get_w(id)->diry = 0;
    float ang = (float)(rand() % 0x3dc9) * 0.00039724255f; // 0x536a9f / 0x54a6e0
    float c = (float)cos(ang);
    float s = (float)sin(ang);
    Party* ro = parties.get_w(id);
    float oy = ro->diry;
    float ox = ro->dirx;
    ro->diry = c * oy + s * ox;
    ro->dirx = c * ox - s * oy;
    tstring* n = g_strtable.find(&tr->key);     // 0x4c6010
    if (n == 0)
        n = &tr->name;
    parties.get_w(id)->name = *n;
    parties.get_w(id)->flag = 1;
    parties.get_w(id)->key = tr->key;
    parties.get_w(id)->flags_hi = tr->f24;
    parties.get_w(id)->flags_lo = tr->f20;
    parties.get_w(id)->f90 = tr->f28;
    parties.get_w(id)->f70 = tr->f2c;
    parties.get_w(id)->f74 = tr->f30;
    parties.get_w(id)->state = 2;
    parties.get_w(id)->ai_target_x = x;
    parties.get_w(id)->ai_target_y = y;
    add_template(id, tmpl, 0);                            // 0x48fbd0
    ((c49bea0::MissionAgents*)this)->add_order_arrow(id);  // 0x49bea0
    tr->num_created += 1;
    return id;
}

// @ 0x49ef00
// Add the party for troop `troop` at (x, y): named after the troop (its translation when the string table has one).
struct GameCompanion : GameData { int create_companion_party(int troop, rgl::vector4 pos); };   // game_data.h declares 3 floats; ret 0x14: the vector4 is by value
int GameCompanion::create_companion_party(int troop, rgl::vector4 pos)
{
    int id = add_sprite(pos.x, pos.y, 3);                // 0x49ebd0
    tstring* n = g_strtable.find(&chars[troop].key);     // 0x4c6010
    if (n == 0)
        n = &chars[troop].key;
    parties.get_w(id)->name = *n;
    parties.get_w(id)->flag = 1;
    Party* p = parties.get_w(id);
    p->f70 = chars[troop].faction;
    p->add_members((int)n, 1, 0, 0, 0);            // 0x48c840
    return id;
}

struct GameWpx { void n(float x, float y, float radius, float* out); };   // 0x490030 (the pair-as-two-floats view of GameW::random_position_around)

// @ 0x49f020
// Pick a position around (x, y) (x and y are adjusted in place) and add a party of template `tmpl` there.
int GameData::spawn_around(float x, float y, int tmpl, float radius)
{
    ((GameWpx*)this)->n(x, y, radius, &x);                     // 0x490030
    return add_sprite(((float*)&x)[0], ((float*)&x)[1], tmpl); // 0x49ebd0
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
                              ? (void*)g_anim.skin_vec.first[t].skeleton : 0));
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
    if (parties.get_w(lead)->f88 >= 0) {
        if (a == party_id_78) {
            attach_party = agent_rec(this, parties.get_w(lead)->f88)->f58;
            attach_scene = agent_rec(this, parties.get_w(lead)->f88)->f54;
            attach_flip = 0;
            parties.get_w(a)->f88 = parties.get_w(lead)->f88;
            return 1;
        }
        Rec98* nd = agent_rec(this, parties.get_w(lead)->f88);
        int x = nd->f58;
        if (x == lead)
            x = nd->f54;
        if (relation(x, a) < -0.01f)
            return 0;
        ((GameW*)this)->set_objective(a, x);
        parties.get(a)->f8cc = 1;
        return 1;
    }
    if (a == party_id_78) {
        lead = battle_root(b);
        attach_party = lead;
        attach_scene = -100000;
        attach_flip = 0;
        ::Party* pl = parties.get_w(lead);
        Party* pa = parties.get_w(a);
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
        Party* p1 = parties.get(la);
        Party* p2 = parties.get(b);
        attach_center(this, (p1->pos.x + p2->pos.x) * 0.5f, (p2->pos.y + p1->pos.y) * 0.5f, b, la);
        return 1;
    }
    Party* p1 = parties.get(a);
    Party* p2 = parties.get(lead);
    attach_center(this, (p2->pos.x + p1->pos.x) * 0.5f, (p2->pos.y + p1->pos.y) * 0.5f, a, lead);
    return 1;
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
    return create((const rgl::matrix*)g_game->trigger_frame, (const ItemStackG*)&s, 1);
}

// @ 0x49f9e0
int Mission::exec_trigger(int val)
{
    char out[0x20];
    return spawn_full((const rgl::matrix*)g_game->trigger_frame, val, -1, 0, 0, -1, 0, 0, 0, (int*)out);
}


// the interpreter views' forwarders to the classes above
__forceinline int Presentation::create_text_overlay(const tstring& text, int flags) { return ((c49e780::View*)this)->create_text_overlay((int)&text, flags); }   // 0x49e780
__forceinline int GameX::create_companion_party(int troop, rgl::vector4 pos) { return ((GameCompanion*)this)->create_companion_party(troop, pos); }   // 0x49ef00
__forceinline void MissionX::order(int team, int order) { ((c49c2c0::Mission*)this)->order(team, order); }   // 0x49c2c0
__forceinline void MissionX::check_slot(int entry, int count) { ((c49bea0::Mission*)this)->check_slot(entry, count); }   // 0x49c170

// ==== the interpreter ===========================================================================================

// 64-bit operand range test as the original spells it: lo <= v < hi on the full 64-bit value
#define IN_RANGE(v, lo, hi) ((v) >= (lo) && (v) < (hi))
// team t's class selector (Sel 0x489890 / 0x4898e0): the original addresses it at the team record - 4
// (mission +0x527c + t*0x16c), so that its rec[k] fields land on cls[k] (present +0x4c = cls.use, the vector +0x18 = cls.pos)
#define TEAM_SEL(m, t) ((Sel*)((char*)&(m)->teams[t] - 4))
// identity3 (0x40cd60) seen through its hidden return pointer, as IDENTITY_VALUE does for identity_pos (the copy reads the returned pointer)
struct mat3_value : rgl::mat3 {};
typedef mat3_value (__cdecl* identity3_value_fn)();
#define IDENTITY3_VALUE() (((identity3_value_fn)(rgl::mat3* (__cdecl*)(rgl::mat3*))rgl::identity3)())
// d = s with s evaluated first (the original calls the identity function, then computes the destination register)
static __forceinline const rgl::matrix& mat_ref(const rgl::matrix& m) { return m; }
static __forceinline const rgl::mat3& mat3_ref(const rgl::mat3& m) { return m; }
#define ASSIGN_MAT(d, s) do { const rgl::matrix* m_; (m_ = &mat_ref(s), (d) = *m_); } while (0)
#define ASSIGN_MAT3(d, s) do { const rgl::mat3* m_; (m_ = &mat3_ref(s), (d) = *m_); } while (0)
#define SLOT_OK(v) IN_RANGE(v, 0, 0x100000)
// store a value into the lvalue operand through one named temporary (the original's single 64-bit slot at
// esp+0x18: a by-value temporary per call site would get a frame slot of its own)
#define SET_LV(e) set_lvalue(locals, (__int64)(e))
// one full-expression: the temporaries of e (an expanded string) die after the store, as in the original
#define SET_LV_FULL(e) set_lvalue(locals, (__int64)(e))
// d = s with s evaluated first: the string (get_string + expand) is built before the party record d is located
// (PartyMgr::get), and both temporaries die after the assignment, as in the original (add_party_note_from_sreg:
// get_string, expand, PartyMgr::get, operator=, ~, ~). One full-expression, sequenced by the comma operator.
static __forceinline const tstring& str_ref(const tstring& s) { return s; }
#define ASSIGN_STR(d, s) do { const tstring* s_; (s_ = &str_ref(s), (d) = *s_); } while (0)

// buf: 0x920..0xb20 in the original frame (single-use temporaries fill the rest up to the link buffers at 0xd20)
// In execute the accessors expand in place: the original writes g_game->troops[id] directly, so the expression is evaluated where it stands,
// after a call's arguments (a function, even __forceinline, computes the pointer into a temporary first and that rotates the scratch registers).
#define game() ((GameX*)g_game)
#define CS(s) (((const string_view_*)(s))->get())
#define troop(id) (&((GameX*)g_game)->troops[id])
#define faction(id) (&((GameX*)g_game)->factions[id])
#define quest(id) (&((GameX*)g_game)->quests[id])
#define scene(id) (&((GameX*)g_game)->scenes[id])
// @ 0x49fa10
char Operation::execute(__int64* locals, int* context, int* error)
{
    char buf[0x200];
    g_script_error = 0;
    g_cur_opcode = id;
    unsigned op_word = id;
    char result = 1;
    char* result_addr = &result;   // result's address is taken in the front end (the original keeps it in memory, and stores to it may alias loop bounds: main_party_has_troop reloads num_stacks)
    unsigned neg = op_word & op::neg;
    unsigned or_next = op_word & op::this_or_next;
    if (g_or_latch) {
        if (!or_next)
            g_or_latch = 0;
        return 1;
    }
    int opcode = op_word & 0xfffffff;
    __int64 params[16];
    int types[16];
    __int64* args = params;   // params' address is taken in the front end: what keeps its dwords reloaded after calls (the original's register map)
    params[0] = 0;
    params[1] = 0;
    params[2] = 0;
    int i;
    for (i = 0; i < n; i++)
        params[i] = get_operand(locals, &i, &types[i]);

    switch (opcode) {

    // ---- conditions: comparisons --------------------------------------------------------------------------
    case op::ge:
        if (params[0] >= params[1])
            result = 1;
        else
            result = 0;
        break;
    case op::eq:
        if (params[0] == params[1])
            result = 1;
        else
            result = 0;
        break;
    case op::gt:
        if (params[0] > params[1])
            result = 1;
        else
            result = 0;
        break;
    case op::is_between:
        if (params[1] <= params[0] && params[0] < params[2])
            result = 1;
        else
            result = 0;
        break;

    case op::entering_town: {
        // the main party is travelling to the town (ai_bhvr_attack_party with the town as object) and is within
        // 2.3 times its radius of it
        GameX* game = (GameX*)g_game;
        int mp = game->main_party; PartyMgr* pm = &game->parties;
        result = 0;
        if ((__int64)((PartyV*)pm->get(mp))->ai_object == params[0] &&
            ((PartyV*)pm->get(mp))->ai_behavior == ai_bhvr_attack_party) {
            float dist = ((rgl::vector4&)pm->get(((PartyV*)pm->get(mp))->ai_object)->pos - (rgl::vector4&)pm->get(mp)->pos).out_length();
            if (((PartyV*)pm->get(game->main_party))->radius * 2.3f >= dist)
                result = 1;
        }
        break;
    }
    case op::map_free:
        result = (*context & 1) && !(*context & 0x30000);
        break;
    case op::encountered_party_is_attacker:
        result = game()->encountered_party_is_attacker != 0;
        break;
    case op::conversation_screen_is_active:
        result = g_windows.dq.back() == 0x10;
        break;
    case op::in_meta_mission:
        result = g_dialogs.in_meta_mission;
        break;
    case op::set_player_troop:
        if (check_troop((int)params[0]))
            game()->player_troop = (int)params[0];
        break;
    case op::store_repeat_object:
        SET_LV((__int64)game()->repeat_object);
        break;
    case op::set_result_string:
        if (check_string_arg((int)params[0], types[0]))
            g_script.result_string = g_script.expand(g_strings.get_string((int)params[0], types[0]));
        break;
    case op::key_is_down:
        if (IN_RANGE(params[0], 0, 0x100))
            result = g_keyboard.down[(int)params[0]] != 0;
        break;
    case op::key_clicked:
        if (IN_RANGE(params[0], 0, 0x100))
            result = (char)g_keyboard.is_ready((int)params[0]);   // 0x487900
        break;
    case op::game_key_is_down:
        if (IN_RANGE(params[0], 0, 0x25))
            result = g_game_keys.is_down((int)params[0]);
        break;
    case op::game_key_clicked:
        if (IN_RANGE(params[0], 0, 0x25))
            result = g_game_keys.clicked((int)params[0]);
        break;
    case op::mouse_get_position:
    { float sf_0; vector4 sv4_0;
        if (check_register((int)params[0])) {
            sf_0 = 0;
            sv4_0;
            script_pos((int)params[0]).o = sv4_0.set(g_gui.mouse[0], g_gui.mouse[1], sf_0);
        }
        break;
    }
    case op::get_global_cloud_amount:
        SET_LV((__int64)(int)(game()->cloud_amount * 100.0));
        break;
    case op::set_global_cloud_amount: {
        game()->cloud_amount = (float)clamp_index((int)params[0], 0, 100) * 0.01f;
        break;
    }
    case op::get_global_haze_amount:
        SET_LV((__int64)(int)(game()->haze_amount * 100.0));
        break;
    case op::set_global_haze_amount: {
        game()->haze_amount = (float)clamp_index((int)params[0], 0, 100) * 0.01f;
        break;
    }

    // ---- conditions: parties joining -----------------------------------------------------------------------
    case op::hero_can_join:
        // a free stack slot and room under the party size limit ([party_id], default the main party)
        if (check_party((int)params[0])) {
            if (params[0] == 0) {
                int main = game()->main_party;
                PartyMgr* m = &game()->parties;
                int stacks = party(main)->max_stacks();
                int members;
                if (stacks - ((PartyV*)m->get(main))->first_flagged() >= 1 && ((members = party(game()->main_party)->sum_head()), ((PartyV*)m->get(main))->max_members() > members))
                    result = 1;
                else
                    result = 0;
            } else {
                int stacks = party((int)params[0])->max_stacks();
                result = stacks > party((int)params[0])->first_flagged();
            }
        }
        break;
    case op::hero_can_join_as_prisoner:
        if (check_party((int)params[0])) {
            if (params[0] == 0) {
                int main = game()->main_party;
                PartyMgr* m = &game()->parties;
                int stacks = party(main)->max_stacks();
                int members;
                if (stacks - ((PartyV*)m->get(main))->count_tail() >= 1 && ((members = party(game()->main_party)->sum_flagged()), ((PartyV*)m->get(main))->max_prisoners() > members))
                    result = 1;
                else
                    result = 0;
            } else {
                int stacks = party((int)params[0])->max_stacks();
                result = stacks > party((int)params[0])->count_tail();
            }
        }
        break;
    case op::party_can_join:
        if (g_dialogs.f10 < 0 || !((GameW*)g_game)->can_transfer(g_dialogs.f10, ((GameX*)g_game)->main_party, 0))
            result = 0;
        else
            result = 1;
        break;
    case op::party_can_join_as_prisoner:
        if (g_dialogs.f10 < 0 || !((GameW*)g_game)->can_transfer(g_dialogs.f10, ((GameX*)g_game)->main_party, 1))
            result = 0;
        else
            result = 1;
        break;
    case op::troops_can_join:
        if (params[0] > 0) {
            int main = game()->main_party;
            PartyMgr* m = &game()->parties;
            int stacks = party(main)->max_stacks();
            __int64 total;
            if (stacks - ((PartyV*)m->get(main))->first_flagged() >= 1 &&
                ((total = (__int64)((PartyV*)m->get(main))->sum_head() + params[0]), (__int64)((PartyV*)m->get(main))->max_members() >= total))
                result = 1;
            else
                result = 0;
        }
        break;
    case op::troops_can_join_as_prisoner:
        if (params[0] > 0) {
            int main = game()->main_party;
            PartyMgr* m = &game()->parties;
            int stacks = party(main)->max_stacks();
            __int64 total;
            if (stacks - ((PartyV*)m->get(main))->count_tail() >= 1 &&
                ((total = (__int64)((PartyV*)m->get(main))->sum_flagged() + params[0]), (__int64)((PartyV*)m->get(main))->max_prisoners() >= total))
                result = 1;
            else
                result = 0;
        }
        break;
    case op::party_can_join_party:
        result = 0;
        if (check_party((int)params[0]) && check_party((int)params[1]))
            result = ((GameW*)game())->can_transfer((int)params[0], (int)params[1], params[2] != 0);
        break;
    case op::main_party_has_troop: {
        int main = game()->main_party;
        result = 0;
        for (i = 0; i < party(main)->num_stacks; i++)
            if ((__int64)party(main)->stacks[i].troop == params[0])
                result = 1;
        break;
    }
    case op::party_is_in_town:
        if (check_party((int)params[0])) {
            int p = (int)params[0];
            if (party(p)->ai_behavior == ai_bhvr_in_town && (__int64)party(p)->ai_object == params[1])
                result = 1;
            else
                result = 0;
        }
        break;
    case op::party_is_in_any_town:
        if (check_party((int)params[0]))
            result = party((int)params[0])->ai_behavior == ai_bhvr_in_town;
        break;
    case op::party_is_active:
        if (game()->parties.is_live((int)params[0]) &&
            (party((int)params[0])->flags & 0x1080100) == 0)   // pf_disabled | 0x80000 | 0x1000000
            result = 1;
        else
            result = 0;
        break;

    // ---- conditions: items and quests ------------------------------------------------------------------------
    case op::player_has_item: {
        TroopX* t = troop(game()->player_troop);
        result = 0;
        for (int k = 0; k < t->inv_count(); k++)
            if ((__int64)t->items[k].item == params[0])
                result = 1;
        break;
    }
    case op::troop_has_item_equipped:
        result = 0;
        if (check_troop((int)params[0])) {
            ItemRef* e = troop((int)params[0])->equipment;
            // count-down trip spelling: the original keeps the loop (an up-counter gets unrolled x10)
            for (int k = 10; k; k--, e++)
                if ((__int64)e->item == params[1])
                    result = 1;
        }
        break;
    case op::troop_is_mounted:
        result = 0;
        if (check_troop((int)params[0]))
            result = troop((int)params[0])->equipment[ek_horse].item >= 0;
        break;
    case op::troop_is_guarantee_ranged:
        result = 0;
        if (check_troop((int)params[0])) {
            if ((troop((int)params[0])->flags & tf_guarantee_ranged) != 0)
                result = 1;
            else
                result = 0;
        }
        break;
    case op::troop_is_guarantee_horse:
        result = 0;
        if (check_troop((int)params[0])) {
            if ((troop((int)params[0])->flags & tf_guarantee_horse) != 0)
                result = 1;
            else
                result = 0;
        }
        break;
    case op::check_quest_active:
        if (check_quest((int)params[0]))
            result = (quest((int)params[0])->flags >> 8) & 1;
        break;
    case op::check_quest_finished:
        if (check_quest((int)params[0]))
            result = (quest((int)params[0])->flags >> 9) & 1;
        break;
    case op::check_quest_succeeded:
        if (check_quest((int)params[0]))
            result = (quest((int)params[0])->flags >> 10) & 1;
        break;
    case op::check_quest_failed:
        if (check_quest((int)params[0]))
            result = (quest((int)params[0])->flags >> 11) & 1;
        break;
    case op::check_quest_concluded:
        if (check_quest((int)params[0]))
            result = (quest((int)params[0])->flags >> 12) & 1;
        break;

    // ---- slots ---------------------------------------------------------------------------------------------------
    case op::troop_set_slot:
        if (check_troop((int)params[0]) && SLOT_OK(params[1]))
            troop((int)params[0])->slots.set((int)params[1], params[2]);
        break;
    case op::party_set_slot:
        if (check_party((int)params[0]) && SLOT_OK(params[1]))
            party((int)params[0])->slots.set((int)params[1], params[2]);
        break;
    case op::faction_set_slot:
        if (check_faction((int)params[0]) && SLOT_OK(params[1]))
            faction((int)params[0])->slots.set((int)params[1], params[2]);
        break;
    case op::scene_set_slot:
        if (check_scene((int)params[0]) && SLOT_OK(params[1]))
            scene((int)params[0])->slots.set((int)params[1], params[2]);
        break;
    case op::party_template_set_slot:
        // the original bounds the slot number by the party template count here (not by 0x100000)
        if (check_party_template((int)params[0]) && params[1] >= 0 && params[1] < g_party_template_count)
            g_party_templates[(int)params[0]].slots.set((int)params[1], params[2]);
        break;
    case op::agent_set_slot:
        if (warn_agent((int)params[0]) && SLOT_OK(params[1]))
            agent((int)params[0])->slot_vars.set((int)params[1], params[2]);
        break;
    case op::quest_set_slot:
        if (check_quest((int)params[0]) && SLOT_OK(params[1]))
            quest((int)params[0])->slots.set((int)params[1], params[2]);
        break;
    case op::item_set_slot:
        if (check_item_kind((int)params[0]) && SLOT_OK(params[1]))
            item_kind((int)params[0])->slots.set((int)params[1], params[2]);
        break;

    case op::troop_get_slot:
        if (check_troop((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(troop((int)params[1])->slots.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::party_get_slot:
        if (check_party((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(party((int)params[1])->slots.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::faction_get_slot:
        if (check_faction((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(faction((int)params[1])->slots.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::scene_get_slot:
        if (check_scene((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(scene((int)params[1])->slots.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::party_template_get_slot:
        if (check_party_template((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(g_party_templates[(int)params[1]].slots.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::agent_get_slot:
        if (warn_agent((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(agent((int)params[1])->slot_vars.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::quest_get_slot:
        if (check_quest((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(quest((int)params[1])->slots.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::item_get_slot:
        if (check_item_kind((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(item_kind((int)params[1])->slots.get((int)params[2]));
        else
            SET_LV(0);
        break;

    case op::troop_slot_eq:
        result = 0;
        if (check_troop((int)params[0]) && SLOT_OK(params[1]))
            result = troop((int)params[0])->slots.get((int)params[1]) == params[2];
        break;
    case op::party_slot_eq:
        result = 0;
        if (check_party((int)params[0]) && SLOT_OK(params[1]))
            result = party((int)params[0])->slots.get((int)params[1]) == params[2];
        break;
    case op::faction_slot_eq:
        result = 0;
        if (check_faction((int)params[0]) && SLOT_OK(params[1]))
            result = faction((int)params[0])->slots.get((int)params[1]) == params[2];
        break;
    case op::scene_slot_eq:
        result = 0;
        if (check_scene((int)params[0]) && SLOT_OK(params[1]))
            result = scene((int)params[0])->slots.get((int)params[1]) == params[2];
        break;
    case op::party_template_slot_eq:
        result = 0;
        if (check_party_template((int)params[0]) && SLOT_OK(params[1]))
            result = g_party_templates[(int)params[0]].slots.get((int)params[1]) == params[2];
        break;
    case op::agent_slot_eq:
        result = 0;
        if (warn_agent((int)params[0]) && SLOT_OK(params[1]))
            result = agent((int)params[0])->slot_vars.get((int)params[1]) == params[2];
        break;
    case op::quest_slot_eq:
        result = 0;
        if (check_quest((int)params[0]) && SLOT_OK(params[1]))
            result = quest((int)params[0])->slots.get((int)params[1]) == params[2];
        break;
    case op::item_slot_eq:
        result = 0;
        if (check_item_kind((int)params[0]) && SLOT_OK(params[1]))
            result = item_kind((int)params[0])->slots.get((int)params[1]) == params[2];
        break;

    case op::troop_slot_ge:
        result = 0;
        if (check_troop((int)params[0]) && SLOT_OK(params[1]))
            result = troop((int)params[0])->slots.get((int)params[1]) >= params[2];
        break;
    case op::party_slot_ge:
        result = 0;
        if (check_party((int)params[0]) && SLOT_OK(params[1]))
            result = party((int)params[0])->slots.get((int)params[1]) >= params[2];
        break;
    case op::faction_slot_ge:
        result = 0;
        if (check_faction((int)params[0]) && SLOT_OK(params[1]))
            result = faction((int)params[0])->slots.get((int)params[1]) >= params[2];
        break;
    case op::scene_slot_ge:
        result = 0;
        if (check_scene((int)params[0]) && SLOT_OK(params[1]))
            result = scene((int)params[0])->slots.get((int)params[1]) >= params[2];
        break;
    case op::party_template_slot_ge:
        result = 0;
        if (check_party_template((int)params[0]) && SLOT_OK(params[1]))
            result = g_party_templates[(int)params[0]].slots.get((int)params[1]) >= params[2];
        break;
    case op::agent_slot_ge:
        result = 0;
        if (warn_agent((int)params[0]) && SLOT_OK(params[1]))
            result = agent((int)params[0])->slot_vars.get((int)params[1]) >= params[2];
        break;
    case op::quest_slot_ge:
        result = 0;
        if (check_quest((int)params[0]) && SLOT_OK(params[1]))
            result = quest((int)params[0])->slots.get((int)params[1]) >= params[2];
        break;
    case op::item_slot_ge:
        result = 0;
        if (check_item_kind((int)params[0]) && SLOT_OK(params[1]))
            result = item_kind((int)params[0])->slots.get((int)params[1]) >= params[2];
        break;

    // ---- sound and music -------------------------------------------------------------------------------------------
    case op::play_sound:
        if (warn_sound((int)params[0])) {
            int ent = trigger_entity(*context);
            if (ent)
                g_sound_mgr.play((int)params[0], (int)params[1], (const float*)(ent + 0x88), 1.0f, 0);
            else
                g_sound_mgr.play_ui((int)params[0], (int)params[1]);
        }
        break;
    case op::play_track:
        if (warn_track((int)params[0]) && IN_RANGE(params[1], 0, 3)) {
            g_script.next_track = (int)params[0];
            g_script.next_track_options = (int)params[1];
        }
        break;
    case op::play_cue_track:
        if (warn_track((int)params[0]))
            g_script.cue_track = (int)params[0];
        break;
    case op::music_set_situation:
        g_music.situation = (int)params[0];
        break;
    case op::music_set_culture:
        g_music.mask = (int)params[0];
        break;
    case op::stop_all_sounds:
        g_sound_mgr.stop_all();
        break;

    // ---- positions -------------------------------------------------------------------------------------------------
    case op::copy_position:
        if (check_register((int)params[0]) && check_register((int)params[1]))
            script_pos((int)params[0]) = script_pos((int)params[1]);
        break;
    case op::init_position:
        if (check_register((int)params[0]))
            ASSIGN_MAT(script_pos((int)params[0]), IDENTITY_VALUE());
        break;
    case op::get_trigger_object_position:
        if (check_register((int)params[0]))
            script_pos((int)params[0]) = game()->trigger_object_pos;
        break;
    case op::get_distance_between_positions:
        if (check_register((int)params[1]) && check_register((int)params[2]))
            SET_LV((__int64)(rgl::length(script_pos((int)params[1]).o - script_pos((int)params[2]).o) * 100.0f));
        else
            SET_LV(0);
        break;
    case op::position_is_behind_position:
        if (check_register((int)params[0]) && check_register((int)params[1]) &&
            rgl::dot(script_pos((int)params[1]).f, script_pos((int)params[0]).o - script_pos((int)params[1]).o) < 0.0f)
            result = 1;
        else
            result = 0;
        break;
    case op::position_transform_position_to_parent:
        if (check_register((int)params[0]) && check_register((int)params[1]) && check_register((int)params[2]))
            ASSIGN_MAT(script_pos((int)params[0]), script_pos((int)params[1]) * script_pos((int)params[2]));
        break;
    case op::position_transform_position_to_local:
        if (check_register((int)params[0]) && check_register((int)params[1]) && check_register((int)params[2]))
            ASSIGN_MAT(script_pos((int)params[0]), script_pos((int)params[1]).relative(script_pos((int)params[2])));
        break;
    case op::position_copy_rotation:
        if (check_register((int)params[0]) && check_register((int)params[1]))
            *(rgl::mat3*)&script_pos((int)params[0]) = *(rgl::mat3*)&script_pos((int)params[1]);
        break;
    case op::position_copy_origin:
        if (check_register((int)params[0]) && check_register((int)params[1]))
            script_pos((int)params[0]).o = script_pos((int)params[1]).o;
        break;
    case op::position_move_x:
        // movement in cm; [value] 0 = along the local axis, else along the global one
        if (check_register((int)params[0])) {
            if (params[2])
                script_pos((int)params[0]).o.x += (float)params[1] * 0.01f;
            else
                script_pos((int)params[0]).o += ((float)params[1] * 0.01f) * script_pos((int)params[0]).s;
        }
        break;
    case op::position_move_y:
        if (check_register((int)params[0])) {
            if (params[2])
                script_pos((int)params[0]).o.y += (float)params[1] * 0.01f;
            else
                script_pos((int)params[0]).o += ((float)params[1] * 0.01f) * script_pos((int)params[0]).f;
        }
        break;
    case op::position_move_z:
        if (check_register((int)params[0])) {
            if (params[2])
                script_pos((int)params[0]).o.z += (float)params[1] * 0.01f;
            else
                script_pos((int)params[0]).o += ((float)params[1] * 0.01f) * script_pos((int)params[0]).u;
        }
        break;
    case op::position_rotate_x:
    { float sf_0;
        if (check_register((int)params[0])) {
            sf_0 = (float)params[1] * 0.017453292f;
            script_pos((int)params[0]).rotate_about_s(&sf_0);
        }
        break;
    }
    case op::position_rotate_y:
    { float sf_0;
        if (check_register((int)params[0])) {
            sf_0 = (float)params[1] * 0.017453292f;
            script_pos((int)params[0]).rotate_about_f(&sf_0);
        }
        break;
    }
    case op::position_rotate_z:
    { float sf_0;
        // [use_global_z_axis]: rotate every row about the global z axis instead of the local one
        if (check_register((int)params[0])) {
            sf_0 = (float)params[1] * 0.017453292f;
            if (params[2]) {
                int p = (int)params[0];
                rotate_b(script_pos(p).s, sf_0);
                rotate_b(script_pos(p).f, sf_0);
                rotate_b(script_pos(p).u, sf_0);
            } else
                script_pos((int)params[0]).rotate_about_u(&sf_0);
        }
        break;
    }
    case op::position_get_x:
        if (check_register((int)params[1])) {
            SET_LV(round_i64(g_script.fixed_point_multiplier * script_pos((int)params[1]).o.x));
        }
        break;
    case op::position_get_y:
        if (check_register((int)params[1])) {
            SET_LV(round_i64(g_script.fixed_point_multiplier * script_pos((int)params[1]).o.y));
        }
        break;
    case op::position_get_z:
        if (check_register((int)params[1])) {
            SET_LV(round_i64(g_script.fixed_point_multiplier * script_pos((int)params[1]).o.z));
        }
        break;
    case op::position_set_x:
        if (check_register((int)params[0]))
            script_pos((int)params[0]).o.x = (float)params[1] / g_script.fixed_point_multiplier;
        break;
    case op::position_set_y:
        if (check_register((int)params[0]))
            script_pos((int)params[0]).o.y = (float)params[1] / g_script.fixed_point_multiplier;
        break;
    case op::position_set_z:
        if (check_register((int)params[0]))
            script_pos((int)params[0]).o.z = (float)params[1] / g_script.fixed_point_multiplier;
        break;
    case op::position_get_scale_x:
        if (check_register((int)params[1])) {
            SET_LV(round_i64(g_script.fixed_point_multiplier * script_pos((int)params[1]).s.x));
        }
        break;
    case op::position_get_scale_y:
        if (check_register((int)params[1])) {
            SET_LV(round_i64(g_script.fixed_point_multiplier * script_pos((int)params[1]).f.y));
        }
        break;
    case op::position_get_scale_z:
        if (check_register((int)params[1])) {
            SET_LV(round_i64(g_script.fixed_point_multiplier * script_pos((int)params[1]).u.z));
        }
        break;
    case op::position_get_rotation_around_z:
        // degrees, 0..359
        if (check_register((int)params[1])) {
            rgl::vector4& f = script_pos((int)params[1]).f;
            SET_LV((round_i64((float)atan2((double)-f.x, (double)f.y) * 57.29578f) + 360) % 360);
        }
        break;
    case op::position_normalize_origin:
        // normalizes the origin; the destination gets its old length (fixed point)
        if (check_register((int)params[1])) {
            float len = script_pos((int)params[1]).o.normalize();
            SET_LV(round_i64(g_script.fixed_point_multiplier * len));
        }
        break;
    case op::position_set_z_to_ground_level:
        if (check_register((int)params[0])) {
            MissionX* m = mission();
            if (m->min_x <= script_pos((int)params[0]).o.x && script_pos((int)params[0]).o.x < m->max_x && m->min_y <= script_pos((int)params[0]).o.y && script_pos((int)params[0]).o.y < m->max_y) {
                float z = m->scene->floor_z(&script_pos((int)params[0]).o.x);
                script_pos((int)params[0]).o.z = z;
            }
        }
        break;

    // ---- presentations and overlays ---------------------------------------------------------------------------------
    case op::start_presentation:
        if (check_presentation((int)params[0])) {
            game()->next_window = 0xf;
            game()->presentation = (int)params[0];
        }
        break;
    case op::start_background_presentation:
        // only from game menus (context 8)
        if (*context == 8 && check_presentation((int)params[0]))
            game()->background_presentation = (int)params[0];
        break;
    case op::presentation_set_duration:
        if (g_presentation)
            g_presentation->duration = (float)params[0] * 0.01f;
        break;
    case op::create_text_overlay:
        if (g_presentation && check_string_arg((int)params[1], types[1]))
            SET_LV_FULL((__int64)((c49e780::View*)g_presentation)->create_text_overlay(
                (int)&g_script.expand(g_strings.get_string((int)params[1], types[1])), (int)params[2]));
        break;
    case op::create_mesh_overlay:
        if (g_presentation && check_mesh((int)params[1]))
            SET_LV((__int64)((WidgetMgrW*)g_presentation)->create_mesh_overlay((int)params[1]));
        break;
    case op::create_button_overlay:
        if (g_presentation && check_string_arg((int)params[1], types[1]))
            SET_LV_FULL((__int64)((WidgetMgrW*)g_presentation)->create_button_overlay(
                &g_script.expand(g_strings.get_string((int)params[1], types[1])), (int)params[2]));
        break;
    case op::create_image_button_overlay:
        if (g_presentation && check_mesh((int)params[1]) && check_mesh((int)params[2]))
            SET_LV((__int64)((widget_mgr*)g_presentation)->create_image_button_overlay((int)params[1], (int)params[2]));
        break;
    case op::create_slider_overlay:
        if (g_presentation)
            SET_LV((__int64)((WidgetMgrW*)g_presentation)->create_slider_overlay((int)params[1], (int)params[2]));
        break;
    case op::create_progress_overlay:
        if (g_presentation)
            SET_LV((__int64)((widget_mgr*)g_presentation)->create_progress_overlay((int)params[1], (int)params[2]));
        break;
    case op::create_combo_button_overlay:
        if (g_presentation)
            SET_LV((__int64)((widget_mgr*)g_presentation)->create_combo_button_overlay());
        break;
    case op::create_text_box_overlay:
        if (g_presentation)
            SET_LV((__int64)((widget_mgr*)g_presentation)->create_text_box_overlay());
        break;
    case op::create_check_box_overlay:
        if (g_presentation && check_mesh((int)params[1]) && check_mesh((int)params[2]))
            SET_LV((__int64)((widget_mgr*)g_presentation)->create_check_box_overlay((int)params[1], (int)params[2]));
        break;
    case op::overlay_set_text:
        if (g_presentation && check_string_arg((int)params[1], types[1]))
            g_presentation->set_text((int)params[0], g_script.expand(g_strings.get_string((int)params[1], types[1])));
        break;
    case op::overlay_set_color:
        if (g_presentation)
            ((widget_mgr*)g_presentation)->set_color((int)params[0], (int)params[1]);
        break;
    case op::overlay_set_alpha:
        if (g_presentation)
            ((widget_mgr*)g_presentation)->set_alpha((int)params[0], (int)params[1]);
        break;
    case op::overlay_set_hilight_color:
        if (g_presentation)
            ((widget_mgr*)g_presentation)->set_hilight_color((int)params[0], (int)params[1]);
        break;
    case op::overlay_set_hilight_alpha:
        if (g_presentation)
            ((widget_mgr*)g_presentation)->set_hilight_alpha((int)params[0], (int)params[1]);
        break;
    case op::overlay_set_size:
        if (g_presentation && check_register((int)params[1]))
            g_presentation->set_size((int)params[0], &script_pos((int)params[1]).o);
        break;
    case op::overlay_set_position:
        if (g_presentation && check_register((int)params[1]))
            g_presentation->set_position((int)params[0], &script_pos((int)params[1]).o);
        break;
    case op::overlay_set_val:
        if (g_presentation)
            ((widget_mgr*)g_presentation)->set_val((int)params[0], (int)params[1]);
        break;
    case op::overlay_set_boundaries:
        if (g_presentation)
            ((widget_mgr*)g_presentation)->set_limits((int)params[0], (int)params[1], (int)params[2]);
        break;
    case op::overlay_set_area_size:
        // the position register is not validated here
        if (g_presentation)
            g_presentation->set_area_size((int)params[0], &script_pos((int)params[1]).o);
        break;
    case op::overlay_set_mesh_rotation:
        if (g_presentation)
            ((widget_mgr*)g_presentation)->set_mesh_matrix((int)params[0], &script_pos((int)params[1]));
        break;
    case op::overlay_add_item:
        if (g_presentation && check_string_arg((int)params[1], types[1]))
            g_presentation->add_item((int)params[0], g_script.expand(g_strings.get_string((int)params[1], types[1])));
        break;
    case op::overlay_animate_to_color:
        // duration in 1/1000 seconds
        if (g_presentation)
            g_presentation->animate_to_color((int)params[0], (float)params[1] * 0.001f, (int)params[2]);
        break;
    case op::overlay_animate_to_alpha:
        if (g_presentation)
            g_presentation->animate_to_alpha((int)params[0], (float)params[1] * 0.001f, (int)params[2]);
        break;
    case op::overlay_animate_to_highlight_color:
        if (g_presentation)
            g_presentation->animate_to_highlight_color((int)params[0], (float)params[1] * 0.001f, (int)params[2]);
        break;
    case op::overlay_animate_to_highlight_alpha:
        if (g_presentation)
            g_presentation->animate_to_highlight_alpha((int)params[0], (float)params[1] * 0.001f, (int)params[2]);
        break;
    case op::overlay_animate_to_size:
        if (g_presentation && check_register((int)params[2]))
            g_presentation->animate_to_size((int)params[0], (float)params[1] * 0.001f, &script_pos((int)params[2]).o);
        break;
    case op::overlay_animate_to_position:
        if (g_presentation && check_register((int)params[2]))
            g_presentation->animate_to_position((int)params[0], (float)params[1] * 0.001f, &script_pos((int)params[2]).o);
        break;
    case op::create_image_button_overlay_with_tableau_material:
        // mesh -1 = none (not validated)
        if (g_presentation && (params[1] == -1 || check_mesh((int)params[1])) && warn_tableau((int)params[2]))
            SET_LV((__int64)((widget_mgr*)g_presentation)->create_image_button_overlay_with_tableau(
                (int)params[1], (int)params[2], (int)params[3]));
        break;
    case op::create_mesh_overlay_with_tableau_material:
        if (g_presentation && (params[1] == -1 || check_mesh((int)params[1])) && warn_tableau((int)params[2]))
            SET_LV((__int64)((WidgetMgrW*)g_presentation)->create_mesh_overlay_with_tableau(
                (int)params[1], (int)params[2], *(float*)&params[3]));
        break;
    case op::create_game_button_overlay:
        if (g_presentation && check_string_arg((int)params[1], types[1]))
            SET_LV_FULL((__int64)((widget_mgr*)g_presentation)->create_game_button_overlay(
                (int)&g_script.expand(g_strings.get_string((int)params[1], types[1]))));
        break;
    case op::context_menu_add_item:
        if (check_string_arg((int)params[0], types[0])) {
            ASSIGN_STR(game()->context_menu_texts[game()->num_context_menu_items],
                g_script.expand(g_strings.get_string((int)params[0], types[0])));
            game()->context_menu_values[game()->num_context_menu_items] = (int)params[1];
            game()->num_context_menu_items++;
        }
        break;
    case op::get_average_game_difficulty:
        ((GameData*)g_game)->sl_update_avg();
        SET_LV((__int64)(int)game()->average_difficulty);
        break;

    // ---- missions, map, resting, experience ------------------------------------------------------------------------
    case op::all_enemies_defeated:
        result = mission()->all_defeated((int)params[0]);
        break;
    case op::race_completed_by_player:
        break;
    case op::num_active_teams_le:
        if ((__int64)mission()->team_count() <= params[0])
            result = 1;
        else
            result = 0;
        break;
    case op::main_hero_fallen:
        result = mission()->hero_down();
        break;
    case op::set_party_battle_mode:
        g_mission_ctx.f2c8 = 1;   // set_party_battle_mode 0x4a2ad0
        break;
    case op::set_camera_follow_party:
        if (check_party((int)params[0])) {
            game()->camera_follow_party = (int)params[0];
            game()->camera_follow = 1;
        }
        break;
    case op::start_map_conversation:
        game()->map_conversation_troop = (int)params[0];
        game()->map_conversation_mode = 5;
        game()->start_map_conversation = 1;
        break;
    case op::rest_for_hours:
        // (rest_for_hours, <rest_period>, [time_speed], [remain_attackable])
        game()->rest_period = (float)params[0];
        game()->rest_time_speed = (int)params[1];
        game()->rest_interactive = 0;
        if (params[0] > 0) {
            if (params[2]) {
                game()->rest_attackable = 0;
                party(game()->main_party)->ai_behavior = ai_bhvr_hold;
            } else {
                game()->rest_attackable = 1;
                party(game()->main_party)->ai_behavior = ai_bhvr_in_town;
            }
        }
        break;
    case op::rest_for_hours_interactive:
        game()->rest_period = (float)params[0];
        game()->rest_time_speed = (int)params[1];
        game()->rest_interactive = 1;
        if (params[0] > 0) {
            if (params[2]) {
                game()->rest_attackable = 0;
                party(game()->main_party)->ai_behavior = ai_bhvr_hold;
            } else {
                game()->rest_attackable = 1;
                party(game()->main_party)->ai_behavior = ai_bhvr_in_town;
            }
        }
        break;
    case op::add_xp_to_troop:
        // (add_xp_to_troop, <value>, [troop_id])
        if (check_troop((int)params[1]))
            ((GameData*)g_game)->add_experience((int)params[1], (int)params[0]);
        break;
    case op::add_gold_as_xp:
        ((GameW*)g_game)->add_random_experience((int)params[0]);
        break;
    case op::add_xp_as_reward:
        ((GameW*)g_game)->spread_party_experience((int)params[0]);
        break;
    case op::add_gold_to_party:
        // (add_gold_to_party, <value>, <party_id>): 1.011 only clears the party's carried-gold byte
        if (check_party((int)params[1]))
            party((int)params[1])->carry_gold = 0;
        break;
    case op::set_party_creation_random_limits:
        // both limits clamped to 0..101
        if (params[0] < 0)
            params[0] = 0;
        else if (params[0] > 101)
            params[0] = 101;
        if (params[1] < 0)
            params[1] = 0;
        else if (params[1] > 101)
            params[1] = 101;
        if (params[0] <= params[1]) {
            game()->party_creation_min_random = (int)params[0];
            game()->party_creation_max_random = (int)params[1];
        }
        break;

    // ---- spawning, messages, notes ---------------------------------------------------------------------------------
    case op::spawn_around_party:
        // (spawn_around_party, <party_id>, <party_template_id>): the new party's id goes to reg0
        if (check_party((int)params[0]) && check_party_template((int)params[1])) {
            rgl::vector4& p = party_pos(party((int)params[0]));
            g_script.reg[0] = ((GameData*)g_game)->spawn_around(p.x, p.y, (int)params[1], g_spawn_radius);
        }
        break;
    case op::set_spawn_radius:
        g_spawn_radius = (float)params[0];
        break;
    case op::display_log_message:
        if (check_string_arg((int)params[0], types[0])) {
            int color = -1;
            if (params[1])
                color = (int)params[1];
            tstring text = g_script.expand(g_strings.get_string((int)params[0], types[0]));
            g_gui.add(text, color);
            char num[32];      // 0x890..0x8b0 in the original frame
            _itoa(color, num, 10);
            GameX* g = game();
            if (g->message_log.len > 2)
                g->message_log.push_back('|');
            g = game(); game()->message_log.append(((GameData*)g)->event_string((int)g->clock.elapsed()));
            game()->message_log.push_back('|');
            { tstring s(num); game()->message_log.append(s); }
            game()->message_log.push_back('|');
            game()->message_log.append(text);
            if (g_cfg.enable_edit_mode) {
                text.push_back('\n');
                ((stream_base&)g_log).w_tstr(&text);
            }
        }
        break;
    case op::display_message:
        if (check_string_arg((int)params[0], types[0])) {
            int color = -1;
            if (params[1])
                color = (int)params[1];
            tstring text = g_script.expand(g_strings.get_string((int)params[0], types[0]));
            g_gui.add(text, color);
            if (g_cfg.enable_edit_mode) {
                text.push_back('\n');
                ((stream_base&)g_log).w_tstr(&text);
            }
        }
        break;
    case op::set_show_messages:
        {bool v = params[0] != 0; g_gui.enabled = v;}      // 0x4a30a0
        break;
    case op::add_troop_note_tableau_mesh:
        if (check_troop((int)params[0]) && warn_tableau((int)params[1]))
            troop((int)params[0])->note_tableau = (int)params[1];
        break;
    case op::add_faction_note_tableau_mesh:
        if (check_faction((int)params[0]) && warn_tableau((int)params[1]))
            faction((int)params[0])->note_tableau = (int)params[1];
        break;
    case op::add_party_note_tableau_mesh:
        if (check_party((int)params[0]) && warn_tableau((int)params[1]))
            party((int)params[0])->note_tableau = (int)params[1];
        break;
    case op::add_quest_note_tableau_mesh:
        if (check_quest((int)params[0]) && warn_tableau((int)params[1]))
            quest((int)params[0])->note_tableau = (int)params[1];
        break;
    // (add_*_note_from_dialog, <id>, <note_slot_no>, <value>): in a conversation only (context 4); the note is
    // "<speaker name>: <current sentence>"; value 1 = time-stamp it (else -1 = hidden). The slot number has only an
    // upper bound. add_quest_note_from_dialog validates its quest id as a troop id.
    case op::add_troop_note_from_dialog:
        if (*context == 4 && check_troop((int)params[0]) && params[1] < 8) {
            ASSIGN_STR(troop((int)params[0])->notes[(int)params[1]], *troop(g_dialogs.speaker())->display_name());
            { tstring sep(": "); troop((int)params[0])->notes[(int)params[1]].append(sep); }
            troop((int)params[0])->notes[(int)params[1]].append(((GameData*)&g_dialogs)->get_name());
            troop((int)params[0])->note_times[(int)params[1]] = params[2] ? game()->note_time : -1;
        }
        break;
    case op::add_faction_note_from_dialog:
        if (*context == 4 && check_faction((int)params[0]) && params[1] < 8) {
            ASSIGN_STR(faction((int)params[0])->notes[(int)params[1]], *troop(g_dialogs.speaker())->display_name());
            { tstring sep(": "); faction((int)params[0])->notes[(int)params[1]].append(sep); }
            faction((int)params[0])->notes[(int)params[1]].append(((GameData*)&g_dialogs)->get_name());
            faction((int)params[0])->note_times[(int)params[1]] = params[2] ? game()->note_time : -1;
        }
        break;
    case op::add_party_note_from_dialog:
        if (*context == 4 && check_party((int)params[0]) && params[1] < 8) {
            ASSIGN_STR(party((int)params[0])->notes[(int)params[1]], *troop(g_dialogs.speaker())->display_name());
            { tstring sep(": "); party((int)params[0])->notes[(int)params[1]].append(sep); }
            ((PartyV*)((GameX*)g_game)->parties.get((int)params[0]))->notes[(int)params[1]].append(((GameData*)&g_dialogs)->get_name());
            party((int)params[0])->note_times[(int)params[1]] = params[2] ? game()->note_time : -1;
        }
        break;
    case op::add_quest_note_from_dialog:
        if (*context == 4 && check_troop((int)params[0]) && params[1] < 8) {
            ASSIGN_STR(quest((int)params[0])->notes[(int)params[1]], *troop(g_dialogs.speaker())->display_name());
            { tstring sep(": "); quest((int)params[0])->notes[(int)params[1]].append(sep); }
            quest((int)params[0])->notes[(int)params[1]].append(((GameData*)&g_dialogs)->get_name());
            quest((int)params[0])->note_times[(int)params[1]] = params[2] ? game()->note_time : -1;
        }
        break;
    case op::add_troop_note_from_sreg:
        if (check_string_arg((int)params[2], types[2]) && check_troop((int)params[0]) && params[1] < 8) {
            ASSIGN_STR(troop((int)params[0])->notes[(int)params[1]], g_script.expand(g_strings.get_string((int)params[2], types[2])));
            troop((int)params[0])->note_times[(int)params[1]] = params[3] ? game()->note_time : -1;
        }
        break;
    case op::add_faction_note_from_sreg:
        if (check_string_arg((int)params[2], types[2]) && check_faction((int)params[0]) && params[1] < 8) {
            ASSIGN_STR(faction((int)params[0])->notes[(int)params[1]], g_script.expand(g_strings.get_string((int)params[2], types[2])));
            faction((int)params[0])->note_times[(int)params[1]] = params[3] ? game()->note_time : -1;
        }
        break;
    case op::add_party_note_from_sreg:
        if (check_string_arg((int)params[2], types[2]) && check_party((int)params[0]) && params[1] < 8) {
            ASSIGN_STR(party((int)params[0])->notes[(int)params[1]], g_script.expand(g_strings.get_string((int)params[2], types[2])));
            party((int)params[0])->note_times[(int)params[1]] = params[3] ? game()->note_time : -1;
        }
        break;
    case op::add_quest_note_from_sreg:
        // (add_quest_note_from_sreg, <quest_id>, <note_slot_no>, <string_id>, <value>)
        if (check_string_arg((int)params[2], types[2]) && check_quest((int)params[0]) && params[1] < 8) {
            ASSIGN_STR(quest((int)params[0])->notes[(int)params[1]], g_script.expand(g_strings.get_string((int)params[2], types[2])));
            quest((int)params[0])->note_times[(int)params[1]] = params[3] ? game()->note_time : -1;
        }
        break;
    case op::tutorial_box:
        // (tutorial_box / dialog_box, <text_string_id>, [title_string_id]); title defaults to string 1
        if (check_string_arg((int)params[0], types[0]) && g_windows.dq.back() != 0) {
            int title = 1, title_type = 3;
            if (params[1] > 0 && check_string_arg((int)params[1], types[1])) {
                title = (int)params[1];
                title_type = types[1];
            }
            g_windows.box()->setup(&g_script.expand(g_strings.get_string(title, title_type)),
                                       &g_script.expand(g_strings.get_string((int)params[0], types[0])));
            g_windows.open(0);
        }
        break;
    case op::question_box:
        // (question_box, <string_id>, [yes_string_id], [no_string_id]); defaults strings 2 / 3; answer in game +0x1a208
        if (check_string_arg((int)params[0], types[0]) && g_windows.dq.back() != 0) {
            int yes = 2, yes_type = 3, no = 3, no_type = 3;
            if (params[1] > 0 && check_string_arg((int)params[1], types[1])) {
                yes = (int)params[1];
                yes_type = types[1];
            }
            if (params[2] > 0 && check_string_arg((int)params[2], types[2])) {
                no = (int)params[2];
                no_type = types[2];
            }
            g_windows.box()->setup2(&g_script.expand(g_strings.get_string((int)params[0], types[0])),
                                      &g_script.expand(g_strings.get_string(yes, yes_type)),
                                      &g_script.expand(g_strings.get_string(no, no_type)), (int)&game()->question_answer);
            g_windows.open(0);
        }
        break;
    case op::tutorial_message:
        // (tutorial_message, <string_id>, [color]); an invalid string (-1) clears the message
        if (check_string_arg((int)params[0], types[0])) {
            int color = -1;
            if (params[1])
                color = (int)params[1];
            g_gui.set_font_mesh(&g_script.expand(g_strings.get_string((int)params[0], types[0])), color);
        } else {
            tstring empty("");
            g_gui.set_font_mesh(&empty, (int)params[1]);
        }
        break;

    // ---- prices, joining and leaving -----------------------------------------------------------------------------
    case op::reset_price_rates:
        game()->sl_item_flag_all();
        break;
    case op::set_price_rate_for_item:
        if (check_item_kind((int)params[0]))
            item_kind((int)params[0])->price_rate = (float)params[1] * 0.01f;
        break;
    case op::set_price_rate_for_item_type:
        for (int k = 0; k < g_item_kind_count; k++)
            if ((__int64)(item_kind(k)->type_flags & 0xff) == params[0])
                item_kind(k)->price_rate = (float)params[1] * 0.01f;
        break;
    case op::party_join:
        {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
            tstring key("ui_value_joined_your_party");
            sprintf(buf, CS(g_ui_strings.lookup(key)), CS(party(g_dialogs.f10)->display_name()));
        }
        { tstring msg(buf); g_gui.push(msg, -1); }
        ((GameW*)g_game)->transfer_stack(g_dialogs.f10, game()->main_party, 0);
        break;
    case op::party_join_as_prisoner:
        {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
            tstring key("ui_value_joined_party_as_prisoner");
            sprintf(buf, CS(g_ui_strings.lookup(key)), CS(party(g_dialogs.f10)->display_name()));
        }
        { tstring msg(buf); g_gui.push(msg, -1); }
        ((GameW*)g_game)->transfer_stack(g_dialogs.f10, game()->main_party, 1);
        break;
    case op::troop_join:
        if (check_troop((int)params[0])) {
            {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
                tstring key("ui_value_has_joined_party");
                sprintf(buf, CS(g_ui_strings.lookup(key)), CS(troop((int)params[0])->display_name()));
            }
            { tstring msg(buf); g_gui.push(msg, -1); }
            party(game()->main_party)->add_members((int)params[0], 1, 0, 0, 0);
        }
        break;
    case op::troop_join_as_prisoner:
        if (check_troop((int)params[0])) {
            {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
                tstring key("ui_value_has_been_taken_prisoner");
                sprintf(buf, CS(g_ui_strings.lookup(key)), CS(troop((int)params[0])->display_name()));
            }
            { tstring msg(buf); g_gui.push(msg, -1); }
            party(game()->main_party)->add_members((int)params[0], 1, 1, 0, 0);
        }
        break;
    case op::remove_member_from_party:
        // (remove_member_from_party, <troop_id>, [party_id]): prisoners first, then the companions
        if (check_troop((int)params[0]) && check_party((int)params[1])) {
            int p = (int)params[1];
            if (!params[1])
                p = game()->main_party;
            if (!((GameData*)g_game)->remove_troops(p, (int)params[0], 1, 1, 0))
                ((GameData*)g_game)->remove_troops(p, (int)params[0], 1, 0, 0);
            if (!params[1]) {
                {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
                    tstring key("ui_value_left_the_party");
                    sprintf(buf, CS(g_ui_strings.lookup(key)), CS(troop((int)params[0])->display_name()));
                }
                { tstring msg(buf); g_gui.push(msg, -1); }
            }
        }
        break;
    case op::remove_regular_prisoners:
        // the party argument is ignored: the main party's non-hero prisoner stacks are released
        for (int k = ((PartyV*)((GameX*)g_game)->parties.get(game()->main_party))->num_stacks - 1; k >= 0; k--) {
            int main = game()->main_party;
            PartyMgr* m = &game()->parties;
            if ((((PartyV*)m->get(main))->stacks[k].flags & 1) && !troop(((PartyV*)m->get(main))->stacks[k].troop)->is_hero()) {
                {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
                    tstring key("ui_number_values_left_the_party");
                    sprintf(buf, CS(g_ui_strings.lookup(key)), ((PartyV*)((GameX*)g_game)->parties.get(game()->main_party))->stacks[k].count, CS(troop(((PartyV*)((GameX*)g_game)->parties.get(game()->main_party))->stacks[k].troop)->display_name()));
                }
                { tstring msg(buf); g_gui.push(msg, -1); }
                ((PartyV*)((GameX*)g_game)->parties.get(game()->main_party))->remove_stack(k, game()->stack_rows);
            }
        }
        break;
    case op::remove_troops_from_companions:
        if (check_troop((int)params[0]) && params[1] > 0) {
            ((GameData*)g_game)->remove_troops(game()->main_party, (int)params[0], (int)params[1], 0, 0);
            if (troop((int)params[0])->is_hero())
                {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
                    tstring key("ui_value_left_the_party");
                    sprintf(buf, CS(g_ui_strings.lookup(key)), CS(troop((int)params[0])->display_name()));
                }
            else
                {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
                    tstring key("ui_number_value_left_the_party");
                    sprintf(buf, CS(g_ui_strings.lookup(key)), params[1], CS(troop((int)params[0])->display_name()));
                }
            { tstring msg(buf); g_gui.push(msg, -1); }
        }
        break;
    case op::remove_troops_from_prisoners:
        // formats the message like remove_troops_from_companions but never shows it
        if (check_troop((int)params[0]) && params[1] > 0) {
            ((GameData*)g_game)->remove_troops(game()->main_party, (int)params[0], (int)params[1], 1, 0);
            if (troop((int)params[0])->is_hero())
                {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
                    tstring key("ui_value_left_the_party");
                    sprintf(buf, CS(g_ui_strings.lookup(key)), CS(troop((int)params[0])->display_name()));
                }
            else
                {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
                    tstring key("ui_number_value_left_the_party");
                    sprintf(buf, CS(g_ui_strings.lookup(key)), params[1], CS(troop((int)params[0])->display_name()));
                }
        }
        break;

    case op::heal_party:
        // the party argument is ignored: the main party's regular stacks are healed, and the player agent
        {
            GameX* g = game();
            int main = g->main_party;
            PartyMgr* pm = &g->parties;
            for (i = 0; i < ((PartyV*)pm->get(main))->num_stacks; i++) {
                if (!(((PartyV*)pm->get(main))->stacks[i].flags & 1)) {
                    TroopX* troops = g->troops;
                    int t = ((PartyV*)pm->get(main))->stacks[i].troop;
                    if (troops[t].is_hero()) {
                        float hp = (float)troops[((PartyV*)pm->get(main))->stacks[i].troop].max_hit_points();
                        troops[((PartyV*)pm->get(main))->stacks[i].troop].hit_points = hp;
                    } else
                        ((PartyV*)pm->get(main))->stacks[i].wounded = 0;
                    g = game();
                }
                main = g->main_party;
                pm = &g->parties;
            }
            if (g_mission && mission()->player_agent >= 0)
                agent(mission()->player_agent)->health = agent(mission()->player_agent)->max_health;
        }
        break;
    case op::disable_party:
        if (check_party((int)params[0]))
            party((int)params[0])->flags |= 0x100;          // pf_disabled
        break;
    case op::enable_party:
        if (check_party((int)params[0]))
            party((int)params[0])->flags &= ~(unsigned __int64)0x100;
        break;
    case op::remove_party:
        if (check_party((int)params[0]))
            ((GameW*)g_game)->remove_party((int)params[0]);
        break;
    case op::add_companion_party:
        // (add_companion_party, <troop_id_hero>)
        if (check_troop((int)params[0])) {
            int np = game()->create_companion_party((int)params[0], party_pos(party(game()->main_party)));
            party(np)->faction = 0;
            party(np)->flags |= 0x100000;                   // pf_limit_members
            party(np)->f8e4 = 6;
            party(np)->morale = party(game()->main_party)->morale;
            party(np)->f80 = party(game()->main_party)->f80;
        }
        break;

    // ---- sites and visitors --------------------------------------------------------------------------------------
    case op::add_troop_to_site:
        // (add_troop_to_site, <troop_id>, <scene_id>, <entry_no>)
        if (check_troop((int)params[0]))
            troop((int)params[0])->set_colors((int)params[1], (int)params[2]);
        break;
    case op::remove_troop_from_site:
        if (check_troop((int)params[0]))
            troop((int)params[0])->site = 0;
        break;
    case op::modify_visitors_at_site:
        // the old scene is validated, not the new one
        if (check_scene(g_visitor_scene))
            g_visitor_scene = (int)params[0];
        break;
    case op::reset_visitors:
        if (check_scene(g_visitor_scene))
            for (int k = 0; k < 64; k++) {
                scene(g_visitor_scene)->visitors[k].troop = -1;
                scene(g_visitor_scene)->visitors[k].dna = 0xffff2f33;
            }
        break;
    case op::set_visitor:
        // (set_visitor, <entry_no>, <troop_id>, [<dna>]); troop -1 clears the entry
        if (IN_RANGE(params[0], 0, 0x40) && (params[1] == -1 || check_troop((int)params[1]))) {
            scene(g_visitor_scene)->visitors[(int)params[0]].troop = (int)params[1];
            if (params[1] == -1) {
                scene(g_visitor_scene)->visitor_counts[(int)params[0]] = 0;
                break;
            }
            scene(g_visitor_scene)->visitor_counts[(int)params[0]] = 1;
            if (params[2])
                scene(g_visitor_scene)->visitors[(int)params[0]].dna = (int)params[2];
            if (g_mission)
                mission()->clear_done();
        }
        break;
    case op::set_visitors:
        // (set_visitors, <entry_no>, <troop_id>, <number_of_troops>)
        if (IN_RANGE(params[0], 0, 0x40) && check_troop((int)params[1]) && params[2] > 0) {
            scene(g_visitor_scene)->visitors[(int)params[0]].troop = (int)params[1];
            scene(g_visitor_scene)->visitors[(int)params[0]].dna = 0xffff2f33;
            scene(g_visitor_scene)->visitor_counts[(int)params[0]] = (int)params[2];
            if (g_mission)
                mission()->clear_done();
        }
        break;
    case op::add_visitors_to_current_scene:
        // (add_visitors_to_current_scene, <entry_no>, <troop_id>, <number_of_troops>): only during a mission
        if (g_mission && IN_RANGE(params[0], 0, 0x40) && check_troop((int)params[1]) && params[2] > 0) {
            scene(g_visitor_scene)->visitors[(int)params[0]].troop = (int)params[1];
            scene(g_visitor_scene)->visitors[(int)params[0]].dna = 0xffff2f33;
            scene(g_visitor_scene)->visitor_counts[(int)params[0]] = (int)params[2];
            mission()->clear_done();
            mission()->add_amount((int)params[0], (int)params[2]);
        }
        break;

    // ---- factions and quests -----------------------------------------------------------------------------------
    case op::set_relation:
    { int si_0; int si_1; float sf_0; float sf_1; float sf_2;
        // (set_relation, <faction_id>, <faction_id>, <value>): value -100..100, symmetric; the player is told when
        // one side is faction 0
        if (check_faction((int)params[0]) && check_faction((int)params[1])) {
            sf_0 = (float)params[2] * 0.01f;
            int old_rel = round_float(faction((int)params[0])->relations[(int)params[1]] * 100.0f);
            sf_1 = -1.0f, sf_2 = 1.0f;
            float c = clamp(&sf_0, &sf_1, &sf_2);
            faction((int)params[0])->relations[(int)params[1]] = c;
            faction((int)params[1])->relations[(int)params[0]] = c;
            int new_rel = round_float(faction((int)params[0])->relations[(int)params[1]] * 100.0f);
            if (params[0] == 0 || params[1] == 0) {
                si_0 = (int)params[1], si_1 = (int)params[0];
                if (old_rel < new_rel) {
                    {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
                        tstring key("ui_your_relations_with_value_has_improved_from_number_to_number");
                        sprintf(buf, CS(g_ui_strings.lookup(key)), CS(faction(imax_ref(&si_1, &si_0))->display_name()), old_rel, new_rel);
                    }
                    { tstring msg(buf); g_gui.push(msg, 0xff6fff28); }
                } else if (old_rel > new_rel) {
                    {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
                        tstring key("ui_your_relations_with_value_has_deteriorated_from_number_to_number");
                        sprintf(buf, CS(g_ui_strings.lookup(key)), CS(faction(imax_ref(&si_1, &si_0))->display_name()), old_rel, new_rel);
                    }
                    { tstring msg(buf); g_gui.push(msg, 0xffff7028); }
                }
                if (params[0] == 0)
                    ((GameW*)g_game)->mark_kind((int)params[1]);
                else if (params[1] == 0)
                    ((GameW*)g_game)->mark_kind((int)params[0]);
            }
        }
        break;
    }
    case op::faction_set_name:
        if (check_faction((int)params[0]) && check_string_arg((int)params[1], types[1]))
            { tstring text = g_strings.get_string((int)params[1], types[1]); tstring nm = g_script.expand(text);
              ((Named16*)faction((int)params[0]))->set(&nm); }
        break;
    case op::faction_set_color:
        if (check_faction((int)params[0])) {
            FactionRec* f = faction((int)params[0]);
            f->color = (int)params[1];
            f->color = (int)params[1] | 0xff000000;
        }
        break;
    case op::start_quest:
        // (start_quest, <quest_id>, [giver_troop_id]); the giver defaults to the conversation troop
        if (check_quest((int)params[0])) {
            __int64 giver;
            if (params[1] > 0 && check_troop((int)params[1]))
                giver = params[1];
            else
                giver = g_dialogs.fc;
            ((GameData*)g_game)->start_quest((int)params[0], (int)giver);
        }
        break;
    case op::complete_quest:
        if (check_quest((int)params[0]))
            ((GameW*)g_game)->quest_done((int)params[0]);
        break;
    case op::succeed_quest:
        if (check_quest((int)params[0]))
            ((GameW*)g_game)->quest_succeeded((int)params[0]);
        break;
    case op::fail_quest:
        if (check_quest((int)params[0]))
            ((GameW*)g_game)->quest_failed((int)params[0]);
        break;
    case op::cancel_quest:
        if (check_quest((int)params[0]))
            ((GameW*)g_game)->set_quest_cancelled((int)params[0]);
        break;
    case op::set_quest_progression:
        if (check_quest((int)params[0]))
            ((Troop*)quest((int)params[0]))->set_alpha((int)params[1]);
        break;
    case op::conclude_quest:
        if (check_quest((int)params[0]))
            ((GameW*)g_game)->set_quest_concluded((int)params[0]);
        break;
    case op::setup_quest_text:
        if (check_quest((int)params[0])) {
            quest((int)params[0])->title = g_script.expand(*quest((int)params[0])->raw_title());
            quest((int)params[0])->text = g_script.expand(*quest((int)params[0])->raw_text());
        }
        break;
    case op::setup_quest_giver:
        // (setup_quest_giver, <quest_id>, <string_id>)
        if (check_quest((int)params[0]) && check_string_arg((int)params[1], types[1]))
            quest((int)params[0])->giver = g_script.expand(g_strings.get_string((int)params[1], types[1]));
        break;

    // ---- encounters and merchandise ------------------------------------------------------------------------------
    case op::start_encounter:
        if (check_party((int)params[0]))
            ((GameData*)g_game)->deploy_party(game()->main_party, (int)params[0]);
        break;
    case op::leave_encounter:
        game()->encounter_flags |= 1;
        break;
    case op::encounter_attack:
        game()->encounter_flags2 |= 0x40;
        break;
    case op::select_enemy:
        if (params[0] == 0 || params[0] == 1)
            g_mission_ctx.select_side(1 - (int)params[0]);
        break;
    case op::set_passage_menu:
        if (check_menu((int)params[0]))
            g_mission_ctx.passage_menu = (int)params[0];
        break;
    case op::end_current_battle:
        g_mission_ctx.f164 |= 1;
        break;
    case op::set_mercenary_source_party:
        if (check_party((int)params[0]))
            game()->mercenary_source_party = (int)params[0];
        break;
    case op::set_merchandise_modifier_quality: {
    { float sf_0; float sf_1; float sf_2;
        // percentage, clamped to 0..1000 (as a factor 0..10)
        sf_0 = (float)params[0] * 0.01f, sf_1 = 0, sf_2 = 1000.0f;
        game()->merchandise_modifier_quality = clamp(&sf_0, &sf_1, &sf_2);
        break;
    }
    }
    case op::reset_item_probabilities: {
        // (reset_item_probabilities, [value]): default 100; only the low word of the operand is tested
        int v = (int)params[0];
        if (!v)
            v = 100;
        game()->sl_item_set_all((float)v * 0.01f);
        break;
    }
    case op::set_item_probability_in_merchandise:
        if (IN_RANGE(params[0], 0, g_item_kind_count))
            item_kind((int)params[0])->merchandise_probability = (float)params[1] * 0.01f;
        break;

    // ---- troops ------------------------------------------------------------------------------------------------
    case op::troop_is_hero:
        if (check_troop((int)params[0]))
            result = troop((int)params[0])->is_hero();
        break;
    case op::troop_is_wounded:
        if (check_troop((int)params[0]))
            result = ((GameData*)g_game)->agent_low((int)params[0]);
        break;
    case op::troop_set_type:
        if (check_troop((int)params[0])) {
            troop((int)params[0])->set_flags_low(clamp_index((int)params[1], 0, g_anim.char_count));
        }
        break;
    case op::troop_get_type:
        if (check_troop((int)params[1]))
            SET_LV((__int64)(int)(troop((int)params[1])->flags & 0xf));
        break;
    case op::troop_set_auto_equip:
        // value 0 disables auto-equipping (tf_is_merchant)
        if (check_troop((int)params[0])) {
            if (params[1])
                troop((int)params[0])->flags &= ~(unsigned __int64)0x1000;
            else
                troop((int)params[0])->flags |= 0x1000;
        }
        break;
    case op::troop_ensure_inventory_space:
        if (check_troop((int)params[0]))
            ((GameData*)g_game)->init_slots((int)params[0], (int)params[1]);
        break;
    case op::troop_sort_inventory:
        if (check_troop((int)params[0]))
            troop((int)params[0])->sort_inv();
        break;
    case op::troop_add_merchandise:
        // (troop_add_merchandise, <troop_id>, <item_type_id>, <value>)
        if (check_troop((int)params[0]))
            ((GameData*)g_game)->spawn_items((int)params[1], (int)params[2], (int)params[0]);
        break;
    case op::troop_get_xp:
        if (check_troop((int)params[1]))
            SET_LV((__int64)troop((int)params[1])->xp);
        break;
    case op::troop_raise_attribute:
        if (check_troop((int)params[0]) && params[2] && IN_RANGE(params[1], 0, 4))
            troop((int)params[0])->add_attr((int)params[1], (int)params[2]);
        break;
    case op::troop_raise_skill:
        if (check_troop((int)params[0]) && params[2] && IN_RANGE(params[1], 0, 0x2a))
            troop((int)params[0])->add_skill((int)params[1], (int)params[2]);
        break;
    case op::troop_raise_proficiency:
        if (check_troop((int)params[0]) && params[2] > 0 && IN_RANGE(params[1], 0, 7))
            troop((int)params[0])->adjust((int)params[1], (float)params[2]);
        break;
    case op::troop_raise_proficiency_linear:
        if (check_troop((int)params[0]) && params[2] && IN_RANGE(params[1], 0, 7))
            troop((int)params[0])->add_stat((int)params[1], (int)params[2]);
        break;
    case op::troop_add_proficiency_points:
        // at most 99 points at a time
        if (check_troop((int)params[0]) && params[1] > 0) {
            int* points = &troop((int)params[0])->proficiency_points;
            *points += clamp_index((int)params[1], 0, 100);
        }
        break;
    case op::troop_add_gold:
        if (check_troop((int)params[0]) && params[1])
            ((GameData*)g_game)->add_gold((int)params[1], (int)params[0]);
        break;
    case op::troop_remove_gold:
        if (params[1] > 0) {
            if (params[0] == 0)
                ((GameData*)g_game)->remove_gold((int)params[1], 0);
            else if (check_troop((int)params[0]))
                troop((int)params[0])->remove_gold((int)params[1]);
        }
        break;
    case op::troop_add_item:
        // (troop_add_item, <troop_id>, <item_id>, [modifier])
        if (check_troop((int)params[0]) && IN_RANGE(params[1], 0, g_item_kind_count)) {
            if (params[0] == (__int64)game()->player_troop)
                ((GameData*)g_game)->add_inventory((int)params[1], (int)params[2]);
            else {
                TroopX* t = troop((int)params[0]);
                int k = t->find_free();
                if (k >= 0)
                    t->set_slot((int)params[1], k, (int)params[2]);
            }
        }
        break;
    case op::troop_remove_item:
        if (check_troop((int)params[0])) {
            int n = ((GameData*)g_game)->sl_take((int)params[0], (int)params[1], 1, 1);
            if (params[0] == 0 && n > 0) {
                {   // the original's order: the key, the item name, then the lookup (the key dies after sprintf)
                    tstring key("ui_lost_item_value");
                    const char* name = CS(item_kind((int)params[1])->display_name());
                    sprintf(buf, CS(g_ui_strings.lookup(key)), name);
                }
                { tstring msg(buf); g_gui.push(msg, -1); }
            }
        }
        break;
    case op::troop_clear_inventory:
        if (check_troop((int)params[0]))
            clear_slots(troop((int)params[0])->items);
        break;
    case op::troop_equip_items:
        if (check_troop((int)params[0]))
            troop((int)params[0])->auto_equip(-1, 0, 0, 0);
        break;
    case op::troop_inventory_slot_set_item_amount:
        // (troop_inventory_slot_set_item_amount, <troop_id>, <inventory_slot_no>, <value>); 0 empties the slot.
        // The low byte of the packed word holds the missing amount (max - value).
        if (check_troop((int)params[0]) && params[1] >= 0 &&
            params[1] < (__int64)(troop((int)params[0])->inv_count() + 10)) {
            if (params[1] < 10) {
                if (params[2] > 0) {
                    int mx = troop((int)params[0])->equipment[(int)params[1]].max_amount();
                    ItemRef& r = troop((int)params[0])->equipment[(int)params[1]];
                    r.b.amount = 0;
                    r.m4 |= mx - (int)params[2];
                } else
                    troop((int)params[0])->equipment[(int)params[1]].item = -1;
            } else {
                if (params[2] > 0) {
                    int mx = troop((int)params[0])->items[(int)params[1] - 10].max_amount();
                    ItemRef& r = troop((int)params[0])->items[(int)params[1] - 10];
                    r.b.amount = 0;
                    r.m4 |= mx - (int)params[2];
                } else
                    troop((int)params[0])->items[(int)params[1] - 10].item = -1;
            }
        }
        break;
    case op::troop_inventory_slot_get_item_amount:
        // (troop_inventory_slot_get_item_amount, <destination>, <troop_id>, <inventory_slot_no>)
        if (check_troop((int)params[1]) && params[2] >= 0 &&
            params[2] < (__int64)(troop((int)params[1])->inv_count() + 10)) {
            if (params[2] < 10)
                SET_LV((__int64)troop((int)params[1])->equipment[(int)params[2]].room_left());
            else
                SET_LV((__int64)troop((int)params[1])->items[(int)params[2] - 10].room_left());
        }
        break;
    case op::troop_inventory_slot_get_item_max_amount:
        if (check_troop((int)params[1]) && params[2] >= 0 &&
            params[2] < (__int64)(troop((int)params[1])->inv_count() + 10)) {
            if (params[2] < 10)
                SET_LV((__int64)troop((int)params[1])->equipment[(int)params[2]].max_amount());
            else
                SET_LV((__int64)troop((int)params[1])->items[(int)params[2] - 10].max_amount());
        }
        break;
    case op::troop_add_items:
        // (troop_add_items, <troop_id>, <item_id>, <number>): a count <= 0 is ignored before the id checks
        if (params[2] > 0 && check_troop((int)params[0]) && check_item_kind((int)params[1])) {
            ((GameData*)g_game)->give_item((int)params[0], (int)params[1], (int)params[2]);
            if (params[0] == 0) {
                {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
                    tstring key("ui_got_number_value");
                    sprintf(buf, CS(g_ui_strings.lookup(key)), (int)params[2], CS(item_kind((int)params[1])->display_name()));
                }
                { tstring msg(buf); g_gui.push(msg, -1); }
            }
        }
        break;
    case op::troop_remove_items:
        // puts the value of the removed items into reg0; a count <= 0 is ignored before the id checks
        if (params[2] > 0 && check_troop((int)params[0]) && check_item_kind((int)params[1])) {
            g_script.reg[0] = ((GameData*)g_game)->sl_take((int)params[0], (int)params[1], (int)params[2], 1);
            if (params[0] == 0) {
                {   // the key first, the arguments, then the lookup (the original's order); the key dies after sprintf
                    tstring key("ui_lost_number_value");
                    sprintf(buf, CS(g_ui_strings.lookup(key)), (int)params[2], CS(item_kind((int)params[1])->display_name()));
                }
                { tstring msg(buf); g_gui.push(msg, -1); }
            }
        }
        break;

    case op::troop_loot_troop:
    { float sf_0;
        // (troop_loot_troop, <target_troop>, <source_troop_id>, <probability>): every itp_always_loot item, then ten
        // random draws weighted by the source's level against the item value
        if (params[2] > 0 && check_troop((int)params[0]) && check_troop((int)params[1])) {
            i = troop((int)params[1])->inv_count();
            TroopX* t = troop((int)params[0]);
            int dst = t->items[0].check_slots(t->inv_count());
            for (int k = 0; k < i; k++) {
                TroopX* troops = game()->troops;
                if (dst < troops[(int)params[0]].inv_count()) {
                    ItemRef* src = &troops[(int)params[1]].items[k];
                    if (src->item >= 0 && (item_kind(src->item)->type_flags & itp_always_loot)) {
                        troops[(int)params[0]].items[dst] = *src;
                        dst++;
                    }
                }
            }
            for (int n = 10; n; n--) {
                int k = rand_below(&i);
                TroopX* troops = game()->troops;
                if (dst < troops[(int)params[0]].inv_count() && troops[(int)params[1]].items[k].item >= 0) {
                    float one = 1.0f;
                    sf_0 = ((float)troops[(int)params[1]].level + 10.0f) * 10.0f /
                              item_kind(troops[(int)params[1]].items[k].item)->value;
                    float chance = fmin_ref(&one, &sf_0) * ((float)params[2] * 0.01f);
                    if (frand() < chance) {
                        ItemRef* src = &troop((int)params[1])->items[k];
                        if (!(item_kind(src->item)->type_flags & (itp_unique | itp_always_loot))) {
                            troop((int)params[0])->items[dst] = *src;
                            IR(troop((int)params[0])->items[dst]).randomize_modifier(0.1f);
                            dst++;
                        }
                    }
                }
            }
        }
        break;
    }
    case op::troop_get_inventory_capacity:
        if (check_troop((int)params[1]))
            SET_LV((__int64)(troop((int)params[1])->inv_count() + 10));
        else
            SET_LV(0);
        break;
    case op::troop_get_inventory_slot:
        // (troop_get_inventory_slot, <destination>, <troop_id>, <inventory_slot_no>); -1 when invalid
        if (check_troop((int)params[1]) && params[2] >= 0 &&
            params[2] < (__int64)(troop((int)params[1])->inv_count() + 10)) {
            if (params[2] < 10)
                SET_LV((__int64)troop((int)params[1])->equipment[(int)params[2]].item);
            else
                SET_LV((__int64)troop((int)params[1])->items[(int)params[2] - 10].item);
        } else
            SET_LV(-1);
        break;
    case op::troop_get_inventory_slot_modifier:
        if (check_troop((int)params[1]) && params[2] >= 0 &&
            params[2] < (__int64)(troop((int)params[1])->inv_count() + 10)) {
            if (params[2] < 10)
                SET_LV((__int64)troop((int)params[1])->equipment[(int)params[2]].b.mod);
            else
                SET_LV((__int64)troop((int)params[1])->items[(int)params[2] - 10].b.mod);
        } else
            SET_LV(-1);
        break;
    case op::troop_set_inventory_slot:
        if (check_troop((int)params[0]) && params[1] >= 0 &&
            params[1] < (__int64)(troop((int)params[0])->inv_count() + 10)) {
            if (params[1] < 10)
                IR(troop((int)params[0])->equipment[(int)params[1]]).set_item((int)params[2]);
            else
                IR(troop((int)params[0])->items[(int)params[1] - 10]).set_item((int)params[2]);
        }
        break;
    case op::troop_set_inventory_slot_modifier:
        if (check_troop((int)params[0]) && params[1] >= 0 &&
            params[1] < (__int64)(troop((int)params[0])->inv_count() + 10)) {
            if (params[1] < 10)
                IR(troop((int)params[0])->equipment[(int)params[1]]).set_modifier((int)params[2]);
            else
                IR(troop((int)params[0])->items[(int)params[1] - 10]).set_modifier((int)params[2]);
        }
        break;
    case op::troop_set_faction:
        if (check_troop((int)params[0]) && check_faction((int)params[1]))
            troop((int)params[0])->faction = (int)params[1];
        break;
    case op::troop_set_health:
        // (troop_set_health, <troop_id>, <relative health (0-100)>, [absolute])
        if (check_troop((int)params[0])) {
            TroopX* t = troop((int)params[0]);
            if (params[2]) {
                t->hit_points = (float)clamp_index((int)params[1], 0, t->max_hit_points() + 1);
            } else {
                t->hit_points = (float)clamp_index((int)((__int64)t->max_hit_points() * params[1] / 100), 0,
                                                   t->max_hit_points() + 1);
            }
        }
        break;
    case op::troop_get_upgrade_troop:
        // (troop_get_upgrade_troop, <destination>, <troop_id>, <upgrade_path>): path 0 or 1
        if (check_troop((int)params[1]) && IN_RANGE(params[2], 0, 2)) {
            if (params[2] == 0)
                SET_LV((__int64)(unsigned)troop((int)params[1])->upgrade_troop[0]);
            else
                SET_LV((__int64)(unsigned)troop((int)params[1])->upgrade_troop[1]);
        }
        break;
    case op::item_get_type:
        if (check_item_kind((int)params[1]))
            SET_LV((__int64)(item_kind((int)params[1])->type_flags & 0xff));
        break;

    // ---- parties -----------------------------------------------------------------------------------------------
    case op::party_get_num_companions:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->sum_head());
        break;
    case op::party_get_num_prisoners:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->sum_flagged());
        break;
    case op::party_set_flags:
        // (party_set_flags, <party_id>, <flag>, <clear_or_set>)
        if (check_party((int)params[0])) {
            if (params[2])
                party((int)params[0])->flags |= params[1];
            else
                party((int)params[0])->flags &= ~params[1];
            party((int)params[0])->update_flags();
            party((int)params[0])->f8b5 = 1;
        }
        break;
    case op::party_set_extra_text:
        if (check_party((int)params[0]) && check_string_arg((int)params[1], types[1]))
            ASSIGN_STR(party((int)params[0])->extra_text, g_script.expand(g_strings.get_string((int)params[1], types[1])));
        break;
    case op::party_get_current_terrain:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->current_terrain);
        break;
    case op::party_get_template_id:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->template_id);
        break;
    case op::party_add_members:
        // stores the number added to reg0
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            g_script.reg[0] = ((GameData*)g_game)->add_members((int)params[0], (int)params[1], (int)params[2], 0);
        break;
    case op::party_add_prisoners:
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            g_script.reg[0] = ((GameData*)g_game)->add_members((int)params[0], (int)params[1], (int)params[2], 1);
        break;
    case op::party_force_add_members:
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            party((int)params[0])->force_add_members((int)params[1], (int)params[2], 0, 0);
        break;
    case op::party_force_add_prisoners:
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            party((int)params[0])->force_add_members((int)params[1], (int)params[2], 1, 0);
        break;
    case op::party_add_leader:
        // (party_add_leader, <party_id>, <troop_id>, [<number>]): inserted as the first stack (not into party 0)
        if (check_party((int)params[0]) && params[0] != 0 && check_troop((int)params[1])) {
            int max_index = 0x3f;
            int n = min_int(party((int)params[0])->num_stacks, max_index);
            party((int)params[0])->num_stacks = n + 1;
            int k;
            for (k = n; k > 0; k--)
                party((int)params[0])->stacks[k] = party((int)params[0])->stacks[k - 1];
            party((int)params[0])->stacks[k].clear();
            party((int)params[0])->stacks[k].troop = (int)params[1];
            int count = (int)params[2], one = 1;
            party((int)params[0])->stacks[k].count = imax_ref(&one, &count);
        }
        break;
    case op::party_remove_members:
        // stores the number removed to reg0
        g_script.reg[0] = 0;
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            g_script.reg[0] = ((GameData*)g_game)->remove_troops((int)params[0], (int)params[1], (int)params[2], 0, 0);
        break;
    case op::party_remove_prisoners:
        g_script.reg[0] = 0;
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            g_script.reg[0] = ((GameData*)g_game)->remove_troops((int)params[0], (int)params[1], (int)params[2], 1, 0);
        break;
    case op::party_clear:
        // the main party keeps its first stack (the player)
        if (check_party((int)params[0])) {
            party((int)params[0])->num_stacks = 0;
            if (params[0] == (__int64)game()->main_party)
                party((int)params[0])->num_stacks = 1;
        }
        break;
    case op::party_wound_members:
    { int si_0;
        // (party_wound_members, <party_id>, <troop_id>, <number>): the first regular stack of the troop
        if (check_party((int)params[0])) {
            PartyMgr* pm = &game()->parties;
            int p = (int)params[0];
            int found = -1;
            for (i = 0; found == -1 && i < ((PartyV*)pm->get(p))->num_stacks; i++)
                if ((__int64)((PartyV*)pm->get(p))->stacks[i].troop == params[1] &&
                    !(((PartyV*)pm->get(p))->stacks[i].flags & 1))
                    found = i;
            if (found >= 0) {
                si_0 = ((PartyV*)pm->get(p))->stacks[found].wounded + (int)params[2];
                int v = min_int(si_0, ((PartyV*)pm->get(p))->stacks[found].count);
                if (v < 0)
                    v = 0;
                ((PartyV*)pm->get(p))->stacks[found].wounded = v;
            }
        }
        break;
    }
    case op::party_remove_members_wounded_first:
        g_script.reg[0] = 0;
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            g_script.reg[0] = ((GameData*)g_game)->remove_troops((int)params[0], (int)params[1], (int)params[2], 0, 1);
        break;
    case op::party_set_faction:
        if (check_party((int)params[0]) && check_faction((int)params[1])) {
            party((int)params[0])->faction = (int)params[1];
            party((int)params[0])->f8b5 = 1;
        }
        break;

    case op::party_relocate_near_party:
        // (party_relocate_near_party, <party_id>, <target_party_id>, <value_spawn_radius>)
        if (check_party((int)params[0]) && check_party((int)params[1])) {
            GameX* g = game();
            PartyV* target = party((int)params[1]);
            g->set_pos(target->pos.x, target->pos.y, (int)params[0], (float)params[2]);
        }
        break;
    case op::party_get_position:
        if (check_register((int)params[0]) && check_party((int)params[1])) {
            ASSIGN_MAT3(*(rgl::mat3*)&script_pos((int)params[0]), IDENTITY3_VALUE());
            script_pos((int)params[0]).o = party_pos(party((int)params[1]));
        }
        break;
    case op::party_set_position:
        if (check_party((int)params[0]) && check_register((int)params[1])) {
            party_pos(party((int)params[0])) = script_pos((int)params[1]).o;
            party((int)params[0])->place_on_map();
        }
        break;
    case op::map_get_random_position_around_position:
        // (map_get_random_position_around_position, <dest_position_no>, <position_no>, <radius>)
        if (check_register((int)params[0]) && check_register((int)params[1]) && params[2] >= 0)
            ((GameW*)game())->random_position_around(*(const Vec2*)&script_pos((int)params[1]).o, (float)params[2], (Vec2*)&script_pos((int)params[0]).o);
        break;
    case op::map_get_land_position_around_position:
        result = 0;
        if (check_register((int)params[0]) && check_register((int)params[1]) && params[2] >= 0)
            result = ((GameW*)game())->position_around(*(const Vec2*)&script_pos((int)params[1]).o, (float)params[2], (Vec2*)&script_pos((int)params[0]).o, 0);
        break;
    case op::map_get_water_position_around_position:
        result = 0;
        if (check_register((int)params[0]) && check_register((int)params[1]) && params[2] >= 0)
            result = ((GameW*)game())->position_around(*(const Vec2*)&script_pos((int)params[1]).o, (float)params[2], (Vec2*)&script_pos((int)params[0]).o, 1);
        break;
    case op::party_count_members_of_type:
        // (party_count_members_of_type, <destination>, <party_id>, <troop_id>)
        if (check_party((int)params[1]) && check_troop((int)params[2])) {
            int t = (int)params[2], p = (int)params[1];
            int prisoners = ((GameData*)g_game)->count_troops(t, p, 1);
            SET_LV((__int64)(((GameData*)g_game)->count_troops(t, p, 0) + prisoners));
        }
        break;
    case op::party_count_companions_of_type:
    case op::party_count_prisoners_of_type:
        if (check_party((int)params[1]) && check_troop((int)params[2]))
            SET_LV((__int64)((GameData*)g_game)->count_troops((int)params[2], (int)params[1],
                                                              opcode == op::party_count_prisoners_of_type));
        break;
    case op::party_get_free_companions_capacity:
        if (check_party((int)params[1])) {
            int stacks = party((int)params[1])->max_stacks();       // the original's call order
            if (stacks - party((int)params[1])->first_flagged() < 1)
                SET_LV(0);
            else {
                int members = party((int)params[1])->sum_head();
                SET_LV((__int64)(party((int)params[1])->max_members() - members));
            }
        }
        break;
    case op::party_get_free_prisoners_capacity:
        if (check_party((int)params[1])) {
            int stacks = party((int)params[1])->max_stacks();       // the original's call order
            if (stacks - party((int)params[1])->count_tail() < 1)
                SET_LV(0);
            else {
                int members = party((int)params[1])->sum_flagged();
                SET_LV((__int64)(party((int)params[1])->max_prisoners() - members));
            }
        }
        break;
    case op::party_set_ai_initiative:
        // (party_set_ai_initiative, <party_id>, <value>): the original clamps the party id (not the value) to
        // 0..100 and stores it as the initiative
        if (check_party((int)params[0])) {
            float initiative = (float)clamp_index((int)params[0], 0, 101) * 0.01f;
            party((int)params[0])->ai_initiative = initiative;
        }
        break;
    case op::party_set_ai_behavior:
        if (check_party((int)params[0])) {
            PartyMgr* pm = &game()->parties;
            int p = (int)params[0];
            if (((PartyV*)pm->get(p))->ai_behavior == ai_bhvr_in_town ||
                ((PartyV*)pm->get(p))->ai_behavior == ai_bhvr_negotiate_party)
                ((PartyV*)pm->get(p))->ai_behavior = ai_bhvr_hold;
            party((int)params[0])->f8e4 = (int)params[1];
            party((int)params[0])->ai_state = 0;
        }
        break;
    case op::party_set_ai_object:
        if (check_party((int)params[0]) && check_party((int)params[1]))
            party((int)params[0])->ai_target_object = (int)params[1];
        break;
    case op::party_set_ai_target_position:
        if (check_party((int)params[0]) && check_register((int)params[1])) {
            PartyV* p = party((int)params[0]);
            p->ai_target_x = script_pos((int)params[1]).o.x;
            p->ai_target_y = script_pos((int)params[1]).o.y;
        }
        break;
    case op::party_set_ai_patrol_radius:
        if (check_party((int)params[0])) {
            float r = (float)params[1];
            party((int)params[0])->ai_patrol_radius = r;
        }
        break;
    case op::party_ignore_player:
        // (party_ignore_player, <party_id>, <duration_in_hours>)
        if (check_party((int)params[0])) {
            int h = (int)params[1];
            if (h < 0)
                h = 0;
            party((int)params[0])->ignore_player_timer.set_delay((float)-h);
        }
        break;
    case op::party_set_bandit_attraction:
        if (check_party((int)params[0])) {
            float a = (float)params[1] * 0.01f;
            party((int)params[0])->bandit_attraction = a;
        }
        break;
    case op::party_get_num_companion_stacks:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->first_flagged());
        break;
    case op::party_get_num_prisoner_stacks:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->count_tail());
        break;
    case op::party_stack_get_troop_id:
        if (check_party((int)params[1]) && IN_RANGE(params[2], 0, 0x40))
            SET_LV((__int64)party((int)params[1])->stacks[(int)params[2]].troop);
        break;
    case op::party_stack_get_size:
        if (check_party((int)params[1]) && IN_RANGE(params[2], 0, 0x40))
            SET_LV((__int64)party((int)params[1])->stacks[(int)params[2]].count);
        break;
    case op::party_stack_get_num_wounded:
        if (check_party((int)params[1]) && IN_RANGE(params[2], 0, 0x40))
            SET_LV((__int64)party((int)params[1])->stacks[(int)params[2]].wounded);
        break;
    case op::party_stack_get_troop_dna:
        // the main party's stacks keep their dna in the game's stack rows
        if (check_party((int)params[1]) && IN_RANGE(params[2], 0, 0x40)) {
            if (params[1] != (__int64)game()->main_party)
                SET_LV((__int64)((GameW*)g_game)->stack_dna((int)params[1], (int)params[2], 0, 0));
            else
                SET_LV((__int64)*(int*)(game()->stack_rows + (int)params[2] * 0x80));
        }
        break;
    case op::party_prisoner_stack_get_troop_id:
        if (check_party((int)params[1])) {
            PartyMgr* pm = &game()->parties;
            int k = ((PartyV*)pm->get((int)params[1]))->first_flagged() + (int)params[2];
            if (k >= 0 && k < 0x40)
                SET_LV((__int64)((PartyV*)pm->get((int)params[1]))->stacks[k].troop);
        }
        break;
    case op::party_prisoner_stack_get_size:
        if (check_party((int)params[1])) {
            int k = party((int)params[1])->first_flagged() + (int)params[2];
            if (k >= 0 && k < 0x40)
                SET_LV((__int64)party((int)params[1])->stacks[k].count);
        }
        break;
    case op::party_prisoner_stack_get_troop_dna:
        if (check_party((int)params[1])) {
            GameX* g = game();
            int k = party((int)params[1])->first_flagged() + (int)params[2];
            if (k >= 0 && k < 0x40) {
                if (params[1] != (__int64)g->main_party)
                    SET_LV((__int64)g->stack_dna((int)params[1], k, 0, 0));
                else
                    SET_LV((__int64)*(int*)(g->stack_rows + k * 0x80));
            }
        }
        break;

    case op::party_attach_to_party:
        if (check_party((int)params[0]) && check_party((int)params[1])) {
            ((GameW*)g_game)->set_objective((int)params[0], (int)params[1]);
            party((int)params[0])->f8cc = 0;
        }
        break;
    case op::party_detach:
        if (check_party((int)params[0]))
            ((GameW*)g_game)->detach_party((int)params[0]);
        break;
    case op::party_collect_attachments_to_party:
        if (check_party((int)params[0]) && check_party((int)params[1])) {
            GameX* g = game();
            g->collect_attachments((int)params[0], (PartyV*)g->parties.get((int)params[1]));
        }
        break;
    case op::party_quick_attach_to_current_battle:
        // (party_quick_attach_to_current_battle, <party_id>, <side (0:players side, 1:enemy side)>)
        if (check_party((int)params[0]))
            ((GameW*)g_game)->switch_objective((int)params[0], (int)params[1]);
        break;
    case op::party_get_cur_town:
        // the town the party is in (ai_bhvr_in_town), else -1
        if (check_party((int)params[1])) {
            PartyMgr* pm = &game()->parties;
            int p = (int)params[1];
            if (((PartyV*)pm->get(p))->ai_behavior == ai_bhvr_in_town)
                SET_LV((__int64)((PartyV*)pm->get(p))->ai_object);
            else
                SET_LV(-1);
        }
        break;
    case op::party_leave_cur_battle:
        if (check_party((int)params[0]))
            party((int)params[0])->leave_battle();
        break;
    case op::party_set_next_battle_simulation_time:
        if (check_party((int)params[0])) {
            GameX* g = game();
            int p = (int)params[0];
            if (((PartyV*)g->parties.get(p))->battle >= 0) {
                float t = (float)params[1];
                g->battles.get(((PartyV*)g->parties.get(p))->battle)->next_simulation_time = t;
            }
        }
        break;
    case op::party_set_name:
        if (check_party((int)params[0]) && check_string_arg((int)params[1], types[1]))
            // the string first (get_string, expand), then the party record (PartyMgr::get), as the original
            { const tstring* s_; (s_ = &str_ref(g_script.expand(g_strings.get_string((int)params[1], types[1]))),
                                  party((int)params[0])->set_name(*s_)); }
        break;
    case op::party_add_xp_to_stack:
        if (check_party((int)params[0]) && params[1] >= 0) {
            PartyMgr* pm = &game()->parties;
            int p = (int)params[0];
            if (params[1] < (__int64)((PartyV*)pm->get(p))->num_stacks && params[2] > 0)
                ((PartyV*)pm->get(p))->add_xp_to_stack((int)params[1], (int)params[2]);
        }
        break;
    case op::party_get_morale:
        if (check_party((int)params[1])) {
            SET_LV((__int64)round_float(party((int)params[1])->morale * 100.0f));
        }
        break;
    case op::party_set_morale:
        // (party_set_morale, <party_id>, <value>), #value is clamped to range [0...100].
        if (check_party((int)params[0])) {
            float m = (float)clamp_index((int)params[1], 0, 101) * 0.01f;
            party((int)params[0])->morale = m;
        }
        break;
    case op::party_upgrade_with_xp:
        // (party_upgrade_with_xp, <party_id>, <xp_amount>, <upgrade_path>), upgrade_path 0 random, 1 first, 2 second
        if (check_party((int)params[0]) && params[1] > 0 && IN_RANGE(params[2], 0, 3))
            ((GameW*)g_game)->award_experience((int)params[0], (float)params[1], (int)params[2]);
        break;
    case op::party_add_xp:
        if (check_party((int)params[0]) && params[1] > 0)
            ((GameW*)g_game)->share_experience((int)params[0], (float)params[1]);
        break;
    case op::party_add_template:
        // (party_add_template, <party_id>, <party_template_id>, [reverse_prisoner_status])
        if (check_party((int)params[0]) && check_party_template((int)params[1]))
            ((GameData*)g_game)->add_template((int)params[0], (int)params[1], (int)params[2]);
        break;
    case op::party_set_icon:
        if (check_party((int)params[0]) && check_map_icon((int)params[1])) {
            party((int)params[0])->set_id((int)params[1]);
            party((int)params[0])->update_flags();
        }
        break;
    case op::party_set_banner_icon:
        if (check_party((int)params[0]) && check_map_icon((int)params[1])) {
            party((int)params[0])->banner_icon = (int)params[1];
            party((int)params[0])->update_flags();
        }
        break;
    case op::party_add_particle_system:
        if (check_party((int)params[0]) && warn_particle_system((int)params[1]))
            party((int)params[0])->add((int)params[1]);
        break;
    case op::party_clear_particle_systems:
        if (check_party((int)params[0]))
            party((int)params[0])->clear_particle_systems();
        break;
    case op::party_get_battle_opponent:
        // -1, then the root party of the other side of the battle the party's root is in
        if (check_party((int)params[1])) {
            GameX* g = game();
            int root = g->battle_root((int)params[1]);
            int b = ((PartyV*)g->parties.get(root))->battle;
            SET_LV(-1);
            if (b >= 0) {
                BattleTable* bt = &game()->battles;
                if (bt->get(b)->party_b == root)
                    SET_LV((__int64)bt->get(b)->party_a);
                else
                    SET_LV((__int64)bt->get(b)->party_b);
            }
        }
        break;
    case op::party_get_icon:
        if (check_party((int)params[1]))
            SET_LV((__int64)(unsigned)((unsigned)party((int)params[1])->flags & 0xff));
        break;
    case op::party_get_skill_level:
        {
            int v = 0;
            if (check_party((int)params[1]) && IN_RANGE(params[2], 0, 0x2a))
                v = party((int)params[1])->skill_level((int)params[2]);
            SET_LV((__int64)v);
        }
        break;

    case op::get_battle_advantage:
        SET_LV((__int64)g_mission_ctx.f1c);
        break;
    case op::set_battle_advantage:
        g_mission_ctx.f1c = clamp_index((int)params[0], -32, 6);
        break;
    case op::party_get_attached_to:
        {
            int v = -1;
            if (check_party((int)params[1]))
                v = party((int)params[1])->attached_to;
            SET_LV((__int64)v);
        }
        break;
    case op::party_get_num_attached_parties:
        {
            int n = 0;
            if (check_party((int)params[1]))
                n = ((std::vector<int>&)party((int)params[1])->attached).size();
            SET_LV((__int64)n);
        }
        break;
    case op::party_get_attached_party_with_rank:
        // no lower bound on the rank (the original reads before the vector for a negative one)
        {
            int v = -1;
            if (check_party((int)params[1])) {
                PartyMgr* pm = &game()->parties;
                int p = (int)params[1];
                unsigned n = ((std::vector<int>&)((PartyV*)pm->get(p))->attached).size();
                if (params[2] < (__int64)n)
                    v = ((std::vector<int>&)((PartyV*)pm->get(p))->attached)[(int)params[2]];
            }
            SET_LV((__int64)v);
        }
        break;
    case op::inflict_casualties_to_party_group:
        // (inflict_casualties_to_party_group, <parent_party_id>, <attack_rounds>, <party_id_to_add_causalties_to>)
        if (check_party((int)params[0])) {
            if (params[2] != (__int64)game()->main_party && check_party((int)params[2])) {
                GameX* g = game();
                g->recruit((int)params[0], (int)params[1], (PartyV*)g->parties.get((int)params[2]));
            } else
                ((GameW*)g_game)->recruit((int)params[0], (int)params[1], 0);
        }
        break;
    case op::distribute_party_among_party_group:
        if (check_party((int)params[0]) && check_party((int)params[1])) {
            GameX* g = game();
            g->handout((PartyV*)g->parties.get((int)params[0]), (int)params[1]);
        }
        break;

    // ---- agents -------------------------------------------------------------------------------------------------
    case op::get_player_agent_no:
        if (g_mission)
            SET_LV((__int64)mission()->player_agent);
        break;
    case op::get_player_agent_kill_count:
        // (get_player_agent_kill_count, <destination>, [get_wounded])
        if (params[1] == 0)
            SET_LV((__int64)game()->player_kill_count);
        else
            SET_LV((__int64)game()->player_wounded_count);
        break;
    case op::get_player_agent_own_troop_kill_count:
        if (params[1] == 0)
            SET_LV((__int64)game()->player_own_troop_kill_count);
        else
            SET_LV((__int64)game()->player_own_troop_wounded_count);
        break;
    case op::agent_is_alive:
        if (warn_agent((int)params[0]))
            result = agent((int)params[0])->state == 1;
        break;
    case op::agent_is_wounded:
        if (warn_agent((int)params[0]))
            result = agent((int)params[0])->state == 4;
        break;
    case op::agent_is_human:
        if (warn_agent((int)params[0]))
            result = agent((int)params[0])->kind == 0;
        break;
    case op::agent_is_ally:
        if (warn_agent((int)params[0])) {
            int pa = mission()->player_agent;
            if (pa >= 0) {
                AgentTable* t = &mission()->agents;
                result = t->get(pa)->is_friend(t->get((int)params[0]));
            }
        }
        break;
    case op::agent_is_defender:
        // in a siege (mode 2) the defenders are the player's side unless the player attacks (side 1)
        if (warn_agent((int)params[0])) {
            MissionX* m = mission();
            int pa = m->player_agent;
            if (pa >= 0) {
                if (m->mode != 0 && (m->mode != 2 || m->mode_side != 1)) {
                    AgentTable* t = &m->agents;
                    result = t->get(pa)->is_enemy(t->get((int)params[0]));
                } else {
                    AgentTable* t = &m->agents;
                    result = t->get(pa)->is_friend(t->get((int)params[0]));
                }
            }
        }
        break;
    case op::agent_get_look_position:
        if (check_register((int)params[0]) && warn_agent((int)params[1])) {
            ASSIGN_MAT3(*(rgl::mat3*)&script_pos((int)params[0]), IDENTITY3_VALUE());
            rgl::matrix* dst = &script_pos((int)params[0]);
            AgentTable* t = &mission()->agents;
            int a = (int)params[1];
            dst->o = t->get(a)->pos;
            if (t->get(a)->body) {
                dst->f = *(rgl::vector4*)&t->get(a)->fx;      // the facing direction (+0x55d8)
                dst->orthonormalize();
            }
        }
        break;
    case op::agent_get_position:
        if (check_register((int)params[0]) && warn_agent((int)params[1])) {
            AgentTable* t = &mission()->agents;
            int a = (int)params[1];
            if (t->get(a)->body)
                script_pos((int)params[0]) = (rgl::matrix&)t->get(a)->body->m;      // the body frame (+0x58)
            else {
                ASSIGN_MAT3(*(rgl::mat3*)&script_pos((int)params[0]), IDENTITY3_VALUE());
                script_pos((int)params[0]).o = t->get(a)->pos;
            }
        }
        break;
    case op::agent_set_position:
        if (check_register((int)params[1]) && warn_agent((int)params[0])) {
            agent((int)params[0])->pos = script_pos((int)params[1]).o;
            agent((int)params[0])->spawn_corpse();
        }
        break;
    case op::agent_get_horse:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->horse);
        break;
    case op::agent_get_rider:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->rider);
        break;
    case op::agent_get_party_id:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->party);
        break;
    case op::agent_get_entry_no:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->entry_no);
        break;
    case op::agent_get_troop_id:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->troop);
        break;
    case op::agent_get_item_id:
        // (works only for horses, returns -1 otherwise)
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->f8c.item);
        break;
    case op::store_agent_hit_points:
        // [absolute] 1 = hit points, else percent of the maximum
        if (warn_agent((int)params[1])) {
            if (params[2] == 1)
                SET_LV((__int64)agent((int)params[1])->health);
            else {
                AgentTable* t = &mission()->agents;
                int a = (int)params[1];
                float h = t->get(a)->health * 100.0f;
                SET_LV((__int64)(h / t->get(a)->max_health));
            }
        }
        break;
    case op::agent_set_hit_points:
        // [absolute] 1 = hit points, else percent of the maximum; clamped to 0..max
        if (warn_agent((int)params[0])) {
            if (params[2] == 1) {
                AgentTable* t = &mission()->agents;
                int a = (int)params[0];
                float h = (float)clamp_index((int)params[1], 0, (int)(__int64)(t->get(a)->max_health + 1.0f));
                t->get(a)->health = h;
            } else {
                AgentTable* t = &mission()->agents;
                int a = (int)params[0];
                float h = (float)clamp_index((int)(__int64)((float)params[1] * t->get(a)->max_health * 0.01f), 0,
                                             (int)(__int64)(t->get(a)->max_health + 1.0f));
                t->get(a)->health = h;
            }
        }
        break;
    case op::agent_deliver_damage_to_agent:
        // a blow from <agent_id_deliverer> as strong as its best wielded damage, at the victim's position
        if (warn_agent((int)params[0]) && warn_agent((int)params[1])) {
            AgentTable* t = &mission()->agents;
            int victim = (int)params[1];
            if (t->get(victim)->state == 1) {
                Blow b;
                b.init();
                int attacker = (int)params[0];
                b.flags = 0x20;
                b.bone = 0;
                b.attacker = attacker;
                b.pos = t->get(victim)->pos;
                int best = 0;
                int d = (int)t->get(attacker)->slot_pair(0).stat_21x(0);
                if (d > 0)
                    best = d;
                d = (int)agent((int)params[0])->slot_pair(0).stat_21x(1);
                if (best < d)
                    best = d;
                d = (int)agent((int)params[0])->slot_pair(1).stat_21x(0);
                if (best < d)
                    best = d;
                d = (int)agent((int)params[0])->slot_pair(1).stat_21x(1);
                if (best < d)
                    best = d;
                b.dmg_in = (float)best;
                agent((int)params[1])->note_blow(&b);
                agent((int)params[1])->process_blow(0);
            }
        }
        break;
    case op::agent_get_kill_count:
        // (agent_get_kill_count, <destination>, <agent_id>, [get_wounded])
        if (warn_agent((int)params[1])) {
            if (params[2] == 0)
                SET_LV((__int64)agent((int)params[1])->kill_count);
            else
                SET_LV((__int64)agent((int)params[1])->wounded_count);
        }
        break;
    case op::agent_get_wielded_item:
        // (agent_get_wielded_item, <destination>, <agent_id>, <hand_no>)
        if (warn_agent((int)params[1]) && IN_RANGE(params[2], 0, 2)) {
            ItemRef r = agent((int)params[1])->slot_pair((int)params[2]);
            SET_LV((__int64)r.item);
        }
        break;
    case op::agent_get_ammo:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->total_missing_ammo());
        break;
    case op::agent_refill_ammo:
        if (warn_agent((int)params[0]))
            agent((int)params[0])->clear_ammo((int)params[1]);
        break;
    case op::agent_has_item_equipped:
        if (warn_agent((int)params[0]) && check_item_kind((int)params[1])) {
            AgentTable* t = &mission()->agents;
            int a = (int)params[0];
            result = 0;
            for (int k = 0; k < 10; ++k)
                if ((__int64)t->get(a)->equipment[k].item == params[1])
                    result = 1;
        }
        break;
    case op::agent_set_scripted_destination:
        // [auto_set_z_to_ground_level] inside the scene bounds only
        if (warn_agent((int)params[0]) && check_register((int)params[1])) {
            MissionX* m = mission();
            if (params[2]) {
                rgl::vector4* o = &script_pos((int)params[1]).o;
                if (m->min_x <= o->x && o->x < m->max_x && m->min_y <= o->y && o->y < m->max_y) {
                    script_pos((int)params[1]).o.z = m->scene->floor_z(&o->x);
                    m = mission();
                }
            }
            (rgl::matrix&)agent((int)params[0])->scripted_destination = script_pos((int)params[1]);
            agent((int)params[0])->scripted_mode = 1;
            agent((int)params[0])->f2c0 = 0;
        }
        break;
    case op::agent_get_scripted_destination:
        if (warn_agent((int)params[1]) && check_register((int)params[0]))
            script_pos((int)params[0]) = (rgl::matrix&)agent((int)params[1])->scripted_destination;
        break;
    case op::agent_clear_scripted_mode:
        if (warn_agent((int)params[0]))
            agent((int)params[0])->scripted_mode = 0;
        break;
    case op::agent_set_speed_limit:
        // km/h to m/s
        if (warn_agent((int)params[0])) {
            float v = (float)params[1] * 0.2777778f;
            agent((int)params[0])->speed_limit = v;
        }
        break;
    case op::agent_ai_set_always_attack_in_melee:
        if (warn_agent((int)params[0]))
            agent((int)params[0])->always_attack_in_melee = (int)params[1];
        break;
    case op::agent_get_simple_behavior:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->simple_behavior);
        break;
    case op::agent_get_combat_state:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->combat_state);
        break;
    case op::agent_set_animation:
        if (warn_agent((int)params[0]))
            agent((int)params[0])->set_order(0, (int)params[1], 0x500007f, 1.0f);
        break;
    case op::agent_set_stand_animation:
        if (warn_agent((int)params[0]))
            agent((int)params[0])->stand_animation = (int)params[1];
        break;
    case op::agent_set_walk_forward_animation:
        if (warn_agent((int)params[0]))
            agent((int)params[0])->walk_forward_animation = (int)params[1];
        break;
    case op::agent_set_animation_progress:
        // the fraction part of the fixed-point value
        if (warn_agent((int)params[0])) {
            __int64 frac = params[1] % (__int64)g_script.fixed_point_multiplier;
            ((Agent_f8*)agent((int)params[0]))->set_slot(0, (float)frac / (float)g_script.fixed_point_multiplier);
        }
        break;
    case op::agent_play_sound:
    { int si_0;
        if (warn_agent((int)params[0]) && warn_sound((int)params[1])) {
            si_0 = (int)params[1];
            agent((int)params[0])->play_sound(&si_0);
        }
        break;
    }
    case op::agent_get_team:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->clamped_team());
        break;
    case op::agent_set_team:
    { int si_0;
        // an agent joining a team without a leader becomes its leader
        if (warn_agent((int)params[0])) {
            si_0 = (int)params[1];
            ((NamedSelRec*)agent((int)params[0]))->set_sel(&si_0);
            MissionX* m = mission();
            int a = (int)params[0];
            TeamRec* tr = &m->teams[m->agents.get(a)->clamped_team()];
            if (tr->leader < 0)
                tr->leader = m->agents.get(a)->id;
        }
        break;
    }
    case op::agent_get_class:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)((AgentX*)agent((int)params[1]))->agent_class());
        break;

    // ---- teams and orders ---------------------------------------------------------------------------------------
    case op::class_is_listening_order:
        // (class_is_listening_order, <team_no>, <sub_class>)
        if (IN_RANGE(params[0], 0, 8) && IN_RANGE(params[1], 0, 3))
            result = mission()->teams[(int)params[0]].cls[(int)params[1]].use == 1;
        break;
    case op::entry_point_get_position:
        if (check_register((int)params[0]) && IN_RANGE(params[1], 0, 0x40) && g_mission)
            script_pos((int)params[0]) = mission()->entries[(int)params[1]].pos;
        break;
    case op::team_get_movement_order:
        if (IN_RANGE(params[1], 0, 8)) {
            mission()->select_team((int)params[1]);
            mission()->select_class((int)params[1], (int)params[2]);
            SET_LV((__int64)mission()->movement_order((int)params[1]));
            mission()->unselect_team((int)params[1]);
        }
        break;
    case op::team_get_riding_order:
        if (IN_RANGE(params[1], 0, 8)) {
            mission()->select_team((int)params[1]);
            mission()->select_class((int)params[1], (int)params[2]);
            SET_LV((__int64)mission()->riding_order((int)params[1]));
            mission()->unselect_team((int)params[1]);
        }
        break;
    case op::team_get_weapon_usage_order:
        if (IN_RANGE(params[1], 0, 8)) {
            mission()->select_team((int)params[1]);
            mission()->select_class((int)params[1], (int)params[2]);
            SET_LV((__int64)mission()->weapon_usage_order((int)params[1]));
            mission()->unselect_team((int)params[1]);
        }
        break;
    case op::team_get_hold_fire_order:
        // (team_get_hold_fire_order, <destination>, <team_no>, <sub_class>)
        if (IN_RANGE(params[1], 0, 8)) {
            mission()->select_team((int)params[1]);
            mission()->select_class((int)params[1], (int)params[2]);
            SET_LV((__int64)mission()->hold_fire_order((int)params[1]));
            mission()->unselect_team((int)params[1]);
        }
        break;
    case op::teams_are_enemies:
        if (IN_RANGE(params[0], 0, 8) && IN_RANGE(params[1], 0, 8))
            result = mission()->teams[(int)params[0]].relation[(int)params[1]] < 0;
        break;

    case op::team_give_order:
        // (team_give_order, <team_no>, <sub_class>, <order_id>): a negative sub class orders the whole team
        if (IN_RANGE(params[0], 0, 8) && IN_RANGE(params[2], 0, 0xe)) {
            mission()->select_team((int)params[0]);
            if (params[1] >= 0)
                mission()->select_class((int)params[0], (int)params[1]);
            mission()->order((int)params[0], (int)params[2]);
            mission()->unselect_team((int)params[0]);
        }
        break;
    case op::team_set_order_position:
    { int si_0; float sf_0; float sf_1; float sf_2;
        // (team_set_order_position, <team_no>, <sub_class>, <position_no>)
        if (IN_RANGE(params[0], 0, 8) && check_register((int)params[2]) &&
            mission()->point_in_rect(&script_pos((int)params[2]).o)) {
            mission()->select_team((int)params[0]);
            mission()->select_class((int)params[0], (int)params[1]);
            MissionX* m = mission();
            rgl::vector4* p = &script_pos((int)params[2]).o;
            sf_0 = 0, sf_1 = 0, sf_2 = 0;
            si_0 = m->scene->nearest_face((const float*)p);
            rgl::vector4 off;
            TEAM_SEL(m, (int)params[0])->set_vec((const int*)p, &si_0, (const int*)&off.set(sf_2, sf_1, sf_0));
            mission()->unselect_team((int)params[0]);
        }
        break;
    }
    case op::team_get_leader:
        if (IN_RANGE(params[1], 0, 8))
            SET_LV((__int64)mission()->teams[(int)params[1]].leader);
        break;
    case op::team_set_leader:
        if (IN_RANGE(params[0], 0, 8) && warn_agent((int)params[1]))
            mission()->teams[(int)params[0]].leader = (int)params[1];
        break;
    case op::team_get_order_position:
        // (team_get_order_position, <position_no>, <team_no>, <sub_class>)
        if (IN_RANGE(params[1], 0, 8) && check_register((int)params[0])) {
            mission()->select_team((int)params[1]);
            mission()->select_class((int)params[1], (int)params[2]);
            MissionX* m = mission();
            int t = (int)params[1];
            script_pos((int)params[0]).o = TEAM_SEL(m, t)->first();
            m->unselect_team(t);
        }
        break;
    case op::team_set_order_listener:
        if (IN_RANGE(params[0], 0, 8))
            mission()->select_class((int)params[0], (int)params[1]);
        break;
    case op::team_set_relation:
        // (team_set_relation, <team_no>, <team_no_2>, <value>), # -1 for enemy, 1 for friend, 0 for neutral
        if (IN_RANGE(params[0], 0, 8) && IN_RANGE(params[1], 0, 8) && IN_RANGE(params[2], -1, 2)) {
            mission()->teams[(int)params[0]].relation[(int)params[1]] = (int)params[2];
            mission()->teams[(int)params[1]].relation[(int)params[0]] = (int)params[2];
        }
        break;

    // ---- scene, weather and scene props -------------------------------------------------------------------------
    case op::set_rain:
        // (set_rain, <rain-type>, <strength>), (rain_type: 1= rain, 2=snow ; strength: 0 - 100)
        if (g_mission) {
            mission()->rain_type = (int)params[0];
            mission()->rain_strength = (float)params[1] * 0.01f;
            if (params[0] && params[1] > 50)
                mission()->rain_f65fc = 3;
        }
        break;
    case op::set_fog_distance:
    { unsigned su_0; float sf_0; float sf_1;
        // (set_fog_distance, <distance_in_meters>, [fog_color]): 0 = the colour of the scene's environment row
        if (g_mission) {
            mission()->fog_distance = (float)params[0];
            if (mission()->scene) {
                MissionX* m = mission();
                su_0 = g_env_rows[m->env_row].f44;
                if (params[1])
                    su_0 = (unsigned)params[1];
                sf_0 = (float)params[0];
                sf_1 = 1.0f;
                m->scene->set_fog(&sf_1, &sf_0, (const int*)&su_0);
            }
        }
        break;
    }
    case op::get_scene_boundaries:
        if (check_register((int)params[0]) && check_register((int)params[1])) {
            ASSIGN_MAT(script_pos((int)params[0]), IDENTITY_VALUE());
            ASSIGN_MAT(script_pos((int)params[1]), IDENTITY_VALUE());
            MissionX* m = mission();
            script_pos((int)params[0]).o.x = m->rect_min_x;
            script_pos((int)params[0]).o.y = m->rect_min_y;
            script_pos((int)params[1]).o.x = m->rect_max_x;
            script_pos((int)params[1]).o.y = m->rect_max_y;
        }
        break;
    case op::scene_prop_get_num_instances:
        if (warn_scene_prop((int)params[1]))
            SET_LV((__int64)mission()->count_props((int)params[1]));
        break;
    case op::scene_prop_get_instance:
        // fails when there is no such instance
        if (warn_scene_prop((int)params[1])) {
            int n = mission()->find_prop((int)params[1], (int)params[2]);
            if (n >= 0)
                SET_LV((__int64)n);
            else
                result = 0;
        }
        break;
    case op::prop_instance_get_position:
        if (check_register((int)params[0]) && warn_scene_prop_instance((int)params[1]) && g_mission)
            ASSIGN_MAT(script_pos((int)params[0]), prop_at(mission()->props, (int)params[1])->frame);
        break;
    case op::prop_instance_get_starting_position:
        if (check_register((int)params[0]) && warn_scene_prop_instance((int)params[1]) && g_mission)
            ASSIGN_MAT(script_pos((int)params[0]), prop_at(mission()->props, (int)params[1])->start_frame);
        break;
    case op::prop_instance_get_scale:
        // the scale as the diagonal of an otherwise identity frame
        if (check_register((int)params[0]) && warn_scene_prop_instance((int)params[1]) && g_mission) {
            rgl::matrix* dst = &script_pos((int)params[0]);
            ASSIGN_MAT(*dst, IDENTITY_VALUE());
            PropVec* pv = &mission()->props;
            int k = (int)params[1];
            dst->s.x = prop_at(*pv, k)->scale_x;
            dst->f.y = prop_at(*pv, k)->scale_y;
            dst->u.z = prop_at(*pv, k)->scale_z;
        }
        break;
    case op::prop_instance_set_position:
        if (warn_scene_prop_instance((int)params[0]) && check_register((int)params[1]) && g_mission)
            prop_at(mission()->props, (int)params[0])->set_position(&script_pos((int)params[1]));
        break;
    case op::prop_instance_animate_to_position:
    { float sf_0; float sf_1;
        // (prop_instance_animate_to_position, <scene_prop_id>, position, <duration-in-1/100-seconds>)
        if (warn_scene_prop_instance((int)params[0]) && check_register((int)params[1]) && g_mission) {
            sf_0 = 0;
            sf_1 = (float)params[2] * 0.01f;
            prop_at(mission()->props, (int)params[0])->animate_to(&script_pos((int)params[1]), &sf_1, &sf_0);
        }
        break;
    }
    case op::replace_scene_props:
        // only in the scene-load trigger (context -19)
        if (warn_scene_prop((int)params[0]) && warn_scene_prop((int)params[1]) && *context == -19)
            mission()->retag_props((int)params[0], (int)params[1]);
        break;


    // ---- mission control, lights, particles, spawning -----------------------------------------------------------
    case op::set_mission_result:
        mission()->result = (int)params[0];
        break;
    case op::finish_mission:
        mission()->start_clock((int)params[0]);
        break;
    case op::jump_to_scene:
        // (jump_to_scene, <scene_id>, <entry_no>): the entry is set with set_jump_entry
        if (check_scene((int)params[0]))
            game()->jump_scene = (int)params[0];
        break;
    case op::set_jump_mission:
        game()->jump_mission = (int)params[0];
        break;
    case op::set_jump_entry:
        if (IN_RANGE(params[0], 0, 0x40))
            game()->jump_entry = (int)params[0];
        break;
    case op::start_mission_conversation:
        {
            MissionX* m = mission();
            m->conversation = m->find_agent_by_troop((int)params[0]);
        }
        break;
    case op::add_reinforcements_to_entry:
        mission()->check_slot((int)params[0], (int)params[1]);
        break;
    case op::mission_enable_talk:
        game()->talk_enabled = 1;
        break;
    case op::mission_disable_talk:
        game()->talk_disabled = 1;
        break;
    case op::mission_tpl_entry_set_override_flags:
        // (mission_entry_set_override_flags, <mission_template_id>, <entry_no>, <value>)
        if (check_mission_template((int)params[0]) && params[1] >= 0) {
            MissionTemplate* t = &g_mission_templates[(int)params[0]];
            if (params[1] < (__int64)t->num_entries)
                t->entries[(int)params[1]].override_flags = (int)params[2];
        }
        break;
    case op::mission_tpl_entry_clear_override_items:
        if (check_mission_template((int)params[0]) && params[1] >= 0) {
            MissionTemplate* t = &g_mission_templates[(int)params[0]];
            if (params[1] < (__int64)t->num_entries)
                t->entries[(int)params[1]].num_override_items = 0;
        }
        break;
    case op::mission_tpl_entry_add_override_item:
        // at most 8 override items per entry
        if (check_mission_template((int)params[0]) && params[1] >= 0 &&
            params[1] < (__int64)g_mission_templates[(int)params[0]].num_entries &&
            check_item_kind((int)params[2])) {
            SpawnRec* e = &g_mission_templates[(int)params[0]].entries[(int)params[1]];
            int n = e->num_override_items;
            if (n < 8) {
                e->override_items[n] = (int)params[2];
                ++g_mission_templates[(int)params[0]].entries[(int)params[1]].num_override_items;
            }
        }
        break;
    case op::set_current_color:
    { float sf_0; float sf_1; float sf_2; rgl::vector4 sv_0;
        // red, green, blue: a value of 255 means 100% (the original scales by 1/100)
        {
            sf_0 = (float)params[2] * 0.01f;
            sf_1 = (float)params[1] * 0.01f;
            sf_2 = (float)params[0] * 0.01f;
            sv_0;
            game()->current_color = sv_0.set(sf_2, sf_1, sf_0);
        }
        break;
    }
    case op::set_position_delta:
    { float sf_0; float sf_1; float sf_2; rgl::vector4 sv_0;
        {
            sf_0 = (float)params[2] * 0.01f;
            sf_1 = (float)params[1] * 0.01f;
            sf_2 = (float)params[0] * 0.01f;
            sv_0;
            game()->position_delta = sv_0.set(sf_2, sf_1, sf_0);
        }
        break;
    }
    case op::add_point_light:
        // (add_point_light, [flicker_magnitude], [flicker_interval]): on the trigger's item (context -50) or the scene
        if (params[0] >= 0 && params[1] >= 0) {
            if (*context == -50) {
                SceneEntity* ent = (SceneEntity*)trigger_entity(-50);
                if (ent) {
                    int mesh = trigger_item_mesh(-50);
                    Light* l = new Light;
                    l->flicker_magnitude = (float)params[0] * 0.01f;
                    l->flicker_interval = (float)params[1] * 0.01f;
                    l->color = game()->current_color;
                    l->type = 0;
                    l->v34 = game()->position_delta;
                    l->base_color = l->color;
                    ((TimeKey*)l)->calc();
                    if (mesh >= 0 && ent->meshes)
                        ent->meshes->call1(mesh, l);
                }
            } else
                mission()->add_wind_effect((float)params[0] * 0.01f, (float)params[1] * 0.01f);
        }
        break;
    case op::particle_system_add_new:
    { int si_0; int si_1;
        // (particle_system_add_new, <par_sys_id>, [position_no]): on the trigger's entity, at the position delta
        if (warn_particle_system((int)params[0])) {
            int ctx = *context;
            SceneEntity* ent = (SceneEntity*)trigger_entity(ctx);
            if (ent) {
                int mesh = trigger_item_mesh(ctx);
                si_0 = 0;
                si_1 = (int)params[0];
                void* ps = g_emit_defs.spawn(&si_1, &game()->position_delta, &si_0);
                if (ps) {
                    if (mesh >= 0) {
                        if (ent->meshes)
                            ent->meshes->call0(mesh, ps);
                    } else
                        ent->add_particle_system(ps);
                }
            }
        }
        break;
    }
    case op::particle_system_emit:
        // (particle_system_emit, <par_sys_id>, <value_num_particles>, <value_period>)
        if (warn_particle_system((int)params[0])) {
            int ctx = *context;
            SceneEntity* ent = (SceneEntity*)trigger_entity(ctx);
            if (ent) {
                int mesh = trigger_item_mesh(ctx);
                PSys* ps;
                if (mesh >= 0) {
                    if (!ent->meshes)
                        break;
                    ps = ent->meshes->find_particle_system(mesh, (int)params[0]);
                } else {
                    int k = ent->find_particle_system((int)params[0]);
                    if (k < 0)
                        break;
                    ps = ent->particle_systems[k];
                }
                if (ps)
                    ps->reset_pending((int)params[1]);
            }
        }
        break;
    case op::particle_system_burst:
        // (particle_system_burst, <par_sys_id>, <position_no>, [percentage_burst_strength]) default 100
        if (warn_particle_system((int)params[0]) && check_register((int)params[1]) && g_mission) {
            int strength = 100;
            if (params[2])
                strength = (int)params[2];
            mission()->spawn_effect((int)params[0], &script_pos((int)params[1]), strength);
        }
        break;
    case op::set_spawn_position:
        if (check_register((int)params[0]))
            game()->trigger_object_pos = script_pos((int)params[0]);
        break;
    case op::spawn_item:
        // (spawn_item, <item_kind_id>, <item_modifier>): the instance id goes to reg0
        if (check_item_kind((int)params[0])) {
            ItemRef r;
            r.item = (int)params[0];
            r.set_modifier((int)params[1]);
            r.reduce(0);
            g_script.reg[0] = mission()->place(&r);
        }
        break;
    case op::spawn_agent:
        // (spawn_agent, <troop_id>), (stores agent_id in reg0)
        g_script.reg[0] = -1;
        if (check_troop((int)params[0]))
            g_script.reg[0] = mission()->exec_trigger((int)params[0]);
        break;
    case op::spawn_horse:
        g_script.reg[0] = -1;
        if (check_item_kind((int)params[0]))
            g_script.reg[0] = mission()->enter_trigger((int)params[0], (int)params[1]);
        break;
    case op::spawn_scene_prop:
        // opcode 1974 (spawn_scene_prop in later versions): only validates the prop id in 1.011
        warn_scene_prop((int)params[0]);
        break;


    // ---- tableaus -----------------------------------------------------------------------------------------------
    case op::cur_tableau_add_tableau_mesh:
        // (cur_tableau_add_tableau_mesh, <tableau_material_id>, <value>, <position_register_no>, [scale_fixed_point])
        if (warn_tableau((int)params[0]) && check_register((int)params[2]) && g_tableaus.worlds.size() > 0) {
            // rgl::vector4 (set returns the pointer the call leaves in eax) and the address of the operator*
            // temporary itself (C4238), as the original: no copy into a third vector
            rgl::vector4 a, b;
            const rgl::vector4* scale;
            if (params[3] == 0) {
                float z = 1.0f, y = 1.0f, x = 1.0f;
                scale = &a.set(x, y, z);
            } else {
                float s = 1.0f / g_script.fixed_point_multiplier;
                float x, y, z;
                x = y = z = (float)params[3];
                scale = &(b.set(z, y, x) * s);
            }
            g_tableaus.spawn((int)params[0], (int)params[1], &script_pos((int)params[2]), scale);
        }
        break;
    case op::cur_item_set_tableau_material:
        // only inside ti_on_init_item (context -50): the item, else the agent's mesh object
        if (warn_tableau((int)params[0]) && *context == -50) {
            TriggerItem* it = (TriggerItem*)trigger_item(-50);
            if (it) {
                int code = (int)params[1], tab = (int)params[0];
                it->set_owner(g_tableaus.add_spawned(&tab, &code));
            } else {
                TriggerMeshObj* o = (TriggerMeshObj*)trigger_mesh_obj(-50);
                if (o) {
                    int code = (int)params[1], tab = (int)params[0];
                    o->set_part((mesh_part*)g_tableaus.add_spawned(&tab, &code));
                }
            }
        }
        break;
    case op::cur_scene_prop_set_tableau_material:
    { int si_0; int si_1;
        // only inside ti_on_init_scene_prop (context -40)
        if (warn_tableau((int)params[0]) && *context == -40) {
            TriggerMeshObj* o = (TriggerMeshObj*)trigger_mesh_obj(-40);
            if (o) {
                si_0 = (int)params[1], si_1 = (int)params[0];
                o->set_part((mesh_part*)g_tableaus.add_spawned(&si_1, &si_0));
            }
        }
        break;
    }
    case op::cur_map_icon_set_tableau_material:
    { int si_0; int si_1;
        // only inside ti_on_init_map_icon (context -70)
        if (warn_tableau((int)params[0]) && *context == -70) {
            TriggerMeshObj* o = (TriggerMeshObj*)trigger_mesh_obj(-70);
            if (o) {
                si_0 = (int)params[1], si_1 = (int)params[0];
                o->set_part((mesh_part*)g_tableaus.add_spawned(&si_1, &si_0));
            }
        }
        break;
    }
    case op::cur_tableau_render_as_alpha_mask:
        if (g_tableaus.worlds.size() > 0)
            g_tableaus.alpha_mask = 1;
        break;
    case op::cur_tableau_set_background_color:
        if (g_tableaus.worlds.size() > 0)
            g_tableaus.set_background_color((int)params[0]);
        break;
    case op::cur_tableau_set_ambient_light:
    { float sf_0; float sf_1; float sf_2; rgl::vector4 sv_0;
        // the original divides each component by the int multiplier: a float local divisor gets rewritten as one
        // reciprocal and three multiplies (different rounding). Same in camera_parameters / point_light / sun_light
        // (cur_tableau_set_ambient_light, <red_fixed_point>, <green_fixed_point>, <blue_fixed_point>)
        if (g_tableaus.worlds.size() > 0) {
            sf_0 = (float)params[2] / g_script.fixed_point_multiplier;
            sf_1 = (float)params[1] / g_script.fixed_point_multiplier;
            sf_2 = (float)params[0] / g_script.fixed_point_multiplier;
            sv_0;
            g_tableaus.set_ambient_light((const float*)&sv_0.set(sf_2, sf_1, sf_0));
        }
        break;
    }
    case op::cur_tableau_set_camera_position:
        if (check_register((int)params[0]) && g_tableaus.worlds.size() > 0)
            g_tableaus.set_camera_position(&script_pos((int)params[0]));
        break;
    case op::cur_tableau_set_camera_parameters:
    { float sf_0; float sf_1; float sf_2; float sf_3;
        // (.., <is_perspective>, <camera_width_times_1000>, <camera_height_times_1000>, <camera_near_times_1000>,
        // <camera_far_times_1000>): fixed point in fact
        if (g_tableaus.worlds.size() > 0) {
            sf_0 = (float)params[4] / g_script.fixed_point_multiplier;
            sf_1 = (float)params[3] / g_script.fixed_point_multiplier;
            sf_2 = (float)params[2] / g_script.fixed_point_multiplier;
            sf_3 = (float)params[1] / g_script.fixed_point_multiplier;
            bool perspective = params[0] != 0;
            g_tableaus.set_camera_parameters((const char*)&perspective, &sf_3, &sf_2, &sf_1, &sf_0);
        }
        break;
    }
    case op::cur_tableau_add_point_light:
    { float sf_0; float sf_1; float sf_2;
        // the position is operand 0 and the colour operands 1..3 (the header's <map_icon_id> is not read)
        if (check_register((int)params[0]) && g_tableaus.worlds.size() > 0) {
            sf_0 = (float)params[3] / g_script.fixed_point_multiplier;
            sf_1 = (float)params[2] / g_script.fixed_point_multiplier;
            sf_2 = (float)params[1] / g_script.fixed_point_multiplier;
            rgl::vector4 c;
            g_tableaus.add_point_light(&script_pos((int)params[0]), c.set(sf_2, sf_1, sf_0));
        }
        break;
    }
    case op::cur_tableau_add_sun_light:
    { float sf_0; float sf_1; float sf_2;
        if (check_register((int)params[0]) && g_tableaus.worlds.size() > 0) {
            sf_0 = (float)params[3] / g_script.fixed_point_multiplier;
            sf_1 = (float)params[2] / g_script.fixed_point_multiplier;
            sf_2 = (float)params[1] / g_script.fixed_point_multiplier;
            rgl::vector4 c;
            g_tableaus.add_sun_light(&script_pos((int)params[0]), c.set(sf_2, sf_1, sf_0));
        }
        break;
    }
    case op::cur_tableau_add_mesh:
        // (cur_tableau_add_mesh, <mesh_id>, <position_no>, <scale_fixed_point>, <alpha_fixed_point>)
        if (check_mesh((int)params[0]) && check_register((int)params[1]) && g_tableaus.worlds.size() > 0) {
            int color = -1;
            float alpha;
            if (params[3] == 0)
                alpha = 1.0f;
            else
                alpha = (float)params[3] / g_script.fixed_point_multiplier;
            // rgl::vector4 (set returns the pointer the call leaves in eax) and the address of the operator*
            // temporary itself (C4238), as the original: no copy into a third vector
            rgl::vector4 a, b;
            const rgl::vector4* scale;
            if (params[2] == 0) {
                float z = 1.0f, y = 1.0f, x = 1.0f;
                scale = &a.set(x, y, z);
            } else {
                float s = 1.0f / g_script.fixed_point_multiplier;
                float x, y, z;
                x = y = z = (float)params[2];
                scale = &(b.set(z, y, x) * s);
            }
            g_tableaus.spawn_scaled((int)params[0], &script_pos((int)params[1]), scale, &alpha, (const unsigned*)&color);
        }
        break;
    case op::cur_tableau_add_mesh_with_vertex_color:
        // (.., <mesh_id>, <position_no>, <scale_fixed_point>, <alpha_fixed_point>, <vertex_color>)
        if (check_mesh((int)params[0]) && check_register((int)params[1]) && g_tableaus.worlds.size() > 0) {
            int color = (int)params[4];
            float alpha;
            if (params[3] == 0)
                alpha = 1.0f;
            else
                alpha = (float)params[3] / g_script.fixed_point_multiplier;
            // rgl::vector4 (set returns the pointer the call leaves in eax) and the address of the operator*
            // temporary itself (C4238), as the original: no copy into a third vector
            rgl::vector4 a, b;
            const rgl::vector4* scale;
            if (params[2] == 0) {
                float z = 1.0f, y = 1.0f, x = 1.0f;
                scale = &a.set(x, y, z);
            } else {
                float s = 1.0f / g_script.fixed_point_multiplier;
                float x, y, z;
                x = y = z = (float)params[2];
                scale = &(b.set(z, y, x) * s);
            }
            g_tableaus.spawn_scaled((int)params[0], &script_pos((int)params[1]), scale, &alpha, (const unsigned*)&color);
        }
        break;
    case op::cur_tableau_add_map_icon:
    { float sf_0;
        // (cur_tableau_add_map_icon, <map_icon_id>, <position_no>, <scale_fixed_point>)
        if (check_map_icon((int)params[0]) && check_register((int)params[1]) && g_tableaus.worlds.size() > 0) {
            sf_0;
            if (params[2] == 0)
                sf_0 = 1.0f;
            else
                sf_0 = (float)params[2] / g_script.fixed_point_multiplier;
            g_tableaus.spawn_scaled2((int)params[0], &script_pos((int)params[1]), &sf_0);
        }
        break;
    }
    case op::cur_tableau_add_troop:
    { int si_0; int si_1;
        // (.., <troop_id>, <position_no>, <animation_id>, <instance_no>): instance <= 0 = no random face (heroes)
        if (check_troop((int)params[0]) && check_register((int)params[1]) && check_animation((int)params[2]) &&
            g_tableaus.worlds.size()) {
            si_0;
            if (params[3] > 0)
                si_0 = (int)params[3];
            else
                si_0 = -1;
            si_1 = (int)params[2];
            tableau_add_agent((int)params[0], -1, &script_pos((int)params[1]), &si_1, &si_0);
        }
        break;
    }
    case op::cur_tableau_add_horse:
    { int si_0; int si_1;
        // (cur_tableau_add_horse, <item_id>, <position_no>, <animation_id>)
        if (check_item_kind((int)params[0]) && check_register((int)params[1]) && check_animation((int)params[2]) &&
            g_tableaus.worlds.size()) {
            si_0 = (int)params[2];
            si_1 = -1;
            tableau_add_agent(-1, (int)params[0], &script_pos((int)params[1]), &si_0, &si_1);
        }
        break;
    }
    case op::cur_tableau_set_override_flags:
        if (g_tableaus.worlds.size() > 0)
            tableau_overrides()->override_flags = (int)params[0];
        break;
    case op::cur_tableau_clear_override_items:
        if (g_tableaus.worlds.size() > 0)
            tableau_overrides()->num_override_items = 0;
        break;
    case op::cur_tableau_add_override_item:
        // no bound on the item count
        if (g_tableaus.worlds.size() > 0) {
            int n = tableau_overrides()->num_override_items;
            tableau_overrides()->override_items[n] = (int)params[0];
            ++tableau_overrides()->num_override_items;
        }
        break;
    case op::cur_tableau_add_mesh_with_scale_and_vertex_color:
    { int si_0; float sf_0; float sf_1;
        // (.., <mesh_id>, <position_no>, <scale_position_no>, <alpha_fixed_point>, <vertex_color>): the scale is the
        // origin of the second position, in fixed point
        if (check_mesh((int)params[0]) && check_register((int)params[1]) && check_register((int)params[2]) &&
            g_tableaus.worlds.size()) {
            si_0 = (int)params[4];
            sf_0;
            if (params[3] == 0)
                sf_0 = 1.0f;
            else
                sf_0 = (float)params[3] / g_script.fixed_point_multiplier;
            sf_1 = 1.0f / g_script.fixed_point_multiplier;
            g_tableaus.spawn_scaled((int)params[0], &script_pos((int)params[1]),
                                &(script_pos((int)params[2]).o * sf_1), &sf_0, (const unsigned*)&si_0);
        }
        break;
    }


    // ---- mission camera and time speed --------------------------------------------------------------------------
    case op::mission_cam_set_mode:
        // (mission_cam_set_mode, <mission_cam_mode>, <duration-in-1/1000-seconds>, <value>): leaving manual mode
        // animates from the current frame
        if (g_mission) {
            MissionX* m = mission();
            m->cam_mode = clamp_index((int)params[0], 0, 2);
            if (mission()->cam_mode == 0 && params[1] > 0) {
                mission()->cam_target_frame = mission()->get_frame();
                mission()->cam_target_aperture = mission()->blend_param();
                mission()->cam_anim_time = (float)params[1] * 0.001f;
                mission()->cam_timer.restart();
                mission()->cam_easing = 0;
                if (params[2] > 0)
                    mission()->cam_easing = (int)params[2];
            } else
                mission()->cam_anim_time = 0;
        }
        break;
    case op::mission_get_time_speed:
        if (g_mission)
            SET_LV((__int64)(mission()->blend2_param() * g_script.fixed_point_multiplier));
        break;
    case op::mission_set_time_speed:
        // (mission_set_time_speed, <value_fixed_point>) #this works only when cheat mode is enabled
        if (g_mission && params[0] > 0) {
            mission()->time_speed = (float)params[0] / g_script.fixed_point_multiplier;
            mission()->time_speed_anim_time = 0;
        }
        break;
    case op::mission_time_speed_move_to_value:
        // (mission_speed_move_to_value, <value_fixed_point>, <duration-in-1/1000-seconds>)
        if (g_mission && params[0] > 0 && params[1] > 0) {
            MissionX* m = mission();
            m->time_speed = m->blend2_param();
            mission()->target_time_speed = (float)params[0] / g_script.fixed_point_multiplier;
            mission()->time_speed_anim_time = (float)params[1] * 0.001f;
            mission()->time_speed_timer.restart();
        }
        break;
    case op::mission_cam_get_position:
        if (g_mission && check_register((int)params[0]))
            script_pos((int)params[0]) = mission()->get_frame();
        break;
    case op::mission_cam_set_position:
        if (g_mission && check_register((int)params[0])) {
            mission()->cam_frame = script_pos((int)params[0]);
            mission()->cam_anim_time = 0;
        }
        break;
    case op::mission_cam_animate_to_position:
        // (.., <position_register_no>, <duration-in-1/1000-seconds>, <value>) # value 0: linear velocity
        if (g_mission && check_register((int)params[0]) && params[1] > 0) {
            mission()->cam_frame = mission()->get_frame();
            mission()->cam_target_frame = script_pos((int)params[0]);
            MissionX* m = mission();
            m->cam_target_aperture = m->blend_param();
            mission()->cam_anim_time = (float)params[1] * 0.001f;
            mission()->cam_timer.restart();
            mission()->cam_easing = 0;
            if (params[2] > 0)
                mission()->cam_easing = (int)params[2];
        }
        break;
    case op::mission_cam_get_aparture:
        if (g_mission)
            SET_LV((__int64)mission()->blend_param());
        break;
    case op::mission_cam_set_aparture:
        // clamped to 37..149
        if (g_mission) {
            MissionX* m = mission();
            m->cam_aperture = (float)clamp_index((int)params[0], 0x25, 0x96);
            mission()->cam_anim_time = 0;
        }
        break;
    case op::mission_cam_animate_to_aparture:
        // (mission_cam_animate_to_aparture, <value>, <duration-in-1/1000-seconds>, <value>)
        if (g_mission && params[1] > 0) {
            mission()->cam_frame = mission()->get_frame();
            mission()->cam_target_frame = mission()->cam_frame;
            MissionX* m = mission();
            m->cam_aperture = m->blend_param();
            mission()->cam_target_aperture = (float)clamp_index((int)params[0], 0x25, 0x96);
            mission()->cam_anim_time = (float)params[1] * 0.001f;
            mission()->cam_timer.restart();
            mission()->cam_easing = (int)params[2];
        }
        break;
    case op::mission_cam_animate_to_position_and_aparture:
        // (.., <position_register_no>, <value>, <duration-in-1/1000-seconds>, <value>)
        if (g_mission && check_register((int)params[0]) && params[2] > 0) {
            mission()->cam_frame = mission()->get_frame();
            mission()->cam_target_frame = script_pos((int)params[0]);
            MissionX* m = mission();
            m->cam_aperture = m->blend_param();
            mission()->cam_target_aperture = (float)clamp_index((int)params[1], 0x25, 0x96);
            mission()->cam_anim_time = (float)params[2] * 0.001f;
            mission()->cam_timer.restart();
            mission()->cam_easing = (int)params[3];
        }
        break;
    case op::mission_cam_set_target_agent:
        // (mission_cam_set_target_agent, <agent_id>, <value>) #value 0: do not use the agent's rotation
        if (g_mission && warn_agent((int)params[0])) {
            mission()->cam_target_agent = (int)params[0];
            mission()->cam_use_agent_rotation = (int)params[1];
        }
        break;
    case op::mission_cam_clear_target_agent:
        if (g_mission)
            mission()->cam_target_agent = -1;
        break;


    // ---- talk info, menus, screens, trigger parameters ----------------------------------------------------------
    case op::talk_info_show:
        // (talk_info_show, <hide_or_show>) :0=hide 1=show
        game()->talk_info_shown = (int)params[0];
        break;
    case op::talk_info_set_relation_bar:
        // a value outside -100..100 hides the bar
        game()->talk_relation_bar = (int)params[0];
        break;
    case op::talk_info_set_line:
        // (talk_info_set_line, <line_no>, <string_no>)
        if (IN_RANGE(params[0], 0, 5) && check_string_arg((int)params[1], types[1])) {
            game()->set_names((int)params[0], g_script.expand(g_strings.get_string((int)params[1], types[1])));
            game()->talk_line_set[(int)params[0]] = 1;
        }
        break;
    case op::set_background_mesh:
        // only while a game menu is up
        if (check_mesh((int)params[0]) && check_menu(game()->current_menu))
            game()->background_mesh = ((MeshRec*)g_meshes.records)[(int)params[0]].name;
        break;
    case op::set_game_menu_tableau_mesh:
        // (set_game_menu_tableau_mesh, <tableau_material_id>, <value>, <position_register_no>)
        if (warn_tableau((int)params[0]) && check_menu(game()->current_menu) && check_register((int)params[2])) {
            game()->menu_tableau = (int)params[0];
            game()->menu_tableau_value = (int)params[1];
            game()->menu_tableau_pos = script_pos((int)params[2]).o;
        }
        break;
    case op::change_screen_return:
        game()->next_window = 1;
        break;
    case op::change_screen_loot:
        if (check_troop((int)params[0])) {
            game()->screen_troop = (int)params[0];
            game()->next_window = 2;
        }
        break;
    case op::change_screen_trade:
        // (change_screen_trade, <troop_id>)
        if (check_troop((int)params[0])) {
            game()->screen_troop = (int)params[0];
            game()->next_window = 4;
        }
        break;
    case op::change_screen_exchange_members:
        // (change_screen_exchange_members, [0,1 = exchange_leader])
        game()->screen_party = game()->encountered_party;
        if (params[0])
            game()->next_window = 6;
        else
            game()->next_window = 5;
        break;
    case op::change_screen_give_members:
        game()->screen_party = game()->encountered_party;
        game()->next_window = 7;
        break;
    case op::change_screen_trade_prisoners:
        game()->next_window = 9;
        break;
    case op::change_screen_buy_mercenaries:
        game()->next_window = 10;
        break;
    case op::change_screen_view_character:
        game()->next_window = 0xb;
        break;
    case op::change_screen_training:
        game()->next_window = 0xc;
        break;
    case op::change_screen_mission:
        game()->next_window = 0xd;
        break;
    case op::change_screen_map_conversation:
        // (change_screen_map_conversation, <troop_id>)
        if (check_troop((int)params[0])) {
            game()->screen_troop = (int)params[0];
            game()->next_window = 0xe;
        }
        break;
    case op::change_screen_exchange_with_party:
        if (check_party((int)params[0])) {
            game()->screen_party = (int)params[0];
            game()->next_window = 8;
        }
        break;
    case op::change_screen_equip_other:
        if (check_troop((int)params[0])) {
            game()->screen_troop = (int)params[0];
            game()->next_window = 3;
        }
        break;
    case op::change_screen_map:
        game()->next_window = 0x11;
        break;
    case op::change_screen_notes:
        // (change_screen_notes, <note_type>, <object_id>), 1 = troops, 2 = factions, 3 = parties, 4 = quests
        if (params[0] >= 1 && params[0] <= 4) {
            char ok;
            switch ((int)params[0]) {
            case 1: ok = check_troop((int)params[1]); break;
            case 2: ok = check_faction((int)params[1]); break;
            case 3: ok = check_party((int)params[1]); break;
            case 4: ok = check_quest((int)params[1]); break;
            }
            if (ok) {
                game()->next_window = 0x10;
                game()->notes_type = (int)params[0];
                game()->notes_object = (int)params[1];
            }
        }
        break;
    case op::change_screen_quit:
        // destroys the game object
        g_windows.clear_all();
        if (g_game) {
            // `delete game()` spelled out: the original calls ~GameX (0x492f40) and operator delete in place, where cl
            // (inline budget spent) would call the scalar deleting destructor out of line
            GameData* g = g_game;
            g->world_free();
            operator delete(g);
            g_game = 0;
        }
        g_windows.set_state(4);
        break;
    case op::jump_to_menu:
        if (check_menu((int)params[0])) {
            game()->current_menu = (int)params[0];
            g_mission_ctx.passage_menu = (int)params[0];
            g_map_flags.f8 = (int)params[0];
            game()->next_window = 0x12;
        }
        break;
    case op::disable_menu_option:
        game()->menu_option_disabled = 1;
        break;
    case op::store_trigger_param_1:
        set_lvalue(locals, g_script.param1);
        break;
    case op::store_trigger_param_2:
        set_lvalue(locals, g_script.param2);
        break;
    case op::set_trigger_result:
        g_script.trigger_result.value = params[0];
        break;

    // ---- arithmetic (64-bit; fixed point by g_fixed_point_multiplier) ------------------------------------------
    case op::val_add:
        SET_LV(params[0] + params[1]);
        break;
    case op::val_sub:
        SET_LV(params[0] - params[1]);
        break;
    case op::val_mul:
        SET_LV(params[1] * params[0]);
        break;
    case op::val_div:
        // division by zero stores 40000000
        if (params[1] == 0)
            SET_LV(40000000);
        else
            SET_LV(params[0] / params[1]);
        break;
    case op::val_mod:
        if (params[1] != 0)
            SET_LV(params[0] % params[1]);
        break;
    case op::val_min:
    { int si_0; int si_1;
        // on the low 32 bits
        {
            si_0 = (int)params[0], si_1 = (int)params[1];
            SET_LV((__int64)min_int(si_0, si_1));
        }
        break;
    }
    case op::val_max:
    { int si_0; int si_1;
        {
            si_0 = (int)params[0], si_1 = (int)params[1];
            SET_LV((__int64)imax_ref(&si_0, &si_1));
        }
        break;
    }
    case op::val_clamp:
        // dest = max(min(dest, <upper_bound> - 1), <lower_bound>)
        SET_LV((__int64)clamp_index((int)params[0], (int)params[1], (int)params[2]));
        break;
    case op::val_abs:
        {
            __int64 v = params[0];
            if (v < 0)
                v = -v;
            SET_LV(v);
        }
        break;
    case op::val_or:
        SET_LV(params[1] | params[0]);
        break;
    case op::val_and:
        SET_LV(params[1] & params[0]);
        break;
    case op::store_or:
        SET_LV(params[2] | params[1]);
        break;
    case op::store_and:
        SET_LV(params[2] & params[1]);
        break;
    case op::store_mod:
        if (params[2] > 0)
            SET_LV(params[1] % params[2]);
        break;
    case op::store_add:
        SET_LV(params[2] + params[1]);
        break;
    case op::store_sub:
        SET_LV(params[1] - params[2]);
        break;
    case op::store_mul:
        SET_LV(params[2] * params[1]);
        break;
    case op::store_div:
        if (params[2] == 0)
            SET_LV(40000000);
        else
            SET_LV(params[1] / params[2]);
        break;
    case op::set_fixed_point_multiplier:
        if (params[0] > 0)
            g_script.fixed_point_multiplier = (int)params[0];
        break;
    case op::store_sqrt:
        // (store_sqrt, <destination_fixed_point>, <value_fixed_point>)
        if (params[1] > 0) {
            float m = (float)g_script.fixed_point_multiplier;
            SET_LV(round_i64((float)(sqrt((float)params[1] / m) * m)));
        }
        break;
    case op::store_pow:
        // pow(double, double): the original calls the CRT's _CIpow on the x87 stack (the float overload would call a
        // float pow). The multiplier is converted at each use: cl stores it once and divides by it, where a float
        // local divisor is rewritten as one reciprocal and multiplies (different rounding)
        SET_LV(round_i64((float)(pow((double)((float)params[1] / (float)g_script.fixed_point_multiplier),
                                     (double)((float)params[2] / (float)g_script.fixed_point_multiplier)) *
                                 (float)g_script.fixed_point_multiplier)));
        break;
    case op::store_sin:
        // degrees in fixed point. The parentheses are the original's operation order (x * k, then / m): without
        // them cl 13.10 reassociates `x * k / m` into `x / m * k`, which can round differently (also cos/tan below)
        {
            float m = (float)g_script.fixed_point_multiplier;
            SET_LV(round_i64((float)(sin(((float)params[1] * 0.017453292f) / m) * m)));
        }
        break;
    case op::store_cos:
        {
            float m = (float)g_script.fixed_point_multiplier;
            SET_LV(round_i64((float)(cos(((float)params[1] * 0.017453292f) / m) * m)));
        }
        break;
    case op::store_tan:
        {
            float m = (float)g_script.fixed_point_multiplier;
            SET_LV(round_i64((float)(tan(((float)params[1] * 0.017453292f) / m) * m)));
        }
        break;
    case op::convert_to_fixed_point:
        SET_LV((__int64)g_script.fixed_point_multiplier * params[0]);
        break;
    case op::convert_from_fixed_point:
        SET_LV(params[0] / (__int64)g_script.fixed_point_multiplier);
        break;
    case op::assign:
        set_lvalue(locals, params[1]);
        break;
    case op::shuffle_range:
        // (shuffle_range, <reg_no>, <reg_no>): registers [first, last); the swapped-in value keeps only 32 bits
        if (check_register((int)params[0]) && check_register((int)params[1])) {
            int first = (int)params[0];
            for (int k = first; (__int64)k < params[1]; ++k) {
                int n = (int)params[1] - first;
                int j = rand_below(&n) + (int)params[0];
                int v = (int)g_script.reg[k];
                g_script.reg[k] = g_script.reg[j];
                g_script.reg[j] = v;
            }
        }
        break;
    case op::store_random:
        // deprecated, use store_random_in_range instead
        {
            int n = (int)params[1];
            SET_LV((__int64)rand_below(&n));
        }
        break;
    case op::store_random_in_range:
        // [range_low, range_high) exclusive
        {
            int n = (int)params[2] - (int)params[1];
            SET_LV(rand_below(&n) + params[1]);
        }
        break;


    // ---- store_*: troops, parties, quests, time, map ------------------------------------------------------------
    case op::store_troop_gold:
        if (check_troop((int)params[1]))
            SET_LV((__int64)troop((int)params[1])->gold);
        break;
    case op::store_num_free_stacks:
        // (store_num_free_stacks, <destination>, <party_id>): the used stacks are counted on the main party
        {
            int used = party(game()->main_party)->first_flagged();
            SET_LV((__int64)(party((int)params[1])->max_stacks() - used));
        }
        break;
    case op::store_num_free_prisoner_stacks:
        {
            int used = party(game()->main_party)->count_tail();
            SET_LV((__int64)(party((int)params[1])->max_stacks() - used));
        }
        break;
    case op::store_party_size:
        // (store_party_size, <destination>, [party_id])
        SET_LV((__int64)party((int)params[1])->sum_all());
        break;
    case op::store_party_size_wo_prisoners:
        SET_LV((__int64)party((int)params[1])->sum_head());
        break;
    case op::store_troop_kind_count:
        // deprecated: matches the main party's stacks against <troop_id> but adds the sizes of the stacks of the
        // party whose id is the troop id (original bug)
        {
            int count = 0, k = 0;
            PartyMgr* pm = &game()->parties;
            int mp = game()->main_party;
            if (((PartyV*)pm->get(mp))->num_stacks > 0) {
                do {
                    if ((__int64)((PartyV*)pm->get(mp))->stacks[k].troop == params[1])
                        count += ((PartyV*)pm->get((int)params[1]))->stacks[k].count;
                    ++k;
                } while (k < ((PartyV*)pm->get(mp))->num_stacks);
            }
            SET_LV((__int64)count);
        }
        break;
    case op::store_num_regular_prisoners:
        // the main party's non-hero prisoner stacks, sizes read from party <party_id> (original bug)
        {
            int count = 0, k = 0;
            PartyMgr* pm = &game()->parties;
            int mp = game()->main_party;
            if (((PartyV*)pm->get(mp))->num_stacks > 0) {
                do {
                    if ((((PartyV*)pm->get(mp))->stacks[k].flags & 1) &&
                        !troop(((PartyV*)pm->get(mp))->stacks[k].troop)->is_hero())
                        count += ((PartyV*)pm->get((int)params[1]))->stacks[k].count;
                    ++k;
                } while (k < ((PartyV*)pm->get(mp))->num_stacks);
            }
            SET_LV((__int64)count);
        }
        break;
    case op::store_troop_count_companions:
        // (store_troop_count_companions, <destination>, <troop_id>, [party_id])
        SET_LV((__int64)((GameData*)g_game)->count_troops((int)params[1], (int)params[2], 0));
        break;
    case op::store_troop_count_prisoners:
        SET_LV((__int64)((GameData*)g_game)->count_troops((int)params[1], (int)params[2], 1));
        break;
    case op::store_item_kind_count:
        // (store_item_kind_count, <destination>, <item_id>, [troop_id])
        SET_LV((__int64)((GameData*)g_game)->count_in((int)params[1], (int)params[2], 1));
        break;
    case op::store_free_inventory_capacity:
        SET_LV((__int64)((GameData*)g_game)->count_in(-1, (int)params[1], 0));
        break;
    case op::store_skill_level:
        // (store_skill_level, <destination>, <skill_id>, [troop_id])
        if (check_troop((int)params[2]))
            SET_LV((__int64)((GameData*)g_game)->skill_level((int)params[2], (int)params[1]));
        break;
    case op::store_character_level:
        if (check_troop((int)params[1]))
            SET_LV((__int64)troop((int)params[1])->level);
        break;
    case op::store_attribute_level:
        if (check_troop((int)params[1]) && IN_RANGE(params[2], 0, 4))
            SET_LV((__int64)troop((int)params[1])->attributes[(int)params[2]]);
        break;
    case op::store_proficiency_level:
        if (check_troop((int)params[1]) && IN_RANGE(params[2], 0, 7))
            SET_LV((__int64)troop((int)params[1])->stat_int((int)params[2]));
        break;
    case op::store_troop_faction:
        if (check_troop((int)params[1]))
            SET_LV((__int64)troop((int)params[1])->faction);
        break;
    case op::store_troop_health:
        // (store_troop_health, <destination>, <troop_id>, [absolute]): else percent of the maximum
        if (check_troop((int)params[1])) {
            if (params[2])
                SET_LV((__int64)(int)troop((int)params[1])->hit_points);
            else {
                TroopX* t = troop((int)params[1]);
                float h = t->hit_points * 100.0f;
                SET_LV((__int64)(h / t->max_hit_points()));
            }
        }
        break;
    case op::store_troop_op_2180:
        // opcode 2180 (no header_operations name in 1.011): over the main party's companion stacks, heroes count
        // ceil(15% of their missing hit points), regular stacks their wounded
        {
            int total = 0, k = 0;
            if (party(game()->main_party)->num_stacks > 0) {
                do {
                    PartyMgr* pm = &game()->parties;
                    int mp = game()->main_party;
                    if (!(((PartyV*)pm->get(mp))->stacks[k].flags & 1)) {
                        TroopX* troops = game()->troops;
                        if (troops[((PartyV*)pm->get(mp))->stacks[k].troop].is_hero()) {
                            float mhp = (float)troops[((PartyV*)pm->get(mp))->stacks[k].troop].max_hit_points();
                            total += (int)ceil_float_comdat(
                                (mhp - troops[((PartyV*)pm->get(mp))->stacks[k].troop].hit_points) * 0.15f);
                        } else
                            total += ((PartyV*)pm->get(mp))->stacks[k].wounded;
                    }
                    ++k;
                } while (k < party(game()->main_party)->num_stacks);
            }
            SET_LV((__int64)total);
        }
        break;

    case op::store_relation:
        // (store_relation, <destination>, <faction_id_1>, <faction_id_2>): -100..100
        if (check_faction((int)params[1]) && check_faction((int)params[2])) {
            SET_LV((__int64)round_float(faction((int)params[1])->relations[(int)params[2]] * 100.0f));
        }
        break;
    case op::store_conversation_agent:
        // the dialog's agent for non-heroes; else the last agent of the conversation troop
        {
            int v = -1;
            if (!troop(g_dialogs.fc)->is_hero() && g_dialogs.partner_agent != -1) {
                v = g_dialogs.partner_agent;
                SET_LV((__int64)v);
            } else {
                AgentTable* t = &mission()->agents;
                unsigned id = t->first_used();
                while (id < (unsigned)t->count) {
                    if (t->get(id)->troop == g_dialogs.fc)
                        v = id;
                    t->next_used(&id);
                }
                SET_LV((__int64)v);
            }
        }
        break;
    case op::store_conversation_troop:
        SET_LV((__int64)g_dialogs.fc);
        break;
    case op::store_partner_faction:
        SET_LV((__int64)troop(g_dialogs.fc)->faction);
        break;
    case op::store_encountered_party:
        SET_LV((__int64)game()->encountered_party);
        break;
    case op::store_encountered_party2:
        SET_LV((__int64)game()->encountered_party2);
        break;
    case op::store_faction_of_party:
        // (store_faction_of_party, <destination>, <party_id>)
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->faction);
        break;
    case op::store_current_scene:
        SET_LV((__int64)mission()->current_scene);
        break;
    case op::store_item_value:
        if (check_item_kind((int)params[1]))
            SET_LV((__int64)item_kind((int)params[1])->value);
        break;
    case op::store_troop_value:
        if (check_troop((int)params[1]))
            SET_LV((__int64)((GameData*)g_game)->sl_party_price((int)params[1]));
        break;
    case op::store_partner_quest:
        // the last active quest given by the conversation troop, -1 if none
        {
            int v = -1;
            int n = game()->num_quests;
            for (int k = 0; k < n; ++k) {
                QuestRec* q = &game()->quests[k];
                if ((q->flags & 0x100) && (q->flags2 & 2) && q->giver_troop == g_dialogs.fc)
                    v = k;
            }
            SET_LV((__int64)v);
        }
        break;
    case op::store_random_quest_in_range:
        SET_LV((__int64)((GameData*)g_game)->sl_roll_party((int)params[1], (int)params[2]));
        break;
    case op::store_random_troop_to_raise:
        SET_LV((__int64)((GameData*)g_game)->sl_pick_chain((int)params[1], 0));
        break;
    case op::store_random_troop_to_capture:
        SET_LV((__int64)((GameData*)g_game)->sl_pick_chain((int)params[1], 1));
        break;
    case op::store_random_party_in_range:
        SET_LV((__int64)((GameData*)g_game)->sl_roll_other((int)params[1], (int)params[2]));
        break;
    case op::store01_random_parties_in_range:
        // two different random parties to reg0 and reg1 (at most 100000 tries for the second)
        {
            int a = ((GameData*)g_game)->sl_roll_other((int)params[0], (int)params[1]);
            int b = a;
            int n = 0;
            while (n < 100000) {
                ++n;
                b = ((GameData*)g_game)->sl_roll_other((int)params[0], (int)params[1]);
                if (b != a)
                    break;
            }
            g_script.reg[0] = a;
            g_script.reg[1] = b;
        }
        break;
    case op::store_random_horse:
        SET_LV((__int64)((GameData*)g_game)->pick_random_item(1, -1, -1, -1));
        break;
    case op::store_random_equipment:
        SET_LV((__int64)((GameData*)g_game)->pick_random_item(2, 3, 4, -1));
        break;
    case op::store_random_armor:
        SET_LV((__int64)((GameData*)g_game)->pick_random_item(0xc, 0xd, 0xe, -1));
        break;
    case op::store_quest_number:
        // no quest id check
        SET_LV((__int64)quest((int)params[1])->number);
        break;
    case op::store_quest_item:
        SET_LV((__int64)quest((int)params[1])->item);
        break;
    case op::store_quest_troop:
        if (check_quest((int)params[1]))
            SET_LV((__int64)quest((int)params[1])->troop);
        break;
    case op::store_current_hours:
        SET_LV((__int64)game()->current_hours);
        break;
    case op::store_time_of_day:
        SET_LV((__int64)((int)game()->clock.elapsed() % 24));
        break;
    case op::store_current_day:
        SET_LV((__int64)game()->note_time);
        break;
    case op::is_currently_night:
        result = game()->is_night();
        break;

    case op::store_distance_to_party_from_party:
        // (store_distance_to_party_from_party, <destination>, <party_id>, <party_id>): map distance, rounded
        if (check_party((int)params[1]) && check_party((int)params[2])) {
            PartyMgr* pm = &game()->parties;
            const rgl::vector4& b = party_pos((PartyV*)pm->get((int)params[2]));
            SET_LV((__int64)round_float(rgl::length(party_pos((PartyV*)pm->get((int)params[1])) - b)));
        }
        break;
    case op::get_party_ai_behavior:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->f8e4);
        break;
    case op::get_party_ai_object:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->ai_target_object);
        break;
    case op::party_get_ai_target_position:
        if (check_register((int)params[0]) && check_party((int)params[1])) {
            rgl::matrix* dst = &script_pos((int)params[0]);
            ASSIGN_MAT(*dst, IDENTITY_VALUE());
            PartyV* p = party((int)params[1]);
            dst->o.x = p->ai_target_x;
            dst->o.y = p->ai_target_y;
        }
        break;
    case op::get_party_ai_current_behavior:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->ai_behavior);
        break;
    case op::get_party_ai_current_object:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->ai_object);
        break;
    case op::store_num_parties_created:
        {
            int n = 0;
            if (check_party_template((int)params[1]))
                n = g_party_templates[(int)params[1]].num_created;
            SET_LV((__int64)n);
        }
        break;
    case op::store_num_parties_destroyed:
        {
            int n = 0;
            if (check_party_template((int)params[1]))
                n = g_party_templates[(int)params[1]].num_destroyed;
            SET_LV((__int64)n);
        }
        break;
    case op::store_num_parties_destroyed_by_player:
        {
            int n = 0;
            if (check_party_template((int)params[1]))
                n = g_party_templates[(int)params[1]].num_destroyed_by_player;
            SET_LV((__int64)n);
        }
        break;
    case op::store_num_parties_of_template:
        {
            int n = 0;
            PartyMgr* pm = &game()->parties;
            int id = pm->first_party_id();
            unsigned end = game()->parties.deflt;
            while ((unsigned)id < end) {
                if ((__int64)((PartyV*)pm->get(id))->template_id == params[1])
                    ++n;
                pm->next_party_id(&id);
            }
            SET_LV((__int64)n);
        }
        break;
    case op::store_random_party_of_template:
        // fails if no party exists with the template (two passes over the party table)
        {
            int n = 0;
            PartyMgr* pm = &game()->parties;
            int id = pm->first_party_id();
            unsigned end = game()->parties.deflt;
            while ((unsigned)id < end) {
                if ((__int64)((PartyV*)pm->get(id))->template_id == params[1])
                    ++n;
                pm->next_party_id(&id);
            }
            int found = -1;
            if (n > 0) {
                int pick = rand_below(&n);
                pm = &game()->parties;
                n = 0;
                id = pm->first_party_id();
                if (pick >= 0) {
                    end = game()->parties.deflt;
                    do {
                        if ((unsigned)id >= end)
                            break;
                        if ((__int64)((PartyV*)pm->get(id))->template_id == params[1]) {
                            if (n == pick)
                                found = id;
                            ++n;
                        }
                        pm->next_party_id(&id);
                    } while (n <= pick);
                }
            }
            result = found >= 0;
            SET_LV((__int64)found);
        }
        break;

    // ---- string registers ----------------------------------------------------------------------------------------
    case op::str_clear:
        if (check_register((int)params[0]))
            g_script.str[(int)params[0]].clear();
        break;
    case op::str_store_string:
        // (str_store_string, <string_register>, <string_id>)
        if (check_register((int)params[0]) && check_string_arg((int)params[1], types[1]))
            g_script.str[(int)params[0]] = g_script.expand(g_strings.get_string((int)params[1], types[1]));
        break;
    case op::str_store_string_reg:
        // copies one string register to another (expanded)
        if (check_register((int)params[0]) && check_register((int)params[1]))
            g_script.str[(int)params[0]] = g_script.expand(g_script.str[(int)params[1]]);
        break;
    case op::str_store_troop_name:
        if (check_register((int)params[0]) && check_troop((int)params[1]))
            g_script.str[(int)params[0]] = *troop((int)params[1])->display_name();
        break;
    case op::str_store_troop_name_plural:
        if (check_register((int)params[0]) && check_troop((int)params[1]))
            g_script.str[(int)params[0]] = *troop((int)params[1])->plural_name();
        break;
    case op::str_store_troop_name_by_count:
        // (str_store_troop_name_by_count, <string_register>, <troop_id>, <number>): plural from 2 on
        if (check_register((int)params[0]) && check_troop((int)params[1])) {
            if (params[2] >= 2)
                g_script.str[(int)params[0]] = *troop((int)params[1])->plural_name();
            else
                g_script.str[(int)params[0]] = *troop((int)params[1])->display_name();
        }
        break;
    case op::str_store_item_name:
        if (check_register((int)params[0]) && check_item_kind((int)params[1]))
            g_script.str[(int)params[0]] = *item_kind((int)params[1])->display_name();
        break;
    case op::str_store_item_name_plural:
        if (check_register((int)params[0]) && check_item_kind((int)params[1]))
            g_script.str[(int)params[0]] = *item_kind((int)params[1])->plural_name();
        break;
    case op::str_store_item_name_by_count:
        if (check_register((int)params[0]) && check_item_kind((int)params[1])) {
            if (params[2] >= 2)
                g_script.str[(int)params[0]] = *item_kind((int)params[1])->plural_name();
            else
                g_script.str[(int)params[0]] = *item_kind((int)params[1])->display_name();
        }
        break;
    case op::str_store_party_name:
        if (check_register((int)params[0]) && check_party((int)params[1]))
            g_script.str[(int)params[0]] = *party((int)params[1])->display_name();
        break;
    case op::str_store_agent_name:
        // the troop name, else the (horse) item name
        if (check_register((int)params[0]) && warn_agent((int)params[1])) {
            AgentTable* t = &mission()->agents;
            int a = (int)params[1];
            if (t->get(a)->troop >= 0)
                g_script.str[(int)params[0]] = *troop(t->get(a)->troop)->display_name();
            else if (t->get(a)->f8c.item >= 0)
                g_script.str[(int)params[0]] = *item_kind(t->get(a)->f8c.item)->display_name();
        }
        break;
    case op::str_store_faction_name:
        if (check_register((int)params[0]) && check_faction((int)params[1]))
            g_script.str[(int)params[0]] = *faction((int)params[1])->display_name();
        break;
    case op::str_store_quest_name:
        if (check_register((int)params[0]) && check_quest((int)params[1]))
            g_script.str[(int)params[0]] = g_script.expand(*quest((int)params[1])->raw_text());
        break;
    case op::str_store_date:
        // (str_store_date, <string_register>, <number_of_days_to_add_to_the_current_date>)
        if (check_register((int)params[0]) && params[1] >= 0)
            g_script.str[(int)params[0]] = ((GameData*)g_game)->event_string((int)params[1]);
        break;
    case op::str_store_troop_name_link:
        if (check_register((int)params[0]) && check_troop((int)params[1])) {
            char link[0x400];
            sprintf(link, "<trp=%d>%s</>", (int)params[1], CS(troop((int)params[1])->display_name()));
            g_script.str[(int)params[0]].assign(link);
        }
        break;
    case op::str_store_party_name_link:
        if (check_register((int)params[0]) && check_party((int)params[1])) {
            char link[0x400];
            sprintf(link, "<p=%d>%s</>", (int)params[1], CS(party((int)params[1])->display_name()));
            g_script.str[(int)params[0]].assign(link);
        }
        break;
    case op::str_store_faction_name_link:
        if (check_register((int)params[0]) && check_faction((int)params[1])) {
            char link[0x400];
            sprintf(link, "<fac=%d>%s</>", (int)params[1], CS(faction((int)params[1])->display_name()));
            g_script.str[(int)params[0]].assign(link);
        }
        break;
    case op::str_store_quest_name_link:
        if (check_register((int)params[0]) && check_quest((int)params[1])) {
            char link[0x400];
            int q = (int)params[1];
            sprintf(link, "<qst=%d>%s</>", q, CS(&quest(q)->text));
            g_script.str[(int)params[0]].assign(link);
        }
        break;

    // ---- mission counters ----------------------------------------------------------------------------------------
    case op::store_remaining_team_no:
        SET_LV((__int64)mission()->last_team());
        break;
    case op::store_mission_timer_a:
        {
            SET_LV(round_i64(mission()->timer_a.elapsed()));
        }
        break;
    case op::store_mission_timer_b:
        {
            SET_LV(round_i64(mission()->timer_b.elapsed()));
        }
        break;
    case op::store_mission_timer_c:
        {
            SET_LV(round_i64(mission()->timer_c.elapsed()));
        }
        break;
    case op::reset_mission_timer_a:
        mission()->timer_a.restart();
        break;
    case op::reset_mission_timer_b:
        mission()->timer_b.restart();
        break;
    case op::reset_mission_timer_c:
        mission()->timer_c.restart();
        break;
    case op::store_enemy_count:
        SET_LV((__int64)mission()->count_agents(1));
        break;
    case op::store_friend_count:
        SET_LV((__int64)mission()->count_agents(0));
        break;
    case op::store_ally_count:
        SET_LV(0);
        break;
    case op::store_attacker_count:
        {
            MissionX* m = mission();
            if (m->mode == 1)
                SET_LV((__int64)m->count_agents(0));
            else if (m->mode == 0)
                SET_LV((__int64)m->count_agents(1));
            else if (g_mission_ctx.side)
                SET_LV((__int64)m->count_agents(1));
            else
                SET_LV((__int64)m->count_agents(0));
        }
        break;
    case op::store_defender_count:
        // mode 1: team 1; mode 0: team 0; else team 0 when the player attacks, team 1 otherwise
        {
            MissionX* m = mission();
            if (m->mode == 1)
                SET_LV((__int64)m->count_agents(1));
            else if (m->mode == 0)
                SET_LV((__int64)m->count_agents(0));
            else if (g_mission_ctx.side)
                SET_LV((__int64)m->count_agents(0));
            else
                SET_LV((__int64)m->count_agents(1));
        }
        break;
    case op::store_normalized_team_count:
        // (store_normalized_team_count, <destination>, <team_no>)
        if (IN_RANGE(params[1], 0, 8))
            SET_LV((__int64)mission()->count_agents((int)params[1]));
        else
            SET_LV(0);
        break;

    default:
        sprintf(buf, "Unrecognized opcode %d.", id);
        report(buf);
        break;
    }

    char r;
    if (neg)
        r = !result;
    else
        r = result;
    if (r) {
        if (or_next)
            g_or_latch = 1;
    } else if (or_next)
        r = 1;
    *error = g_script_error;
    return r;
}
#undef game
#undef CS
#undef troop
#undef faction
#undef quest
#undef scene

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
