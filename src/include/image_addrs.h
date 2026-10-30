// image_addrs.h: the original image's globals that our source reaches by address, declared once with their address so the
// resolver and the link bind them (a number cast to a pointer points into our own image, not the original's).
// Objects of a class are declared with the class's name (forward declared here); the cast at the use gives the view
// the function needs. vtbl_*: a vtable in .rdata (named for its class where one is known).
#ifndef IMAGE_ADDRS_H
#define IMAGE_ADDRS_H

struct AnimDefV;
struct Config;
struct InputSet;
struct ItemKind;
struct MapFlags;
struct Mission;
struct MissionCtx;
struct PropKind;
struct ShadowMap;
struct SoundMgr;
struct StrTable;
struct TableauTbl;
struct gui_manager;
struct ini_file;
struct main_menu;
struct options_record;
struct ptype;
struct render_batch;
struct renderer;
struct resmgr;
struct terrain_obj;
struct text_stream;

namespace orig {
extern const float g_zero;                 // 0x549ca8
extern const float g_one;                  // 0x549cdc
extern const float k_0_3f;                 // 0x549d68
extern const float k_0_9f;                 // 0x549d6c
extern const float g_half;                 // 0x549d70
extern const float g_ten;                  // 0x549d74
extern const float k_0_1f;                 // 0x549d7c
extern const char g_empty_text[];                      // 0x54a480
extern const float k_1_5708f;              // 0x54a534
extern const double g_day_period;          // 0x54a5d8
extern const float k_1em05f;               // 0x54a6d0
extern const float k_0_2f;                 // 0x54a6e4
extern const float g_three;                // 0x54a710
extern const float k_100f;                 // 0x54ab28
extern const char g_rgl_name[];                        // 0x54b048
extern const float k_0_39f;                // 0x54b3b0
extern const float k_0_4f;                 // 0x54b3b8
extern const float k_0_05f;                // 0x54b4d0
extern const float k_100000f;              // 0x54b86c
extern const float k_1_1f;                 // 0x54b888
extern const float k_1_5f;                 // 0x54b8b0
extern const float k_0_03f;                // 0x54b8b4
extern const float k_9_8f;                 // 0x590af0
extern const float k_300f;                 // 0x590b20
extern const float k_400f;                 // 0x590b58
extern const float k_250f;                 // 0x590b60
extern const float k_0_65f;                // 0x590b6c
extern const float k_1_2f;                 // 0x590b94
extern const float k_1_6f;                 // 0x590c48
extern const float k_0_08f;                // 0x590c74
extern const float k_0_002f;               // 0x590c78
extern const float k_1_25f;                // 0x590c80
extern const float k_0_02f;                // 0x590cd0
extern const float k_0_0005f;              // 0x590d7c
extern const float k_3_5f;                 // 0x590d8c
extern const float k_0_23f;                // 0x590e68
extern const float k_0_014f;               // 0x590f7c
extern const float k_1_7f;                 // 0x590fa4
extern const float k_75f;                  // 0x5910fc
extern const float k_0_98f;                // 0x5911d8
extern const float k_m10f;                 // 0x5915a0
extern const float k_0_94f;                // 0x591af4
extern const float g_four;                 // 0x5935c8
extern const float k_5f;                   // 0x59393c
extern const char str_passage[];                       // 0x595e6c
extern const char str_entry_point[];                   // 0x595e74
extern const char str_ui_difficulty_rating_percentage[]; // 0x5967a0
extern const float k_30000f;               // 0x596854
extern const float k_0_00117647f;          // 0x596858
extern const float k_0_000909091f;         // 0x59685c
extern const char str_no_beard_mesh[];                 // 0x598308
extern const char str_no_face_mesh[];                  // 0x598328
extern const float k_m1em05f;              // 0x598688
extern const float k_235f;                 // 0x59868c
extern const float k_105f;                 // 0x598690
extern const char str_plants[];                        // 0x5999ac
extern const char str_item_kinds[];                    // 0x5999b4
extern const char str_scene_props[];                   // 0x5999c0
extern const float k_0_00105f;             // 0x59c268
extern unsigned __int64 g_bit[];                             // 0x5a8e68
extern const char str_m[];                             // 0x5a9c3c
extern const char str_mount_and_blade[];               // 0x5a9c54
extern text_stream g_log;                  // 0x5a9f60
extern int g_texture_detail;               // 0x5a9f9c
extern int g_no_cam;                       // 0x5a9fb0
extern int g_simple_reflect;               // 0x5a9fc4
extern int g_sound_muted;                  // 0x5a9fd8
extern int g_iWindowWidth;                 // 0x5a9fe4
extern int g_iWindowHeight;                // 0x5a9fe8
extern int g_shadow_quality_req;           // 0x5aa004
extern options_record g_options;           // 0x5aa008
extern float g_music_volume;               // 0x5aa010
extern float g_master_volume;              // 0x5aa014
extern float g_exp_arg;                    // 0x5aa018
extern int g_invert;                       // 0x5aa01c
extern int g_opt_blood;                    // 0x5aa020
extern int g_char_detail;                  // 0x5aa024
extern int g_char_shadow;                  // 0x5aa028
extern int g_shadows_on;                   // 0x5aa030
extern int g_plant_shadow;                 // 0x5aa034
extern int g_ragdoll_lim;                  // 0x5aa03c
extern int g_particle;                     // 0x5aa040
extern unsigned g_max_dead;                // 0x5aa044
extern int g_tick_a;                       // 0x5aa048
extern int g_difficulty;                   // 0x5aa04c
extern int g_agent_count;                  // 0x5aa050
extern int g_opt_dmg_to_player;            // 0x5aa054
extern int g_opt_dmg_to_friends;           // 0x5aa058
extern int g_fx_on;                        // 0x5aa05c
extern int g_opt_damage_msgs;              // 0x5aa060
extern int g_opt_shot_difficulty;          // 0x5aa064
extern int g_opt_kill_msgs;                // 0x5aa068
extern int g_flag5aa06c;                   // 0x5aa06c
extern int g_flag5aa070;                   // 0x5aa070
extern int g_ctl_mode;                     // 0x5aa074
extern int g_stick_mode;                   // 0x5aa078
extern float g_speed_base;                 // 0x5aa07c
extern int g_show_messages;                // 0x5aa080
extern int g_mesh_quality;                 // 0x5aa084
extern float g_scale_x;                    // 0x5aa088
extern float g_scale_y;                    // 0x5aa08c
extern float g_scale_z;                    // 0x5aa090
extern Config g_launcher_cfg;              // 0x5aa0a8
extern int g_opt_5aa0ac;                   // 0x5aa0ac
extern int g_opt_5aa0b4;                   // 0x5aa0b4
extern SoundMgr g_sound_mgr;               // 0x5aa128
extern resmgr g_resmgr_obj;                    // 0x5d6fa0
extern gui_manager g_gui_mgr;                  // 0x5d7080
extern float g_mouse_x;                    // 0x5d7094
extern float g_mouse_y;                    // 0x5d7098
extern int g_mission_clock;                // 0x5d8790
extern char g_5e2630;                      // 0x5e2630
extern ptype* g_ptypes;                    // 0x5e28f8
extern int g_use_shaders;                  // 0x5e28fc
extern MapFlags g_map_flags;               // 0x5e291c
extern Mission* g_mission;                 // 0x5e292c
extern int g_trial_mode;                   // 0x5e29c4
extern ItemKind* g_items;                  // 0x5e29d0
extern int g_item_kind_count;              // 0x5e29d4
extern PropKind* g_propkinds;              // 0x5e29dc
extern int g_propkind_count;               // 0x5e29e0
extern int g_ptype_count;                  // 0x5e29e8
extern StrTable g_strtable;                // 0x5e2a18
extern render_batch g_batch;               // 0x5fe898
extern InputSet g_input_a;                 // 0xa170c8
extern void* g_a17110;                     // 0xa17110
extern int g_a1711c;                       // 0xa1711c
extern int g_a17454;                       // 0xa17454
extern void* g_a17510;                     // 0xa17510
extern int g_a1751c;                       // 0xa1751c
extern int g_a17854;                       // 0xa17854
extern renderer g_render;                  // 0xa17910
extern char g_use_hw_shadow;               // 0xa17f24
extern unsigned char g_show_party_ids_instead_of_names; // 0xa181b8
extern int g_works_with_version_min;       // 0xa181c8
extern int g_works_with_version_max;       // 0xa181cc
extern ini_file g_module_info;             // 0xa18238
extern ShadowMap g_shadow_map;             // 0xa70248
extern MissionCtx g_mission_ctx;           // 0xa73300
extern int g_a73304[];                          // 0xa73304
extern main_menu g_main_menu_obj;              // 0xa73980
extern void* g_wB0;                        // 0xa739b0
extern void* g_wBC;                        // 0xa739bc
extern int g_a739d4;                       // 0xa739d4
extern void* g_wDC;                        // 0xa739dc
extern void* g_wE8;                        // 0xa739e8
extern TableauTbl g_tableaus;              // 0xa75f40
extern AnimDefV g_anim_set;                // 0xa76628
extern terrain_obj g_terrain;              // 0xa76630
}  // namespace orig

// vtables
extern char vtbl_CD3DApplication[];              // 0x549c60
extern char vtbl_54a740[];                       // 0x54a740
extern char vtbl_54ad68[];                       // 0x54ad68
extern char vtbl_54ae48[];                       // 0x54ae48
extern char vtbl_591d58[];                       // 0x591d58
extern char vtbl_XyzButton[];                    // 0x591ee8
extern char vtbl_591fb0[];                       // 0x591fb0
extern char vtbl_UiD[];                          // 0x592140
extern char vtbl_592208[];                       // 0x592208
extern char vtbl_5922e8[];                       // 0x5922e8
extern char vtbl_5924a0[];                       // 0x5924a0
extern char vtbl_592570[];                       // 0x592570
extern char vtbl_592644[];                       // 0x592644
extern char vtbl_592848[];                       // 0x592848
extern char vtbl_5931a8[];                       // 0x5931a8
extern char vtbl_5947d8[];                       // 0x5947d8
extern char vtbl_5948e0[];                       // 0x5948e0
extern char vtbl_C4c2c50[];                      // 0x595740
extern char vtbl_C4c3050[];                      // 0x5957bc
extern char vtbl_597050[];                       // 0x597050
extern char vtbl_598698[];                       // 0x598698
extern char vtbl_5995c0[];                       // 0x5995c0
extern char vtbl_599bf8[];                       // 0x599bf8
extern char vtbl_599d28[];                       // 0x599d28

#endif
