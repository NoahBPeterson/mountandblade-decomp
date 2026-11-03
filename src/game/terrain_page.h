// terrain_page.h: the terrain "edit" page (0x2c0 bytes; a polymorphic page whose slot 0 is page_update 0x4f2d70): the
// checkboxes / sliders / number fields that edit the terrain bit words at 0xa76630 and the preview camera (the page's
// +0x6c world pointer, the 0x120-byte map block at +0x70 and the camera state). ONE definition for char_sheet:
// terrain_page (the page refresh, captions, hex text, bit sync: 0x4f2730..0x4f2ae0), terrain_page_update (0x4f2d70, 0x4f34c0)
// and terrain_page_input (0x4f3570..0x4f3cf0). The three slices named the same offsets differently; the names below follow
// the update/input slices (checkbox / camera spelling) and say which offsets the slices read in two ways.
// Needs `tstring` (for Widget) declared by the includer.
#ifndef MB_GAME_TERRAIN_PAGE_H
#define MB_GAME_TERRAIN_PAGE_H
#include "./ui_widget.h"

struct World;                                               // the preview scene (each TU declares its own view of it)

// One plain virtual in the class: a class with no virtuals at all makes cl 13.10 lay out every *later*
// polymorphic class with a vfptr+vbtable-style prefix, so the Widget slots come out 0x20 too high.
struct Paged {
    virtual void page_update(float dt);                     // 0x4f2d70
};

struct TerrainPage : Paged {
    char    pad04[0x18 - 0x04];
    int     option;                                         // +0x18 (the option the menu returns to)
    char    pad1c[0x2c - 0x1c];
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
    char    pad68[4];                                       // +0x68
    World*  world6c;                                        // +0x6c  (terrain_page_input: the page's world pointer)
    int     map_block[(0x138 - 0x70) / 4];                  // +0x70  the 0x120-byte map block (runs on through frame[]; frustum source at
                                                            //        +0xf8 = block+0x88; copied to world +0x1ac)
    float   frame[0x10];                                    // +0x138  the frame path() builds (from a temp)
    char    pad178[0x18];                                   // +0x178
    float   view_x;                                         // +0x190  eye X (clamped to 0..250)
    float   view_y;                                         // +0x194  eye Y (clamped to 0..250)
    float   view_z;                                         // +0x198  eye Z
    char    pad19c[0x1a0 - 0x19c];
    float   yaw;                                            // +0x1a0  camera heading
    float   pitch;                                          // +0x1a4  camera pitch (0.1..1.5)
    float   zoom;                                           // +0x1a8  camera zoom (1.5..400.5)
    World*  world;                                          // +0x1ac  the scene object the page drives (terrain_page: obj1ac, used by 0x4f2d70 only)
    int     f1b0;                                           // +0x1b0  (unknown; 0x4f2960 reads the text box at +0x1b4)
    Widget* hexbox;                                         // +0x1b4  the hex terrain text field
    Widget* copy_btn;                                       // +0x1b8  copy terrain code
    Widget* paste_btn;                                      // +0x1bc  paste terrain code
    Widget* x_slider;                                       // +0x1c0  X position slider (a range widget: set_value is slot +0x34)
    Widget* y_slider;                                       // +0x1c4  Y position slider
    Widget* z_slider;                                       // +0x1c8  camera height slider
    Widget* fog_slider;                                     // +0x1cc  fog slider
    Widget* level_slider;                                   // +0x1d0  water level slider
    Widget* height_slider;                                  // +0x1d4  water height slider
    char    pad1d8[0x200 - 0x1d8];
    Widget* bit_fields[3];                                  // +0x200  number fields of terrain slot 0..2 (num2)
    Widget* layer_boxes[3];                                 // +0x20c  select checkbox / bit-field strip of terrain slot 0..2 (box3)
    Widget* water_box;                                      // +0x218  Enable water (w218)
    Widget* river_box;                                      // +0x21c  Enable river (w21c)
    Widget* terrain_box;                                    // +0x220  Enable terrain (w220)
    Widget* terraingen_box;                                 // +0x224  Enable terrain generation (w224)
    Widget* type_box;                                       // +0x228  terrain type (w228)
    Widget* size_box;                                       // +0x22c  terrain size (w22c)
    Widget* regen_btn;                                      // +0x230  regenerate (w230)
    Widget* back_btn;                                       // +0x234  back (w234)
    char    pad238[0x2c0 - 0x238];

    void update();                                          // 0x4f2730 (terrain_page: refresh every widget from the page fields, then the hex field)
    void update(float dt);                                  // 0x4f2d70 (terrain_page_update; the Paged slot-0 function)
    __declspec(noinline) void set_enabled_text(Widget* w);                       // 0x4f28a0
    __declspec(noinline) void set_hex3_text(int i);                              // 0x4f28f0
    void refresh_hex();                                     // 0x4f2960
    bool changed(Widget* w) { return w->changed(0x10000); } // +0x44 (the page's change flag)
    void sync_bits();                                       // 0x4f2ae0
    void copy_clipboard();                                  // 0x4f34c0
    void paste_clipboard();                                 // 0x4f3c30
    void read_fields();                                     // 0x4f3570
    void simulate(float dt);                                // 0x4f37f0
    void path(float dt);                                    // 0x4f3cf0
};

#endif // MB_GAME_TERRAIN_PAGE_H
