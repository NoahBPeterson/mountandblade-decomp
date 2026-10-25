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

#include "rgl.h"
#include "../rgl_mesh.h"                  // rgl::mesh (build_panel 0x4de3e0, compute_normals, finish_load, set_color, clone)
using rgl::vector4;
using rgl::mesh;

struct resmgr {                                       // rgl/core/resmgr.h (the object at 0x5d6fa0)
    mesh* get_object(const rgl::string* name);      // 0x419540
    int load_all(int check_exists, rgl::string* root);   // 0x41c540
};
extern resmgr g_res;                    // 0x5d6fa0
// rgl::mesh::build_panel is declared with float* arguments by rgl_mesh.h; the function at 0x4de3e0 is PanelMesh::build_panel
struct PanelMesh {
    void build_panel(const float* a, const float* b, const float* c, const float* d, int* color);   // 0x4de3e0 (game/cls_map)
};

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
    mesh* m = (mesh*)((char*)this + 0x570);
    int color = 0xff000000;
    float uv1[2] = { 0.1f, 1.0f };
    float uv0[2] = { 0.0f, 0.0f };
    float z[6];
    z[0] = 0.1f;
    z[1] = 1.0f;
    z[2] = 0.0f;
    z[3] = 0.0f;
    z[4] = 0.0f;

    ((PanelMesh*)m)->build_panel(&z[2], z, uv0, uv1, &color);
    m->compute_normals();
}

// ---------------------------------------------------------------- 0x50f930
struct LoadingMeshes {
    char pad0[0x1210];
    mesh* f1210;                        // +0x1210
    mesh* f1214;                        // +0x1214
    mesh* f1218;                        // +0x1218
    mesh* f121c;                        // +0x121c
    void build_frames();
};

static void set_frame_color(mesh* m)
{
    vector4 c;
    c.x = 0.65f;
    c.y = 0.65f;
    c.z = 1.0f;
    m->set_color(c);
    m->compute_normals();
    m->compute_bounds();
}

// @ 0x50f930
void LoadingMeshes::build_frames()
{
    {
        mesh* m = (mesh*)((char*)this + 0xfb0);
        int color = 0xa0ffffff;
        float uv1[2] = { 0.1f, 1.0f };
        float uv0[2] = { 0.0f, 0.0f };
        float z[4];
        float base[3] = { 0.0f, 0.0f, 0.0f };

        z[0] = 0.1f;
        z[1] = 1.0f;
        ((PanelMesh*)m)->build_panel(base, z, uv0, uv1, &color);
        m->compute_normals();
    }

    {
        tstring n("frame_corner_a");
        f1210 = g_res.get_object((const rgl::string*)&n)->clone();
    }
    {
        vector4 c;
        c.x = 0.65f; c.y = 0.65f; c.z = 1.0f;
        f1210->set_color(c);
    }
    f1210->compute_normals();
    f1210->compute_bounds();

    {
        tstring n("frame_corner_b");
        f1214 = g_res.get_object((const rgl::string*)&n)->clone();
    }
    {
        vector4 c;
        c.x = 0.65f; c.y = 0.65f; c.z = 1.0f;
        f1214->set_color(c);
    }
    f1214->compute_normals();
    f1214->compute_bounds();

    {
        tstring n("frame_corner_c");
        f1218 = g_res.get_object((const rgl::string*)&n)->clone();
    }
    {
        vector4 c;
        c.x = 0.65f; c.y = 0.65f; c.z = 1.0f;
        f1218->set_color(c);
    }
    f1218->compute_normals();
    f1218->compute_bounds();

    {
        tstring n("frame_corner_d");
        f121c = g_res.get_object((const rgl::string*)&n)->clone();
    }
    {
        vector4 c;
        c.x = 0.65f; c.y = 0.65f; c.z = 1.0f;
        f121c->set_color(c);
    }
    f121c->compute_normals();
    f121c->compute_bounds();
}

// ---------------------------------------------------------------- 0x50fbf0
struct TrackEntry {
    char b[0x20];
};

// 0x4f65f0 (game/editor_ui.cpp) is cdecl and its definition has no trailing parameter; the original pushes the value's address once more
struct NameVecRec; struct RecAlloc;
void __cdecl uninit_fill(NameVecRec* dst, unsigned n, const NameVecRec& val, RecAlloc* al);   // 0x4f65f0
typedef void (__cdecl *uninit_fill_t)(NameVecRec*, unsigned, const NameVecRec&, RecAlloc*, const void*);
struct named_vec;
struct vec_named {
    void _Insert_n(named_vec* where, unsigned n, const named_vec& val);   // 0x50d260 (inventory_panel/widget_text.cpp): Entries is the same 0x20-byte-element vector
};

struct Entries {
    void* alloc;                        // +0
    TrackEntry* first;                       // +4
    TrackEntry* last;                        // +8
    TrackEntry* end;                         // +0xc
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
    unsigned capacity() const { return first == 0 ? 0 : (unsigned)(end - first); }
    void add(const TrackEntry& val);         // 0x50fbf0
};

// @ 0x50fbf0
void Entries::add(const TrackEntry& val)
{
    if (size() < capacity()) {
        ((uninit_fill_t)uninit_fill)((NameVecRec*)last, 1, *(const NameVecRec*)&val, (RecAlloc*)this, &val);
        last++;
    } else {
        ((vec_named*)this)->_Insert_n((named_vec*)last, 1, (const named_vec&)val);
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
void load_data_files();                                    // 0x4d0c20 (options_panel/module_load.cpp)
void load_module_files();                                    // 0x50d5e0 (inventory_panel/module_files.cpp)

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
extern logger g_log;                              // 0x5a9f60

// the text stream used to read music.txt (vtable 0x54ac88, 0x38 bytes, `failed` at +0x34).
struct text_stream {
    char data[0x34];
    char failed;                                     // +0x34
    text_stream();                                   // 0x4172d0
    ~text_stream();                                  // 0x417310
    bool open_read_text(const char* path);           // 0x415d60
};

struct Table2;                                       // built by MusicTable::build below (0x50f7e0)
extern MusicTable g_music_situation;                          // 0xa73a10

#include "../ini_file.h"                          // ini_file (load 0x4bd630) and g_module_info (0xa18238)

// 0x4c0520 is a cdecl function without parameters (game/triggers/trigger_vec.cpp); the original calls it with ecx = the application object,
// which it ignores: the call goes to that symbol through the fastcall type of the original's call
void load_game_variables();                                      // 0x4c0520
struct EditorDialog {};
extern EditorDialog g_app_obj;                 // 0x8ee3c48

#include "../gui_mgr.h"
extern GuiManager g_gui;                                 // 0x5d7080


extern int   g_sound_paused;                         // 0x5a9fd4
extern int   g_sound_muted;                          // 0x5a9fd8
extern int   g_scan_module_textures;                 // 0xa18108
extern char* g_xa44238;                              // 0xa44238

#include "../loading_screen.h"       // LoadingScreen: advance 0x50f440 here

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
        ((void (__fastcall*)(void*))load_game_variables)(&g_app_obj);
        tstring path(g_module_path);
        { tstring ini("module.ini"); path.append(ini); }
        FILE* f = fopen(path.c_str(), "r");
        if (f) {
            g_module_info.load(f);
            f44 = g_xa44238 + 5;
            fclose(f);
        } else {
            mb_error("Unable to find module info file");
        }
        { tstring key("ui_processing_ini_file"); text->assign(g_strtable.lookup(key)->c_str()); }
        (*step)++;
    } else if (*step == 2) {
        parse_module_info();
        ((LoadingMeshes*)&g_gui)->build_frames();   // 0x50f930: the gui object at 0x5d7080 holds the loading meshes
        ((Frame570*)&g_gui)->build_quad();          // 0x50f8a0
        { tstring key("ui_loading_music"); text->assign(g_strtable.lookup(key)->c_str()); }
        (*step)++;
    } else if (*step == 3) {
        if (!g_sound_paused && !g_sound_muted) {
            g_log << "Loading Music...\n";
            text_stream ts;
            {
                tstring name(g_module_path);
                { tstring m("music.txt"); name.append(m); }
                if (ts.open_read_text(name.c_str())) {
                    g_music_situation.build((Src*)&ts);
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
        load_data_files();
        { tstring key("ui_loading_setting_data"); text->assign(g_strtable.lookup(key)->c_str()); }
        (*step)++;
    } else if (*step == 6) {
        load_module_files();
        { tstring key("ui_loading_textures"); text->assign(g_strtable.lookup(key)->c_str()); }
        (*step)++;
    } else if (*step == 7) {
        g_log << "Loading Textures...\n";
        g_res.load_all(g_scan_module_textures, (rgl::string*)&g_module_path);
        g_log << "Finished Loading Textures...\n";
        { tstring key("ui_finished"); text->assign(g_strtable.lookup(key)->c_str()); }
        (*step)++;
    } else {
        finished = true;
    }
    return finished;
}
