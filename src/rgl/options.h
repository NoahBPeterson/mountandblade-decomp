// options.h: the game options record (global at 0x5aa008), split out of resource_manager.h so headers that need only
// g_options do not pull in the engine log and render globals of resource_manager.h.
#ifndef RGL_OPTIONS_H
#define RGL_OPTIONS_H

// ---- game options (global at 0x5aa008) --------------------------------------------------------
// Serialised by options::save() (options.dat) in the order of the code; floats go through slot 3, ints through slot 8.
// Names follow the options pages (quest_page / video_page / keys_page) and the readers; the rest are still offsets.
struct options {
    int   i00, i04;
    float music_volume, master_volume, exp_arg;    // +0x08 +0x0c +0x10
    int   invert;                  // +0x14
    int   blood;                   // +0x18
    int   char_detail;             // +0x1c
    int   i20, i24;
    int   i28;                 // cleared when a render-target texture cannot be created
    int   i2c;                 // cleared when the environment shadow map cannot be created
    int   i30, i34, i38;
    int   max_dead;            // +0x3c
    int   i40;
    int   difficulty;          // +0x44
    int   agent_count;         // +0x48
    int   dmg_to_player;       // +0x4c
    int   dmg_to_friends;      // +0x50
    int   fx_on;               // +0x54
    int   damage_msgs;         // +0x58 (the HUD's "ui enabled" test reads it too)
    int   shot_difficulty;     // +0x5c
    int   kill_msgs;           // +0x60
    int   i64, i68;
    int   ctl_mode;            // +0x6c
    int   stick_mode;          // +0x70
    float f74;
    int   show_messages;       // +0x78
    int   i7c;
    float scale_x, scale_y, scale_z;               // +0x80 +0x84 +0x88 (gamma and the two scales of the video page)
    int   i8c, i90, i94, i98;
    void save();               // 0x4181c0
};
typedef char options_size_check[(sizeof(options) == 0x9c) ? 1 : -1];
extern options g_options;                      // 0x5aa008

#endif
