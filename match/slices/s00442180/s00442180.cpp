// Slice s00442180: the world's "entities fully touching a box" query, FxElem / FxDef list appends, the effect
// emitter setup from a definition table (0x4424c0), the FMOD sound player (0x442650) and its by-name wrapper.
// All names provisional. Layouts reused from s004260a0 (dyn.h: world / cell / enode / Obj slots), s004291c0
// (FxElem / FxDef), s00427900 (Emitter) and s004333c0 (Obj3::set_placement).
#include <stddef.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// ---- world grid (see s004260a0 dyn.h) -----------------------------------------------------------------------------
struct Obj {                                            // entity base: virtual slots, 13 = max, 14 = min
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9();
    virtual void s10(); virtual void s11(); virtual void s12();
    virtual const vector4* get_max();                   // 13 (+0x34)
    virtual const vector4* get_min();                   // 14 (+0x38)
};
struct enode { enode* next; int pad4; Obj* ent; };
struct cell {
    char pad0[0x30];
    enode* ents;                                        // +0x30
};                                                      // 0x34 bytes
static __forceinline bool box3_inl(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y && a->z <= d->z && c->z <= b->z)
        return true;
    return false;
}
typedef std::vector<unsigned>::iterator UIt;
void __cdecl sort_range(UIt first, UIt last, int ideal);        // 0x426a30 (std::_Sort, s00426a30)

struct world {
    char pad0[0x28];
    cell** cells;                                       // +0x28
    void locate(const vector4* p, int* out) const;      // 0x41ed00
    void collect_boxed(const vector4* bmin, const vector4* bmax, std::vector<unsigned>* out);   // 0x442180
};

// @ 0x442180
void world::collect_boxed(const vector4* bmin, const vector4* bmax, std::vector<unsigned>* out)
{
    if (out->_Myfirst != 0)
        operator delete(out->_Myfirst);
    out->_Myfirst = 0;
    out->_Mylast = 0;
    out->_Myend = 0;
    int lo[2];
    volatile int hi[2];                     // read from memory at every test (that is what the original compiled to)
    int c[2];
    locate(bmin, lo);
    locate(bmax, (int*)hi);
    for (c[0] = lo[0]; c[0] <= hi[0]; c[0]++) {
        for (c[1] = lo[1]; c[1] <= hi[1]; c[1]++) {
            for (enode* nd = cells[c[0]][c[1]].ents; nd != 0; nd = nd->next) {
                Obj** pe = &nd->ent;
                if (box3_inl(bmin, bmax, (*pe)->get_min(), (*pe)->get_max()))
                    out->push_back(*(unsigned*)pe);
            }
        }
    }
    sort_range(out->begin(), out->end(), (int)(out->end() - out->begin()));
    std::vector<unsigned>::iterator e = std::unique(out->begin(), out->end());
    out->erase(e, out->end());
}

// 0x440b30 vector<int>::push_back is kept out of line
template<> void std::vector<int>::push_back(const int&);

// ---- effect elements / definitions (FxElem, FxDef of s004291c0 / s00421aa0) ---------------------------------------
template<class T> __forceinline void vpush(std::vector<T>& v, const T& x)
{
    if (v.size() < v.capacity()) {
        T* p = v._Mylast;
        *p = x;
        v._Mylast = p + 1;
    } else {
        v.insert(v.end(), x);
    }
}

struct mat4 {                                           // 64-byte frame (rows s, f, u, o)
    vector4 s, f, u, o;
    mat4 operator*(const mat4& b) const;                // 0x40e870 (this = left operand; hidden return pointer)
};
struct Psys {                                           // particle system (provisional): frame at +0xcc
    char pad0[0xcc];
    mat4 frame;
};
struct PsysGroup;
struct FxElem {                                         // 0x1dc bytes
    char pad0[0x19c];
    std::vector<int> psys;                              // +0x19c (first at +0x1a0); holds Psys*
    int ngroups;                                        // +0x1ac
    std::vector<int> groups;                            // +0x1b0 (first at +0x1b4); holds PsysGroup*
    char pad1c0[0x1dc - 0x1c0];
    void add_psys(int p);                               // 0x442390
    void add_group(int g);                              // 0x4423f0
};
struct FxDef {
    int count;                                          // +0
    FxElem* elems;                                      // +4
    void attach_psys(int idx, Psys* p, const mat4* m);  // 0x442460
    void attach_group(int idx, PsysGroup* g);           // 0x4424a0
};
// @ 0x442390
void FxElem::add_psys(int p) { vpush(psys, p); }
// @ 0x4423f0
void FxElem::add_group(int g)
{
    vpush(groups, g);
    ngroups++;
}
// @ 0x442460
void FxDef::attach_psys(int idx, Psys* p, const mat4* m)
{
    p->frame = *m;
    elems[idx].add_psys((int)p);
}
// @ 0x4424a0
void FxDef::attach_group(int idx, PsysGroup* g)
{
    elems[idx].add_group((int)g);
}

// ---- emitter setup from a definition table (0x4424c0) ---------------------------------------------------------------
struct tstring { int pos; char* buf; int cap; int len; };
struct ResObj {                                         // resource returned by resmgr::get_object (provisional)
    char pad0[0x1c];
    unsigned flags;                                     // +0x1c (0x40: follows the camera)
    char pad20[0x2c - 0x20];
    int f2c;                                            // +0x2c
};
struct resmgr { ResObj* get_object(const tstring* name); };   // 0x419540
extern resmgr g_resmgr;                                 // 0x5d6fa0
struct P8 { int a, b; };
struct EmitDef {                                        // 0xc0-byte definition record
    char pad0[0x10];
    int f10;                                            // +0x10
    tstring name;                                       // +0x14
    int f24;                                            // +0x24
    char pad28[0x30 - 0x28];
    int f30, f34, f38, f3c, f40;                        // +0x30
    P8 t[5][2];                                         // +0x44 (rows of 0x10)
    vector4 a;                                          // +0x94
    vector4 b;                                          // +0xa4
    int c;                                              // +0xb4
    int fb8, fbc;                                       // +0xb8
};
struct Emit {                                           // particle emitter (0x1bc bytes; Emitter of s00427900)
    char pad0[4];
    int f4;                                             // +4
    ResObj* obj;                                        // +8
    char padc[0x1c - 0xc];
    int f1c, f20, f24;                                  // +0x1c
    P8 t[5][2];                                         // +0x28
    char pad78[0xa0 - 0x78];
    int fa0;                                            // +0xa0
    char pada4[0xc4 - 0xa4];
    vector4 gravity;                                    // +0xc4
    int fd4;                                            // +0xd4
    vector4 offs;                                       // +0xd8
    unsigned flags;                                     // +0xe8
    char padec[0xf4 - 0xec];
    mat4 world;                                         // +0xf4
    char pad134[0x198 - 0x134];
    int f198;                                           // +0x198
    int f19c, f1a0, f1a4;                               // +0x19c
    int idx;                                            // +0x1a8
    char pad1ac[4];
    int f1b0, f1b4;                                     // +0x1b0
    char pad1b8[4];
    void init();                                        // 0x420b40
    void set_placement(const mat4* m, const vector4* a, const vector4* b, int c);   // 0x433770
};
mat4* __cdecl identity_pos(mat4* out);                  // 0x40cd00 (cdecl; returns out)
struct EmitTable {
    EmitDef* defs;                                      // +0
    int count;                                          // +4
    void setup(Emit* e, int idx, const vector4* pos);   // 0x4424c0
};
extern EmitTable g_emit_defs;                           // 0x8ed5090

// @ 0x4424c0
void EmitTable::setup(Emit* e, int idx, const vector4* pos)
{
    if (idx < 0 || idx >= count)
        return;
    EmitDef* d = &defs[idx];
    ResObj* r = g_resmgr.get_object(&d->name);
    if (r == 0)
        return;
    e->init();
    e->idx = idx;
    e->obj = r;
    e->f4 = r->f2c;
    e->flags = d->f10;
    e->f198 = d->f24;
    e->fa0 = d->f30;
    e->f24 = d->f34;
    vector4 g(0.0f, 0.0f, -9.8f);
    e->gravity = g;
    e->fd4 = d->f38;
    e->f1c = d->f3c;
    e->f20 = d->f40;
    for (int k = 0; k < 2; k++) {
        e->t[0][k] = d->t[0][k];
        e->t[1][k] = d->t[1][k];
        e->t[2][k] = d->t[2][k];
        e->t[3][k] = d->t[3][k];
        e->t[4][k] = d->t[4][k];
    }
    e->f1b0 = d->fb8;
    e->f1b4 = d->fbc;
    mat4 m;
    identity_pos(&m);
    m.o = *pos;
    e->set_placement(&m, &d->a, &d->b, d->c);
}

// ---- FMOD 3 imports (stdcall thunks into the import table; names from the import library) ----------------------------
extern "C" {
int __stdcall FSOUND_PlaySoundEx(int channel, void* sample, void* dsp, int startpaused);        // 0x5342d6
int __stdcall FSOUND_3D_SetAttributes(int channel, const float* pos, const float* vel);         // 0x5342d0
int __stdcall FSOUND_SetVolume(int channel, int vol);                                           // 0x5342ca
int __stdcall FSOUND_SetLoopMode(int channel, unsigned mode);                                   // 0x5342c4
int __stdcall FSOUND_SetCurrentPosition(int channel, unsigned offset);                          // 0x5342be
int __stdcall FSOUND_SetFrequency(int channel, int freq);                                       // 0x5342b8
int __stdcall FSOUND_SetPriority(int channel, int priority);                                    // 0x5342b2
int __stdcall FSOUND_SetPaused(int channel, int paused);                                        // 0x5342ac
}
// small helpers seen from here (s00432130 / s00435490)
int __cdecl clamp_index(const int* v, const int* lo, const int* hi);    // 0x432510
int __cdecl round_float(const float* p);                                // 0x432540
int __cdecl rand_below(const int* n);                                   // 0x432580

struct SoundDef {                                       // 0x14 bytes
    void* sample;                                       // +0
    int f4;                                             // +4 (bits 4..7: distance range)
    int id;                                             // +8
    int freq;                                           // +0xc
    int length;                                         // +0x10
    float range_scale();                                // 0x4355f0 (fastcall/thiscall)
};
struct SoundGroup {                                     // 0x98 bytes
    char pad0[0x10];
    unsigned flags;                                     // +0x10 (2: looping, 4: random start, bits 8..11: volume)
    int var[32];                                  // +0x14 variations (def indices)
    int nvar;                                           // +0x94
    float volume_scale();                               // 0x435620
};
struct SoundMgr {
    char pad0[4];
    SoundDef* defs;                                     // +4
    int ngroups;                                        // +8
    SoundGroup* groups;                                 // +0xc
    char pad10[0x40 - 0x10];
    vector4 listener;                                   // +0x40
    int slot[(0x248 - 0x50) / 4];                       // +0x50 group index by sound id
    std::vector<int> channels;                          // +0x248 looping channels
    int play(int idx, int flags, const float* pos, float freq, int* out_id);   // 0x442650
    int play_slot(int n, int flags, const float* pos, float freq, int* out_id);   // 0x4428a0
};
extern int g_sound_off;                                 // 0x5a9fd8
extern int g_no_freq;                                   // 0x5a9fc8
extern float g_master_volume;                           // 0x5aa014

static __forceinline int round_i(float v) { return round_float(&v); }
// @ 0x442650
int SoundMgr::play(int idx, int flags, const float* pos, float freq, int* out_id)
{
    int ret = -1;
    if (g_sound_off == 0 && idx >= 0 && idx < ngroups) {
        int* pn = &groups[idx].nvar;
        int n = *pn;
        if (n > 0) {
            int k = 0;
            if (n > 1)
                k = rand() % *pn;
            int dn = groups[idx].var[k];
            int ch = FSOUND_PlaySoundEx(-1, defs[dn].sample, 0, 1);
            ret = ch;
            if (out_id)
                *out_id = defs[dn].id;
            if (ch >= 0) {
                int looping = groups[idx].flags & 2;
                int hi = 0x100;
                int vol = 0;
                int r = round_i(groups[idx].volume_scale() * g_master_volume * 255.0f);
                vol = clamp_index(&r, &vol, &hi);
                int prio;
                if (flags & 1) {
                    FSOUND_3D_SetAttributes(ch, 0, 0);
                    prio = 0xff;
                } else {
                    FSOUND_3D_SetAttributes(ch, pos, 0);
                    float dist = (listener - *(const vector4*)pos).out_length();
                    float f = 3.0f / (dist + 3.0f);
                    int lo = 0;
                    int hi2 = 0x100;
                    int pr = round_i(defs[dn].range_scale() * f * 254.0f);
                    prio = clamp_index(&pr, &lo, &hi2);
                }
                FSOUND_SetVolume(ch, vol);
                FSOUND_SetLoopMode(ch, (looping != 0) + 1);
                if (groups[idx].flags & 4)
                    FSOUND_SetCurrentPosition(ch, rand_below(&defs[dn].length));
                if (looping) {
                    int t = ch;
                    channels.push_back(t);
                }
                if (freq != 1.0f && g_no_freq == 0)
                    FSOUND_SetFrequency(ch, (int)((float)defs[dn].freq * freq));
                FSOUND_SetPriority(ch, prio);
                FSOUND_SetPaused(ch, 0);
            }
        }
    }
    return ret;
}
// @ 0x4428a0
int SoundMgr::play_slot(int n, int flags, const float* pos, float freq, int* out_id)
{
    n = slot[n];
    return play(n, flags, pos, freq, out_id);
}
