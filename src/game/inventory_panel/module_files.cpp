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
#include <vector>
#include <new>                       // makes cl emit operator new[]/delete[] (??_U/??_V)

// ---------------------------------------------------------------- basics

// rgl string with a read cursor (16 bytes).
#include "../tstring.h"

void mb_error(const char* fmt, ...);                 // 0x42cc80

#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128
#include "../shared_vars.h"
#include "../dialog_mgr.h"                  // DialogMgr: g_dialogs 0x8ed50c8
#include "../../include/mission_obj.h"       // MissionObj: g_propkinds 0x5e29dc (scene_props.txt)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
DialogMgr g_dialogs;                                       // 0x8ed50c8 (constructed by the compiler: DialogMgr::DialogMgr 0x4c2470, ~DialogMgr 0x4c4090)
extern tstring g_module_path;                              // 0x8ee3c28
MissionObj* g_propkinds;                            // 0x5e29dc
int g_menu_count;                                   // 0x5e29b4
int g_party_template_count;                        // 0x5e29fc
char g_path[0x100];                            // 0x8ee41f0
int g_map_icon_count;   // 0x5e29f4
int g_mission_template_count;   // 0x5e2a04

#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/game_recs.h"
// The reader streams (vtable 0x54ac88): rgl/stream.h's tstream3 over text_stream2 (0x54a890), whose user-declared empty destructor
// (0x417310) the loader inlines: its vtable store is the first thing the end-of-function destruction does.
#include "../../rgl/stream.h"
#include "../../rgl/stream_inline.h"
#define RSTR(p) ((rgl::string*)(p))   // the stream's string slot takes rgl::string* (same 16-byte tstring)

// ---------------------------------------------------------------- module path helper


// g_path = <module path><file name>; abort the load if the file cannot be opened.  Inlined at
// all sixteen call sites in the original.  The stream is returned as a base reference, which is
// how the callers reach the reader methods (so their virtual dispatch stays indirect).
__forceinline stream_base& open_module_file(tstream3& s, const char* name)
{
    g_path[0] = 0;
    strncpy(g_path, g_module_path.c_str(), 0xff);
    strncat(g_path, name, 0xff);
    if (!s.open_read_text(g_path))
        mb_error("Unable to open %s", g_path);
    return s;
}

// ---------------------------------------------------------------- record types

#include "../trigger_vecs.h"   // SimpleTriggerVec: the simple-trigger list

// menus.txt element (0x60 bytes): ctor is inlined, load 0x4c2b20, array reject 0x4c4f40 (~MenuRec: the items through 0x431810).
#include "../menu_rec.h"           // MenuRec (menus.txt element, 0x60 bytes)

#include "../map_icon.h"           // MapIcon (map_icons.txt element, 0x48 bytes: inline ctor, read 0x4c20c0)

// item_kinds1.txt element (0x238 bytes): ctor 0x4cdf20, dtor 0x4c8a40, read 0x4c8110.
#include "../../include/item_kind.h"

// mission_templates.txt element (0x58 bytes): inline ctor, read 0x4c2300, array reject 0x4c3430 (../mode_rec.h: MissionTemplate).
#include "../mode_rec.h"
#include "../game_globals.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
MissionTemplate* g_mission_templates;                       // 0x5e2a00 mission templates (0x58-byte records, count 0x5e2a04)
MenuRec* g_menus;                                           // 0x5e29b0 the game menus (0x60-byte records)
ItemKind* g_items;                                          // 0x5e29d0 array of 0x238-byte item kinds
int g_propkind_count;                                       // 0x5e29e0 number of scene prop kinds
int g_item_kind_count;                                      // 0x5e29d4 number of item kinds (g_items)


// (party_templates.txt element: PartyTemplate, include/game_recs.h: inline ctor, read 0x4bdff0, array reject 0x4c8980.)

// ---------------------------------------------------------------- table objects

// each one is a global singleton that swallows a stream; only the entry points matter here.
struct music_set {
    void load(stream_base* s, int count, const tstring* path);   // 0x4c4340 (rgl/mesh2/rec_tables.h: SoundTable::load)
};
#include "../emit.h"
#include "../../include/rec34.h"                 // rec34_table, g_scripts (0xa75eb0)
#include "../animator.h"                          // Animator g_anim (0x8ed4f60): the skin table skins.txt is read into
#include "../../include/prsnt_table.h"           // PresentationTable, g_presentations (0xa75f2c)
#include "../string_table.h"                    // StringTable (0xa75ea0): the one definition
#include "../../include/mesh_table.h"           // MeshTable, g_meshes (0x8ed4f58)
#include "../../include/tableau_table.h"        // TableauTbl, g_tableaus (0xa75f40)
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)

// ---------------------------------------------------------------- module state


extern int g_propkind_count;                        // 0x5e29e0

extern MenuRec* g_menus;                             // 0x5e29b0

MapIcon* g_map_icons;                               // 0x5e29f0 (.bss) the map-icon table, built by the loader below

extern ItemKind* g_items;                            // 0x5e29d0
extern int g_item_kind_count;                             // 0x5e29d4

extern ModeRec* g_mission_templates;                             // 0x5e2a00

extern int g_party_template_count;                   // 0x5e29fc

// ---- data this module owns (docs/standalone_data.md) ----
PartyTemplate* g_party_templates;                    // 0x5e29f8 (.bss) the party-template table, built by the loader below

// ---------------------------------------------------------------- loader

// @ 0x50d5e0
void load_module_files()
{
    tstring str1;                // +0x14
    tstring str2;                // +0x24
    int version;                 // +0x0c
    stream_base& log = g_log;        // reached through a reference: the log calls stay virtual

    // sounds.txt
    tstream3 s_sounds;
    stream_base& in_sounds = open_module_file(s_sounds, "sounds.txt");
    in_sounds.r_tstr(RSTR(&str1));
    in_sounds.r_tstr(RSTR(&str2));
    in_sounds.r_int(&version);
    ((music_set*)&g_sound_mgr)->load((stream_base*)&s_sounds, g_module_settings.scan_module_sounds, &g_module_path);
    s_sounds.failed = 1;
    g_gui.click_sample = *(void**)g_sound_mgr.defs;

    // particle_systems.txt
    tstream3 s_particles;
    stream_base& in_particles = open_module_file(s_particles, "particle_systems.txt");
    in_particles.r_tstr(RSTR(&str1));
    in_particles.r_tstr(RSTR(&str2));
    in_particles.r_int(&version);
    g_emit_defs.resize(*(stream_base*)&s_particles);
    s_particles.failed = 1;

    // skins.txt
    tstream3 s_skins;
    stream_base& in_skins = open_module_file(s_skins, "skins.txt");
    in_skins.r_tstr(RSTR(&str1));
    in_skins.r_tstr(RSTR(&str2));
    in_skins.r_int(&version);
    g_anim.read(&s_skins);
    s_skins.failed = 1;

    // scene_props.txt
    tstream3 s_scene;
    stream_base& in_scene = open_module_file(s_scene, "scene_props.txt");
    in_scene.r_tstr(RSTR(&str1));
    in_scene.r_tstr(RSTR(&str2));
    in_scene.r_int(&version);
    in_scene.r_int(&g_propkind_count);
    if (g_propkinds) {
        delete[] g_propkinds;
        g_propkinds = 0;
    }
    g_propkinds = new MissionObj[g_propkind_count];
    for (int i = 0; i < g_propkind_count; i++)
        g_propkinds[i].read((stream_base*)&s_scene);
    s_scene.failed = 1;

    // menus.txt
    tstream3 s_menus;
    stream_base& in_menus = open_module_file(s_menus, "menus.txt");
    in_menus.r_tstr(RSTR(&str1));
    in_menus.r_tstr(RSTR(&str2));
    in_menus.r_int(&version);
    in_menus.r_int(&g_menu_count);
    if (g_menus) {
        delete[] g_menus;
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
    g_scripts.load(*(stream_base*)&s_scripts);
    s_scripts.failed = 1;

    // map_icons.txt
    tstream3 s_map_icons;
    stream_base& in_map_icons = open_module_file(s_map_icons, "map_icons.txt");
    in_map_icons.r_tstr(RSTR(&str1));
    str1.to_lower();
    if (str1.differs("map_icons_file"))
        mb_error("Invalid Map Icons File!");
    in_map_icons.r_tstr(RSTR(&str2));
    in_map_icons.r_int(&version);
    in_map_icons.r_int(&g_map_icon_count);
    if (g_map_icons) {
        delete[] g_map_icons;
        g_map_icons = 0;
    }
    g_map_icons = new MapIcon[g_map_icon_count];
    for (int i = 0; i < g_map_icon_count; i++)
        g_map_icons[i].read(*(stream_base*)&s_map_icons);
    s_map_icons.failed = 1;
    log.w_cstr("Loading Module...\n");

    // strings.txt
    tstream3 s_strings;
    stream_base& in_strings = open_module_file(s_strings, "strings.txt");
    in_strings.r_tstr(RSTR(&str1));
    in_strings.r_tstr(RSTR(&str2));
    in_strings.r_int(&version);
    g_strings.load1((stream_base*)&s_strings);
    s_strings.failed = 1;

    // quick_strings.txt
    tstream3 s_quick_strings;
    open_module_file(s_quick_strings, "quick_strings.txt");
    g_strings.load2((stream_base*)&s_quick_strings);
    s_quick_strings.failed = 1;

    // meshes.txt
    tstream3 s_meshes;
    open_module_file(s_meshes, "meshes.txt");
    g_meshes.load((stream_base*)&s_meshes);
    s_meshes.failed = 1;
    log.w_cstr("Loading item kinds...\n");

    // item_kinds1.txt
    tstream3 s_items;
    stream_base& in_items = open_module_file(s_items, "item_kinds1.txt");
    in_items.r_tstr(RSTR(&str1));
    in_items.r_tstr(RSTR(&str2));
    in_items.r_int(&version);
    in_items.r_int(&g_item_kind_count);
    if (g_items) {
        delete[] g_items;
        g_items = 0;
    }
    g_items = new ItemKind[g_item_kind_count];
    for (int i = 0; i < g_item_kind_count; i++)
        g_items[i].read(&s_items, version);
    s_items.failed = 1;
    log.w_cstr("Loading dialogs...\n");

    // conversation.txt
    tstream3 s_conversation;
    stream_base& in_conversation = open_module_file(s_conversation, "conversation.txt");
    in_conversation.r_tstr(RSTR(&str1));
    in_conversation.r_tstr(RSTR(&str2));
    in_conversation.r_int(&version);
    g_dialogs.read((stream_base*)&s_conversation);
    s_conversation.failed = 1;
    log.w_cstr("Loading mission templates...\n");

    // mission_templates.txt
    tstream3 s_mission;
    stream_base& in_mission = open_module_file(s_mission, "mission_templates.txt");
    in_mission.r_tstr(RSTR(&str1));
    in_mission.r_tstr(RSTR(&str2));
    in_mission.r_int(&version);
    in_mission.r_int(&g_mission_template_count);
    if (g_mission_templates) {
        delete[] g_mission_templates;
        g_mission_templates = 0;
    }
    g_mission_templates = new MissionTemplate[g_mission_template_count];
    for (int i = 0; i < g_mission_template_count; i++)
        g_mission_templates[i].read(*(stream_base*)&s_mission);
    s_mission.failed = 1;
    log.w_cstr("Loading party templates...\n");

    // party_templates.txt
    tstream3 s_party;
    stream_base& in_party = open_module_file(s_party, "party_templates.txt");
    in_party.r_tstr(RSTR(&str1));
    in_party.r_tstr(RSTR(&str2));
    in_party.r_int(&version);
    in_party.r_int(&g_party_template_count);
    if (g_party_templates) {
        delete[] g_party_templates;
        g_party_templates = 0;
    }
    g_party_templates = new PartyTemplate[g_party_template_count];
    for (int i = 0; i < g_party_template_count; i++)
        g_party_templates[i].load(*(stream_base*)&s_party);
    s_party.failed = 1;
}
