// agent_logic.h: the classes of agent_logic.cpp (0x444c50..0x460160) that no shared header has: the troop-class voice table, the
// scene entity / missile / blow views the agent code calls, the Newton Game Dynamics 1.x imports and the few small records of the
// object. Everything else is a shared class (src/game/*.h, src/include/*.h, agent3/agent_types.h); the per-field additions made
// to those headers are listed in MIGRATION.md. Offsets are read by a function of the module (address cited). Names provisional.
#ifndef AGENT4_LOGIC_H
#define AGENT4_LOGIC_H

#include <vector>
#include "../tstring.h"
#include "../item_ref.h"
#include "../../include/rgl_base.h"

// 0x1f8-byte voice records (table object at 0x8ed4f60: records at +0x1c, count at +0x28; 0x4459c0 reads snd[kind] at +0x1cc)
struct VoiceRec { char pad[0x1cc]; int snd[1]; char pad1d0[0x1f8 - 0x1d0]; };
struct VoiceTable {
    char pad0[0x18];
    std::vector<VoiceRec> recs;                         // +0x18 (first at +0x1c); element size 0x1f8
    unsigned count;                                     // +0x28
    int get_f1f0(unsigned i);                           // 0x43b100 (agent2/agent_ai.h)
    int get_f1f4(unsigned i);                           // 0x43b130
    int sound(unsigned cls, int kind)
    {
        if (cls < count)
            return recs[cls].snd[kind];
        return -1;
    }
};
extern VoiceTable g_anim_table;                             // 0x8ed4f60

// ---- free functions of earlier objects the module calls -------------------------------------------------------------
float frand();                                                          // 0x40e400 (rand() % 15817) / 15817
float __cdecl accel_factor(const float* a, const float* b, const float* c);   // 0x435ae0
float __cdecl fmin_ref(const float* a, const float* b);                 // 0x4324b0
float __cdecl fmax_ref(const float* a, const float* b);                 // 0x4324d0
float __cdecl clamp(const float* x, const float* lo, const float* hi);  // 0x40e3d0
int __stdcall in_id_set(int id);                                        // 0x432640
extern int g_difficulty;                                                // 0x5aa04c
extern unsigned g_input_mask;                                           // 0x5e2474
extern unsigned g_input_flags;                                          // 0x5e2470

// ---- the scene entity as the agent code calls it (vtable 0x54b460 family: src/game/entity.h has the base class rgl::entity,
// which cannot be extended here: the physics entities add virtual slots 24..28 and the code reads fields of the derived classes).
// Slot / field names are the ones of the calls in this module; unused slots are placeholders.
struct shape_base;
struct res_obj;
struct capsule;
struct SceneEnt {
    virtual ~SceneEnt();                                // 0
    virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual float get_fbc();                            // 5 (+0x14) 0x41fd80
    virtual void set_fbc(const float* p);               // 6 (+0x18) 0x41fd90
    virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14();
    virtual bool is_movable();                          // 15 (+0x3c) 0x45d820
    virtual void s16();
    virtual void s17(); virtual void s18(); virtual void s19(); virtual void s20(); virtual void s21();
    virtual void set_frame(const rgl::matrix* m, const rgl::vector4* scale);   // 22 (+0x58) (0x44c210)
    virtual void s23(); virtual void s24(); virtual void s25();
    virtual void collide(const rgl::vector4* point, const rgl::vector4* impulse);   // 26 (+0x68) (0x45d820)
    virtual void s27();
    virtual void set_shape(shape_base* s);              // 28 (+0x70) (0x44c210)
    char pad4[0xc - 4];
    unsigned id;                                        // +0xc (0x448eb0, 0x44c210, 0x45d820)
    int type;                                           // +0x10 (0xb = agent, 0 = static; the prop code calls it kind)
    char pad14[0x18 - 0x14];
    rgl::matrix p18;                                    // +0x18 the frame the entity moves in (0x45d820: transform_point_to_local)
    rgl::matrix local;                                  // +0x58 local frame (0x44be50)
    shape_base* shape;                                  // +0x98 collision shape
    char pad9c[0xc4 - 0x9c];
    rgl::vector4 vc4;                                   // +0xc4 (0x44be50)
    char padd4[0x118 - 0xd4];
    std::vector<res_obj*> meshes;                       // +0x118 (first at +0x11c) (0x44c210)
    char pad128[0x140 - 0x128];
    char f140;                                          // +0x140 (visible) (0x44be50)
    char pad141[0x15c - 0x141];
    float f15c;                                         // +0x15c (0x44c210)
    char pad160[0x170 - 0x160];
    float f170;                                         // +0x170 (0x44c210)
    char pad174[0x19c - 0x174];
    rgl::vector4 v19c;                                  // +0x19c (0x44c210)
    rgl::vector4 v1ac;                                  // +0x1ac
    char pad1bc[0x1d4 - 0x1bc];
    char f1d4;                                          // +0x1d4 (0x44c210)
    void add_extra(res_obj* m);                          // 0x442110
};
struct EntVec {                                         // std::vector<SceneEnt*> layout (VC7.1), freed by hand (0x448eb0); Mission +0x63c0 (0x44c210)
    int alloc_pad;
    SceneEnt** first;
    SceneEnt** last;
    SceneEnt** end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    unsigned capacity() const { return first == 0 ? 0 : end - first; }
    void insert_n(SceneEnt** where, unsigned n, SceneEnt* const& v);       // 0x43e7b0 (ICF-folded with vector<int>::_Insert_n)
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

// std::vector<res_obj*> whose assignment and push_back stay out of line (pointer vectors fold with vector<int>) (0x44c210)
struct MeshVec : std::vector<res_obj*> {
    MeshVec& operator=(const std::vector<res_obj*>& o);     // 0x43f0a0
    void push_back(res_obj* const& x);                      // 0x4119d0
};

// the body resource of a prop (0x44c210): src/game/res_shape.h (shared with terrain_b)
#include "../res_shape.h"

// a list of sound events (count + array of 0x30-byte records): ItemKind +0x228, Rec78 +0x6c (0x44c210)
struct SoundSet {
    int count; char* items;
    void play(int id);                                  // 0x48add0
};

// one 0x1dc-byte bone record of the animator (Skeleton::recs): the world frame at +0x84 (0x44e2b0)
struct BoneW {
    char pad0[0x84];
    rgl::matrix world;                                  // +0x84 (origin at +0xb4)
    char pad_c4[0x1dc - 0xc4];
};
rgl::vector4* __cdecl rotate_scale_xy(rgl::vector4* out, const rgl::vector4* in, const float* angle, const rgl::vector4* scale);   // 0x436210
void __cdecl rand_cone_dir(rgl::vector4* out, rgl::vector4 dir, float spread);   // 0x432c40

extern __int64 g_a70448;                                // 0xa70448 (sound-event context)
extern __int64 g_a70450;                                // 0xa70450

// ---- the flying missile record (0xb0 bytes, src/game/missile.h: shared with terrain_a/misc.h)
#include "../missile.h"


// ---- the second half of the object (formerly agent_update.h) ------------------------------------------------------------------------

#include "rgl.h"
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
rgl::mat3* __cdecl identity3(rgl::mat3* out);                                                                   // 0x40cd60

struct V4 { float x, y, z, w; };               // constructor-free storage for the local arrays of start_ragdoll
struct M4 { V4 s, f, u, o; };

extern float g_missile_damage_speed_power;                   // 0xa18194
extern float g_melee_damage_speed_power;                   // 0xa18198
extern float g_couched_lance_damage_multiplier;             // 0xa181a0
extern int g_ui_enabled;                // 0x5aa060

// ---- the blow record (0x58 bytes; agent/agent.h calls it RecZSubB with the fields m0..m54 and the initialiser 0x4353a0, which the
// manifests bind by that name, so this is the second declaration of the same record, with the names the blow code uses) ----
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
extern int g_fx_on;                                 // 0x5aa05c
extern int g_sound_blood;                           // 0x5aa2e4
extern int g_tick_a;                                // 0x5aa048
extern int g_tick_b;                                // 0xa18234
float __cdecl capsule_sphere_gap(capsule* c, sphere* s);   // 0x409d20
extern float g_armor_soak_factor_against_cut[3];                          // 0xa1817c
extern float g_armor_reduction_factor_against_cut[3];                          // 0xa18188


#endif // AGENT4_LOGIC_H
