// font.h: the game font (global g_font 0x5e2934): glyph metrics and text layout.
//   advance 0x43e190 / measure 0x43e200: src/game/agent3/agent_combat.cpp (the glyph tables below are what they read)
//   layout_text 0x4520d0 (ret 0x24): called by src/game/config/widget_mgr.cpp (MissionUi::layout_text_to 0x49abc0)
// Offsets are the ones read by those functions; the rest is padding.
#ifndef GAME_FONT_H
#define GAME_FONT_H
#include <stddef.h>
#include <vector>
#include "./tstring.h"
#include "../include/vec.h"                 // V2 (layout_map's extent)

namespace rgl { struct vector4; struct mesh; }
struct V4List;                                       // ui/map_list.h: the widget's rectangle lists
struct NotePage;                                     // ui/note_page.h: the page a map-text link opens

// ---- font glyph metrics -----------------------------------------------------------------------------------
struct GlyphPt { float x, y; GlyphPt() {} GlyphPt(const float& a, const float& b) : x(a), y(b) {} };   // an (x, y) metric pair
struct Glyph {                                       // 0x1c bytes: two (x, y) pairs + 3 floats (the hash_map<int, Glyph> mapped type)
    union { int key; float f0; };                    // f0: the first metric as src/game/cls_map/cls_map.cpp Font::load 0x4d9aa0 stores it (a float)
    float f4, f8, fc, f10, f14, f18;
    // the glyph's constructor: the two pairs assigned from const-float& temporaries, then three zeroed floats (Font::Font
    // 0x4d99d0 runs it for dflt and the 0x200 ascii glyphs, GlyphMap::at 0x4d92c0 for the inserted value)
    Glyph() { *(GlyphPt*)&f0 = GlyphPt(0.0f, 0.0f); *(GlyphPt*)&f8 = GlyphPt(0.0f, 0.0f); f10 = 0.0f; f14 = 0.0f; f18 = 0.0f; }
};
// a node of the glyph table's list: std::list<std::pair<const int, Glyph> >'s node (next, prev, then the pair)
struct GlyphNode { GlyphNode *next, *prev; int key; Glyph val; };
// stdext::hash_map<int, Glyph> as VC7.1 lays it out (glyph_hash.h's IntHashBase reads it): the comparator, the node list
// (allocator, head, size), the bucket vector (node pointers), the mask and the max index. The member types keep that
// layout on 64-bit targets too (size_t / ptrdiff_t words where the library has size_type / pointers).
struct GlyphMap {
    int pad0;                                        // +0 the comparator (one byte)
    void* list_alloc;                                // +4 the node list's allocator word (list_int views the list from here)
    GlyphNode* head;                                 // +8 (0x43e190)
    size_t count;                                    // +0xc  list size, set to 0 by Font::Font 0x4d99d0
    std::vector<ptrdiff_t> buckets;                  // +0x10 (0x10 bytes) 9 copies of `head` (0x4d99d0): node pointers
    size_t i20;                                      // +0x20 the bucket mask (1, 0x4d99d0)
    size_t i24;                                      // +0x24 the max index (1, 0x4d99d0)
    GlyphMap();                                      // inline in cls_map.cpp (the hash_map constructor, expanded in Font::Font 0x4d99d0)
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
    void draw_glyph(int ch, const rgl::vector4* pos, const float* scale, rgl::mesh* m, const unsigned* color);   // 0x44f5d0 (ret 0x14) (agent4/agent_logic.cpp: one glyph quad added to a mesh in *color)
    rgl::vector4* layout_clear(rgl::vector4* out_extent, const tstring* text, const rgl::vector4* pos, const float* scale, rgl::mesh* m,
                               const int* a, const float* b, const float* c, unsigned flags);   // 0x452bb0 (ret 0x24: clears the mesh, then layout_text; a = the colour word pointer)
    float measure(const int* text, int len, int* start, int* end);   // 0x43e200 (ret 0x10)
    // 0x4fd700 (src/game/agent5/agent_hud.cpp): the map-text row renderer, `this` in ECX, ret 0x44 (17 stack words: the hidden
    // result pointer of the returned extent first, the last five are the widget's vectors)
    V2 layout_map(const tstring* text, const rgl::vector4* pos, const float* scale, rgl::mesh* m, const void* col_text, const void* col_link,
                     const void* col_hot, const float* xmin, const float* xmax, unsigned flags, int cur, V4List* rects_a, V4List* rects_b,
                     std::vector<int>* links, std::vector<NotePage>* pages, std::vector<int>* args);
    void layout_text(float* out, const tstring* text, const rgl::vector4* pos_in, const float* scale_in, rgl::mesh* m,
                     const unsigned* color, const float* xmin, const float* xmax, unsigned flags);   // 0x4520d0 (ret 0x24; color: the colour word)
};

extern Font* g_font; // 0x5e2934
#endif // GAME_FONT_H
