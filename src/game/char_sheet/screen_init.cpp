#include "./screen_init.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4ec4d0
void Screen::init_screen()
{
    g_gui.reset();
    g_game->f1a090 = 0;
    g_5e212c = 1;
    ctor_494310((void*)0x5aa128);
    g_game->f88 = 1;
    f2d4 = 0;
    f2d5 = 0;

    Overlay* bar = (Overlay*)operator new(0x190);
    if (bar != 0) {
        new (bar) Overlay();
        *(void**)bar = (void*)0x592848;
        *(char*)((char*)bar + 0x188) = 0;
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
    ((vec<Widget*>*)&self->first)->push_back3(*(Widget**)&overlay, 4);

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
        buttons->set_mesh(clone_41c6d0(g_resmgr.get_object(tstring("map_bar"))), 0);
    }
    f48 = 0;
    if (g_5a9fb0) {
        f48 = (int)ui_terrain_quad(buttons, (void*)g_strtab.find(tstring("ui_terrain")));
    }
    f4c = new_medium_button(buttons, (void*)g_strtab.find(tstring("ui_camp")), 0.1f, 0.006f, 0.03f);
    f38 = new_medium_button(buttons, (void*)g_strtab.find(tstring("ui_reports")), 0.18f, 0.006f, 0.03f);
    f50 = new_medium_button(buttons, (void*)g_strtab.find(tstring("ui_quests")), 0.33f, 0.006f, 0.03f);
    f40 = new_medium_button(buttons, (void*)g_strtab.find(tstring("ui_inventory")), 0.448f, 0.006f, 0.03f);
    f44 = new_medium_button(buttons, (void*)g_strtab.find(tstring("ui_character")), 0.5625f, 0.006f, 0.03f);
    f3c = new_medium_button(buttons, (void*)g_strtab.find(tstring("ui_party")), 0.68f, 0.006f, 0.03f);
    {
        Overlay* self = (Overlay*)buttons;
        ((vec<Widget*>*)&self->first)->push_back3(*(Widget**)&buttons, 4);
    }

    {
        tstring none("");
        f5c = make_hint(none, 0xf, buttons, vector2(0.8f, 0.2f), vector2(0.015f, 0.05f));
        int flag = 0x11;
        f5c->set_flag(&flag, 0);
        f5c->f121 = 1;
    }
    {
        tstring none("");
        f60 = make_hint(none, 0xf, buttons, vector2(0.8f, 0.2f), vector2(0.012f, 0.05f));
        int flag = 0x11;
        f60->set_flag(&flag, 0);
        f60->f121 = 1;
    }

    MapWidget* map = (MapWidget*)operator new(0x358);
    if (map != 0)
        ctor_4ed630(map);
    else
        map = 0;
    f30 = map;
    map->f178 = g_a732b4;
    memcpy(map->labels, labels, 0x120);
    vector2 mp;
    mp.x = 0.0f;
    mp.y = 0.04f;
    map->set_pos(&mp.x);
    vector2 ms;
    ms.x = 1.0f;
    ms.y = 0.71f;
    map->set_scale(&ms.x);
    ((vec<Widget*>*)&overlay->first)->push_back(*(Widget**)&map);
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
        int white = -1;
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
        f54->set_text((const void*)&name, (const void*)g_strtab.find(name));
    }
    ((vec<Widget*>*)&overlay->first)->push_back2(*(Widget**)&f54);

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
        int white = -1;
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
        f58->set_text((const void*)&name, (const void*)g_strtab.find(name));
    }
    ((vec<Widget*>*)&overlay->first)->push_back2(*(Widget**)&f58);

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

    if (g_a732b4 != g_a70438 && g_5e28fc) {
        g_psys.finish(g_a732b4);
        void* scene_objs = *(void**)((char*)g_a732b4 + 0xb0);
        int count = (*(int*)((char*)scene_objs + 8) - *(int*)((char*)scene_objs + 4)) / 0x24;
        char* p = *(char**)((char*)scene_objs + 4);
        for (int i = 0; i < count; i++, p += 0x24) {
            g_psys.add_scene_obj(p);
        }
        g_psys.flush();
        g_psys.reset();
    }
}
// @ 0x4ecce0
void Screen::camera_mark_entities()
{
    g_game->f13acc = (f2b8 * g_590bb8 + g_549cdc) * g_54b3b4;
    PartyMgr* mgr = (PartyMgr*)((char*)g_game + 0xf8);
    for (unsigned id = (unsigned)mgr->first_id(); id < (unsigned)g_game->parties_end; mgr->next(&id)) {
        int idx = mgr->index_of(id);
        if (*(int*)(mgr->at(idx) + 0x59c0)) {
            idx = mgr->index_of(id);
            if (*(long long*)(mgr->at(idx) + 0xa0) & 0x400) {
                idx = mgr->index_of(id);
                if (!(*(long long*)(mgr->at(idx) + 0xa0) & 0x80000)) {
                    idx = mgr->index_of(id);
                    mgr->at(idx)[0x8b5] = 1;
                }
            }
        }
    }
}
