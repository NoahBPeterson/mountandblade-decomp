//-----------------------------------------------------------------------------
// File: widget_mgr.cpp  (0x499830..0x49ad20)
//
// Desc: the editor window's widget manager: factory methods for the medium button, the scene (xyz) buttons, the
//       progress bar, the drop-down button and the party button (each builds a widget, fills its mesh slots from
//       the resource manager, hooks it into the scene container and appends it to the manager's vector), the
//       preset placement helpers (rescale a placed widget for a new size), and MissionUi's text layout wrapper.
//       Names provisional. Functions in ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP.
//
// Boundary: DropDown::key (0x49aca0) calls DropDown::create (0x49ad20) forward, so the object ends before
// 0x49ad20 (see MIGRATION.md). The start (0x499830) is a class change only (obj_tbl|widget_mgr).
//-----------------------------------------------------------------------------
#include "./widget_mgr.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../font.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/resource.h"      // resmgr g_resmgr (0x5d6fa0)
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)

// @ 0x499830
int widget_mgr::create_game_button_overlay(int id)
{
    MediumButton* w = new MediumButton;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        w->move_a(&v.x);
    }
    {
        int flags = 0x10;
        w->set148(&flags);
    }

    {
        rgl::mesh* m;
        {
            rgl::string n("medium_button");
            m = ((rgl::mesh*)(g_resmgr.get_object(&n)))->clone();
        }
        m->scale(0.5555556f);
        w->set_track(2, m);
    }
    {
        rgl::mesh* m;
        {
            rgl::string n("medium_button_down");
            m = ((rgl::mesh*)(g_resmgr.get_object(&n)))->clone();
        }
        m->scale(0.5555556f);
        w->set_track(1, m);
    }

    {
        unsigned c = 0xff007700;
        w->set_target(&c);
    }

    {
        vec2 v;
        v.x = 0.148f;
        v.y = 0.04f;
        w->set_xy(&v.x);
        w->set_text(&v.x, (const tstring*)id);   // TO FIX: id is the text (script_interpreter.cpp passes (int)&text)
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
int widget_mgr::create_image_button_overlay(int a, int b)
{
    XyzButton* w = new XyzButton;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        w->move_a(&v.x);
    }
    {
        int flags = 0x700;
        w->set148(&flags);
    }

    w->set_track(2, g_meshes.placed(a));
    w->set_track(1, g_meshes.placed(b));
    w->update_bounds();

    scene->d50(w);
    widgets.push_back(w);
    return (int)(widgets.size() - 1);
}

// @ 0x499b50
int widget_mgr::create_check_box_overlay(int a, int b)
{
    int i = create_image_button_overlay(a, b);
    ((XyzButton*)widgets[i])->set_flag();
    return i;
}

// @ 0x499b80
int widget_mgr::create_progress_overlay(int lo, int hi)
{
    UiC* w = new UiC;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        w->move_a(&v.x);
    }

    {
        vector4 sc(0.38940807f, 1.0f, 1.0f);

        {
            rgl::string n("progressbar");
            w->set_track(0, ((rgl::mesh*)(g_resmgr.get_object(&n)))->clone());
        }
        ((rgl::mesh*)w->tracks[0])->set_color(sc);

        {
            rgl::string n("handle_hor");
            w->set_track(2, ((rgl::mesh*)(g_resmgr.get_object(&n)))->clone());
        }
        {
            rgl::string n("progressbar_handle");
            w->src = g_resmgr.get_object(&n);
        }
        w->v198 = sc;
    }
    w->update_bounds();

    {
        vec2 h;
        h.x = 0.25f;
        h.y = w->screen[1];
        w->set_xy(&h.x);
    }

    float fa = (float)lo;
    w->f16c = fa;
    w->f170 = (float)hi;
    w->set168(fa);

    scene->d50(w);
    widgets.push_back(w);
    return (int)(widgets.size() - 1);
}

// @ 0x499d90
int widget_mgr::create_combo_button_overlay()
{
    DropDown* w = new DropDown;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        w->move_a(&v.x);
    }

    {
        rgl::string n("button_drop");
        w->set_track(2, ((rgl::mesh*)(g_resmgr.get_object(&n)))->clone());
    }
    {
        vector4 c(1.0f, 0.5f, 1.0f);
        ((rgl::mesh*)w->tracks[2])->set_color(c);
    }

    {
        rgl::string n("button_drop_clicked");
        w->set_track(1, ((rgl::mesh*)(g_resmgr.get_object(&n)))->clone());
    }
    {
        vector4 c(1.0f, 0.5f, 1.0f);
        ((rgl::mesh*)w->tracks[1])->set_color(c);
    }

    {
        rgl::string n("button_drop_hl");
        w->set_track(3, ((rgl::mesh*)(g_resmgr.get_object(&n)))->clone());
    }
    {
        vector4 c(1.0f, 0.5f, 1.0f);
        ((rgl::mesh*)w->tracks[3])->set_color(c);
    }

    {
        rgl::string n("button_drop_child");
        w->m1a4 = g_resmgr.get_object(&n);
    }
    {
        rgl::string n("button_drop_child_clicked");
        w->m1a0 = g_resmgr.get_object(&n);
    }
    {
        rgl::string n("button_drop_child_hl");
        w->m1a8 = g_resmgr.get_object(&n);
    }

    w->update_bounds();

    {
        vec2 p;
        p.x = 0.02f;
        p.y = 0.02f;
        w->set_size(&p.x);
        p.x = 0.02f;
        p.y = 0.013f;
        w->set_offset(&p.x);
        {
            rgl::string n("");
            w->set_text(&p.x, &n);
        }
    }

    scene->d50(w);
    widgets.push_back(w);
    return (int)(widgets.size() - 1);
}

// @ 0x49a0b0
int widget_mgr::create_text_box_overlay()
{
    UiE* w = new UiE(0);

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        w->move_a(&v.x);
    }

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.009f;
        w->set_offset(&v.x);
    }
    {
        vec2 v;
        v.x = 0.02f;
        v.y = 0.02f;
        w->set_size(&v.x);
    }

    {
        rgl::string n("party_button");
        w->set_track(2, ((rgl::mesh*)(g_resmgr.get_object(&n)))->clone());
    }
    {
        vector4 c(0.5f, 0.5f, 1.0f);
        ((rgl::mesh*)w->tracks[2])->set_color(c);
    }

    {
        rgl::string n("party_button_down");
        w->set_track(1, ((rgl::mesh*)(g_resmgr.get_object(&n)))->clone());
    }
    {
        vector4 c(0.5f, 0.5f, 1.0f);
        ((rgl::mesh*)w->tracks[1])->set_color(c);

        w->update_bounds();

        {
            rgl::string n("");
            w->set_text(&c.x, &n);
        }
    }

    scene->d50(w);
    widgets.push_back(w);
    return (int)(widgets.size() - 1);
}

// rescale child mesh `sub` of a widget for a new size; the old size is MissionUi::fD8 / fDC
#define ITEM ((WItem*)widgets[idx])
#define STORE_SIZE() \
    do { *(vec2*)&ITEM->fD8 = *(const vec2*)pos; } while (0)     // the new size, copied as a pair (no float loads)
#define SCALE_BY_OLD(t) \
    ((rgl::mesh*)ITEM->tracks[t])->set_color(vector4(pos[0] / ITEM->fD8, pos[1] / ITEM->fDC, 1.0f))

// a 2-float temporary built from references: the y read keeps its own address (`add eax,0xf4` in the kind 2/3 tail)
struct V2r : vec2 {
    V2r(const float& a, const float& b) { x = a; y = b; }
    operator const float*() const { return &x; }   // the setters take the pair as `const float*`
};
// a 2-float temporary built from values, passed the same way
struct V2t : vec2 {
    V2t(float a, float b) { x = a; y = b; }
    operator const float*() const { return &x; }
};
// @ 0x49a300
void widget_mgr::set_size(int idx, const float* pos)
{
    if (ITEM->kind() == 0 || ITEM->kind() == 1) {
        ITEM->set_size(V2t(pos[0] * 0.02f, pos[1] * 0.02f));
        STORE_SIZE();
    }
    else if (ITEM->kind() == 9) {
        SCALE_BY_OLD(2);
        SCALE_BY_OLD(1);
        ITEM->set_xy(pos);
        { float sz[2]; ITEM->set_text(sz, &ITEM->name); }   // sz: slot 41's text size, never read
        STORE_SIZE();
    }
    else if (ITEM->kind() == 4) {
        SCALE_BY_OLD(0);
        ITEM->update_bounds();
        STORE_SIZE();
    }
    else if (ITEM->kind() == 8) {
        SCALE_BY_OLD(2);
        SCALE_BY_OLD(1);
        ITEM->update_bounds();
        STORE_SIZE();
    }
    else if (ITEM->kind() == 2) {
        SCALE_BY_OLD(0);
        SCALE_BY_OLD(2);
        ITEM->update_bounds();
        ITEM->set_xy(V2r(pos[0] * 0.25f, ITEM->screen[1]));
        STORE_SIZE();
    }
    else if (ITEM->kind() == 3) {
        vector4 c;
        c = vector4(pos[0] * 0.38940808f, pos[1], 1.0f);
        SCALE_BY_OLD(0);
        SCALE_BY_OLD(2);
        ((UiC*)ITEM)->v198 = c;   // 0x49a786: stored after the two scalings
        *(int*)&((UiC*)ITEM)->f194 = (int)0xcb3c57cb;
        ITEM->update_bounds();
        ITEM->set_xy(V2r(pos[0] * 0.25f, ITEM->screen[1]));
        STORE_SIZE();
    }
    else if (ITEM->kind() == 5) {
        SCALE_BY_OLD(2);
        SCALE_BY_OLD(1);
        SCALE_BY_OLD(3);
        ITEM->set_size(V2t(pos[0] * 0.02f, pos[1] * 0.02f));
        ITEM->set_offset(V2t(pos[0] * 0.02f, pos[1] * 0.013f));   // 0x49a956: x is pos[0] * 0.02 (kind 6 passes 0)
        ITEM->update_bounds();
        STORE_SIZE();
        { float sz[2]; ITEM->set_text(sz, &ITEM->name); }   // sz: slot 41's text size, never read
    }
    else if (ITEM->kind() == 6) {
        SCALE_BY_OLD(2);
        SCALE_BY_OLD(1);
        ITEM->set_size(V2t(pos[0] * 0.02f, pos[1] * 0.02f));
        ITEM->set_offset(V2t(0.0f, pos[1] * 0.009f));
        ITEM->update_bounds();
        STORE_SIZE();
    }
    else if (ITEM->kind() == 7) {
        ITEM->terrain_refresh(pos);
    }
}
#undef ITEM

// 0x498420 is HudOuter::scroll_refresh (world_data/mesh_hud.h; its header defines the world_data widget classes over this one's, so only the
// member is declared here, under the defining class)
struct HudOuter {
    void scroll_refresh(rgl::vector4* arg);   // 0x498420 (ret 4)
};

// @ 0x49ab80
void widget_mgr::set_area_size(int idx, const float* pos)
{
#define ITEM ((WItem*)widgets[idx])
    if (ITEM->kind() == 7)
        ((HudOuter*)ITEM)->scroll_refresh((rgl::vector4*)pos);         // 0x498420 (kind 7 is the outer HUD that owns the scrollbar)
#undef ITEM
}

// @ 0x49abc0
// The text is laid out by the widget's own font (+0x124, MissionUi::font) or by the global one (0x5e2934); the 0xf8 / 0xf0 bounds are
// the same either way (the original duplicates the argument pushes per branch).
void MissionUi::layout_text_to(const tstring* text, const float* pos)
{
    float out[2];
    if (font) {
        int arg = -1;
        vector4 p;
        p.z = 0.0f;
        p.x = pos[0];
        p.y = pos[1];
        font->layout_text(out, text, &p, &f140, tracks[4], (const unsigned*)&arg, &screen[2], &screen[0], 0);
    } else {
        int arg = -1;
        vector4 p;
        p.z = 0.0f;
        p.x = pos[0];
        p.y = pos[1];
        g_font->layout_text(out, text, &p, &f140, tracks[4], (const unsigned*)&arg, &screen[2], &screen[0], 0);
    }
    if (b122) {
        int one = 1;
        tracks[4]->slots[0].f4 = one;
        tracks[4]->slots[1].f4 = one;
    }
}

// @ 0x49b4f0 (in the original it sits after DropDown::create, in the object of the text-list code)
// The widget's text rect: lays the name out into the primary mesh (a "ui_font" mesh made on first use) and
// hands the rect back through the hidden result pointer.
void MissionUi::get_xy(float* out)
{
    if (tracks[4] == 0) {
        rgl::mesh* m = new rgl::mesh;
        {
            tstring s("ui_font");
            m->res = g_resmgr.get_material(s);           // the mesh's material
        }
        set_mesh(4, m);
    }
    if (b122) {
        tracks[4]->slots[0].f4 = 1;
        tracks[4]->slots[1].f4 = 1;
    }
    {
        float r[2];
        float pos[4];
        Font* font = this->font ? this->font : g_font;
        pos[2] = 0.0f;
        int arg = -1;
        rgl::mesh* m = tracks[4];
        pos[0] = f138;
        unsigned flags = f148;
        pos[1] = f13c;
        m->clear_geometry();
        font->layout_text(r, &name, (vector4*)pos, &f140, m, (const unsigned*)&arg, &screen[2], &screen[0], flags);
        vec2 rr = *(vec2*)r;
        set_color(&color14c);
        *(vec2*)out = rr;
    }
}

// @ 0x49aca0
char DropDown::key(unsigned ev)
{
    char r = 0;
    if (ev == 0x10) {
        r = 1;
        f180 = 1;
        if (rows.size() != 0) {
            g_gui.play_sound();
            NodeA* o = create();
            f188 = o;
            o->f158 = this;
        }
    } else if (ev == 0x10000) {
        f180 = 0;
    }
    set_key_flags(ev);
    refresh();
    return r;
}
