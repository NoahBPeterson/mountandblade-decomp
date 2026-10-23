// font.h: the game font (global g_font 0x5e2934): glyph metrics and text layout.
//   advance 0x43e190 / measure 0x43e200: src/game/agent3/agent_combat.cpp (the glyph tables below are what they read)
//   layout_text 0x4520d0 (ret 0x24): called by src/game/config/widget_mgr.cpp (MissionUi::layout_text_to 0x49abc0)
// Offsets are the ones read by those functions; the rest is padding.
#ifndef GAME_FONT_H
#define GAME_FONT_H
#include "./tstring.h"

namespace rgl { struct vector4; }

// ---- font glyph metrics -----------------------------------------------------------------------------------
struct Glyph {                                       // 0x1c bytes: key + 6 floats (std::map<int, glyph> value_type)
    int key;
    float f4, f8, fc, f10, f14, f18;
};
struct GlyphNode { GlyphNode *left, *parent, *right; Glyph val; };
struct GlyphMap {                                    // std::map<int, glyph> (comparator, head, size)
    int pad0[2];
    GlyphNode* head;                                 // +8 (0x43e190)
    void lookup(GlyphNode** out, const int* key);    // 0x43b210 (lower_bound/find with an out iterator)
};
struct Font {
    char pad0[0x28];
    Glyph dflt;                                      // +0x28 (returned for unknown characters) (0x43e190)
    GlyphMap map;                                    // +0x44
    char pad50[0x6c - 0x50];
    Glyph ascii[0x200];                              // +0x6c (0x43e190)
    __declspec(noinline) float advance(int ch);      // 0x43e190
    float measure(const int* text, int len, int* start, int* end);   // 0x43e200 (ret 0x10)
    void layout_text(float* out, const tstring* text, const rgl::vector4* pos_in, const float* scale_in, void* m,
                     int arg, const float* xmin, const float* xmax, unsigned flags);   // 0x4520d0 (ret 0x24)
};

#endif // GAME_FONT_H
