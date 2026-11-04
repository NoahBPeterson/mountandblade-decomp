// image_addrs.h: the original image's globals that our source reaches by address, declared once with their address so the
// resolver and the link bind them (a number cast to a pointer points into our own image, not the original's).
// Objects of a class are declared with the class's name (forward declared here); the cast at the use gives the view
// the function needs. vtbl_*: a vtable in .rdata (named for its class where one is known).
#ifndef IMAGE_ADDRS_H
#define IMAGE_ADDRS_H
#include "render_flags.h"
#include "../game/trial_mode.h"

struct Config;
struct InputSet;
struct ItemKind;
struct MapFlags;
struct Mission;
struct MissionCtx;
struct ShadowMap;
struct SoundMgr;
struct StrTable;
struct TableauTbl;
struct gui_manager;
struct ini_file;
struct main_menu;
struct options_record;
struct ptype;
struct renderer;
struct resmgr;

namespace orig {
extern const float g_zero;                 // 0x549ca8
extern const float k_0_9f;                 // 0x549d6c
extern const float g_half;                 // 0x549d70
extern const float g_ten;                  // 0x549d74
extern const float k_1_5708f;              // 0x54a534
extern const char g_rgl_name[];                        // 0x54b048
extern const float k_0_4f;                 // 0x54b3b8
extern const float k_0_05f;                // 0x54b4d0
extern const float k_1_5f;                 // 0x54b8b0
extern const double g_day_period;          // 0x54a5d8
extern const float k_1em05f;               // 0x54a6d0
extern const float g_three;                // 0x54a710
extern const float k_100f;                 // 0x54ab28
extern const float k_0_39f;                // 0x54b3b0
extern const float k_0_4f;                 // 0x54b3b8
extern const float k_0_05f;                // 0x54b4d0
extern const float k_1_1f;                 // 0x54b888
extern const float k_0_03f;                // 0x54b8b4
extern const float k_9_8f;                 // 0x590af0
extern const float k_300f;                 // 0x590b20
extern const float k_400f;                 // 0x590b58
extern const float k_250f;                 // 0x590b60
extern const float k_1_2f;                 // 0x590b94
extern const float k_1_6f;                 // 0x590c48
extern const float k_0_08f;                // 0x590c74
extern const float k_0_02f;                // 0x590cd0
extern const float k_0_002f;               // 0x590c78
extern const float k_1_25f;                // 0x590c80
extern const float k_0_0005f;              // 0x590d7c
extern const float k_3_5f;                 // 0x590d8c
extern const float k_0_014f;               // 0x590f7c
extern const float k_0_23f;                // 0x590e68
extern const float k_1_7f;                 // 0x590fa4
extern const float k_75f;                  // 0x5910fc
extern const float k_0_98f;                // 0x5911d8
extern const float k_m10f;                 // 0x5915a0
extern const float k_0_94f;                // 0x591af4
extern const char str_passage[];                       // 0x595e6c
extern const char str_entry_point[];                   // 0x595e74
extern const char str_no_beard_mesh[];                 // 0x598308
extern const char str_no_face_mesh[];                  // 0x598328
extern const char str_plants[];                        // 0x5999ac
extern const char str_item_kinds[];                    // 0x5999b4
extern const char str_scene_props[];                   // 0x5999c0
extern const float g_four;                 // 0x5935c8
extern const char str_ui_difficulty_rating_percentage[]; // 0x5967a0
extern const float k_30000f;               // 0x596854
extern const float k_0_00117647f;          // 0x596858
extern const float k_0_000909091f;         // 0x59685c
extern const float k_m1em05f;              // 0x598688
extern const float k_235f;                 // 0x59868c
extern const float k_105f;                 // 0x598690
extern const float k_0_00105f;             // 0x59c268
extern unsigned __int64 g_bit[];                             // 0x5a8e68
extern const char str_m[];                             // 0x5a9c3c
extern const char str_mount_and_blade[];               // 0x5a9c54


extern options_record g_options;           // 0x5aa008

























extern SoundMgr g_sound_mgr;               // 0x5aa128

extern Config g_launcher_cfg;              // 0x5aa0a8
extern resmgr g_resmgr_obj;                    // 0x5d6fa0
extern gui_manager g_gui_mgr;                  // 0x5d7080
extern ptype* g_ptypes;                    // 0x5e28f8
extern MapFlags g_map_flags;               // 0x5e291c
extern Mission* g_mission;                 // 0x5e292c
extern ItemKind* g_items;                  // 0x5e29d0
extern int g_item_kind_count;              // 0x5e29d4
extern MapFlags g_map_flags;               // 0x5e291c
extern int g_propkind_count;               // 0x5e29e0
extern StrTable g_strtable;                // 0x5e2a18




extern int g_ptype_count;                  // 0x5e29e8
extern InputSet g_input_a;                 // 0xa170c8
extern renderer g_render;                  // 0xa17910

extern MissionCtx g_mission_ctx;           // 0xa73300
extern main_menu g_main_menu_obj;              // 0xa73980
extern TableauTbl g_tableaus;              // 0xa75f40
}  // namespace orig

// vtables
extern char vtbl_591d58[];                       // 0x591d58
extern char vtbl_XyzButton[];                    // 0x591ee8
extern char vtbl_591fb0[];                       // 0x591fb0
extern char vtbl_UiD[];                          // 0x592140
extern char vtbl_5924a0[];                       // 0x5924a0
extern char vtbl_592570[];                       // 0x592570
extern char vtbl_592644[];                       // 0x592644
extern char vtbl_592848[];                       // 0x592848
extern char vtbl_5947d8[];                       // 0x5947d8
extern char vtbl_C4c3050[];                      // 0x5957bc
extern char vtbl_597050[];                       // 0x597050
extern char vtbl_5995c0[];                       // 0x5995c0
extern char vtbl_599bf8[];                       // 0x599bf8
extern char vtbl_599d28[];                       // 0x599d28

#endif
