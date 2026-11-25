#include "platform/image_addrs.h"
#include "game/config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "game/campaign/campaign_screen_init.h"
#include "game/ui/gui_manager.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "engine/core/tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/data/str_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/ui/widgets/ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/ui/widgets/container.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/campaign/map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/campaign/campaign_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "engine/audio/sound_mgr.h"   // g_sound_mgr 0x5aa128
#include "game/common/shared_vars.h"
#include "game/ui/ui_globals.h"      // UI / core globals
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object

#include "engine/render/render_flags.h"
#include "engine/scene/world.h"      // rgl::world (ground at +0xb0)
#include "game/terrain/nav_data.h"      // NavData / NavVert (the scene's ground vertices)

// the widget a member names, as an inline accessor: the object is then evaluated once, between the key string's
// constructor and the lookup (0x4eca9b: mov edi,[esi]; mov ebx,[edi] before the call, mov ecx,edi after it)
static inline MissionUi* label_of(MissionUi* w) { return w; }

// std::vector<MissionUi*>::push_back as the original expands it at 0x4ec824: the fast path fills through the out-of-line
// std::_Uninitialized_fill_n of the pointer vector (0x40f8a0, ICF-folded: last, 1, &value, allocator; cdecl), the slow
// path is insert -> _Insert_n (0x4927d0). The fill is defined here, inline + noinline: a COMDAT other units' implicit
// instances fold with, so the call site keeps the out-of-line call and both builds link.
namespace std {
template<> inline __declspec(noinline) void _Uninitialized_fill_n<MissionUi**, vector<MissionUi*>::size_type, MissionUi*, allocator<MissionUi*> >(
    MissionUi** _First, vector<MissionUi*>::size_type _Count, MissionUi* const& _Val, allocator<MissionUi*>& _Al)
{
    _Uninit_fill_n(_First, _Count, _Val, _Al, _Ptr_cat(_First, _First));
}
}
struct kids_vec : std::vector<MissionUi*> {
    __forceinline void push(MissionUi* const& _Val)
    {
        if (size() < capacity()) {
            MissionUi** p = _Mylast;
            std::_Uninitialized_fill_n(p, (size_type)1, _Val, this->_Alval);
            p = p + 1;
            _Mylast = p;
        } else
            insert(end(), _Val);
    }
};
// the same push_back at 0x4ec9b9: the original's fast path there is the plain store, not the fill call
struct kids_vec2 : std::vector<MissionUi*> {
    __forceinline void push(MissionUi* const& _Val)
    {
        if (size() < capacity()) {
            MissionUi** p = _Mylast;
            *p = _Val;
            _Mylast = p + 1;
        } else
            insert(end(), _Val);
    }
};

// the party record of dense index idx (pages of 16), as the original indexes it
static Party* party_bytes(PartyMgr* mgr, int idx)
{
    return mgr->pages[idx >> 4] + (idx & 0xf);
}

// @ 0x4ec4d0
void CampaignScreen::init_screen()
{
    g_gui.reset();
    g_gui.hud_flag = 1;
    g_game->editor.active = 0;
    g_sound_mgr.stop_all();   // 0x494310
    g_game->map_live = 1;
    f2d4 = 0;
    f2d5 = 0;

    NodeA* bar = new NodeA(0);
    overlay = bar;
    overlay->move_a(V2(0.0f, 0.0f));
    overlay->set_xy(V2(1.0f, 0.75f));
    {
        NodeA* w = overlay;
        g_gui.dirty = 1;
        g_gui.objs.push_back((NodeA* const&)w);   // 0x49b120
    }

    f34 = (MissionUi*)new text_box;
    f34->move_a(V2(0.0f, 0.0f));
    f34->set_xy(V2(1.0f, 0.05f));
    {
        rgl::mesh* bar_mesh;
        {
            tstring nm("map_bar");
            bar_mesh = ((rgl::mesh*)g_resmgr.get_object(&nm))->clone();
        }
        f34->set_mesh(0, bar_mesh);
    }
    f48 = 0;
    if (g_cfg.enable_edit_mode) {
        tstring t("ui_terrain");
        f48 = (MissionUi*)make_short_button(f34, g_strtab.lookup(t), 0.002f, 0.006f, 0.03f);
    }
    {
        tstring t("ui_camp");
        f4c = (MissionUi*)new_medium_button(f34, g_strtab.lookup(t), 0.1f, 0.006f, 0.03f);
    }
    {
        tstring t("ui_reports");
        f38 = (MissionUi*)new_medium_button(f34, g_strtab.lookup(t), 0.216f, 0.006f, 0.03f);
    }
    {
        tstring t("ui_quests");
        f50 = (MissionUi*)new_medium_button(f34, g_strtab.lookup(t), 0.332f, 0.006f, 0.03f);
    }
    {
        tstring t("ui_inventory");
        f40 = (MissionUi*)new_medium_button(f34, g_strtab.lookup(t), 0.448f, 0.006f, 0.03f);
    }
    {
        tstring t("ui_character");
        f44 = (MissionUi*)new_medium_button(f34, g_strtab.lookup(t), 0.564f, 0.006f, 0.03f);
    }
    {
        tstring t("ui_party");
        f3c = (MissionUi*)new_medium_button(f34, g_strtab.lookup(t), 0.68f, 0.006f, 0.03f);
    }
    {
        MissionUi* bar_copy = f34;
        ((kids_vec&)overlay->kids).push(bar_copy);
    }   // 0x4ec824: the map-bar overlay takes the button bar

    {
        {
            tstring empty("");
            f5c = (MissionUi*)make_hint((ContainerBase*)f34, empty, 0.8f, -0.006f, 0.2f, 0.027f);
        }
        f5c->set148(0x11);
        f5c->b122 = 1;                               // 0x4ec8dd: +0x122
    }
    {
        {
            tstring empty("");
            f60 = (MissionUi*)make_hint((ContainerBase*)f34, empty, 0.8f, 0.015f, 0.2f, 0.027f);
        }
        f60->set148(0x11);
        f60->b122 = 1;                               // 0x4ec93c: +0x122
    }

    f30 = new MapWidget;
    f30->world = g_script.level;
    memcpy(&f30->scene3d.fr, &cam, 0x120);
    f30->move_a(V2(0.0f, 0.04f));
    f30->set_xy(V2(1.0f, 0.71f));
    ((kids_vec2&)overlay->kids).push((MissionUi*)f30);            // inlined in the original (0x4ec9b9): store or _Insert_n 0x4927d0
    f54 = new MissionUi;
    f54->move_a(V2(0.445f, 0.06f));        // 0x4eca31
    f54->set_xy(V2(0.09f, 0.04f));         // 0x4eca4f
    f54->set_color(0xffffffffu);
    f54->set_size(V2(0.03f, 0.03f));
    label_of(f54)->set_text(*g_strtab.lookup("ui_paused"));
    overlay->kids.push_back(f54);

    f58 = new MissionUi;
    f58->move_a(V2(0.73f, 0.045f));
    f58->set_xy(V2(0.27f, 0.03f));
    f58->set_color(0xffffffffu);
    f58->set_size(V2(0.018f, 0.018f));
    label_of(f58)->set_text(*g_strtab.lookup("ui_click_left_button_to_cancel_wait"));
    overlay->kids.push_back(f58);

    {
        PartyMgr* mgr = &g_game->parties;
        int rec = mgr->lookup(g_game->side_party);   // 0x48ad00
        target = *(const rgl::vector4*)&party_bytes(mgr, rec)->pos;   // the party's map position (0x4ecbc8)
    }
    {
        target.z = target.z + 1.0f;                           // +0x298 (camera target z; 0x4ecc09), not the zoom at +0x2b8
        *(vec2*)&f2bc = V2(0.0f, 0.0f);                // the pan rates, one pair (0x4ecc01: through the [esp+0x10] temporary)
        (int&)f2c4 = 0;                                // the yaw and zoom rates, stored as integer zeros
        (int&)f2c8 = 0;
    }
    camera_mark_entities();
    camera_update(0);                                      // 0x4ece20 (push ebx = 0)

    if ((void*)g_shadow_map.cam != (void*)g_script.level && g_use_shaders) {
        g_shadow_map.attach_camera((rgl::world*)g_script.level);   // 0x4711e0
        for (unsigned i = 0; i < g_script.level->ground->verts.size(); i++)
            g_shadow_map.extend(&g_script.level->ground->verts[i].p);   // 0x468c20
        g_shadow_map.fit_bounds();   // 0x463cc0
        g_shadow_map.render();   // 0x47fef0
    }
}

// PartyMgr::next_party_id (0x460ca0; the row is owned elsewhere): a visible duplicate, as in game_data_party_quest.cpp. With the body in
// this unit cl knows the callee does not keep &id, so the loop counter lives in a register and its home store is independent of
// the bound's load (the store schedules after the compare, as in the original).
__declspec(noinline) void PartyMgr::next_party_id(int* id)
{
    unsigned i = lookup(*id) + 1;
    while ((int)i < count && (&*pages.begin())[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        *id = serial;
    else
        *id = (&*pages.begin())[i >> 4][i & 0xf].id;
}
// @ 0x4ecce0
void CampaignScreen::camera_mark_entities()
{
    extern float g_one;   // 0x549cdc (float_consts.h; declared here, the shared header is not ours to edit)
    g_game->f13acc = (f2b8 * 0.07f + g_one) * 0.8f;
    GameData* game = g_game;
    PartyMgr* mgr = &game->parties;
    for (unsigned cur = (unsigned)mgr->first_party_id(); cur < game->parties.serial; mgr->next_party_id((int*)&cur)) {
            int idx = mgr->lookup(cur);
            if (party_bytes(mgr, idx)->model) {
                idx = mgr->lookup(cur);
                if (party_bytes(mgr, idx)->flags & 0x400) {
                    idx = mgr->lookup(cur);
                    if (!(party_bytes(mgr, idx)->flags & 0x80000)) {
                        idx = mgr->lookup(cur);
                        party_bytes(mgr, idx)->label_dirty = 1;
                        game = g_game;
                    }
                }
            }
            mgr = &game->parties;
    }
}
