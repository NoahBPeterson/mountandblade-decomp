// glyph_table_calls.h: the glyph table's out-of-line helpers (the callers are Font's constructor / destructor, cls_map.cpp, and the
// insert 0x4d3c00, options_panel/world_app.cpp). Explicit specializations declared without a body keep every use a call to the
// instance the original's other objects define: rgl/mesh2/gui_edit.cpp (list clear 0x4c4d80, _Buynode 0x4c4dc0 / 0x4c4e50) and
// rgl/mesh2/mesh_2.cpp (the bucket vector's members, _Incsize 0x4cb6f0).
#ifndef GAME_GLYPH_TABLE_CALLS_H
#define GAME_GLYPH_TABLE_CALLS_H
#include "./glyph_hash.h"
namespace std {
template<> void list<pair<const int, Glyph> >::clear();                                                          // 0x4c4d80
template<> list<pair<const int, Glyph> >::_Nodeptr list<pair<const int, Glyph> >::_Buynode();                    // 0x4c4dc0
template<> list<pair<const int, Glyph> >::_Nodeptr list<pair<const int, Glyph> >::_Buynode(
    list<pair<const int, Glyph> >::_Nodeptr, list<pair<const int, Glyph> >::_Nodeptr, const pair<const int, Glyph>&);   // 0x4c4e50
template<> void list<pair<const int, Glyph> >::_Incsize(size_type);                                              // 0x4cb6f0
template<> void vector<GlyphIt>::resize(size_t, GlyphIt);                                                       // 0x4cd010
}
#endif
