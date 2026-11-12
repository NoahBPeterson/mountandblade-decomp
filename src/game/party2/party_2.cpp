//-----------------------------------------------------------------------------
// File: party_2.cpp  (0x4b1160..0x4b8f40)
//
// Desc: the map party: its stack economy (value, experience, morale, speed, skills), its orders and path following on the
//       world map (orders, planner, route stepping, nav driver, the per-frame tick), the battle records between two parties
//       (BattleRec: bind, detach, commit, per-frame visuals), the world-map path finder (NavData::find_path, can_move), the
//       tableau render callback, and the STL helper instances the linker kept from this object (vector<E92> / vector<E12>
//       _Insert_n / insert / push_back, the 0xc- and 0x5c-byte copy and heap helpers). Names provisional.
//       Flags: /O2 /MT /Gy /TP.
//
// Boundary: see MIGRATION.md (start 0x4b1160: the tableau render callback whose address 0x4b14b0 / 0x4b14f0 store; the
// interpreter 0x49fa10 calls 0x4b14b0 forward).
//-----------------------------------------------------------------------------
#define RGL_VECTOR4_OPS_OUT_OF_LINE   // Party::collect_targets 0x4b3910 calls vector4::operator- 0x407ca0 (and out_length 0x407c50) instead of expanding them
#include "./party_2.h"
#include "../../include/wl_marker.h"
#include "../../include/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40; listed directly for the resolution audit)
#include "../skill_cap.h"      // SkillCap g_skill_tbl (0x8ee3100): [36].max_rank is the clamp at 0x8ee3ac4
#include "../../include/crt.h"      // canonical CRT declarations (listed directly: the resolution audit reads only the source's own includes)
#include "../game_globals.h"      // canonical g_module_settings.seeing_range
#include "../terrain_b/terrain_b.h"
#include "../terrain_b/cell_grid.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../../rgl/mesh.h"
#include "../render_batch.h"   // g_batch (0x5fe898): render 0x47e520
#include "../config/widget_mgr.h"
#include "../battle_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/sound_mgr.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/game_recs.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

#include "../../include/item_kind.h"   // g_items 0x5e29d0
#include "../float_consts.h"   // shared float constants kept in memory (0x549cdc ...)
#include "../../rgl/options.h"

// ---- the tableau render callback, its material table and the tableau button ----------------------------------------------
// @ 0x4b1160
// Render one tableau material: append a fresh world, set up the 0x1d8-byte render scene for the record, stamp its
// frustum, run the record's init script (0x4b09e0) with the instance code, step the world and render until the scene
// registry accepts the pass, then drop the world.
void scene_step(int material)
{
    TabSpawn* m = (TabSpawn*)material;
    if (!m->target->enabled)
        return;

    TabWorld* o = (TabWorld*)g_tableaus.push();
    TabRec* rec = (TabRec*)&g_tableaus.recs[m->rec];

    rgl::scene sc;
    sc.mask = 0x2000;
    sc.f0 = 0;
    sc.f4 = 0;
    sc.f8 = rec->w;
    sc.fc = rec->h;
    sc.f10 = 0;
    sc.f14 = 1.0f;
    sc.f18 = 0;
    sc.f1c = 0;
    sc.f20 = rec->w;
    sc.f24 = rec->h;
    sc.f28 = 0;
    sc.f2c = 1.0f;
    sc.f1a5 = 1;
    sc.active = false;
    sc.flags = 0x300;
    sc.f16a = 0;

    o->fr.flag = 0;
    g_tableaus.alpha_mask = 0;
    *(vector4*)o->fr.mat4 = vector4(1.0f, 1.0f, 1.0f);
    o->f348 = 0;
    o->f344 = 0;
    ((rgl::world*)o)->reset();

    TabFrustum& fr = o->fr;
    fr.perspective = 0;
    fr.nearz = 0.0f;
    fr.farz = 501.0f;   // 0x43fa8000 at 4b12af..4b12b9
    fr.left = -0.5f;
    fr.right = 0.5f;
    fr.bottom = -0.375f;
    fr.top = 0.375f;
    fr.build_planes();

    {
        rgl::matrix ident;
        rgl::identity_pos(&ident);
        ident.o = vector4(0.0f, 0.0f, 400.0f);
        fr.m = ident;
    }
    fr.build_planes();

    {
        __int64 params[16];                        // the script's argument registers (0x4b1344: frame 0x28..0xa8, only [0] is set)
        params[0] = *(int*)((char*)m + 0x60);      // the instance code, sign extended (0x4b133a mov eax,[ecx+0x60]; cdq)
        int count = 1;
        ((DialogBlock*)rec->holder)->emit(-0x41, 0, &count, params, -1);
    }

    sc.fr = fr;
    sc.f190 = o->fr.flag;
    sc.f1b0 = &o->view;
    sc.f160 = 1;
    sc.list = *(rgl::ptr_list*)&o->lights;
    o->ambient = *(vector4*)o->fr.mat4;
    o->ambient.w = 1.0f;

    sc.target = (texture*)m->target;
    o->step(0.0f);
    if (g_tableaus.alpha_mask != 0)
        sc.f164 = 1;

    for (;;) {
        o->render_scene(&sc, false);
        int h = rec->h;
        if (h <= 0x200)
            h = 0x200;
        int w = rec->w;
        if (w <= 0x200)
            w = 0x200;
        texture* t = g_resmgr.get_texture(w, h, 0x50);
        if (t != 0)
            sc.f15c = (int)t;
        bool ok = g_batch.render(&sc);
        if (t != 0)
            CALL_STDCALL_ON(clear_f14, &g_resmgr, t);   // 0x4175f0 (the resource manager in ecx)
        if (ok)
            break;
    }

    m->target->done = 1;
    g_tableaus.pop_back_delete();
}

// @ 0x4b14b0
TabSpawn* TableauTbl::add_spawned(const int* tableau, const int* instance)
{
    purge_all();
    TabRec* e = (TabRec*)&recs[*tableau];
    TabSpawn* r = e->find_or_create(instance);
    r->rec = *tableau;
    r->step = scene_step;
    return r;
}

// @ 0x4b14f0
// The button of a tableau: an XyzButton whose two images are the mesh `pos` of the editor's scene table with a tableau
// material installed in place of its texture (or, for a negative `pos`, a mesh made from the tableau record).
int widget_mgr::create_image_button_overlay_with_tableau(int pos, int rec, int key)
{
    XyzButton* w = new XyzButton;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        vt(w)->d22(&v);
        *(int*)&v.x = 0x700;
        vt(w)->d29(&v);
    }

    if (pos >= 0) {
        rgl::mesh* m = g_meshes.placed(pos);
        g_tableaus.purge_all();
        TabRec* r = (TabRec*)&g_tableaus.recs[rec];
        TabSpawn* e = r->find_or_create(&key);
        e->rec = rec;
        e->step = scene_step;
        TabSpawn*& mres = *(TabSpawn**)&m->res;
        if (mres)
            mres->refs--;
        mres = e;
        if (e)
            e->refs++;
        vt(w)->d32(2, m);

        m = g_meshes.placed(pos);
        g_tableaus.purge_all();
        r = (TabRec*)&g_tableaus.recs[rec];
        e = r->find_or_create(&key);
        e->rec = rec;
        e->step = scene_step;
        TabSpawn*& mres2 = *(TabSpawn**)&m->res;
        if (mres2)
            mres2->refs--;
        mres2 = e;
        if (e)
            e->refs++;
        vt(w)->d32(1, m);
    } else {
        vt(w)->d32(2, g_tableaus.make_mesh_from(&rec, &key));
        vt(w)->d32(1, g_tableaus.make_mesh_from(&rec, &key));
    }

    vt(w)->d03();
    scene->d50(w);
    widgets.push_back(w);
    return (int)widgets.size() - 1;
}

// @ 0x4b1930
// The relation colour ramp: red for hostile (-1), a pale green-white for neutral, green for friendly (+1); `flag` == 0
// darkens it for the map labels.
unsigned GameData::color_of(float t, int flag)
{
    vector4 c(1.0f, 1.0f, 1.0f);
    if (t < -0.0001f) {
        float a = 1.0f + t;
        float hi[3];
        float u = -t;
        hi[0] = 0.95f * a;
        hi[1] = 0.65f * a;
        hi[2] = 0.65f * a;
        float lo[3];
        lo[0] = 0.95f * u;
        lo[1] = 0.0f * u;
        lo[2] = 0.0f * u;
        c = vector4(lo[0] + hi[0], lo[1] + hi[1], lo[2] + hi[2]);
    } else if (t > 0.0001f) {
        float u = 1.0f - t;
        float hi[3];
        hi[0] = 0.75f * u;
        hi[1] = 0.95f * u;
        hi[2] = 0.65f * u;
        float lo[3];
        lo[0] = 0.50f * t;
        lo[1] = 1.0f * t;
        lo[2] = 0.20f * t;
        c = vector4(lo[0] + hi[0], lo[1] + hi[1], lo[2] + hi[2]);
    }
    if (flag == 0) {
        c.x = c.x * 0.65f + 0.2f;
        c.y = c.y * 0.65f + 0.2f;
        c.z = c.z * 0.65f * 0.7f + 0.2f;
    }
    c.w = 1.0f;
    return rgl::pack_color(&c);
}

// constants of the original's .rdata (declared by address: the literal pool of the object is shared)
extern float g_k0_01;                               // 0x54a6f0
extern float g_k0_7;                                // 0x54b3c8
extern float g_k0_6;                                // 0x590d94
extern float g_k1_2;                                // 0x590b94
extern float g_k1_25;                               // 0x590c80
extern float g_k1_05;                               // 0x590f24
extern float g_k0;                                  // 0x549ca8
extern float g_k3;                                  // 0x54a710
extern const float g_k002;                              // 0x590cd0
extern float g_k0;                                  // 0x549ca8
extern float g_k3;                                  // 0x54a710
extern float g_k0_4;                                // 0x54b3b8

// @ 0x4b1b20
void copy_items_forward(E92* dst, E92* end, const E92* src)
{
    while (dst != end) {
        *dst = *src;
        ++dst;
    }
}

// @ 0x4b1b50
void copy_items_backward(E92* stop, E92* first, E92* last)
{
    while (first != stop) {
        --first;
        --last;
        *last = *first;
    }
}

// @ 0x4b1b80
bool Party::update()
{
    for (int i = 0; i < nstacks; i++) {
        Stack& slot = stacks[i];
        if (slot.count <= 0) {
            slot.count = 0;
        }
        if (!(g_game->troops[slot.troop].flags30 & 0x10) && slot.wounded > slot.count) {
            slot.wounded = slot.count;
        }
    }
    return true;
}

// @ 0x4b1be0
int Party::slot_value(int idx, unsigned flags) const
{
    GameData* g = g_game;
    const Stack* slot = &stacks[idx];
    if (flags & 0x20) {
        if (slot->troop == g->player) {
            goto zero;
        }
    }
    if (flags & 1) {
        if (idx == 0) {
            if ((g->troops[slot->troop].flags30 & 0x10) != 0) {
                goto zero;
            }
        }
    }
    if (flags & 2) {
        if ((g->troops[slot->troop].flags30 & 0x10) != 0) {
            goto zero;
        }
    }
    if (flags & 4) {
        if ((g->troops[slot->troop].flags30 & 0x10) != 0) {
            if (g->agent_low(slot->troop)) {
                goto zero;
            }
        }
    }
    if (flags & 8) {
        if ((g->troops[slot->troop].flags30 & 0x10) == 0) {
            goto zero;
        }
    }
    if (flags & 0x10) {
        if (slot->flags & 1) {
            goto zero;
        }
    }
    int amount = slot->count;
    if (flags & 0x100) {
        if (!(g->troops[slot->troop].flags30 & 0x10)) {
            amount -= slot->wounded;
        }
    }
    if (amount < 0) {
        goto zero;
    }
    return amount;
zero:
    return 0;
}

// @ 0x4b1cf0
int Party::sum_stat(int what)
{
    int total = 0;
    int n = nstacks;
    for (int i = 0; i < n; i++) {
        total += slot_value(i, what);
    }
    return total;
}

// @ 0x4b1d30
int Party::find_worth(int value, unsigned flags) const
{
    int total = 0;
    for (int i = 0; i < nstacks; i++) {
        int n = slot_value(i, flags);
        total += n;
        if (n != 0 && value < total) {
            return i;
        }
    }
    return -1;
}

// @ 0x4b1d80
void Party::add_xp_to_stack(int idx, int amount)
{
    GameData* g = g_game;
    Troop* chars = g->troops;
    Stack& slot = stacks[idx];
    Troop* chr = &chars[slot.troop];
    float& acc = *(float*)&stacks[idx].f18;
    if (chr->flags30 & 0x10) {
        chr->exp += amount;
        return;
    }
    if (chr->f50 == 0) {
        return;
    }
    acc += (float)amount;
    if (acc >= (float)(int)(g_level_xp[chr->level + 3] * 0.006f + 30.0f)) {
        while (slot.count > slot.f14) {
            slot.f14++;
            acc -= (float)(int)(g_level_xp[chr->level + 3] * 0.006f + 30.0f);
            if ((float)(int)(g_level_xp[chr->level + 3] * 0.006f + 30.0f) >= acc) {
                break;
            }
        }
    }
    if ((float)slot.f14 >= (float)slot.count) {
        acc = 0;
    }
}

// @ 0x4b1eb0
void Party::prepare()
{
    int keep = 0;
    for (int i = 0; i < nstacks; i++) {
        if (stacks[i].flags & 1) {
            break;
        }
        keep++;
    }
    int dst = 0;
    for (int j = 0; keep > j; j++) {
        if (g_game->troops[stacks[j].troop].flags30 & 0x10) {
            if (dst != j) {
                rotate_stacks(dst, j);
            }
            dst++;
        }
    }
    int k2 = keep;
    for (int j = keep; j < nstacks; j++) {
        if (g_game->troops[stacks[j].troop].flags30 & 0x10) {
            if (k2 != j) {
                rotate_stacks(k2, j);
            }
            k2++;
        }
    }
}
// @ 0x4b1fa0
float Party::strength()
{
    if (f88 != -1) {
        return g_one;
    }
    int a = 0;
    int b = 0;
    for (int i = 0; i < nstacks; i++) {
        Stack s = stacks[i];
        unsigned __int64 fl = g_game->troops[s.troop].flags30;
        if (fl & 0x10) {
            ++a;
        } else if (fl & 0x400) {
            a += s.count;
        } else {
            b += s.count;
        }
    }
    float v = ((float)a * 3.0f + (float)b) * g_k002 + 0.4f;
    if (v < 0.0f) {
        return 0.0f;
    }
    if (v >= g_one) {
        v = g_one;
    }
    return v;
}

// @ 0x4b20b0
float Party::speed_factor()
{
    if (*(unsigned __int64*)&flags_lo & 0x200) {
        return 15.0f;
    }
    float v = f8b0 * f8ac;
    int type = map_mask;
    if (type == 0xb || type == 0xc || type == 0xa || type == 0xd) {
        v = v * 0.7f;
    }
    if (g_game->is_night()) {
        return v * 0.6f;
    }
    return v;
}

// @ 0x4b2120
void Party::set_speed_from_stat()
{
    g_script.trigger_result.value = 100;
    g_game->emit_event(0x1a, id, 0);
    f8b0 = (float)g_script.trigger_result.value * 0.01f;
}

// @ 0x4b2180
MB_NOINLINE OpenItem* construct_items(OpenItem* first, OpenItem* last, OpenItem* dst, ElemCopier* ctx, ptr_tag tag)
{
    try {
        while (first != last) {
            ctx->copy(dst, first);
            ++dst;
            ++first;
        }
    } catch (...) {
    }
    return dst;
}

// @ 0x4b21e6
void throw_stub_21e6()
{
    throw;
}

// @ 0x4b21f0
float Troop::food_factor()
{
    unsigned __int64 f = flags30;
    if (f & 0x10) {
        if (f4e4 >= 0) {
            return (nibble(0x18) * 0.04f + 1.25f) * 6.9f;
        }
        return (nibble(0x19) * 0.04f + 1.05f) * 6.9f;
    }
    if (f & 0x400) {
        return (nibble(0x18) * 0.04f + 1.2f) * 6.9f;
    }
    return (nibble(0x19) * 0.04f + g_one) * 6.9f;
}

// @ 0x4b22b0
ItemIter ItemArray::at(int idx)
{
    return ItemIter(base + idx);
}

// @ 0x4b22d0
int Party::max_members()
{
    if (g_game->emit_event(0x11, id, 0)) {
        return (int)g_script.trigger_result.value;
    }
    int item = stacks[0].troop;
    int bonus = g_game->troops[item].f64;
    return g_game->skill_level(item, 1) * g_module_settings.skill_leadership_bonus + g_module_settings.base_companion_limit + bonus;
}

// @ 0x4b2330
int Party::max_prisoners()
{
    if (g_game->emit_event(0x12, id, 0)) {
        return (int)g_script.trigger_result.value;
    }
    return g_game->skill_level(stacks[0].troop, 2) * g_module_settings.skill_prisoner_management_bonus;
}

// @ 0x4b2370
int Party::skill_level(int skill)
{
    int best = 0;
    if (nstacks != 0 && !g_game->agent_low(stacks[0].troop)) {
        best = g_game->skill_level(stacks[0].troop, skill);
    }
    int found = 0;
    for (int i = 0; i < nstacks; i++) {
        int item = stacks[i].troop;
        if (!(g_game->troops[item].flags30 & 0x10)) {
            continue;
        }
        if (stacks[i].flags != 0) {
            continue;
        }
        if (item == g_game->player) {
            if (!(g_game->troops[item].hp >= g_module_settings.player_wounded_treshold)) {
                continue;
            }
        } else {
            if (!(g_game->troops[item].hp >= g_module_settings.hero_wounded_treshold)) {
                continue;
            }
        }
        int v = g_game->skill_level(item, skill);
        if (v > found) {
            found = v;
        }
    }
    int r = found;
    if (best >= 2) r = found + 1;
    if (best >= 5) r++;
    if (best >= 8) r++;
    if (best >= 0xa) r++;
    return r;
}


// @ 0x4b1700
int Party::max_stacks()
{
    return 0x20;
}

// @ 0x4b1710
void Party::rotate_stacks(int from, int to)
{
    Stack tmp = stacks[to];
    if (from < to)
        for (int i = to; i > from; i--)
            stacks[i] = stacks[i - 1];
    stacks[from] = tmp;
}

// @ 0x4b1780
void Party::set_order_face(int* pos, face* f)
{
    kind = 1;
    target_pos[0] = pos[0];
    target_pos[1] = pos[1];
    tgt = f;
    if (f5984 != f->m4) {
        moving = 0;
        f5988 = 1;
    }
}

// @ 0x4b17d0
bool rgl::mesh::in_use() const
{
    if (parent)
        return parent->kf.count > 0;
    return kf.count > 0;
}

// @ 0x4b1800
void PartyModel::set_fade(const float* p)
{
    f15c = *p;
    float t = f15c * 4.0f - 0.4f;
    if (t < 0.0f)
        t = 0.0f;
    else if (t >= 1.0f)
        t = 1.0f;
    f160 = t;
    f164 = 1.0f - t;
    if (f15c > 0.5f) {
        f160 = 1.0f;
        f164 = 0.0f;
        return;
    }
    if (f15c > 0.15f) {
        t = (f15c - 0.15f) * 2.857143f;
        f160 = t;
        f164 = 1.0f - t;
        return;
    }
    if (f15c > 0.03f) {
        f160 = 0.0f;
        f164 = 1.0f;
        return;
    }
    if (f15c > 0.015f) {
        f160 = 0.0f;
        f164 = (f15c - 0.015f) * 66.66667f;
        return;
    }
    f160 = 0.0f;
    f164 = 0.0f;
}

// @ 0x4b1af0
int EventVec::size() const
{
    if (!first)
        return 0;
    return ((char*)last - (char*)first) / 0x5c;
}


// ---- STL helper instances of the 0xc-byte open-list element and the 0x5c-byte event record --------------------------------

// @ 0x4b24b0
// std::uninitialized_fill_n(dst, count, *val): the original takes the pointer and count by value (they are advanced in
// their stack slots because the loop sits in a try block); a 5th pushed argument is unused.
void construct_elems2(OpenItem* dst, unsigned count, OpenItem* val, ElemCopier* ctx, ptr_tag tag)
{
    OpenItem* s = dst;
    try {
        for (; 0 < count; --count, ++dst) ctx->copy(dst, val);
    } catch (...) {
        for (; s != dst; ++s) ;
        throw;
    }
}

// @ 0x4b2514
void throw_stub_2514()
{
    throw;
    __assume(0);
}

// @ 0x4b2520
// std::_Push_heap (min-heap on dist): sift `value` up from `hole` while the parent is greater.
// The comparator is an empty functor passed by value (one 4-byte slot).
MB_NOINLINE void push_heap_elem(OpenItem* first, int hole, int top, OpenItem value, HeapGt pred)
{
    for (int i = (hole - 1) / 2; top < hole && pred(first[i], value); i = (hole - 1) / 2) {
        first[hole] = first[i];
        hole = i;
    }
    OpenItem* d = &first[hole];
    d->node = value.node;
    d->cost = value.cost;
    d->dist = value.dist;
}

// @ 0x4b25a0
OpenItem* ElemCopier::fill_elems(OpenItem* first, OpenItem* last, OpenItem* dst)
{
    ptr_tag tag;
    return construct_items(first, last, dst, this, tag);
}

// @ 0x4b25c0
float Party::map_speed()
{
    float v = (skill_level(0xd) * 0.1f + 1.0f) * g_module_settings.seeing_range;
    if (g_game->is_night()) {
        return v * 0.5f;
    }
    return v;
}

// @ 0x4b2610
// std::__pop_heap_aux: *result = *first; adjust_heap(first, 0, last - first, value, comp); the sixth argument (the distance
// type pointer of std::__pop_heap) is pushed by the callers and unused
MB_NOINLINE void pop_heap_elem(OpenItem* first, OpenItem* last, OpenItem* result, OpenItem value, HeapGt comp, int* dist)
{
    *result = *first;
    adjust_heap(first, 0, (int)(last - first), value, comp);
}

// @ 0x4b2670
MB_NOINLINE E92* construct_events(E92* first, E92* last, E92* dst, ElemCopier* ctx, ptr_tag tag)
{
    E92* s = dst;
    try {
        for (; first != last; ++dst, ++first) ctx->copy_event(dst, first);   // 0x4b2160
    } catch (...) {
        for (; s != dst; ++s) ;
        throw;
    }
    return dst;
}

// @ 0x4b26d6
void throw_stub_26d6()
{
    throw;
    __assume(0);
}

// @ 0x4b26e0
OpenItem* ElemCopier::fill_n_elems(OpenItem* dst, unsigned count, OpenItem* val)
{
    ptr_tag tag;   // uninitialised: pushes whatever eax holds (val)
    construct_elems2(dst, count, val, this, tag);
    return dst + count;
}

typedef std::vector<E12>::iterator OpenIt;
static __forceinline OpenItem* oi(OpenIt it) { return (OpenItem*)&*it; }

// @ 0x4b2710
// std::push_heap tail: sift up the last element
void push_back_elem(OpenIt first, OpenIt last, HeapGt comp)
{
    if (first != last) {
        OpenIt l = last - 1;
        int n = (int)(l - first);
        if (n > 0) {
            push_heap_elem(oi(first), n, 0, *oi(l), comp);
        }
    }
}

// Re-seat a battle record on the world map: face under the first position, its height above the face's plane, the clock
// stamp, and (type 2) a sanity check of the terrain masks under both positions.
// @ 0x4b2770
void BattleRec::bind_host()
{
    int clk = born.clock;
    face1 = g_game->find_at(&x1);
    z = face1->w[0].z - ((x1 - face1->w[0].x) * face1->normal.x + (y1 - face1->w[0].y) * face1->normal.y) / face1->normal.z;
    ((int*)&born.start)[0] = ((int*)&g_ticks[clk])[0];
    ((int*)&born.start)[1] = ((int*)&g_ticks[clk])[1];
    next_simulation_time = 1.0f;
    opacity = -10000.0f;
    f74 = 0.0f;
    sound = -1;
    if (type == 2) {
        int m = (int)nav_recs()[face1->m4].mask();
        if ((m & 7) != 0 && m != 7) {
            mb_warning("Ship position not in water");
        }
        face* b = g_game->find_at(&x2);
        face2 = b;
        m = (int)nav_recs()[b->m4].mask();
        if (((m & 7) < 2 || (m & 7) >= 7) && m != 7) {
            mb_warning("Ship land position not on land");
        }
    }
}

// Place the party on the map: face under (x, y), height z over the face's plane, terrain mask, and the "last position"
// copies.
// @ 0x4b2880
void Party::place_on_map()
{
    float* p = &pos.x;
    mesh = g_game->find_at(p);
    if (!mesh) {
        mb_error("Unable to position party %s at (%2.2f, %2.2f).", name.c_str(), (double)pos.x, (double)pos.y);
    }
    face* m = mesh;
    pos.z = m->w[0].z - ((p[1] - m->w[0].y) * m->normal.y + (p[0] - m->w[0].x) * m->normal.x) / m->normal.z;
    map_mask = nav_recs()[m->m4].mask();
    int x = ((int*)p)[0];
    int y = ((int*)p)[1];
    dest_xi = x;
    dest_yi = y;
    goal_xi = x;
    goal_yi = y;
    f5984 = -1;
    f597c = 5.0f;
    *(vector4*)&trail = *(vector4*)p;
}

// @ 0x4b2970
void Party::set_order_pos(int* pos)
{
    kind = 1;
    target_pos[0] = pos[0];
    target_pos[1] = pos[1];
    tgt = g_game->find_at((const float*)pos);
    if (f5984 != tgt->m4) {
        moving = 0;
        f5988 = 1;
    }
}

// @ 0x4b29d0
void Party::set_order_party(int* t)
{
    kind = 2;
    tgt_id = t[0];
    PartyMgr* mgr = &g_game->parties;
    if (f5984 != party_of(mgr, t[0])->mesh->m4) {
        const PartyV4* pp = &party_of(mgr, tgt_id)->pos;
        float dx = pp->x - pos.x;
        float dy = pp->y - pos.y;
        float dz = pp->z - pos.z;
        moving = 0;
        float s = dx * dx;
        s += dy * dy;
        s += dz * dz;
        if (sqrtf(s) > 3.0f) {
            f5988 = 1;
        }
    }
}

// @ 0x4b2ab0
void Party::set_order_agent(int* t)
{
    kind = 3;
    int id = t[0];
    tgt_id = id;
    PartyMgr* mgr = &g_game->parties;
    const PartyV4* pp = &party_of(mgr, id)->pos;
    vector4 d;
    d.x = pp->x - pos.x;
    d.y = pp->y - pos.y;
    d.z = pp->z - pos.z;
    const PartyV4* qq = &party_of(mgr, id)->pos;
    float ex = qq->x - f5990;
    float ey = qq->y - f5994;
    if (ex * ex + ey * ey > 10.0f) {
        moving = 0;
        if (d.z * d.z + d.y * d.y + d.x * d.x > 10.0f) {
            f5988 = 1;
        }
        struct xy { float x, y; };
        *(xy*)&f5990 = *(xy*)&party_of(&g_game->parties, tgt_id)->pos;
    }
}


// ---- the per-frame party stats --------------------------------------------------------------------------------------------
extern float g_k0_1;                                 // 0x54b8b0  0.1f
extern float g_k_item_weight;                        // 0x593640  per-item weight of the equipped grid item
struct options;
extern options g_options;                   // 0x5aa008
#define g_clamp_max g_skill_tbl[36].max_rank     // 0x8ee3ac4

// the body of ItemRef::max_amount (0x434b70), which the original inlined at two of the three call sites here
static __forceinline int item_max(const ItemRef* s)
{
    if (s->item < 0)
        return 0;
    int cap = g_items[s->item].f210;
    if (cap != 0 && (s->m4 & 0xff000000) == 0x2a000000) {
        float x = (float)cap * 0.13f;
        float r = (float)floor(x);
        int n = (int)r;
        int v = x - r <= 0.5f ? n : n + 1;
        if (v < 1)
            v = 1;
        cap += v;
    }
    return cap;
}
// the body of Troop's max_hit_points (0x468770), inlined here
static inline int max_hit_points(Troop* t)
{
    int base = t->lvl58;
    int v = ((unsigned char*)t)[0x7a] & 0xf;
    if (v < 0)
        v = 0;
    else if (v > g_clamp_max)
        v = g_clamp_max;
    return base + v * 2 + 0x23;
}
// @ 0x4b2c00
void Party::tick_stats()
{
    float speed = (float)skill_level(0xb) * 0.2f + 1.0f;
    GameData* g = g_game;                            // read after skill_level: the original loads g_game after the call
    if (id == g->f78 && g->f13ad0f > 0.0f && g->f13ad4 != 0) {
        speed = speed + speed;
    } else if (f88 > 0) {
        speed = speed * 0.0f;
    } else if (f8c8 >= 0 && party_of(&g->parties, f8c8)->f88 > 0) {
        speed = speed * 0.0f;
    }

    // food growth / stack decay over the party's troop stacks
    for (int i = 0; i < nstacks; i++) {
        Troop* ch = &g->troops[stacks[i].troop];
        if (ch->flags30 & 0x10) {
            float limit = (float)max_hit_points(ch);
            if (ch->hp < limit) {
                float add = ((float)(rand() % 15817) * g_rand_k + 0.5f) * 0.5f * speed;
                if (stacks[i].troop == g_game->player) {
                    add = add * (3 - (*(int*)((char*)&g_options + 0x4c)));
                }
                ch->hp = add + ch->hp;
                g = g_game;
            }
            if (ch->hp > limit) {
                ch->hp = limit;
            }
        } else if (stacks[i].wounded > 0) {
            int n = stacks[i].wounded;
            float chance = speed * 0.04f;
            do {
                if ((float)(rand() % 15817) * g_rand_k < chance) {
                    stacks[i].wounded--;
                }
            } while (--n);
            g = g_game;
        }
    }

    if (id != g->f78) {
        return;
    }

    // main party: zoom easing, the first record's item slot decay, morale
    if (g_module_settings.disable_food_slot == 0) {
        if (g->f13ad0f > 0.0f && g->f13ad4 != 0) {
            zoom = zoom - 0.03f;
            if (zoom < 0.0f) {
                zoom = 0.0f;
            } else if (zoom > 1.0f) {
                zoom = 1.0f;
            }
        } else {
            ItemRef* slot = &g->troops->equip[9];
            if (slot->item >= 0) {
                float chance = (float)sum_stat(0x10) * 0.02f;
                if (zoom > 0.1f) {
                    chance = chance * 3.0f;
                }
                while ((float)(rand() % 15817) * g_rand_k < chance) {
                    if (slot->item < 0) {
                        break;
                    }
                    int count = (slot->m4 & 0xff) + 1;
                    if (count > item_max(slot)) {
                        count = slot->max_amount();
                    }
                    *(unsigned char*)&slot->m4 = 0;
                    slot->m4 = slot->m4 | count;
                    int left = item_max(slot) - (int)(slot->m4 & 0xff);
                    if (left <= 0) {
                        slot->item = -1;
                    }
                    chance = chance - 1.0f;
                }
                zoom = zoom - 0.03f;
                if (zoom > 0.1f) {
                    zoom = zoom - 0.1f;
                }
            } else {
                zoom = zoom + 0.03f;
            }
            if (zoom > 0.1f) {
                f7c = f7c - 0.002f;
            }
            int item = slot->item;
            int value;
            if (item < 0) {
                value = 0;
            } else {
                value = g_items[item].f1fc;
            }
            f7c = (float)value * 7.999999e-06f + f7c;
        }
        if (zoom < 0.0f) {
            zoom = 0.0f;
        } else if (zoom > 1.0f) {
            zoom = 1.0f;
        }
        if (f7c < 0.0f) {
            f7c = 0.0f;
        } else if (f7c > 1.0f) {
            f7c = 1.0f;
        }
        g = g_game;                                  // only on this path: with the food slot disabled the original keeps edi
    }

    // the player's item grid decay
    Troop* pc = &g->troops[g->player];
    PartyMgr* m = &g->parties;
    float grid_chance = (float)m->at(m->lookup(g->f78))->skill_level(0xb) * 0.001f;
    if (!(GDP(float, 0x13ad0) > 0.0f)) {
        grid_chance = grid_chance * 1.5f;
    }
    for (int i = 0; i < pc->inv_count(); i++) {
        ItemRef* g = &pc->inv[i];
        if (g->item >= 0
            && *(unsigned char*)&g_items[g->item].type_flags == 1
            && (g->m4 & 0xff000000) == 0x1e000000
            && (float)(rand() % 15817) * g_rand_k < grid_chance) {
            WL_MARKER(12);
            *((unsigned char*)g + 7) = 0;
            g->reduce(0);
            WL_MARKER(13);
        }
    }
}

// @ 0x4b32f0
void Party::clear_map_flag()
{
    if (owner <= 0 && (*(unsigned __int64*)&flags_lo & 0x400)) {
        return;
    }
    if (model != 0) {
        ((rgl::world*)g_script.level)->remove_entity(((int*)model)[2]);
    }
    unsigned __int64 flags = *(unsigned __int64*)&flags_lo | 0x100;
    model = 0;
    *(unsigned __int64*)&flags_lo = flags;
}

// @ 0x4b3350
MB_NOINLINE void construct_events_n(void* dst, unsigned count, const void* src, ElemCopier* ctx, int tag)
{
    try {
        while (count > 0) {
            ctx->copy_event(dst, (void*)src);
            count--;
            dst = (char*)dst + 0x5c;
        }
    } catch (...) {
        throw;
    }
}

// @ 0x4b33b4
void throw_stub_33b4()
{
    throw;
    __assume(0);
}

// @ 0x4b33c0
// Start a new search generation of the world map's formation records (NavData::m64 counts the searches): when the counter
// gets large, clear every record's stamp (+0x6c) and restart it; then advance it.
void NavData::reset()
{
    if (m64 > 1000000000) {
        for (unsigned i = 0; i < cells.size(); i++) {
            std::vector<NavRec>::iterator it = (*(std::vector<NavRec>*)&cells).begin() + i;
            it->gen() = 0;
        }
        m64 = 0;
    }
    m64 += 3;
}


// ---- the world-map walk test and the event record frame -------------------------------------------------------------------

static __forceinline void cross_into(vector4& d, const vector4& a, const vector4& b)
{
    d.x = a.y * b.z - a.z * b.y;
    d.y = a.z * b.x - a.x * b.z;
    d.z = a.x * b.y - a.y * b.x;
}

#define RECS ((NavRec*)cells.first)

// @ 0x4b3420
// Walk the formation-record graph from `from` toward `to`, testing every record edge for a crossing of the move segment
// (0.01 <= t <= 0.99, 0 <= s <= 1), honouring the order-target mask and the per-edge keep distance.
bool NavData::can_move(int from, float x, float y, int to, const float* p, unsigned flags, int max)
{
    if (!shares(from, to))
        return false;
    int cur = -1;
    if (from < 0 || to < 0)
        return cur == to;
    float dx = p[0] - x;
    float dy = p[1] - y;
    if (dx * dx + dy * dy < 0.0001f)
        return true;
    int vert = 1;
    if (-0.000001f < dx && dx < 0.000001f)
        vert = 0;
    bool stop = false;
    int hops = 0;
    int prev = -1;
    if (from != to) {
        do {
            if (from < 0 || stop || hops >= max)
                break;
            hops++;
            int hit = -1;
            NavRec* r = &RECS[from];
            int i = 0;
            while (i < r->npts) {
                if (r->agents()[i] != prev) {
                    NavVert* a = &verts.first[r->verts()[i]];
                    NavVert* b = &verts.first[r->verts()[(i + 1) % r->npts]];
                    float ex = b->p.x - a->p.x;
                    float ey = b->p.y - a->p.y;
                    float t = ((x - a->p.x) * dy - (y - a->p.y) * dx) / (ex * dy - ey * dx);
                    float s;
                    if (vert)
                        s = ((ex * t + a->p.x) - x) / dx;
                    else
                        s = ((ey * t + a->p.y) - y) / dy;
                    if (0.01f <= t && t <= 0.99f && 0.0f <= s && s <= 1.0f)
                        hit = i;
                }
                i++;
                if (hit != -1)
                    break;
            }
            if (hit < 0) {
                stop = true;
                continue;
            }
            int tgt = r->targets()[hit];
            prev = r->agents()[hit];
            if (tgt < 0 || 100.0f < r->dists()[hit] || (RECS[tgt].mask() & flags) != flags)
                stop = true;
            else
                from = tgt;
        } while (from != to);
    }
    return from == to;
}

// Orient an event record on the map: its frame is the identity with the target as origin; the face under the target gives
// the height and the up row (the face's record), the heading rotates the forward row, and the rows are scaled by the size
// class (0.5 + 0.01 * size, at most 2.5).
// @ 0x4b36a0
void GameData::init_event(E92* rec)
{
    EventRec* r = (EventRec*)rec;
    vector4 t = r->target;
    float angle = r->angle;
    rgl::matrix m;
    r->pos = *rgl::identity_pos(&m);
    r->pos.o = t;
    face* e = find_at(&t.x);
    if (e) {
        r->pos.o.z = e->w[0].z - ((t.x - e->w[0].x) * e->normal.x + (t.y - e->w[0].y) * e->normal.y) / e->normal.z + 0.01f;
        r->pos.u = nav_recs()[e->m4].trans;
        float c = cosf(angle);
        float s = sinf(angle);
        float fx = r->pos.f.x, fy = r->pos.f.y;
        r->pos.f.x = c * fx - s * fy;
        r->pos.f.y = s * fx + c * fy;
        // both cross products are built in t's slot (dead after find_at) and copied out whole, so the frame's s.w and
        // f.w are the target's w (0x4b37f0, 0x4b3846 read it back from t)
        cross_into(t, r->pos.f, r->pos.u);
        r->pos.s = t;
        r->pos.s.normalize();
        cross_into(t, r->pos.u, r->pos.s);
        r->pos.f = t;
        r->pos.f.normalize();
    }
    float k = (float)(r->scale & 0xff) * 0.01f + 0.5f;
    if (k > 2.5f)
        k = 2.5f;
    r->pos.s.x *= k; r->pos.s.y *= k; r->pos.s.z *= k;
    r->pos.f.x *= k; r->pos.f.y *= k; r->pos.f.z *= k;
    r->pos.u.x *= k; r->pos.u.y *= k; r->pos.u.z *= k;
}

// @ 0x4b38f0
void ElemCopier::move_events(E92* first, E92* last, E92* dst)
{
    construct_events(first, last, dst, this, *(ptr_tag*)&dst);
}

// ---- the party scanning for targets, re-issuing its order and planning the next one -----------------------------------------

static __forceinline PartyMgr* parties() { return &g_game->parties; }

// @ 0x4b3910
void Party::collect_targets(int* out, int* max, int* count)
{
    *count = 0;
    float radius = ((float)skill_level(0xd) * 0.1f + 1.0f) * g_module_settings.seeing_range;
    if (g_game->is_night())
        radius = radius * 0.5f;
    unsigned id = (unsigned)parties()->first_party_id();
    while (id < (unsigned)parties()->deflt) {
        if (*count >= *max)
            return;
        int idx = parties()->lookup(id);
        Party* p = &parties()->pages[idx >> 4][idx & 0xf];
        if (p->f8c8 >= 0)
            goto next;
        unsigned f = p->flags_lo;
        if ((f & 0x1080100) != 0)
            goto next;
        if (id == (unsigned)this->id)
            goto next;
        {
            unsigned g1 = p->map_mask & 7;
            char a = ((g1 >= 2 && g1 < 7) || p->map_mask == 7);
            unsigned g2 = this->map_mask & 7;
            char b = ((g2 >= 2 && g2 < 7) || this->map_mask == 7);
            if (a != b)
                goto next;
        }
        if (p->id == g_game->f78 && 0.0f < GDP(float, 0x13ad0) && GDP(int, 0x13ad4) != 0)
            goto next;
        if ((f & 0x400) != 0)
            goto next;
        {
            int ag = p->f88;
            if (ag >= 0 && parties()->is_live(battles()->pages[ag >> 4][ag & 0xf].party_b)) {
                if (parties()->get(battle_get((unsigned)ag)->party_b)->flags_lo & 0x400)
                    goto next;
            }
        }
        if (p->action == 8)
            goto next;
        {
            vector4 d = *(vector4*)&p->pos - *(vector4*)&this->pos;
            float len = d.out_length();
            if (!(len < radius))
                goto next;
            if (!(len < p->strength() * radius))
                goto next;
        }
        g_script.trigger_result.store0 = 1;
        g_script.trigger_result.store1 = 0;
        g_game->emit_event(0x19, this->id, (int)id);
        if ((g_script.trigger_result.store1 | g_script.trigger_result.store0) != 0)
            out[(*count)++] = (int)id;
    next:
        parties()->next_party_id((int*)&id);
    }
}

// @ 0x4b3bc0
void Party::reissue_order()
{
    if (action == 9) {
        set_order_face((int*)&battles()->pages[(unsigned)target >> 4][target & 0xf].x2,
                       battles()->pages[(unsigned)target >> 4][target & 0xf].face2);
    } else if (action == 6) {
        set_order_pos((int*)&order_pos);
    } else if (action == 4 || action == 1) {
        if (target >= 0 && parties()->is_live(target)) {
            if (parties()->get(target)->flags_lo & 0x400)
                set_order_face((int*)&parties()->get(target)->pos,
                               parties()->get(target)->mesh);
            else
                set_order_party(&target);
        }
    }
    order_state = 0;
}


// ---- the map-order planner ----------------------------------------------------------------------------------------------
// The two terrain-kind tests live in ui/mission_ui.cpp (0x489390, 0x4893b0), but this unit saw their bodies (header
// inlines in the original): Party::plan_order keeps `flags & 0x200` in a register across the call to the first one, which
// cl only does for a callee it knows writes no memory, and it has the first one expanded inline in its second loop.
inline int __cdecl terrain_is_road(unsigned v)           // 0x489390 (is-farm/road terrain kind)
{
    if ((v & 7) != 0 && v != 7)
        return 0;
    return 1;
}
inline int terrain_is_water(unsigned x)                  // 0x4893b0 (is-water terrain kind)
{
    int m = x & 7;
    if (m < 2 || m >= 7) {
        if (x != 7)
            return 0;
    }
    return 1;
}

static __forceinline bool party_flag(const Party* o, int mask) { return (o->flags & mask) != 0; }
static __forceinline Vec2 v2_mul(const vec2& v, float k) { return Vec2(v.x * k, v.y * k); }
static inline void rot2(vec2& v, float ang)
{
    float c = cosf(ang);
    float s = sinf(ang);
    float x = c * v.x - s * v.y;
    v.y = c * v.y + s * v.x;
    v.x = x;
}
// @ 0x4b3ce0
// Act on the current map order: finish a move, chase a party or an agent, or sample a random walkable spot around the
// current position and hand it to set_order_face as a new order.
// What the original's code shows about the source (each of these changes registers, x87 operand order or frame slots,
// not the instructions' meaning):
//  - one function-scope float is the length in the act == 6 arm and the scale in both search loops (its low symbol key
//    puts the vector's member first in `v.x * len`, which keeps len on the x87 stack);
//  - the search loops run on a `found` flag set in the two accepting arms (cl threads it away; the store in the arm
//    is what makes the inliner leave terrain_is_water as a call in the second loop while it expands terrain_is_road);
//  - `flags & 0x200` is tested on the member both times (see the note at terrain_is_road);
//  - in the second loop the target point `to` and the sampled point `p` are two variables: `to` is never address-taken,
//    so its two loads are scheduled before its two stores.
void Party::plan_order()
{
    float len;
    int act = action;
    if (act == 0) { kind = 0; moving = 0; f5988 = 0; return; }
    if (act == 7) { kind = 0; moving = 0; f5988 = 0; return; }
    if (act == 8) { kind = 0; moving = 0; f5988 = 0; return; }

    if (act != 2 && act != 3) {
        if (act == 9)
            return;
        if (act == 6) {
            float dx = order_pos.x - pos.x;
            float dy = order_pos.y - pos.y;
            len = sqrtf(dy * dy + dx * dx);
            if (len < 0.01f)
                action = 0;
            return;
        }

        int t = target;
        int* tp = &target;
        if (target >= 0 && (act == 4 || act == 1)) {
            PartyMgr* m = &g_game->parties;
            if (m->is_live(t)) {
                if (party_flag(m->get(t), 0x400))
                    return;
                set_order_party(tp);
                return;
            }
            action = 0;
            return;
        }
        if (t >= 0 && act == 10) {
            if (g_game->parties.is_live(t)) {
                set_order_agent(tp);
                return;
            }
            action = 0;
            return;
        }
        if (act != 5 && act != 11)
            return;
        {
            PartyMgr* m = &g_game->parties;
            if (m->is_live(t)) {

            vec2r fwd;
            fwd.x = 0.0f;
            fwd.y = 1.0f;
            ((vec2x*)&fwd)->rotate_a(angle);
            Party* q = m->get(t);
            vec2 o = *(vec2*)&q->pos;
            vec2r d;
            d.x = o.x - pos.x;
            d.y = o.y - pos.y;
            d.normalize();
            {
                Vec2 a(fwd.x * 0.85f, fwd.y * 0.85f);
                Vec2 b(-d.x, -d.y);
                d.x = b.x + a.x;
                d.y = a.y + b.y;
            }
            if (d.y * d.y + d.x * d.x < 1e-13f) {
                fwd.x = 1.0f;
                fwd.y = 0.0f;
                *(vec2*)&d = *(vec2*)&fwd;
            }
            d.normalize();

            bool give_up = false;
            int tries = 0;
            float spread = 0.6f;
            face* a;
            bool found = false;
            while (!found) {
                ((vec2x*)&d)->rotate_a((frand() - 0.5f) * spread);
                len = speed_factor() * (interval + interval + 0.5f);
                len = len + len;
                Vec2 s = v2_mul(d, len);
                fwd.x = s.x + pos.x;
                fwd.y = s.y + pos.y;
                a = g_game->find_at(&fwd.x);
                tries++;
                spread += 0.4f;
                if (tries > 6) {
                    kind = 0;
                    moving = 0;
                    f5988 = 0;
                    give_up = true;
                }
                if (a) {
                    int& cell = a->m4;
                    NavData* w = ((rgl::world*)g_script.level)->ground;
                    if (w->shares(cell, mesh->m4)) {
                        if ((flags & 0x200) && (char)terrain_is_road(((NavRec*)&(*(std::vector<NavRec>*)&w->cells)[cell])->mask())) {
                            found = true;
                            continue;
                        }
                        if (!(flags & 0x200) && (char)terrain_is_water(((NavRec*)&(*(std::vector<NavRec>*)&w->cells)[cell])->mask())) {
                            found = true;
                            continue;
                        }
                    }
                }
                if (give_up)
                    return;
            }
            set_order_face((int*)&fwd, a);
            return;
            }
        }
        action = 0;
        return;
    }

    {
        clk8f8.restamp();
        vec2 to;
        int t;
        PartyMgr* m;
        if (action == 3 && (t = target) >= 0 && (m = &g_game->parties)->is_live(t)) {
            to = *(vec2*)&m->get(t)->pos;
        } else {
            to = *(vec2*)&order_pos;
        }
        bool give_up = false;
        int tries = 0;
        float spread = 0.6f;
        face* a;
        vec2 delta;
        delta.x = to.x - pos.x;
        delta.y = to.y - pos.y;
        vec2 p;
        bool found = false;
        while (!found) {
            vec2 h = *(vec2*)&dirx;
            float ang = ((float)(rand() % 15817) * 6.322312e-05f - 0.5f) * spread * 3.1415927f;
            rot2(h, ang);
            float m2 = (float)(rand() % 15817) * 0.00012644623f + 1.0f;
            len = speed_factor() * m2;
            Vec2 w;
            w.x = h.x * len;
            w.y = h.y * len;
            p.x = w.x + pos.x;
            p.y = w.y + pos.y;
            len = (float)(rand() % 15817) / f8f4 * 6.322312e-05f;
            Vec2 e(delta.x * len, delta.y * len);
            p.x = e.x + p.x;
            p.y = e.y + p.y;
            a = g_game->find_at(&p.x);
            tries++;
            spread += 0.4f;
            if (tries > 6) {
                kind = 0;
                moving = 0;
                f5988 = 0;
                give_up = true;
            }
            if (a) {
                int& cell = a->m4;
                NavData* g = ((rgl::world*)g_script.level)->ground;
                if (g->shares(cell, mesh->m4)) {
                    if ((flags & 0x200) && (char)terrain_is_road(((NavRec*)&(*(std::vector<NavRec>*)&g->cells)[cell])->mask())) {
                        found = true;
                        continue;
                    }
                    if (!(flags & 0x200) && (char)terrain_is_water(((NavRec*)&(*(std::vector<NavRec>*)&g->cells)[cell])->mask())) {
                        found = true;
                        continue;
                    }
                }
            }
            if (give_up)
                return;
        }
        set_order_face((int*)&p, a);
    }
}

// @ 0x4b4320
// Advance along the current route: walk the path towards the next waypoint and publish it as the destination.
void Party::step_route()
{
    if (moving == 0)
        return;
    int z = 0;
    int n = npath;
    int cur = mesh->m4;
    if (cur == *(int*)((char*)this + 0x968 + n * 4)) {     // arrived at the last node
        moving = z;
        f5988 = z;
        return;
    }
    if (cur >= 0) {
        int i = cursor;
        vec2 d;
        int goal;
        if (i + 1 < n) {
            d = *(vec2*)((char*)this + 0x197c + i * 0x10);
            goal = *(int*)((char*)this + 0x970 + i * 4);
        } else {
            d.x = goal_x;
            d.y = goal_y;
            goal = f5984;
        }
        float lim;
        int tries;
        if (id == g_game->f78) {
            lim = 22500.0f;
            tries = 200;
        } else {
            lim = 900.0f;
            tries = 10;
        }
        float ndx = pos.x - d.x;
        float ndy = pos.y - d.y;
        float dist = ndx * ndx;
        dist += ndy * ndy;
        if (dist < lim && ((rgl::world*)g_script.level)->ground->can_move(cur, pos.x, pos.y, goal, &d.x, 0, tries))
            cursor = i + 1;
    }
    if (cursor >= n) {
        moving = z;
        f5988 = z;
        return;
    }
    int off = cursor * 0x10;
    int vv = *(int*)((char*)this + 0x196c + off);
    dest_xi = vv;
    dest_yi = *(int*)((char*)this + 0x1970 + off);
}


// ---- stacks, party values and the party AI --------------------------------------------------------------------------------
#pragma warning(disable : 4700)               // the original returns the never-written `result` local on the "already present" path
// @ 0x4b4470
// 4 stack arguments (ret 0x10): the 4th is stored in the row's +0x10 field.  The "already present" path returns the
// (never written) result local, exactly like the original (it reads [esp+0x10] uninitialised).
int Party::force_add_members(int troop, int count, unsigned flags, int f10)
{
    int result;
    if (count == 0)
        return 0;
    int n = nstacks;
    int found = -1;
    for (int i = 0; i < n; i++) {
        if (stacks[i].troop == troop) {
            if (((stacks[i].flags ^ flags) & 1) == 0)
                found = i;
        }
    }
    if (found != -1)
        return result;
    int cls = flags & 1;
    int lead;
    if (cls != 0) {
        lead = 0;
        for (int k = 0; k < n; k++) {
            if (stacks[k].flags & 1)
                break;
            lead++;
        }
        if (n - lead < 0x20)
            goto make;
    }
    if (cls == 0) {
        lead = 0;
        for (int k = 0; k < n; k++) {
            if (stacks[k].flags & 1)
                break;
            lead++;
        }
        if (lead < 0x20)
            goto make;
    }
    goto full;
make:
    if (n >= 0) {
        Stack* s = &stacks[n];
        s->count = 0;
        s->troop = -1;
        s->flags = 0;
        stacks[nstacks].troop = troop;
        stacks[nstacks].count = count;
        s->f0c = 0;
        s->f10 = 0;
        s->f14 = 0;
        stacks[nstacks].f10 = f10;
        stacks[nstacks].f14 = 0;
        s->f18 = 0;
        *(int*)((char*)this + (nstacks + 6) * 0x20) = 0;
        stacks[nstacks].flags = flags;
        int idx = nstacks + 1;
        nstacks = idx;
        if (cls == 0)
            remove_slot(idx - 1);
        result = n;
        dirty = 1;
        return result;
    }
full:
    result = -1;
    if ((g_game->troops[troop].flags30 & 0x10) != 0) {
        found = -1;
        for (int i = 0; i < nstacks; i++) {
            if ((g_game->troops[stacks[i].troop].flags30 & 0x10) == 0 && (int)(stacks[i].flags & 1) == cls)
                found = i;
        }
        if (found >= 0) {
            g_game->remove_troops(id, stacks[found].troop, stacks[found].count, (char)cls, 0);
            result = force_add_members(troop, count, flags, f10);
        }
    }
    dirty = 1;
    return result;
}

// @ 0x4b4700
void GameData::refresh_party(unsigned id)
{
    PartyMgr* m = &parties;
    if (id >= m->n_parties)
        return;
    int idx = m->lookup(id);
    if (idx < 0)
        return;
    if (m->atu(idx)->type != 1)
        return;
    int j = m->lookup(id);
    Party* p = m->at(j);
    for (int i = 0; i < p->nstacks; i++) {
        Stack* st = &p->stacks[i];
        if (st->f14 > st->count)
            st->f14 = st->count;
    }
    for (int i = p->nstacks - 1; i >= 0; i--) {
        if (p->stacks[i].count <= 0) {
            int slot[8];
            p->remove_row(slot, i, (int)id == f78 ? (int*)((char*)this + 0x1405c) : 0);
        }
    }
    p->update();
}

// @ 0x4b4800
float GameData::party_value(unsigned id)
{
    float total = 0.0f;
    if (id >= parties.n_parties)
        return total;
    int idx = parties.lookup(id);
    if (idx < 0)
        return total;
    if (parties.pages[idx >> 4][idx & 0xf].type != 1)
        return total;
    int j = parties.lookup(id);
    Party* p = &parties.pages[j >> 4][j & 0xf];
    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if ((s.flags & 1) != 0)
            continue;
        Troop* t = &troops[s.troop];
        float lv = (float)t->level + 12.0f;
        if ((t->flags30 & 0x10) != 0) {
            if (this->agent_low(s.troop))
                continue;
            total += lv * lv * 0.01f;
        } else {
            total += (float)(s.count - s.f10) * lv * lv * 0.01f;
        }
    }
    for (int i = 0;; i++) {
        int idx2 = parties.lookup(id);
        Party* q = &parties.pages[idx2 >> 4][idx2 & 0xf];
        if (q->attached._Myfirst == 0 || (unsigned)(q->attached._Mylast - q->attached._Myfirst) <= (unsigned)i)
            break;
        int idx3 = parties.lookup(id);
        int copy = parties.pages[idx3 >> 4][idx3 & 0xf].attached._Myfirst[i];
        total = party_value(copy) + total;
    }
    return total;
}

// @ 0x4b49d0
void* ElemCopier::fill_n_events(void* dst, int count, const void* val)
{
    construct_events_n(dst, count, val, this, (int)val);
    return (char*)dst + count * 0x5c;
}

inline float vec2::normalize()
{
    float len = (float)sqrt(y * y + x * x);
    if (len > 0.0f) {
        float inv = 1.0f / len;
        x = inv * x;
        y = inv * y;
    } else {
        x = 0.0f;
        y = 1.0f;
    }
    return len;
}
static inline float fmin1(float v)
{
    if (v < 1.0f)
        v = 1.0f;
    return v;
}
static inline float fclamp(float v, float lo, float hi)
{
    if (v < lo)
        v = lo;
    else if (v > hi)
        v = hi;
    return v;
}
static __forceinline BattleRec* battle_page(unsigned t)
{
    std::vector<BattleRec*>& pages = *(std::vector<BattleRec*>*)((char*)g_game + 0xe0);
    return pages[t >> 4] + (t & 0xf);
}
static inline float norm_xy(vec2& d)
{
    float len = sqrtf(d.x * d.x + d.y * d.y);
    if (len > 0.0f) {
        float r = 1.0f / len;
        float nx = d.x * r;
        float ny = d.y * r;
        d.x = nx;
        d.y = ny;
    } else {
        d.x = 0.0f;
        d.y = 1.0f;
    }
    return len;
}
// @ 0x4b4a00
// The party's map AI step: gather the nearby parties (0x4b3910), score each one from troop strength, relation and distance,
// then either commit a new order (action / target) or fall back to the pending one (state, +0x8e4).
void Party::think()
{
    if (action == 7 || action == 8)
        return;
    if (f88 != -1)
        return;
    if (flags & 0x400)
        return;

    int ids[32];
    int maxn = 32;
    int n;
    collect_targets(ids, &maxn, &n);

    float base = g_game->party_value(id);
    float lo = 0.0f;
    float hi = base + 0.1f;
    set_speed_from_stat();

    float near_v[32];
    float adj_v[32];
    float rel_v[32];
    float accum[32];
    float score[32];
    int ckind[32];
    char friendly[32];

    {
        for (int i = 0; i < n; i++)
            accum[i] = 0.0f;
    }
    {
        int me = id;
        PartyMgr* m = &g_game->parties;
        for (int i = 0; i < n; i++) {
            score[i] = 0.0f;
            ckind[i] = 0;
            int tid = ids[i];
            Party* p = m->at(m->lookup(tid));
            float rel = g_game->relation(me, tid);
            rel_v[i] = rel;
            float v = g_game->party_value(tid);
            near_v[i] = v;
            adj_v[i] = v;
            friendly[i] = 1;
            int sub = p->f88;
            if (sub >= 0) {
                BattleRec* b = battle_page(sub);
                unsigned other = b->party_b;
                if (other == (unsigned)tid)
                    other = (unsigned)b->party_a;
                if (other < m->n_parties) {
                    int oi = m->lookup(other);
                    if (oi >= 0 && m->atu(oi)->type == 1) {
                        GameData* g = g_game;
                        float ov = g->party_value(other);
                        float a = v - ov * 0.9f;
                        adj_v[i] = a;
                        if (adj_v[i] < 1.0f)
                            adj_v[i] = 1.0f;
                        Party* op = m->at(m->lookup(other));
                        accum[i] = ov + accum[i];
                        friendly[i] = ((float*)g->factions)[op->f70 * 0xc0 + f70 + 0xb] >= 0.0f ? 1 : 0;
                    }
                }
            }
            if (sub == -1) {
                if (rel < -0.01f)
                    lo = v + lo;
                else if (rel > 0.01f)
                    hi = v + hi;
            }
        }
    }

    float frac = lo / hi;
    int i = 0;
    if (n > 0) {
        unsigned a74 = f74;
        PartyMgr* m = &g_game->parties;

        float prod = f8b0 * f8ac;
        float prod2 = prod + 0.05f;
        do {
            int tid = ids[i];
            Party* p = m->at(m->lookup(tid));
            float x = base / fmin1(near_v[i] - accum[i] * 0.9f) * (1.0f / frac);
            int nib = a74 & 0xf;
            float mod = (nib != 0 ? (float)nib * 0.125f : 1.0f) * x;
            if (mod < 0.2f)
                mod = 0.2f;
            else if (mod > 5.0f)
                mod = 5.0f;

            PartyV4* tp = &m->at(m->lookup(tid))->pos;
            vec2 d;
            d.x = tp->x - pos.x;
            d.y = tp->y - pos.y;
            float dist = norm_xy(d);
            float invd = 3.0f / dist;
            if (invd < 0.6f)
                invd = 0.6f;
            else if (invd > 2.0f)
                invd = 2.0f;

            float scale = fclamp(prod / (m->at(m->lookup(tid))->f8ac * m->at(m->lookup(tid))->f8b0), 0.5f, 1.1f);

            float sc = (float)((int)a74 >> 4 & 0xf) * -rel_v[i] * scale * invd * mod * 0.125f;
            if (rel_v[i] < 0.0f) {
                float s = 0.0f;
                if (a74 & 0x100)
                    s = 1.0f;
                float alt = s * p->f78 * scale * invd * mod;
                if (alt > sc)
                    sc = alt;
            }
            if (near_v[i] * 3.0f < accum[i]) {
                float t = near_v[i] / accum[i] * sc;
                sc = t + t;
            }

            float freq = nib != 0 ? (float)nib * 0.125f : 1.0f;
            if (mod < 1.0f / freq || !friendly[i]) {
                sc = 0.0f;
            } else if (tid == g_game->f78 && (float)(g_ticks[ignore_timer.clock] - ignore_timer.start) * 1e-05f < 0.0f) {
                sc = 0.0f;
            }

            if (m->at(m->lookup(tid))->f8ac * m->at(m->lookup(tid))->f8b0 > prod2 && sc > 0.01f && p->f88 < 0) {
                float ang = m->at(m->lookup(tid))->angle;
                float c = (float)cos(ang);
                float s = (float)sin(ang);
                float fx = c * 0.0f - s;
                float fy = s * 0.0f + c;
                if (fy * d.y + fx * d.x > 0.8f)
                    sc = sc * 0.1f;
            }

            if (sc > score[i]) {
                score[i] = sc;
                ckind[i] = 4;
            }

            float alt2 = 0.0f;
            if (rel_v[i] < -0.01f && mod < 1.0f
                && (float)((int)p->f74 >> 4 & 0xf) * 0.125f > 0.0f
                && !(p->flags & 0x400) && p->f88 < 0 && p->f8c8 < 0)
                alt2 = (1.0f - rel_v[i]) * invd / (mod * mod) - 1.0f;
            if (alt2 > score[i]) {
                score[i] = alt2;
                ckind[i] = 5;
            }
        } while (++i < n);
    }

    float best = -10.0f;
    int bi = -1;
    for (i = 0; i < n; i++) {
        if (best < score[i]) {
            best = score[i];
            bi = i;
        }
    }
    best = best * f958;

    float rnd = (float)(rand() % 0x3dc9) * 3.1611557e-06f + 0.15f;

    if (bi == -1 || best < rnd || (flags & 0x10000))
        goto pending;
commit:
    if (bi == -1)
        goto stay;
    action = ckind[bi];
    target = ids[bi];
    reissue_order();
    return;

pending:
    if (state == 4) {
        PartyMgr* m = &g_game->parties;
        unsigned pid = (unsigned)state_target;
        if (pid >= m->n_parties)
            goto commit;
        int oi = m->lookup(pid);
        if (oi < 0 || m->atu(oi)->type != 1)
            goto commit;
        Party* q = m->at(m->lookup(pid));
        if (q->f8c8 >= 0 || (q->flags & 0x1080100))
            goto commit;
        if (m->at(m->lookup(pid))->action == 8)
            goto commit;
    }

    action = state;
    target = state_target;
    f924 = state_f8ec;
    f928 = state_f8f0;
    if (action == 2 || action == 3) {
        if ((float)(rand() % 0x3dc9) * 6.322312e-05f < 0.25f) {
            action = 0;
            reissue_order();
            return;
        }
    }
    goto done;
stay:
    action = 2;
    f924 = *(int*)((char*)this + 0x30);
    f928 = *(int*)((char*)this + 0x34);
done:
    reissue_order();
}


// ---- the vector<E92> / vector<E12> instances the linker kept from this object ----------------------------------------------
// 0x4b54a0 vector<E92>::_Insert_n (+ 0x4b566f catch funclet and cold branches)
template void std::vector<E92>::_Insert_n(std::vector<E92>::iterator, size_t, const E92&);
// 0x4b5790 vector<E12>::_Insert_n (+ 0x4b5937 catch funclet and cold branches)
template void std::vector<E12>::_Insert_n(std::vector<E12>::iterator, size_t, const E12&);
// 0x4b5a40 vector<E92>::insert(iterator, const E92&)
template std::vector<E92>::iterator std::vector<E92>::insert(std::vector<E92>::iterator, const E92&);
// 0x4b5ab0 vector<E12>::insert(iterator, const E12&)
template std::vector<E12>::iterator std::vector<E12>::insert(std::vector<E12>::iterator, const E12&);
// 0x4b5b20 vector<E92>::push_back(const E92&)
template void std::vector<E92>::push_back(const E92&);
// 0x4b5bb0 vector<E12>::push_back(const E12&)
template void std::vector<E12>::push_back(const E12&);

// ---- the group power evaluator and the event records -----------------------------------------------------------------------
// @ 0x4b5c30
float __cdecl march_time(Party* self)
{
    float ally_sum = 0.0f;
    float min_val = 100000.0f;
    int total = 0;
    int bonus = self->skill_level(0xe);
    float base_power = (float)((unsigned char*)self)[0xa6] * 50.0f;
    int ally_total = 0;
    int other_sum = 0;
    int kind1 = 0;

    Party rec;                                          // a scratch block: constructed by 0x4934d0, destroyed by 0x431b90
    rec.nstacks = 0;
    g_game->fill_parties(self->id, &rec);
    rec.prepare();

    if (rec.nstacks > 0) {
        Troop* troops = g_game->troops;
        ItemKind* units = g_items;
        Stack* e = rec.stacks;
        int left = rec.nstacks;
        do {
            Troop* t = &troops[e->troop];
            float v = t->food_factor();
            int n = e->count;
            if (e->flags & 1)
                other_sum = other_sum + n;
            else
                ally_total = ally_total + n;
            ally_sum = (float)n * v + ally_sum;
            if (v < min_val)
                min_val = v;
            total = total + n;
            if (t->flags30 & 0x10) {
                int cnt = t->inv_count();
                for (int k = 0; k < cnt; k++) {
                    int u = t->inv[k].item;
                    if (u < 0)
                        continue;
                    ItemKind* ur = &units[u];
                    if ((ur->type_flags & 0xff) == 0xb)
                        base_power = base_power + ur->f208;
                    else if ((ur->type_flags & 0xff) == 1)
                        kind1 = kind1 + 1;
                }
            }
            e++;
        } while (--left);
    }

    float fx = (self->f7c - 0.5f) * 0.05f + 1.0f;
    float fy = 1.0f - self->zoom * 0.1f;
    if (min_val > 10000.0f)
        min_val = 0.0f;
    if (ally_sum == 0.0f)
        return 0.0f;
    int logarg = (other_sum >> 1) + 0x10 + (kind1 >> 1) + ally_total;
    float mid = min_val * 0.2f + (ally_sum / (float)total) * 0.8f;
    float inv = 1.0f / ((base_power / (float)(total + 3 + kind1 * 4)) * 0.007f + 1.0f);
    float r = ((mid * inv) * (1.0f + (float)bonus * 0.03f)) * fy;
    r = r * fx * 2.6f;
    r = r / log((float)logarg);
    return r;
}

struct Pos4i { int x, y, z, w; };
union EventFlags {                                   // the dword at event+0x18
    unsigned all;
    struct {
        unsigned low : 8;                            // +0x18 bits 0..7
        unsigned mid : 8;                            // +0x19 bits 8..15
        unsigned high : 4;                           // +0x1a bits 16..19
        unsigned unused : 8;
        unsigned top : 4;                            // +0x1b bits 28..31
    } f;
};
struct event_rec {                                   // 0x5c (E92)
    Pos4i p;                                          // +0x00
    float angle;                                     // +0x10 (random facing or supplied heading)
    int f14;                                         // +0x14
    EventFlags flags;                                // +0x18
    char pad1c[0x5c - 0x1c];
};

// @ 0x4b5fe0
void GameData::push_event(const int* pos, unsigned nation)
{
    event_rec r;
    r.p = *(const Pos4i*)pos;
    r.angle = (float)(rand() % 15817) * 0.00039724255f;
    r.f14 = 0;
    r.flags.all = (nation & 0xff) | 0x10000000;
    init_event((E92*)&r);
    GD(std::vector<E92>, 0x13a60).push_back(*(E92*)&r);
}

// @ 0x4b6070
void GameData::push_party_event(int party, const int* pos, const float* src)
{
    PartyMgr* pm = &parties;
    int idx = pm->lookup(party);
    Party* p = pm->pages[idx >> 4] + (idx & 0xf);
    event_rec r;
    r.p = *(const Pos4i*)pos;
    r.angle = *src;
    int a, b, c, d;
    a = b = c = d = 0;
    r.f14 = 0;
    count_units(party, &a, &b, &c, &d);
    r.flags.all = 0;
    r.flags.f.low = (a + c) & 0xff;
    r.flags.f.high = rand() % 16;
    r.flags.f.mid = (p->owner == -1) ? 0xff : p->owner;
    init_event((E92*)&r);
    GD(std::vector<E92>, 0x13a60).push_back(*(E92*)&r);
}


// ---- battle records: detach, commit, per-frame visuals -------------------------------------------------------------------
struct MissionCtx;
extern MissionCtx g_mission_ctx;            // 0xa73300

// helpers of BattleRec::detach (0x4b6180)
static __forceinline void detach_attached(Party* q)
{
    for (int k = (int)q->attached.size() - 1; k >= 0; k--) {
        int& mid = q->attached[k];
        if (g_game->parties.get_w(mid)->f8cc) {
            ((GameW*)g_game)->detach_party(mid);
            g_game->parties.get_w(q->attached[k])->action = 0;
        }
    }
}
static __forceinline bool party_valid(PartyMgr* m, unsigned id)
{
    if (id < m->n_parties) {
        int i = m->lookup(id);
        if (i >= 0 && m->atu(i)->type == 1)
            return true;
    }
    return false;
}
static __forceinline int party_troops(const Party* p)
{
    int sum = 0;
    for (int t = 0; t < p->nstacks; t++) {
        int c = p->stacks[t].count;
        if (c < 0)
            c = 0;
        sum += c;
    }
    return sum;
}
// @ 0x4b6180
// Unwind a battle record: both sides leave their battle state (animation stamp, speed, attached parties), the losses are
// reported, empty parties are removed (counting the kills for the player's side) and the record is released.
void BattleRec::detach()
{
    if (type == 1) {
        unsigned list1[128], list2[128];
        int n1 = 0;
        g_game->list_members(party_a, (int*)list1, &n1);
        int n2 = 0;
        int total = 0;
        g_game->list_members(party_b, (int*)list2, &n2);
        {
            PartyMgr* m = &g_game->parties;
            int a = party_a;
            if (m->is_live(a)) {
                Party* q = m->at(m->lookup(a));
                q->action = 0;
                q->interval = 0.4f;
                q->clk8d0.restamp();
                q->f88 = -1;
                total = q->sum_stat(0x10);
                detach_attached(q);
            }
        }
        {
            PartyMgr* m = &g_game->parties;
            unsigned b = party_b;
            if (party_valid(m, b)) {
                int sum = 0;
                Party* q = m->at(m->lookup(b));
                q->action = 0;
                q->interval = 0.1f;
                q->clk8d0.restamp();
                q->f88 = -1;
                for (int t = 0; t < q->nstacks; t++) {
                    Stack& s = q->stacks[t];
                    sum += (s.flags & 1) == 0 && s.count >= 0 ? s.count : 0;
                }
                total += sum;
                detach_attached(q);
            }
        }

        if (total < height) {
            rgl::vector4 v(x1, y1, z);
            g_game->push_event((const int*)&v, height);
        }

        char is1, is2;
        {
            int cur_id = g_game->f78;
            PartyMgr* m = &g_game->parties;
            Party* cur = m->at(m->lookup(cur_id));
            int st = (*(int*)((char*)&g_mission_ctx + 0x138));
            if (party_a == cur_id || (cur->f88 == rec_id && st == 0))
                is1 = 1;
            else
                is1 = 0;
            if (party_b == cur_id || (cur->f88 == rec_id && st == 1))
                is2 = 1;
            else
                is2 = 0;
        }

        for (int i = 0; i < n2; i++) {
            PartyMgr* m = &g_game->parties;
            unsigned id = list2[i];
            if (party_valid(m, id)) {
                Party* p = m->at(m->lookup(id));
                if (!(p->flags & 0x400) && party_troops(p) <= 0) {
                    if (is1 && p->owner >= 0)
                        g_party_templates[p->owner].num_destroyed_by_player++;
                    ((GameW*)g_game)->remove_party(list2[i]);
                }
            }
        }

        for (int i = 0; i < n1; i++) {
            PartyMgr* m = &g_game->parties;
            unsigned id = list1[i];
            if (party_valid(m, id)) {
                Party* p = m->at(m->lookup(id));
                if (!(p->flags & 0x400) && party_troops(p) <= 0) {
                    if (is2 && p->owner >= 0)
                        g_party_templates[p->owner].num_destroyed_by_player++;
                    ((GameW*)g_game)->remove_party(list1[i]);
                }
            }
        }
    }

    ((rgl::world*)g_script.level)->remove_entity(obj->id);
    obj = 0;
    int rid = rec_id;
    BattleTable* t = battles();
    t->pages[(unsigned)rid >> 4][rid & 0xf].live = 0;
    t->free_ids.push_back(rid);
}

// @ 0x4b6870
void BattleRec::commit()
{
    unsigned list1[128], list2[128];
    int n1 = 0, n2 = 0;                       // signed counts, zeroed before the calls (jle/jl loops in the original)

    g_script.param1 = party_b;
    g_script.param2 = party_a;
    g_script.trigger_result.value = 0;
    ((TimerList*)((char*)g_game + 0x139f4))->update(-5);
    if (g_script.trigger_result.value == 0)
        g_game->emit_event(2, party_b, party_a);

    g_game->list_members(party_a, (int*)list1, &n1);
    g_game->list_members(party_b, (int*)list2, &n2);

    for (int i = 0; i < n2; i++) {
        unsigned id = list2[i];
        if (id < g_game->parties.n_parties) {
            int j = g_game->parties.lookup(id);
            if (j >= 0 && (g_game->parties.pages[(unsigned)j >> 4] + (j & 15))->type == 1) {
                g_game->refresh_party(id);
                int m = g_game->parties.lookup(list2[i]);
                (g_game->parties.pages[m >> 4] + (m & 15))->dirty = 1;
            }
        }
    }

    for (int i = 0; i < n1; i++) {
        unsigned id = list1[i];
        if (id < g_game->parties.n_parties) {
            int j = g_game->parties.lookup(id);
            if (j >= 0 && (g_game->parties.pages[(unsigned)j >> 4] + (j & 15))->type == 1) {
                g_game->refresh_party(id);
                int m = g_game->parties.lookup(list1[i]);
                (g_game->parties.pages[m >> 4] + (m & 15))->dirty = 1;
            }
        }
    }

    if (g_script.trigger_result.value != 0) {
        detach();
        g_game->emit_event(0x1b, party_b, party_a);
    }
}

// @ 0x4b6ae0
void Party::leave_battle()
{
    if (f8c8 < 0 || !f8cc) {
        if (f88 >= 0) {
            unsigned bi = (unsigned)f88;
            std::vector<BattleRec*>& pages = *(std::vector<BattleRec*>*)((char*)battles() + 0x14);
            BattleRec* b = (BattleRec*)((char*)pages[bi >> 4] + (bi & 15) * 0x98);
            b->detach();
        }
    } else {
        ((GameW*)g_game)->detach_party(id);
    }
}


// ---- the world-map path finder and the nav drivers ----------------------------------------------------------------------
extern const float g_half;        // 0x549d70 = 0.5f
extern float g_105;         // 0x59364c = 10.5f
extern float g_005;         // 0x54b4d0 = 0.05f
extern float g_vis;         // 0x54a6f0 = 0.01f

struct DetailVert { char pad0[0x14]; rgl::vector4 p; };   // the 0x24-byte detail vertices of the map mesh (NavData +0x24)
#define NODE(i) ((std::vector<NavRec>&)cells)[i]
#define OPENQ (*(OpenQ*)&v30)                        // the open list: a vector<E12> at +0x30 (allocator word, first, last, end), then the comparator

// The search's open list as the original saw it: a priority queue at NavData +0x30 (vector<E12> plus its comparator at +0x40).
// E12 is the open-list record (OpenItem) as the shared vector instance names it. The heap steps follow std::pop_heap and
// std::push_heap on real iterators (compare PQ, terrain_b/poly_mesh.h, for the same search over the mission's nav mesh).
static __forceinline void pop_heap_1(OpenIt first, OpenIt last, OpenIt dest, OpenItem val, HeapGt pred)
{
    *oi(dest) = *oi(first);
    adjust_heap_node((hnode*)oi(first), 0, (int)(last - first), *(hnode*)&val, *(node_gt*)&pred);
}
static __forceinline void pop_heap_x(OpenIt first, OpenIt last, HeapGt pred)
{
    if (1 < last - first)
        pop_heap_1(first, last - 1, last - 1, *oi(last - 1), pred);
}
static __forceinline void pop_heap_y(OpenIt first, OpenIt last, HeapGt pred)
{
    if (1 < last - first)
        pop_heap_elem(oi(first), oi(last - 1), oi(last - 1), *oi(last - 1), pred, 0);
}
static __forceinline void push_heap_z(OpenIt first, OpenIt last, HeapGt pred)
{
    int n = (int)(last - first);
    if (n > 0)
        push_heap_elem(oi(first), n, 0, *oi(last), pred);
}
static __forceinline void push_heap_y(OpenIt first, OpenIt last, HeapGt pred)
{
    if (first != last)
        push_heap_z(first, --last, pred);
}
struct OpenQ {
    std::vector<E12> c;
    HeapGt comp;
    bool empty() const { return c.empty(); }
    const E12& top() const { return c.front(); }
    __forceinline void pop_x() { pop_heap_x(c.begin(), c.end(), comp); c.pop_back(); }
    __forceinline void pop_y() { pop_heap_y(c.begin(), c.end(), comp); c.pop_back(); }
    __forceinline void push_x(const E12& v) { c.push_back(v); push_back_elem(c.begin(), c.end(), comp); }
    __forceinline void push_y(const E12& v) { c.push_back(v); push_heap_y(c.begin(), c.end(), comp); }
};
static inline rgl::vector4 v4_sub(const rgl::vector4& a, const rgl::vector4& b) { return rgl::vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
// @ 0x4b6b40
// A* over the map's 0x88-byte formation records; fills the record-id chain, the per-node positions and the stall limit of
// the target node.
bool NavData::find_path(int from, int to, int* path, float* detail, int* count, int* visited, int* guard,
                       unsigned* mask_req, unsigned* mask_ban, char* avoid)
{
    (void)guard;
    int chain[0x2004];                               // frame 0x8070: the array ends 0x10 past chain[0x2000], which the original also writes

    // drain whatever the previous search left on the open list
    while (!OPENQ.empty())
        OPENQ.pop_x();

    reset();

    NODE(from).gen() = m64 + 1;
    NODE(from).parent() = from;
    OpenItem cur;
    cur.cost = 0.0f;
    cur.dist = 0.0f;
    cur.node = from;
    OPENQ.push_x(*(const E12*)&cur);
    *visited = 0;
    float best = 0.0f;
    float limit = 10000.0f;
    if (*avoid != 0)
        limit = 1e6f;

    while (NODE(to).gen() <= m64) {
        if (OPENQ.empty() || !(cur.dist < limit))
            break;
        (*visited)++;
        cur = *(const OpenItem*)&OPENQ.top();
        OPENQ.pop_y();

        int cn = cur.node;
        for (int k = 0; k < NODE(cn).npts; k++) {
            int nb = NODE(cn).targets()[k];
            if (nb != -1 && NODE(nb).gen() <= m64) {
                NODE(nb).gen() = m64 + 1;
                NODE(nb).parent() = cn;
                float cost = NODE(cn).dists()[k];
                if ((*mask_req & NODE(nb).mask()) != *mask_req)
                    cost += 10000.0f;
                if ((*mask_ban & NODE(nb).mask()) != 0)
                    cost += 10000.0f;
                rgl::vector4 dt = v4_sub(NODE(to).pos4(), NODE(nb).pos4());
                rgl::vector4 df = v4_sub(NODE(from).pos4(), NODE(nb).pos4());
                float d_from = df.length();
                float d_to = dt.length();
                cost += cur.cost;
                if (cur.cost > best)
                    best = cur.cost;
                OpenItem add;
                add.cost = cost;
                add.dist = (d_to / d_from + 1.0f) * cost;
                add.node = nb;
                OPENQ.push_y(*(const E12*)&add);
            }
        }
        NODE(cn).gen() = m64 + 2;
    }

    if (NODE(to).gen() <= m64) {
        *count = 0;
        return false;
    }

    bool reachable = true;
    if (!(best < 10000.0f))
        reachable = false;

    int len = 0;
    int f = to;
    while (f != from && len < 0x2000) {
        chain[len++] = f;
        f = NODE(f).parent();
    }
    chain[len++] = from;
    *count = len < 0x400 ? len : 0x400;

    rgl::vector4* out = (rgl::vector4*)detail;
    int prev = from;                             // the edge is looked up on the previous step's record
    for (int i = 0; i < *count; i++) {
        int id = chain[len - 1 - i];
        path[i] = id;
        int e = 0;
        NavRec& pr = NODE(prev);
        for (int j = 0; j < pr.npts; j++) {
            if (pr.targets()[j] == id) {
                e = j;
                break;
            }
        }
        out[i] = ((std::vector<DetailVert>&)v20)[NODE(prev).agents()[e]].p;
        prev = path[i];
    }

    for (int i = 1; i < *count; i++)
        out[i - 1] = out[i];
    out[*count - 1] = NODE(path[*count - 1]).pos4();
    return reachable;
}

// @ 0x4b71e0
void BattleRec::update_nav_anim(int)
{
    if (party_a == g_game->f78 || party_b == g_game->f78) {
        obj->visible = 0;
        return;
    }

    if (type == 1) {
        if (opacity > 0.5f && GDP(char, 0x95) == 0) {
            if (anim.elapsed() > 10.5f) {
                sound = g_sound_mgr.play(g_sound_mgr.slot[99], 0, obj->pos, 1.0f, 0);
                anim.start = g_ticks[anim.clock];
            }
        }
        if (opacity < 0.05f && sound != -1) {
            if (anim.elapsed() < 10.0f)
                FSOUND_StopSound(sound);
        }
        {
            __int64 elapsed = g_ticks[born.clock] - born.start;
            float secs = (float)elapsed * 1e-5f;
            if (next_simulation_time < secs) {
                next_simulation_time = frand() + g_one;
                born.start = g_ticks[born.clock];
                commit();
            }
        }
    }

    BattleObj* o = obj;
    if (o) {
        float rgba[4];
        rgba[0] = 1.0f;
        rgba[1] = 1.0f;
        rgba[2] = 1.0f;
        rgba[3] = opacity;
        unsigned color = rgl::pack_color((const vector4*)rgba);
        o->visible = (opacity > 0.01f) ? 1 : 0;
        unsigned i = 0;
        for (;;) {
            int* b = obj->begin;
            if (b == 0)
                break;
            if (i >= (unsigned)(((char*)obj->end - (char*)b) >> 2))
                break;
            *(unsigned*)(b[i] + 0x3c) = color;
            i++;
        }
    }
}

// @ 0x4b73c0
bool Party::start_path(int from, float tx, float ty)
{
    int mask_req = 0;
    int mask_ban = 0;
    char mine = (id == g_game->f78);
    bool ok = ((rgl::world*)g_script.level)->ground->find_path(mesh->m4, from, path, detail, &npath, &visited, &from,
                                      (unsigned*)&mask_req, (unsigned*)&mask_ban, &mine);
    goal_x = tx;
    goal_y = ty;
    cursor = 0;
    return ok;
}

// @ 0x4b7460
// The per-frame navigation driver of a map party: advance along the current path while it is being followed; otherwise pick
// the next destination from the order state -- the party's own position, the stored target position, the target party's
// position, or a jitter around it -- then make sure a path to the new target exists and consume the leading path nodes that
// the party can already move through.
void Party::update_nav()
{
    if (moving) {
        if (kind == 2) {
            int d = tgt_id;
            if (!(!g_game->parties.is_live(d))) { if (g_game->parties.get(d)->mesh != tgt) {
                moving = 0;
                f5988 = 1;
            } } else {
                kind = 0;
            }
        }
        if (moving) {
            step_route();
            if (moving)
                return;
        }
    }

    if (kind == 0) {
        dest_x = pos.x;
        dest_y = pos.y;
    } else if (kind == 1) {
        dest_x = lx;
        dest_y = ly;
    } else if (kind == 2) {
        int d = tgt_id;
        if (!(!g_game->parties.is_live(d))) {
            float* p = &g_game->parties.get(d)->pos.x;
            dest_x = p[0];
            dest_y = p[1];
            lx = p[0];
            ly = p[1];
        } else {
            kind = 0;
        }
    } else if (kind == 3) {
        if (!(frand() < 0.35f)) {
            // keep the destination of the previous frame
        } else {
            int d = tgt_id;
            if (g_game->parties.is_live(d)) {
                float* p = &g_game->parties.get(d)->pos.x;
                float dx = p[0] - pos.x;
                float dy = p[1] - pos.y;
                if (vx * vx + vy * vy < dx * dx + dy * dy) {
                    float* q = &g_game->parties.get(d)->pos.x;
                    dest_x = q[0];
                    dest_y = q[1];
                    dest_x = vx + dest_x;
                    dest_y = vy + dest_y;
                    lx = dest_x;
                    ly = dest_y;
                }
            }
        }
    }

    face* tf;
    if (!(kind == 1)) {
        int d = tgt_id;
        if (kind == 2 && g_game->parties.is_live(d)) {
            tgt = g_game->parties.get(d)->mesh;
            tf = tgt;
        } else if (kind == 3 && g_game->parties.is_live(d)) {
            tgt = g_game->parties.get(d)->mesh;
            tf = tgt;
        } else {
            tf = mesh;
        }
    } else {
        tf = tgt;
    }

    if (f5988 && tf && mesh->m4 != tf->m4) {
        if (!(!start_path(tf->m4, dest_x, dest_y))) {
            f598c = 0;
            f5984 = tf->m4;
            int lim = 10;                                    // 0x4b7758: the step budget ...
            int max = 10;                                    // ... and the last can_move argument are separate locals
            if (id == g_game->f78) {
                lim = 0x64;
                max = 0x96;
            }
            int progress = 0;
            while (progress < lim) {
                int from = mesh->m4;
                if (from < 0)
                    break;
                int cur = cursor;
                if (cursor + 1 >= npath)
                    break;
                float p4[4];
                p4[0] = detail[cur * 4 + 4];                 // 0x4b77ac: +0x197c + cur * 16
                p4[1] = detail[cur * 4 + 5];                 // 0x4b77b3: +0x1980 + cur * 16
                if (!((rgl::world*)g_script.level)->ground->can_move(from, pos.x, pos.y, path[cur + 1], p4, 0, max))
                    break;
                cursor = cur + 1;
                progress++;
            }
            moving = 1;
            step_route();
        } else {
            f598c = 1;
            f5988 = 0;
        }
    }
}


// ---- the map party model: built from its type record, refreshed every frame, the hud readout ------------------------------
struct PartyTypeDef {                                // 0x48-byte map party type record (table 0x5e29f0, indexed by flags & 0xff)
    char pad0[0x10];
    int f10;                                         // +0x10 bit 0: plain model (no fade ramp)   0x4b7840
    tstring name;                                    // +0x14 mesh resource   0x4b7840
    float scale;                                     // +0x24   0x4b7840, 0x4b7e50
    char pad28[4];
    vector4 pos;                                     // +0x2c translation of the rider mesh   0x4b7840
    TimerList frames;                                // +0x3c keyframe voices   0x4b7840
};
namespace rgl { vector4 unpack_color(const unsigned* argb); }   // 0x40e4e0 (cdecl, hidden return pointer: shapes.cpp)

#define SCALE_ROWS(m, a) \
    (m)->m_cc.s.x *= (a); (m)->m_cc.s.y *= (a); (m)->m_cc.s.z *= (a); \
    (m)->m_cc.f.x *= (a); (m)->m_cc.f.y *= (a); (m)->m_cc.f.z *= (a); \
    (m)->m_cc.u.x *= (a); (m)->m_cc.u.y *= (a); (m)->m_cc.u.z *= (a)

// helpers of Party::update_flags (0x4b7840)
#define ICON_DEF(i) (&((PartyTypeDef*)g_map_icons)[i])
#define MODEL_ENTS(m) (*(std::vector<rgl::mesh*>*)&(m)->ents)
static __forceinline void rows_scale(Rot3& r, const vector4& v)
{
    r.s.x = v.x * r.s.x; r.s.y = v.x * r.s.y; r.s.z = v.x * r.s.z;
    r.f.x = v.y * r.f.x; r.f.y = v.y * r.f.y; r.f.z = v.y * r.f.z;
    r.u.x = v.z * r.u.x; r.u.y = v.z * r.u.y; r.u.z = v.z * r.u.z;
}
// @ 0x4b7840
// Build the party's two model meshes (the party type's body and, for a rider, the rider mesh) from the type table, swap them
// into the model's mesh list and set the model's fade and colour state.
void Party::update_flags()
{
    if (f5a04) {
        f5a04 = 0;
        for (unsigned i = 0; i < deferred.size(); i++) {
            vector4 p(0.0f, 0.0f, 0.0f);
            int idx = 0;
            rgl::psys* e = g_emit_defs.spawn(&deferred[i], &p, &idx);
            if (e)
                model->add_emitter(e);
        }
    }
    unsigned def = flags_lo & 0xff;
    int rider = f94;
    rgl::mesh* m2 = 0;
    rgl::mesh* m = ((rgl::mesh*)g_resmgr.get_object(&ICON_DEF(def)->name))->copy_of();
    rows_scale(*(Rot3*)&m->m_cc, vector4(ICON_DEF(def)->scale, ICON_DEF(def)->scale, ICON_DEF(def)->scale));
    GDP(rgl::mesh*, 0x1a200) = m;
    GDP(int, 0x1a1fc) = 0;
    g_script.param1 = id;
    ICON_DEF(def)->frames.update(-0x46);
    if (rider > 0) {
        unsigned rdef = (unsigned)rider;
        m2 = ((rgl::mesh*)g_resmgr.get_object(&ICON_DEF(rdef)->name))->clone();
        m2->translate(&ICON_DEF(def)->pos);
        if (!(flags & 0x400)) {
            rows_scale(*(Rot3*)&m2->m_cc, vector4(ICON_DEF(rdef)->scale, ICON_DEF(rdef)->scale, ICON_DEF(rdef)->scale));
        } else {
            Rot3 dm;
            identity3(&dm);
            // one block-scoped float per call (all four share the slot at 0x1c, above idx; a function-scope float
            // with 9 uses sorts first and takes 0x10)
            { float arg = 1.5707964f; dm.rotate_about_u(&arg); }
            { float arg = -angle; dm.rotate_about_u(&arg); }
            Rot3& rc = *(Rot3*)&m2->m_cc;
            rc = dm;
            rows_scale(rc, vector4(ICON_DEF(rdef)->scale * 1.5f, ICON_DEF(rdef)->scale * 1.5f, ICON_DEF(rdef)->scale * 1.5f));
        }
        GDP(rgl::mesh*, 0x1a200) = m2;
        GDP(int, 0x1a1fc) = 0;
        g_script.param1 = id;
        ICON_DEF(rdef)->frames.update(-0x46);
    }
    if (e59ec != 0) {
        for (unsigned i = 0; i < MODEL_ENTS(model).size(); i++) {
            if (MODEL_ENTS(model)[i] == e59ec) {
                rgl::mesh* old = MODEL_ENTS(model)[i];
                if (old)
                    delete old;
                MODEL_ENTS(model)[i] = m;
            }
        }
    } else {
        model->add_extra(m);
    }
    if (e59f0 != 0) {
        for (unsigned i = 0; i < MODEL_ENTS(model).size(); i++) {
            if (MODEL_ENTS(model)[i] == e59f0) {
                rgl::mesh* old = MODEL_ENTS(model)[i];
                if (old)
                    delete old;
                if (m2) {
                    MODEL_ENTS(model)[i] = m2;
                } else {
                    MODEL_ENTS(model)[i] = MODEL_ENTS(model)[MODEL_ENTS(model).size() - 1];
                    MODEL_ENTS(model).pop_back();
                }
            }
        }
    } else {
        if (m2)
            model->add_extra(m2);
    }
    e59ec = m;
    e59f0 = m2;
    if (ICON_DEF(def)->f10 & 1) {
        model->set_mode(0);
    } else {
        model->set_mode(2);
        { float arg = 1.0f; model->set_fade(&arg); }
        model->f_c0 = ICON_DEF(def)->scale * 3.0f;
        model->f294 = 0.5f;
    }
    unsigned mask = (flags & 0x400) ? 0x1ffff : 0xffff;
    m->mask = mask;
    if (m2)
        m2->mask = mask;
    // The original assigns a (0, 0, 0.5) temporary (3-argument vector4 constructor: w left unset); the w lane it copies
    // (0x4b7e11 [esp+0x2c] -> 0x4b7e24) is never read: the entity's +0xc4 is the box centre, whose consumers use x, y, z.
    model->color = vector4(0.0f, 0.0f, 0.5f);
    { float arg = 0.95f; model->set_color(&arg); }
}

// @ 0x4b7e50
void Party::update_hud_text()
{
    if (dirty) {
        f8ac = march_time(this);
        g_script.trigger_result.store0 = 100;
        g_script.trigger_result.store1 = 0;
        g_game->emit_event(0x1a, id, 0);
        f8b0 = (float)g_script.trigger_result.value * 0.01f;
        dirty = 0;
        f8b5 = 1;
    }
    if (!f8b5)
        return;
    int b = 0, d = 0, c = 0;
    int ammo = 0;
    vector4 t;
    float v;
    g_game->count_units(id, &ammo, &c, &b, &d);
    GameData* g = g_game;
    v = *(float*)((char*)g->factions + f70 * 0x300 + 0x2c);
    unsigned color;
    if (!(g_module_settings.show_faction_color != 0)) {
        color = g->color_of(v, f6c);
    } else {
        t.x = *(float*)((char*)g->factions + f70 * 0x300 + 0x28);
        if (f6c != 0) {
            vector4 tmp = rgl::unpack_color((unsigned*)&t.x);
            vector4* p = &tmp;
            t.x = p->x * 1.2f;
            t.y = p->y * 1.2f;
            t.z = p->z * 1.2f;
            color = rgl::pack_color(&t) | 0xff000000;
        } else {
            color = *(unsigned*)&t.x;
        }
    }
    if (id == g->f78) {
        color = 0xffffffff;
    } else if ((flags_lo & 0x40000) != 0) {
        color = 0xffaa22dd;
        if (!(v < -0.01f))
            color = 0xff22aaff;
    }
    char buf[128];
    if (!(b == 0)) { sprintf(buf, "%d+%d", ammo, b); } else { sprintf(buf, "%d", ammo); }
    float ct = *(float*)&flags_hi;
    if ((flags_lo & 0x80000) != 0) {
        ct = 1.0f;
        tstring str("");
        g_game->label_text(label, &str, 0, 1, &ct);
    } else if ((flags_lo & 0x400) != 0) {
        ct = this->scale();
        g_game->label_text(label, display_name(), 1, color, &ct);
    } else {
        ct = this->scale();
        tstring str(buf);
        g_game->label_text(label, &str, 0, color, &ct);
    }
    f8b5 = 0;
}


// ---- the party's per-tick update ---------------------------------------------------------------------------------------------

// header inlines of the engine's vectors that cl left out of line in the original (bodies visible here, so a local whose
// address only goes to them can share its frame slot; the copies live at 0x4367c0 and 0x407d60)
namespace rgl {
inline float vector4::normalize()
{
    float len = length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv;
    } else {
        x = 0.0f; y = 1.0f; z = 0.0f;
    }
    return len;
}
}
// the morph-time setter (0x433190, rgl/xmlcfg/agent_pick.cpp): out of line, but its body was visible to the original's callers
struct SetTimeKeys { void* frames; int* times; int count; };
inline void mesh_node::set_time(const float* t)
{
    rgl::mesh* m = (rgl::mesh*)this;
    SetTimeKeys& k = m->local_a1 ? *(SetTimeKeys*)&m->kf : *(SetTimeKeys*)&m->parent->get_kf();
    if (k.count > 0) {
        if (*t > (float)k.times[k.count - 1]) m->c8 = (float)k.times[k.count - 1];
        else m->c8 = *t;
    }
}
// helpers of Party::tick (0x4b80d0)
struct IconRow { char pad0[0x28]; int sound; char pad2c[0x48 - 0x2c]; };
static inline float plane_dist(const vec2& a, const rgl::vector4& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dy * dy + dx * dx);
}

static __forceinline rgl::vector4 v4_half(const rgl::vector4& v) { return rgl::vector4(v.x * 0.5f, v.y * 0.5f, v.z * 0.5f); }
static __forceinline rgl::vector4 v4_add(const rgl::vector4& a, const rgl::vector4& b) { return rgl::vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline NavRec& nav_cell(int i) { return (*(std::vector<NavRec>*)&((rgl::world*)g_script.level)->ground->cells)[i]; }
struct GDTimer { char pad[0x13a90]; Timer t; };
static inline rgl::vector4 v2_to3d(const vec2& v) { return rgl::vector4(v.x, v.y, 0.0f); }
static __forceinline bool mesh_used(const rgl::mesh* m)
{
    if (m->parent)
        return m->parent->kf.count > 0;
    return m->kf.count > 0;
}
static inline Vec2 v2_scale(const float& s, const float* v) { return Vec2(s * v[0], s * v[1]); }

// @ 0x4b80d0
// The per-tick update of a map party: periodic re-planning, steering towards the destination, the nav-mesh probe at the
// candidate position, the order state machine (chase a party, reach a site, wait next to a party), the party-event trail on
// large moves and the model refresh (matrix, colour, the two action meshes).
void Party::tick(float dt)
{
    // ---- early outs ---------------------------------------------------------------------------------------
    GameData* g;
    int cur, me0;
    if (!(flags & 0x100) && (g = g_game, me0 = id, cur = g->f78, !(me0 == cur && g->f13ad0f > 0.0f && g->f13ad4 != 0)) && f8c8 < 0) {
        // ---- periodic re-planning (never for the camera party) --------------------------------------------
        if (!(flags & 0x400)) {
            if (me0 != cur) {
                Timer& t = clk8d0;
                if (t.elapsed() >= interval) {
                    f598c = 0;
                    think();
                    t.restamp();
                    interval = frand() * 0.15f + 0.25f;
                    if (action == 0xb)
                        interval = interval * 0.1f;
                    t934 = 0.0f;
                }
            }
            if ((float)(g_ticks[idx938] - t940) * 1e-05f > t934) {
                plan_order();
                clk948.set_delay(0.1f);
                clk938.restamp();
                t934 = (frand() + 1.0f) * 0.1f;
                if (f597c < 5.0f)
                    f597c = f597c + 0.5f;
            }
            if ((float)(g_ticks[clk948.clock] - clk948.start) * 1e-05f > 0.05f || id == g_game->f78) {
                update_nav();
                clk948.restamp();
            }
            if (moving == 0 && kind == 2) {
                int oid = tgt_id;
                PartyMgr* m = &g_game->parties;
                if (m->is_live(oid)) {
                    Party* o = m->get(oid);
                    dest_x = o->pos.x;
                    dest_y = o->pos.y;
                }
            }
        }

        // ---- movement -------------------------------------------------------------------------------------
        model->visible = 1;
        if (action == 8) {
            model->visible = 0;
            return;
        }
        update_hud_text();

        rgl::vector4& p = *(rgl::vector4*)&pos;
        bool moved;
        {
        vec2 dir;
        dir.x = dest_x - p.x;
        dir.y = dest_y - p.y;
        float dist = dir.normalize();
        moved = false;
        float speed = speed_factor();
        float step = speed * dt;

        if (!(moving == 0 && kind == 2)) {
            if (dist < step)
                step = dist;
        } else {
            if (dist - radius * 1.2f < step) {
                step = dist - radius * 1.2f;
                if (step < 0.0f)
                    step = 0.0f;
                dist = dist - radius * 1.2f;
                if (dist < 0.0f)
                    goto probe;
            }
        }

        if (dist > 0.0f) {
            vec2& hd = *(vec2*)&dirx;
            hd = dir;
            dir.x = dirx - oldx;
            dir.y = diry - oldy;
            dir.normalize();
            float want = (float)atan2(-dir.x, dir.y);
            if (angle > 6.2831855f)
                angle = (float)fmod(angle, 6.2831855f);
            else if (angle < 0.0f)
                angle = 6.2831855f - (float)fmod(-angle, 6.2831855f);
            if (want > 6.2831855f)
                want = want - 6.2831855f;
            else if (want < 0.0f)
                want = want + 6.2831855f;
            float diff = want - angle;
            if (diff > 3.1415927f)
                diff = -(6.2831855f - diff);
            if (diff < -3.1415927f)
                diff = diff + 6.2831855f;
            float k = dt * 50.0f;
            if (k > 1.0f)
                k = 1.0f;
            moved = true;
            angle = k * diff + angle;
        }

probe:
        // ---- nav-mesh probe at the candidate position -----------------------------------------------------
        {
            Vec2 ds = v2_scale(step, &dirx);
            Vec2 next(ds.x + p.x, ds.y + p.y);
            *(Vec2*)&dir = next;
            face* node = g_game->find_at(&dir.x);
            if (node != 0) {
                unsigned mask = nav_cell(node->m4).mask();
                bool ok = ((rgl::world*)g_script.level)->ground->shares(node->m4, mesh->m4);
                if (flags & 0x200) {
                    if ((char)terrain_is_road(mask)) {
                        ok = true;
                    } else if (mask != 0xf && id == g_game->f78 && ((GDTimer*)g_game)->t.elapsed() > 0.05f) {
                        rgl::vector4 ev = v2_to3d(dir);
                        g_script.pos[0].o = ev;
                        g_game->emit_event(0x14, 0, 0);
                        ((GDTimer*)g_game)->t.restamp();
                        kind = 0;
                        moving = 0;
                        f5988 = 0;
                    }
                }
                if (ok) {
                    *(Vec2*)&p = next;
                    mesh = node;
                    map_mask = nav_cell(node->m4).mask();
                    pos.z = node->plane_z(&pos.x);
                } else {
                    f597c = 0;
                    if (f598c == 0)
                        f5988 = 1;
                }
            }
        }

        }

        // ---- action state machine -------------------------------------------------------------------------
        {
            int act = action;
            int t;
            PartyMgr* m;
            if (action == 4 && (t = target) >= 0 && (m = &g_game->parties)->is_live(t) &&
                !party_flag(m->get(t), 0x100) &&
                m->get(t)->action != 8 && dt > 0.0f) {
                vec2 o = *(vec2*)&m->get(t)->pos;
                float d = plane_dist(o, p);
                if (d < m->get(t)->radius + radius) {
                    int him = t;
                    int me = id;
                    if (me == g_game->f78 && !party_flag(m->get(t), 0x400) && m->get(t)->f88 < 0 &&
                        m->get(t)->action == 4 && m->get(t)->target == me) {
                        him = me;
                    } else {
                        t = me;
                    }
                    g_game->deploy_party(t, him);
                    Party* o4 = g_game->parties.get(target);
                    o4->kind = 0;
                    o4->moving = 0;
                    o4->f5988 = 0;
                    kind = 0;
                    moving = 0;
                    f5988 = 0;
                }
                goto acted;
            }
            if (act == 9) {
                t = target;
                vec2 o = *(vec2*)&battle_page(t)->x2;
                float d = plane_dist(o, p);
                if (d < *(float*)((char*)(*(std::vector<char*>*)((char*)g_game + 0xe0))[(unsigned)t >> 4] + (t & 0xf) * 0x98 + 0x40) + radius) {
                    BattleRec* n = ((BattleTable*)((char*)g_game + 0xcc))->get(t);
                    p.x = n->x1;
                    p.y = n->y1;
                    pos.z = ((BattleTable*)((char*)g_game + 0xcc))->get(t)->z;
                    mesh = ((BattleTable*)((char*)g_game + 0xcc))->get(t)->face1;
                    flags |= 0x200ui64;
                    ((BattleTable*)((char*)g_game + 0xcc))->get(t)->detach();
                    action = 0;
                    kind = 0;
                    moving = 0;
                    f5988 = 0;
                    update_flags();
                }
                goto acted;
            }
            if (act == 1) {
                if ((t = target) >= 0 && (m = &g_game->parties)->is_live(t)) {
                    vec2 o = *(vec2*)&m->get(t)->pos;
                    float d = plane_dist(o, p);
                    if (d < m->get(t)->radius + radius) {
                        if (id == g_game->f78) {
                            action = 8;
                            kind = 0;
                            moving = 0;
                            f5988 = 0;
                        } else {
                            action = 8;
                            kind = 0;
                            moving = 0;
                            f5988 = 0;
                        }
                    }
                }
            }
        }

acted:
        // ---- party-event trail on large moves -------------------------------------------------------------
        {
            rgl::vector4 delta;
            delta.x = p.x - trail.x;
            delta.y = p.y - trail.y;
            delta.z = p.z - trail.z;
            if (delta.normalize() > trail_len && !(flags & 0x200) && map_mask != 7) {
                rgl::vector4 mid = v4_add(v4_half(p), v4_half(*(rgl::vector4*)&trail));
                float ang = (float)atan2(-delta.x, delta.y);
                g_game->push_party_event(id, (const int*)&mid, &ang);
                float r1 = frand();
                float rs = frand() + r1;
                *(rgl::vector4*)&trail = p;
                trail_len = rs * 0.7f + 0.5f;
            }
        }

        // ---- model refresh --------------------------------------------------------------------------------
        {
            Rot3 m;
            model->basis = *identity3(&m);
        }
        model->basis.rotate_about_u(&angle);
        model->origin = p;

        unsigned color;
        if (id == g_game->f78) {
            if (flags & 0x1000000)
                color = 0x88ffffffu;
            else
                color = 0xffffffffu;
        } else {
            rgl::vector4 c(1.0f, 1.0f, 1.0f);
            c.w = alpha;
            color = rgl::pack_color(&c);
            model->visible = alpha > 0.01f ? 1 : 0;
            model->f294 = alpha * 0.5f;
        }
        for (unsigned i = 0; i < MODEL_ENTS(model).size(); i++) {
            if (MODEL_ENTS(model)[i] != 0)
                MODEL_ENTS(model)[i]->color = color;
        }
        if (!model->visible)
            return;
        if (e59ec != 0)
            *(int*)((char*)e59ec + 0x114) = 0;
        if (e59f0 != 0)
            *(int*)((char*)e59f0 + 0x114) = 0;

        // ---- the party's first action mesh ----------------------------------------------------------------
        rgl::mesh* e1 = e59ec;
        float tm;
        if (!moved) {
            if (e1 != 0 && e1->in_use()) {
                tm = 1.0f;
                ((mesh_node*)e1)->set_time(&tm);
            }
        } else {
            if (e1 != 0 && e1->in_use()) {
                tm = (float)fmod((double)(((float)g_ticks[0] * 1e-05f + (float)id) * 2.3f), 1.0) * 40.0f + 100.0f;
                ((mesh_node*)e1)->set_time(&tm);
                *(int*)((char*)e59ec + 0x114) = 1;
                Timer& t = clk908;
                if (t.elapsed() > 0.5f && dt > 0.0f) {
                    t.restamp();
                    int snd = ((IconRow*)g_map_icons)[flags_lo & 0xff].sound;
                    if (snd != 0)
                        g_sound_mgr.play(snd, 0, (const float*)&model->origin, 1.0f, 0);
                }
            }
        }

        // ---- the party's second action mesh ---------------------------------------------------------------
        rgl::mesh* e2 = e59f0;
        if (e2 == 0 || !mesh_used(e2))
            return;
        float sx = *(float*)((char*)g_script.level + 0x31c) - p.x;
        float sy = *(float*)((char*)g_script.level + 0x320) - p.y;
        float sz = *(float*)((char*)g_script.level + 0x324) - p.z;
        float dd = sz * sz + sy * sy + sx * sx;      // 0x4b8ea4: z squared first, then y, then x (float sums are order-sensitive)
        if (!(dd < 2000.0f))
            return;
        tm = (float)fmod((double)((float)id * 0.05f + (float)g_ticks[0] * 1e-05f), 1.0) * 40.0f + 100.0f;
        ((mesh_node*)e2)->set_time(&tm);
        *(int*)((char*)e59f0 + 0x114) = 1;
        return;
    }
    model->visible = 0;
}

// ==== END ====

