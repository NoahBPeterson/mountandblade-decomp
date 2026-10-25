#include "./facegen_win.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/frustum.h"
#include "../res_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4f1460
void facegen_win::init()
{
    int i;
    for (i = 0; i < 3; i++)
        *(int*)((char*)this + 0x54 + i * 4) = rand() % 0x1b5781;
    ((Terrain*)(void*)&g_terrain_gen)->reset();   // 0x4647e0

    *(float*)((char*)this + 0x3c) = 150.0f;
    *(float*)((char*)this + 0x40) = 150.0f;
    *(float*)((char*)this + 0x44) = 100.0f;
    *(float*)((char*)this + 0x48) = 0.5f;
    *(int*)((char*)this + 0x50) = 30;
    *(float*)((char*)this + 0x4c) = 0.8f;
    *(int*)((char*)this + 0x60) = 3;

    ((gui_manager*)(void*)g_gui)->reset();   // 0x42d1b0

    UiWindow* w = (UiWindow*)new_container(0x190, vtbl_592848);
    if (w)
        w->f188 = 0;
    board = w;
    { tvec2 p = { 0.0f, 0.0f }; w->set_pos((const float*)&p); }
    { tvec2 s = { 1.0f, 0.75f }; w->set_scale((const float*)&s); }
    ((NodeVec*)(void*)g_gui)->push_back((NodeA* const&)board);   // 0x49b120

    UiContainer* c = (UiContainer*)new_container(0x188, 0);
    panel = c;
    { tvec2 p = { 0.5f, 0.0f }; c->set_pos((const float*)&p); }
    { tvec2 s = { 0.5f, 0.75f }; c->set_scale((const float*)&s); }
    c->set_mesh(0, load_mesh("facegen_board", 1.0f));
    { tvec2 p = { 0.017f, 0.61f }; c->set_text_at(tstring(""), (const float*)&p); }
    { tvec2 s = { 0.02f, 0.02f }; c->set_size((const float*)&s); }

    w218 = make_medium(panel, &tstring("Shade occluded:"), 0.24f, 0.62f, 0.7f, 0.3f,
                       &tstring("medium_button_down"), &tstring("medium_button"));
    w220 = make_medium(panel, &tstring("Place river:"), 0.24f, 0.54f, 0.7f, 0.3f,
                       &tstring("medium_button_down"), &tstring("medium_button"));
    w21c = make_medium(panel, &tstring("Disable grass:"), 0.24f, 0.58f, 0.7f, 0.3f,
                       &tstring("medium_button_down"), &tstring("medium_button"));
    w224 = make_medium(panel, &tstring("Deep water:"), 0.24f, 0.50f, 0.7f, 0.3f,
                       &tstring("medium_button_down"), &tstring("medium_button"));

    for (i = 0; i < 3; i++) {
        seeds[i] = (UiSlider*)make_slider((MissionUi*)panel, 0.72f, 0.22f + (float)i * 0.055f, 1.0f);
        seeds[i]->set_text(&tstring(""), tstring("randomize"));
    }

    for (i = 0; i < 6; i++) {
        sliders[i] = (UiSlider*)make_slider((MissionUi*)panel, 0.1f, 0.83f - (float)i * 0.088f, 1.0f);
        sliders[i]->set_text(&tstring(""), tstring(""));
    }

    region_type = (UiWidget*)make_dropdown((MissionUi*)panel, &tstring("region type"), 0.79f, 0.9f);
    region_detail = (UiWidget*)make_dropdown((MissionUi*)panel, &tstring("region detail"), 0.79f, 0.9f);
    (void)region_detail;

    key = make_short(panel, &tstring("key"), 0.79f, 0.697f, 0.1f, 0.5f, &tstring(""),
                     &tstring(""));
    copy = make_short(panel, &tstring("Copy"), 0.616f, 0.68f, 3.0f, 0.3f, &tstring(""),
                      &tstring(""));
    paste = make_short(panel, &tstring("Paste"), 0.616f, 0.68f, 3.0f, 0.3f, &tstring(""),
                       &tstring(""));

    generate = make_medium(panel, &tstring("Generate Terrain"), 0.571f, 0.035f, 0.07f, 3.0f,
                           &tstring(""), &tstring(""));
    close = make_medium(panel, &tstring("Close"), 0.07f, 0.028f, 0.33f, 3.0f, &tstring(""),
                        &tstring(""));
}

// @ 0x4f25a0
#pragma intrinsic(tan)
void rgl::frustum::init(float fov, float aspect, float znear, float zfar)
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
