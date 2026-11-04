// terrain_page.h: the terrain "edit" page / terrain-generator window (vtable 0x595c44, a head_rec: slot 1 reset_screen 0x4cb650, slot 2
// init_screen 0x4f1460 builds the window, slot 3 frame_update 0x4f2d70; 0x23c bytes as 0x4d0620 allocates it): the
// checkboxes / sliders / number fields that edit the terrain bit words at 0xa76630 and the preview camera (the page's
// +0x6c world pointer, the 0x120-byte map block at +0x70 and the camera state). ONE definition for char_sheet:
// terrain_page (the page refresh, captions, hex text, bit sync: 0x4f2730..0x4f2ae0), terrain_page_update (0x4f2d70, 0x4f34c0)
// and terrain_page_input (0x4f3570..0x4f3cf0). The three slices named the same offsets differently; the names below follow
// the update/input slices (checkbox / camera spelling) and say which offsets the slices read in two ways.
// Needs `tstring` (for MissionUi) declared by the includer.
#ifndef MB_GAME_TERRAIN_PAGE_H
#define MB_GAME_TERRAIN_PAGE_H
#include "./head_rec.h"                                     // head_rec: the vptr, +4 / +8, the option at +0x18 (w18), the name at +0x1c
#include "./ui_widget.h"
#include "../include/frustum.h"

namespace rgl { struct world; }                             // the preview scene
struct NodeA; struct text_box; struct MapWidget;

struct TerrainPage : head_rec { MB_ZERO_TAIL(TerrainPage)   // vtable 0x595c44
    int     terrain_on;                                     // +0x2c  "Enable terrain" checkbox (slice terrain_page: enabled1)
    int     terraingen_on;                                  // +0x30  "Enable terrain generation" checkbox (enabled2)
    int     water_on;                                       // +0x34  "Enable water" checkbox (enabled3)
    int     river_on;                                       // +0x38  "Enable river" checkbox (water2)
    float   camera_x;                                       // +0x3c  X position (f3c)
    float   camera_y;                                       // +0x40  Y position (f40)
    float   camera_z;                                       // +0x44  camera height above the ground (f44)
    float   fog;                                            // +0x48  fog (0..1) (f48)
    float   water_height;                                   // +0x4c  water height (0..1) (f4c)
    int     water_level;                                    // +0x50  water level, written into bits 0x8e..0x94
    int     layer_bits[3];                                  // +0x54 / +0x58 / +0x5c  random bits of the water layers (hex3)
    int     terrain_type;                                   // +0x60  terrain type (4 bits) (type)
    int     map_size;                                       // +0x64  terrain size (2 bits) (size)
    NodeA*  board;                                          // +0x68  the window (0x190, vptr 0x592848)
    MapWidget* view;                                        // +0x6c  the 3D preview (0x358, ctor 0x4ed630); its frustum at +0x1ac takes a copy of cam
    rgl::frustum cam;                                       // +0x70  the preview camera (0x120 bytes; cam.m is the frame path() builds, its origin row is +0x168)
    rgl::vector4 view_pos;                                  // +0x190  eye position (x, y clamped to 0..250)
    float   yaw;                                            // +0x1a0  camera heading
    float   pitch;                                          // +0x1a4  camera pitch (0.1..1.5)
    float   zoom;                                           // +0x1a8  camera zoom (1.5..400.5)
    rgl::world* world;                                          // +0x1ac  the preview scene (0x344, ctor 0x478ad0): the page drives it and reset_screen deletes it
    text_box* panel;                                        // +0x1b0  the board panel (0x188, vptr 0x597050)
    MissionUi* hexbox;                                         // +0x1b4  the hex terrain text field
    MissionUi* copy_btn;                                       // +0x1b8  copy terrain code
    MissionUi* paste_btn;                                      // +0x1bc  paste terrain code
    union {
        MissionUi* sliders[6];                                 // +0x1c0  the six parameter sliders (window init builds them in a loop)
        struct {
            MissionUi* x_slider;                               // +0x1c0  X position slider (a range widget: set_value is slot +0x34)
            MissionUi* y_slider;                               // +0x1c4  Y position slider
            MissionUi* z_slider;                               // +0x1c8  camera height slider
            MissionUi* fog_slider;                             // +0x1cc  fog slider
            MissionUi* level_slider;                           // +0x1d0  water level slider
            MissionUi* height_slider;                          // +0x1d4  water height slider
        };
    };
    char    pad1d8[0x200 - 0x1d8];
    MissionUi* bit_fields[3];                                  // +0x200  number fields of terrain slot 0..2 (num2)
    MissionUi* layer_boxes[3];                                 // +0x20c  select checkbox / bit-field strip of terrain slot 0..2 (box3)
    MissionUi* water_box;                                      // +0x218  Enable water (w218)
    MissionUi* river_box;                                      // +0x21c  Enable river (w21c)
    MissionUi* terrain_box;                                    // +0x220  Enable terrain (w220)
    MissionUi* terraingen_box;                                 // +0x224  Enable terrain generation (w224)
    MissionUi* type_box;                                       // +0x228  terrain type (w228)
    MissionUi* size_box;                                       // +0x22c  terrain size (w22c)
    MissionUi* regen_btn;                                      // +0x230  regenerate (w230)
    MissionUi* back_btn;                                       // +0x234  back (w234)
    int     f238;                                           // +0x238

    virtual void reset_screen();                            // slot 1: 0x4cb650 (rgl/mesh2/mesh_2.cpp)
    virtual void init_screen();                             // slot 2: 0x4f1460 (char_sheet/facegen_win.cpp)
    virtual void frame_update(float dt);                    // slot 3: 0x4f2d70 (terrain_page_update)
    virtual char pure_7() { return 1; }                     // slot 7: 0x48b170
    void update();                                          // 0x4f2730 (terrain_page: refresh every widget from the page fields, then the hex field)
    __declspec(noinline) void set_enabled_text(MissionUi* w);                       // 0x4f28a0
    __declspec(noinline) void set_hex3_text(int i);                              // 0x4f28f0
    void refresh_hex();                                     // 0x4f2960
    bool changed(MissionUi* w) { return w->clear_mask(0x10000); } // +0x44 (the page's change flag)
    void sync_bits();                                       // 0x4f2ae0
    void copy_clipboard();                                  // 0x4f34c0
    void paste_clipboard();                                 // 0x4f3c30
    void read_fields();                                     // 0x4f3570
    void simulate(float dt);                                // 0x4f37f0
    void path(float dt);                                    // 0x4f3cf0
};

extern const int g_region_types[8];                         // 0x59394c (terrain type ids in drop-down order; defined in char_sheet/terrain_page.cpp)

#endif // MB_GAME_TERRAIN_PAGE_H
