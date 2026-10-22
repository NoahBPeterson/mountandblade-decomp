// Slice s00520d60 (batch ds1).
//   0x520d60  GameState::build_scene -- the map scene: clears/rebuilds the map page grid, resets the fog and
//             sun colours, spawns the wind record, the map-track entity + mesh, the map-object entity (the
//             "compass" resource cloned and scaled) and the compass entity, then steps the world, adds the
//             party order arrows, clears the expired orders and finally runs the per-scene queries.
//   0x5213a0  GameState::release_orders -- walks the 0x5c-byte order array [begin,end) and destroys each.
// Class layouts copied from s0051f1a0 (GameState) / s00463790 (GameState) / s004902a0 (PartyMgr) /
// s00428240 (World, entity, mesh). Names provisional; every offset is read off this slice's disassembly.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

// ---- tstring (0x42bbd0 / 0x42bbe0 / 0x42bc40) -------------------------------------------------------
struct tstring {
    int pos;                                        // +0
    char* buf;                                      // +4
    int cap;                                        // +8
    int len;                                        // +0xc
    tstring();                                      // 0x42bbd0
    tstring(const char* s);                         // 0x42bbe0
    ~tstring();                                     // 0x42bc40
    void reserve(unsigned n);                       // 0x42bb50
};
void mb_error(const char* fmt, ...);                // 0x42cc80 (cdecl)

// ---- text stream base (vtable 0x54ac88 family) ------------------------------------------------------
struct stream_base {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9();
    virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13();
    virtual stream_base& r_float(float* v);         // 14 (+0x38)
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18();
    virtual stream_base& r_int(int* v);             // 19 (+0x4c)
};
struct tstream3 : stream_base {                     // vtable 0x54ac88
    char pad0[0x30 - 4];
    FILE* fp;                                       // +0x30
    char failed;                                    // +0x34
    bool open_read_text(const char* path);          // 0x415d60 (ret 4)
};

// ---- game records -----------------------------------------------------------------------------------
struct mesh;
struct entity;
struct Wind {                                       // 0x7c-byte world wind/effect record (0x461480)
    int type;                                       // +0
    vector4 a;                                      // +4
    char pad14[0x24 - 0x14];
    vector4 b;                                      // +0x24
    char pad34[0x44 - 0x34];
    vector4 c;                                      // +0x44
    float f54;                                      // +0x54
    char pad58[0x7c - 0x58];
    Wind();                                         // 0x461480
};

struct World {
    char pad0[0x68];
    vector4 fog;                                    // +0x68
    float f74;                                      // +0x74
    entity* track;                                  // +0x78
    char pad7c[0xbc - 0x7c];
    std::vector<Wind*> winds;                       // +0xbc
    char padc8[0x1d8 - 0xc8];
    entity* create_entity(int type);                // 0x428440 (ret 4)
    void step(float dt);                            // 0x427f40 (ret 4)
    void destroy_obj(void* o);                      // 0x41f450 (ret 4)
    void finish_mesh();                             // 0x4207e0 (ret 0)
    void finish_one(void* o);                       // 0x4207a0 (ret 4)
};
extern World* g_scene;                              // 0xa732b4

struct mesh {
    void* vptr;                                     // +0
    char pad4[0x2c - 4];
    void* res;                                      // +0x2c
    char pad30[0x10c - 0x30];
    int mask;                                       // +0x10c
    char pad110[0x260 - 0x110];
    mesh();                                         // 0x412a90
    mesh* clone();                                  // 0x41c6d0 (ret 0)
    void scale(float s);                            // 0x44f0c0 (ret 4)
};

struct entity {                                     // vtable 0x54b460
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
    virtual void v4(); virtual void v5();
    virtual void set_fbc(const float* p);           // 6 (+0x18)
    void add_mesh(mesh* m);                         // 0x442110 (ret 4)
};

struct resmgr {
    void* get_material(const tstring& name);        // 0x418a70 (ret 4)
    mesh* get_object(const tstring& name);          // 0x419540 (ret 4)
};
extern resmgr g_resmgr;                             // 0x5d6fa0
extern tstring* g_compass_name;                     // 0x5e28f4

struct log_stream {                                 // the static log stream at 0x5a9f60
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9();
    virtual void w_cstr(const char* s);             // 10 (+0x28)
};
extern log_stream g_log;                            // 0x5a9f60

// ---- party manager (at GameState+0xf8; layout from s0051f1a0) ---------------------------------------
struct PartyMgr {
    int first_party_id();                           // 0x460d20 (ret 0)
    void next_party_id(int* id);                    // 0x460ca0 (ret 4)
    char pad[0x138dc];
};

// ---- paged list at GameState+0xcc (0x98-byte elements, 16 per page) ----------------------------------
struct Container {
    int count;                                      // +0
    char pad4[0x18];
    void** pages;                                   // +0x18
    void next(int* idx);                            // 0x517960 (ret 4)
};

struct GameState {
    char       pad0[0x2c];
    mesh*      map_track;                           // +0x2c
    entity*    track_entity;                        // +0x30
    char       pad34[0x10];
    mesh*      map_obj;                             // +0x44
    vector4    fog1;                                // +0x48
    vector4    fog2;                                // +0x58
    vector4    fog3;                                // +0x68
    char       pad78[0xcc - 0x78];
    Container  orders;                              // +0xcc
    char       pade8[0xf8 - 0xcc - sizeof(Container)];
    PartyMgr   mgr;                                 // +0xf8
    unsigned   f139d4;                              // +0x139d4
    char       pad139d8[4];
    Wind*      wind;                                // +0x139dc
    char       pad139e0[0x13a34 - 0x139e0];
    int        f13a34;                              // +0x13a34
    char       pad13a38[0x13a64 - 0x13a38];
    char*      order_begin;                         // +0x13a64
    char*      order_end;                           // +0x13a68
    char       pad13a6c[0x1a2a8 - 0x13a6c];
    entity*    compass;                             // +0x1a2a8
    __declspec(noinline) void release_orders();     // 0x5213a0
    void       build_scene();                       // 0x520d60
    void       build_map_trees();                   // 0x5213d0
    void       destroy_order(void* p);              // 0x4b36a0
    void       more_start();                        // 0x51f800
    vector4*   fog_query_a(vector4* out);           // 0x5176e0
    vector4*   fog_query_b(vector4* out);           // 0x517820
    void       add_order_arrow(int id);             // 0x49bea0
    void       del_order(int id);                   // 0x49bbb0
    void       sl_query(int a, int b, int c);       // 0x435790
};
extern GameState* g_game;                           // 0x5e2930

// page array of the map (void* cells, column stride 0x20, row stride 4)
extern char g_page_base[];                          // 0xa73094
extern int  g_page_cols;                            // 0xa73294
extern int  g_page_rows;                            // 0xa73298
extern char g_map_loaded;                           // 0xa73090

// std::vector push helper (as in s00428240/world.h)
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

// @ 0x520d60 -----------------------------------------------------------------------------------------
void GameState::build_scene()
{
    if (g_map_loaded) {
        for (int i = 0; i < g_page_rows; i++) {
            char* row = g_page_base + i * 4;
            for (int j = 0; j < g_page_cols; j++) {
                char* cell = row + j * 0x20;
                void* p = *(void**)cell;
                if (p) {
                    g_scene->destroy_obj(p);
                    *(void**)(cell + 0x100) = *(void**)cell;
                    *(void**)cell = 0;
                }
            }
        }
        g_scene->finish_mesh();
    }
    g_log.w_cstr("map mesh built.\n");
    vector4 tmp;
    fog1 = *fog_query_a(&tmp);
    g_log.w_cstr("get_ideal_sun_color.\n");
    fog2 = *fog_query_b(&tmp);
    g_log.w_cstr("get_ideal_fog_color.\n");
    fog1 = vector4(0.5f, 0.48f, 0.47f);
    fog2 = vector4(0.8f, 0.8f, 0.8f);
    fog3 = vector4(0.35f, 0.37f, 0.4f);
    g_scene->fog = vector4(0.35f, 0.37f, 0.4f);
    g_scene->f74 = 1.0f;

    Wind* w = new Wind();
    vpush(g_scene->winds, w);
    wind = w;
    wind->type = 2;
    wind->a = fog1;
    wind->f54 = 30000.0f;
    wind->b = vector4(700.0f, 200.0f, 900.0f);
    wind->c = vector4(-0.8f, -0.6f, -1.0f);
    float len = wind->c.length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        wind->c.x = wind->c.x * inv;
        wind->c.y = wind->c.y * inv;
        wind->c.z = wind->c.z * inv;
    } else {
        wind->c.x = 0.0f;
        wind->c.y = 1.0f;
        wind->c.z = 0.0f;
    }

    entity* track = g_scene->create_entity(9);
    track_entity = track;
    float d = 1000000.0f;
    track->set_fbc(&d);
    map_track = new mesh();
    map_track->res = g_resmgr.get_material(tstring("map_track"));
    track_entity->add_mesh(map_track);

    entity* obj = g_scene->create_entity(9);
    map_obj = g_resmgr.get_object(*g_compass_name)->clone();
    map_obj->scale(60.0f);
    map_obj->mask = 0xfffeffff;
    obj->add_mesh(map_obj);
    d = 100000000.0f;
    obj->set_fbc(&d);
    g_scene->track = obj;

    compass = g_scene->create_entity(9);
    mesh* mc = g_resmgr.get_object(tstring("compass"))->clone();
    mc->scale(0.28f);
    compass->add_mesh(mc);

    if (!g_map_loaded) {
        build_map_trees();
    } else {
        for (int i = 0; i < g_page_rows; i++) {
            char* row = g_page_base + i * 4;
            for (int j = 0; j < g_page_cols; j++) {
                char* cell = row + j * 0x20;
                *(void**)cell = *(void**)(cell + 0x100);
                *(void**)(cell + 0x100) = 0;
                g_scene->finish_one(*(void**)cell);
            }
        }
    }

    g_scene->step(0.0f);

    unsigned id = (unsigned)mgr.first_party_id();
    if (id < f139d4) {
        do {
            add_order_arrow((int)id);
            mgr.next_party_id((int*)&id);
        } while (id < f139d4);
    }
    g_log.w_cstr("parties added.\n");

    int idx = 0;
    if (orders.count > 0) {
        do {
            if (*(int*)((char*)orders.pages[idx >> 4] + (idx & 0xf) * 0x98 + 4) != 0)
                break;
            idx++;
        } while (idx < orders.count);
    }
    if (idx < orders.count) {
        do {
            del_order(idx);
            orders.next(&idx);
        } while (idx < orders.count);
    }
    release_orders();
    more_start();
    if (f13a34 == 0) {
        sl_query(0, 0, 0);
        f13a34 = 1;
    }
}

// @ 0x5213a0 -----------------------------------------------------------------------------------------
void GameState::release_orders()
{
    char* p = order_begin;
    if (p < order_end) {
        do {
            destroy_order(p);
            p += 0x5c;
        } while (p < order_end);
    }
}
