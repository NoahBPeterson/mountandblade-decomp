// font.h: the game font (global g_font 0x5e2934): glyph metrics and text layout.
//   advance 0x43e190 / measure 0x43e200: src/game/agent3/agent_combat.cpp (the glyph tables below are what they read)
//   layout_text 0x4520d0 (ret 0x24): called by src/game/config/widget_mgr.cpp (MissionUi::layout_text_to 0x49abc0)
// Offsets are the ones read by those functions; the rest is padding.
#ifndef GAME_FONT_H
#define GAME_FONT_H
#include <vector>
#include "./tstring.h"

namespace rgl { struct vector4; struct mesh; }

// ---- font glyph metrics -----------------------------------------------------------------------------------
struct Glyph {                                       // 0x1c bytes: key + 6 floats (std::map<int, glyph> value_type)
    union { int key; float f0; };                    // f0: the first metric as src/game/cls_map/cls_map.cpp Font::load 0x4d9aa0 stores it (a float)
    float f4, f8, fc, f10, f14, f18;
};
struct GlyphNode { GlyphNode *left, *parent, *right; Glyph val; };
struct GlyphMap {                                    // std::map<int, glyph> (comparator, head, size)
    int pad0[2];
    GlyphNode* head;                                 // +8 (0x43e190)
    int count;                                       // +0xc  list size, set to 0 by Font::Font 0x4d99d0
    std::vector<int> buckets;                        // +0x10 (0x10 bytes) 9 copies of `head` (0x4d99d0)
    int i20;                                         // +0x20 (1, 0x4d99d0)
    int i24;                                         // +0x24 (1, 0x4d99d0)
    void lower_bound(GlyphNode** out, const int* key);    // 0x43b210 (lower_bound/find with an out iterator)
    void insert(void* out, const int* key);          // 0x4d3c00 (ret 8; returns `out` = {node, inserted})
    Glyph* at(const int& key);                       // 0x4d92c0 (operator[]: the glyph of `key`, inserted zeroed when absent)
};
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
    GlyphMap map;                                    // +0x44 (0x28 bytes: Font::Font 0x4d99d0 builds it)
    Glyph ascii[0x200];                              // +0x6c (0x43e190)
    Font();                                          // 0x4d99d0
    void load(const char* filename);                 // 0x4d9aa0 (reads the XML font definition)
    __declspec(noinline) float advance(int ch);      // 0x43e190
    void draw_glyph(int ch, const rgl::vector4* pos, const float* scale, rgl::mesh* m, int arg);   // 0x44f5d0 (ret 0x14) (agent4/agent_logic.cpp: one glyph quad added to a mesh)
    rgl::vector4* layout_clear(rgl::vector4* out_extent, const tstring* text, const rgl::vector4* pos, const float* scale, rgl::mesh* m,
                               const int* a, const float* b, const float* c, unsigned flags);   // 0x452bb0 (ret 0x24: clears the mesh, then layout_text; a = the colour word pointer)
    float measure(const int* text, int len, int* start, int* end);   // 0x43e200 (ret 0x10)
    // 0x4fd700 (src/game/agent5/agent_hud.cpp): the map-text row renderer, 17 stack arguments (the first is the caller's 8-byte result block, the last five
    // are the widget's vectors), `this` in ECX, ret 0x44; returns the result block
    void* layout_map(void* out, const tstring* text, const rgl::vector4* pos, const float* scale, rgl::mesh* m, const void* col_text, const void* col_link,
                     const void* col_hot, const float* xmin, const float* xmax, unsigned flags, int cur, void* v178, void* v188, void* v1b8, void* v198, void* v1a8);
    void layout_text(float* out, const tstring* text, const rgl::vector4* pos_in, const float* scale_in, void* m,
                     int arg, const float* xmin, const float* xmax, unsigned flags);   // 0x4520d0 (ret 0x24)
};

extern Font* g_font; // 0x5e2934
#endif // GAME_FONT_H
