// Slice s0044bc30: Agent::cycle_slot (0x44bc30, next usable held item for the weapon / shield selector) and
// Mission::spawn_missile (0x44be50, creates the flying entity and the missile record for a shot or throw).
// Class and field names are provisional; Agent/Pair/ItemKind layouts follow s00437f00 and s0043d040.
#include <math.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// rgl string with a read cursor (16 bytes), as in s00434ad0.h
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
};

struct mesh {
    char pad0[0xcc];
    matrix xform;                               // +0xcc
    mesh* clone();                              // 0x41c6d0
};
struct resmgr {
    mesh* get_object(const tstring& name);      // 0x419540
};
extern resmgr g_resmgr;                         // 0x5d6fa0

struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                        // +0x1c8 (low byte = itp_type_*)
    char pad1cc[0x238 - 0x1cc];
    mesh* find_mesh(int a, unsigned b, int shift);   // 0x433cb0 (ret 0xc)
};
extern ItemKind* g_items;                       // 0x5e29d0
extern int g_item_count;                        // 0x5e29d4

struct ItemRef {
    int item;
    unsigned b;                                 // +4 (byte 3 = modifier id)
    int max_amount();                           // 0x434b70
    tstring mesh_name();                        // 0x434b20
    int stat_21x(int which);                    // 0x434cb0
};

struct Agent {
    char pad0[0x2c];
    int m2c;                                    // +0x2c (mount agent id, -1 = on foot)
    char pad30[0xac - 0x30];
    float fac;                                  // +0xac
    char padb0[0x1d0 - 0xb0];
    ItemRef held[11];                           // +0x1d0
    int pad228;
    int slots[5];                               // +0x22c (slots[0] = current weapon, slots[1] = shield?, slots[4] at +0x23c)
    char pad240[0x56c8 - 0x240];
    float f56c8;                                // +0x56c8
    char pad56cc[0x5764 - 0x56cc];
    int f5764;                                  // +0x5764
    char pad5768[0x57a4 - 0x5768];
    int f57a4;                                  // +0x57a4
    char pad57a8[0x5928 - 0x57a8];

    void update_equipment_display();            // 0x443aa0 (s00443aa0)
    void f444370();                             // 0x444370
    float attack_speed();                       // 0x438360
    void cycle_slot(int which);                 // 0x44bc30
};

// @ 0x44bc30
void Agent::cycle_slot(int which)
{
    int old = slots[which];
    bool found = false;
    int old_item = -1;
    if (old >= 0)
        old_item = held[old].item;
    if (which == 0 || slots[0] < 0 || slots[1] < 0 || slots[4] == 0) {
        int n = 0;
        do {
            if (n >= 4)
                break;
            int v = (slots[which] + 1) % 4;
            slots[which] = v;
            if (which == 0 || slots[1] != slots[0]) {
                int it = held[v].item;
                if (it >= 0) {
                    ItemKind* ik = &g_items[it];
                    if (which == 0) {
                        unsigned fl = ik->type_flags;
                        if ((char)fl == 10) {
                            int cnt = held[v].b & 0xff;
                            int val = held[v].max_amount();
                            if (val - cnt > 0 && held[slots[0]].item != old_item) {
                                if (m2c < 0 || !(ik->type_flags & 0x10000000))
                                    found = true;
                            }
                        } else if (fl & 0x400000) {
                            if (m2c < 0 || !(fl & 0x10000000))
                                found = true;
                        }
                    } else {
                        unsigned fl = ik->type_flags;
                        unsigned t = fl & 0xff;
                        if ((t == 7 || ((fl & 0x800000) && t == 7)) &&
                            (m2c < 0 || !(fl & 0x10000000)) && (int)(held[v].b & 0xffff) > 0)
                            found = true;
                    }
                }
            }
            n++;
        } while (!found);
        if (which != 0 && old >= slots[which])
            slots[which] = -1;
        if (!found)
            slots[which] = -1;
    } else {
        slots[0] = -1;
    }
    slots[4] = 0;
    if (slots[0] >= 0) {
        int it = held[slots[0]].item;
        if (it >= 0 && (g_items[it].type_flags & 0x200000))
            slots[4] = 1;
    }
    update_equipment_display();
    f444370();
    f5764 = f57a4 = 0xcb989680;
}

// ---- Mission::spawn_missile --------------------------------------------------------------------------------
struct Entity {                                 // vtable 0x54b460 and derived (see s00428240/world.h); +0x58 local frame
    virtual ~Entity();                          // 0
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual float get_fbc();                    // 5 (+0x14)  0x41fd80
    virtual void set_fbc(const float* p);       // 6 (+0x18)  0x41fd90
    char pad4[0x58 - 4];
    matrix local;                               // +0x58
    char pad98[0xc4 - 0x98];
    vector4 vc4;                                // +0xc4
    char padd4[0x140 - 0xd4];
    char f140;                                  // +0x140 (visible)
    char pad141[0x15c - 0x141];
    void add_mesh(mesh* m);                     // 0x442110
};
struct World {
    Entity* create_entity(int type_id);         // 0x428440
};
struct Missile {                                // 0xb0 bytes, 16 per page
    char pad0[0x10];
    vector4 p10;                                // +0x10
    vector4 p20;                                // +0x20
    char pad30[0x40 - 0x30];
    vector4 p40;                                // +0x40
    Entity* ent;                                // +0x50
    char pad54[0x58 - 0x54];
    unsigned shooter;                           // +0x58
    float f5c;                                  // +0x5c
    float f60;                                  // +0x60
    float f64;                                  // +0x64
    char pad68[0x80 - 0x68];
    ItemRef launcher;                           // +0x80
    ItemRef ammo;                               // +0x88
    char pad90[0xb0 - 0x90];
    void init();                                // 0x467fb0
};
struct Missile;
struct MissilePool {                            // at Mission+0x30 (see 0x440c90): free-id list, page table, counter
    int base;                                   // +0
    std::vector<int> free_ids;                  // +4
    std::vector<Missile*> pages;                // +0x14 (first at +0x18)
    char pad24[4];
    int next_id;                                // +0x24
    int alloc();                                // 0x440c90
    Missile& operator[](unsigned id) { Missile** pp = &pages[id >> 4]; return *(*pp + (id & 0xf)); }
};
struct Mission {
    World* world;                               // +0
    char pad4[0x18 - 4];
    std::vector<Agent*> agents;                 // +0x18 (16 agents per page)
    char pad28[0x30 - 0x28];
    MissilePool missiles;                       // +0x30
    int spawn_missile(const vector4* pos, const vector4* dir, ItemRef* launcher, ItemRef* ammo,
                      unsigned shooter_id);     // 0x44be50
    Agent& agent(unsigned id) { return agents[id >> 4][id & 0xf]; }
};
rgl::matrix* identity_pos(rgl::matrix* out);    // 0x40cd00 (cdecl, returns out)

// @ 0x44be50
int Mission::spawn_missile(const vector4* pos, const vector4* dir, ItemRef* launcher, ItemRef* ammo,
                           unsigned shooter_id)
{
    Agent* shooter = &agent(shooter_id);
    float mount_f;
    if (shooter->m2c >= 0)
        mount_f = (float)cos(shooter->f56c8) * agent(shooter->m2c).fac;
    else
        mount_f = shooter->fac;
    int id = missiles.alloc();
    Entity* e = world->create_entity(9);
    matrix ident;
    identity_pos(&ident);
    mesh* m;
    if ((g_items[ammo->item].type_flags & 0xff) == 10) {
        m = g_resmgr.get_object(ammo->mesh_name());
    } else {
        m = 0;
        if (ammo->item >= 0 && ammo->item < g_item_count)
            m = g_items[ammo->item].find_mesh(0, 0x20000000, ((signed char*)ammo)[7]);
        if (!m) {
            tstring name("flying_missile");
            m = g_resmgr.get_object(name);
        }
    }
    if (m) {
        mesh* c = m->clone();
        c->xform = ident;
        e->add_mesh(c);
    }
    e->local.f = *dir;
    e->local.u = vector4(0.0f, 0.0f, 1.0f);
    e->local.orthonormalize();
    e->local.o = *pos;
    float half = 0.5f;
    e->set_fbc(&half);
    e->vc4 = vector4(0.0f, e->get_fbc(), 0.0f);
    e->f140 = 0;
    missiles[id].ent = e;
    missiles[id].p20 = *pos;
    missiles[id].p10 = *pos;
    missiles[id].p40 = *dir;
    missiles[id].f64 = (float)launcher->stat_21x(1);
    unsigned lt = g_items[launcher->item].type_flags & 0xff;
    if (lt == 8 || lt == 9)
        missiles[id].f64 += (float)ammo->stat_21x(1);
    missiles[id].f60 = shooter->attack_speed();
    missiles[id].f5c = missiles[id].f60;
    missiles[id].f5c = mount_f + missiles[id].f5c;
    missiles[id].shooter = shooter_id;
    missiles[id].launcher = *launcher;
    missiles[id].ammo = *ammo;
    missiles[id].init();
    return id;
}
