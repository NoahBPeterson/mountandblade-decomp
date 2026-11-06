#include "../../include/image_addrs.h"
#include "./facegen_win.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/frustum.h"
#include "../res_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../terrain_page.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../std_vec_insert_n.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../terrain_a/terrain.h"      // Terrain, g_terrain
#include "../config/widget_mgr.h"      // DropDown (append 0x493400)
#include "../config/ui_widgets_c.h"      // UiE
#include "./cs_common.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)
#include "../terrain_b/cell_grid.h"      // CellGrid (ctor 0x478ad0, dtor 0x475d50, spawn_wind 0x478f30): the one definition

// the scene entity's slot 4 (+0x10), called once its mesh list is filled
struct EntityV {
    virtual void s00(); virtual void s04(); virtual void s08(); virtual void s0c();
    virtual void refresh();                                 // +0x10
};

// a "medium"/"short" button of the board panel (0x4dcc50 with the two skin names and the 3.7 / 3.0 text scale): the label
// is the caller's temporary, the two skin names this helper's, so they die first
static inline MissionUi* medium_button(MissionUi* parent, const tstring& label, float x, float y, float h)
{
    return (MissionUi*)new_sized_button(parent, label, x, y, h, "medium_button", "medium_button_down", 3.7f, 0.072f, 0.085f);
}
static inline MissionUi* short_button(MissionUi* parent, const tstring& label, float x, float y, float h)
{
    return (MissionUi*)new_sized_button(parent, label, x, y, h, "short_button", "short_button_down", 3.0f, 0.072f, 0.085f);
}
static __forceinline mesh* board_mesh(const char* name)
{
    tstring n(name);
    return g_resmgr.get_object(&n)->clone();   // 0x419540, 0x41c6d0
}

// the entity's mesh list: push_back expanded in place (a file that sees std_vec_push_back.h would call 0x440b30)
struct BoardMeshVec : std::vector<int> {
    __forceinline void add(const int& x)
    {
        if (size() < capacity())
            _Mylast = _Ufill(_Mylast, 1, x);
        else
            _Insert_n(end(), 1, x);
    }
};
// the scene bounds, passed by address as temporaries (0x478ad0)
struct board_pair { float x, y; board_pair(float a, float b) : x(a), y(b) {} };
// the board's child list (ContainerBase::kids): a real inlined push_back; the slow path is 0x4927d0
static __forceinline void add_kid(ContainerBase* c, MissionUi* w)
{
    ((std::vector<SceneObject*>&)c->kids).push_back((SceneObject*)w);
}

// @ 0x4f1460
void TerrainPage::init_screen()
{
    int i;
    for (i = 0; i < 3; i++)
        layer_bits[i] = rand() % 0x1b5781;
    g_terrain.reset();   // 0x4647e0

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

    g_gui.reset();   // 0x42d1b0

    NodeA* w = new NodeA(0);
    board = w;
    { tvec2 p = { 0.0f, 0.0f }; board->move_a((const float*)&p); }
    { tvec2 s = { 1.0f, 0.75f }; board->set_xy((const float*)&s); }
    {
        MissionUi* root = board;                     // (the original passes a stack copy)
        g_gui.dirty = 1;
        g_gui.objs.push_back((MissionUi* const&)root);   // 0x49b120
    }

    panel = new text_box;
    { tvec2 p = { 0.5f, 0.0f }; panel->move_a((const float*)&p); }
    { tvec2 s = { 0.5f, 0.75f }; panel->set_xy((const float*)&s); }
    {
        mesh* m = board_mesh("facegen_board");
        panel->set_mesh(0, m);                      // +0x7c
    }
    {
        tstring e("");
        char out[8];
        panel->set_text((float*)out, &(e));                    // +0xa4
    }
    { tvec2 s = { 0.02f, 0.02f }; panel->set_size((const float*)&s); }
    add_kid(board, panel);

    { tstring s("Shade occluded:"); tvec2 p = { 0.24f, 0.62f }; panel->layout_text_to(&s, (const float*)&p); }
    water_box = medium_button(panel, "disabled", 0.36f, 0.62f, 0.03f);
    *(int*)((char*)water_box + 0x184) = 1;
    { tstring s("Place river:"); tvec2 p = { 0.01f, 0.62f }; panel->layout_text_to(&s, (const float*)&p); }
    terrain_box = medium_button(panel, "place river", 0.1f, 0.62f, 0.03f);
    *(int*)((char*)terrain_box + 0x184) = 1;
    { tstring s("Disable grass:"); tvec2 p = { 0.24f, 0.58f }; panel->layout_text_to(&s, (const float*)&p); }
    river_box = medium_button(panel, "3dgrass", 0.36f, 0.58f, 0.03f);
    *(int*)((char*)river_box + 0x184) = 1;
    { tstring s("Deep water:"); tvec2 p = { 0.01f, 0.58f }; panel->layout_text_to(&s, (const float*)&p); }
    terraingen_box = medium_button(panel, "deep water", 0.1f, 0.58f, 0.03f);
    *(int*)((char*)terraingen_box + 0x184) = 1;

    // the three seeds: a caption, a "randomize" button and an edit box (UiE, inlined constructor)
    for (i = 0; i < 3; i++) {
        float t = (float)i * 0.04f;
        float ty = t + 0.07f;
        { tstring s(i == 0 ? "terrain seed" : i == 1 ? "river seed" : "flora seed"); tvec2 p = { 0.03f, ty }; panel->layout_text_to(&s, (const float*)&p); }
        layer_boxes[i] = medium_button(panel, "randomize", 0.38f, t + 0.06f, 0.03f);

        bit_fields[i] = new UiE(0);                 // the inlined constructor (config/ui_widgets_c.h)
        { tvec2 p = { 0.1f, t + 0.06f }; bit_fields[i]->move_a((const float*)&p); }
        { tvec2 s = { 0.225f, 0.025f }; bit_fields[i]->set_xy((const float*)&s); }
        mesh* up = board_mesh("party_button");
        mesh* down = board_mesh("party_button_down");
        up->scale(0.41666669f);                     // 0x44f0c0
        down->scale(0.41666669f);
        bit_fields[i]->set_track(2, up);                 // +0x80
        bit_fields[i]->set_track(1, down);
        panel->add_child(bit_fields[i]);                 // +0xc8
    }

    // the six parameter sliders, each with its caption
    char name[64];                                  // function scope: the buffer below keeps a separate slot
    for (i = 0; i < 6; i++) {
        switch (i) {
        case 0: strcpy(name, "Size x"); break;
        case 1: strcpy(name, "Size y"); break;
        case 2: strcpy(name, "Hill height"); break;
        case 3: strcpy(name, "Valley"); break;
        case 4: strcpy(name, "Ruggedness"); break;
        case 5: strcpy(name, "Vegetation"); break;
        default: strcpy(name, "unknown"); break;
        }
        { tstring s(name); float t = (float)i * 0.04f; tvec2 p = { 0.05f, t + 0.2f }; panel->layout_text_to(&s, (const float*)&p); }
        sliders[i] = (MissionUi*)make_slider((MissionUi*)panel, 0.23f, (float)i * 0.04f + 0.19f, 1.0f);
    }

    regen_btn = medium_button(panel, "Generate Terrain", 0.07f, 0.02f, 0.035f);
    back_btn = medium_button(panel, "Close", 0.33f, 0.01f, 0.028f);
    {
        tstring k("key");
        hexbox = (MissionUi*)make_hint((ContainerBase*)panel, k, 0.0f, 0.72f, 0.5f, 0.02f);   // 0x4dada0
    }
    { tvec2 s = { 0.02f, 0.02f }; hexbox->set_size((const float*)&s); }
    copy_btn = short_button(panel, "Copy", 0.1f, 0.685f, 0.025f);
    paste_btn = short_button(panel, "Paste", 0.25f, 0.685f, 0.025f);
    { tstring s("Terrain:"); tvec2 p = { 0.03f, 0.49f }; panel->layout_text_to(&s, (const float*)&p); }
    { tstring s("Polygon Size:"); tvec2 p = { 0.03f, 0.54f }; panel->layout_text_to(&s, (const float*)&p); }
    {
        tstring n("region type");
        type_box = (MissionUi*)make_dropdown((MissionUi*)panel, &n, 0.23f, 0.49f);
    }
    for (i = 0; i < 8; i++) {
        char buf[64];
        region_name(g_region_types[i], buf);
        tstring s(buf);
        ((DropDown*)type_box)->append(&s);                    // +0xc8: the drop-down's add-item
    }
    {
        tstring n("region detail");
        size_box = (MissionUi*)make_dropdown((MissionUi*)panel, &n, 0.23f, 0.54f);
    }
    { tstring s("2 meters"); ((DropDown*)size_box)->append(&s); }
    { tstring s("3 meters"); ((DropDown*)size_box)->append(&s); }
    { tstring s("4 meters"); ((DropDown*)size_box)->append(&s); }
    { tstring s("5 meters"); ((DropDown*)size_box)->append(&s); }

    // the 3D preview: the view widget, its scene and camera
    view = new MapWidget;                           // 0x4ed630
    world = new rgl::world((const float*)&board_pair(-200.0f, -200.0f), (const float*)&board_pair(200.0f, 200.0f),
                           &(const int&)6, &(const int&)6, &(const int&)0);   // 0x478ad0
    world->set_fog(&(const float&)500.0f, &(const float&)1000.0f, &(const int&)0xff405060);   // 0x488cd0
    view->world = world;
    { tvec2 p = { 0.0f, 0.0f }; view->move_a((float*)(const float*)&p); }
    { tvec2 s = { 0.5f, 0.75f }; view->set_xy((const float*)&s); }
    view->scene3d.flags |= 0x100;
    view->scene3d.f190 = 0xff111133;
    cam.init(0.69813174f, 1.5f, 2.0f, 1000.0f);    // 0x4f25a0 (40 degrees)
    view->scene3d.fr = cam;
    add_kid(board, view);

    EntityV* ent = (EntityV*)((rgl::world*)(void*)world)->create_entity(9);   // 0x428440
    {
        tstring n("prtcl_drop");
        mesh* m = g_resmgr.get_object(&n)->clone();
        ((BoardMeshVec*)((char*)ent + 0x118))->add((int)m);   // _Insert_n 0x410610
        ent->refresh();
    }

    {
        v4raw o;
        o.x = 0.0f;
        o.y = 0.0f;
        o.z = 10.0f;
        cam.m.o = *(rgl::vector4*)&o;         // the camera frame's origin row (cam +0xf8)
    }
    {
        v4raw o;
        o.x = 0.0f;
        o.y = 0.0f;
        o.z = 0.0f;
        view_pos = *(rgl::vector4*)&o;
    }
    yaw = 0.0f;
    pitch = 1.0f;
    zoom = 250.0f;
    refresh_hex();     // 0x4f2960
    update();          // 0x4f2730
}
// @ 0x4f25a0
#pragma intrinsic(tan)
__declspec(noinline) void rgl::frustum::init(float fov, float aspect, float znear, float zfar)   // (called, not inlined, by 0x4f1460)
{
    perspective = 1;
    nearz = znear;
    double t = tan(fov * 0.5f);
    farz = zfar;
    float p = (float)(znear * t);
    left = -p;
    float r = (float)t * znear;
    right = r;
    bottom = -p * aspect;
    top = r * aspect;
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
