// mission_window.cpp: the mission and meeting window screens (0x4dae40..0x4db810). The file boundary is the original's: 0x4db1f0 calls
// UiA::UiA 0x48b1c0 out of line, while new_button 0x4db810 and the rest of cls_map.cpp (0x4dcc50, 0x4dd830) expand it, so this code saw
// only the constructor's declaration. Names provisional.
#define UIA_CTOR_OUT_OF_LINE   // Rec118::init_screen calls the constructor 0x48b1c0
#include "../../include/image_addrs.h"
#include "../env_row.h"                // EnvRow, g_env_rows (0x5e28f4)
#include "../music_player.h"   // music_player g_music (0xa73a10): listed directly (the resolution audit reads only the source's own includes)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "./cls_map.h"
#include "../../include/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40)
#include "../../include/prsnt_table.h"   // g_presentations (0xa75f2c): used[0..2] are the menu kinds
#include "./progress_bar.h"
// the headers whose `// 0x...` annotations the resolution audit has to see are included directly (it scans only the direct includes)
#include "../config/config.h"
#include "../config/widget_mgr.h"
#include "../config/ui_widgets_c.h"
#include "../strat/gui_manager.h"
#include "../font.h"
#include "../../include/e68.h"
#include "../../include/vec_elems.h"
#include "../arr_set.h"
#include "../../include/ptr_tag.h"
#include "../../include/game_data.h"
#include "../../rgl/mesh2/mesh2_common.h"
#include "../../rgl/mesh.h"
#include <process.h>
#include <shlobj.h>
#include "../../tinyxml/tinyxml.h"
#include "../ui/ui_frame.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../shared_vars.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
struct OptionsDlg {
    void choose_dx_settings(Config* cfg, bool overwrite);   // 0x4b9b90 (config/options_dlg.h pulls winsock2.h: member-only stub)
};
#include "../../dxframework/dxutil.h"   // DXUtil_Timer 0x407660 (stdcall, TIMER_COMMAND)
#include "../../rgl/options.h"       // options::save 0x4181c0
#include <hash_map>
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/game_recs.h"
#include "../map_icon.h"      // g_map_icons (0x5e29f0)
#include "../../include/mission_obj.h"      // MissionObj: the scene prop kind (g_propkinds)
#include "../ptype.h"
#include "../../include/render_flags.h"
#include "../module_name.h"      // g_module_name (0x8ee3c38)
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128
#include "../mode_rec.h"   // g_mission_templates 0x5e2a00
#include "../../include/skinmagic.h"
#include "../window_mgr.h"
#include "../mission.h"   // canonical: g_mission (0x5e292c)
extern float g_5a9f20;                                      // 0x5a9f20 the meeting window columns' left edge (progress_bar.cpp)
extern float g_5a9f24;                                      // 0x5a9f24 the meeting window columns' top (cls_map.cpp)
extern float g_5a9f28[2];                                   // 0x5a9f28 {x, y} column size (cls_map.cpp)

// @ 0x4db020
void MissionScreen::fill_single_mission()
{
    float pos[2];
    {
        tstring none("");
        overlay->set_text(pos, &none);
    }
    row[2]->set_text(pos, &g_mission_templates[g_mission->mode].display);
}

// @ 0x4db090
void MissionScreen::fill_mission_grid()
{
    float y = 0.7f;
    {
        tstring none("");
        float pos[2];
        overlay->set_text(pos, &none);
    }
    for (int k = 0; k < g_mission_ctx.rows; k++) {
        for (int j = 0; j < g_mission_ctx.depth; j++) {
            for (int i = 0; i < g_mission_ctx.cols; i++) {
                float v[2] = { 0.1f, y };
                Troop* u = &g_game->troops[g_mission_ctx.grid[k * 14 + j * 3 + i]];
                const tstring* txt = g_strtable.find(&u->key);
                if (!txt)
                    txt = (const tstring*)u;
                overlay->layout_text_to(txt, v);
                y -= 0.02f;
            }
            y -= 0.005f;
        }
        y -= 0.01f;
    }
}

// the base construction shared by 0x4dae40 and 0x4db1f0 (both build the same sequence with a different overlay mesh name)
#define NEW_OVERLAY_BASE(name_string)                                                   \
    g_gui.reset();                                                                      \
    g_sound_mgr.stop_all();                                                \
    overlay = new NodeA(0);                                            \
    {                                                                                   \
        float v[2] = { 0.0f, 0.0f };                                                    \
        overlay->move_a(v);                                                            \
    }                                                                                   \
    {                                                                                   \
        float v[2] = { 1.0f, 0.75f };                                                   \
        overlay->set_xy(v);                                                            \
    }                                                                                   \
    {                                                                                   \
        tstring name(name_string);                                                      \
        overlay->set_mesh(0, res_get(name)->clone());                                    \
    }                                                                                   \
    {                                                                                   \
        unsigned c[1] = { ~0u };                                                         \
        overlay->set_color(c);                                                            \
    }

#define NEW_OVERLAY(name_string)                                                        \
    NEW_OVERLAY_BASE(name_string)                                                       \
    {                                                                                   \
        float v[2] = { 0.05f, 0.05f };                                                  \
        overlay->set_size(v);                                                            \
    }                                                                                   \
    row[2] = new MissionUi;                                                             \
    {                                                                                   \
        float v[2] = { 0.1f, 0.2f };                                                    \
        row[2]->move_a(v);                                                             \
    }                                                                                   \
    {                                                                                   \
        float v[2] = { 0.6f, 0.3f };                                                    \
        row[2]->set_xy(v);                                                             \
    }                                                                                   \
    {                                                                                   \
        unsigned c[1] = { ~0u };                                                         \
        row[2]->set_color(c);                                                             \
    }                                                                                   \
    {                                                                                   \
        float v[2] = { 0.03f, 0.03f };                                                  \
        row[2]->set_size(v);                                                             \
    }                                                                                   \
    overlay->add_child(row[2]);

// @ 0x4dae40
void Rec44::init_screen()
{
    row[4] = 0;
    NEW_OVERLAY("mission_window")
    if (g_mission_ctx.mode == 2)
        ((MissionScreen*)this)->fill_mission_grid();
    else if (g_mission_ctx.mode == 3 || g_mission_ctx.mode == 4 || g_mission_ctx.mode == 5)
        ((MissionScreen*)this)->fill_single_mission();
    { NodeA* root_ = (NodeA*)overlay; g_gui.dirty = 1; g_gui.objs.push_back(root_); }
}



// @ 0x4db1f0
void Rec118::init_screen()
{
    NEW_OVERLAY_BASE("meeting_window")
    for (int i = 0; i < 16; i++) {
        row[i] = (MissionUi*)new UiA;
        {
            float v[2] = { 0.0f, 0.41f - i * 0.05f };
            row[i]->move_a(v);
        }
        {
            float v[2] = { g_5a9f20, 0.05f };      // the global (0x4db326), not the literal
            row[i]->set_xy(v);
        }
        {
            int c[1] = { (int)0xffaaff55 };
            row[i]->set_target(c);
        }
        overlay->add_child(row[i]);
    }
    for (int j = 0; j < 3; j++) {
        col[j] = new text_box;
        {
            float v[2] = { g_5a9f20, g_5a9f24 - (j + 1) * g_5a9f28[1] };     // globals in the original (0x4db3aa / 0x4db3bf / 0x4db3c6)
            col[j]->move_a(v);
        }
        col[j]->set_xy(g_5a9f28);
        overlay->add_child(col[j]);
    }
    { NodeA* root_ = (NodeA*)overlay; g_gui.dirty = 1; g_gui.objs.push_back(root_); }
    ((MissionScreen*)this)->fill_mission_row(0);
    ((MissionScreen*)this)->fill_mission_row(1);
    ((MissionScreen*)this)->fill_mission_row(2);
}

// @ 0x4db440
void MissionScreen::fill_mission_row(int side)
{
    int id;
    GameData* gd = g_game;
    if (side == 0)
        id = gd->main_party;
    else if (g_mission_ctx.side >= 0) {
        if (side == 1)
            id = g_mission_ctx.ids[g_mission_ctx.side];
        else {
            int k = 1 - g_mission_ctx.side;
            id = g_mission_ctx.ids[k];
        }
    } else {
        id = g_mission_ctx.ids[side - 1];          // side 1 / 2: the two sides' party ids
    }
    if (id == -1)
        return;
    Party* party = gd->parties.get_w(id);
    float v[2];
    const tstring* text;
    if (g_module_settings.show_party_ids_instead_of_names)
        text = &party->key;
    else if (party->flag == 0 && (text = g_strtable.find(&party->key)) != 0)
        text = text;
    else
        text = &party->name;
    col[side]->set_text(v, text);
    float y = 0.15f;
    for (int i = 0; i < party->first_flagged(); i++) {
        Stack* rec = (Stack*)((char*)party + 0xa8) + i;
        Troop* u = &g_game->troops[rec->troop];
        // buf (0x14) is followed directly by line in the original's frame (esp+0x34 / esp+0x48): its sprintf of a long
        // count pair overflows into line, and the line sprintf then reads buf while overwriting that overlap. One struct
        // pins the adjacency whatever the register allocator does with the frame.
        struct { char buf[0x14]; char line[0x30]; char spare[0xc]; } fr;   // spare: the original's frame is 0xc larger above line
        char* buf = fr.buf;
        if (u->flags30 & 0x10) {
            int lvl = u->lvl58;
            int h = u->nib_b[0x12] & 0xf;
            if (h < 0)
                h = 0;
            else if (h > g_skill_tbl[36].max_rank)
                h = g_skill_tbl[36].max_rank;
            sprintf(buf, " %d%%",
                    (int)(u->hp * 100.0f / (lvl + h * 2 + 0x23) + 0.0001f));
        } else {
            sprintf(buf, " (%d/%d)", rec->count - rec->wounded, rec->count);
        }
        const tstring* nm = g_strtable.find(&u->key);
        if (!nm)
            nm = (const tstring*)u;
        const char* nm2 = nm->c_str();
        char* line = fr.line;
        sprintf(line, "%s %s", nm2, buf);
        {
            tstring ts(line);
            v[0] = 0.01f;
            v[1] = y;
            col[side]->layout_text_to(&ts, v);
        }
        y -= 0.02f;
    }
}

// @ 0x4db660
void Rec3c::init_screen()
{
    g_gui.reset();
    overlay = new NodeA(0);
    {
        float mv[2];
        mv[0] = 0.0f;
        mv[1] = 0.0f;
        overlay->move_a(mv);
    }
    {
        float sv[2];
        sv[0] = 1.0f;
        sv[1] = 0.75f;
        overlay->set_xy(sv);
    }
    {
        tstring n("ui_done");
        const tstring& t = *g_strtable_c.lookup(n);     // 0x4db6f0: lookup (a fallback string when absent), not find
        row[0] = new_button(overlay, t, 0.65f, 0.03f, 0.15f, 0.05f);
    }
    {
        tstring none("");
        row[2] = new_button(overlay, none, 0.65f, 0.6f, 0.15f, 0.05f);
    }
    {
        ((UiA*)row[2])->f184 = 1;       // original: `or [eax+0x184],1`; the freshly built button has f184 == 0, so set-to-1 is equivalent
        tstring n("ui_factions");
        float ret[2];
        row[2]->set_text(ret, g_strtable_c.lookup(n));
    }
    {
        void* self = overlay;
        g_gui.dirty = 1;
        g_gui.objs.push_back((NodeA* const&)(self));
    }
    {
        tstring none("");
        float ret[2];
        overlay->set_text(ret, &none);
    }
    if (w04 == 0)
        ((MissionScreen*)this)->draw_faction_report();
}

