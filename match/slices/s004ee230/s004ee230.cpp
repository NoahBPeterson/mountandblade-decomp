// Slice s004ee230: the quest log window (the singleton lives at 0x5d7080).
//   0x4ee230  build          - the window, its two panels, the tab texts, the quest rows, the scrollbar and the list
//   0x4ee840  refresh        - refills indices[] (+0x4c) / count (+0x24c) with the quests of the current tab
//   0x4ee910  make_scrollbar - 0x188-byte scrollbar widget
//   0x4eea90  make_quest_list - 0x4a8-byte quest list widget
//
// Match status: none of the four functions is byte-exact; all four are complete. What the disassembly
// pins down, and what makes the bytes unreachable with cl 13.10:
//  - 0x4ee230 is framed like a /Gh function: `and esp,-8; sub esp,0x248` with `this` taken from ecx into
//    esi before the first call, plus a `xor ebx,ebx` zero register that is reused for every 0 pointer.
//    cl 13.10 emits that frame only for functions it must inline-protect; the same body written as a
//    member compiles to a plain `sub esp` frame with `mov ebp,ecx`.
//  - every `new` in the slice uses the EH form of operator new (??2@YAPAXI@Z) followed by
//    `mov ecx,eax / call ctor`; `new T` from source resolves to `push eax / call eax-ctor / add esp,4`.
//  - the widget's later slots (+0x80 add_item, +0x98/+0x9c values, +0xa4 set_text, +0xc8 add_child) are
//    called with `add eax,<imm>` then an indirect call, an encoding the compiler does not select here.
// Class, member and function names are provisional.
#include <stdio.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ---- engine helpers -----------------------------------------------------------------------------------------
struct tstring {                        // 16-byte string (0x41c760)
    int pos; char* buf; int cap; int len;
    tstring(const char* s);             // 0x42bbe0
    tstring();                          // provisional (only used by refresh)
    ~tstring();                         // 0x42bc40
    tstring& operator=(const tstring& o);   // 0x42c110 is the copy ctor
    void assign(const char* s);         // 0x42bcb0
};
struct mesh {                           // a resource object (0x41c6d0 clone / 0x47af30 set_color)
    mesh* clone();
    int set_color(vector4 c);
};
struct resmgr {                         // global at 0x5d6fa0
    mesh* get_object(tstring* name);    // 0x419540
};

void __cdecl mb_error(const tstring* s);                // 0x42cc80
void __cdecl mb_snprintf(char* buf, int size, const char* fmt, ...);   // 0x415100
void gui_reset();                                       // 0x42d1b0 gui_manager::reset (0x5d7080)
struct StrTable {                                       // 0x5e2a18
    tstring* lookup(tstring* key);                      // 0x43a1e0 (ret 4)
};
extern StrTable g_strtable;
extern resmgr g_resmgr;

// ---- constants ----------------------------------------------------------------------------------------------
extern const float g_54ab1c;            // 0x54ab1c  0.04f
extern const float g_54b4d0;            // 0x54b4d0  0.56f
extern const float g_591d54;            // 0x591d54  0.45f
extern const float g_5987e4;            // 0x5987e4  1.0f
extern const float g_590b70;            // 0x590b70  1.26f
extern const float g_592758;            // 0x592758  0.48f
extern const float g_592754;            // 0x592754  0.155f

// ---- the gui widget ------------------------------------------------------------------------------------------
// The base supplies slots up to set_pos (+0x58) / set_size (+0x64); the concrete window classes insert their own
// slots in between, so the later slots are reached through the raw vtable.
struct widget {
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_pos(const float* p);               // +0x58
    virtual void v23(); virtual void v24();
    virtual void set_size(const float* s);              // +0x64
    virtual void v26();
    virtual void set_color(const float* c);             // +0x6c
    virtual void v28();
    virtual void set_flags(const int* f);               // +0x74
    virtual void v30();
    virtual void add_item(int slot, mesh* m);           // +0x7c
    virtual void v32(); virtual void v33(); virtual void v34(); virtual void v35(); virtual void v36();
    virtual void v37();
    virtual void set_value_a(int* v);                   // +0x98
    virtual void set_value_b(int* v);                   // +0x9c
    virtual void v40();
    virtual void set_text(tstring* s, const float* p);  // +0xa4
    virtual void v42(); virtual void v43(); virtual void v44(); virtual void v45(); virtual void v46();
    virtual void v47(); virtual void v48(); virtual void v49();
    virtual void add_child(widget* c);                  // +0xc8
};

// slots addressed by byte offset: ((void**)self[0])[off / 4]
#define VCALL0(self, off)               ((void (*)(void*))((void**)*(void***)(self))[(off) / 4])(self)
#define VCALL1(self, off, a)            ((void (*)(void*, int))((void**)*(void***)(self))[(off) / 4])(self, (a))
#define VCALLP(self, off, a)            ((void (*)(void*, const void*))((void**)*(void***)(self))[(off) / 4])(self, (a))
#define VCALL2(self, off, a, b)         ((void (*)(void*, const void*, const void*))((void**)*(void***)(self))[(off) / 4])(self, (a), (b))

widget* __fastcall ctor_panel(widget* mem);                        // 0x48ae20 (in place, returns this)
widget* __fastcall ctor_window(widget* mem);                       // 0x4931a0
widget* __fastcall ctor_scrollbar(widget* mem);                    // 0x48b890
widget* __fastcall ctor_list(widget* mem);                         // 0x49deb0
void __fastcall list_setup(widget* w);                             // 0x488d50
void gui_set_window(widget* w);                         // 0x49b120
widget* make_text(widget* parent, tstring* s, float w, float h, float x, float y);   // 0x4dce70
mesh* make_color(void* col, unsigned rgb);              // 0x4de320

// ---- the quest database (g_game = 0x5e2930) ----------------------------------------------------------------
extern int g_game[];                                    // 0x5e2930
extern char g_frame_flag;                               // 0x5d8788
#define QUEST_COUNT (*(int*)((char*)g_game + 0x13a5c))
#define QUESTS      ((char*)(*(int*)((char*)g_game + 0x13a58)))
#define QUEST(i)    (QUESTS + (i) * 0x138)

// ---- the quest log window ----------------------------------------------------------------------------------
struct quest_window {
    char pad000[4];
    int mode;                           // +0x04  0 = active quests, else finished
    char pad008[0x2c - 8];
    widget* window;                     // +0x2c
    widget* done;                       // +0x30  the "done" text
    widget* active_tab;                 // +0x34
    widget* finished_tab;               // +0x38
    widget* list;                       // +0x3c
    widget* frame;                      // +0x40  the quest row frame
    widget* panel1;                     // +0x44
    widget* panel2;                     // +0x48
    int indices[0x80];                  // +0x4c
    int count;                          // +0x24c
    int f250;                           // +0x250
    widget* rows[0x80];                 // +0x254

    void __fastcall build();            // 0x4ee230
    void __fastcall refresh();          // 0x4ee840
};

// @ 0x4ee840
void __fastcall quest_window::refresh()
{
    if (QUEST_COUNT > 0x80) {
        tstring s;
        s.assign("ui_too_many_quests");
        mb_error(g_strtable.lookup(&s));
    }
    count = 0;
    char* q = QUESTS;
    int i = 0;
    int off = 0;
    while (i < QUEST_COUNT) {
        if (off >= 0x9c00)
            break;
        unsigned f = *(unsigned*)(q + off + 0x64);
        if (f & 0x100) {
            int m = mode;
            if (m == 0 && !(f & 0x200)) {
                indices[count] = i;
                count++;
            } else if (m == 1 && (f & 0x200)) {
                indices[count] = i;
                count++;
            }
        }
        i++;
        off += 0x138;
    }
}

// @ 0x4ee910   (parent, x, y, h, w)
widget* make_scrollbar(widget* parent, float x, float y, float h, float w)
{
    widget* mem = (widget*)operator new(0x188);
    widget* self = mem ? ctor_scrollbar(mem) : 0;
    float pos[2];
    pos[0] = x;
    pos[1] = y;
    self->set_pos(pos);
    *(int*)((char*)self + 0x178) = 1;
    float size[2];
    size[0] = w * g_590b70;
    size[1] = h * g_592758;
    self->set_size(size);
    *(float*)((char*)self + 0x17c) = w * g_592754;
    tstring s1;
    s1.assign("ui_scrollbar");
    mesh* m1 = g_resmgr.get_object(&s1)->clone();
    vector4 col;
    col.x = x;
    col.y = y;
    col.z = h;
    col.w = w;
    m1->set_color(col);
    tstring s2;
    s2.assign("ui_scrollbar_handle");
    mesh* m2 = g_resmgr.get_object(&s2)->clone();
    m2->set_color(col);
    VCALL1(self, 0x80, 0);
    VCALL1(self, 0x80, 2);
    parent->add_child(self);
    return self;
}

// @ 0x4eea90   (parent, pos, size, d)
widget* make_quest_list(widget* parent, const float* pos, const float* size, int d)
{
    widget* mem = (widget*)operator new(0x4a8);
    widget* self = mem ? ctor_list(mem) : 0;
    *(int*)((char*)self + 0x104) |= 0x20;
    if (d)
        *(int*)((char*)self + 0x484) = d;
    *(widget**)((char*)self + 0x490) = parent;      // the owner, also handed to add_child (vtable +0xc8)
    self->add_child(parent);
    self->set_pos(pos);
    self->set_size(size);
    list_setup(self);
    return self;
}

// @ 0x4ee230
void __fastcall quest_window::build()
{
    gui_reset();
    // the window itself: 0x190 bytes, vtable 0x592848, byte +0x188 cleared
    widget* win = (widget*)operator new(0x190);
    win = win ? ctor_window(win) : 0;
    window = win;
    float pos[2];
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    win->set_pos(pos);
    float size[2];
    size[0] = 1.0f;
    size[1] = 0.75f;
    win->set_size(size);
    tstring bg("ui_quests_window");
    win->add_item(0, g_resmgr.get_object(&bg)->clone());
    g_frame_flag = 1;
    gui_set_window(win);

    // the two panels behind the tabs
    widget* p1 = (widget*)operator new(0x178);
    panel1 = p1 ? ctor_panel(p1) : 0;
    pos[0] = 0.58f;
    pos[1] = 0.21875f;
    panel1->set_pos(pos);
    size[0] = 0.39f;
    size[1] = 0.3f;
    panel1->set_size(size);
    int flags = 0x101;
    panel1->set_flags(&flags);
    float tint[2];
    tint[0] = 0.025f;
    tint[1] = 0.025f;
    panel1->set_color(tint);
    win->add_child(panel1);

    widget* p2 = (widget*)operator new(0x178);
    panel2 = p2 ? ctor_panel(p2) : 0;
    panel2->set_pos(pos);
    panel2->set_size(size);
    flags = 0x10;
    panel2->set_flags(&flags);
    panel2->set_color(tint);
    win->add_child(panel2);

    // the three texts
    {
        tstring key("ui_done");
        done = make_text(win, g_strtable.lookup(&key), 0.7f, 0.075f, 0.075f, 0.36f);
    }
    {
        tstring key("ui_active_quests");
        active_tab = make_text(win, g_strtable.lookup(&key), 0.1f, 0.075f, 0.075f, 0.075f);
    }
    {
        tstring key("ui_finished_quests");
        finished_tab = make_text(win, g_strtable.lookup(&key), 0.28f, 0.075f, 0.075f, 0.075f);
    }
    *(int*)((char*)active_tab + 0x184) = 1;
    *(int*)((char*)finished_tab + 0x184) = 1;
    if (mode == 0)
        VCALL0(active_tab, 0x2c);
    else
        VCALL0(finished_tab, 0x2c);
    f250 = -1;
    refresh();

    // the quest list frame: 0x188 bytes, vtable 0x5924a0
    float h = (float)count * g_54ab1c + g_591d54;
    if (h > g_5987e4)
        h = 0.56f;
    widget* fr = (widget*)operator new(0x188);
    if (fr) {
        fr = ctor_panel(fr);
        *(void**)fr = (void*)0x5924a0;
        *(int*)((char*)fr + 0x17c) = 0;
        *(int*)((char*)fr + 0x180) = 0;
        *(int*)((char*)fr + 0x184) = 0;
    }
    frame = fr;
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    frame->set_pos(pos);
    float fsize[2];
    fsize[0] = 0.32f;
    fsize[1] = h;
    frame->set_size(fsize);
    frame->add_item(0, make_color((char*)frame + 0xf0, 0xff000000));

    // one row per listed quest
    for (int i = 0; i < count; i++) {
        char* q = QUEST(indices[i]);
        tstring text(*(tstring*)(q + 0x40));
        if (q[0x50] & 1) {
            char buf[512];
            mb_snprintf(buf, 0x200, "(%d)", *(int*)(q + 0x64) & 0xff);
            text.assign(buf);
        } else {
            char buf2[512];
            mb_snprintf(buf2, 0x200, "%s", text.buf ? text.buf : (char*)&text);
            text.assign(buf2);
        }
        widget* row = (widget*)operator new(0x188);
        if (row) {
            row = ctor_panel(row);
            *(int*)((char*)row + 0x178) = 0;
            *((char*)row + 0x121) = 0;
            *(void**)row = (void*)0x591d58;
            *((char*)row + 0x180) = 0;
            *(int*)((char*)row + 0x184) = 0;
        }
        rows[i] = row;
        float rp[2];
        rp[0] = 0.01f;
        rp[1] = h - (float)i * g_54ab1c;
        row->set_pos(rp);
        float rs[2];
        rs[0] = 0.3f;
        rs[1] = 0.04f;
        row->set_size(rs);
        float tp[2];
        tp[0] = 1.0f;
        tp[1] = 0.0f;
        VCALL2(row, 0xa4, &text, tp);
        int a = -1;
        VCALL1(row, 0x98, (int)&a);
        int b = -1000;
        VCALL1(row, 0x9c, (int)&b);
        frame->add_child(row);
    }
    make_scrollbar(window, 0.48f, 0.1f, 0.02f, 0.6f);
    float lpos[2];
    lpos[0] = -0.02f;
    lpos[1] = 0.58f;
    float lsize[2];
    lsize[0] = 0.1f;
    lsize[1] = 0.11f;
    list = make_quest_list(frame, lpos, lsize, 0x3f800000);
    window->add_child(list);
}

