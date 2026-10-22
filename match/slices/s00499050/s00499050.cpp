// Slice s00499050: resource-record -> entity helpers for the scene-editor / mission object spawners.
// Each of these builds a mesh from a resource record and hands it to a freshly created entity.
// ALL NAMES PROVISIONAL.
#include <windows.h>
#include <math.h>
#include "rgl.h"
#include "mesh.h"

using namespace rgl;

extern float g_rad_per_deg;         // 0x590ce4: PI / 180
extern float g_one;                 // 0x549cdc: 1.0f (shared constant)
extern float g_zero;                // 0x549ca8: 0.0f (shared constant)
extern int   g_gamma_flag;          // 0x5e28fc: non-zero -> apply gamma to colours
extern void* g_meshes;              // 0x5e29f0: base of the 0x48-stride mesh records (name at +0x14)

// A 3x3 transform (the "frame" the scene records describe): identity3 at 0x40cd60 writes 0x30 bytes.
namespace rgl {
struct mat3 {
    vector4 s, f, u;
    void rotate_fu(const float* angle);         // 0x40e570
    void rotate_su(const float* angle);         // 0x40e670
    void rotate_sf(const float* angle);         // 0x40e770
};
mat3* identity3(mat3* out);                     // 0x40cd60
}

// ---- entity / World (vtable 0x54b460; layout from s00428240/world.h) ----
struct World;
struct entity {
    virtual ~entity();
    virtual int type_id();
    virtual void* type_name();
    virtual void v3();
    virtual void v4();
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23();
    World* owner;                               // 0x04
    int id;                                     // 0x08
    char pad0[0x58 - 0x0c];
    rgl::matrix local;                          // 0x58
    void add_mesh(mesh* m);                     // 0x442110
};
struct World {
    entity* create_entity(int type_id);         // 0x428440
};

// ---- the "widget" the scene editor hangs its buttons off (vtable 0x5922e8, size 0x178) ----
struct widget_base {
    widget_base();                              // 0x48ae20 (panel ctor: installs the base vtable)
    virtual void b0();                          // slot 0: base vtable entry
};
struct widget : widget_base {
    widget() {}
    virtual void w1();  virtual void w2();
    virtual void w3();                          // slot 3  (vtable +0x0c): no args
    virtual void w4();  virtual void w5();  virtual void w6();  virtual void w7();
    virtual void w8();  virtual void w9();  virtual void w10(); virtual void w11();
    virtual void w12(); virtual void w13(); virtual void w14(); virtual void w15();
    virtual void w16(); virtual void w17(); virtual void w18(); virtual void w19();
    virtual void w20(); virtual void w21();
    virtual void w22(void* a);                  // slot 22 (vtable +0x58): one pointer
    virtual void w23(); virtual void w24(); virtual void w25(); virtual void w26();
    virtual void w27(); virtual void w28();
    virtual void w29(void* a);                  // slot 29 (vtable +0x74): one pointer
    virtual void w30(); virtual void w31();
    virtual void w32(int a, mesh* m);           // slot 32 (vtable +0x80): (int, mesh*)
    char pad[0x174];
};

// the scene the editor window is attached to (vtable +0xc8 takes the new widget)
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
    virtual void d50(widget* w);                // slot 50 (vtable +0xc8)
};

// ---- the editor's 0x48-byte scene record ----
struct rec48 {
    mstr name;                      // 0x00: resource name
    unsigned char flags;            // 0x10: bit 0 -> mesh gets the 0x2000 flag
    char pad1[0x24 - 0x11];
    float px, py, pz;               // 0x24
    float rx, ry, rz;               // 0x30: rotations in degrees
    float sx, sy, sz;               // 0x3c: scale
};

// the editor's scene table: a 0x48-byte record array at +0.
struct scene_tbl {
    rec48* recs;                    // 0x00
    mesh* placed(int index);                        // 0x499050
};

// the widget factory: a std::vector<widget*> whose _Myfirst sits at +4 (the STL vector is 16 bytes), the
// scene at +0x2c.
struct widget_mgr {
    std::vector<widget*> widgets;   // 0x00 (_Myfirst at +4)
    char f10[0x2c - 0x10];
    scene_iface* scene;             // 0x2c

    int add_widget(int index);                      // 0x4995e0
    int add_widget2(int a, int b, float c);         // 0x4996e0
    void ins_slow(widget** where, int count, widget* const& val);   // 0x4927d0
};

// ---- the object table: a 0x68-byte record array at +4, its worlds vector whose _Myfirst is at +0x14 ----
struct rec68 {
    char pad0[0x28];
    float f28, f2c, f30, f34;
    char padtail[0x68 - 0x38];
};
struct obj_tbl {
    void* f0;                       // 0x00
    rec68* recs;                    // 0x04
    char f8[0x10 - 0x8];
    std::vector<World*> worlds;     // 0x10 (_Myfirst at +0x14)

    res_t* make(const int* index, const int* from);                                 // 0x4b14b0
    mesh* make_mesh_from(const int* index, const void* from);                       // 0x4991d0
    void spawn(int index, int from, const mat4* frame, const vector4* scale);       // 0x4992b0
    void spawn_scaled(int index, const mat4* frame, const vector4* scale,
                      const float* alpha, const unsigned* color);                   // 0x499390
    void spawn_scaled2(int index, const mat4* frame, const float* scale);           // 0x499540
};

extern scene_tbl  g_scene;          // 0x8ed4f58
extern obj_tbl    g_obj;            // 0xa75f40

typedef char _chk_rec68[sizeof(rec68) == 0x68 ? 1 : -1];
typedef char _chk_ent[offsetof(entity, local) == 0x58 ? 1 : -1];
typedef char _chk_widget[sizeof(widget) == 0x178 ? 1 : -1];

// @ 0x499050
mesh* scene_tbl::placed(int index)
{
    mesh* m = g_resmgr.get_object(recs[index].name)->clone();
    if (recs[index].flags & 1)
        m->f214 += 0x2000;
    vector4 pos;
    rec48* r = &recs[index];
    vector4 sc(recs[index].sx, recs[index].sy, recs[index].sz);
    pos.x = r->px;
    pos.y = r->py;
    pos.z = r->pz;
    mat3 frame;
    identity3(&frame);
    float a = r->rx * 0.017453292f;
    frame.rotate_fu(&a);
    a = r->ry * 0.017453292f;
    frame.rotate_su(&a);
    a = r->rz * 0.017453292f;
    frame.rotate_sf(&a);
    if (r->sx != g_one || r->sy != g_one || r->sz != g_one)
        m->set_scale(sc);
    m->set_part((const mat4*)&frame);
    if (pos.z * pos.z + pos.y * pos.y + pos.x * pos.x != g_zero)
        m->translate(&pos);
    return m;
}

// @ 0x4991d0
mesh* obj_tbl::make_mesh_from(const int* index, const void* from)
{
    mesh* m = new mesh;
    int i = *index;
    vector4 b, a;
    vec2 uv1, uv0;
    unsigned color;
    b.x = recs[i].f30;
    b.y = recs[i].f34;
    a.x = recs[i].f28;
    a.y = recs[i].f2c;
    color = 0xffffffff;
    uv1.x = 1.0f;
    uv1.y = 1.0f;
    uv0.x = 0.0f;
    uv0.y = 0.0f;
    b.z = 0.0f;
    a.z = 0.0f;
    m->add_quad(&a, &b, &uv0, &uv1, &color);
    res_t* nr = make(index, (const int*)from);
    if (m->res)
        m->res->refs--;
    m->res = nr;
    if (nr)
        nr->refs++;
    return m;
}

// @ 0x4992b0
void obj_tbl::spawn(int index, int from, const mat4* frame, const vector4* scale)
{
    World* w = worlds[worlds.size() - 1];
    entity* e = w->create_entity(9);
    mesh* m = make_mesh_from(&index, &from);
    if (!(0.99999f <= scale->x && scale->x < 1.00001f
       && 0.99999f <= scale->y && scale->y < 1.00001f
       && 0.99999f <= scale->z && scale->z < 1.00001f))
        m->set_scale(*scale);
    e->add_mesh(m);
    *(mat4*)((char*)e + 0x58) = *frame;
}

// @ 0x499390
void obj_tbl::spawn_scaled(int index, const mat4* frame, const vector4* scale,
                           const float* alpha, const unsigned* color)
{
    World* w = worlds[worlds.size() - 1];
    entity* e = w->create_entity(9);
    mesh* m = g_scene.placed(index);
    if (!(0.99999f <= scale->x && scale->x < 1.00001f
       && 0.99999f <= scale->y && scale->y < 1.00001f
       && 0.99999f <= scale->z && scale->z < 1.00001f))
        m->set_scale(*scale);
    if (*color != 0xffffffffu) {
        vector4 c;
        unpack_color(&c, color);
        if (g_gamma_flag) {
            c.x = pow((double)c.x, (double)2.2f);
            c.y = pow((double)c.y, (double)2.2f);
            c.z = pow((double)c.z, (double)2.2f);
        }
        m->set_color(pack_color(&c));
    }
    if (*alpha < 0.99f) {
        float a;
        if (*alpha < 0.0f)
            a = 0.0f;
        else if (*alpha >= 1.0f)
            a = 1.0f;
        else
            a = *alpha;
        m->set_alpha(a);
    }
    e->add_mesh(m);
    *(mat4*)((char*)e + 0x58) = *frame;
}

// @ 0x499540
void obj_tbl::spawn_scaled2(int index, const mat4* frame, const float* scale)
{
    World* w = worlds[worlds.size() - 1];
    entity* e = w->create_entity(9);
    mesh* m = g_resmgr.get_object(*(mstr*)((char*)g_meshes + index * 0x48 + 0x14))->copy_of();
    if (!(0.99999f <= *scale && *scale < 1.00001f))
        m->scale(*scale);
    e->add_mesh(m);
    *(mat4*)((char*)e + 0x58) = *frame;
}

// @ 0x4995e0
int widget_mgr::add_widget(int index)
{
    widget* w = new widget;
    vec2 v;
    v.x = 0.0f;
    v.y = 0.0f;
    w->w22(&v);
    *(int*)&v.x = 0x700;
    w->w29(&v);
    w->w32(0, g_scene.placed(index));
    w->w3();
    scene->d50(w);
    widgets.push_back(w + 0);
    return (int)widgets.size() - 1;
}

// @ 0x4996e0
int widget_mgr::add_widget2(int a, int b, float c)
{
    widget* w = new widget;
    vec2 v;
    v.x = 0.0f;
    v.y = 0.0f;
    w->w22(&v);
    *(int*)&v.x = 0x700;
    w->w29(&v);
    if (a >= 0) {
        mesh* m = g_scene.placed(a);
        res_t* nr = g_obj.make(&a, &b);
        if (m->res)
            m->res->refs--;
        m->res = nr;
        if (nr)
            nr->refs++;
        w->w32(0, m);
    } else {
        w->w32(0, g_obj.make_mesh_from(&a, &b));
    }
    w->w3();
    scene->d50(w);
    widgets.push_back(w + 0);
    return (int)widgets.size() - 1;
}
