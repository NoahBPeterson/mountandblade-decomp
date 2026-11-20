// mesh_game.cpp: TU 63 "mesh", part 7 (0x4832b0..0x484590): the mission's agent spawner (spawn, create, spawn_full,
// spawn_one, run_spawns) and the per-frame mission tick. The object is the Mission of src/game/mission.h. See MIGRATION.md.
#include "../../include/image_addrs.h"
#include "../../include/layout32.h"
#include "../../include/anim_set.h"      // AnimSet g_anim_set (0xa76628)
#include "./mesh_slot.h"
#include "../../include/game_data.h"   // GameData (parties, f78), PartyMgr
#include "../terrain_b/mission.h"
#include "../../include/pool.h"             // Pool<RecJ>::next_used 0x474300
#include "../../include/recj.h"
#include "../../include/fx.h"           // FxDef (the skeleton of an agent entity: ctor 0x428820, init 0x4288c0, set_env_count 0x46b9d0, f5c)
#include "../../include/phys_set.h"     // PhysSet::update 0x472dc0
#include "../render_batch.h"   // g_batch (0x5fe898): counter 0x418828
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/skin.h"      // Skin (g_anim.skin_vec._Myfirst)
#include "../mode_rec.h"   // g_mission_templates 0x5e2a00
#include "../../include/troop.h"   // Troop::auto_equip (0x4687e0)
#include "../strat/gui_manager.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
unsigned char g_mt_inited;                         // 0x5e2a09
float g_paused_time;                               // 0x5e2a0c


using namespace rgl;
// src/game/terrain_b/poly_mesh.h (pulled in through mesh_slot.h) declares a global ::vat with the same signature
#define vat rgl::vat

// ---- the 0x5928-byte agent record as the spawner fills it --------------------------------------------------------------
// (Agent in src/game/agent_obj.h and MAgent in terrain_b/agent_rec.h name the same bytes; neither declares the spawn
// words 0x94, 0x194.., 0x258, 0x56f0.. that this code writes, so the record is spelled out here in the order of the offsets.
// Names: Agent's where it has the field; the spawn words after spawn_full's arguments, which are pick_spawn's outputs
// and the spawn rule's fields (mode_rec.h SpawnRec: override_flags, the ai flags at +0x20, the override items).)
struct EntityG;
struct node_groups;
namespace rgl { struct lod_mesh2; }
struct AgentS {
    char pad0[4];
    int used;                           // +0x04 (0 = slot free)
    int id;                             // +0x08
    char padc[4];
    int kind;                           // +0x10 0 human / 1 horse
    int state;                          // +0x14 (1 = alive; Agent::state)
    unsigned flags;                     // +0x18
    char pad1c[0x2c - 0x1c];
    int horse;                          // +0x2c the mount's agent id (Agent::horse)
    int rider;                          // +0x30 the rider's agent id (Agent::rider)
    char pad34[0x50 - 0x34];
    vector4 pos;                        // +0x50
    vector4 dir;                        // +0x60
    char pad70[0x7c - 0x70];
    int f7c;                            // +0x7c (2 for the player's agent)
    float scale;                        // +0x80
    char pad84[0x88 - 0x84];
    int troop;                          // +0x88
    ItemRef item;                       // +0x8c the horse item (Agent::f8c; a horse agent's own item)
    int seed;                           // +0x94 the equipment roll seed (pick_spawn's out_seed; build_appearance, Troop::auto_equip)
    char pad98[0xd4 - 0x98];
    AnimSlot anim[2];                   // +0xd4 (stride 0x2c, Agent::anim)
    AnimSet* anim_set;                  // +0x12c (&g_anim_set; Agent::defs)
    int weapon_flags;                   // +0x130 (Agent::f130: weapon class flags)
    char pad134[0x13c - 0x134];
    rgl::lod_mesh2* item_meshes[17];    // +0x13c the item display meshes build_appearance / setup_item_meshes fill (Agent::slot_mesh.. slot_mesh4)
    node_groups* body_a;                // +0x180
    node_groups* body_b;                // +0x184
    char pad188[0x194 - 0x188];
    rgl::lod_mesh2* extra[4];           // +0x194 build_appearance's extra lods
    int extra_count;                    // +0x1a4
    rgl::lod_mesh2* body_lod;           // +0x1a8 build_appearance's out_body
    char pad1ac[0x1bc - 0x1ac];
    int party;                          // +0x1bc (pick_spawn's out_party; Agent::party)
    int party_stack;                    // +0x1c0 (pick_spawn's out_stack)
    int team;                           // +0x1c4 (pick_spawn's out_side; Agent::team)
    char pad1c8[0x1d0 - 0x1c8];
    ItemRef equipment[10];              // +0x1d0 (Agent::equipment)
    unsigned char slot_flag[10];        // +0x220
    char pad22a[0x240 - 0x22a];
    int armor[3];                       // +0x240 (build_appearance's stats; Agent::armor)
    float equip_value;                  // +0x24c (Agent::f24c)
    EntityG* ent;                       // +0x250
    char pad254[0x258 - 0x254];
    int ai_flags;                       // +0x258 the spawn rule's ai flags (SpawnRec +0x20; 0x10 = aif_start_alarmed)
    char pad25c[0x2c4 - 0x25c];
    int alarmed;                        // +0x2c4 (Agent::f2c4 bit 0 = alarmed)
    int behavior;                       // +0x2c8 (the ai flags again; Agent::f2c8)
    char pad2cc[0x55d8 - 0x2cc];
    vector4 userdata;                   // +0x55d8 (a copy of dir)
    char pad55e8[0x56f0 - 0x55e8];
    unsigned override_flags;            // +0x56f0 the spawn rule's override flags (SpawnRec +8; Troop::auto_equip reads bits 9, 28)
    int override_items[8];              // +0x56f4 (SpawnRec::override_items)
    int num_override_items;             // +0x5714
    int entry_no;                       // +0x5718 (Agent::entry_no)
    float f571c;                        // +0x571c (a copy of health)
    float max_health;                   // +0x5720 (Agent::max_health)
    float health;                       // +0x5724 (Agent::health)
    char pad5728[0x5928 - 0x5728];

    // the members are the ones of Agent (game/agent_obj.h) and MAgent (terrain_b/agent_rec.h), which name the same bytes
    void reset() { ((MAgent*)this)->reset(); }                              // 0x472f90
    float equipment_value() const { return ((const Agent*)this)->equipment_value(); }      // 0x436050
    void spawn_corpse() { ((Agent*)this)->spawn_corpse(); }                 // 0x4554c0
    void cycle_slot(int i) { ((Agent*)this)->cycle_slot(i); }               // 0x44bc30
    void on_wield_reset() { ((Agent*)this)->on_wield_reset(); }             // 0x445a20
    bool is_enemy(AgentS* a) { return ((Agent*)this)->is_enemy((const Agent*)a); }    // 0x432350 (ecx = commander record)
    bool is_friend(AgentS* a) { return ((Agent*)this)->is_friend((const Agent*)a); }  // 0x4323a0 (ecx = commander record)
    void update(float dt) { ((Agent*)this)->update(dt); }                   // 0x45f660
    void tick() { ((Agent*)this)->tick(); }                                 // 0x45ff30
};
MB_LAYOUT32_ASSERT(agents_size_check, (sizeof(AgentS) == 0x5928));

// ---- the id pool embedded in the mission object (include/pool.h.s Pool, with this record) -------------------------------
template<class T> struct PoolS {
    int next_id;                        // +0
    std::vector<int> free_ids;          // +4
    std::vector<T*> pages;              // +0x14
    int serial;                         // +0x24
    int alloc() { return ((Pool<MAgent>*)this)->alloc(); }   // 0x478020 (thiscall, 16-record pages: terrain_b.cpp's Pool<MAgent>::alloc)
};

// ---- the objects behind the game pointer ---------------------------------------------------------------------------
// 0x4625d0 is snd_slot::update (ui/item_ref.cpp, the sound slot of a wave record); the header (ui/mission_ui.h) brings another layout, so only the member
struct snd_slot {
    void update(int arg);   // 0x4625d0 (thiscall, ret 4)
};
struct SndSlotS {
    int pad0[0x78 / 4];
    void update(int n) { ((snd_slot*)this)->update(n); }       // 0x4625d0 (thiscall, ret 4)
};
// an item reference (ItemRef, game/item_ref.h): 0x434df0 is ItemRef::hit_points, 0x434eb0 ItemRef::body_armor
struct ItemStackG {
    unsigned a, b;
    int value() const { return ((ItemRef*)this)->hit_points(); }        // 0x434df0
};
struct ItemStack2 {
    unsigned a, b;
    int value() const { return ((ItemRef*)this)->body_armor(); }        // 0x434eb0
};
struct EntityG {                        // the 0x30c-byte script entity (rgl::strategic_entity)
    char pad0[0xc];
    int id;                             // +0xc
    int type;                           // +0x10
    char pad14[0x98 - 0x14];
    void* shape;                        // +0x98
    char pad9c[0x304 - 0x9c];
    FxDef* skel;                        // +0x304
    char pad308[4];
};
MB_LAYOUT32_ASSERT(EntityG_size_check, sizeof(EntityG) == 0x30c);
// 0x439000 is node_groups::cap_all (agent2/agent_types.h: the display mesh's node lists of a body)
struct node_groups {
    void cap_all(const float* limit);   // 0x439000
};
#include "./mesh_build.h"          // setup_item_meshes (0x47cfe0)
#include "../skill_cap.h"
#include "../str_table.h"                 // StrTable g_strtable (0x5e2a18; its lookup takes the key by reference)
long long pick_slot_item(int a, int b, int c, int d, int* e, char f, char* g);   // 0x46b190

// the mission object as the spawner sees it
#define POOL (*(PoolS<AgentS>*)&agents)
// Mission members in mission.h padding (reported for mission.h; named accessors meanwhile)

// @ 0x4832b0 -- the shared "initialise one agent" routine: applies the troop's skin mesh, builds the collision capsule from
// the record's scale, runs the equipment fillers and finishes with the corpse spawn
NOINL void Mission::spawn(AgentS* a)
{
    if (a->kind == 0) {
        a->anim_set = &g_anim_set;
        a->anim[1].f10 = 7;
        Troop* t = &g_game->troops[a->troop];
        if (t->flags & 0x10) {
            int n = 1;
            PartyMgr& pm = g_game->parties;
            do {
                int idx = pm.lookup(g_game->main_party);
                Party* rec = pm.at(idx);
                int cnt = rec->count;
                int found = 0;
                for (int i = 0; i < cnt; i++) {
                    if (rec->stacks[i].flags & 1)
                        break;
                    found++;
                }
                if (n >= found)
                    break;
                n++;
            } while (1);
        }
        if (a->id != this->player_id) {
            unsigned long long f = t->flags;
            if (!(f & 0x10) || !(f & 0x1000))
                t->auto_equip(a->seed, 0x100e, (a->override_flags >> 28) & 0xffffff01, (a->override_flags >> 9) & 0xffffff01);
        }
    } else if (a->kind == 1) {
        a->anim_set = &g_anim_set;
    }
    EntityG* e = (EntityG*)((rgl::world*)scene)->create_entity(9);
    e->id = a->id;
    e->type = 0xb;
    a->ent = e;
    void* mem = operator new(0x7c);
    e->skel = mem ? new (mem) FxDef(2) : 0;
    e->skel->set_env_count(2);
    e->skel->keep_weight = 0;
    shape_base* shp;
    // any other kind skips the shape (0x4835fc, the jne at 0x483566) and falls through to the common tail
    if (a->kind == 0) {
        unsigned skin_i = g_game->troops[a->troop].flags32 & 0xf;
        FxRes* obj;
        if (skin_i < (unsigned)g_anim.char_count)
            obj = (FxRes*)g_anim.skin_vec._Myfirst[skin_i].skeleton;
        else
            obj = 0;
        e->skel->init(obj);
        skin_i = g_game->troops[a->troop].flags32 & 0xf;
        float h;
        if (skin_i < (unsigned)g_anim.char_count)
            h = g_anim.skin_vec._Myfirst[skin_i].scale;
        else
            h = 1.0f;
        a->scale = h;
        if ((mem = operator new(0x64)) != 0) {
            vector4 b = vector4(0.0f, 0.0f, 1.3f) * a->scale;
            vector4 c = vector4(0.0f, 0.01f, 2.3f) * a->scale;
            shp = new (mem) capsule(a->scale * 0.3f, &c, &b);
        } else
            shp = 0;
        e->shape = shp;
    } else if (a->kind == 1) {
        {
            tstring name("skel_horse");
            e->skel->init((FxRes*)g_resmgr.get_object70(name));
        }
        if ((mem = operator new(0x64)) != 0) {
            vector4 b(0.0f, 0.8f, 0.8f);
            vector4 c(0.0f, -0.3f, 0.7f);
            shp = new (mem) capsule(0.4f, &c, &b);
        } else
            shp = 0;
        e->shape = shp;
    }
    pick_slots((AgentEq*)a);
    build_appearance((rgl::strategic_entity*)e, a->kind, a->troop, a->seed, &a->item, a->item_meshes, a->armor,
                     a->extra, &a->extra_count, &a->body_lod, (EquipSet*)a->equipment, a->id);   // 0x482240
    setup_item_meshes((SkelBodyV*)e, a->kind, a->troop, a->item_meshes, &a->weapon_flags, a->equipment, a->id);
    a->equip_value = a->equipment_value();
    this->post_spawn((MAgent*)a);
    if (a->kind == 0) {
        if (a->body_a) {
            float r = 20.0f;
            a->body_a->cap_all(&r);
        }
        if (a->body_b) {
            float r = 20.0f;
            a->body_b->cap_all(&r);
        }
    }
    a->spawn_corpse();
}

// @ 0x4836e0 -- hand out an id from the embedded pool, reset the record, fill the spawn fields and spawn it
int Mission::create(const matrix* m, const ItemStackG* it, int troop)
{
    int own = POOL.alloc();
    AgentS* a = &POOL.pages[(unsigned)own >> 4][own & 0xf];
    a->reset();
    a->item.item = it->a;
    a->item.m4 = it->b;
    a->troop = troop;
    a->pos = m->o;
    a->team = -1;
    a->kind = 1;
    a->ai_flags = 0;
    a->dir = m->f;
    a->userdata = a->dir;
    a->state |= 1;
    this->spawn(a);
    for (int i = 0; i < 10; i++) {
        a->equipment[i].item = -1;
        a->slot_flag[i] = 1;
    }
    int v = ((ItemStackG*)&a->item)->value();
    a->max_health = (float)v;
    a->health = (float)v;
    a->armor[0] = ((ItemStack2*)it)->value();
    a->armor[1] = ((ItemStack2*)it)->value();
    a->armor[2] = ((ItemStack2*)it)->value();
    g_script.param1 = own;
    struct SndBody { int num; SndSlotS* slots; };
    SndBody* p = (SndBody*)&g_mission_templates[this->mode].triggers;   // the template's sound bank: count +0x4c, slots +0x50
    for (int i = 0; i < p->num; i++)
        p->slots[i].update(-25);
    return own;
}

// @ 0x483870 -- a 10-argument agent creator: allocates a record from the pool, resets it, fills the spawn fields from a matrix
// and a wave record, applies the skin-based scale/height, runs spawn, then optionally creates a carried-item agent through
// pick_slot_item + create and announces the wave on the ui queue
NOINL int Mission::spawn_full(const matrix* m, int troop, int a1, int a2, int a3, int a4, int a5, int a6, int cnt, int* arr)
{
    int own = POOL.alloc();
    if (troop == g_game->player) {
        this->player_id = own;
        this->f6598 = m->o;
    }
    AgentS* a = &POOL.pages[(unsigned)own >> 4][own & 0xf];
    a->reset();
    a->party = a1;
    a->party_stack = a2;
    a->seed = a4;
    a->override_flags = a5;
    a->troop = troop;
    a->item.item = -1;
    a->num_override_items = cnt;
    for (int i = 0; i < cnt; i++)
        a->override_items[i] = arr[i];
    a->team = a3;
    a->pos = m->o;
    a->pos.z = a->pos.z + 0.3f;
    a->dir = m->f;
    a->userdata = a->dir;

    struct Helpers {
        static int max_hp(Troop* t) {
            int base = t->lvl58;
            int v = t->nib_b[0x12] & 0xf;
            if (v < 0)
                v = 0;
            else if (v > g_skill_tbl[36].max_rank)
                v = g_skill_tbl[36].max_rank;
            return base + v * 2 + 0x23;
        }
        static char mode_flag1(const Mission* m) {
            return ((g_mission_templates[m->mode].flags >> 1) & 1) != 0;
        }
    };
    float h = (float)Helpers::max_hp(&g_game->troops[troop]);
    a->max_health = h;
    if (g_game->troops[troop].flags & 0x10) {
        a->health = g_game->troops[troop].hp;
    } else {
        a->health = h;
        int pid;
        if (a1 == (pid = g_game->main_party)) {
            PartyMgr* pm = &g_game->parties;
            if (pm->get_party(pid)->zoom > 0.5f)
                a->health = (1.5f - pm->get_party(pid)->zoom) * h;
        }
    }
    a->f571c = a->health;
    a->kind = g_game->troops[troop].level48;
    a->state = 1;
    a->ai_flags = a6;
    a->alarmed = 0;
    if (a6 & 0x10)
        a->alarmed = 1;
    a->override_flags = a5;
    if (a->id == this->player_id) {
        a->flags |= 4;
        a->f7c = 2;
    }
    a->state = 1;
    for (int i = 0; i < 10; i++) {
        a->equipment[i].item = -1;
        a->slot_flag[i] = 1;
    }
    this->spawn(a);
    a->behavior = a6;
    {
        char found;
        long long item = pick_slot_item(troop, 8, a5, cnt, arr, 0, &found);
        if ((int)item >= 0) {
            int nid = this->create(m, (const ItemStackG*)&item, troop);
            POOL.pages[(unsigned)nid >> 4][nid & 0xf].rider = own;
            a->horse = nid;
            POOL.pages[(unsigned)nid >> 4][nid & 0xf].party = a1;
            POOL.pages[(unsigned)nid >> 4][nid & 0xf].party_stack = a2;
            if (troop == g_game->player)
                this->f60 = nid;
        }
    }
    if (a->id == this->player_id) {
        if ((terrain.f158 & 1) || Helpers::mode_flag1(this)) {
            a->cycle_slot(0);
            a->cycle_slot(1);
        }
    }
    a->on_wield_reset();
    int t1 = a->team;
    if (t1 < 0 || t1 >= 8) t1 = 7;
    if (this->teams[t1].leader < 0 || a->id == this->player_id) {
        int t2 = a->team;
        if (t2 < 0 || t2 >= 8) t2 = 7;
        this->teams[t2].leader = a->id;
    }
    if (this->player_id >= 0) {
        if (this->announce_enemies != 0 && this->agents[this->player_id].is_enemy((const Agent*)a)) {
            this->announce_enemies = 0;
            tstring msg("ui_new_enemies_have_arrived");
            tstring* t2 = g_strtable.lookup(msg);
            if (g_gui.enabled)
                g_gui.ring.add(*t2, -1);
        } else if (this->announce_reinforcements != 0 && this->agents[this->player_id].is_friend((const Agent*)a)) {
            this->announce_reinforcements = 0;
            tstring msg("ui_reinforcements_have_arrived");
            tstring* t2 = g_strtable.lookup(msg);
            if (g_gui.enabled)
                g_gui.ring.add(*t2, -1);
        }
    }
    g_script.param1 = own;
    struct SndBody { int num; SndSlotS* slots; };
    SndBody* p = (SndBody*)&g_mission_templates[this->mode].triggers;   // the template's sound bank: count +0x4c, slots +0x50
    for (int i = 0; i < p->num; i++)
        p->slots[i].update(-25);
    return own;
}

// @ 0x483de0 -- read a wave record through 0x473a60 and forward it to spawn_full, then store the record index on the agent
NOINL bool Mission::spawn_one(int id)
{
    int o7, o5, o3, o2, o6;
    int orig = id;
    pick_spawn(id, &o2, &o3, &id, &o5, &o6, &o7);
    if (id < 0)
        return false;
    SpawnRec* r = &g_mission_templates[this->mode].spawns[orig];
    int nid = spawn_full(&recs[r->id].m, id, o2, o3, o5, o7,
                         r->override_flags, r->f20, r->num_override_items, r->override_items);
    if (nid >= 0)
        POOL.pages[(unsigned)nid >> 4][nid & 0xf].entry_no = orig;
    return true;
}

// @ 0x483eb0 -- up to 100 passes over the current wave set, spawning every pending entry whose done flag is clear
void Mission::run_spawns()
{
    MissionTemplate* p = &g_mission_templates[this->mode];
    char done = 0;
    int iter = 0;
    do {
        if (done)
            break;
        done = 1;
        for (int i = 0; i < p->n_spawns; i++) {
            SpawnRec* r = &p->spawns[i];
            if (r->f18 > 0 && this->done[i] == 0 && this->spawn_one(i)) {     // f18: troops still to spawn, f1c: spawned
                r->f18--;
                r->f1c++;
                replace_mark(r->id);
                done = 0;
            }
        }
        iter++;
    } while (iter < 100);
}

// ---- 0x483f60: the per-frame mission tick ----------------------------------------------------------------------------------
// the 0xb0-byte prop record of the pool at Mission+0x30 (RecJ, src/include/recj.h, as this tick sees it)
// 0x47d3e0 is MissileU::update (mesh_scene.cpp: the flying projectile record of the prop pool); 0x467c50 is Prop::update (terrain_a/terrain_a.cpp)
struct MissileU {
    void update(float dt);   // 0x47d3e0
};
struct Prop {
    void update(float dt);   // 0x467c50
};
struct PropRecF {
    char pad0[4];
    int used;                           // +0x04 (0 = the prop retires this frame)
    char pad8[0x28 - 8];
    float f28;                          // +0x28 (a life value; < -100 frees the prop)
    char pad2c[0x50 - 0x2c];
    char* f50;                          // +0x50 (the prop's entity; 0 = none)
    char pad54[0x68 - 0x54];
    int state;                          // +0x68 (1/2/3 age out)
    char pad6c[0x70 - 0x6c];
    int clock;                          // +0x70 (index into g_ticks)
    char pad74[4];
    long long start;                    // +0x78
    char pad80[0xb0 - 0x80];
    void update(float dt) { ((MissileU*)this)->update(dt); }        // 0x47d3e0 (thiscall, ret 4)
};
struct PtrTableF {                      // the paged pointer table (count; the page vector's first member sits at +0x18)
    int count;                          // +0
    char pad4[0x14];
    char** pages;                       // +0x18
    char pad1c[4];
    // the iterators are the pool's own members (ecx = the table, not the record)
    void next_prop(unsigned* i) { ((Pool<RecJ>*)this)->next_used(i); }      // 0x474300 (thiscall, ret 4)
    void next_agent(unsigned* i) { ((AgentTable*)this)->next_used(i); }   // 0x43b160 (AgentTable::next_used) (thiscall, ret 4)
    AgentS* agent(unsigned i) { return (AgentS*)pages[i >> 4] + (i & 0xf); }
    __forceinline unsigned first_agent()        // the agent table's first_used (0x43b1b0 inlined)
    {
        unsigned i = 0;
        if (0 < count) {
            do {
                if (((AgentS*)pages[i >> 4])[i & 0xf].used != 0)
                    break;
                i++;
            } while ((int)i < count);
        }
        return i;
    }
};
MB_LAYOUT32_ASSERT(assert_ptrtable, (sizeof(PtrTableF) == 0x20));
struct PropAnim {                       // the 0x1c8-byte prop tween record (PlacedObj)
    char pad0[0x1c8];
    void update(float dt) { ((Prop*)this)->update(dt); }       // 0x467c50 (thiscall, ret 4)
};
struct SndSetF { char pad0[0x4c]; int count; SndSlotS* slots; char pad54[4]; };   // the 0x58-byte mode record (+0x4c count, +0x50 slots)
struct SkyF {                           // the sky record at Mission+0x65a8
    char pad0[0x48];
    float f48;                          // +0x48 (the mission clock in seconds)
    char pad4c[0x1f8 - 0x4c];
    float f1f8;                         // +0x1f8 (the sky time)
    char pad1fc[0x208 - 0x1fc];
    float f208;                         // +0x208 (the sky time again)
};
struct psys_rec {                       // the shadow object's target skin (ShadowMap::target, g_shadow_map): a texture reference at +0x48
    char pad0[0x48];
    char* scene;
};
// Mission members in mission.h padding (reported for mission.h; named accessors meanwhile)
static inline int& water_obj(Mission* m) { return *(int*)((char*)m + 0x63c0); }           // the water object (Terrain::destroy_water)
static inline SkyF*& sky(Mission* m) { return *(SkyF**)&m->f65a8; }                       // +0x65a8 is the sky record pointer (mission.h: float)
static inline std::vector<char*>& retired(Mission* m) { return *(std::vector<char*>*)&m->v5dfc; }   // the retire list holds entity pointers
static inline PtrTableF& agent_table(Mission* m) { return *(PtrTableF*)&m->agents; }
#include "../../include/fx.h"
extern "C" int rand();                 // 0x536a9f

// @ 0x483f60 -- the per-frame mission tick: the shadow-object upkeep, the 100 kHz frame clock, the carried-item sweep, the
// spawn pass, the wave sound slots, the prop pool (per-entry update + retire, list overflow cull), the prop tweens, three agent
// passes (non-horses, horses, ticks), the mission timers/sky clock and finally world::step + the frame counter
void Mission::frame(float dt, bool spawn)
{
    psys_rec* ps = (psys_rec*)g_shadow_map.target;
    bool scene_changed = false;
    if (ps != 0)
        scene_changed = *(int*)(ps->scene + 0x50) != g_batch.counter;
    if (scene_changed) {
        unsigned flags = g_game->levels[scene_idx].flags;
        int* water = &water_obj(this);
        if (flags & 0x100)
            g_terrain.destroy_water(water);        // 0x4814a0
        else if (!(flags & 1))
            ((ShadowMapP*)&g_shadow_map)->render();     // this is &g_shadow_map, not the target (0x47fef0)
    }

    if (!g_mt_inited && dt > 0.0f)
        g_phys.update(dt);                       // 0x472dc0 (g_phys 0xa181f8)

    long long t = (long long)(dt * 100000.0f);
    g_tick_dt[2] = t;
    g_ticks[2] += t;
    g_step_tbl[2] = dt;

    // every sixteenth frame the carried-item records restart from their first matrix
    if ((terrain.f180 & 0xf) == 1) {
        MarkRec* c = recs;
        for (int k = 0; k < 0x40; k++) {
            c[k].cnt = 0;
            c[k].cnt2 = 0;
            c[k].floor = 0;
            c[k].m = c[k].m1;               // plain matrix copies (rep movsd x16 at 0x484060 / 0x484086), not normalize
            c[k].m2 = c[k].m1;
        }
    }

    if (spawn)
        run_spawns();

    if (terrain.f180 != 0) {
        struct SndBody { int num; SndSlotS* slots; };
        SndBody* p = (SndBody*)&g_mission_templates[this->mode].triggers;
        for (int i = 0; i < p->num; i++)
            p->slots[i].update(0x10);
    }

    // the prop pool: per-entry update, retire the finished ones, and steady the used slots
    {
    unsigned i = pool_j(this).first_used();
    while (i < (unsigned)pool_j(this).count) {
        PropRecF* p = (PropRecF*)&pool_j(this).pages[i >> 4][i & 0xf];
        p->update(dt);
        if ((p->state == 2 || p->state == 3 || p->state == 1) && (g_ticks[p->clock] - p->start) * 1e-5f > 1.0f
            || p->f28 < -100.0f) {
            if (p->f50 != 0)
                retired(this).push_back(p->f50);
            p->used = 0;
            pool_j(this).free_ids.push_back(i);
        }
        pool_j(this).next_used(&i);
    }

    // the retire list overflows: drop a random prop and compact
    if (retired(this).size() > 0x300) {
        int n = (int)retired(this).size();                    // signed: cmp edi,1; jg / cdq; idiv (0x4842a5)
        int k = n > 1 ? rand() % n : 0;
        ((rgl::world*)scene)->remove_entity(*(int*)(retired(this)[k] + 8));       // 0x41f0d0
        retired(this)[k] = retired(this)[retired(this).size() - 1];
        retired(this).resize(retired(this).size() - 1);
    }

    for (unsigned k = 0; k < props.size(); k++)
        ((PropAnim*)&props[k])->update(dt);

    // agents: everything but the horses, then the horses, then every agent's tick
    for (i = agent_table(this).first_agent(); i < (unsigned)agent_table(this).count; agent_table(this).next_agent(&i))
        if (agent_table(this).agent(i)->kind != 1)
            agent_table(this).agent(i)->update(dt);
    for (i = agent_table(this).first_agent(); i < (unsigned)agent_table(this).count; agent_table(this).next_agent(&i))
        if (agent_table(this).agent(i)->kind == 1)
            agent_table(this).agent(i)->update(dt);
    for (i = agent_table(this).first_agent(); i < (unsigned)agent_table(this).count; agent_table(this).next_agent(&i))
        agent_table(this).agent(i)->tick();
    }

    update_prop_flags();

    // the sky clock and the mission timers
    if (sky(this) != 0) {
        float t = f65ac = f65ac + dt * 0.04f;
        if (t > 100.0f)
            f65ac = (float)fmod((double)t, 1.0);     // 0x4844df: the divisor is the double at 0x54a5d8 (1.0), the 100.0 is only the wrap test (0x54ab28)
        sky(this)->f1f8 = f65ac;
        sky(this)->f208 = f65ac;
        sky(this)->f48 = (float)(g_ticks[2] % 1000000000) * 7.5e-6f;
    }

    update_team_stats();
    ((rgl::world*)scene)->step(dt);                  // 0x427f40
    update_team_counts();
    terrain.f180++;
    if (g_mt_inited)
        g_paused_time += dt;
}
