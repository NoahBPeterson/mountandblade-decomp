// Slice s00433cb0: item-kind helpers: mesh object lookup by class/bit, and item type predicates. Names provisional.
#include "s00433cb0.h"

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
