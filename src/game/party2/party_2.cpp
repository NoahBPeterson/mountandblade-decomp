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
#include "./party_2.h"
#include "../../include/crt.h"      // canonical CRT declarations (listed directly: the resolution audit reads only the source's own includes)
#include "../game_globals.h"      // canonical g_seeing_range
#include "../terrain_b/terrain_b.h"
#include "../terrain_b/cell_grid.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../../rgl/mesh.h"
#include "../config/widget_mgr.h"
#include "../battle_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/sound_mgr.h"      // (listed directly: the resolution audit scans only the source's own includes)

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

    TabWorld* o = g_tableaus.push();
    TabRec* rec = (TabRec*)((char*)g_tableaus.recs + m->rec * 0x68);

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
    g_5a75f60 = 0;
    o->fr.mat4[0] = 1.0f;
    o->fr.mat4[1] = 1.0f;
    o->fr.mat4[2] = 1.0f;
    o->fr.mat4[3] = 1.0f;
    o->f344 = 0;
    o->f348 = 0;
    ((rgl::world*)o)->reset();

    o->fr.perspective = 0;
    o->fr.nplanes = 0;
    o->fr.nearz = 0.0f;
    o->fr.farz = 400.0f;
    o->fr.left = -0.5f;
    o->fr.right = 0.5f;
    o->fr.bottom = -0.375f;
    o->fr.top = 0.375f;
    o->fr.build_planes();

    char ident[0x40];
    rgl::identity_pos((rgl::matrix*)ident);
    ((vector4*)(ident + 0x30))->x = 0.0f;
    ((vector4*)(ident + 0x30))->y = 0.0f;
    ((vector4*)(ident + 0x30))->z = 400.0f;
    *(vector4*)(ident + 0x30) = *(vector4*)(ident + 0x30);
    o->fr.m = *(rgl::matrix*)ident;
    o->fr.build_planes();

    __int64 v = *(__int64*)((char*)m + 0x60);
    int flag = 1;
    ((DialogBlock*)rec->holder)->emit(-0x41, 0, &flag, &v, -1);

    sc.fr = o->fr;
    sc.f190 = o->fr.flag;
    sc.f1b0 = &o->view;
    sc.f160 = 1;
    sc.list = *(rgl::ptr_list*)&o->winds;
    o->amb = *(vector4*)o->fr.mat4;
    o->amb.w = 1.0f;

    sc.target = (texture*)m->target;
    o->step(0.0f);
    if (g_5a75f60 != 0)
        sc.f160 = 1;

    for (;;) {
        o->render_scene(&sc, false);
        int h = rec->h;
        if (h < 0x201)
            h = 0x200;
        int w = rec->w;
        if (w < 0x201)
            w = 0x200;
        texture* t = g_resmgr_tab.get_texture(w, h, 0x50);
        if (t != 0)
            sc.f15c = (int)t;
        bool ok = rgl::g_scene_registry.render(&sc);
        if (t != 0)
            g_resmgr_tab.release(t);
        if (ok)
            break;
    }

    m->target->done = 1;
    g_tableaus.pop_back_delete();
}

// @ 0x4b14b0
void TabTable::add_spawned(const int* tableau, const int* instance)
{
    purge_all();
    TabRec* e = recs + *tableau;
    TabSpawn* r = e->find_or_create(instance);
    r->rec = *tableau;
    r->step = scene_step;
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
        TabRec* r = g_tableaus.recs + rec;
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
        r = g_tableaus.recs + rec;
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
        hi[0] = 0.95f * a;
        hi[1] = 0.65f * a;
        hi[2] = 0.65f * a;
        float u = -t;
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
extern float g_k6_9;                                // 0x5935d0
extern float g_k0_04;                               // 0x54ab1c
extern float g_k0_006;                              // 0x5935cc
extern float g_k30;                                 // 0x54b658
extern float g_k0_01;                               // 0x54a6f0
extern float g_k15;                                 // 0x590da0
extern float g_k0_7;                                // 0x54b3c8
extern float g_k0_6;                                // 0x590d94
extern float g_k1_2;                                // 0x590b94
extern float g_k1_25;                               // 0x590c80
extern float g_k1_05;                               // 0x590f24
extern float g_k1;                                  // 0x549cdc
extern float g_k0;                                  // 0x549ca8
extern float g_k3;                                  // 0x54a710
extern float g_k0_02;                               // 0x590cd0
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
    float& acc = *(float*)&stacks[idx].f18;
    Troop* chr = &chars[slot.troop];
    if (chr->flags30 & 0x10) {
        chr->exp += amount;
        return;
    }
    if (chr->f50 == 0) {
        return;
    }
    acc += (float)amount;
    if (acc >= (float)(int)(g_level_exp[chr->level] * g_k0_006 + g_k30)) {
        while (slot.f14 < slot.count) {
            slot.f14++;
            acc -= (float)(int)(g_level_exp[chr->level] * g_k0_006 + g_k30);
            if ((float)(int)(g_level_exp[chr->level] * g_k0_006 + g_k30) >= acc) {
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
    for (int j = 0; j < keep; j++) {
        if (g_game->troops[stacks[j].troop].flags30 & 0x10) {
            if (dst != j) {
                rotate_stacks(dst, j);
            }
            dst++;
        }
    }
    for (int j = keep; j < nstacks; j++) {
        if (g_game->troops[stacks[j].troop].flags30 & 0x10) {
            if (keep != j) {
                rotate_stacks(keep, j);
            }
            keep++;
        }
    }
}
// @ 0x4b1fa0
float Party::strength()
{
    if (f88 != -1) {
        return g_k1;
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
    float v = ((float)a * g_k3 + (float)b) * g_k0_02 + g_k0_4;
    if (v < g_k0) {
        return g_k0;
    }
    if (v >= g_k1) {
        return g_k1;
    }
    return v;
}

// @ 0x4b20b0
float Party::speed_factor()
{
    if (*(unsigned __int64*)&flags_lo & 0x200) {
        return g_k15;
    }
    float v = f8b0 * f8ac;
    int type = map_mask;
    if (type == 0xb || type == 0xc || type == 0xa || type == 0xd) {
        v = g_k0_7 * v;
    }
    if (g_game->is_night()) {
        return g_k0_6 * v;
    }
    return v;
}

// @ 0x4b2120
void Party::set_speed_from_stat()
{
    (*(__int64*)((char*)&g_script + 0x18)) = 100;
    g_game->emit_event(0x1a, id, 0);
    f8b0 = (float)(*(__int64*)((char*)&g_script + 0x18)) * g_k0_01;
}

// @ 0x4b2160
void __stdcall copy_item(void* dst, void* src)
{
    if (dst) {
        E92 tmp = *(E92*)src;
        *(E92*)dst = tmp;
    }
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
    __CxxThrowException(0, 0);
}

// @ 0x4b21f0
float Troop::food_factor()
{
    unsigned __int64 f = flags30;
    if (f & 0x10) {
        if (f4e4 >= 0) {
            return (nibble(0x18) * g_k0_04 + g_k1_25) * g_k6_9;
        }
        return (nibble(0x19) * g_k0_04 + g_k1_05) * g_k6_9;
    }
    if (f & 0x400) {
        return (nibble(0x18) * g_k0_04 + g_k1_2) * g_k6_9;
    }
    return (nibble(0x19) * g_k0_04 + g_k1) * g_k6_9;
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
        return (int)(*(__int64*)((char*)&g_script + 0x18));
    }
    int item = stacks[0].troop;
    int bonus = g_game->troops[item].f64;
    return g_game->skill_level(item, 1) * g_skill_leadership_bonus + g_base_companion_limit + bonus;
}

// @ 0x4b2330
int Party::max_prisoners()
{
    if (g_game->emit_event(0x12, id, 0)) {
        return (int)(*(__int64*)((char*)&g_script + 0x18));
    }
    return g_game->skill_level(stacks[0].troop, 2) * g_skill_prisoner_management_bonus;
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
            if (!(g_game->troops[item].hp >= g_player_wounded_treshold)) {
                continue;
            }
        } else {
            if (!(g_game->troops[item].hp >= g_hero_wounded_treshold)) {
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
extern float g_gk3;                                  // 0x54a710  3.0f
extern float g_gk0_05a;                              // 0x549d74
extern float g_gk0_05b;                              // 0x549d7c
extern float g_gk0_03;                               // 0x549d70

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
    __CxxThrowException(0, 0);
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
    float v = (skill_level(0xd) * g_gk0_05b + 1.0f) * g_seeing_range;
    if (g_game->is_night()) {
        return g_gk0_03 * v;
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
    __CxxThrowException(0, 0);
    __assume(0);
}

// @ 0x4b26e0
OpenItem* ElemCopier::fill_n_elems(OpenItem* dst, unsigned count, OpenItem* val)
{
    ptr_tag tag;   // uninitialised: pushes whatever eax holds (val)
    construct_elems2(dst, count, val, this, tag);
    return dst + count;
}

// @ 0x4b2710
// std::push_heap tail: sift up the last element
void push_back_elem(OpenItem* first, OpenItem* last, HeapGt comp)
{
    if (first != last) {
        OpenItem* l = last - 1;
        int n = (int)(l - first);
        if (n > 0) {
            push_heap_elem(first, n, 0, *l, comp);
        }
    }
}

// Re-seat a battle record on the world map: face under the first position, its height above the face's plane, the clock
// stamp, and (type 2) a sanity check of the terrain masks under both positions.
// @ 0x4b2770
void BattleRec::bind_host()
{
    face1 = g_game->find_at(&x1);
    z = face1->w[0].z - ((x1 - face1->w[0].x) * face1->normal.x + (y1 - face1->w[0].y) * face1->normal.y) / face1->normal.z;
    int clk = born.clock;
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
        if (sqrtf(dx * dx + dy * dy + dz * dz) > g_gk3) {
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
    if (ex * ex + ey * ey > g_gk0_05a) {
        moving = 0;
        if (d.z * d.z + d.y * d.y + d.x * d.x > g_gk0_05a) {
            f5988 = 1;
        }
        PartyV4& rp = party_of(&g_game->parties, tgt_id)->pos;
        f5990 = rp.x;
        f5994 = rp.y;
    }
}


// ---- the per-frame party stats --------------------------------------------------------------------------------------------
extern float g_k0_1;                                 // 0x54b8b0  0.1f
extern float g_rand_k;                               // 0x54a6bc  1/15817
extern float g_k_item_weight;                        // 0x593640  per-item weight of the equipped grid item
struct options;
extern options g_options;                   // 0x5aa008
#define g_clamp_max (*(int*)0x8ee3ac4)               // outside the annotation range, so a literal cast

// @ 0x4b2c00
void Party::tick_stats()
{
    float speed = (float)skill_level(0xb) * 0.2f + 1.0f;
    if (id == g_game->f78 && GDP(float, 0x13ad0) > 0.0f && GDP(int, 0x13ad4) != 0) {
        speed = speed + speed;
    } else if (f88 > 0) {
        speed = speed * 0.0f;
    } else if (f8c8 >= 0 && party_of(&g_game->parties, f8c8)->f88 > 0) {
        speed = speed * 0.0f;
    }

    // food growth / stack decay over the party's troop stacks
    for (int i = 0; i < nstacks; i++) {
        int troop = stacks[i].troop;
        Troop* ch = &g_game->troops[troop];
        if (ch->flags30 & 0x10) {
            int level = ((unsigned char*)ch)[0x7a] & 0xf;
            if (level < 0) {
                level = 0;
            } else if (level > g_clamp_max) {
                level = g_clamp_max;
            }
            float limit = (float)(ch->lvl58 + level * 2 + 0x23);
            if (ch->hp < limit) {
                float add = ((float)(rand() % 15817) * g_rand_k + 0.5f) * 0.5f * speed;
                if (troop == g_game->player) {
                    add = add * (3 - (*(int*)((char*)&g_options + 0x4c)));
                }
                ch->hp = add + ch->hp;
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
        }
    }

    if (id != g_game->f78) {
        return;
    }

    // main party: zoom easing, the first record's item slot decay, morale
    if (g_disable_food_slot == 0) {
        if (GDP(float, 0x13ad0) > 0.0f && GDP(int, 0x13ad4) != 0) {
            zoom = zoom - 0.03f;
            if (zoom < 0.0f) {
                zoom = 0.0f;
            } else if (zoom > 1.0f) {
                zoom = 1.0f;
            }
        } else {
            ItemRef* slot = &g_game->troops->equip[9];
            if (slot->item < 0) {
                zoom = zoom + 0.03f;
            } else {
                float chance = (float)sum_stat(0x10) * 0.02f;
                if (zoom > 0.1f) {
                    chance = chance * 3.0f;
                }
                while ((float)(rand() % 15817) * g_rand_k < chance) {
                    if (slot->item < 0) {
                        break;
                    }
                    int count = (slot->m4 & 0xff) + 1;
                    int cap = g_items[slot->item].f210;
                    if (cap != 0 && (slot->m4 & 0xff000000) == 0x2a000000) {
                        float x = (float)cap * 0.13f;
                        double r = floor(x);
                        int n = (int)r;
                        if (x - r > 0.5f) {
                            n = n + 1;
                        }
                        if (n < 1) {
                            n = 1;
                        }
                        cap += n;
                    }
                    if (count > cap) {
                        count = slot->max_amount();
                    }
                    *(unsigned char*)&slot->m4 = 0;
                    slot->m4 = slot->m4 | count;
                    int cap2 = 0;
                    if (slot->item >= 0) {
                        int k2 = g_items[slot->item].f210;
                        cap2 = k2;
                        if (k2 != 0 && (slot->m4 & 0xff000000) == 0x2a000000) {
                            float x2 = (float)k2 * 0.13f;
                            double r2 = floor(x2);
                            int m = (int)r2;
                            if (x2 - r2 > 0.5f) {
                                m = m + 1;
                            }
                            if (m < 1) {
                                m = 1;
                            }
                            cap2 += m;
                        }
                    }
                    if (cap2 - (int)(slot->m4 & 0xff) <= 0) {
                        slot->item = -1;
                    }
                    chance = chance - 1.0f;
                }
                zoom = zoom - 0.03f;
                if (zoom > 0.1f) {
                    zoom = zoom - 0.1f;
                }
            }
            if (zoom > 0.1f) {
                f7c = f7c - 0.002f;
            }
            int item = g_game->troops->equip[9].item;
            int value = 0;
            if (item >= 0) {
                value = g_items[item].f1fc;
            }
            f7c = (float)value * g_k_item_weight + f7c;
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
    }

    // the player's item grid decay
    Troop* pc = &g_game->troops[g_game->player];
    float grid_chance = (float)party_of(&g_game->parties, g_game->f78)->skill_level(0xb) * 0.001f;
    if (!(GDP(float, 0x13ad0) > 0.0f)) {
        grid_chance = grid_chance * 1.5f;
    }
    for (int i = 0; i < pc->inv_count(); i++) {
        ItemRef* g = &pc->inv[i];
        if (g->item >= 0
            && *(unsigned char*)&g_items[g->item].type_flags == 1
            && (g->m4 & 0xff000000) == 0x1e000000
            && (float)(rand() % 15817) * g_rand_k < grid_chance) {
            *((unsigned char*)g + 7) = 0;
            g->reduce(0);
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
        ((rgl::world*)g_level)->remove_entity(((int*)model)[2]);
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
    __CxxThrowException(0, 0);
    __assume(0);
}

// @ 0x4b33c0
// Start a new search generation of the world map's formation records (NavData::m64 counts the searches): when the counter
// gets large, clear every record's stamp (+0x6c) and restart it; then advance it.
void NavData::reset()
{
    if (m64 > 1000000000) {
        int cnt = (int)((unsigned)cells.last - (unsigned)cells.first) / 0x88;
        if ((unsigned)cells.first != 0) {
            for (unsigned i = 0; i < (unsigned)cnt; i++) {
                ((NavRec*)cells.first)[i].gen() = 0;
            }
        }
        m64 = 0;
    }
    m64 += 3;
}


// ---- the world-map walk test and the event record frame -------------------------------------------------------------------

static __forceinline vector4 crossv(const vector4& a, const vector4& b)
{
    return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

#define RECS ((NavRec*)cells.first)

// @ 0x4b3420
// Walk the formation-record graph from `from` toward `to`, testing every record edge for a crossing of the move segment
// (0.01 <= t <= 0.99, 0 <= s <= 1), honouring the order-target mask and the per-edge keep distance.
bool NavData::can_move(int from, float x, float y, int to, const float* p, unsigned flags, int max)
{
    if (!((RiderTable*)this)->shares(from, to))
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
    int hops = 0;
    int prev = -1;
    bool stop = false;
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
        r->pos.s = crossv(r->pos.f, r->pos.u);
        r->pos.s.normalize();
        r->pos.f = crossv(r->pos.u, r->pos.s);
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
    float radius = ((float)skill_level(0xd) * 0.1f + 1.0f) * g_seeing_range;
    if (g_game->is_night())
        radius = radius * 0.5f;
    unsigned id = (unsigned)parties()->first_party_id();
    while (id < (unsigned)parties()->deflt) {
        if (*count >= *max)
            return;
        int idx = ((Map10007*)parties())->lookup((unsigned)id);
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
            if (len >= strength() * radius)
                goto next;
        }
        (*(int*)((char*)&g_script + 0x18)) = 1;
        (*(int*)((char*)&g_script + 0x1c)) = 0;
        g_game->emit_event(0x19, this->id, (int)id);
        if (((*(int*)((char*)&g_script + 0x1c)) | (*(int*)((char*)&g_script + 0x18))) != 0)
            out[(*count)++] = (int)id;
    next:
        parties()->next_party_id(&id);
    }
}

// @ 0x4b3bc0
void Party::reissue_order()
{
    if (action == 9) {
        set_order_face((int*)&battles()->pages[target >> 4][target & 0xf].x2,
                       battles()->pages[target >> 4][target & 0xf].face2);
        order_state = 0;
    } else if (action == 6) {
        set_order_pos((int*)&order_pos);
        order_state = 0;
    } else if (action == 4 || action == 1) {
        if (target >= 0 && parties()->is_live(target)) {
            if (parties()->get(target)->flags_lo & 0x400)
                set_order_face((int*)&parties()->get(target)->pos,
                               parties()->get(target)->mesh);
            else
                set_order_party(&target);
        }
        order_state = 0;
    }
}


// ---- the map-order planner ----------------------------------------------------------------------------------------------
int __cdecl terrain_is_road(unsigned kind);              // 0x489390 (is-farm/road terrain kind)
int __cdecl terrain_is_water(unsigned kind);             // 0x4893b0 (is-water terrain kind)

// @ 0x4b3ce0
// Act on the current map order: finish a move, chase a party or an agent, or sample a random walkable spot around the
// current position and hand it to set_order_face as a new order.
void Party::plan_order()
{
    if (action == 0) {
        goto done;
    }
    if (action == 7) {
        goto done;
    }
    if (action == 8) {
    done:
        kind = 0;
        moving = 0;
        f5988 = 0;
        return;
    }

    if (action == 2 || action == 3) {
        f900 = ((int*)g_ticks)[f8f8 * 2];
        f904 = ((int*)g_ticks)[f8f8 * 2 + 1];
        vec2r to;
        if (action == 3 && target >= 0 && parties()->is_live(target)) {
            Party* p = parties()->get(target);
            to.x = p->pos.x;
            to.y = p->pos.y;
        } else {
            to.x = order_pos.x;
            to.y = order_pos.y;
        }
        bool give_up = false;
        int tries = 0;
        float spread = 0.6f;
        vec2r p;
        for (;;) {
            int r = rand() % 15817;
            float ang = ((float)r * 6.322312e-05f - 0.5f) * spread * 3.1415927f;
            float c = cosf(ang);
            float s = sinf(ang);
            r = rand();
            float len = speed_factor() * ((float)r * 0.00012644623f + 1.0f);
            p.x = (c * dirx - s * diry) * len + pos.x;
            p.y = (s * dirx + c * diry) * len + pos.y;
            r = rand();
            float k = (float)r / f8f4 * 6.322312e-05f;
            p.x = (to.x - pos.x) * k + p.x;
            p.y = (to.y - pos.y) * k + p.y;
            face* a = g_game->find_at(&p.x);
            tries++;
            spread += 0.4f;
            if (tries > 6) {
                kind = 0;
                moving = 0;
                f5988 = 0;
                give_up = true;
            }
            if (a) {
                NavData* w = g_level->core;
                if (((RiderTable*)w)->shares(a->m4, mesh->m4)) {
                    unsigned k2 = nav_recs()[a->m4].mask();
                    if (flags_lo & 0x200) {
                        if ((char)terrain_is_road(k2)) {
                            set_order_face((int*)&p, a);
                            return;
                        }
                    } else if ((char)terrain_is_water(k2)) {
                        set_order_face((int*)&p, a);
                        return;
                    }
                }
            }
            if (give_up)
                return;
        }
    }

    if (action == 6) {
        float dx = order_pos.x - pos.x;
        float dy = order_pos.y - pos.y;
        if (sqrtf(dx * dx + dy * dy) < 0.01f) {
            action = 0;
            return;
        }
    }
    if (action == 9) {
        return;
    }

    int t = target;
    if (t >= 0) {
        if (action == 4 || action == 1) {
            if (parties()->is_live(t)) {
                if (parties()->get(t)->flags_lo & 0x400)
                    return;
                set_order_party(&target);
                return;
            }
            action = 0;
            return;
        }
        if (action == 10) {
            if (parties()->is_live(t)) {
                set_order_agent(&target);
                return;
            }
            action = 0;
            return;
        }
    }
    if (action == 5 || action == 11) {
        if (parties()->is_live(t)) {
            goto chase;
        }
        action = 0;
        return;
    }
    return;

chase:
    vec2r dir;
    dir.x = 0.0f;
    dir.y = 1.0f;
    dir.rotate_a(angle);
    Party* q = parties()->get(t);
    vec2r d;
    d.x = q->pos.x - pos.x;
    d.y = q->pos.y - pos.y;
    d.normalize();
    vec2r v;
    v.x = dir.x * 0.85f - d.x;
    v.y = dir.y * 0.85f - d.y;
    if (v.x * v.x + v.y * v.y < 1e-13f) {
        v.x = 1.0f;
        v.y = 0.0f;
    }
    v.normalize();

    bool give_up = false;
    int tries = 0;
    float spread = 0.6f;
    vec2r p;
    for (;;) {
        float ang = (frand() - 0.5f) * spread;
        v.rotate_a(ang);
        float len = (interval + interval + 0.5f) * speed_factor();
        len = len + len;
        p.x = v.x * len + pos.x;
        p.y = v.y * len + pos.y;
        face* a = g_game->find_at(&p.x);
        tries++;
        spread += 0.4f;
        if (tries > 6) {
            kind = 0;
            moving = 0;
            f5988 = 0;
            give_up = true;
        }
        if (a) {
            NavData* w = g_level->core;
            if (((RiderTable*)w)->shares(a->m4, mesh->m4)) {
                unsigned k2 = nav_recs()[a->m4].mask();
                if (flags_lo & 0x200) {
                    if (k2 == 7 || (k2 & 7) == 0) {
                        set_order_face((int*)&p, a);
                        return;
                    }
                } else if ((char)terrain_is_water(k2)) {
                    set_order_face((int*)&p, a);
                    return;
                }
            }
        }
        if (give_up)
            return;
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
            d.y = goal_y;
            d.x = goal_x;
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
        float dist = ndx * ndx + ndy * ndy;
        if (dist < lim && g_level->core->can_move(cur, pos.x, pos.y, goal, &d.x, 0, tries))
            cursor = i + 1;
    }
    if (cursor < n) {
        int off = cursor * 0x10;
        int vv = *(int*)((char*)this + 0x196c + off);
        dest_xi = vv;
        dest_yi = *(int*)((char*)this + 0x1970 + off);
        return;
    }
    moving = z;
    f5988 = z;
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
        s->f0c = 0;
        s->f10 = 0;
        s->f14 = 0;
        s->f18 = 0;
        stacks[nstacks].troop = troop;
        stacks[nstacks].count = count;
        stacks[nstacks].f10 = f10;
        stacks[nstacks].f14 = 0;
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
    if ((g_game->troops[troop].flags32 & 0x10) != 0) {
        found = -1;
        for (int i = 0; i < nstacks; i++) {
            if ((g_game->troops[stacks[i].troop].flags32 & 0x10) == 0 && (int)(stacks[i].flags & 1) == cls)
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
    if (id >= parties.n_parties)
        return;
    int idx = ((Map10007*)m)->lookup(id);
    if (idx < 0)
        return;
    Party** pp = &m->pages[(unsigned)idx >> 4];
    if ((*pp)[idx & 0xf].type != 1)
        return;
    int j = ((Map10007*)m)->lookup(id);
    Party** pq = &m->pages[j >> 4];
    Party* p = &(*pq)[j & 0xf];
    for (int i = 0; i < p->nstacks; i++) {
        Stack* st = &p->stacks[i];
        int c = st->count;
        if (st->f14 > c)
            st->f14 = c;
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
    int idx = ((Map10007*)&parties)->lookup(id);
    if (idx < 0)
        return total;
    if (parties.pages[idx >> 4][idx & 0xf].type != 1)
        return total;
    int j = ((Map10007*)&parties)->lookup(id);
    Party* p = &parties.pages[j >> 4][j & 0xf];
    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if ((s.flags & 1) != 0)
            continue;
        Troop* t = &troops[s.troop];
        float lv = (float)t->level + 12.0f;
        if ((t->flags32 & 0x10) != 0) {
            if (this->agent_low(s.troop))
                continue;
            total += lv * lv * 0.01f;
        } else {
            total += (float)(s.count - s.f10) * lv * lv * 0.01f;
        }
    }
    for (int i = 0;; i++) {
        int idx2 = ((Map10007*)&parties)->lookup(id);
        Party* q = &parties.pages[idx2 >> 4][idx2 & 0xf];
        if (q->attached.first == 0 || (unsigned)(q->attached.last - q->attached.first) <= (unsigned)i)
            break;
        int idx3 = ((Map10007*)&parties)->lookup(id);
        int copy = parties.pages[idx3 >> 4][idx3 & 0xf].attached.first[i];
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

// @ 0x4b4a00
// The party's map AI step: gather the nearby parties (0x4b3910), score each one from troop strength, relation and distance,
// then either commit a new order (action / target) or fall back to the pending one (state, +0x8e4).
void Party::think()
{
    if (action == 7 || action == 8)
        return;
    if (f88 != -1)
        return;
    if (flags_lo & 0x400)
        return;

    int ids[32];
    int maxn = 32;
    int n;
    collect_targets(ids, &maxn, &n);

    float base = g_game->party_value(id);
    float hi = base + 0.1f;
    float lo = 0.0f;
    set_speed_from_stat();

    float near_v[32];
    float adj_v[32];
    float rel_v[32];
    float accum[32];
    float score[32];
    int ckind[32];
    char friendly[32];

    if (n > 0) {
        for (int i = 0; i < n; i++) {
            accum[i] = 0.0f;
            adj_v[i] = 0.0f;
            ckind[i] = 0;
            score[i] = 0.0f;
        }
        for (int i = 0; i < n; i++) {
            int tid = ids[i];
            int pi = ((Map10007*)&g_game->parties)->lookup(tid);
            if (pi < 0)
                continue;
            Party* p = (Party*)((char*)g_game->parties.pages[pi >> 4]
                                + (pi & 0xf) * sizeof(Party));
            float rel = g_game->relation(id, tid);
            rel_v[i] = rel;
            float v = g_game->party_value(tid);
            near_v[i] = v;
            adj_v[i] = v;
            friendly[i] = 1;
            int sub = p->f88;
            if (sub >= 0) {
                BattleRec* b = (BattleRec*)((char*)battles()->pages[sub >> 4]
                                            + (sub & 0xf) * 0x98);
                unsigned other = b->party_b;
                if (other == (unsigned)tid)
                    other = (unsigned)b->party_a;
                int oi = -1;
                if (other < g_game->parties.n_parties)
                    oi = ((Map10007*)&g_game->parties)->lookup((int)other);
                if (oi >= 0) {
                    Party* op = (Party*)((char*)g_game->parties.pages[oi >> 4]
                                         + (oi & 0xf) * sizeof(Party));
                    if (op->type == 1) {
                        float ov = g_game->party_value(other);
                        float a = v - ov * 0.9f;
                        if (a < 1.0f)
                            a = 1.0f;
                        adj_v[i] = a;
                        accum[i] = ov + accum[i];
                        friendly[i] = ((float*)g_game->factions)[op->f70 * 0xc0 + f70 + 0xb] >= 0.0f;
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

        float frac = lo / hi;
        float inv = 1.0f / frac;
        unsigned a74 = f74;
        float hi_nib = (float)((int)a74 >> 4 & 0xf);
        float prod = f8b0 * f8ac;
        float prod2 = prod + 0.05f;

        for (int i = 0; i < n; i++) {
            int tid = ids[i];
            int pi = ((Map10007*)&g_game->parties)->lookup(tid);
            if (pi < 0)
                continue;
            Party* p = (Party*)((char*)g_game->parties.pages[pi >> 4]
                                + (pi & 0xf) * sizeof(Party));
            float adj = near_v[i] - accum[i] * 0.9f;
            if (adj < 1.0f)
                adj = 1.0f;
            float mod = (a74 & 0xf) == 0 ? 1.0f : (float)(a74 & 0xf) * 0.125f;
            mod = mod * ((base / adj) * inv);
            if (mod < 0.2f)
                mod = 0.2f;
            else if (mod > 5.0f)
                mod = 5.0f;

            float ax = p->pos.x - pos.x;
            float ay = p->pos.y - pos.y;
            float dist = (float)sqrt(ay * ay + ax * ax);
            if (dist > 0.0f) {
                ax = ax * (1.0f / dist);
                ay = ay * (1.0f / dist);
            } else {
                ax = 0.0f;
                ay = 1.0f;
            }
            float invd = 3.0f / dist;
            if (invd < 0.6f)
                invd = 0.6f;
            else if (invd > 2.0f)
                invd = 2.0f;

            float scale = prod / (p->f8ac * p->f8b0);
            if (scale < 0.5f)
                scale = 0.5f;
            else if (scale > 1.1f)
                scale = 1.1f;

            float sc = (((-rel_v[i] * hi_nib) * scale) * invd) * mod * 0.125f;
            if (rel_v[i] < 0.0f) {
                float s = (a74 & 0x100) ? 1.0f : 0.0f;
                float alt = (((s * p->f78) * scale) * invd) * mod;
                if (sc < alt)
                    sc = alt;
            }
            if (near_v[i] * 3.0f < accum[i]) {
                sc = (near_v[i] / accum[i]) * sc;
                sc = sc + sc;
            }

            float freq = (a74 & 0xf) == 0 ? 1.0f : (float)(a74 & 0xf) * 0.125f;
            if (mod < 1.0f / freq || !friendly[i]) {
                sc = 0.0f;
            } else if ((unsigned)tid == (unsigned)g_game->f78) {
                __int64 d = g_ticks[ignore_timer.clock] - (__int64)((unsigned*)&ignore_timer.start)[0];
                d -= (__int64)((unsigned*)&ignore_timer.start)[1] << 32;
                if ((double)d * 1e-05 < 0.0)
                    sc = 0.0f;
            }

            if (prod2 < p->f8ac * p->f8b0 && sc > 0.01f && p->f88 < 0) {
                float ang = p->angle;
                float s = (float)sin(ang);
                float c = (float)cos(ang);
                if (0.8f < (s * 0.0f + c) * ay + (c * 0.0f - s) * ax)
                    sc = sc * 0.1f;
            }

            if (score[i] < sc) {
                score[i] = sc;
                ckind[i] = 4;
            }

            float alt2 = 0.0f;
            if (rel_v[i] < -0.01f && mod < 1.0f
                && (float)((int)p->f74 >> 4 & 0xf) * 0.125f > 0.0f
                && (p->flags_lo & 0x400) == 0 && p->f88 < 0 && p->f8c8 < 0)
                alt2 = ((1.0f - rel_v[i]) * invd) / (mod * mod) - 1.0f;
            if (score[i] < alt2) {
                score[i] = alt2;
                ckind[i] = 5;
            }
        }
    }

    float best = -10.0f;
    int bi = -1;
    for (int i = 0; i < n; i++) {
        if (best < score[i]) {
            best = score[i];
            bi = i;
        }
    }
    best = best * f958;

    int r = rand();
    float rnd = (float)(r % 0x3dc9) * 3.1611557e-06f + 0.15f;

    if (bi != -1 && !(best < rnd) && !(flags_lo & 0x10000)) {
        action = ckind[bi];
        target = ids[bi];
        reissue_order();
        return;
    }

    bool pending_bad = false;
    if (state == 4) {
        unsigned pid = (unsigned)state_target;
        if (pid < g_game->parties.n_parties) {
            int oi = ((Map10007*)&g_game->parties)->lookup((int)pid);
            if (oi >= 0) {
                Party* pp = (Party*)((char*)g_game->parties.pages[oi >> 4]
                                     + (oi & 0xf) * sizeof(Party));
                if (pp->type == 1) {
                    int oi2 = ((Map10007*)&g_game->parties)->lookup((int)pid);
                    Party* pp2 = (Party*)((char*)g_game->parties.pages[oi2 >> 4]
                                          + (oi2 & 0xf) * sizeof(Party));
                    if (pp2->f8c8 < 0 && (pp2->flags_lo & 0x1080100) == 0) {
                        int oi3 = ((Map10007*)&g_game->parties)->lookup((int)pid);
                        Party* pp3 = (Party*)((char*)g_game->parties.pages[oi3 >> 4]
                                              + (oi3 & 0xf) * sizeof(Party));
                        pending_bad = pp3->action == 8;
                    } else {
                        pending_bad = true;
                    }
                } else {
                    pending_bad = true;
                }
            } else {
                pending_bad = true;
            }
        } else {
            pending_bad = true;
        }
    }

    if (state == 4 && pending_bad) {
        if (bi != -1) {
            action = ckind[bi];
            target = ids[bi];
            reissue_order();
            return;
        }
        action = 2;
        f924 = *(int*)((char*)this + 0x30);
        f928 = *(int*)((char*)this + 0x34);
        reissue_order();
        return;
    }

    action = state;
    target = state_target;
    f924 = state_f8ec;
    f928 = state_f8f0;
    if (action == 2 || action == 3) {
        int q = rand();
        if ((float)(q % 0x3dc9) * 6.322312e-05f < 0.25f) {
            action = 0;
            reissue_order();
            return;
        }
    }
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

    PartyTmp rec;
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
    int idx = ((Map10007*)pm)->lookup(party);
    Party* p = pm->pages._Myfirst[idx >> 4] + (idx & 0xf);
    event_rec r;
    r.p = *(const Pos4i*)pos;
    r.angle = *src;
    int a = 0, b = 0, c = 0, d = 0;
    r.f14 = 0;
    count_units(party, &a, &b, &c, &d);
    EventFlags fl;
    fl.all = 0;
    fl.f.low = (a + c) & 0xff;
    fl.f.high = rand() % 16;
    int troop = p->owner;
    if (troop == -1)
        troop = 0xff;
    fl.f.mid = troop;
    r.flags = fl;
    init_event((E92*)&r);
    GD(std::vector<E92>, 0x13a60).push_back(*(E92*)&r);
}


// ---- battle records: detach, commit, per-frame visuals -------------------------------------------------------------------
struct ScriptState;
extern ScriptState g_script;                // 0xa70440
struct MissionCtx;
extern MissionCtx g_mission_ctx;            // 0xa73300
extern PartyTemplate* g_party_templates;             // 0x5e29f8 (0xb0-byte rows; +0x3c counts the player's kills)

// @ 0x4b6180
// Unwind a battle record: both sides leave their battle state (animation stamp, speed, attached parties), the losses are
// reported, empty parties are removed (counting the kills for the player's side) and the record is released.
void BattleRec::detach()
{
    unsigned list1[128], list2[128];
    int n1, n2;
    int total = 0;

    if (type != 1)
        goto tail;

    g_game->list_members(party_a, list1, (unsigned*)&n1);
    g_game->list_members(party_b, list2, (unsigned*)&n2);

    {
        PartyMgr* pm = &g_game->parties;
        int i, j, k, m;

        if (pm->is_live(party_a)) {
            i = ((Map10007*)pm)->lookup(party_a);
            Party* q = pm->pages[i >> 4] + (i & 15);
            q->action = 0;
            q->interval = 0.4f;
            q->clk8d0.start = g_ticks[q->clk8d0.clock];
            q->f88 = -1;
            total = q->sum_stat(0x10);

            for (k = (q->attached.first ? (int)(q->attached.last - q->attached.first) : 0) - 1; k >= 0; k--) {
                int mid = q->attached.first[k];
                j = ((Map10007*)pm)->lookup(mid);
                if ((pm->pages[j >> 4] + (j & 15))->f8cc) {
                    ((GameW*)g_game)->detach_party(mid);
                    m = ((Map10007*)pm)->lookup(q->attached.first[k]);
                    (pm->pages[m >> 4] + (m & 15))->action = 0;
                }
            }
        }

        {
            unsigned uid = party_b;
            if (uid < g_game->parties.n_parties) {
                j = ((Map10007*)pm)->lookup(uid);
                if (j >= 0 && (pm->pages[(unsigned)j >> 4] + (j & 15))->type == 1) {
                    i = ((Map10007*)pm)->lookup(party_b);
                    Party* q = pm->pages[i >> 4] + (i & 15);
                    q->action = 0;
                    q->interval = 0.1f;
                    q->clk8d0.start = g_ticks[q->clk8d0.clock];
                    q->f88 = -1;

                    m = 0;
                    if (q->nstacks > 0) {
                        for (int t = 0; t < q->nstacks; t++) {
                            int c = q->stacks[t].count;
                            if (((q->stacks[t].flags & 1) == 0) && c >= 0)
                                m += c;
                        }
                    }
                    total += m;

                    for (k = (q->attached.first ? (int)(q->attached.last - q->attached.first) : 0) - 1; k >= 0; k--) {
                        int mid = q->attached.first[k];
                        j = ((Map10007*)pm)->lookup(mid);
                        if ((pm->pages[j >> 4] + (j & 15))->f8cc) {
                            ((GameW*)g_game)->detach_party(mid);
                            m = ((Map10007*)pm)->lookup(q->attached.first[k]);
                            (pm->pages[m >> 4] + (m & 15))->action = 0;
                        }
                    }
                }
            }
        }

        if (total < height) {
            float v[3];
            v[0] = x1;
            v[1] = y1;
            v[2] = z;
            g_game->push_event((const int*)v, height);
        }

        {
            int cur_id = g_game->f78;
            i = ((Map10007*)pm)->lookup(cur_id);
            Party* cur = pm->pages[i >> 4] + (i & 15);
            int st = (*(int*)((char*)&g_mission_ctx + 0x138));
            char is1, is2;

            if (party_a == cur_id || (cur->f88 == rec_id && st == 0))
                is1 = 1;
            else
                is1 = 0;
            if (party_b == cur_id || (cur->f88 == rec_id && st == 1))
                is2 = 1;
            else
                is2 = 0;

            for (i = 0; i < n2; i++) {
                unsigned id = list2[i];
                if (id < g_game->parties.n_parties) {
                    j = ((Map10007*)pm)->lookup(id);
                    if (j >= 0 && (pm->pages[(unsigned)j >> 4] + (j & 15))->type == 1) {
                        j = ((Map10007*)pm)->lookup(id);
                        Party* p = pm->pages[j >> 4] + (j & 15);
                        if ((p->flags_lo & 0x400) == 0) {
                            m = 0;
                            for (int t = 0; t < p->nstacks; t++) {
                                int c = p->stacks[t].count;
                                if (c < 0)
                                    c = 0;
                                m += c;
                            }
                            if (m <= 0) {
                                if (is1 && p->owner >= 0)
                                    g_party_templates[p->owner].num_destroyed_by_player++;
                                ((GameW*)g_game)->remove_party(list2[i]);
                            }
                        }
                    }
                }
            }

            for (i = 0; i < n1; i++) {
                unsigned id = list1[i];
                if (id < g_game->parties.n_parties) {
                    j = ((Map10007*)pm)->lookup(id);
                    if (j >= 0 && (pm->pages[(unsigned)j >> 4] + (j & 15))->type == 1) {
                        j = ((Map10007*)pm)->lookup(id);
                        Party* p = pm->pages[j >> 4] + (j & 15);
                        if ((p->flags_lo & 0x400) == 0) {
                            m = 0;
                            for (int t = 0; t < p->nstacks; t++) {
                                int c = p->stacks[t].count;
                                if (c < 0)
                                    c = 0;
                                m += c;
                            }
                            if (m <= 0) {
                                if (is2 && p->owner >= 0)
                                    g_party_templates[p->owner].num_destroyed_by_player++;
                                ((GameW*)g_game)->remove_party(list1[i]);
                            }
                        }
                    }
                }
            }
        }
    }

tail:
    {
        BattleObj* o = obj;
        ((rgl::world*)g_level)->remove_entity(o->id);
        obj = 0;

        unsigned rid = rec_id;
        char* page = (char*)battles()->pages[rid >> 4];
        *(int*)(page + (rid & 15) * 0x98 + 4) = 0;
        battles()->free_ids.push_back(rid);
    }
}

// @ 0x4b6870
void BattleRec::commit()
{
    unsigned list1[128], list2[128];
    int n1 = 0, n2 = 0;                       // signed counts, zeroed before the calls (jle/jl loops in the original)

    (*(__int64*)((char*)&g_script + 8)) = party_b;
    (*(__int64*)((char*)&g_script + 0x10)) = party_a;
    (*(__int64*)((char*)&g_script + 0x18)) = 0;
    ((TimerList*)((char*)g_game + 0x139f4))->update(-5);
    if ((*(__int64*)((char*)&g_script + 0x18)) == 0)
        g_game->emit_event(2, party_b, party_a);

    g_game->list_members(party_a, list1, (unsigned*)&n1);
    g_game->list_members(party_b, list2, (unsigned*)&n2);

    for (int i = 0; i < n2; i++) {
        unsigned id = list2[i];
        if (id < g_game->parties.n_parties) {
            int j = ((Map10007*)&g_game->parties)->lookup(id);
            if (j >= 0 && (g_game->parties.pages[(unsigned)j >> 4] + (j & 15))->type == 1) {
                g_game->refresh_party(id);
                int m = ((Map10007*)&g_game->parties)->lookup(list2[i]);
                (g_game->parties.pages[m >> 4] + (m & 15))->dirty = 1;
            }
        }
    }

    for (int i = 0; i < n1; i++) {
        unsigned id = list1[i];
        if (id < g_game->parties.n_parties) {
            int j = ((Map10007*)&g_game->parties)->lookup(id);
            if (j >= 0 && (g_game->parties.pages[(unsigned)j >> 4] + (j & 15))->type == 1) {
                g_game->refresh_party(id);
                int m = ((Map10007*)&g_game->parties)->lookup(list1[i]);
                (g_game->parties.pages[m >> 4] + (m & 15))->dirty = 1;
            }
        }
    }

    if ((*(__int64*)((char*)&g_script + 0x18)) != 0) {
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
            BattleRec* b = (BattleRec*)((char*)battles()->pages[bi >> 4] + (bi & 15) * 0x98);
            b->detach();
        }
    } else {
        ((GameW*)g_game)->detach_party(id);
    }
}


// ---- the world-map path finder and the nav drivers ----------------------------------------------------------------------
extern float g_half;        // 0x549d70 = 0.5f
extern float g_one;         // 0x549cdc = 1.0f
extern float g_105;         // 0x59364c = 10.5f
extern float g_10;          // 0x549d74 = 10.0f
extern float g_005;         // 0x54b4d0 = 0.05f
extern float g_vis;         // 0x54a6f0 = 0.01f
extern float g_scale;       // 0x54a6d0 = 1e-5f
extern float g_nav_jitter_chance;                    // 0x54b3c4 (0.35)

struct DetailVert { char pad0[0x14]; float x, y, z, w; };   // the 0x24-byte detail vertices of the map mesh (NavData +0x24)
#define OPEN_BEGIN (*(OpenItem**)&v30.first)         // the open list: a vector<E12> at +0x30 (allocator word, first, last, end)
#define OPEN_END (*(OpenItem**)&v30.last)

// @ 0x4b6b40
// A* over the map's 0x88-byte formation records; fills the record-id chain, the per-node positions and the stall limit of
// the target node.
bool NavData::find_path(int from, int to, int* path, float* detail, int* count, int* visited, int* guard,
                       unsigned* mask_req, unsigned* mask_ban, char* avoid)
{
    (void)guard;
    HeapGt hg;

    // drain whatever the previous search left on the open list
    if (OPEN_BEGIN != 0) {
        for (;;) {
            int n = (int)(OPEN_END - OPEN_BEGIN);
            if (n == 0)
                break;
            if (n > 1) {
                OpenItem last = OPEN_END[-1];
                OPEN_END[-1] = OPEN_BEGIN[0];
                adjust_heap(OPEN_BEGIN, 0, n - 1, last, hg);
            }
            if (OPEN_BEGIN != 0) {
                n = (int)(OPEN_END - OPEN_BEGIN);
                if (n != 0)
                    OPEN_END -= 1;
            }
        }
    }

    reset();

    RECS[from].gen() = m64 + 1;
    RECS[from].parent() = from;
    {
        OpenItem root;
        root.node = from;
        root.cost = 0.0f;
        root.dist = 0.0f;
        ((std::vector<E12>*)&v30)->push_back(*(const E12*)&root);
    }
    push_back_elem(OPEN_BEGIN, OPEN_END, *(HeapGt*)&m40);
    *visited = 0;
    float best = 0.0f;
    float limit = 10000.0f;
    if (*avoid != 0)
        limit = 1e6f;
    int so = from * 0x88;

    float last_dist = 0.0f;                          // the loop tests the dist of the element popped by the previous round (0 at the start)
    if (RECS[to].gen() <= m64) {
        for (;;) {
            if (OPEN_BEGIN == 0)
                break;
            if ((int)(OPEN_END - OPEN_BEGIN) == 0)
                break;
            if (!(last_dist < limit))
                break;

            *visited = *visited + 1;
            OpenItem cur = OPEN_BEGIN[0];
            last_dist = cur.dist;
            if ((int)(OPEN_END - OPEN_BEGIN) > 1) {
                OpenItem last = OPEN_END[-1];
                pop_heap_elem(OPEN_BEGIN, OPEN_END - 1, OPEN_END - 1, last, hg, 0);
            }
            if ((int)(OPEN_END - OPEN_BEGIN) != 0)
                OPEN_END -= 1;

            int node = cur.node;
            int sn = node * 0x88;
            int nidx = node * 0x22;
            int nb = 0;
            if (RECS[node].npts > 0) {
                int off = sn + 0x28;
                do {
                    int nbr = *(int*)((char*)RECS + off);
                    if (nbr != -1) {
                        int sb = nbr * 0x88;
                        if (RECS[nbr].gen() <= m64) {
                            RECS[nbr].gen() = m64 + 1;
                            RECS[nbr].parent() = node;

                            unsigned fl = *(unsigned*)((char*)RECS + sb + 0x24);
                            float cost = *(float*)((char*)RECS + (nidx + nb) * 4 + 0x38);
                            if ((*mask_req & fl) != *mask_req)
                                cost += 10000.0f;
                            if ((*mask_ban & fl) != 0)
                                cost += 10000.0f;

                            float* pb = (float*)((char*)RECS + sb + 0x74);
                            float tz = *(float*)((char*)RECS + to * 0x88 + 0x7c) - pb[2];
                            float ty = *(float*)((char*)RECS + to * 0x88 + 0x78) - pb[1];
                            float tx = *(float*)((char*)RECS + to * 0x88 + 0x74) - pb[0];
                            float fz = *(float*)((char*)RECS + so + 0x7c) - pb[2];
                            float fy = *(float*)((char*)RECS + so + 0x78) - pb[1];
                            float fx = *(float*)((char*)RECS + so + 0x74) - pb[0];
                            float d_from = (float)sqrt(fx * fx + (fy * fy + fz * fz));
                            float d_to = (float)sqrt(tx * tx + (ty * ty + tz * tz));

                            cost += cur.cost;
                            if (best < cur.cost)
                                best = cur.cost;

                            OpenItem add;
                            add.node = nbr;
                            add.cost = cost;
                            add.dist = (d_to / d_from + 1.0f) * cost;
                            ((std::vector<E12>*)&v30)->push_back(*(const E12*)&add);
                            if (OPEN_BEGIN != OPEN_END && (int)(OPEN_END - 1 - OPEN_BEGIN) > 0) {
                                OpenItem last = OPEN_END[-1];
                                push_heap_elem(OPEN_BEGIN, (int)(OPEN_END - 1 - OPEN_BEGIN), 0, last, hg);
                            }
                        }
                    }
                    off += 4;
                    nb++;
                } while (nb < RECS[node].npts);
            }
            RECS[node].gen() = m64 + 2;
        }
    }

    if (RECS[to].gen() <= m64) {
        *count = 0;
        return 0;
    }

    char reachable = best < 10000.0f;

    int chain[0x2001];                               // the original writes chain[0x2000] too (into its saved edi)
    int k = 0;
    int cur = to;
    while (cur != from && k < 0x2000) {
        chain[k++] = cur;
        cur = RECS[cur].parent();
    }
    chain[k++] = from;
    int m = k;
    if (k > 0x400)
        m = 0x400;
    *count = m;

    if (m > 0) {
        int prev = from;                             // the edge is looked up on the previous step's record
        for (int i = 0; i < m; i++) {
            int id = chain[k - 1 - i];
            path[i] = id;
            int cnt = RECS[prev].npts;
            int idx = 0;                              // 0 when the edge is not found
            for (int j = 0; j < cnt; j++) {
                if (RECS[prev].targets()[j] == id) {
                    idx = j;
                    break;
                }
            }
            int v = RECS[prev].agents()[idx];
            prev = path[i];
            DetailVert* vv = &((DetailVert*)v20.first)[v];
            detail[i * 4 + 0] = vv->x;
            detail[i * 4 + 1] = vv->y;
            detail[i * 4 + 2] = vv->z;
            detail[i * 4 + 3] = vv->w;
        }
    }

    for (int i = 1; i < *count; i++) {
        detail[i * 4 - 4] = detail[i * 4 + 0];
        detail[i * 4 - 3] = detail[i * 4 + 1];
        detail[i * 4 - 2] = detail[i * 4 + 2];
        detail[i * 4 - 1] = detail[i * 4 + 3];
    }
    {
        int lastid = path[*count - 1];
        float* e = (float*)((char*)RECS + lastid * 0x88 + 0x74);
        float* o = detail + *count * 4 - 4;
        o[0] = e[0];
        o[1] = e[1];
        o[2] = e[2];
        o[3] = *(float*)((char*)RECS + lastid * 0x88 + 0x80);
    }
    return reachable ? 1 : 0;
}

// @ 0x4b71e0
void BattleRec::update_nav_anim(int)
{
    if (party_a == g_game->f78 || party_b == g_game->f78) {
        obj->visible = 0;
        return;
    }

    if (type == 1) {
        if (opacity > g_half && GDP(char, 0x95) == 0) {
            if (anim.elapsed() > g_105) {
                sound = g_sound_mgr.play(g_sound_bank, 0, obj->pos, 1.0f, 0);
                anim.start = g_ticks[anim.clock];
            }
        }
        if (opacity < g_005 && sound != -1) {
            if (anim.elapsed() < g_10)
                FSOUND_StopSound(sound);
        }
        {
            __int64 elapsed = g_ticks[born.clock] - born.start;
            float secs = (float)elapsed * g_scale;
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
        o->visible = (opacity > g_vis) ? 1 : 0;
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
    bool ok = g_level->core->find_path(mesh->m4, from, path, detail, &npath, &visited, &from,
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
            if (!g_game->parties.is_live(d)) {
                kind = 0;
            } else if (g_game->parties.get(d)->mesh != tgt) {
                moving = 0;
                f5988 = 1;
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
        if (!g_game->parties.is_live(d)) {
            kind = 0;
        } else {
            float* p = &g_game->parties.get(d)->pos.x;
            dest_x = p[0];
            dest_y = p[1];
            lx = p[0];
            ly = p[1];
        }
    } else if (kind == 3) {
        if (!(frand() < g_nav_jitter_chance)) {
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
    if (kind == 1) {
        tf = tgt;
    } else {
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
    }

    if (f5988 && tf && mesh->m4 != tf->m4) {
        if (!start_path(tf->m4, dest_x, dest_y)) {
            f598c = 1;
            f5988 = 0;
        } else {
            f598c = 0;
            f5984 = tf->m4;
            int max = 10;
            if (id == g_game->f78)
                max = 0x96;
            int progress = 0;
            while (progress < max) {
                int from = mesh->m4;
                if (from < 0)
                    break;
                int cur = cursor;
                if (cursor + 1 >= npath)
                    break;
                float p4[4];
                p4[0] = detail[cur * 4 + 1];
                p4[1] = detail[cur * 4 + 2];
                if (!g_level->core->can_move(from, pos.x, pos.y, path[cur + 1], p4, 0, max))
                    break;
                cursor = cur + 1;
                progress++;
            }
            moving = 1;
            step_route();
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
    char pad44[4];
};
namespace rgl { vector4 unpack_color(const unsigned* argb); }   // 0x40e4e0 (cdecl, hidden return pointer: shapes.cpp)

#define SCALE_ROWS(m, a) \
    (m)->m_cc.s.x *= (a); (m)->m_cc.s.y *= (a); (m)->m_cc.s.z *= (a); \
    (m)->m_cc.f.x *= (a); (m)->m_cc.f.y *= (a); (m)->m_cc.f.z *= (a); \
    (m)->m_cc.u.x *= (a); (m)->m_cc.u.y *= (a); (m)->m_cc.u.z *= (a)

// @ 0x4b7840
// Build the party's two model meshes (the party type's body and, for a rider, the rider mesh) from the type table, swap them
// into the model's mesh list and set the model's fade and colour state.
void Party::update_flags()
{
    if (f5a04) {
        f5a04 = 0;
        for (unsigned i = 0; deferred.first != 0 && i < (unsigned)(deferred.last - deferred.first); i++) {
            vector4 p(0.0f, 0.0f, 0.0f);
            int idx = 0;
            Emit* e = g_emit_defs.spawn(&deferred.first[i], &p, &idx);
            if (e)
                model->add_emitter(e);
        }
    }
    unsigned def = (flags_lo & 0xff) * 0x48;
    rgl::mesh* m2 = 0;
    int rider = f94;
    PartyTypeDef* d = (PartyTypeDef*)((char*)g_map_icons + def);
    rgl::mesh* m = ((MeshRes*)((::resmgr*)&g_resmgr_full)->get_object((const rgl::string*)&d->name))->copy_of();
    float a = d->scale;
    SCALE_ROWS(m, a);
    GDP(rgl::mesh*, 0x1a200) = m;
    GDP(int, 0x1a1fc) = 0;
    (*(__int64*)((char*)&g_script + 8)) = id;
    d->frames.update(-0x46);
    if (rider > 0) {
        PartyTypeDef* r = (PartyTypeDef*)((char*)g_map_icons + (unsigned)rider * 0x48);
        m2 = ((MeshRes*)((::resmgr*)&g_resmgr_full)->get_object((const rgl::string*)&r->name))->clone();
        m2->translate(&d->pos);
        if (!(flags_lo & 0x400)) {
            float b = r->scale;
            SCALE_ROWS(m2, b);
        } else {
            Rot3 dm;
            identity3(&dm);
            float ang = 1.5707964f;
            dm.rotate_about_u(&ang);
            ang = -angle;
            dm.rotate_about_u(&ang);
            *(Rot3*)&m2->m_cc = dm;
            float c = r->scale * 1.5f;
            SCALE_ROWS(m2, c);
        }
        GDP(rgl::mesh*, 0x1a200) = m2;
        GDP(int, 0x1a1fc) = 0;
        (*(__int64*)((char*)&g_script + 8)) = id;
        d->frames.update(-0x46);
    }
    if (e59ec == 0) {
        model->add_extra(m);
    } else {
        for (unsigned i = 0; model->ents.first != 0 &&
                          i < (unsigned)((rgl::mesh**)model->ents.last - (rgl::mesh**)model->ents.first); i++) {
            rgl::mesh** et = (rgl::mesh**)model->ents.first;
            if (et[i] == e59ec) {
                if (et[i])
                    delete et[i];
                et[i] = m;
            }
        }
    }
    if (e59f0 == 0) {
        if (m2)
            model->add_extra(m2);
    } else {
        for (unsigned i = 0; model->ents.first != 0 &&
                          i < (unsigned)((rgl::mesh**)model->ents.last - (rgl::mesh**)model->ents.first); i++) {
            rgl::mesh** et = (rgl::mesh**)model->ents.first;
            if (et[i] == e59f0) {
                if (et[i])
                    delete et[i];
                if (m2) {
                    ((rgl::mesh**)model->ents.first)[i] = m2;
                } else {
                    unsigned n = (unsigned)((rgl::mesh**)model->ents.last - (rgl::mesh**)model->ents.first);
                    ((rgl::mesh**)model->ents.first)[i] = ((rgl::mesh**)model->ents.first)[n - 1];
                    model->ents.last = (int*)((rgl::mesh**)model->ents.last - 1);
                }
            }
        }
    }
    e59ec = m;
    e59f0 = m2;
    if (d->f10 & 1) {
        model->set_mode(0);
    } else {
        model->set_mode(2);
        float one = 1.0f;
        model->set_fade(&one);
        model->f_c0 = d->scale * 3.0f;
        model->f294 = 0.5f;
    }
    unsigned mask = (flags_lo & 0x400) ? 0x1ffff : 0xffff;
    m->mask = mask;
    if (m2)
        m2->mask = mask;
    model->color.x = 0.0f;
    model->color.y = 0.0f;
    model->color.z = 0.5f;
    model->color.w = a;
    float shade = 0.95f;
    model->set_color(&shade);
}

// @ 0x4b7e50
void Party::update_hud_text()
{
    if (dirty) {
        f8ac = march_time(this);
        (*(int*)((char*)&g_script + 0x18)) = 100;
        (*(int*)((char*)&g_script + 0x1c)) = 0;
        g_game->emit_event(0x1a, id, 0);
        dirty = 0;
        f8b5 = 1;
        f8b0 = (float)(*(__int64*)&(*(int*)((char*)&g_script + 0x18))) * 0.01f;
    }
    if (!f8b5)
        return;
    int ammo = 0;
    vector4 t;
    float v;
    int b = 0, d = 0, c = 0;
    g_game->count_units(id, &ammo, &c, &b, &d);
    GameData* g = g_game;
    v = *(float*)((char*)g->factions + f70 * 0x300 + 0x2c);
    unsigned color;
    if (g_show_faction_color != 0) {
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
    } else {
        color = g->color_of(v, f6c);
    }
    if (id == g->f78) {
        color = 0xffffffff;
    } else if ((flags_lo & 0x40000) != 0) {
        color = 0xffaa22dd;
        if (!(v < -0.01f))
            color = 0xff22aaff;
    }
    char buf[128];
    if (b == 0)
        sprintf(buf, "%d", ammo);
    else
        sprintf(buf, "%d+%d", ammo, b);
    float ct = *(float*)&flags_hi;
    if ((flags_lo & 0x80000) != 0) {
        tstring str("");
        ct = 1.0f;
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
extern vector4 g_emit_pos;                           // 0xa708c0

// @ 0x4b80d0
// The per-tick update of a map party: periodic re-planning, steering towards the destination, the nav-mesh probe at the
// candidate position, the order state machine (chase a party, reach a site, wait next to a party), the party-event trail on
// large moves and the model refresh (matrix, colour, the two action meshes).
void Party::tick(float dt)
{
    // ---- early outs ---------------------------------------------------------------------------------------
    if (flags_lo & 0x100) {
        model->visible = 0;
        return;
    }
    if ((id == g_game->f78 && GDP(float, 0x13ad0) > 0.0f && GDP(int, 0x13ad4) != 0) || f8c8 >= 0) {
        model->visible = 0;
        return;
    }

    // ---- periodic re-planning (never for the camera party) ------------------------------------------------
    if (!(flags_lo & 0x400)) {
        if (id != g_game->f78) {
            if (interval <= clk8d0.elapsed()) {
                f598c = 0;
                think();
                clk8d0.start = g_ticks[clk8d0.clock];
                float nv = frand() * 0.15f + 0.25f;
                interval = nv;
                if (action == 0xb)
                    interval = nv * 0.1f;
                t934 = 0.0f;
            }
        }
        float d1 = (float)(g_ticks[idx938] - t940) * 1e-05f;
        if (t934 < d1) {
            plan_order();
            clk948.set_delay(0.1f);
            t940 = g_ticks[idx938];
            t934 = (frand() + 1.0f) * 0.1f;
            if (f597c < 5.0f)
                f597c = f597c + 0.5f;
        }
        float d2 = (float)(g_ticks[clk948.clock] - clk948.start) * 1e-05f;
        if (d2 > 0.05f || id == g_game->f78) {
            update_nav();
            clk948.start = g_ticks[clk948.clock];
        }
        if (moving == 0 && kind == 2) {
            int oid = tgt_id;
            if (g_game->parties.is_live(oid)) {
                Party* o = g_game->parties.get(oid);
                dest_x = o->pos.x;
                dest_y = o->pos.y;
            }
        }
    }

    // ---- movement -----------------------------------------------------------------------------------------
    model->visible = 1;
    if (action == 8) {
        model->visible = 0;
        return;
    }
    update_hud_text();

    vec2 dir2;
    dir2.x = dest_x - pos.x;
    dir2.y = dest_y - pos.y;
    float dist = dir2.normalize();
    bool moved = false;
    float step = speed_factor() * dt;

    bool skip_heading = false;
    if (moving == 0 && kind == 2) {
        float lim = dist - radius * 1.2f;
        if (lim < step) {
            step = lim;
            if (lim < 0.0f)
                step = 0.0f;
            dist = lim;
            if (lim < 0.0f)
                skip_heading = true;
        }
    } else {
        if (dist < step)
            step = dist;
    }

    if (!skip_heading && dist > 0.0f) {
        dirx = dir2.x;
        diry = dir2.y;
        vec2 t;
        t.x = dirx - oldx;
        t.y = diry - oldy;
        t.normalize();
        float want = (float)atan2(-t.x, t.y);
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

    // ---- nav-mesh probe at the candidate position ---------------------------------------------------------
    float px = step * dirx + pos.x;
    float py = step * diry + pos.y;
    face* node = g_game->find_at(&px);
    if (node != 0) {
        unsigned animfield = nav_recs()[node->m4].mask();
        bool adjacent = ((RiderTable*)g_level->core)->shares(node->m4, mesh->m4) != 0;
        bool commit = false;
        if (flags_lo & 0x200) {
            if ((char)terrain_is_road(animfield)) {
                commit = true;
            } else if (animfield != 0xf && id == g_game->f78 && 0.05f < GDP(Timer, 0x13a90).elapsed()) {
                g_emit_pos.x = px;
                g_emit_pos.y = py;
                g_emit_pos.z = 0.0f;
                g_game->emit_event(0x14, 0, 0);
                GDP(Timer, 0x13a90).start = g_ticks[GDP(Timer, 0x13a90).clock];
                kind = 0;
                moving = 0;
                f5988 = 0;
            }
        }
        if (!commit) {
            if (adjacent) {
                commit = true;
            } else {
                f597c = 0;
                if (f598c == 0)
                    f5988 = 1;
            }
        }
        if (commit) {
            pos.x = px;
            pos.y = py;
            mesh = node;
            map_mask = animfield;
            pos.z = node->plane_z(&pos.x);
        }
    }

    // ---- action state machine -----------------------------------------------------------------------------
    if (action == 4 && target >= 0 && g_game->parties.is_live(target) &&
        !(g_game->parties.get(target)->flags_lo & 0x100) &&
        g_game->parties.get(target)->action != 8 && dt > 0.0f) {
        Party* o = g_game->parties.get(target);
        float ox = o->pos.x;
        float oy = o->pos.y;
        float ax = ox - pos.x;
        float ay = oy - pos.y;
        float ad = sqrtf(ax * ax + ay * ay);
        Party* o2 = g_game->parties.get(target);
        if (ad < o2->radius + radius) {
            Party* o3 = g_game->parties.get(target);
            if (id == g_game->f78 && !(o3->flags_lo & 0x400) && o3->f88 < 0 &&
                o3->action == 4 && o3->target == id) {
                ((c49f060::GameData*)g_game)->deploy_party(target, id);
            } else {
                ((c49f060::GameData*)g_game)->deploy_party(id, target);
            }
            Party* o4 = g_game->parties.get(target);
            o4->kind = 0;
            o4->moving = 0;
            o4->f5988 = 0;
            kind = 0;
            moving = 0;
            f5988 = 0;
        }
    } else if (action == 9) {
        int cell = target;
        char* page = *(char**)((char*)g_game + 0xe4 + (cell >> 4) * 4);
        char* rec = page + (cell & 0xf) * 0x98;
        float cx = *(float*)(rec + 0x2c);
        float cy = *(float*)(rec + 0x30);
        float ax = cx - pos.x;
        float ay = cy - pos.y;
        float ad = sqrtf(ax * ax + ay * ay);
        float cr = *(float*)(rec + 0x40);
        if (ad < cr + radius) {
            BattleRec* n = battle_get(cell);
            pos.x = n->x1;
            pos.y = n->y1;
            pos.z = battle_get(cell)->z;
            mesh = battle_get(cell)->face1;
            flags_lo |= 0x200;
            battle_get(cell)->detach();
            action = 0;
            kind = 0;
            moving = 0;
            f5988 = 0;
            update_flags();
        }
    } else if (action == 1) {
        if (target >= 0 && g_game->parties.is_live(target)) {
            Party* o = g_game->parties.get(target);
            float ox = o->pos.x;
            float oy = o->pos.y;
            float ax = ox - pos.x;
            float ay = oy - pos.y;
            float ad = sqrtf(ax * ax + ay * ay);
            Party* o2 = g_game->parties.get(target);
            if (ad < o2->radius + radius) {
                action = 8;
                kind = 0;
                moving = 0;
                f5988 = 0;
            }
        }
    }

    // ---- party-event trail on large moves -----------------------------------------------------------------
    {
        vector4 delta;
        delta.x = pos.x - trail.x;
        delta.y = pos.y - trail.y;
        delta.z = pos.z - trail.z;
        float mlen = delta.normalize();
        if (mlen > trail_len && !(flags_lo & 0x200) && map_mask != 7) {
            float midx = pos.x * 0.5f + trail.x * 0.5f;
            float midy = pos.y * 0.5f + trail.y * 0.5f;
            float midz = pos.z * 0.5f + trail.z * 0.5f;
            float ang = (float)atan2(-delta.x, delta.y);
            vector4 mid(midx, midy, midz);
            g_game->push_party_event(id, (const int*)&mid, &ang);
            float r1 = frand();
            float r2 = frand();
            trail.x = pos.x;
            trail.y = pos.y;
            trail.z = pos.z;
            trail.w = pos.w;
            trail_len = (r2 + r1) * 0.7f + 0.5f;
        }
    }

    // ---- model refresh ------------------------------------------------------------------------------------
    Rot3 m;
    identity3(&m);
    model->basis = m;
    model->basis.rotate_about_u(&angle);
    model->origin = *(vector4*)&pos.x;

    unsigned color;
    if (id == g_game->f78) {
        color = (flags_lo & 0x1000000) ? 0x88ffffffu : 0xffffffffu;
    } else {
        vector4 c(1.0f, 1.0f, 1.0f);
        c.w = alpha;
        color = rgl::pack_color(&c);
        model->visible = (alpha > 0.01f);
        model->f294 = alpha * 0.5f;
    }
    for (int i = 0; i < (int)((rgl::mesh**)model->ents.last - (rgl::mesh**)model->ents.first); i++)
        if (((rgl::mesh**)model->ents.first)[i] != 0)
            ((rgl::mesh**)model->ents.first)[i]->color = color;
    if (!model->visible)
        return;
    if (e59ec != 0)
        *(int*)((char*)e59ec + 0x114) = 0;
    if (e59f0 != 0)
        *(int*)((char*)e59f0 + 0x114) = 0;

    // ---- the party's first action mesh -------------------------------------------------------------------
    rgl::mesh* e1 = e59ec;
    if (!moved) {
        if (e1 != 0 && e1->in_use()) {
            float one = 1.0f;
            ((mesh_node*)e1)->set_time(&one);
        }
    } else {
        if (e1 != 0 && e1->in_use()) {
            float ph = (float)fmod(((double)g_ticks[0] * 1e-05f + (double)id) * 2.3f, 1.0) * 40.0f + 100.0f;
            ((mesh_node*)e1)->set_time(&ph);
            *(int*)((char*)e59ec + 0x114) = 1;
            if (clk908.elapsed() > 0.5f && dt > 0.0f) {
                clk908.start = g_ticks[clk908.clock];
                int snd = *(int*)((char*)g_map_icons + (flags_lo & 0xff) * 0x48 + 0x28);
                if (snd != 0)
                    g_sound_mgr.play(snd, 0, (const float*)&model->origin, 1.0f, 0);
            }
        }
    }

    // ---- the party's second action mesh -------------------------------------------------------------------
    rgl::mesh* e2 = e59f0;
    if (e2 == 0)
        return;
    int v = e2->in_use();
    if (!(v > 0))
        return;
    float sx = *(float*)((char*)g_level + 0x31c) - pos.x;
    float sy = *(float*)((char*)g_level + 0x320) - pos.y;
    float sz = *(float*)((char*)g_level + 0x324) - pos.z;
    float dd = sx * sx + sy * sy + sz * sz;
    if (!(dd < 2000.0f))
        return;
    float ph2 = (float)fmod((double)id * 0.05f + (double)g_ticks[0] * 1e-05f, 1.0) * 40.0f + 100.0f;
    ((mesh_node*)e2)->set_time(&ph2);
    *(int*)((char*)e59f0 + 0x114) = 1;
}

// ==== END ====
