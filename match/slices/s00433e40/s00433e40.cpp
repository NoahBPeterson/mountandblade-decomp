// Slice s00433e40: item carry placement. Given an item's capability flags (itcf_carry_*), fills in the local
// transform (position/frame relative to the carrying bone), the bone index and a bone flag word.
#include <windows.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

struct pos2 : rgl::matrix {
    void rotate_fu(const float* angle);     // 0x40e570
    void rotate_su(const float* angle);     // 0x40e670
    void rotate_sf(const float* angle);     // 0x40e770
};

pos2 identity_pos();                        // 0x40cd00 (cdecl, hidden return pointer: identity rotation, zero origin)

struct item_kind {
    char pad0[0x1d0];
    __int64 capability_flags;               // 0x1d0 itcf_* (64-bit)
    char pad1[0x20c - 0x1d8];
    int weight;                             // 0x20c (provisional)
    void get_carry_pos(pos2* out, int* bone_flags, int* bone);
};

#define itcf_carry_slot1 0x100000000i64
#define itcf_carry_slot2 0x200000000i64
#define itcf_carry_mask                      0x00000007f0000000i64
#define itcf_carry_sword_left_hip            0x10000000i64
#define itcf_carry_axe_left_hip              0x20000000i64
#define itcf_carry_dagger_front_left         0x30000000i64
#define itcf_carry_dagger_front_right        0x40000000i64
#define itcf_carry_quiver_front_right        0x50000000i64
#define itcf_carry_quiver_back_right         0x60000000i64
#define itcf_carry_quiver_right_vertical     0x70000000i64
#define itcf_carry_quiver_back               0x80000000i64
#define itcf_carry_revolver_right            0x90000000i64
#define itcf_carry_pistol_front_left         0xa0000000i64
#define itcf_carry_bowcase_left              0xb0000000i64
#define itcf_carry_mace_left_hip             0xc0000000i64

// @ 0x433e40
void item_kind::get_carry_pos(pos2* out, int* bone_flags, int* bone)
{
    vector4 v;
    float a;
    *bone = 0;
    *out = identity_pos();
    __int64 carry = capability_flags & itcf_carry_mask;
    *bone_flags = 0;
    if (carry == itcf_carry_sword_left_hip) {
        *bone = 1;
        *bone_flags = 0;
        v.x = 0.24f; v.y = 0.1f; v.z = 0.1f;
        out->o = v;
        a = 1.5393804f; out->rotate_su(&a);
        a = -2.2f; out->rotate_sf(&a);
        a = 0.12f; out->rotate_fu(&a);
        a = -0.2f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_axe_left_hip) {
        vector4& o = out->o;
        *bone = 1;
        *bone_flags = 0;
        v.x = 0.21f; v.y = 0.12f; v.z = 0.14f;
        o = v;
        a = 1.4451327f; out->rotate_su(&a);
        a = 1.0f; out->rotate_sf(&a);
        a = 3.3415928f; out->rotate_su(&a);
        float s = (float)weight * 0.01f - 0.3f;
        vector4 t(s * out->f.x, s * out->f.y, s * out->f.z);
        o.x = o.x - t.x;
        o.y = o.y - t.y;
        o.z = o.z - t.z;
    }
    else if (carry == itcf_carry_mace_left_hip) {
        vector4& o = out->o;
        *bone = 1;
        *bone_flags = 0;
        v.x = 0.21f; v.y = 0.12f; v.z = 0.14f;
        o = v;
        a = 1.4451327f; out->rotate_su(&a);
        a = 1.0f; out->rotate_sf(&a);
        a = 3.3415928f; out->rotate_su(&a);
        float s = (float)weight * 0.01f - 0.1f;
        vector4 t(s * out->f.x, s * out->f.y, s * out->f.z);
        o.x = o.x - t.x;
        o.y = o.y - t.y;
        o.z = o.z - t.z;
    }
    else if (carry == itcf_carry_dagger_front_left) {
        *bone = 7;
        *bone_flags = 0;
        v.x = 0.04f; v.y = 0.25f; v.z = 0.15f;
        out->o = v;
        a = 0.15707964f; out->rotate_su(&a);
        a = -2.5f; out->rotate_sf(&a);
        a = -0.05f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_dagger_front_right) {
        *bone = 8;
        *bone_flags = 0;
        v.x = -0.04f; v.y = 0.25f; v.z = 0.15f;
        out->o = v;
        a = -3.2986722f; out->rotate_su(&a);
        a = -2.5f; out->rotate_sf(&a);
        a = 0.05f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_slot1) {
        vector4& o = out->o;
        *bone = 5;
        *bone_flags = 8;
        v.x = -0.19f; v.y = 0.29f; v.z = -0.17f;
        o = v;
        a = 0.5654867f; out->rotate_sf(&a);
        a = 3.1415927f; out->rotate_su(&a);
        float s = (float)weight * 0.01f - 0.3f;
        vector4 t(s * out->f.x, s * out->f.y, s * out->f.z);
        o.x = o.x - t.x;
        o.y = o.y - t.y;
        o.z = o.z - t.z;
    }
    else if (carry == (itcf_carry_sword_left_hip | itcf_carry_slot1)) {
        *bone = 5;
        *bone_flags = 8;
        v.x = -0.19f; v.y = 0.29f; v.z = -0.17f;
        out->o = v;
        a = -2.576106f; out->rotate_sf(&a);
    }
    else if (carry == (itcf_carry_quiver_right_vertical | itcf_carry_slot1)) {
        vector4& o = out->o;
        *bone = 5;
        *bone_flags = 8;
        v.x = -0.19f; v.y = 0.29f; v.z = -0.17f;
        o = v;
        a = 0.5654867f; out->rotate_sf(&a);
        vector4 t(out->f.x * 0.67f, out->f.y * 0.67f, out->f.z * 0.67f);
        o.x = o.x - t.x;
        o.y = o.y - t.y;
        o.z = o.z - t.z;
    }
    else if (carry == (itcf_carry_axe_left_hip | itcf_carry_slot1)) {
        *bone = 0;
        *bone_flags = 8;
        v.x = -0.19f; v.y = 0.04f; v.z = -0.18f;
        out->o = v;
        a = 3.2986722f; out->rotate_su(&a);
        a = 1.0995574f; out->rotate_sf(&a);
    }
    else if (carry == (itcf_carry_quiver_back | itcf_carry_slot1)) {
        *bone = 0;
        *bone_flags = 8;
        v.x = -0.06f; v.y = 0.18f; v.z = -0.2f;
        out->o = v;
        a = -3.2986722f; out->rotate_su(&a);
        a = 1.5707964f; out->rotate_sf(&a);
    }
    else if (carry == (itcf_carry_dagger_front_left | itcf_carry_slot1)) {
        *bone = 0;
        *bone_flags = 8;
        v.x = -0.22f; v.y = -0.06f; v.z = -0.2f;
        out->o = v;
        a = 3.138451f; out->rotate_su(&a);
        a = 1.0995574f; out->rotate_sf(&a);
    }
    else if (carry == (itcf_carry_quiver_front_right | itcf_carry_slot1)) {
        vector4& o = out->o;
        *bone = 6;
        *bone_flags = 8;
        v.x = 0.19f; v.y = 0.34f; v.z = -0.155f;
        o = v;
        a = -0.5654867f; out->rotate_sf(&a);
        a = 1.4137167f; out->rotate_su(&a);
        a = 0.1f; out->rotate_sf(&a);
        float s = (float)weight * 0.01f + 0.5f;
        vector4 t(s * out->f.x, s * out->f.y, s * out->f.z);
        o.x = o.x - t.x;
        o.y = o.y - t.y;
        o.z = o.z - t.z;
    }
    else if (carry == (itcf_carry_quiver_back_right | itcf_carry_slot1)) {
        *bone = 6;
        *bone_flags = 8;
        v.x = -0.05f; v.y = 0.0f; v.z = -0.19f;
        out->o = v;
        a = -0.5654867f; out->rotate_sf(&a);
        a = -1.5707964f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_quiver_front_right) {
        *bone = 4;
        *bone_flags = 0;
        v.x = -0.21f; v.y = 0.15f; v.z = 0.0f;
        out->o = v;
        a = 1.4451327f; out->rotate_su(&a);
        a = -2.5f; out->rotate_sf(&a);
        a = -0.2f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_quiver_back_right) {
        *bone = 4;
        *bone_flags = 0;
        v.x = -0.05f; v.y = 0.03f; v.z = -0.19f;
        out->o = v;
        a = 0.15707964f; out->rotate_su(&a);
        a = 1.1f; out->rotate_sf(&a);
        a = 3.1415927f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_quiver_right_vertical) {
        *bone = 4;
        *bone_flags = 0;
        v.x = -0.23f; v.y = 0.05f; v.z = 0.0f;
        out->o = v;
        a = 1.4451327f; out->rotate_su(&a);
        a = 0.5f; out->rotate_sf(&a);
        a = -0.05f; out->rotate_su(&a);
        a = 0.2f; out->rotate_fu(&a);
    }
    else if (carry == itcf_carry_quiver_back) {
        *bone = 5;
        *bone_flags = 8;
        v.x = -0.05f; v.y = 0.02f; v.z = -0.17f;
        out->o = v;
        a = 0.37699112f; out->rotate_sf(&a);
    }
    else if (carry == (itcf_carry_dagger_front_right | itcf_carry_slot1)) {
        *bone = 3;
        *bone_flags = 0;
        v.x = 0.18f; v.y = -0.1f; v.z = -0.1f;
        out->o = v;
        a = 2.1991148f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_revolver_right) {
        *bone = 9;
        *bone_flags = 4;
        v.x = -0.1f; v.y = 0.0f; v.z = 0.05f;
        out->o = v;
        a = -1.5707964f; out->rotate_su(&a);
        a = 0.9424779f; out->rotate_sf(&a);
        a = -0.1f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_pistol_front_left) {
        *bone = 7;
        *bone_flags = 0;
        v.x = -0.07f; v.y = 0.25f; v.z = 0.15f;
        out->o = v;
        a = 0.15707964f; out->rotate_su(&a);
        a = -1.5f; out->rotate_sf(&a);
        a = -0.05f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_bowcase_left) {
        *bone = 10;
        *bone_flags = 0;
        v.x = 0.26f; v.y = 0.03f; v.z = -0.1f;
        out->o = v;
        a = -4.3982296f; out->rotate_su(&a);
        a = -0.25f; out->rotate_fu(&a);
        a = -0.84823006f; out->rotate_sf(&a);
    }
    else if (carry == (itcf_carry_sword_left_hip | itcf_carry_slot2)) {
        *bone = 1;
        *bone_flags = 0;
        v.x = 0.23f; v.y = 0.15f; v.z = 0.23f;
        out->o = v;
        a = -1.4451327f; out->rotate_su(&a);
        a = 1.5f; out->rotate_sf(&a);
        a = -0.2f; out->rotate_su(&a);
    }
    else if (carry == (itcf_carry_axe_left_hip | itcf_carry_slot2)) {
        *bone = 2;
        *bone_flags = 0;
        v.x = 0.05f; v.y = 0.19f; v.z = 0.24f;
        out->o = v;
        a = -2.1991148f; out->rotate_su(&a);
        a = 1.8f; out->rotate_sf(&a);
        a = -0.2f; out->rotate_su(&a);
    }
}
