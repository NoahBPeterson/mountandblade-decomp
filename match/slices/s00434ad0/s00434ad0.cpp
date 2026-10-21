// Slice s00434ad0: item-record helpers (lookup by name, modifier-adjusted stats of an item reference) followed by
// small unit-record accessors and initialisers. Names provisional.
#include "s00434ad0.h"

// @ 0x434ad0
int find_item(const tstring* name)
{
    for (int i = 0; i < g_item_count; i++)
        if (g_items[i].name.equals(*name)) return i;
    return 0;
}
// @ 0x434b20
tstring ItemRef::mesh_name()
{
    if (item >= 0 && item < g_item_count)
        return g_items[item].mesh_name_for(b.mod);
    return tstring("Error: Invalid item");
}
// @ 0x434b70
int ItemRef::max_amount()
{
    if (item < 0) return 0;
    int v = g_items[item].f210;
    if (v != 0 && (m4 & 0xff000000) == 0x2a000000) {
        float f = v * 0.13f;
        int d = round_float(&f);
        if (d < 1) d = 1;
        v += d;
    }
    return v;
}
// @ 0x434be0
void ItemRef::add_amount(int n)
{
    int v = (m4 & 0xff) + n;
    if (v > max_amount()) {
        v = max_amount();
        b.amount = 0;
        m4 |= v;
        return;
    }
    b.amount = 0;
    m4 |= v;
}
// The same function seen from outside this TU (cl must not assume the register use of the body above).
struct ItemRefExt { int item; unsigned m4; int max_amount(); };   // 0x434b70
// @ 0x434c30
int ItemRef::room_left()
{
    int a = m4 & 0xff;
    return ((ItemRefExt*)this)->max_amount() - a;
}
// @ 0x434c50
int ItemRef::stat_1ec()
{
    if (item < 0) return 0;
    int v = g_items[item].f1ec;
    if (v != 0) {
        int m = b.mod;
        if ((char)g_items[item].type_flags == itp_type_horse) {
            if (m == imod_timid) return v - 1;
            if (m == imod_stubborn) return v + 1;
            if (m == imod_champion) return v + 2;
        } else {
            if (m == imod_heavy) return v + 1;
            if (m == imod_strong) return v + 2;
            if (m == imod_masterwork) v += 4;
        }
    }
    return v;
}
// @ 0x434cb0
unsigned ItemRef::stat_21x(int which)
{
    if (item < 0) return 0;
    unsigned v;
    if (which == 0) v = g_items[item].f218;
    else v = g_items[item].f214;
    v &= 0xff;
    if (v != 0) {
        switch (b.mod) {
        case imod_cracked: v -= 5; break;
        case imod_rusty:
        case imod_bent: v -= 3; break;
        case imod_chipped: v -= 1; break;
        case imod_crude: v -= 2; break;
        case imod_fine: v += 1; break;
        case imod_heavy: v += 2; break;
        case imod_balanced:
        case imod_strong: v += 3; break;
        case imod_tempered: v += 4; break;
        case imod_masterwork: v += 5; break;
        }
    }
    return v;
}
// @ 0x434d70
int ItemRef::stat_1f0a()
{
    if (item < 0) return 100;
    int v = g_items[item].f1f0;
    if (v != 0) {
        switch (b.mod) {
        case imod_balanced: v += 3; break;
        case imod_tempered: return v;
        case imod_masterwork: v += 1; break;
        case imod_heavy: v -= 2; break;
        case imod_bent:
        case imod_strong: v -= 3; break;
        }
    }
    return v;
}
// @ 0x434df0
int ItemRef::stat_1e8()
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
// @ 0x434eb0
int ItemRef::stat_200()
{
    if (item < 0) return 0;
    int v = g_items[item].f200;
    if (v != 0) {
        switch (b.mod) {
        case 2:
        case 0x15: v -= 3; break;
        case 7: v -= 1; break;
        case 0x18: v += 1; break;
        case 0x19: v += 2; break;
        case 0x12:
        case 0x1a: v += 3; break;
        case 1: v -= 4; break;
        case 5:
        case 0x16: v -= 2; break;
        case 0x1b: v += 4; break;
        case 0x1d: v += 6; break;
        }
        if (v < 0) v = 0;
    }
    return v;
}
// @ 0x434f60
int ItemRef::stat_1f4()
{
    if (item < 0) return 0;
    int v = g_items[item].f1f4;
    if (v != 0) {
        switch (b.mod) {
        case 0x1e: v -= 5; break;
        case 0x1f: v -= 2; break;
        case 0x23: v += 1; break;
        case 0x24: v += 2; break;
        }
        if (v < 1) v = 1;
    }
    return v;
}
// @ 0x434fd0
int ItemRef::stat_1f0b()
{
    if (item < 0) return 0;
    int v = g_items[item].f1f0;
    if (v != 0) {
        switch (b.mod) {
        case 0x1e: v -= 5; break;
        case 0x1f: v -= 2; break;
        case 0x23: v += 1; break;
        case 0x24: v += 2; break;
        }
        if (v < 1) v = 1;
    }
    return v;
}
// @ 0x435040
int ItemRef::stat_214()
{
    if (item < 0) return 0;
    int v = g_items[item].f214 & 0xff;
    if (v != 0) {
        switch (b.mod) {
        case 0x12: v += 4; break;
        case 0x23: v += 1; break;
        case 0x24: v += 2; break;
        }
        if (v < 1) v = 1;
    }
    return v;
}
// @ 0x435090
int ItemRef::stat_204()
{
    if (item < 0) return 100;
    int v = g_items[item].f204;
    if (v == 0) v = 99;
    return v;
}
// @ 0x4350c0
static __declspec(noinline) int slot_class_impl(int item, int flag)
{
    unsigned t = g_items[item].type_flags & 0xff;
    if (t == itp_type_bow) return 3;
    if (t == itp_type_crossbow) return 4;
    if (t == itp_type_thrown) return 5;
    if (t == itp_type_pistol || t == itp_type_musket) return 6;
    if (t == itp_type_polearm) return 2;
    if (t == itp_type_one_handed_wpn) return 0;
    return flag == 0;
}
int slot_class(int item, int flag) { return slot_class_impl(item, flag); }
// @ 0x435130
bool Agent::has_flag10()
{
    if (flags30 & 0x10) return true;
    return false;
}
// @ 0x435150
int Agent::stat_int(int i) { return (int)stat[i]; }
// @ 0x435170
int Agent::scaled_b0()
{
    return (int)((b0 + 10) * (b0 + 10) * 0.1f);
}
// @ 0x4351a0
int Agent::nibble(int i)
{
    int v = (nib[i / 8] >> ((i % 8) * 4)) & 0xf;
    if (v < 0) return 0;
    int cap = *(int*)(g_skill_tbl + i * 0x44 + 0x34);
    if (v > cap) v = cap;
    return v;
}
// @ 0x4351f0
ItemRef Agent::ranged_ref()
{
    ItemRef r;
    int it = slots.a.item;
    if (it < 0) {
        r.item = -1;
        return r;
    }
    if ((char)g_items[it].type_flags == itp_type_thrown) {
        r.item = slots.a.item;
        r.m4 = slots.a.m4;
        return r;
    }
    r.item = slots.b.item;
    r.m4 = slots.b.m4;
    return r;
}
// @ 0x435260
int find_in_table20(const tstring* name)
{
    for (int i = 0; i < g_tbl20_count; i++)
        if (g_tbl20[i].name.equals(*name)) return i;
    return 0;
}
// @ 0x4352a0
int find_in_table78(const tstring* name)
{
    int r = -1;
    for (int i = 0; i < g_tbl78_count && r == -1; i++)
        if (g_tbl78[i].name.equals(*name)) r = i;
    return r;
}
// @ 0x4352f0
void Agent::reset()
{
    m10 = 0;
    m188 = 0;
    m18c = 0;
    m1a0 = 0;
    m1a4 = 0;
    m1b0 = 0;
    m1b4 = 0;
    m1b8 = 0;
    m168 = m14 = -1;
    pos tmp;
    md8 = *identity_pos(&tmp);
    f16c = 1.0f;
    v158 = vector4(1.0f, 1.0f, 1.0f);
}
// @ 0x4353a0
Rows* Rows::init()
{
    vector4 zero(0.0f, 0.0f, 0.0f);
    vector4 unit(1.0f, 0.0f, 0.0f);
    v8 = zero;
    v18 = unit;
    v28 = unit;
    m0 = 0;
    m4 = -1;
    m38 = 0;
    m3c = 0;
    m48 = 0;
    m4c = -1;
    m54 = 0;
    m40 = 0;
    m44 = 0;
    return this;
}
// @ 0x435440
int ftoi_helper(int x)
{
    int r = -1;
    switch (x) {
    case 0: return 4;
    case 1: return 5;
    case 2: return 6;
    case 3: return 7;
    case 4: r = 3;
    }
    return r;
}
