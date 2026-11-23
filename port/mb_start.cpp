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
//        message:<n>              the message screen's n-th button (the saving-policy question of character creation)
//        dialog:<n>               the dialog box's first (0) or second (1) button (the tutorial boxes' OK)
//        name:<text>              the character sheet's name box gets that text (what typing it would do)
//        sheet:done               the character sheet's Done button (character creation: on to the face page / the map)
//        face:done                the face generator's Done button
//        speed:hold|release       hold Space down on the map (the game advances time only while it is down; a travel order advances it too)
//        travel:<name>            the player's party starts travelling to the first live party whose name contains <name> (the map
//                                 click's own order: action 4, target the party, reissue_order)
//        esc                      the map's Escape key: opens the escape menu (screen 5)
//        escrow:<n>               presses the escape menu's row n (0 resume, 1 save and exit to the main menu, 2 options,
//                                 3 save, 4 load, 5 exit without saving; 3-5 exist only when the game has them)
//        continue_to_map          press `continue` on every game menu (debrief, captivity, ...) until the map is up
//        frames:<n>               wait n frames before the next step
//        conv:<n> | conv:continue the conversation screen's n-th choice | a line that waits for the left button
//        widget:<n>               the running presentation's n-th overlay (a scripted screen: the banner choice, ...), pressed
//                                 once the presentation has been up a few frames; widget:list only logs their kinds
//    Saves are read and written where the game puts them: under MB_USER_DIR when set (port/platform/fs.cpp).
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>   // tolower: macOS's shim chain pulled it in transitively, glibc's does not
#include <time.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "../src/game/ui/window_mgr.h"
#include "../src/game/data/presentation.h"
#include "../src/game/ui/screens/screen_records.h"
#include "../src/game/ui/screens/result_window.h"
#include "../src/game/ui/screens/face_generator/face_gen.h"
#include "../src/game/ui/screens/conversation/conv_screen.h"
#include "../src/game/input/slot.h"
#include "../src/game/ui/screens/character_sheet/char_sheet_panel.h"
#include "../src/game/ui/widgets/text_editor_widget.h"
#include "../src/game/data/item_head_records.h"
#include "../src/game/data/mission_item_recs.h"
#include "../src/game/ui/widgets/widget_base.h"
#include "../src/game/data/menu_rec.h"
#include "../src/game/data/game_data.h"
#include "../src/game/mission/mission.h"
#include "../src/game/agent/agent_obj.h"
#include "../src/game/script/script_state.h"

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
bool g_bearing = false;             // log the nearest enemy's bearing every 15 mission frames (tools/port/fight.py reads it)
bool g_hold_space = false;          // Space (scan code 57) is held down while the map is the screen
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
    if (head == "dialog") {
        if (scr != 0 || g_frames_on_screen < 4)
            return false;
        ResultWindow* d = slot_as<ResultWindow>(0);
        MissionUi* b = atoi(arg.c_str()) == 0 ? (MissionUi*)d->b3c : (MissionUi*)d->b40;
        if (!b) {
            state("start_failed %s: no such button", st.c_str());
            g_step = g_steps.size();
            return true;
        }
        press(b);
        return true;
    }
    if (head == "speed" || head == "travel") {
        if (scr != 7 || !g_game || g_frames_on_screen < 3)
            return false;
        if (head == "speed") {   // speed:hold = Space held down while the map is up (frame_update 0x514d40 reads down[57]); speed:release lets go
            g_hold_space = arg != "release";
            return true;
        }
        int found = -1;
        if (strncmp(arg.c_str(), "list", 4) == 0) {          // travel:list or travel:list<radius>
            float radius = arg.size() > 4 ? (float)atof(arg.c_str() + 4) : 120.0f;      // the parties a click could pick: the target-lost test of MapPanel::update_target (alpha, flags 0x1080100)
            Party* me = g_game->parties.get(g_game->main_party);
            for (unsigned id = 0; id < g_game->parties.serial; id++) {
                if (!g_game->parties.is_live((int)id) || (int)id == g_game->main_party)
                    continue;
                Party* q = g_game->parties.get((int)id);
                if (q->alpha < 0.01f || q->attached_to >= 0 || (q->flags_lo & 0x1080100))
                    continue;
                float dx = q->pos.x - me->pos.x, dy = q->pos.y - me->pos.y;
                if (dx * dx + dy * dy < 900.0f)
                    state("party %u %s type %d flags %08x ai %d dist %.1f", id, q->name.c_str(), q->type, (unsigned)q->flags_lo, (int)q->ai_behavior, (double)sqrtf(dx * dx + dy * dy));
            }
            return true;
        }
        for (unsigned id = 0; id < g_game->parties.serial && found < 0; id++) {
            if (!g_game->parties.is_live((int)id) || (int)id == g_game->main_party)
                continue;
            if (g_game->parties.get((int)id)->alpha < 0.01f || (g_game->parties.get((int)id)->flags_lo & 0x1080100))
                continue;                    // update_target would drop the order as "target lost"
            std::string nm = g_game->parties.get((int)id)->name.c_str(), want = arg;
            for (char& c : nm) c = (char)tolower((unsigned char)c);
            for (char& c : want) c = (char)tolower((unsigned char)c);
            if (nm.find(want) != std::string::npos)
                found = (int)id;
        }
        if (found < 0) {
            state("start_failed %s: no such party", st.c_str());
            g_step = g_steps.size();
            return true;
        }
        g_game->map_live = 1;
        g_game->sel_party = g_game->main_party;
        g_game->parties.get(g_game->main_party)->action = 4;
        g_game->parties.get(g_game->main_party)->target = found;
        g_game->parties.get(g_game->main_party)->reissue_order();
        state("travel to party %d (%s)", found, g_game->parties.get(found)->name.c_str());
        return true;
    }
    if (head == "warp") {       // warp:<name>: moves the player's party next to the named party (test shortcut), then the order is the usual travel
        if (scr != 7 || !g_game || g_frames_on_screen < 3)
            return false;
        Party* me = g_game->parties.get(g_game->main_party);
        for (unsigned id = 0; id < g_game->parties.serial; id++) {
            if (!g_game->parties.is_live((int)id) || (int)id == g_game->main_party)
                continue;
            Party* q = g_game->parties.get((int)id);
            std::string nm = q->name.c_str(), want = arg;
            for (char& c : nm) c = (char)tolower((unsigned char)c);
            for (char& c : want) c = (char)tolower((unsigned char)c);
            if (nm.find(want) == std::string::npos || (q->flags_lo & 0x1080100))
                continue;
            me->pos.x = q->pos.x + 1.0f;
            me->pos.y = q->pos.y;
            state("warp to party %u (%s)", id, nm.c_str());
            return run_step("travel:" + arg, scr);
        }
        state("start_failed %s: no such party", st.c_str());
        g_step = g_steps.size();
        return true;
    }
    if (head == "goto") {       // goto:<name>: travel there like a player: roadside encounters are talked through and left, the order is re-issued; done when the arrival menu is up
        if (scr == 0)           // a message box: its first button
            return run_step("dialog:0", scr), false;
        if (scr == 16)
            return run_step("conv:all", scr), false;
        if (scr == 7) {
            if (g_frames_on_screen < 3 || !g_game)
                return false;
            Party* me = g_game->parties.get(g_game->main_party);
            if (me->action != 4)                    // not travelling (just left an encounter, or not started)
                run_step("travel:" + arg, scr);
            return false;
        }
        if (scr == 23 && g_game && g_menus && g_game->mode == 0 && g_menu_frames >= 2) {
            const char* id = cstr(g_menus[g_game->current_menu].id);
            if (same_id(id, "simple_encounter", "menu_")) {         // a roadside encounter: leave when the menu offers it
                if (run_step("option:encounter_leave", scr))
                    state("goto: left an encounter");
                else if (g_menu_frames > 30) {
                    state("goto: encounter without a leave row: attacking");
                    run_step("option:encounter_attack", scr);
                }
                return false;
            }
            if (same_id(id, "continue_captivity", "menu_") || !strcmp(id, "menu_battle_debrief") || strncmp(id, "menu_captivity", 14) == 0)
                return run_step("option:continue", scr), false;
            return true;                                              // the arrival menu
        }
        return false;
    }
    if (head == "continue_to_map") {
        if (scr == 7)
            return true;
        if (scr == 23 && run_step("option:continue", scr))
            state("continue_to_map: continued");
        return false;
    }
    if (head == "slots") {      // slots:<party name>: the nonzero script slots of the first live party whose name contains it
        if (!g_game)
            return false;
        for (unsigned id = 0; id < g_game->parties.serial; id++) {
            if (!g_game->parties.is_live((int)id))
                continue;
            Party* q = g_game->parties.get((int)id);
            std::string nm = q->name.c_str();
            if (nm.find(arg) == std::string::npos)
                continue;
            std::string out;
            char b[64];
            for (size_t i = 0; i < q->slots.size(); i++)
                if (q->slots[i]) {
                    snprintf(b, sizeof b, " %d=%lld", (int)i, (long long)q->slots[i]);
                    out += b;
                }
            state("slots party %u %s flags %08x:%s", id, nm.c_str(), (unsigned)q->flags_lo, out.c_str());
        }
        return true;
    }
    if (head == "var") {        // var:<n>: logs script variable n (g_script.vars)
        if (!g_game)
            return false;
        int n = atoi(arg.c_str());
        state("var %d = %lld", n, (long long)((size_t)n < g_script.vars.size() ? g_script.vars[n] : 0));
        return true;
    }
    if (head == "pos") {        // pos: logs the player's agent position and facing in a mission
        if (scr != 14 || !g_mission || g_mission->player_id < 0)
            return false;
        Agent& a = g_mission->agents[g_mission->player_id];
        state("player pos %.3f %.3f %.3f", (double)a.pos.x, (double)a.pos.y, (double)a.pos.z);
        return true;
    }
    if (head == "inv") {        // inv:<troop id> or inv:screen: logs the non-empty inventory slots (slot, item, modifier) of a troop; `screen` = the trade screen's own two troops
        if (!g_game)
            return false;
        int ids[2] = {atoi(arg.c_str()), -1};
        if (strncmp(arg.c_str(), "name=", 5) == 0) {   // inv:name=<text>: every troop whose name contains it (counts only)
            const char* want = arg.c_str() + 5;
            if (!*want)                 // inv:name= : the first troops' names
                for (int i = 0; i < 12 && i < g_game->ntroops; i++)
                    state("troop %d name '%s' key '%s' gold %d", i, g_game->troops[i].name.c_str(), g_game->troops[i].key.c_str(), (int)g_game->troops[i].gold);
            state("ntroops %d", g_game->ntroops);
            for (unsigned i = 0; i < 0x1000 && i < (unsigned)g_game->ntroops; i++) {
                Troop& tr = g_game->troops[i];
                if (!strstr(tr.name.c_str(), want))
                    continue;
                int n = tr.inv_count(), shown = 0;
                for (int k = 0; k < 0x60 && k < n; k++)
                    if (tr.inv[k].item >= 0)
                        shown++;
                state("inv troop %u %s gold %d slots %d items %d", i, tr.name.c_str(), (int)tr.gold, n, shown);
            }
            return true;
        }
        if (arg == "screen") {
            if (scr != 8)
                return false;
            head_rec* t = slot_as<head_rec>(8);
            ids[0] = t->w14;
            ids[1] = g_game->player;
            state("inv screen: w04 %d w10 %d w14 %d player %d", t->w04, t->w10, t->w14, g_game->player);
        }
        for (int k = 0; k < 2; k++) {
            if (ids[k] < 0)
                continue;
            Troop& tr = g_game->troops[ids[k]];
            std::string out;
            char b[64];
            int n = tr.inv_count(), shown = 0;
            for (int i = 0; i < 0x60 && i < n; i++)
                if (tr.inv[i].item >= 0 && shown++ < 40) {
                    snprintf(b, sizeof b, " [%d]=%d/%d", i, tr.inv[i].item, tr.inv[i].m4);
                    out += b;
                }
            state("inv troop %d %s gold %d slots %d items %d:%s", ids[k], tr.name.c_str(), (int)tr.gold, n, shown, out.c_str());
        }
        return true;
    }
    if (head == "await") {      // await:<screen name>: like wait:, pressing message boxes (screen 0) on the way
        if (scr == 0)
            return run_step("dialog:0", scr), false;
        return scr >= 0 && arg == k_screen_names[scr];
    }
    if (head == "bearing") {    // bearing: from now on, MBSTATE `bearing` lines while a mission runs
        g_bearing = true;
        return true;
    }
    if (head == "esc") {
        if (scr != 7 || g_frames_on_screen < 3)
            return false;
        slot_as<head_rec>(5)->w18 = 7;          // CampaignScreen::frame_update's own Escape branch
        g_windows.set_state(5);
        return true;
    }
    if (head == "escrow") {
        if (scr != 5 || g_frames_on_screen < 3)
            return false;
        ItemRec2* e = slot_as<ItemRec2>(5);
        MissionUi* rows[6] = {e->row0, e->row1, e->row2, e->row3, e->row4, e->row5};
        int n = atoi(arg.c_str());
        if (n < 0 || n > 5 || !rows[n]) {
            state("start_failed %s: no such row", st.c_str());
            g_step = g_steps.size();
            return true;
        }
        press(rows[n]);
        return true;
    }
    if (head == "frames") {
        static int left = -1;
        if (left < 0)
            left = atoi(arg.c_str());
        if (--left > 0)
            return false;
        left = -1;
        return true;
    }
    if (head == "conv") {
        if (arg != "all" && (scr != 16 || g_frames_on_screen < 6))
            return false;
        Rec1208* c = slot_as<Rec1208>(16);
        if (arg == "all") {                 // continue every line until the screen changes
            if (scr != 16)
                return g_frames_on_screen < 3 ? false : true;
            if (g_frames_on_screen % 30 == 0 && c->n_lines > 1) {       // choices: the first one
                state("conv choice 0 of %d", c->n_lines);
                press(c->buttons[0]);
            } else if (g_frames_on_screen % 30 == 0) {
                g_keyboard.prev[0xe0] = 1;
                g_keyboard.down[0xe0] = 0;
            }
            return false;
        }
        if (arg == "continue") {            // "press left mouse button to continue": the button (scan code 0xe0) released this frame
            g_keyboard.prev[0xe0] = 1;
            g_keyboard.down[0xe0] = 0;
            return true;
        }
        int n = atoi(arg.c_str());
        if (n < 0 || n >= 0x400 || !c->buttons[n]) {
            state("start_failed %s: no such choice", st.c_str());
            g_step = g_steps.size();
            return true;
        }
        press(c->buttons[n]);
        return true;
    }
    if (head == "face") {
        if (scr != 29 || g_frames_on_screen < 4)
            return false;
        press(slot_as<FaceGen>(29)->b_done);
        return true;
    }
    if (head == "name" || head == "sheet") {
        if (scr != 15 || g_frames_on_screen < 4)
            return false;
        CharSheet* cs = slot_as<CharSheet>(15);
        if (head == "name") {
            tstring t(arg.c_str());
            cs->name->set_text(t);
        } else {
            press(cs->done);
        }
        return true;
    }
    if (head == "message") {
        if (scr != 1 || g_frames_on_screen < 3)
            return false;
        C4c3050* m = slot_as<C4c3050>(1);
        int n = atoi(arg.c_str());
        if (n < 0 || n >= m->count || !m->rows[n]) {
            state("start_failed %s: the message has %d buttons", st.c_str(), m->count);
            g_step = g_steps.size();
            return true;
        }
        press(m->rows[n]);
        return true;
    }
    if (head == "widget") {
        if (!g_presentation || g_frames_on_screen < 4)
            return false;
        if (arg == "list") {
            for (size_t i = 0; i < g_presentation->widgets.size(); i++)
                state("widget %d kind %d", (int)i, (int)g_presentation->widgets[i]->kind());
            return true;
        }
        size_t n = (size_t)atoi(arg.c_str());
        if (n >= g_presentation->widgets.size()) {
            state("start_failed %s: the presentation has %d overlays", st.c_str(), (int)g_presentation->widgets.size());
            g_step = g_steps.size();
            return true;
        }
        press(g_presentation->widgets[n]);
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
    if (g_hold_space) {
        if (scr == 7)
            g_keyboard.down[57] = 1;      // (any other screen: not held, resumed when the map is back)
    }
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
    if (scr == 7 && g_game && g_frames_on_screen % 120 == 0) {
        Party* mp = g_game->parties.get(g_game->main_party);
        state("map_frame %d action %d target %d at %.5f %.5f dest %.5f %.5f order %.5f %.5f speed_flag %d step %.4f", g_frames_on_screen, (int)mp->action, (int)mp->target, (double)mp->pos.x, (double)mp->pos.y, (double)mp->dest.x, (double)mp->dest.y, (double)mp->order_pos.x, (double)mp->order_pos.y, (int)g_game->f13aa4, (double)g_game->f14058);
    }
    if (g_bearing && scr == 14 && g_mission && g_mission->player_id >= 0 && g_mission_frames % 15 == 0) {
        Agent& me = g_mission->agents[g_mission->player_id];
        int best = -1, enemies = 0;
        float bd = 1e30f;
        unsigned end = g_mission->agents.count;
        for (unsigned i = g_mission->agents.first_used(); i < end; g_mission->agents.next_used(&i)) {
            Agent& o = g_mission->agents[i];
            if ((int)i == g_mission->player_id || o.team == me.team || o.state != 1 || o.f5830 != 2 || o.kind != 0)   // kind 0: a person (the horses are agents of their own, the player's at the player's position)
                continue;
            enemies++;
            float dx = o.pos.x - me.pos.x, dy = o.pos.y - me.pos.y, d = dx * dx + dy * dy;
            if (d < bd) { bd = d; best = (int)i; }
        }
        float yaw = atan2f(me.in_now.move.y, me.in_now.move.x) * 57.29578f;
        if (best >= 0) {
            Agent& o = g_mission->agents[best];
            float dx = o.pos.x - me.pos.x, dy = o.pos.y - me.pos.y;
            float err = atan2f(dy, dx) * 57.29578f - yaw;
            while (err > 180.0f) err -= 360.0f;
            while (err < -180.0f) err += 360.0f;
            state("bearing yaw %.1f err %.1f dist %.2f enemies %d hp %.1f", (double)yaw, (double)err, (double)sqrtf(bd), enemies, (double)me.health);
            if (g_mission_frames % 150 == 0)
                state("bearing_detail me %.2f %.2f %.2f face %.2f %.2f %.2f enemy %d %.2f %.2f %.2f", (double)me.pos.x, (double)me.pos.y, (double)me.pos.z, (double)me.in_now.move.x, (double)me.in_now.move.y, (double)me.in_now.move.z, best, (double)o.pos.x, (double)o.pos.y, (double)o.pos.z);
        } else
            state("bearing yaw %.1f err 0 dist -1 enemies 0 hp %.1f", (double)yaw, (double)me.health);
    }
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
