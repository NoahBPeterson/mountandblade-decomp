//-----------------------------------------------------------------------------
// File: item_kind.cpp  (0x433bf0..0x434b20)
//
// Desc: the item definition record's helpers: mesh variant lookup by modifier bit / class, the weapon-type
//       predicates, the carry placement of an item on the body, and the lookup of an item by name.
//       Functions in ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <math.h>
#include "./item_kind.h"
#include "../../include/item_kind.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

pos* __cdecl identity_pos(pos* out);        // 0x40cd00 (cdecl: writes the identity rotation and a zero origin to out, returns out; the definition's own declaration)

// @ 0x433bf0
tstring ItemKind::mesh_name_for(int bit)
{
    int found = -1;
    unsigned __int64 mask = 1i64 << bit;
    int n = mesh_count;
    for (int i = 0; i < n; i++) {
        if ((meshes[i].flags & 0xF000000000000000ui64) == 0) {
            if (found == -1) found = i;
            if (meshes[i].flags & mask) found = i;
        }
    }
    if (found >= 0) return meshes[found].name;
    return key;
}
// @ 0x433cb0
res_obj* ItemKind::mesh_for(unsigned __int64 cls, int bit)
{
    res_obj* none = 0;
    unsigned __int64 mask = 1i64 << bit;
    int n = mesh_count;
    bit = -1;
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            unsigned __int64 c = meshes[i].flags & 0xF000000000000000ui64;
            if (c != 0 && c == cls) {
                if (bit == -1) bit = i;
                if (meshes[i].flags & mask) bit = i;
            }
        }
        if (bit == -1) return 0;
        return g_resmgr.get_object(&meshes[bit].name);
    }
    return none;
}
// @ 0x433d90
bool ItemKind::is_ranged()
{
    unsigned t = type_flags & 0xff;
    return t == itp_type_bow || t == itp_type_crossbow || t == itp_type_thrown || t == itp_type_pistol || t == itp_type_musket;
}
// @ 0x433dc0
bool ItemKind::is_melee()
{
    unsigned t = type_flags & 0xff;
    return t == itp_type_one_handed_wpn || t == itp_type_two_handed_wpn || t == itp_type_polearm;
}
// @ 0x433df0
int ItemKind::ammo_skill()
{
    unsigned t = type_flags & 0xff;
    int r = -1;
    if (t == 2 || t == 3 || t == 4 || t == 9 || t == 10 || t == 16 || t == 17) r = 19;
    if (t == 8) return 14;
    if (t == 7) r = 12;
    return r;
}

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
void ItemKind::get_carry_pos(pos2* out, int* bone_flags, int* bone)
{
    vector4 v;
    float a;
    *bone = 0;
    pos2 ident;
    *out = *(pos2*)identity_pos((pos*)&ident);
    __int64 carry = flags & itcf_carry_mask;
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

// @ 0x434ad0
int find_item(const tstring* name)
{
    for (int i = 0; i < g_item_kind_count; i++)
        if (g_items[i].key.equals(*name)) return i;
    return 0;
}
