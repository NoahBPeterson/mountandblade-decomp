#include <direct.h>
#include <stdio.h>
#include "./save_panel.h"
#include "../window_mgr.h"      // g_windows slot 3 (0xa7398c): the save dialog
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)

// the loop bound reads the vector's pointers at +0x11c / +0x120 directly (the real size() materialises the vector's
// address first: add eax,0x118, which the original does not have); the elements go through the real operator[]
static bool more_skins(skin_data* s, int i)
{
    return s->recs._Myfirst && (unsigned)i < (unsigned)(s->recs._Mylast - s->recs._Myfirst);
}

// @ 0x4ea0c0
void __stdcall select_entity(mesh* m, skin_owner* o, unsigned color)
{
    m->clear_geometry();   // 0x428e80
    if (!o->skins)
        return;
    float prod = o->f160;
    prod *= o->f15c;
    prod *= o->f158;
    char flip = 0;
    if (prod < 0.0f)
        flip = 1;
    for (int i = 0; more_skins(o->skins, i); i++) {
        mat3 tmp;
        // the original (0x4ea147..0x4ea241) multiplies a full mat4 (rows from identity3, origin zeroed): 0x40e870 reads the
        // left operand's origin (+0x30..+0x38), so it must be zero, not the stack bytes after a bare mat3
        mat4 id;
        vector4 zero(0.0f, 0.0f, 0.0f);
        *(mat3*)&id = *identity3(&tmp);
        id.o = zero;
        id.s.x *= o->f158; id.s.y *= o->f158; id.s.z *= o->f158;
        id.f.x *= o->f15c; id.f.y *= o->f15c; id.f.z *= o->f15c;
        id.u.x *= o->f160; id.u.y *= o->f160; id.u.z *= o->f160;
        mat4 M = id * ((mesh*)o->skins->recs[i])->m_cc;
        m->append_skinned((mesh*)o->skins->recs[i], &M, -1, color, flip);
    }
}

// ---- widget virtual slots: the canonical MissionUi view (src/game/ui_widget.h): +0x2c activate (the cursor on/off),
// +0x44 changed (input test), +0xb8 disable, +0xbc enable (the harness types the widget pointers through its vtable)
#include "../ui_widget.h"
typedef MissionUi Control;

#include "../screen_records.h"       // C4c31c0: the save / load window (slot 3 is update 0x4ea290, select 0x4ea850)

#include "../str_table.h"
#include "../strat/gui_manager.h"
#include "../../rgl/stream_inline.h"   // bin_reader: the inline ctor/dtor the save path builds on its stack (0x4ea46f)
void save_path(char* out);                                      // 0x4dd7b0
int  save_game(int arg, tstring* name);                         // 0x4e8980



// main menu layout (see s004d7d10)
#include "../window_mgr.h"
#include "../key_names.h"      // g_keyboard (0xa170c8)
#include "../game_globals.h"      // canonical global declarations (g_module_name ...)
#include "../../rgl/text_stream.h"
struct GameData;
extern GameData* g_game;                                    // 0x5e2930
// The menu record's tstrings/fields live on the object g_windows.box() points at, NOT on the screen manager itself:
// the manager (0xa73980) is 30 screen pointers plus the open-screen deque at +0x78, so writing the record's
// +0x78/+0x88 answers (or constructing them) lands on the deque and destroys it (crash in current_screen_id).
#define g_menu_rec (g_windows.box())

static const char* text_of(const tstring* s)
{
    char* b = s->buf;
    return b ? b : (const char*)&s->buf;
}

// @ 0x4ea290
void C4c31c0::frame_update(float)
{
    // save_path reads this as a C string, then WRITES the whole "%s\Mount&Blade Savegames\%s" path back into
    // it (its own local buffer is 0x104), so this is a char buffer of that size - the 16 bytes it used to have
    // were overrun, corrupting the neighbouring `name` buffer (crash 0x73726573 = the tail of "...\users").
    for (int i = 0; i < 9; i++) {
        if (!labels[i])
            continue;
        MissionUi* button = labels[i];
        if (button->clear_mask(0x10000))
            select(i);
    }
    if (btn_done->clear_mask(0x10000) && (is_load || ((char*)this + sel218 * 0x30)[0x58] == 0
                                          && ((char*)this + sel218 * 0x30)[0x59] == 0
                                          && ((char*)this + sel218 * 0x30)[0x5a] == 0)) {
        char pathbuf[0x200];
        char name[0x200];
        if (is_load) {
            *(int*)((char*)this + 0x220) = -1;
            {
                tstring title("ui_set_save_file_name");
                const tstring* r = g_strtable.lookup(title);
                mb_snprintf(pathbuf, 0x200, text_of(r));
            }
            int* slotname;
            if (*(int*)((char*)this + sel218 * 0x30 + 0x4c) != 0)
                slotname = (int*)((char*)this + sel218 * 0x30 + 0x2c);
            else
                slotname = (int*)(*(int*)((char*)g_game + 0x8c) * 0x508 +
                                  *(int*)((char*)g_game + 0x13a00));
            {
                tstring cancel("ui_cancel");
                tstring ok("ui_ok");
                tstring title2(pathbuf);
                const tstring* rc = g_strtable.lookup(cancel);
                const tstring* ro = g_strtable.lookup(ok);
                NamedRec* rec = g_menu_rec;
                rec->name68.clear();
                // The original (0x4ea40c..0x4ea42d) stores the prompt into +0x1c and the two answers into +0x78/+0x88:
                // +0x1c = the question, +0x78 = ui_ok, +0x88 = ui_cancel.
                rec->name1c = title2;
                rec->name78 = *ro;
                rec->name88 = *rc;
                rec->f4c = (int)((char*)this + 0x220);
                rec->f50 = (int)((char*)this + 0x224);
                rec->name54 = *(tstring*)slotname;
                rec->f04 = 3;
            }
            g_windows.set_state(0);
        } else {
            bin_reader s;
            SlotHeader hdr;
            const char* pf = g_module_name.c_str();
            int sel = sel218;
            sprintf(pathbuf, "");
            save_path(pathbuf);
            mb_snprintf(name, 0x200, "%s%s\\sg%d%d.sav", pathbuf, pf, sel / 10, sel % 10);
            *(int*)(slot_as<char>(3) + 4) = 1;
            ((tstring*)(slot_as<char>(3) + 0x48))->assign(name);
            *(int*)(slot_as<char>(3) + 0x8c) = sel218;
            g_windows.set_state(3);
        }
    } else if (btn_cancel->clear_mask(0x10000)) {
        g_windows.set_state(*(int*)((char*)this + 0x18));
    } else if (!btn_delete->clear_mask(0x10000) &&
               !(g_keyboard.down[0xd3] != 0 && g_keyboard.prev[0xd3] == 0 && ((*(unsigned char*)((char*)btn_delete + 0x104) & 2) == 0))) {
        // +0x21c is the dialog's answer slot (the confirm sets it to -1 before opening the box). The original
        // (0x4ea5f5: cmp [esi+0x21c], 0 / jne 0x4ea6bc) removes the file when the answer is 0 (ui_yes).
        if (*(int*)((char*)this + 0x21c) == 0) {
            char pathbuf[0x200];
            char name[0x200];
            const char* pf = g_module_name.c_str();
            int sel = sel218;
            sprintf(pathbuf, "");
            save_path(pathbuf);
            mb_snprintf(name, 0x200, "%s%s\\sg%d%d.sav", pathbuf, pf, sel / 10, sel % 10);
            if (_rmdir(name) != 0) {
                tstring err("ui_error_removing_file");
                g_gui.add(*g_strtable.lookup(err), 0xffff0000);
            }
            g_windows.set_state(0x15);
        } else if (*(int*)((char*)this + 0x220) == 0) {
            save_game(sel218, (tstring*)((char*)this + 0x224));
            ((tstring*)((char*)g_game + 8))->operator=(*(tstring*)((char*)this + 0x224));
            {
                tstring ok("ui_game_saved");
                g_gui.add(*g_strtable.lookup(ok), -1);
            }
            g_windows.set_state(*(int*)((char*)this + 0x18));
        }
    } else {
        char name[0x200];
        *(int*)((char*)this + 0x21c) = -1;
        {
            tstring title("ui_confirm_delete_game");
            const tstring* r = g_strtable.lookup(title);
            mb_snprintf(name, 0x200, text_of(r));   // original passes the text itself as the format (0x4ea368, 0x4ea776)
        }
        {
            tstring no("ui_no");
            tstring yes("ui_yes");
            tstring title2(name);
            const tstring* rn = g_strtable.lookup(no);
            const tstring* ry = g_strtable.lookup(yes);
            NamedRec* rec = g_menu_rec;
            rec->name68.clear();
            // Same shape as the save-name box (0x4ea7e4..0x4ea801): +0x1c = the question, +0x78 = ui_yes, +0x88 = ui_no.
            rec->name1c = title2;
            rec->name78 = *ry;
            rec->name88 = *rn;
            rec->f4c = (int)((char*)this + 0x21c);
            rec->f50 = 0;
            rec->f04 = 1;
        }
        g_windows.set_state(0);
    }
}

// @ 0x4ea850
void C4c31c0::select(int idx)
{
    if (sel218 != -1)
        labels[sel218]->set15c(0);
    sel218 = idx;
    if (sel218 != -1) {
        labels[sel218]->set15c(1);
        btn_done->flag2_clear();
        if (*(int*)((char*)this + sel218 * 0x30 + 0x4c))
            btn_delete->flag2_clear();
        else
            btn_delete->flag2_set();
    } else {
        btn_done->flag2_set();
        btn_delete->flag2_set();
    }
}
