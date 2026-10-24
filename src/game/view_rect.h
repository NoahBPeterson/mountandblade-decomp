// view_rect.h: the view-rectangle object whose cell_rect (0x4bd2f0) converts the visible region to two cell corners.
// ONE definition for char_sheet/render_shape (this = &Ctx::f0e0, the view rectangle of the current frame) and
// char_sheet/pick_entity (this = Ctx::f60); both are the same call, cell_rect(int c0[4], int c1[4]).
#ifndef MB_GAME_VIEW_RECT_H
#define MB_GAME_VIEW_RECT_H

struct ViewRect {
    void cell_rect(int c0[4], int c1[4]);       // 0x4bd2f0
};

#endif // MB_GAME_VIEW_RECT_H
