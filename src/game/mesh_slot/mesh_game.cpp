// mesh_game.cpp: TU 63 "mesh", part 7 (0x4832b0..0x484590): the mission's agent spawner (spawn, create, spawn_full,
// spawn_one, run_spawns) and the per-frame mission tick. The object is the Mission of src/game/mission.h. See MIGRATION.md.
#include "../../include/image_addrs.h"
#include "../../include/layout32.h"
#include "../../include/anim_set.h"      // AnimSet g_anim_set (0xa76628)
#include "./mesh_slot.h"
#include "../../include/game_data.h"   // GameData (parties, f78), PartyMgr
#include "../terrain_b/scene_file.h"     // SceneFile: GameData::scenes' record (flags)
#include "../terrain_b/mission.h"
#include "../../include/pool.h"             // Pool<Missile>::next_used 0x474300
#include "../missile.h"   // Missile, the record of Pool<Missile> (Mission::poolJ)
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

// ---- the agent record is Agent (game/agent_obj.h): the spawner fills it through the mission's agent table (Pool<Agent>) ----
namespace rgl { struct strategic_entity; }
namespace rgl { struct lod_mesh2; }

// ---- the objects behind the game pointer ---------------------------------------------------------------------------
#include "../trigger_vecs.h"             // TriggerVec: the mission template's triggers (TriggerRec::update 0x4625d0)
// an item reference (ItemRef, game/item_ref.h): 0x434df0 is ItemRef::hit_points, 0x434eb0 ItemRef::body_armor
struct ItemStackG {
    unsigned a, b;
    int value() const { return ((ItemRef*)this)->hit_points(); }        // 0x434df0
};
struct ItemStack2 {
    unsigned a, b;
    int value() const { return ((ItemRef*)this)->body_armor(); }        // 0x434eb0
};
// 0x439000 is node_groups::cap_all (agent2/agent_types.h: the display mesh's node lists of a body)
struct node_groups {
    void cap_all(const float* limit);   // 0x439000
};
#include "./mesh_build.h"          // setup_item_meshes (0x47cfe0)
#include "../skill_cap.h"
#include "../str_table.h"                 // StrTable g_strtable (0x5e2a18; its lookup takes the key by reference)
long long pick_slot_item(int a, int b, int c, int d, int* e, char f, char* g);   // 0x46b190

// Mission members in mission.h padding (reported for mission.h; named accessors meanwhile)

// @ 0x4832b0 -- the shared "initialise one agent" routine: applies the troop's skin mesh, builds the collision capsule from
// the record's scale, runs the equipment fillers and finishes with the corpse spawn
NOINL void Mission::spawn(Agent* a)
{
    if (a->kind == 0) {
        a->defs = &g_anim_set;
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
        if (a->pool_id != this->player_id) {
            unsigned long long f = t->flags;
            if (!(f & 0x10) || !(f & 0x1000))
                t->auto_equip(a->seed, 0x100e, (a->override_flags >> 28) & 0xffffff01, (a->override_flags >> 9) & 0xffffff01);
        }
    } else if (a->kind == 1) {
        a->defs = &g_anim_set;
    }
    rgl::strategic_entity* e = (rgl::strategic_entity*)((rgl::world*)scene)->create_entity(9);
    e->game_id = a->pool_id;
    e->game_kind = 0xb;
    a->body = e;
    e->fx = new FxDef(2);
    e->fx->set_env_count(2);
    e->fx->keep_weight = 0;
    shape_base* shp;
    // any other kind skips the shape (0x4835fc, the jne at 0x483566) and falls through to the common tail
    if (a->kind == 0) {
        unsigned skin_i = g_game->troops[a->troop].flags32 & 0xf;
        frame_set* obj;
        if (skin_i < (unsigned)g_anim.char_count)
            obj = (frame_set*)g_anim.skin_vec._Myfirst[skin_i].skeleton;
        else
            obj = 0;
        e->fx->init(obj);
        skin_i = g_game->troops[a->troop].flags32 & 0xf;
        float h;
        if (skin_i < (unsigned)g_anim.char_count)
            h = g_anim.skin_vec._Myfirst[skin_i].scale;
        else
            h = 1.0f;
        a->scale = h;
        {
            vector4 b = vector4(0.0f, 0.0f, 1.3f) * a->scale;
            vector4 c = vector4(0.0f, 0.01f, 2.3f) * a->scale;
            shp = new capsule(a->scale * 0.3f, &c, &b);
        }
        e->shape = shp;
    } else if (a->kind == 1) {
        {
            tstring name("skel_horse");
            e->fx->init((frame_set*)g_resmgr.get_object70(name));
        }
        {
            vector4 b(0.0f, 0.8f, 0.8f);
            vector4 c(0.0f, -0.3f, 0.7f);
            shp = new capsule(0.4f, &c, &b);
        }
        e->shape = shp;
    }
    pick_slots(a);
    build_appearance(e, a->kind, a->troop, a->seed, &a->item, (rgl::lod_mesh2**)a->slot_mesh, a->armor,
                     a->lods.extra, &a->lods.extra_count, &a->lods.body, (EquipSet*)a->equipment, a->pool_id);   // 0x482240
    setup_item_meshes(e, a->kind, a->troop, (rgl::lod_mesh2**)a->slot_mesh, &a->weapon_flags, a->equipment, a->pool_id);
    a->equip_value = a->equipment_value();
    this->post_spawn(a);
    if (a->kind == 0) {
        if (a->meters[0]) {
            float r = 20.0f;
            a->meters[0]->cap_all(&r);
        }
        if (a->meters[1]) {
            float r = 20.0f;
            a->meters[1]->cap_all(&r);
        }
    }
    a->spawn_corpse();
}

// @ 0x4836e0 -- hand out an id from the embedded pool, reset the record, fill the spawn fields and spawn it
int Mission::create(const matrix* m, const ItemStackG* it, int troop)
{
    int own = agents.alloc();
    Agent* a = &agents.pages[(unsigned)own >> 4][own & 0xf];
    a->reset();
    a->item.item = it->a;
    a->item.m4 = it->b;
    a->troop = troop;
    a->pos = m->o;
    a->team = -1;
    a->kind = 1;
    a->ai.ai_flags = 0;
    a->vel = m->f;
    a->facing = a->vel;
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
    TriggerVec* p = &g_mission_templates[this->mode].triggers;
    for (int i = 0; i < p->count; i++)
        p->data[i].update(-25);
    return own;
}

// @ 0x483870 -- a 10-argument agent creator: allocates a record from the pool, resets it, fills the spawn fields from a matrix
// and a wave record, applies the skin-based scale/height, runs spawn, then optionally creates a carried-item agent through
// pick_slot_item + create and announces the wave on the ui queue
NOINL int Mission::spawn_full(const matrix* m, int troop, int a1, int a2, int a3, int a4, int a5, int a6, int cnt, int* arr)
{
    int own = agents.alloc();
    if (troop == g_game->player) {
        this->player_id = own;
        this->f6598 = m->o;
    }
    Agent* a = &agents.pages[(unsigned)own >> 4][own & 0xf];
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
    a->vel = m->f;
    a->facing = a->vel;

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
    a->start_health = a->health;
    a->kind = g_game->troops[troop].level48;
    a->state = 1;
    a->ai.ai_flags = a6;
    a->ai.alarm = 0;
    if (a6 & 0x10)
        a->ai.alarm = 1;
    a->override_flags = a5;
    if (a->pool_id == this->player_id) {
        a->flags |= 4;
        a->f7c = 2;
    }
    a->state = 1;
    for (int i = 0; i < 10; i++) {
        a->equipment[i].item = -1;
        a->slot_flag[i] = 1;
    }
    this->spawn(a);
    a->ai.simple_behavior = a6;
    {
        char found;
        long long item = pick_slot_item(troop, 8, a5, cnt, arr, 0, &found);
        if ((int)item >= 0) {
            int nid = this->create(m, (const ItemStackG*)&item, troop);
            agents.pages[(unsigned)nid >> 4][nid & 0xf].rider = own;
            a->horse = nid;
            agents.pages[(unsigned)nid >> 4][nid & 0xf].party = a1;
            agents.pages[(unsigned)nid >> 4][nid & 0xf].party_stack = a2;
            if (troop == g_game->player)
                this->f60 = nid;
        }
    }
    if (a->pool_id == this->player_id) {
        if ((flags & 1) || Helpers::mode_flag1(this)) {
            a->cycle_slot(0);
            a->cycle_slot(1);
        }
    }
    a->on_wield_reset();
    int t1 = a->team;
    if (t1 < 0 || t1 >= 8) t1 = 7;
    if (this->teams[t1].leader < 0 || a->pool_id == this->player_id) {
        int t2 = a->team;
        if (t2 < 0 || t2 >= 8) t2 = 7;
        this->teams[t2].leader = a->pool_id;
    }
    if (this->player_id >= 0) {
        if (this->announce_enemies != 0 && this->agents[this->player_id].is_enemy(a)) {
            this->announce_enemies = 0;
            tstring msg("ui_new_enemies_have_arrived");
            tstring* t2 = g_strtable.lookup(msg);
            if (g_gui.enabled)
                g_gui.ring.add(*t2, -1);
        } else if (this->announce_reinforcements != 0 && this->agents[this->player_id].is_friend(a)) {
            this->announce_reinforcements = 0;
            tstring msg("ui_reinforcements_have_arrived");
            tstring* t2 = g_strtable.lookup(msg);
            if (g_gui.enabled)
                g_gui.ring.add(*t2, -1);
        }
    }
    g_script.param1 = own;
    TriggerVec* p = &g_mission_templates[this->mode].triggers;
    for (int i = 0; i < p->count; i++)
        p->data[i].update(-25);
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
        agents.pages[(unsigned)nid >> 4][nid & 0xf].entry_no = orig;
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
// the missile pool at Mission+0x30 (Missile, ../missile.h): update 0x47d3e0 is mesh_scene.cpp's
struct psys_rec {                       // the shadow object's target skin (ShadowMap::target, g_shadow_map): a texture reference at +0x48
    char pad0[0x48];
    char* scene;
};
// Mission members in mission.h padding (reported for mission.h; named accessors meanwhile)
static inline std::vector<rgl::entity*>& retired(Mission* m) { return *(std::vector<rgl::entity*>*)&m->v5dfc; }   // the retire list holds entity pointers
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
        unsigned flags = g_game->scenes[scene_idx].flags;
        if (flags & 0x100)
            g_terrain.destroy_water(&prop_ents);   // 0x4814a0 (the argument is ignored)
        else if (!(flags & 1))
            g_shadow_map.render();     // this is &g_shadow_map, not the target (0x47fef0)
    }

    if (!g_mt_inited && dt > 0.0f)
        g_phys.update(dt);                       // 0x472dc0 (g_phys 0xa181f8)

    long long t = (long long)(dt * 100000.0f);
    g_tick_dt[2] = t;
    g_ticks[2] += t;
    g_step_tbl[2] = dt;

    // every sixteenth frame the carried-item records restart from their first matrix
    if ((f6550 & 0xf) == 1) {
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

    if (f6550 != 0) {
        TriggerVec* p = &g_mission_templates[this->mode].triggers;
        for (int i = 0; i < p->count; i++)
            p->data[i].update(0x10);
    }

    // the prop pool: per-entry update, retire the finished ones, and steady the used slots
    {
    unsigned i = pool_j(this).first_used();
    while (i < (unsigned)pool_j(this).count) {
        Missile* p = &pool_j(this).pages[i >> 4][i & 0xf];
        p->update(dt);
        if ((p->state == 2 || p->state == 3 || p->state == 1) && (g_ticks[p->clk.clock] - p->clk.start) * 1e-5f > 1.0f
            || p->pos.z < -100.0f) {
            if (p->ent != 0)
                retired(this).push_back(p->ent);
            p->in_use = 0;
            pool_j(this).free_ids.push_back(i);
        }
        pool_j(this).next_used(&i);
    }

    // the retire list overflows: drop a random prop and compact
    if (retired(this).size() > 0x300) {
        int n = (int)retired(this).size();                    // signed: cmp edi,1; jg / cdq; idiv (0x4842a5)
        int k = n > 1 ? rand() % n : 0;
        ((rgl::world*)scene)->remove_entity(retired(this)[k]->id);       // 0x41f0d0
        retired(this)[k] = retired(this)[retired(this).size() - 1];
        retired(this).resize(retired(this).size() - 1);
    }

    for (unsigned k = 0; k < props.size(); k++)
        props[k].animate(dt);

    // agents: everything but the horses, then the horses, then every agent's tick
    for (i = agents.Pool<Agent>::first_used(); i < (unsigned)agents.count; agents.next_used(&i))
        if (agents.atu(i)->kind != 1)
            agents.atu(i)->update(dt);
    for (i = agents.Pool<Agent>::first_used(); i < (unsigned)agents.count; agents.next_used(&i))
        if (agents.atu(i)->kind == 1)
            agents.atu(i)->update(dt);
    for (i = agents.Pool<Agent>::first_used(); i < (unsigned)agents.count; agents.next_used(&i))
        agents.atu(i)->tick();
    }

    update_prop_flags();

    // the sky clock and the mission timers
    if (water != 0) {
        float t = f65ac = f65ac + dt * 0.04f;
        if (t > 100.0f)
            f65ac = (float)fmod((double)t, 1.0);     // 0x4844df: the divisor is the double at 0x54a5d8 (1.0), the 100.0 is only the wrap test (0x54ab28)
        water->tex_matrix.u.y = f65ac;              // the uv offset (the u row) and the translation scroll with the water time
        water->tex_matrix.o.y = f65ac;
        water->anim_phase = (float)(g_ticks[2] % 1000000000) * 7.5e-6f;
    }

    update_team_stats();
    ((rgl::world*)scene)->step(dt);                  // 0x427f40
    update_team_counts();
    f6550++;
    if (g_mt_inited)
        g_paused_time += dt;
}
