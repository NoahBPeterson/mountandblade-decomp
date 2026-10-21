// Slice s00445a90: Agent::refill_ammo (tops up the stacks of the held ranged weapon from the matching ammo items and,
// for thrown weapons that ran out, switches to the next weapon) and std::vector<E56>::_Insert_n with its helpers
// _Ufill (0x445e70) and the two catch funclets 0x44608f / 0x44614e (E56 = 0x38-byte record holding two vector<int>,
// see s004417d0 / s0043edf0 / s00440a30). All names provisional; layouts from s004362e0, s00437f00, s00434ad0.
#include <vector>
#include <memory>
#include <new>
#include <math.h>

struct E56 {
    int a, b, c;
    std::vector<int> v1;       // +0x0c
    std::vector<int> v2;       // +0x1c
    int d, e, f;
    E56(const E56& o)
        : a(o.a), b(o.b), c(o.c), v1(o.v1), v2(o.v2), d(o.d), e(o.e), f(o.f)
    {
    }
};

namespace std {
// out-of-TU helpers (other slices)
template<> E56* _Uninit_copy<E56*, E56*, allocator<E56> >(E56*, E56*, E56*, allocator<E56>&, _Nonscalar_ptr_iterator_tag);        // 0x442080
template<> void _Uninit_fill_n<E56*, unsigned int, E56, allocator<E56> >(E56*, unsigned int, const E56&, allocator<E56>&, _Nonscalar_ptr_iterator_tag);   // 0x444bc0
template<> void _Destroy_range<E56, allocator<E56> >(E56*, E56*, allocator<E56>&, _Nonscalar_ptr_iterator_tag);                   // 0x43e6e0
template<> void fill<E56*, E56>(E56*, E56*, const E56&);                                                                               // 0x440dc0
template<> E56* _Copy_backward_opt<E56*, E56*>(E56*, E56*, E56*, _Nonscalar_ptr_iterator_tag);                                       // 0x440a30
}

// 0x445700 (s004452f0): vector<E56>::_Ucopy<E56*>; instantiated here too so that the call stays a call
template std::vector<E56>::pointer std::vector<E56>::_Ucopy<E56*>(E56*, E56*, E56*);
// @ 0x445e70
template std::vector<E56>::pointer std::vector<E56>::_Ufill(E56*, size_t, const E56&);
// @ 0x445ea0 (the whole COMDAT is 0x33e bytes: 0x445ea0..0x4461dd, including the two catch(...) funclets below)
template void std::vector<E56>::_Insert_n(std::vector<E56>::iterator, size_t, const E56&);
// @ 0x44608f
// (catch(...) funclet of the _Insert_n above: _Destroy(_Newvec, _Ptr), deallocate, `throw;`)
// @ 0x44614e
// (catch(...) funclet of the _Insert_n above: _Destroy of the spilled suffix copy, `throw;`; the common tail 0x4461ad
//  destroys the two vectors of the local `_Tmp` copy)

// ---- Agent: refill the ammunition of the held ranged weapon ----------------------------------------------------
// (names provisional; layouts from s004362e0 / s00437f00 / s00434ad0)
struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                        // +0x1c8 (low byte = itp_type_*)
    char pad1cc[0x210 - 0x1cc];
    int f210;                                   // +0x210 (stack size)
    char pad214[0x238 - 0x214];
};
extern ItemKind* g_items;                       // 0x5e29d0

// 0x432540 (inlined here)
static inline int round_float(const float* p)
{
    float fl = (float)floor((double)*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}

// item index + packed amount word (amount = low byte, 0x2a in the top byte = "sized" modifier)
struct Pair {
    int a;
    unsigned b;
    int max_amount();                           // 0x434b70
    void add_amount(int n);                     // 0x434be0
    int room_left();                            // 0x434c30
};
int Pair::max_amount()
{
    if (a < 0) return 0;
    int v = g_items[a].f210;
    if (v != 0 && (b & 0xff000000) == 0x2a000000) {
        float f = v * 0.13f;
        int d = round_float(&f);
        if (d < 1) d = 1;
        v += d;
    }
    return v;
}

void Pair::add_amount(int n)
{
    int v = (b & 0xff) + n;
    if (v > max_amount())
        v = max_amount();
    ((unsigned char*)&b)[0] = 0;
    b = b | v;
}
int Pair::room_left()
{
    return max_amount() - (int)(b & 0xff);
}

static inline unsigned itype(const Pair* p) { return g_items[p->a].type_flags & 0xff; }

struct OrderSlot {                              // 0x2c bytes at agent+0xd4
    int a, b;
    char pad8[0x20 - 8];
    float rate;                                 // +0x20
    char pad24[0x2c - 0x24];
    __declspec(noinline) bool set(int id, unsigned prio);   // 0x4378a0
};
struct DefList { void* defs; int count; };
struct SlotLink { char pad[0xcc]; int m_cc; };

struct Agent {
    char pad0[0xd4];
    OrderSlot oslots[2];                        // +0xd4
    DefList* defs;                              // +0x12c
    char pad130[0x14c - 0x130];
    SlotLink* links[4];                         // +0x14c
    char pad15c[0x1d0 - 0x15c];
    Pair held[11];                              // +0x1d0
    int pad228;
    int slots[5];                               // +0x22c (slots[0] = current held item)
    char pad240[0x24c - 0x240];
    float f24c;                                 // +0x24c
    char pad250[0x55f0 - 0x250];
    int f55f0;                                  // +0x55f0
    char pad55f4[0x5928 - 0x55f4];

    float equipment_value() const;              // 0x436050
    Pair best_ammo(int idx, int* missing, int* total, int* slot_out);   // 0x436880
    void refresh_weapon_state();                // 0x445a20
    float damage_factor(int idx);               // 0x4385e0
    int refill_ammo(int count);                 // 0x445a90
};

// @ 0x445a90
int Agent::refill_ammo(int count)
{
    Pair* cur = 0;
    if (slots[0] >= 0)
        cur = &held[slots[0]];
    int added = 0;
    if (cur != 0) {
        unsigned wt = g_items[cur->a].type_flags & 0xff;
        if (wt == 8 || wt == 9 || wt == 0x10 || wt == 0x11 || wt == 10) {
            bool done = false;
            for (int i = 0; i < 4 && !done; i++) {
                Pair* s = &held[i];
                if (s->a >= 0) {
                    if ((itype(s) == 5 && itype(cur) == 8) ||
                        (itype(s) == 6 && itype(cur) == 9) ||
                        (itype(s) == 0x12 && itype(cur) == 0x10) ||
                        (itype(s) == 0x12 && itype(cur) == 0x11) ||
                        (itype(cur) == 10 && s->a == cur->a)) {
                        while (added < count) {
                            if (s->room_left() <= 0)
                                break;
                            s->add_amount(1);
                            added++;
                        }
                        if (added >= count)
                            done = true;
                    }
                }
            }
            f24c = equipment_value();
            if ((char)g_items[cur->a].type_flags == 10) {
                int missing, total, slot;
                best_ammo(-1, &missing, &total, &slot);
                if (missing <= 0) {
                    slots[0] = -1;
                    refresh_weapon_state();
                    float rate = damage_factor(0);
                    int anim = f55f0;
                    if (anim >= -1 && anim < defs->count && oslots[1].set(anim, 0x11400020)) {
                        if (rate < 0.1f)
                            rate = 0.1f;
                        else if (rate >= 10.0f)
                            rate = 10.0f;
                        oslots[1].rate = rate;
                        if (slots[0] >= 0 && held[slots[0]].a >= 0)
                            links[slots[0]]->m_cc = 0;
                    }
                }
            }
        }
    }
    return added;
}
