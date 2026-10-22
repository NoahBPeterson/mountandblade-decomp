// clang-format off
// Slice s004e55f0: the escape menu's save/load panel.
//   0x4e55f0 SavePanel::init   (this, x, y, out)  -- picks the panel geometry out of the settings table,
//            builds the slot's long button (0x4dcc50), sets its scale, then reads the row texts and hands
//            each of the panel's 0x30-byte row slots a position pair, reporting the save header status.
//   0x4e5d30 SavePanel::hex_field -- "%x" of one settings value (records of 0x2c bytes at 0xa44240+0x1c).
//   0x4e5d80 jmp thunk into 0x4e5d90 (linker stub; not reproducible from source, see partial.txt).
//   0x4e5d90 SavePanel::create  -- resets the GUI, builds the window and its menu block.
//   0x4e5e50 sets window flag 0xa73a10 = 0x400000 then tails into 0x4e5e60.
//   0x4e5e60 SavePanel::create2 -- the same build plus the optional "load_window" background image.
//   0x4e5f50 destroy_range -- runs the 0x1f8-byte element destructor 0x4d3720 over a range.
// Layouts follow the verified neighbours: s004dd830 (escape menu: gui window vptr 0x592848 / ctor 0x4931a0
// / 0x190 bytes, menu block 0x4dd920), s004e05c0 (options screen), s004dfd50 (settings table 0xa44240).
// Names are provisional.
#include <windows.h>
#include <new>

#define MB_NOTHROW throw()

// rgl::string: 16 bytes { pos, buf, cap, len } (ctor 0x42bbe0, dtor 0x42bc40); match/include/rgl.h.
struct tstring {
    int pos;            // +0x00
    char* buf;          // +0x04
    int cap;            // +0x08
    int len;            // +0x0c
    tstring(const char* s);                                 // 0x42bbe0
    ~tstring();                                             // 0x42bc40
};

// settings (ini) table at 0xa44240; find() returns an entry index or -1.  Entries are 0x2c bytes from
// 0xa4425c (this+0x1c) with the string pointer at +4 (a null pointer field holds the string itself).
struct config_entry {
    int f0;                                                 // +0x00
    char* value;                                            // +0x04
    char pad[0x2c - 8];
};

struct config_table {
    int find(const tstring& key);                           // 0x4bd5a0
    int hex_field(const tstring& key);                      // 0x4e5d30
    char pad[0x1c];
    config_entry entries[1];                                // +0x1c
};
extern config_table g_config;                               // 0xa44240

#define CONFIG_ENTRIES ((config_entry*)0xa4425c)            // 0xa4425c

extern "C" double atof(const char* s);                      // 0x537352 lib_crt::_atof
extern "C" int sscanf(const char* s, const char* fmt, ...); // 0x535fe4 lib_crt::_sscanf

// one float setting (this body is inlined 18 times in 0x4e55f0).
static __forceinline float config_float(const char* name)
{
    tstring n(name);
    int i = g_config.find(n);
    float r = 0.0f;
    if (i >= 0) {
        config_entry* e = &CONFIG_ENTRIES[i];
        char* v = e->value;
        if (!v)
            v = (char*)&e->value;
        r = (float)atof(v);
    }
    return r;
}

// one "%x" setting (0x4e5d30): the same lookup, through the table's own record array.
// @ 0x4e5d30
int config_table::hex_field(const tstring& key)
{
    unsigned v = 0;
    int i = find(key);
    if (i >= 0) {
        config_entry* e = &entries[i];
        char* s = e->value ? e->value : (char*)&e->value;
        sscanf(s, "%x", &v);
    }
    return v;
}

// a two-float pair (the widgets take these by pointer)
struct vec2 {
    float x, y;
    __forceinline vec2(float a, float b) { x = a; y = b; }
};

// ---- widgets ------------------------------------------------------------------------------------------
// slot offsets are the ones verified in s004dd830 / s004e05c0 (comment = vtable byte offset).
struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_pos(const float* p);               // +0x58
    virtual void v23(); virtual void v24();
    virtual void set_size(const float* p);              // +0x64
    virtual void v26();
    virtual void set_scale(const vec2* p);              // +0x6c
    virtual void v28(); virtual void v29(); virtual void v30();
    virtual void set_mesh(int layer, void* m);          // +0x7c
    virtual void add_image(int f, void* m);             // +0x80
    virtual void v33(); virtual void v34(); virtual void v35(); virtual void v36(); virtual void v37();
    virtual void set_color(const int* c);               // +0x98
    virtual void set_text(const int* v);                // +0x9c
    virtual void v40(); virtual void v41();
    virtual void set_image(void* p);                    // +0xa4
    virtual void add_at(void* child, const vec2* p);    // +0xac
    virtual void v44(); virtual void v45(); virtual void v46(); virtual void v47(); virtual void v48();
    virtual void v49();
    virtual void add_child(void* c);                    // +0xc8
};

// the menu block (vptr 0x5924a0, ctor 0x4931a0, 0x188 bytes)
struct GuiBlock : Widget {
    char pad0[0x188 - 0x04];
    GuiBlock();                                         // 0x4931a0
};

// the plain GUI window: the block base plus one flag (vptr 0x592848, 0x190 bytes)
struct UiWindow : GuiBlock {
    char f188;                                          // +0x188
    char pad189[0x190 - 0x189];
    UiWindow() { f188 = 0; }
    virtual void v00();                                 // own vtable 0x592848
    void ctor_container();                              // 0x4931a0 (the base ctor)
};

struct BlockOwner {
    char pad0[0x10];
    int clock;                                          // +0x10
    char pad14[0x2c - 0x14];
    GuiBlock* block;                                    // +0x2c
    void init(UiWindow* parent, int which);             // 0x4dd920
};

// the 0x188-byte label / long button (vptr 0x591d58, ctor 0x48ae20)
struct UiLabel : Widget {
    char pad0[0x121 - 0x04];
    char f121;
    char pad122[0x178 - 0x122];
    int f178;
    char pad17c[0x180 - 0x17c];
    char f180;
    char pad181[0x184 - 0x181];
    int f184;                                           // +0x184
};

// 0x4dcc50: the long/menu button maker (parent, text name, position pair, height, label pair, two
// constants and a scale).  Called by 0x4e55f0 with 10 dwords.
UiLabel* make_long_button(UiWindow* parent, tstring* name, float px, float py, float h,
                          tstring* up, tstring* down, float k1, float k2, float k3);   // 0x4dcc50

struct Mesh {
    Mesh* clone();                                      // 0x41c6d0
};
struct resmgr {
    Mesh* get_object(const tstring& name);              // 0x419540
};
extern resmgr g_alloc;                                  // 0x5d6fa0

struct gui_manager {
    void reset();                                       // 0x42d1b0
    void push_back(void* const& x);                     // 0x49b120
};
extern gui_manager g_gui;                               // 0x5d7080

// localization table at 0x5e2a18: lookup(key) hands back the text stored for the key (never null).
struct StrTable {
    tstring* lookup(const tstring& key) MB_NOTHROW;     // 0x43a1e0
};
extern StrTable g_strtable;                             // 0x5e2a18

extern char* g_game;                                    // 0x5e2930
extern char g_reload;                                   // 0x5d8788
extern int g_window_flag;                               // 0xa73a10
extern int g_menu_kind_count;                           // 0xa75f3c

// the panel: a window pointer, the menu block owner, and 0x30-byte row slots from +0x2c on.
struct SavePanel {
    char pad0[0x2c];
    UiWindow* win;                                      // +0x2c
    BlockOwner block;                                   // +0x30
    char pad44[0x234 - 0x44];
    char f234;                                          // +0x234
    void init(float x, int slot, UiLabel** out);        // 0x4e55f0
    void create();                                      // 0x4e5d90
    void create2();                                     // 0x4e5e60
    void create_stub();                                 // 0x4e5d80
    void create_flagged();                              // 0x4e5e50
};

// a 0x1f8-byte game record with the destructor at 0x4d3720
struct Rec {
    void destroy();                                     // 0x4d3720
    char pad[0x1f8];
};

// ---- code -------------------------------------------------------------------------------------------
// @ 0x4e55f0
// `slot` is a raw integer (the caller passes the slot index; the code divides it by three).
void SavePanel::init(float x, int slot, UiLabel** out)
{
    float save_x = config_float("save_b_save_x");
    float save_y = config_float("save_b_save_y");
    float save_x_a = config_float("save_b_save_x_a");
    float save_y_a = config_float("save_b_save_y_a");
    float save_size_y = config_float("save_b_save_size_y");
    float save_button_h = config_float("save_b_save_button_h");

    int row = slot / 3;
    int col = slot % 3;

    {
        tstring empty("");
        tstring key_down("restore_game_panel_down");
        tstring key("restore_game_panel");
        *out = make_long_button(win, &empty,
                                save_x + (float)col * save_x_a,
                                save_y - (float)row * save_y_a,
                                save_size_y, &key, &key_down,
                                1.5544f, save_button_h, 0.085f);
    }
    (*out)->f184 = 1;

    {
        vec2 sc(config_float("save_b_save_t_s"), config_float("save_b_save_t_s"));
        (*out)->set_scale(&sc);
    }
    {
        tstring k("save_b_save_t_c");
        int v = g_config.hex_field(k);
        (*out)->set_color(&v);
    }
    {
        tstring k("save_b_save_t_h_c");
        int v = g_config.hex_field(k);
        (*out)->set_text(&v);
    }

    char* rec = (char*)this + slot * 0x30;
    UiLabel* row0 = (UiLabel*)(rec + 0x2c);

    float name_x = config_float("save_t_name_x");
    float name_y = config_float("save_t_name_y");
    float info_x = config_float("save_t_info_x");
    float info_y = config_float("save_t_info_y");
    float error_x = config_float("save_t_error_x");
    float error_y = config_float("save_t_error_y");

    if (f234 != 0 && *(int*)(rec + 0x4c) == 0) {
        tstring k("ui_empty_slot");
        (*out)->add_at(g_strtable.lookup(k), &vec2(name_x, name_y));
        return;
    }
    (*out)->add_at(rec + 0x2c, &vec2(name_x, name_y));
    (*out)->add_at(rec + 0x3c, &vec2(info_x, info_y));

    if (*((char*)row0 + 0x2c) != 0) {
        tstring k("Incrct. File Type!");
        (*out)->add_at(&k, &vec2(error_x, error_y));
        return;
    }
    if (*((char*)row0 + 0x2d) != 0) {
        tstring k("Incomp. File Ver.!");
        (*out)->add_at(&k, &vec2(error_x, error_y));
        return;
    }
    if (*((char*)row0 + 0x2e) != 0) {
        tstring k("Incomp. Mod. Ver.!");
        (*out)->add_at(&k, &vec2(error_x, error_y));
    }
}

// @ 0x4e5d30 is defined above as config_table::hex_field.

// @ 0x4e5d80 (jmp 0x4e5d90; linker stub, see partial.txt)
void SavePanel::create_stub()
{
    create();
}

// @ 0x4e5d90
void SavePanel::create()
{
    g_gui.reset();
    win = new UiWindow;
    {
        float v[2];
        v[0] = -0.001f;
        v[1] = -0.001f;
        win->set_pos(v);
    }
    {
        float v[2];
        v[0] = 1.002f;
        v[1] = 0.752f;
        win->set_size(v);
    }
    block.init(win, *(int*)(g_game + 0x13b04));
    void* w = win;
    g_reload = 1;
    g_gui.push_back(w);
}

// @ 0x4e5e50
void SavePanel::create_flagged()
{
    g_window_flag = 0x400000;
    create2();
}

// @ 0x4e5e60
void SavePanel::create2()
{
    g_gui.reset();
    win = new UiWindow;
    {
        float v[2];
        v[0] = -0.001f;
        v[1] = -0.001f;
        win->set_pos(v);
    }
    {
        float v[2];
        v[0] = 1.002f;
        v[1] = 0.752f;
        win->set_size(v);
    }
    block.init(win, g_menu_kind_count);
    if (block.clock < 0) {
        tstring n("load_window");
        win->set_mesh(0, g_alloc.get_object(n)->clone());
    }
    void* w = win;
    g_reload = 1;
    g_gui.push_back(w);
}

// @ 0x4e5f50
void __stdcall destroy_range(Rec* first, Rec* last)
{
    while (first != last) {
        first->destroy();
        first = (Rec*)((char*)first + 0x1f8);
    }
}
