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
};
struct resmgr { mesh* get_object(const tstring& name);      // 0x419540
                resource* get_material(const tstring& name); };  // 0x418a70
extern resmgr g_resmgr;                                     // 0x5d6fa0

int _ftol2();                                               // 0x535a64 (lib_crt 0640_ftol2.obj: x87 -> int)
struct pos : matrix {};
pos* identity_pos(pos* out);                                // 0x40cd00 (cdecl)
void identity3(pos* out);                                   // 0x40cd60 (cdecl)
void matrix_rotate_sf(matrix* m, const float* angle);       // 0x40e770
void mesh_transform(mesh* m, const pos* frame);             // 0x495ba0

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
    float health;                                           // +0x5720
    float max_health;                                       // +0x5724
};
struct Game {
    char pad0[0x1a20c];
    int status_mode;                                        // +0x1a20c
};
extern Game* g_game;                                        // 0x5e2930

// a scene node / ui element: show/hide (0xb0/0xb4), mesh set (0x80, ret 8), visibility (0x84),
// colour (0x98, ret 4), text (0xa4, ret 8)
struct UiVt {
    char pad0[0x80];
    void* (*set_mesh)(void* self, int a, mesh* m);          // 0x80 (ret 8)
    int (*visible)(void* self);                             // 0x84
    char pad88[0x98 - 0x88];
    void (*set_color)(void* self, unsigned c);              // 0x98 (ret 4)
    char pad9c[0xa4 - 0x9c];
    void* (*set_text)(void* self, const tstring* text);     // 0xa4 (ret 8)
    char pada8[0xb0 - 0xa8];
    void (*hide)(void* self);                               // 0xb0
    void (*show)(void* self);                               // 0xb4
};
struct Ui { UiVt* vt; };

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
void FUN_004f3e10(const vector4* pos);                      // 0x4f3e10 (thiscall on 0xa76630)

// item attribute block of an agent (pairs and 0x24-byte records)
void attr_pair(Agent* a, int slot, int* out);               // 0x432790 (thiscall, ret 0xc)
void attr_vec2(Agent* a, int idx, int* out1, int* out2);    // 0x436810 (thiscall, ret 0xc)
int* attr_vec4(Agent* a, int idx, int* a2, int* a3, int* a4);  // 0x436880 (thiscall, ret 0x10)

// @ 0x4fb350
void StatusWidget::update()
{
    int id = g_mission->local_id;
    if (id < 0)
        return;
    Agent* ag = (Agent*)((char*)g_mission->tbl.pages[id >> 4] + (id & 0xf) * 0x5928);
    if (g_game->status_mode)
        e_c4->vt->show(e_c4);
    else
        e_c4->vt->hide(e_c4);
    int hp = (int)(ag->health * 1.0f);
    if (hp != f_e0) {
        f_e0 = hp;
        m_dc->clear_geometry();
        tstring name("status_health_bar");
        pos mat;
        pos* m = identity_pos(&mat);
        mesh* obj = g_resmgr.get_object(name);
        m_dc->append_skinned(obj, m, -1, -1, 0);
        vector4 col((float)f_e0 / ag->max_health, 1.0f, 1.0f);
        m_dc->set_color(col);
    }

    // shield counts: pair (cur, max); a negative max hides the shield
    int pair[2];
    pair[0] = -1;
    pair[1] = -1;
    attr_pair(ag, 0, pair);
    int cur = -1, flags = -1;
    if (pair[0] >= 0) {
        int a2, a3, a4;
        int* p = attr_vec4(ag, -1, pair, &pair[1], &a3);
        cur = p[0];
        flags = p[1];
    }
    if (pair[0] != f_e4 || cur != f_e8) {
        f_e4 = pair[0];
        char buf[512];
        sprintf(buf, "%d", pair[0]);
        tstring t1(buf);
        e_bc->vt->set_text(e_bc, &t1);
        t1.~tstring();
        tstring t2(buf);
        e_c0->vt->set_text(e_c0, &t2);
        t2.~tstring();
        if (f_e4 < 0) {
            e_cc->vt->hide(e_cc);
            e_bc->vt->hide(e_bc);
            e_c0->vt->hide(e_c0);
        } else {
            e_cc->vt->show(e_cc);
            e_bc->vt->show(e_bc);
            e_c0->vt->show(e_c0);
            unsigned color = 0xffffffff;
            if (f_e4 < 1)
                color = 0xffb02020;
            e_bc->vt->set_color(e_bc, color);
        }
        if (cur != f_e8) {
            m_d4->clear_geometry();
            m_d8->clear_geometry();
            if (cur >= 0) {
                int def[2];
                pos at;
                def[0] = cur;
                def[1] = 0;
                at.s.x = 0.0550000005f;
                mesh* piece = ammo_mesh(def, (const vector4*)&at);
                if (piece) {
                    pos frame;
                    at.s.x = 0.001f;
                    piece->set_offset(at.s);
                    identity3(&frame);
                    at.s.x = -1.45f;
                    matrix_rotate_sf(&frame, &at.s.x);
                    mesh_transform(piece, &frame);
                    identity_pos(&frame);
                    frame.o.x -= 10.0f;
                    m_d4->append_skinned(piece, &frame, -1, -1, 0);
                    m_d4->res = piece->res;
                    frame.o.y += 0.0012f;
                    frame.o.z -= 0.001f;
                    frame.o.x -= 10.0f;
                    piece->scale(1.05f);
                    m_d8->append_skinned(piece, &frame, -1, -1, 0);
                    tstring shadow("ui_item_shadow");
                    m_d8->res = g_resmgr.get_material(shadow);
                    shadow.~tstring();
                    m_d8->set_bone_index(-1);
                    m_d8->color = 0xff777777;
                    delete piece;
                }
            }
            f_e8 = cur;
            f_ec = flags;
        }
    }

    // ammo count
    int ammo_pair[2];
    ammo_pair[0] = -1;
    ammo_pair[1] = -1;
    int count = -1;
    attr_pair(ag, 1, ammo_pair);
    if (ammo_pair[0] >= 0) {
        int lo, hi;
        attr_vec2(ag, -1, &lo, &hi);
        count = lo;
    }
    bool shown = false;
    if (count != f_f0) {
        f_f0 = count;
        if (count == -1) {
            f_f4 = -1;
            e_c8->vt->hide(e_c8);
        } else {
            int stage = (int)((float)count * 4.0f / (float)ammo_pair[1] + 0.5f);
            if (stage != f_f4) {
                f_f4 = stage;
                if (stage <= 0) {
                    e_c8->vt->hide(e_c8);
                } else {
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
                    tstring icon(nm);
                    mesh* obj = g_resmgr.get_object(icon);
                    mesh* cl = obj->clone();
                    icon.~tstring();
                    e_c8->vt->set_mesh(e_c8, 0, cl);
                    e_c8->vt->show(e_c8);
                }
            }
        }
        shown = true;
    }
    if (shown && f_f0 < 0) {
        pos at;
        at.s.x = 0.0550000005f;
        FUN_004f3e10((const vector4*)&at);
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
