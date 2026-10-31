#include "../../include/image_addrs.h"
#include "./screen_init.h"
#include "../gui_mgr.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../overlay.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../campaign_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)

// std::vector<Widget*>::push_back as the original inlines it (0x4ec824): with room, std::_Uninit_fill_n of the pointer
// instance (0x40f8a0, ICF-folded: last, 1, &value, allocator; cdecl), else _Insert_n at the end (0x4927d0, thiscall)
void __cdecl uninit_fill_ptrs(Widget** dst, unsigned n, Widget* const* v, void* al);   // 0x40f8a0
static inline void push_child(vec<Widget*>& c, Widget* const& v)
{
    unsigned n = c.first == 0 ? 0 : (unsigned)(c.last - c.first);
    if (c.first != 0 && n < (unsigned)(c.end - c.first)) {
        Widget** p = c.last;
        uninit_fill_ptrs(p, 1, &v, &c);
        c.last = p + 1;
    } else {
        ((SceneObjVec*)&c)->_Insert_n(std::vector<SceneObject*>::iterator((SceneObject**)c.last), 1, (SceneObject* const&)v);
    }
}

// the 0x5ac8-byte record of dense index idx (page table at PartyMgr +0x138d0), as the original indexes it
static char* party_bytes(PartyMgr* mgr, int idx)
{
    return (char*)mgr->pages._Myfirst[idx >> 4] + (idx & 0xf) * 0x5ac8;
}

// @ 0x4ec4d0
void Screen::init_screen()
{
    g_gui.reset();
    g_game->f1a090 = 0;
    g_5e212c = 1;
    ((SoundW*)&orig::g_sound_mgr)->stop_all();   // 0x494310
    g_game->f88 = 1;
    f2d4 = 0;
    f2d5 = 0;

    Overlay* bar = (Overlay*)new_container(0x190, vtbl_592848);   // 0x4931a0, then vptr 0x592848
    if (bar != 0)
        bar->f188 = 0;
    overlay = bar;
    vector2 p0;
    p0.x = 0.0f;
    p0.y = 0.0f;
    overlay->set_pos(&p0.x);
    vector2 s0;
    s0.x = 1.0f;
    s0.y = 0.75f;
    overlay->set_scale(&s0.x);
    g_5d8788 = 1;
    ((NodeVec*)&orig::g_gui_mgr)->push_back((NodeA* const&)overlay);   // 0x49b120

    Overlay* buttons = (Overlay*)new_container(0x188, vtbl_597050);   // 0x4931a0, then vptr 0x597050
    f34 = (Widget*)buttons;
    p0.x = 0.0f;
    p0.y = 0.0f;
    buttons->set_pos(&p0.x);
    s0.x = 1.0f;
    s0.y = 0.05f;
    buttons->set_scale(&s0.x);
    {
        void* bar_mesh;
        {
            tstring map_bar("map_bar");
            bar_mesh = (void*)((mesh_x*)g_alloc.get_object((const rgl::string*)&map_bar))->clone();
        }                                           // the key string dies before set_mesh (0x4ec681 dtor, then 0x4ec6.. call 0x488aa0)
        buttons->set_mesh(0, bar_mesh);
    }
    f48 = 0;
    if (g_5a9fb0) {
        f48 = (int)make_short_button((MissionUi*)buttons, g_strtab.lookup(tstring("ui_terrain")), 0.002f, 0.006f, 0.03f);
    }
    f4c = (Widget*)new_medium_button((MissionUi*)buttons, g_strtab.lookup(tstring("ui_camp")), 0.1f, 0.006f, 0.03f);
    f38 = (Widget*)new_medium_button((MissionUi*)buttons, g_strtab.lookup(tstring("ui_reports")), 0.216f, 0.006f, 0.03f);
    f50 = (Widget*)new_medium_button((MissionUi*)buttons, g_strtab.lookup(tstring("ui_quests")), 0.332f, 0.006f, 0.03f);
    f40 = (Widget*)new_medium_button((MissionUi*)buttons, g_strtab.lookup(tstring("ui_inventory")), 0.448f, 0.006f, 0.03f);
    f44 = (Widget*)new_medium_button((MissionUi*)buttons, g_strtab.lookup(tstring("ui_character")), 0.564f, 0.006f, 0.03f);
    f3c = (Widget*)new_medium_button((MissionUi*)buttons, g_strtab.lookup(tstring("ui_party")), 0.68f, 0.006f, 0.03f);
    push_child(overlay->children, *(Widget**)&f34);   // 0x4ec824: the map-bar overlay takes the button bar

    {
        {
            tstring none("");
            f5c = (WidgetObj*)make_hint((ContainerBase*)buttons, none, 0.8f, -0.006f, 0.2f, 0.027f);
        }                                           // the empty key string dies before set_flag (0x4ec8bf dtor, then vt+0x74)
        int flag = 0x11;
        f5c->set_flag(&flag);
        f5c->b122 = 1;                               // 0x4ec8dd: +0x122
    }
    {
        {
            tstring none("");
            f60 = (WidgetObj*)make_hint((ContainerBase*)buttons, none, 0.8f, 0.015f, 0.2f, 0.027f);
        }                                           // the empty key string dies before set_flag (0x4ec8bf dtor, then vt+0x74)
        int flag = 0x11;
        f60->set_flag(&flag);
        f60->b122 = 1;                               // 0x4ec93c: +0x122
    }

    MapWidget* map = (MapWidget*)operator new(0x358);
    if (map != 0)
        ctor_4ed630(map);
    else
        map = 0;
    f30 = map;
    map->world = (void*)g_level;
    memcpy(&map->view, &cam, 0x120);
    vector2 mp;
    mp.x = 0.0f;
    mp.y = 0.04f;
    map->set_pos(&mp.x);
    vector2 ms;
    ms.x = 1.0f;
    ms.y = 0.71f;
    map->set_scale(&ms.x);
    push_back_inline(overlay->children, *(Widget**)&map);   // inlined in the original (0x4ec9b9): store or _Insert_n 0x4927d0
    f54 = new_widget(0x178);
    {
        vector2 pp;
        pp.x = 0.445f;                               // 0x4eca31
        pp.y = 0.06f;
        f54->set_pos(&pp.x);
    }
    {
        vector2 ps;
        ps.x = 0.09f;                                // 0x4eca4f
        ps.y = 0.04f;
        f54->set_scale(&ps.x);
    }
    {
        unsigned white = 0xffffffffu;
        f54->set_color(&white);
    }
    {
        vector2 pz;
        pz.x = 0.03f;
        pz.y = 0.03f;
        f54->set_size(&pz.x);
    }
    {
        tstring name("ui_paused");
        struct { int a, b; } ret;                    // set_text's 8-byte result: its own slot, not the key string
        f54->set_text(&ret, *(const tstring*)g_strtab.lookup(name));
    }
    overlay->children.push_back2(*(Widget**)&f54);

    f58 = new_widget(0x178);
    {
        vector2 pp;
        pp.x = 0.73f;
        pp.y = 0.045f;
        f58->set_pos(&pp.x);
    }
    {
        vector2 ps;
        ps.x = 0.27f;
        ps.y = 0.03f;
        f58->set_scale(&ps.x);
    }
    {
        unsigned white = 0xffffffffu;
        f58->set_color(&white);
    }
    {
        vector2 pz;
        pz.x = 0.018f;
        pz.y = 0.018f;
        f58->set_size(&pz.x);
    }
    {
        tstring name("ui_click_left_button_to_cancel_wait");
        struct { int a, b; } ret;                    // set_text's 8-byte result: its own slot, not the key string
        f58->set_text(&ret, *(const tstring*)g_strtab.lookup(name));
    }
    overlay->children.push_back2(*(Widget**)&f58);

    {
        PartyMgr* mgr = (PartyMgr*)((char*)g_game + 0xf8);
        int rec = ((Map10007*)(void*)mgr)->lookup(g_game->side_party);   // 0x48ad00
        Rect* r = (Rect*)(party_bytes(mgr, rec) + 0x30);   // pages[rec >> 4] + (rec & 0xf) * 0x5ac8 (0x4ecbc8), not mgr + the table address
        *(Rect*)((char*)this + 0x290) = *r;
    }
    {
        float zero = 0.0f;
        f298 = f298 + 1.0f;                           // +0x298 (camera target z; 0x4ecc09), not the zoom at +0x2b8
        *(float*)((char*)this + 0x2bc) = zero;
        *(float*)((char*)this + 0x2c0) = zero;
        *(float*)((char*)this + 0x2c4) = zero;
        *(float*)((char*)this + 0x2c8) = zero;
    }
    camera_mark_entities();
    camera_update(0.0f);                                   // 0x4ece20 (push ebx = 0)

    if (g_level != g_a70438 && g_use_shaders) {
        ((ShadowMap*)g_psys)->attach_camera((CellGrid*)g_level);   // 0x4711e0
        void* scene_objs = *(void**)((char*)g_level + 0xb0);
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
void Screen::camera_mark_entities()
{
    g_game->f13acc = (f2b8 * g_590bb8 + g_549cdc) * g_54b3b4;
    PartyMgr* mgr = (PartyMgr*)((char*)g_game + 0xf8);
    for (unsigned id = (unsigned)mgr->first_party_id(); id < (unsigned)g_game->parties.n_parties; mgr->next_party_id((int*)&id)) {
        int idx = ((Map10007*)(void*)mgr)->lookup(id);
        if (*(int*)(party_bytes(mgr, idx) + 0x59c0)) {
            idx = ((Map10007*)(void*)mgr)->lookup(id);
            if (*(long long*)(party_bytes(mgr, idx) + 0xa0) & 0x400) {
                idx = ((Map10007*)(void*)mgr)->lookup(id);
                if (!(*(long long*)(party_bytes(mgr, idx) + 0xa0) & 0x80000)) {
                    idx = ((Map10007*)(void*)mgr)->lookup(id);
                    party_bytes(mgr, idx)[0x8b5] = 1;
                }
            }
        }
    }
}
