#include "./save_game.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)

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
