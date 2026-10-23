// Slice s004fb350: the agent status widget (health bar, shield piece, ammo counter) and its ammo-mesh helper.
// The object at 0xa76630 is a thiscall object (member functions; also called from 0x4fa790). Names provisional.
// Reuses the verified rgl::mesh / resmgr / tstring layouts (s0041bf40/mesh.h, s00454320/agent_fx.h).
#include <vector>
#include <stddef.h>
#include <stdio.h>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// ---- strings -------------------------------------------------------------------------------------------
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() {}
    tstring(const char* s);                                 // 0x42bbe0
    ~tstring();                                             // 0x42bc40
};

// ---- rgl::mesh (0x260 bytes, verified in s0041bf40/mesh.h) ---------------------------------------------
struct resource;
struct vertex60 {                                           // 0x60 bytes (mesh+0x54)
    vector4 pos, f10, f20, normal;                          // 0x00..0x40
    float u, v;                                             // 0x40, 0x44
    int f48, f4c;
    int index;                                              // 0x50
    unsigned color;                                         // +0x54
    int f58, f5c;
};
struct point20 { vector4 a, b; };                           // 0x20 (mesh+0x74)
struct frame_t { std::vector<vector4> verts, norms; };      // 0x20
struct keyframes { frame_t* frames; float* times; int count; };   // 0x0c (mesh+0xa4)
struct mesh {
    virtual ~mesh();                                        // 0
    char pad4[0x2c - 4];
    resource* res;                                          // +0x2c
    char pad30[0x3c - 0x30];
    unsigned color;                                         // +0x3c
    char pad40[0x50 - 0x40];
    std::vector<vertex60> v50;                              // +0x50 (VC7.1 vector: _Myfirst at +4 -> begin +0x54)
    char pad60[0x70 - 0x60];
    std::vector<point20> v70;                               // +0x70 (_Myfirst at +4 -> begin +0x74)
    char pad80[0xa4 - 0x80];
    keyframes kf;                                           // +0xa4 (count at +0xac)
    char padb0[0x138 - 0xb0];
    char f138;                                              // +0x138
    void clear_geometry();                                  // 0x428e80
    void append_skinned(mesh* other, const matrix* m, int bone, unsigned color, char flip);  // 0x44eb00
    void scale(float s);                                    // 0x44f0c0
    void set_color(vector4 c);                              // 0x47af30
    void set_bone_index(int i);                             // 0x47b360
    void ensure_local();                                    // 0x412ee0
    mesh* clone();                                          // 0x41c6d0
    void set_offset(const vector4& pos);                    // 0x4fb8e0
    void recalc();                                          // 0x419300 (thiscall, ret 0)
    void transform(const struct pos* frame);                // 0x495ba0 (thiscall, ret 4)
};
struct resmgr { mesh* get_object(const tstring& name);      // 0x419540
                resource* get_material(const tstring& name); };  // 0x418a70
extern resmgr g_resmgr;                                     // 0x5d6fa0

struct pos : matrix {
    void rotate_sf(const float* angle);                     // 0x40e770 (thiscall: rotates the side/forward rows)
};
pos* identity_pos(pos* out);                                // 0x40cd00 (cdecl)
void identity3(pos* out);                                   // 0x40cd60 (cdecl)

// ---- the mission and its agents ------------------------------------------------------------------------
struct AgentTable {
    char pad0[0x18];
    void** pages;                                           // +0x18
};
struct Mission {
    int f0;
    AgentTable tbl;                                         // +4
    char pad_[0x5c - 4 - sizeof(AgentTable)];
    int local_id;                                           // +0x5c: the player's agent id
};
extern Mission* g_mission;                                  // 0x5e292c

// agent: the held item's scene node at +0x250, health at +0x5720 / +0x5724
struct Agent {
    char pad0[0x250];
    void* node;                                             // +0x250
    char pad254[0x5720 - 0x254];
    float max_health;                                       // +0x5720
    float health;                                           // +0x5724
    struct Pair { int a, b; };
    Pair attr_pair(int slot);                               // 0x432790 (thiscall, hidden 8-byte result = Agent::slot_pair)
    void attr_vec2(int idx, int* out1, int* out2);          // 0x436810 (thiscall, ret 0xc)
    Pair attr_vec4(int idx, int* a, int* b, int* c);        // 0x436880 (thiscall, hidden 8-byte result)
};
struct Game {
    char pad0[0x1a20c];
    int status_mode;                                        // +0x1a20c
};
extern Game* g_game;                                        // 0x5e2930

// a scene node / ui element: show/hide (0xb0/0xb4), mesh set (0x80, ret 8), visibility (0x84),
// colour (0x98, ret 4), text (0xa4, ret 8)
struct WRet { int a, b; };                                  // set_text's hidden 8-byte result (never read here)
struct Ui {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23(); virtual void d24();
    virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28(); virtual void d29();
    virtual void d30(); virtual void d31();
    virtual void set_mesh(int a, mesh* m);                  // +0x80 (ret 8)
    virtual int visible();                                  // +0x84
    virtual void d34(); virtual void d35(); virtual void d36(); virtual void d37();
    virtual void set_color(const unsigned* c);              // +0x98 (ret 4)
    virtual void d39(); virtual void d40();
    virtual void set_text(WRet* out, const tstring* text);  // +0xa4 (ret 8, hidden result)
    virtual void d42(); virtual void d43();
    virtual void hide();                                    // +0xb0
    virtual void show();                                    // +0xb4
};

// the 0xa76630 object (layout provisional; only the members touched here)
struct StatusWidget {
    char pad0[0xbc];
    Ui* e_bc;                                               // +0xbc shield text (cur)
    Ui* e_c0;                                               // +0xc0 shield text (max)
    Ui* e_c4;                                               // +0xc4 name/status node
    Ui* e_c8;                                               // +0xc8 shield icon
    Ui* e_cc;                                               // +0xcc shield piece
    char padd0[0xd4 - 0xd0];
    mesh* m_d4;                                             // +0xd4 shield piece mesh
    mesh* m_d8;                                             // +0xd8 shield shadow mesh
    mesh* m_dc;                                             // +0xdc health bar mesh
    int f_e0;                                               // +0xe0 shown health
    int f_e4;                                               // +0xe4 shield max
    int f_e8;                                               // +0xe8 shield cur
    int f_ec;                                               // +0xec shield cur (y)
    int f_f0;                                               // +0xf0 ammo count
    int f_f4;                                               // +0xf4 ammo icon stage
    void update();                                          // 0x4fb350
};

// ammo mesh from an item def (cdecl, ret 0x8); def[0] is the item kind, def[1] the modifier
mesh* ammo_mesh(int* def, const vector4* pos);              // 0x4fba40

// item attribute block of an agent (pairs and 0x24-byte records)

// @ 0x4fb350
void StatusWidget::update()
{
    int id = g_mission->local_id;
    if (id < 0)
        return;
    Agent* ag = (Agent*)((char*)g_mission->tbl.pages[id >> 4] + (id & 0xf) * 0x5928);
    if (g_game->status_mode)
        e_c4->hide();
    else
        e_c4->show();
    pos frame;
    int hp = (int)ag->health;
    if (hp != f_e0) {
        f_e0 = hp;
        m_dc->clear_geometry();
        {
            tstring name("status_health_bar");
            pos* m = identity_pos(&frame);
            mesh* obj = g_resmgr.get_object(name);
            m_dc->append_skinned(obj, m, -1, -1, 0);
        }
        vector4 col((float)f_e0 / ag->max_health, 1.0f, 1.0f);
        m_dc->set_color(col);
    }

    // shield: attr_pair(0) says whether the agent has one; attr_vec4 then gives (max, ?, ?) and the (cur, flags) pair
    Agent::Pair pair;
    int la = -1, lb = -1, lc;
    Agent::Pair res;
    res.a = -1;
    pair = ag->attr_pair(0);
    if (pair.a >= 0)
        res = ag->attr_vec4(-1, &la, &lb, &lc);
    if (la != f_e4 || res.a != f_e8) {
        f_e4 = la;
        char buf[512];
        WRet wr;
        sprintf(buf, "%d", la);
        {
            tstring t1(buf);
            e_bc->set_text(&wr, &t1);
        }
        {
            tstring t2(buf);
            e_c0->set_text(&wr, &t2);
        }
        if (f_e4 < 0) {
            e_cc->hide();
            e_bc->hide();
            e_c0->hide();
        } else {
            e_cc->show();
            e_bc->show();
            e_c0->show();
            unsigned color = 0xffffffff;
            if (f_e4 <= 0)
                color = 0xffb02020;
            e_bc->set_color(&color);
        }
        if (res.a != f_e8) {
            m_d4->clear_geometry();
            m_d8->clear_geometry();
            if (res.a >= 0) {
                vector4 at;
                at.x = 0.0550000005f;
                mesh* piece = ammo_mesh((int*)&res, &at);
                if (piece) {
                    float v;
                    v = 0.0005f;
                    piece->set_offset(*(const vector4*)&v);
                    identity3(&frame);
                    float ang = -1.45f;
                    frame.rotate_sf(&ang);
                    piece->transform(&frame);
                    identity_pos(&frame);
                    frame.o.z -= 10.0f;
                    m_d4->append_skinned(piece, &frame, -1, -1, 0);
                    m_d4->res = piece->res;
                    frame.o.x += 0.0012f;
                    frame.o.y -= 0.001f;
                    frame.o.z -= 10.0f;
                    piece->scale(1.05f);
                    m_d8->append_skinned(piece, &frame, -1, -1, 0);
                    {
                        tstring shadow("ui_item_shadow");
                        m_d8->res = g_resmgr.get_material(shadow);
                    }
                    m_d8->set_bone_index(-1);
                    m_d8->color = 0xff777777;
                    delete piece;
                }
            }
            f_e8 = res.a;
            f_ec = res.b;
        }
    }

    // ammo count: (count, capacity) from attr_vec2
    Agent::Pair ammo_pair;
    int lo = -1, hi = -1;
    ammo_pair = ag->attr_pair(1);
    if (ammo_pair.a >= 0)
        ag->attr_vec2(-1, &lo, &hi);
    if (lo != f_f0) {
        f_f0 = lo;
        if (lo == -1) {
            f_f4 = -1;
            e_c8->hide();
            return;
        }
        int stage = (int)((float)lo * 5.0f / (float)hi + 0.999f);
        if (stage != f_f4) {
            f_f4 = stage;
            if (stage <= 0) {
                e_c8->hide();
                return;
            }
            const char* nm;
            if (stage == 1)
                nm = "status_shield_20";
            else if (stage == 2)
                nm = "status_shield_40";
            else if (stage == 3)
                nm = "status_shield_60";
            else if (stage == 4)
                nm = "status_shield_80";
            else
                nm = "status_shield_100";
            mesh* cl;
            {
                tstring icon(nm);
                mesh* obj = g_resmgr.get_object(icon);
                cl = obj->clone();
            }
            e_c8->set_mesh(0, cl);
            e_c8->show();
        }
    }
}

// @ 0x4fb8e0: shift every point of the ammo mesh by the scaled vertex normal, and every morph
// key frame's vertex by its scaled normal.
void mesh::set_offset(const vector4& pos)
{
    ensure_local();
    for (unsigned i = 0; v50.begin() != NULL && i < v50.size(); i++) {
        int idx = v50[i].index;
        point20& p = v70[idx];
        p.a.x += v50[i].normal.x * pos.x;
        p.a.y += v50[i].normal.y * pos.x;
        p.a.z += v50[i].normal.z * pos.x;
        p.b.x += v50[i].normal.x * pos.x;
        p.b.y += v50[i].normal.y * pos.x;
        p.b.z += v50[i].normal.z * pos.x;
        for (int j = 0; j < kf.count; j++) {
            frame_t& f = kf.frames[j];
            f.verts[idx].x += f.norms[i].x * pos.x;
            f.verts[idx].y += f.norms[i].y * pos.x;
            f.verts[idx].z += f.norms[i].z * pos.x;
        }
    }
    f138 = 1;
    recalc();
}
