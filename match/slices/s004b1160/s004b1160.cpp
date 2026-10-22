// Slice s004b1160: the mission scene / render setup helpers. Names and layouts provisional.
// Matched: 0x4b14b0, 0x4b1700, 0x4b1710, 0x4b1780, 0x4b17d0, 0x4b1800, 0x4b1af0.
// Reconstructed here: 0x4b1160 (the whole-scene rebuild), 0x4b14f0 (the two-entry scene widget
// factory) and 0x4b1930 (the relation colour ramp).
#include <windows.h>
#include <math.h>
#include <vector>
#include <new>
#include "rgl.h"

using namespace rgl;

// ---- world / scene context (the object returned by 0x495650) ----
struct World {};

// ---- 0x4b14b0 ------------------------------------------------------------------------------------------
struct Elem68;                                   // 0x68-byte source template in the page array
struct Spawned {                                 // the object 0x493df0 builds
    char pad0[0x5c];
    int f5c;                                     // +0x5c
    char pad60[4];
    int refs;                                    // +0x64: reference count
    void (*f68)(int);                            // +0x68 (per-instance step callback)
};
struct Elem68 {
    char pad[0x68];
    Spawned* spawn(int b);                       // 0x493df0 (thiscall)
};
struct Holder {
    int f0;                                      // +0
    Elem68* f4;                                  // +4  page array
    void prep();                                 // 0x493f30 (thiscall)
    void add_spawned(int* a, int b);
};
extern void scene_step(int);                     // 0x4b1160 (the per-instance step)

// @ 0x4b14b0
void Holder::add_spawned(int* a, int b)
{
    prep();
    Elem68* e = f4 + *a;
    Spawned* r = e->spawn(b);
    r->f5c = *a;
    r->f68 = scene_step;
}

// @ 0x4b1700
int max_scene_slots()
{
    return 0x20;
}

// ---- 0x4b1710: rotate one 0x20-byte record in the table at +0xa8 -----------------------------------------
struct Elem20 {
    int v[8];                                    // 0x20 bytes
};
struct RotHolder {
    char pad0[0xa8];
    Elem20 arr[64];                              // +0xa8 (inline table of 0x20-byte records)
    void rotate(int from, int to);
};

// @ 0x4b1710
void RotHolder::rotate(int from, int to)
{
    Elem20 tmp = arr[to];
    if (from < to)
        for (int i = to; i > from; i--)
            arr[i] = arr[i - 1];
    arr[from] = tmp;
}

// ---- 0x4b1780 ------------------------------------------------------------------------------------------
struct ScriptState {
    char pad0[0x5980];
    int f5980;                                   // +0x5980
    int f5984;                                   // +0x5984
    int f5988;                                   // +0x5988
    char pad8c[0x5998 - 0x598c];
    int f5998;                                   // +0x5998
    int f599c;                                   // +0x599c
    int f59a0;                                   // +0x59a0
    int f59a4;                                   // +0x59a4
    void set(int* a, int* b);
};

// @ 0x4b1780
void ScriptState::set(int* a, int* b)
{
    f5998 = 1;
    f599c = a[0];
    f59a0 = a[1];
    f59a4 = (int)b;
    if (f5984 != b[1]) {
        f5980 = 0;
        f5988 = 1;
    }
}

// ---- 0x4b17d0 ------------------------------------------------------------------------------------------
struct VisHolder {
    char pad0[0x18];
    VisHolder* f18;                              // +0x18
    char pad1c[0xac - 0x1c];
    int fac;                                     // +0xac
    bool visible() const;
};

// @ 0x4b17d0
bool VisHolder::visible() const
{
    if (f18)
        return f18->fac > 0;
    return fac > 0;
}

// ---- 0x4b1af0 ------------------------------------------------------------------------------------------
struct CountHolder {
    int f0;                                      // +0
    int f4;                                      // +4  begin
    int f8;                                      // +8  end
    int count() const;
};

// @ 0x4b1af0
int CountHolder::count() const
{
    if (!f4)
        return 0;
    return (f8 - f4) / 0x5c;
}

// ---- 0x4b1800: fade level from a distance ratio --------------------------------------------------------
struct FadeCam {
    char pad0[0x15c];
    float f15c;                                  // +0x15c
    float f160;                                  // +0x160
    float f164;                                  // +0x164
    void set_fade(const float* p);
};

// @ 0x4b1800
void FadeCam::set_fade(const float* p)
{
    f15c = *p;
    float t = f15c * 4.0f - 0.4f;
    if (t < 0.0f)
        t = 0.0f;
    else if (t >= 1.0f)
        t = 1.0f;
    f160 = t;
    f164 = 1.0f - t;
    if (f15c > 0.5f) {
        f160 = 1.0f;
        f164 = 0.0f;
        return;
    }
    if (f15c > 0.15f) {
        t = (f15c - 0.15f) * 2.857143f;
        f160 = t;
        f164 = 1.0f - t;
        return;
    }
    if (f15c > 0.03f) {
        f160 = 0.0f;
        f164 = 1.0f;
        return;
    }
    if (f15c > 0.015f) {
        f160 = 0.0f;
        f164 = (f15c - 0.015f) * 66.66667f;
        return;
    }
    f160 = 0.0f;
    f164 = 0.0f;
}

// =========================================================================================================
// The scene-rebuild side: 0x4b1160 / 0x4b14f0 / 0x4b1930
// =========================================================================================================

// ---- engine declarations (bodies live in other slices; declared only) -----------------------------------
struct res_t {
    char pad0[0x64];
    int refs;                                                // +0x64
    void (*fn68)(res_t*);                                    // +0x68
};
struct mesh {
    char pad0[0x2c];
    res_t* res;                                              // +0x2c
};
struct render_res;

unsigned pack_color(const vector4* c);                       // 0x40e440
void identity_pos(void* out);                                // 0x40cd00
void vector_assign(void* dst, const void* src);              // 0x43f0a0 (std::vector<int>::operator=)

struct plane {
    vector4 pos;
    vector4 normal;
    int in_front(const vector4* p, float eps) const;         // 0x41e470
};

// the view frustum proper: 0x120 bytes (planes, camera frame, clip distances)
struct frustum {
    char perspective;                                        // +0
    char pad1[3];
    int nplanes;                                             // +4
    plane planes[6];                                         // +8
    matrix m;                                                // +0xc8
    float left, right, top, bottom, nearz, farz;             // +0x108
    void build_planes();                                     // 0x41e4f0
};

// +0x120: the scene record carried after the frustum
struct frustum_ex : frustum {
    int flag;                                                // +0x120
    float mat4[4];                                           // +0x124
};

typedef char _chk_frustum[sizeof(frustum) == 0x120 ? 1 : -1];
typedef char _chk_frustum_ex[sizeof(frustum_ex) == 0x134 ? 1 : -1];

// std::vector<void*> whose copy-assignment is out of line (0x43f0a0)
struct PtrVec {
    std::vector<void*> v;
    PtrVec& operator=(const PtrVec& o);                      // 0x43f0a0
};

// the render scene (0x1d8 bytes, ctor 0x429740)
struct scene {
    int f0, f4, f8, fc, f10;                                 // 0x00
    float f14;                                               // 0x14
    int f18, f1c, f20, f24, f28;                             // 0x18
    float f2c;                                               // 0x2c
    frustum fr;                                              // 0x30
    render_res* target;                                      // 0x150
    render_res* rt;                                          // 0x154
    scene* refl;                                             // 0x158
    int f15c;                                                // 0x15c
    char f160, f161, f162, pad163;                           // 0x160
    int f164;                                                // 0x164
    char f168, f169, f16a, pad16b;                           // 0x168
    int f16c, f170, f174;                                    // 0x16c
    float f178;                                              // 0x178
    vector4 ambient;                                         // 0x17c
    int f18c, f190;                                          // 0x18c
    PtrVec list;                                             // 0x194
    char f1a4, f1a5, f1a6, f1a7;                             // 0x1a4
    unsigned mask;                                           // 0x1a8
    char f1ac, pad1ad[3];                                    // 0x1ac
    void* f1b0;                                              // 0x1b0
    char pad1b4[0x1c4 - 0x1b4];
    void* owner;                                             // 0x1c4
    std::vector<void*> cache;                                // 0x1c8
    scene();                                                 // 0x429740
    void reset();
};

typedef char _chk_scene_fr[offsetof(scene, fr) == 0x30 ? 1 : -1];
typedef char _chk_scene_target[offsetof(scene, target) == 0x150 ? 1 : -1];
typedef char _chk_scene_list[offsetof(scene, list) == 0x194 ? 1 : -1];
typedef char _chk_scene_mask[offsetof(scene, mask) == 0x1a8 ? 1 : -1];

// the object appended by g_obj.push() (0x495650): a world/render context with a frustum at +0x36c
struct SceneObj {
    char pad0[0x68];
    vector4 mat68;                                           // 0x68
    char pad78[0xb8 - 0x78];
    PtrVec list;                                             // 0xb8
    char padc8[0xd8 - 0xc8];
    void* fd8;                                               // 0xd8
    char pade0[0x344 - 0xdc];
    int f344;                                                // 0x344
    int f348;                                                // 0x348
    char pad34c[0x36c - 0x34c];
    frustum_ex fr;                                           // 0x36c
    void reset();                                            // 0x421390
    void step(float dt);                                     // 0x427f40
    void render_scene(scene* sc, bool reflect);              // 0x429890
    void emit_sub(int a, int b, int* flag, __int64* v, int last);  // 0x4b09e0 (ecx = rec+0x38)
};
typedef char _chk_obj_fr[offsetof(SceneObj, fr) == 0x36c ? 1 : -1];

struct rec68_t {
    char pad0[0x2c];
    res_t* res;                                              // +0x2c
    char pad30[0x68 - 0x30];
    Spawned* find_or_create(int* key);                       // 0x493df0 (thiscall, ret 4)
};

struct obj_tbl {
    int f0;                                                  // +0
    rec68_t* recs;                                           // +4 (0x68-byte records)
    char pad8[0x10 - 8];
    std::vector<void*> worlds;                               // +0x10 (first at +0x14)
    SceneObj* push();                                        // 0x495650
    void purge_all();                                        // 0x493f30
    void pop_back_delete();                                  // 0x492ee0
    mesh* make_mesh_from(const int* index, const void* from);  // 0x4991d0
};
extern obj_tbl g_obj;                                        // 0xa75f40
extern int g_5a75f60;                                        // 0xa75f60

struct resmgr2 {
    render_res* get_texture(int w, int h, int fmt);          // 0x41c400
    void release(void* p);                                   // 0x4175f0
};
extern resmgr2 g_resmgr;                                     // 0x5d6fa0

struct SceneReg2 { bool select(scene* sc); };                // 0x47e520
extern SceneReg2 g_scene_reg;                                // 0x5fe898

// ---- the mission side used by the step ----------------------------------------------------------------
struct MissionCtx {
    char pad0[0x18];
    char f18;                                                // +0x18 (render enabled)
    char pad19[0x4c - 0x19];
    int f4c;                                                 // +0x4c
};
struct Mission {
    char pad0[0x48];
    MissionCtx* f48;                                         // +0x48
    char pad4c[0x5c - 0x4c];
    int f5c;                                                 // +0x5c (scene index)
    __int64 f60;                                             // +0x60
};
struct MissionRec {
    char pad0[0x20];
    int f20, f24;                                            // +0x20
    char pad28[0x38 - 0x28];
    char sub38[0x30];
};

// @ 0x4b1160
void scene_step(int mission)
{
    Mission* m = (Mission*)mission;
    if (!m->f48->f18)
        return;

    SceneObj* o = g_obj.push();
    MissionRec* rec = (MissionRec*)((char*)g_obj.recs + m->f5c * 0x68);

    scene sc;
    sc.mask = 0x2000;
    sc.f0 = 0;
    sc.f4 = 0;
    sc.f8 = rec->f20;
    sc.fc = rec->f24;
    sc.f10 = 0;
    sc.f14 = 1.0f;
    sc.f18 = 0;
    sc.f1c = 0;
    sc.f20 = rec->f20;
    sc.f24 = rec->f24;
    sc.f28 = 0;
    sc.f2c = 1.0f;
    sc.f1a5 = 1;
    sc.f1ac = 0;
    sc.f18c = 0x300;
    sc.f16a = 0;

    o->fr.flag = 0;
    g_5a75f60 = 0;
    o->fr.mat4[0] = 1.0f;
    o->fr.mat4[1] = 1.0f;
    o->fr.mat4[2] = 1.0f;
    o->fr.mat4[3] = 1.0f;
    o->f344 = 0;
    o->f348 = 0;
    o->reset();

    o->fr.perspective = 0;
    o->fr.nplanes = 0;
    o->fr.nearz = 0.0f;
    o->fr.farz = 400.0f;
    o->fr.left = -0.5f;
    o->fr.right = 0.5f;
    o->fr.bottom = -0.375f;
    o->fr.top = 0.375f;
    o->fr.build_planes();

    char ident[0x40];
    identity_pos(ident);
    ((vector4*)(ident + 0x30))->x = 0.0f;
    ((vector4*)(ident + 0x30))->y = 0.0f;
    ((vector4*)(ident + 0x30))->z = 400.0f;
    *(vector4*)(ident + 0x30) = *(vector4*)(ident + 0x30);
    o->fr.m = *(matrix*)ident;
    o->fr.build_planes();

    __int64 v = m->f60;
    int flag = 1;
    o->emit_sub(-0x41, 0, &flag, &v, -1);

    sc.fr = o->fr;
    sc.f190 = o->fr.flag;
    sc.f1b0 = o->fd8;
    sc.f160 = 1;
    sc.list = o->list;
    o->mat68 = *(vector4*)o->fr.mat4;
    o->mat68.w = 1.0f;

    sc.target = (render_res*)m->f48;
    o->step(0.0f);
    if (g_5a75f60 != 0)
        sc.f160 = 1;

    for (;;) {
        o->render_scene(&sc, false);
        int h = rec->f24;
        if (h < 0x201)
            h = 0x200;
        int w = rec->f20;
        if (w < 0x201)
            w = 0x200;
        render_res* t = g_resmgr.get_texture(w, h, 0x50);
        if (t != 0)
            sc.f15c = (int)t;
        bool ok = g_scene_reg.select(&sc);
        if (t != 0)
            g_resmgr.release(t);
        if (ok)
            break;
    }

    m->f48->f4c = 1;
    g_obj.pop_back_delete();
    sc.~scene();
}

// ---- the widget factory (vtable 0x591ee8, size 0x188) ---------------------------------------------------
struct vec2 { float x, y; };
struct panel {
    panel();                                                 // 0x48ae20
    virtual void z0();
};

#define WG_VIRTUALS \
    virtual void w1();  virtual void w2();  virtual void w3();  virtual void w4(); \
    virtual void w5();  virtual void w6();  virtual void w7();  virtual void w8(); \
    virtual void w9();  virtual void w10(); virtual void w11(); virtual void w12(); \
    virtual void w13(float); \
    virtual void w14(); virtual void w15(); virtual void w16(); virtual void w17(); \
    virtual void w18(); virtual void w19(); virtual void w20(); virtual void w21(); \
    virtual void w22(void*); \
    virtual void w23(); virtual void w24(); \
    virtual void w25(vec2*); \
    virtual void w26(); \
    virtual void w27(vec2*); \
    virtual void w28(vec2*); \
    virtual void w29(void*); \
    virtual void w30(); virtual void w31(); \
    virtual void w32(int, mesh*); \
    virtual void w33(); virtual void w34(); virtual void w35(); virtual void w36(); \
    virtual void w37(); virtual void w38(); \
    virtual void w39(const void*); \
    virtual void w40(); \
    virtual void w41(void*, void*);

struct ab_base : panel {
    char p0[0xd8 - 4];
    float fd8, fdc;                                          // 0xd8, 0xdc
    char p1[0x121 - 0xe0];
    unsigned char b121;                                      // 0x121
    char p2[0x178 - 0x122];
    int f178;                                                // 0x178
    char p3[0x180 - 0x17c];
    unsigned char b180;                                      // 0x180
    char p4[0x184 - 0x181];
    int f184;                                                // 0x184
    ab_base() { f178 = 0; b121 = 0; b180 = 0; f184 = 0; }
    void set_flag() { f184 = 1; }
};
struct cls_b : ab_base { WG_VIRTUALS };

struct scene_iface {
    virtual void d0();  virtual void d1();  virtual void d2();  virtual void d3();  virtual void d4();
    virtual void d5();  virtual void d6();  virtual void d7();  virtual void d8();  virtual void d9();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23(); virtual void d24();
    virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28(); virtual void d29();
    virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33(); virtual void d34();
    virtual void d35(); virtual void d36(); virtual void d37(); virtual void d38(); virtual void d39();
    virtual void d40(); virtual void d41(); virtual void d42(); virtual void d43(); virtual void d44();
    virtual void d45(); virtual void d46(); virtual void d47(); virtual void d48(); virtual void d49();
    virtual void d50(cls_b* w);                              // slot 50 (vtable +0xc8)
};

struct scene_tbl2 {
    char* recs;                                              // +0
    mesh* placed(int index);                                 // 0x499050
};
extern scene_tbl2 g_scene;                                   // 0x8ed4f58

struct widget_mgr2 {
    std::vector<panel*> widgets;                             // 0x00 (_Myfirst at +4)
    char f10[0x2c - 0x10];
    scene_iface* scene;                                      // 0x2c
    void ins_slow(panel** where, int count, panel* const& val);    // 0x4927d0
    int add_scene(int pos, int rec, int key);                // 0x4b14f0
};

// @ 0x4b14f0
int widget_mgr2::add_scene(int pos, int rec, int key)
{
    cls_b* w = new cls_b;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        w->w22(&v);
        *(int*)&v.x = 0x700;
        w->w29(&v);
    }

    if (pos >= 0) {
        mesh* m = g_scene.placed(pos);
        g_obj.purge_all();
        rec68_t* r = g_obj.recs + rec;
        Spawned* e = r->find_or_create(&key);
        e->f5c = rec;
        e->f68 = scene_step;
        if (m->res)
            m->res->refs--;
        m->res = (res_t*)e;
        if (e)
            e->refs++;
        w->w32(2, m);

        m = g_scene.placed(pos);
        g_obj.purge_all();
        r = g_obj.recs + rec;
        e = r->find_or_create(&key);
        e->f5c = rec;
        e->f68 = scene_step;
        if (m->res)
            m->res->refs--;
        m->res = (res_t*)e;
        if (e)
            e->refs++;
        w->w32(1, m);
    } else {
        w->w32(2, g_obj.make_mesh_from(&rec, &key));
        w->w32(1, g_obj.make_mesh_from(&rec, &key));
    }

    w->w3();
    scene->d50(w);
    widgets.push_back(w);
    return (int)widgets.size() - 1;
}

// @ 0x4b1930
unsigned __stdcall GameData_relation_color(float t, int flag)
{
    vector4 c(1.0f, 1.0f, 1.0f);
    if (t < -0.0001f) {
        float a = 1.0f + t;
        float hi[3];
        hi[0] = 0.95f * a;
        hi[1] = 0.65f * a;
        hi[2] = 0.65f * a;
        float u = -t;
        float lo[3];
        lo[0] = 0.95f * u;
        lo[1] = 0.0f * u;
        lo[2] = 0.0f * u;
        c = vector4(lo[0] + hi[0], lo[1] + hi[1], lo[2] + hi[2]);
    } else if (t > 0.0001f) {
        float u = 1.0f - t;
        float hi[3];
        hi[0] = 0.75f * u;
        hi[1] = 0.95f * u;
        hi[2] = 0.65f * u;
        float lo[3];
        lo[0] = 0.50f * t;
        lo[1] = 1.0f * t;
        lo[2] = 0.20f * t;
        c = vector4(lo[0] + hi[0], lo[1] + hi[1], lo[2] + hi[2]);
    }
    if (flag == 0) {
        c.x = c.x * 0.65f + 0.2f;
        c.y = c.y * 0.65f + 0.2f;
        c.z = c.z * 0.65f * 0.7f + 0.2f;
    }
    c.w = 1.0f;
    return pack_color(&c);
}
