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
#include <new>                       // makes cl emit operator new[]/delete[] (??_U/??_V)

// ---------------------------------------------------------------- basics

// rgl string with a read cursor (16 bytes).
#include "../tstring.h"

void mb_error(const char* fmt, ...);                 // 0x42cc80

// Root of the reader-stream hierarchy (vtable 0x54a740): vptr, name at +4.
struct stream_root {
    virtual void reset();                            // slot 0
    virtual void rewind();                           // 1
    virtual stream_root& s2(stream_root* o);
    virtual stream_root& s3(float* v);
    virtual stream_root& s4(char* v);
    virtual stream_root& s5(unsigned __int64* v);
    virtual stream_root& s6(__int64* v);
    virtual stream_root& s7(unsigned* v);
    virtual stream_root& s8(int* v);
    virtual stream_root& s9(unsigned char* v);
    virtual stream_root& s10(const char* s);
    virtual stream_root& s11(const tstring* s);
    virtual stream_root& s12(void* v);
    virtual stream_root& s13(stream_root* o);
    virtual stream_root& r_float(float* v);          // 14
    virtual stream_root& r_bool(char* v);            // 15
    virtual stream_root& r_uint(unsigned* v);        // 16
    virtual stream_root& r_u64(unsigned __int64* v); // 17
    virtual stream_root& r_i64(__int64* v);          // 18
    virtual stream_root& r_int(int* v);              // 19 -> vtable +0x4c
    virtual stream_root& r_byte(char* v);            // 20
    virtual stream_root& r_tstr(tstring* v);         // 21 -> vtable +0x54
    virtual stream_root& s22(void* v);
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

// The 0x54a890 stream class: its destructor (0x417310) stores that vtable and owns nothing else, so
// the base's members are destroyed after it, `buf` before `name` -- exactly the order the original
// inlines at the end of the loader.
struct tstream2 : stream_root {
    FILE* fp;                    // +0x30
    char failed;                 // +0x34
    ~tstream2() {}                                   // 0x417310 (inlined: vtable 0x54a890)
    bool open_read_text(const char* path);           // 0x415d60
};

// The concrete text stream (vtable 0x54ac88); its constructor at 0x4172d0 stores that vtable last.
// It declares no destructor, so the implicit one just runs ~tstream2.
struct tstream3 : tstream2 {
    tstream3() { autoflush = 0; f1c = 0; fp = 0; state = 0; failed = 1; }
};

// ---------------------------------------------------------------- module path helper

extern tstring g_module_path;                        // 0x8ee3c28
extern char g_path[0x100];                           // 0x8ee41f0

// g_path = <module path><file name>; abort the load if the file cannot be opened.  Inlined at
// all sixteen call sites in the original.  The stream is returned as a base reference, which is
// how the callers reach the reader methods (so their virtual dispatch stays indirect).
__forceinline stream_root& open_module_file(tstream3& s, const char* name)
{
    g_path[0] = 0;
    strncpy(g_path, g_module_path.c_str(), 0xff);
    strncat(g_path, name, 0xff);
    if (!s.open_read_text(g_path))
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
struct MapPoint {
    char data[0x78];
    MapPoint();                                      // 0x4ce2a0
    void read(tstream3* s);                       // 0x4ce360
    void destroy_array(int flags);                   // 0x4c7ee0
};

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
    void read(tstream3* s);                       // 0x4c20c0
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
    void save(tstream3* s);                       // 0x4bdff0
    void destroy_array(int flags);                   // 0x4c8980
};

// ---------------------------------------------------------------- table objects

// each one is a global singleton that swallows a stream; only the entry points matter here.
struct SoundTable {                                  // 0x5aa128
    int pad0;                                        // +0x00
    int* p4;                                         // +0x04  -> sound count
    void load(tstream3* s, int count, tstring* path);  // 0x4c4340
};
#include "../emit.h"
struct AnimTable {                                   // 0x8ed4f60
    void read(tstream3* s);                          // 0x50d520
};
struct TableauTable {                                // 0xa75f40
    void read(tstream3* s);                          // 0x4e7a70
};
struct PresentationTable {                           // 0xa75f2c
    void fill_records(tstream3* s);                  // 0x4c6950
};
struct ScriptTable {                                 // 0xa75eb0
    void load(tstream3* s);                          // 0x4c48d0
};
#include "../string_table.h"                    // StringTable (0xa75ea0): the one definition
struct MeshTable {                                   // 0x8ed4f58
    void load(tstream3* s);                          // 0x4c3fb0
};
struct DialogTable {                                 // 0x8ed50c8
    void read(tstream3* s);                          // 0x460ab0
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
extern int g_sound_count;                            // 0x5d878c

extern int g_propkind_count;                        // 0x5e29e0
extern MapPoint* g_propkinds;                       // 0x5e29dc

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
    g_sound_mgr.load(&s_sounds, g_scan_module_sounds, &g_module_path);
    g_sound_count = *g_sound_mgr.p4;
    s_sounds.failed = 1;

    // particle_systems.txt
    tstream3 s_particles;
    stream_root& in_particles = open_module_file(s_particles, "particle_systems.txt");
    in_particles.r_tstr(&str1);
    in_particles.r_tstr(&str2);
    in_particles.r_int(&version);
    g_emit_defs.read(&s_particles);
    s_particles.failed = 1;

    // skins.txt
    tstream3 s_skins;
    stream_root& in_skins = open_module_file(s_skins, "skins.txt");
    in_skins.r_tstr(&str1);
    in_skins.r_tstr(&str2);
    in_skins.r_int(&version);
    g_anim_table.read(&s_skins);
    s_skins.failed = 1;

    // scene_props.txt
    tstream3 s_scene;
    stream_root& in_scene = open_module_file(s_scene, "scene_props.txt");
    in_scene.r_tstr(&str1);
    in_scene.r_tstr(&str2);
    in_scene.r_int(&version);
    in_scene.r_int(&g_propkind_count);
    if (g_propkinds) {
        g_propkinds->destroy_array(3);
        g_propkinds = 0;
    }
    g_propkinds = new MapPoint[g_propkind_count];
    for (int i = 0; i < g_propkind_count; i++)
        g_propkinds[i].read(&s_scene);
    s_scene.failed = 1;

    // menus.txt
    tstream3 s_menus;
    stream_root& in_menus = open_module_file(s_menus, "menus.txt");
    in_menus.r_tstr(&str1);
    in_menus.r_tstr(&str2);
    in_menus.r_int(&version);
    in_menus.r_int(&g_menu_count);
    if (g_menus) {
        g_menus->destroy_array(3);
        g_menus = 0;
    }
    g_menus = new MenuRec[g_menu_count];
    for (int i = 0; i < g_menu_count; i++)
        g_menus[i].read(*(stream_base*)&s_menus);
    s_menus.failed = 1;

    // tableau_materials.txt
    tstream3 s_tableau;
    open_module_file(s_tableau, "tableau_materials.txt");
    g_tableaus.read(&s_tableau);
    s_tableau.failed = 1;

    // presentations.txt
    tstream3 s_presentations;
    open_module_file(s_presentations, "presentations.txt");
    g_presentations.fill_records(&s_presentations);
    s_presentations.failed = 1;

    // scripts.txt
    tstream3 s_scripts;
    open_module_file(s_scripts, "scripts.txt");
    g_scripts.load(&s_scripts);
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
        g_map_icons[i].read(&s_map_icons);
    s_map_icons.failed = 1;
    log.w_cstr("Loading Module...\n");

    // strings.txt
    tstream3 s_strings;
    stream_root& in_strings = open_module_file(s_strings, "strings.txt");
    in_strings.r_tstr(&str1);
    in_strings.r_tstr(&str2);
    in_strings.r_int(&version);
    g_strings.load(&s_strings);
    s_strings.failed = 1;

    // quick_strings.txt
    tstream3 s_quick_strings;
    open_module_file(s_quick_strings, "quick_strings.txt");
    g_strings.load_quick(&s_quick_strings);
    s_quick_strings.failed = 1;

    // meshes.txt
    tstream3 s_meshes;
    open_module_file(s_meshes, "meshes.txt");
    g_meshes.load(&s_meshes);
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
    g_items = new ItemKind[g_item_kind_count];
    for (int i = 0; i < g_item_kind_count; i++)
        g_items[i].read(&s_items, version);
    s_items.failed = 1;
    log.w_cstr("Loading dialogs...\n");

    // conversation.txt
    tstream3 s_conversation;
    stream_root& in_conversation = open_module_file(s_conversation, "conversation.txt");
    in_conversation.r_tstr(&str1);
    in_conversation.r_tstr(&str2);
    in_conversation.r_int(&version);
    g_dialogs.read(&s_conversation);
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
        g_mission_templates->destroy_array(3);
        g_mission_templates = 0;
    }
    g_mission_templates = new ModeRec[g_mission_template_count];
    for (int i = 0; i < g_mission_template_count; i++)
        g_mission_templates[i].read_trigger_block(&s_mission);
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
        g_party_templates->destroy_array(3);
        g_party_templates = 0;
    }
    g_party_templates = new PartyTplRec[g_party_template_count];
    for (int i = 0; i < g_party_template_count; i++)
        g_party_templates[i].save(&s_party);
    s_party.failed = 1;
}
