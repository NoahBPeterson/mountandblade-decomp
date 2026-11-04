#include "./render_shape.h"
#include "../view_rect.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_globals.h"      // UI / core globals

// @ 0x4ed6f0
void MapWidget::render(rgl::scene* sc, float* v, int b, int c)
{
    if (f354) {
        f354 = 0;
        return;
    }
    scene3d.f0 = grid_count((float)g_screen_w * fE0);
    scene3d.f4 = grid_count((g_one / g_div) * (g_div - (fE4 + screen[1])) * (float)g_screen_h);
    scene3d.f8 = grid_count((float)g_screen_w * screen[0]);
    scene3d.fc = grid_count((g_one / g_div) * (float)g_screen_h * screen[1]);
    scene3d.f1b0 = ((rgl::world*)world)->env;
    scene3d.f10 = 0;
    scene3d.f14 = 1.0f;
    scene3d.f160 = 1;
    scene3d.list = (const rgl::ptr_list&)((rgl::world*)world)->lights;   // 0x43f0a0 (ptr_list::operator=)
    ((rgl::world*)world)->render_scene(&scene3d, true);   // 0x429890
    v[0] += 1.0f;
    MissionUi::render(sc, v, b, c);   // 0x488720
}
