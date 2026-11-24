#define TRACKREC_VEC_INSTANTIATED_HERE   // game_state_load.h: vector<TrackRec>::_Buy 0x51d1c0 and resize 0x51e7a0 are this file's
//-----------------------------------------------------------------------------
// File: game_state_load.cpp  (0x51c5a0..0x51ea?0)
//
// Desc: the game-state object: the "creating game" screen that builds it (0x51c5a0, 0x51c720), its initialisers
//       (init, reset, the 0x1a2b0-byte constructor), the map graph search used when a new map is launched
//       (0x51d2b0..0x51db70) and the save-game loaders (0x51dbd0..0x51ea10). Names provisional.
//       "T regime" region (docs/tu_map.md): cl emitted this object in source order, so the file keeps the binary
//       order and the helpers that follow their caller are __declspec(noinline) where the original calls them.
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "platform/image_addrs.h"
#include "game/data/skill_cap.h"
#include <stdio.h>
#include "game/campaign/game_state_load.h"
#include "game/common/game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)
#include "game/terrain/nav_data.h"          // NavData: the map object g_script.nav (its zone numbering 0x51d3a0..0x51db70 is defined here)
#include "engine/scene/world.h"         // rgl::world: ctor 0x478ad0, build_faces 0x479980 (NavData: the map object g_script.nav)
#include "game/data/raw_place.h"       // construct in raw storage
#include "engine/core/tstring.h"
#include "engine/io/stream.h"
#include "game/ui/screens/screen_records.h"        // C4c2fb0: the "creating game" panel (slot 3 is update 0x51c5a0, sub 0x51c720)
#include "game/campaign/externs.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include <vector>
#include "game/data/str_table.h"
#include "game/agent/ai/agent_ai_game.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "game/data/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/game_recs.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/terrain/scene_file.h"   // SceneFile: the scene records whose slots load_parties reads (without stream_inline.h)

#include "game/data/item_kind_include.h"      // g_item_kind_count 0x5e29d4 (canonical; the audit resolves it by this annotation)

#define MB_NOINLINE __declspec(noinline)    // (tools/difftest/slice.py reads the name before a '__declspec(' as the function name)

#include "game/ui/window_mgr.h"                         // g_windows (0xa73980)
#include "game/campaign/map_flags.h"                                     // MapFlags (0x5e291c): the one definition
#include "game/ui/window_mgr.h"                                 // g_windows (0xa73980; screen slot 15 = 0xa739bc: +0x14 cleared; slot 23 = 0xa739dc: +0x18 set)
#include "game/common/level_global.h"                     // g_level (0xa732b4)
#include "game/data/rec34.h"                 // g_scripts (0xa75eb0): .success is the extra-triggers argument of load_game_data
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object
template<> bool std::vector<Party*, std::allocator<Party*> >::_Buy(size_t);   // 0x43e760 (the _Buy instance every pointer vector shares, agent3/agent_combat.cpp: the party manager's page list is built by a call to it)

// ---- data this module owns (docs/standalone_data.md) ----
int g_save_kind;                                            // 0x5e29bc (.bss) kind of the loaded save header

#include "game/mission/mission_ctx_rec.h"          // MissionCtx (0xa73300): init is 0x4be730 (the marker entry)
#include "platform/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers

template<> void std::vector<__int64>::resize(size_t, __int64);   // 0x493090 (game/campaign/game_data_party_quest.cpp: the out-of-line instance, only declared)
template<> void std::vector<int>::resize(size_t, int);       // 0x43f310 (agent3/agent_combat.cpp: the out-of-line instance, only declared)


// @ 0x51c5a0
void C4c2fb0::frame_update(float)
{
    char tmp;
    tstring s;
    char buf[0x100];

    bool ok = sub(&counter, &f40, &s);
    _snprintf(buf, 0x100, "%d", counter);
    hint->set_text(s);

    if (ok) {
        int m = w04;
        if (m == 0) {
            log_stream_ptr()->w_cstr("Switching to menu window...\n");
            g_game->current_menu = 0;
            slot_as<head_rec>(15)->w14 = 0;           // the character sheet: troop 0
            slot_as<head_rec>(23)->w18 = 0xf;         // the game menu returns to the character sheet
            g_windows.set_state(0x17);
            return;
        }
        if (m == 1) {
            g_windows.set_state(7);
            return;
        }
        if (m == 2) {
            g_game->current_menu = 2;
            g_map_flags.f8 = g_game->current_menu;
            g_map_flags.f0 = 1;
        } else {
            g_game->current_menu = 3;
            g_map_flags.f0 = 1;
            g_map_flags.f8 = g_game->current_menu;
        }
        g_map_flags.f2 = 0;
        g_map_flags.f1 = 0;
        g_map_flags.f4 = -1;
        g_map_flags.commit(&tmp);
    }
}

// 0x51c720: the "ui_player" name assignment was an inlined by-value helper (its parameter temp sits in sub's
// lower temp slot, the key in the upper one).
static __forceinline void set_troop_name(Troop* t, tstring s)
{
    troop_name(t) = s;
}

// @ 0x51c720
bool C4c2fb0::sub(int* counter, int* unused, tstring* out)
{
    (void)unused;
    bool r = false;
    out->assign("");
    int step = *counter;
    if (step == 0) {
        if (w04 == 1) {
            bin_reader* sg = new bin_reader;
            save = sg;
            if (!sg->open_read(path.c_str()))
                mb_error("Unable to open found savegame!!!");
            header.read_slot(save);
            g_save_kind = header.year;
        }
        *counter = *counter + 1;
        tstring t0("ui_creating_game");
        out->assign(g_strtable.lookup(t0)->c_str());
        goto done;
    }
    r = true;
    if (step == 1) {
        Game* g = new Game;
        g_game = g;
        g->reset();
        static_cast<Game*>(g_game)->init(g_module_name, f8c, w04 == 2 || w04 == 3);
        g_game->scene_open = 1;
        *counter = *counter + 1;
        tstring t1("ui_loading_setting_data");
        out->assign(g_strtable.lookup(t1)->c_str());
        return false;
    }
    if (step == 2) {
        static_cast<Game*>(g_game)->load_game_data(g_scripts.success);   // 0x5236f0
        if (w04 == 1) {
            {
                tstring tmp(header.body);
                g_game->troops[g_game->player].name = tmp;
            }
            g_game->title = header.title;
        }
        *counter = *counter + 1;
        {
            tstring t2("ui_loading_savegame_file");
            out->assign(g_strtable.lookup(t2)->c_str());
        }
        return false;
    }
    if (step == 3) {
        if (w04 == 1) {
            static_cast<Game*>(g_game)->load_state(save);
            save->failed = 1;
            delete save;
            save = 0;
        }
        *counter = *counter + 1;
        {
            tstring t3("ui_loading_map_file");
            out->assign(g_strtable.lookup(t3)->c_str());
        }
        return false;
    }
    if (step == 4) {
        int m = w04;
        if (m == 0 || m == 2 || m == 3) {
            static_cast<Game*>(g_game)->start_new_game();          // 0x51f1a0
            if (w04 == 2 || w04 == 3) {      // re-read after the call (0x51ca17)
                tstring key("ui_player");
                set_troop_name(&g_game->troops[g_game->player], *g_strtable.lookup(key));
            }
        }
        if (w04 != 2 && w04 != 3)
            static_cast<Game*>(g_game)->build_map();               // 0x520b20
        log_stream_ptr()->w_cstr("load_map_data complete.\n");
        *counter = *counter + 1;
        {
            tstring t4("ui_initializing_map");
            out->assign(g_strtable.lookup(t4)->c_str());
        }
        return false;
    }
    if (step == 5) {
        if (w04 != 2 && w04 != 3)
            static_cast<Game*>(g_game)->init_map();
        *counter = *counter + 1;
        {
            tstring t5("ui_launching_game");
            out->assign(g_strtable.lookup(t5)->c_str());
        }   // the key dies before the log line (0x51cb4a, then the call at 0x51cb5e)
        log_stream_ptr()->w_cstr("Init_map complete.\n");
        return false;
    }
    if (step == 6) {
        g_mission_ctx.init();
        log_stream_ptr()->w_cstr("init_meta_mission complete.\n");
        if (w04 != 2 && w04 != 3) {
            static_cast<Game*>(g_game)->build_scene();             // 0x520d60
            g_script.new_game_done = 1;
        }
        log_stream_ptr()->w_cstr("launch complete.\n");
        *counter = *counter + 1;
        {
            tstring t6("ui_finished");
            out->assign(g_strtable.lookup(t6)->c_str());
        }
        return false;
    }
done:
    return r;
}

// @ 0x51cc10
void Game::init(const tstring& name, int value, char flag)
{
    version = value;
    flag18 = flag;
    module = name;
    player = 0;
    hero_troop = 1;
    main_party = 0;
    sel_party = 0;
    map_live = 1;
    f80 = 2;
    cnt_killed = 0;
    cnt_killed_friend = 0;
    cnt_ko = 0;
    cnt_ko_friend = 0;
}

// @ 0x51cc80
void Game::reset()
{
    float v[4];

    version = -1;
    title.assign("");
    sel_party = main_party;
    editor.mx = 2.0f;
    editor.mz = 2.0f;
    editor.my = 2.0f;
    editor.col.x = 1.0f;
    editor.col.y = 1.0f;
    editor.col.z = 1.0f;
    flag18 = 0;
    reveal_all = 0;
    f7c = 1;
    map_live = 1;
    f13acc = 1.0f;
    f13aa0 = 0;
    f13aa4 = 1;
    f14058 = 0;
    f13ac8 = 0;
    map_track = 0;
    map_obj = 0;
    f139e4 = 0;
    start_map_conversation = 0;
    map_conversation_troop = -1;
    map_conversation_mode = -1;
    f13b60 = -1;
    f13ae8 = 0;
    fb4 = 1;
    debugger.enabled = 0;
    debugger.shown = 0;
    editor.active = 0;
    editor.mode = 0;
    editor.cam_step = 1;
    editor.cam_step_time = 0;
    editor.sub_mode = 0;
    editor.rt_flag = 0;
    editor.opt_cc = 0;
    editor.opt_d0 = 0;
    editor.list_top = -1;
    editor.list_sel = -1;
    editor.flag_c8 = 0;
    editor.f1a0c0 = -1;
    editor.f1a0c4 = -1;
    editor.dialog_shown = 0;
    editor.sx = 1.0f;
    editor.sz = 1.0f;
    editor.sy = 1.0f;
    editor.tx = 1.0f;
    editor.tz = 1.0f;
    editor.col.w = v[3];
    editor.ty = 1.0f;
    editor.axis = 0;
    editor.sel_index = -1;
    editor.sel.clear();
    f13a34 = 1;
    screen_troop = -1;
    jump_scene = -1;
    presentation = -1;
    notes_type = -1;
    notes_object = -1;
    merchandise_modifier_quality = 1.0f;
    background_presentation = -1;
    menu_tableau = -1;
    editor.list_dirty = 0;
    editor.dirty = 0;
    editor.f1a1d4 = 0;
    jump_entry = 0;
    jump_mission = 0;
    screen_party = 0;
    f13a08 = 0;
    attach_party = -100000;
    attach_scene = -100000;
    f13a18 = 0;
    attach_flip = 0;
    f13a24 = 0;
    f13a28 = 0;
    f13a2c = 0;
    range_lo = 0;
    range_hi = 100;
    rest_period = -10.0f;
    rest_attackable = 0;
    rest_interactive = 0;
    rest_time_speed = 0;
    f94 = 0;
    f13a20 = 20;
    encounter_flags = 0;
    cloud_amount = 0.5f;
    haze_amount = 0.5f;
    avg_value = 100.0f;
    avg_weight = 0;
    avg_prev_time = (float)g_ticks[0] * 1e-5f;
    current_menu = -1;
    event_ctx.prop = -1;
    f1a1d8 = 0;
    f1a1dc = 0;
    f1a1e4 = 0;
    f1a1e0 = 0;
    f1a1e8 = 0;
    f1a1ec = 0;
    f1a20c = 0;
    mode = 0;
    question_answer = -1;
    talk_relation_bar = 0;
    talk_info_shown = 0;

    tstring* p = talk_lines;
    for (int i = 0; i < 5; i++) {
        p->clear();
        talk_line_set[i] = 0;
        p++;
    }

    int idx = timer13a90.clock;
    timer13a90.start = g_ticks[idx];
}

// @ 0x51cfa0
Game::Game()
{
    WL_MARKER(12)
    scene_open = 0;
    reset();
    debugger.enabled = 0;
    debugger.shown = 0;
    editor.active = 0;
    editor.dialog_shown = 0;
    factions = 0;
    troops = 0;
    scenes = 0;
    quests = 0;
    WL_MARKER(13)
}

// @ 0x51d1c0
// std::vector<TrackRec>::_Buy (the library's own; _Xlen is the one shared instance 0x43e2a0)
template bool std::vector<TrackRec, std::allocator<TrackRec> >::_Buy(size_t);

// @ 0x51d220
MB_NOINLINE SpriteTable::SpriteTable()
{
}

// @ 0x51d250
MB_NOINLINE PartyMgr::PartyMgr()
    : Pool<Party>(Pool<Party>::unzeroed())
{
    int i = 0;
    do {
        tab[i].key = -1;
        tab[i].value = -1;
        i++;
    } while (i < 0x2717);
    count = 0;
    serial = 0;
    f28 = 0;
}

// the map objects reached through globals

// @ 0x51d2b0
void Game::init_map()
{
    struct Tri { int a, b, c; };
    Tri t;

    if (g_script.new_game_done != 0)
        return;
    g_script.nav->compute_bounds();   // 0x4704b0
    t.b = 5;
    t.c = 5;
    t.a = 0;
    g_script.level = new rgl::world(&g_script.nav->e44.x, &g_script.nav->f54.x, &t.c, &t.b, &t.a);   // 0x478ad0
    t.c = 1;
    t.b = 0;
    t.a = 7;
    g_script.nav->score_triggers(&t.a, &t.b, &t.c);
    t.c = 7;
    t.b = 0;
    t.a = 7;
    g_script.nav->solve(&t.a, &t.b, &t.c);
    g_script.level->build_faces(g_script.nav);   // 0x479980
}

// @ 0x51d3a0
// Zone numbering over the node graph: cells whose class mask equals *arg3 are the 0x5f5e0ff "blocked" zone; the
// others are flood-filled into numbered zones (find_trigger decides what a blocked neighbour contributes) and two
// zones met through one node are merged. The special zone's members are then walked.
void NavData::solve(int* arg1, int* arg2, int* arg3)
{
    unsigned i;
    int zone = -1, top;
    for (i = 0; i < cells.size(); i++) {
        cells[i].zone = -1;
        cells[i].zone_pair[0] = -1;
        cells[i].zone_pair[1] = -1;
    }
    int* stack = new int[cells.size()];
    int* path = new int[cells.size()];
    top = -1;
    for (int ctr = 0; (unsigned)ctr < cells.size(); ctr++) {
        int cur = ctr;
        if (top >= 0) {
            ctr--;
            cur = stack[top];
            top--;
        }
        if (cells[cur].zone >= 0)
            continue;
        if (cells[cur].kind == *arg3) {
            cells[cur].zone = 0x5f5e0ff;
            for (int k = 0; k < cells[cur].npts; k++) {
                int nb = cells[cur].target[k];
                if (nb >= 0 && cells[nb].zone < 0)
                    stack[++top] = nb;
            }
        } else {
            int comp = -1;
            for (int k = 0; k < cells[cur].npts; k++) {
                int nb = cells[cur].target[k];
                if (nb < 0)
                    continue;
                int res = -1;
                if (cells[nb].kind == *arg3) {
                    int cnt = 0;
                    path[0] = cur;
                    res = find_trigger(&nb, &cur, path, &cnt, (unsigned*)arg1, (unsigned*)arg2, arg3);
                }
                if (!(cells[cur].dist[k] < 1e9f) && cells[nb].kind != *arg3)
                    continue;
                if (res < 0 && cells[nb].kind != *arg3)
                    res = cells[nb].zone;
                if (cells[nb].zone < 0)
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
                for (i = 0; i < cells.size(); i++) {
                    int c = cells[i].zone;
                    if (c > hi) {
                        if (c != 0x5f5e0ff)
                            cells[i].zone = c - 1;
                    } else if (c == hi) {
                        cells[i].zone = comp;
                    }
                }
            }
            if (comp < 0)
                comp = ++zone;
            cells[cur].zone = comp;
        }
    }
    for (i = 0; i < cells.size(); i++) {
        if (cells[i].zone == 0x5f5e0ff) {
            *path = (int)i;
            int cnt = 0;
            int ii = (int)i;
            walk(&ii, &ii, path, &cnt);
        }
    }
    delete[] stack;
    delete[] path;
}

// @ 0x51d7c0
MB_NOINLINE int* NavData::walk(int* a, int* idx, int* seen, int* n)
{
    NavCell* node = &cells[*idx];
    for (int j = 0; j < node->npts; j++, node = &cells[*idx]) {
        int child = node->target[j];
        int num = *n;
        for (int k = 0; k <= *n; k++)
            if (child == seen[k])
                goto next;
        if (child < 0)
            continue;
        {
            if (cells[child].zone != 0x5f5e0ff) {
                if (cells[*a].zone_pair[0] == -1)
                    cells[*a].zone_pair[0] = cells[child].zone;
                else if (cells[*a].zone_pair[0] != cells[child].zone) {
                    NavCell* c = &cells[child];
                    cells[*a].zone_pair[1] = c->zone;
                    return (int*)c;
                }
            } else {
                *n = num + 1;
                seen[*n] = *idx;
                walk(a, &child, seen, n);
                *n = num;
                NavCell* r = &cells[*a];
                if (r->zone_pair[1] != -1)
                    return (int*)r;
            }
        }
    next:;
    }
    return (int*)node;
}

// @ 0x51d920    MB_NOINLINE int NavData::find_trigger(...)   (thiscall, ret 0x1c)
// Walks the trigger graph from the record *idp: `arr`/`*cnt` is the visited stack, m1/m2 the flag masks and
// *flag the flag that decides whether to recurse.
MB_NOINLINE int NavData::find_trigger(int* idp, int* a2, int* arr, int* cnt, unsigned* m1, unsigned* m2, int* flag)
{
    for (int i = 0; i < cells[*idp].npts; i++) {
        int v = cells[*idp].target[i];
        for (int k = 0; k <= *cnt; k++)
            if (v == arr[k])
                goto next;
        if (v < 0)
            continue;
        if (cells[v].zone < 0 && cells[v].kind != *flag)
            continue;
        if (cells[v].kind == *flag) {
            int n = *cnt;
            *cnt = n + 1;
            arr[n + 1] = *idp;
            int res = find_trigger(&v, a2, arr, cnt, m1, m2, flag);
            *cnt = n;
            if (res >= 0)
                return res;
        } else if ((cells[*a2].kind & *m1) == *m2 && (cells[v].kind & (*m1 == *m2))
            || (cells[*a2].kind & *m1) != *m2 && (cells[v].kind & (*m1 != *m2))) {
            if (cells[v].zone >= 0)
                return cells[v].zone;
        }
next:
        ;
    }
    return -1;
}

// @ 0x51dac0    void NavData::score_triggers(int* a, int* b, int* c)   (thiscall, ret 0xc)
// For every trigger record and every neighbour slot that is set, stores the compatibility score.
void NavData::score_triggers(int* a, int* b, int* c)
{
    for (unsigned i = 0; i < cells.size(); i++)
        for (int k = 0; k < cells[i].npts; k++)
            if (cells[i].target[k] != -1)
                cells[i].dist[k] = trigger_score(i, k, *a, *b, *c);
}

// @ 0x51db70    MB_NOINLINE float NavData::trigger_score(int row, int idx, unsigned a, unsigned b, unsigned c)   (ret 0x14)
MB_NOINLINE float NavData::trigger_score(int row, int idx, unsigned a, unsigned b, unsigned c)
{
    NavCell& r = cells[row];
    NavCell& o = cells[r.target[idx]];
    unsigned m = r.kind & a;
    unsigned n = o.kind & a;
    if (m == c || n == c)
        return 1000000000.0f;
    if (m == b) {
        if (n == b)
            return 2.0f;
    } else if (n != b)
        return 2.0f;
    return 1000000000.0f;
}


// @ 0x51dbd0
void Game::load_state(stream_base* s)
{
    unsigned __int64 tick;
    WL_MARKER(12)
    s->r_u64(&tick);
    ((int*)g_ticks)[2] = *(int*)&tick;
    ((int*)g_ticks)[3] = *((int*)&tick + 1);             // g_ticks[1] high word (0x5e2954)
    s->r_int(&f4);
    s->r_int(&fb4);
    WL_MARKER(13)
    load_clock(s);
    g_script.vars.read(s);
    triggers.read(s);
    simple.read(s);
    load_quests(s);
    load_parties(s);
    load_factions(s);
    load_tracks(s);
    load_party_templates(s);
    load_scene_parties(s);
    WL_MARKER(12)
    events.read(s, 0);
    WL_MARKER(13)
    load_troops(s);
    load_items(s);
    s->r_u64(&troops[player].face_keys.word[0]);
    s->r_u64(&troops[player].face_keys.word[1]);
    s->r_int(&cnt_killed);
    s->r_int(&cnt_ko);
    s->r_int(&cnt_killed_friend);
    s->r_int(&cnt_ko_friend);
}

// @ 0x51dd60
void TriggerVec::read(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n > count)
        mb_error("Load error: Number of triggers in file: %d, exceeds %d", n, count);
    for (int i = 0; i < n; i++)
        data[i].read(s);
}

// @ 0x51ddc0
MB_NOINLINE void TriggerRec::read(stream_base* s)
{
    s->r_int(&state);
    s->r_i64(&check_timer.start);
    s->r_i64(&delay_timer.start);
    s->r_i64(&rearm_timer.start);
}

// @ 0x51de00
void SimpleTriggerVec::read(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n > count)
        mb_error("Load error: Number of simple triggers in file: %d, exceeds %d", n, count);
    for (int i = 0; i < n; i++)
        s->r_i64(&data[i].timer.start);
}

// @ 0x51de70
void Game::load_clock(stream_base* s)
{
    WL_MARKER(12)
    s->r_i64(&game_clock.start);
    s->r_int(&current_hours);
    s->r_int(&string_count);
    s->r_int(&f13b30);
    s->r_int(&f13b34);
    s->r_int(&f13b38);
    s->r_int(&f13b60);
    s->r_float(&cloud_amount);
    s->r_float(&haze_amount);
    s->r_float(&avg_value);
    s->r_float(&avg_weight);
    s->r_tstr(rs(&t13a70));
    s->r_bool(&f13aa0);
    s->r_int(&f13ae8);
    s->r_int(&f13a20);
    s->r_int(&attach_party);
    s->r_int(&attach_scene);
    s->r_int(&current_menu);
    s->r_int(&jump_scene);
    s->r_int(&jump_entry);
    s->r_int(&jump_mission);
    s->r_int(&range_lo);
    s->r_int(&range_hi);
    tstring& ts = message_log;
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
    s->r_i64(&timer13a80.start);
    s->r_float(&rest_period);
    s->r_int(&rest_time_speed);
    s->r_int(&rest_interactive);
    s->r_int(&rest_attackable);
    WL_MARKER(13)
}

// @ 0x51e0b0
MB_NOINLINE void SlotArray::read(stream_base* s)
{
    int n;
    s->r_int(&n);
    resize((unsigned)n, 0);                         // 0x493090
    for (int i = 0; i < n; i++)
        s->r_i64(&(*this)[i]);
}

// @ 0x51e100
void Game::load_quests(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n > n_quests)
        mb_error("Load error: Number of quests in file: %d, exceeds %d", n, n_quests);
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
    read_soa(s, &notes);
    slots.read(s);
}

// @ 0x51e1f0
stream_base* read_soa(stream_base* s, SoaBlock* b)
{
    for (int i = 0; i < 8; i++) {
        s->r_tstr(rs(&b->texts[i]));
        s->r_int(&b->times[i]);
        s->r_int(&b->tableau);
    }
    return s;
}

// @ 0x51e240
void Game::load_parties(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n != num_scenes)
        mb_error("Load error: Number of scenes in file: %d, exceeds %d", n, num_scenes);
    for (int i = 0; i < num_scenes; i++)
        scenes[i].slots.read(s);
}

// @ 0x51e2b0
void Game::load_factions(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n > n_factions)
        mb_error("Load error: Number of factions in file: %d, exceeds %d", n, n_factions);
    for (int i = 0; i < n; i++)
        static_cast<Faction*>(&factions[i])->read(s, n);
}

// @ 0x51e320
MB_NOINLINE void Faction::read(stream_base* s, int n)
{
    slots.read(s);
    for (int i = 0; i < n; i++)
        s->r_float(&relations[i]);
    s->r_tstr(rs(&name));
    s->r_bool(&f10);
    s->r_uint(&f28);
    s->r_int(&f230);
    read_soa(s, &notes);
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
    slots.read(s);
}

// @ 0x51e450
void Game::load_items(stream_base* s)
{
    int n;
    s->r_int(&n);
    if (n < 0 || n >= g_item_kind_count + 1)
        mb_error("Load error: Invalid number of item kinds: %d", n);
    for (int i = 0; i < n; i++)
        g_items[i].slots.read(s);
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
    int k = 0;
    do {
        s->r_int(&g_skill_tbl[k].cap);
        k++;
    } while (k < 42);
}

// @ 0x51e540
MB_NOINLINE void TroopIO::read(stream_base* s)
{
    slots.read(s);
    for (int i = 0; i < 4; i++)
        s->r_int(&attributes[i]);
    for (int i = 0; i < 7; i++)
        s->r_float(&stat[i]);
    for (int i = 0; i < 6; i++)
        s->r_uint(&nib[i]);
    s->r_u64(&flags30).r_int(&site);
    s->r_int(&gold).r_int(&exp);
    s->r_float(&hp);
    s->r_int(&faction);
    s->r_int(&skill_points).r_int(&attr_points).r_int(&prof_points);
    s->r_int(&level);
    inv[0].read_run(s);
    ItemRef* q = equip;
    for (int i = 0; i < 10; i++, q++) {
        s->r_int(&q->item);
        s->r_int(&q->word);
    }
    for (int i = 0; i < 4; i++)
        s->r_u64(&face_keys.word[i]);
    read_soa(s, &notes);
}

// @ 0x51e690
MB_NOINLINE void ItemRef::read_run(stream_base* s)
{
    for (int i = 0; i < 0x60; i++) {
        s->r_int(&this[i].item);
        s->r_int(&this[i].word);
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
        // std::vector's own operator[] (begin() + i through the iterator class): the element address is then computed
        // before the argument push.
        tracks[i].read(s);
    }
}

// @ 0x51e750
MB_NOINLINE void TrackRec::read(stream_base* s)
{
    s->r_float(&x).r_float(&y).r_float(&z);
    s->r_float(&f10);
    s->r_float(&age).r_int(&i18);
}

// @ 0x51e7a0
// std::vector::resize: the iterator erase returns is discarded, in n's own slot.
template void std::vector<TrackRec, std::allocator<TrackRec> >::resize(size_t, TrackRec);

// @ 0x51e840
void SpriteTable::read(stream_base* s, int arg)
{
    if (count)
        clear();
    int n;
    s->r_int(&n);
    s->r_int((int*)&serial);
    for (int i = 0; i < n; i++)
        alloc();
    clear();
    free_ids.resize(0, 0);
    int flag;
    for (int i = 0; i < n; i++) {
        Event* e = atu(i);
        s->r_int(&flag);
        e->in_use = flag;
        if (flag) {
            s->r_int(&e->serial);
            e->read(s, arg);
        } else {
            free_ids.push_back(i);
        }
    }
}

// @ 0x51e970
MB_NOINLINE void Event::read(stream_base* s, int arg)
{
    float* p;
    stream_base* rs_;
    s->r_tstr(rs(&name));
    s->r_int(&type);
    p = &pos.x;
    rs_ = &s->r_float(p);
    rs_->r_float(++p);
    p = &pos2.x;
    rs_ = &s->r_float(p);
    rs_->r_float(++p);
    p = &f34;
    rs_ = &s->r_float(p);
    rs_->r_float(++p);
    s->r_int(&party_a);
    s->r_int(&party_b);
    s->r_i64(&born.start);
    s->r_float(&next_simulation_time);
}

// @ 0x51ea10
MB_NOINLINE void SpriteTable::clear()
{
    free_ids.resize(0, 0);
    for (int i = 0; i < count; i++) {
        Event* e = atu(i);
        e->in_use = 0;
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
    PartyMgr& sc = parties;
    sc.read(s, 1);
    int idx = sc.lookup(main_party);
    Party* rec = &sc.pages[idx >> 4][idx & 0xf];
    for (int i = 0; i < rec->nstacks; i++) {
        Stack* e = &rec->stacks[i];
        if ((troops[e->troop].flags30 & 0x10) == 0) {
            s->r_float(&e->xp);
            s->r_int(&e->f14);
            for (int k = 0; k < 0x20; k++)
                s->r_int(&stack_seeds[i][k]);
        }
    }
}
