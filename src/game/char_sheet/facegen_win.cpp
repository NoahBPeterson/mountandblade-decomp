#include "./facegen_win.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/frustum.h"
#include "../res_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../terrain_page.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../int_vec.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "./cs_common.h"      // (listed directly: the resolution audit reads only the source's own includes)

// the scene entity's slot 4 (+0x10), called once its mesh list is filled
struct EntityV {
    virtual void s00(); virtual void s04(); virtual void s08(); virtual void s0c();
    virtual void refresh();                                 // +0x10
};

// a "medium"/"short" button of the board panel (0x4dcc50 with the two skin names and the 3.7 / 3.0 text scale);
// the three strings are built label first and die in reverse order, as the original's named temporaries do
static __forceinline UiWidget* board_button(UiWidget* parent, const char* label, float x, float y, float h,
                                            const char* down, const char* up, float scale)
{
    tstring a(label);
    tstring d(down);
    tstring u(up);
    return (UiWidget*)new_sized_button((MissionUi*)parent, a, x, y, h, u, d, scale, 0.072f, 0.085f);
}
static __forceinline void board_text(UiWidget* parent, const char* text, float x, float y)
{
    tstring s(text);
    tvec2 p = { x, y };
    parent->set_text_at(s, (const float*)&p);   // +0xac
}
static __forceinline mesh* board_mesh(const char* name)
{
    tstring n(name);
    return g_alloc.get_object((const rgl::string*)&n)->clone();   // 0x419540, 0x41c6d0
}

// @ 0x4f1460
void facegen_win::init()
{
    int i;
    for (i = 0; i < 3; i++)
        layer_bits[i] = rand() % 0x1b5781;
    ((Terrain*)(void*)&g_terrain_gen)->reset();   // 0x4647e0

    water_on = 0;
    river_on = 0;
    terrain_on = 0;
    terraingen_on = 0;
    camera_x = 150.0f;
    camera_y = 150.0f;
    camera_z = 100.0f;
    fog = 0.5f;
    water_level = 30;
    water_height = 0.8f;
    terrain_type = 3;
    map_size = 0;

    ((gui_manager*)(void*)g_gui)->reset();   // 0x42d1b0

    UiWidget* w = (UiWidget*)new_container(0x190, vtbl_592848);
    if (w)
        *((char*)w + 0x188) = 0;
    board = w;
    { tvec2 p = { 0.0f, 0.0f }; board->set_pos((const float*)&p); }
    { tvec2 s = { 1.0f, 0.75f }; board->set_scale((const float*)&s); }
    {
        UiWidget* root = board;                     // (the original passes a stack copy)
        g_gui_dirty = 1;
        ((NodeVec*)(void*)g_gui)->push_back((NodeA* const&)root);   // 0x49b120
    }

    panel = (UiWidget*)new_container(0x188, vtbl_597050);
    { tvec2 p = { 0.5f, 0.0f }; panel->set_pos((const float*)&p); }
    { tvec2 s = { 0.5f, 0.75f }; panel->set_scale((const float*)&s); }
    {
        mesh* m = board_mesh("facegen_board");
        panel->set_mesh(0, m);                      // +0x7c
    }
    {
        tstring e("");
        char out[8];
        panel->set_text(out, e);                    // +0xa4
    }
    { tvec2 s = { 0.02f, 0.02f }; panel->set_size((const float*)&s); }
    push_child((ChildList*)((char*)board + 0x178), panel);

    board_text(panel, "Shade occluded:", 0.24f, 0.62f);
    w218 = board_button(panel, "disabled", 0.36f, 0.62f, 0.03f, "medium_button_down", "medium_button", 3.7f);
    *(int*)((char*)w218 + 0x184) = 1;
    board_text(panel, "Place river:", 0.01f, 0.62f);
    w220 = board_button(panel, "place river", 0.1f, 0.62f, 0.03f, "medium_button_down", "medium_button", 3.7f);
    *(int*)((char*)w220 + 0x184) = 1;
    board_text(panel, "Disable grass:", 0.24f, 0.58f);
    w21c = board_button(panel, "3dgrass", 0.36f, 0.58f, 0.03f, "medium_button_down", "medium_button", 3.7f);
    *(int*)((char*)w21c + 0x184) = 1;
    board_text(panel, "Deep water:", 0.01f, 0.58f);
    w224 = board_button(panel, "deep water", 0.1f, 0.58f, 0.03f, "medium_button_down", "medium_button", 3.7f);
    *(int*)((char*)w224 + 0x184) = 1;

    // the three seeds: a caption, a "randomize" button and an edit box (UiE, inlined constructor)
    for (i = 0; i < 3; i++) {
        float t = (float)i * 0.04f;
        board_text(panel, i == 0 ? "terrain seed" : i == 1 ? "river seed" : "flora seed", 0.03f, t + 0.07f);
        float y = t + 0.06f;
        randomize[i] = board_button(panel, "randomize", 0.38f, y, 0.03f, "medium_button_down", "medium_button", 3.7f);

        char* e = (char*)new_widget_base(0x188);    // 0x48ae20
        if (e) {
            *(int*)(e + 0x178) = 0;
            *(e + 0x121) = 0;
            *(void**)e = vtbl_592208;
            *(e + 0x180) = 0;
            *(e + 0x181) = 0;
            *(int*)(e + 0x184) = 0x1c;
        }
        seeds[i] = (UiWidget*)e;
        { tvec2 p = { 0.1f, y }; seeds[i]->set_pos((const float*)&p); }
        { tvec2 s = { 0.225f, 0.025f }; seeds[i]->set_scale((const float*)&s); }
        mesh* up = board_mesh("party_button");
        mesh* down = board_mesh("party_button_down");
        up->scale(0.41666669f);                     // 0x44f0c0
        down->scale(0.41666669f);
        seeds[i]->set_track(2, up);                 // +0x80
        seeds[i]->set_track(1, down);
        panel->add_child(seeds[i]);                 // +0xc8
    }

    // the six parameter sliders, each with its caption
    for (i = 0; i < 6; i++) {
        char name[32];
        switch (i) {
        case 0: strcpy(name, "Size x"); break;
        case 1: strcpy(name, "Size y"); break;
        case 2: strcpy(name, "Hill height"); break;
        case 3: strcpy(name, "Valley"); break;
        case 4: strcpy(name, "Ruggedness"); break;
        case 5: strcpy(name, "Vegetation"); break;
        default: strcpy(name, "unknown"); break;
        }
        float t = (float)i * 0.04f;
        board_text(panel, name, 0.05f, t + 0.2f);
        sliders[i] = (UiWidget*)make_slider((MissionUi*)panel, 0.23f, t + 0.19f, 1.0f);
    }

    generate = board_button(panel, "Generate Terrain", 0.07f, 0.02f, 0.035f, "medium_button_down", "medium_button", 3.7f);
    close = board_button(panel, "Close", 0.33f, 0.01f, 0.028f, "medium_button_down", "medium_button", 3.7f);
    {
        tstring k("key");
        key = (UiWidget*)make_hint((ContainerBase*)panel, k, 0.0f, 0.72f, 0.5f, 0.02f);   // 0x4dada0
    }
    { tvec2 s = { 0.02f, 0.02f }; key->set_size((const float*)&s); }
    copy = board_button(panel, "Copy", 0.1f, 0.685f, 0.025f, "short_button_down", "short_button", 3.0f);
    paste = board_button(panel, "Paste", 0.25f, 0.685f, 0.025f, "short_button_down", "short_button", 3.0f);
    board_text(panel, "Terrain:", 0.03f, 0.49f);
    board_text(panel, "Polygon Size:", 0.03f, 0.54f);

    {
        tstring n("region type");
        region_type = (UiWidget*)make_dropdown((MissionUi*)panel, &n, 0.23f, 0.49f);
    }
    for (int* r = g_region_names; r < g_region_names + 8; r++) {
        char buf[64];
        region_name(*r, buf);
        tstring s(buf);
        region_type->add_child(&s);                 // +0xc8: the drop-down's add-item
    }
    {
        tstring n("region detail");
        region_detail = (UiWidget*)make_dropdown((MissionUi*)panel, &n, 0.23f, 0.54f);
    }
    { tstring s("2 meters"); region_detail->add_child(&s); }
    { tstring s("3 meters"); region_detail->add_child(&s); }
    { tstring s("4 meters"); region_detail->add_child(&s); }
    { tstring s("5 meters"); region_detail->add_child(&s); }

    // the 3D preview: the view widget, its scene and camera
    view = new MapWidget;                           // 0x4ed630
    {
        tvec2 lo = { -200.0f, -200.0f };
        tvec2 hi = { 200.0f, 200.0f };
        int cells = 6;
        int cells2 = 6;
        int flag = 0;
        void* p = operator new(0x344);
        scene = p ? new (p) CellGrid((const float*)&lo, (const float*)&hi, &cells, &cells2, &flag) : 0;   // 0x478ad0
    }
    {
        float start = 500.0f;
        float end = 1000.0f;
        unsigned color = 0xff405060;
        ((MissionUi*)(void*)scene)->set_fog(&start, &end, (const float*)&color);   // 0x488cd0
    }
    view->world = scene;
    { tvec2 p = { 0.0f, 0.0f }; view->set_pos((const float*)&p); }
    { tvec2 s = { 0.5f, 0.75f }; view->set_scale((const float*)&s); }
    view->flags |= 0x100;
    view->clear_color = 0xff111133;
    cam.init(0.69813174f, 1.5f, 2.0f, 1000.0f);    // 0x4f25a0 (40 degrees)
    view->view = cam;
    push_child((ChildList*)((char*)board + 0x178), view);

    EntityV* ent = (EntityV*)((rgl::world*)(void*)scene)->create_entity(9);   // 0x428440
    {
        tstring n("prtcl_drop");
        mesh* m = g_alloc.get_object((const rgl::string*)&n)->clone();
        ((IntVec*)((char*)ent + 0x118))->push_back_mesh((int)m);   // _Insert_n 0x410610
        ent->refresh();
    }

    {
        v4raw o;
        o.x = 0.0f;
        o.y = 0.0f;
        o.z = 10.0f;
        *(v4raw*)((char*)this + 0x168) = o;         // the camera frame's origin row (cam +0xf8)
    }
    {
        v4raw o;
        o.x = 0.0f;
        o.y = 0.0f;
        o.z = 0.0f;
        view_pos = o;
    }
    yaw = 0.0f;
    pitch = 1.0f;
    zoom = 250.0f;
    ((TerrainPage*)(void*)this)->refresh_hex();     // 0x4f2960
    ((TerrainPage*)(void*)this)->update();          // 0x4f2730
}
// @ 0x4f25a0
#pragma intrinsic(tan)
__declspec(noinline) void rgl::frustum::init(float fov, float aspect, float znear, float zfar)   // (called, not inlined, by 0x4f1460)
{
    perspective = 1;
    nearz = znear;
    double t = tan(fov * 0.5f);
    farz = zfar;
    double p = t * znear;
    left = (float)-p;
    znear = (float)(znear * t);
    right = znear;
    bottom = (float)-(p * aspect);
    top = znear * aspect;
    build_planes();
}
#pragma function(tan)

// @ 0x4f2610
void region_name(int i, char* out)
{
    switch (i) {
    case 0: sprintf(out, "Water"); break;
    case 1: sprintf(out, "Mountain"); break;
    case 2: sprintf(out, "Steppe"); break;
    case 3: sprintf(out, "Plain"); break;
    case 4: sprintf(out, "Snow"); break;
    case 5: sprintf(out, "Desert"); break;
    case 8: sprintf(out, "River"); break;
    case 9: sprintf(out, "Mountain Forest"); break;
    case 10: sprintf(out, "Steppe Forest"); break;
    case 11: sprintf(out, "Forest"); break;
    case 12: sprintf(out, "Snow Forest"); break;
    }
}
