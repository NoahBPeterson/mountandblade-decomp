#include "./render_shape.h"
#include "../view_rect.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4ed6f0
void GuiCtx::render_shape_frame(int a, vector4* v, int b, int c)
{
    if (f354) {
        f354 = 0;
        return;
    }
    cell_x0 = grid_count((float)g_view_z * f0e0);
    cell_y0 = grid_count((g_549cdc / g_div) * (g_div - (f0e4 + f0f4)) * (float)g_view_w);
    cell_x1 = grid_count((float)g_view_z * f0f0);
    cell_y1 = grid_count((g_549cdc / g_div) * (float)g_view_w * f0f4);
    f32c = (int)f178 + 0xd8;
    f18c = 0;
    f190 = 1.0f;
    f2dc = 1;
    *(std::vector<int>*)&f310 = *(const std::vector<int>*)&f178->fb8;   // 0x43f0a0 (vector<int>::operator=)
    ((rgl::world*)f178)->render_scene((rgl::scene*)&cell_x0, true);   // 0x429890
    v->x += 1.0f;
    ((MissionUi*)(void*)this)->render((rgl::scene*)a, (float*)v, b, c);   // 0x488720
}
