// Slice s004dd830: the in-game (escape) menu.  0x4dda40 / 0x4ddf80 build an "escape menu" object: a
// container widget plus rows of label widgets; 0x4dd920 / 0x4dd9e0 build and activate a menu block.
// Names provisional; every offset is read off the disassembly.
#include <windows.h>
#include <new>

#define MB_NOTHROW throw()

// ---- engine string ---------------------------------------------------------------------------------
struct tstring {
    int pos;            // +0x00  read cursor
    char* buf;          // +0x04
    int cap;            // +0x08
    int len;            // +0x0c
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
    void assign(const char* s);                 // 0x42bcb0
};

struct StrTable {
    tstring* lookup(const tstring& key) MB_NOTHROW;     // 0x43a1e0 (never null)
};
extern StrTable g_strtable;                             // 0x5e2a18

// ---- widgets ---------------------------------------------------------------------------------------
struct Widget {
    virtual void w00(); virtual void w01(); virtual void w02(); virtual void w03(); virtual void w04();
    virtual void w05(); virtual void w06(); virtual void w07(); virtual void w08(); virtual void w09();
    virtual void w10(); virtual void w11(); virtual void w12(); virtual void w13(); virtual void w14();
    virtual void w15(); virtual void w16(); virtual void w17(); virtual void w18(); virtual void w19();
    virtual void w20(); virtual void w21();
    virtual void set_pos(float* p);             // +0x58
    virtual void w23(); virtual void w24();
    virtual void set_size(float* p);            // +0x64
    virtual void w26();
    virtual void set_scale(float* p);           // +0x6c
    virtual void w28(); virtual void w29(); virtual void w30();
    virtual void add_image(int f, void* m);     // +0x7c
    virtual void add_image2(int f, void* m);    // +0x80
    virtual void w34(); virtual void w35(); virtual void w36(); virtual void w37();
    virtual void w38();
    virtual void set_colour(int* c);            // +0x98
    virtual void set_colour2(int* c);           // +0x9c
    virtual void w40();
    virtual void set_text(const float* p, const tstring* s);    // +0xa4
    virtual void w42(); virtual void w43(); virtual void w44(); virtual void w45();
    virtual void hide();                        // +0xb8
    virtual void w46(); virtual void w47(); virtual void w48();
    virtual void add_child(void* c);            // +0xc8

    void base_ctor();                           // 0x48ae20
};

struct ItemSound { void play(int id); };        // 0x48add0

// the label widget (vptr 0x591d58, size 0x188)
struct UiLabel : Widget {
    char pad0[0x121 - 0x04];
    char f121;
    char pad122[0x178 - 0x122];
    int f178;
    char pad17c[0x180 - 0x17c];
    char f180;
    char pad181[0x184 - 0x181];
    int f184;
    void label_ctor();                          // 0x48ae20
};

// a plain widget with room for a position / size pair (size 0x178)
struct UiImage : Widget {
    char pad0[0x1c - 0x04];
    char f1c[0x48];
};

// the container widget (vptr 0x592848, size 0x190)
struct UiContainer : Widget {
    char pad0[0xe0 - 0x04];
    float pos_e0_[2];                           // +0xe0
    char pade8[0xf0 - 0xe8];
    float size_f0_[2];                          // +0xf0
    char padf8[0x188 - 0xf8];
    char f188;
    void ctor_container();                      // 0x4931a0
};

// a menu block (vptr 0x5924a0, size 0x188)
struct GuiBlock : Widget {
    char pad0[0x10 - 0x04];
    int index;                                  // +0x10
    int f14;                                    // +0x14
    int clock;                                  // +0x18
    char pad1c[0x20 - 0x1c];
    int t20;
    int t24;
    char pad28[0x17c - 0x28];
    int f17c;
    int f180;
    int f184;
    void ctor_block();
};

// a "menu kind" record, stride 0x24
struct MenuKind {
    char pad0[0x10];
    int a;                                      // +0x10
    ItemSound sound;                            // +0x14
    char pad18[0x20 - 0x18];
    int first;                                  // +0x20
};

UiLabel* build_row(UiContainer* parent, const tstring* key, float x, float y, float w,
                   float h);                    // 0x4dd830 (builds one labelled menu row)
Widget* build_long_row(UiContainer* parent, const tstring* key, float x, float y, float w,
                       const tstring* name, float h1, float h2, const tstring* name2,
                       float h3);                   // 0x4dcc50

extern MenuKind* g_menu_kinds;                  // 0xa75f2c
extern int g_menu_kind_count;                   // 0xa75f38
extern GuiBlock* g_active_menu;                 // 0x5e299c

struct gui_manager {
    void reset();                               // 0x42d1b0
    void push_back(void* const& x);             // 0x49b120
};
extern gui_manager g_gui;                       // 0x5d7080

extern char* g_game;                            // 0x5e2930
extern int g_main_menu_font_color_red;          // 0xa181bc
extern int g_main_menu_font_color_green;        // 0xa181c0
extern int g_main_menu_font_color_blue;         // 0xa181c4
extern char g_reload;                           // 0x5d8788
extern char g_msg_flag;                         // 0x5d75e0

extern char g_key_return[];                     // 0x59736c "ui_return_to_game"
extern char g_key_options[];                    // 0x597234 "ui_options"
extern char g_key_save_exit[];                  // 0x597358 "ui_save_and_exit"
extern char g_key_save[];                       // 0x597350 "ui_save"
extern char g_key_save_as[];                    // 0x597344 "ui_save_as"
extern char g_key_quit[];                       // 0x59732c "ui_quit_without_saving"
extern char g_img_logo[];                       // 0x597020 "warrider_logo"
extern char g_img_warrior[];                    // 0x597318 "pic_mb_warrior_1"
extern char g_img_warrior2[];                   // 0x597304
extern char g_img_warrior3[];                   // 0x5972f0
extern char g_img_warrior4[];                   // 0x5972dc
extern char g_img_load[];                       // 0x597030 "load_window"
extern char g_img_message[];                    // 0x5971b0 "message_window"
extern char g_key_button_down[];                // 0x5971d0 "longer_button_down"
extern char g_key_button[];                     // 0x5971c0 "longer_button"
extern float g_rand_range;                      // 0x54a6bc
extern float g_pic_split1;                      // 0x54b868
extern float g_pic_split2;                      // 0x549d70
extern float g_pic_split3;                      // 0x54b8ac

void* resmgr_get_object(const tstring& name);   // 0x419540 (ecx = 0x5d6fa0)
void* clone_mesh(void* m);                      // 0x41c6d0
int rand_int();                                 // 0x536a9f (_rand)
struct BmpMaker { void* make(int n); };         // 0x499050
void set_3d_flag(void* m, int f);               // 0x44f0c0

void __fastcall activate_menu(GuiBlock* w);     // 0x4dd9e0

static inline void widget_add_child(Widget* w, void* c)
{
    w->add_child(c);
}
static inline void widget_hide(Widget* w)
{
    w->hide();
}

struct BlockOwner {
    char pad0[0x10];
    int clock;                                  // +0x10
    char pad14[0x2c - 0x14];
    GuiBlock* block;                            // +0x2c
    void init(UiContainer* parent, int which);  // 0x4dd920
};

struct EscapeMenu {
    char pad0[0x2c];
    UiContainer* win;                           // +0x2c
    BlockOwner block;                           // +0x30
    Widget* row0;                               // +0x60
    Widget* row1;                               // +0x64
    Widget* row2;                               // +0x68
    Widget* row3;                               // +0x6c
    Widget* row4;                               // +0x70
    Widget* row5;                               // +0x74
    Widget* logo;                               // +0x78
    char pad7c[0x88 - 0x7c];
    int count;                                  // +0x88
    void create();                              // 0x4dda40
};

// the object built by 0x4ddf80: a message window with up to six longer buttons
struct MessageMenu {
    char pad0[0x1c];
    tstring text;                               // +0x1c
    UiContainer* win;                           // +0x2c
    char pad30[0x34 - 0x30];
    Widget* image;                              // +0x34
    Widget* rows[6];                            // +0x38
    char pad50[0x54 - 0x50];
    int count;                                  // +0x54
    char f58;                                   // +0x58
    char pad59[0x6c - 0x59];
    tstring texts[1];                           // +0x6c
    void create_message();                      // 0x4ddf80
};

// ---- code ------------------------------------------------------------------------------------------
// @ 0x4dd830
UiLabel* build_row(UiContainer* parent, const tstring* key, float x, float y, float w, float h)
{
    UiLabel* p = (UiLabel*)::operator new(0x188);
    if (p != 0) {
        p->label_ctor();
        p->f178 = 0;
        p->f121 = 0;
        *(void**)p = (void*)0x591d58;
        p->f180 = 0;
        p->f184 = 0;
    } else {
        p = 0;
    }

    typedef void (__fastcall *AddChild)(Widget*, void*);

    float v[2] = {x, y};
    p->set_pos(v);

    v[0] = w;
    v[1] = h;
    p->set_size(v);

    {
        int colour = 0xffefefef;
        p->set_colour(&colour);
    }

    v[0] = h * 0.9f;
    v[1] = h * 0.9f;
    p->set_scale(v);

    {
        int colour2 = 0xff55ff50;
        p->set_colour2(&colour2);
    }

    p->set_text(v, key);

    parent->add_child(p);
    return p;
}

// @ 0x4dd9e0
void __fastcall activate_menu(GuiBlock* w)
{
    GuiBlock* p = w;
    g_active_menu = w;
    if (p->index >= 0) {
        p->f14 = g_menu_kinds[p->index].a;
        int clk = p->clock;
        p->t20 = *(int*)(0x5e2948 + clk * 8);
        p->t24 = *(int*)(0x5e2948 + clk * 8 + 4);
        int idx2 = *(volatile int*)(&p->index);
        g_menu_kinds[idx2].sound.play(-60);
    } else {
        p->f14 = 0;
    }
}

// @ 0x4dd920
void BlockOwner::init(UiContainer* parent, int which)
{
    if (which < 0)
        return;
    GuiBlock* b;
    {
        GuiBlock* w = (GuiBlock*)::operator new(0x188);
        if (w != 0) {
            w->base_ctor();
            *(void**)w = (void*)0x5924a0;
            w->f17c = 0;
            w->f180 = 0;
            w->f184 = 0;
            b = w;
        } else {
            b = 0;
        }
    }
    block = b;
    block->set_size(parent->size_f0_);
    block->set_pos(parent->pos_e0_);
    widget_add_child(parent, block);
    clock = which;
    int n = g_menu_kinds[which].first;
    if (n > 0)
        block->add_image(0, ((BmpMaker*)0x8ed4f58)->make(n));
    activate_menu((GuiBlock*)this);
}

// @ 0x4dda40
void EscapeMenu::create()
{
    g_gui.reset();
    UiContainer* w = (UiContainer*)::operator new(0x190);
    if (w != 0) {
        w->ctor_container();
        *(void**)w = (void*)0x592848;
        w->f188 = 0;
    } else {
        w = 0;
    }
    win = w;

    float v[2];
    v[0] = -0.001f;
    v[1] = -0.001f;
    win->set_pos(v);

    v[0] = 1.01f;
    v[1] = 0.75f;
    win->set_size(v);

    int no = -1;
    win->set_colour(&no);

    int r = g_main_menu_font_color_red;
    r = r < 0 ? 0 : (r >= 0xff ? 0xfe : r);
    int g = g_main_menu_font_color_green;
    g = g < 0 ? 0 : (g >= 0xff ? 0xfe : g);
    int b = g_main_menu_font_color_blue;
    b = b < 0 ? 0 : (b >= 0xff ? 0xfe : b);
    int color = ((((unsigned)r | 0xffffff00) << 8) | (unsigned)g) << 8 | (unsigned)b;

    {
        tstring k(g_key_return);
        row0 = build_row(w, g_strtable.lookup(k), 0.1f, 0.42f, 0.4f, 0.05f);
    }
    {
        tstring k(g_key_options);
        row2 = build_row(w, g_strtable.lookup(k), 0.1f, 0.36f, 0.4f, 0.05f);
    }
    {
        tstring k(g_key_save_exit);
        row1 = build_row(w, g_strtable.lookup(k), 0.1f, 0.29f, 0.4f, 0.05f);
    }
    if (g_game[0x18] != 0)
        widget_hide(row1);
    row0->set_colour(&color);
    row2->set_colour(&color);
    row1->set_colour(&color);
    row3 = 0;
    row4 = 0;
    row5 = 0;

    if (*(int*)(g_game + 0xb4) != 0) {
        {
            tstring k(g_key_save);
            row3 = build_row(w, g_strtable.lookup(k), 0.1f, 0.22f, 0.4f, 0.05f);
        }
        row3->set_colour(&color);
        {
            tstring k(g_key_save_as);
            row4 = build_row(w, g_strtable.lookup(k), 0.1f, 0.15f, 0.4f, 0.05f);
        }
        row4->set_colour(&color);
        {
            tstring k(g_key_quit);
            row5 = build_row(w, g_strtable.lookup(k), 0.1f, 0.08f, 0.4f, 0.05f);
        }
        row5->set_colour(&color);
        if (g_game[0x18] != 0) {
            widget_hide(row3);
            widget_hide(row4);
        }
    }

    UiImage* lg = (UiImage*)::operator new(0x178);
    if (lg != 0)
        lg->base_ctor();
    else
        lg = 0;
    logo = lg;

    v[0] = 0.3f;
    v[1] = 0.6f;
    lg->set_pos(v);

    v[0] = 0.4f;
    v[1] = 0.1f;
    lg->set_size(v);

    {
        tstring n(g_img_logo);
        lg->add_image(0, clone_mesh(resmgr_get_object(n)));
    }
    widget_add_child(win, lg);

    int pick = rand_int() % 0x3dc9;
    float f = (float)pick * g_rand_range;
    tstring pic(g_img_warrior);
    if (f < g_pic_split1)
        pic.assign(g_img_warrior2);
    else if (f < g_pic_split2)
        pic.assign(g_img_warrior3);
    else if (f < g_pic_split3)
        pic.assign(g_img_warrior4);

    win->add_image2(2, clone_mesh(resmgr_get_object(pic)));

    block.init(win, g_menu_kind_count);
    if (block.clock < 0) {
        tstring n(g_img_load);
        win->add_image(0, clone_mesh(resmgr_get_object(n)));
    }
    void* wv = win;
    g_reload = 1;
    g_gui.push_back(wv);
}

// @ 0x4ddf80
void MessageMenu::create_message()
{
    UiContainer* w;
    {
        UiContainer* p = (UiContainer*)::operator new(0x190);
        if (p != 0) {
            p->ctor_container();
            *(void**)p = (void*)0x592848;
            p->f188 = 0;
            w = p;
        } else {
            w = 0;
        }
    }
    win = w;

    float v[4];
    v[0] = 0.25f;
    v[1] = 0.2f;
    v[2] = 0.5f;
    v[3] = 0.44f;
    win->set_pos(v);

    win->set_size(v + 2);

    Widget* m;
    {
        tstring t(g_img_message);
        m = (Widget*)clone_mesh(resmgr_get_object(t));
    }
    win->add_image(0, m);

    image = (UiImage*)::operator new(0x178);
    if (image != 0)
        image->base_ctor();
    else
        image = 0;

    v[0] = 0.05f;
    v[1] = 0.29f;
    image->set_pos(v);

    v[0] = 0.4f;
    v[1] = 0.1f;
    image->set_size(v);

    image->set_text(v, &text);
    win->add_child(image);

    rows[0] = 0;
    rows[1] = 0;
    rows[2] = 0;
    rows[3] = 0;
    rows[4] = 0;
    rows[5] = 0;

    for (int i = 0; i < count; i++) {
        tstring t1(g_key_button_down);
        tstring t2(g_key_button);
        rows[i] = build_long_row(win, &texts[i], 0.05f,
                                 0.3f - (float)(i - count + 6) * 0.05f,
                                 0.04f, &t2, 9.7f, 0.072f, &t1, 0.085f);
    }

    void* wv = win;
    g_reload = 1;
    g_gui.push_back(wv);
    f58 = g_msg_flag;
}
