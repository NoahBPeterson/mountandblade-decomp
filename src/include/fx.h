// fx.h: the effect definition ("skeleton") and its parts, ONE definition for src/rgl (scene/fx*.cpp, scene_objects.cpp,
// render_scene.cpp), src/game (entity.cpp, agent3/agent_fx_sound.cpp) and src/game/terrain_a (the envelope record).
//   FxElem (0x1dc bytes, vtable 0x54b454 = { 0x420e90 add_to_first, 0x41f250 set_data, 0x41f2a0 find_kid }: three virtual
//          methods, NO virtual destructor): one bone of the definition with its psys / lod lists.
//          Constructor 0x428d10 (src/rgl/scene/fx_support.cpp), destructor 0x41f1d0 and set_data / find_kid (src/game/entity.cpp).
//          The two lists at +0x19c / +0x1b0 hold rgl::mesh* / rgl::lod_mesh2* (src/rgl); agent_fx_sound.cpp (0x442390 /
//          0x4423f0) pushes the same pointers as ints (it casts the lists to std::vector<int>).
//   FxDef  (0x7c bytes): owns an array of FxElem, its own lists and the envelope records (EnvRec). Constructor 0x428820,
//          destructor 0x421680, init 0x4288c0, update 0x421df0, render 0x428ff0, skin 0x4291c0 (src/rgl/scene);
//          attach_psys / attach_group 0x442460 / 0x4424a0 (src/game/agent3/agent_fx_sound.cpp).
//   EnvRec (0xd8 bytes): one envelope record. alloc 0x428c60 (rgl), the inline constructor / destructor and reset 0x4686a0
//          (src/game/terrain_a/terrain_a.cpp, FxDef::set_env_count 0x46b9d0 allocates the array; the vector deleting
//          destructor ??_E is 0x41ed70).
// Offsets cite the function that reads or writes them. Needs rgl::vector4 / rgl::matrix (rgl_base.h picks the module's own).
// The slices' names: Slot, NodeOwner, slotarr (FxElem, FxDef).
#ifndef INCLUDE_FX_H
#define INCLUDE_FX_H
#include <vector>
#include "./rgl_base.h"
#include "./mat4.h"
#include "clock_table.h"
#include "./clock_table.h"
#include "./timer.h"                     // gui_timer: EnvRec::tm

namespace rgl { struct lod_mesh; struct mesh; struct lod_mesh2; struct psys; struct scene; }
struct Psys;                                                    // src/game/agent3/agent_fx_sound.h (a particle mesh as the game sees it)
struct PsysGroup;
struct NewtonRagDollBone;
struct KeyObj;                                                  // src/rgl/dynamic/fx_anim.h (the animation's key tables)

// unit-length-ish rotation as w, x, y, z (stored x y z first in the original's arrays; see FxElem::q)
struct quat4 {
    float v[4];
    quat4() {}
    quat4(float w, float x, float y, float z) { v[1] = x; v[2] = y; v[3] = z; v[0] = w; }
};
void quat_from_matrix(quat4* out, const float* m);              // 0x428b10 (cdecl): trace / largest-diagonal branches

struct frame_set;               // rgl/resource.h: the skeleton resource (get_object70); its bones are core/resource_io.h's Bone
struct Bone;

// std::vector<mesh*> whose push_back is the out-of-line instance of the mesh object (0x4119d0): skeleton_entity::load
// calls it, while the other users in these objects reach their own copy
struct mesh_ptr_vec : std::vector<rgl::mesh*> {
    void push_back(rgl::mesh* const& x);                        // 0x4119d0
};

struct FxElem {                                                 // 0x1dc bytes, vtable 0x54b454
    virtual void add_to_first(rgl::psys* o);                    // 0: 0x420e90 (first psys' emitter list)
    virtual void set_data(void* p);                             // 1: 0x41f250 (entity.cpp)
    virtual int find_kid(int id);                               // 2: 0x41f2a0 (entity.cpp)
    rgl::mat4 m4;               // 0x04 world frame (parent * local)       FxDef::init 0x4288c0
    rgl::mat4 m44;             // 0x44 local frame as loaded
    rgl::mat4 m84;             // 0x84 copy of the world frame (FxDef::render 0x428ff0)
    quat4 q;                    // 0xc4 rotation of the local frame (quat_from_matrix)
    quat4 abs_q;                // 0xd4 absolute rotation (parent's abs_q * q; FxAnim::update_poses2 0x422130)
    quat4 qe4;                  // 0xe4 (identity)   ctor 0x428d10
    rgl::vector4 vf4;           // 0xf4
    int t[2][2];                // 0x104 (-1000)
    int c[2];                   // 0x114
    quat4 qa[2][2];             // 0x11c
    quat4 qb[2][2];             // 0x15c
    mesh_ptr_vec psys;          // 0x19c particle meshes (first at 0x1a0; add_psys 0x442390)
    int ngroups;                // 0x1ac groups pushed by add_group 0x4423f0 (skeleton.h's Region::count)
    std::vector<rgl::lod_mesh2*> groups;    // 0x1b0 (first at 0x1b4; add_group 0x4423f0)
    int parent;                 // 0x1c0 (-1 = none)       entity.cpp 0x41f1d0
    int nchild;                 // 0x1c4                   entity.cpp 0x41f1d0
    int* children;              // 0x1c8                   entity.cpp 0x41f1d0
    const Bone* src;            // 0x1cc the skeleton's bone
    float blend_weight;         // 0x1d0 eased towards 1 at blend_rate per second (FxAnim::update_envelopes 0x421ef0)
    float blend_rate;           // 0x1d4 (ctor 1; mesh_appear.cpp sets 0.2..0.3 for the arm bones)
    const NewtonRagDollBone* ragdoll_bone;  // 0x1d8 (skeleton.h's Region::bone, 0x44fc60)

    FxElem();                                                   // 0x428d10
    ~FxElem();                                                  // 0x41f1d0 (entity.cpp)
    void set_color(int c);                                      // 0x4219c0
    void add_psys(rgl::mesh* p);                                // 0x442390 (agent_logic.cpp)
    void add_group(rgl::lod_mesh2* g);                          // 0x4423f0 (agent_logic.cpp)
};

// The key pair at EnvRec+0xc0: the weights of its two keys and their key offsets (zeroed member-wise in this order, then
// w[0] = 1; EnvRec::reset 0x4686a0). The agent sets the weights and offs[1] (Agent 0x43d040: the aim pitch blend).
struct KeyPair {
    float w[2];                 // +0 / +4
    int offs[2];                // +8 / +0xc
    void zero() { w[0] = 0; offs[0] = 0; w[1] = 0; offs[1] = 0; }
};

// 0xd8-byte envelope / animation record; the inline destructor frees three arrays (vector deleting dtor ??_E at 0x41ed70).
// The same bytes are AnimRec (src/rgl/dynamic/fx_anim.h), Channel (src/game/channel.h) and Curve (src/game/agent/curve.h);
// the names below follow them. Defaults: EnvRec::reset 0x4686a0.
struct EnvRec {
    float weight;               // 0x00 current value, eased towards target (FxAnim::update_envelopes 0x421ef0; AnimRec::weight)
    char pad4[4];
    unsigned __int64 active_mask;   // 0x08 bit per bone (ctor / reset: all ones)
    unsigned __int64 flip_mask;     // 0x10 bit per bone
    float shaped_phase;         // 0x18 (AnimRec::f18)
    int cur_key;                // 0x1c
    gui_timer tm;               // 0x20 clock index (ctor: 0), 0x28 start tick (ctor: g_ticks[0]); FxDef::reset_rec 0x4339d0 restamps it
    float target;               // 0x30 weight the record eases towards (reset 0)
    float release;              // 0x34 seconds to fall (reset 1.3)
    float attack;               // 0x38 seconds to rise (reset 4.3)
    int bone;                   // 0x3c (reset -1)
    float blend_a;              // 0x40 (reset 1)
    int k44, k48;               // 0x44 / 0x48 (Skeleton::start_anim 0x439fc0: 0x33, 0x33)
    int key_begin, key_end;     // 0x4c / 0x50 (reset 0 / 1)
    int f54, f58;               // 0x54 / 0x58 (reset 0)
    KeyObj* table;              // 0x5c key tables of the animation (reset 0; Channel::anim)
    rgl::vector4* keys_a;       // 0x60 key a per bone (alloc 0x428c60; Channel::pa)
    rgl::vector4* keys_b;       // 0x64 key b per bone (Channel::pb)
    rgl::vector4* ref;          // 0x68 reference rotation per bone (AnimRec::ref)
    rgl::vector4 add_pos;       // 0x6c (AnimRec::add_pos, three floats used)
    rgl::vector4 v7c[2];        // 0x7c constant value / key value (AnimRec::v7c / v8c, Channel::va / vb)
    int type;                   // 0x9c 11 constant v7c[0], 13 table, 14 lerp, else v7c[1] (reset 13)
    float period;               // 0xa0 (reset 1)
    int fa4;                    // 0xa4 (reset 10)
    int mode;                   // 0xa8 20 loop, 21 clamp, 22 clamp and clear flag (reset 20)
    int flag;                   // 0xac (reset 0)
    char fb0;                   // 0xb0 (reset 0)
    char padb1[3];
    int shape_mode;             // 0xb4 1 linear, 2 sine, 4 square, 5 square root (reset 1)
    int fb8;                    // 0xb8 (reset 0)
    float fbc;                  // 0xbc (reset 1)
    KeyPair wt;                 // 0xc0 the key pair (AnimRec::wt / offs; reset w[0] = 1)
    int step_clock;             // 0xd0 clock index into g_step_tbl (set_env_count 0x46b9d0; Channel::tbl_idx)
    char padd4[4];
    EnvRec() {
        step_clock = 0;
        active_mask = ~0ull;
        weight = 0;
        keys_a = 0; keys_b = 0; ref = 0;
        reset();
    }
    ~EnvRec() { delete[] keys_a; delete[] keys_b; delete[] ref; }
    void reset();                                               // 0x4686a0 (terrain_a.cpp)
    void alloc(int n);                                          // 0x428c60
    // the phase helpers of src/game (agent/curve.cpp 0x4338e0.., rgl/xmlcfg/agent_pick.cpp busy 0x433840)
    float shape(float x);                                       // inline in agent/curve.cpp (0x433930 / 0x433ac0): 1 linear, 2 sine, 5 sqrt, 4 square
    bool busy();                                                // 0x433840 (the step of g_step_tbl[step_clock] reached)
    float unshape(float y);                                     // 0x4338e0
    void set_phase(float period);                               // 0x433930
};

struct FxDef {                                                  // 0x7c bytes
    int count;                  // 0x00 elements
    FxElem* elems;              // 0x04
    frame_set* res;             // 0x08 the skeleton
    rgl::vector4 root_pos;      // 0x0c root bone offset blended from the keys (FxAnim::update_poses2 0x422130)
    int key_idx;                // 0x1c key found by the last table lookup (FxAnim::table_value 0x423440)
    std::vector<rgl::mesh*> psys;   // 0x20 (first at 0x24)
    std::vector<rgl::lod_mesh2*> groups;    // 0x30 (first at 0x34)
    EnvRec* env;                // 0x40
    int nenv;                   // 0x44
    int* envidx;                // 0x48 one index per envelope record (set_env_count 0x46b9d0)
    int f4c;                    // 0x4c (ctor 0)
    bool pose_valid;            // 0x50 (Skeleton::done)
    float pose_dt;              // 0x54 time since the last pose pass (update 0x421df0 adds dt, update_poses 0x422080 clears it)
    bool frozen;                // 0x58 update does nothing (Skeleton::frozen)
    int keep_weight;            // 0x5c ctor 1; reset_rec 0x4339d0 clears a record's weight only when 0 (Curve::f5c)
    int clock;                  // 0x60 clock id of the envelope records (the constructor argument; set_env_count 0x46b9d0)
    int pose_src;               // 0x64 the poses come from the ragdoll: update_poses instead of update_poses2 (Agent::start_ragdoll 0x44fc60 sets 1)
    char track_root_vel;        // 0x68 compute root_vel each update (set when the agent dies; Skeleton::f68)
    rgl::vector4 root_vel;      // 0x6c root_pos velocity (Skeleton::rvx..)

    FxDef(int a);                                               // 0x428820
    ~FxDef();                                                   // 0x421680
    void clear_defs();                                          // 0x420ec0
    void set_env_count(int n);                                  // 0x46b9d0 (ret 4; src/game/terrain_a/terrain_a.cpp: allocates the envelope array)
    // the game's animation controller calls (src/game: agent/curve.cpp, agent2/agent_stats.cpp, agent4/agent_logic.cpp)
    void start_anim(int rec, struct Anim* anim, int a, int b);  // 0x439fc0 (ret 0x10; agent_stats.cpp)
    void reset_rec(int i);                                      // 0x4339d0 (ret 4; agent/curve.cpp)
    float rec_shaped_phase(int i);                              // 0x433ac0 (agent/curve.cpp)
    void set_rec_phase(int i, float p);                         // 0x433b20 (agent/curve.cpp)
    void ensure_updated();                                      // 0x43e740 (agent_logic.cpp: the pose pass unless pose_valid)
    rgl::mat4* rec(int i);                                      // 0x43f260 (agent_logic.cpp: elems[i].m84, the poses brought up to date first)
    void remove_group(rgl::lod_mesh2* g);                       // 0x439ec0 (agent_stats.cpp: from groups, else the first element list holding it)
    void all_groups(rgl::lod_mesh2** out, int* n);              // 0x43a130 (agent_stats.cpp: at most 32)
    void set_visible(bool on);                                  // 0x452f80 (agent_logic.cpp: on the first show re-skins every psys and lod group)
    void set_color(int c);                                      // 0x4218e0
    void update(float dt, const rgl::matrix* m);                // 0x421df0
    void init(frame_set* r);                                    // 0x4288c0
    void render(rgl::scene* sc, const rgl::matrix* frame, const float* dist);   // 0x428ff0
    void skin(rgl::mesh* p, char flag);                         // 0x4291c0
    void skin_lods(rgl::lod_mesh* g, int flag);                 // 0x452530 (ret 8; agent4/agent_update.cpp: skin every mesh of every list of a lod mesh)
    void skin_group(rgl::lod_mesh2* g);                         // 0x4296f0
    void attach_psys(int idx, Psys* p, const rgl::mat4* m);     // 0x442460 (agent_fx_sound.cpp)
    void attach_group(int idx, PsysGroup* g);                   // 0x4424a0 (agent_fx_sound.cpp)
    void add_group(rgl::lod_mesh2* g);                          // 0x478fb0 (terrain_b/mission.cpp: push_back onto groups)
    void elem_add_psys(int i, rgl::psys* ps);                   // 0x488e20 (ui/mission_ui.cpp: elems[i].add_to_first)
    void elem_set_data(int i, void* p);                         // 0x488e40 (ui/mission_ui.cpp: elems[i].set_data)
};

#endif // INCLUDE_FX_H
