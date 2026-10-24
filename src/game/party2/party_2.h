// party_2.h: classes and externs of party_2.cpp (0x4b1160..0x4b8f40): the tableau render callback, the map party's
// economy (stacks, skills, morale, speed), its orders and path following on the world map, the battle records between two
// parties, the world-map path finder and the STL helper instances the linker kept from this object.
// Shared classes are the canonical ones: Party (include/party.h, with the map-party fields of this module added there),
// GameData (include/game_data.h; the fields it lacks are reached by offset through GD()), Troop / ItemRef / ItemKind, NavData
// / NavCell / CellGrid (terrain_b/cell_grid.h; NavRec below is an accessor view of the 0x88-byte NavCell), face (shape.h).
// Everything declared here exists nowhere else in src/. Every offset cites the function that proves it. Names provisional.
#ifndef GAME_PARTY2_PARTY_2_H
#define GAME_PARTY2_PARTY_2_H
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <vector>
#include "../terrain_b/terrain_b.h"
#include "../ui/mission_ui.h"
#include "../emit.h"
#include "../config/widget_mgr.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/pod_elems.h"
#include "../../include/ptr_tag.h"
#include "../../include/shape.h"
#include "../../include/dialog_rec.h"
#include "../../include/vec.h"

using rgl::vector4;

#define MB_NOINLINE __declspec(noinline)    // (tools/difftest/slice.py reads the name before a '__declspec(' as the function name)

// a field of the game data that the shared GameData does not declare yet: GD(type, offset)
#define GD(T, off) (*(T*)((char*)this + (off)))
#define GDP(T, off) (*(T*)((char*)g_game + (off)))

// ---- engine services and constants (other objects) ---------------------------------------------------------------------
__declspec(noreturn) void __stdcall throw_exception(void*, void*);     // 0x5368e6 (_CxxThrowException)
void mb_warning(const char* fmt, ...);                  // 0x42cb80 (logs a message)
void mb_error(const char* fmt, ...);                    // 0x42cc80 (printf-style log)
int __cdecl game_rand();                                // 0x536a9f (CRT rand)
float __cdecl frand();                                  // 0x40e400 (rand() % 15817 / 15817, in st0)
extern int g_level_exp[];                               // 0x5934bc experience needed per level
extern int g_perk_a;                                    // 0xa18120
extern int g_perk_b;                                    // 0xa18124
extern int g_perk_c;                                    // 0xa18128
extern float g_perk_d;                                  // 0xa18138
extern float g_perk_e;                                  // 0xa1813c
extern float g_skill_speed_k;                           // 0xa18114
extern __int64 g_script_result;                         // 0xa70458 (the perk / event result word of the script state)
extern int g_ev_a;                                      // 0xa70458 (low dword of the same word)
extern int g_ev_b;                                      // 0xa7045c (high dword)

// ---- 0x5c-byte records and their helpers (vector<E92>: the game's event / track records) ------------------------------
struct E92 { int v[23]; ~E92() {} };                    // 0x5c: event record (events vector at GameData +0x13a60; party/game.h TrackRec); the empty destructor keeps _Tidy's loop
// a 0xc-byte heap element (min-heap on dist): the world-map search's open list, vector<E12>
struct OpenItem { int node; float cost; float dist; };  // 0x4b24b0.., 0x4b6b40

struct ElemCopier {                                     // the allocator-like context of the copy helpers (thiscall copy / fill)
    void copy(OpenItem* dst, OpenItem* src);            // 0x436cf0 (agent_stats.cpp, ret 8)
    void copy_event(void* dst, void* src);              // 0x4b2160 (ret 8, ignores this)
    OpenItem* fill_elems(OpenItem* first, OpenItem* last, OpenItem* dst);          // 0x4b25a0 (_Uninit_copy)
    OpenItem* fill_n_elems(OpenItem* dst, unsigned count, OpenItem* val);          // 0x4b26e0 (_Uninit_fill_n)
    void* fill_n_events(void* dst, int count, const void* val);                   // 0x4b49d0 (E92 version)
    void move_events(E92* first, E92* last, E92* dst);                             // 0x4b38f0 (tail call into 0x4b2670)
};
// a party record used as a scratch block (constructed by 0x4934d0, destroyed by 0x431b90)
struct PartyTmp : Party {
    PartyTmp();                                         // 0x4934d0
    ~PartyTmp();                                        // 0x431b90
};
struct vec2r : vec2 {
    void rotate(float ang);                             // 0x432600 (replace by the unit vector at `ang`)
};
// an event record seen field by field (E92 is the same 0x5c bytes): written by GameData::push_event / init_event
struct EventRec {
    vector4 target;                                     // +0x00 map position   0x4b36a0
    float angle;                                        // +0x10 facing   0x4b36a0, 0x4b5fe0
    float f14;                                          // +0x14
    unsigned scale;                                     // +0x18 low byte = size class   0x4b36a0
    rgl::matrix pos;                                    // +0x1c frame built from the target and the face under it   0x4b36a0
};
struct HeapGt { bool operator()(const OpenItem& a, const OpenItem& b) const { return a.dist > b.dist; } };

// the vector iterator of an E92 array, returned through a hidden pointer (non-POD)
struct ItemIter {
    E92* p;
    ItemIter(E92* q) : p(q) {}
};
struct ItemArray {                                      // a container whose first member is the array base
    E92* base;
    ItemIter at(int idx);                               // 0x4b22b0
};

// ---- tableaus: the render callback of a tableau material and the object table that hands the materials out ------------------
struct TabTarget {                                      // the render target record of a tableau material (TabSpawn::target)
    char pad0[0x18];
    char enabled;                                       // +0x18 rendering enabled   0x4b1160
    char pad19[0x4c - 0x19];
    int done;                                           // +0x4c set after the scene was rendered   0x4b1160
};
struct TabSpawn {                                       // the material object 0x493df0 builds for a tableau record
    char pad0[0x48];
    TabTarget* target;                                  // +0x48   0x4b1160
    char pad4c[0x5c - 0x4c];
    int rec;                                            // +0x5c record index   0x4b14b0, 0x4b1160
    char pad60[4];
    int refs;                                           // +0x64 reference count   0x4b14f0
    void (*step)(int);                                  // +0x68 render callback (scene_step)   0x4b14b0
};
struct TabRec {                                         // a 0x68-byte tableau record (the script holder of its init script sits at +0x38)
    char pad0[0x20];
    int w, h;                                           // +0x20 render size   0x4b1160
    char pad28[0x38 - 0x28];
    char holder[0x30];                                  // +0x38 ScriptHolder   0x4b1160
    char pad68[0x68 - 0x68];
    TabSpawn* spawn(const int* key);                    // 0x493df0 (ret 4: find or create the material)
};
struct TabWorld;                                        // a tableau's private world (0x495650 appends one)
struct TabTable {                                       // the tableau object (global 0xa75f40)
    int f0;                                             // +0
    TabRec* recs;                                       // +4 (0x68-byte records)   0x4b14b0, 0x4b1160
    char pad8[0x10 - 8];
    std::vector<void*> worlds;                          // +0x10 (first at +0x14)
    TabWorld* push();                                   // 0x495650 (append a fresh world)
    void purge_all();                                   // 0x493f30
    void pop_back_delete();                             // 0x492ee0
    rgl::mesh* make_mesh_from(const int* index, const void* from);   // 0x4991d0 (ret 8)
    void add_spawned(const int* tableau, const int* instance);       // 0x4b14b0 (ret 8)
};
extern TabTable g_obj;                                  // 0xa75f40
extern int g_5a75f60;                                   // 0xa75f60
void scene_step(int material);                          // 0x4b1160 (the render callback stored in TabSpawn::step)

struct TabFrustum : rgl::frustum {                      // the frustum plus the tableau's light state (0x134 bytes)
    int flag;                                           // +0x120   0x4b1160
    float mat4[4];                                      // +0x124
};
struct TabWorld : CellGrid {                            // a tableau world: the scene container plus a frustum at +0x36c
    int f344, f348;                                     // +0x344   0x4b1160
    char pad34c[0x36c - 0x34c];
    TabFrustum fr;                                      // +0x36c
    void step(float dt);                                // 0x427f40
    void render_scene(rgl::scene* sc, bool reflect);    // 0x429890
};

// ---- map-party visuals ---------------------------------------------------------------------------------------------
// ---- the map party's model (an rgl entity of type 9) and the meshes it carries ---------------------------------------------
struct Rot3 {                                           // the three rows of a frame (side, forward, up)
    vector4 s, f, u;
    void rotate_sf(const float* angle);                 // 0x40e770 (ret 4: rows s and f about u)
};
Rot3* __cdecl identity3(Rot3* out);                     // 0x40cd60
struct MeshRes : rgl::mesh {                            // the master mesh the resource manager hands out for a name
    rgl::mesh* get();                                   // 0x4958f0
    rgl::mesh* clone();                                 // 0x41c6d0
};
// the party's map model (Party::model, an rgl entity of type 9): the slots, fields and members the update code uses
struct PartyModel {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void set_color(const float* shade);         // +0x18   0x4b7840
    virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11(); virtual void s12();
    virtual void s13(); virtual void s14(); virtual void s15(); virtual void s16(); virtual void s17();
    virtual void set_mode(int mode);                    // +0x48   0x4b7840
    virtual void s19(); virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23(); virtual void s24();
    virtual void add_emitter(Emit* e);                  // +0x64   0x4b7840
    char pad4[0x58 - 4];
    Rot3 basis;                                         // +0x58 orientation rows   0x4b80d0
    vector4 origin;                                     // +0x88 position (the sound source)   0x4b80d0
    char pad98[0xc0 - 0x98];
    float f_c0;                                         // +0xc0   0x4b7840
    vector4 color;                                      // +0xc4   0x4b7840
    char padd4[0x118 - 0xd4];
    ptr_vec ents;                                       // +0x118 std::vector<rgl::mesh*> (first at +0x11c)   0x4b7840, 0x4b80d0
    char pad128[0x140 - 0x128];
    char visible;                                       // +0x140   0x4b80d0
    char pad141[0x15c - 0x141];
    float f15c;                                         // +0x15c  0x4b1800
    float f160;                                         // +0x160
    float f164;                                         // +0x164
    char pad168[0x294 - 0x168];
    float f294;                                         // +0x294  0x4b7840, 0x4b80d0
    void add_mesh(rgl::mesh* m);                        // 0x442110 (ret 4)
    void set_fade(const float* p);                      // 0x4b1800 (ret 4)
};
// a std::vector of 0x5c-byte records (allocator word, first, last, end)
struct EventVec {
    int al;
    E92* first;
    E92* last;
    E92* end;
    int size() const;                                   // 0x4b1af0
};

// ---- the world map's formation records and the battle records ---------------------------------------------------------
// NavData::cells (terrain_b/cell_grid.h NavCell, 0x88 bytes) seen field by field (NavCell::ids is the whole +4..+0x53 block)
struct NavRec : NavCell {
    int* verts() { return &ids[0]; }                    // +0x04 vertex indices of the edges (4)   0x4b3420
    int* agents() { return &ids[4]; }                   // +0x14 neighbouring record per edge        0x4b3420
    unsigned& mask() { return *(unsigned*)&ids[8]; }    // +0x24 terrain mask (low 3 bits: kind)    0x4b2770, 0x4b2880, 0x4b3420
    int* targets() { return &ids[9]; }                  // +0x28 record reached through the edge     0x4b3420, 0x4b6b40
    float* dists() { return (float*)&ids[13]; }         // +0x38 keep distance per edge              0x4b3420, 0x4b6b40
    int& gen() { return *(int*)((char*)this + 0x6c); }  // +0x6c search stamp (compare with NavData::m64)   0x4b6b40
    int& parent() { return *(int*)((char*)this + 0x70); }   // +0x70 search parent                   0x4b6b40
    float* pos() { return (float*)((char*)this + 0x74); }   // +0x74 x, y, z                         0x4b6b40
    float& limit() { return *(float*)((char*)this + 0x80); }// +0x80 keep distance of the node       0x4b6b40
};
inline NavRec* nav_recs() { return (NavRec*)g_level->core->cells.first; }       // the record array (+0x14 of the nav data)

#include "../battle_rec.h"            // BattleObj, BattleRec, BattleTable: the one definition (party2 and script)
inline BattleTable* battles() { return (BattleTable*)((char*)g_game + 0xcc); }
inline Party* party_at(PartyMgr* m, int k) { return (Party*)((k & 0xf) * 0x5ac8 + (char*)*(m->pages.begin() + (k >> 4))); }
inline Party* party_of(PartyMgr* m, int id) { return party_at(m, m->index_of(id)); }

struct SoundMgr {                                       // the sound object (global 0x5aa128)
    int play(int sound, int priority, const float* pos, float pitch, float* out);   // 0x442650 (ret 0x14)
};
extern SoundMgr g_sound_mgr;                            // 0x5aa128
extern int g_sound_bank;                                // 0x5aa304 (the sound index, passed by value)
void __stdcall stop_sound(int id);                      // 0x5342a6 (FSOUND_StopSound@4)

struct resmgr_tab : resmgr_gui {                        // the resource manager (0x5d6fa0) as the tableau code uses it
    texture* make_texture(int w, int h, int fmt);       // 0x41c400
    void release_res(void* p);                          // 0x4175f0
};
#define g_resmgr_tab (*(resmgr_tab*)&g_resmgr)

// ---- callees of other objects, declared with the argument lists the call sites push (see MIGRATION.md) ----------------------
void adjust_heap(OpenItem* first, int hole, int len, OpenItem value, HeapGt comp);        // 0x438b10 (_Adjust_heap)
void push_back_ext(OpenItem* first, OpenItem* last, int pred);                            // 0x4b2710
void __fastcall vec_push_back(std::vector<E12>* v, int unused, const OpenItem* item);      // 0x4b5bb0 (vector<E12>::push_back, thiscall)

#endif // GAME_PARTY2_PARTY_2_H
