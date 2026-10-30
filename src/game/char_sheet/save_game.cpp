#include "../../include/image_addrs.h"
#include "./save_game.h"
#include "../tstring.h"
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../main_menu.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../out_stream.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../mission_source.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4e87d0    MapScreen::buttons(int arg)
void MapScreen::buttons(int arg)
{
    source.pick(&arg);
    bool save = false;
    if (!f64->changed(0x10000)) {
        if (f74 == 0 || !f74->changed(0x10000)) {
            if (f68->changed(0x10000)) {
                *(int*)(g_a739c8 + 0x18) = 5;
                g_main_menu.set_state(0x12);
                return;
            }
            if (f6c != 0 && f6c->changed(0x10000)) {
                save_game(-1, 0);
                g_msglog.add(*g_strtable.lookup(tstring("ui_game_saved")), -1);
                g_main_menu.set_state(menu_sel);
                return;
            }
            if (f70 != 0 && f70->changed(0x10000)) {
                *(int*)(g_a739d4 + 0x18) = 5;
                g_main_menu.set_state(0x15);
                return;
            }
            if (f60->changed(0x10000) || (char)((Slot*)(void*)&g_keyboard)->is_ready(1)) {
                g_main_menu.set_state(menu_sel);
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
        GameData* g = g_game;
        ((GameW*)g)->world_free();   // 0x492f40
        operator delete(g);
        g_game = 0;
    }
    g_main_menu.set_state(4);
}

// ---- the file writer (stream classes of s00417850/mbbin.h: vtable 0x54a740 -> 0x54add8) ----------------
// This file's own tstring-based copy of the stream root (rgl/stream.h cannot be included here: see stream_fwd.h).
#include "../../rgl/stream_fwd.h"
struct stream_root {
    virtual void reset();                               // slot 0
    virtual void rewind();                              // slot 1
    STREAM_SLOTS_OF(stream_root, rgl::string, = 0)
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
#undef SF_RT
#define SF_RT stream_root
struct file_writer : file_writer_base {
    STREAM_SLOTS_OF(stream_root, rgl::string, )
    virtual int flush();
};
STREAM_FWD_BIN_WRITER(file_writer)
int file_writer::flush() { return ((bin_writer*)this)->bin_writer::flush(); }

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
    GameData* g = g_game;
    const char* ps = g->module.c_str();
    int ver = g->version;
    sprintf(dirA, "");
    save_path(dirA);
    mb_snprintf(pathB, 0x200, "%s%s\\sg%d%d.sav", dirA, ps, ver / 10, ver % 10);
    file_writer w;
    sprintf(dirC, "");
    save_path(dirC);
    g = g_game;
    ps = g->module.c_str();
    mb_snprintf(pathD, 0x200, "%s%s\\new_game.sav", dirC, ps);
    LOG("Creating new savegame file...");
    r = ((text_stream*)&w)->open_write(pathD);
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
                    save_path(dirA);
                    g = g_game;
                    ps = g->module.c_str();
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
                        g_msglog.add(*g_strtable.lookup(tstring("ui_game_saved_successfully")), -1);
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
    out->w_raw(&g_module_version);
    out->w_struct(this);
    out->w_struct(&slot).w_raw(&flag);
    out->w_int((int*)&stamp);
    return 0;
}

// @ 0x4e8df0    log_save_path()
void log_save_path()
{
    char dir[260];
    char path[512];
    crt_sprintf(dir, "");
    save_path(dir);
    CreateDirectoryA(dir, NULL);
    mb_snprintf(path, 0x200, "%s%s", dir, g_game->module.c_str());
    CreateDirectoryA(path, NULL);
}

// @ 0x4e8e70    void GameData::load_header(SaveCtx* h)   (thiscall: this = g, ret 4)
void GameData::load_header(SaveCtx* h)
{
    Troop* rec = records + f8c;
    tstring* name = g_strtable.find(&rec->key);
    if (!name)
        name = &rec->name;
    h->slot = *name;
    if (title.len == 0)
        h->title = h->slot;
    else
        h->title = title;
    h->flag = rec->fb0;
    h->stamp = (float)(g_ticks[f13b18] - clock) * g_54a6d0;
}

// ItemRec2 slot 3 (0x4e87d0 MapScreen::buttons).
#include "../triggers/item_rec.h"
void ItemRec2::screen_buttons() { ((MapScreen*)this)->buttons(0); }
