// Slice s0052ae70: the mission order markers.
//   0x52ae70  build (once) the two "order" entities: the round/tetrahedron marker built from the shape
//             at Screen+0x6698 into three meshes, and the ground rectangle marker (four points, four
//             vertices, two triangles) of the second entity.
//   0x52b300  order tick: derive the current order id from the local agent's kind, hand it to
//             Mission::order, and when the "give order" state is 1 arm the 7.5 s timeout and repaint.
//   0x52b3c0  paint the five order labels (group / hold position / follow / charge / mount / dismount)
//             through the gui manager, tinting the selected one.
// The Screen layout (+0x29c.. / +0x6698.. / +0x67xx) is shared with slice s005261e0; names are
// provisional and every offset is read off this disassembly.
#include <windows.h>
#include <vector>
#include "rgl.h"

extern "C" int _fltused = 0;

// ---- engine string (0x10 bytes, ctor 0x42bbe0, dtor 0x42bc40) ------------------------------------------
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring(const char* s);                             // 0x42bbe0
    ~tstring();                                         // 0x42bc40
};

// ---- resolved material / resource manager (0x5d6fa0) ---------------------------------------------------
struct material;
struct resmgr {
    material* get_material(tstring* name);              // 0x418a70 (thiscall, ret 4)
};
extern resmgr g_resmgr;                                 // 0x5d6fa0

// ---- gui manager (0x5d7080) ----------------------------------------------------------------------------
struct gui_manager {
    void set_o568(void* obj, int on);                   // 0x42cdb0
    void set_view(const float* v);                      // 0x4c5190
    void draw_string(const tstring* s, float y, unsigned color, int centered);   // 0x4e43e0
};
extern gui_manager g_gui;                               // 0x5d7080

// ---- localisation table (0x5e2a18) ---------------------------------------------------------------------
struct StrTable {
    tstring* lookup(const tstring& key);                // 0x43a1e0 (thiscall, ret 4)
};
extern StrTable g_strtable;                             // 0x5e2a18

// ---- rgl::mesh (0x260 bytes; ctor 0x412a90) ------------------------------------------------------------
namespace rgl {
struct vec2 { float x, y; vec2() {} vec2(float a, float b) : x(a), y(b) {} };

struct mesh {
    mesh();                                             // 0x412a90
    char pad0[0x2c];
    material* res;                                      // +0x2c
    char pad30[0x138 - 0x30];
    char f138;                                          // +0x138 (dirty flag)
    char pad139[0x164 - 0x139];
    char f164;                                          // +0x164 (dirty flag)
    char pad165[0x260 - 0x165];
    int add_point(const vector4* p);                    // 0x412f50
    int add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos,
                   const int* f5c);                     // 0x413c50 (ret 0x14)
    int add_face(int a, int b, int c);                  // 0x411920
};
} // namespace rgl

// ---- entity (created by World::create_entity(9)) --------------------------------------------------------
struct Entity {
    virtual void v0(); virtual void v1(); virtual void v2();
    virtual void v3(); virtual void v4(); virtual void v5();
    virtual void set_scale(const float* s);             // vtable +0x18
    char pad[0x140 - 4];
    unsigned char f140;                                 // +0x140 visible flag
    void add_mesh(rgl::mesh* m);                        // 0x442110 (thiscall, ret 4)
};

struct World {
    Entity* create_entity(int type);                    // 0x428440 (thiscall, ret 4)
};

// the shape owned by the Screen at +0x6698 (0x94 bytes; see s004e65e0 / s004e6a70)
struct Shape {
    char pad[0x94];
    void build_tri(rgl::mesh* m);                       // 0x4e75a0 (thiscall, ret 4)
    void build_ribbon(rgl::mesh* m);                    // 0x4e6a70 (thiscall, ret 4)
    void build_tetra(rgl::mesh* m);                     // 0x4e65e0 (thiscall, ret 4)
};

// ---- mission (0x5e292c) and the game clock -------------------------------------------------------------
struct Agent {
    char pad[0x1c4];
    int kind;                                           // +0x1c4
    char pad1[0x5928 - 0x1c8];
};
struct Mission {
    char pad0[0x18];
    std::vector<Agent*> pages;                          // +0x18 (std::vector; data at +0x1c)
    char pad2c[0x5c - 0x28];
    int local_id;                                       // +0x5c (-1 when no local agent)
    void order(int id, int arg);                        // 0x49c2c0
};
extern Mission* g_mission;                              // 0x5e292c
extern __int64 g_ticks[];                               // 0x5e2948 table of game clocks

extern int g_a5a9fbc;                                   // 0x5a9fbc
extern int g_a1717c;                                    // 0xa1717c
extern int g_a17148;                                    // 0xa17148

// ---- the mission screen --------------------------------------------------------------------------------
struct Screen {
    World* world;                                       // +0
    char pad4[0x29c - 4];
    int f29c;                                           // +0x29c order state
    int f2a0;                                           // +0x2a0 selected order
    char pad2a4[0x2a8 - 0x2a4];
    int f2a8;                                           // +0x2a8 clock index
    char pad2ac[0x2b0 - 0x2ac];
    __int64 f2b0;                                       // +0x2b0 order deadline
    char pad2b8[0x654c - 0x2b8];
    float f654c;                                        // +0x654c marker height
    char pad6550[0x6698 - 0x6550];
    Shape shape;                                        // +0x6698
    rgl::mesh* f672c;                                   // +0x672c
    rgl::mesh* f6730;                                   // +0x6730
    rgl::mesh* f6734;                                   // +0x6734
    Entity*    f6738;                                   // +0x6738
    rgl::mesh* f673c;                                   // +0x673c
    Entity*    f6740;                                   // +0x6740

    void init_order_markers();                          // 0x52ae70
    void order_tick();                                  // 0x52b300
    void draw_order_menu();                             // 0x52b3c0
};

// shape builders (all take the shape in ecx and the target mesh on the stack)

// @ 0x52ae70
void Screen::init_order_markers()
{
    if (f6738 == 0) {
        f6738 = world->create_entity(9);
        float scale = 100000.0f;
        f6738->set_scale(&scale);

        f6730 = new rgl::mesh;
        f6734 = new rgl::mesh;
        f672c = new rgl::mesh;

        {
            tstring name("vertex_color_blend_mat");
            f6730->res = g_resmgr.get_material(&name);
        }
        f6734->res = f6730->res;
        f672c->res = f6730->res;

        f6738->add_mesh(f6730);
        f6738->add_mesh(f6734);
        f6738->add_mesh(f672c);

        Shape* s = &shape;
        s->build_tri(f672c);
        s->build_ribbon(f6734);
        s->build_tetra(f6730);

        scale = 100000.0f;
        f6738->set_scale(&scale);
        f6738->f140 = 0;
    }

    if (f6740 == 0) {
        f6740 = world->create_entity(9);

        f673c = new rgl::mesh;
        {
            tstring name("vertex_color_blend_mat");
            f673c->res = g_resmgr.get_material(&name);
        }
        f6740->add_mesh(f673c);

        rgl::vector4 p;
        p.x = -1000.0f; p.y = -1000.0f; p.z = f654c;
        int p0 = f673c->add_point(&p);
        p.x = 1000.0f; p.y = -1000.0f; p.z = f654c;
        int p1 = f673c->add_point(&p);
        p.x = 1000.0f; p.y = 1000.0f; p.z = f654c;
        int p2 = f673c->add_point(&p);
        p.x = -1000.0f; p.y = 1000.0f; p.z = f654c;
        int p3 = f673c->add_point(&p);

        int v0, v1, v2, v3;
        rgl::vec2 uv;
        unsigned color;
        int zero;
        uv.x = 0.0f; uv.y = 0.0f; color = 0x663333ff; p.x = 0.0f; p.y = 0.0f; p.z = 1.0f; zero = 0;
        v0 = f673c->add_vertex(p0, &uv, &color, &p, &zero);
        uv.x = 0.0f; uv.y = 0.0f; color = 0x663333ff; p.x = 0.0f; p.y = 0.0f; p.z = 1.0f; zero = 0;
        v1 = f673c->add_vertex(p1, &uv, &color, &p, &zero);
        uv.x = 0.0f; uv.y = 0.0f; color = 0x663333ff; p.x = 0.0f; p.y = 0.0f; p.z = 1.0f; zero = 0;
        v2 = f673c->add_vertex(p2, &uv, &color, &p, &zero);
        uv.x = 0.0f; uv.y = 0.0f; color = 0x663333ff; p.x = 0.0f; p.y = 0.0f; p.z = 1.0f; zero = 0;
        v3 = f673c->add_vertex(p3, &uv, &color, &p, &zero);

        f673c->add_face(v0, v1, v2);
        f673c->add_face(v0, v2, v3);

        f673c->f164 = 1;
        f673c->f138 = 1;

        float scale = 100000.0f;
        f6740->set_scale(&scale);
        f6740->f140 = 1;
    }
}

// @ 0x52b300
void Screen::order_tick()
{
    unsigned id = g_mission->local_id;
    if ((int)id < 0)
        return;

    Mission* m = g_mission;
    Agent* a = m->pages[id >> 4] + (id & 0xf);
    int kind = a->kind;
    if (kind < 0 || kind >= 8)
        kind = 7;
    if (g_a5a9fbc && g_a1717c && g_a17148)
        kind = 1;

    m->order(kind, f2a0);

    if (f29c == 1) {
        f2b0 = g_ticks[f2a8] - 750000;
        f29c = 2;
        draw_order_menu();
    }
}

// @ 0x52b3c0
void Screen::draw_order_menu()
{
    {
        float view[2];
        view[0] = 0.5f;
        view[1] = 0.4f;
        g_gui.set_o568(0, 1);
        g_gui.set_view(view);
    }

    {
        tstring s("ui_order_group");
        g_gui.draw_string(g_strtable.lookup(s), 0.035f, 0xfff0e0d0, 1);
    }

    unsigned base = (f29c == 1) ? 0xffffffff : 0xff50ff60;

    unsigned c;
    c = (f2a0 == 0) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_hold_position");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }

    c = (f2a0 == 1) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_follow");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }

    c = (f2a0 == 2) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_charge");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }

    c = (f2a0 == 3) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_mount");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }

    c = (f2a0 == 4) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_dismount");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }
}
