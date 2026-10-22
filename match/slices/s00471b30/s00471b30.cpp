// Slice s00471b30: sun lighting of the world-map particles (terrain_obj, see s00466aa0): for every particle of a list
// the 0x471b30 routine darkens the particle meshes' vertex colours by the sun term (and a ground-shadow ray test),
// 0x472020 loops over the list; plus the property-table accessors of the mission (0x472050) and a few container helpers.
// All names provisional; layouts shared with s00466aa0 (terrain_obj, particle) and s0044f3c0 (mesh).
#include <vector>
#include <list>
#include <stdlib.h>
#include <math.h>
#include "rgl.h"
#include "mesh.h"
using rgl::vector4;
using rgl::mesh;
using rgl::vertex60;
using rgl::point20;

extern int g_use_shaders;                       // 0x5e28fc: programmable pipeline in use
extern int g_x5aa084;                           // 0x5aa084: shader quality / mode

struct HCell { float h; unsigned flags; int id; int id2; };
struct HeightField {
    HCell** cols;                               // +0
    int line_clear(const vector4* a, const vector4* b);          // 0x436da0 (thiscall, ret 8; returns al)
};
struct SceneW {
    char pad0[0x64];
    HeightField* hf;                            // +0x64
    int trace(const vector4* from, const vector4* to, float maxlen, int a, void** hit);   // 0x439d60 (thiscall, ret 0x14)
};
struct ptype {                                  // 0x20, table at *0x5e28f8
    char pad0[0x10];
    __int64 flags;                              // +0x10
    int nframes;                                // +0x18
    struct AnimBox* boxes;                      // +0x1c: nframes boxes of 0x48 bytes
};
struct AnimBox {                                // 0x48
    char pad0[0x28];
    float mn[3];                                // +0x28
    float pad34;
    float mx[3];                                // +0x38
    char pad44[0x48 - 0x44];
};
extern ptype* g_ptypes;                         // 0x5e28f8

struct particle {                               // 0xa0
    int type;                                   // 0x00
    int cx, cy;                                 // 0x04 / 0x08
    int f0c;                                    // 0x0c
    char pad10[0x14 - 0x10];
    rgl::matrix frame;                          // 0x14
    vector4 dir;                                // 0x54
    int anim;                                   // 0x64
    char pad68[0x6c - 0x68];
    mesh* meshes[4];                            // 0x6c
    int first[4];                               // 0x7c: vertex range [first, last) of each mesh
    int last[4];                                // 0x8c
    int nmesh;                                  // 0x9c
};
struct particle_list {
    int count;
    particle items[1];                          // really 0x40000
};

static inline float dot3(const vector4& a, const vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

static inline void vscale_to(vector4& r, const vector4& v, float k) { r.x = v.x * k; r.y = v.y * k; r.z = v.z * k; }
static inline vector4 vadd3(const vector4& a, const vector4& b) { vector4 r; r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; return r; }
static inline void vadd_to(vector4& r, const vector4& a, const vector4& b) { r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; }
static inline void vmul_to(vector4& r, const vector4& a, const vector4& b) { r.x = a.x * b.x; r.y = a.y * b.y; r.z = a.z * b.z; }
static inline float max2(float a, float b) { return a > b ? a : b; }

struct Probe {
    bool test(unsigned p, float* out);          // 0x468c80
};
extern Probe g_probe;                           // 0xa70248

struct Terrain {
    char pad0[0xc3b0dc];
    SceneW* world;                              // 0xc3b0dc
    char pad_c3b0e0[0x843bd0c - 0xc3b0e0];
    vector4 sun;                                // 0x843bd0c (direction of the light)
    char pad_843bd1c[0x843bd2c - 0x843bd1c];
    vector4 sun_amb;                            // 0x843bd2c
    vector4 sun_dif;                            // 0x843bd3c
    void light_particle(particle_list* l, int idx, bool trace);   // 0x471b30
    void light_all(particle_list* l, bool trace);                 // 0x472020
};

// @ 0x471b30
void Terrain::light_particle(particle_list* l, int idx, bool trace)
{
    particle* p = &l->items[idx];
    ptype* pt = &g_ptypes[p->type];
    if (g_use_shaders == 0 || g_x5aa084 != 2) {
        for (int k = 0; k < p->nmesh; k++) {
            mesh* m = p->meshes[k];
            if (m->res->f1c & 0x40) {
                float light = 1.0f;
                if ((pt->flags & 0x80000i64) != 0 && ((int*)world->hf->cols[p->cx])[p->f0c + p->cy * 4 + 2] >= 0) {
                    vector4 n;
                    n.x = -sun.x; n.y = -sun.y; n.z = -sun.z;
                    light = max2(0.0f, dot3(n, p->frame.u));
                }
                if (g_use_shaders == 0 || g_x5aa084 == 0) {
                    AnimBox* bx = &pt->boxes[p->anim];
                    vector4 sum, c;
                    sum.x = bx->mn[0] + bx->mx[0];
                    sum.y = bx->mn[1] + bx->mx[1];
                    sum.z = bx->mn[2] + bx->mx[2];
                    c.x = sum.x * 0.5f;
                    c.y = sum.y * 0.5f;
                    c.z = sum.z * 0.5f;
                    c.z = (c.z < 0.3f) ? 0.3f : c.z;
                    vector4 pos;
                    pos.x = c.x + p->frame.o.x;
                    pos.y = c.y + p->frame.o.y;
                    pos.z = c.z + p->frame.o.z;
                    if (c.z < 1.5f) {
                        vector4 d, far_, near_;
                        vscale_to(d, sun, 1000.0f);
                        far_.x = pos.x - d.x; far_.y = pos.y - d.y; far_.z = pos.z - d.z;
                        vscale_to(d, sun, 0.05f);
                        near_.x = pos.x - d.x; near_.y = pos.y - d.y; near_.z = pos.z - d.z;
                        bool visible;
                        if (trace)
                            visible = world->trace(&near_, &far_, 50.0f, 0, 0) != 0;
                        else
                            visible = world->hf->line_clear(&near_, &far_) != 0;
                        if (!visible)
                            light *= 0.1f;
                    }
                }
                vector4 wp;
                wp.w = 1.0f;
                for (int j = p->first[k]; j < p->last[k]; j++) {
                    float lf = light;
                    if (g_use_shaders != 0 && g_x5aa084 == 1) {
                        vector4 q = m->v70[m->v50[j].index].a;
                        wp.x = q.x * m->m_cc.s.x + (q.z * m->m_cc.u.x + q.y * m->m_cc.f.x) + m->m_cc.o.x;
                        wp.y = q.x * m->m_cc.s.y + (q.z * m->m_cc.u.y + q.y * m->m_cc.f.y) + m->m_cc.o.y;
                        wp.z = q.x * m->m_cc.s.z + (q.z * m->m_cc.u.z + q.y * m->m_cc.f.z) + m->m_cc.o.z;
                        float occ;
                        g_probe.test((unsigned)&wp, &occ);
                        lf = occ * light;
                    }
                    vertex60& v = m->v50[j];
                    vector4 col;
                    rgl::unpack_color(&col, (const unsigned*)&v.f58);
                    vector4 t1, t2, t3, r;
                    vscale_to(t1, sun_dif, lf);
                    vscale_to(t2, sun_amb, 1.5f);
                    t3.x = t2.x + t1.x; t3.y = t2.y + t1.y; t3.z = t2.z + t1.z;
                    r.x = t3.x * col.x; r.y = t3.y * col.y; r.z = t3.z * col.z;
                    v.color = rgl::pack_color(&r);
                }
                m->slots[0].f28 = 1;
            }
        }
    }
}

// @ 0x472020
void Terrain::light_all(particle_list* l, bool trace)
{
    for (int i = 0; i < l->count; i++)
        light_particle(l, i, trace);
}

// ---- 0x472050: mission props: show/hide flag of the scene entity of every prop that is "alive or special" -----------------
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() {}
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
};
struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct EntFlag { char pad0[0x140]; char flag; };        // scene entity: +0x140 visibility
struct PropRec : IfaceB {                       // 0x1c8 bytes (vtable 0x590b84)
    char pad10[0x188 - 0x10];
    int destroyed;                              // +0x188 (0 intact, 1 / 5 special states)
    int kind;                                   // +0x18c
    char pad190[0x1b4 - 0x190];
    EntFlag* ent;                               // +0x1b4
    char pad1b8[0x1c8 - 0x1b8];
    PropRec(const PropRec& o);                  // 0x437bb0
    PropRec& operator=(const PropRec& o);       // 0x437d60
};
struct PropKind {                               // 0x78 bytes (array at 0x5e29dc)
    char pad0[0x11];
    unsigned char flags;                        // +0x11
    char pad12[0x78 - 0x12];
};
extern PropKind* g_propkinds;                   // 0x5e29dc
struct GameData {
    char pad0[0x1a090];
    int f1a090;                                 // +0x1a090
    char pad1a094[0x1a20c - 0x1a094];
    int f1a20c;                                 // +0x1a20c
};
extern GameData* g_game;                        // 0x5e2930
extern int g_flag5a9fb0;                        // 0x5a9fb0
struct Mission {
    char pad0[0x6688];
    std::vector<PropRec> props;                 // +0x6688 (first at +0x668c)
    char pad6698[0x6740 - 0x6698];
    EntFlag* special;                           // +0x6740
    void update_prop_flags();                   // 0x472050
};
// @ 0x472050
void Mission::update_prop_flags()
{
    if (g_flag5a9fb0 != 0) {
        if (special)
            special->flag = (g_game->f1a20c == 0 && g_game->f1a090 != 0) ? 1 : 0;
        for (unsigned i = 0; i < props.size(); i++) {
            PropRec& e = props[i];
            if (e.destroyed == 0) {
                if ((g_propkinds[e.kind].flags & 0x40) == 0)
                    continue;
            } else {
                if (e.destroyed != 1 && e.destroyed != 5)
                    continue;
            }
            {
                if (g_game->f1a090 != 0 && g_game->f1a20c == 0)
                    e.ent->flag = 1;
                else
                    e.ent->flag = 0;
            }
        }
    }
}

// ---- 0x472460: clear a list of owned arrays -------------------------------------------------------------------------------
struct ArrList {
    std::list<char*> l;                         // +0 (head at +4, size at +8)
    int pad;                                    // +0xc
    int f10;                                    // +0x10
    void clear();                               // 0x472460
};
// @ 0x472460
void ArrList::clear()
{
    for (std::list<char*>::iterator i = l.begin(); i != l.end(); ++i)
        delete[] *i;
    l.clear();
    f10 = 0;
}

// ---- 0x4724c0: vector::_Ufill for 0x24-byte records -----------------------------------------------------------------------
struct ptr_tag {};
struct R24 { char b[0x24]; };
struct alloc24 {};
R24* __cdecl uninit_fill_n_24(R24* f, unsigned n, const R24& v, alloc24& al, ptr_tag tag);   // 0x46b500
struct vec24 : alloc24 { R24* first; R24* last; R24* end; R24* ufill(R24* f, unsigned n, const R24& v); };
// @ 0x4724c0
R24* vec24::ufill(R24* f, unsigned n, const R24& v)
{
    ptr_tag t;
    uninit_fill_n_24(f, n, v, *this, t);
    return f + n;
}

// ---- 0x472160: re-place an impact decal / marker: random scatter around its anchor until a floor with a clear line is found -----
struct face_shape;
struct SceneF {
    char pad0[0x58];
    face_shape** default_floor;                 // +0x58
    void floor_at(const float* p, float* out_z, face_shape** out_face);          // 0x439be0 (thiscall, ret 0xc)
    int trace(const vector4* from, const vector4* to, float maxlen, int a, void** hit);   // 0x439d60 (thiscall, ret 0x14)
};
extern __int64 g_ticks[];                       // 0x5e2948 (100 kHz clocks)
struct MarkRec {                                // 0x130 bytes; array at FxMarks+0x6b0 (offsets relative to the record)
    rgl::matrix m;                              // +0x00 current frame (s, f, u, o)
    char pad40[0x80 - 0x40];
    rgl::matrix m2;                             // +0x80 next frame (0x730)
    int cnt2;                                   // +0xc0 (0x770)
    int cnt;                                    // +0xc4 (0x774)
    char pad_c8[0xd0 - 0xc8];
    int clock;                                  // +0xd0 (0x780)
    char pad_d4[0xd8 - 0xd4];
    __int64 stamp;                              // +0xd8 (0x788)
    float f790;                                 // +0xe0
    char pad_e4[0xe8 - 0xe4];
    face_shape* floor;                          // +0xe8 (0x798)
    char pad_ec[0x130 - 0xec];
};
struct FxMarks {
    SceneF* scene;                              // +0
    char pad4[0x6b0 - 4];
    MarkRec recs[1];                            // +0x6b0
    void replace(int idx);                      // 0x472160
};
static inline vector4 vsub3(const vector4& a, const vector4& b) { vector4 r; r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; return r; }
// @ 0x472160
void FxMarks::replace(int idx)
{
    recs[idx].f790 = 0.1f;
    recs[idx].stamp = g_ticks[recs[idx].clock];
    recs[idx].cnt++;
    vector4* sv = &recs[idx].m.s;
    recs[idx].floor = 0;
    vector4* ov = &recs[idx].m.o;
    vector4 a;
    a.x = sv->x; a.y = sv->y; a.z = sv->z;
    if (recs[idx].cnt > 15) {
        recs[idx].cnt2++;
        recs[idx].cnt = 0;
        *(rgl::matrix*)sv = recs[idx].m2;
        vector4 t1, t2;
        vscale_to(t1, recs[idx].m.f, 0.7f);
        vscale_to(t2, *sv, -0.5f);
        a = vsub3(t2, t1);
    }
    int n = 0;
    vector4 b;
    do {
        n++;
        b = vadd3(a, *ov);
        float fz;
        face_shape* fs;
        scene->floor_at(&b.x, &fz, &fs);
        b.z = fz;
        if (fs) {
            recs[idx].floor = fs;
            if (fabs(ov->z - fz) < 2.0f) {
                vector4 from = *ov;
                from.z = from.z + 0.7f;
                vector4 to = b;
                to.z = to.z + 0.7f;
                if (scene->trace(&from, &to, 15.0f, 0, 0)) {
                    *ov = b;
                    goto done;
                }
            }
        }
        a.x = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 1.2f;
        a.y = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 1.2f;
    } while (n <= 100);
    if (recs[idx].floor == 0)
        recs[idx].floor = *scene->default_floor;
done:
    if (recs[idx].cnt == 0)
        recs[idx].m2 = *(rgl::matrix*)sv;
}
