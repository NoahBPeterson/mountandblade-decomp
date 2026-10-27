// facegen_win::init (0x4f1460) -- the terrain/face generator page's initialiser.
//
//   three random seeds, terrain reset, page field defaults, gui reset;
//   the 0x190-byte board window (vtable 0x592848) with the "facegen_board" mesh, registered with the gui;
//   the 0x188-byte work panel (vtable 0x597050), hung off the board's child vector;
//   four check boxes, three seed rows (label + "randomize" button + 0x188 face), six sliders with a name
//   switch, the generate/close/key/copy/paste controls, the region-type and region-detail drop downs;
//   then the 0x358-byte camera body (ctor 0x4ed630), its 0x344-byte world (ctor 0x478ad0: 400x400 units
//   over a 6x6 cell grid), the view frustum copied in, the "prtcl_drop" particle entity, and the refresh.
//
// Widget slots are the verified ones of the 0x597050 interface (s004f25a0, charsheet_init):
//   +0x58 set_rect(float[2])  +0x64 set_scale(float[2])  +0x6c set_text_size(float[2])
//   +0x7c add_image(int, mesh*)  +0x80 set_mesh(int, mesh*)
//   +0xa4 set_text(tstring&) -> 8-byte object  +0xac set_text2(tstring&, float[2])
//   +0xc8 add_child(child) / the drop downs' add_item(tstring&)
// Names are provisional.  The two inlined helpers (0x4dce70 new_medium_button, 0x4e5570 new_short_button)
// construct "medium_button[_down]" / "short_button[_down]" around new_sized_button 0x4dcc50; label_at is the
// inlined caption-with-position idiom and load_mesh the tstring+get_object+clone pair.
#include <windows.h>
#include <new>
#include <vector>
#include "rgl.h"

// ---- engine string: 16 bytes {pos, buf, cap, len}, ctor 0x42bbe0, dtor 0x42bc40 ---------------------
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring(const char* s);                             // 0x42bbe0
    ~tstring();                                         // 0x42bc40
};

// ---- rgl mesh ---------------------------------------------------------------------------------------
struct mesh {
    mesh* clone();                                      // 0x41c6d0
    void scale(float s);                                // 0x44f0c0
};

struct resmgr {
    mesh* get_object(const tstring& name);              // 0x419540
};
extern resmgr g_alloc;                                  // 0x5d6fa0

// ---- gui manager ------------------------------------------------------------------------------------
struct GuiMgr {
    void reset();                                       // 0x42d1b0
    void add_root(void* node);                          // 0x49b120
};
extern GuiMgr g_gui;                                    // 0x5d7080
extern unsigned char g_gui_dirty;                       // 0x5d8788

// ---- the terrain object -----------------------------------------------------------------------------
struct terrain_gen {
    void reset();                                       // 0x4647e0
};
extern terrain_gen g_terrain;                           // 0xa76630

extern int g_region_names[];                            // 0x59394c (8 entries, ending at 0x59396c)

// ---- the widget interface (vtable 0x597050) ---------------------------------------------------------
struct WRet { int a, b; WRet(); };                       // 8-byte object returned through a hidden pointer

struct MissionUi {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_rect(const float* v);              // +0x58 (22)
    virtual void v23(); virtual void v24();
    virtual void set_scale(const float* v);             // +0x64 (25)
    virtual void v26();
    virtual void set_text_size(const float* v);         // +0x6c (27)
    virtual void v28(); virtual void v29(); virtual void v30();
    virtual void add_image(int layer, mesh* m);         // +0x7c (31)
    virtual void set_mesh(int layer, mesh* m);          // +0x80 (32)
    virtual void v33(); virtual void v34(); virtual void v35(); virtual void v36();
    virtual void v37(); virtual void v38(); virtual void v39(); virtual void v40();
    virtual WRet set_text(const tstring& s);            // +0xa4 (41)
    virtual void v42();
    virtual void set_text2(const tstring& s, const float* pos);     // +0xac (43)
    virtual void v44(); virtual void v45(); virtual void v46(); virtual void v47();
    virtual void v48(); virtual void v49();
    virtual void add_child(void* p);                    // +0xc8 (50)
    char tail[0x178 - 4];                            // sizeof(MissionUi) == 0x178
    MissionUi();                                        // 0x4931a0 / 0x48ae20
};

// the 0x190-byte board window (vtable 0x592848)
struct UiWindow : MissionUi {
    char pad[0x188 - 0x178];
    char f188;                                          // +0x188
    char pad2[0x190 - 0x189];
    UiWindow() : f188(0) {}
};

// the 0x188-byte work panel (vtable 0x597050)
struct Panel : MissionUi {
    char pad[0x188 - 0x178];
    Panel() {}
};

// the intermediate widget layer that owns the +0x178 word and the +0x121 byte
struct MidWidget : MissionUi {
    int f178;                                           // +0x178
    MidWidget() : f178(0) { *((char*)this + 0x121) = 0; }
};

// the 0x188-byte button face (vtable 0x592208)
struct SeedFace : MidWidget {
    char pad180[0x180 - 0x17c];
    char b180;                                          // +0x180
    char b181;                                          // +0x181
    char pad182[0x184 - 0x182];
    int f184;                                           // +0x184
    SeedFace() : b180(0), b181(0), f184(0x1c) {}
};

// ---- widget factories -------------------------------------------------------------------------------
MissionUi* new_sized_button(MissionUi* parent, const tstring& text, float a, float b, float c,
                            const tstring& name, const tstring& down,
                            float k1, float k2, float k3);                  // 0x4dcc50
MissionUi* make_hint(MissionUi* parent, const tstring& text, float a, float b, float c, float d);
                                                                            // 0x4dada0
MissionUi* make_slider(MissionUi* parent, float x, float y, float k);       // 0x4dfd50
MissionUi* make_button(MissionUi* parent, const tstring& name, float x, float y);   // 0x4dfec0
void region_name(int i, char* out);                                         // 0x4f2610

// 0x4dce70, inlined at every call site: the standard medium button
inline MissionUi* new_medium_button(MissionUi* parent, const tstring* text, float a, float b, float c)
{
    tstring d("medium_button_down");
    tstring nm("medium_button");
    return new_sized_button(parent, *text, a, b, c, nm, d, 3.7f, 0.072f, 0.085f);
}

// 0x4e5570, inlined: the standard short button
inline MissionUi* new_short_button(MissionUi* parent, const tstring* text, float a, float b, float c)
{
    tstring d("short_button_down");
    tstring nm("short_button");
    return new_sized_button(parent, *text, a, b, c, nm, d, 3.0f, 0.072f, 0.085f);
}

// the caption-at helper (inlined at every use)
inline void label_at(MissionUi* w, const char* s, float x, float y)
{
    tstring t(s);
    float p[2];
    p[0] = x;
    p[1] = y;
    w->set_text2(t, p);
}

// register a window with the gui (inlined)
inline void add_root(MissionUi* w)
{
    g_gui_dirty = 1;
    g_gui.add_root(&w);
}

// load and clone a named mesh (inlined at every use)
inline mesh* load_mesh(const char* name)
{
    tstring n(name);
    return g_alloc.get_object(n)->clone();
}

// ---- the world object (0x344 bytes, ctor 0x478ad0) --------------------------------------------------
struct Entity;

struct World {
    char pad0[0x178];
    void* body;                                         // +0x178
    char pad17c[0x344 - 0x17c];
    World(const float* lo, const float* hi, const int* dimx, const int* dimy, const int* v);
    Entity* create_entity(int type);                    // 0x428440
    void set_visual(const void* a, const void* b, const void* c);   // 0x488cd0
};

// the 0x358-byte camera body (ctor 0x4ed630): widget + world link + the 0x120-byte view frustum
struct face_body {
    char pad0[0x178];
    World* world;                                       // +0x178
    char pad17c[0x1ac - 0x17c];
    char view[0x120];                                   // +0x1ac
    char pad2cc[0x308 - 0x2cc];
    unsigned flags;                                     // +0x308
    unsigned colour;                                    // +0x30c
    char pad310[0x358 - 0x310];
    face_body();                                        // 0x4ed630
};

// the 0x120-byte view frustum record (the 0x4f25a0 builder writes it)
struct view_params {
    char data[0xf8];
    rgl::vector4 f_f8;                                  // +0xf8
    char rest[0x120 - 0x108];
    void build(float fov, float aspect, float znear, float zfar);   // 0x4f25a0
};

// the particle entity: a widget-ish record with an int mesh list at +0x118
struct Entity {
    virtual void e0(); virtual void e1(); virtual void e2(); virtual void e3(); virtual void e4();
    char pad[0x118 - 0x14];
    std::vector<int> meshes;                            // +0x118
};

// ---- the face-generator window ----------------------------------------------------------------------
struct facegen_win {
    char pad0[0x2c];
    int f2c, f30, f34, f38;                             // +0x2c
    float f3c, f40, f44, f48, f4c;                      // +0x3c
    int f50;                                            // +0x50
    int seed[3];                                        // +0x54
    int f60, f64;                                       // +0x60
    MissionUi* board;                                   // +0x68
    face_body* body;                                    // +0x6c
    view_params view;                                   // +0x70
    rgl::vector4 f190;                                  // +0x190
    int f1a0;                                           // +0x1a0
    float f1a4, f1a8;                                   // +0x1a4
    World* world;                                       // +0x1ac
    MissionUi* panel;                                   // +0x1b0
    MissionUi* key;                                     // +0x1b4
    MissionUi* copy;                                    // +0x1b8
    MissionUi* paste;                                   // +0x1bc
    MissionUi* sliders[6];                              // +0x1c0
    char pad1d8[0x200 - 0x1d8];
    MissionUi* seed_panel[3];                           // +0x200
    MissionUi* seed_btn[3];                             // +0x20c
    MissionUi* chk_occl;                                // +0x218
    MissionUi* chk_grass;                               // +0x21c
    MissionUi* chk_river;                               // +0x220
    MissionUi* chk_deep;                                // +0x224
    MissionUi* dd_region;                               // +0x228
    MissionUi* dd_detail;                               // +0x22c
    MissionUi* generate;                                // +0x230
    MissionUi* close;                                   // +0x234

    void init();                                        // 0x4f1460
    void refresh_hex();                                 // 0x4f2960
    void update();                                      // 0x4f2730
};

// @ 0x4f1460
void facegen_win::init()
{
    int i;
    char name[0x40];
    char rname[0x30];

    // ---- three random seeds ------------------------------------------------------------------------
    {
        int* p = seed;
        for (i = 3; i != 0; i--)
            *p++ = rand() % 0x1b5781;
    }
    g_terrain.reset();

    f34 = 0;
    f38 = 0;
    f2c = 0;
    f30 = 0;
    f3c = 150.0f;
    f40 = 150.0f;
    f44 = 100.0f;
    f48 = 0.5f;
    f50 = 30;
    f4c = 0.8f;
    f60 = 3;
    f64 = 0;

    g_gui.reset();

    // ---- the board window --------------------------------------------------------------------------
    board = new UiWindow;
    {
        float v[2];
        v[0] = 0.0f;
        v[1] = 0.0f;
        board->set_rect(v);
    }
    {
        float v[2];
        v[0] = 1.0f;
        v[1] = 0.75f;
        board->set_scale(v);
    }
    add_root(board);

    // ---- the work panel ----------------------------------------------------------------------------
    panel = new Panel;
    {
        float v[2];
        v[0] = 0.5f;
        v[1] = 0.0f;
        panel->set_rect(v);
    }
    {
        float v[2];
        v[0] = 0.5f;
        v[1] = 0.75f;
        panel->set_scale(v);
    }
    {
        mesh* m = load_mesh("facegen_board");
        panel->add_image(0, m);
    }
    panel->set_text(tstring(""));
    {
        float v[2];
        v[0] = 0.02f;
        v[1] = 0.02f;
        panel->set_text_size(v);
    }
    ((std::vector<MissionUi*>*)((char*)board + 0x17c))->push_back(panel);

    // ---- four check boxes --------------------------------------------------------------------------
    label_at(panel, "Shade occluded:", 0.24f, 0.62f);
    chk_occl = new_medium_button(panel, &tstring("disabled"), 0.36f, 0.62f, 0.03f);
    *(int*)((char*)chk_occl + 0x184) = 1;

    label_at(panel, "Place river:", 0.01f, 0.62f);
    chk_river = new_medium_button(panel, &tstring("place river"), 0.1f, 0.62f, 0.03f);
    *(int*)((char*)chk_river + 0x184) = 1;

    label_at(panel, "Disable grass:", 0.24f, 0.58f);
    chk_grass = new_medium_button(panel, &tstring("3dgrass"), 0.36f, 0.58f, 0.03f);
    *(int*)((char*)chk_grass + 0x184) = 1;

    label_at(panel, "Deep water:", 0.01f, 0.58f);
    chk_deep = new_medium_button(panel, &tstring("deep water"), 0.1f, 0.58f, 0.03f);
    *(int*)((char*)chk_deep + 0x184) = 1;

    // ---- the three seed rows -----------------------------------------------------------------------
    for (i = 0; i < 3; i++) {
        float y = (float)i * 0.04f;
        const char* nm;
        if (i == 0)
            nm = "terrain seed";
        else if (i == 1)
            nm = "river seed";
        else
            nm = "flora seed";
        label_at(panel, nm, 0.03f, y + 0.07f);
        float by = y + 0.06f;
        seed_btn[i] = new_medium_button(panel, &tstring("randomize"), 0.38f, by, 0.03f);

        seed_panel[i] = new SeedFace;
        {
            float v[2];
            v[0] = 0.1f;
            v[1] = by;
            seed_panel[i]->set_rect(v);
        }
        {
            float v[2];
            v[0] = 0.225f;
            v[1] = 0.025f;
            seed_panel[i]->set_scale(v);
        }
        {
            mesh* up = load_mesh("party_button");
            mesh* down = load_mesh("party_button_down");
            up->scale(0.4166667f);
            down->scale(0.4166667f);
            seed_panel[i]->set_mesh(2, up);
            seed_panel[i]->set_mesh(1, down);
        }
        panel->add_child(seed_panel[i]);
    }

    // ---- six sliders -------------------------------------------------------------------------------
    for (i = 0; i < 6; i++) {
        float y = (float)i * 0.04f;
        switch (i) {
        case 0: strcpy(name, "Size x"); break;
        case 1: strcpy(name, "Size y"); break;
        case 2: strcpy(name, "Hill height"); break;
        case 3: strcpy(name, "Valley"); break;
        case 4: strcpy(name, "Ruggedness"); break;
        case 5: strcpy(name, "Vegetation"); break;
        default: strcpy(name, "unknown");
        }
        label_at(panel, name, 0.05f, y + 0.2f);
        sliders[i] = make_slider(panel, 0.23f, y + 0.19f, 1.0f);
    }

    // ---- generate / close --------------------------------------------------------------------------
    generate = new_medium_button(panel, &tstring("Generate Terrain"), 0.07f, 0.02f, 0.035f);
    close = new_medium_button(panel, &tstring("Close"), 0.33f, 0.01f, 0.028f);

    // ---- key hint, copy, paste ---------------------------------------------------------------------
    key = make_hint(panel, tstring("key"), 0.0f, 0.72f, 0.5f, 0.02f);
    {
        float v[2];
        v[0] = 0.02f;
        v[1] = 0.02f;
        key->set_text_size(v);
    }

    copy = new_short_button(panel, &tstring("Copy"), 0.1f, 0.685f, 0.025f);
    paste = new_short_button(panel, &tstring("Paste"), 0.1f, 0.685f, 0.25f);

    label_at(panel, "Terrain:", 0.03f, 0.49f);
    label_at(panel, "Polygon Size:", 0.03f, 0.54f);

    // ---- the two drop downs ------------------------------------------------------------------------
    dd_region = make_button(panel, tstring("region type"), 0.23f, 0.49f);
    {
        int* p = g_region_names;
        while (p < g_region_names + 8) {
            region_name(*p, rname);
            dd_region->add_child((void*)&tstring(rname));
            p++;
        }
    }
    dd_detail = make_button(panel, tstring("region detail"), 0.23f, 0.54f);
    dd_detail->add_child((void*)&tstring("2 meters"));
    dd_detail->add_child((void*)&tstring("3 meters"));
    dd_detail->add_child((void*)&tstring("4 meters"));
    dd_detail->add_child((void*)&tstring("5 meters"));

    // ---- the camera body and its world -------------------------------------------------------------
    body = new face_body;
    {
        float lo[2];
        lo[0] = -200.0f;
        lo[1] = -200.0f;
        float hi[2];
        hi[0] = 200.0f;
        hi[1] = 200.0f;
        int dimx = 6;
        int dimy = 6;
        int v = 0;
        world = new World(lo, hi, &dimx, &dimy, &v);
    }
    {
        float near_ = 500.0f;
        float far_ = 1000.0f;
        int colour = (int)0xff405060;
        world->set_visual(&near_, &far_, &colour);
    }
    ((face_body*)body)->world = world;
    {
        float v[2];
        v[0] = 0.0f;
        v[1] = 0.0f;
        ((MissionUi*)body)->set_rect(v);
    }
    {
        float v[2];
        v[0] = 0.5f;
        v[1] = 0.75f;
        ((MissionUi*)body)->set_scale(v);
    }
    ((face_body*)body)->flags |= 0x100;
    ((face_body*)body)->colour = 0xff111133;

    view.build(0.6981317f, 1.5f, 2.0f, 1000.0f);
    *(view_params*)((char*)body + 0x1ac) = view;

    ((std::vector<MissionUi*>*)((char*)board + 0x17c))->push_back((MissionUi*)body);

    {
        Entity* e = world->create_entity(9);
        mesh* m = load_mesh("prtcl_drop");
        e->meshes.push_back((int)m);
        e->e4();
    }

    view.f_f8 = rgl::vector4(0.0f, 0.0f, 10.0f);
    f190 = rgl::vector4(0.0f, 0.0f, 0.0f);
    f1a0 = 0;
    f1a4 = 1.0f;
    f1a8 = 250.0f;

    refresh_hex();
    update();
}
