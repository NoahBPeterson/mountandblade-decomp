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
#include "../../include/rec78.h"
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
        vt(overlay)->d41(pos, &none);
    }
    vt(row[2])->d41(pos, (void*)((const char*)g_mission_templates + *(int*)((char*)((void*)g_mission) + 0x5df0) * 0x58 + 0x24));
}

// @ 0x4db090
void MissionScreen::fill_mission_grid()
{
    float y = 0.7f;
    {
        tstring none("");
        float pos[2];
        vt(overlay)->d41(pos, &none);
    }
    int kb = 0;
    for (int k = 0; k < g_mission_ctx.rows; k++) {
        int jb = 0;
        for (int j = 0; j < g_mission_ctx.depth; j++) {
            int* cell = (int*)((char*)g_mission_ctx.grid + kb + jb);
            for (int i = 0; i < g_mission_ctx.cols; i++) {
                Troop* u = &g_game->troops[*cell];
                const tstring* txt = g_strtable.find(&u->key);
                if (!txt)
                    txt = (const tstring*)u;
                float v[2] = { 0.1f, y };
                vt(overlay)->d43((void*)txt, v);
                cell++;
                y -= 0.02f;
            }
            jb += 0xc;
            y -= 0.005f;
        }
        kb += 0x38;
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
        vt(overlay)->d22(v);                                                            \
    }                                                                                   \
    {                                                                                   \
        float v[2] = { 1.0f, 0.75f };                                                   \
        vt(overlay)->d25(v);                                                            \
    }                                                                                   \
    {                                                                                   \
        tstring name(name_string);                                                      \
        vt(overlay)->d31(0, res_get(name)->clone());                                    \
    }                                                                                   \
    {                                                                                   \
        int c[1] = { -1 };                                                              \
        vt(overlay)->d38(c);                                                            \
    }

#define NEW_OVERLAY(name_string)                                                        \
    NEW_OVERLAY_BASE(name_string)                                                       \
    {                                                                                   \
        float v[2] = { 0.05f, 0.05f };                                                  \
        vt(overlay)->d27(v);                                                            \
    }                                                                                   \
    row[2] = new MissionUi;                                                             \
    {                                                                                   \
        float v[2] = { 0.1f, 0.2f };                                                    \
        vt(row[2])->d22(v);                                                             \
    }                                                                                   \
    {                                                                                   \
        float v[2] = { 0.6f, 0.3f };                                                    \
        vt(row[2])->d25(v);                                                             \
    }                                                                                   \
    {                                                                                   \
        int c[1] = { -1 };                                                              \
        vt(row[2])->d38(c);                                                             \
    }                                                                                   \
    {                                                                                   \
        float v[2] = { 0.03f, 0.03f };                                                  \
        vt(row[2])->d27(v);                                                             \
    }                                                                                   \
    vt(overlay)->d50(row[2]);

// @ 0x4dae40
void Rec44::init_screen()
{
    row[4] = 0;
    NEW_OVERLAY("mission_window")
    if (g_mission_ctx.mode == 2)
        ((MissionScreen*)this)->fill_mission_grid();
    else if (g_mission_ctx.mode == 3 || g_mission_ctx.mode == 4 || g_mission_ctx.mode == 5)
        ((MissionScreen*)this)->fill_single_mission();
    { MissionUi* root_ = (MissionUi*)overlay; g_gui.objs.push_back(root_); }
    g_gui.dirty = 1;
}



// @ 0x4db1f0
void Rec118::init_screen()
{
    NEW_OVERLAY_BASE("meeting_window")
    for (int i = 0; i < 16; i++) {
        row[i] = (MissionUi*)new UiA;
        {
            float v[2] = { 0.0f, 0.41f - i * 0.05f };
            vt(row[i])->d22(v);
        }
        {
            float v[2] = { g_5a9f20, 0.05f };      // the global (0x4db326), not the literal
            vt(row[i])->d25(v);
        }
        {
            int c[1] = { (int)0xffaaff55 };
            vt(row[i])->d39(c);
        }
        vt(overlay)->d50(row[i]);
    }
    for (int j = 0; j < 3; j++) {
        col[j] = new text_box;
        {
            float v[2] = { g_5a9f20, g_5a9f24 - (j + 1) * g_5a9f28[1] };     // globals in the original (0x4db3aa / 0x4db3bf / 0x4db3c6)
            vt(col[j])->d22(v);
        }
        vt(col[j])->d25(g_5a9f28);
        vt(overlay)->d50(col[j]);
    }
    { MissionUi* root_ = (MissionUi*)overlay; g_gui.dirty = 1; g_gui.objs.push_back(root_); }
    ((MissionScreen*)this)->fill_mission_row(0);
    ((MissionScreen*)this)->fill_mission_row(1);
    ((MissionScreen*)this)->fill_mission_row(2);
}

// @ 0x4db440
void MissionScreen::fill_mission_row(int side)
{
    int id;
    if (side == 0)
        id = g_game->f78;
    else if (g_mission_ctx.side < 0)
        id = g_mission_ctx.ids[side - 1];          // side 1 / 2: the two sides' party ids
    else if (side == 1)
        id = g_mission_ctx.ids[g_mission_ctx.side];
    else
        id = g_mission_ctx.ids[1 - g_mission_ctx.side];
    if (id == -1)
        return;
    int pidx = g_game->parties.lookup(id);
    Party* party = (Party*)((char*)g_game->parties.pages._Myfirst[pidx >> 4] + (pidx & 0xf) * 0x5ac8);      // PartyMgr::get_party with the lookup 0x48ad00 under its defining class

    float v[2];
    const tstring* text;
    if (g_module_settings.show_party_ids_instead_of_names)
        text = &party->key;
    else if (party->flag == 0 && (text = g_strtable.find(&party->key)) != 0)
        text = text;
    else
        text = &party->name;
    vt(col[side])->d41(v, (void*)text);

    float y = 0.15f;
    for (int i = 0; i < party->first_flagged(); i++) {
        Stack* rec = (Stack*)((char*)party + 0xa8) + i;
        Troop* u = &g_game->troops[rec->troop];
        char buf[0x14];   // 0x14 like the original: its sprintf of a long count pair overflows into the line buffer that follows
        if (u->flags30 & 0x10) {
            int h = ((unsigned char*)u)[0x7a] & 0xf;
            if (h > g_skill_tbl[36].max_rank)
                h = g_skill_tbl[36].max_rank;
            sprintf(buf, " %d%%",
                    (int)(u->hp * 100.0f / (u->lvl58 + h * 2 + 0x23) + 0.0001f));
        } else {
            sprintf(buf, " (%d/%d)", rec->count - rec->wounded, rec->count);
        }
        const tstring* nm = g_strtable.find(&u->key);
        if (!nm)
            nm = (const tstring*)u;
        const char* nm2 = nm->c_str();
        char line[0x30];
        sprintf(line, "%s %s", nm2, buf);
        {
            tstring ts(line);
            v[0] = 0.01f;
            v[1] = y;
            vt(col[side])->d43(&ts, v);
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
        vt(overlay)->d22(mv);
    }
    {
        float sv[2];
        sv[0] = 1.0f;
        sv[1] = 0.75f;
        vt(overlay)->d25(sv);
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
    ((UiA*)row[2])->f184 |= 1;
    {
        tstring n("ui_factions");
        struct { int a, b; } ret;
        vt(row[2])->d41(&ret, (void*)g_strtable_c.lookup(n));
    }
    {
        void* self = overlay;
        g_gui.dirty = 1;
        g_gui.objs.push_back((MissionUi* const&)(self));
    }
    {
        tstring none("");
        struct { int a, b; } ret;
        vt(overlay)->d41(&ret, &none);
    }
    if (w04 == 0)
        ((MissionScreen*)this)->draw_faction_report();
}

