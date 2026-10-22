// Slice s004735e0: face generator helpers. 0x4735e0 relaxes a set of linear constraints over the morph values (each
// constraint: sum(coef * v[idx]) must stay on one side of a limit; the free values are nudged and clamped to [0,1]);
// 0x473800 unpacks a 3 x 64-bit face key into a face record (hair/beard/texture picks, age, weight, morph values);
// 0x473980 refreshes a per-cell value over the whole terrain grid; 0x4739f0 clears the "spawned" counters of a party's
// stacks. All names provisional.
#include <windows.h>
#include <math.h>
#include <vector>

// ---- 0x4735e0 --------------------------------------------------------------------------------------------------------
struct Term { float coef; int idx; };           // 8 bytes
struct Constraint {                             // 0x204 bytes
    int n;                                      // +0x000 number of terms
    Term t[63];                                 // +0x004
    int mode;                                   // +0x1fc -1: value must stay >= limit, +1: must stay <= limit
    float limit;                                // +0x200
};
struct MorphSolver {
    char pad0[0x78];
    int alloc;                                  // +0x78 (std::vector allocator slot)
    Constraint* first;                          // +0x7c
    Constraint* last;                           // +0x80
    int size() const { return first == 0 ? 0 : (int)(last - first); }   // vector<Constraint>::size (cf. 0x467b20)
    void relax(int skip, float* v);             // 0x4735e0
};

// @ 0x4735e0
void MorphSolver::relax(int skip, float* v)
{
    for (unsigned i = 0; i < (unsigned)size(); i++) {
        bool done = false;
        for (int iter = 0; !done; iter++) {
            Constraint* c = &first[i];
            float sum = 0.0f;
            int free_n = 0;
            for (int j = 0; j < c->n; j++) {
                sum += v[c->t[j].idx] * c->t[j].coef;
                if (c->t[j].idx != skip)
                    free_n++;
            }
            if ((c->mode == -1 && sum < c->limit) || (c->mode == 1 && c->limit < sum)) {
                float d = c->limit - sum;
                float cnt = (float)free_n;
                if (!(cnt > 1.0f))
                    cnt = 1.0f;
                float step = d / cnt;
                for (int j = 0; j < c->n; j++) {
                    if (c->t[j].idx != skip) {
                        v[c->t[j].idx] += step / c->t[j].coef;
                        if (v[c->t[j].idx] < 0.0f)
                            v[c->t[j].idx] = 0.0f;
                        else if (v[c->t[j].idx] > 1.0f)
                            v[c->t[j].idx] = 1.0f;
                    }
                }
                if (iter > 30)
                    done = true;
            } else {
                done = true;
            }
        }
    }
}

// ---- 0x473800 --------------------------------------------------------------------------------------------------------
struct FaceRace {                               // 0x1f8 bytes, table at FaceGen+0x1c
    char pad0[0x3c];
    int nmorph;                                 // +0x3c
    char pad40[0x88 - 0x40];
    int n_a;                                    // +0x88
    char pad8c[0x18c - 0x8c];
    int n_b;                                    // +0x18c
    int n_c;                                    // +0x190
    int n_d;                                    // +0x194
    char pad198[0x1f8 - 0x198];
};
struct FaceGen {
    int pick_a;                                 // +0x00 (key bits 6..11, minus 1)
    int pick_b;                                 // +0x04 (key bits 0..5, minus 1)
    int race;                                   // +0x08
    int pick_d;                                 // +0x0c (bits 18..23)
    int pad10;
    int pick_c;                                 // +0x14 (bits 12..17)
    char pad18[4];
    FaceRace* races;                            // +0x1c
    char pad20[0x2c - 0x20];
    float morph[63];                            // +0x2c
    float weight;                               // +0x128
    float age;                                  // +0x12c
    FaceRace* unpack(const unsigned __int64* key);   // 0x473800 (returns the race record, left in EAX)
};
extern unsigned g_mask64;                       // 0x5a8e98 (64)
extern unsigned g_mask8;                        // 0x5a8e80 (8)

// @ 0x473800
FaceRace* FaceGen::unpack(const unsigned __int64* key)
{
    pick_b = ((unsigned)key[0] & (g_mask64 - 1)) - 1;
    pick_a = ((unsigned)(key[0] >> 6) & (g_mask64 - 1)) - 1;
    pick_c = (unsigned)(key[0] >> 12) & (g_mask64 - 1);
    pick_d = (unsigned)(key[0] >> 18) & (g_mask64 - 1);
    FaceRace* r = &races[race];
    if (pick_b >= r->n_b)
        pick_b = -1;
    if (pick_a >= r->n_a)
        pick_a = -1;
    if (pick_c >= r->n_c)
        pick_c = 0;
    if (pick_d >= r->n_d)
        pick_d = 0;
    age = (float)(int)((unsigned)(key[0] >> 24) & (g_mask64 - 1)) * 0.015873017f;
    weight = (float)(int)((unsigned)(key[0] >> 30) & (g_mask64 - 1)) * 0.015873017f;
    int i = 0;
    for (r = &races[race]; i < r->nmorph; r = &races[race]) {
        int w = 1;
        int sh = i * 3;
        if (sh >= 63) {
            w = 2;
            sh -= 63;
        }
        morph[i] = (float)(int)((unsigned)(key[w] >> sh) & (g_mask8 - 1)) * 0.14285715f;
        i++;
    }
    return r;
}

// ---- 0x473980 --------------------------------------------------------------------------------------------------------
struct cell94 {                                 // 0x94-byte terrain cell
    char pad0[0x40];
    float f40;                                  // +0x40
    char pad44[0x94 - 0x44];
};
struct terrain_obj2 {
    char pad0[0x278];
    int w;                                      // +0x278
    int h;                                      // +0x27c
    char pad280[8];
    cell94 cells[251 * 251];                    // +0x288, index x * 251 + y
    float cell_value(cell94* c);                // 0x471a30 (thiscall, ret 4)
    void refresh_cells();                       // 0x473980
};

// @ 0x473980
void terrain_obj2::refresh_cells()
{
    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            cell94* c = &cells[x * 251 + y];
            c->f40 = cell_value(c);
        }
    }
}

// ---- 0x4739f0 --------------------------------------------------------------------------------------------------------
struct Stack2 {                                 // 0x20 bytes
    int count, troop, flags, spawned, wounded, pad14[3];
};
struct Party2 {                                 // 0x5ac8 bytes
    char pad0[0xa8];
    Stack2 stacks[64];                          // +0xa8
    int nstacks;                                // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
};
struct PartyMgr2 {
    char pad0[0x138cc];
    std::vector<Party2*> pages;                 // +0x138cc (first at +0x138d0)
    int index_of(int id);                       // 0x48ad00 (thiscall, ret 4)
};
struct GameData2 {
    char pad0[0xf8];
    PartyMgr2 parties;                          // +0xf8
};
extern GameData2* g_game2;                      // 0x5e2930

// @ 0x4739f0
void __stdcall reset_spawned(int party_id)
{
    if (party_id != -1) {
        PartyMgr2* pm = &g_game2->parties;
        int i = pm->index_of(party_id);
        Party2* p = &pm->pages[i >> 4][i & 15];
        for (int k = 0; k < p->nstacks; k++)
            p->stacks[k].spawned = 0;
    }
}
