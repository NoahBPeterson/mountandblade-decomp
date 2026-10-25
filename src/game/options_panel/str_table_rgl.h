// str_table_rgl.h: the translated-string table (global 0x5e2a18, str_table.h StrTable in the src/game universe) as the three option
// pages (keys_page, quest_page, video_page) see it: their `tstring` is a typedef of rgl::string (src/rgl/rgl_math.h universe), so the
// class of str_table.h (src/game/tstring.h's tstring) cannot be included; include this header AFTER the page's
// `typedef rgl::string tstring;`. ONE definition for the three pages (named StringTable in each; renamed: StringTable is the module
// script string table 0xa75ea0 of src/game/string_table.h). lookup: 0x43a1e0 (thiscall, ret 4), by pointer or by reference.
#ifndef OPTIONS_PANEL_STR_TABLE_RGL_H
#define OPTIONS_PANEL_STR_TABLE_RGL_H

struct StrTableRgl {
    tstring* lookup(tstring* key);                          // 0x43a1e0
    tstring* lookup(tstring& key);                          // 0x43a1e0
};
extern StrTableRgl g_string_table;                          // 0x5e2a18

#endif // OPTIONS_PANEL_STR_TABLE_RGL_H
