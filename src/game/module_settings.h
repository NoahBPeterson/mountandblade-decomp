// module_settings.h: the module settings object, ONE global g_module_settings at 0xa180f8..0xa181f8 (0x100 bytes).
// Its dynamic initialiser (0x547bc0, ModuleSettings::ModuleSettings) writes the defaults before module_info.txt /
// game_variables.txt are read (the module_name directive, the game_variables keys of triggers/trigger_vec.cpp
// read_module_info); the rest of the game reads the values (map speeds, wounded tresholds, xp multipliers, ...).
// Member names are the game_variables keys. 0xa181d8.. is ten words the constructor fills and nothing reads by name.
#ifndef GAME_MODULE_SETTINGS_H
#define GAME_MODULE_SETTINGS_H
#include "./tstring.h"

struct ModuleSettings {
    tstring module_name;                        // +0x00 0xa180f8
    int scan_module_textures;                   // +0x10 0xa18108
    int scan_module_sounds;                     // +0x14 0xa1810c
    float time_multiplier;                      // +0x18 0xa18110
    float seeing_range;                         // +0x1c 0xa18114
    float track_spotting;                       // +0x20 0xa18118
    float blood_multiplier;                     // +0x24 0xa1811c
    int skill_leadership_bonus;                 // +0x28 0xa18120
    int skill_prisoner_management_bonus;        // +0x2c 0xa18124
    int base_companion_limit;                   // +0x30 0xa18128
    float player_xp_multiplier;                 // +0x34 0xa1812c
    float hero_xp_multiplier;                   // +0x38 0xa18130
    float regulars_xp_multiplier;               // +0x3c 0xa18134
    float hero_wounded_treshold;                // +0x40 0xa18138
    float player_wounded_treshold;              // +0x44 0xa1813c
    int num_hints;                              // +0x48 0xa18140
    float map_sea_direction;                    // +0x4c 0xa18144
    float map_sea_speed_x;                      // +0x50 0xa18148
    float map_sea_speed_y;                      // +0x54 0xa1814c
    float map_river_direction;                  // +0x58 0xa18150
    float map_river_speed_x;                    // +0x5c 0xa18154
    float map_river_speed_y;                    // +0x60 0xa18158
    union {
        struct {
            int map_tree_types;                 // +0x64 0xa1815c
            int map_snow_tree_types;            // +0x68 0xa18160
            int map_steppe_tree_types;          // +0x6c 0xa18164
            int map_desert_tree_types;          // +0x70 0xa18168
        };
        int tree_types[4];                      // the same four counts by biome (map, snow, steppe, desert)
    };
    int starting_year;                          // +0x74 0xa1816c
    int starting_month;                         // +0x78 0xa18170
    int starting_day;                           // +0x7c 0xa18174
    float damage_interrupt_attack_treshold;     // +0x80 0xa18178
    float armor_soak_factor[3];             // +0x84 (0xa180f8 + 0x84 = 0xa1817c) cut, pierce, blunt
    float armor_reduction_factor[3];        // +0x90 (0xa180f8 + 0x90 = 0xa18188) cut, pierce, blunt
    float missile_damage_speed_power;           // +0x9c 0xa18194
    float melee_damage_speed_power;             // +0xa0 0xa18198
    float horse_charge_damage_multiplier;       // +0xa4 0xa1819c
    float couched_lance_damage_multiplier;      // +0xa8 0xa181a0
    int enable_quick_battles;                   // +0xac 0xa181a4
    int disable_food_slot;                      // +0xb0 0xa181a8
    int limit_hair_colors;                      // +0xb4 0xa181ac
    int show_faction_color;                     // +0xb8 0xa181b0
    int show_quest_notes;                       // +0xbc 0xa181b4
    char show_party_ids_instead_of_names;       // +0xc0 0xa181b8
    int main_menu_font_color_red;               // +0xc4 0xa181bc
    int main_menu_font_color_green;             // +0xc8 0xa181c0
    int main_menu_font_color_blue;              // +0xcc 0xa181c4
    int works_with_version_min;                 // +0xd0 0xa181c8
    int works_with_version_max;                 // +0xd4 0xa181cc
    int f0d8, f0dc, f0e0, f0e4, f0e8, f0ec, f0f0, f0f4, f0f8, f0fc;   // +0xd8..+0xfc  0xa181d0..0xa181f4: limits (10000, 200000, 0x5399)
    ModuleSettings();                                                  // 0x547bc0
};
extern ModuleSettings g_module_settings;                               // 0xa180f8

#endif
