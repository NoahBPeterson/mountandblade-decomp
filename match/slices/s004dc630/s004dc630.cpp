// Slice s004dc630: the campaign "message window" (result window) setup 0x4dc630 and the two button makers
// it uses (0x4dcbd0 long button, 0x4dce70 medium button) around the shared maker 0x4dcc50.
// Names provisional; every offset is read off the disassembly.  T-regime region: the helpers are defined
// after their caller, in binary order, and kept out of line.
#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <new>
#include "rgl.h"

using rgl::vector4;

extern "C" int _fltused = 0;

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    tstring& operator=(const tstring& o);       // 0x42bc50
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct StrTable {
    const tstring* lookup(const tstring& key);  // 0x43a1e0
};
extern StrTable g_strtable;                     // 0x5e2a18

struct mesh {
    mesh* clone();                              // 0x41c6d0
    void scale(float s);                        // 0x44f0c0
    void set_color(vector4 c);                  // 0x47af30 (four floats by value)
};
struct resmgr { mesh* get_object(const tstring& name); };      // 0x419540
extern resmgr g_resmgr;                         // 0x5d6fa0

// set_text returns an 8-byte non-POD object through a hidden pointer that nobody destroys
struct WRet { int a, b; WRet(); };

// the widget base (ctor 0x48ae20, 0x178 bytes).  Vtable slots verified against the call sites.
struct widget {
    widget();
    virtual void v0(); virtual void v1(); virtual void v2();
    virtual void v3();                                          // +0x0c
    virtual void v4(); virtual void v5(); virtual void v6(); virtual void v7();
    virtual void v8(); virtual void v9(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_pos(const float* v);                       // +0x58
    virtual void v23(); virtual void v24();
    virtual void set_size(const float* v);                      // +0x64
    virtual void v26();
    virtual void set_scale(const float* v);                     // +0x6c
    virtual void set_pos2(const float* v);                      // +0x70
    virtual void v29();
    virtual void v30(const int* v);                             // +0x78
    virtual void set_mesh(int kind, mesh* m);                   // +0x7c
    virtual void set_mesh2(int layer, mesh* m);                 // +0x80
    virtual void v33(); virtual void v34(); virtual void v35(); virtual void v36();
    virtual void v37(); virtual void v38();
    virtual void set_color(const int* c);                       // +0x9c
    virtual void v40();
    virtual WRet set_text(const tstring& s);                    // +0xa4
    virtual void v42(); virtual void v43(); virtual void v44(); virtual void v45();
    virtual void v46(); virtual void v47(); virtual void v48(); virtual void v49();
    virtual void add_child(widget* child);                      // +0xc8

    char pad4[0x10c - 4];
    mesh* m10c;                                 // +0x10c
    mesh* m110;                                 // +0x110
    char pad114[0x121 - 0x114];
    char f121;                                  // +0x121
    char pad122[0x178 - 0x122];
};

// the 0x188-byte button / edit widgets
struct button : widget {
    int f178;                                   // +0x178
    char pad17c[4];
    unsigned char f180;                         // +0x180
    char pad181[3];
    unsigned f184;                              // +0x184
};
struct edit_box : button {
    edit_box();                                 // 0x48bb10
};

void* __cdecl operator new(unsigned n);         // 0x53545e
void __fastcall ctor_4931a0(void* p);           // 0x4931a0 (ecx = overlay)

// the 0x190-byte container window (vptr 0x592848)
struct overlay : widget {
    int f178, f17c, f180, f184;
    char f188;                                  // +0x188
};
static inline overlay* new_overlay()
{
    overlay* p = (overlay*)operator new(0x190);
    return p != 0 ? (ctor_4931a0(p), *(void**)p = (void*)0x592848, p->f188 = 0, p) : 0;
}

struct GuiMgr { void add_root(void* const& p); };               // 0x49b120
#define g_gui     (*(GuiMgr*)0x5d7080)
#define g_5d8788  (*(char*)0x5d8788)
#define g_5d75e0  (*(unsigned char*)0x5d75e0)
#define g_5d7090  (*(widget**)0x5d7090)

// the config table (0xa44240): 0x2c-byte entries from +0x1c, the value string first
struct cfg_entry { tstring val; char pad10[0x1c]; };
struct ini_file {
    int find(const tstring& key);               // 0x4bd5a0
};
#define g_cfg_entries ((cfg_entry*)0xa4425c)
#define g_config  (*(ini_file*)0xa44240)

extern float g_549cdc, g_590b6c, g_54b3b0, g_549d70, g_549d68;

__declspec(noinline) button* new_button(widget* parent, const tstring& text, float a, float b, float c,
                                        const tstring& name, const tstring& down, float k1, float k2, float k3);   // 0x4dcc50
button* new_long_button(widget* parent, const tstring* text, float a, float b, float c);     // 0x4dcbd0
button* new_medium_button(widget* parent, const tstring* text, float a, float b, float c);   // 0x4dce70

// the message window (`this`)
struct result_window {
    char pad0[4];
    int outcome;                                // +0x04
    char pad8[0x1c - 8];
    tstring s1c;                                // +0x1c
    overlay* win;                               // +0x2c
    widget* panel30;                            // +0x30
    widget* panel34;                            // +0x34
    edit_box* box38;                            // +0x38
    button* b3c;                                // +0x3c
    button* b40;                                // +0x40
    char pad44[0x54 - 0x44];
    tstring s54;                                // +0x54
    unsigned char f64;                          // +0x64
    char pad65[3];
    tstring s68;                                // +0x68
    tstring s78;                                // +0x78
    tstring s88;                                // +0x88
    void init();                                // 0x4dc630
};

// @ 0x4dc630
void result_window::init()
{
    float mv[4];
    if (outcome == 0) {
        tstring key("ui_ok");
        s78 = *g_strtable.lookup(key);
    }
    win = new_overlay();
    mv[0] = 0.25f; mv[1] = 0.2f; mv[2] = 0.5f; mv[3] = 0.44f;
    win->set_pos(mv);
    win->set_size(&mv[2]);
    mesh* m;
    {
        tstring name("message_window");
        m = g_resmgr.get_object(name)->clone();
    }
    win->set_mesh(0, m);

    panel30 = new widget;
    {
        float v[2] = { 0.05f, 0.34f };
        panel30->set_pos(v);
    }
    {
        float v[2] = { 0.4f, 0.05f };
        panel30->set_size(v);
    }
    panel30->set_text(s68);
    win->add_child(panel30);

    panel34 = new widget;
    {
        int fl = 0x100;
        panel34->v30(&fl);
    }
    {
        float v[2] = { 0.05f, 0.135f };
        panel34->set_pos(v);
    }
    {
        float v[2] = { 0.4f, 0.2f };
        panel34->set_size(v);
    }
    {
        float v[2] = { 0.02f, 0.02f };
        panel34->set_scale(v);
    }
    panel34->set_text(s1c);
    win->add_child(panel34);

    b3c = 0;
    b40 = 0;
    box38 = 0;
    if (outcome == 3) {
        b3c = new_medium_button(win, &s78, 0.15f, 0.085f, 0.045f);
        b40 = new_medium_button(win, &s88, 0.15f, 0.03f, 0.045f);
        box38 = new edit_box;
        {
            float y = 0.0f;
            tstring ky("yn_tb_name_y");
            int iy = g_config.find(ky);
            if (iy >= 0)
                { cfg_entry* en = g_cfg_entries + iy; y = (float)atof(en->val.c_str()); }
            float x = 0.0f;
            tstring kx("yn_tb_name_x");
            int ix = g_config.find(kx);
            if (ix >= 0)
                { cfg_entry* en = g_cfg_entries + ix; x = (float)atof(en->val.c_str()); }
            float v[2] = { x, y };
            box38->set_pos(v);
        }
        {
            float v[2] = { 0.0f, 0.009f };
            box38->set_pos2(v);
        }
        {
            float v[2] = { 0.02f, 0.02f };
            box38->set_scale(v);
        }
        {
            tstring n("party_button");
            box38->set_mesh2(2, g_resmgr.get_object(n)->clone());
        }
        {
            vector4 c(0.5f, 0.5f, 1.0f);
            box38->m110->set_color(c);
        }
        {
            tstring n("party_button_down");
            box38->set_mesh2(1, g_resmgr.get_object(n)->clone());
        }
        {
            vector4 c(0.5f, 0.5f, 1.0f);
            box38->m10c->set_color(c);
        }
        box38->v3();
        box38->set_text(s54);
        box38->f180 = 1;
        g_5d7090 = box38;
        win->add_child(box38);
    } else if (outcome == 2) {
        b3c = new_long_button(win, &s78, 0.03f, 0.085f, 0.045f);
        b40 = new_long_button(win, &s88, 0.03f, 0.03f, 0.045f);
    } else if (outcome == 1) {
        b3c = new_medium_button(win, &s78, 0.15f, 0.085f, 0.045f);
        b40 = new_medium_button(win, &s88, 0.15f, 0.03f, 0.045f);
    } else {
        b3c = new_medium_button(win, &s78, 0.15f, 0.05f, 0.045f);
    }
    {
        void* self = win;
        g_5d8788 = 1;
        g_gui.add_root(self);
    }
    f64 = g_5d75e0;
}


// @ 0x4dcbd0
button* new_long_button(widget* parent, const tstring* text, float a, float b, float c)
{
    tstring d("longer_button_down");
    tstring nm("longer_button");
    return new_button(parent, *text, a, b, c, nm, d, 9.7f, 0.072f, 0.085f);
}

// UTF-8 -> int code points (0x487b00; custom register convention in the original: text in edi, count out in edx)
static __declspec(noinline) void decode_utf8(const tstring* s, int* out_len, int* buf, const int* maxn)   // 0x487b00
{
    *out_len = 0;
    int n = s->len;
    int i = 0;
    if (n > 0) {
        do {
            if (*out_len >= *maxn)
                break;
            unsigned char c = s->buf[i];
            int ch;
            if (c >= 0xf0 && c < 0xf5) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((((q[0] & 7) << 6) | (q[1] & 0x3f)) << 6 | (q[2] & 0x3f)) << 6) | (q[3] & 0x3f);
                i += 4;
            } else if (c >= 0xe0 && c < 0xf0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = ((((q[0] & 0xf) << 6) | (q[1] & 0x3f)) << 6) | (q[2] & 0x3f);
                i += 3;
            } else if (c >= 0xc2 && c < 0xe0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((q[0] & 0x1f) << 6) | (q[1] & 0x3f));
                i += 2;
            } else {
                i++;
                if (c >= 0x80)
                    continue;
                ch = (signed char)c;
            }
            buf[*out_len] = ch;
            (*out_len)++;
        } while (i < n);
    }
}

// (parent, text, a, b, c, mesh name, pressed mesh name, k1, k2, k3): a button sized from the text width
__declspec(noinline)
// @ 0x4dcc50
button* new_button(widget* parent, const tstring& text, float a, float b, float c,
                   const tstring& name, const tstring& down, float k1, float k2, float k3)
{
    float tmp = c * k1;
    int buf[0x800];
    button* p = (button*)operator new(0x188);
    if (p != 0) {
        new (p) widget();
        p->f178 = 0;
        p->f121 = 0;
        *(void**)p = (void*)0x591d58;
        p->f180 = 0;
        p->f184 = 0;
    } else {
        p = 0;
    }
    {
        float v[2] = { a, b };
        p->set_pos(v);
    }
    {
        float v[2] = { tmp, c };
        p->set_size(v);
    }
    float r = g_549cdc / k2;
    float s = r * c;
    {
        mesh* m = g_resmgr.get_object(name)->clone();
        m->scale(s);
        p->set_mesh2(2, m);
    }
    {
        mesh* m = g_resmgr.get_object(down)->clone();
        m->scale(s);
        p->set_mesh2(1, m);
    }
    {
        int col = 0xff007700;
        p->set_color(&col);
    }
    float v = c * g_590b6c;
    int len;
    int maxn = 0x800;
    decode_utf8(&text, &len, buf, &maxn);
    float t = (float)len * v * g_54b3b0;
    float e = tmp - r * (c * k3);
    if (t > e) {
        v = v * (e / t);
        s = v;
    }
    {
        float sv[2] = { v, v };
        p->set_scale(sv);
    }
    {
        float pv[2] = { 0.0f, c * g_549d70 - s * g_549d68 };
        p->set_pos2(pv);
    }
    p->set_text(text);
    parent->add_child(p);
    return p;
}

// @ 0x4dce70
button* new_medium_button(widget* parent, const tstring* text, float a, float b, float c)
{
    tstring d("medium_button_down");
    tstring nm("medium_button");
    return new_button(parent, *text, a, b, c, nm, d, 3.7f, 0.072f, 0.085f);
}
