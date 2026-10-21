// Slice s00462540: sound-slot scheduler (per-slot state machine driven by the 1e-5 s clock table), the tail of the
// envelope parameter setters, and item/equipment helpers of the Agent record: mesh names by class and modifier
// bit, modifier-adjusted item stats, an item-to-skill-slot mapping and the "may this item go into slot N" tests.
// Names are provisional. Flags: default /O2 /MT /Gy /TP.
//
// Codegen levers found here: the snd_slot state/clock-index fields behave as volatile (the original re-reads them
// and never CSEs g_time[clk]); a static helper called with `&item` as an out-parameter gets cl's register convention
// when its formals are ordered (item, f58_idx, nib_idx, stat_idx); `float secs = r * gap * c1; (int)(secs * c2)`
// keeps two multiplies where one expression would be reassociated.
#include <stdlib.h>
#include "rgl.h"

#define MB_NOTHROW throw()
#define MB_NOINLINE __declspec(noinline)    // (tools/difftest/slice.py reads the name before a '__declspec(' as the function name)

// ---- strings -----------------------------------------------------------------------------------
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() MB_NOTHROW;                           // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;              // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;           // 0x42c110
    ~tstring() MB_NOTHROW;                          // 0x42bc40
    void operator=(const tstring& o) MB_NOTHROW;    // 0x42bc50
};

// ---- globals -----------------------------------------------------------------------------------
extern __int64 g_time[];                             // 0x5e2948 clock table (1e-5 s ticks)
int __cdecl round_float(float* f);          // 0x432540

// ---- envelope / fade parameters (first half is in slice s00461bd0) -----------------------------------
struct envelope {
    char pad00[0xe8];
    char dirty;                     // 0xe8
    char pade9[7];
    float f0;                       // 0xf0
    float time;                     // 0xf4
    int f8;                         // 0xf8
    int fc;                         // 0xfc
    float smooth;                   // 0x100
    void set_params(float a, float t, const int* b, const int* c);      // 0x462540
};

// @ 0x462540
void envelope::set_params(float a, float t, const int* b, const int* c)
{
    f0 = a;
    dirty = 1;
    time = t;
    f8 = *b;
    fc = *c;
    float s = (1.0f / t) * 3.0f;
    if (s < 0.001f) smooth = 0.001f;
    else if (s >= 0.95f) smooth = 0.95f;
    else smooth = s;
}

// ---- sound slot scheduler ---------------------------------------------------------------------------
struct snd_channel {                // 0x1c bytes
    char pad[0x1c];
    bool play(int arg);             // 0x4875e0 (thiscall, ret 4)
};
struct snd_slot {                   // 0x78 bytes
    float min_gap;                  // 0x00  seconds between plays (<= -0.1: follow the caller's request)
    float play_time;                // 0x04  seconds the first channel runs before the second starts
    float hold_time;                // 0x08  seconds the second channel runs
    volatile int state;             // 0x0c  0 idle, 1 first channel playing, 2 second channel playing
    snd_channel first;              // 0x10
    snd_channel second;             // 0x2c
    volatile int clk_gap;           // 0x48
    char pad4c[4];
    __int64 t_gap;                  // 0x50
    volatile int clk_play;          // 0x58
    char pad5c[4];
    __int64 t_play;                 // 0x60
    volatile int clk_hold;          // 0x68
    char pad6c[4];
    __int64 t_hold;                 // 0x70
    void update(int arg);           // 0x4625d0
};
struct snd_bank {
    int count;                      // 0
    snd_slot* slots;                // 4
    void update_all(int arg);       // 0x462760
    void randomize();               // 0x462790
};

// @ 0x4625d0
void snd_slot::update(int arg)
{
    if (state == 2) {
        if (hold_time <= (float)(g_time[clk_hold] - t_hold) * 1e-5f) {
            state = 0;
            t_gap = g_time[clk_gap];
        }
    }
    if (state == 0) {
        if (min_gap <= -0.1f) {
            if (round_float(&min_gap) == arg && first.play(arg)) {
                second.play(arg);
                t_hold = g_time[clk_hold];
                state = 2;
                return;
            }
        } else if (arg >= 0) {
            if ((float)(g_time[clk_gap] - t_gap) * 1e-5f < min_gap) return;
            t_gap = g_time[clk_gap];
            if (first.play(arg)) {
                t_play = g_time[clk_play];
                state = 1;
            }
        }
    }
    if (state != 1) return;
    if (!(play_time <= (float)(g_time[clk_play] - t_play) * 1e-5f)) return;
    second.play(arg);
    t_hold = g_time[clk_hold];
    state = 2;
}

// @ 0x462760
void snd_bank::update_all(int arg)
{
    for (int i = 0; i < count; i++)
        slots[i].update(arg);
}

// @ 0x462790
void snd_bank::randomize()
{
    for (int i = 0; i < count; i++) {
        snd_slot* s = &slots[i];
        int r = rand() % 15817;
        float secs = (float)r * s->min_gap * 6.32231167e-05f;
        s->t_gap = g_time[s->clk_gap] - (int)(secs * 100000.0f);
    }
}

// ---- items and troops ---------------------------------------------------------------------------
enum {
    itp_type_horse = 1, itp_type_one_handed_wpn, itp_type_two_handed_wpn, itp_type_polearm, itp_type_arrows,
    itp_type_bolts, itp_type_shield, itp_type_bow, itp_type_crossbow, itp_type_thrown, itp_type_goods,
    itp_type_head_armor, itp_type_body_armor, itp_type_foot_armor, itp_type_hand_armor, itp_type_pistol,
    itp_type_musket, itp_type_bullets, itp_type_animal, itp_type_book
};
enum {
    imod_plain = 0, imod_cracked, imod_rusty, imod_bent, imod_chipped, imod_battered, imod_poor, imod_crude, imod_old,
    imod_cheap, imod_fine, imod_well_made, imod_sharp, imod_balanced, imod_tempered, imod_deadly, imod_exquisite,
    imod_masterwork, imod_heavy, imod_strong, imod_powerful, imod_tattered, imod_ragged, imod_rough, imod_sturdy,
    imod_thick, imod_hardened, imod_reinforced, imod_superb, imod_lordly
};
#define itp_food 0x00080000

struct ItemMesh { tstring name; unsigned __int64 flags; };      // 0x18
struct ItemKind {                           // 0x238 bytes (array at 0x5e29d0)
    char pad0[0x44];
    int mesh_count;                         // +0x44
    ItemMesh meshes[1];                     // +0x48
    char pad60[0x1c8 - 0x60];
    unsigned type_flags;                    // +0x1c8 (low byte = itp_type_*)
    char pad1cc[0x1fc - 0x1cc];
    int f1fc;                               // +0x1fc
    int f200;                               // +0x200
    int f204;                               // +0x204
    char pad208[0x238 - 0x208];
    tstring mesh_name(unsigned __int64 cls, int bit);   // 0x462820
    bool is_ranged();                       // 0x433d90
    bool is_melee();                        // 0x433dc0
};
extern ItemKind* g_items;                   // 0x5e29d0
extern int g_item_count;                    // 0x5e29d4

// @ 0x462820
tstring ItemKind::mesh_name(unsigned __int64 cls, int bit)
{
    tstring name;
    unsigned __int64 mask = 1i64 << bit;
    bit = -1;
    int i = 0;
    if (0 < mesh_count) {
        while (i < mesh_count) {
            unsigned __int64 c = meshes[i].flags & 0xF000000000000000ui64;
            if (c != 0 && c == cls) {
                if (bit == -1) bit = i;
                if (meshes[i].flags & mask) bit = i;
            }
            i++;
        }
        if (bit != -1) name = meshes[bit].name;
    }
    return name;
}

// reference to an item with modifier: item index, amount (low byte of m4) and modifier (byte 7)
struct ItemRef {
    int item;
    union {
        unsigned m4;
        struct { unsigned char amount; unsigned char pad[2]; signed char mod; } b;
    };
    tstring mesh_name(unsigned __int64 cls);    // 0x462900
    int stat_1fc();                             // 0x462960
    int stat_204();                             // 0x462a10
    int stat_1ec();                             // 0x434c50
};

// @ 0x462900
tstring ItemRef::mesh_name(unsigned __int64 cls)
{
    if (item >= 0 && item < g_item_count)
        return g_items[item].mesh_name(cls, b.mod);
    return tstring("");
}

// @ 0x462960
int ItemRef::stat_1fc()
{
    if (item < 0) return 0;
    int v = g_items[item].f1fc;
    if (v != 0) {
        switch (b.mod) {
        case imod_rusty:
        case imod_tattered: v -= 3; break;
        case imod_crude: v -= 1; break;
        case imod_sturdy: v += 1; break;
        case imod_thick: v += 2; break;
        case imod_heavy:
        case imod_hardened: v += 3; break;
        case imod_cracked: v -= 4; break;
        case imod_battered:
        case imod_ragged: v -= 2; break;
        case imod_reinforced: v += 4; break;
        case imod_lordly: v += 6; break;
        }
        if (v < 0) v = 0;
    }
    return v;
}

// @ 0x462a10
int ItemRef::stat_204()
{
    if (item < 0) return 0;
    int v = g_items[item].f204;
    if (v != 0) {
        switch (b.mod) {
        case imod_rusty:
        case imod_tattered: v -= 3; break;
        case imod_crude: v -= 1; break;
        case imod_sturdy: v += 1; break;
        case imod_thick: v += 2; break;
        case imod_heavy:
        case imod_hardened: v += 3; break;
        case imod_cracked: v -= 4; break;
        case imod_battered:
        case imod_ragged: v -= 2; break;
        case imod_reinforced: v += 4; break;
        case imod_lordly: v += 6; break;
        }
        if (v < 0) v = 0;
    }
    return v;
}

struct Agent {
    char pad0[0x30];
    unsigned __int64 flags30;               // +0x30
    char pad38[0x58 - 0x38];
    int f58[2];                             // +0x58
    char pad60[0x80 - 0x60];
    float stat[8];                          // +0x80
    char padA0[0xb4 - 0xa0];
    ItemRef refs[1];                        // +0xb4
    int nibble(int i);                      // 0x4351a0
    int slot_value(int item);               // 0x462c00
    int tier_base();                        // 0x462b60
    bool fits_slot(ItemRef* ref, int slot); // 0x462c80
    bool item_matches(int idx, int slot, unsigned need, unsigned forbid, int t1, int t2, int t3);   // 0x462da0
};

// @ 0x462b60
int Agent::tier_base()
{
    if (!(flags30 & 0x10)) return 0x5a;
    switch (nibble(12)) {
    case 0: return 0x1e;
    case 1: return 0x24;
    case 2: return 0x2a;
    case 3: return 0x30;
    case 4: return 0x36;
    case 5: return 0x3c;
    case 6: return 0x42;
    case 7: return 0x48;
    case 8: return 0x4e;
    case 9: return 0x54;
    case 10: return 0x60;
    default: return 0x60;
    }
}

// register convention in the original: item in eax, outputs in ecx / edx / stack
// @ 0x462ac0
static MB_NOINLINE void item_slot_source(int item, int* f58_idx, int* nib_idx, int* stat_idx)
{
    *f58_idx = -1;
    *nib_idx = -1;
    *stat_idx = -1;
    if (item >= 0) {
        ItemKind* it = &g_items[item];
        unsigned t = *(volatile unsigned*)&it->type_flags & 0xff;     // (volatile read: keeps the original's add + load pair)
        if (t != itp_type_one_handed_wpn && t != itp_type_two_handed_wpn && t != itp_type_polearm && t != itp_type_crossbow) {
            if (t == itp_type_horse) { *nib_idx = 0x18; return; }
            if (t == itp_type_bow) { *nib_idx = 0x21; return; }
            if (t == itp_type_thrown) { *nib_idx = 0x22; return; }
            if (t == itp_type_shield) { *nib_idx = 0x1a; return; }
            if (t != itp_type_head_armor && t != itp_type_body_armor && t != itp_type_foot_armor && t != itp_type_hand_armor)
                return;
        }
        *f58_idx = 0;
    }
}

// @ 0x462c00
int Agent::slot_value(int item)
{
    int result = 0;
    int nib_idx, stat_idx;
    item_slot_source(item, &item, &nib_idx, &stat_idx);
    if (item >= 0) result = f58[item];
    else if (nib_idx >= 0) result = nibble(nib_idx);
    else if (stat_idx >= 0) result = (int)stat[stat_idx];
    return result;
}

// @ 0x462c80
bool Agent::fits_slot(ItemRef* ref, int slot)
{
    int item = ref->item;
    unsigned tf = g_items[item].type_flags;
    ItemKind* it = &g_items[item];
    unsigned t = tf & 0xff;
    bool ok = false;
    if (flags30 & 0x10) {
        int need = slot_value(item);
        if (ref->stat_1ec() > need) { ok = false; goto done; }
    }
    if (slot < 0 || slot > 3) {
        if (slot == 4) { if (t != itp_type_head_armor) goto done; }
        else if (slot == 5) { if (t != itp_type_body_armor) goto done; }
        else if (slot == 6) { if (t != itp_type_foot_armor) goto done; }
        else if (slot == 7) { if (t != itp_type_hand_armor) goto done; }
        else {
            if (slot != 8) {
                if (slot != 9) goto done;
                if (t == itp_type_goods) {
                    if (tf & itp_food) { ok = true; goto done; }
                    goto done;
                }
            }
            if (t != itp_type_horse) goto done;
        }
    } else if (t != itp_type_bolts && t != itp_type_arrows && t != itp_type_bullets) {
        if (!it->is_ranged() && t != itp_type_shield) {
            if (it->is_melee()) { ok = true; goto done; }
            goto done;
        }
    }
    ok = true;
done:
    return ok;
}

// @ 0x462da0
bool Agent::item_matches(int idx, int slot, unsigned need, unsigned forbid, int t1, int t2, int t3)
{
    ItemKind* it = &g_items[refs[idx].item];
    if (fits_slot(&refs[idx], slot)) {
        unsigned tf = it->type_flags;
        if ((tf & need) == need && !(forbid & tf)) {
            if (t1 == -1 || (int)(tf & 0xff) == t1 || (int)(tf & 0xff) == t2 || (int)(tf & 0xff) == t3)
                return true;
        }
    }
    return false;
}
