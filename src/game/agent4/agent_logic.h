// agent_logic.h: the classes of agent_logic.cpp (0x444c50..0x460160) that no shared header has: the troop-class voice table, the
// scene entity / missile / blow views the agent code calls, the Newton Game Dynamics 1.x imports and the few small records of the
// object. Everything else is a shared class (src/game/*.h, src/include/*.h, agent3/agent_types.h); the per-field additions made
// to those headers are listed in MIGRATION.md. Offsets are read by a function of the module (address cited). Names provisional.
#ifndef AGENT4_LOGIC_H
#define AGENT4_LOGIC_H

#include <vector>
#include "../tstring.h"
#include "../item_ref.h"
#include "../../rgl/rgl_math.h"   // first: rgl_base.h would otherwise pull rgl_math.h in before agent/item_kind.h includes rgl.h
#include "../../include/rgl_base.h"
#include "../agent2/agent_ai.h"                       // Table1f8 (0x43b100, 0x43b130)

// ---- canonical declarations -----------------------------------------------------------------------------------------------
// Members this module calls that other objects define, under the defining classes' names (the views below and the call sites in the .cpp
// reach them through these; each cites the evidence that the object is that class: same layout / the shared out-of-line code).
struct Poser;                                            // agent3/agent_fx_sound.h: scene entity and lod mesh share add_extra 0x442110 / add_to_list 0x441ff0
#include "../ui/timer_list.h"                           // TimerList: the sound-event list of PropKind / ItemKind (update 0x48add0)
struct FireRay;
struct FireAgent { void ray(FireRay* r); };            // 0x46b410 (terrain_a/terrain_a.cpp: Missile::ray is the same code)
struct Obj3;   // members in agent3/agent_types.h (set_v184 0x433740, reset_pending 0x433810)
#include "../../include/timer.h"                    // Timer::set_delay 0x42cd20 (strat/gui_manager.cpp)
// AnimRec::phase 0x4231f0: agent/curve.h
namespace rgl {
struct entity;

}
struct shape_base;
struct shape_base;

struct StratEnt;

struct lod_group;                                        // rgl/core/resmgr.h (0xe0 bytes, ctor 0x41c2a0)
bool is_night();                                         // 0x435740 (agent/agent.cpp)
struct PlacedObj;
void __stdcall init_placed(PlacedObj* o, const int* kind, const int* id, const int* value, const rgl::matrix* m);   // 0x435860 (agent/agent.cpp)
#include "../std_vec_out_of_line.h"
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
// 0x43e7b0: the out-of-line std::vector<int>::_Insert_n shared by the pointer vectors

// 0x1f8-byte voice records (table object at 0x8ed4f60: records at +0x1c, count at +0x28; 0x4459c0 reads snd[kind] at +0x1cc)
struct VoiceRec { char pad[0x1cc]; int snd[1]; char pad1d0[0x1f8 - 0x1d0]; };
struct VoiceTable {
    char pad0[0x18];
    std::vector<VoiceRec> recs;                         // +0x18 (first at +0x1c); element size 0x1f8
    unsigned count;                                     // +0x28
    __forceinline int get_f1f0(unsigned i) { return ((Table1f8*)this)->get_f1f0(i); }   // 0x43b100 (agent2/agent_ai.h)
    __forceinline int get_f1f4(unsigned i) { return ((Table1f8*)this)->get_f1f4(i); }   // 0x43b130
    int sound(unsigned cls, int kind)
    {
        if (cls < count)
            return recs[cls].snd[kind];
        return -1;
    }
};
#include "../animator.h"                            // Animator g_anim (0x8ed4f60): the voice table is the same object
#define g_anim_table (*(VoiceTable*)&g_anim)         // g_anim seen as the voice table (recs at +0x18, count at +0x28)

// ---- free functions of earlier objects the module calls -------------------------------------------------------------
float frand();                                                          // 0x40e400 (rand() % 15817) / 15817
float __cdecl accel_factor(const float* a, const float* b, const float* c);   // 0x435ae0
float __cdecl fmin_ref(const float* a, const float* b);                 // 0x4324b0
float __cdecl fmax_ref(const float* a, const float* b);                 // 0x4324d0
float __cdecl clamp(const float* x, const float* lo, const float* hi);  // 0x40e3d0
int __stdcall in_id_set(int id);                                        // 0x432640
extern unsigned g_input_mask;                                           // 0x5e2474
extern unsigned g_input_flags;                                          // 0x5e2470

#include "../scene_ent.h"                              // SceneEnt: the scene entity as the agent code calls it (shared with terrain_b/terrain_b.cpp: Missile::settle)
struct EntVec {                                         // std::vector<SceneEnt*> layout (VC7.1), freed by hand (0x448eb0); Mission +0x63c0 (0x44c210)
    int alloc_pad;
    SceneEnt** first;
    SceneEnt** last;
    SceneEnt** end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    unsigned capacity() const { return first == 0 ? 0 : end - first; }
    __forceinline void insert_n(SceneEnt** where, unsigned n, SceneEnt* const& v)       // 0x43e7b0 (ICF-folded with vector<int>::_Insert_n)
    { ((int_stdvec*)this)->insert_n((int*)where, n, *(const int*)&v); }
    void push_back(SceneEnt* const& x)                  // VC7.1: inline fast path, out-of-line _Insert_n
    {
        if (size() < capacity()) {
            *last = x;
            ++last;
        } else {
            insert_n(last, 1, x);
        }
    }
};

// std::vector<rgl::mesh*> whose assignment and push_back stay out of line (pointer vectors fold with vector<int>) (0x44c210)
struct MeshVec : std::vector<rgl::mesh*> {
    __forceinline MeshVec& operator=(const std::vector<rgl::mesh*>& o);     // 0x43f0a0 (std::vector<int>::operator=; defined in the .cpp)
    __forceinline void push_back(rgl::mesh* const& x);                      // 0x4119d0 (std::vector<rgl::scene*>::push_back; defined in the .cpp)
};

// the body resource of a prop (0x44c210) is rgl's body_part (clone_shape 0x4285e0, copy_shape 0x439100)
#include "../../rgl/resource.h"


// one 0x1dc-byte bone record of the animator (Skeleton::recs): the world frame at +0x84 (0x44e2b0)
struct BoneW {
    char pad0[0x84];
    rgl::matrix world;                                  // +0x84 (origin at +0xb4)
    char pad_c4[0x1dc - 0xc4];
};
rgl::vector4* __cdecl rotate_scale_xy(rgl::vector4* out, const rgl::vector4* in, const float* angle, const rgl::vector4* scale);   // 0x436210 (returns out: eax holds it to the ret)
void __cdecl rand_cone_dir(rgl::vector4* out, rgl::vector4 dir, float spread);   // 0x432c40


// ---- the flying missile record (0xb0 bytes, src/game/missile.h: shared with terrain_a/misc.h)
#include "../missile.h"


// ---- the second half of the object (formerly agent_update.h) ------------------------------------------------------------------------

#include "../../rgl/rgl_math.h"
#include "../../include/mat4.h"
#include "../../include/phys_set.h"
#include "../../include/shape.h"
#include "../tstring.h"
namespace rgl { struct mesh; }

extern "C" {
__declspec(dllimport) NewtonWorld* __cdecl NewtonCreate(void* alloc, void* free_);                              // 0x54921c
__declspec(dllimport) void __cdecl NewtonSetSolverModel(const NewtonWorld* w, int model);                       // 0x549220
__declspec(dllimport) void __cdecl NewtonSetWorldSize(const NewtonWorld* w, const float* mn, const float* mx);   // 0x549224
__declspec(dllimport) int __cdecl NewtonMaterialGetDefaultGroupID(const NewtonWorld* w);                         // 0x549228
__declspec(dllimport) void __cdecl NewtonMaterialSetDefaultSoftness(const NewtonWorld* w, int a, int b, float v);   // 0x54922c
__declspec(dllimport) void __cdecl NewtonMaterialSetDefaultElasticity(const NewtonWorld* w, int a, int b, float v); // 0x549230
__declspec(dllimport) void __cdecl NewtonMaterialSetDefaultFriction(const NewtonWorld* w, int a, int b, float s, float k);   // 0x549234
__declspec(dllimport) void __cdecl NewtonBodySetVelocity(const NewtonBody* b, const float* v);                  // 0x549214
__declspec(dllimport) void __cdecl NewtonBodySetOmega(const NewtonBody* b, const float* w);                     // 0x549218
__declspec(dllimport) void __cdecl NewtonBodySetMatrix(const NewtonBody* b, const float* m);                    // 0x549278
__declspec(dllimport) NewtonRagDoll* __cdecl NewtonCreateRagDoll(const NewtonWorld* w);                         // 0x549288
__declspec(dllimport) void __cdecl NewtonRagDollBegin(const NewtonRagDoll* rd);                                 // 0x54928c
__declspec(dllimport) void __cdecl NewtonRagDollEnd(const NewtonRagDoll* rd);                                   // 0x549284
__declspec(dllimport) void __cdecl NewtonRagDollSetForceAndTorqueCallback(const NewtonRagDoll* rd, void (__cdecl* cb)(const NewtonBody*));   // 0x549280
__declspec(dllimport) NewtonBody* __cdecl NewtonRagDollBoneGetBody(const NewtonRagDollBone* b);                 // 0x549294
}
void __cdecl apply_force_and_torque(const NewtonBody* body);                                                    // 0x432a50

struct V4 { float x, y, z, w; };               // constructor-free storage for the local arrays of start_ragdoll
struct M4 { V4 s, f, u, o; };



// ---- the blow record (0x58 bytes, initialiser 0x4353a0) ----
#include "../blow.h"                                // Blow, HitInfo: the one definition (agent4 and script)

// the per-agent text / label state (*(agent+0x190))
struct AgentFx {
    char pad0[0x38];
    rgl::mesh** pmesh;                              // +0x38
    char pad3c[0xc4 - 0x3c];
    float scale;                                    // +0xc4
    char padc8[4];
    int visible;                                    // +0xcc
};

// the agent body object seen through its vtable (slots 18 / 24) and the fields the update code writes (0x4543a0)
struct CamParams;
struct AgentBodyV {
    virtual void v0();  virtual void v1();  virtual void v2();  virtual void v3();
    virtual void v4();  virtual void v5();  virtual void v6();  virtual void v7();
    virtual void v8();  virtual void v9();  virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17();
    virtual void set_mode(int mode);                // slot 18 (+0x48)
    virtual void v19(); virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23();
    virtual void set_color(unsigned c);             // slot 24 (+0x60)
    char pad4[4];
    int f8;                                         // +8
    char padc[0x98 - 0xc];
    CamParams* camp;                                // +0x98
    char pad9c[0x294 - 0x9c];
    float f294;                                     // +0x294
    int f298;                                       // +0x298
};

extern "C" {
__declspec(dllimport) void __cdecl NewtonBodyGetMatrix(const NewtonBody* b, float* m);                           // 0x54920c
__declspec(dllimport) void __cdecl NewtonBodyGetVelocity(const NewtonBody* b, float* v);                         // 0x549210
__declspec(dllimport) void __cdecl NewtonRagDollBoneGetGlobalMatrix(const NewtonRagDollBone* b, float* m);       // 0x549248
}
#include "../../include/fx.h"   // canonical: g_step_tbl (0x5a9c20)

float __cdecl capsule_sphere_gap(capsule* c, sphere* s);   // 0x409d20


#endif // AGENT4_LOGIC_H
