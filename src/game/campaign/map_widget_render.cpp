#include "game/campaign/map_widget_render.h"
#include "game/ui/view_rect.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/ui/ui_globals.h"      // UI / core globals
#include "game/ui/gui_manager.h"      // g_gui (0x5d7080)

// @ 0x4ed6f0
void MapWidget::render(rgl::scene* sc, float* v, float* b, float* c)
{
    if (f354) {
        f354 = 0;
        return;
    }
    scene3d.vp_local.X = grid_count((float)g_gui.screen_w * position.x);
    scene3d.vp_local.Y = grid_count((1.0f / g_gui.aspect) * (g_gui.aspect - (position.y + rect_max.y)) * (float)g_gui.screen_h);
    scene3d.vp_local.Width = grid_count((float)g_gui.screen_w * rect_max.x);
    scene3d.vp_local.Height = grid_count((1.0f / g_gui.aspect) * (float)g_gui.screen_h * rect_max.y);
    scene3d.fog = &world->view;
    scene3d.vp_local.MinZ = 0.0f;
    scene3d.vp_local.MaxZ = 1.0f;
    scene3d.lights_active = 1;
    scene3d.list = (const rgl::ptr_list&)world->lights;   // 0x43f0a0 (ptr_list::operator=)
    world->render_scene(&scene3d, true);   // 0x429890
    v[0] += 1.0f;
    MissionUi::render(sc, v, b, c);   // 0x488720
}
