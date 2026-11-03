#include "./save_panel.h"
namespace rgl {
struct mesh_ext {                                   // rgl/scene/scene.h: the mesh object with the scene-side members
    void clear_geometry();                          // 0x428e80
};
}
#include "../window_mgr.h"      // g_windows slot 3 (0xa7398c): the save dialog
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4ea0c0
void __stdcall select_entity(mesh* m, skin_owner* o, unsigned color)
{
    ((rgl::mesh_ext*)m)->clear_geometry();   // 0x428e80
    if (!o->skins)
        return;
    float prod = o->f160;
    prod *= o->f15c;
    prod *= o->f158;
    char flip = 0;
    if (prod < 0.0f)
        flip = 1;
    for (int i = 0; ; i++) {
        if (!o->skins->begin)
            break;
        if ((unsigned)i >= (unsigned)(o->skins->end - o->skins->begin))
            break;
        mat3 tmp;
        // the original (0x4ea147..0x4ea241) multiplies a full mat4 (rows from identity3, origin zeroed): 0x40e870 reads the
        // left operand's origin (+0x30..+0x38), so it must be zero, not the stack bytes after a bare mat3
        mat4 id;
        *(mat3*)&id = *identity3(&tmp);
        id.o.x = 0.0f; id.o.y = 0.0f; id.o.z = 0.0f;
        id.s.x *= o->f158; id.s.y *= o->f158; id.s.z *= o->f158;
        id.f.x *= o->f15c; id.f.y *= o->f15c; id.f.z *= o->f15c;
        id.u.x *= o->f160; id.u.y *= o->f160; id.u.z *= o->f160;
        skin_rec* rec = o->skins->begin[i];
        mat4 M = id * rec->mat;
        m->append_skinned((mesh*)rec, &M, -1, color, flip);
    }
}

// ---- widget virtual slots: the canonical Widget view (src/game/ui_widget.h): +0x2c activate (the cursor on/off),
// +0x44 changed (input test), +0xb8 disable, +0xbc enable (the harness types the widget pointers through its vtable)
#include "../ui_widget.h"
typedef Widget Control;

#include "../screen_records.h"       // C4c31c0: the save / load window (slot 3 is update 0x4ea290, select 0x4ea850)

#include "../str_table.h"
#include "../../include/msg_log.h"
extern MsgLog g_msglog;                                         // 0x5d7080
void mb_snprintf(char* out, unsigned size, const char* fmt, ...);  // 0x415100
int  __cdecl crt_sprintf(char* buf, const char* fmt, ...);      // 0x536f8f
int  __cdecl crt_remove(const char* path);                      // 0x539023
void save_path(char* out);                                      // 0x4dd7b0
int  save_game(int arg, tstring* name);                         // 0x4e8980
extern char* g_game;                                            // 0x5e2930
extern char g_main_menu_obj[];                                  // 0xa73980

// main menu layout (see s004d7d10)
#include "../main_menu.h"
#include "../key_names.h"      // g_keyboard (0xa170c8)
#include "../game_globals.h"      // canonical global declarations (g_module_name ...)
#define g_main_menu (*(main_menu*)g_main_menu_obj)
// The menu record's tstrings/fields live on the object g_main_menu.rec points at, NOT on the screen manager itself:
// the manager (0xa73980) is 30 screen pointers plus the open-screen deque at +0x78, so writing the record's
// +0x78/+0x88 answers (or constructing them) lands on the deque and destroys it (crash in current_screen_id).
#define g_menu_rec (g_main_menu.rec)

static const char* text_of(const tstring* s)
{
    char* b = s->buf;
    return b ? b : (const char*)&s->buf;
}

// @ 0x4ea290
void C4c31c0::scale_3(float)
{
    // save_path reads this as a C string, then WRITES the whole "%s\Mount&Blade Savegames\%s" path back into
    // it (its own local buffer is 0x104), so this is a char buffer of that size - the 16 bytes it used to have
    // were overrun, corrupting the neighbouring `name` buffer (crash 0x73726573 = the tail of "...\users").
    char pathbuf[0x200];
    char name[0x200];
    for (int i = 0; i < 9; i++) {
        Widget* button = labels[i];
        if (button && button->changed(0x10000))
            select(i);
    }
    if (btn_done->changed(0x10000)) {
        if (is_load) {
        *(int*)((char*)this + 0x220) = -1;
        tstring title("ui_set_save_file_name");
        const tstring* r = g_strtable.lookup(title);
        mb_snprintf(name, 0x200, text_of(r));   // original passes the text itself as the format (0x4ea368, 0x4ea776)
        tstring cancel("ui_cancel");
        tstring ok("ui_ok");
        tstring title2(name);
        const tstring* rc = g_strtable.lookup(cancel);
        const tstring* ro = g_strtable.lookup(ok);
        g_menu_rec->header.clear();
        // The original (0x4ea40c..0x4ea42d) stores the prompt into +0x1c and the two answers into +0x78/+0x88:
        // +0x1c = the question, +0x78 = ui_ok, +0x88 = ui_cancel. Ours had +0x1c and +0x88 swapped.
        g_menu_rec->title = title2;
        g_menu_rec->answer_yes = *ro;
        g_menu_rec->answer_no = *rc;
        g_menu_rec->choice = (int)((char*)this + 0x220);
        g_menu_rec->v50 = (int)((char*)this + 0x224);
        int* slotname;
        if (*(int*)((char*)this + sel218 * 0x30 + 0x4c) != 0)
            slotname = (int*)((char*)this + sel218 * 0x30 + 0x2c);
        else
            slotname = (int*)(*(int*)(g_game + 0x8c) * 0x508 +
                              *(int*)(g_game + 0x13a00));
        ((tstring*)((char*)g_menu_rec + 0x54))->operator=(*(tstring*)slotname);
        g_menu_rec->open = 3;
        g_main_menu.set_state(0);
        return;
        } else {
        char* slot = (char*)this + sel218 * 0x30;
        if (slot[0x58] == 0 && slot[0x59] == 0 && slot[0x5a] == 0) {
            crt_sprintf(pathbuf, "");
            save_path(pathbuf);
            const char* path = pathbuf;
            const char* pf = g_module_name.c_str();
            mb_snprintf(name, 0x200, "%s%s\\sg%d%d.sav", path, pf,
                        sel218 / 10, sel218 % 10);
            *(int*)(slot_as<char>(3) + 4) = 1;
            ((tstring*)(slot_as<char>(3) + 0x48))->assign(name);
            *(int*)(slot_as<char>(3) + 0x8c) = sel218;
            g_main_menu.set_state(3);
            return;
        }
        }
    }
    if (btn_cancel->changed(0x10000)) {
        g_main_menu.set_state(*(int*)((char*)this + 0x18));
        return;
    }
    Widget* panel = btn_delete;
    // The original (verified by the byte-level reconstruction in match/slices/save_panel) reaches the delete
    // confirmation on the delete widget OR (a17420 set, a17820 clear, AND the panel's +0x104 bit 1 clear); the
    // remaining case is the save/remove path. Ours had that second test as an OR of the positive bit, which sent
    // the ordinary save/load click into the confirmation (equiv.py reported FAIL until this was ANDed).
    if (panel->changed(0x10000) ||
        (g_keyboard.down[0xd3] != 0 && g_keyboard.prev[0xd3] == 0 && ((*(unsigned char*)((char*)panel + 0x104) & 2) == 0)))
        goto confirm_delete;
    {
        // +0x21c is the dialog's answer slot (the confirm sets it to -1 before opening the box). The original
        // (0x4ea5f5: cmp [esi+0x21c], 0 / jne 0x4ea6bc) removes the file when the answer is 0 (ui_yes).
        if (*(int*)((char*)this + 0x21c) == 0) {
            crt_sprintf(pathbuf, "");
            save_path(pathbuf);
            const char* path = pathbuf;
            const char* pf = g_module_name.c_str();
            mb_snprintf(name, 0x200, "%s%s\\sg%d%d.sav", path, pf,
                        sel218 / 10, sel218 % 10);
            if (crt_remove(name) != 0) {
                tstring err("ui_error_removing_file");
                g_msglog.add(*g_strtable.lookup(err), 0xffff0000);
            }
            g_main_menu.set_state(0x15);
            return;
        }
        if (*(int*)((char*)this + 0x220) != 0)
            return;
        save_game(sel218, (tstring*)((char*)this + 0x224));
        ((tstring*)(g_game + 8))->operator=(*(tstring*)((char*)this + 0x224));
        tstring ok("ui_game_saved");
        g_msglog.add(*g_strtable.lookup(ok), -1);
        g_main_menu.set_state(*(int*)((char*)this + 0x18));
        return;
    }
confirm_delete:
    *(int*)((char*)this + 0x21c) = -1;
    tstring title("ui_confirm_delete_game");
    const tstring* r = g_strtable.lookup(title);
    mb_snprintf(name, 0x200, text_of(r));   // original passes the text itself as the format (0x4ea368, 0x4ea776)
    tstring no("ui_no");
    tstring yes("ui_yes");
    tstring title2(name);
    const tstring* rn = g_strtable.lookup(no);
    const tstring* ry = g_strtable.lookup(yes);
    g_menu_rec->header.clear();
    // Same shape as the save-name box (0x4ea7e4..0x4ea801): +0x1c = the question, +0x78 = ui_yes, +0x88 = ui_no.
    g_menu_rec->title = title2;
    g_menu_rec->answer_yes = *ry;
    g_menu_rec->answer_no = *rn;
    g_menu_rec->choice = (int)((char*)this + 0x21c);
    g_menu_rec->v50 = 0;
    g_menu_rec->open = 1;
    g_main_menu.set_state(0);
}

// @ 0x4ea850
void C4c31c0::select(int idx)
{
    if (sel218 != -1)
        labels[sel218]->activate(0);
    sel218 = idx;
    if (sel218 != -1) {
        labels[sel218]->activate(1);
        btn_done->enable();
        if (*(int*)((char*)this + sel218 * 0x30 + 0x4c))
            btn_delete->enable();
        else
            btn_delete->disable();
    } else {
        btn_done->disable();
        btn_delete->disable();
    }
}
