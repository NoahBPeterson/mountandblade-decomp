// Slice s00531380 (batch ds1): mission placed-object (prop) maintenance and mission-start/menu glue.
//   0x531380  matrix "advance along the forward row" helper (matrix layout: s,f,u,o)
//   0x5313c0  Mission::remove_prop(index): unlink the prop's scene entity, erase it from the
//             std::vector<PlacedObj> at mission+0x6688, then renumber the remaining entities
//   0x531470  erase: the prop vector's erase (same body as the STL's, written out below)
//   0x5314e0  append a placed object's entity meshes to a skinned target mesh
//   0x531590  menu: pick the pressed option and hand it to the mission UI
//   0x531600  mission UI: set the entry up from the template record (weighted random option)
//   0x5316f0  mission UI: option weight from its flags
//   0x531780  menu: tick (button 0 -> commit the flags block)
// All names are provisional; every offset comes from the instruction that reads it.
#include <stdlib.h>
#include <vector>

#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- engine types touched through pointers --------------------------------------------------
namespace rgl {

// 64-byte transform, same layout as rgl::matrix (only pointers cross this slice).
struct mat4 { vector4 s, f, u, o; };

// the 0x260-byte mesh instance; only the skinned-append entry point is needed here.
struct mesh {
    void append_skinned(mesh* other, const mat4* m, int bone, unsigned color, char flip);  // 0x44eb00
};

} // namespace rgl

// placement frame: `pos` is a matrix with a relative-transform helper.
struct pos : matrix { matrix relative(const matrix& base) const; };          // 0x4329d0

// scene entity that owns a placed object's meshes (id/index pair, own frame, mesh list).
struct SceneEnt {
    char pad0[8];
    int id;                                 // +0x08
    int index;                              // +0x0c
    char pad10[0x58 - 0x10];
    matrix frame;                           // +0x58
    char pad98[0x118 - 0x98];
    std::vector<rgl::mesh*> meshes;         // +0x118 (first at +0x11c)
};

// ---- placed objects (0x1c8 bytes; layout verified by s0044f3c0) -----------------------------
struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};

struct PlacedObj : IfaceB {
    int m10, m14;                           // +0x10
    matrix a18, a58, a98, ad8, a118;        // +0x18 .. +0x157
    vector4 v158;                           // +0x158
    int m168;                               // +0x168
    float f16c;                             // +0x16c
    char q170[0x10];                        // +0x170
    int m180, m184, m188, m18c;             // +0x180
    char name[0x10];                        // +0x190
    int m1a0, m1a4, m1a8, m1ac, m1b0;       // +0x1a0
    SceneEnt* m1b4;                         // +0x1b4
    int m1b8, m1bc, m1c0, m1c4;             // +0x1b8
    PlacedObj& operator=(const PlacedObj&); // 0x437d60 (out of line: std::copy must call it)
};

struct World { void remove_entity(int id); };                               // 0x41f0d0 (ret 4)

// the prop list is a VC7.1 std::vector<PlacedObj> (empty allocator at +0, so _Myfirst is at +4).
// erase is the one member that needs its own out-of-line body here (0x531470): same erase the STL
// writes, spelled out so the copy/destroy loops are emitted directly.
struct PropVec : std::vector<PlacedObj> {
    iterator erase(iterator where);         // 0x531470
};

struct Mission {
    World* world;                           // +0x0000
    char pad4[0x6688 - 4];
    PropVec props;                          // +0x6688 (a real VC7.1 std::vector: first at +0x668c)
    void remove_prop(int index);            // 0x5313c0
    void append_prop(rgl::mesh* target, pos* frame, PlacedObj* prop, unsigned color);   // 0x5314e0
};

// ---- 0x531380: matrix helper (rgl::matrix has no such member in the shared header) -----------
struct MatFrame {
    vector4 s, f, u, o;
    void advance(const float& k);           // 0x531380
};

// ---- minimal UI widget: vtable slot 0x44 is pressed(int) ------------------------------------
struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16();
    virtual bool pressed(int);                                      // +0x44
};

// one menu option (8 bytes, array at Menu+0x94)
struct MenuEntry { int id; int value; };

struct Flags { void commit(char* out); };                            // 0x513710 (ret 4)
struct MsgLog { void flush(); };                                    // 0x4f1390

// the mission-start menu (buttons from +0x30, options at +0x94, count at +0x114)
struct Menu {
    char pad0[4];
    int f4;                                 // +0x04
    char pad8[0x30 - 8];
    Widget* buttons[0x19];                  // +0x30 (25 pointers, ends at +0x94)
    MenuEntry entries[16];                  // +0x94
    int count;                              // +0x114
    void select(int arg);                   // 0x531590
    void update(int arg);                   // 0x531780
};

// ---- mission template tables (records at 0xa735d0, 0x20 bytes each) --------------------------
struct TplOpt {                             // 0x18 bytes
    char pad0[0x10];
    unsigned flags;                         // +0x10
    int setup_arg;                          // +0x14
};
struct TplRec {
    char pad0[0x10];
    int f10;                                // +0x10
    char pad14[4];
    int count;                              // +0x18
    TplOpt* list;                           // +0x1c
};

// the mission UI object at 0xa73300 (fields used by this slice).
struct MissionUi {
    char pad0[0x10];
    int f10;                                // +0x10
    char pad14[0x1c - 0x14];
    int f1c;                                // +0x1c
    char pad20[0x2c - 0x20];
    int f2c;                                // +0x2c
    int f30;                                // +0x30
    char pad34[0x3c - 0x34];
    int f3c;                                // +0x3c
    char pad40[0x160 - 0x40];
    int f160;                               // +0x160
    void setup(unsigned flags, int delta);  // 0x513f80 (ret 8)
    void setup_entry(MenuEntry* e);         // 0x531600
    int weight(unsigned flags);             // 0x5316f0
};

extern MissionUi g_mui;                     // 0xa73300
extern TplRec g_templates[];                // 0xa735d0
extern Flags g_flags;                       // 0x5e291c
extern MsgLog g_msglog;                     // 0x5d7080
extern int g_a17144;                        // 0xa17144
extern int g_a17544;                        // 0xa17544
extern const float g_k59c48c;               // 0x59c48c (weight ramp constant)

// @ 0x531380
void MatFrame::advance(const float& k)
{
    vector4 d;
    d.x = f.x * k;
    d.y = f.y * k;
    d.z = f.z * k;
    o.x += d.x;
    o.y += d.y;
    o.z += d.z;
}

// @ 0x5313c0
void Mission::remove_prop(int index)
{
    if (props[index].m1b4)
    {
        world->remove_entity(props[index].m1b4->id);
        props[index].m1b4 = 0;
    }
    props.erase(props.begin() + index);
    for (unsigned i = 0; i < props.size(); i++)
    {
        SceneEnt* e = props[i].m1b4;
        if (e)
            e->index = i;
    }
}

// @ 0x531470
std::vector<PlacedObj>::iterator PropVec::erase(std::vector<PlacedObj>::iterator where)
{
    PlacedObj* p = where._Myptr;
    PlacedObj* end = _Mylast;
    PlacedObj* q = p + 1;
    for (; q != end; ++q)
        *p++ = *q;
    end = _Mylast;
    for (q = end - 1; q != end; ++q)
        q->~PlacedObj();
    --_Mylast;
    return where;
}

// @ 0x5314e0
void Mission::append_prop(rgl::mesh* target, pos* frame, PlacedObj* prop, unsigned color)
{
    if (!prop->m1b4)
        return;
    char flip = 0;
    if (prop->v158.z * prop->v158.y * prop->v158.x < 0.0f)
        flip = 1;
    for (unsigned i = 0; i < prop->m1b4->meshes.size(); i++)
    {
        rgl::mesh* msh = prop->m1b4->meshes[i];
        matrix m = frame->relative(prop->m1b4->frame);
        target->append_skinned(msh, (const rgl::mat4*)&m, -1, color, flip);
    }
}

// @ 0x531590
void Menu::select(int arg)
{
    int idx = -1;
    for (int i = 0; i < count; i++)
        if (buttons[i]->pressed(0x10000))
            idx = i;
    if (idx != -1)
    {
        g_mui.setup_entry(&entries[idx]);
        char b;
        g_flags.commit(&b);
    }
}

// @ 0x531600
void MissionUi::setup_entry(MenuEntry* e)
{
    f2c = e->id;
    f30 = e->value;
    f160 = -1;
    TplRec* t = &g_templates[e->id];
    if (e->id <= 6)
    {
        setup(t->f10, 0);
        return;
    }
    int r = rand() % 100;
    int sum = 0;
    for (int i = 0; i < t->count; i++)
    {
        if (f160 != -1)
            break;
        sum += weight(t->list[i].flags);
        if (r < sum)
            f160 = i;
    }
    if (f160 != -1)
    {
        setup(t->list[f160].setup_arg, 0);
        f3c |= 0x10;
    }
    else
        f3c |= 4;
    f10 |= 0x20;
}

// @ 0x5316f0
int MissionUi::weight(unsigned flags)
{
    float f = (float)(int)(flags & 0x7f);
    float x;
    if (flags & 0x100)
        x = (float)(f1c + 0x20);
    else if (flags & 0x200)
        x = (float)(6 - f1c);
    else
        return (int)f;
    float k = x * g_k59c48c - 0.4f;
    if (k < 0.0f)
        k = 0.0f;
    else if (k >= 1.0f)
        k = 1.0f;
    f *= k;
    return (int)f;
}

// @ 0x531780
void Menu::update(int arg)
{
    g_msglog.flush();
    if (f4 == 0)
    {
        char b;
        if (buttons[0]->pressed(0x10000) || (g_a17144 == 0 && g_a17544 != 0))
            g_flags.commit(&b);
    }
    else
    {
        char b;
        if (buttons[0]->pressed(0x10000) || (g_a17144 == 0 && g_a17544 != 0))
            g_flags.commit(&b);
    }
}
