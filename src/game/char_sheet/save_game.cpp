#include "../../include/image_addrs.h"
#include "./save_game.h"
#include "../window_mgr.h"      // g_windows slots 18 (0xa739c8), 21 (0xa739d4)
#include "../tstring.h"
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../window_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../out_stream.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../triggers/item_rec.h"       // ItemRec2: the escape menu record (slot 3 is buttons 0x4e87d0)
#include "../mission_source.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)

#define W(x) ((MissionUi*)(void*)(x))
// @ 0x4e87d0    ItemRec2::frame_update(float)
void ItemRec2::frame_update(float arg)
{
    ((MissionSource*)&owned)->pick((int*)&arg);
    bool save = false;
    if (W(row1)->clear_mask(0x10000))
        save = true;
    if (!save && (row5 == 0 || !W(row5)->clear_mask(0x10000))) {
        if (W(row2)->clear_mask(0x10000)) {
                *(int*)(slot_as<char>(18) + 0x18) = 5;
                g_windows.set_state(0x12);
                return;
            }
            if (row3 != 0 && W(row3)->clear_mask(0x10000)) {
                save_game(-1, 0);
                {
                    tstring msg("ui_game_saved");
                    g_gui.add(*g_strtable.lookup(msg), -1);
                }
                g_windows.set_state(w18);
                return;
            }
            if (row4 != 0 && W(row4)->clear_mask(0x10000)) {
                *(int*)(slot_as<char>(21) + 0x18) = 5;
                g_windows.set_state(0x15);
                return;
            }
            if (W(row0)->clear_mask(0x10000) || (char)((Slot*)(void*)&g_keyboard)->is_ready(1)) {
                g_windows.set_state(w18);
                return;
            }
            return;
    }
    g_game->sl_update_avg();
    if (save)
        save_game(-1, 0);
    if (g_game) {
        GameData* g = g_game;
        g->world_free();   // 0x492f40
        operator delete(g);
        g_game = 0;
    }
    g_windows.set_state(4);
}
#undef W

// ---- the file writer (vtable 0x54add8 = bin_writer; its constructor is inline here, stream_inline.h) ----
#include "../../rgl/stream.h"
#include "../../rgl/stream_inline.h"
__forceinline bin_writer::bin_writer() {}   // inline here (resmgr.cpp load_pack calls the out-of-line 0x417c10)

#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../../include/clock_table.h"

#include <string>
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)
std::string g_base64_chars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/", 0x40);   // 0x5aa380 the base64 alphabet (no reader in this source; built at startup)

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
    bin_writer w;
    sprintf(dirC, "");
    save_path(dirC);
    g = g_game;
    ps = g->module.c_str();
    mb_snprintf(pathD, 0x200, "%s%s\\new_game.sav", dirC, ps);
    LOG("Creating new savegame file...");
    r = ((text_file*)&w)->open_write(pathD);
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
                    rc = ((stream_base*)&w)->flush();
                    if (!rc)
                        rc = fclose(w.fp);
                    w.failed = 1;
                    w.is_open = 0;
                }
                r = rc;
                if (!r) {
                    WL_MARKER(0x0c)
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
                    WL_MARKER(0x0d)
                    if (!r) {
                        LOG("Savegame succeded...");
                        {
                            tstring msg("ui_game_saved_successfully");
                            g_gui.add(*g_strtable.lookup(msg), -1);
                        }
                        goto done;
                    }
                }
            }
        }
    }
    LOG("Savegame failed...");
    g_gui.add(tstring("Error: Unable to save game!!!!!"), 0xffff3333);
done:
    return r;
}

// @ 0x4e8d70    int SaveCtx::write(OutStream* out)
int SaveCtx::write(OutStream* out)
{
    struct { char magic[5]; } hdr = { "WRDR" };
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
    sprintf(dir, "");
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
    h->stamp = (float)(g_ticks[f13b18] - clock) * 1e-5f;
}

