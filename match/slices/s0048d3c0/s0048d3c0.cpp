// Slice s0048d3c0: the battle-scene "field" object and its helpers.  A Mission-like object owns the
// scene (+0x490), five bounds shapes (+0x108..+0x118) and a mirror pointer (+0x484); the functions
// here recolour it, retarget it to the camera, break a force label into its parts, name the parts via
// the string table, and copy a transform into the shapes.  The accessors at 0x48d5f0..0x48db00 belong
// to several small container types (an inline 0x1dc-stride array, two 8-byte vectors, a name table and
// an object with a couple of cached members).  Names provisional; offsets/call shapes from the disasm.
#include <windows.h>
#include <string.h>
#include <math.h>
#include "rgl.h"

// ---- rgl string (16 bytes): pos / buffer / capacity / length (see s0048e5d0) --------------------
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    tstring(const tstring& o);                          // 0x42c110
    ~tstring();                                         // 0x42bc40
    tstring operator+(const tstring& o) const;          // 0x42c840
    void operator=(const tstring& o);                   // 0x42bc50
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// The string table at 0x5e2a18: find() returns 0 when absent, lookup() shares a default string.
struct StrTable {
    tstring* find(const tstring* key);                  // 0x4c6010
    tstring* lookup(const tstring* key);                // 0x43a1e0
};
extern StrTable g_strtable;                             // 0x5e2a18

int  __cdecl spr(char* buf, const char* fmt, ...);      // 0x536f8f (CRT _sprintf)
void identity_pos(void* m);                             // 0x40cd00
void FUN_00488d50();                                    // 0x488d50
void __cdecl operator delete(void* p);                  // 0x535712

struct Scene; struct Shape; struct Visual; struct Mission;

// ---- the view object (the camera): colour at +0x14c, vtable +0x58 / +0x94 / +0x98 ----------------
struct VisualVt {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c();
    virtual void v50(); virtual void v54();
    virtual void set_pos(float* p);                     // +0x58
    virtual void v5c(); virtual void v60(); virtual void v64(); virtual void v68();
    virtual void v6c(); virtual void v70(); virtual void v74(); virtual void v78();
    virtual void v7c(); virtual void v80(); virtual void v84(); virtual void v88();
    virtual void v8c(); virtual void v90();
    virtual void get_color(unsigned* out);              // +0x94
    virtual void set_color(unsigned* c);                // +0x98
};
struct Visual : VisualVt {
    char pad4[0xf0 - 4];
    float f0;                                           // +0xf0
    float f4;                                           // +0xf4
    char padf8[0x14c - 0xf8];
    unsigned color;                                     // +0x14c
};

// ---- the scene: vtable +0x64 (set_offset), the view slot at +0x17c, floats at +0xf0/+0xf4 ---------
struct SceneVt {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c();
    virtual void v50(); virtual void v54(); virtual void v58(); virtual void v5c();
    virtual void v60();
    virtual void set_offset(float* p);                  // +0x64
};
struct Scene : SceneVt {
    char pad4[0xf0 - 4];
    float f0;                                           // +0xf0
    float f4;                                           // +0xf4
    char padf8[0x17c - 0xf8];
    Visual** view;                                      // +0x17c
};

// ---- a bounds shape: colour at +0x3c, an AABB at +0x22c, a transform slot at +0xcc ---------------
struct Shape {
    char pad0[0x3c];
    unsigned color;                                     // +0x3c
    char pad40[0x22c - 0x40];
    float x0;                                           // +0x22c
    float y0;                                           // +0x230
    char pad234[0x23c - 0x234];
    float x1;                                           // +0x23c
    float y1;                                           // +0x240
    void compute_bounds();                              // 0x419300
};

// ---- the field/mission object: vtable +0x58 / +0x64, shapes at +0x108, mirror at +0x484 ----------
struct MissionVt {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c();
    virtual void v50(); virtual void v54();
    virtual void set_pos(float* p);                     // +0x58
    virtual void v5c(); virtual void v60();
    virtual void set_offset(float* p);                  // +0x64
    virtual void v68();                                 // +0x68
};
struct Mission : MissionVt {
    char pad4[0xc - 4];
    char flag_c;                                        // +0xc
    char pad0d[0x1c - 0xd];
    char flag_1c;                                       // +0x1c
    char pad1d[0x20 - 0x1d];
    tstring name;                                       // +0x20
    char pad30[0x4c - 0x30];
    int f4c;                                            // +0x4c
    char pad50[0x54 - 0x50];
    tstring label;                                      // +0x54
    char pad64[0xb8 - 0x64];
    int fB8;                                            // +0xb8
    char padbc[0xe0 - 0xbc];
    float fE0;                                          // +0xe0
    float fE4;                                          // +0xe4
    char pade8[0xf0 - 0xe8];
    float fF0;                                          // +0xf0
    float fF4;                                          // +0xf4
    char padf8[0x108 - 0xf8];
    Shape* s0;                                          // +0x108
    Shape* s1;                                          // +0x10c
    Shape* s2;                                          // +0x110
    Shape* s3;                                          // +0x114
    Shape* s4;                                          // +0x118
    char pad11c[0x124 - 0x11c];
    int f124;                                           // +0x124
    char pad128[0x484 - 0x128];
    Mission* mirror;                                    // +0x484
    char pad488[0x490 - 0x488];
    Scene* scene;                                       // +0x490
    char pad494[0x49c - 0x494];
    float f49c;                                         // +0x49c
    float f4a0;                                         // +0x4a0
    char pad4a4[0x59c0 - 0x4a4];
    MissionVt* p59c0;                                   // +0x59c0
    char pad59c4[0x59f8 - 0x59c4];
    void* p59f8;                                        // +0x59f8
    void* p59fc;                                        // +0x59fc
    void* p5a00;                                        // +0x5a00

    void set_alpha(unsigned a);                         // 0x48d3c0
    void update();                                      // 0x48d470
    void make_label(char* out);                         // 0x48d940
};

// ---- 0x48d3c0: recolour the view and both mirrored shapes, keeping the rgb, replacing the alpha ----
// @ 0x48d3c0
void Mission::set_alpha(unsigned a)
{
    unsigned u = a << 24;
    a = (scene->view[0]->color & 0xffffff) | u;
    scene->view[0]->set_color(&a);
    unsigned out[2];
    scene->view[0]->get_color(out);
    mirror->s0->color = (s0->color & 0xffffff) | u;
    mirror->s2->color = (s2->color & 0xffffff) | u;
}

// ---- 0x48d470: retarget the field to the camera, then push the scroll offset into the scene -------
// @ 0x48d470
void Mission::update()
{
    Visual* v = scene->view[0];
    float p[2];
    p[0] = v->f0;
    p[1] = v->f4;
    scene->set_offset(p);

    if (sqrt(f49c * f49c + f4a0 * f4a0) > 0.0f) {
        float q[2];
        q[0] = scene->f0;
        q[1] = fF4;
        set_offset(q);
    } else {
        set_offset(&f49c);
    }

    if (scene->f4 == fF4) {
        float z[2];
        z[0] = 0.0f;
        z[1] = 0.0f;
        v->set_pos(z);
    } else {
        float z[2];
        z[0] = 0.0f;
        z[1] = fF4 - scene->f4;
        v->set_pos(z);
        float w[2];
        w[0] = scene->f0;
        w[1] = fF4;
        scene->set_offset(w);
    }

    float m[2];
    m[0] = fF0 + fE0 + *(float*)0x54a6f0;
    m[1] = fE4;
    mirror->set_pos(m);

    FUN_00488d50();
}

// ---- 0x48d5f0: forward (index, arg) into the 0x1dc-stride element's vtable slot +8, then index the
//      element's +0x1a0 pointer list, then the pointed object's +0xb4 table -------------------------
struct ArrElem {
    virtual int v00();
    virtual int v04();
    virtual int v08(int arg);
    char pad8[0x1a0 - 4];
    int* a;                                             // +0x1a0
    int* b;                                             // +0x1a4
};
struct ArrHolder {
    char pad0[4];
    ArrElem* base;                                      // +4
    int lookup(int index, int arg);                     // 0x48d5f0
};
// @ 0x48d5f0
int ArrHolder::lookup(int index, int arg)
{
    ArrElem* e = (ArrElem*)((char*)base + index * 0x1dc);
    int i = e->v08(arg);
    if (i < 0)
        return 0;
    int* a = e->a;
    if (!a)
        return 0;
    if ((e->b - a) >> 2 == 0)
        return 0;
    int* v = (int*)a[0];
    return *(int*)((char*)v + 0xb4 + i * 4);
}

// ---- 0x48d650: element of a vector of 8-byte records, bounds-checked (index passed by pointer) -----
struct Cont650 {
    char pad0[4];
    char* base;                                         // +4
    char* end;                                          // +8
    unsigned __int64 get(const unsigned* index);        // 0x48d650
};
// @ 0x48d650
unsigned __int64 Cont650::get(const unsigned* index)
{
    unsigned i = *index;
    if ((int)i < 0 || !base || i >= (unsigned)((end - base) >> 3))
        return 0;
    return *(unsigned __int64*)(base + i * 8);
}

// ---- 0x48d690: element of a vector of 8-byte records at +0x44 (index passed by value) -------------
struct Pair690 { int a; int b; };
struct Cont690 {
    char pad0[0x44];
    char* base;                                         // +0x44
    char* end;                                          // +0x48
    int get(unsigned index);                            // 0x48d690 (only the first dword is returned)
};
// @ 0x48d690
int Cont690::get(unsigned index)
{
    Pair690 r;
    if ((int)index >= 0 && base != 0 && index < (unsigned)((end - base) >> 3))
        r = *(Pair690*)(base + index * 8);
    else {
        r.a = 0;
        r.b = 0;
    }
    return r.a;
}

// ---- 0x48d6d0: look up "<name at +0x20>_pl" in the string table, else the object itself ------------
struct Named20 {
    char pad0[0x20];
    tstring name;                                       // +0x20
    void* name_pl();                                    // 0x48d6d0
};
// @ 0x48d6d0
void* Named20::name_pl()
{
    tstring* r = g_strtable.find(&(name + tstring((const char*)0x5923b0)));
    return r ? (void*)r : (void*)((char*)this + 0x10);
}

// ---- 0x48d730: resolve the string at +0x54 in the string table, else the object -------------------
struct Named54 {
    char pad0[0x54];
    tstring s;                                          // +0x54
    void* resolve();                                    // 0x48d730
    void* text();                                       // 0x48d750
};
// @ 0x48d730
void* Named54::resolve()
{
    tstring* r = g_strtable.find(&s);
    return r ? (void*)r : this;
}

// ---- 0x48d750: look up "<string at +0x54>_text" in the string table, else the object +0x10 --------
// @ 0x48d750
void* Named54::text()
{
    tstring* r = g_strtable.find(&(s + tstring((const char*)0x5923b4)));
    return r ? (void*)r : (void*)((char*)this + 0x10);
}

// ---- 0x48d7b0: pick the display name: global flag, cached name at +0x20, or the key at +0xc --------
struct Named7b0 {
    char pad0[0xc];
    char flag_c;                                        // +0xc
    char pad0d[0x1c - 0xd];
    char flag_1c;                                       // +0x1c
    char pad1d[0x20 - 0x1d];
    tstring name;                                       // +0x20
    void* display_name();                               // 0x48d7b0
};
extern char g_a181b8;                                   // 0xa181b8
// @ 0x48d7b0
void* Named7b0::display_name()
{
    if (g_a181b8)
        return (char*)this + 0x20;
    if (flag_1c == 0) {
        void* r = g_strtable.find(&name);
        if (r)
            return r;
    }
    return (char*)this + 0xc;
}

// ---- 0x48d7e0: release the cached buffer at +0x59f8 and reset the vector, then release +0x59c0 -----
struct Owned7e0 {
    char pad0[0x59c0];
    MissionVt* p59c0;                                   // +0x59c0
    char pad59c4[0x59f8 - 0x59c4];
    void* p59f8;                                        // +0x59f8
    void* p59fc;                                        // +0x59fc
    void* p5a00;                                        // +0x5a00
    void clear();                                       // 0x48d7e0
};
// @ 0x48d7e0
void Owned7e0::clear()
{
    if (p59f8)
        operator delete(p59f8);
    p59f8 = 0;
    p59fc = 0;
    p5a00 = 0;
    if (p59c0)
        p59c0->v68();
}

static __declspec(noinline) void unit_class_name(unsigned kind, char* out);

// ---- 0x48d830: copy a force-class name (0 infantry / 1 archers / 2 cavalry / else everyone) into
//      `out` (esi).  The original passes the class in eax and the buffer in esi, a convention cl does
//      not produce from C; modelled here with the class first, buffer second.
// @ 0x48d830
static void unit_class_name(unsigned kind, char* out)
{
    if (kind == 0) {
        tstring key((const char*)0x5923e0);
        strcpy(out, g_strtable.lookup(&key)->c_str());
    } else if (kind == 2) {
        tstring key((const char*)0x5923d4);
        strcpy(out, g_strtable.lookup(&key)->c_str());
    } else if (kind == 1) {
        tstring key((const char*)0x5923c8);
        strcpy(out, g_strtable.lookup(&key)->c_str());
    } else {
        tstring key((const char*)0x5923bc);
        strcpy(out, g_strtable.lookup(&key)->c_str());
    }
}

// ---- 0x48d940: build the three-part force label ("infantry, archers and cavalry") ------------------
// @ 0x48d940
// The three force groups are 0x6c-byte records at +0x4c (the +0xb8 and +0x124 fields are their counts too).
void Mission::make_label(char* out)
{
    struct Grp { int n; char pad[0x68]; };
    Grp* g = (Grp*)((char*)this + 0x4c);
    char all = 1;
    if (g->n == 0)
        all = 0;
    if (fB8 == 0)
        all = 0;
    if (f124 != 0 && all) {
        tstring key((const char*)0x5923bc);
        strcpy(out, g_strtable.lookup(&key)->c_str());
        return;
    }

    char part[0x200];
    char acc[0x200];
    char first_done = 1;
    for (int i = 0; i < 3; i++) {
        if (g[i].n == 0)
            continue;
        char grouped = 1;
        for (int j = i + 1; j < 3; j++)
            if (g[j].n != 0)
                grouped = 0;
        unit_class_name(i, part);
        if (first_done) {
            strcpy(out, part);
            first_done = 0;
        } else {
            strcpy(acc, out);
            if (grouped) {
                tstring key((const char*)0x592400);
                spr(out, g_strtable.lookup(&key)->c_str(), acc, part);
            } else {
                tstring key((const char*)0x5923ec);
                spr(out, g_strtable.lookup(&key)->c_str(), acc, part);
            }
        }
    }
}

// ---- 0x48db00: name cached at +0x14 unless the flag at +0x10 is set ---------------------------------
struct NamedDb00 {
    char pad0[0x10];
    char flag_10;                                       // +0x10
    char pad11[0x14 - 0x11];
    tstring key;                                        // +0x14
    void* display();                                    // 0x48db00
};
// @ 0x48db00
void* NamedDb00::display()
{
    if (flag_10 == 0) {
        void* r = g_strtable.find(&key);
        if (r)
            return r;
    }
    return this;
}

// ---- 0x48db20: copy a 0x40-byte transform (with 0x30 bytes taken from the caller) into the five
//      shapes of the indexed node -------------------------------------------------------------------
struct Mat4 { unsigned m[16]; };
struct Node {
    char pad0[0x108];
    Shape* s0;                                          // +0x108
    Shape* s1;                                          // +0x10c
    Shape* s2;                                          // +0x110
    Shape* s3;                                          // +0x114
    Shape* s4;                                          // +0x118
};
struct NodeTable {
    char pad0[4];
    Node** nodes;                                       // +4
    void* place(int index, const void* src);            // 0x48db20
};
// @ 0x48db20
void* NodeTable::place(int index, const void* src)
{
    Mat4 m;
    identity_pos(&m);
    memcpy(&m, src, 0x30);
    if (nodes[index]->s0) memcpy((char*)nodes[index]->s0 + 0xcc, &m, 0x40);
    if (nodes[index]->s2) memcpy((char*)nodes[index]->s2 + 0xcc, &m, 0x40);
    if (nodes[index]->s1) memcpy((char*)nodes[index]->s1 + 0xcc, &m, 0x40);
    if (nodes[index]->s3) memcpy((char*)nodes[index]->s3 + 0xcc, &m, 0x40);
    if (nodes[index]->s4) memcpy((char*)nodes[index]->s4 + 0xcc, &m, 0x40);
    return &nodes[index];
}
