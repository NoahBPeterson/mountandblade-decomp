// Slice s0051a2d0: seven functions from the map-screen / GUI translation unit.
//   0x51a2d0  PlaneTrace::intersect        (segment/plane intersection, x87 inlined vector math)
//   0x51a3a0  Picker::collect              (collect objects whose v1() bit-mask contains the key)
//   0x51a420  MapPickScreen::set_label         (rebuild the map party label + tooltip)
//   0x51a690  Camera::project              (matrix transform + viewport mapping, 2 floats out)
//   0x51a760  gui_manager::set_pos         (clamp a label offset inside the mesh bounds)
//   0x51a880  gui_manager::add_label       (lay out one text row into the "ui_font" mesh)
//   0x51aa20  gui_manager::set_quad        (write the 4 corner points of the label background)
// Names and field names are provisional; every offset is read off the disassembly.
#include <vector>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- engine types ---------------------------------------------------------------------------------------
#include "../tstring.h"

void* operator new(unsigned size);      // 0x53545e

#include "../str_table.h"
extern StrTable g_strtable;             // 0x5e2a18

#include "../res_mgr.h"                         // resmgr (0x5d6fa0): the one definition for the rgl.h-universe modules
extern resmgr g_resmgr;                 // 0x5d6fa0

#include "../font.h"
extern Font* g_font;                    // 0x5e2934

// ---- the 0x260-byte mesh instance (vtable 0x54a6f4, ctor 0x412a90) ---------------------------------------
#include "../../include/mesh_prims.h"
using rgl::point20;
#include "../rgl_mesh.h"
using rgl::mesh;

// ---- globals --------------------------------------------------------------------------------------------
extern int   g_5d8784;          // 0x5d8784 (zeroed with the current label object)
extern int   g_flag_a181b0;     // 0xa181b0 (alternate label-colour mode)
extern char  g_ui_mode;         // 0xa181b8 (show raw translation tokens)

struct GuiObj { virtual ~GuiObj(); };   // deleted through vtable slot 0
extern GuiObj* g_label_obj;     // 0x5d75ec (current label object)

// the GUI root singleton at 0x5d7080 (layout from s0042c9b0 / s0042d360)
struct gui_manager {
    char pad0[0x56c];
    mesh* o56c;                     // +0x56c (the label mesh, allocated lazily)
    mesh m570;                      // +0x570 (0x260-byte inline mesh)
    char pad7d0[0x16f8 - 0x7d0];
    float f16f8;                    // +0x16f8 (label x offset)
    float f16fc;                    // +0x16fc (label y offset)
    char pad1700[0x1704 - 0x1700];
    float f1704;                    // +0x1704 (running vertical cursor)

    void set_pos(const float* p);                               // 0x51a760
    void add_label(const tstring* text, float h, int color);    // 0x51a880
    void set_quad(const float* origin, const float* size);      // 0x51aa20
};
extern gui_manager g_gui;           // 0x5d7080

// ---- game data ------------------------------------------------------------------------------------------

#include "../../include/game_data.h"

// ---- 0x51a2d0 -------------------------------------------------------------------------------------------
// a plane record: point at +0x20, normal at +0xa0 (it is the face shape of include/shape.h, whose intersect() is the same function;
// kept as this local view because the harness cannot build a polymorphic `this` for the verified PASS row of 0x51a2d0)
struct PlaneTrace {
    char pad0[0x20];
    vector4 pt;                         // +0x20
    char pad30[0xa0 - 0x30];
    vector4 nrm;                        // +0xa0
    void intersect(vector4* out, const vector4* a, const vector4* b);   // 0x51a2d0 ret 0xc
};

// @ 0x51a2d0
// `volatile` on d (and on py) plus the right-associative numerator reproduce three distinct
// properties of the original: d lives in its own float32 frame slot (so the frame is `sub esp`,
// not `push ecx`), the numerator/denominator group as ...*n.z + (d.y*n.y + d.z*n.z), and e.y*t
// is rounded to float32 before `+ a->y`.
void PlaneTrace::intersect(vector4* out, const vector4* a, const vector4* b)
{
    volatile vector4 d;
    d.x = pt.x - a->x;
    d.y = pt.y - a->y;
    d.z = pt.z - a->z;
    float t = (d.x * nrm.x + (d.y * nrm.y + d.z * nrm.z)) /
              ((b->x - a->x) * nrm.x + ((b->y - a->y) * nrm.y + (b->z - a->z) * nrm.z));
    float rx = (b->x - a->x) * t + a->x;
    volatile float py = (b->y - a->y) * t;
    float ry = py + a->y;
    float rz = (b->z - a->z) * t + a->z;
    out->x = rx;
    out->y = ry;
    out->z = rz;
}

// ---- 0x51a3a0 -------------------------------------------------------------------------------------------
struct Pickable {
    virtual ~Pickable();
    virtual unsigned v1();                          // 0
};
struct Picker {
    char pad0[0x7c];
    std::vector<Pickable*> objs;                    // +0x7c (_Myfirst at +0x80, _Mylast at +0x84)
    void collect(unsigned key, Pickable** out, int* count);     // 0x51a3a0 ret 0xc
};

// bit-prefix match: true when every set bit of "key" is also set in "value" (key==0 matches anything).
// The equality case is handled here, so the loop cannot spin on value == key.
static bool mask_contains(unsigned value, unsigned key)
{
    if (value == key)
        return true;
    while (((value ^ key) & 1) == 0) {
        value >>= 1;
        key >>= 1;
    }
    return key == 0;
}

// @ 0x51a3a0
void Picker::collect(unsigned key, Pickable** out, int* count)
{
    *count = 0;
    for (std::vector<Pickable*>::iterator it = objs.begin(); it < objs.end(); ++it) {
        if (mask_contains((*it)->v1(), key)) {
            out[*count] = *it;
            ++*count;
        }
    }
}

// ---- 0x51a420 -------------------------------------------------------------------------------------------
struct Camera {
    char pad0[0xe0];
    float f0e0, f0e4;                   // +0xe0, +0xe4 (screen origin)
    char pade8[0xf0 - 0xe8];
    float f0f0, f0f4;                   // +0xf0, +0xf4 (world extent)
    char padf8[0x1ac - 0xf8];
    char f1ac;                          // +0x1ac (perspective flag)
    char pad1ad[0x274 - 0x1ad];
    matrix m274;                        // +0x274 (view matrix)
    float f2b4, f2b8, f2bc, f2c0, f2c4; // +0x2b4 (viewport rect + 1/z scale)
    void project(vector4* out, const vector4* in);      // 0x51a690 ret 8
};

struct MapPickScreen {
    char pad0[0x30];
    Camera* cam;                        // +0x30
    char pad34[0x2d0 - 0x34];
    float f2d0;                         // +0x2d0 (label fade timer)
    void set_label(int id);             // 0x51a420 ret 4
};

extern int g_sel_party;             // 0x5aa3ac (currently selected party id)
extern int g_sel_label;             // 0x5aa3a8 (its label index)

// @ 0x51a420
void MapPickScreen::set_label(int id)
{
    tstring unused_a, unused_b;
    char show = 1;
    if (id == g_sel_party) {
        if (g_game->cur_label == g_sel_label && f2d0 < 1.0f)
            return;
        show = 0;
    }
    g_sel_party = id;
    g_sel_label = g_game->cur_label;
    f2d0 = 0.0f;
    if (g_label_obj)
        delete g_label_obj;
    g_label_obj = 0;
    g_5d8784 = 0;
    if (id < 0)
        return;

    PartyMgr* pool = &g_game->parties;
    int idx = pool->index_of(id);
    char* rec = (char*)g_game->parties.pages._Myfirst[idx >> 4] + (idx & 0xf) * 0x5ac8;
    vector4 pos = *(vector4*)(rec + 0x30);

    unsigned color;
    if (g_flag_a181b0 == 0) {
        float rel = g_game->relation(id, g_game->f78);
        color = g_game->color_of(rel, 1);
    } else {
        int idx2 = pool->index_of(id);
        char* rec2 = (char*)g_game->parties.pages._Myfirst[idx2 >> 4] + (idx2 & 0xf) * 0x5ac8;
        color = *(unsigned*)((char*)g_game->factions + *(int*)(rec2 + 0x70) * 0x300 + 0x28);
    }

    int idx3 = pool->index_of(id);
    char* rec3 = (char*)g_game->parties.pages._Myfirst[idx3 >> 4] + (idx3 & 0xf) * 0x5ac8;
    const tstring* name;
    if (g_ui_mode)
        name = (const tstring*)(rec3 + 0x20);
    else if (*(char*)(rec3 + 0x1c) == 0 && (name = g_strtable.find((const tstring*)(rec3 + 0x20))) != 0)
        ;
    else
        name = (const tstring*)(rec3 + 0xc);
    g_gui.add_label(name, 0.021f, (int)color);

    for (int i = 0; i < g_game->n_labels; i++)
        g_gui.add_label(&g_game->labels[i], 0.015f, (int)(((g_game->cur_label != i) - 1 & 0xffff0001) - 1));

    if (show) {
        vector4 pt;
        cam->project(&pt, &pos);
        g_gui.set_pos((const float*)&pt);
    }
}

// ---- 0x51a690 -------------------------------------------------------------------------------------------
// @ 0x51a690
void Camera::project(vector4* out, const vector4* in)
{
    vector4 p = m274.transform_point_to_local(*in);
    volatile float x, y;
    if (f1ac) {
        x = -((1.0f / p.z) * f2c4 * p.x);
        y = -((1.0f / p.z) * f2c4 * p.y);
    } else {
        x = p.x;
        y = p.y;
    }
    out->x = ((x - f2b4) * f0f0) / (f2b8 - f2b4) + f0e0;
    out->y = ((y - f2c0) * f0f4) / (f2bc - f2c0) + f0e4;
}

// ---- 0x51a760 -------------------------------------------------------------------------------------------
// @ 0x51a760
void gui_manager::set_pos(const float* p)
{
    f16f8 = p[0];
    f16fc = p[1];
    if (f16f8 > 0.95f)
        f16f8 = 0.95f;
    if (f16f8 < 0.05f)
        f16f8 = 0.05f;
    if (o56c == 0)
        return;
    vector4 mn, mx;
    o56c->calc_bounds(&mn, &mx);
    if (mn.x + f16f8 < 0.011f)
        f16f8 = 0.011f - mn.x;
    else if (mx.x + f16f8 > 0.989f)
        f16f8 = 0.989f - mx.x;
    if (mn.y + f16fc < 0.011f)
        f16fc = 0.011f - mn.y;
    else if (mx.y + f16fc > 0.739f)
        f16fc = 0.739f - mx.y;
}

// ---- 0x51a880 -------------------------------------------------------------------------------------------
// @ 0x51a880
void gui_manager::add_label(const tstring* text, float h, int color)
{
    if (o56c == 0) {
        mesh* m = new mesh;
        if (o56c != 0)
            delete o56c;
        o56c = m;
        f1704 = 0.0f;
    }
    {
        tstring name("ui_font");
        o56c->res = (rgl::res_t*)g_resmgr.get_material(name);
    }
    {
        float out[2];
        float scale[2];
        scale[0] = h;
        scale[1] = h;
        float xmin = -1.0f;
        float xmax = 1.0f;
        vector4 pos(0.0f, f1704, 0.0f);
        g_font->layout_text(out, text, &pos, scale, o56c, (int)&color, &xmin, &xmax, 0x10);
    }
    f1704 -= h;
    {
        vector4 mn, mx;
        o56c->calc_bounds(&mn, &mx);
        float corner[2];
        float size[2];
        corner[0] = mn.x - 0.01f;
        corner[1] = mn.y - 0.01f;
        size[0] = (mx.x - mn.x) + 0.02f;
        size[1] = (mx.y - mn.y) + 0.02f;
        set_quad(corner, size);
    }
    {
        float p[2];
        p[0] = f16f8;
        p[1] = f16fc;
        set_pos(p);
    }
}

// ---- 0x51aa20 -------------------------------------------------------------------------------------------
// @ 0x51aa20
void gui_manager::set_quad(const float* origin, const float* size)
{
    float sy = origin[1] + size[1];
    float sx = origin[0] + size[0];
    m570.ensure_local();
    vector4 v;
    v.x = origin[0];
    v.y = origin[1];
    v.z = 0.0f;
    m570.v70[0].a = m570.v70[0].b = v;
    v.x = sx;
    v.y = origin[1];
    m570.v70[1].a = m570.v70[1].b = v;
    v.x = sx;
    v.y = sy;
    m570.v70[2].a = m570.v70[2].b = v;
    v.x = origin[0];
    v.y = sy;
    m570.v70[3].a = m570.v70[3].b = v;
    m570.slots[0].f28 = 1;
    {
        tstring name("matte");
        m570.res = (rgl::res_t*)g_resmgr.get_material(name);
    }
}
