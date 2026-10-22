// Slice s004b7840: 0x4b7840 Mission::f4b7840 (builds the mission's two "extra" view meshes from the
// emit-definition table, swaps them into the renderer's entity list and sets the HUD counters) and
// 0x4b7e50 Mission::update_hud_text (the ammo/time readout over the crosshair).
// Class and field names are provisional; the layouts follow the member accesses of these two functions.
#include <windows.h>
#include <stdio.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// 16-byte rgl string (layout: s00477530/tstring.h)
struct tstring {
    int pos;        // +0
    char* buf;      // +4
    int cap;        // +8
    int len;        // +0xc
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// one 0x30-byte transform row (rgl::vector4); only x/y/z take part in the per-axis scaling below
struct Row { float x, y, z, w; };
struct mat3 { Row s, f, u; void rotate_sf(const float* angle); };       // 0x40e770 (ret 4)
mat3* __cdecl identity3(mat3* out);                                     // 0x40cd60 (cdecl, 1 arg)

// the animation list at the tail of an emit definition: count + array of 0x30-byte frames
struct Frame { char pad[0x30]; };
struct FrameList {
    int count;                                  // +0
    Frame* frames;                              // +4
    char pad8[4];
    void apply(int tag);                        // 0x48add0 (ret 4)
};

// the emit-table definition record (0x48 bytes; read by 0x479d20)
struct EmitDef {
    char pad0[0x10];
    int f10;                                    // +0x10 (bit0 selects the HUD mode)
    tstring name;                               // +0x14
    float scale;                                // +0x24
    char pad28[0x2c - 0x28];
    vector4 pos;                                // +0x2c (translation for the rider mesh)
    FrameList frames;                           // +0x3c
};

// ---- engine objects ---------------------------------------------------------------------------------------
struct Entity {
    virtual void v0(int arg);
};
struct mesh : Entity {
    char pad0[0xc8];
    mat3 rot;                                   // +0xcc (rows s, f, u; scaled in place)
    char pad_fc[0x10];
    unsigned mask;                              // +0x10c
    void translate(const vector4* d);           // 0x44f240 (ret 4)
};
struct mesh_holder {
    mesh* get();                                // 0x4958f0
    mesh* clone();                              // 0x41c6d0
};
struct resmgr {
    char pad0[0x14];
    mesh_holder* get_object(tstring* name);     // 0x419540 (ret 4)
};
extern resmgr g_resmgr;                         // 0x5d6fa0

// the "emit" object handed to the renderer (only its vtable matters here)
struct Emitter {
    virtual void v0();
};
struct EmitTable {
    void* defs;                                 // +0
    int count;                                  // +4
    Emitter* setup(int* elem, const vector4* pos, int* out);   // 0x479d20 (ret 0xc)
};
extern EmitTable g_emit_defs;                   // 0x8ed5090
extern EmitDef* g_emit_defs_data;               // 0x5e29f0 (base of the 0x48-byte definition records)

struct Renderer {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void set_color(const float* shade); // +0x18
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void fl_48(int mode);               // +0x48
    virtual void v19();
    virtual void v20();
    virtual void v21();
    virtual void v22();
    virtual void v23();
    virtual void v24();
    virtual void v25_emit(Emitter* e);          // +0x64
    char pad68[0xc0 - 0x68];
    float f_c0;                                 // +0xc0
    vector4 color;                              // +0xc4
    char pad_d4[0x11c - 0xd4];
    std::vector<Entity*> ents;                  // +0x11c (_Myfirst/_Mylast)
    char pad128[0x294 - 0x128];
    float f294;                                 // +0x294
    void add_mesh(mesh* m);                     // 0x442110 (ret 4)
    void f4b1800(const float* t);               // 0x4b1800 (ret 4)
};

struct GameData {
    char pad0[0x78];
    int f78;                                    // +0x78
    char pad7c[0x13a48 - 0x7c];
    char* ui;                                   // +0x13a48
    char pad_1a1fc[0x1a1fc - 0x13a4c];
    int f1a1fc;                                 // +0x1a1fc
    mesh* f1a200;                               // +0x1a200
    void update_ammo(int id, int* a, int* b, int* c, int* d);      // 0x490ce0 (ret 0x14)
    unsigned color_of(float v, int troop);      // 0x4b1930 (ret 8)
    void show_text(unsigned id, const void* txt, int mode, unsigned color, const float* t);   // 0x49baa0 (ret 0x14)
    char emit_event(int id, int a, int b);      // 0x435790 (ret 0xc)
};
extern GameData* g_game;                        // 0x5e2930

// ---- mission ----------------------------------------------------------------------------------------------
struct Mission {
    char pad0[8];
    int f8;                                     // +8
    char padc[0x50 - 0xc];
    float cam_pitch;                            // +0x50
    char pad54[0x6c - 0x54];
    int f6c;                                    // +0x6c
    int f70;                                    // +0x70
    char pad74[0x94 - 0x74];
    int f94;                                    // +0x94 (the rider's troop id)
    char pad98[0xa0 - 0x98];
    unsigned flags;                             // +0xa0
    float fa4;                                  // +0xa4
    char pada8[0x8ac - 0xa8];
    float f8ac;                                 // +0x8ac (march time)
    float f8b0;                                 // +0x8b0
    char f8b4;                                  // +0x8b4 (pending kill-feed refresh)
    char f8b5;                                  // +0x8b5 (kill-feed visible)
    char pad8b6[0x59c0 - 0x8b6];
    Renderer* renderer;                         // +0x59c0
    char padc4[0x59e8 - 0x59c4];
    int f59e8;                                  // +0x59e8
    Entity* f59ec;                              // +0x59ec
    Entity* f59f0;                              // +0x59f0
    std::vector<int> f59f8;                     // +0x59f8
    char f5a04;                                 // +0x5a04

    void f4b7840();                             // 0x4b7840
    void update_hud_text();                     // 0x4b7e50
    const void* get_text();                     // 0x48d7b0 (returns this+0xc or this+0x20)
    float cost();                               // 0x489460
};

extern __int64 g_dt;                            // 0xa70448
extern int g_ev_a;                              // 0xa70458
extern int g_ev_b;                              // 0xa7045c
extern int g_flag_a181b0;                       // 0xa181b0

float march_time(Mission* m);                   // 0x4b5c30
vector4* __cdecl unpack_color(vector4* out, const unsigned* argb);   // 0x40e4e0 (cdecl)
unsigned __cdecl pack_color(const vector4* c);                       // 0x40e440 (cdecl)

// @ 0x4b7840
void Mission::f4b7840()
{
    if (f5a04) {
        f5a04 = 0;
        for (unsigned i = 0; f59f8._Myfirst != 0 && i < (unsigned)(f59f8._Mylast - f59f8._Myfirst); i++) {
            vector4 pos(0.0f, 0.0f, 0.0f);
            int idx = 0;
            Emitter* e = g_emit_defs.setup(&f59f8._Myfirst[i], &pos, &idx);
            if (e)
                renderer->v25_emit(e);
        }
    }
    unsigned def = (flags & 0xff) * 0x48;
    mesh* m2 = 0;
    int rider = f94;
    EmitDef* d = (EmitDef*)((char*)g_emit_defs_data + def);
    mesh* m = g_resmgr.get_object(&d->name)->get();
    float a = d->scale;
    m->rot.s.x *= a;
    m->rot.s.y *= a;
    m->rot.s.z *= a;
    m->rot.f.x *= a;
    m->rot.f.y *= a;
    m->rot.f.z *= a;
    m->rot.u.x *= a;
    m->rot.u.y *= a;
    m->rot.u.z *= a;
    g_game->f1a200 = m;
    g_game->f1a1fc = 0;
    g_dt = f8;
    d->frames.apply(-0x46);
    if (rider > 0) {
        EmitDef* r = (EmitDef*)((char*)g_emit_defs_data + (unsigned)rider * 0x48);
        m2 = g_resmgr.get_object(&r->name)->clone();
        m2->translate(&d->pos);
        if (!(flags & 0x400)) {
            float b = r->scale;
            m2->rot.s.x *= b;
            m2->rot.s.y *= b;
            m2->rot.s.z *= b;
            m2->rot.f.x *= b;
            m2->rot.f.y *= b;
            m2->rot.f.z *= b;
            m2->rot.u.x *= b;
            m2->rot.u.y *= b;
            m2->rot.u.z *= b;
        } else {
            mat3 dm;
            identity3(&dm);
            float ang = 1.5707964f;
            dm.rotate_sf(&ang);
            ang = -cam_pitch;
            dm.rotate_sf(&ang);
            m2->rot = dm;
            float c = r->scale * 1.5f;
            m2->rot.s.x *= c;
            m2->rot.s.y *= c;
            m2->rot.s.z *= c;
            m2->rot.f.x *= c;
            m2->rot.f.y *= c;
            m2->rot.f.z *= c;
            m2->rot.u.x *= c;
            m2->rot.u.y *= c;
            m2->rot.u.z *= c;
        }
        g_game->f1a200 = m2;
        g_game->f1a1fc = 0;
        g_dt = f8;
        d->frames.apply(-0x46);
    }
    if (f59ec == 0) {
        renderer->add_mesh(m);
    } else {
        for (unsigned i = 0; renderer->ents._Myfirst != 0 &&
                          i < (unsigned)(renderer->ents._Mylast - renderer->ents._Myfirst); i++) {
            if (renderer->ents._Myfirst[i] == f59ec) {
                if (renderer->ents._Myfirst[i])
                    renderer->ents._Myfirst[i]->v0(1);
                renderer->ents._Myfirst[i] = m;
            }
        }
    }
    if (f59f0 == 0) {
        if (m2)
            renderer->add_mesh(m2);
    } else {
        for (unsigned i = 0; renderer->ents._Myfirst != 0 &&
                          i < (unsigned)(renderer->ents._Mylast - renderer->ents._Myfirst); i++) {
            if (renderer->ents._Myfirst[i] == f59f0) {
                if (renderer->ents._Myfirst[i])
                    renderer->ents._Myfirst[i]->v0(1);
                if (m2) {
                    renderer->ents._Myfirst[i] = m2;
                } else {
                    unsigned n = (unsigned)(renderer->ents._Mylast - renderer->ents._Myfirst);
                    renderer->ents._Myfirst[i] = renderer->ents._Myfirst[n - 1];
                    renderer->ents._Mylast--;
                }
            }
        }
    }
    f59ec = m;
    f59f0 = m2;
    if (d->f10 & 1) {
        renderer->fl_48(0);
    } else {
        renderer->fl_48(2);
        float one = 1.0f;
        renderer->f4b1800(&one);
        renderer->f_c0 = d->scale * 3.0f;
        renderer->f294 = 0.5f;
    }
    unsigned mask = (flags & 0x400) ? 0x1ffff : 0xffff;
    m->mask = mask;
    if (m2)
        m2->mask = mask;
    renderer->color.x = 0.0f;
    renderer->color.y = 0.0f;
    renderer->color.z = 0.5f;
    renderer->color.w = a;
    float shade = 0.95f;
    renderer->set_color(&shade);
}

// @ 0x4b7e50
void Mission::update_hud_text()
{
    if (f8b4) {
        f8ac = march_time(this);
        g_ev_a = 100;
        g_ev_b = 0;
        g_game->emit_event(0x1a, f8, 0);
        f8b4 = 0;
        f8b5 = 1;
        f8b0 = (float)(*(__int64*)&g_ev_a) * 0.01f;
    }
    if (!f8b5)
        return;
    int ammo = 0;
    vector4 t;
    float v;
    int b = 0, d = 0, c = 0;
    g_game->update_ammo(f8, &ammo, &c, &b, &d);
    GameData* g = g_game;
    v = *(float*)(g->ui + f70 * 0x300 + 0x2c);
    unsigned color;
    if (g_flag_a181b0 != 0) {
        t.x = *(float*)(g->ui + f70 * 0x300 + 0x28);
        if (f6c != 0) {
            vector4 tmp;
            vector4* p = unpack_color(&tmp, (unsigned*)&t.x);
            t.x = p->x * 1.2f;
            t.y = p->y * 1.2f;
            t.z = p->z * 1.2f;
            color = pack_color(&t) | 0xff000000;
        } else {
            color = *(unsigned*)&t.x;
        }
    } else {
        color = g->color_of(v, f6c);
    }
    if (f8 == g->f78) {
        color = 0xffffffff;
    } else if ((flags & 0x40000) != 0) {
        color = 0xffaa22dd;
        if (!(v < -0.01f))
            color = 0xff22aaff;
    }
    char buf[128];
    if (b == 0)
        sprintf(buf, "%d", ammo);
    else
        sprintf(buf, "%d+%d", ammo, b);
    float ct = fa4;
    if ((flags & 0x80000) != 0) {
        tstring str("");
        ct = 1.0f;
        g_game->show_text(f59e8, &str, 0, 1, &ct);
    } else if ((flags & 0x400) != 0) {
        ct = this->cost();
        g_game->show_text(f59e8, get_text(), 1, color, &ct);
    } else {
        ct = this->cost();
        tstring str(buf);
        g_game->show_text(f59e8, &str, 0, color, &ct);
    }
    f8b5 = 0;
}
