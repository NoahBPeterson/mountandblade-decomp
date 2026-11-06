//-----------------------------------------------------------------------------
// File: party_screen.cpp  (0x502cc0..0x507400)
//
// Desc: the party / prisoner management screen: window construction, the exchange halves (read / member rows /
//       bar fill), morale and money labels, the weekly wage, the two party pages, the skills page, the selection
//       refresh and the character quad. "T regime" region (docs/tu_map.md): the file keeps the binary order.
//       Names provisional. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "../../include/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40; listed directly for the resolution audit)
#include "../../include/image_addrs.h"
#include "../skill_cap.h"      // SkillCap g_skill_tbl (0x8ee3100): [36].cap is the clamp at 0x8ee3ac4
#include "./party_screen.h"
#include "../res_mgr.h"                         // g_resmgr
#include "./inv_common.h"      // (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../str_table.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../shared_vars.h"
#include "../module_name.h"      // g_module_name (0x8ee3c38)
#include "../ui_globals.h"      // UI / core globals
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

// the id lookup: visible here too, as a header inline in the original (the same definition as ui/mission_ui.cpp; the row
// owner wins the link). cl then knows the call keeps g_game, so `&g_game->parties` stays in a register across it (0x503f74).
inline int Map10007::lookup(unsigned id)
{
    if (id < 0x2717)
        return tab[id].value;
    int h = id % 0x2717;
    int n = 0;
    if (tab[h].key != id) {
        do {
            if (n >= 1000)
                return -1;
            h = (h + 0x4f) % 0x2717;
            n++;
        } while (tab[h].key != id);
    }
    if (n < 1000)
        return tab[h].value;
    return -1;
}

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
int g_item_sel;                                  // 0xa75f6c (the selected item index; negative = nothing selected)

#define g_clamp_max g_skill_tbl[36].cap         // 0x8ee3ac4 (the level cap of skill 36)
#define WINT(w, off) (*(int*)((char*)(w) + (off)))
// slot +0xa4 returns the text's size by value; nothing here reads it (0x5063e0, 0x503ed0)
#define STX(w, s) ((PsOpsW*)(w))->set_text_w(s)

// ini lookup with the scan loop expanded in the caller (0x4bd5e0 inlined: tstring::equals per entry, then atof)
static __forceinline float ini_float_scan(const tstring& key)
{
    int found = -1;
    float r = 0.0f;
    for (int i = 0; found == -1 && i < g_game_vars.count; i++) {
        if (g_game_vars.entries[i].key.equals(key))
            found = i;
    }
    if (found >= 0)
        r = (float)atof(g_game_vars.entries[found].value.c_str());
    return r;
}

// ---- helpers of the skills page (0x5059c0): the troop record a stack names, whether its stack is shown, its skill value ----
static inline Troop* rec_of(int troop) { return &g_game->troops[troop]; }
// a member's value for `skill`: members whose skill points (the hit-point field) are below the threshold count as zero;
// the threshold is the higher one for the player
static __forceinline int skill_of(const Stack* ms, int skill)
{
    int v = g_game->skill_level(ms->troop, skill);
    if (ms->troop == g_game->player) {
        if (g_game->troops[ms->troop].hp < g_module_settings.player_wounded_treshold)
            v = 0;
    } else {
        if (g_game->troops[ms->troop].hp < g_module_settings.hero_wounded_treshold)
            v = 0;
    }
    return v;
}

// @ 0x502cc0
void PartyScreen::init_screen()
{
    g_gui.reset();
    skills_list = 0;
    char_quad = 0;
    name_lbl = 0;

    {
        tstring k("party_bo_member1_h");
        bar_amount[0] = ini_float(k);
    }
    {
        tstring k("party_bo_member2_h");
        bar_amount[1] = ini_float(k);
    }
    {
        tstring k("party_bo_member1_y");
        bar_start[0] = ini_float(k);
    }
    {
        tstring k("party_bo_member2_y");
        bar_start[1] = ini_float(k);
    }

    // main window
    window = new NodeA(0);

    {
        float pos[2];
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        PSO(window)->set_pos(pos);
        pos[0] = 1.0f;
        pos[1] = 0.75f;
        PSO(window)->set_size(pos);
    }

    rgl::mesh* mesh;
    {
        tstring key("party_window_b");
        mesh = ((rgl::mesh*)g_resmgr.get_object(&key))->clone();
    }
    PSO(window)->add_image(0, mesh);
    {
        MissionUi* w = window;
        g_gui.dirty = 1;
        g_gui.objs.push_back(w);
    }

    // buttons on the window
    {
        tstring h("party_b_done_h");
        tstring k("ui_done");
        done_btn = (MissionUi*)make_short_button(window, (const tstring*)g_strtable.lookup(k), 0.0f, 0.0f, ini_float(h));
    }
    label_refresh(done_btn, tstring("party_b_done"));
    {
        tstring h("party_b_reset_h");
        tstring k("ui_reset");
        reset_btn = (MissionUi*)make_short_button(window, (const tstring*)g_strtable.lookup(k), 0.0f, 0.0f, ini_float(h));
    }
    label_refresh(reset_btn, tstring("party_b_reset"));

    // info panel
    info = new text_box;
    label_refresh(info, tstring("party_bo_info"));
    {
        MissionUi* w = info;
        ((NodeA*)window)->kids.push_back(w);
    }

    // text panels inside the info panel
    name_lbl = new MissionUi;
    label_refresh(name_lbl, tstring("party_t_selected_troop_name"));
    PSO(info)->add_child(name_lbl);

    cost_panel = new MissionUi;
    label_refresh(cost_panel, tstring("party_t_troop_cost"));
    PSO(info)->add_child(cost_panel);

    // talk / release / move buttons
    {
        tstring h("party_b_talk_h");
        tstring k("ui_talk");
        talk_btn = (MissionUi*)make_short_button(info, (const tstring*)g_strtable.lookup(k), 0.0f, 0.0f, ini_float(h));
    }
    label_refresh(talk_btn, tstring("party_b_talk"));
    {
        tstring h("party_b_release_h");
        tstring k("ui_release_one");
        action_btn = (MissionUi*)new_medium_button(info, (const tstring*)g_strtable.lookup(k), 0.0f, 0.0f, ini_float(h));
    }
    label_refresh(action_btn, tstring("party_b_release"));
    {
        tstring h("party_b_move_up_h");
        tstring k("ui_move_up");
        move_up = (MissionUi*)make_short_button(info, (const tstring*)g_strtable.lookup(k), 0.0f, 0.0f, ini_float(h));
    }
    label_refresh(move_up, tstring("party_b_move_up"));
    {
        tstring h("party_b_move_down_h");
        tstring k("ui_move_down");
        move_down = (MissionUi*)make_short_button(info, (const tstring*)g_strtable.lookup(k), 0.0f, 0.0f, ini_float(h));
    }
    label_refresh(move_down, tstring("party_b_move_down"));

    f8d4 = 0;
    f8d0 = 0;

    // upgrade panel
    upgrade_panel = new MissionUi;
    label_refresh(upgrade_panel, tstring("party_t_upgrade"));
    PSO(info)->add_child(upgrade_panel);

    {
        tstring h("party_b_upgrade1_h");
        tstring k("ui_upgrade_one");
        upgrade1 = (MissionUi*)new_long_button(info, (const tstring*)g_strtable.lookup(k), 0.0f, 0.0f, ini_float(h));
    }
    label_refresh(upgrade1, tstring("party_b_upgrade1"));
    {
        tstring h("party_b_upgrade2_h");
        tstring k("ui_upgrade_one");
        upgrade2 = (MissionUi*)new_long_button(info, (const tstring*)g_strtable.lookup(k), 0.0f, 0.0f, ini_float(h));
    }
    label_refresh(upgrade2, tstring("party_b_upgrade2"));

    sel_row = -1;
    sel_type = -1;
    sel_slot = -1;
    f8cc = 0;

    if (w04 == 0 || w04 == 5) {
        show_healthy[0] = 1; show_wounded[0] = 1; show_healthy[1] = 0; show_wounded[1] = 0;
        w10 = GD_I(0x7c);
    }
    else if (w04 == 1 || w04 == 6 || w04 == 7) {
        show_healthy[0] = 1; show_wounded[0] = 1; show_healthy[1] = 1; show_wounded[1] = 1;
        if (GD_I(0x13b00) > 0)
            w10 = GD_I(0x13b00);
        else
            w10 = GD_I(0x7c);
    }
    else if (w04 == 2) {
        show_healthy[0] = 1; show_wounded[0] = 1; show_healthy[1] = 0; show_wounded[1] = 0;
        w10 = GD_I(0x7c);
        PARTIES->get(GD_I(0x7c))->nstacks = 0;
    }
    else if (w04 == 3) {
        show_healthy[0] = 1; show_wounded[0] = 1; show_healthy[1] = 1; show_wounded[1] = 0;
        w10 = GD_I(0x13a30);
        if (!PARTIES->is_live(w10))
            mb_error("mercenary_source_party not set");
    }
    else if (w04 == 4) {
        show_healthy[0] = 1; show_wounded[0] = 1; show_healthy[1] = 1; show_wounded[1] = 1;
        if (GD_I(0x13b00) > 0)
            w10 = GD_I(0x13b00);
        else
            w10 = GD_I(0x7c);
    }

    setup_party(0);
    if (w04 == 0 || w04 == 5) {
        f8c8 = 1;
        setup_skills();
    } else {
        f8c8 = 2;
        setup_party(1);
    }

    g_game->refresh_party(g_game->f78);
    refresh_party_members(0);
    refresh_party_members(1);
    refresh_selection();
    refresh_morale();
    read(0);
    read(1);
}


// the party record of an id as a reference: `.stacks[i]` then stays one address chain (page + (record + stack) at 0x50397d)
static inline Party& party_ref(PartyMgr* m, int id)
{
    int i = m->lookup(id);
    return m->pages[i >> 4][i & 0xf];
}

// @ 0x503810
void PartyScreen::read(int which)
{
    GameData* g = g_game;
    int mainp = g->f78;
    int owner = g->player;
    if (which != 0) {
        mainp = w10;
        if (w04 == 3 || w04 == 2)
            owner = w14;
        else
            owner = *(int*)((char*)g->parties.get(mainp) + 0xac);
    }
    if (owner >= 0) {
        owner_troop[which] = g->chars[owner].f3b4;
        g = g_game;                                 // reloaded on this path only: the other keeps edi (0x503875)
    }
    {
        PartyMgr* m = &g->parties;
        party_7c[which] = *(int*)((char*)party_at(m, m->lookup(mainp)) + 0x7c);
    }
    party_80[which] = *(int*)((char*)party_index(mainp) + 0x80);
    nstacks[which] = *(int*)((char*)party_index(mainp) + 0x8a8);
    for (int i = 0; i < nstacks[which]; i++) {
        party_ref(PARTIES, mainp).stacks[i].f0c = 0;
        Stack* q = &party_index(mainp)->stacks[i];
        rows[which][i] = *(PsRow*)q;
    }
    if (which == 0) {
        for (int i = 0; i < 64; i++)
            for (int j = 0; j < 32; j++)
                table[i][j] = g_game->tbl[i][j];
    }
}

// 0x432540: floor, then "> 0.5 rounds up" (inlined here)
static inline int round_half_up(const float* p)
{
    float fl = floor(*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}

// the party's morale, through a reference (the local stays in memory across the compares)
static inline void get_morale(float& out, Party* p) { out = *(float*)((char*)p + 0x7c); }

// the morale name past the first arm goes through an inline taking the text by value: its parameter is the separate
// temporary the original has there (`mov eax,ecx; mov esi,eax` on arms 2..9, `mov esi,ecx` on arm 1)
static inline const char* text_of(const char* s) { return s; }

// @ 0x503a50
void PartyScreen::refresh_morale()
{
    char text[0x200];
    {
        float morale = 0.0f;
        get_morale(morale, PARTIES->get_w(g_game->f78));

        sprintf(text,
                g_strtable.lookup("ui_morale")->c_str(),
                morale < 0.1f ? g_strtable.lookup("ui_terrible")->c_str()
              : text_of(morale < 0.2f ? g_strtable.lookup("ui_very_low")->c_str()
              : morale < 0.3f ? g_strtable.lookup("ui_low")->c_str()
              : morale < 0.4f ? g_strtable.lookup("ui_below_average")->c_str()
              : morale < 0.6f ? g_strtable.lookup("ui_average")->c_str()
              : morale < 0.7f ? g_strtable.lookup("ui_above_average")->c_str()
              : morale < 0.8f ? g_strtable.lookup("ui_high")->c_str()
              : morale < 0.9f ? g_strtable.lookup("ui_very_high")->c_str()
              : g_strtable.lookup("ui_excellent")->c_str()));
    }

    {
        tstring line(text);
        STX(morale_lbl, line);
    }

    float morale = *(float*)((char*)party_index(g_game->f78) + 0x80);
    if (!(morale >= 0.1f))
        return;
    morale *= 100.0f;

    {
        tstring fmt("ui_starving");
        int n = round_half_up(&morale);
        sprintf(text, g_strtable.lookup(fmt)->c_str(), n);
    }

    {
        tstring line(" (");
        morale_lbl->name.append(line);
    }
    {
        tstring line(text);
        morale_lbl->name.append(line);
    }
    morale_lbl->name.push_back(')');

    {
        float rect[2];                  // slot +0x94 returns the 8-byte text rect (0x49b613)
        morale_lbl->get_xy(rect);
    }
}


static inline Party* get_party(int id) { return g_game->parties.get_w(id); }

// the troop's localised display name: its name record when the string table knows it, else its id
static inline tstring* display_name(Troop* rec)
{
    tstring* nm = g_strtable.find(&rec->key);
    if (nm)
        return nm;
    return (tstring*)rec;
}

// a hero's hit points at full health: the level term of the (clamped) strength nibble
static inline int hero_max_hp(Troop* rec)
{
    int base = rec->f58[0];
    int v = ((unsigned char*)rec)[0x7a] & 0xf;
    if (v < 0)
        v = 0;
    else if (v > g_clamp_max)
        v = g_clamp_max;
    return base + v * 2 + 0x23;
}

// a "%x" colour out of the settings table entry named by key
static __forceinline int table_colour(const tstring& k)
{
    int colour = 0;
    int found = -1;
    for (int i = 0; found == -1 && i < g_game_vars.count; i++) {
        if (g_game_vars.entries[i].key.equals(k))
            found = i;
    }
    if (found >= 0)
        sscanf(g_game_vars.entries[found].value.c_str(), "%x", &colour);
    return colour;
}

// @ 0x503ed0
void PartyScreen::refresh_party_members(int type)
{
    if ((w04 == 0 || w04 == 5) && type == 1)
        return;

    int party_id = g_game->f78;
    if (type != 0)
        party_id = w10;

    // wipe the two rows of this half
    for (int i = 0; i < 64; i++) {
        PSO(buttons[type * 2 + 0][i])->clear();
        PSO(buttons[type * 2 + 1][i])->clear();
    }
    for (int i = 0; i < 64; i++) {
        slots[type][0][i] = -1;
        slots[type][1][i] = -1;
    }

    // first wounded stack: the wounded rows are shown compacted from there on
    Party* p = get_party(party_id);
    int n = p->nstacks;
    int first_wounded = 0;
    for (int i = 0; i < n; i++) {
        if (p->stacks[i].flags & 1)
            break;
        first_wounded++;
    }

    int healthy = 0;
    int wounded = 0;
    char buf[512];

    if (party_id != -1) {
        // label every row that exists with an empty string
        for (int i = 0; i < get_party(party_id)->max_stacks(); i++) {
            {
                tstring s("");
                STX(buttons[type * 2 + 0][i], s);
            }
            PSO(buttons[type * 2 + 0][i])->disable();
        }
        for (int i = 0; i < get_party(party_id)->max_stacks(); i++) {
            {
                tstring s("");
                STX(buttons[type * 2 + 1][i], s);
            }
            PSO(buttons[type * 2 + 1][i])->disable();
        }

        for (int i = 0; i < get_party(party_id)->nstacks; i++) {
            Stack* st = &get_party(party_id)->stacks[i];

            if (!((show_healthy[type] && (st->flags & 1) == 0) || (show_wounded[type] && (st->flags & 1) != 0)))
                continue;

            if (st->flags & 1)
                wounded++;
            else
                healthy++;

            tstring label;

            if ((g_game->troops[st->troop].flags & 0x10) == 0) {
                if (st->wounded > 0 && (st->flags & 1) == 0) {        // a stack with wounded troops shows healthy / total (0x504254)
                    tstring key("ui_button_party_member_healthy_total");
                    sprintf(buf, g_strtable.lookup(key)->c_str(), display_name(&g_game->troops[st->troop])->c_str(),
                            st->count - st->wounded, st->count);
                } else {
                    tstring key("ui_button_party_member_total");
                    sprintf(buf, g_strtable.lookup(key)->c_str(), display_name(&g_game->troops[st->troop])->c_str(),
                            st->count);
                }
                label.assign(buf);
                if (st->f14 > 0) {
                    tstring plus(" +");
                    label.append(plus);
                }
            } else {
                // hero: percentage of the (clamped) level denominator
                if (g_game->agent_low(st->troop)) {
                    {
                        tstring key("ui_button_party_member_hero_percentage_wounded");
                        Troop* rec = &g_game->troops[st->troop];
                        sprintf(buf, g_strtable.lookup(key)->c_str(), display_name(rec)->c_str(),
                                (int)(g_game->troops[st->troop].hp * 100.0f / hero_max_hp(rec) + 0.0001f));
                    }
                } else {
                    {
                        tstring key("ui_button_party_member_hero_percentage");
                        Troop* rec = &g_game->troops[st->troop];
                        sprintf(buf, g_strtable.lookup(key)->c_str(), display_name(rec)->c_str(),
                                (int)(g_game->troops[st->troop].hp * 100.0f / hero_max_hp(rec) + 0.0001f));
                    }
                }
                label.assign(buf);
                if (g_game->troops[st->troop].fa8 != 0) {
                    tstring plus(" +");
                    label.append(plus);
                }
            }

            int row = 0;
            int col = i;
            if (st->flags & 1) {
                row = 1;
                col = i - first_wounded;
            }
            int cell = (type * 2 + row) * 64 + col;

            STX(buttons[0][cell], label);
            if ((g_game->troops[st->troop].flags & 0x10) && g_game->agent_low(st->troop)) {
                tstring k("party_b_member_t_c_wounded");
                int c = table_colour(k);
                PSO(buttons[0][cell])->set_colour(&c);
            } else {
                tstring k("party_b_member_t_c");
                int c = table_colour(k);
                PSO(buttons[0][cell])->set_colour(&c);
            }
            PSO(buttons[0][cell])->act_b4();
            PSO(buttons[0][cell])->enable();
            ((int*)slots)[cell] = i;
        }

        fill_member(type, 0, get_party(party_id), healthy);
        fill_member(type, 1, get_party(party_id), wounded);
    }

    if (type == 0)
        money_label(type, g_game->chars[g_game->player].f3b4);

    if (type == 0) {
        int cost = GV->weekly_cost();
        if (cost > 0) {
            {
                tstring key("ui_weekly_cost_value");
                tstring val = g_game->string_value(cost);
                sprintf(buf, g_strtable.lookup(key)->c_str(), val.c_str());
            }
            tstring text(buf);
            STX(weekly_cost_lbl, text);
        } else {
            tstring text("");
            STX(weekly_cost_lbl, text);
        }

        {
            tstring key("ui_company");
            sprintf(buf, g_strtable.lookup(key)->c_str(), get_party(g_game->f78)->sum_head(),
                    get_party(g_game->f78)->max_members());
        }
        {
            tstring text(buf);
            STX(max_companions, text);
        }

        {
            tstring key("ui_prisoners_equal_value");
            sprintf(buf, g_strtable.lookup(key)->c_str(), get_party(g_game->f78)->sum_flagged(),
                    get_party(g_game->f78)->max_prisoners());
        }
        {
            tstring text(buf);
            STX(max_prisoners, text);
        }
    }
}


// @ 0x504ba0
void PartyScreen::fill_member(int page, int type, Party* unused, int count)
{
    (void)unused;
    float unit;
    {
        tstring key("party_b_member_h");
        unit = 0.0f;
        int n = g_game_vars.find(key);
        if (n >= 0)
            unit = (float)atof(g_game_vars.entries[n].value.c_str());
    }
    float total = (float)count * unit;
    if (total < bar_amount[type]) {
        total += unit * 0.2f;
        if (!(total < bar_amount[type]))
            total = bar_amount[type];
    }

    for (int i = 0; i < count; i++) {
        MissionUi* bar = buttons[type + page * 2][i];
        float p[2];
        *(int*)&p[0] = WINT(bar, 0xe8);
        p[1] = total - (float)(i + 1) * unit;
        PSO(bar)->set_pos(p);
    }

    {
        MissionUi* a = ((MissionUi**)members_page)[type + page * 2];
        float p[2];
        *(int*)&p[0] = WINT(a, 0xf0);
        p[1] = total;
        PSO(a)->set_size(p);
    }

    float x = (bar_amount[type] < total) ? bar_amount[type] : total;
    MissionUi** pb = &((MissionUi**)member_list)[type + page * 2];
    {
        float p[2];
        *(int*)&p[0] = WINT(*pb, 0xf0);
        p[1] = x;
        PSO(*pb)->set_size(p);
    }
    {
        float p[2];
        *(int*)&p[0] = WINT(*pb, 0xe8);
        p[1] = bar_start[type] + bar_amount[type] - x;
        PSO(*pb)->set_pos(p);
    }

    quarter_update(*pb);
}

// @ 0x504d50
void PartyScreen::money_label(int type, int troop)
{
    if (type == 2 || type != 0)
        return;

    char buf[512];
    {
        tstring key("ui_money_value");
        sprintf(buf, g_strtable.lookup(key)->c_str(), g_game->string_value(troop).c_str());
    }
    {
        tstring text(buf);
        ((PsOpsW*)gold)->set_text_w(text);
    }
}

// ---------------------------------------------------------------------------------------------------------
// The Mission thunk 0x504ba0 forwards into the party menu (see the note above the marker):
//     mov ecx, [ecx + 0x5c]   ; page = the Mission's widget/anchor at +0x5c
//     push edx                ; buffer
//     push eax                ; count
//     jmp 0x5966c0            ; party_menu::fill_member(??)
// Our reconstruction below is the menu method itself; when the menu object is laid out with the widget
// anchor as its first field, Mission's ecx is exactly this pointer, which is why the original keeps ESI
// (= ecx) alive across the whole body instead of reloading `this`.

// @ 0x504e20
int GameDataV::weekly_cost()
{
    g_script.trigger_result.store0 = 0;
    g_script.trigger_result.store1 = 0;
    int total = 0;
    if (!sl_query(8, 0, 0)) {
        Party* p = parties.get_w(f78);
        for (int i = 1; i < p->nstacks; i++) {
            Stack* st = &p->stacks[i];
            if (st->flags & 1)
                break;
            total += (troops[st->troop].flags & 0x10) ? sl_party_value(st->troop) : sl_party_value(st->troop) * st->count;
        }
        return total;
    }
    return g_script.trigger_result.store0;
}


// a widget's position relative to its parent, as the two-float value set_pos takes
static inline void place_rel(MissionUi* w, MissionUi* parent)
{
    const float* b = &parent->fE0;
    float d[2];
    d[1] = w->fE4 - b[1];
    d[0] = w->fE0 - b[0];
    PSO(w)->set_pos(d);
}

// Party::display_name: the party's id when the module asks for ids, else its translation, else its name
static inline tstring* party_shown_name(Party* rec)
{
    if (g_module_settings.show_party_ids_instead_of_names)
        return &rec->key;
    if (rec->flag == 0) {
        tstring* n = g_strtable.find(&rec->key);
        if (n)
            return n;
    }
    return &rec->name;
}

// @ 0x504ef0
void PartyScreen::setup_party(int side)
{
    members_base[side] = new text_box;
    if (side == 0) {
        label_refresh(members_base[0], tstring("party_bo_party1"));
        if (g_game->skill_level(g_game->player, 2) == 0) {
            prisoner_mgmt = new MissionUi;
            label_refresh(prisoner_mgmt, tstring("party_t_prisoner_management"));
            {
                tstring k("ui_need_to_learn_prisoner_management");
                STX(prisoner_mgmt, *g_strtable.lookup(k));
            }
            PSO(members_base[0])->add_child(prisoner_mgmt);
        }
    } else {
        label_refresh(members_base[side], tstring("party_bo_party2"));
    }

    for (int page = 0; page < 2; page++) {
        ContainerBase* c = new ContainerBase(ContainerBase::inl_tag());
        MissionUi** mp = &members_page[side][page];
        *mp = c;
        label_refresh(*mp, tstring("party_bo_members_base"));
        // the scrollbar goes on the base, the list on this page (0x5050cc / 0x505131); each name dies after its call
        if (page == 0) {
            MissionUi* sc;
            {
                tstring s1("party_s_members1");
                sc = (MissionUi*)make_scrollbar((void*)members_base[side], &s1);
            }
            tstring s2("party_sc_members1");
            member_list[side][page] = (MissionUi*)make_editor_list((void*)members_page[side][page], &s2, (int)sc);
        } else {
            MissionUi* sc;
            {
                tstring s1("party_s_members2");
                sc = (MissionUi*)make_scrollbar((void*)members_base[side], &s1);
            }
            tstring s2("party_sc_members2");
            member_list[side][page] = (MissionUi*)make_editor_list((void*)members_page[side][page], &s2, (int)sc);
        }
        PSO(members_base[side])->add_child(member_list[side][page]);

        for (int i = 0; i < 0x40; i++) {
            {
                tstring k_button_h("party_b_members_button_h");
                tstring k_width_ratio("party_b_members_width_ratio");
                tstring k_h("party_b_members_h");
                tstring k_x("party_b_members_x");
                tstring k_empty("");                    // the button's name (arg 1 at 0x5053ac is this "", not k_x)
                float ev    = ini_float_scan(k_button_h);
                float ratio = ini_float_scan(k_width_ratio);
                float h     = ini_float_scan(k_h);
                float x     = ini_float_scan(k_x);
                MissionUi* parent = *mp;
                MissionUi* b;
                {
                    tstring down("party_member_button_pressed");
                    tstring up("party_member_button");
                    b = (MissionUi*)new_sized_button(parent, k_empty, x, 0.0f, h, up, down, ratio, ev, 0.085f);
                }
                buttons[side * 2 + page][i] = b;
            }
            ((UiA*)buttons[side * 2 + page][i])->f184 |= 3;
        }
    }

    // the member base is a child of the party body
    {
        MissionUi* v = members_base[side];
        ((NodeA*)window)->kids.push_back(v);
    }

    if (side == 0) {
        gold = new MissionUi;
        label_refresh(gold, tstring("party_t_gold"));
        place_rel(gold, members_base[0]);
        PSO(members_base[0])->add_child(gold);

        weekly_cost_lbl = new MissionUi;
        label_refresh(weekly_cost_lbl, tstring("party_t_weekly_cost"));
        place_rel(weekly_cost_lbl, members_base[0]);
        PSO(members_base[0])->add_child(weekly_cost_lbl);

        max_companions = new MissionUi;
        label_refresh(max_companions, tstring("party_t_max_companions"));
        place_rel(max_companions, members_base[0]);
        PSO(members_base[0])->add_child(max_companions);

        max_prisoners = new MissionUi;
        label_refresh(max_prisoners, tstring("party_t_max_prisoners"));
        place_rel(max_prisoners, members_base[0]);
        PSO(members_base[0])->add_child(max_prisoners);

        morale_lbl = new MissionUi;
        label_refresh(morale_lbl, tstring("party_t_morale"));
        place_rel(morale_lbl, members_base[0]);
        PSO(members_base[0])->add_child(morale_lbl);
    } else {
        choose_companions = new MissionUi;
        label_refresh(choose_companions, tstring("party_t_choose_companions"));
        place_rel(choose_companions, members_base[side]);
        PSO(members_base[side])->add_child(choose_companions);

        choose_prisoners = new MissionUi;
        label_refresh(choose_prisoners, tstring("party_t_choose_prisoners"));
        place_rel(choose_prisoners, members_base[side]);
        PSO(members_base[side])->add_child(choose_prisoners);

        // the two captions depend on what kind of party is shown
        if (w04 == 1 || w04 == 6 || w04 == 7) {
            // Party::display_name inlined (0x505949): the ids, else the string-table entry, else the name (+0xc)
            STX(choose_companions, *party_shown_name(get_party(w10)));
            tstring empty("");
            STX(choose_prisoners, empty);
        } else if (w04 == 3 || w04 == 2) {
            // each caption string dies before the next is built (0x505886)
            {
                tstring empty("");
                STX(choose_companions, empty);
            }
            {
                tstring empty2("");
                STX(choose_prisoners, empty2);
            }
        } else {
            {
                tstring s1("ui_rescued_prisoners");
                STX(choose_companions, *g_strtable.lookup(s1));
            }
            {
                tstring s2("ui_captured_enemies");
                STX(choose_prisoners, *g_strtable.lookup(s2));
            }
        }
    }
}


static inline bool attr_bit(unsigned v, int n) { return ((v >> n) & 1) != 0; }

// @ 0x5059c0
void PartyScreen::setup_skills()
{
    // ---- the skills panel ----
    skills_base = new text_box;
    label_refresh(skills_base, tstring("party_bo_skills"));
    {
        tstring empty("");
        STX(skills_base, empty);
    }
    MissionUi* bar = new MissionUi;                       // 0x178, ctor 0x48ae20
    place_rel(bar, skills_base);
    {
        tstring key("ui_party_skills");
        STX(bar, *g_strtable.lookup(key));
    }
    PSO(skills_base)->add_child(bar);

    // ---- the party this page shows and its members ----
    Party* members;
    {
        PartyMgr* pm = PARTIES;
        int idx = pm->lookup(g_game->f78);
        members = (Party*)((idx & 0xf) * 0x5ac8 + (int)pm->pages[idx >> 4]);
    }

    // ---- the page's layout steps ----
    float t_y;
    {
        tstring k("party_bo_skills_t_y");
        t_y = ini_float(k);
    }
    float t_y_a;
    {
        tstring k("party_bo_skills_t_y_a");
        t_y_a = ini_float(k);
    }
    float t_y_a_short;
    {
        tstring k("party_bo_skills_t_y_a_short");
        t_y_a_short = ini_float(k);
    }
    float t_x;
    {
        tstring k("party_bo_skills_t_x");
        t_x = ini_float(k);
    }
    float t_x_deep;
    {
        tstring k("party_bo_skills_t_x_deep");
        t_x_deep = ini_float(k);
    }

    // ---- pass 1: the content height ----
    float y = 0.0f;
    for (int m = 0; m < members->nstacks; m++) {
        if ((members->stacks[m].flags & 1) != 0 || (g_game->troops[members->stacks[m].troop].flags & 0x10) == 0)
            continue;
        y += t_y_a;
        for (int k = 0; k < 42; k++) {
            if (!attr_bit(g_skill_tbl[k].attr, 4))
                continue;
            int best = 0;
            int best_i = -1;
            for (int i = 0; i < members->nstacks; i++) {
                if ((members->stacks[i].flags & 1) != 0 || (g_game->troops[members->stacks[i].troop].flags & 0x10) == 0)
                    continue;
                int v = skill_of(&members->stacks[i], k);
                if (v > best) {
                    best = v;
                    best_i = i;
                }
            }
            if (best_i == m)
                y += t_y_a_short;
        }
    }
    {
        float h = t_y_a + y;
        y = t_y + h;
        float pos[2];
        pos[0] = skills_base->screen[0];
        pos[1] = h;
        PSO(skills_base)->set_size(pos);
    }

    // ---- pass 2: a caption line per member and a line per skill ----
    char buf[512];
    for (int mi = 0; mi < members->nstacks; mi++) {
        if ((members->stacks[mi].flags & 1) != 0 || (g_game->troops[members->stacks[mi].troop].flags & 0x10) == 0)
            continue;
        Troop* r = rec_of(members->stacks[mi].troop);
        tstring* nm = (tstring*)r;
        tstring* tr = g_strtable.find(&r->key);
        if (tr)
            nm = tr;                  // 0x505f14: the record itself (esi), not its key
        tstring cap(*nm);
        cap.capitalize();
        sprintf(buf, "%s:", cap.c_str());
        {
            tstring line(buf);
            float pos[2];
            pos[0] = t_x;
            pos[1] = y;
            PSO(skills_base)->set_label(&line, pos);
        }
        y -= t_y_a;

        for (int k = 0; k < 42; k++) {
            SkillRec* se = &g_skill_tbl[k];
            if (!attr_bit(se->attr, 4))
                continue;
            int best = 0;
            int best_i = -1;
            for (int i = 0; i < members->nstacks; i++) {
                if ((members->stacks[i].flags & 1) != 0 || (g_game->troops[members->stacks[i].troop].flags & 0x10) == 0)
                    continue;
                int v = skill_of(&members->stacks[i], k);
                if (v > best) {
                    best = v;
                    best_i = i;
                }
            }
            if (best_i == mi) {
                int pq = members->skill_level(k);
                if (pq == best) {
                    tstring* sn = g_strtable.find(&se->key);
                    if (sn == 0)
                        sn = &se->name;         // 0x5060d1: the row itself (edi), not its key
                    sprintf(buf, "%s (%d)", sn->c_str(), best);
                } else {
                    tstring* sn = g_strtable.find(&se->key);
                    if (sn == 0)
                        sn = &se->name;
                    sprintf(buf, "%s (%d)(+%d)", sn->c_str(), best, pq - best);
                }
                {
                    tstring line(buf);
                    float pos[2];
                    pos[0] = t_x_deep;
                    pos[1] = y;
                    PSO(skills_base)->set_label(&line, pos);
                }
                y -= t_y_a_short;
            }
        }
    }

    // ---- the scroll list ----
    UiD* sb;
    {
        tstring name("party_s_skills");
        sb = make_scrollbar((void*)window, &name);
    }
    {
        tstring name("party_sc_skills");
        MissionUi* panel = skills_base;
        Camera* list = new_scroll_list();                  // 0x4a8, ctor 0x49deb0
        list->f104 |= 0x20;
        if (sb != 0)
            list->follow[1] = sb;           // +0x484 the scroll bar
        list->target = (ContainerBase*)panel;            // +0x490
        PSO(list)->add_child(panel);
        label_refresh(list, name);
        skills_list = list;
    }

    // ---- place the list in the panel ----
    {
        float dy = skills_base->screen[1] > skills_list->screen[1] ? 0.0f : skills_list->screen[1] - skills_base->screen[1];
        float pos[2];
        pos[0] = skills_list->fE0;
        pos[1] = dy + skills_list->fE4;
        PSO(skills_list)->set_pos(pos);
    }
    {
        const float& lower = skills_base->screen[1] > skills_list->screen[1] ? skills_list->screen[1] : skills_base->screen[1];     // 0x50630c: the lower of the two
        float pos[2];
        pos[0] = skills_list->screen[0];
        pos[1] = lower;
        PSO(skills_list)->set_size(pos);
    }
    quarter_update((MissionUi*)skills_list);
    PSO(window)->add_child(skills_list);

    // ---- the party-skills information bar ----
    party_skills_bar = new MissionUi;
    label_refresh(party_skills_bar, tstring("party_t_party_skills"));
    {
        tstring key("ui_party_skills");
        STX(party_skills_bar, *g_strtable.lookup(key));
    }
    PSO(window)->add_child(party_skills_bar);
}


// the price a prisoner sells for: the script's answer, else the game's ransom value
static inline int sale_price(GameData* g, int troop)
{
    g_script.trigger_result.value = 0;
    if (g->emit_event(10, troop, g->f78))
        return g_script.trigger_result.store0;
    return *(int*)((char*)g + 0x13a20);
}

static inline Party* party_of(GameData* g) { return g->parties.get(g->f78); }
static inline bool stack_nb(const Stack& s) { return (s.flags & 1) != 0; }

// @ 0x5063e0
void PartyScreen::refresh_selection()
{
    { tstring t(""); STX(info, t); }

    PSO(talk_btn)->disable();
    locked = 1;
    PSO(action_btn)->disable();
    PSO(action_btn)->clear();
    PSO(upgrade1)->disable();
    PSO(upgrade2)->disable();
    PSO(upgrade1)->clear();
    PSO(upgrade2)->clear();
    PSO(move_up)->disable();
    PSO(move_down)->disable();
    PSO(cost_panel)->clear();
    PSO(upgrade_panel)->clear();
    if (char_quad) {
        { tstring t(""); STX(name_lbl, t); }
        PSO(char_quad)->add_image(0, 0);
    }
    tstring label;                                  // the action button's caption (constructed at 0x5064e5)

    if (sel_row == -1 || sel_type == -1 || sel_slot == -1)
        return;                                     // nothing selected (0x5064f3..0x506515 -> 0x5072f6)
    int slot = sel_slot;
    GameData* g = g_game;
    int p1 = w10;                                   // the party a moved stack goes to
    if (sel_type != 0)
        p1 = g->f78;
    int slot_idx = slots[sel_type][sel_slot][sel_row];      // (sel_slot*2 + sel_type) << 6
    int p2 = g->f78;                                // the party the selected stack is in
    if (sel_type != 0)
        p2 = w10;
    Stack stk = g->parties.get(p2)->stacks[slot_idx];
    if (stk.troop < 0)
        return;                                     // an empty stack slot (0x506586 -> 0x5072e2)
    TroopRec* rec = g->chars + stk.troop;
    show_selected_character(stk.troop, slot);   // (0x5065a0 pushes sel_slot)
    bool action = false;
    if (w04 == 0) {
        if (sel_row != -1 && !(sel_row == 0 && sel_slot == 0))
            PSO(talk_btn)->enable();
    }

    if (w04 == 0 || w04 == 5) {
        if (sel_type == 0)
            { tstring k("ui_disband"); label = *g_strtable.lookup(k); }
        else if (stk.flags & 1)
            { tstring k("ui_take_prisoner"); label = *g_strtable.lookup(k); }
        else
            { tstring k("ui_take_back"); label = *g_strtable.lookup(k); }
        if (sel_row != -1 && !(sel_type == 0 && sel_row == 0 && sel_slot == 0))
            action = true;
    } else if (w04 == 1 || w04 == 6 || w04 == 7) {
        if (sel_type == 0)
            { tstring k("ui_give"); label = *g_strtable.lookup(k); }
        else
            { tstring k("ui_take"); label = *g_strtable.lookup(k); }
        if (sel_row != 0 || sel_slot != 0 || w04 == 6) {
            if ((sel_type == 0 && sel_slot == 0) || w04 != 7)      // 0x5068b8..0x5068d4
                action = true;
        }
    } else if (w04 == 2) {
        { tstring k("ui_sell"); label = *g_strtable.lookup(k); }
        if (sel_type == 0 && (stk.flags & 1) != 0) {
            g_script.trigger_result.value = 1;
            // a script that answers and leaves the result clear refuses the sale (0x506684..0x50669e)
            if (g_game->emit_event(0xb, stk.troop, 0) && g_script.trigger_result.value == 0)
                action = false;
            else
                action = true;
        }
    } else if (w04 == 3) {
        { tstring k("ui_hire"); label = *g_strtable.lookup(k); }
        if (sel_type != 0) {
            if (g_game->chars[g_game->player].f3b4 < GV->sl_party_price(stk.troop) - f8cc) {
                tstring ky("party_info_warn_y");
                float y = g_game_vars.get_float(ky);
                tstring kx("party_info_warn_x");
                float x = g_game_vars.get_float(kx);
                tstring msg("ui_notification_cant_hire");
                float pt[2];
                pt[0] = x;
                pt[1] = y;
                PSO(info)->set_label(g_strtable.lookup(msg), pt);
            } else {
                action = true;
            }
        }
    } else if (w04 == 4) {
        action = true;
        if (sel_type == 0)
            { tstring k("ui_uncapture"); label = *g_strtable.lookup(k); }
        else if (stk.flags & 1)
            { tstring k("ui_capture"); label = *g_strtable.lookup(k); }
        else
            { tstring k("ui_hire"); label = *g_strtable.lookup(k); }
    }

    int cur = g_game->f78;
    if (p2 == cur) {
        int n = PARTIES->get(cur)->first_flagged();
        if (slot_idx > 1 && slot_idx < n)
            PSO(move_up)->enable();
        if (slot_idx > 0 && slot_idx < n - 1)
            PSO(move_down)->enable();
    }

    if (action) {
        // (the stack's whole flags dword is passed: 0x5069ca)
        if (PARTIES->get(p1)->find_or_add(stk.troop, stk.flags, 1) == -1) {
            action = false;
            tstring ky("party_info_warn_y");
            float y = g_game_vars.get_float(ky);
            tstring kx("party_info_warn_x");
            float x = g_game_vars.get_float(kx);
            tstring msg("ui_party_capcity_reached");
            float pt[2];
            pt[0] = x;
            pt[1] = y;
            PSO(info)->set_label(g_strtable.lookup(msg), pt);
        }
    }

    unsigned __int64 fl = rec->flags;
    if (((fl & 0x10) != 0 && sel_type == 0 && w04 != 6) || (fl & 0x10000000) != 0) {
        action = false;                             // 0x506adb: the later `w04 == 2 && action` reads the cleared flag
    } else if (action) {
        PSO(action_btn)->enable();
        locked = 0;
    }

    tstring caption(label);                         // a copy (0x506b08), alive to the end
    STX(action_btn, caption);

    char cost_buf[512];                             // esp+0x2b0
    sprintf(cost_buf, "");
    if (stk.troop != g_game->player && (stk.flags & 1) == 0) {
        if (sel_type == 1 && w04 == 3) {
            tstring k("ui_joining_cost_weekly_wage");
            const char* fmt = g_strtable.lookup(k)->c_str();
            sprintf(cost_buf, fmt, GV->sl_party_price(stk.troop), GV->sl_party_value(stk.troop));
        } else {
            tstring k("ui_weekly_wage");
            const char* fmt = g_strtable.lookup(k)->c_str();
            sprintf(cost_buf, fmt, GV->sl_party_value(stk.troop));
        }
        { tstring t(cost_buf); STX(cost_panel, t); }
        PSO(cost_panel)->act_b4();
    } else if (w04 == 2 && action) {
        {
            tstring k("ui_price");
            int v = sale_price(g_game, stk.troop);
            sprintf(cost_buf, g_strtable.lookup(k)->c_str(), v);
        }
        { tstring t(cost_buf); STX(cost_panel, t); }
        PSO(cost_panel)->act_b4();
    }

    // the two upgrade buttons: shown (a) when the troop has that upgrade, available (c) when the party has room
    bool shown1 = false, shown2 = false, room1 = false, room2 = false;
    char upgrade_buf[512];                          // esp+0x4b0
    char name_buf[512];                             // esp+0xb0
    sprintf(upgrade_buf, "");
    int ready = stk.f14;                            // the stack's troops ready to upgrade
    if (sel_type == 0 && (rec->flags & 0x10) == 0 && !stack_nb(stk)) {
        {
            tstring k("ui_number_ready_to_upgrade");
            sprintf(upgrade_buf, g_strtable.lookup(k)->c_str(), ready);
        }
        if (rec->f50 != 0) {
            shown1 = true;
            {
                tstring k("ui_upgrade_to_value");
                sprintf(name_buf, g_strtable.lookup(k)->c_str(), g_game->chars[rec->f50].display_name()->c_str());
            }
            { tstring t(name_buf); STX(upgrade1, t); }
            {
                tstring k("party_b_member_t_c");
                int colour = g_game_vars.hex_field(k);
                PSO(upgrade1)->set_colour(&colour);
            }
        }
        if (*(unsigned*)&rec->upgrade_troop[1] != 0) {
            shown2 = true;
            {
                tstring k("ui_upgrade_to_value");
                sprintf(name_buf, g_strtable.lookup(k)->c_str(), g_game->chars[*(unsigned*)&rec->upgrade_troop[1]].display_name()->c_str());
            }
            { tstring t(name_buf); STX(upgrade2, t); }
            {
                tstring k("party_b_member_t_c");
                int colour = g_game_vars.hex_field(k);
                PSO(upgrade2)->set_colour(&colour);
            }
        }
        if (shown1) {
            PSO(upgrade1)->act_b4();
            PSO(upgrade1)->disable();
            if (ready != 0 && PARTIES->get(p2)->find_or_add(rec->f50, 0, 0) != -1)
                room1 = true;
        }
        if (shown2) {
            PSO(upgrade2)->act_b4();
            PSO(upgrade2)->disable();
            if (ready != 0 && PARTIES->get(p2)->find_or_add(*(unsigned*)&rec->upgrade_troop[1], 0, 0) != -1)
                room2 = true;
        }
    }
    if (room1) {                                    // 0x507031..0x507064
        PSO(upgrade1)->enable();
    } else if (!stack_nb(stk) && ready >= stk.count) {
        PSO(upgrade1)->enable();
        room1 = true;
    }
    if (room2) {
        PSO(upgrade2)->enable();
    } else if (!stack_nb(stk) && ready >= stk.count) {
        PSO(upgrade2)->enable();
        room2 = true;
    }
    if (ready != 0) {
        if (!room2) {
            {
                tstring k("ui_notification_no_slot_for_upgrade");
                sprintf(name_buf, g_strtable.lookup(k)->c_str(),
                        g_game->chars[*(unsigned*)&rec->upgrade_troop[1]].display_name()->c_str());
            }
            { tstring t(name_buf); STX(upgrade2, t); }
            {
                tstring k("party_b_upgrade_t_c_passive");
                int colour = g_game_vars.hex_field(k);
                PSO(upgrade2)->set_colour(&colour);
            }
        }
        if (!room1) {
            {
                tstring k("ui_notification_no_slot_for_upgrade");
                sprintf(name_buf, g_strtable.lookup(k)->c_str(), g_game->chars[rec->f50].display_name()->c_str());
            }
            { tstring t(name_buf); STX(upgrade1, t); }
            {
                tstring k("party_b_upgrade_t_c_passive");
                int colour = g_game_vars.hex_field(k);
                PSO(upgrade1)->set_colour(&colour);
            }
        }
        PSO(upgrade_panel)->act_b4();
    }
    { tstring t(upgrade_buf); STX(upgrade_panel, t); }
    if (w04 == 3) {
        if (sel_type == 0) {
            PSO(action_btn)->disable();
            locked = 1;
        } else {
            PSO(action_btn)->act_b4();              // +0xb4 (0x5072c6)
        }
    } else {
        PSO(action_btn)->act_b4();                  // +0xb4 (0x5072d0)
    }
}


// the troop's display name, expanded in place (0x43a480 inlined here): its translation, else the record's own name
static inline tstring* troop_name(TroopRec* t)
{
    tstring* n = g_strtable.find(&t->key);
    return n ? n : (tstring*)t;
}

// @ 0x507310
void PartyScreen::show_selected_character(int troop, int slot)
{
    int item = g_item_sel;
    if (item < 0)
        return;

    if (!char_quad) {
        char_quad = new MissionUi;
        label_refresh(char_quad, "party_i_character");
        PSO(window)->add_child(char_quad);
    }

    int t = troop;
    troop = slot + troop * 2;
    PSO(char_quad)->add_image(0, g_tableaus.make_mesh_from(&item, &troop));

    TroopRec* rec = (TroopRec*)((char*)g_game->chars + t * 0x508);
    float size[2];
    PSO(name_lbl)->set_text(size, troop_name(rec));
}

