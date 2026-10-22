// Slice s004fbfa0: (re)builds the in-game HUD root (0x4fbfa0) and the one-argument thunk that
// forwards the game-log visibility flag (0x4fc810). Names provisional; offsets from the disassembly.
#include <windows.h>
#include <new>

struct vector4 { float x, y, z, w; };

struct tstring {                                    // 0x10 bytes
    int pos; char* buf; int cap; int len;
    tstring(const char* s);                        // 0x42bbe0
    ~tstring();                                    // 0x42bc40
    void operator=(const tstring& o);              // 0x42bc50
    bool differs(const char* s) const;             // 0x42bdb0
};

// The shared widget interface (offsets confirmed by s004f7f00 / s0048add0).
struct Ui {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_size(float* v);                // +0x58
    virtual void v23();                             // +0x5c
    virtual void v24();                             // +0x60
    virtual void set_pos(float* v);                 // +0x64
    virtual void v26();                             // +0x68
    virtual void v27();                             // +0x6c
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
    virtual void v44();                             // +0xb0
    virtual void v45();                             // +0xb4
    virtual void v46(); virtual void v47(); virtual void v48(); virtual void v49();
    virtual void add_child(Ui* c);                  // +0xc8
};

// A 0x178-byte base widget (ctor 0x48ae20).
struct UiBase : Ui {
    char pad178[0x178 - sizeof(Ui*)];
};

extern void* g_vt_592848[];                         // 0x592848
extern void* g_vt_592140[];                         // 0x592140
extern void* g_vt_5924a0[];                         // 0x5924a0
extern void* g_vt_591d58[];                         // 0x591d58

void __fastcall ui_ctor_4931a0(void* p);            // 0x4931a0
void __fastcall ui_ctor_48ae20(void* p);            // 0x48ae20
void __fastcall ui_ctor_49deb0(void* p);            // 0x49deb0
void __fastcall emit_simple(void* p, int n);        // 0x4875e0
void* __fastcall mesh_clone(void* m);               // 0x41c6d0
void* __fastcall resmgr_get_object(tstring* name);  // 0x419540 (this = 0x5d6fa0)
void* __fastcall f_4991d0(void* self, int* a, int* b);  // 0x4991d0
void __fastcall f_48d470(void* self);               // 0x48d470
void __fastcall f_498420(void* self, float* v);     // 0x498420
int __cdecl f_4ee910(void* parent, float a, float b, float c, float d);   // 0x4ee910
int __cdecl f_4eea90(void* obj, float* a, float* b, int x);               // 0x4eea90
void __fastcall f_4dd920(void* self, void* frame, int id);                // 0x4dd920
void __fastcall f_49b120(void* self, void** p);     // 0x49b120

void __fastcall gui_manager_reset(void* self);      // 0x42d1b0 (this = 0x5d7080)
void __fastcall f_494310(void* self);               // 0x494310 (this = 0x5aa128)

struct Mesh {
    void set_color(vector4 c);                      // 0x47af30
    void translate(const vector4* v);               // 0x44f240
    void scale(float s);                            // 0x44f0c0
};

struct game_data {
    char pad0[0xa8];
    int f_a8;                                       // +0x0a8
    char padac[0xc8 - 0xac];
    int f_c8;                                       // +0x0c8
    char padcc[0x13aec - 0xcc];
    int selected;                                   // +0x13aec
    char pad13af0[0x1a210 - 0x13af0];
    tstring log_name;                               // +0x1a210
    int f_1a220;                                    // +0x1a220
    int f_1a224;                                    // +0x1a224
    float f_1a228;                                  // +0x1a228
    float f_1a22c;                                  // +0x1a22c
    float f_1a230;                                  // +0x1a230
};
extern game_data* g_game;                           // 0x5e2930
extern char g_5e29b0[];                             // 0x5e29b0 (record array, stride 0x60)
extern float g_5d75d8;                              // 0x5d75d8
extern float g_5d75dc;                              // 0x5d75dc
extern char g_5d8788;                               // 0x5d8788

struct GameUi {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08();
    virtual void begin();                           // +0x24
    char pad28[0x2c - 0x28];
    Ui* frame;                                      // +0x2c
    char pad30[0x40 - 0x30];
    int f40;                                        // +0x40
    char pad44[0x60 - 0x44];
    Ui* kids[16];                                   // +0x60
    int fA0;                                        // +0xa0
    char padA4[0xe4 - 0xa4];
    Ui* f_e4;                                       // +0xe4
    void* f_e8;                                     // +0xe8
    Ui* f_ec;                                       // +0xec
    char padF0[0xf4 - 0xf0];
    Ui* f_f4;                                       // +0xf4
};

static inline void* alloc(unsigned n)
{
    return ::operator new(n);
}

// @ 0x4fbfa0 -------------------------------------------------------------------------------
void __fastcall game_ui_build(GameUi* self)
{
    gui_manager_reset((void*)0x5d7080);
    f_494310((void*)0x5aa128);

    char* rec = g_5e29b0 + g_game->selected * 0x60;
    int color = *(int*)(rec + 0x24) | 0xff000000;
    bool ok = g_game->f_c8 == 0;

    g_game->f_c8 = 0;
    g_game->f_a8 = 0;
    g_game->log_name = *(tstring*)(rec + 0x28);
    g_game->f_1a220 = -1;
    emit_simple(rec + 0x38, 8);
    if (g_game->f_c8 != 0)
        ok = false;

    for (int i = 0; i < 16; i++)
        self->kids[i] = 0;

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
    self->frame = frame;
    {
        float a[2] = { -0.001f, -0.001f };
        frame->set_size(a);
        float b[2] = { 1.0020001f, 0.75200003f };
        frame->set_pos(b);
    }

    g_game->f_a8 = -1;

    p = alloc(0x178);
    Ui* w = p ? ((ui_ctor_48ae20(p), (Ui*)p)) : 0;
    self->f_f4 = w;
    {
        float a[2] = { 0.1f, 0.1f };
        w->set_size(a);
        float b[2] = { 1e-05f, 1e-05f };
        w->set_pos(b);
    }
    w->v44();
    frame->add_child(w);

    if (ok) {
        if (g_game->log_name.differs("none")) {
            void* src = resmgr_get_object(&g_game->log_name);
            if (src) {
                Mesh* m = (Mesh*)mesh_clone(src);
                rec = g_5e29b0 + g_game->selected * 0x60;
                if ((*(unsigned*)(rec + 0x20) & 0x1000) && 1.35f < g_5d75d8 / g_5d75dc) {
                    float f = 1.33f / (g_5d75d8 / g_5d75dc);
                    vector4 c;
                    c.x = f; c.y = 1.0f; c.z = 1.0f; c.w = 1.0f;
                    m->set_color(c);
                    vector4 tr;
                    tr.x = 1.0f - f; tr.y = 0.0f; tr.z = 0.0f; tr.w = 0.0f;
                    m->translate(&tr);
                }
                frame->set_mesh(2, m);
            }
        }
        if (g_game->f_1a220 >= 0) {
            Mesh* m = (Mesh*)f_4991d0((void*)0xa75f40, &g_game->f_1a220, &g_game->f_1a224);
            if (g_game->f_1a230 != 1.0f)
                m->scale(g_game->f_1a230);
            self->f_f4->set_mesh(0, m);
            float* sz = &g_game->f_1a228;
            self->f_f4->set_size(sz);
            self->f_f4->v45();
        }
    }

    {
        int c = -1;
        frame->set_color98(&c);
    }
    self->fA0 = 0;

    p = alloc(0x178);
    Ui* panel = p ? ((ui_ctor_48ae20(p), (Ui*)p)) : 0;
    self->f_e4 = panel;
    {
        float a[2] = { 0.0f, 0.0f };
        panel->set_size(a);
        int v = 0x710;
        panel->set_int(&v);
        float b[2] = { 1.0f, 0.75f };
        panel->set_pos(b);
        panel->set_color98(&color);
    }
    {
        tstring name("");
        char outbuf[16];
        panel->set_name((tstring*)outbuf, &name);
    }

    // the "flag" widget (vtable 0x592140)
    p = alloc(0x188);
    Ui* flag;
    if (p) {
        ui_ctor_48ae20(p);
        *(void**)p = g_vt_592140;
        ((int*)p)[0x178 / 4] = 0;
        *(float*)((char*)p + 0x17c) = 0.1f;
        *(float*)((char*)p + 0x168) = 1.0f;
        ((int*)p)[0x180 / 4] = 0;
        ((int*)p)[0x184 / 4] = 0;
        flag = (Ui*)p;
    } else
        flag = 0;
    ((int*)flag)[0x178 / 4] = 1;
    {
        float a[2] = { 0.0f, 0.0f };
        flag->set_size(a);
    }
    frame->add_child(flag);

    // the tree / layout object
    p = alloc(0x4a8);
    void* tree = p ? ((ui_ctor_49deb0(p), p)) : 0;
    self->f_e8 = tree;
    *(unsigned*)((char*)tree + 0x104) |= 0x20;
    *(Ui**)((char*)tree + 0x484) = flag;

    p = alloc(0x188);
    Ui* bar;
    if (p) {
        ui_ctor_48ae20(p);
        *(void**)p = g_vt_5924a0;
        ((int*)p)[0x17c / 4] = 0;
        ((int*)p)[0x180 / 4] = 0;
        ((int*)p)[0x184 / 4] = 0;
        bar = (Ui*)p;
    } else
        bar = 0;
    {
        float a[2] = { 0.0f, 0.0f };
        bar->set_size(a);
        bar->set_pos((float*)((char*)self->f_e4 + 0xf0));
        bar->add_child(self->f_e4);
    }
    *(Ui**)((char*)tree + 0x490) = bar;
    ((Ui*)tree)->add_child(bar);
    {
        float a[2] = { 0.0f, 0.0f };
        ((Ui*)tree)->set_size(a);
        float b[2] = { 0.01f, 0.38f };
        ((Ui*)tree)->set_size(b);
    }
    f_48d470(tree);
    {
        float v[2] = { 0.65f, 0.31f };
        f_498420(tree, v);
    }
    frame->add_child((Ui*)tree);

    // the value bar
    p = alloc(0x188);
    Ui* value;
    if (p) {
        ui_ctor_48ae20(p);
        *(void**)p = g_vt_5924a0;
        ((int*)p)[0x17c / 4] = 0;
        ((int*)p)[0x180 / 4] = 0;
        ((int*)p)[0x184 / 4] = 0;
        value = (Ui*)p;
    } else
        value = 0;
    {
        float a[2] = { 0.0f, 0.0f };
        value->set_size(a);
        float b[2] = { 0.75f, 0.36f };
        value->set_pos(b);
    }
    int made = f_4ee910(frame, 0.7f, 0.02f, 0.01f, 0.36f);
    {
        float a[2] = { 0.01f, 0.02f };
        float b[2] = { 0.75f, 0.36f };
        made = f_4eea90(value, a, b, made);
    }
    self->f_ec = (Ui*)made;
    frame->add_child((Ui*)made);

    for (int i = 0; i < 16; i++) {
        p = alloc(0x188);
        Ui* child;
        if (p) {
            ui_ctor_48ae20(p);
            ((int*)p)[0x178 / 4] = 0;
            ((char*)p)[0x121] = 0;
            *(void**)p = g_vt_591d58;
            ((char*)p)[0x180] = 0;
            ((int*)p)[0x184 / 4] = 0;
            child = (Ui*)p;
        } else
            child = 0;
        self->kids[i] = child;
        int c = 0xff55ff50;
        child->set_color9c(&c);
        float pos[2] = { 0.64f, 0.04f };
        child->set_pos(pos);
        child->set_color98(&color);
        child->v44();
        value->add_child(child);
    }

    if (ok) {
        self->begin();
        f_4dd920((char*)self + 0x30, frame, g_game->f_a8);
        if (self->f40 < 0) {
            tstring name("meeting_window");
            void* src = resmgr_get_object(&name);
            void* m = mesh_clone(src);
            frame->set_mesh(0, m);
        }
    }
    g_5d8788 = 1;
    f_49b120((void*)0x5d7080, (void**)&frame);
}

// @ 0x4fc810 -------------------------------------------------------------------------------
struct LogUi {
    char pad0[4];
    int f4;                                         // +0x04
    void build(int show);                           // 0x4fc820
};

void __fastcall log_ui_thunk(LogUi* self)
{
    int v = self->f4;
    self->build(v != 0);
}
