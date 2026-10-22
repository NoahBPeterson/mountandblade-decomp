// Slice s0047d3e0: the projectile ("Missile") per-tick update and the two mesh-application helpers that
// go with it.  The Missile record is the one s0045a170 (Agent::swing_trace) and s00454e30 already see as
// the `Missile*` a swing writes into; here it is the *this* of its own update.
//
// Missile layout (recovered from 0x47d3e0 / 0x47d320 / 0x47dc50 / 0x47dcd0 and cross-checked against the
// s0045a170 use):
//   +0x08  int  id (scene entity id; handed to Mission::remove_entity 0x41f0d0)
//   +0x10  vector4  previous position sample
//   +0x20  vector4  current position
//   +0x30  vector4  position at the start of the frame
//   +0x40  vector4  velocity (unit dir; scaled by +0x5c each step)
//   +0x50  entity*  the flying mesh/scene object (0 when the missile has landed/despawned)
//   +0x58  int      shooter agent id (compared to Mission::local_id at +0x5c)
//   +0x5c  float    speed
//   +0x68  int      state: 0 flying, 1 stuck/thrown, 2 removed, 3 hit ground
//   +0x70  int      clock index (into the 64-bit g_time table at 0x5e2948)
//   +0x78  __int64  start time of the current state
//   +0x80  int      item index (into g_items, stride 0x238)
//   +0x88  int      troop/item index used for the size lookup and hit sound
//   +0x94  float    swing scale / hit point scratch
//   +0x98  float    tumble rate
//   +0x9c  char     cleared at the end of every update
//   +0xa0  int      0 none, 1 embedded, 3 impact
//   +0xa4  int      playing sound handle
//   +0xa8  float    sound start time
#include <vector>
#include <new>
#include <stddef.h>
#include <string.h>
#include "tstring.h"
#include "mesh.h"

using rgl::mesh;
using rgl::mat4;
using rgl::matrix;
using rgl::vector4;
using rgl::res_t;
using rgl::g_resmgr;

// ---- shared entity / scene types (same as s0047cae0) ---------------------------------------------------
struct ebox {
    vector4 min, max, center;
    void reset();                                // 0x416e40
    void extend(const vector4* p);               // 0x416ef0
    void update();                               // 0x416f70
};
struct MeshVec : std::vector<mesh*> {
    iterator begin() { return _Myfirst; }
    iterator end() { return _Mylast; }
    size_type size() const { return _Mylast - _Myfirst; }
};
struct entity {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void add_mesh(mesh* m);              // +0x10
    virtual float s5();                          // +0x14
    virtual void s6(vector4* p);                 // +0x18
    char pad04[4];
    int id;                                      // 0x08 (scene entity id)
    char pad0c[0x2c - 0x0c];
    res_t* res;                                  // 0x2c
    char pad30[0x58 - 0x30];
    mat4 frame;                                  // 0x58
    char pad98[0xc4 - 0x98];
    vector4 origin;                              // 0xc4
    ebox box;                                    // 0xd4
    vector4 f104;                                // 0x104
    char pad114[0x118 - 0x114];
    MeshVec meshes;                              // 0x118 (_Myfirst at +0x11c)
    char pad128[0x140 - 0x128];
    char f140;                                   // 0x140 dirty flag
};

struct missile_holder {                          // Mission::remove_entity lives here
    char pad0[8];
    int id;                                      // 0x08
};
struct Mission {
    char pad0[4];
    missile_holder* holder;                      // 0x04
    char pad08[0x1c - 8];
    void* pages;                                 // 0x1c (agent array base)
    char pad20[0x5c - 0x20];
    int local_id;                                // 0x5c
    char pad60[0x6654 - 0x60];
    vector4 view;                                // 0x6654 (camera/aim point, 3 floats used)
    void remove_entity(int id);                  // 0x41f0d0
};
extern Mission* g_mission;                       // 0x5e292c

struct ItemSound { void play(int id); };         // 0x48add0 (thiscall; `this` = &kind + 0x228)
struct ItemKind {
    char pad0[0x1c8];
    int type;                                    // 0x1c8 (low byte = kind, bits 0x?0000 = subclass)
    int f1cc;                                    // 0x1cc
    int f1d0;                                    // 0x1d0 (bits 0xff000 = subtype)
    char pad1d4[0x20c - 0x1d4];
    int f20c;                                    // 0x20c
    char pad210[0x228 - 0x210];
    ItemSound sound;                             // 0x228
    char pad22c[0x238 - 0x22c];                  // stride of g_items[] is 0x238
};
extern ItemKind* g_items;                        // 0x5e29d0
extern int g_sound_ids[];                        // 0x5aa178

struct SoundMgr { int play(int sound, int priority, const vector4* pos, float pitch, float* out); };  // 0x442650 (ret 0x14)
extern SoundMgr g_sounds;                        // 0x5aa128
extern "C" int __stdcall FSOUND_StopSound(int channel);   // 0x5342a6
extern __int64 g_time[];                         // 0x5e2948 (64-bit clock table; g_time[i*2] halves)

// capsule ctor: __thiscall, args (float r, const vector4* a, const vector4* b); 0x64-byte object
struct shape_base { char pad[0x64]; };
struct capsule : shape_base {
    capsule(float r, const vector4* a, const vector4* b);    // 0x433310
};
// three-row placeholder record initialised by 0x4353a0 (s00434ad0 "Rows")
struct Rows {
    int m0, m4;
    vector4 v8, v18, v28;
    int m38, m3c, m40, m44, m48, m4c, m50, m54;
    Rows* init();                                // 0x4353a0 (returns this)
};
struct scene_iface { int query(capsule* c); };            // 0x472650 (ecx = the missile)
struct Missile;
struct Agent {                                            // 0x45a170 is Agent::swing_trace: ecx = agent
    int swing_trace(int hand, Missile* missile, void* blow, float* scale);
};
extern float frand();                             // 0x40e400
struct timer_iface { float elapsed(); };                  // 0x432c10 (ecx = the timer object)
mat4* identity4(mat4* out);                       // 0x40cd00 (cdecl; returns out)
struct mesh_anim { void set_time(const float* t); };   // 0x433190
static __forceinline float veclen(const vector4& v) { return (float)sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }

// constants
extern float g_gravity;        // 0x549ca8 == 0.0f (param_2 * 0.0f offset)
extern float g_f54b708;        // 0x54b708 (per-tick gravity coefficient)
extern float g_f54a6f0;        // 0x54a6f0 (0.01f)
extern float g_f590fa4;        // 0x590fa4 (1.7f: first-sample distance)
extern float g_f590e68;        // 0x590e68 (0.23f)
extern float g_f549cdc;        // 0x549cdc (1.0f)
extern float g_f591334;        // 0x591334 (20.0f)
extern float g_f590c74;        // 0x590c74 (0.08f)
extern float g_f59190c;        // 0x59190c (0.92f)
extern float g_f54a6d0;        // 0x54a6d0 (1e-5f)
extern float g_f590c90;        // 0x590c90 (40.0f)
extern float g_f590d94;        // 0x590d94 (0.6f)
extern float g_f591908;        // 0x591908 (0.0027f)
extern float g_f54b660;        // 0x54b660 (0.0001f)
extern float g_f54b8b0;        // 0x54b8b0 (1.5f)

struct Missile {
    char pad0[8];
    int id;                                      // 0x08
    char padc[0x10 - 0x0c];
    vector4 prev;                                // 0x10
    vector4 pos;                                 // 0x20
    vector4 start;                               // 0x30
    vector4 vel;                                 // 0x40
    entity* ent;                                 // 0x50
    char pad54[0x58 - 0x54];
    unsigned shooter;                            // 0x58 (the original shifts it with `shr`, so it is unsigned)
    float speed;                                 // 0x5c
    char pad60[0x68 - 0x60];
    int state;                                   // 0x68
    char pad6c[0x70 - 0x6c];
    int clock;                                   // 0x70
    __int64 t_start;                             // 0x78
    int item;                                    // 0x80
    char pad84[0x88 - 0x84];
    int troop;                                   // 0x88
    char pad8c[0x94 - 0x8c];
    float hit;                                   // 0x94
    float tumble;                                // 0x98
    char f9c;                                    // 0x9c
    char pad9d[0xa0 - 0x9d];
    int mode;                                    // 0xa0
    int sound;                                   // 0xa4
    float sound_t;                               // 0xa8

    void update(float dt);                       // 0x47d3e0
    void update_ranged_mesh();                   // 0x47d320
};

// ---- 0x47d3e0 ------------------------------------------------------------------------------------------

static __forceinline vector4 vmul(const vector4& v, const float& k) { return vector4(v.x * k, v.y * k, v.z * k); }
static __forceinline vector4 addv(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 subv(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

// @ 0x47d3e0
// One integration step of a flying projectile: keep the previous / frame-start position samples, advance the
// position, test the swept capsule against the scene, run the owner's swing trace on a hit, then update the
// owning mesh's frame and apply the drag / tumble.
void Missile::update(float dt)
{
    start = pos;
    if (state != 0)
        return;

    // v = vel*speed + (0,0,g)*dt; pos += v*dt
    vector4 grav(0.0f, 0.0f, g_f54b708);
    vector4 v = addv(vmul(vel, speed), vmul(grav, dt));
    vector4 step = vmul(v, dt);
    pos.x = step.x + pos.x;
    pos.y = step.y + pos.y;
    pos.z = step.z + pos.z;

    entity* e = ent;
    if (e->f140 == 0) {
        vector4 d = subv(pos, prev);
        if (d.length() > g_f590fa4)
            e->f140 = 1;
    }

    float reach;
    if (troop < 0)
        reach = 0.8f;
    else
        reach = (float)g_items[troop].f20c * g_f54a6f0;
    vector4 dir = vmul(vel, reach);
    vector4 pa = addv(dir, pos);
    vector4 pb = addv(dir, start);

    capsule cap(0.01f, &pb, &pa);
    char qhit = (char)((scene_iface*)this)->query(&cap);
    if (qhit == 0) {
        Rows rows;
        rows.init();
        // shooter agent: mission->pages[shooter>>4] + (shooter&0xf)*0x5928, then the pointer at its head
        void* agent = (void*)(*(char**)((char*)g_mission + 0x1c) + (shooter >> 4) * 4);
        Agent* a = *(Agent**)agent;
        a = (Agent*)((char*)a + (shooter & 0xf) * 0x5928);
        int r = a->swing_trace(0, this, &rows, &this->hit);
        if (r >= 3) {
            pos = ent->frame.o;
            qhit = 1;
        } else if (r != 0) {
            g_mission->remove_entity(ent->id);
            ent = 0;
            state = 2;
            t_start = g_time[clock * 2];
        } else {
            if (mode == 1) {
                vector4 p = addv(vmul(v, g_f590e68), pos);
                vector4 d = subv(g_mission->view, p);
                if (d.x * vel.x + d.y * vel.y + d.z * vel.z < 0.0f
                    && g_mission->view.z - 1.0f < p.z) {
                    mode = 3;
                    float l1 = d.out_length();
                    if (l1 < g_f591334) {
                        float l2 = subv(pos, prev).out_length();
                        int snd = 0x35;
                        int t = g_items[item].type & 0xff;
                        if (t == 9) snd = 0x30;
                        else if (t == 8) snd = 0x2f;
                        else {
                            int sub = g_items[item].f1d0 & 0xff000;
                            if (sub == 0x10000) snd = 0x32;
                            else if (sub == 0x30000) snd = 0x33;
                            else if (sub == 0x40000) snd = 0x31;
                            else if (sub == 0x20000) snd = 0x34;
                        }
                        float pitch = l2 * g_f54a6f0 + frand() * g_f590c74 + g_f59190c;
                        sound = g_sounds.play(g_sound_ids[snd], 0, &pa, pitch, 0);
                        sound_t = (float)g_time[0] * g_f54a6d0;
                    }
                }
            }
            // the r == 0 path rejoins *after* the item-sound block (the original jumps straight to the
            // code that follows the qhit block, so `item` is never dereferenced on this path).
            goto after_item_sound;
        }
    }

    if (item >= 0) {
        int t = item;
        if ((g_items[t].type & 0xff) != 0x0a)
            t = troop;
        if (t >= 0)
            g_items[t].sound.play(-0x34);
    }

after_item_sound:
    if (qhit != 0) {
        t_start = g_time[clock * 2];
        int sub = g_items[item].f1d0 & 0xff000;
        if (sub == 0x10000 || sub == 0x80000 || sub == 0x70000) {
            state = 3;
            g_mission->remove_entity(ent->id);
            ent = 0;
            t_start = g_time[clock * 2];
        } else {
            state = 1;
            update_ranged_mesh();
        }

        vector4 d = subv(g_mission->view, pos);
        if (d.normalize() < g_f590c90) {
            int snd = -1;
            int m = mode;
            if (m == 1 || m == 3 || (m == 0 && shooter == g_mission->local_id)) {
                int type = g_items[item].type & 0xff;
                if (type == 9) snd = (-(unsigned)(m != 1) & 7) + 0x37;
                else if (type == 8) snd = (-(unsigned)(m != 1) & 7) + 0x36;
                else {
                    int sub2 = g_items[item].f1d0 & 0xff000;
                    if (sub2 == 0x10000) snd = (-(unsigned)(m != 1) & 7) + 0x39;
                    else if (sub2 == 0x30000) snd = (-(unsigned)(m != 1) & 7) + 0x3a;
                    else if (sub2 == 0x20000) snd = (-(unsigned)(m != 1) & 7) + 0x3b;
                    else if (sub2 == 0x40000) snd = (-(unsigned)(m != 1) & 7) + 0x38;
                    else snd = (-(unsigned)(m != 1) & 7) + 0x3c;
                }
            }
            if (sound >= 0 && (float)g_time[0] * g_f54a6d0 - g_f590d94 < sound_t)
                FSOUND_StopSound(sound);
            if (snd >= 0)
                g_sounds.play(g_sound_ids[snd], 0, &pos, 1.0f, 0);
        }
    }

    if (ent == 0) {
        f9c = 0;
        return;
    }

    speed = veclen(v);
    v.normalize();
    speed = speed - (speed * speed * dt) * g_f591908;
    if (speed > g_f54b660) {
        vel = v;
        vel.normalize();
    }

    ent->frame.f = vel;
    ent->frame.u = vector4(0.0f, 0.0f, 1.0f);
    ((matrix&)ent->frame).orthonormalize();

    if (state == 0) {
        mesh* m = ent->meshes._Myfirst[0];
        identity4((mat4*)((char*)m + 0xcc));
        float a = 1.5707964f;
        ((mat4*)((char*)m + 0xcc))->rotate_su(&a);
        if (tumble > 0.0f) {
            float t = ((timer_iface*)((char*)this + 0x70))->elapsed() * tumble;
            float b = 1.5f - t;
            ((mat4*)((char*)m + 0xcc))->rotate_sf(&b);
        }
    }
    ent->frame.o = pos;
    f9c = 0;
}

// ---- 0x47dc50 ------------------------------------------------------------------------------------------

// 0x38-byte physics record (E56: {3 ints, vector<int>, vector<int>, 3 ints}); see s0047a0c0
struct E56 {
    int f0, f4, f8;                              // 0x00
    std::vector<int> a;                          // 0x0c
    std::vector<int> b;                          // 0x1c
    int f2c, f30, f34;                           // 0x2c
    E56() : f8(0), f2c(0), f30(0) {}             // f0, f4, f34 and the allocator lanes are left uninitialised
};

// @ 0x47dc50
// Tear down every physics world this object owns and reset its handle / vectors.
struct phys_owner {
    std::vector<E56> worlds;                     // 0x00 (_Myfirst at +0x04)
    int f10;                                     // 0x10 (set to -1)
    int f14, f18;                                // 0x14, 0x18 (cleared)
    char pad1c[0x3c - 0x1c];
    int f3c;                                     // 0x3c (cleared)
    void destroy_world(int idx);                 // 0x441360
    void reset();                                // 0x47dc50
};

void phys_owner::reset()
{
    unsigned i = 0;
    for (;;) {
        if (worlds._Myfirst == 0)
            break;
        if (i >= (unsigned)(worlds._Mylast - worlds._Myfirst))
            break;
        destroy_world(i);
        i++;
    }
    f10 = -1;
    f14 = 0;
    f18 = 0;
    worlds.resize(0, E56());
    f3c = 0;
}

// ---- 0x47dcd0 ------------------------------------------------------------------------------------------

// @ 0x47dcd0
// Snapshot the animation time into the mesh, rebuild the morph targets, then fold every morph
// (v70, 0x20-byte) and vertex (v50, 0x60-byte) record back onto its source, and re-upload.
void mesh::apply_anim(const float* t)
{
    ((mesh_anim*)this)->set_time(t);              // 0x433190
    update_morph();                               // 0x413320
    ensure_local();                               // 0x412ee0

    for (unsigned i = 0; v70._Myfirst != 0 && i < (unsigned)(v70._Mylast - v70._Myfirst); i++) {
        point20* p = &v70._Myfirst[i];
        p->a = p->b;
    }
    for (unsigned j = 0; v50._Myfirst != 0 && j < (unsigned)(v50._Mylast - v50._Myfirst); j++) {
        vertex60* p = &v50._Myfirst[j];
        p->pos = p->normal;
    }
    finish_load();                                // 0x419300
}
