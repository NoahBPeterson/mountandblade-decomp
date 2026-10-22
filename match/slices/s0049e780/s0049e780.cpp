// Slice s0049e780: view construction (0x49e780) and the "party sprite" wrappers that hang a party's
// character record on the view's scene graph (0x49e9c0..0x49f020).
//
//   0x49e780  View::create(int arg, int flags)            -> index of the new child in the view's list
//   0x49e9c0  Party::attach_sprite(float2*, int, int)     -> sprite record id
//   0x49ebd0  Party::add_sprite(int x, int y, int troop)  -> sprite record id
//   0x49ef00  Party::add_member_sprite(int slot, int x, int y, int a) -> sprite record id
//   0x49f020  Party::add_sprite_at(int x, int y, int troop, int a)
//
// The party object keeps a free-id list at +0xcc, a paged pool of 0x98-byte sprite records at +0xe4 and
// the embedded scene at +0xf8.  The scene holds paged 0x5ac8-byte object records at +0x138d0 (party
// 0x139c8) and the character table at +0x13908 (party 0x13a00, 0x508-byte records).
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <new>
#include "rgl.h"

struct float2 {
    float x, y;
    float2() {}
    float2(float a, float b) { x = a; y = b; }
};

struct tstring;                                         // 16-byte engine string (below)

// ---- engine services that live in other slices -----------------------------------------------------
void* __cdecl operator_new(unsigned int n);              // 0x53545e (operator new)
int   __cdecl _rand();                                   // 0x536a9f (CRT)
struct scene_api;                                        // 0x498420 (thiscall scene position, below)

extern float g_rand_scale;                               // 0x54a6e0 (angle per rand() step)
extern void* g_troop_base;                               // 0x5e29f8 (0xb0-byte troop record table)

// The global string table (this = 0x5e2a18) and its lookup (0 when absent).
struct Golist {
    tstring* get(tstring* key);                          // 0x4c6010
};

struct tstring {                                         // 16-byte engine string
    int pos;                                             // +0
    char* buf;                                           // +4
    int cap;                                             // +8
    int len;                                             // +0xc
    tstring();                                           // 0x42bbd0
    tstring(const char* s);                              // 0x42bbe0
    void operator=(const tstring& o);                    // 0x42bc50
};

// The 0xb0-byte troop-type record (the name at +0x10; the field offsets are relative to the record).
struct TroopRec {
    char pad0[0x10];
    tstring name;                                        // +0x10
    int f20;                                             // +0x20
    int f24;                                             // +0x24
    int f28;                                             // +0x28
    int f2c;                                             // +0x2c
    int f30;                                             // +0x30
    int f34;                                             // +0x34
    char pad38[0xb0 - 0x38];
};

// ---- the objects create() builds ---------------------------------------------------------------
// The constructors are thiscall (object in ecx) and return the object: 0x48ae20 the base scene object,
// 0x48b890 its particle-system sibling, 0x49deb0 the view camera (slice s0049deb0).
// Scene-object vtable slots used here: +0x58 set position (float2*), +0x64 set rotation (float2*),
// +0x74 set attribute (int*), +0xa4 (used around the attribute bits, see create), +0xc8 add child.
struct scene_obj {
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03();
    virtual void s04(); virtual void s05(); virtual void s06(); virtual void s07();
    virtual void s08(); virtual void s09(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21();
    virtual void set_pos(void* v);                       // +0x58
    virtual void s23(); virtual void s24();
    virtual void set_rot(void* v);                       // +0x64
    virtual void s26(); virtual void s27(); virtual void s28();
    virtual void set_attr(void* v);                      // +0x74
    virtual void s30(); virtual void s31(); virtual void s32(); virtual void s33();
    virtual void s34(); virtual void s35(); virtual void s36(); virtual void s37();
    virtual void s38(); virtual void s39(); virtual void s40();
    virtual void attach(void* v, int n);                 // +0xa4
    virtual void s42(); virtual void s43(); virtual void s44(); virtual void s45();
    virtual void s46(); virtual void s47(); virtual void s48(); virtual void s49();
    virtual void s50(); virtual void s51(); virtual void s52();
    virtual void add_child(void* v);                     // +0xc8
};
scene_obj* base_ctor(scene_obj* p);                      // 0x48ae20
scene_obj* ps_ctor(scene_obj* p);                        // 0x48b890
scene_obj* cam_ctor(scene_obj* p);                       // 0x49deb0

// 0x498420: a free thiscall position setter (argument in ecx, one pointer argument).
struct scene_api { void set_pos(float2* p); };

// The 0x5ac8-byte object record (only the fields the sprite wrappers touch).
struct ObjRec {
    char pad0[0xc];
    tstring name;                                        // +0xc
    char flag1c;                                         // +0x1c
    char pad1d[0x3];
    tstring name2;                                       // +0x20
    int f30;                                             // +0x30
    int f34;                                             // +0x34
    char pad38[0x8];
    float2 rot;                                          // +0x40
    char pad48[0x70 - 0x48];
    int f70;                                             // +0x70
    int f74;                                             // +0x74
    int id;                                              // +0x78
    char pad7c[0x88 - 0x7c];
    int sprite;                                          // +0x88  owning sprite record id (-1 = none)
    int f8c;                                             // +0x8c
    int f90;                                             // +0x90
    char pad94[0xa0 - 0x94];
    int fa0;                                             // +0xa0
    int fa4;                                             // +0xa4
    char pad_a8[0x8e4 - 0xa8];
    int f8e4;                                            // +0x8e4
    int f8ec;                                            // +0x8ec
    int f8f0;                                            // +0x8f0
    char pad8f0[0x918 - 0x8f0];
    int f918;                                            // +0x918
    char pad91c[0x5ac8 - 0x91c];

    void setup();                                        // 0x48f180 (thiscall, no args)
    void set_record(tstring* n, int a, int b, int c, int d); // 0x48c840
};

// The 0x508-byte character record.
struct CharRec {
    char pad0[0x20];
    tstring name;                                        // +0x20
    char pad30[0x4c - 0x30];
    int f4c;                                             // +0x4c
    char pad50[0x508 - 0x50];
};

// The 0x98-byte sprite record (16 to a page).
struct SpriteRec {
    char pad0[0x10];
    int alive;                                           // +0x10
    char pad14[0x24 - 0x14];
    float2 pos;                                          // +0x24
    char pad2c[0x54 - 0x2c];
    int a;                                               // +0x54
    int b;                                               // +0x58
    char pad5c[0x78 - 0x5c];
    int height;                                          // +0x78 sum of the two queried heights
    char pad7c[0x98 - 0x7c];
};

// The child-id list at party+0xcc.
struct ChildList {
    int pop_front();                                     // 0x493700 (thiscall)
};

// The scene embedded at party+0xf8.
struct Scene {
    char pad0[0x138d0];
    ObjRec** objects;                                    // +0x138d0 (party +0x139c8)
    char pad1[0x13908 - 0x138d4];
    CharRec** chars;                                     // +0x13908 (party +0x13a00)

    int obj_index(int key);                              // 0x48ad00 (thiscall, key -> object id)
    int last();                                          // 0x493960 (thiscall, back object id)
    ObjRec* obj(int key)
    {
        int id = obj_index(key);
        return objects[id >> 4] + (id & 0xf);
    }
};

// The party object the sprite wrappers work on.
struct Party {
    char pad0[0xcc];
    ChildList list;                                      // +0xcc
    char pad1[0xe4 - 0xd0];
    SpriteRec** sprites;                                 // +0xe4 paged sprite records
    char pad2[0xf8 - 0xe8];
    Scene scene;                                         // +0xf8

    CharRec* chars() { return *scene.chars; }
    SpriteRec* sprite(int id) { return sprites[id >> 4] + (id & 0xf); }

    int attach_sprite(float2* pos, int a, int b);        // 0x49e9c0
    int add_sprite(int x, int y, int troop);             // 0x49ebd0
    int add_member_sprite(int slot, int x, int y, int a);  // 0x49ef00
    void add_sprite_at(int x, int y, int troop, int a);   // 0x49f020

    void count_dims(int troop, int* h, int* d, int* w, int* o); // 0x490ce0
    void slot_setup(int id, int troop, int a);           // 0x48fbd0
    void sprite_hook(int id);                            // 0x49bbb0
    void sprite_flag(int id);                            // 0x49bea0
    void pick(int x, int y, int a, int* out);            // 0x490030
};

inline TroopRec* troop_rec(int troop) { return (TroopRec*)((char*)g_troop_base + 0xb0 * troop); }

// The view (`this`) seen from here: the child list object at +0x2c and its own vector at +4/+8.
struct View {
    char pad0[4];
    int* begin;                                          // +4
    int* end;                                            // +8
    char pad_c[0x2c - 0xc];
    scene_obj* list;                                     // +0x2c

    int create(int arg, int flags);                      // 0x49e780
    void push(void** obj);                               // 0x4936b0 (thiscall push_back)
};

// ---- @ 0x49e780 -----------------------------------------------------------------------------------
// Build the scene object for `flags` (attribute bits flags&0x381c | 0x700, scale 0.9 x 0.75) and, for
// the "3d" flag 0x2000, the particle system + camera + group around it.
int View::create(int arg, int flags)
{
    scene_obj* o = (scene_obj*)operator_new(0x178);
    if (o)
        o = base_ctor(o);                                // 0x48ae20
    else
        o = 0;
    float2 p;
    p.x = 0;
    p.y = 0;
    o->set_pos(&p);                                      // +0x58
    int attr = (flags & 0x381c) | 0x700;
    o->set_attr(&attr);                                  // +0x74
    float2 r;
    r.x = 0.9f;
    r.y = 0.75f;
    o->set_rot(&r);                                      // +0x64
    o->attach((void*)arg, *(int*)&r.y);                  // +0xa4 (operand pair of the original)
    if (flags & 0x2000) {
        scene_obj* ps = (scene_obj*)operator_new(0x188);
        if (ps)
            ps = ps_ctor(ps);                            // 0x48b890
        else
            ps = 0;
        *(int*)((char*)ps + 0x178) = 1;
        float2 z;
        z.x = 0;
        z.y = 0;
        ps->set_pos(&z);                                 // +0x58
        this->list->add_child(ps);                       // +0xc8
        scene_obj* cam = (scene_obj*)operator_new(0x4a8);
        if (cam)
            cam = cam_ctor(cam);                         // 0x49deb0
        else
            cam = 0;
        *(int*)((char*)cam + 0x104) |= 0x20;
        *(scene_obj**)((char*)cam + 0x484) = ps;
        scene_obj* grp = (scene_obj*)operator_new(0x188);
        if (grp) {
            grp = base_ctor(grp);                        // 0x48ae20
            *(void**)grp = (void*)0x5924a0;
            *(int*)((char*)grp + 0x17c) = 0;
            *(int*)((char*)grp + 0x180) = 0;
            *(int*)((char*)grp + 0x184) = 0;
        } else {
            grp = 0;
        }
        float2 z2;
        z2.x = 0;
        z2.y = 0;
        grp->set_pos(&z2);                               // +0x58
        grp->set_rot((float2*)((char*)o + 0xf0));        // +0x64
        grp->add_child(o);                               // +0xc8
        *(scene_obj**)((char*)cam + 0x490) = grp;
        cam->add_child(grp);                             // +0xc8
        float2 z3;
        z3.x = 0;
        z3.y = 0;
        cam->set_pos(&z3);                               // +0x58
        float2 cp;
        cp.x = *(float*)((char*)o + 0xf0);
        cp.y = 0.1f;
        ((scene_api*)cam)->set_pos(&cp);                 // 0x498420
        this->list->add_child(cam);                      // +0xc8
        scene_obj* t = cam;
        push((void**)&t);                                // 0x4936b0
    } else {
        this->list->add_child(o);                        // +0xc8
        scene_obj* t = o;
        push((void**)&t);                                // 0x4936b0
    }
    if (this->begin == 0)
        return -1;
    return ((this->end - this->begin) >> 2) - 1;
}

// ---- @ 0x49e9c0 -----------------------------------------------------------------------------------
// Build a sprite record for troop types `a` and `b` at `pos`: the party's free list hands out the
// record id, the record takes the position, both troop ids and the summed heights, and both troop
// objects point at it.
int Party::attach_sprite(float2* pos, int a, int b)
{
    int id = list.pop_front();                           // 0x493700
    sprite(id)->alive = 1;
    sprite(id)->pos = *pos;
    sprite(id)->alive = 1;
    sprite(id)->a = a;
    sprite(id)->b = b;
    sprite(id)->height = 0;
    if (a >= 0) {
        int d[4] = { 0, 0, 0, 0 };
        count_dims(a, &d[0], &d[3], &d[2], &d[1]);       // 0x490ce0
        sprite(id)->height += d[0];
    }
    if (b >= 0) {
        int d[4] = { 0, 0, 0, 0 };
        count_dims(b, &d[0], &d[3], &d[2], &d[1]);       // 0x490ce0
        sprite(id)->height += d[0];
    }
    scene.obj(a)->f918 = 7;
    scene.obj(b)->f918 = 7;
    scene.obj(a)->sprite = id;
    scene.obj(b)->sprite = id;
    sprite_hook(id);                                     // 0x49bbb0
    return id;
}

// ---- @ 0x49ebd0 -----------------------------------------------------------------------------------
// Add a scene object for troop type `troop` at (x, y): the scene's last object record is filled from
// the 0xb0-byte troop record, named after it, randomly rotated, and hooked to a fixture.
int Party::add_sprite(int x, int y, int troop)
{
    TroopRec* tr = troop_rec(troop);                     // 0x5e29f8 + 0xb0 * troop
    int id = scene.last();                               // 0x493960
    scene.obj(id)->setup();                              // 0x48f180
    scene.obj(id)->f8c = troop;
    scene.obj(id)->f30 = x;
    scene.obj(id)->f34 = y;
    scene.obj(id)->rot.x = 1.0f;
    scene.obj(id)->rot.y = 0;
    float ang = (float)(_rand() % 0x3dc9) * g_rand_scale; // 0x536a9f / 0x54a6e0
    float c = (float)cos(ang);
    float s = (float)sin(ang);
    ObjRec* ro = scene.obj(id);
    float oy = ro->rot.y;
    float ox = ro->rot.x;
    ro->rot.y = c * oy + s * ox;
    ro->rot.x = c * ox - s * oy;
    tstring* n = ((Golist*)0x5e2a18)->get(&tr->name);    // 0x4c6010
    if (n == 0)
        n = (tstring*)tr;
    scene.obj(id)->name = *n;
    scene.obj(id)->flag1c = 1;
    scene.obj(id)->name2 = tr->name;
    scene.obj(id)->fa0 = tr->f20;
    scene.obj(id)->fa4 = tr->f24;
    scene.obj(id)->f90 = tr->f28;
    scene.obj(id)->f70 = tr->f2c;
    scene.obj(id)->f74 = tr->f30;
    scene.obj(id)->f8e4 = 2;
    scene.obj(id)->f8ec = x;
    scene.obj(id)->f8f0 = y;
    slot_setup(id, troop, 0);                            // 0x48fbd0
    sprite_flag(id);                                     // 0x49bea0
    tr->f34 += 1;
    return id;
}

// ---- @ 0x49ef00 -----------------------------------------------------------------------------------
// Add the sprite for character record `slot` at (x, y): name it after the character and move it to the
// character's own position field.
int Party::add_member_sprite(int slot, int x, int y, int a)
{
    int id = add_sprite(x, y, 3);                        // 0x49ebd0
    tstring* n = ((Golist*)0x5e2a18)->get(&chars()[slot].name); // 0x4c6010
    if (n == 0)
        n = &chars()[slot].name;
    scene.obj(id)->name = *n;
    scene.obj(id)->flag1c = 1;
    scene.obj(id)->f70 = chars()[slot].f4c;
    scene.obj(id)->set_record(n, 1, 0, 0, 0);            // 0x48c840
    return id;
}

// ---- @ 0x49f020 -----------------------------------------------------------------------------------
// Pick the on-screen position for troop `troop` (x is adjusted in place) and add its sprite there.
void Party::add_sprite_at(int x, int y, int troop, int a)
{
    pick(x, y, a, &x);                                   // 0x490030
    add_sprite(x, y, troop);                             // 0x49ebd0
}
