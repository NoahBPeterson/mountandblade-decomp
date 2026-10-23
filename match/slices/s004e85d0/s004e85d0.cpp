// Slice s004e85d0 (batch ds1): world-map screen init/frame/buttons and the save-file helpers it calls.
// Layouts follow the verified neighbouring slices (s004d5410, s004e2ea0, s00477ea0).
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include "rgl.h"

#define MB_NOTHROW throw()

// ---- tstring (see s00467bf0) ----------------------------------------------------------------------------
#pragma pack(push, 4)
struct tstring {
    int pos;                                            // +0x0
    char* buf;                                          // +0x4
    int cap;                                            // +0x8
    int len;                                            // +0xc
    tstring() MB_NOTHROW;                               // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;                  // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;               // 0x42c110
    ~tstring() MB_NOTHROW;                              // 0x42bc40
    void operator=(const tstring& o) MB_NOTHROW;        // 0x42bc50
};
#pragma pack(pop)
struct StrTable {
    tstring* find(const tstring* key) MB_NOTHROW;       // 0x4c6010 (0 when absent)
    tstring* lookup(const tstring* key) MB_NOTHROW;     // 0x43a1e0 (this = 0x5e2a18)
};
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

// @ 0x4e85d0    MapScreen::init(void* desc)   (thiscall, ret 4)
void MapScreen::init(void* desc)
{
    ((TerrainDesc*)desc)->init();
    int r;
    for (r = 0; r <= nrows; r++) {
        float* p = (float*)(cells + 0x84 + r * 0x94);
        int i;
        for (i = 0; i <= ncols; i++, p = (float*)((char*)p + 0x911c)) {
            ((int*)p)[1] = 0;
            ((int*)p)[2] = -1;
            p[-0x1f] = p[1] + p[0];
            int* z = (int*)((char*)p - 0x2c);
            int k;
            for (k = 0; k < 11; k++)
                z[k] = 0;
        }
    }
    rgl::vector4 box = *(rgl::vector4*)(mapview + 0x68);
    float f = box.z + box.y;
    f = (f + box.x) * g_chunk_scale;
    int row;
    for (row = 0; row < nchunk_rows; row++) {
        int col;
        for (col = 0; col < nchunk_cols; col++) {
            int slot;
            for (slot = 0; slot < 0xf; slot++) {
                int type = -1;
                if (g_use_shaders && f60) {
                    if (slot == 1)
                        type = 2;
                    else if (slot == 2)
                        continue;
                }
                build_chunk(slot, type, row, col, f);
            }
        }
    }
    gen_objects_aligned(desc);
}

// @ 0x4e8720    MapScreen::frame_step()
void MapScreen::frame_step()
{
    if (!(*(unsigned*)(g_game->parties + f5df4 * 0x494 + 0x30) & 0x100))
        return;
    int mode = 3;
    if (((g_mask_w - 1) & g_a76644) != 0 &&
        ((g_a76644 & (g_mask_w - 1)) == 1 || (g_a76644 & (g_mask_w - 1)) == 2 ||
         ((g_mask_w - 1) & g_a76644) == 3))
        mode = 2;
    mesh.build(*(void**)((char*)view + 0x64), mode);
    mesh.f1(f672c);
    mesh.f2(f6734);
    mesh.f3(f6730);
}

// @ 0x4e87d0    MapScreen::buttons(int arg)
void MapScreen::buttons(int arg)
{
    source.pick(&arg);
    bool save = false;
    if (!f64->used(0x10000)) {
        if (f74 == 0 || !f74->used(0x10000)) {
            if (f68->used(0x10000)) {
                *(int*)(g_a739c8 + 0x18) = 5;
                g_main_menu.choose(0x12);
                return;
            }
            if (f6c != 0 && f6c->used(0x10000)) {
                save_game(-1, 0);
                g_msglog.add(*g_strtable.lookup(&tstring("ui_game_saved")), -1);
                g_main_menu.choose(menu_sel);
                return;
            }
            if (f70 != 0 && f70->used(0x10000)) {
                *(int*)(g_a739d4 + 0x18) = 5;
                g_main_menu.choose(0x15);
                return;
            }
            if (f60->used(0x10000) || g_a170c8.test(1)) {
                g_main_menu.choose(menu_sel);
                return;
            }
            return;
        }
    } else {
        save = true;
    }
    g_game->sl_update_avg();
    if (save)
        save_game(-1, 0);
    if (g_game) {
        Game* g = g_game;
        g->world_free();
        operator delete(g);
        g_game = 0;
    }
    g_main_menu.choose(4);
}

// ---- the file writer (stream classes of s00417850/mbbin.h: vtable 0x54a740 -> 0x54add8) ----------------
struct stream_root {
    virtual void reset();                               // slot 0
    virtual void rewind();                              // slot 1
    virtual void w_stream(void* o) = 0;                 // 2 .. 22 (declared as one block here)
    virtual void s3() = 0; virtual void s4() = 0; virtual void s5() = 0; virtual void s6() = 0;
    virtual void s7() = 0; virtual void s8() = 0; virtual void s9() = 0; virtual void s10() = 0;
    virtual void s11() = 0; virtual void s12() = 0; virtual void s13() = 0; virtual void s14() = 0;
    virtual void s15() = 0; virtual void s16() = 0; virtual void s17() = 0; virtual void s18() = 0;
    virtual void s19() = 0; virtual void s20() = 0; virtual void s21() = 0; virtual void s22() = 0;
    virtual bool at_end();                              // 23
    virtual void set_autoflush(char f);                 // 24
    virtual char get_autoflush();                       // 25
    virtual int flush();                                // 26 (+0x68)
    tstring name;                                       // +4
    int state;                                          // +0x14
    char autoflush;                                     // +0x18
    int flushed;                                        // +0x1c
    tstring buf;                                        // +0x20
    stream_root() { autoflush = 0; flushed = 0; }
};
struct file_writer_base : stream_root {
    FILE* fp;                                           // +0x30
    char failed;                                        // +0x34
    char is_open;                                       // +0x35
    file_writer_base() { failed = 1; is_open = 0; state = 0; }
};
struct file_writer : file_writer_base {
    virtual void w_stream(void* o);
    virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6();
    virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10();
    virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14();
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18();
    virtual void s19(); virtual void s20(); virtual void s21(); virtual void s22();
    virtual int flush();
};
struct TextOps { int open_write(const char* path); };   // 0x415320 (ret 4)

#define WL_MARK(id) \
    __asm _emit 0xEB __asm _emit 0x10 __asm _emit 0x57 __asm _emit 0x4C __asm _emit 0x20 __asm _emit 0x20 \
    __asm _emit id __asm _emit 0 __asm _emit 0 __asm _emit 0 __asm _emit 0 __asm _emit 0 __asm _emit 0 \
    __asm _emit 0 __asm _emit 0x57 __asm _emit 0x4C __asm _emit 0x20 __asm _emit 0x20

// @ 0x4e8980    int save_game(int version, tstring* name)
int save_game(int version, tstring* name)
{
    // the five path buffers keep this relative order in the original frame
    struct { char dirA[0x104]; char dirC[0x104]; char pathD[0x200]; char pathB[0x200]; char pathE[0x200]; } L;
#define dirA L.dirA
#define dirC L.dirC
#define pathD L.pathD
#define pathB L.pathB
#define pathE L.pathE
    int r;
    if (version >= 0)
        g_game->version = version;
    LOG("Saving Game...");
    log_save_path();
    Game* g = g_game;
    char* ps = g->save_slot;
    if (!ps)
        ps = (char*)&g->save_slot;
    int ver = g->version;
    sprintf(dirA, "");
    save_dir(dirA);
    mb_snprintf(pathB, 0x200, "%s%s\\sg%d%d.sav", dirA, ps, ver / 10, ver % 10);
    file_writer w;
    sprintf(dirC, "");
    save_dir(dirC);
    g = g_game;
    ps = g->save_slot ? g->save_slot : (char*)&g->save_slot;
    mb_snprintf(pathD, 0x200, "%s%s\\new_game.sav", dirC, ps);
    LOG("Creating new savegame file...");
    r = ((TextOps*)&w)->open_write(pathD);
    if (!r) {
        LOG("Saving header...");
        {
            SaveCtx hdr;
            g_game->load_header(&hdr);
            if (name)
                hdr.title = *name;
            r = hdr.write((OutStream*)&w);
        }
        if (!r) {
            LOG("Saving game state...");
            r = g_game->save_state((OutStream*)&w);
            if (!r) {
                LOG("Closing new save game file...");
                int rc = 1;
                if (w.is_open) {
                    rc = ((stream_root*)&w)->flush();
                    if (!rc)
                        rc = fclose(w.fp);
                    w.failed = 1;
                    w.is_open = 0;
                }
                r = rc;
                if (!r) {
                    WL_MARK(0x0c)
                    LOG("Backing up old file name...");
                    sprintf(dirA, "");
                    save_dir(dirA);
                    g = g_game;
                    ps = g->save_slot ? g->save_slot : (char*)&g->save_slot;
                    mb_snprintf(pathE, 0x200, "%s%s\\last_savegame_backup.sav", dirA, ps);
                    LOG("Removing old backup file.");
                    remove(pathE);
                    LOG("Renaming old savegame to backup file.");
                    rename(pathB, pathE);
                    LOG("Renaming new savegame file.");
                    r = rename(pathD, pathB);
                    WL_MARK(0x0d)
                    if (!r) {
                        LOG("Savegame succeded...");
                        g_msglog.add(*g_strtable.lookup(&tstring("ui_game_saved_successfully")), -1);
                        goto done;
                    }
                }
            }
        }
    }
    LOG("Savegame failed...");
    g_msglog.add(tstring("Error: Unable to save game!!!!!"), 0xffff3333);
done:
    return r;
}

// @ 0x4e8d70    int SaveCtx::write(OutStream* out)
int SaveCtx::write(OutStream* out)
{
    struct { int magic; char zero; } hdr = { g_59811c, g_598120 };
    out->w_raw(&hdr);
    int version = 0x3f3;
    out->w_raw(&version);
    out->w_raw(&g_5e29c0);
    out->w_struct(this);
    out->w_struct(&slot)->w_raw(&flag);
    out->w_int((int*)&stamp);
    return 0;
}

// @ 0x4e8df0    log_save_path()
void log_save_path()
{
    char dir[260];
    char path[512];
    crt_sprintf(dir, "");
    save_dir(dir);
    CreateDirectoryA(dir, NULL);
    char* slot = g_game->save_slot;
    mb_snprintf(path, 0x200, "%s%s", dir, slot ? slot : (char*)&g_game->save_slot);
    CreateDirectoryA(path, NULL);
}

// @ 0x4e8e70    void Game::load_header(SaveCtx* h)   (thiscall: this = g, ret 4)
void Game::load_header(SaveCtx* h)
{
    GameRecord* rec = records + f8c;
    tstring* name = g_strtable.find(&rec->title);
    if (!name)
        name = &rec->name;
    h->slot = *name;
    if (title.len == 0)
        h->title = h->slot;
    else
        h->title = title;
    h->flag = rec->f0b0;
    h->stamp = (float)(g_ticks[f13b18] - clock) * g_54a6d0;
}
