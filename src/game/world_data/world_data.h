// world_data.h: the party / quest / experience members of the game object (g_game, 0x5e2930), the mission-side helpers
// that sit between them (0x490030..0x4958f0: translation units 83 world_2 and 84 game_data_2 of the TU map), and the
// small engine objects those functions reach (message log, sound player, window manager, scene list, panel family).
// Names provisional; every offset cites the function that proves it.
//
// The shared classes are included, not redefined: GameData / PartyMgr / Party / Stack / Troop / QuestRec / tstring /
// MsgLog / StrTable / Vec2 / MissionUi / ContainerBase / DropDown come from src/include and src/game. What those headers
// lack is added here as *views*: a derived class with no data members (`GameW : GameData`, ...) whose
// inline accessors read the raw offsets this module's functions use (the technique of script/script_interpreter.h).
// Pure additions to canonical headers: none.
#ifndef GAME_WORLD_DATA_H
#define GAME_WORLD_DATA_H
#include "../../include/image_addrs.h"
#include <new>
#include <vector>
#include <deque>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../tstring.h"
#include "../game_globals.h"      // canonical g_module_settings.player_wounded_treshold
#include "../str_table.h"
#include "../../rgl/rgl_math.h"
#include "../../include/vec.h"
#include "../../include/game_data.h"
#include "../strat/gui_manager.h"
#include "../config/widget_mgr.h"
#include "../ui/mission.h"
#include "../../include/rec_dtors.h"
#include "../../include/fx.h"        // FxElem (add_group 0x4423f0)
#include "../mission.h"
#include "../placed_obj.h"

#include "./raw_place.h"            // raw_place: constructing a canonical class in raw storage
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/game_recs.h"
#include "../../include/level_global.h"      // g_level (0xa732b4)
#include "../../rgl/text_stream.h"

// raw field of a view class
#define WD_FLD(T, off) (*(T*)((char*)this + (off)))

// ---- the party manager embedded in the game object at +0xf8 (include/party.h PartyMgr) --------------------------------


// ---- virtual views ------------------------------------------------------------------------------------------------
struct EntVt {                                                  // vtable view of the container and of its children (vtable 0x5924a0 / MissionUi)
    virtual void v00(int f);                                    // +0x00 deleting destructor
    virtual void v01();
    virtual void v02(int p);                                    // +0x08 update (0x4908c0)
    virtual void v03();
    virtual void v04();
    virtual void v05();
    virtual void v06();
    virtual void v07();
    virtual void v08();
    virtual void v09();
    virtual void v10(int* a, float* b, float* c, float* d);     // +0x28 bounds (0x490710)
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();                                         // +0x40 reset (0x4906e0)
    virtual void v17();
    virtual void v18();
    virtual void v19();
    virtual void v20();
    virtual void v21();
    virtual void v22();
    virtual void v23(void* child);                              // +0x5c child moved (0x490570, 0x4906b0)
    virtual void v24();                                         // +0x60 (0x490570, 0x4905c0)
};

// the container widget is ui/container.h's ContainerBase (its members are defined below, in world_data.cpp)

inline int& troop_f54(Troop* t) { return t->upgrade_troop[1]; }   // +0x54 second upgrade troop (0x494e40)

// ---- the game object (include/game_data.h GameData) -----------------------------------------------------------------
// (the level object's face-sharing check is NavData::shares 0x48efa0, terrain_b/nav_data.h)
struct FaceId {
    int m0;
    int m4;                                                     // +4 face index (-1: none)
};

void* __fastcall trigger_entity(int context);                     // 0x490430 (the scene entity of a trigger context)

#include "../../include/game_recs.h"   // PartyTemplate* g_party_templates (0x5e29f8)
extern float g_k15;                                             // 0x590da0 (15.0f)

// GameData's operations of 0x490030..0x4957d0 are declared on GameData (include/game_data.h). add_random_experience
// (0x4952a0, a byte-exact row returning int) returns the callee's eax: its one call goes through GameData::add_experience
// viewed as returning int (a pointer-to-member of the same function, see there)

// ---- the mission object (game/mission.h Mission) and its agent records -----------------------------------------------
struct AgentW : Agent {
    int& f56d4() { return weapon_mode; }                // +0x56d4 weapon mode (0x492230)
};
struct MissionW : Mission {
    int& f6520() { return WD_FLD(int, 0x6520); }                // +0x6520 (0x491f50: the signed speed term)
    AgentW* rec(unsigned i) { return (AgentW*)((char*)agents.pages[i >> 4] + (i & 0xf) * 0x5928); }   // agent record i (page of 16, no validity check)
    int count_agents(int team);                                 // 0x491f50 (ret 4)
    bool all_defeated(int team);                                // 0x4920e0 (ret 4)
    bool hero_down();                                           // 0x492170
    unsigned find_agent_by_troop(int troop);                    // 0x4921b0 (ret 4)
    void add_wind_effect(float a1, float a2);                       // 0x4952f0 (ret 8)
    void set_weapon_order_1(int team, unsigned value);          // 0x492230 (ret 8)
    void set_weapon_order_2(int team, unsigned value);          // 0x492350 (ret 8)
};



// ---- vectors of 8-byte integers, 4-byte-integer vectors (VC7.1 layout: allocator word first) -----------------------------
#include "../../include/pod_elems.h"
struct FPair { float a, b; };

struct I64VecW : I64Vec {                                       // include/i64_vec.h; the slot-array / quest-record vector
    unsigned size() const { return _Myfirst == 0 ? 0 : (unsigned)(_Mylast - _Myfirst); }
    unsigned capacity() const { return (unsigned)(_Myend - _Myfirst); }
    unsigned max_size() const { return (unsigned)(-1) / sizeof(E8); }
    __declspec(noinline) void reserve(int* newsize);            // 0x493c40 (thiscall, ret 4; callers keep it a call)
};
struct I64VecOp {                                               // the same vector as an opaque callee (the callers reload size after reserve)
    int pad0;
    long long* _Myfirst;
    E8* _Mylast;
    E8* _Myend;
    void reserve(int* newsize);                                 // 0x493c40
};

struct IntVecW {                                                // std::vector<int> with the leading allocator dword (game/int_vec.h IntVec)
    int pad0;
    int* _Myfirst;
    int* _Mylast;
    int* _Myend;
    unsigned size() const { return _Myfirst == 0 ? 0 : (unsigned)(_Mylast - _Myfirst); }
    void insert_a(int* where, unsigned count, const int& val);     // 0x4927d0 (the instance below; also used for pointer vectors)
    void insert_n(int* where, unsigned count, const int& val);     // 0x43e7b0
    void insert_c(int* where, unsigned count, const int& val);     // 0x41a2c0
    void resize(unsigned n, int v);                                 // 0x43f310
    void kill_range(int n, int pad);                                // 0x41ad20
    void push_back(const int& v);                                   // 0x4936b0 (out of line)
};


// (the record arrays the game object's teardown deletes (0x492a20) are TroopFile / FactionFile / QuestFile: mission5/data_files.h,
// and the scene records SceneFile: terrain_b/scene_file.h)

typedef void (__stdcall* PanelFn)(void* self, unsigned char v);

// ---- the event pool (GameData +0xcc: Pool<Event>, include/pool.h; alloc 0x493700) and the cache group (E68 members) -----------
// Its record is game/event.h's Event (constructor 0x48c210, vtable 0x5922cc, an IfaceB of 0x98 bytes); the pages are `new Event[16]`,
// which is where the vector deleting destructor in the vtable is emitted. The party pages are `new Party[16]` (PartyMgr::grow).
#include "../mission_ui2/stat_panel_x.h"
#include "./thiscall_free.h"
#include "../../rgl/resource.h"                               // resmgr g_resmgr 0x5d6fa0
// the tableau record's materials: E68::find_or_create 0x493df0 / E68::purge 0x493e90 (include/e68.h) on rgl skin (key f60, refcount)
#include "../../include/view_params.h"                       // ViewParams (ctor 0x46b940)
struct SceneBaseCtor {                                          // the scene base object (0x36c bytes): its constructor body, kept opaque from 0x495650
    SceneBaseCtor* construct();                                 // 0x493ab0 (returns this)
};

// ---- the tableau table (g_tableaus 0xa75f40; the shared header tableau.h has the whole class) -------------------------------------
#include "./tableau.h"

#include "./widget_mgr_w.h"

// ---- the window manager (g_windows 0xa73980: 30 screen objects, the open-screen deque at +0x78) --------------------------------
#include "../screen_obj.h"                                    // ScreenObj: the vtable view of a screen (slot 0 = deleting destructor)
#include "../window_mgr.h"                                   // WindowMgrW g_windows (0xa73980)
struct GlobVec {                                                // the running presentation's list holder (global 0x5e299c)
    std::vector<void*> list;                                    // +0 (first at +4; set_state 0x495700 frees it: element type not known)
};

// ---- the sound player (g_sound_mgr 0x5aa128): include/sound_mgr.h ----------------
#include "../../include/sound_mgr.h"


#include "../config/config.h"                                  // g_cfg 0x5a9f98
#include "../../include/renderer.h"                              // g_render 0xa17910 (dev at +0x618 = 0xa17f28)
struct RotRows { rgl::vector4 s, f, u; };                       // the rotation part of a frame (0x30 bytes)
#include "../../include/mat4.h"                              // rgl::mat3
extern "C" void __stdcall FSOUND_StopSound(int handle);                            // 0x5342a6 _FSOUND_StopSound@4
#include "../../include/fmod_api.h"   // FMOD 3 (one declaration per entry point)

// ---- the carried-item placement of an agent body (0x495430): views of the item kind, mesh, skeleton ---------------------------------------
struct CarryKind {                                              // include/item_kind.h ItemKind (0x238 bytes, g_items 0x5e29d0)
    char pad0[0x1c8];
    unsigned char type;                                         // +0x1c8 (7 = shield)
    char pad1c9[0x1d0 - 0x1c9];
    unsigned f1d0;                                              // +0x1d0
    unsigned f1d4;                                              // +0x1d4
    char pad1d8[0x238 - 0x1d8];
};
struct CarryMesh {                                              // rgl::mesh seen by the placement: visibility mask and local frame
    char pad0[0x8c];
    rgl::matrix frame;                                          // +0x8c (m_cc)
    unsigned vis;                                               // +0xcc (mask)
};
struct CarryRef { int item; unsigned m4; };                     // an 8-byte carried item ref (include/item_ref.h ItemRef)
struct CarryCtx {                                               // the agent's three per-slot mesh arrays
    char pad0[0x10];
    CarryMesh* c1[4];                                           // +0x10
    CarryMesh* c2[4];                                           // +0x20
    CarryMesh* c3[4];                                           // +0x30
};
// The placement calls FxDef::remove_group / FxElem::add_group (include/fx.h) on the body's skeleton (rgl::strategic_entity::fx),
// ItemKind::get_carry_pos (include/item_kind.h) and Agent::agent_class (game/agent_obj.h).
#include "../../rgl/scene/scene_entity.h"                      // rgl::strategic_entity: the body that carries the items
void __cdecl place_scene_agents(rgl::strategic_entity* holder, CarryRef* table, CarryCtx* ctx);   // 0x495430

// ---- 0x4952f0: the effect object the mission's scene container hands out (the wind / light record, 0x7c bytes) ----------------------------

// ---- the particle-system table and the emitter control (0x4955e0) -----------------------------------------------------------------------------
struct Emitter;
struct EmitTarget {                                             // virtual object at mission +0x59c0
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23();
    virtual void s24();
    virtual void add(Emitter* e);                               // +0x64
};
#include "../emit.h"
struct EmitterCtl {
    char pad0[0x59c0];
    EmitTarget* target;                                         // +0x59c0
    char pad59c4[0x59f4 - 0x59c4];
    std::vector<int> list;                                      // +0x59f4
    void add(int e);                                            // 0x4955e0 (ret 4)
};

// ---- 0x495650: the scene list ------------------------------------------------------------------------------------------------------
#include "../../include/frustum.h"                              // rgl::frustum (0x120 bytes; the ctor 0x41ef60); the stand-in "Frustum" was 0x134 bytes with 0x14 unknown after it
struct SceneBase {
    char d[0x36c];
    inline SceneBase();                                         // 0x493ab0 (forwards to SceneBaseCtor::construct, the one definition)
};
inline SceneBase::SceneBase() { ((SceneBaseCtor*)this)->construct(); }
struct SceneObject : SceneBase { rgl::frustum frustum; char tail[0x14]; };   // 0x4a0 total


#endif // GAME_WORLD_DATA_H
