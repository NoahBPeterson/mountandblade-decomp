// fx_anim.h: the effect definition's animation (src/rgl/dynamic/fx_anim.cpp, 0x421df0..0x423c00): the per-frame update
// of FxDef (fx.h, src/rgl/scene), its pose and envelope passes, the quaternion type, the key-frame curve evaluator
// and the key table. One header for the classes of this object.
//
// FxDef and FxElem (../scene/fx.h) are the shared classes; this header only adds views of their storage:
//   FxBone     : FxElem  accessors for the quaternion fields (FxElem::q / padd4 / qe4 / vf4 are typed as raw floats there)
//   AnimRec            the 0xd8-byte record of FxDef::env (fx.h calls it EnvRec: cur / target / release / attack /
//                      k0..k2 are the same bytes seen through the envelope pass 0x421ef0 and ~EnvRec)
//   FxAnim   : FxDef   the member functions of FxDef that fx.h does not declare (no new fields; cast `this`)
// The slices called FxAnim "Animator" (s00422130) and "Curve" (s00422db0, s00423540), AnimRec "Rec" / "CurveRec"
// and FxBone "FxBone" / "QBlock": all views of the same FxDef / FxElem / env storage.
#ifndef RGL_DYNAMIC_FX_ANIM_H
#define RGL_DYNAMIC_FX_ANIM_H
#include <vector>
#include <math.h>
#include <stddef.h>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../scene/scene_math.h"
#include "../scene/frustum.h"
#include "../scene/scene.h"
#include "../scene/emitter.h"
#include "../scene/scene_entity.h"
#include "../scene/world.h"
#include "../scene/fx.h"

#include "../../include/quaternion.h"
__declspec(noinline) void quat_to_matrix(rgl::matrix* m, const quaternion* q);                                  // 0x423080
__declspec(noinline) void quat_to_axis_angle(rgl::vector4* axis, float* angle, const quaternion* q);            // 0x423160

extern __int64 g_time[];                    // 0x5e2948 clock table, 1e-5 s ticks (scene_objects.cpp uses it too)
extern __int64 g_bit[];                     // 0x5a8e68 g_bit[i] == 1 << i
extern int g_use_slerp;                     // 0xa171dc

// ---- key tables ------------------------------------------------------------------------------------------------
// One table: values (vector4 or quaternion records), key array (ints), key count. KeyTable::find 0x423a50.
#include "../../include/key_table.h"   // KeyTable (one definition for src/game and src/rgl; find 0x423a50 is defined in fx_anim.cpp)
// the table object an AnimRec points to: +0x1c one KeyTable per bone (quaternion keys), +0x28 one table (vector keys)
struct KeyObj {
    char pad0[0x1c];
    KeyTable* tables;                       // +0x1c  read by FxAnim::value 0x423540
    char pad20[8];
    KeyTable tbl;                           // +0x28  read by FxAnim::table_value 0x423440
};

// ---- the 0xd8-byte animation record (FxDef::env, count FxDef::nenv) ---------------------------------------------
// Offsets: update_poses2 0x422130 (weight, masks, f18, bone, blend_a, ref, add_pos, wt), AnimRec::phase 0x4231f0,
// AnimRec::shape 0x4232b0, key_value 0x423300, lerp_value 0x423390, table_value 0x423440, value 0x423540.
struct AnimRec {
    float weight;                           // +0x00 (EnvRec::cur)
    char pad4[4];
    unsigned __int64 active_mask;           // +0x08 bit per bone
    unsigned __int64 flip_mask;             // +0x10 bit per bone
    float f18;                              // +0x18 shaped phase
    int cur;                                // +0x1c
    int clock;                              // +0x20 index into g_time
    char pad24[4];
    __int64 start;                          // +0x28
    char pad30[0x3c - 0x30];
    int bone;                               // +0x3c
    float blend_a;                          // +0x40
    char pad44[0x4c - 0x44];
    int key_begin, key_end;                 // +0x4c, +0x50
    char pad54[0x5c - 0x54];
    KeyObj* table;                          // +0x5c
    char pad60[0x68 - 0x60];
    quaternion* ref;                        // +0x68 per-bone reference quaternions
    float add_pos[3];                       // +0x6c
    char pad78[4];
    rgl::vector4 v7c;                       // +0x7c
    rgl::vector4 v8c;                       // +0x8c
    int type;                               // +0x9c  11 constant v7c, 13 table, 14 lerp, else v8c
    float period;                           // +0xa0
    char pada4[4];
    int mode;                               // +0xa8  20 loop, 21 clamp, 22 clamp and clear flag
    int flag;                               // +0xac
    char padb0[4];
    int shape_mode;                         // +0xb4
    char padb8[8];
    float wt[2];                            // +0xc0 weights of the two keys of the pair (update_poses2)
    int offs[3];                            // +0xc8 key offsets, indexed by j (FxAnim::value 0x423540)
    char padd4[4];
    __declspec(noinline) float phase();                       // 0x4231f0
    __declspec(noinline) float shape(float x);                // 0x4232b0
};

#define FA_CHECK(T, m, off) typedef char T##_##m##_at_##off[(offsetof(T, m) == (off)) ? 1 : -1]
FA_CHECK(AnimRec, active_mask, 0x08); FA_CHECK(AnimRec, f18, 0x18); FA_CHECK(AnimRec, start, 0x28);
FA_CHECK(AnimRec, bone, 0x3c); FA_CHECK(AnimRec, key_begin, 0x4c); FA_CHECK(AnimRec, table, 0x5c);
FA_CHECK(AnimRec, ref, 0x68); FA_CHECK(AnimRec, v7c, 0x7c); FA_CHECK(AnimRec, v8c, 0x8c); FA_CHECK(AnimRec, type, 0x9c);
FA_CHECK(AnimRec, period, 0xa0); FA_CHECK(AnimRec, mode, 0xa8); FA_CHECK(AnimRec, shape_mode, 0xb4);
FA_CHECK(AnimRec, wt, 0xc0); FA_CHECK(AnimRec, offs, 0xc8);

// ---- the bone (FxElem) with its quaternion fields typed ----------------------------------------------------------
// FxElem::q 0xc4 (rotation), padd4 0xd4 (absolute rotation), qe4 0xe4 (delta of the last frame), vf4 0xf4 (angular
// velocity); the key cache of FxAnim::value: t 0x104 (cur index per 2*i+j), c 0x114 (table per i), qa 0x11c / qb 0x15c
// (sign-corrected key pair per 2*i+j).
struct FxBone : FxElem {
    quaternion& rot() { return *(quaternion*)&q; }
    quaternion& abs_rot() { return *(quaternion*)padd4; }
    quaternion& delta() { return *(quaternion*)&qe4; }
    rgl::vector4& ang_vel() { return vf4; }
    int* cur_idx() { return (int*)t; }
    KeyObj** tab() { return (KeyObj**)c; }
    quaternion* ka() { return (quaternion*)qa; }
    quaternion* kb() { return (quaternion*)qb; }
};

// ---- FxDef members that fx.h does not declare ----------------------------------------------------------------------
struct FxAnim : FxDef {
    FxBone* bone(int i) { return (FxBone*)elems + i; }
    AnimRec* rec(int i) { return (AnimRec*)env + i; }
    __declspec(noinline) void update_envelopes(float dt);   // 0x421ef0
    __declspec(noinline) void update_poses();               // 0x422080
    __declspec(noinline) void update_poses2();              // 0x422130
    __declspec(noinline) rgl::vector4 key_value(int i);     // 0x423300
    __declspec(noinline) rgl::vector4 lerp_value(int i);    // 0x423390
    __declspec(noinline) rgl::vector4 table_value(int i);   // 0x423440
    __declspec(noinline) quaternion* value(quaternion* out, int i, int j, int k);   // 0x423540
    __declspec(noinline) quaternion* chain(quaternion* out, int i, int k);          // 0x423b60
};

__declspec(noinline) void slerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t);      // 0x4238c0
__declspec(noinline) void nlerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t);      // 0x4239c0

// ---- "ext" twins: the functions of this object as their callers saw them -------------------------------------------
// The callers of these functions were compiled without knowledge of the callee (cl 13.10 keeps the callee's register
// use and side effects when the callee is compiled first in the same object, and the callers then differ in register
// choice; the same effect as in src/rgl/shapes/shapes.h). Each twin is only declared; fx_anim.cpp binds its symbol to
// the real function with /alternatename, so the link has one function per address. The `// 0x...` annotations give
// the resolver the address.
struct quaternion_x : quaternion {
    quaternion& set(const float& x_, const float& y_, const float& z_, const float& w_);   // 0x422db0
    void add(const quaternion& o);                                                         // 0x422de0
    quaternion operator+(const quaternion& o) const;                                       // 0x422e10
    void normalize();                                                                      // 0x422e40
    quaternion mul(const quaternion& o) const;                                             // 0x422eb0
    quaternion* mul_out(quaternion* r, const quaternion* o) const;                     // 0x422eb0 (the by-value return as its out-parameter)
    quaternion mul_conj(const quaternion& o) const;                                        // 0x422f60
    float dot(const quaternion& o) const;                                                  // 0x423010
    bool dot_negative(const quaternion& o) const;                                          // 0x423040
};
void quat_to_matrix_x(rgl::matrix* m, const quaternion* q);                                // 0x423080
void quat_to_axis_angle_x(rgl::vector4* axis, float* angle, const quaternion* q);          // 0x423160
void slerp_x(quaternion* out, const quaternion* a, const quaternion* b, const float* t);   // 0x4238c0
void nlerp_x(quaternion* out, const quaternion* a, const quaternion* b, const float* t);   // 0x4239c0
struct KeyTable_x : KeyTable { void find(int* cur, float* frac, float* key); };            // 0x423a50
struct AnimRec_x : AnimRec {
    float phase();                                          // 0x4231f0
    float shape(float x);                                   // 0x4232b0
};
struct FxAnim_x : FxAnim {
    rgl::vector4 key_value(int i);                          // 0x423300
    quaternion bone_quat(int i, int j, int k);              // 0x423540 (value, by-value return)
    quaternion parent_quat(int i, int k);                   // 0x423b60 (chain, by-value return)
};

#endif
