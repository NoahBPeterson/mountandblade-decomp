// Slice s004c05b0: module_info.txt parser (best-effort reconstruction).
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "rgl.h"

struct tstring {
    int pos;            // +0x00
    char* buf;          // +0x04 (null: text is stored at &buf)
    int cap;            // +0x08
    int len;            // +0x0c
    tstring();                                   // 0x42bbd0
    tstring(const char* s);                      // 0x42bbe0
    ~tstring();                                  // 0x42bc40
    const tstring& operator=(const tstring& o);  // 0x42bc50
    char equals(const char* s) const;            // 0x42bd10
};
static inline const char* c_str(const tstring& s) { return s.buf ? s.buf : (const char*)&s.buf; }

// @ 0x40e3d0
float clamp_float(const float* x, const float* lo, const float* hi)
{
    return *x < *lo ? *lo : (*x >= *hi ? *hi : *x);
}

// logger at 0x5a9f60
struct logger {
    virtual void dtor();                         // +0x00
    virtual logger& operator<<(const float& v);  // +0x0c
    virtual void v10();                          // +0x10
    virtual void v14();                          // +0x14
    virtual void v18();                          // +0x18
    virtual void v1c();                          // +0x1c
    virtual logger& operator<<(const int& v);    // +0x20
    virtual void v24();                          // +0x24
    virtual logger& operator<<(const char* s);   // +0x28
    virtual logger& operator<<(const tstring& s);// +0x2c
};
extern logger g_logger;                          // 0x5a9f60

// resource manager at 0x5d6fa0
struct resmgr {
    char pad[0xbc];
    tstring cur;                                 // +0xbc
    char pad_cc[2];
    char fce;                                    // +0xce
    void load_pack(const tstring& name, int user);   // 0x41e150
    void finalize();                                 // 0x419640
};
extern resmgr g_resmgr;                          // 0x5d6fa0

extern int g_num_modules;                        // 0xa44238
struct module_rec {                              // 0x2c bytes
    tstring name;                                // +0x00
    char pad10[0xc];
    tstring slot;                                // +0x1c
};
extern module_rec g_modules[];                   // 0xa18238

extern int g_display_wp_one_handed;          // 0x5e2900
extern int g_display_wp_two_handed;          // 0x5e2904
extern int g_display_wp_polearms;            // 0x5e2908
extern int g_display_wp_archery;             // 0x5e290c
extern int g_display_wp_crossbows;           // 0x5e2910
extern int g_display_wp_throwing;            // 0x5e2914
extern int g_display_wp_firearms;            // 0x5e2918
extern int g_compatible_module_version;      // 0x5e29b8
extern int g_module_version;                 // 0x5e29c0
extern tstring g_module_name;                // 0xa180f8
extern int g_scan_module_textures;           // 0xa18108
extern int g_scan_module_sounds;             // 0xa1810c
extern float g_time_multiplier;              // 0xa18110
extern float g_seeing_range;                 // 0xa18114
extern float g_track_spotting;               // 0xa18118
extern float g_blood_multiplier;             // 0xa1811c
extern int g_skill_leadership_bonus;         // 0xa18120
extern int g_skill_prisoner_management_bonus;// 0xa18124
extern int g_base_companion_limit;           // 0xa18128
extern float g_player_xp_multiplier;         // 0xa1812c
extern float g_hero_xp_multiplier;           // 0xa18130
extern float g_regulars_xp_multiplier;       // 0xa18134
extern float g_hero_wounded_treshold;        // 0xa18138
extern float g_player_wounded_treshold;      // 0xa1813c
extern int g_num_hints;                      // 0xa18140
extern float g_map_sea_direction;            // 0xa18144
extern float g_map_sea_speed_x;              // 0xa18148
extern float g_map_sea_speed_y;              // 0xa1814c
extern float g_map_river_direction;          // 0xa18150
extern float g_map_river_speed_x;            // 0xa18154
extern float g_map_river_speed_y;            // 0xa18158
extern int g_map_tree_types;                 // 0xa1815c
extern int g_map_snow_tree_types;            // 0xa18160
extern int g_map_steppe_tree_types;          // 0xa18164
extern int g_map_desert_tree_types;          // 0xa18168
extern int g_starting_year;                  // 0xa1816c
extern int g_starting_month;                 // 0xa18170
extern int g_starting_day;                   // 0xa18174
extern float g_damage_interrupt_attack_treshold;      // 0xa18178
extern float g_armor_soak_factor_against_cut;         // 0xa1817c
extern float g_armor_soak_factor_against_pierce;      // 0xa18180
extern float g_armor_soak_factor_against_blunt;       // 0xa18184
extern float g_armor_reduction_factor_against_cut;    // 0xa18188
extern float g_armor_reduction_factor_against_pierce; // 0xa1818c
extern float g_armor_reduction_factor_against_blunt;  // 0xa18190
extern float g_missile_damage_speed_power;   // 0xa18194
extern float g_melee_damage_speed_power;     // 0xa18198
extern float g_horse_charge_damage_multiplier;   // 0xa1819c
extern float g_couched_lance_damage_multiplier;  // 0xa1819c+4 (provisional)
extern int g_main_menu_font_color_red;       // 0xa181bc
extern int g_main_menu_font_color_green;     // 0xa181c0
extern int g_main_menu_font_color_blue;      // 0xa181c4
extern int g_works_with_version_min;         // 0xa181c8
extern int g_works_with_version_max;         // 0xa181cc
extern float g_map_min_x;                    // 0x5aa120
extern float g_map_min_y;                    // 0x5aa124
extern float g_map_max_x;                    // 0x5aa118
extern float g_map_max_y;                    // 0x5aa11c
extern int g_xa181a4;                        // 0xa181a4
extern int g_xa181a8;                        // 0xa181a8
extern int g_xa181ac;                        // 0xa181ac
extern int g_xa181b0;                        // 0xa181b0
extern int g_xa181b4;                        // 0xa181b4
extern unsigned char g_xa181b8;              // 0xa181b8
extern int g_missile_damage_speed_power_int; // 0x5e29c8 (provisional)

// @ 0x4c05b0
void parse_module_info()
{
    tstring msg, val;
    g_display_wp_one_handed = 1;
    g_display_wp_two_handed = 1;
    g_display_wp_polearms = 1;
    g_display_wp_archery = 1;
    g_display_wp_crossbows = 1;
    g_display_wp_throwing = 1;
    g_display_wp_firearms = 1;
    int n = g_num_modules;
    for (int i = 0; i < n; i++) {
        msg = g_modules[i].name;
        val = g_modules[i].slot;
        if (msg.equals("load_resource")) {
            g_logger << "Loading Resource " << val << "\n";
            g_resmgr.load_pack(val, 0);
            continue;
        }
        if (msg.equals("load_mod_resource") || msg.equals("load_module_resource")) {
            g_logger << "Loading Module Resource " << val << "\n";
            g_resmgr.load_pack(val, 1);
            continue;
        }
        if (msg.equals("time_multiplier")) {
            g_time_multiplier = (float)atof(c_str(val));
            g_logger << "Setting Time Multiplier = " << g_time_multiplier << "\n";
            continue;
        }
        if (msg.equals("armor_soak_factor_against_cut")) {
            g_armor_soak_factor_against_cut = (float)atof(c_str(val));
            g_logger << "Setting armor_soak_factor_against_cut = " << g_armor_soak_factor_against_cut << "\n";
            continue;
        }
        if (msg.equals("armor_soak_factor_against_pierce")) {
            g_armor_soak_factor_against_pierce = (float)atof(c_str(val));
            g_logger << "Setting armor_soak_factor_against_pierce = " << g_armor_soak_factor_against_pierce << "\n";
            continue;
        }
        if (msg.equals("armor_soak_factor_against_blunt")) {
            g_armor_soak_factor_against_blunt = (float)atof(c_str(val));
            g_logger << "Setting armor_soak_factor_against_blunt = " << g_armor_soak_factor_against_blunt << "\n";
            continue;
        }
        if (msg.equals("armor_reduction_factor_against_cut")) {
            g_armor_reduction_factor_against_cut = (float)atof(c_str(val));
            g_logger << "Setting armor_reduction_factor_against_cut = " << g_armor_reduction_factor_against_cut << "\n";
            continue;
        }
        if (msg.equals("armor_reduction_factor_against_pierce")) {
            g_armor_reduction_factor_against_pierce = (float)atof(c_str(val));
            g_logger << "Setting armor_reduction_factor_against_pierce = " << g_armor_reduction_factor_against_pierce << "\n";
            continue;
        }
        if (msg.equals("armor_reduction_factor_against_blunt")) {
            g_armor_reduction_factor_against_blunt = (float)atof(c_str(val));
            g_logger << "Setting armor_reduction_factor_against_blunt = " << g_armor_reduction_factor_against_blunt << "\n";
            continue;
        }
        if (msg.equals("missile_damage_speed_power")) {
            float x = (float)atof(c_str(val));
            float lo = 2.0f;
            float hi = 1.0f;
            g_missile_damage_speed_power = clamp_float(&x, &hi, &lo);
            g_logger << "Setting missile_damage_speed_power = " << g_missile_damage_speed_power << "\n";
            continue;
        }
        if (msg.equals("melee_damage_speed_power")) {
            float x = (float)atof(c_str(val));
            float lo = 2.0f;
            float hi = 1.0f;
            g_melee_damage_speed_power = clamp_float(&x, &hi, &lo);
            g_logger << "Setting melee_damage_speed_power = " << g_melee_damage_speed_power << "\n";
            continue;
        }
        if (msg.equals("damage_interrupt_attack_treshold") || msg.equals("damage_interrupt_attack_threshold")) {
            g_damage_interrupt_attack_treshold = (float)atof(c_str(val));
            g_logger << "Setting damage_interrupt_attack_treshold = " << g_damage_interrupt_attack_treshold << "\n";
            continue;
        }
        if (msg.equals("horse_charge_damage_multiplier")) {
            g_horse_charge_damage_multiplier = (float)atof(c_str(val));
            g_logger << "Setting horse_charge_damage_multiplier = " << g_horse_charge_damage_multiplier << "\n";
            continue;
        }
        if (msg.equals("couched_lance_damage_multiplier")) {
            g_couched_lance_damage_multiplier = (float)atof(c_str(val));
            g_logger << "Setting couched_lance_damage_multiplier = " << g_couched_lance_damage_multiplier << "\n";
            continue;
        }
        if (msg.equals("seeing_range")) {
            g_seeing_range = (float)atof(c_str(val));
            g_logger << "Setting Seeing Range Multiplier = " << g_seeing_range << "\n";
            continue;
        }
        if (msg.equals("track_spotting_multiplier")) {
            g_track_spotting = (float)atof(c_str(val));
            g_logger << "Setting Track Spotting Multiplier = " << g_track_spotting << "\n";
            continue;
        }
        if (msg.equals("blood_multiplier")) {
            g_blood_multiplier = (float)atof(c_str(val));
            g_logger << "Setting Blood Multiplier = " << g_blood_multiplier << "\n";
            continue;
        }
        if (msg.equals("skill_leadership_bonus")) {
            g_skill_leadership_bonus = atol(c_str(val));
            g_logger << "Setting Skill Leadership Bonus = " << g_skill_leadership_bonus << "\n";
            continue;
        }
        if (msg.equals("skill_prisoner_management_bonus")) {
            g_skill_prisoner_management_bonus = atol(c_str(val));
            g_logger << "Setting Skill Prisoner Management Bonus = " << g_skill_prisoner_management_bonus << "\n";
            continue;
        }
        if (msg.equals("base_companion_limit")) {
            g_base_companion_limit = atol(c_str(val));
            g_logger << "Setting Base Companion Limit = " << g_base_companion_limit << "\n";
            continue;
        }
        if (msg.equals("player_xp_multiplier")) {
            float x = (float)atof(c_str(val));
            float lo = 10.0f;
            float hi = 1.0f;
            g_player_xp_multiplier = clamp_float(&x, &hi, &lo);
            g_logger << "Setting player_xp_multiplier = " << g_player_xp_multiplier << "\n";
            continue;
        }
        if (msg.equals("hero_xp_multiplier")) {
            g_hero_xp_multiplier = (float)atof(c_str(val));
            g_logger << "Setting hero_xp_multiplier = " << g_hero_xp_multiplier << "\n";
            continue;
        }
        if (msg.equals("regulars_xp_multiplier")) {
            g_regulars_xp_multiplier = (float)atof(c_str(val));
            g_logger << "Setting regulars_xp_multiplier = " << g_regulars_xp_multiplier << "\n";
            continue;
        }
        if (msg.equals("hero_wounded_treshold")) {
            g_hero_wounded_treshold = (float)atof(c_str(val));
            g_logger << "Setting hero_wounded_treshold = " << g_hero_wounded_treshold << "\n";
            continue;
        }
        if (msg.equals("player_wounded_treshold")) {
            g_player_wounded_treshold = (float)atof(c_str(val));
            g_logger << "Setting player_wounded_treshold = " << g_player_wounded_treshold << "\n";
            continue;
        }
        if (msg.equals("map_min_x")) {
            g_map_min_x = (float)atof(c_str(val));
            g_logger << "Setting Map Min X = " << g_map_min_x << "\n";
            continue;
        }
        if (msg.equals("map_min_y")) {
            g_map_min_y = (float)atof(c_str(val));
            g_logger << "Setting Map Min Y = " << g_map_min_y << "\n";
            continue;
        }
        if (msg.equals("map_max_x")) {
            g_map_max_x = (float)atof(c_str(val));
            g_logger << "Setting Map Max X = " << g_map_max_x << "\n";
            continue;
        }
        if (msg.equals("map_max_y")) {
            g_map_max_y = (float)atof(c_str(val));
            g_logger << "Setting Map Max Y = " << g_map_max_y << "\n";
            continue;
        }
        if (msg.equals("map_sea_direction")) {
            g_map_sea_direction = (float)atof(c_str(val));
            continue;
        }
        if (msg.equals("map_sea_speed_x")) {
            g_map_sea_speed_x = (float)atof(c_str(val));
            continue;
        }
        if (msg.equals("map_sea_speed_y")) {
            g_map_sea_speed_y = (float)atof(c_str(val));
            continue;
        }
        if (msg.equals("map_river_direction")) {
            g_map_river_direction = (float)atof(c_str(val));
            continue;
        }
        if (msg.equals("map_river_speed_x")) {
            g_map_river_speed_x = (float)atof(c_str(val));
            continue;
        }
        if (msg.equals("map_river_speed_y")) {
            g_map_river_speed_y = (float)atof(c_str(val));
            continue;
        }
        if (msg.equals("map_tree_types")) {
            g_map_tree_types = atol(c_str(val));
            continue;
        }
        if (msg.equals("map_snow_tree_types")) {
            g_map_snow_tree_types = atol(c_str(val));
            continue;
        }
        if (msg.equals("map_steppe_tree_types")) {
            g_map_steppe_tree_types = atol(c_str(val));
            continue;
        }
        if (msg.equals("map_desert_tree_types")) {
            g_map_desert_tree_types = atol(c_str(val));
            continue;
        }
        if (msg.equals("display_wp_one_handed")) {
            g_display_wp_one_handed = atol(c_str(val));
            continue;
        }
        if (msg.equals("display_wp_two_handed")) {
            g_display_wp_two_handed = atol(c_str(val));
            continue;
        }
        if (msg.equals("display_wp_polearms")) {
            g_display_wp_polearms = atol(c_str(val));
            continue;
        }
        if (msg.equals("display_wp_archery")) {
            g_display_wp_archery = atol(c_str(val));
            continue;
        }
        if (msg.equals("display_wp_crossbows")) {
            g_display_wp_crossbows = atol(c_str(val));
            continue;
        }
        if (msg.equals("display_wp_throwing")) {
            g_display_wp_throwing = atol(c_str(val));
            continue;
        }
        if (msg.equals("display_wp_firearms")) {
            g_display_wp_firearms = atol(c_str(val));
            continue;
        }
        if (msg.equals("module_version")) {
            g_module_version = atol(c_str(val));
            continue;
        }
        if (msg.equals("compatible_module_version")) {
            g_compatible_module_version = atol(c_str(val));
            continue;
        }
        if (msg.equals("scan_module_textures")) {
            g_scan_module_textures = atol(c_str(val));
            g_logger << "Scan Module Textures = " << g_scan_module_textures << "\n";
            continue;
        }
        if (msg.equals("scan_module_sounds")) {
            g_scan_module_sounds = atol(c_str(val));
            g_logger << "Scan Module Sounds = " << g_scan_module_sounds << "\n";
            continue;
        }
        if (msg.equals("module_name")) {
            g_module_name = val;
            g_logger << "Module_name = " << g_module_name << "\n";
            continue;
        }
        if (msg.equals("num_hints")) {
            g_num_hints = atol(c_str(val));
            g_logger << "Num Hints = " << g_num_hints << "\n";
            continue;
        }
        if (msg.equals("starting_year")) {
            g_starting_year = atol(c_str(val));
            continue;
        }
        if (msg.equals("starting_month")) {
            g_starting_month = atol(c_str(val));
            continue;
        }
        if (msg.equals("starting_day")) {
            g_starting_day = atol(c_str(val));
            continue;
        }
        if (msg.equals("enable_quick_battles")) {
            g_xa181a4 = atol(c_str(val));
            continue;
        }
        if (msg.equals("disable_food_slot")) {
            g_xa181a8 = atol(c_str(val));
            continue;
        }
        if (msg.equals("limit_hair_colors")) {
            g_xa181ac = atol(c_str(val));
            continue;
        }
        if (msg.equals("show_faction_color")) {
            g_xa181b0 = atol(c_str(val));
            continue;
        }
        if (msg.equals("show_quest_notes")) {
            g_xa181b4 = atol(c_str(val));
            continue;
        }
        if (msg.equals("show_party_ids_instead_of_names")) {
            g_xa181b8 = atol(c_str(val)) != 0;
            continue;
        }
        if (msg.equals("main_menu_font_color_red")) {
            g_main_menu_font_color_red = atol(c_str(val));
            continue;
        }
        if (msg.equals("main_menu_font_color_green")) {
            g_main_menu_font_color_green = atol(c_str(val));
            continue;
        }
        if (msg.equals("main_menu_font_color_blue")) {
            g_main_menu_font_color_blue = atol(c_str(val));
            continue;
        }
        if (msg.equals("works_with_version_min")) {
            g_works_with_version_min = atol(c_str(val));
            continue;
        }
        if (msg.equals("works_with_version_max")) {
            g_works_with_version_max = atol(c_str(val));
            continue;
        }
        if (n > 1)
            g_logger << "Unrecognized Module_info directive = " << msg << ". Skipping.\n";
    }
    g_resmgr.finalize();
}
