// Slice s004e4790: the saved-game ("save") screen constructor.
//   0x4e4790  build the save window: mark whether it is the "load" flavour (+0x234), reset the gui
//             manager, allocate the 0x190-byte window container, set its rect/scale, fetch the
//             "load_window" mesh, fill the nine save-slot labels (+0x1e4..+0x204) through 0x4e55f0 when
//             the slot exists or the load flavour is on, reset the selection, then either lay out the
//             nine "Save game N" entries (load flavour) or the "Save" heading and the empty slots.  All
//             names provisional; every offset is read off the disassembly.
#include <windows.h>
#include <stdio.h>
#include "rgl.h"

extern "C" int _fltused = 0;

// ---- engine string (0x10 bytes) -------------------------------------------------------------------------
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0 (ret 4)
    tstring(const tstring& o);                          // 0x42c110 (ret 4)
    ~tstring();                                         // 0x42bc40
    void operator=(const tstring& o);                   // 0x42bc50 (ret 4)
    char equals(const tstring& o) const;                // 0x42bdd0 (ret 4)
    tstring operator+(const tstring& s) const;          // 0x42c840 (ret 8)
};

// ---- globals --------------------------------------------------------------------------------------------
extern char g_config;                                   // 0xa44240 (the ini table, see s004e4c70)
extern tstring* g_save_names;                           // 0xa70240 (the table base) + count
extern int g_save_count;                                // 0xa70240? (see 0x4e4920: cmp ebp,[0xa70240])
extern const char g_save_prefix[];                      // 0x8ee3c3c

struct gui_manager { void reset(); };                    // 0x42d1b0 (ECX = 0x5d7080)
extern "C" gui_manager g_msg_log;                        // 0x5d7080
struct resmgr { void* get_object(tstring* name); };      // 0x419540 (ECX = this, ret 4)
extern resmgr g_alloc;                                   // 0x5d6fa0
extern unsigned char g_gui_dirty;                        // 0x5d8788

void* __cdecl operator new(unsigned n);                  // 0x53545e
void* __cdecl operator new(unsigned n, void* p);         // placement

void* container_ctor(void* o);                           // 0x4931a0 (ECX = storage, ret 4)
void* mesh_clone(void* m);                               // 0x41c6d0 (ECX = mesh)
char* save_path(char* buf);                              // 0x4dd7b0 (ECX = ?, ret 4)

// ---- the window -----------------------------------------------------------------------------------------
struct vector2 {
    float x, y;
    vector2() {}
    vector2(float a, float b) { x = a; y = b; }
};

// the 0x190-byte container: base ctor 0x4931a0, vptr 0x592848, a byte at +0x188
struct Container {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_rect(const vector2& v);            // +0x58 = slot 22
    virtual void v23(); virtual void v24();
    virtual void set_scale(const vector2& v);           // +0x64 = slot 25
    virtual void v26(); virtual void v27(); virtual void v28(); virtual void v29(); virtual void v30();
    virtual void set_mesh(int slot, void* mesh);        // +0x7c = slot 31
    virtual void v32(); virtual void v33(); virtual void v34(); virtual void v35(); virtual void v36();
    virtual void v37(); virtual void v38(); virtual void v39(); virtual void v40(); virtual void v41();
    virtual void v42(); virtual void v43(); virtual void v44(); virtual void v45();
    virtual void show();                                // +0xb8 = slot 46
    char f188;                                          // +0x188
};
struct SaveWindow {
    char pad4c[0x4c];
    char slots[8][0x30];                // +0x4c .. +0x1cb (slots 0..7, 0x30 apart)
    char slot_tail[0x1e0 - 0x1cc];
    Container* window;                  // +0x1e0 (also slot 8's first word)
    char slot_names[0x24];              // +0x1e4 .. +0x207: nine slot labels; only pos is cleared here
    Container* btn_done;                // +0x208
    Container* btn_cancel;              // +0x20c
    Container* btn_delete;              // +0x210
    char tail218[0x234 - 0x214];        // +0x214 .. +0x233 (three ints set to -1 at +0x218/+0x21c/+0x220)
    bool is_load;                       // +0x234
    void init();                        // 0x4e4790
    int  list_saves();                  // 0x4e5270 (s004e4c70)
};

// ---- the ini table --------------------------------------------------------------------------------------
#define CONFIG_BASE ((char*)0xa44240)
#define CONFIG_ENTRY(i) ((char*)0xa4425c + 0x2c * (i))
struct config_table2 {
    int find(const tstring& key);                       // 0x42bdd0 is tstring::equals, 0x4bd5a0 find
};
// The screen searches the table by hand: it walks 0x2c bytes at a time from 0xa44240, calls
// tstring::equals (0x42bdd0) on each entry key, and stops at the first match or at the count in 0xa70240.
static int config_index(const tstring& key)
{
    int n = *(int*)0xa70240;
    int found = -1;
    for (int i = 0; i < n; ++i) {
        if (((const tstring*)(CONFIG_BASE + 0x2c * i))->equals(key))
            found = i;
        if (found != -1)
            break;
    }
    return found;
}

// the value of entry i as a float (atof of the string, 0 when the entry does not exist)
static float config_value(int i)
{
    if (i < 0)
        return 0.0f;
    char* e = CONFIG_ENTRY(i);
    char* v = *(char**)(e + 4);
    return (float)atof(v ? v : e + 4);
}

extern "C" double __cdecl atof(const char* s);           // 0x537352
struct strtab3 { tstring* lookup(const tstring& key); };  // 0x43a1e0 (ECX = 0x5e2a18)
extern strtab3 g_strtab;                                  // 0x5e2a18
void label_refresh(Container* w, const tstring& key);    // 0x4e4c70 (s004e4c70)
Container* make_short_button(Container* parent, tstring* caption, int a, int b, float f); // 0x4e5570
void slot_label(Container* parent, int i, tstring* name);   // 0x4e55f0
void gui_set_window(gui_manager* m, Container** w);      // 0x49b120

// ---- code -----------------------------------------------------------------------------------------------

// @ 0x4e4790   SaveWindow::init(): build the window, list the saves, then either the save or the load layout
void SaveWindow::init()
{
    is_load = (*(int*)(*(char**)0xa739d4 + 0x18) == 5);
    g_msg_log.reset();

    Container* w = (Container*)::operator new(0x190);
    if (w) {
        container_ctor(w);
        *(void**)w = (void*)0x592848;
        w->f188 = 0;
    }
    window = w;

    vector2 pos(-0.001f, -0.001f);
    w->set_rect(pos);
    vector2 scale(1.002f, 0.52f);
    window->set_scale(scale);

    tstring mesh_name("load_window");
    void* mesh = g_alloc.get_object(&mesh_name);
    void* clone = mesh_clone(mesh);
    window->set_mesh(0, clone);

    list_saves();

    for (int i = 0; i < 9; ++i)
        ((int*)slot_names)[i] = 0;
    for (int i = 0; i < 9; ++i) {
        if (*(int*)&slots[i][0] != 0 || is_load)
            slot_label(window, i, (tstring*)&slot_names[0x10 * i]);
    }

    ((int*)tail218)[1] = -1;                            // +0x218
    ((int*)tail218)[2] = -1;                            // +0x21c
    ((int*)tail218)[3] = -1;                            // +0x220

    if (is_load) {
        tstring h("save_b_done_h");
        tstring caption("ui_save");
        int idx = config_index(h);
        float f = config_value(idx);
        btn_done = make_short_button(window, g_strtab.lookup(caption), 0, 0, f);
        label_refresh(btn_done, h);
        btn_done->show();
    } else {
        tstring h("save_b_done_h");
        tstring caption("ui_restore");
        int idx = config_index(h);
        float f = config_value(idx);
        btn_done = make_short_button(window, g_strtab.lookup(caption), 0, 0, f);
        label_refresh(btn_done, h);
    }

    {
        tstring h("save_b_cancel_h");
        tstring caption("ui_cancel");
        int idx = config_index(h);
        float f = config_value(idx);
        btn_cancel = make_short_button(window, g_strtab.lookup(caption), 0, 0, f);
        label_refresh(btn_cancel, h);
    }
    {
        tstring h("save_b_delete_h");
        tstring caption("ui_delete");
        int idx = config_index(h);
        float f = config_value(idx);
        btn_delete = make_short_button(window, g_strtab.lookup(caption), 0, 0, f);
        label_refresh(btn_delete, h);
        btn_delete->show();
    }

    g_gui_dirty = 1;
    Container* tmp = window;
    gui_set_window(&g_msg_log, &tmp);
}
