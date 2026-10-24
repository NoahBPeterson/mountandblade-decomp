#include "./render_shape.h"

// @ 0x4ed6f0
void Ctx::render_shape_frame(int a, vector4* v, int b, int c)
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
    f310.assign(&f178->fb8);
    f178->render(&cell_x0, 1);
    v->x += 1.0f;
    follow(a, v, b, c);
}
