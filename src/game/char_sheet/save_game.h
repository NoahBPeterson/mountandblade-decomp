#ifndef SAVE_GAME_H
#define SAVE_GAME_H
// Slice s004e85d0 (batch ds1): world-map screen init/frame/buttons and the save-file helpers it calls.
// Layouts follow the verified neighbouring slices (s004d5410, s004e2ea0, s00477ea0).
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include "rgl.h"

#define MB_NOTHROW throw()

// ---- tstring (see s00467bf0) ----------------------------------------------------------------------------
#include "../tstring.h"
#include "../str_table.h"
extern StrTable g_strtable;                             // 0x5e2a18

struct MsgLog {
    void add(tstring msg, int color);                   // 0x452c00 (ret 0x14)
};
extern MsgLog g_msglog;                                 // 0x5d7080

struct log_stream {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void s8(); virtual void s9();
    virtual void w_cstr(const char* s);                 // +0x28
};
#define LOG(s) ((log_stream*)0x5a9f60)->w_cstr(s)          // 0x5a9f60 (virtual call through the object's vptr)

bool print_and_remove(const char* path);                // 0x539023
int  rename_file(char* from, const char* to);           // 0x538ff5
extern "C" int __cdecl crt_sprintf(char* buf, const char* fmt, ...);   // 0x536f8f
extern "C" void __cdecl crt_free(void* p);              // 0x537168
extern "C" int _fltused = 0;
void mb_snprintf(char* out, unsigned size, const char* fmt, ...);   // 0x415100
void save_dir(char* buf);                               // 0x4dd7b0
int  open_text_write(void* stream, char* path);         // 0x415320

// ---- widgets --------------------------------------------------------------------------------------------
struct Widget {
    virtual void m00(); virtual void m01(); virtual void m02(); virtual void m03();
    virtual void m04(); virtual void m05(); virtual void m06(); virtual void m07();
    virtual void m08(); virtual void m09(); virtual void m0a(); virtual void m0b();
    virtual void m0c(); virtual void m0d(); virtual void m0e(); virtual void m0f();
    virtual void m10();
    virtual char used(int flag);                        // +0x44
};

struct MainMenu {
    char pad0[0x18];
    int  sel;                                           // +0x18
    void choose(int option);                            // 0x495700 (this = 0xa73980, ret 4)
};
extern MainMenu g_main_menu;                            // 0xa73980

// ---- serialisation streams ------------------------------------------------------------------------------
struct OutStream;

struct OutStream {
    virtual void s00(); virtual void s01(); virtual void s02();
    virtual void w_int(int* p);                         // +0xc
    virtual void s04(); virtual void s05(); virtual void s06(); virtual void s07();
    virtual void w_raw(const void* p);                  // +0x20
    virtual void s09(); virtual void s0a();
    virtual OutStream* w_struct(const void* p);         // +0x2c
    virtual void s0d(); virtual void s0e(); virtual void s0f();
    virtual int  close();                               // +0x3c
    virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14();
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18();
    virtual void s19(); virtual void s1a(); virtual void s1b(); virtual void s1c();
    virtual void s1d(); virtual void s1e(); virtual void s1f(); virtual void s20();
    virtual void s21(); virtual void s22(); virtual void s23(); virtual void s24();
    virtual void s25(); virtual void s26(); virtual void s27(); virtual void s28();
    virtual void s29(); virtual void s2a(); virtual void s2b(); virtual void s2c();
    virtual void s2d(); virtual void s2e(); virtual void s2f(); virtual void s30();
    virtual void s31(); virtual void s32(); virtual void s33(); virtual void s34();
    virtual void s35(); virtual void s36(); virtual void s37(); virtual void s38();
    virtual void s39(); virtual void s3a();
    virtual int  open(char* path);                      // +0x68
};

struct SaveWriter {
    OutStream* vtbl;                                    // +0x00
    char       pad04[0x14 - 0x04];
    char       failed;                                  // +0x14
    char       pad15[0x2c - 0x15];
    tstring    slot;                                    // +0x2c
    char       pad3c[0x40 - 0x3c];
    char       closed;                                  // +0x40
    char       released;                                // +0x41
};
void copy_ctor(void* dst, const void* src);             // 0x42c110

// ---- the game object (see s00473a60) ---------------------------------------------------------------------
struct SaveCtx;
#pragma pack(push, 4)
struct GameRecord {
    tstring name;                                       // +0x00
    char    pad10[0x20 - 0x10];
    tstring title;                                      // +0x20
    char    pad30[0xb0 - 0x30];
    int     f0b0;                                       // +0xb0
    char    padb4[0x508 - 0xb4];
};

struct Game {
    int          version;                               // +0x0
    int          pad04;                                 // +0x4
    tstring      title;                                 // +0x8
    int          unk18;                                 // +0x18
    char         pad1c[0x20 - 0x1c];
    char*        save_slot;                             // +0x20
    char         pad24[0x8c - 0x24];
    int          f8c;                                   // +0x8c
    char         pad90[0x13a00 - 0x90];
    GameRecord*  records;                               // +0x13a00
    char         pad13a04[0x13a50 - 0x13a04];
    char*        parties;                               // +0x13a50
    char         pad13a54[0x13b18 - 0x13a54];
    int          f13b18;                                // +0x13b18
    char         pad13b1c[0x13b20 - 0x13b1c];
    __int64      clock;                                 // +0x13b20
    void load_header(SaveCtx* h);                       // 0x4e8e70 (ret 4)
    int save_state(OutStream* out);                     // 0x4e8f10 (later slice)
    void sl_update_avg();                               // 0x48a000
    void world_free();                                  // 0x492f40
};
#pragma pack(pop)
extern Game* g_game;                                    // 0x5e2930
extern __int64 g_ticks[2];                              // 0x5e2948
extern float g_54a6d0;                                  // 0x54a6d0
extern int   g_59811c;                                  // 0x59811c ("WRDR")
extern char  g_598120;                                  // 0x598120
extern int   g_5e29c0;                                  // 0x5e29c0

struct SaveCtx {
    tstring title;                                      // +0x0
    tstring slot;                                       // +0x10
    int     flag;                                       // +0x20
    float   stamp;                                      // +0x24
    int     pad28[3];                                   // +0x28 (unused by the writer and the header reader)
    int write(OutStream* out);                          // 0x4e8d70 (ret 4)
};

// ---- the map screen -------------------------------------------------------------------------------------
struct MapScreen;
struct MissionSource {
    void pick(const int* arg);                          // 0x4d5410 (ret 4)
};
struct MapMesh {
    char build(void* grid, int mode);                   // 0x4785e0 (ret 8)
    void f1(int v);                                     // 0x4e75a0 (ret 4)
    void f2(int v);                                     // 0x4e6a70 (ret 4)
    void f3(int v);                                     // 0x4e65e0 (ret 4)
};

struct MapScreen {
    void*        view;                                  // +0x00
    char         pad04[0x18 - 0x04];
    int          menu_sel;                              // +0x18
    char         pad1c[0x30 - 0x1c];
    MissionSource source;                               // +0x30
    char         pad31[0x60 - 0x31];
    Widget*      f60;                                   // +0x60
    Widget*      f64;                                   // +0x64
    Widget*      f68;                                   // +0x68
    Widget*      f6c;                                   // +0x6c
    Widget*      f70;                                   // +0x70
    Widget*      f74;                                   // +0x74
    char         pad78[0x278 - 0x78];
    int          ncols;                                 // +0x278
    int          nrows;                                 // +0x27c
    char         pad280[0x288 - 0x280];
    char         cells[1];                              // +0x288 (stride 0x94, row 0x911c)
    char         pad5df4[0x5df4 - 0x289];
    int          f5df4;                                 // +0x5df4
    char         pad5df8[0x6698 - 0x5df8];
    MapMesh      mesh;                                  // +0x6698
    char         pad66a0[0x672c - 0x6698 - 4];
    int          f672c;                                 // +0x672c
    int          f6730;                                 // +0x6730
    int          f6734;                                 // +0x6734
    char         pad6738[0xc3b0dc - 0x6738];
    char*        mapview;                               // +0xc3b0dc
    char         padview[0x845d984 - 0xc3b0dc - 4];
    int          nchunk_rows;                           // +0x0845d984
    int          nchunk_cols;                           // +0x0845d988

    char build_chunk(int slot, int type, int row, int col, float f);   // 0x47ff60 (ret 0x14)
    void gen_objects_aligned(void* desc);               // 0x4656c0 (ret 4)
    void init(void* desc);                              // 0x4e85d0 (ret 4)
    void frame_step();                                  // 0x4e8720
    void buttons(int arg);                              // 0x4e87d0 (ret 4)
};

// ---- callees --------------------------------------------------------------------------------------------
struct TerrainDesc { void init(); };                    // 0x464400 (ret 0)
int  save_game(int arg, tstring* name);                 // 0x4e8980
struct ButtonState { char test(int v); };               // 0x487900 (ret 4)
extern ButtonState g_a170c8;                            // 0xa170c8
void log_save_path();                                   // 0x4e8df0

extern int      g_use_shaders;                          // 0x5e28fc
extern unsigned g_mask_w;                               // 0x5a8e78
extern unsigned g_a76644;                               // 0xa76644
extern int      g_a739c8;                               // 0xa739c8
extern int      g_a739d4;                               // 0xa739d4
extern float    g_chunk_scale;                          // 0x549d68

#endif
