//-----------------------------------------------------------------------------
// File: party_screen.cpp  (0x502cc0..0x507400)
//
// Desc: the party / prisoner management screen: window construction, the exchange halves (read / member rows /
//       bar fill), morale and money labels, the weekly wage, the two party pages, the skills page, the selection
//       refresh and the character quad. "T regime" region (docs/tu_map.md): the file keeps the binary order.
//       Names provisional. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./party_screen.h"
#include "./inv_common.h"      // (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../str_table.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"

extern const char g_empty_string[];                 // 0x54a480 ("")
extern tstring g_module_name;                       // 0x8ee3c38 (end of the skill table)
extern int g_skill_clamp;                  // 0x8ee3ac4
#define g_clamp_max g_skill_clamp              // (clamped level value of the hero percentage; lives outside the loaded image)
#define WINT(w, off) (*(int*)((char*)(w) + (off)))

// ini lookup with the scan loop expanded in the caller (0x4bd5e0 inlined: tstring::equals per entry, then atof)
static __forceinline float ini_float_scan(const tstring& key)
{
    float r = 0.0f;
    int idx = -1;
    for (int i = 0; i < g_game_vars.count; i++) {
        if (g_game_vars.entries[i].key.equals(key)) { idx = i; break; }
    }
    if (idx >= 0) {
        char* p = g_game_vars.entries[idx].value.buf;
        if (p == 0) p = (char*)&g_game_vars.entries[idx].value.buf;
        r = (float)atof(p);
    }
    return r;
}

// ---- helpers of the skills page (0x5059c0): the troop record a stack names, whether its stack is shown, its skill value ----
static inline Troop* rec_of(int troop) { return &g_game->troops[troop]; }
static inline bool shown(const Stack* ms) { return (ms->flags & 1) == 0 && (rec_of(ms->troop)->flags32 & 0x10) != 0; }
// a member's value for `skill`: members whose skill points (the hit-point field) are below the threshold count as zero;
// the threshold is the higher one for the player
static inline int skill_of(const Stack* ms, int skill)
{
    int v = g_game->skill_level(ms->troop, skill);
    Troop* r = rec_of(ms->troop);
    if (ms->troop == g_game->player) {
        if (r->hp < g_player_wounded_treshold)
            v = 0;
    } else {
        if (r->hp < g_hero_wounded_treshold)
            v = 0;
    }
    return v;
}

// a label text pushed into a widget through the string (slot +0xa4, then the +0xb4 refresh)
static void put_text(MissionUi* w, const char* s)
{
    tstring t(s);
    float out[4];
    PSO(w)->set_text(out, &t);
    PSO(w)->act_b4();
}

// helper: build a button from a string-table label and an ini height
static MissionUi* button(PartyScreen* self, MissionUi* parent, const char* label_key, const char* cfg_key)
{
    tstring k(label_key);
    tstring h(cfg_key);
    tstring* text = g_strtable.lookup(k);
    return make_short_button(parent, text, 0, 0, ini_float(h));
}

// @ 0x502cc0
void PartyScreen::open_party_window()
{
    g_gui_root.reset();
    skills_list = 0;
    char_quad = 0;
    name_lbl = 0;

    bar_amount[0] = ini_float(tstring("party_bo_member1_h"));
    bar_amount[1] = ini_float(tstring("party_bo_member2_h"));
    bar_start[0] = ini_float(tstring("party_bo_member1_y"));
    bar_start[1] = ini_float(tstring("party_bo_member2_y"));

    // main window
    PsWindow* win = (PsWindow*)::operator new(0x190);
    if (win) {
        window_ctor(win);
        *(void**)win = (void*)0x592848;
        win->f188 = 0;
    }
    window = win;

    float pos[2];
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    PSO(win)->set_pos(pos);
    pos[0] = 1.0f;
    pos[1] = 0.75f;
    PSO(win)->set_size(pos);

    {
        tstring key("party_window_b");
        ResMesh* m = (ResMesh*)((resmgr*)&g_resmgr_inv)->get_object((const rgl::string*)&key);
        ResMesh* c = m->clone();
        PSO(win)->add_image(0, c);
    }
    g_ui_changed = 1;
    g_gui_root.add_root((MissionUi**)&window);

    // buttons on the window
    done_btn = button(this, win, "ui_done", "party_b_done_h");
    label_refresh(done_btn, tstring("party_b_done"));
    reset_btn = button(this, win, "ui_reset", "party_b_reset_h");
    label_refresh(reset_btn, tstring("party_b_reset"));

    // info panel
    MissionUi* panel = (MissionUi*)::operator new(0x188);
    if (panel) {
        window_ctor(panel);
        *(void**)panel = (void*)0x597050;
    }
    info = panel;
    label_refresh(info, tstring("party_bo_info"));
    win->panels.push_back(info);

    // text panels inside the info panel
    name_lbl = (MissionUi*)::operator new(0x178);
    if (name_lbl) panel_ctor(name_lbl);
    label_refresh(name_lbl, tstring("party_t_selected_troop_name"));
    PSO(info)->add_child(name_lbl);

    cost_panel = (MissionUi*)::operator new(0x178);
    if (cost_panel) panel_ctor(cost_panel);
    label_refresh(cost_panel, tstring("party_t_troop_cost"));
    PSO(info)->add_child(cost_panel);

    // talk / release / move buttons
    talk_btn = button(this, info, "ui_talk", "party_b_talk_h");
    label_refresh(talk_btn, tstring("party_b_talk"));

    {
        tstring k("ui_release_one");
        tstring h("party_b_release_h");
        tstring* text = g_strtable.lookup(k);
        action_btn = new_medium_button(info, text, 0, 0, ini_float(h));
    }
    label_refresh(action_btn, tstring("party_b_release"));

    move_up = button(this, info, "ui_move_up", "party_b_move_up_h");
    label_refresh(move_up, tstring("party_b_move_up"));
    move_down = button(this, info, "ui_move_down", "party_b_move_down_h");
    label_refresh(move_down, tstring("party_b_move_down"));

    f8d4 = 0;
    f8d0 = 0;

    // upgrade panel
    upgrade_panel = (MissionUi*)::operator new(0x178);
    if (upgrade_panel) panel_ctor(upgrade_panel);
    label_refresh(upgrade_panel, tstring("party_t_upgrade"));
    PSO(info)->add_child(upgrade_panel);

    {
        tstring k("ui_upgrade_one");
        tstring h("party_b_upgrade1_h");
        tstring* text = g_strtable.lookup(k);
        upgrade1 = new_long_button(info, text, 0, 0, ini_float(h));
    }
    label_refresh(upgrade1, tstring("party_b_upgrade1"));

    {
        tstring k("ui_upgrade_one");
        tstring h("party_b_upgrade2_h");
        tstring* text = g_strtable.lookup(k);
        upgrade2 = new_long_button(info, text, 0, 0, ini_float(h));
    }
    label_refresh(upgrade2, tstring("party_b_upgrade2"));

    sel_row = -1;
    sel_type = -1;
    sel_slot = -1;
    int kind = mode;
    f8cc = 0;

    if (kind == 0 || kind == 5) {
        show_healthy[0] = 1; show_wounded[0] = 1; show_healthy[1] = 0; show_wounded[1] = 0;
        other_party = GD_I(0x7c);
    }
    else if (kind == 1 || kind == 6 || kind == 7) {
        show_healthy[0] = 1; show_wounded[0] = 1; show_healthy[1] = 1; show_wounded[1] = 1;
        int x = GD_I(0x13b00);
        other_party = (x > 0) ? x : GD_I(0x7c);
    }
    else if (kind == 2) {
        show_healthy[0] = 1; show_wounded[0] = 1; show_healthy[1] = 0; show_wounded[1] = 0;
        other_party = GD_I(0x7c);
        PartyMgr* pm = PARTIES;
        Party* p = pm->get(GD_I(0x7c));
        p->nstacks = 0;
    }
    else if (kind == 3) {
        show_healthy[0] = 1; show_wounded[0] = 1; show_healthy[1] = 1; show_wounded[1] = 0;
        int pid = GD_I(0x13a30);
        other_party = pid;
        PartyMgr* pm = PARTIES;
        if (!pm->is_live(pid))
            mb_error("mercenary_source_party not set");
    }
    else if (kind == 4) {
        show_healthy[0] = 1; show_wounded[0] = 1; show_healthy[1] = 1; show_wounded[1] = 1;
        int x = GD_I(0x13b00);
        other_party = (x > 0) ? x : GD_I(0x7c);
    }

    setup_party(0);
    if (mode == 0 || mode == 5) {
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


// @ 0x503810
void PartyScreen::read(int which)
{
    int mainp = g_game->f78;
    int owner = g_game->player;
    if (which != 0) {
        mainp = other_party;
        if (mode == 3 || mode == 2)
            owner = other_owner;
        else
            owner = *(int*)((char*)PARTIES->get(mainp) + 0xac);
    }
    if (owner >= 0)
        owner_troop[which] = g_game->chars[owner].f3b4;
    party_7c[which] = *(int*)((char*)party_index(mainp) + 0x7c);
    party_80[which] = *(int*)((char*)party_index(mainp) + 0x80);
    nstacks[which] = *(int*)((char*)party_index(mainp) + 0x8a8);
    if (nstacks[which] > 0) {
        int i = 0;
        PsRow* dst = rows[which];
        do {
            *(int*)((char*)party_index(mainp) + 0xb4 + i * 0x20) = 0;
            *dst = *(PsRow*)((char*)party_index(mainp) + 0xa8 + i * 0x20);
            i++;
            dst++;
        } while (i < nstacks[which]);
    }
    if (which == 0) {
        for (int i = 0; i < 64; i++)
            for (int j = 0; j < 32; j++)
                table[i][j] = ((int*)((char*)g_game + 0x1405c))[i * 32 + j];
    }
}

// @ 0x503a50
void PartyScreen::refresh_morale()
{
    float morale = 0.0f;
    morale = *(float*)((char*)party_index(g_game->f78) + 0x7c);

    char text[0x1f0];

    sprintf(text,
            g_strtable.lookup(tstring("ui_morale"))->c_str(),
            morale < 0.1f ? g_strtable.lookup(tstring("ui_terrible"))->c_str()
          : morale < 0.2f ? g_strtable.lookup(tstring("ui_very_low"))->c_str()
          : morale < 0.3f ? g_strtable.lookup(tstring("ui_low"))->c_str()
          : morale < 0.4f ? g_strtable.lookup(tstring("ui_below_average"))->c_str()
          : morale < 0.6f ? g_strtable.lookup(tstring("ui_average"))->c_str()
          : morale < 0.7f ? g_strtable.lookup(tstring("ui_above_average"))->c_str()
          : morale < 0.8f ? g_strtable.lookup(tstring("ui_high"))->c_str()
          : morale < 0.9f ? g_strtable.lookup(tstring("ui_very_high"))->c_str()
          : g_strtable.lookup(tstring("ui_excellent"))->c_str());

    {
        tstring line(text);
        float extents[4];
        PSO(morale_lbl)->set_text(extents, &line);
    }

    float value = *(float*)((char*)party_index(g_game->f78) + 0x80);
    if (!(value >= 0.1f))
        return;
    value *= 100.0f;

    {
        tstring fmt("ui_starving");
        float whole = floor(value);
        int n = (int)whole;
        if (!(value - whole <= 0.5f))
            n++;
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

    float pos[4];
    morale_lbl->get_xy(pos);
}


static Party* get_party(int id)
{
    PartyMgr* m = &g_game->parties;
    int i = ((Map10007*)m)->lookup(id);
    return &m->pages._Myfirst[i >> 4][i & 0xf];
}

// the troop's localised display name: its name record when the string table knows it, else its id
static const char* display_name(TroopRec* rec)
{
    tstring* nm = g_strtable.find(&rec->key);
    if (!nm)
        nm = (tstring*)rec;
    return nm->c_str();
}

// push a label into a widget (slot 0xa4 writes the text extents into a scratch vector4)
static void push_text(MissionUi* w, const tstring* s)
{
    float out[4];
    PSO(w)->set_text(out, s);
}

static void push_empty(MissionUi* w)
{
    tstring s("");
    push_text(w, &s);
}

static void append_plus(tstring& s)
{
    tstring plus(" +");
    s.append(plus);
}

// read a "%x" colour out of the settings table entry named by key and hand it to the widget
static __forceinline void push_colour(MissionUi* w, const char* key)
{
    tstring k(key);
    int colour = 0;
    int found = -1;
    for (int i = 0; found == -1 && i < g_game_vars.count; i++) {
        if (g_game_vars.entries[i].key.equals(k))
            found = i;
    }
    if (found >= 0)
        sscanf(g_game_vars.entries[found].value.c_str(), "%x", &colour);
    PSO(w)->set_colour(&colour);
}

// @ 0x503ed0
void PartyScreen::refresh_party_members(int type)
{
    if ((mode == 0 || mode == 5) && type == 1)
        return;

    int party_id = g_game->f78;
    if (type != 0)
        party_id = other_party;

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
    int first_wounded = 0;
    {
        Party* p = get_party(party_id);
        for (int i = 0; i < p->nstacks; i++) {
            if (p->stacks[i].flags & 1)
                break;
            first_wounded++;
        }
    }

    int healthy = 0;
    int wounded = 0;
    char buf[512];

    if (party_id != -1) {
        // label every row that exists with an empty string
        for (int i = 0; i < get_party(party_id)->max_stacks(); i++) {
            push_empty(buttons[type * 2 + 0][i]);
            PSO(buttons[type * 2 + 0][i])->enable();
        }
        for (int i = 0; i < get_party(party_id)->max_stacks(); i++) {
            push_empty(buttons[type * 2 + 1][i]);
            PSO(buttons[type * 2 + 1][i])->enable();
        }

        int negfirst = -first_wounded;

        for (int i = 0; i < get_party(party_id)->nstacks; i++, negfirst++) {
            Stack* st = &get_party(party_id)->stacks[i];
            bool is_wounded = (st->flags & 1) != 0;

            bool include = (show_healthy[type] && !is_wounded) || (show_wounded[type] && is_wounded);
            if (!include)
                continue;

            if (is_wounded)
                wounded++;
            else
                healthy++;

            tstring label;
            TroopRec* rec = (TroopRec*)&g_game->troops[st->troop];

            if (rec->flags32 & 0x10) {
                // hero: percentage of the (clamped) level denominator
                if (g_game->agent_low(st->troop)) {
                    {
                        tstring key("ui_button_party_member_hero_percentage_wounded");
                        int v = ((unsigned char*)rec)[0x7a] & 0xf;
                        if (v < 0)
                            v = 0;
                        else if (v > g_clamp_max)
                            v = g_clamp_max;
                        // the name, then the format, then the percentage (0x50441d..0x504494)
                        const char* nm = display_name(rec);
                        const char* fmt = g_strtable.lookup(key)->c_str();
                        int pct = (int)(rec->hp * 100.0f / (float)(rec->f58[0] + v * 2 + 0x23) + 0.0001f);
                        sprintf(buf, fmt, nm, pct);
                    }
                    label.assign(buf);
                    if (rec->fa8 != 0)
                        append_plus(label);
                } else {
                    {
                        tstring key("ui_button_party_member_hero_percentage");
                        int v = ((unsigned char*)rec)[0x7a] & 0xf;
                        if (v < 0)
                            v = 0;
                        else if (v > g_clamp_max)
                            v = g_clamp_max;
                        // the name, then the format, then the percentage (0x50441d..0x504494)
                        const char* nm = display_name(rec);
                        const char* fmt = g_strtable.lookup(key)->c_str();
                        int pct = (int)(rec->hp * 100.0f / (float)(rec->f58[0] + v * 2 + 0x23) + 0.0001f);
                        sprintf(buf, fmt, nm, pct);
                    }
                    label.assign(buf);
                    if (rec->fa8 != 0)
                        append_plus(label);
                }
            } else {
                if (st->wounded > 0 && !is_wounded) {        // a stack with wounded troops shows healthy / total (0x504254)
                    {
                        tstring key("ui_button_party_member_healthy_total");
                        sprintf(buf, g_strtable.lookup(key)->c_str(), display_name(rec),
                                st->count - st->wounded, st->count);
                    }
                    label.assign(buf);
                    if (st->f14 > 0)
                        append_plus(label);
                } else {
                    {
                        tstring key("ui_button_party_member_total");
                        sprintf(buf, g_strtable.lookup(key)->c_str(), display_name(rec),
                                st->count);
                    }
                    label.assign(buf);
                    if (st->f14 > 0)
                        append_plus(label);
                }
            }

            int row = is_wounded ? 1 : 0;
            int col = is_wounded ? negfirst : i;

            push_text(buttons[type * 2 + row][col], &label);
            if ((rec->flags32 & 0x10) && g_game->agent_low(st->troop))
                push_colour(buttons[type * 2 + row][col], "party_b_member_t_c_wounded");
            else
                push_colour(buttons[type * 2 + row][col], "party_b_member_t_c");
            PSO(buttons[type * 2 + row][col])->act_b4();
            PSO(buttons[type * 2 + row][col])->act_bc();
            slots[type][row][col] = i;
        }

        fill_member(type, 0, get_party(party_id), healthy);
        fill_member(type, 1, get_party(party_id), wounded);
    }

    if (type == 0)
        money_label(0, g_game->troops[g_game->player].f3b4);

    if (type == 0) {
        int cost = GV->weekly_cost();
        if (cost > 0) {
            {
                tstring key("ui_weekly_cost_value");
                tstring val = g_game->string_value(cost);
                sprintf(buf, g_strtable.lookup(key)->c_str(), val.c_str());
            }
            tstring text(buf);
            push_text(weekly_cost_lbl, &text);
        } else {
            tstring text("");
            push_text(weekly_cost_lbl, &text);
        }

        {
            tstring key("ui_company");
            tstring* fmt = g_strtable.lookup(key);
            sprintf(buf, fmt->c_str(), get_party(g_game->f78)->sum_head(),
                    get_party(g_game->f78)->max_members());
        }
        {
            tstring text(buf);
            push_text(max_companions, &text);
        }

        {
            tstring key("ui_prisoners_equal_value");
            tstring* fmt = g_strtable.lookup(key);
            sprintf(buf, fmt->c_str(), get_party(g_game->f78)->sum_flagged(),
                    get_party(g_game->f78)->max_prisoners());
        }
        {
            tstring text(buf);
            push_text(max_prisoners, &text);
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
        total += unit * g_grow_frac;
        if (!(total < bar_amount[type]))
            total = bar_amount[type];
    }

    float p[2];                     // the one float pair every set_pos / set_size call below reuses
    MissionUi** row = buttons[type + page * 2];
    for (int i = 0; i < count; i++, row++) {
        MissionUi* bar = *row;
        *(int*)&p[0] = WINT(bar, 0xe8);
        p[1] = total - (float)(i + 1) * unit;
        PSO(bar)->set_pos(p);
    }

    MissionUi* a = ((MissionUi**)members_page)[type + page * 2];
    *(int*)&p[0] = WINT(a, 0xf0);
    p[1] = total;
    PSO(a)->set_size(p);

    float x = (bar_amount[type] < total) ? bar_amount[type] : total;
    MissionUi** pb = &((MissionUi**)member_list)[type + page * 2];
    *(int*)&p[0] = WINT(*pb, 0xf0);
    p[1] = x;
    PSO(*pb)->set_size(p);

    *(int*)&p[0] = WINT(*pb, 0xe8);
    p[1] = bar_start[type] + bar_amount[type] - x;
    PSO(*pb)->set_pos(p);

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
    g_store0 = 0;
    g_store1 = 0;
    int total = 0;
    if (sl_query(8, 0, 0))
        return g_store0;

    int id = ((Map10007*)&parties)->lookup(f78);
    int* blk = (int*)parties.pages._Myfirst[id >> 4];
    char* rec = (char*)blk + (id & 0xf) * 0x5ac8;
    int cnt = *(int*)(rec + 0x8a8);

    int* row = (int*)(rec + 0xcc);
    for (int i = 1; i < cnt; i++) {
        if (row[1] & 1)
            break;
        int member = row[0];
        int v = sl_party_value(member);
        if ((troops[member].flags32 & 0x10) == 0)
            v *= row[-1];
        total += v;
        row += 8;
    }
    return total;
}


// @ 0x504ef0
void PartyScreen::setup_party(int side)
{
    float caption_pos[2];           // passed uninitialized by the original (a game bug)

    members_base[side] = new_ps_bar();
    if (side == 0) {
        label_refresh(members_base[0], tstring("party_bo_party1"));
        if (g_game->skill_level(g_game->player, 2) == 0) {
            prisoner_mgmt = new MissionUi;
            label_refresh(prisoner_mgmt, tstring("party_t_prisoner_mgmt"));
            PSO(prisoner_mgmt)->set_text(
                caption_pos, g_strtable.lookup(tstring("ui_need_to_learn_prisoner_management")));
            PSO(members_base[0])->add_child(prisoner_mgmt);
        }
    } else {
        label_refresh(members_base[side], tstring("party_bo_party2"));
    }

    for (int page = 0; page < 2; page++) {
        members_page[side][page] = new_ps_list();
        label_refresh(members_page[side][page], tstring("party_bo_members_base"));
        // the scrollbar goes on the base, the list on this page (0x5050cc / 0x505131); each name dies after its call
        if (page == 0) {
            MissionUi* sc;
            {
                tstring s1("party_s_members1");
                sc = make_scrollbar(members_base[side], &s1);
            }
            tstring s2("party_sc_members1");
            member_list[side][page] = make_member_list(members_page[side][page], &s2, sc);
        } else {
            MissionUi* sc;
            {
                tstring s1("party_s_members2");
                sc = make_scrollbar(members_base[side], &s1);
            }
            tstring s2("party_sc_members2");
            member_list[side][page] = make_member_list(members_page[side][page], &s2, sc);
        }
        PSO(members_base[side])->add_child(member_list[side][page]);

        for (int i = 0; i < 0x40; i++) {
            tstring k_button_h("party_b_members_button_h");
            tstring k_width_ratio("party_b_members_width_ratio");
            tstring k_h("party_b_members_h");
            tstring k_x("party_b_members_x");
            tstring k_empty("");                    // the button's name (arg 1 at 0x5053ac is this "", not k_x)
            float ev    = ini_float_scan(k_button_h);
            float ratio = ini_float_scan(k_width_ratio);
            float h     = ini_float_scan(k_h);
            float x     = ini_float_scan(k_x);
            buttons[side * 2 + page][i] =
                new_member_button(members_page[side][page], k_empty, x, 0, h,
                                  tstring("party_member_button"),
                                  tstring("party_member_button_pressed"),
                                  ratio, ev, 0.085f);
            ((PsListWidget*)buttons[side * 2 + page][i])->f184 |= 3;
        }
    }

    // the member base is a child of the party body
    ((PsWindow*)window)->panels.push_back(members_base[side]);

    if (side == 0) {
        gold = new MissionUi;
        label_refresh(gold, tstring("party_t_gold"));
        float d[2];
        d[1] = gold->fE4 - members_base[0]->fE4;
        d[0] = gold->fE0 - members_base[0]->fE0;
        PSO(gold)->set_pos(d);
        PSO(members_base[0])->add_child(gold);

        weekly_cost_lbl = new MissionUi;
        label_refresh(weekly_cost_lbl, tstring("party_t_weekly_cost"));
        d[1] = weekly_cost_lbl->fE4 - members_base[0]->fE4;
        d[0] = weekly_cost_lbl->fE0 - members_base[0]->fE0;
        PSO(weekly_cost_lbl)->set_pos(d);
        PSO(members_base[0])->add_child(weekly_cost_lbl);

        max_companions = new MissionUi;
        label_refresh(max_companions, tstring("party_t_max_companions"));
        d[1] = max_companions->fE4 - members_base[0]->fE4;
        d[0] = max_companions->fE0 - members_base[0]->fE0;
        PSO(max_companions)->set_pos(d);
        PSO(members_base[0])->add_child(max_companions);

        max_prisoners = new MissionUi;
        label_refresh(max_prisoners, tstring("party_t_max_prisoners"));
        d[1] = max_prisoners->fE4 - members_base[0]->fE4;
        d[0] = max_prisoners->fE0 - members_base[0]->fE0;
        PSO(max_prisoners)->set_pos(d);
        PSO(members_base[0])->add_child(max_prisoners);

        morale_lbl = new MissionUi;
        label_refresh(morale_lbl, tstring("party_t_morale"));
        d[1] = morale_lbl->fE4 - members_base[0]->fE4;
        d[0] = morale_lbl->fE0 - members_base[0]->fE0;
        PSO(morale_lbl)->set_pos(d);
        PSO(members_base[0])->add_child(morale_lbl);
    } else {
        choose_companions = new MissionUi;
        label_refresh(choose_companions, tstring("party_t_choose_companions"));
        float d[2];
        d[1] = choose_companions->fE4 - members_base[side]->fE4;
        d[0] = choose_companions->fE0 - members_base[side]->fE0;
        PSO(choose_companions)->set_pos(d);
        PSO(members_base[side])->add_child(choose_companions);

        choose_prisoners = new MissionUi;
        label_refresh(choose_prisoners, tstring("party_t_choose_prisoners"));
        d[1] = choose_prisoners->fE4 - members_base[side]->fE4;
        d[0] = choose_prisoners->fE0 - members_base[side]->fE0;
        PSO(choose_prisoners)->set_pos(d);
        PSO(members_base[side])->add_child(choose_prisoners);

        // the two captions depend on what kind of party is shown
        if (mode == 1 || mode == 6 || mode == 7) {
            int           idx = ((Map10007*)&g_game->parties)->lookup(other_party);
            Party* rec = (Party*)((char*)g_game->parties.pages._Myfirst[idx >> 4]
                                                + (idx & 0xf) * 0x5ac8);
            // Party::display_name inlined (0x505949): the ids, else the string-table entry, else the name (+0xc)
            tstring* name;
            if (g_show_party_ids_instead_of_names)
                name = &rec->key;
            else {
                name = 0;
                if (rec->flag == 0)
                    name = g_strtable.find(&rec->key);
                if (name == 0)
                    name = &rec->name;
            }
            PSO(choose_companions)->set_text(caption_pos, name);
            tstring empty("");
            PSO(choose_prisoners)->set_text(caption_pos, &empty);
        } else if (mode == 2 || mode == 3) {
            // each caption string dies before the next is built (0x505886)
            {
                tstring empty("");
                PSO(choose_companions)->set_text(caption_pos, &empty);
            }
            {
                tstring empty2("");
                PSO(choose_prisoners)->set_text(caption_pos, &empty2);
            }
        } else {
            {
                tstring s1("ui_rescued_prisoners");
                PSO(choose_companions)->set_text(caption_pos, g_strtable.lookup(s1));
            }
            {
                tstring s2("ui_captured_enemies");
                PSO(choose_prisoners)->set_text(caption_pos, g_strtable.lookup(s2));
            }
        }
    }
}


// @ 0x5059c0
void PartyScreen::setup_skills()
{
    // ---- the skills panel ----
    MissionUi* w = new_ps_bar();                   // 0x188, ctor 0x4931a0, vptr 0x597050
    skills_base = w;
    label_refresh(skills_base, tstring("party_bo_skills"));
    {
        tstring empty(g_empty_string);
        float pos[2];                                   // passed uninitialised by the original
        PSO(skills_base)->set_text(pos, &empty);
    }
    MissionUi* bar = new MissionUi;                       // 0x178, ctor 0x48ae20
    float d[2];
    d[1] = bar->fE4 - skills_base->fE4;
    d[0] = bar->fE0 - skills_base->fE0;
    PSO(bar)->set_pos(d);
    {
        tstring key("ui_party_skills");
        float pos[2];
        PSO(bar)->set_text(pos, g_strtable.lookup(key));
    }
    PSO(skills_base)->add_child(bar);

    // ---- the party this page shows and its members ----
    GameData* g = g_game;
    int idx = ((Map10007*)&g->parties)->lookup(g->f78);
    Party* members = (Party*)((char*)g->parties.pages._Myfirst[idx >> 4] + (idx & 0xf) * 0x5ac8);
    int count = members->nstacks;

    // ---- the page's layout steps ----
    float t_y = ini_float(tstring("party_bo_skills_t_y"));
    float t_y_a = ini_float(tstring("party_bo_skills_t_y_a"));
    float t_y_a_short = ini_float(tstring("party_bo_skills_t_y_a_short"));
    float t_x = ini_float(tstring("party_bo_skills_t_x"));
    float t_x_deep = ini_float(tstring("party_bo_skills_t_x_deep"));

    // ---- pass 1: the content height ----
    float y = 0.0f;
    if (count > 0) {
        Stack* ms = members->stacks;
        for (int m = 0; m < count; m++, ms++) {
            if (!shown(ms))
                continue;
            y += t_y_a;
            for (int k = 0; &g_skill_rows[k] < (SkillRow*)&g_module_name; k++) {
                if (((g_skill_rows[k].flags >> 4) & 1) == 0)
                    continue;
                int best = 0;
                int best_i = -1;
                if (count > 0) {
                    Stack* ms2 = members->stacks;
                    for (int i = 0; i < count; i++, ms2++) {
                        if (!shown(ms2))
                            continue;
                        int v = skill_of(ms2, k);
                        if (best < v) {
                            best = v;
                            best_i = i;
                        }
                    }
                }
                if (best_i == m)
                    y += t_y_a_short;
            }
        }
    }
    y += t_y_a;
    {
        float pos[2];
        pos[0] = skills_base->screen[0];
        pos[1] = y + t_y;
        PSO(skills_base)->set_size(pos);
    }

    // ---- pass 2: a caption line per member and a line per skill ----
    if (count > 0) {
        Stack* ms = members->stacks;
        char buf[0x224];
        for (int mi = 0; mi < count; mi++, ms++) {
            if (!shown(ms))
                continue;
            Troop* r = rec_of(ms->troop);
            {
                tstring* nm = g_strtable.find(&r->key);
                if (nm == 0)
                    nm = &r->key;
                tstring cap(*nm);
                cap.capitalize();
                sprintf(buf, "%s:", cap.c_str());
            }
            {
                tstring cap(buf);
                float pos[2];
                pos[0] = t_x;
                pos[1] = y;
                PSO(skills_base)->set_label(&cap, pos);
            }
            y -= t_y_a;

            for (int k = 0; &g_skill_rows[k] < (SkillRow*)&g_module_name; k++) {
                SkillRow* se = &g_skill_rows[k];
                if (((se->flags >> 4) & 1) == 0)
                    continue;
                int best = 0;
                int best_i = -1;
                if (count > 0) {
                    Stack* ms2 = members->stacks;
                    for (int i = 0; i < count; i++, ms2++) {
                        if (!shown(ms2))
                            continue;
                        int v = skill_of(ms2, k);
                        if (best < v) {
                            best = v;
                            best_i = i;
                        }
                    }
                }
                if (best_i == mi) {
                    int pq = members->skill_level(k);
                    {
                        tstring* nm = g_strtable.find(&se->key);
                        if (nm == 0)
                            nm = &se->key;
                        if (pq == best)
                            sprintf(buf, "%s (%d)", nm->c_str(), best);
                        else
                            sprintf(buf, "%s (%d)(+%d)", nm->c_str(), best, pq - best);
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
    }

    // ---- the scroll list ----
    MissionUi* sb;
    {
        tstring name("party_s_skills");
        sb = make_scrollbar(window, &name);
    }
    {
        tstring name("party_sc_skills");
        PsScrollList* list = new_scroll_list();            // 0x4a8, ctor 0x49deb0
        list->f104 |= 0x20;
        if (sb != 0)
            list->scrollbar = sb;
        list->list = skills_base;
        PSO(list)->add_child(skills_base);
        label_refresh(list, name);
        skills_list = list;
    }

    // ---- place the list in the panel ----
    PsScrollList* list = skills_list;
    if (skills_base->screen[1] > list->screen[1])
        d[1] = 0.0f;
    else
        d[1] = list->screen[1] - skills_base->screen[1];
    d[0] = list->fE0;
    d[1] = d[1] + list->fE4;
    PSO(list)->set_pos(d);
    {
        MissionUi* lower = skills_base->screen[1] > list->screen[1] ? skills_base : list;
        float pos[2];
        pos[0] = list->screen[0];
        pos[1] = lower->screen[1];
        PSO(list)->set_size(pos);
    }
    quarter_update(list);
    PSO(window)->add_child(skills_list);

    // ---- the party-skills information bar ----
    party_skills_bar = new MissionUi;
    label_refresh(party_skills_bar, tstring("party_t_party_skills"));
    {
        tstring key("ui_party_skills");
        float pos[2];
        PSO(party_skills_bar)->set_text(pos, g_strtable.lookup(key));
    }
    PSO(window)->add_child(party_skills_bar);
}


// @ 0x5063e0
void PartyScreen::refresh_selection()
{
    float size[2];                                  // the text size every +0xa4 call writes (esp+0x28)
    {
        tstring empty("");
        PSO(info)->set_text(size, &empty);
    }                                               // destroyed at once (0x506414)

    PSO(talk_btn)->enable();
    locked = 1;
    PSO(action_btn)->enable();
    PSO(action_btn)->clear();
    PSO(upgrade1)->enable();
    PSO(upgrade2)->enable();
    PSO(upgrade1)->clear();
    PSO(upgrade2)->clear();
    PSO(move_up)->enable();
    PSO(move_down)->enable();
    PSO(cost_panel)->clear();
    PSO(upgrade_panel)->clear();
    if (char_quad) {
        {
            tstring name("");
            PSO(name_lbl)->set_text(size, &name);
        }                                           // destroyed before add_image (0x5064ca)
        PSO(char_quad)->add_image(0, 0);
    }
    tstring label;                                  // the action button's caption (constructed at 0x5064e5)

    if (sel_row == -1 || sel_type == -1 || sel_slot == -1)
        return;                                     // nothing selected (0x5064f3..0x506515 -> 0x5072f6)
    int p1 = sel_type != 0 ? g_game->f78 : other_party;     // the party a moved stack goes to
    int slot_idx = slots[sel_type][sel_slot][sel_row];      // (sel_slot*2 + sel_type) << 6
    int p2 = sel_type != 0 ? other_party : g_game->f78;     // the party the selected stack is in
    Stack stk = PARTIES->get(p2)->stacks[slot_idx];
    if (stk.troop < 0)
        return;                                     // an empty stack slot (0x506586 -> 0x5072e2)
    TroopRec* rec = g_game->chars + stk.troop;
    show_selected_character(stk.troop, sel_slot);   // (0x5065a0 pushes sel_slot)
    bool action = false;
    if (mode == 0) {
        if (sel_row != -1 && !(sel_row == 0 && sel_slot == 0))
            PSO(talk_btn)->act_bc();
    }

    int cmd = mode;
    if (cmd == 0 || cmd == 5) {
        tstring k(sel_type == 0 ? "ui_disband" : (stk.flags & 1) != 0 ? "ui_take_prisoner" : "ui_take_back");
        label = *g_strtable.lookup(k);
        if (sel_row != -1 && !(sel_type == 0 && sel_row == 0 && sel_slot == 0))
            action = true;
    } else if (cmd == 1 || cmd == 6 || cmd == 7) {
        tstring k(sel_type == 0 ? "ui_give" : "ui_take");
        label = *g_strtable.lookup(k);
        if (sel_row != 0 || sel_slot != 0 || mode == 6) {
            if ((sel_type == 0 && sel_slot == 0) || mode != 7)      // 0x5068b8..0x5068d4
                action = true;
        }
    } else if (cmd == 2) {
        {
            tstring k("ui_sell");
            label = *g_strtable.lookup(k);
        }
        if (sel_type == 0 && (stk.flags & 1) != 0) {
            g_store0 = 1;
            g_store1 = 0;
            // a script that answers and leaves both out-slots clear refuses the sale (0x506684..0x50669e)
            if (!(g_game->emit_event(0xb, stk.troop, 0) != 0 && g_store0 == 0 && g_store1 == 0))
                action = true;
        }
    } else if (cmd == 3) {
        {
            tstring k("ui_hire");
            label = *g_strtable.lookup(k);
        }
        if (sel_type != 0) {
            int diff = GV->sl_party_price(stk.troop) - f8cc;
            if (g_game->chars[g_game->player].f3b4 >= diff) {
                action = true;
            } else {
                tstring ky("party_info_warn_y");
                float y = g_game_vars.get_float(ky);
                tstring kx("party_info_warn_x");
                float x = g_game_vars.get_float(kx);
                tstring msg("ui_notification_cant_hire");
                float pt[2];
                pt[0] = x;
                pt[1] = y;
                PSO(info)->set_label(g_strtable.lookup(msg), pt);
            }                                       // msg, kx, ky destroyed here (0x5067b6..0x5067cb)
        }
    } else if (cmd == 4) {
        action = true;
        tstring k(sel_type == 0 ? "ui_uncapture" : (stk.flags & 1) != 0 ? "ui_capture" : "ui_hire");
        label = *g_strtable.lookup(k);
    }

    if (p2 == g_game->f78) {
        int n = PARTIES->get(g_game->f78)->first_flagged();
        if (slot_idx > 1 && slot_idx < n)
            PSO(move_up)->act_bc();
        if (slot_idx > 0 && slot_idx < n - 1)
            PSO(move_down)->act_bc();
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

    if (((rec->flags32 & 0x10) == 0 || sel_type != 0 || mode == 6) && (rec->flags32 & 0x10000000) == 0 && action) {
        PSO(action_btn)->act_bc();
        locked = 0;
    }

    tstring caption(label);                         // a copy (0x506b08), alive to the end
    PSO(action_btn)->set_text(size, &caption);

    char cost_buf[512];                             // esp+0x2b0
    sprintf(cost_buf, "");
    if (stk.troop == g_game->player || (stk.flags & 1) != 0) {
        if (mode == 2 && action) {
            int v;
            {
                tstring k("ui_price");
                g_store0 = 0;
                g_store1 = 0;
                if (g_game->emit_event(10, stk.troop, g_game->f78) != 0)
                    v = g_store0;
                else
                    v = GD_I(0x13a20);
                sprintf(cost_buf, g_strtable.lookup(k)->c_str(), v);
            }
            put_text(cost_panel, cost_buf);
        }
    } else {
        if (sel_type == 1 && mode == 3) {
            tstring k("ui_joining_cost_weekly_wage");
            const char* fmt = g_strtable.lookup(k)->c_str();
            sprintf(cost_buf, fmt, GV->sl_party_price(stk.troop), GV->sl_party_value(stk.troop));
        } else {
            tstring k("ui_weekly_wage");
            const char* fmt = g_strtable.lookup(k)->c_str();
            sprintf(cost_buf, fmt, GV->sl_party_value(stk.troop));
        }
        put_text(cost_panel, cost_buf);
    }

    // the two upgrade buttons: shown (a) when the troop has that upgrade, available (c) when the party has room
    bool shown1 = false, shown2 = false, room1 = false, room2 = false;
    char upgrade_buf[512];                          // esp+0x4b0
    char name_buf[512];                             // esp+0xb0
    sprintf(upgrade_buf, "");
    int ready = stk.f14;                            // the stack's troops ready to upgrade
    if (sel_type == 0 && (rec->flags32 & 0x10) == 0 && (stk.flags & 1) == 0) {
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
            {
                tstring t(name_buf);
                PSO(upgrade1)->set_text(size, &t);
            }
            {
                tstring k("party_b_member_t_c");
                int colour = g_game_vars.hex_field(k);
                PSO(upgrade1)->set_colour(&colour);
            }
        }
        unsigned up2 = *(unsigned*)((char*)rec + 0x54);
        if (up2 != 0) {
            shown2 = true;
            {
                tstring k("ui_upgrade_to_value");
                sprintf(name_buf, g_strtable.lookup(k)->c_str(), g_game->chars[up2].display_name()->c_str());
            }
            {
                tstring t(name_buf);
                PSO(upgrade2)->set_text(size, &t);
            }
            {
                tstring k("party_b_member_t_c");
                int colour = g_game_vars.hex_field(k);
                PSO(upgrade2)->set_colour(&colour);
            }
        }
        if (shown1) {
            PSO(upgrade1)->act_b4();
            PSO(upgrade1)->enable();
            if (ready != 0 && PARTIES->get(p2)->find_or_add(rec->f50, 0, 0) != -1)
                room1 = true;
        }
        if (shown2) {
            PSO(upgrade2)->act_b4();
            PSO(upgrade2)->enable();
            if (ready != 0 && PARTIES->get(p2)->find_or_add(*(unsigned*)((char*)rec + 0x54), 0, 0) != -1)
                room2 = true;
        }
    }
    bool nb = (stk.flags & 1) != 0;
    if (room1) {                                    // 0x507031..0x507064
        PSO(upgrade1)->act_bc();
    } else if (!nb && ready >= stk.count) {
        PSO(upgrade1)->act_bc();
        room1 = true;
    }
    if (room2) {
        PSO(upgrade2)->act_bc();
    } else if (!nb && ready >= stk.count) {
        PSO(upgrade2)->act_bc();
        room2 = true;
    }
    if (ready != 0) {
        if (!room2) {
            {
                tstring k("ui_notification_no_slot_for_upgrade");
                sprintf(name_buf, g_strtable.lookup(k)->c_str(),
                        g_game->chars[*(unsigned*)((char*)rec + 0x54)].display_name()->c_str());
            }
            {
                tstring t(name_buf);
                PSO(upgrade2)->set_text(size, &t);
            }
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
            {
                tstring t(name_buf);
                PSO(upgrade1)->set_text(size, &t);
            }
            {
                tstring k("party_b_upgrade_t_c_passive");
                int colour = g_game_vars.hex_field(k);
                PSO(upgrade1)->set_colour(&colour);
            }
        }
        PSO(upgrade_panel)->act_b4();
    }
    {
        tstring u(upgrade_buf);
        PSO(upgrade_panel)->set_text(size, &u);
    }
    if (mode == 3) {
        if (sel_type == 0) {
            PSO(action_btn)->enable();
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
    PSO(char_quad)->add_image(0, ((TableauTbl*)&g_tableaus)->make_mesh_from(&item, &troop));

    TroopRec* rec = (TroopRec*)((char*)g_game->chars + t * 0x508);
    float size[2];
    PSO(name_lbl)->set_text(size, troop_name(rec));
}

