// options.h: the game options record (global at 0x5aa008), split out of resource_manager.h so headers that need only
// g_options do not pull in the engine log and render globals of resource_manager.h.
#ifndef RGL_OPTIONS_H
#define RGL_OPTIONS_H

// ---- game options (global at 0x5aa008) --------------------------------------------------------
// Field names provisional. Serialised by options::save() (options.dat) in the order of the code; floats go
// through slot 3, ints through slot 8.
struct options {
    int   i00, i04;
    float f08, f0c, f10;
    int   i14, i18, i1c, i20, i24;
    int   i28;                 // 0x5aa030: cleared when a render-target texture cannot be created
    int   i2c;                 // 0x5aa034: cleared when the environment shadow map cannot be created
    int   i30, i34, i38, i3c, i40, i44, i48, i4c, i50, i54, i58, i5c, i60, i64, i68, i6c, i70;
    float f74;
    int   i78, i7c;
    float f80, f84, f88;
    int   i8c, i90, i94, i98;
    void save();               // 0x4181c0
};
extern options g_options;                      // 0x5aa008

#endif
