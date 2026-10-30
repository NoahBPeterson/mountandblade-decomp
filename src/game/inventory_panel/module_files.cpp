// Slice s0050d5e0: the module-data loader.
//
// One 5728-byte function that opens every module data file in turn (sounds, particle systems,
// skins, scene props, menus, tableau materials, presentations, scripts, map icons, strings,
// quick strings, meshes, item kinds, conversation, mission templates, party templates), feeds
// each through a fresh text stream and hands the stream to the corresponding table object.
//
// Everything lives in one scope in the original: the two scratch tstrings (str1/str2) and the
// version int are shared by all blocks, and all sixteen streams are destroyed together in
// reverse declaration order at the end of the function.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <new>                       // makes cl emit operator new[]/delete[] (??_U/??_V)

// ---------------------------------------------------------------- basics

// rgl string with a read cursor (16 bytes).
#include "../tstring.h"

void mb_error(const char* fmt, ...);                 // 0x42cc80

// Root of the reader-stream hierarchy (vtable 0x54a740): vptr, name at +4. This file's own tstring-based copy of rgl/stream.h's
// stream_base (stream.h cannot be included here: see stream_fwd.h); every typed slot is abstract, as in the original.
#include "../../rgl/stream_fwd.h"
struct stream_root {
    virtual void reset();                            // slot 0
    virtual void rewind();                           // 1
    STREAM_SLOTS_OF(stream_root, tstring, = 0)       // 2 .. 22
    virtual bool at_end();
    virtual void set_autoflush(char f);
    virtual char get_autoflush();
    virtual int flush();
    tstring name;                                    // +0x04
    int state;                                       // +0x14
    char autoflush;                                  // +0x18
    int f1c;                                         // +0x1c
    tstring buf;                                     // +0x20
    stream_root() {}
};
#undef SF_RT
#define SF_RT stream_root
#undef SF_STR
#define SF_STR tstring
// the base slots (0x415220..0x415280, 0x408020): stream_base's functions
void stream_root::reset() { ((stream_base*)this)->stream_base::reset(); }
void stream_root::rewind() { ((stream_base*)this)->stream_base::rewind(); }
bool stream_root::at_end() { return ((stream_base*)this)->stream_base::at_end(); }
void stream_root::set_autoflush(char f) { ((stream_base*)this)->stream_base::set_autoflush(f); }
char stream_root::get_autoflush() { return ((stream_base*)this)->stream_base::get_autoflush(); }
int stream_root::flush() { return ((stream_base*)this)->stream_base::flush(); }

// The 0x54a890 stream class (rgl/core/resmgr.h's text_reader_b): its destructor (0x417310) stores that vtable and owns nothing
// else, so the base's members are destroyed after it, `buf` before `name` -- exactly the order the original inlines at the end of
// the loader. Its typed slots are text_stream's.
struct text_reader_b : stream_root {
    FILE* fp;                    // +0x30
    char failed;                 // +0x34
    STREAM_SLOTS_OF(stream_root, tstring, )
    ~text_reader_b() {}                                 // 0x417310 (inlined: vtable 0x54a890)
};
STREAM_FWD_TEXT_BOTH(text_reader_b)
// the text stream's file opener (0x415d60; stream_fwd.h's text_stream)
inline bool open_read_text(void* s, const char* path) { return ((text_stream*)s)->open_read_text(path); }

// The concrete text stream (vtable 0x54ac88); its constructor at 0x4172d0 stores that vtable last.
// It declares no destructor, so the implicit one just runs ~text_reader_b.
struct tstream3 : text_reader_b {
    STREAM_SLOTS_OF(stream_root, tstring, )
    tstream3() { autoflush = 0; f1c = 0; fp = 0; state = 0; failed = 1; }
};
STREAM_FWD_TEXT_READER(tstream3)

// ---------------------------------------------------------------- module path helper

extern tstring g_module_path;                        // 0x8ee3c28
extern char g_path[0x100];                           // 0x8ee41f0

// g_path = <module path><file name>; abort the load if the file cannot be opened.  Inlined at
// all sixteen call sites in the original.  The stream is returned as a base reference, which is
// how the callers reach the reader methods (so their virtual dispatch stays indirect).
// MB_FAST_LOAD=1 skips the per-sound sample decode at startup (the game's own load_samples == 0 path):
// FMOD needs ~58 s for the 359 entries under Wine, which makes every test run pay for it.
int mb_fast_load(void)
{
    static int cached = -1;
    if (cached < 0) {
        FILE* f = fopen("mb_fast_load.on", "rb");   // a marker file in the working directory
        if (f) { fclose(f); cached = 1; } else cached = 0;
    }
    return cached;
}

__forceinline stream_root& open_module_file(tstream3& s, const char* name)
{
    g_path[0] = 0;
    strncpy(g_path, g_module_path.c_str(), 0xff);
    strncat(g_path, name, 0xff);
    if (!open_read_text(&s, g_path))
        mb_error("Unable to open %s", g_path);
    return s;
}

// ---------------------------------------------------------------- record types

// One 12 byte vector-like container (three pointers); used as a sub-object of several records.
struct Vec12 {
    void* begin;                 // +0x00
    void* end;                   // +0x04
    void* cap;                   // +0x08
    Vec12() { begin = end = cap = 0; }
    ~Vec12();                                        // 0x4317b0
};

// scene_props.txt element (0x78 bytes): ctor 0x4ce2a0, read 0x4ce360, array-deleting
// destructor 0x4c7ee0 (flag 3 = destroy the elements and free the cookie-prefixed block).
struct MissionObj {
    char data[0x78];
    MissionObj();                                      // 0x4ce2a0
    void read(stream_base* s);                       // 0x4ce360
};
// the array-deleting destructors of the four record arrays, as the definitions declare them (the loader calls them with the
// array pointer in ecx and the flags on the stack; the second slot is the definitions' unused edx parameter)
void* __fastcall destroy_record_array(void* p, int, unsigned char flags);             // 0x4c7ee0 (the 0x78-byte records)
struct elem60;
void* __fastcall elem60_vector_delete(elem60* p, void*, unsigned flags);               // 0x4c4f40 (the 0x60-byte menu records)
struct elem58 {
    void* vector_deleting_delete(unsigned flags);   // 0x4c3430 (the 0x58-byte mission templates)
};
struct conv_rec;
void* __fastcall conv_item_array_dtor(conv_rec* p, int, unsigned char flags);          // 0x4c8980 (the 0xb0-byte party templates)

// menus.txt element (0x60 bytes): ctor is inlined, load 0x4c2b20, array reject 0x4c4f40.
#include "../menu_rec.h"           // MenuRec (menus.txt element, 0x60 bytes)

// map_icons.txt element (0x48 bytes): inline ctor, read 0x4c20c0.
struct MapIcon {
    tstring a;                   // +0x00
    int f10;                     // +0x10
    tstring b;                   // +0x14
    char pad24[0x18];            // +0x24
    Vec12 v;                     // +0x3c
    MapIcon() {}
    void read(stream_base& s);                    // 0x4c20c0
};

// item_kinds1.txt element (0x238 bytes): ctor 0x4cdf20, dtor 0x4c8a40, read 0x4c8110.
#include "../../include/item_kind.h"

// mission_templates.txt element (0x58 bytes): inline ctor, read 0x4c2300, array reject 0x4c3430 (../mode_rec.h: ModeRec).
#include "../mode_rec.h"

// party_templates.txt element (0xb0 bytes): inline ctor, read 0x4bdff0, array reject 0x4c8980.
struct PartyTplRec {
    tstring a;                   // +0x00
    tstring b;                   // +0x10
    char pad20[0x84];            // +0x20
    int fA4;                     // +0xa4
    int fA8;                     // +0xa8
    int fAC;                     // +0xac
    PartyTplRec() : fA4(0), fA8(0), fAC(0) {}
};
struct sound_def {
    void save(stream_base& s);   // 0x4bdff0 (game/config: the party template's record writer)
};

// ---------------------------------------------------------------- table objects

// each one is a global singleton that swallows a stream; only the entry points matter here.
struct SoundTable {                                  // 0x5aa128
    int pad0;                                        // +0x00
    int* p4;                                         // +0x04  -> sound count
};
struct music_set {
    void load(stream_base* s, int count, const tstring* path);   // 0x4c4340 (rgl/mesh2/rec_tables.h: SoundTable::load)
};
#include "../emit.h"
struct AnimTable { char pad; };                      // 0x8ed4f60
struct AccessStream;
struct mesh_scene {
    void read(AccessStream* s);   // 0x50d520 (AnimTable::read: game/mesh_slot)
};
struct TableauTable { char pad; };                   // 0xa75f40
struct SlotReader;
struct MapLoader {
    void read(SlotReader* s);   // 0x4e7a70 (TableauTable::read)
};
struct PresentationTable { char pad; };              // 0xa75f2c
struct RefRecord;
struct ItemPage {
    void fill_records(RefRecord* s);         // 0x4c6950 (PresentationTable::fill_records: rgl/mesh2/gui_edit.h)
};
struct ScriptTable { char pad; };                    // 0xa75eb0
struct rec34_list {
    void load(stream_base& s);              // 0x4c48d0 (ScriptTable::load, ret 4: rgl/mesh2/gui_edit.h)
};
#include "../string_table.h"                    // StringTable (0xa75ea0): the one definition
struct MeshTable { char pad; };                      // 0x8ed4f58
struct elem48_holder;
void __fastcall load_elem48_array(elem48_holder* self, int, stream_base* s);   // 0x4c3fb0 (MeshTable::load: rgl/mesh2/rec_tables.cpp)
struct DialogTable { char pad; };                    // 0x8ed50c8
struct DialogMgr {
    void read(stream_base* s);   // 0x460ab0 (DialogTable::read)
};
struct pair_holder {
    void load1(stream_base* s);   // 0x4c46d0 / 0x4c47d0 (StringTable::load1 / load2: rgl/mesh2/rec_tables.h)
    void load2(stream_base* s);   // 0x4c47d0
};
struct Block {
    void read(stream_base* s, int version);   // 0x4c8110 (ItemKind::read: rgl/mesh2/gui_edit.h; ret 8, the file version is the second argument)
};
struct csv_row {
    char data[0x238];
    csv_row();   // 0x4cdf20 (ItemKind::ItemKind: rgl/mesh2)
};

extern SoundTable g_sound_mgr;                          // 0x5aa128
extern EmitTable g_emit_defs;                        // 0x8ed5090
extern AnimTable g_anim_table;                            // 0x8ed4f60
extern TableauTable g_tableaus;                       // 0xa75f40
extern PresentationTable g_presentations;            // 0xa75f2c
extern ScriptTable g_scripts;                        // 0xa75eb0
extern MeshTable g_meshes;                           // 0x8ed4f58
extern DialogTable g_dialogs;                        // 0x8ed50c8

// logging sink (0x5a9f60): w_cstr is vtable slot 10 (+0x28).
struct LogSink {
    virtual void l0(); virtual void l1(); virtual void l2(); virtual void l3(); virtual void l4();
    virtual void l5(); virtual void l6(); virtual void l7(); virtual void l8(); virtual void l9();
    virtual void w_cstr(const char* s);              // slot 10 -> vtable +0x28
};
extern LogSink g_log;                                // 0x5a9f60

// ---------------------------------------------------------------- module state

extern int g_scan_module_sounds;                        // 0xa1810c
extern int g_loaded_sound_count;                            // 0x5d878c

extern int g_propkind_count;                        // 0x5e29e0
extern MissionObj* g_propkinds;                       // 0x5e29dc

extern int g_menu_count;                             // 0x5e29b4
extern MenuRec* g_menus;                             // 0x5e29b0

extern int g_map_icon_count;                         // 0x5e29f4
extern MapIcon* g_map_icons;                         // 0x5e29f0

extern int g_item_kind_count;                             // 0x5e29d4
extern ItemKind* g_items;                            // 0x5e29d0

extern int g_mission_template_count;                             // 0x5e2a04
extern ModeRec* g_mission_templates;                             // 0x5e2a00

extern int g_party_template_count;                   // 0x5e29fc
extern PartyTplRec* g_party_templates;                  // 0x5e29f8

// ---------------------------------------------------------------- loader

// `new ItemKind[n]` as the original runs it: the element count in front of the elements, each element built by 0x4cdf20
static ItemKind* new_item_array(int n)
{
    int* raw = (int*)::operator new(n * 0x238 + 4);
    if (raw == 0)
        return 0;
    raw[0] = n;
    csv_row* e = (csv_row*)(raw + 1);
    for (int i = 0; i < n; i++)
        new (&e[i]) csv_row;
    return (ItemKind*)e;
}

// @ 0x50d5e0
void load_module_files()
{
    tstring str1;                // +0x14
    tstring str2;                // +0x24
    int version;                 // +0x0c
    LogSink& log = g_log;        // reached through a reference: the log calls stay virtual

    // sounds.txt
    tstream3 s_sounds;
    stream_root& in_sounds = open_module_file(s_sounds, "sounds.txt");
    in_sounds.r_tstr(&str1);
    in_sounds.r_tstr(&str2);
    in_sounds.r_int(&version);
    ((music_set*)&g_sound_mgr)->load((stream_base*)&s_sounds, mb_fast_load() ? 0 : g_scan_module_sounds, &g_module_path);
    g_loaded_sound_count = *g_sound_mgr.p4;
    s_sounds.failed = 1;

    // particle_systems.txt
    tstream3 s_particles;
    stream_root& in_particles = open_module_file(s_particles, "particle_systems.txt");
    in_particles.r_tstr(&str1);
    in_particles.r_tstr(&str2);
    in_particles.r_int(&version);
    g_emit_defs.resize(&s_particles);
    s_particles.failed = 1;

    // skins.txt
    tstream3 s_skins;
    stream_root& in_skins = open_module_file(s_skins, "skins.txt");
    in_skins.r_tstr(&str1);
    in_skins.r_tstr(&str2);
    in_skins.r_int(&version);
    ((mesh_scene*)&g_anim_table)->read((AccessStream*)&s_skins);
    s_skins.failed = 1;

    // scene_props.txt
    tstream3 s_scene;
    stream_root& in_scene = open_module_file(s_scene, "scene_props.txt");
    in_scene.r_tstr(&str1);
    in_scene.r_tstr(&str2);
    in_scene.r_int(&version);
    in_scene.r_int(&g_propkind_count);
    if (g_propkinds) {
        destroy_record_array(g_propkinds, 0, 3);
        g_propkinds = 0;
    }
    g_propkinds = new MissionObj[g_propkind_count];
    for (int i = 0; i < g_propkind_count; i++)
        g_propkinds[i].read((stream_base*)&s_scene);
    s_scene.failed = 1;

    // menus.txt
    tstream3 s_menus;
    stream_root& in_menus = open_module_file(s_menus, "menus.txt");
    in_menus.r_tstr(&str1);
    in_menus.r_tstr(&str2);
    in_menus.r_int(&version);
    in_menus.r_int(&g_menu_count);
    if (g_menus) {
        elem60_vector_delete((elem60*)g_menus, 0, 3);
        g_menus = 0;
    }
    g_menus = new MenuRec[g_menu_count];
    for (int i = 0; i < g_menu_count; i++)
        g_menus[i].read(*(stream_base*)&s_menus);
    s_menus.failed = 1;

    // tableau_materials.txt
    tstream3 s_tableau;
    open_module_file(s_tableau, "tableau_materials.txt");
    ((MapLoader*)&g_tableaus)->read((SlotReader*)&s_tableau);
    s_tableau.failed = 1;

    // presentations.txt
    tstream3 s_presentations;
    open_module_file(s_presentations, "presentations.txt");
    ((ItemPage*)&g_presentations)->fill_records((RefRecord*)&s_presentations);
    s_presentations.failed = 1;

    // scripts.txt
    tstream3 s_scripts;
    open_module_file(s_scripts, "scripts.txt");
    ((rec34_list*)&g_scripts)->load(*(stream_base*)&s_scripts);
    s_scripts.failed = 1;

    // map_icons.txt
    tstream3 s_map_icons;
    stream_root& in_map_icons = open_module_file(s_map_icons, "map_icons.txt");
    in_map_icons.r_tstr(&str1);
    str1.to_lower();
    if (str1.differs("map_icons_file"))
        mb_error("Invalid Map Icons File!");
    in_map_icons.r_tstr(&str2);
    in_map_icons.r_int(&version);
    in_map_icons.r_int(&g_map_icon_count);
    delete[] g_map_icons;
    g_map_icons = 0;
    g_map_icons = new MapIcon[g_map_icon_count];
    for (int i = 0; i < g_map_icon_count; i++)
        g_map_icons[i].read(*(stream_base*)&s_map_icons);
    s_map_icons.failed = 1;
    log.w_cstr("Loading Module...\n");

    // strings.txt
    tstream3 s_strings;
    stream_root& in_strings = open_module_file(s_strings, "strings.txt");
    in_strings.r_tstr(&str1);
    in_strings.r_tstr(&str2);
    in_strings.r_int(&version);
    ((pair_holder*)&g_strings)->load1((stream_base*)&s_strings);
    s_strings.failed = 1;

    // quick_strings.txt
    tstream3 s_quick_strings;
    open_module_file(s_quick_strings, "quick_strings.txt");
    ((pair_holder*)&g_strings)->load2((stream_base*)&s_quick_strings);
    s_quick_strings.failed = 1;

    // meshes.txt
    tstream3 s_meshes;
    open_module_file(s_meshes, "meshes.txt");
    load_elem48_array((elem48_holder*)&g_meshes, 0, (stream_base*)&s_meshes);
    s_meshes.failed = 1;
    log.w_cstr("Loading item kinds...\n");

    // item_kinds1.txt
    tstream3 s_items;
    stream_root& in_items = open_module_file(s_items, "item_kinds1.txt");
    in_items.r_tstr(&str1);
    in_items.r_tstr(&str2);
    in_items.r_int(&version);
    in_items.r_int(&g_item_kind_count);
    delete[] g_items;
    g_items = 0;
    g_items = new_item_array(g_item_kind_count);
    for (int i = 0; i < g_item_kind_count; i++)
        ((Block*)&g_items[i])->read((stream_base*)&s_items, version);
    s_items.failed = 1;
    log.w_cstr("Loading dialogs...\n");

    // conversation.txt
    tstream3 s_conversation;
    stream_root& in_conversation = open_module_file(s_conversation, "conversation.txt");
    in_conversation.r_tstr(&str1);
    in_conversation.r_tstr(&str2);
    in_conversation.r_int(&version);
    ((DialogMgr*)&g_dialogs)->read((stream_base*)&s_conversation);
    s_conversation.failed = 1;
    log.w_cstr("Loading mission templates...\n");

    // mission_templates.txt
    tstream3 s_mission;
    stream_root& in_mission = open_module_file(s_mission, "mission_templates.txt");
    in_mission.r_tstr(&str1);
    in_mission.r_tstr(&str2);
    in_mission.r_int(&version);
    in_mission.r_int(&g_mission_template_count);
    if (g_mission_templates) {
        ((elem58*)g_mission_templates)->vector_deleting_delete(3);
        g_mission_templates = 0;
    }
    g_mission_templates = new ModeRec[g_mission_template_count];
    for (int i = 0; i < g_mission_template_count; i++)
        g_mission_templates[i].read(*(stream_base*)&s_mission);
    s_mission.failed = 1;
    log.w_cstr("Loading party templates...\n");

    // party_templates.txt
    tstream3 s_party;
    stream_root& in_party = open_module_file(s_party, "party_templates.txt");
    in_party.r_tstr(&str1);
    in_party.r_tstr(&str2);
    in_party.r_int(&version);
    in_party.r_int(&g_party_template_count);
    if (g_party_templates) {
        conv_item_array_dtor((conv_rec*)g_party_templates, 0, 3);
        g_party_templates = 0;
    }
    g_party_templates = new PartyTplRec[g_party_template_count];
    for (int i = 0; i < g_party_template_count; i++)
        ((sound_def*)&g_party_templates[i])->save(*(stream_base*)&s_party);
    s_party.failed = 1;
}
