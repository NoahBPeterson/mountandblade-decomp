// Slice s0048dc30: Mission agent-slot accessors (0x48dc30..0x48e590). Each function is a __thiscall member of the
// same class `Mission` (the owner of the agent page table at +4): resolve agent slot `index` through the table
// (pages[index >> 4][index & 0xf], 0x1c8-byte records), dispatch on the agent's kind (+0xc0), then either set a
// packed ARGB colour on the agent's mesh objects, forward an int/float to the agent, or clamp it to a range.
// Names provisional; page-table layout from s00474e60 / s004759c0; object colour at +0x3c and the colour setters
// at vtable +0x94/+0x98 from s00435c10 and this slice's disassembly.
#include <windows.h>

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring(const char* s);                             // 0x42bbe0
};

struct ItemRef {
    int item;
    unsigned m4;
    void set_amount(int amount);                        // 0x437510 (thiscall, ret 4)
};

struct ItemKind {                                       // 0x238 bytes, array at 0x5e29d0 (s0044f3c0)
    char pad0[0x1c8];
    unsigned type_flags;                                // +0x1c8 (bit 0xc: unique)
};
extern ItemKind* g_items;                               // 0x5e29d0

// the engine object an agent is built on (vtable + the packed colour at +0x3c and a colour at +0x14c/+0x150).
struct Object {
    char pad0[0x3c];
    unsigned color;                                     // +0x3c (0xAARRGGBB)
    char pad40[0x14c - 0x40];
    unsigned color14c;                                  // +0x14c
    unsigned color150;                                  // +0x150
    char pad154[0x160 - 0x154];
    int min160;                                         // +0x160
    int max164;                                         // +0x164
    float v168;                                         // +0x168
    float lo16c;                                        // +0x16c
    float hi170;                                        // +0x170
    char pad174[0x180 - 0x174];
    bool on180;                                         // +0x180
    char pad181[0x184 - 0x181];
    int kind184;                                        // +0x184
    char pad188[0x194 - 0x188];
    int f194;                                           // +0x194
    char pad198[0x1a8 - 0x198];
    int f1a8;                                           // +0x1a8
};

// vtable of the agent objects (only the slots this slice calls are named).
struct ObjVt {
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void set_value(int v);                      // +0x2c
    virtual void v30();
    virtual void set_value_f(float v);                  // +0x34
    virtual void v38();
    virtual void v3c();
    virtual void v40();
    virtual void v44();
    virtual void v48();
    virtual void v4c();
    virtual void v50();
    virtual void v54();
    virtual void v58(float* pt);
    virtual void v5c();
    virtual void v60();
    virtual void v64(float* out);
    virtual void v68();
    virtual void v6c(float* v);
    virtual void v70();
    virtual void v74();
    virtual void v78();
    virtual void v7c();
    virtual void v80();
    virtual void v84();
    virtual void v88();
    virtual void v8c();
    virtual void v90();
    virtual void get_pos(float* out);                   // +0x94
    virtual void set_color(unsigned* c);                // +0x98
    virtual void set_color2(unsigned* c);               // +0x9c
    virtual void va0();
    virtual void va4();
    virtual void va8();
    virtual void vac();
    virtual void vb0();
    virtual void vb4();
    virtual void vb8();
    virtual void vbc();
    virtual int type();                                 // +0xc0
    virtual void vc4();
    virtual void vc8(int v);                            // +0xc8
};

// an agent record (0x1c8 bytes): vptr (Object) + used flag + its mesh objects.
struct AgentObj : ObjVt, Object {
    char pad0[4];
    int used;                                           // +4
    char pad8[0x108 - 8];
    Object* f108;                                       // +0x108 / +0x10c (same object twice)
    Object* f10c;
    Object* f110;                                       // +0x110 (also +0x114)
    Object* f114;
};
typedef AgentObj Agent;

// Mission's agent page table at +4 (see s004759c0).
struct AgentTable {
    int count;                                          // +0
    Agent** pages;                                      // +4
};
struct Mission {
    int scene;                                          // +0
    AgentTable agents;                                  // +4
    __forceinline Agent* slot(unsigned index) const
    {
        return &agents.pages[index >> 4][index & 0xf];
    }

    void set_a_color(unsigned index, unsigned argb);                // 0x48dc30
    void set_b_color(unsigned index, unsigned color);               // 0x48de00
    void set_highlight_colors(unsigned index, unsigned color);      // 0x48e1a0
    void set_c_color(unsigned index, unsigned color);               // 0x48e270
    void set_value(unsigned index, int value);                      // 0x48e340
    void set_limits(unsigned index, int lo, int hi);                // 0x48e4c0
    void set_extra(unsigned index, int value);                      // 0x48e590
};

void set_all_colors_team(Mission* m, unsigned color);               // 0x48d3c0
void set_all_colors(Mission* m, unsigned color);                    // 0x48d350

// @ 0x48dc30
void Mission::set_a_color(unsigned index, unsigned argb)
{
    Agent* a = slot(index);
    if (a->type() != 0 && a->type() != 1 && a->type() != 9 && a->type() != 5 && a->type() != 6) {
        if (a->type() == 4) {
            a->f108->color = (a->f108->color & 0xff000000) | argb;
            a->f10c->color = (a->f10c->color & 0xff000000) | argb;
        }
        else if (a->type() == 8) {
            a->f110->color = (a->f110->color & 0xff000000) | argb;
            a->f10c->color = (a->f10c->color & 0xff000000) | argb;
        }
        else if (a->type() == 7) {
            set_all_colors(this, argb);
            return;
        }
        else
            return;
        unsigned c = (a->color14c & 0xff000000) | argb;
        a->set_color(&c);
    } else {
        unsigned c = (a->color14c & 0xff000000) | argb;
        a->set_color(&c);
        int x;
        a->vc8(x);
    }
}

// @ 0x48de00
void Mission::set_b_color(unsigned index, unsigned color)
{
    Agent* a = slot(index);
    if (a->type() == 0 || a->type() == 1) {
        unsigned c = (a->color14c & 0xffffff) | (color << 24);
        a->set_color2(&c);
        int x;
        a->vc8(x);
        return;
    }
    switch (a->type()) {
    case 4:
        a->f108->color = (a->f108->color & 0xffffff) | (color << 24);
        a->f10c->color = (a->f10c->color & 0xffffff) | (color << 24);
        break;
    case 8:
    case 9:
        a->f110->color = (a->f110->color & 0xffffff) | (color << 24);
        a->f10c->color = (a->f10c->color & 0xffffff) | (color << 24);
        break;
    case 2:
        a->f108->color = (a->f108->color & 0xffffff) | (color << 24);
        a->f110->color = (a->f110->color & 0xffffff) | (color << 24);
        break;
    case 3:
        a->f108->color = (a->f108->color & 0xffffff) | (color << 24);
        a->f110->color = (a->f110->color & 0xffffff) | (color << 24);
        a->f1a8 = color << 24;
        a->f194 = 0xcb3c57cb;
        break;
    case 5:
    case 6:
        a->f110->color = (a->f110->color & 0xffffff) | (color << 24);
        a->f10c->color = (a->f10c->color & 0xffffff) | (color << 24);
        if (a->type() == 5) a->f114->color = (a->f114->color & 0xffffff) | (color << 24);
        break;
    case 7:
        set_all_colors_team(this, color);
        return;
    default:
        return;
    }
    unsigned c = (a->color14c & 0xffffff) | (color << 24);
    a->set_color(&c);
}

// @ 0x48e1a0
void Mission::set_highlight_colors(unsigned index, unsigned color)
{
    Agent* a = slot(index);
    if (a->type() != 0 && a->type() != 1 && a->type() != 9 && a->type() != 5 && a->type() != 6) return;
    unsigned c = (a->color14c & 0xff000000) | color;
    a->set_color2(&c);
    int x;
    a->vc8(x);
}

// @ 0x48e270
void Mission::set_c_color(unsigned index, unsigned color)
{
    Agent* a = slot(index);
    if (a->type() != 0 && a->type() != 1 && a->type() != 9 && a->type() != 5 && a->type() != 6) return;
    unsigned c = (a->color150 & 0xffffff) | (color << 24);
    a->set_color2(&c);
    int x;
    a->vc8(x);
}

// @ 0x48e340
void Mission::set_value(unsigned index, int value)
{
    Agent* a = slot(index);
    if (a->type() == 2 || a->type() == 3) {
        a->set_value_f((float)value);
        if (!(a->v168 < a->lo16c)) {
            if (a->v168 > a->hi170) a->v168 = a->hi170;
            return;
        }
        a->v168 = a->lo16c;
        return;
    }
    if (a->type() == 5) {
        if (a->min160 <= value) {
            if (a->max164 < value) value = a->max164;
            a->set_value(value);
            return;
        }
        a->set_value(a->min160);
        return;
    }
    if (a->type() != 8) return;
    if (a->kind184 != 1) return;
    if (value < 0) value = 0;
    else if (value > 2) value = 2;
    a->set_value(value);
    a->on180 = value != 0;
}

// @ 0x48e4c0
void Mission::set_limits(unsigned index, int lo, int hi)
{
    Agent* a = slot(index);
    if (a->type() != 2 && a->type() != 3) return;
    a->lo16c = (float)lo / 100.0f;
    a->hi170 = (float)hi / 100.0f;
    if (a->v168 < a->lo16c) {
        a->v168 = a->lo16c;
        return;
    }
    if (a->v168 > a->hi170) a->v168 = a->hi170;
}

// @ 0x48e590
void Mission::set_extra(unsigned index, int value)
{
    Agent* a = slot(index);
    if (a->type() == 5) a->vc8(value);
}
