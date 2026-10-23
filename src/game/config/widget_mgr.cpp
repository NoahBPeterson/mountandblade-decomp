//-----------------------------------------------------------------------------
// File: widget_mgr.cpp  (0x499830..0x49ad20)
//
// Desc: the editor window's widget manager: factory methods for the medium button, the scene (xyz) buttons, the
//       progress bar, the drop-down button and the party button (each builds a widget, fills its mesh slots from
//       the resource manager, hooks it into the scene container and appends it to the manager's vector), the
//       preset placement helpers (rescale a placed widget for a new size), and MissionUi's text layout wrapper.
//       Names provisional. Functions in ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP.
//
// Boundary: DropDown::on_event (0x49aca0) calls DropDown::create (0x49ad20) forward, so the object ends before
// 0x49ad20 (see MIGRATION.md). The start (0x499830) is a class change only (obj_tbl|widget_mgr).
//-----------------------------------------------------------------------------
#include "./widget_mgr.h"

// @ 0x499830
int widget_mgr::add_medium(int id)
{
    MediumButton* w = new MediumButton;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        vt(w)->d22(&v);
        *(int*)&v.x = 0x10;
        vt(w)->d29(&v);
    }

    {
        gui_mesh* m;
        {
            rgl::string n("medium_button");
            m = clone_gui(g_resmgr_full.get_object(&n));
        }
        m->scale(0.5555556f);
        vt(w)->d32(2, m);
    }
    {
        gui_mesh* m;
        {
            rgl::string n("medium_button_down");
            m = clone_gui(g_resmgr_full.get_object(&n));
        }
        m->scale(0.5555556f);
        vt(w)->d32(1, m);
    }

    {
        unsigned c = 0xff007700;
        vt(w)->d39(&c);
    }

    {
        vec2 v;
        v.x = 0.148f;
        v.y = 0.04f;
        vt(w)->d25(&v);
        vt(w)->d41(&v, (void*)(size_t)id);
        vec2 s;
        s.x = 0.148f;
        s.y = 0.04f;
        *(vec2*)&w->fD8 = s;
    }

    scene->d50(w);
    widgets.push_back(w);
    return (int)(widgets.size() - 1);
}

// @ 0x499a20
int widget_mgr::add_xyz(int a, int b)
{
    XyzButton* w = new XyzButton;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        vt(w)->d22(&v);
        *(int*)&v.x = 0x700;
        vt(w)->d29(&v);
    }

    vt(w)->d32(2, g_scene.placed(a));
    vt(w)->d32(1, g_scene.placed(b));
    vt(w)->d03();

    scene->d50(w);
    widgets.push_back(w);
    return (int)(widgets.size() - 1);
}

// @ 0x499b50
int widget_mgr::add_xyz_clicked(int a, int b)
{
    int i = add_xyz(a, b);
    ((XyzButton*)widgets[i])->set_flag();
    return i;
}

// @ 0x499b80
int widget_mgr::add_progress(int lo, int hi)
{
    UiC* w = new UiC;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        vt(w)->d22(&v);
    }

    {
        vector4 sc(0.38940807f, 1.0f, 1.0f);

        {
            rgl::string n("progressbar");
            vt(w)->d32(0, clone_gui(g_resmgr_full.get_object(&n)));
        }
        ((gui_mesh*)w->tracks[0])->set_scale(sc);

        {
            rgl::string n("handle_hor");
            vt(w)->d32(2, clone_gui(g_resmgr_full.get_object(&n)));
        }
        {
            rgl::string n("progressbar_handle");
            w->f18c = (int)g_resmgr_full.get_object(&n);
        }
        *(vector4*)&w->f198 = sc;
    }
    vt(w)->d03();

    {
        vec2 h;
        h.x = 0.25f;
        h.y = w->screen[1];
        vt(w)->d25(&h);
    }

    float fa = (float)lo;
    w->f16c = fa;
    w->f170 = (float)hi;
    vt(w)->d13(fa);

    scene->d50(w);
    widgets.push_back(w);
    return (int)(widgets.size() - 1);
}

// @ 0x499d90
int widget_mgr::add_dropdown()
{
    DropDown* w = new DropDown;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        vt(w)->d22(&v);
    }

    {
        rgl::string n("button_drop");
        vt(w)->d32(2, clone_gui(g_resmgr_full.get_object(&n)));
    }
    {
        vector4 c(1.0f, 0.5f, 1.0f);
        ((gui_mesh*)w->tracks[2])->set_scale(c);
    }

    {
        rgl::string n("button_drop_clicked");
        vt(w)->d32(1, clone_gui(g_resmgr_full.get_object(&n)));
    }
    {
        vector4 c(1.0f, 0.5f, 1.0f);
        ((gui_mesh*)w->tracks[1])->set_scale(c);
    }

    {
        rgl::string n("button_drop_hl");
        vt(w)->d32(3, clone_gui(g_resmgr_full.get_object(&n)));
    }
    {
        vector4 c(1.0f, 0.5f, 1.0f);
        ((gui_mesh*)w->tracks[3])->set_scale(c);
    }

    {
        rgl::string n("button_drop_child");
        w->m1a4 = g_resmgr_full.get_object(&n);
    }
    {
        rgl::string n("button_drop_child_clicked");
        w->m1a0 = g_resmgr_full.get_object(&n);
    }
    {
        rgl::string n("button_drop_child_hl");
        w->m1a8 = g_resmgr_full.get_object(&n);
    }

    vt(w)->d03();

    {
        vec2 p;
        p.x = 0.02f;
        p.y = 0.02f;
        vt(w)->d27(&p);
        p.x = 0.02f;
        p.y = 0.013f;
        vt(w)->d28(&p);
        {
            rgl::string n("");
            vt(w)->d41(&p, &n);
        }
    }

    scene->d50(w);
    widgets.push_back(w);
    return (int)(widgets.size() - 1);
}

// @ 0x49a0b0
int widget_mgr::add_party()
{
    UiE* w = new UiE(0);

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        vt(w)->d22(&v);
    }

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.009f;
        vt(w)->d28(&v);
    }
    {
        vec2 v;
        v.x = 0.02f;
        v.y = 0.02f;
        vt(w)->d27(&v);
    }

    {
        rgl::string n("party_button");
        vt(w)->d32(2, clone_gui(g_resmgr_full.get_object(&n)));
    }
    {
        vector4 c(0.5f, 0.5f, 1.0f);
        ((gui_mesh*)w->tracks[2])->set_scale(c);
    }

    {
        rgl::string n("party_button_down");
        vt(w)->d32(1, clone_gui(g_resmgr_full.get_object(&n)));
    }
    {
        vector4 c(0.5f, 0.5f, 1.0f);
        ((gui_mesh*)w->tracks[1])->set_scale(c);

        vt(w)->d03();

        {
            rgl::string n("");
            vt(w)->d41(&c, &n);
        }
    }

    scene->d50(w);
    widgets.push_back(w);
    return (int)(widgets.size() - 1);
}

// rescale child mesh `sub` of a widget for a new size; the old size is MissionUi::fD8 / fDC
#define ITEM ((WItem*)widgets[idx])
#define STORE_SIZE() \
    do { WItem* it_ = ITEM; it_->fD8 = pos[0]; it_->fDC = pos[1]; } while (0)
#define SCALE_BY_OLD(t) \
    ((gui_mesh*)ITEM->tracks[t])->set_scale(vector4(pos[0] / ITEM->fD8, pos[1] / ITEM->fDC, 1.0f))

// @ 0x49a300
void widget_mgr::apply_preset(int idx, const float* pos)
{
    float sz[4];                                // out of slot 41 (text size), never read
    if (vt(ITEM)->kind() == 0 || vt(ITEM)->kind() == 1)
        goto base;
    if (vt(ITEM)->kind() == 9) {
        SCALE_BY_OLD(2);
        SCALE_BY_OLD(1);
        vt(ITEM)->d25((void*)pos);
        vt(ITEM)->d41(sz, &ITEM->name);
        STORE_SIZE();
        return;
    }
    if (vt(ITEM)->kind() == 4) {
        SCALE_BY_OLD(0);
        vt(ITEM)->d03();
        STORE_SIZE();
        return;
    }
    if (vt(ITEM)->kind() == 8) {
        SCALE_BY_OLD(2);
        SCALE_BY_OLD(1);
        vt(ITEM)->d03();
        STORE_SIZE();
        return;
    }
    if (vt(ITEM)->kind() == 2) {
        SCALE_BY_OLD(0);
        SCALE_BY_OLD(2);
        vt(ITEM)->d03();
        vt(ITEM)->d25(&vector4(pos[0] * 0.25f, ITEM->screen[1], 1.0f));
        STORE_SIZE();
        return;
    }
    if (vt(ITEM)->kind() == 3) {
        *(vector4*)&((UiC*)ITEM)->f198 = vector4(pos[0] * 0.38940808f, pos[1], 1.0f);
        *(int*)&((UiC*)ITEM)->f194 = (int)0xcb3c57cb;
        SCALE_BY_OLD(0);
        SCALE_BY_OLD(2);
        vt(ITEM)->d03();
        vt(ITEM)->d25(&vector4(pos[0] * 0.25f, ITEM->screen[1], 1.0f));
        STORE_SIZE();
        return;
    }
    if (vt(ITEM)->kind() == 5) {
        SCALE_BY_OLD(2);
        SCALE_BY_OLD(1);
        SCALE_BY_OLD(3);
        vt(ITEM)->d27(&vector4(pos[0] * 0.02f, pos[1] * 0.02f, 1.0f));
        vt(ITEM)->d28(&vector4(0.0f, pos[1] * 0.013f, 1.0f));
        vt(ITEM)->d03();
        STORE_SIZE();
        vt(ITEM)->d41(sz, &ITEM->name);
        return;
    }
    if (vt(ITEM)->kind() == 6) {
        SCALE_BY_OLD(2);
        SCALE_BY_OLD(1);
        vt(ITEM)->d27(&vector4(pos[0] * 0.02f, pos[1] * 0.02f, 1.0f));
        vt(ITEM)->d28(&vector4(0.0f, pos[1] * 0.009f, 1.0f));
        vt(ITEM)->d03();
        STORE_SIZE();
        return;
    }
    if (vt(ITEM)->kind() == 7) {
        ITEM->terrain_refresh(pos);
        return;
    }
    return;
base:
    vt(ITEM)->d27(&vector4(pos[0] * 0.02f, pos[1] * 0.02f, 1.0f));
    STORE_SIZE();
}
#undef ITEM

// @ 0x49ab80
void widget_mgr::apply_preset_b(int idx, const float* pos)
{
#define ITEM ((WItem*)widgets[idx])
    if (vt(ITEM)->kind() == 7)
        ITEM->terrain_refresh_mesh(pos);
#undef ITEM
}

// @ 0x49abc0
// The text is laid out by the widget's own font (+0x124) or by the global one (0x5e2934); the 0xf8 / 0xf0 bounds are
// the same either way (the original duplicates the argument pushes per branch).
void MissionUiText::layout_text_to(const tstring* text, const float* pos)
{
    float out[2];
    vector4 p;
    p.z = 0.0f;
    int arg = -1;
    if (f124) {
        p.x = pos[0];
        p.y = pos[1];
        ((Font*)f124)->layout_text(out, text, &p, &f140, tracks[4], (int)&arg, &screen[2], &screen[0], 0);
    } else {
        p.x = pos[0];
        p.y = pos[1];
        g_font->layout_text(out, text, &p, &f140, tracks[4], (int)&arg, &screen[2], &screen[0], 0);
    }
    if (b122) {
        int one = 1;
        *(int*)((char*)tracks[4] + 0x114) = one;
        *(int*)((char*)tracks[4] + 0x140) = one;
    }
}

// @ 0x49aca0
char DropDown::on_event(unsigned ev)
{
    char r = 0;
    if (ev == 0x10) {
        r = 1;
        f180 = 1;
        if (rows_first != 0) {
            int n = (int)(rows_last - rows_first);
            if (n != 0) {
                ::play_sound(&g_sound);
                NodeA* o = create();
                f188 = o;
                o->f158 = (int)this;
            }
        }
    } else if (ev == 0x10000) {
        f180 = 0;
    }
    set_key_flags(ev);
    refresh();
    return r;
}
