// Slice s0043a280: Agent helpers around 0x43a280..0x43abf0 (all names provisional): the item-class name lookup for the
// UI, a few VC7.1 <vector>/<memory> instantiations, the agent's hand-position estimate, the quiver arrow display,
// ragdoll sleep test / static body rebuild, a shape contact probe and an attack-animation setup.
#include <math.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

#define MB_NOTHROW throw()

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring(const char* s) MB_NOTHROW;           // 0x42bbe0
    ~tstring() MB_NOTHROW;                       // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// translation table object at 0x5e2a18: hash buckets of 16-byte strings
struct StrTable {
    tstring* lookup(const tstring& key) MB_NOTHROW;      // 0x43a1e0 (key -> translated string)
};
extern StrTable g_strtable;                      // 0x5e2a18

// stand-in caller: gives the static helper below cl's register convention (the real caller is 0x4598d5)
// @ 0x43a280
// (register convention: type in EAX, destination buffer in ESI)
static __declspec(noinline) void ui_weapon_class_name(int type, char* out)
{
    if (type == 0) {
        tstring k("ui_one_handed_weapons");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 1) {
        tstring k("ui_two_handed_weapons");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 2) {
        tstring k("ui_polearms");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 3) {
        tstring k("ui_archery");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 4) {
        tstring k("ui_crossbows");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 5) {
        tstring k("ui_throwing");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 6) {
        tstring k("ui_firearms");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    }
}
// the real caller is far away (0x4598d5); this stand-in only makes cl pass the buffer in ESI
int ui_other2(char*, int);
void ui_weapon_class_name_user(int t, int x, char* q)
{
    char* p = q + x;
    ui_weapon_class_name(t, p);
    ui_other2(p, 1);
    ui_other2(p, 2);
}

// ---- troop record: display name through the translation table ------------------------------------
struct TroopRec {
    tstring id;                                  // +0
    char pad10[0x20 - 0x10];
    tstring name;                                // +0x20 (key into the translation table)
    tstring* display_name();                     // 0x43a480
};
struct TrTable : StrTable {
    tstring* find(const tstring* key) MB_NOTHROW;        // 0x4c6010 (null when absent)
};

// @ 0x43a480
tstring* TroopRec::display_name()
{
    tstring* t = ((TrTable&)g_strtable).find(&name);
    return t ? t : (tstring*)this;
}

// ---- <vector> helpers (VC7.1) -----------------------------------------------------------------------
struct ptr_tag {};          // std::_Nonscalar_ptr_iterator_tag: empty class passed by value; an UNINITIALISED local pushes whatever EAX holds
struct E12 { int a[3]; };
struct allocE12 {};
struct allocstr {};
struct allocint {};
void __cdecl uninit_fill_n12(E12* f, unsigned n, const E12& v, allocE12& al, ptr_tag tag);        // 0x438960
tstring* __cdecl uninit_copy_str(tstring* f, tstring* l, tstring* d, allocstr& al, ptr_tag tag);     // 0x438a10
int* __cdecl uninit_copy_int(int* f, int* l, int* d, allocint& al, ptr_tag tag);                         // 0x438aa0

struct vec12 : allocE12 { E12* first; E12* last; E12* end; E12* ufill(E12* f, unsigned n, const E12& v); };    // 0x43a4a0
struct vecstr : allocstr { tstring* first; tstring* last; tstring* end; tstring* ucopy(tstring* f, tstring* l, tstring* d); };   // 0x43a4d0
struct vecint : allocint { int* first; int* last; int* end; int* ucopy(int* f, int* l, int* d); };     // 0x43a4f0

// @ 0x43a4a0
E12* vec12::ufill(E12* f, unsigned n, const E12& v)
{
    ptr_tag t;
    uninit_fill_n12(f, n, v, *this, t);
    return f + n;
}
// @ 0x43a4d0
tstring* vecstr::ucopy(tstring* f, tstring* l, tstring* d) { ptr_tag t; return uninit_copy_str(f, l, d, *this, t); }
// @ 0x43a4f0
int* vecint::ucopy(int* f, int* l, int* d) { ptr_tag t; return uninit_copy_int(f, l, d, *this, t); }

// ---- scene / collision ---------------------------------------------------------------------------
struct shape;
struct contact { vector4 a; vector4 point; int extra; };          // 0x24 bytes; point at +0x10 is copied out
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base();
    virtual int shape_type();
    virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6();
    virtual void s7(); virtual void s8(); virtual void s9();
    virtual vector4* get_min(vector4* out);             // slot 10
    virtual vector4* get_max(vector4* out);             // slot 11
};
int __cdecl shape_contact_count(shape_base* a, shape* b, int max, contact* out);    // 0x40bfe0
struct NavMesh {                                       // see s00439100
    void next_stamp();                                 // 0x439410
};
struct Ctx {
    char pad0[0xb4];
    NavMesh* nav;                                      // +0xb4
    int locate_near(const vector4* pos, int* hint, int* mode);                      // 0x43abf0
    void search(const vector4* pos, int* face, int* maxdepth, int depth, int* out_face, float* out_dist);   // 0x439900
    void query_box(const float* bmin, const float* bmax, unsigned mask, shape** out, int* count);   // 0x42aa50
    void remove_entity(int id);                        // 0x41f0d0 (provisional)
    void ground_height(vector4* pos, float* height, void** hit);   // 0x439be0 (terrain probe: *height = z of the surface under pos.xy)
};
// per-weapon-class hand animation record (0x6c bytes; 3 per class, class stride 0x16c, from mission+0x5284)
struct HandRec {
    char pad0[0x10];
    vector4 pos;                                       // +0x10
    char pad20[0x34 - 0x20];
    float dx, dy;                                      // +0x34 / +0x38
    char pad3c[0x58 - 0x3c];
    int mode;                                          // +0x58
    char pad5c[0x6c - 0x5c];
};
struct ClassRec { HandRec hand[3]; char pad[0x16c - 3 * 0x6c]; };
struct Mission { Ctx* scene; char pad4[0x5284 - 4]; ClassRec cls[8]; };   // +0x5284
extern Mission* g_mission;                             // 0x5e292c
struct CharRec { char pad0[0x30]; __int64 flags; char pad38[0x508 - 0x38]; };   // 0x508 bytes
struct GameData { char pad[0x13a00]; CharRec* chars; };
extern GameData* g_game;                               // 0x5e2930

struct AgentH {
    char pad0[0x2c];
    int f2c;                                           // +0x2c (-1: none)
    char pad30[0x88 - 0x30];
    int troop;                                         // +0x88
    char pad8c[0x1c4 - 0x8c];
    int cls;                                           // +0x1c4 (weapon class 0..7)
    char pad1c8[4];
    unsigned char f1cc;                                // +0x1cc
    char pad1cd[0x5340 - 0x1cd];
    float f5340, f5344, f5348, f534c;
    void hand_pos(vector4* out);                       // 0x43a510
};


// @ 0x43a510
void AgentH::hand_pos(vector4* out)
{
    int c = cls;
    if (c < 0 || c >= 8) c = 7;
    int h = 0;
    if (troop >= 0) {
        if (f2c >= 0) h = 2;
        else h = (g_game->chars[troop].flags & 0x4000000i64) != 0 ? 1 : 0;
    }
    HandRec* r = &g_mission->cls[c].hand[h];
    float dy = r->dy;
    float ndx = -r->dx;
    bool left = false;
    float k = 1.0f;
    if (r->mode == 1) {
        k = 0.7f;
        if (f1cc & 1) left = true;
    } else if (r->mode == 2) {
        k = 1.0f;
    } else if (r->mode == 3) {
        k = 1.65f;
    } else if (r->mode == 4) {
        k = 2.5f;
    }
    if (f2c != -1) k *= 1.6f;
    *out = r->pos;
    float nx = (r->dx * f5344) * k + out->x;
    out->x = nx;
    float ny = (r->dy * f5344) * k + out->y;
    out->y = ny;
    if (left) {
        out->x = nx - k * r->dx;
        out->y = ny - k * r->dy;
    }
    float t8 = (dy * f5340) * k;
    float x2 = t8 + out->x;
    out->x = x2;
    float y2 = (ndx * f5340) * k + out->y;
    out->y = y2;
    float t4 = (k * 0.1f) * f534c;
    out->x = x2 + (k * 0.1f) * f5348;
    out->y = y2 + t4;
    float height = x2;
    void* hit;
    g_mission->scene->ground_height(out, &height, &hit);
    out->z = height;
}

// ---- quiver display ---------------------------------------------------------------------------------
struct ItemKind { char pad0[0x1c8]; unsigned type_flags; char pad1cc[0x238 - 0x1cc]; };   // 0x238, array at 0x5e29d0
extern ItemKind* g_items;                              // 0x5e29d0
struct ItemRef {
    int item;
    unsigned m4;                                       // low byte = ammo used
    int amount();                                      // 0x434b70 (fastcall == thiscall, no args)
};
struct AmmoMesh { char pad[0xcc]; int f_cc; };
struct AmmoDisplay {
    void set_value(float* v);                          // 0x439000 (thiscall, ret 4)
};
struct AgentQ {
    char pad0[0x14c];
    AmmoMesh* mesh[4];                                 // +0x14c
    char pad15c[0x16c - 0x15c];
    AmmoDisplay* disp[4];                              // +0x16c
    char pad17c[0x1d0 - 0x17c];
    ItemRef slots[4];                                  // +0x1d0 (stride 8; really 8 slots)
    void update_quiver();                              // 0x43a800
};
// @ 0x43a800
void AgentQ::update_quiver()
{
    for (int i = 0; i < 4; i++) {
        if (slots[i].item >= 0) {
            ItemKind* ik = &g_items[slots[i].item];
            unsigned t = ik->type_flags & 0xff;
            if ((t == 5 || t == 6 || t == 10) && disp[i] != 0) {
                int used = slots[i].m4 & 0xff;
                int n = slots[i].amount() - used;
                if (mesh[i] != 0 && mesh[i]->f_cc != 0) n--;
                float v = (float)((n * 5 + 5) * 2);
                disp[i]->set_value(&v);
            }
        }
    }
}

// @ 0x43abf0
int Ctx::locate_near(const vector4* pos, int* hint, int* mode)
{
    int face = -1;
    float dist = 100000.0f;
    nav->next_stamp();
    search(pos, hint, mode, 0, &face, &dist);
    return face;
}

// ---- animation channel start (provisional names; layouts from s004333c0 / s0043c9c0) ----------------
struct Chan {                                          // 0xd8 bytes
    char pad0[0x38];
    float blend;                                       // +0x38
    char pad3c[0xa0 - 0x3c];
    float period;                                      // +0xa0
    char pada4[4];
    int mode;                                          // +0xa8
    char padac[0xd8 - 0xac];
};
struct Skel {
    char pad0[0x40];
    Chan* chans;                                       // +0x40
    void start(int idx, const void* anim, int a, int b);   // 0x439fc0 (thiscall, ret 0x10)
    void reset_rec(int i);                             // 0x4339d0
    float rec_phase(int i);                            // 0x433ac0
    void set_rec_phase(int i, float p);                // 0x433b20
};
struct AnimCmd {                                       // animation request record
    const void* anim;                                  // +0
    char pad4[4];
    unsigned flags;                                    // +8
    float scale;                                       // +0xc
    int a, b;                                          // +0x10 / +0x14
    char pad18[0x3c - 0x18];
    float offset;                                      // +0x3c
};
struct AgentSlot { char pad0[0x14]; unsigned events; char pad18[0x20 - 0x18]; float rate; char pad24[0x2c - 0x24]; };
struct AgentBody2 { char pad0[0x304]; Skel* skel; };
struct AgentA {
    char pad0[0xd4];
    AgentSlot slot[2];                                 // +0xd4 (stride 0x2c)
    char pad12c[0x250 - 0x12c];
    AgentBody2* body;                                  // +0x250
    void start_channel(int idx, AnimCmd* cmd, unsigned* in_flags, float* base);   // 0x43a980
};
// @ 0x43a980
void AgentA::start_channel(int idx, AnimCmd* cmd, unsigned* in_flags, float* base)
{
    float cur = body->skel->rec_phase(idx);
    float scale = cmd->scale;
    float len = 1.5f;
    if (cmd->flags & 0xff)
        len = (float)(int)((cmd->flags & 0xff) - 1) * 0.1f;
    if (slot[idx].events & 0x400000) {
        float inv = 1.0f / slot[idx].rate;
        scale = inv * scale;
        len = inv * len;
    }
    body->skel->chans[idx].period = scale;
    body->skel->start(idx, cmd->anim, cmd->a, cmd->b);
    body->skel->chans[idx].blend = len + 0.01f;
    body->skel->reset_rec(idx);
    if (cmd->flags & 0x10000000)
        body->skel->chans[idx].mode = 0x14;
    else
        body->skel->chans[idx].mode = 0x15;
    if (cmd->flags & 0xa0000000) {
        float v = 0.0f;
        unsigned fl = *in_flags;
        if (fl & 0x40000000)
            v = -*base;
        else if (fl & 0x20000000)
            v = cur - *base;
        else if (fl & 0x80000000)
            v = -cur - *base;
        float res;
        if (cmd->flags & 0x20000000)
            res = v + cmd->offset;
        else
            res = cmd->offset - v;
        body->skel->set_rec_phase(idx, res);
    } else if (cmd->flags & 0x40000000) {
        body->skel->set_rec_phase(idx, cmd->offset);
    }
}

// ---- Newton Game Dynamics imports (cdecl) ----------------------------------------------------------
struct NewtonBody; struct NewtonWorld; struct NewtonCollision; struct NewtonRagDollBone;
extern "C" {
__declspec(dllimport) void __cdecl NewtonTreeCollisionEndBuild(const NewtonCollision* c, int optimize);              // 0x54926c
__declspec(dllimport) NewtonBody* __cdecl NewtonCreateBody(const NewtonWorld* w, const NewtonCollision* c);          // 0x549270
__declspec(dllimport) void __cdecl NewtonReleaseCollision(const NewtonWorld* w, const NewtonCollision* c);           // 0x549274
__declspec(dllimport) void __cdecl NewtonBodySetMatrix(const NewtonBody* b, const float* m);                          // 0x549278
__declspec(dllimport) int __cdecl NewtonBodyGetSleepingState(const NewtonBody* b);                                    // 0x549290
__declspec(dllimport) NewtonBody* __cdecl NewtonRagDollBoneGetBody(const NewtonRagDollBone* b);                       // 0x549294
}

struct pos : matrix {};
pos* identity_pos(pos* out);                        // 0x40cd00 (cdecl; returns out)

struct Agent;
// @ 0x43a6f0
struct AgentBodyOwner { char pad[0x250]; struct { int a, b; int id; }* f250; void release_body(); };
void AgentBodyOwner::release_body()
{
    g_mission->scene->remove_entity(f250->id);
    f250 = 0;
}

// @ 0x43a720
int __stdcall probe_shape_contact(shape_base* s, vector4* out_point, shape** out_shape, const char* flag)
{
    vector4 mn, mx;
    shape* found[0x2000];
    int n;
    contact c;
    s->get_min(&mn);
    s->get_max(&mx);
    unsigned mask = 0x8116;
    if (*flag)
        mask = 0x811e;
    g_mission->scene->query_box(&mn.x, &mx.x, mask, found, &n);
    int count = n;
    for (int i = 0; i < count; i++) {
        if (shape_contact_count(s, found[i], 1, &c) != 0) {
            *out_point = c.point;
            *out_shape = found[i];
            return 1;
        }
    }
    return 0;
}

// ---- ragdoll / static physics bodies ---------------------------------------------------------------
struct PhysRec {                                  // 0x38 bytes
    int f0;
    const NewtonWorld* world;                     // +4
    int f8;
    std::vector<const NewtonRagDollBone*> bones;  // +0xc (first at +0x10, last at +0x14)
    char pad18[0x2c - 0x1c];
    const NewtonCollision* collision;             // +0x2c
    NewtonBody* body;                             // +0x30
    int f34;
};
struct PhysSet {
    std::vector<PhysRec> recs;                    // +0 (first at +4)
    bool none_sleeping(int idx);                  // 0x43a8a0
    void rebuild_static_body(int idx);            // 0x43a900
};
// @ 0x43a8a0
bool PhysSet::none_sleeping(int idx)
{
    for (unsigned i = 0; i < recs[idx].bones.size(); i++) {
        if (NewtonBodyGetSleepingState(NewtonRagDollBoneGetBody(recs[idx].bones[i])))
            return false;
    }
    return true;
}
// @ 0x43a900
void PhysSet::rebuild_static_body(int idx)
{
    NewtonTreeCollisionEndBuild(recs[idx].collision, 0);
    recs[idx].body = NewtonCreateBody(recs[idx].world, recs[idx].collision);
    NewtonReleaseCollision(recs[idx].world, recs[idx].collision);
    pos m;
    float* mp = (float*)identity_pos(&m);
    PhysRec& r = recs[idx];
    NewtonBodySetMatrix(r.body, mp);
}

// ---- std::_Uninit_fill_n for 0x1c8-byte polymorphic records (VC7.1; catch(...) funclet at 0x43abbb) -------------
struct E1c8 {                                    // 0x1c8-byte polymorphic record (virtual dtor); copy ctor 0x437bb0
    virtual ~E1c8();
    char body[0x1c8 - 4];
};
struct allocE1c8 { void construct(E1c8* p, const E1c8& v); };           // 0x439040
// @ 0x43ab50
void __cdecl uninit_fill_n1c8(E1c8* f, unsigned n, const E1c8& v, allocE1c8& al, ptr_tag tag)
{
    E1c8* s = f;
    try {
        for (; 0 < n; --n, ++f) al.construct(f, v);
    } catch (...) {
        for (; s != f; ++s) s->~E1c8();
        throw;
    }
}
// @ 0x43abbb
// (catch funclet of uninit_fill_n1c8: destroys [s, f) then `throw;`)
