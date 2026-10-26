#include "./save_panel.h"
namespace rgl {
struct mesh_ext {                                   // rgl/scene/scene.h: the mesh object with the scene-side members
    void clear_geometry();                          // 0x428e80
};
}
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
        mat3 id = *identity3(&tmp);
        id.s.x *= o->f158; id.s.y *= o->f158; id.s.z *= o->f158;
        id.f.x *= o->f15c; id.f.y *= o->f15c; id.f.z *= o->f15c;
        id.u.x *= o->f160; id.u.y *= o->f160; id.u.z *= o->f160;
        skin_rec* rec = o->skins->begin[i];
        mat4 M = id * rec->mat;
        m->append_skinned(rec->m, &M, -1, color, flip);
    }
}

// ---- widget virtual slots (indices from the verified UI slices) -----------------------------------------
struct Control {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28();
    virtual void set_cursor(int on);                    // +0x2c
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40();
    virtual char is_key_down(int key);                  // +0x44
    virtual void v48(); virtual void v4c(); virtual void v50(); virtual void v54();
    virtual void v58(); virtual void v5c(); virtual void v60(); virtual void v64();
    virtual void v68(); virtual void v6c(); virtual void v70(); virtual void v74();
    virtual void v78(); virtual void v7c(); virtual void v80(); virtual void v84();
    virtual void v88(); virtual void v8c(); virtual void v90(); virtual void v94();
    virtual void v98(); virtual void v9c(); virtual void va0(); virtual void va4();
    virtual void va8(); virtual void vac(); virtual void vb0(); virtual void vb4();
    virtual void hide();                                // +0xb8
    virtual void show();                                // +0xbc
};

// the save/load panel: nine slot buttons at +0x1e4, the two sub-panels at +0x208/+0x210, the
// selected slot at +0x218 and the slot records at +0 (stride 0x30, selected flag at +0x4c).
struct SavePanel {
    char pad0[0x1e4];
    Control* buttons[9];                                // +0x1e4
    Control* list_panel;                                // +0x208
    char pad20c[4];
    Control* info_panel;                                // +0x210
    char pad214[4];
    int selected;                                       // +0x218
    char pad21c[0x234 - 0x21c];
    char deleting;                                      // +0x234
    void update(int arg);
    void select(int idx);
};

#include "../str_table.h"
extern StrTable g_strtable;                                     // 0x5e2a18
#include "../../include/msg_log.h"
extern MsgLog g_msglog;                                         // 0x5d7080
void mb_snprintf(char* out, unsigned size, const char* fmt, ...);  // 0x415100
int  __cdecl crt_sprintf(char* buf, const char* fmt, ...);      // 0x536f8f
int  __cdecl crt_remove(const char* path);                      // 0x539023
void save_path(char* out);                                      // 0x4dd7b0
int  save_game(int arg, tstring* name);                         // 0x4e8980
extern char* g_save_prefix;                                     // 0x8ee3c3c (prefix string pointer)
extern char* g_game;                                            // 0x5e2930
extern int g_a17420;                                            // 0xa17420
extern int g_a17820;                                            // 0xa17820
extern char* g_save_dialog;                                     // 0xa7398c
extern char g_main_menu_obj[];                                  // 0xa73980

// main menu layout (see s004d7d10)
#include "../main_menu.h"
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
void SavePanel::update(int)
{
    // 0xa73980's tstrings are built by the original's static initialiser, which our build does not have: the glue copies
    // the object's image bytes (cap = a pointer, buf = 0), so the first assignment goes through tstring::reserve's
    // "enough capacity" path and writes to null. A file-scope initialiser did not survive (the fields were re-established
    // before use), so construct them here, once, on the frame path that first touches the record.
    {
        static bool menu_strings_done = false;
        if (!menu_strings_done) {
            menu_strings_done = true;
            new (&g_menu_rec->title) tstring();
            new (&*(tstring*)((char*)g_menu_rec + 0x54)) tstring();
            new (&g_menu_rec->header) tstring();
            new (&g_menu_rec->answer_yes) tstring();
            new (&g_menu_rec->answer_no) tstring();
        }
    }
    char pathbuf[16];
    char name[0x200];
    for (int i = 0; i < 9; i++) {
        Control* button = buttons[i];
        if (button && button->is_key_down(0x10000))
            select(i);
    }
    if (list_panel->is_key_down(0x10000)) {
        if (deleting) {
        *(int*)((char*)this + 0x220) = -1;
        tstring title("ui_set_save_file_name");
        const tstring* r = g_strtable.lookup(title);
        mb_snprintf(name, 0x200, "%s", text_of(r));
        tstring cancel("ui_cancel");
        tstring ok("ui_ok");
        tstring title2(name);
        const tstring* rc = g_strtable.lookup(cancel);
        const tstring* ro = g_strtable.lookup(ok);
        g_menu_rec->header.clear();
        g_menu_rec->title = *rc;
        g_menu_rec->answer_yes = *ro;
        g_menu_rec->answer_no = title2;
        g_menu_rec->choice = (int)((char*)this + 0x220);
        g_menu_rec->v50 = (int)((char*)this + 0x224);
        int* slotname;
        if (*(int*)((char*)this + selected * 0x30 + 0x4c) != 0)
            slotname = (int*)((char*)this + selected * 0x30 + 0x2c);
        else
            slotname = (int*)(*(int*)(g_game + 0x8c) * 0x508 +
                              *(int*)(g_game + 0x13a00));
        ((tstring*)((char*)g_menu_rec + 0x54))->operator=(*(tstring*)slotname);
        g_menu_rec->open = 3;
        g_main_menu.set_state(0);
        return;
        } else {
        char* slot = (char*)this + selected * 0x30;
        if (slot[0x58] == 0 && slot[0x59] == 0 && slot[0x5a] == 0) {
            crt_sprintf(pathbuf, "");
            save_path(pathbuf);
            const char* path = *(char**)(pathbuf + 4);
            const char* pf = g_save_prefix;
            if (!pf)
                pf = (char*)&g_save_prefix;
            mb_snprintf(name, 0x200, "%s%s\\sg%d%d.sav", path, pf,
                        selected / 10, selected % 10);
            *(int*)(g_save_dialog + 4) = 1;
            ((tstring*)(g_save_dialog + 0x48))->assign(name);
            *(int*)(g_save_dialog + 0x8c) = selected;
            g_main_menu.set_state(3);
            return;
        }
        }
    }
    if (((Control*)*(void**)((char*)this + 0x20c))->is_key_down(0x10000)) {
        g_main_menu.set_state(*(int*)((char*)this + 0x18));
        return;
    }
    Control* panel = *(Control**)((char*)this + 0x210);
    if (panel->is_key_down(0x10000))
        goto confirm_delete;
    if ((g_a17420 != 0 && g_a17820 == 0) ||
        ((*(unsigned char*)((char*)panel + 0x104) & 2) != 0)) {
        if (*(int*)((char*)this + 0x21c) == 0) {
            crt_sprintf(pathbuf, "");
            save_path(pathbuf);
            const char* path = *(char**)(pathbuf + 4);
            const char* pf = g_save_prefix;
            if (!pf)
                pf = (char*)&g_save_prefix;
            mb_snprintf(name, 0x200, "%s%s\\sg%d%d.sav", path, pf,
                        selected / 10, selected % 10);
            if (crt_remove(name) != 0) {
                tstring err("ui_error_removing_file");
                g_msglog.add(*g_strtable.lookup(err), -1);
            }
            g_main_menu.set_state(0x15);
            return;
        }
        if (*(int*)((char*)this + 0x220) != 0)
            return;
        save_game(selected, (tstring*)((char*)this + 0x224));
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
    mb_snprintf(name, 0x200, "%s", text_of(r));
    tstring no("ui_no");
    tstring yes("ui_yes");
    tstring title2(name);
    const tstring* rn = g_strtable.lookup(no);
    const tstring* ry = g_strtable.lookup(yes);
    g_menu_rec->header.clear();
    g_menu_rec->title = *rn;
    g_menu_rec->answer_yes = *ry;
    g_menu_rec->answer_no = title2;
    g_menu_rec->choice = (int)((char*)this + 0x21c);
    g_menu_rec->v50 = 0;
    g_menu_rec->open = 1;
    g_main_menu.set_state(0);
}

// @ 0x4ea850
void SavePanel::select(int idx)
{
    if (selected != -1)
        buttons[selected]->set_cursor(0);
    selected = idx;
    if (selected != -1) {
        buttons[selected]->set_cursor(1);
        list_panel->show();
        if (*(int*)((char*)this + selected * 0x30 + 0x4c))
            info_panel->show();
        else
            info_panel->hide();
    } else {
        list_panel->hide();
        info_panel->hide();
    }
}
