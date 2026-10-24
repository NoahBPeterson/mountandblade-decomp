// s0050f440 - loading screen: module table load, frame/quads meshes, record reader.
// All six functions belong to the module-loading state machine object.
#include <stdio.h>

#include "../tstring.h"

// source record stream: the readers call three virtual slots
struct Src {
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03();
    virtual void s04(); virtual void s05(); virtual void s06(); virtual void s07();
    virtual void s08(); virtual void s09(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18();
    virtual void out_int(int* p);       // +0x4c
    virtual void s20();                 // +0x50
    virtual void out_vec(float* p);     // +0x54
};

// one 0x20 byte record of the module table
struct Rec {
    int f0;                             // +0
    int f4;                             // +4
    int f8;                             // +8
    int fc;                             // +0xc
    float f10;                          // +0x10
    float f14;                          // +0x14
    float f18;                          // +0x18
    float f1c;                          // +0x1c
    __declspec(noinline) void read(Src* src);   // 0x50f840
};

struct MusicTable {
    char pad0[4];
    int count;                          // +4
    Rec rec[0x100];                     // +8
    int idx[0x100];                     // +0x2008
    void build(Src* src);               // 0x50f7e0
};

struct vector4 { float x, y, z, w; };

struct Mesh {
    void build_panel(float* base, float* size, float* uv0, float* uv1, int* col);  // 0x4de3e0
    void compute_normals();             // 0x451c50
    void finish_load();                 // 0x419300
    void set_color(vector4 c);          // 0x47af30
    Mesh* clone();                      // 0x41c6d0
};

struct ResMgr2 {
    Mesh* get_object(const tstring& name);  // 0x419540
};
extern ResMgr2 g_res;                   // 0x5d6fa0

// ---------------------------------------------------------------- 0x50f7e0
// @ 0x50f7e0
void MusicTable::build(Src* src)
{
    src->out_int(&count);
    if (count > 0x100)
        count = 0x100;
    for (int i = 0; i < count; i++) {
        rec[i].read(src);
        idx[i] = 0;
    }
}

// ---------------------------------------------------------------- 0x50f840
// @ 0x50f840
void Rec::read(Src* src)
{
    src->out_vec(&f10);
    src->out_int(&f0);
    src->out_int(&f4);
    f4 = f0 | f4;
    f8 = f0 & 0x3f;
    if (!f8)
        f8 = 0x3f;
    fc = (f4 & 0x3f) | f8;
}

// ---------------------------------------------------------------- 0x50f8a0
struct Frame570 {
    char pad0[0x570];
    void build_quad();
};
// @ 0x50f8a0
void Frame570::build_quad()
{
    Mesh* m = (Mesh*)((char*)this + 0x570);
    int color = 0xff000000;
    float uv1[2] = { 0.1f, 1.0f };
    float uv0[2] = { 0.0f, 0.0f };
    float z[6];
    z[0] = 0.1f;
    z[1] = 1.0f;
    z[2] = 0.0f;
    z[3] = 0.0f;
    z[4] = 0.0f;

    m->build_panel(&z[2], z, uv0, uv1, &color);
    m->compute_normals();
}

// ---------------------------------------------------------------- 0x50f930
struct LoadingMeshes {
    char pad0[0xfb0];
    Mesh* f1210;                        // +0x1210
    Mesh* f1214;                        // +0x1214
    Mesh* f1218;                        // +0x1218
    Mesh* f121c;                        // +0x121c
    void build_frames();
};

static void set_frame_color(Mesh* m)
{
    vector4 c;
    c.x = 0.65f;
    c.y = 0.65f;
    c.z = 1.0f;
    m->set_color(c);
    m->compute_normals();
    m->finish_load();
}

// @ 0x50f930
void LoadingMeshes::build_frames()
{
    {
        Mesh* m = (Mesh*)((char*)this + 0xfb0);
        int color = 0xa0ffffff;
        float uv1[2] = { 0.1f, 1.0f };
        float uv0[2] = { 0.0f, 0.0f };
        float z[4];
        float base[3] = { 0.0f, 0.0f, 0.0f };

        z[0] = 0.1f;
        z[1] = 1.0f;
        m->build_panel(base, z, uv0, uv1, &color);
        m->compute_normals();
    }

    {
        tstring n("frame_corner_a");
        f1210 = g_res.get_object(n)->clone();
    }
    {
        vector4 c;
        c.x = 0.65f; c.y = 0.65f; c.z = 1.0f;
        f1210->set_color(c);
    }
    f1210->compute_normals();
    f1210->finish_load();

    {
        tstring n("frame_corner_b");
        f1214 = g_res.get_object(n)->clone();
    }
    {
        vector4 c;
        c.x = 0.65f; c.y = 0.65f; c.z = 1.0f;
        f1214->set_color(c);
    }
    f1214->compute_normals();
    f1214->finish_load();

    {
        tstring n("frame_corner_c");
        f1218 = g_res.get_object(n)->clone();
    }
    {
        vector4 c;
        c.x = 0.65f; c.y = 0.65f; c.z = 1.0f;
        f1218->set_color(c);
    }
    f1218->compute_normals();
    f1218->finish_load();

    {
        tstring n("frame_corner_d");
        f121c = g_res.get_object(n)->clone();
    }
    {
        vector4 c;
        c.x = 0.65f; c.y = 0.65f; c.z = 1.0f;
        f121c->set_color(c);
    }
    f121c->compute_normals();
    f121c->finish_load();
}

// ---------------------------------------------------------------- 0x50fbf0
struct TrackEntry {
    char b[0x20];
};

void uninit_fill(void* dst, unsigned n, const void* val, void* self, const void* val2);  // 0x4f65f0

struct Entries {
    void* alloc;                        // +0
    TrackEntry* first;                       // +4
    TrackEntry* last;                        // +8
    TrackEntry* end;                         // +0xc
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
    unsigned capacity() const { return first == 0 ? 0 : (unsigned)(end - first); }
    void insert_n(TrackEntry* where, unsigned n, const TrackEntry& val);   // 0x50d260
    void add(const TrackEntry& val);         // 0x50fbf0
};

// @ 0x50fbf0
void Entries::add(const TrackEntry& val)
{
    if (size() < capacity()) {
        uninit_fill(last, 1, &val, this, &val);
        last++;
    } else {
        insert_n(last, 1, val);
    }
}

// ---------------------------------------------------------------- 0x50f440 loading-screen state machine
// Called once per frame by the loading screen's update (0x50f390) as
//   advance(&this->step, &this->total, &text);
// It translates the current phase name into `text`, runs the phase's work and bumps the step
// (the caller is done when it returns true).  `total` is unused here.

#include "../str_table.h"
extern StrTable g_strtable;                          // 0x5e2a18
extern tstring  g_module_path;                       // 0x8ee3c28 (the module directory)

void mb_error(const char* fmt, ...);                 // 0x42cc80
void parse_module_info();                            // 0x4c05b0
void fn_4d0c20();                                    // 0x4d0c20
void fn_50d5e0();                                    // 0x50d5e0

// message logger (0x5a9f60): the text sink is the virtual `operator<<(const char*)` at +0x28.
struct logger {
    virtual void v00();                              // +0x00
    virtual void v04();                              // +0x04
    virtual void v08();                              // +0x08
    virtual void v0c();                              // +0x0c
    virtual void v10();                              // +0x10
    virtual void v14();                              // +0x14
    virtual void v18();                              // +0x18
    virtual void v1c();                              // +0x1c
    virtual void v20();                              // +0x20
    virtual void v24();                              // +0x24
    // cl 13.10 lays an overload set out in *reverse* declaration order, occupying the slots of the
    // first-declared member: so the string sink lands back on slot 10 (+0x28) as in the original.
    virtual logger& operator<<(float v);             // +0x30
    virtual logger& operator<<(int v);               // +0x2c
    virtual logger& operator<<(const char* s);       // +0x28
};
extern logger g_logger;                              // 0x5a9f60

// the text stream used to read music.txt (vtable 0x54ac88, 0x38 bytes, `failed` at +0x34).
struct text_stream {
    char data[0x34];
    char failed;                                     // +0x34
    text_stream();                                   // 0x4172d0
    ~text_stream();                                  // 0x417310
    bool open_read_text(const char* path);           // 0x415d60
};

struct Table2;                                       // built by MusicTable::build below (0x50f7e0)
extern MusicTable g_music_table;                          // 0xa73a10

struct ModInfo {
    void load(FILE* f);                              // 0x4bd630
};
extern ModInfo g_mod_info;                           // 0xa18238

struct EditorDialog {
    void restart();                                  // 0x4c0520
};
extern EditorDialog g_editor_dialog;                 // 0x8ee3c48

struct GuiMgr {
    char pad0[0x570];
    void build_frames();                             // 0x50f930 (this+0x570)
    void build_quad();                               // 0x50f8a0
};
extern GuiMgr g_gui;                                 // 0x5d7080

struct ResMgr3 {
    int load_all(int check_exists, tstring* root);   // 0x41c540
};
extern ResMgr3 g_resmgr2;                            // 0x5d6fa0

extern int   g_sound_paused;                         // 0x5a9fd4
extern int   g_sound_muted;                          // 0x5a9fd8
extern int   g_scan_module_textures;                 // 0xa18108
extern char* g_xa44238;                              // 0xa44238

struct LoadingScreen {
    char pad0[0x3c];
    int  step;                                       // +0x3c
    int  total;                                      // +0x40
    char* f44;                                       // +0x44
    int  f48;                                        // +0x48
    bool advance(int* step, int* total, tstring* text);   // 0x50f440
};

// @ 0x50f440
bool LoadingScreen::advance(int* step, int* total, tstring* text)
{
    bool finished = false;

    text->assign("");
    if (*step == 0) {
        f48 = 0;
        { tstring key("ui_loading_module_info_file"); text->assign(g_strtable.lookup(key)->c_str()); }
        (*step)++;
    } else if (*step == 1) {
        g_editor_dialog.restart();
        tstring path(g_module_path);
        { tstring ini("module.ini"); path.append(ini); }
        FILE* f = fopen(path.c_str(), "r");
        if (f) {
            g_mod_info.load(f);
            f44 = g_xa44238 + 5;
            fclose(f);
        } else {
            mb_error("Unable to find module info file");
        }
        { tstring key("ui_processing_ini_file"); text->assign(g_strtable.lookup(key)->c_str()); }
        (*step)++;
    } else if (*step == 2) {
        parse_module_info();
        g_gui.build_frames();
        g_gui.build_quad();
        { tstring key("ui_loading_music"); text->assign(g_strtable.lookup(key)->c_str()); }
        (*step)++;
    } else if (*step == 3) {
        if (!g_sound_paused && !g_sound_muted) {
            g_logger << "Loading Music...\n";
            text_stream ts;
            {
                tstring name(g_module_path);
                { tstring m("music.txt"); name.append(m); }
                if (ts.open_read_text(name.c_str())) {
                    g_music_table.build((Src*)&ts);
                    ts.failed = 1;
                } else {
                    mb_error("Unable to find module music file");
                }
            }
        }
        { tstring key("ui_loading_data"); text->assign(g_strtable.lookup(key)->c_str()); }
        *step += 2;
    } else if (*step == 4) {
        *step = 5;
    } else if (*step == 5) {
        fn_4d0c20();
        { tstring key("ui_loading_setting_data"); text->assign(g_strtable.lookup(key)->c_str()); }
        (*step)++;
    } else if (*step == 6) {
        fn_50d5e0();
        { tstring key("ui_loading_textures"); text->assign(g_strtable.lookup(key)->c_str()); }
        (*step)++;
    } else if (*step == 7) {
        g_logger << "Loading Textures...\n";
        g_resmgr2.load_all(g_scan_module_textures, &g_module_path);
        g_logger << "Finished Loading Textures...\n";
        { tstring key("ui_finished"); text->assign(g_strtable.lookup(key)->c_str()); }
        (*step)++;
    } else {
        finished = true;
    }
    return finished;
}
