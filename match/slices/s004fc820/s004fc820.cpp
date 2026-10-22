// Slice s004fc820: the game-log HUD builder (0x4fc820), the array deleting destructor for a
// tstring array (0x4fd0e0) and the 0x1d0-byte log widget constructor (0x4fd150).
// Names provisional; offsets read off the three functions.
#include <windows.h>
#include <new>
#include <stdlib.h>

struct vector4 { float x, y, z, w; };

struct tstring {                                    // 0x10 bytes
    int pos; char* buf; int cap; int len;
    tstring(const char* s);                        // 0x42bbe0
    ~tstring();                                    // 0x42bc40
    void operator=(const tstring& o);              // 0x42bc50
    void assign(const char* s);                    // 0x42bcb0
    tstring* split(const char& d, int& count) const;    // 0x42c9b0
    void append(const tstring& o);                 // 0x42c1e0
    void push_back(char c);                        // 0x42be70
    void erase(int at, int n);                     // 0x42bea0
    char equals(const tstring& o) const;           // 0x42bdd0
};

extern "C" double __cdecl _atof(const char*);      // 0x537352

struct Ui {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10();
    virtual void select(int on);                    // +0x2c
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15(); virtual void v16();
    virtual void v17();
    virtual void flag_cmd(int v);                   // +0x48
    virtual void v19(); virtual void v20(); virtual void v21();
    virtual void set_size(float* v);                // +0x58
    virtual void v23();                             // +0x5c
    virtual void v24();                             // +0x60
    virtual void set_pos(float* v);                 // +0x64
    virtual void v26();                             // +0x68
    virtual void set_inset(float* v);               // +0x6c
    virtual void v28();                             // +0x70
    virtual void set_int(int* v);                   // +0x74
    virtual void v30(); virtual void v31();         // +0x78..0x7c
    virtual void set_mesh(int flag, void* m);       // +0x80
    virtual void v33(); virtual void v34(); virtual void v35(); virtual void v36(); virtual void v37();
    virtual void set_color98(int* c);               // +0x98
    virtual void set_color9c(int* c);               // +0x9c
    virtual void v40();                             // +0xa0
    virtual void set_name(tstring* out, tstring* name);  // +0xa4
    virtual void v42(); virtual void v43();         // +0xa8..0xac
    virtual void v44(); virtual void v45();         // +0xb0..0xb4
    virtual void refresh();                         // +0xb8
    virtual void v47(); virtual void v48(); virtual void v49();
    virtual void add_child(Ui* c);                  // +0xc8
};

struct UiBase : Ui {
    char pad178[0x178 - sizeof(Ui*)];
    UiBase();                                       // 0x48ae20
};

extern void* g_vt_592848[];                         // 0x592848
extern void* g_vt_597050[];                         // 0x597050
extern void* g_vt_5995c0[];                         // 0x5995c0

void __fastcall ui_ctor_4931a0(void* p);            // 0x4931a0
void __fastcall ui_ctor_48ae20(void* p);            // 0x48ae20
struct LogSub { void init(unsigned n); };           // 0x4fd490 (thiscall)
void __fastcall gui_manager_reset(void* self);      // 0x42d1b0 (this = 0x5d7080)
void* __fastcall mesh_clone(void* m);               // 0x41c6d0
void* __fastcall resmgr_get_object(tstring* name);  // 0x419540 (this = 0x5d6fa0)
void* __fastcall lookup_ui(void* self, tstring* name);  // 0x43a1e0 (this = 0x5e2a18)
void* __cdecl make_tab_widget(Ui* frame, void* obj, float x, float y, float h);  // 0x4e5570
void* __cdecl f_4ee910(void* parent, float a, float b, float c, float d);        // 0x4ee910
void* __cdecl f_4eea90(void* obj, float* a, float* b, void* x);                  // 0x4eea90
void __fastcall f_49b120(void* self, void** p);     // 0x49b120 (this = 0x5d7080)

struct Mesh {
    void scale(float s);                            // 0x44f0c0
};

struct FlagWidget {
    char pad0[0x168];
    float f168;                                     // +0x168
    float f16c;                                     // +0x16c
    float f170;                                     // +0x170
    int f178;                                       // +0x178
    float f17c;                                     // +0x17c
    char pad180[0x184 - 0x180];
    int f184;                                       // +0x184
};

struct Slider : Ui {
    void set_value(float v);                        // 0x48baa0
};

extern char g_game[];                               // 0x5e2930
extern char g_5d8788;                               // 0x5d8788

struct ParamEntry {                                 // 0x2c bytes, value at +0x20
    tstring name;
    char pad10[0x20 - 0x10];
    char* value;                                    // +0x20
    char pad24[0x2c - 0x24];
};
extern ParamEntry g_params[];                       // 0xa44240
extern int g_param_count;                           // 0xa70240
extern tstring* g_msg_begin;                        // 0x5e23c4
extern tstring* g_msg_end;                          // 0x5e23c8

// 0x188-byte tab widget: ctor 0x4931a0, vtable 0x597050; +0x184 is its "visible" flag.
struct TabWidget {
    char pad0[0x184];
    int f184;                                       // +0x184
};

static void* alloc(unsigned n)
{
    return ::operator new(n);
}

static Ui* make_tab(Ui* frame, const char* name, float x, float y, float h)
{
    tstring n(name);
    void* obj = lookup_ui((void*)0x5e2a18, &n);
    void* w = make_tab_widget(frame, obj, x, y, h);
    return (Ui*)w;
}

static float param_value(const char* key)
{
    tstring k(key);
    int found = -1;
    for (int i = 0; i < g_param_count; i++) {
        if (g_params[i].name.equals(k)) {
            found = i;
            break;
        }
    }
    float v = 0.0f;
    if (found >= 0) {
        char* s = g_params[found].value;
        if (s)
            v = (float)_atof(s);
    }
    return v;
}

struct LogWidget {
    char pad0[0x17c];
    int f17c;                                       // +0x17c
    int f180;                                       // +0x180
    int f184;                                       // +0x184
    char pad188[0x18c - 0x188];
    int f18c;                                       // +0x18c
    int f190;                                       // +0x190
    int f194;                                       // +0x194
    char pad198[0x1ac - 0x198];                     // +0x198 subobject
    int f1ac;                                       // +0x1ac
    int f1b0;                                       // +0x1b0
    int f1b4;                                       // +0x1b4
    char pad1b8[0x1bc - 0x1b8];
    int f1bc;                                       // +0x1bc
    int f1c0;                                       // +0x1c0
    int f1c4;                                       // +0x1c4
    int f1c8;                                       // +0x1c8
    char pad1cc[0x1d0 - 0x1cc];

    LogWidget();
};

// @ 0x4fd0e0 -------------------------------------------------------------------------------
struct TStringArray {
    void* destroy(unsigned flags);                  // 0x4fd0e0 (this = first element)
};

void* TStringArray::destroy(unsigned flags)
{
    tstring* first = (tstring*)this;
    if (flags & 2) {
        int n = *(int*)((char*)this - 4);
        tstring* p = first + n;
        for (int i = n - 1; i >= 0; i--) {
            --p;
            p->~tstring();
        }
        if (flags & 1)
            ::operator delete[]((char*)this - 4);
        return (char*)this - 4;
    }
    first[0].~tstring();
    if (flags & 1)
        ::operator delete(this);
    return this;
}

// @ 0x4fd150 -------------------------------------------------------------------------------
LogWidget::LogWidget()
{
    ui_ctor_48ae20(this);
    *(void**)this = g_vt_5995c0;
    f17c = 0;
    f180 = 0;
    f184 = 0;
    f18c = 0;
    f190 = 0;
    f194 = 0;
    ((LogSub*)((char*)this + 0x198))->init(0);
    f1ac = 0;
    f1b0 = 0;
    f1b4 = 0;
    f1bc = 0;
    f1c0 = 0;
    f1c4 = 0;
    f1c8 = -1;
}

// @ 0x4fc820 -------------------------------------------------------------------------------
struct LogUi {
    char pad0[0x2c];
    Ui* frame;                                      // +0x2c
    Ui* log;                                        // +0x30
    Ui* bar;                                        // +0x34
    Ui* slider;                                     // +0x38
    Ui* tab_return;                                 // +0x3c
    Ui* tab_log;                                    // +0x40
    Ui* tab_recent;                                 // +0x44
    Ui* tab_troops;                                 // +0x48
    Ui* tab_locations;                              // +0x4c
    Ui* tab_factions;                               // +0x50
    Ui* tab_quests;                                 // +0x54

    void build(int show);                           // 0x4fc820
};

void LogUi::build(int show)
{
    gui_manager_reset((void*)0x5d7080);

    void* p = alloc(0x190);
    Ui* frame;
    if (p) {
        ui_ctor_4931a0(p);
        *(void**)p = g_vt_592848;
        ((char*)p)[0x188] = 0;
        frame = (Ui*)p;
    } else {
        frame = 0;
    }
    this->frame = frame;
    {
        float a[2] = { 0.0f, 0.0f };
        frame->set_size(a);
        float b[2] = { 1.0f, 0.75f };
        frame->set_pos(b);
    }
    {
        tstring name("game_log_window");
        void* src = resmgr_get_object(&name);
        void* m = mesh_clone(src);
        frame->set_mesh(0, m);
    }

    p = alloc(0x1d0);
    LogWidget* log = p ? new (p) LogWidget : 0;
    this->log = (Ui*)log;
    {
        int v = 0x700;
        ((Ui*)log)->set_int(&v);
        float a[2] = { 0.0f, 0.0f };
        ((Ui*)log)->set_size(a);
        float b[2] = { 0.02f, 0.02f };
        ((Ui*)log)->set_inset(b);
        int c = 0xff000000;
        ((Ui*)log)->set_color98(&c);
        float d[2] = { 0.915f, 0.1f };
        ((Ui*)log)->set_pos(d);
    }
    tstring* text = (tstring*)((char*)log + 0x128);
    text->assign("");

    if (show) {
        char delim = '|';
        int count = 0;
        tstring* arr = ((tstring*)(g_game + 0x13b50))->split(delim, count);
        if (count > 2) {
            int start = count - 300;
            if (start < 0)
                start = 0;
            for (int i = start; i < count; i += 3) {
                text->append(arr[i]);
                text->push_back('\t');
                text->append(arr[i + 2]);
                text->push_back('\n');
            }
            text->erase(text->len - 1, 1);
        }
        if (arr)
            ((TStringArray*)arr)->destroy(3);
    } else {
        int n = 0;
        if (g_msg_begin)
            n = ((int)(g_msg_end - g_msg_begin) / 0x10) * 3;
        for (int i = 0; i < n; i += 3) {
            text->append(g_msg_begin[i / 3]);
            text->push_back('\n');
        }
        if (text->len > 0)
            text->erase(text->len - 1, 1);
    }
    {
        char outbuf[16];
        ((Ui*)log)->set_name((tstring*)outbuf, text);
    }

    float y = *(float*)((char*)log + 0xf4) + 0.08f;

    p = alloc(0x188);
    Ui* bar;
    if (p) {
        ui_ctor_4931a0(p);
        *(void**)p = g_vt_597050;
        bar = (Ui*)p;
    } else
        bar = 0;
    this->bar = bar;
    {
        float a[2] = { 0.0f, 0.0f };
        bar->set_size(a);
        float b[2] = { 0.915f, y };
        bar->set_pos(b);
        bar->add_child((Ui*)log);
    }

    void* made = f_4ee910(frame, 0.915f, 0.08f, 0.02f, 0.64f);
    {
        float a[2] = { 0.03f, 0.03f };
        float b[2] = { 0.915f, y + 0.03f };
        made = f_4eea90(bar, a, b, made);
    }
    this->slider = (Ui*)made;
    frame->add_child((Ui*)made);

    const char* names[7] = {
        "ui_game_log", "ui_recent_messages", "ui_quests", "ui_troops2",
        "ui_locations", "ui_factions", "ui_return"
    };
    Ui** slots[7] = {
        &this->tab_log, &this->tab_recent, &this->tab_quests, &this->tab_troops,
        &this->tab_locations, &this->tab_factions, &this->tab_return
    };
    float x = param_value("note_button_x");
    float yy = param_value("note_button_y");
    float h = param_value("note_button_h");
    float dy = param_value("note_button_x_a");
    for (int i = 0; i < 7; i++) {
        Ui* w = make_tab(frame, names[i], x, yy, h);
        *slots[i] = w;
        ((TabWidget*)w)->f184 = 1;
        yy += dy;
    }

    Ui* active;
    if (show) {
        this->tab_log->select(1);
        active = this->tab_log;
    } else {
        this->tab_recent->select(1);
        active = this->tab_recent;
    }
    active->refresh();

    g_5d8788 = 1;
    f_49b120((void*)0x5d7080, (void**)&frame);

    FlagWidget* flag = *(FlagWidget**)((char*)this->slider + 0x484);
    float* pf = (float*)((char*)flag + 0xf0 + flag->f178 * 4);
    float value = (*pf - flag->f17c)
                * ((flag->f168 - flag->f16c) / (flag->f170 - flag->f16c))
                - (flag->f17c / *pf) * 5000.0f;
    ((Slider*)flag)->set_value(value);
    ((Ui*)flag)->flag_cmd(0x10000);
}
