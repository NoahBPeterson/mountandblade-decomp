//-----------------------------------------------------------------------------
// File: game.cpp  (0x51c5a0..0x51ea?0)
//
// Desc: the game-state object: the "creating game" screen that builds it (0x51c5a0, 0x51c720), its initialisers
//       (init, reset, the 0x1a2b0-byte constructor), the map graph search used when a new map is launched
//       (0x51d2b0..0x51db70) and the save-game loaders (0x51dbd0..0x51ea10). Names provisional.
//       "T regime" region (docs/tu_map.md): cl emitted this object in source order, so the file keeps the binary
//       order and the helpers that follow their caller are __declspec(noinline) where the original calls them.
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include <stdio.h>
#include "./game.h"
#include "../game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)
#include "./nav_graph.h"
#include "./loading_panel.h"
#include "./externs.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../int_vec.h"
#include "../str_table.h"
#include "../view_state.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_recs.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/i64_vec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

#include "../../include/item_kind.h"      // g_item_kind_count 0x5e29d4 (canonical; the audit resolves it by this annotation)

#define MB_NOINLINE __declspec(noinline)    // (tools/difftest/slice.py reads the name before a '__declspec(' as the function name)

#include "../main_menu.h"                         // MainMenu (0xa73980): the one definition
#include "../map_flags.h"                                     // MapFlags (0x5e291c): the one definition
struct WindowMgrW;
extern WindowMgrW g_windows;                                // 0xa73980 (screen slot 15 = 0xa739bc: +0x14 cleared; slot 23 = 0xa739dc: +0x18 set)
extern int g_save_kind;                                     // 0x5e29bc
extern int g_a75eb8;                                        // 0xa75eb8

#include "../meta_mission.h"                                 // MetaMission (0xa73300): the one definition (init 0x4be742, entered through 0x4be730)

template<> void std::vector<int>::resize(size_t, int);       // 0x43f310 (agent3/agent_combat.cpp: the out-of-line instance, only declared)

// @ 0x51c5a0
void LoadingPanel::update(int arg)
{
    char tmp;
    tstring s;
    char buf[0x100];

    bool ok = sub(&counter, &f40, &s);
    _snprintf(buf, 0x100, "%d", counter);
    {
        float out[2];
        WOPS(text)->set_text(out, &s);
    }

    if (ok) {
        int m = mode;
        if (m == 0) {
            log_stream_ptr()->w_cstr("Switching to menu window...\n");
            g_game->f13aec = 0;
            *(int*)(*(char**)((char*)&g_windows + 0x3c) + 0x14) = 0;
            *(int*)(*(char**)((char*)&g_windows + 0x5c) + 0x18) = 0xf;
            (*(main_menu*)&g_windows).set_state(0x17);
            return;
        }
        if (m == 1) {
            (*(main_menu*)&g_windows).set_state(7);
            return;
        }
        if (m == 2) {
            g_game->f13aec = 2;
            g_map_flags.f8 = g_game->f13aec;
            g_map_flags.f0 = 1;
        } else {
            g_game->f13aec = 3;
            g_map_flags.f8 = g_game->f13aec;
            g_map_flags.f0 = 1;
        }
        g_map_flags.f2 = 0;
        g_map_flags.f1 = 0;
        g_map_flags.f4 = -1;
        g_map_flags.commit(&tmp);
    }
}

// @ 0x51c720
bool LoadingPanel::sub(int* counter, int* unused, tstring* out)
{
    (void)unused;
    bool r = false;
    out->assign("");
    int step = *counter;
    if (step == 0) {
        if (mode == 1) {
            bin_reader* sg = new bin_reader;
            save = sg;
            if (!((text_stream*)sg)->open_read(path.c_str()))
                mb_error("Unable to open found savegame!!!");
            header.read_slot(save);
            g_save_kind = header.f28;
        }
        *counter = *counter + 1;
        tstring k("ui_creating_game");
        out->assign(g_strtable.lookup(&k)->c_str());
        return r;
    }
    if (step == 1) {
        Game* g = new Game;
        g_game = g;
        g->reset();
        g_game->init(g_module_name, header.f34, mode == 2 || mode == 3);
        g_game->f1a05c = 1;
        *counter = *counter + 1;
        tstring k("ui_loading_setting_data");
        out->assign(g_strtable.lookup(&k)->c_str());
        return false;
    }
    if (step == 2) {
        g_game->load_game_data(g_a75eb8);   // 0x5236f0
        if (mode == 1) {
            Game* g = g_game;
            {
                tstring tmp(header.hero);
                troop_name(&g->troops[g->cur_troop]) = tmp;
            }
            g_game->title = header.title;
        }
        *counter = *counter + 1;
        tstring k("ui_loading_savegame_file");
        out->assign(g_strtable.lookup(&k)->c_str());
        return false;
    }
    if (step == 3) {
        if (mode == 1) {
            g_game->load_state(save);
            save->failed = 1;
            if (save) {
                ((mem_stream*)save)->mem_stream::~mem_stream();   // 0x417c90, called directly (the stream base destructor)
                ::operator delete(save);
            }
            save = 0;
        }
        *counter = *counter + 1;
        tstring k("ui_loading_map_file");
        out->assign(g_strtable.lookup(&k)->c_str());
        return false;
    }
    if (step == 4) {
        int m = mode;
        if (m == 0 || m == 2 || m == 3) {
            g_game->start_new_game();          // 0x51f1a0
            if (m == 2 || m == 3) {
                Game* g = g_game;
                tstring key("ui_player");
                tstring name(*g_strtable.lookup(&key));
                troop_name(&g->troops[g->cur_troop]) = name;
            }
        }
        if (mode != 2 && mode != 3)
            g_game->build_map();               // 0x520b20
        log_stream_ptr()->w_cstr("load_map_data complete.\n");
        *counter = *counter + 1;
        tstring k("ui_initializing_map");
        out->assign(g_strtable.lookup(&k)->c_str());
        return false;
    }
    if (step == 5) {
        if (mode != 2 && mode != 3)
            g_game->init_map();
        *counter = *counter + 1;
        tstring k("ui_launching_game");
        out->assign(g_strtable.lookup(&k)->c_str());
        log_stream_ptr()->w_cstr("Init_map complete.\n");
        return false;
    }
    if (step == 6) {
        ((MetaMission*)0xa73300)->init();
        log_stream_ptr()->w_cstr("init_meta_mission complete.\n");
        if (mode != 2 && mode != 3) {
            g_game->build_scene();             // 0x520d60
            g_new_game_done = 1;
        }
        log_stream_ptr()->w_cstr("launch complete.\n");
        *counter = *counter + 1;
        tstring k("ui_finished");
        out->assign(g_strtable.lookup(&k)->c_str());
        return false;
    }
    return true;
}

// @ 0x51cc10
void Game::init(const tstring& name, int value, char flag)
{
    kind = value;
    flag18 = flag;
    name1c = name;
    cur_troop = 0;
    hero_troop = 1;
    f78 = 0;
    f84 = 0;
    f88 = 1;
    f80 = 2;
    f98 = 0;
    f9c = 0;
    fa0 = 0;
    fa4 = 0;
}

// @ 0x51cc80
void Game::reset()
{
    int v[4];

    kind = -1;
    title.assign("");
    f84 = f78;
    f1a0d4 = 2.0f;
    f1a0dc = 2.0f;
    f1a0d8 = 2.0f;
    f1a0f8 = f1a0fc = f1a100 = 1.0f;
    f1a104 = v[3];
    flag18 = 0;
    f95 = 0;
    f7c = 1;
    f88 = 1;
    f13acc = 1.0f;
    f13aa0 = 0;
    f13aa4 = 1;
    f14058 = 0;
    f13ac8 = 0;
    f2c = 0;
    f44 = 0;
    f139e4 = 0;
    f13a3c = 0;
    f13a44 = -1;
    f13a40 = -1;
    f13b60 = -1;
    f13ae8 = 0;
    fb4 = 1;
    f1a060 = 0;
    f1a064 = 0;
    f1a090 = 0;
    f1a094 = 0;
    f1a0b0 = 1;
    f1a0b4 = 0;
    f1a1d0 = 0;
    f1a098 = 0;
    f1a0cc = 0;
    f1a0d0 = 0;
    f1a0b8 = -1;
    f1a0bc = -1;
    f1a0c8 = 0;
    f1a0c0 = -1;
    f1a0c4 = -1;
    f1a160 = 0;
    f1a0e0 = 1.0f;
    f1a0e8 = 1.0f;
    f1a0e4 = 1.0f;
    f1a0ec = 1.0f;
    f1a0f4 = 1.0f;
    f1a0f0 = 1.0f;
    f1a108 = 0;
    f1a1b4 = -1;
    {
        void* p = buf1a1bc.p;
        if (p)
            ::operator delete(p);
    }
    buf1a1bc.p = 0;
    buf1a1bc.b = 0;
    buf1a1bc.c = 0;
    f13a34 = 1;
    f13af0 = -1;
    f13af4 = -1;
    f13b04 = -1;
    f13b08 = -1;
    f13b0c = -1;
    f13a38 = 1.0f;
    fac = -1;
    f1a220 = -1;
    f1a1c8 = 0;
    f1a1cc = 0;
    f1a1d4 = 0;
    f13af8 = 0;
    f13afc = 0;
    f13b00 = 0;
    f13a08 = 0;
    f13a0c = -100000;
    f13a10 = -100000;
    f13a18 = 0;
    f13a14 = 0;
    f13a24 = 0;
    f13a28 = 0;
    f13a2c = 0;
    f13ae0 = 0;
    f13ae4 = 100;
    f13ad0 = -10.0f;
    f13ad4 = 0;
    f13ad8 = 0;
    f13adc = 0;
    f94 = 0;
    f13a20 = 20;
    fa8 = 0;
    f13b3c = 0.5f;
    f13b40 = 0.5f;
    f13b44 = 100.0f;
    f13b48 = 0;
    f13b4c = (float)g_ticks[0] * g_tick_scale;
    f13aec = -1;
    f1a1f0 = -1;
    f1a1d8 = 0;
    f1a1dc = 0;
    f1a1e0 = 0;
    f1a1e4 = 0;
    f1a1e8 = 0;
    f1a1ec = 0;
    f1a20c = 0;
    fc8 = 0;
    f1a208 = -1;
    f1a238 = 0;
    f1a23c = 0;

    tstring* p = names1a240;
    for (int i = 0; i < 5; i++) {
        p->clear();
        flags1a290[i] = 0;
        p++;
    }

    int idx = timer13a90.clock;
    timer13a90.t = g_ticks[idx];
}

// @ 0x51cfa0
Game::Game()
    : timer13a80(1), timer13a90(1), timer13b18(1), timer1a150(0), timer1a298(1)
{
    f1a05c = 0;
    reset();
    f1a060 = 0;
    f1a064 = 0;
    f1a090 = 0;
    f1a160 = 0;
    factions = 0;
    troops = 0;
    parties = 0;
    quests = 0;
}

// @ 0x51d1c0
MB_NOINLINE bool TrackVec::buy(unsigned count)
{
    _Myfirst = 0;
    _Mylast = 0;
    _Myend = 0;
    if (count == 0)
        return false;
    if (count > 0x2c8590b) {
        _Xlen();
        return true;
    }
    TrackRec* p = (TrackRec*)operator new(count * 0x5c);
    _Myend = p + count;
    _Myfirst = p;
    _Mylast = p;
    return true;
}

// @ 0x51d220
MB_NOINLINE SpriteTable::SpriteTable()
{
    count = 0;
    f24 = 0;
    f28 = 0;
}

// @ 0x51d250
MB_NOINLINE Scene::Scene() : list(0)
{
    int i = 0;
    do {
        cells[i * 2] = -1;
        cells[i * 2 + 1] = -1;
        i++;
    } while (i < 0x2717);
    f138b8 = 0;
    f138dc = 0;
    f138e0 = 0;
}

// the map objects reached through globals
struct SceneInit { int Init(int a, int b, int* c, int* d, int* e); };      // 0x478ad0 (this = the new 0x344-byte scene)
struct MapReset { void run(); };                                            // 0x4704b0 (this = g_nav)
struct MapBuild { void build(int world); };                                 // 0x479980 (this = g_level)
extern MapBuild* g_level;                                                   // 0xa732b4

// @ 0x51d2b0
void Game::init_map()
{
    struct Tri { int a, b, c; };
    Tri t;

    if (g_new_game_done != 0)
        return;
    ((MapReset*)g_nav)->run();
    void* mem = operator new(0x344);
    if (mem != 0) {
        t.b = 5;
        t.c = 5;
        t.a = 0;
        g_level = (MapBuild*)((SceneInit*)mem)->Init((int)g_nav + 0x44, (int)g_nav + 0x54, &t.c, &t.b, &t.a);
    } else {
        g_level = 0;
    }
    t.c = 1;
    t.b = 0;
    t.a = 7;
    g_nav->score_triggers(&t.a, &t.b, &t.c);
    t.c = 7;
    t.b = 0;
    t.a = 7;
    g_nav->solve(&t.a, &t.b, &t.c);
    g_level->build((int)g_nav);
}

// @ 0x51d3a0
// Zone numbering over the node graph: nodes whose class mask equals *arg3 are the 0x5f5e0ff "blocked" zone; the
// others are flood-filled into numbered zones (find_trigger decides what a blocked neighbour contributes) and two
// zones met through one node are merged. The special zone's members are then walked.
void NavGraph::solve(int* arg1, int* arg2, int* arg3)
{
    std::vector<NavNode>& nv = vec();
    int zone = -1;
    unsigned i;
    for (i = 0; i < nv.size(); i++) {
        nv[i].cost = -1;
        nv[i].prev = -1;
        nv[i].next = -1;
    }
    int* stack = new int[nv.size()];
    int* path = new int[nv.size()];
    int top = -1;
    for (int ctr = 0; (unsigned)ctr < nv.size(); ctr++) {
        int cur = ctr;
        if (top >= 0) {
            cur = stack[top];
            ctr--;
            top--;
        }
        if (nv[cur].cost >= 0)
            continue;
        if (nv[cur].mask == *arg3) {
            nv[cur].cost = 0x5f5e0ff;
            for (int k = 0; k < nv[cur].count; k++) {
                int nb = nv[cur].nb[k];
                if (nb >= 0 && nv[nb].cost < 0)
                    stack[++top] = nb;
            }
        } else {
            int comp = -1;
            for (int k = 0; k < nv[cur].count; k++) {
                int nb = nv[cur].nb[k];
                if (nb < 0)
                    continue;
                int res = -1;
                if (nv[nb].mask == *arg3) {
                    int cnt = 0;
                    path[0] = cur;
                    res = find_trigger(&nb, &cur, path, &cnt, (unsigned*)arg1, (unsigned*)arg2, arg3);
                }
                if (!(nv[cur].weight[k] < 1e9f) && nv[nb].mask != *arg3)
                    continue;
                if (res < 0 && nv[nb].mask != *arg3)
                    res = nv[nb].cost;
                if (nv[nb].cost < 0)
                    stack[++top] = nb;
                if (comp < 0) {
                    comp = res;
                    continue;
                }
                if (res < 0 || comp == res)
                    continue;
                zone--;
                int hi = comp;
                if (comp > res)
                    comp = res;
                else
                    hi = res;
                for (i = 0; i < nv.size(); i++) {
                    int c = nv[i].cost;
                    if (c > hi) {
                        if (c != 0x5f5e0ff)
                            nv[i].cost = c - 1;
                    } else if (c == hi) {
                        nv[i].cost = comp;
                    }
                }
            }
            if (comp < 0)
                comp = ++zone;
            nv[cur].cost = comp;
        }
    }
    for (i = 0; i < nv.size(); i++) {
        if (nv[i].cost == 0x5f5e0ff) {
            int cnt = 0;
            *path = (int)i;
            int ii = (int)i;
            walk(&ii, &ii, path, &cnt);
        }
    }
    delete[] stack;
    delete[] path;
}

// @ 0x51d7c0
MB_NOINLINE int* NavGraph::walk(int* a, int* idx, int* seen, int* n)
{
    int* base = (int*)nodes;
    int* node = (int*)(*idx * 0x88 + (int)base);
    int local_4 = 0;
    if (*node < 1)
        return node;
    int local_8 = 0x28;
    for (;;) {
        int child = *(int*)(local_8 + (int)node);
        int num = *n;
        int k = 0;
        bool skip = false;
        if (num >= 0) {
            do {
                if (child == *(int*)((char*)seen + k * 4)) {
                    skip = true;
                    break;
                }
                k++;
            } while (k <= *n);
        }
        if (!skip && child >= 0) {
            int* c = (int*)(child * 0x88 + (int)base);
            if (c[0x48 / 4] == 0x5f5e0ff) {
                *n = num + 1;
                *(int*)((char*)seen + (num + 1) * 4) = *idx;
                walk(a, &child, seen, n);
                *n = num;
                if (*(int*)(*a * 0x88 + 0x50 + (int)nodes) != -1)
                    return (int*)(*a * 0x88 + (int)nodes);
            } else {
                int* s = (int*)((int)nodes + *a * 0x88);
                if (s[0x4c / 4] == -1) {
                    s[0x4c / 4] = c[0x48 / 4];
                } else if (s[0x4c / 4] != c[0x48 / 4]) {
                    s[0x50 / 4] = c[0x48 / 4];
                    return (int*)(child * 0x88 + (int)nodes);
                }
            }
        }
        base = (int*)nodes;
        local_8 += 4;
        node = (int*)(*idx * 0x88 + (int)base);
        local_4++;
        if (*(int*)(*idx * 0x88 + (int)base) <= local_4)
            return node;
    }
}

// @ 0x51d920    MB_NOINLINE int NavGraph::find_trigger(...)   (thiscall, ret 0x1c)
// Walks the trigger graph from the record *idp: `arr`/`*cnt` is the visited stack, m1/m2 the flag masks and
// *flag the flag that decides whether to recurse.
MB_NOINLINE int NavGraph::find_trigger(int* idp, int* a2, int* arr, int* cnt, unsigned* m1, unsigned* m2, int* flag)
{
    char* f14 = (char*)nodes;
    if (*(int*)(f14 + *idp * 0x88) < 1)
        return -1;
    int off = 0x28;
    for (int i = 0; i < *(int*)(f14 + *idp * 0x88); i++, off += 4) {
        int v = *(int*)(f14 + *idp * 0x88 + off);
        int n = *cnt;
        if (n >= 0) {
            for (int k = 0; k <= *cnt; k++)
                if (v == arr[k])
                    goto next;
        }
        if (v >= 0) {
            char* r2 = f14 + v * 0x88;
            if (*(int*)(r2 + 0x48) >= 0 || *(int*)(r2 + 0x24) == *flag) {
                if (*(int*)(r2 + 0x24) == *flag) {
                    *cnt = n + 1;
                    arr[n + 1] = *idp;
                    int res = find_trigger(&v, a2, arr, cnt, m1, m2, flag);
                    *cnt = n;
                    if (res >= 0)
                        return res;
                } else {
                    unsigned x = *m1;
                    unsigned y = *m2;
                    char* r3 = f14 + *a2 * 0x88;
                    if (((*(unsigned*)(r3 + 0x24) & x) == y && (*(unsigned*)(r2 + 0x24) & (x == y)))
                        || ((*(unsigned*)(r3 + 0x24) & x) != y
                            && (*(unsigned*)(r2 + 0x24) & (x != y)))) {
                        if (*(int*)(r2 + 0x48) >= 0)
                            return *(int*)(f14 + v * 0x88 + 0x48);
                    }
                }
            }
        }
next:
        ;
    }
    return -1;
}

// @ 0x51dac0    void NavGraph::score_triggers(int* a, int* b, int* c)   (thiscall, ret 0xc)
// For every trigger record and every neighbour slot that is set, stores the compatibility score.
void NavGraph::score_triggers(int* a, int* b, int* c)
{
    int i = 0;
    int off = 0;
    while ((char*)nodes) {
        if ((unsigned)i >= (unsigned)(((char*)nodes_end - (char*)nodes) / 0x88))
            break;
        for (int k = 0; k < *(int*)((char*)nodes + off); k++) {
            if (*(int*)((char*)nodes + off + 0x28 + k * 4) != -1)
                *(float*)((char*)nodes + off + 0x38 + k * 4) =
                    trigger_score(i, k, *a, *b, *c);
        }
        i++;
        off += 0x88;
    }
}

// @ 0x51db70    MB_NOINLINE float NavGraph::trigger_score(int row, int idx, unsigned a, unsigned b, unsigned c)   (ret 0x14)
MB_NOINLINE float NavGraph::trigger_score(int row, int idx, unsigned a, unsigned b, unsigned c)
{
    char* rec = (char*)nodes + row * 0x88;
    char* oth = (char*)nodes + *(int*)(rec + 0x28 + idx * 4) * 0x88;
    unsigned m = *(unsigned*)(rec + 0x24) & a;
    unsigned n = *(unsigned*)(oth + 0x24) & a;
    if (m == c)
        return 1000000000.0f;
    if (n == c)
        return 1000000000.0f;
    if (m == b) {
        if (n != b)
            return 1000000000.0f;
    } else if (n == b)
        return 1000000000.0f;
    return 2.0f;
}


// @ 0x51dbd0
void Game::load_state(stream_base* s)
{
    unsigned __int64 tick;
    s->r_u64(&tick);
    g_tick1_hi = *((int*)&tick + 1);
    ((int*)g_ticks)[2] = *(int*)&tick;
    s->r_int(&f4);
    s->r_int(&fb4);
    load_clock(s);
    g_list_a70480.read(s);
    triggers.read(s);
    simple.read(s);
    load_quests(s);
    load_parties(s);
    load_factions(s);
    load_tracks(s);
    load_party_templates(s);
    load_scene_parties(s);
    sprites.read(s, 0);
    load_troops(s);
    load_items(s);
    s->r_u64((unsigned __int64*)((char*)&troops[cur_troop] + 0x3c0));
    s->r_u64((unsigned __int64*)((char*)&troops[cur_troop] + 0x3c8));
    s->r_int(&f98);
    s->r_int(&fa0);
    s->r_int(&f9c);
    s->r_int(&fa4);
}

// @ 0x51dd60
void TriggerTag::read(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n > count)
        mb_error("Load error: Number of triggers in file: %d, exceeds %d", n, count);
    for (int i = 0; i < n; i++)
        array[i].read(s);
}

// @ 0x51ddc0
MB_NOINLINE void TriggerRec::read(stream_base* s)
{
    s->r_int(&f0c);
    s->r_i64(&f50);
    s->r_i64(&f60);
    s->r_i64(&f70);
}

// @ 0x51de00
void SimpleTag::read(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n > count)
        mb_error("Load error: Number of simple triggers in file: %d, exceeds %d", n, count);
    for (int i = 0; i < n; i++)
        s->r_i64(&array[i].f28);
}

// @ 0x51de70
void Game::load_clock(stream_base* s)
{
    s->r_i64(&timer13b18.t);
    s->r_int(&f13b28);
    s->r_int(&f13b2c);
    s->r_int(&f13b30);
    s->r_int(&f13b34);
    s->r_int(&f13b38);
    s->r_int(&f13b60);
    s->r_float(&f13b3c);
    s->r_float(&f13b40);
    s->r_float(&f13b44);
    s->r_float(&f13b48);
    s->r_tstr(rs(&t13a70));
    s->r_bool(&f13aa0);
    s->r_int(&f13ae8);
    s->r_int(&f13a20);
    s->r_int(&f13a0c);
    s->r_int(&f13a10);
    s->r_int(&f13aec);
    s->r_int(&f13af4);
    s->r_int(&f13af8);
    s->r_int(&f13afc);
    s->r_int(&f13ae0);
    s->r_int(&f13ae4);
    tstring& ts = t13b50;
    s->r_tstr(rs(&ts));
    ts.underscores_to_spaces();
    if (ts.equals("-1"))
        ts.assign("");
    s->r_int(&f1a1d8);
    s->r_int(&f1a1dc);
    s->r_int(&f1a1e0);
    s->r_int(&f1a1e4);
    s->r_int(&f1a1e8);
    s->r_int(&f1a1ec);
    s->r_i64(&timer13a80.t);
    s->r_float(&f13ad0);
    s->r_int(&f13adc);
    s->r_int(&f13ad8);
    s->r_int(&f13ad4);
}

// @ 0x51e0b0
MB_NOINLINE void I64Vec::read(stream_base* s)
{
    int n;
    s->r_int(&n);
    resize((unsigned)n, 0);
    for (int i = 0; i < n; i++)
        s->r_i64(&_Myfirst[i]);
}

// @ 0x51e100
void Game::load_quests(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n > nquests)
        mb_error("Load error: Number of quests in file: %d, exceeds %d", n, nquests);
    for (int i = 0; i < n; i++)
        quests[i].read(s);
}

// @ 0x51e170
void QuestRec::read(stream_base* s)
{
    s->r_int(&f64);
    s->r_int(&giver_troop);
    s->r_int(&number);
    s->r_float(&f11c);
    s->r_tstr(rs(&text));
    s->r_tstr(rs(&title));
    s->r_tstr(rs(&giver));
    read_soa(s, (SoaBlock*)note_times);
    list.read(s);
}

// @ 0x51e1f0
stream_base* read_soa(stream_base* s, SoaBlock* b)
{
    for (int i = 0; i < 8; i++) {
        s->r_tstr(rs(&b->names[i]));
        s->r_int(&b->vals[i]);
        s->r_int(&b->total);
    }
    return s;
}

// @ 0x51e240
void Game::load_parties(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n != nparties)
        mb_error("Load error: Number of scenes in file: %d, exceeds %d", n, nparties);
    for (int i = 0; i < nparties; i++)
        ((I64Vec*)(parties + i * 0x494 + 0x474))->read(s);
}

// @ 0x51e2b0
void Game::load_factions(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n > nfactions)
        mb_error("Load error: Number of factions in file: %d, exceeds %d", n, nfactions);
    for (int i = 0; i < n; i++)
        factions[i].read(s, n);
}

// @ 0x51e320
MB_NOINLINE void Faction::read(stream_base* s, int n)
{
    list.read(s);
    for (int i = 0; i < n; i++)
        s->r_float(&rel[i]);
    s->r_tstr(rs(&name));
    s->r_bool(&f10);
    s->r_uint(&f28);
    s->r_int(&f230);
    read_soa(s, (SoaBlock*)note_times);
}

// @ 0x51e3a0
void Game::load_party_templates(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n > g_party_template_count)
        mb_error("Load error: Number of party_templates in file: %d, exceeds %d", n, g_party_template_count);
    for (int i = 0; i < n; i++)
        g_party_templates[i].read(s);
}

// @ 0x51e410
MB_NOINLINE void PartyTemplate::read(stream_base* s)
{
    s->r_int(&f34);
    s->r_int(&f38);
    s->r_int(&f3c);
    list.read(s);
}

// @ 0x51e450
void Game::load_items(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n < 0 || n >= g_item_kind_count + 1)
        mb_error("Load error: Invalid number of item kinds: %d", n);
    for (int i = 0; i < n; i++)
        ((I64Vec*)((char*)&g_items[i] + 0x30))->read(s);
}

// @ 0x51e4c0
void Game::load_troops(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n < 0 || n >= ntroops + 1)
        mb_error("Load error: Invalid number of troops: %d", n);
    for (int i = 0; i < n; i++)
        ((TroopIO*)&troops[i])->read(s);
    char* p = g_troop_extra;
    do {
        s->r_int((int*)p);
        p += 0x44;
    } while ((int)p < (int)0x8ee3c60);
}

// @ 0x51e540
MB_NOINLINE void TroopIO::read(stream_base* s)
{
    char* p = (char*)this;
    ((I64Vec*)(p + 0x4f4))->read(s);
    for (int i = 0; i < 4; i++)
        s->r_int((int*)(p + 0x58) + i);
    for (int i = 0; i < 7; i++)
        s->r_float((float*)(p + 0x80) + i);
    for (int i = 0; i < 6; i++)
        s->r_uint((unsigned*)(p + 0x68) + i);
    s->r_u64((unsigned __int64*)(p + 0x30)).r_int((int*)(p + 0x3b8));
    s->r_int((int*)(p + 0x3b4)).r_int((int*)(p + 0x9c));
    s->r_float((float*)(p + 0xa0));
    s->r_int((int*)(p + 0x4c));
    s->r_int((int*)(p + 0xa4)).r_int((int*)(p + 0xa8)).r_int((int*)(p + 0xac));
    s->r_int((int*)(p + 0xb0));
    ((ItemPairs*)(p + 0xb4))->read(s);
    int* q = (int*)(p + 0x4a4);
    for (int i = 0; i < 10; i++) {
        s->r_int(q);
        s->r_int(q + 1);
        q += 2;
    }
    for (int i = 0; i < 4; i++)
        s->r_u64((unsigned __int64*)(p + 0x3c0 + i * 8));
    read_soa(s, (SoaBlock*)(p + 0x400));
}

// @ 0x51e690
MB_NOINLINE void ItemPairs::read(stream_base* s)
{
    for (int i = 0; i < 0x60; i++) {
        s->r_int(&pair[i].item);
        s->r_int((int*)&pair[i].m4);
    }
}

// @ 0x51e6c0
void Game::load_tracks(stream_base* s)
{
    log_stream_ptr()->w_cstr("Loading tracks\n");
    int n;
    TrackRec val;
    s->r_int(&n);
    tracks.resize(n, val);
    for (int i = 0; i < n; i++) {
        TrackRec* rec = tracks._Myfirst + i;
        rec->read(s);
    }
}

// @ 0x51e750
MB_NOINLINE void TrackRec::read(stream_base* s)
{
    s->r_float(&f0).r_float(&f4).r_float(&f8);
    s->r_float(&f10);
    s->r_float(&f14).r_int(&f18);
}

// @ 0x51e7a0
MB_NOINLINE void TrackVec::resize(unsigned n, TrackRec val)
{
    if ((unsigned)size() < n) {
        insert_n(_Mylast, n - size(), &val);
        return;
    }
    if (_Myfirst && n < (unsigned)size()) {
        TrackRec* it;
        shift_over(&it, _Myfirst + n, _Mylast);
    }
}

// @ 0x51e840
void SpriteTable::read(stream_base* s, int arg)
{
    if (count)
        clear();
    int n;
    s->r_int(&n);
    s->r_int(&f24);
    for (int i = 0; i < n; i++)
        alloc_id();
    clear();
    ((std::vector<int>*)&free_ids)->resize(0, 0);
    for (int i = 0; i < n; i++) {
        SpriteRec* e = at(i);
        int flag;
        s->r_int(&flag);
        e->live = flag;
        if (flag) {
            s->r_int(&e->f0c);
            e->read(s, arg);
        } else {
            free_ids.push_back(i);
        }
    }
}

// @ 0x51e970
MB_NOINLINE void SpriteRec::read(stream_base* s, int arg)
{
    float* p;
    stream_base* rs_;
    s->r_tstr(rs(&name14));
    s->r_int(&f10);
    p = v24;
    rs_ = &s->r_float(p);
    rs_->r_float(++p);
    p = v2c;
    rs_ = &s->r_float(p);
    rs_->r_float(++p);
    p = v34;
    rs_ = &s->r_float(p);
    rs_->r_float(++p);
    s->r_int(&f54);
    s->r_int(&f58);
    s->r_i64(&f68);
    s->r_float(&f70);
}

// @ 0x51ea10
MB_NOINLINE void SpriteTable::clear()
{
    ((std::vector<int>*)&free_ids)->resize(0, 0);
    for (int i = 0; i < count; i++) {
        SpriteRec* e = at(i);
        e->live = 0;
        free_ids.push_back(i);
    }
}

// @ 0x51eaa0
// The scene's party record of Game::f78 (0x5ac8-byte records, 16 per page; the page table is the first pointer of
// scene.list, +0x138d0): its count at +0x8a8, entries of 0x20 bytes from +0xc0 (troop id at -0x14, a float at +0,
// an int at -4), then 0x20 ints per entry into scene_parties (+0x1405c, 0x80 bytes per entry).
// The slices read slot 0x38 as r_int and 0x4c as r_raw; they are r_float and r_int (stream_base slots 14 and 19).
void Game::load_scene_parties(stream_base* s)
{
    scene.read(s, 1);
    int idx = scene.index_of(f78);
    char** pages = *(char***)(scene.list.b + 4);
    char* rec = pages[idx >> 4] + (idx & 0xf) * 0x5ac8;
    for (int i = 0; i < *(int*)(rec + 0x8a8); i++) {
        char* e = rec + 0xc0 + i * 0x20;
        if ((troops[*(int*)(e - 0x14)].flags30 & 0x10) == 0) {
            s->r_float((float*)e);
            s->r_int((int*)(e - 4));
            for (int k = 0; k < 0x20; k++)
                s->r_int((int*)(scene_parties + i * 0x80) + k);
        }
    }
}
