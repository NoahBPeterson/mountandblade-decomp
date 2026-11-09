#include "../../include/image_addrs.h"
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "./screen_init.h"
#include "../strat/gui_manager.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui/container.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../campaign_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128
#include "../shared_vars.h"
#include "../ui_globals.h"      // UI / core globals
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

#include "../../include/render_flags.h"

// std::vector<MissionUi*>::push_back as the original inlines it (0x4ec824): with room, std::_Uninit_fill_n of the pointer
// instance (0x40f8a0, ICF-folded: last, 1, &value, allocator; cdecl), else _Insert_n at the end (0x4927d0, thiscall)
void __cdecl uninit_fill_ptrs(MissionUi** dst, unsigned n, MissionUi* const* v, void* al);   // 0x40f8a0
static inline void push_child(std::vector<MissionUi*>& c, MissionUi* const& v)
{
    unsigned n = c._Myfirst == 0 ? 0 : (unsigned)(c._Mylast - c._Myfirst);
    if (c._Myfirst != 0 && n < (unsigned)(c._Myend - c._Myfirst)) {
        MissionUi** p = c._Mylast;
        uninit_fill_ptrs(p, 1, &v, &c);
        c._Mylast = p + 1;
    } else {
        ((SceneObjVec*)&c)->_Insert_n(std::vector<SceneObject*>::iterator((SceneObject**)c._Mylast), 1, (SceneObject* const&)v);
    }
}

// push_back as the original inlines it at 0x4ec9b9: size first (0 when empty), then the room check, then a direct store
static inline void push_back_direct(std::vector<MissionUi*>& c, MissionUi* const& v)
{
    unsigned n = c._Myfirst == 0 ? 0 : (unsigned)(c._Mylast - c._Myfirst);
    if (c._Myfirst != 0 && n < (unsigned)(c._Myend - c._Myfirst)) {
        *c._Mylast++ = v;
    } else {
        ((SceneObjVec*)&c)->_Insert_n(std::vector<SceneObject*>::iterator((SceneObject**)c._Mylast), 1, (SceneObject* const&)v);
    }
}

// std::vector<MissionUi*>::push_back out of line, as the original calls it at 0x4ecada / 0x4ecbad: the linker folded
// every pointer vector's push_back onto one body (0x4936b0, defined as vector<Party*>'s in world_data/world_data.cpp)
struct Party;
template<> void std::vector<Party*, std::allocator<Party*> >::push_back(Party* const& v);   // 0x4936b0
static inline void push_back_shared(std::vector<MissionUi*>& c, MissionUi* const& v)
{
    ((std::vector<Party*>*)&c)->push_back(*(Party* const*)&v);
}

// the 0x5ac8-byte record of dense index idx (page table at PartyMgr +0x138d0), as the original indexes it
static char* party_bytes(PartyMgr* mgr, int idx)
{
    return (char*)mgr->pages[idx >> 4] + (idx & 0xf) * 0x5ac8;
}

// @ 0x4ec4d0
void CampaignScreen::init_screen()
{
    g_gui.reset();
    g_gui.hud_flag = 1;
    g_game->f1a090 = 0;
    g_sound_mgr.stop_all();   // 0x494310
    g_game->f88 = 1;
    f2d4 = 0;
    f2d5 = 0;

    NodeA* bar = new NodeA(0);
    overlay = bar;
    overlay->move_a((float*)&vector2(0.0f, 0.0f));
    overlay->set_xy((float*)&vector2(1.0f, 0.75f));
    g_gui.dirty = 1;
    g_gui.objs.push_back(overlay);   // 0x49b120

    f34 = (MissionUi*)new text_box;
    f34->move_a((float*)&vector2(0.0f, 0.0f));
    f34->set_xy((float*)&vector2(1.0f, 0.05f));
    {
        rgl::mesh* bar_mesh = ((rgl::mesh*)g_resmgr.get_object((const tstring*)&tstring("map_bar")))->clone();
        f34->set_mesh(0, bar_mesh);
    }
    f48 = 0;
    if (g_cfg.enable_edit_mode) {
        f48 = (int)make_short_button(f34, g_strtab.lookup(tstring("ui_terrain")), 0.002f, 0.006f, 0.03f);
    }
    f4c = (MissionUi*)new_medium_button(f34, g_strtab.lookup(tstring("ui_camp")), 0.1f, 0.006f, 0.03f);
    f38 = (MissionUi*)new_medium_button(f34, g_strtab.lookup(tstring("ui_reports")), 0.216f, 0.006f, 0.03f);
    f50 = (MissionUi*)new_medium_button(f34, g_strtab.lookup(tstring("ui_quests")), 0.332f, 0.006f, 0.03f);
    f40 = (MissionUi*)new_medium_button(f34, g_strtab.lookup(tstring("ui_inventory")), 0.448f, 0.006f, 0.03f);
    f44 = (MissionUi*)new_medium_button(f34, g_strtab.lookup(tstring("ui_character")), 0.564f, 0.006f, 0.03f);
    f3c = (MissionUi*)new_medium_button(f34, g_strtab.lookup(tstring("ui_party")), 0.68f, 0.006f, 0.03f);
    push_child(overlay->kids, (MissionUi*)f34);   // 0x4ec824: the map-bar overlay takes the button bar

    {
        f5c = (MissionUi*)make_hint((ContainerBase*)f34, tstring(""), 0.8f, -0.006f, 0.2f, 0.027f);
        int flag = 0x11;
        f5c->set148(&flag);
        f5c->b122 = 1;                               // 0x4ec8dd: +0x122
    }
    {
        f60 = (MissionUi*)make_hint((ContainerBase*)f34, tstring(""), 0.8f, 0.015f, 0.2f, 0.027f);
        int flag = 0x11;
        f60->set148(&flag);
        f60->b122 = 1;                               // 0x4ec93c: +0x122
    }

    f30 = new MapWidget;
    f30->world = (void*)g_script.level;
    memcpy(&f30->scene3d.fr, &cam, 0x120);
    f30->move_a((float*)&vector2(0.0f, 0.04f));
    f30->set_xy((float*)&vector2(1.0f, 0.71f));
    push_back_direct(overlay->kids, (MissionUi*)f30);   // inlined in the original (0x4ec9b9): store or _Insert_n 0x4927d0
    f54 = new_widget(0x178);
    f54->move_a((float*)&vector2(0.445f, 0.06f));        // 0x4eca31
    f54->set_xy((float*)&vector2(0.09f, 0.04f));         // 0x4eca4f
    {
        unsigned white = 0xffffffffu;
        f54->set_color(&white);
    }
    f54->set_size((float*)&vector2(0.03f, 0.03f));
    f54->set_text((float*)&vector2(), g_strtab.lookup(tstring("ui_paused")));
    push_back_shared(overlay->kids, f54);   // 0x4936b0 (out of line in the original)

    f58 = new_widget(0x178);
    f58->move_a((float*)&vector2(0.73f, 0.045f));
    f58->set_xy((float*)&vector2(0.27f, 0.03f));
    {
        unsigned white = 0xffffffffu;
        f58->set_color(&white);
    }
    f58->set_size((float*)&vector2(0.018f, 0.018f));
    f58->set_text((float*)&vector2(), g_strtab.lookup(tstring("ui_click_left_button_to_cancel_wait")));
    push_back_shared(overlay->kids, f58);   // 0x4936b0 (out of line in the original)

    {
        PartyMgr* mgr = &g_game->parties;
        int rec = mgr->lookup(g_game->side_party);   // 0x48ad00
        Rect* r = (Rect*)(party_bytes(mgr, rec) + 0x30);   // pages[rec >> 4] + (rec & 0xf) * 0x5ac8 (0x4ecbc8), not mgr + the table address
        *(Rect*)((char*)this + 0x290) = *r;
    }
    {
        target.z = target.z + 1.0f;                           // +0x298 (camera target z; 0x4ecc09), not the zoom at +0x2b8
        *(vector2*)((char*)this + 0x2bc) = vector2(0.0f, 0.0f);
        *(vector2*)((char*)this + 0x2c4) = vector2(0.0f, 0.0f);
    }
    camera_mark_entities();
    camera_update(0);                                      // 0x4ece20 (push ebx = 0)

    if ((int)g_script.level != (int)g_shadow_map.cam && g_use_shaders) {
        ((ShadowMap*)g_psys)->attach_camera((rgl::world*)g_script.level);   // 0x4711e0
        void* scene_objs = *(void**)((char*)g_script.level + 0xb0);
        int count = (*(int*)((char*)scene_objs + 8) - *(int*)((char*)scene_objs + 4)) / 0x24;
        char* p = *(char**)((char*)scene_objs + 4);
        for (int i = 0; i < count; i++, p += 0x24) {
            ((ShadowMap*)g_psys)->extend((const rgl::vector4*)p);   // 0x468c20
        }
        ((ShadowMap*)g_psys)->fit_bounds();   // 0x463cc0
        ((ShadowMapP*)g_psys)->render();   // 0x47fef0
    }
}

// @ 0x4ecce0
void CampaignScreen::camera_mark_entities()
{
    extern float g_one;   // 0x549cdc (float_consts.h; declared here, the shared header is not ours to edit)
    g_game->f13acc = (f2b8 * 0.07f + g_one) * 0.8f;
    GameData* game = g_game;
    PartyMgr* mgr = &game->parties;
    unsigned cur = (unsigned)mgr->first_party_id();
    unsigned id = cur;
    if (cur < game->parties.n_parties) {
        do {
            int idx = mgr->lookup(cur);
            if (*(int*)(party_bytes(mgr, idx) + 0x59c0)) {
                idx = mgr->lookup(cur);
                if (*(long long*)(party_bytes(mgr, idx) + 0xa0) & 0x400) {
                    idx = mgr->lookup(cur);
                    if (!(*(long long*)(party_bytes(mgr, idx) + 0xa0) & 0x80000)) {
                        idx = mgr->lookup(cur);
                        party_bytes(mgr, idx)[0x8b5] = 1;
                        game = g_game;
                    }
                }
            }
            mgr = &game->parties;
            mgr->next_party_id((int*)&id);
            cur = id;
        } while (cur < game->parties.n_parties);
    }
}
