#include "./screen_init.h"
#include "../gui_mgr.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../overlay.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../campaign_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4ec4d0
void Screen::init_screen()
{
    g_gui.reset();
    g_game->f1a090 = 0;
    g_5e212c = 1;
    stop_all((void*)0x5aa128);
    g_game->f88 = 1;
    f2d4 = 0;
    f2d5 = 0;

    Overlay* bar = (Overlay*)operator new(0x190);
    if (bar != 0) {
        new (bar) Overlay();
        *(void**)bar = (void*)0x592848;
        bar->f188 = 0;
    } else {
        bar = 0;
    }
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
    g_gui.add_root(overlay);
    Overlay* self = overlay;
    self->children.push_back3(*(Widget**)&overlay, 4);

    Overlay* buttons = (Overlay*)operator new(0x188);
    if (buttons != 0) {
        new (buttons) Overlay();
        *(void**)buttons = (void*)0x597050;
    } else {
        buttons = 0;
    }
    f34 = (Widget*)buttons;
    p0.x = 0.0f;
    p0.y = 0.0f;
    buttons->set_pos(&p0.x);
    s0.x = 1.0f;
    s0.y = 0.05f;
    buttons->set_scale(&s0.x);
    {
        buttons->set_mesh(0, clone(g_resmgr.get_object(tstring("map_bar"))));
    }
    f48 = 0;
    if (g_5a9fb0) {
        f48 = (int)make_short_button(buttons, (void*)g_strtab.lookup(tstring("ui_terrain")));
    }
    f4c = new_medium_button(buttons, (void*)g_strtab.lookup(tstring("ui_camp")), 0.1f, 0.006f, 0.03f);
    f38 = new_medium_button(buttons, (void*)g_strtab.lookup(tstring("ui_reports")), 0.18f, 0.006f, 0.03f);
    f50 = new_medium_button(buttons, (void*)g_strtab.lookup(tstring("ui_quests")), 0.33f, 0.006f, 0.03f);
    f40 = new_medium_button(buttons, (void*)g_strtab.lookup(tstring("ui_inventory")), 0.448f, 0.006f, 0.03f);
    f44 = new_medium_button(buttons, (void*)g_strtab.lookup(tstring("ui_character")), 0.5625f, 0.006f, 0.03f);
    f3c = new_medium_button(buttons, (void*)g_strtab.lookup(tstring("ui_party")), 0.68f, 0.006f, 0.03f);
    {
        Overlay* self = (Overlay*)buttons;
        self->children.push_back3(*(Widget**)&buttons, 4);
    }

    {
        tstring none("");
        f5c = make_hint(none, 0xf, buttons, vector2(0.8f, 0.2f), vector2(0.015f, 0.05f));
        int flag = 0x11;
        f5c->set_flag(&flag);
        f5c->b121 = 1;
    }
    {
        tstring none("");
        f60 = make_hint(none, 0xf, buttons, vector2(0.8f, 0.2f), vector2(0.012f, 0.05f));
        int flag = 0x11;
        f60->set_flag(&flag);
        f60->b121 = 1;
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
    overlay->children.push_back(*(Widget**)&map);
    f54 = new_widget(0x178);
    {
        vector2 pp;
        pp.x = 0.182f;
        pp.y = 0.045f;
        f54->set_pos(&pp.x);
    }
    {
        vector2 ps;
        ps.x = 0.27f;
        ps.y = 0.03f;
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
        f54->set_text(&name, *(const tstring*)g_strtab.lookup(name));
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
        f58->set_text(&name, *(const tstring*)g_strtab.lookup(name));
    }
    overlay->children.push_back2(*(Widget**)&f58);

    {
        PartyMgr* mgr = (PartyMgr*)((char*)g_game + 0xf8);
        int rec = mgr->index_of(g_game->side_party);
        Rect* r = (Rect*)((char*)mgr + (rec >> 4) * 4 + *(int*)((char*)mgr + 0x138d0) + (rec & 0xf) * 0x5ac8 + 0x30);
        *(Rect*)((char*)this + 0x290) = *r;
    }
    {
        float zero = 0.0f;
        f2b8 = f2b8 + 1.0f;
        *(float*)((char*)this + 0x2bc) = zero;
        *(float*)((char*)this + 0x2c0) = zero;
        *(float*)((char*)this + 0x2c4) = zero;
        *(float*)((char*)this + 0x2c8) = zero;
    }
    camera_mark_entities();

    if (g_level != g_a70438 && g_use_shaders) {
        g_psys.attach_camera(g_level);
        void* scene_objs = *(void**)((char*)g_level + 0xb0);
        int count = (*(int*)((char*)scene_objs + 8) - *(int*)((char*)scene_objs + 4)) / 0x24;
        char* p = *(char**)((char*)scene_objs + 4);
        for (int i = 0; i < count; i++, p += 0x24) {
            g_psys.extend(p);
        }
        g_psys.fit_bounds();
        g_psys.render();
    }
}

// the 0x5ac8-byte record of dense index idx (page table at PartyMgr +0x138d0), as the original indexes it
static char* party_bytes(PartyMgr* mgr, int idx)
{
    return (char*)mgr->pages._Myfirst[idx >> 4] + (idx & 0xf) * 0x5ac8;
}

// @ 0x4ecce0
void Screen::camera_mark_entities()
{
    g_game->f13acc = (f2b8 * g_590bb8 + g_549cdc) * g_54b3b4;
    PartyMgr* mgr = (PartyMgr*)((char*)g_game + 0xf8);
    for (unsigned id = (unsigned)mgr->first_party_id(); id < (unsigned)g_game->parties.n_parties; mgr->next_party_id(&id)) {
        int idx = mgr->index_of(id);
        if (*(int*)(party_bytes(mgr, idx) + 0x59c0)) {
            idx = mgr->index_of(id);
            if (*(long long*)(party_bytes(mgr, idx) + 0xa0) & 0x400) {
                idx = mgr->index_of(id);
                if (!(*(long long*)(party_bytes(mgr, idx) + 0xa0) & 0x80000)) {
                    idx = mgr->index_of(id);
                    party_bytes(mgr, idx)[0x8b5] = 1;
                }
            }
        }
    }
}
