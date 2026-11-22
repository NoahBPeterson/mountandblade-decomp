// font.h: the game font (global g_font 0x5e2934): glyph metrics and text layout.
//   advance 0x43e190 / measure 0x43e200: src/game/agent3/agent_combat.cpp (the glyph tables below are what they read)
//   layout_text 0x4520d0 (ret 0x24): called by src/game/ui/widgets/widget_manager.cpp (MissionUi::layout_text_to 0x49abc0)
// Offsets are the ones read by those functions; the rest is padding.
#ifndef GAME_FONT_H
#define GAME_FONT_H
#include <stddef.h>
#include <vector>
#include "engine/core/tstring.h"
#include "engine/math/vec.h"                 // V2 (layout_map's extent)

namespace rgl { struct vector4; struct mesh; }
struct NotePage;                                     // game/ui/widgets/note_page.h: the page a map-text link opens

#include "engine/core/glyph_hash.h"                           // Glyph, GlyphMap (stdext::hash_map<int, Glyph>)
struct Font {
    int p00, p04;                                    // +0x00 (left uninitialised by Font::Font 0x4d99d0)
    int width;                                       // +0x08 texture width  (0x4d9aa0: 512, attribute "width")
    int height;                                      // +0x0c (512, "height")
    int padding;                                     // +0x10 (0, "padding")
    int font_size;                                   // +0x14 (75, "font_size")
    float x_ratio;                                   // +0x18 font_size / width
    float y_ratio;                                   // +0x1c font_size / height
    float font_scale;                                // +0x20 "font_scale" / 100
    float line_spacing;                              // +0x24 "line_spacing" / 100
    Glyph dflt;                                      // +0x28 (returned for unknown characters) (0x43e190)
    GlyphMap map;                                    // +0x44 the glyphs of the characters at or above 0x200 (0x28 bytes: Font::Font 0x4d99d0 builds it)
    Glyph ascii[0x200];                              // +0x6c (0x43e190)
    Font();                                          // 0x4d99d0
    void load(const tstring& filename);                 // 0x4d9aa0 (reads the XML font definition)
    __declspec(noinline) float advance(int ch);      // 0x43e190
    void draw_glyph(int ch, const rgl::vector4* pos, const vec2* scale, rgl::mesh* m, const unsigned* color);   // 0x44f5d0 (ret 0x14) (agent4/agent_logic.cpp: one glyph quad added to a mesh in *color)
    rgl::vector4* layout_clear(rgl::vector4* out_extent, const tstring* text, const rgl::vector4* pos, const float* scale, rgl::mesh* m,
                               const int* a, const float* b, const float* c, unsigned flags);   // 0x452bb0 (ret 0x24: clears the mesh, then layout_text; a = the colour word pointer)
    float measure(const int* text, int len, int* start, int* end);   // 0x43e200 (ret 0x10)
    // 0x4fd700 (src/game/ui/hud/agent_hud.cpp): the map-text row renderer, `this` in ECX, ret 0x44 (17 stack words: the hidden
    // result pointer of the returned extent first, the last five are the widget's vectors)
    V2 layout_map(const tstring* text, const rgl::vector4* pos, const float* scale, rgl::mesh* m, const void* col_text, const void* col_link,
                     const void* col_hot, const float* xmin, const float* xmax, unsigned flags, int cur, std::vector<rgl::vector4>* rects_a, std::vector<rgl::vector4>* rects_b,
                     std::vector<int>* links, std::vector<NotePage>* pages, std::vector<int>* args);
    void layout_text(float* out, const tstring* text, const rgl::vector4* pos_in, const float* scale_in, rgl::mesh* m,
                     const unsigned* color, const float* xmin, const float* xmax, unsigned flags);   // 0x4520d0 (ret 0x24; color: the colour word)
};

extern Font* g_font; // 0x5e2934
#endif // GAME_FONT_H
