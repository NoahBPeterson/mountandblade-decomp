#include <windows.h>
#include <math.h>

// ---- engine state ------------------------------------------------------------------------------------
struct Mission {                                // 0x5e292c
    char pad0[0x1c];
    void** pages;                               // +0x1c (0x5928-byte entities, 16 per page)
    char pad20[0x5c - 0x20];
    int selected;                               // +0x5c
};
struct Entity;
extern char g_mission[]; // 0x5e292c
#define M_PAGES (*(void***)(g_mission + 0x1c))
#define M_SELECTED (*(int*)(g_mission + 0x5c))

extern char* g_game; // 0x5e2930
#define F_13A00 (*(void**)(g_game + 0x13a00))       // the entity the status block follows
#define F_1A090 (*(int*)(g_game + 0x1a090))         // reset to 0
#define F_1A1B4 (*(int*)(g_game + 0x1a1b4))         // reset to -1
#define F_1A1CC (*(int*)(g_game + 0x1a1cc))         // reset to 0

extern char g_5e212c;                           // 0x5e212c (set to 1)
extern char g_5e2938;                           // 0x5e2938 (set to 1 with 5% probability)
extern int g_5a9f18;                            // 0x5a9f18 (set to -1)
extern int g_5aa034;                            // 0x5aa034
extern char g_5d8788;                           // 0x5d8788 (set to 1)
extern int g_skin_cap; // 0x8ee3ac4
extern __int64 g_clock[];                       // 0x5e2948 (game clock: two dwords per entry)

// ---- helpers -----------------------------------------------------------------------------------------
void update_slot_visibility(void* agent);       // 0x4f7d70 (Agent::init)
void push_ui_element(void* where, int n, void** item);   // 0x4927d0 (vector insert, count 1)
void* __cdecl operator_new(unsigned size);      // 0x53545e (??2@YAPAXI@Z)
void __fastcall create_agent_visual(char* mission_state);       // 0x42d1b0, this = 0x5d7080
void __fastcall reset_ui_list(char* vec, int n);                // 0x43f310, this = g_game + 0x1a1b8

struct tstring {                                // 0x10 bytes, ctor 0x42bbe0, dtor 0x42bc40
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
};

struct Mesh;                                    // rgl::mesh, ctor 0x412a90
struct Material {
    char pad0[0x2c];
    void* texture;                              // +0x2c
};
void* __fastcall resmgr_get_object(tstring* name);              // 0x419540, this = 0x5d6fa0
void* __fastcall resource_manager_get_material(tstring* name);  // 0x418a70
void* __fastcall mesh_from_mesh(void* mesh, void* out);         // 0x41c6d0
Mesh* __fastcall new_mesh();                                    // 0x412a90 (new + mesh::mesh)

// ---- the mission-UI widget vtable (offsets confirmed against s0048add0) ------------------------------
struct MissionUi {
    int vptr;                                   // 0x000
    char pad4[0xa0 - 0x4];
    int f_a0;                                   // 0x0a0 (the widget's first mesh)
    char padA4[0x118 - 0xa4];
    void* f_118;                                // 0x118 (-> Material; its +0x2c is the texture)
    char pad11c[0x170 - 0x11c];

    MissionUi();                                // 0x48ae20
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();   // 0x00..0x10
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();   // 0x14..0x24
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();   // 0x28..0x38
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18();                       // 0x3c..0x48
    virtual void v19(); virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23();   // 0x4c..0x5c
    virtual void set_size(float* s);            // +0x58
    virtual void v25();                         // +0x5c
    virtual void v26();                         // +0x60
    virtual void set_pos(float* p);             // +0x64
    virtual void v28(); virtual void v29(); virtual void v30(); virtual void v31(); virtual void v32();   // 0x68..0x78
    virtual void v33();                         // +0x7c
    virtual void set_mesh(void* m, int flag);   // +0x80
    virtual void v35(); virtual void v36(); virtual void v37(); virtual void v38();                       // 0x84..0x90
    virtual void get_size(float* out);          // +0x94
    virtual void v40();                         // +0x98
    virtual void set_color(int* c);             // +0x9c
    virtual void v42();                         // +0xa0
    virtual void set_material_name(tstring* name, tstring* out);  // +0xa4
    virtual void v44(); virtual void v45(); virtual void v46(); virtual void v47(); virtual void v48();   // 0xa8..0xb8
    virtual void v49();                         // +0xbc
    virtual void v50(); virtual void v51();     // 0xc0..0xc4
    virtual void add_child(MissionUi* c);       // +0xc8
};

struct FrameWidget : MissionUi {                // ctor 0x4931a0, vptr 0x592848
    int f_178;                                  // +0x178 (child vector: begin)
    int f_17c;                                  // +0x17c (end)
    int f_180;                                  // +0x180 (capacity end)
    char pad184[0x188 - 0x184];
    char f_188;                                 // +0x188 (zeroed)
    FrameWidget();                              // 0x4931a0
};

struct NameWidget : MissionUi {                 // ctor 0x4ed630, vptr 0x598698
    int f_178;                                  // +0x178 (back-pointer to the mission object)
    char pad17c[0x1a8 - 0x17c];
    char frustum[0x120];                        // +0x1a8 (the copied frustum)
    char pad2c8[0x34c - 0x2c8];
    char f_350;                                 // +0x350 (set when 0x5aa034 != 0)
    char pad351[0x354 - 0x351];
    NameWidget();                               // 0x4ed630
};

struct Frustum {                                // 0x120 bytes; 0x41e4f0 builds its six planes
    char f_00;                                  // +0x00 (enabled)
    char pad01[0x104 - 0x01];
    float f_104;
    float f_108;
    float f_10c;
    float f_110;
    float f_114;
    float f_118;
    float f_11c;
    char pad120[0x48 * 4 - 0x120];
    void build_planes();                        // 0x41e4f0
};

// A 0x5928-byte entity page slot (only the fields this function touches).
struct Entity {
    char pad0[0x2c];
    int f_2c;                                   // +0x2c
    char pad30[0x58 - 0x30];
    int f_58;                                   // +0x58
    char pad5c[0x7a - 0x5c];
    unsigned char f_7a;                         // +0x7a (low nibble: an index)
    char pad7b[0x571c - 0x7b];
    float f_571c;                               // +0x571c
    float f_5720;                               // +0x5720
    float f_5724;                               // +0x5724
};

struct MissionObj {
    char pad0[0x2c];
    FrameWidget* frame;                         // 0x02c
    char pad30[0x60 - 0x30];
    MissionUi* name_widget;                     // 0x060 (a NameWidget)
    char pad64[0x68 - 0x64];
    MissionUi* arrows[3];                       // 0x068..0x073 (three crosshairs)
    MissionUi* crosses[4];                      // 0x074..0x083 (four arrows)
    char pad84[0xbc - 0x84];
    MissionUi* f_bc;                            // 0x0bc (ui_font text)
    MissionUi* f_c0;                            // 0x0c0 (ui_font_background text)
    MissionUi* f_c4;                            // 0x0c4 (status background group)
    MissionUi* f_c8;                            // 0x0c8
    MissionUi* f_cc;                            // 0x0cc (status bar)
    MissionUi* f_d0;                            // 0x0d0 (health bar)
    MissionUi* f_d4;                            // 0x0d4
    Mesh* f_d8;                                 // 0x0d8 (status bar mesh)
    Mesh* f_dc;                                 // 0x0dc (health bar mesh)
    int f_e0;                                   // 0x0e0
    int f_e4;                                   // 0x0e4
    int f_e8;                                   // 0x0e8
    int f_ec;                                   // 0x0ec
    int f_f0;                                   // 0x0f0
    int f_f4;                                   // 0x0f4
    char padF8[0x268 - 0xf8];
    float f_268;                                // 0x268 (view yaw)
    char pad26c[0x270 - 0x26c];
    int f_270;                                  // 0x270
    int f_274;                                  // 0x274
    char pad278[0x27c - 0x278];
    float f_27c;                                // 0x27c
    float f_280;                                // 0x280
    char pad284[0x2a8 - 0x284];
    int f_2a8;                                  // 0x2a8
    char pad2ac[0x2b0 - 0x2ac];
    // (__int64 members are 8-aligned in VC7.1, which would push the clock origins to +0x2b8/+0x2c8;
    //  in the original they sit at +0x2b0 and +0x2c0, so they are stored as dword pairs.)
    int f_2b0;                                  // 0x2b0
    int f_2b4;                                  // 0x2b4
    int f_2b8;                                  // 0x2b8
    char pad2bc[0x2c0 - 0x2bc];
    int f_2c0;                                  // 0x2c0
    int f_2c4;                                  // 0x2c4
    int f_2c8;                                  // 0x2c8
    char f_2cc;                                 // 0x2cc
    char pad2cd[0x2d4 - 0x2cd];
    int f_2d4;                                  // 0x2d4
    int f_2d8;                                  // 0x2d8
    int f_2dc;                                  // 0x2dc
    char pad2e0[0x2e4 - 0x2e0];
    int f_2e4;                                  // 0x2e4
    int f_2e8;                                  // 0x2e8
    char pad2ec[0x2f0 - 0x2ec];
    char f_2f0;                                 // 0x2f0
    char f_2f1;                                 // 0x2f1 (slots need a refresh)
    char f_2f2;                                 // 0x2f2 (status block scroll pending)
    char pad2f3;
    int f_2f4;                                  // 0x2f4 (status block height)
    char pad2f8[0x2fc - 0x2f8];
    int f_2fc;                                  // 0x2fc

    void refresh();                             // 0x4f7f00
};

void finish_pass(MissionObj* o);                // 0x4fb350
void update_health_widget(MissionObj* o, float dt);  // 0x4f8c80
void refresh_name_overlay(MissionObj* o);       // 0x4f8b40

// @ 0x4f7f00 ---------------------------------------------------------------------------------
void MissionObj::refresh()
{
    f_2fc = 0;
    // 0x3dc9 = 15785: the mask is (rand() % 15785) / 15785 < 5%
    if ((float)(rand() % 15785) * 6.322312e-05f < 0.05f)
        g_5e2938 = 1;

    create_agent_visual(&g_game[0]);
    Mission* m = (Mission*)g_mission;
    int id = m->selected;
    g_5e212c = 1;
    F_1A1B4 = -1;
    reset_ui_list(&g_game[0x1a1b8], 0);

    if (f_2f1) {
        if (id >= 0)
            update_slot_visibility((char*)m->pages[id >> 4] + (id & 0xf) * 0x5928);
        f_2f1 = 0;
    } else if (f_2f2) {
        Entity* a = (Entity*)F_13A00;
        int n = a->f_7a & 0xf;
        if (n > g_skin_cap)
            n = g_skin_cap;
        int h = a->f_58 + 0x23 + n * 2;
        if (f_2f4 < h)
            f_2f4 = h;
    }

    // the clock table holds { low, high } pairs (0x5e2948 + 8 * index); both origins are 10 s old
    f_2c0 = (int)g_clock[f_2b8] - 10000000;
    f_2c4 = (int)(g_clock[f_2b8] >> 32) - (f_2c0 < 0 ? 1 : 0);
    f_2b0 = (int)g_clock[f_2a8] - 10000000;
    f_2b4 = (int)(g_clock[f_2a8] >> 32) - (f_2b0 < 0 ? 1 : 0);
    f_2c8 = -1;
    f_2d4 = 0;
    f_2e4 = 0;
    f_2e8 = 0;
    f_2f0 = 0;
    f_2d8 = -1;
    f_2dc = -1;
    g_5a9f18 = -1;
    F_1A1CC = 0;
    f_2a8 = 0;
    f_2b8 = 0;
    f_e0 = -1;
    f_e4 = -2;
    f_f0 = -2;
    f_e8 = -1;
    f_f4 = -1;
    F_1A090 = 0;

    // ---- the status frame -------------------------------------------------------------------
    FrameWidget* fr = new FrameWidget;
    frame = fr;
    float sz[] = { 0.0f, 0.0f };
    float fp[] = { 1.0f, 0.375f };
    fr->set_size(sz);
    fr->set_pos(fp);
    {
        float cp[2] = { 0.03f, 0.04f };
        float s0[2] = { 0.5f, 0.45f };
        float s1[2] = { 0.65f, 0.35f };
        float s2[2] = { 0.5f, 0.3f };
        g_5d8788 = 1;
        for (int i = 0; i < 3; i++) {
            MissionUi* w = new MissionUi;
            arrows[i] = w;
            {
                float s[2] = { 0.5f, 0.375f };
                w->set_size(s);
                w->set_pos(cp);
            }
            void* mesh;
            if (i == 0) {
                tstring n("crosshair_left");
                mesh = resmgr_get_object(&n);
            } else if (i == 1) {
                tstring n("crosshair_left");
                mesh = resmgr_get_object(&n);
            } else {
                tstring n("crosshair_right");
                mesh = resmgr_get_object(&n);
            }
            w->f_a0 = (int)mesh_from_mesh(mesh, 0);
            w->set_mesh((void*)w->f_a0, 0);
            fr->add_child(w);
        }
        arrows[0]->set_size(s0);        arrows[1]->set_size(s1);
        arrows[2]->set_size(s2);
    }

    {
        float cp[2] = { 0.03f, 0.04f };
        float s3[2] = { 0.35f, 0.35f };
        for (int i = 0; i < 4; i++) {
            MissionUi* w = new MissionUi;
            crosses[i] = w;
            {
                float s[2] = { 0.35f, 0.35f };
                w->set_size(s);
                w->set_pos(cp);
            }
            void* mesh;
            if (i == 0) {
                tstring n("arrow_up");
                mesh = resmgr_get_object(&n);
            } else if (i == 1) {
                tstring n("arrow_right");
                mesh = resmgr_get_object(&n);
            } else if (i == 2) {
                tstring n("arrow_down");
                mesh = resmgr_get_object(&n);
            } else {
                tstring n("arrow_left");
                mesh = resmgr_get_object(&n);
            }
            w->f_a0 = (int)mesh_from_mesh(mesh, 0);
            w->set_mesh((void*)w->f_a0, 0);
            fr->add_child(w);
        }
        (void)s3;
    }

    f_c8 = NULL;
    fr->add_child(NULL);

    // ---- the two text widgets ---------------------------------------------------------------
    {
        float s[2] = { 0.308f, 0.0508f };
        MissionUi* font = new MissionUi;
        f_bc = font;
        font->set_size(s);
        font->v33();
        tstring n("");
        tstring out;
        font->set_material_name(&n, &out);
        tstring nm("ui_font");
        ((Material*)font->f_118)->texture = resource_manager_get_material(&nm);
        int col = -1;
        font->set_color(&col);
    }
    {
        float s[2] = { 0.308f, 0.0508f };
        MissionUi* fontbg = new MissionUi;
        f_c0 = fontbg;
        fontbg->set_size(s);
        fontbg->v33();
        tstring n("");
        tstring out;
        fontbg->set_material_name(&n, &out);
        tstring nm("ui_font_background");
        ((Material*)fontbg->f_118)->texture = resource_manager_get_material(&nm);
        int col = 0xff000000;
        fontbg->set_color(&col);
    }

    // ---- the status background group ---------------------------------------------------------
    {
        float s[2] = { 0.877f, 0.024f };
        MissionUi* grp = new MissionUi;
        f_c4 = grp;
        grp->set_size(s);
        tstring n("status_background");
        void* mesh = resmgr_get_object(&n);
        grp->set_mesh(mesh_from_mesh(mesh, 0), 0);
        grp->add_child(f_cc);
        grp->add_child(f_d0);
        grp->add_child(f_c8);
        grp->add_child(f_bc);
        grp->add_child(f_c0);
        push_ui_element(&fr->f_17c, 1, (void**)&grp);
    }

    {
        float s[2] = { 0.0f, 0.0f };
        MissionUi* hp = new MissionUi;
        f_d0 = hp;
        hp->set_size(s);
        float p[2] = { 0.1065f, 0.005f };
        hp->set_pos(p);
        tstring n("status_health_bar");
        void* mesh = resmgr_get_object(&n);
        f_dc = (Mesh*)mesh_from_mesh(mesh, 0);
        hp->set_mesh(f_dc, 0);
    }

    {
        float s[2] = { 0.0724f, 0.0508f };
        MissionUi* st = new MissionUi;
        f_cc = st;
        st->set_size(s);
        float p[2] = { 0.1065f, 0.04052f };
        st->set_pos(p);
        f_d8 = new_mesh();
        st->set_mesh(f_d8, 0);
        f_d4 = (MissionUi*)new_mesh();
        st->set_mesh(f_d4, 2);
    }

    // ---- the agent name overlay -------------------------------------------------------------
    {
        NameWidget* nw = new NameWidget;
        name_widget = nw;
        float t = (float)tan(0.87266463);
        float s[2] = { 0.0f, 0.0f };
        nw->set_size(s);
        float p[2] = { 1.0f, 0.75f };
        nw->set_pos(p);
        if (g_5aa034)
            nw->f_350 = 1;
        (void)frame;
        Frustum f;
        f.f_00 = 1;
        f.f_118 = 0.1f;
        f.f_11c = 1536.0f;
        f.f_104 = -0.75f;
        f.f_108 = -0.1f * t;
        f.f_10c = t * 0.1f;
        f.f_110 = t * 0.1f * 0.75f;
        f.f_114 = -0.1f * t * 0.75f;
        f.build_planes();
        *(Frustum*)nw->frustum = f;
        push_ui_element(&fr->f_17c, 1, (void**)&nw);
    }

    finish_pass(this);

    // ---- per-frame camera state -------------------------------------------------------------
    {
        Entity* e = id < 0 ? NULL
                           : (Entity*)((char*)m->pages[id >> 4] + (id & 0xf) * 0x5928);
        if (e == NULL || e->f_2c < 0) {
            f_27c = 0.0f;
            f_280 = 1.65f;
        } else {
            f_27c = 0.1f;
            f_280 = 2.4f;
        }
    }
    f_270 = 0;
    f_274 = 0;
    {
        if (id < 0) {
            f_268 = 0.0f;
        } else {
            Entity* e = (Entity*)((char*)m->pages[id >> 4] + (id & 0xf) * 0x5928);
            float* dir = (float*)((char*)e + 0x5c);
            f_268 = (float)atan2(-dir[1], dir[2]);
        }
    }
    update_health_widget(this, 0.001f);
    refresh_name_overlay(this);
    (void)sz;
}
