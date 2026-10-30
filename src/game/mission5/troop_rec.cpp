//-----------------------------------------------------------------------------
// File: troop_rec.cpp  (0x51eb90..0x520b20)
//
// Desc: the scene half of the save-game loader (Scene::read / clear / get and the 0x5ac8-byte party record reader), the
//       new-game start-up (Game::start_new_game and more_start), the troop item and level helpers (reset_items,
//       apply_items, level_up, raise_skills, raise_stats) and the animator re-seed (Animator::reset). Names provisional.
//       "T regime" region (docs/tu_map.md): the file keeps the binary order; helpers that follow their caller are
//       __declspec(noinline) where the original calls them. Flags: /O2 /MT /Gy /TP.
//
// Boundary: class changes only (Game | Scene at 0x51eb90, TroopRec | Game at 0x520b20); no forward call proves either.
//-----------------------------------------------------------------------------
#include "../std_vec_out_of_line.h"      // std::vector<int / __int64>::resize: the out-of-line instances of agent_combat.cpp / world_data.cpp
#include "./troop_rec.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../party/game.h"
#include "../ui/item_ref.h"
#include "../ui/ui_common.h"
#include "../int_vec.h"
#include "../item_ref.h"
#include "../skill_cap.h"
#include "../../include/troop.h"
#include "../../include/timer.h"
#include "../../include/item_kind.h"
#include "../../include/game_recs.h"
#include "../mesh_slot/mesh_slot.h"
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)

extern float g_scene_rate;                          // 0x59bfec (rdata: scales the byte at +0xa6 of a party record)

// @ 0x51eb90
void Scene::read(stream_base* s, int flag)
{
    SceneView* v = (SceneView*)this;
    this->clear();
    int n;
    s->r_int(&n);
    int last;
    s->r_int(&last);
    while (v->count < n) {
        this->grow();
        if (v->free_ids._Myfirst)
            operator delete(v->free_ids._Myfirst);
        v->free_ids._Myfirst = 0;
        v->free_ids._Mylast = 0;
        v->free_ids._Myend = 0;
    }
    for (int i = 0; i < n; i++) {
        char* rec = (char*)v->pages._Myfirst[(unsigned)i >> 4] + (i & 0xf) * 0x5ac8;
        int live;
        s->r_int(&live);
        if (live != 0) {
            int index;
            s->r_int(&index);
            int id;
            s->r_int(&id);
            *(int*)(rec + 4) = 1;
            int key = id % 0x2717;
            if (v->cells[key * 2] != -1) {
                do {
                    key = (key + 0x4f) % 0x2717;
                } while (v->cells[key * 2] != -1);
            }
            v->cells[key * 2 + 1] = index;
            v->cells[key * 2] = id;
            *(int*)(rec + 8) = id;
            ((PartyRecIO*)rec)->read(s, flag);
        } else {
            *(int*)(rec + 4) = 0;
        }
    }
    IntVec* ids = &v->free_ids;
    if (ids->_Myfirst)
        operator delete(ids->_Myfirst);
    ids->_Myfirst = 0;
    ids->_Mylast = 0;
    ids->_Myend = 0;
    for (int i = 0; i < v->count; i++) {
        char* rec = (char*)v->pages._Myfirst[(unsigned)i >> 4] + (i & 0xf) * 0x5ac8;
        if (*(int*)(rec + 4) != 1)
            ids->push_back(i);
    }
    v->f138dc = last;
}

// @ 0x51eda0
void Scene::clear()
{
    SceneView* v = (SceneView*)this;
    for (int i = 0; i < 0x2717; i++) {
        v->cells[i * 2] = -1;
        v->cells[i * 2 + 1] = -1;
    }
    ((std::vector<int>*)&v->free_ids)->resize(0, 0);
    for (int i = 0; i < v->count; i++) {
        *(int*)((char*)this->get(i) + 4) = 0;
        v->free_ids.push_back(i);
    }
    v->f138dc = 0;
}

// @ 0x51ee50
PartyRecIO* Scene::get(int id)
{
    SceneView* v = (SceneView*)this;
    int k = this->index_of(id);
    return (PartyRecIO*)((k & 0xf) * 0x5ac8 + (int)v->pages._Myfirst[k >> 4]);
}

// @ 0x51ee80
void PartyRecIO::read(stream_base* s, int flag)
{
    s->r_tstr((rgl::string*)(data + 0x20));
    s->r_tstr((rgl::string*)(data + 0xc));
    ((tstring*)(data + 0xc))->underscores_to_spaces();          // 0x42bf80
    s->r_u64((unsigned __int64*)(data + 0xa0));
    s->r_int((int*)(data + 0x90));
    s->r_int((int*)(data + 0x8c));
    s->r_int((int*)(data + 0x70));
    s->r_int((int*)(data + 0x74));
    s->r_int((int*)(data + 0x8e4));
    s->r_int((int*)(data + 0x918));
    s->r_int((int*)(data + 0x8e8));
    s->r_int((int*)(data + 0x91c));
    int* p = (int*)(data + 0x8ec);
    stream_base* rs = &s->r_float((float*)p);
    rs->r_float((float*)++p);
    p = (int*)(data + 0x924);
    rs = &s->r_float((float*)p);
    rs->r_float((float*)++p);
    p = (int*)(data + 0x30);
    rs = &s->r_float((float*)p);
    rs = &rs->r_float((float*)(p + 1));
    rs->r_float((float*)(p + 2));
    s->r_int((int*)(data + 0x8a8));
    char* e = data + 0xa8;
    for (int i = 0; i < *(int*)(data + 0x8a8); i++, e += 0x20) {
        s->r_int((int*)(e + 4));
        s->r_int((int*)e);
        s->r_int((int*)(e + 0x10));
        s->r_int((int*)(e + 8));
    }
    s->r_float((float*)(data + 0x50));
    if (flag > 0) {
        s->r_bool(data + 0x1c);
        s->r_tstr((rgl::string*)(data + 0x54));
        p = (int*)(data + 0x7c);
        rs = &s->r_float((float*)p);
        rs = &rs->r_float((float*)(data + 0x80));
        rs->r_float((float*)(data + 0x84));
        s->r_float((float*)(data + 0x8f4));
        s->r_float((float*)(data + 0x958));
        s->r_int((int*)(data + 0x6c));
        s->r_float((float*)(data + 0x78));
        s->r_i64((__int64*)(data + 0x5ab8));
        s->r_int((int*)(data + 0x94));
        s->r_int((int*)(data + 0x8c8));
        s->r_bool(data + 0x8cc);
        int n1;
        s->r_int(&n1);
        ((std::vector<int>*)(data + 0x8b8))->resize(n1, 0);
        for (int i = 0; i < n1; i++)
            s->r_int(*(int**)(data + 0x8bc) + i);
        int n2;
        s->r_int(&n2);
        ((std::vector<int>*)(data + 0x59f4))->resize(n2, 0);
        for (int i = 0; i < n2; i++)
            s->r_int(*(int**)(data + 0x59f8) + i);
        data[0x5a04] = 1;
        read_soa(s, (SoaBlock*)(data + 0x5a08));
        int n3;
        s->r_int(&n3);
        I64Vec* slots = (I64Vec*)(data + 0x59d8);
        ((std::vector<__int64>*)slots)->resize(n3, 0);
        for (int i = 0; i < n3; i++)
            s->r_i64(slots->_Myfirst + i);
    } else {
        *(float*)(data + 0x78) = (float)*(unsigned char*)(data + 0xa6) * g_scene_rate;
    }
}

// the globals the start-up resets (the spawn / slider block at 0xa70440 and the free list behind it)
extern int g_script;                                // 0xa70440
extern int g_spawn_hi;                              // 0xa7044c
extern int g_a70454;                                // 0xa70454
extern void* g_a70484;                              // 0xa70484
extern void* g_a70488;                              // 0xa70488
extern void* g_a7048c;                              // 0xa7048c
extern void* g_windows[];                         // 0xa73980 (30 menu pointers)

static inline int clamp_rank(int v)
{
    return v < 0 ? 0 : (v > g_skill_tbl[36].cap ? g_skill_tbl[36].cap : v);
}

// @ 0x51f1a0
void Game::start_new_game()
{
    tstring a;                                      // [esp+0x28] -- first parties.txt string
    tstring b;                                      // [esp+0x18] -- second parties.txt string

    (*(int*)((char*)&g_script + 0x8)) = -100000;                           // 0xa70448
    (*(int*)((char*)&g_script + 0x10)) = -100000;                             // 0xa70450
    g_script = 0;                                   // 0xa70440
    (*(int*)((char*)&g_script + 0x34)) = -1;                                  // 0xa70474
    (*(int*)((char*)&g_script + 0x38)) = 0;                                   // 0xa70478
    (*(int*)((char*)&g_script + 0x3c)) = -1;                                  // 0xa7047c
    g_spawn_hi = -1;                                // 0xa7044c
    g_a70454 = -1;                                  // 0xa70454
    (*(int*)((char*)&g_script + 0x18)) = 0;                                   // 0xa70458
    (*(int*)((char*)&g_script + 0x1c)) = 0;                                   // 0xa7045c
    (*(int*)((char*)&g_script + 0x20)) = 100;                                 // 0xa70460
    if (g_a70484)
        operator delete(g_a70484);
    g_a70484 = 0;
    g_a70488 = 0;
    g_a7048c = 0;

    g_ticks[1] = (__int64)g_start_time;

    ((snd_bank*)&triggers)->randomize();            // 0x462790 (the sound bank sits at +0x139e8)
    simple.arm();                                   // 0x51f7c0

    timer13b18.t = g_ticks[timer13b18.clock] - 600000;
    f4 = rand();

    timer1a298.t = g_ticks[timer1a298.clock];

    if (g_cfg.skip_tutorial == 0) {
        for (int i = 0; i < 30; i++)
            *(int*)((char*)g_windows[i] + 8) = 0;
    }

    f13b28 = -1;
    f13b2c = -1;
    f13b30 = -1;
    f13b34 = -1;
    f13b38 = -1;
    f13b60 = -1;
    f13a34 = 0;

    TextReader stream;
    {
        const char* path = ((GameData*)this)->module_path("parties.txt");
        if (!((text_stream*)&stream)->open_read_text(path))
            mb_error("Unable to open %s", path);
        stream_base* sp = &stream;                  // the original calls the slots through the vtable
        sp->r_tstr(rs(&a));
        sp->r_tstr(rs(&b));
        int version;
        sp->r_int(&version);
        stream.failed = 1;
        scene.read(&stream, 0);
    }

    int slot = scene.index_of(f78);
    char* pr = ((SceneView*)&scene)->pages._Myfirst[slot >> 4] + (slot & 0xf) * 0x5ac8;
    *(int*)(pr + 0x74) = 0x80;

    if (g_party_template_count > 0) {
        for (int i = 0; i < g_party_template_count; i++) {
            PartyTemplate* r = &g_party_templates[i];
            r->f34 = 0;
            r->f38 = 0;
            r->f3c = 0;
            ((VecView*)&r->list)->clear();
        }
    }
    if (g_item_count > 0) {
        for (int i = 0; i < g_item_count; i++)
            ((VecView*)((char*)&g_items[i] + 0x30))->clear();
    }

#define TROOP(i) (((TroopRec*)troops)[i])
    if (ntroops > 0) {
        for (int i = 0; i < ntroops; i++) {
            if (i != 0)
                TROOP(i).level_up();
            TROOP(i).auto_equip(-1, 0, 0, 0);
            if (TROOP(i).flags30 & 0x8000)
                g_anim.reset((char*)&TROOP(i) + 0x3c0, (char*)&TROOP(i) + 0x3e0);
        }
    }

    for (int i = 0; i < 0x40; i++) {
        for (int j = 0; j < 0x20; j++)
            ((int*)scene_parties)[i * 0x20 + j] = rand();
    }

    ((tstring*)&TROOP(cur_troop))[0] = "";

    if (g_cfg.skip_tutorial != 0) {
        int a1 = g_skill_tbl[1].cap < 3 ? g_skill_tbl[1].cap : 3;
        ((NibWord*)&TROOP(cur_troop).nib[0])->n1 = a1;
        int y = TROOP(cur_troop).lvl58 + 0x23 + clamp_rank(((unsigned char*)&TROOP(cur_troop))[0x7a] & 0xf) * 2;
        if ((float)y < TROOP(cur_troop).hp)
            TROOP(cur_troop).hp = (float)(TROOP(cur_troop).lvl58 + 0x23 + clamp_rank(((unsigned char*)&TROOP(cur_troop))[0x7a] & 0xf) * 2);
        int a2 = g_skill_tbl[2].cap < 2 ? g_skill_tbl[2].cap : 2;
        ((NibWord*)&TROOP(cur_troop).nib[0])->n2 = a2;
        int z = TROOP(cur_troop).lvl58 + 0x23 + clamp_rank(((unsigned char*)&TROOP(cur_troop))[0x7a] & 0xf) * 2;
        if ((float)z < TROOP(cur_troop).hp)
            TROOP(cur_troop).hp = (float)(TROOP(cur_troop).lvl58 + 0x23 + clamp_rank(((unsigned char*)&TROOP(cur_troop))[0x7a] & 0xf) * 2);
    }

    TROOP(cur_troop).exp = rand() % 50 + 0x6b;
    TROOP(cur_troop).f3b4 = 0;
    TROOP(cur_troop).hp = (float)(TROOP(cur_troop).lvl58 + 0x23 + clamp_rank(((unsigned char*)&TROOP(cur_troop))[0x7a] & 0xf) * 2);
    TROOP(cur_troop).fa8 = 4;
    TROOP(cur_troop).fa4 = 4;
    TROOP(cur_troop).fac = 10;
    TROOP(cur_troop).fac = 10;

    for (int i = 0; i < 0x2a; i++)
        TROOP(cur_troop).set_skill(i, 0);
#undef TROOP

    more_start();                                   // 0x51f800
}

// @ 0x51f7c0
void SimpleTag::arm()
{
    for (int i = 0; i < count; i++) {
        DelayItem* it = &((DelayItem*)array)[i];
        ((gui_timer*)&it->timer)->set_delay(frand() * it->delay);
    }
}

// ---- item stack helpers (0x51f800..0x520270) ------------------------------------------------------------------------
static inline int item_type(int id)
{
    return g_items[id].type_flags & 0xff;
}

// The stack capacity of the item in a slot: the kind's stack size plus the "nice item" bonus that a quality tag of 0x2a
// grants (one more unit per 13%, rounded to nearest with ties going down).
static __forceinline int slot_cap(ItemRef* s)
{
    if (s->item < 0)
        return 0;
    int v = g_items[s->item].f210;
    if (v != 0 && (s->m4 & 0xff000000) == 0x2a000000) {
        float f = (float)v * g_inv_gain;
        double d = floor(f);                        // 0x536b10
        int q = (int)d;
        if (f - d > g_half)
            q++;
        if (q < 1)
            q = 1;
        v += q;
    }
    return v;
}

static inline int slot_count(ItemRef* s) { return s->m4 & 0xff; }

// @ 0x51f800
void Game::more_start()
{
    for (int i = 0; i < ntroops; i++) {
        ((TroopRec*)&troops[i])->reset_items(i == cur_troop);   // 0x51ff90
        ((TroopRec*)&troops[i])->apply_items();                 // 0x520110
    }

    TroopRec* me = (TroopRec*)(troops + cur_troop);
    int pool = 0;
    int n = me->inv_count();                        // 0x462b60

    // collect the room left in the worn equipment
    ItemRef* s = me->equip;
    for (int i = 0; i < 10; i++, s++) {
        if (s->item > 0) {
            int ty = item_type(s->item);
            if (ty == 0x11 || ty == 0x10)
                pool += slot_count(s) + slot_cap(s) - s->max_amount();
        }
    }
    // ... and in the inventory
    if (n > 0) {
        s = me->inv;
        for (int i = 0; i < n; i++, s++) {
            if (s->item > 0) {
                int ty = item_type(s->item);
                if (ty == 0x11 || ty == 0x10)
                    pool += slot_count(s) + slot_cap(s) - s->max_amount();
            }
        }
    }

    int given = 0;
    // hand the collected room to the ammo stacks in the inventory ...
    if (n > 0) {
        s = me->inv;
        for (int i = 0; i < n; i++, s++) {
            if (s->item > 0 && item_type(s->item) == 0x12) {
                int need = s->max_amount() - slot_count(s);
                if (pool < need)
                    need = pool;
                if (need > 0) {
                    given += need;
                    pool -= need;
                    int v = slot_count(s) + need;
                    if (v > slot_cap(s))
                        v = slot_cap(s);            // the original computes the cap twice
                    s->b.amount = 0;
                    s->m4 |= v;
                    if (s->max_amount() - (s->m4 & 0xff) <= 0)
                        s->item = -1;
                }
            }
        }
    }
    // ... and to those of the worn equipment
    s = me->equip;
    for (int i = 0; i < 10; i++, s++) {
        if (s->item > 0 && item_type(s->item) == 0x12) {
            int need = s->max_amount() - slot_count(s);
            if (pool < need)
                need = pool;
            if (need > 0) {
                given += need;
                pool -= need;
                int v = slot_count(s) + need;
                if (v > slot_cap(s))
                    v = slot_cap(s);
                s->b.amount = 0;
                s->m4 |= v;
                if (s->max_amount() - (s->m4 & 0xff) <= 0)
                    s->item = -1;
            }
        }
    }

    // last pass: top the weapon stacks up with whatever is still left over, first the worn ones ...
    s = me->equip;
    for (int i = 0; i < 10; i++, s++) {
        if (s->item > 0) {
            int ty = item_type(s->item);
            if (ty == 0x11 || ty == 0x10) {
                int room = slot_count(s) + slot_cap(s) - s->max_amount();
                if (room > 0) {
                    int give = given < room ? given : room;
                    if (give > 0) {
                        s->reduce(give);            // 0x437510
                        given -= give;
                    }
                }
            }
        }
    }
    // ... and then the inventory
    if (n > 0) {
        s = me->inv;
        for (int i = 0; i < n; i++, s++) {
            if (s->item > 0) {
                int ty = item_type(s->item);
                if (ty == 0x11 || ty == 0x10) {
                    int free_room = slot_count(s) + slot_cap(s) - s->max_amount();
                    if (free_room > 0) {
                        int give = given < free_room ? given : free_room;
                        if (give > 0) {
                            s->reduce(give);        // 0x437510
                            given -= give;
                        }
                    }
                }
            }
        }
    }
}

// The original inlines two helpers in reset_items / apply_items (the test and the per-slot body), which is why the slot
// id and the item type are each read twice; keep them separate so the same code shape comes out.
static __forceinline void init_slot(ItemRef* s)
{
    int id = *(volatile int*)&s->item;
    if (id <= 0)
        return;
    int ty = item_type(id);
    if (ty == 10 || ty == 5 || ty == 6 || ty == 0x10 || ty == 0x11 || ty == 9 || ty == 0x12) {
        s->b.amount = 0;
    } else if (ty == 7 || ty == 1 || ty == 0x13) {
        int r = s->stat_1e8();                      // 0x434df0 (fill a stack, returns the count)
        *(short*)&s->m4 = 0;
        s->m4 |= r;
    }
}

// @ 0x51ff90
void TroopRec::reset_items(int all)
{
    ItemRef* s = inv;
    for (int i = 0; i < 0x60; i++, s++) {
        if (s->item <= 0)
            continue;
        if (all) {
            int ty = item_type(s->item);
            if (!(ty == 5 || ty == 6 || ty == 10 || ty == 7))
                continue;
        }
        init_slot(s);
    }

    s = equip;
    for (int i = 0; i < 10; i++, s++) {
        if (s->item <= 0)
            continue;
        if (all) {
            int ty = item_type(s->item);
            if (!(ty == 5 || ty == 6 || ty == 10 || ty == 7))
                continue;
        }
        init_slot(s);
    }
}

// @ 0x520110
void TroopRec::apply_items()
{
    ItemRef* s = inv;
    for (int i = 0; i < 0x60; i++, s++) {
        if (s->item <= 0)
            continue;
        {
            int ty = item_type(s->item);
            if (!(ty == 9 || ty == 0x11 || ty == 0x10))
                continue;
        }
        init_slot(s);
    }

    s = equip;
    for (int i = 0; i < 10; i++, s++) {
        if (s->item <= 0)
            continue;
        {
            int ty = item_type(s->item);
            if (!(ty == 9 || ty == 0x11 || ty == 0x10))
                continue;
        }
        init_slot(s);
    }
}

// The attribute words at +0x58 (strength, agility, intelligence, charisma; Troop::f58 covers only the first two).
static __forceinline int& attr(TroopRec* t, int i)
{
    return ((int*)((char*)t + 0x58))[i];
}

// The clamped skill level: the stored 4-bit value, limited by the skill's own cap. The index type matters to the code
// generation (a signed index gets the signed /8 and %8 sequences).
template <class T>
static __forceinline int get_skill(TroopRec* t, T i)
{
    int v = (t->nib[i / 8] >> ((i % 8) * 4)) & 0xf;
    if (v < 0)
        return 0;
    int cap = g_skill_tbl[i].cap;
    if (v > cap)
        return cap;
    return v;
}

// @ 0x520270
void TroopRec::level_up()
{
    // The original writes the running value back after every subtraction (the compiler only keeps those stores when the
    // location is volatile), so model it that way.
    volatile int* hp_max_p = &fa8;
    int v = level + 0x14;
    *hp_max_p = v;
    v -= attr(this, 0); *hp_max_p = v;
    v -= attr(this, 1); *hp_max_p = v;
    v -= attr(this, 2); *hp_max_p = v;
    v -= attr(this, 3); *hp_max_p = v;
    if (v < 0)
        *hp_max_p = 0;

    fa4 = level + 3;
    for (unsigned i = 0; i < 0x2a; i += 6)
        for (unsigned j = 0; j < 6; j++)
            fa4 -= get_skill(this, i + j);
    if (fa4 < 0)
        fa4 = 0;

    raise_stats();                                  // 0x5204d0
    raise_skills();                                 // 0x520420 (tail call)
}

// @ 0x520420
void TroopRec::raise_skills()
{
    int n = 0;
    for (;;) {
        int s = rand() % 0x2a;                      // 0x536a9f
        if (fa4 < 1 || get_skill(this, s) >= g_skill_tbl[s].cap) {
            if (fa4 <= 0 || ++n > 10000)
                break;
            continue;
        }
        set_skill(s, get_skill(this, s) + 1);
        fa4--;
    }
}

// @ 0x5204d0
void TroopRec::raise_stats()
{
    while (fa8) {
        int i = rand() % 4;                         // 0x536a9f
        set_attr(i, attr(this, i) + 1);
        fa8--;
    }
}

// @ 0x520520
void Animator::reset(void* a, void* b)
{
    unpack(a);
    int s3 = variant;
    float s128 = scale;
    float s12c = time;
    int s1 = item;
    int s0 = set;
    int s5 = material;

    float save[63];
    float* pf = weights;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++)
            save[i + k] = pf[k];
        pf += 9;
    }

    unpack(b);

    int r = rand();
    float t = (float)(item - s1) * ((float)(r % 15817) * 6.322311674011871e-05f);
    item = s1 + round_float(&t);

    r = rand();
    t = (float)(set - s0) * ((float)(r % 15817) * 6.322311674011871e-05f);
    set = s0 + round_float(&t);

    r = rand();
    t = (float)(material - s5) * ((float)(r % 15817) * 6.322311674011871e-05f);
    int v = round_float(&t) + s5;
    material = v;

    variant = -1;

    int base = *(int*)(cur * 0x1f8 + 0x1c0 + (int)chars);
    int* q = (int*)(base + 0x1c + v * 0xb0);
    int cnt = q[1];
    int m = (q[2] - cnt) >> 2;
    if (cnt == 0 || m < 1) {
        r = rand();
        t = (float)(variant - s3) * ((float)(r % 15817) * 6.322311674011871e-05f);
        variant = s3 + round_float(&t);
    } else {
        int k = (m < 2) ? 0 : rand() % m;
        variant = *(int*)(*(int*)(base + 0x20 + material * 0xb0) + k * 4);
    }

    r = rand();
    scale = (scale - s128) * ((float)(r % 15817) * 6.322311674011871e-05f) + s128;
    r = rand();
    time = (time - s12c) * ((float)(r % 15817) * 6.322311674011871e-05f) + s12c;

    pf = weights;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++) {
            g_rand_seed = ((g_rand_seed * 0x1c13 + 0xf2f67aaa) * 0x1ddcf5f + 0xf2f67aaa) * 0x1105;
            pf[k] = ((pf[k] - save[i + k]) * ((float)(g_rand_seed % 15817) * 1.2644623348023742e-04f)) * 0.5f + save[i + k];
        }
        pf += 9;
    }

    pack_state((PackedState*)a);
}

// ---- TextReader's slots (folded into text_stream's by the original's linker) ----
STREAM_FWD_TEXT_READER(TextReader)
