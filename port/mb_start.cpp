// port/mb_start.cpp: test-run support for the native port only (nothing here is in the MSVC build; src/ is untouched).
//
// 1. State lines. Once per frame (mbp_frame_hook, called from FSOUND_Update: GameApp::FrameMove calls it after the GUI's
//    input pass and before the current screen's frame_update) the port looks at the window manager and writes a line to
//    stderr whenever something a test waits for changes:
//        MBSTATE <seconds since start> <event> [args]
//    events: main | first_frame | load_step <n> | screen <id> <name> | menu_ready | menu <game menu id> | mission_frame <n> |
//            start <step> | start_done | start_failed <why>
//    menu_ready is the main menu's second frame (its rows exist and have been laid out once). mission_frame counts the
//    frames on the mission screen (14) since it was entered (1, 2, 3, 5, 10, 20, 30, 50, 100, 200, 300, 500, then every
//    500). tools/port/run_native.py waits on these lines (--wait / --on). MB_STATE=0 turns them off.
//
// 2. Direct launch. MB_START=<spec> drives the menus the way a click does: it sets the pressed flag (0x10000, what UiA::key
//    0x48b200 sets on a click) on the button of the current screen, after the GUI's input pass, so the screen's own
//    frame_update (ItemRec 0x4d5290, ItemA 0x533070, C4c31c0 0x4ea290) runs its original handler on the next line. No game
//    logic of its own. Specs (comma-separated steps run in order, each once its screen is up):
//        quick_battle[:N]   main menu custom battle, scenario N (1-based row of start_game_3, default 1), Start
//        new_game           main menu "Start a New Game" (character creation's first menu)
//        tutorial           main menu "Start Tutorial"
//        load[:N] | map[:N] main menu restore, slot N (sg0N.sav, 0-based; default the first slot holding a save), Restore;
//                           map also waits for the map screen
//    and the single steps they expand to, usable directly:
//        main:<row>               row = tutorial | new_game | restore | custom | options | credits | exit
//        option:<item>            a game-menu row by its menus.txt id (mno_ optional), e.g. option:custom_battle_go
//        option:<menu>#<n>        the n-th shown row (1-based) of game menu <menu> (menu_ optional)
//        slot:<n> | slot:first    the save / load window's slot, then its Restore / Save button
//        wait:<screen name>       until that screen is current (e.g. wait:map)
//    Saves are read and written where the game puts them: under MB_USER_DIR when set (port/platform/fs.cpp).
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "../src/game/window_mgr.h"
#include "../src/game/screen_records.h"
#include "../src/game/triggers/item_rec.h"
#include "../src/game/options_panel/mission_item_recs.h"
#include "../src/game/ui/mission_ui.h"
#include "../src/game/menu_rec.h"
#include "../src/include/game_data.h"

namespace {

double now_s()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}
const double g_t0 = now_s();        // static initialisation: the process start, near enough
bool g_on = true;

void state(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void state(const char* fmt, ...)
{
    if (!g_on)
        return;
    char buf[512];
    int n = snprintf(buf, sizeof buf, "MBSTATE %.3f ", now_s() - g_t0);
    va_list ap;
    va_start(ap, fmt);
    n += vsnprintf(buf + n, sizeof buf - n - 1, fmt, ap);
    va_end(ap);
    if (n > (int)sizeof buf - 2)
        n = sizeof buf - 2;
    buf[n++] = '\n';
    write(2, buf, n);
}

const char* const k_screen_names[30] = {
    "dialog", "message", "loading", "game_start", "main_menu", "escape_menu", "mission_start", "map",
    "party_window", "rec38", "rec44", "rec34", "party_screen", "rec454", "mission", "char_sheet",
    "rec1208", "rec3c", "options", "recf0", "rec1bc", "save_load", "terrain_page", "game_menu",
    "rec48", "rec58", "notes", "save_panel", "load_panel", "face_gen"};

int screen_now()
{
    if (g_windows.dq.empty())
        return -1;
    int s = g_windows.dq.back();
    return s >= 0 && s < 30 && g_windows.arr[s] ? s : -1;
}

const char* cstr(const tstring& s) { return s.c_str() ? s.c_str() : ""; }

// "mno_x" / "x" and "menu_x" / "x" name the same record
bool same_id(const char* have, const char* want, const char* prefix)
{
    size_t pl = strlen(prefix);
    if (!strncmp(have, prefix, pl))
        have += pl;
    if (!strncmp(want, prefix, pl))
        want += pl;
    return !strcmp(have, want);
}

// a menu row by its menus.txt id (in one of the item's strings: name is the shown text) or by that text
bool item_is(const menu_item& it, const char* want)
{
    return same_id(cstr(it.name), want, "mno_") || same_id(cstr(it.text), want, "mno_") || same_id(cstr(it.key), want, "mno_");
}

void press(MissionUi* w) { w->set_key_flags(0x10000); }

std::vector<std::string> g_steps;   // what is left of MB_START
size_t g_step = 0;
int g_frames_on_screen = 0;         // frames the current screen has been up (1 = its first)
int g_menu_frames = 0;              // frames the current game menu has been up
int g_last_menu = -1;

void expand(const char* spec)
{
    std::string s(spec);
    size_t a = 0;
    while (a <= s.size()) {
        size_t b = s.find(',', a);
        if (b == std::string::npos)
            b = s.size();
        std::string t = s.substr(a, b - a);
        a = b + 1;
        if (t.empty())
            continue;
        std::string head = t.substr(0, t.find(':')), arg = t.find(':') == std::string::npos ? "" : t.substr(t.find(':') + 1);
        if (head == "quick_battle") {
            g_steps.push_back("main:custom");
            g_steps.push_back("option:start_game_3#" + (arg.empty() ? std::string("1") : arg));
            g_steps.push_back("option:custom_battle_go");
            g_steps.push_back("wait:mission");
        } else if (head == "new_game" || head == "tutorial") {
            g_steps.push_back("main:" + head);
            g_steps.push_back("wait:game_menu");
        } else if (head == "load" || head == "map") {
            g_steps.push_back("main:restore");
            g_steps.push_back("slot:" + (arg.empty() ? std::string("first") : arg));
            if (head == "map")
                g_steps.push_back("wait:map");
        } else {
            g_steps.push_back(t);
        }
    }
}

// one try of the current step; true when it was done (or failed for good: g_step then jumps to the end)
bool run_step(const std::string& st, int scr)
{
    std::string head = st.substr(0, st.find(':')), arg = st.find(':') == std::string::npos ? "" : st.substr(st.find(':') + 1);
    if (head == "wait") {
        return scr >= 0 && arg == k_screen_names[scr];
    }
    if (head == "main") {
        if (scr != 4 || g_frames_on_screen < 2)
            return false;
        ItemRec* m = slot_as<ItemRec>(4);
        MissionUi* w = arg == "tutorial" ? m->row_tutorial : arg == "new_game" ? m->row_new_game
                     : arg == "restore" ? m->row_restore : arg == "custom" ? m->row_custom
                     : arg == "options" ? m->row_options : arg == "credits" ? m->row_credits
                     : arg == "exit" ? m->row_exit : nullptr;
        if (!w) {
            state("start_failed %s: no such row on the main menu", st.c_str());
            g_step = g_steps.size();
            return true;
        }
        press(w);
        return true;
    }
    if (head == "option") {
        if (scr != 23 || !g_game || !g_menus || g_game->mode != 0 || g_menu_frames < 2)
            return false;
        ItemA* gm = slot_as<ItemA>(23);
        MenuRec& rec = g_menus[g_game->current_menu];
        size_t hash = arg.find('#');
        int pick = -1;
        if (hash != std::string::npos) {
            if (!same_id(cstr(rec.id), arg.substr(0, hash).c_str(), "menu_"))
                return false;                            // not that menu (yet)
            int n = atoi(arg.c_str() + hash + 1) - 1;
            if (n < 0 || n >= gm->count) {
                state("start_failed %s: menu %s shows %d rows", st.c_str(), cstr(rec.id), gm->count);
                g_step = g_steps.size();
                return true;
            }
            pick = n;
        } else {
            for (int i = 0; i < gm->count && i < 16; i++)
                if (gm->ids[i] >= 0 && gm->ids[i] < rec.count && item_is(rec.items[gm->ids[i]], arg.c_str()))
                    pick = i;
            if (pick < 0)
                return false;                            // not on this menu: wait for the next one
        }
        press(gm->kids[pick]);
        return true;
    }
    if (head == "slot") {
        if (scr != 21 || g_frames_on_screen < 2)
            return false;
        C4c31c0* sl = slot_as<C4c31c0>(21);
        int n = -1;
        if (arg == "first") {
            for (int i = 0; i < 9 && n < 0; i++)
                if (sl->labels[i] && sl->slots[i].seconds != 0)
                    n = i;
        } else {
            n = atoi(arg.c_str());
        }
        if (n < 0 || n > 8 || !sl->labels[n] || sl->slots[n].seconds == 0) {
            state("start_failed %s: no save in that slot", st.c_str());
            g_step = g_steps.size();
            return true;
        }
        if (sl->sel218 != n) {
            press(sl->labels[n]);                        // select it this frame, the Restore button the next
            return false;
        }
        press(sl->btn_done);
        return true;
    }
    state("start_failed %s: unknown step", st.c_str());
    g_step = g_steps.size();
    return true;
}

int g_screen = -2, g_load_step = -1;
long g_mission_frames = 0;
bool g_first = true, g_menu_ready = false;

bool mission_mark(long n)
{
    static const long marks[] = {1, 2, 3, 5, 10, 20, 30, 50, 100, 200, 300, 500};
    for (long m : marks)
        if (n == m)
            return true;
    return n > 500 && n % 500 == 0;
}

} // namespace

// GameApp::FrameMove -> FSOUND_Update (port/audio) -> here, between g_gui.process and the screen's frame_update
// port/main.cpp: the start of main() (static initialisation is over)
void mbp_state_main()
{
    const char* e = getenv("MB_STATE");
    g_on = !(e && *e == '0');
    state("main");
}

extern "C" void mbp_frame_hook()     // C linkage: port/audio/fmod_sdl.cpp's weak default is in its extern "C" block
{
    if (g_first) {
        g_first = false;
        const char* e = getenv("MB_STATE");
        g_on = !(e && *e == '0');
        if (const char* s = getenv("MB_START"))
            expand(s);
        state("first_frame");
    }
    int scr = screen_now();
    if (scr != g_screen) {
        g_screen = scr;
        g_frames_on_screen = 0;
        g_mission_frames = 0;
        state("screen %d %s", scr, scr >= 0 ? k_screen_names[scr] : "none");
    }
    g_frames_on_screen++;
    if (scr == 2) {
        int st = slot_as<C4c3290>(2)->step;
        if (st != g_load_step) {
            g_load_step = st;
            state("load_step %d", st);
        }
    }
    if (scr == 4 && g_frames_on_screen == 2 && !g_menu_ready) {
        g_menu_ready = true;
        state("menu_ready");
    }
    int menu = scr == 23 && g_game ? g_game->current_menu : -1;
    if (menu != g_last_menu) {
        g_last_menu = menu;
        g_menu_frames = 0;
        if (menu >= 0 && g_menus)
            state("menu %s", cstr(g_menus[menu].id));
    }
    g_menu_frames++;
    if (scr == 14 && mission_mark(++g_mission_frames))
        state("mission_frame %ld", g_mission_frames);
    if (g_step < g_steps.size()) {
        const std::string st = g_steps[g_step];
        if (run_step(st, scr)) {
            if (g_step < g_steps.size()) {
                state("start %s", st.c_str());
                if (++g_step == g_steps.size())
                    state("start_done");
            }
        }
    }
}
