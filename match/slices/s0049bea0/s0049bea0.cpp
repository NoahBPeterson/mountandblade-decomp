// Slice s0049bea0: 0x49bea0 (order arrow creation) and 0x49c170 (reinforcement slot check).
// 0x49bea0: look an agent/order slot up in the mission's agent table, create a scene object
//   (type 9) for it, give it a 0.3-radius sphere shape and an identity frame, refresh the order's
//   text entries and then attach the "ui_..." arrow label through 0x49baa0.
// 0x49c170: validate a reinforcement packet id against the current set and apply its flag bits
//   (reset the unit's copied frame, clear two counters, run 0x4677a0, then set the team slot flags).
// Names are provisional; every offset comes from these two functions and the neighbouring mission slices.
#include <windows.h>

#define MB_NOTHROW throw()

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() MB_NOTHROW;
    tstring(const char* s) MB_NOTHROW;              // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;           // 0x42c110
    ~tstring() MB_NOTHROW;                          // 0x42bc40
};
struct StrTable {                                   // 0x5e2a18
    tstring* lookup(const tstring& key);                    // 0x43a1e0
    char* find(const tstring* key);                         // 0x4c6010
};
extern StrTable g_strtable;                         // 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };                    // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                             // 0x5d7080

struct vector4 { float x, y, z, w; };
struct matrix { float m[4][4]; };
matrix* identity_pos(matrix* out);                  // 0x40cd00 (cdecl; returns out)
void* operator new(unsigned size);                  // 0x53545e

// sprite (0x6c bytes, reference counted at +0x64); a "+0x2c slot" object also ref-counted
struct Sprite { char pad[0x64]; int refs; };
struct RefSlot { char pad[0x2c]; Sprite* sprite; };

// 0x44-byte sphere shape (vtable 0x54a4f0), built by 0x4332a0
struct Shape {
    virtual ~Shape();                               // 0
    virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual float radius() const;                   // 5 (+0x14)
    virtual void set_radius(const float* r);        // 6 (+0x18)
    char pad4[0x18];
    Sprite* own;                                    // +0x1c
    matrix m20;                                     // +0x20
    int m40;                                        // +0x40 (the float* arg, copied)
};
Shape* sphere_shape(Shape* mem, vector4 pos, const float* radius);  // 0x4332a0 (ret 0x14, ECX=mem)

// scene object for one order entry (the mission order contour)
struct OrderObj {
    virtual ~OrderObj();                            // 0
    virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual float param();                          // 5 (+0x14)
    virtual void set_param(const float* x);         // 6 (+0x18)
    int pad4, pad8;
    int mode;                                       // +0xc
    char* key;                                      // +0x10
    char flag14;                                    // +0x14
    char pad15[3];
    void* text;                                     // +0x18
    char pad1c[0x58 - 0x1c];
    matrix frame;                                   // +0x58 (4x4 frame)
    Shape* shape;                                   // +0x98
    char pad9c[0xa0 - 0x9c];
    unsigned flags;                                 // +0xa0
    int f_a4;                                       // +0xa4
    char pada8[0x168 - 0xa8];
    int fade;                                       // +0x168
    void refresh();                                 // 0x4b7840
};

struct OrderEntry {                                 // 0x5ac8-byte order/agent record
    char pad0[0xc];
    char* name;                                     // +0xc
    char flag1c;                                    // +0x1c
    char pad1d[3];
    tstring text;                                   // +0x20
    char pad30[0xa0 - 0x30];
    unsigned flags;                                 // +0xa0 (0x400 normal, 0x80000 hidden, 0x3000 style)
    int f_a4;                                       // +0xa4 (vertical offset)
    char pada8[0x59c0 - 0xa8];
    OrderObj* obj;                                  // +0x59c0
    char pad59c4[0x59e8 - 0x59c4];
    Sprite* label;                                  // +0x59e8 (text mesh)
    void finalize();                                // 0x4b2880
};

struct Reinf {                                      // 0x4c-byte reinforcement record
    int idx;                                        // +0 (unit index)
    int flags;                                      // +4
    char pad8[0x4c - 8];
};
struct ReinfSet {                                   // 0x58-byte reinforcement packet
    char pad0[0x34]; Reinf* recs;                   // +0x34
    int count;                                      // +0x38
    char pad3c[0x58 - 0x3c];
};
extern ReinfSet* g_reinf;                           // 0x5e2a00

struct Unit {                                       // 0x130-byte leader/unit record inside Mission
    char pad0[0x6b0];
    char m6b0[0x40];                                // +0x6b0 (frame copy of +0x6f0)
    char m6f0[0x40];                                // +0x6f0
    int f770;                                       // +0x770
    int f774;                                       // +0x774
    int pad778;
};

// the mission's agent table (member of the object 0x49bea0 is called on)
struct AgentTable {
    struct Pair { int id; int slot; } direct[0x2717];   // +0 (id -> slot, direct-mapped)
    char pad138b8[0x138d0 - 0x138b8];
    char** pages;                                   // +0x138d0 (page table of records)
    int index_of(int id);                           // 0x48ad00 (ret 4)
};

struct MissionAgents {
    char pad0[0xf8];
    AgentTable table;                               // +0xf8
    void add_order_arrow(int id, int mode);         // 0x49bea0 (ret 4)
};

struct OrderObjHost {
    virtual OrderObj* create_entity(int type);       // 0x428440 (ret 4)
};
extern void* g_obj_host;                            // 0xa732b4 (pointer to the host object)

struct Mission {
    char pad0[0x527c];
    char orders[8 * 0x16c];                         // +0x527c (order groups, 0x16c each)
    char pad5df0[0x5df0 - 0x527c - 8 * 0x16c];
    int cur_set;                                    // +0x5df0 (index into g_reinf)
    char pad5df4[0x6214 - 0x5df4];
    int team_b;                                     // +0x6214
    int team_a;                                     // +0x6218
    char pad621c[0x6668 - 0x621c];
    char slot_a;                                    // +0x6668
    char slot_b;                                    // +0x6669
    char pad666a[2];
    Unit units[1];                                  // +0x666c (0x130 each)

    void reset_group(int a, int b);                 // 0x4677a0 (ret 8)
    int add_item(Sprite* label, const char* name, int a, int b, const float* h);   // 0x49baa0 (ret 0x14)
    Sprite* make_label(OrderObj* o);                // 0x495b20 (ret 4)
    void check_slot(int id, unsigned bits);         // 0x49c170 (ret 8)
};
extern Mission* g_mission;                          // 0x5e2930
extern char g_ui_mode;                              // 0xa181b8
struct ResMgr { Sprite* material(const char* name); };  // 0x418a70
extern ResMgr g_resmgr;                             // 0x5d6fa0

// 0xa732b4 holds a pointer; its object creates the scene object for orders.
static OrderObj* order_create_entity(int type)
{
    return ((OrderObjHost*)*(void**)0xa732b4)->create_entity(type);
}

// @ 0x49bea0
void MissionAgents::add_order_arrow(int id, int mode)
{
    vector4 zero = { 0, 0, 0, 0 };
    float radius = 0.3f;
    float h;
    const char* name;
    int idx = table.index_of(id);
    OrderEntry* e = (OrderEntry*)(table.pages[idx >> 4] + (idx & 0xf) * 0x5ac8);
    OrderObj* o = order_create_entity(9);
    Shape* shape = (Shape*)operator new(0x44);
    if (shape != 0)
        shape = sphere_shape(shape, zero, &radius);
    o->shape = shape;
    e->obj = o;
    o->frame = *identity_pos(&o->frame);
    e->obj->refresh();
    e->label = g_mission->make_label(e->obj);
    h = (float)e->f_a4;
    if (e->flags & 0x80000) {
        name = "";
        g_mission->add_item(e->label, name, 0, 1, &h);
    } else if (e->flags & 0x400) {
        if ((e->flags & 0x3000) == 0)
            h = 0.5f;
        else if ((e->flags & 0x3000) == 0x1000)
            h = 0.75f;
        else
            h = 1.0f;
        if (g_ui_mode)
            name = (const char*)&e->text;
        else if (e->flag1c == 0 && (name = g_strtable.find(&e->text)) != 0)
            ;
        else
            name = e->name;
        g_mission->add_item(e->label, name, 1, (int)h, &h);
        ((RefSlot*)e->label)->sprite = g_resmgr.material("map_legend_outline");
    } else {
        if ((e->flags & 0x3000) == 0)
            h = 0.5f;
        else if ((e->flags & 0x3000) == 0x1000)
            h = 0.75f;
        else
            h = 1.0f;
        if (g_ui_mode)
            name = (const char*)&e->text;
        else if (e->flag1c == 0 && (name = g_strtable.find(&e->text)) != 0)
            ;
        else
            name = e->name;
        g_mission->add_item(e->label, name, 0, (int)h, &h);
    }
    o->mode = mode;
    o->fade = 1;
    if (!(o->param() > 0.0f))
        o->set_param(&h);
    e->finalize();
}

// @ 0x49c170
void Mission::check_slot(int id, unsigned bits)
{
    if (id >= 0 && id < g_reinf[cur_set].count) {
        Reinf* r = &g_reinf[cur_set].recs[id];
        char* p = (char*)this + r->idx * 0x130;
        int eax_, ebx_;

        memcpy(p + 0x6b0, p + 0x6f0, 0x40);
        *(int*)(p + 0x770) = 0;
        *(int*)(p + 0x774) = 0;
        reset_group(id, bits);
        if (r->flags & 1) { slot_b = 1; return; }
        if (r->flags & 2) { slot_a = 1; return; }
        eax_ = r->flags & 0x40;
        ebx_ = r->flags & 0x80;
        if (eax_ == 0 && ebx_ == 0) { slot_a = 1; return; }
        if (eax_ != 0 && team_b == 0) { slot_a = 1; return; }
        if (ebx_ != 0 && team_b == 1) { slot_a = 1; return; }
        if (eax_ != 0 && team_b == 1) { slot_b = 1; return; }
        if (ebx_ != 0 && team_b == 0) { slot_b = 1; return; }
        if (eax_ != 0 && team_a == 1) { slot_a = 1; return; }
        if (ebx_ != 0 && team_a != 0)
            goto L270;
        slot_a = 1;
        return;
    L270:
        if (eax_ != 0 && team_a == 0) { slot_b = 1; return; }
        if (ebx_ == 0) return;
        if (team_a != 1) return;
        slot_b = 1;
    } else {
        g_msglog.add(tstring("Invalid entry no for add_reinforcements"), -1);
    }
}
