// progress_bar.cpp: 0x4e3eb0..0x4e87d0 (TU map 108 "progress_bar"), merged from the slices s004e3780 (tail), s004e40f0, s004e4790,
// s004e4c70, s004e55f0, s004e5f80, s004e65e0, s004e6a70, s004e75a0, s004e7b30 and s004e85d0 (head). T-regime region: helpers sit
// right after their first caller; the ones the original does not inline are kept out of line (noinline) in binary order.
#include "../../include/image_addrs.h"
#include "../music_player.h"   // music_player g_music (0xa73a10): listed directly (the resolution audit reads only the source's own includes)
#include "../float_consts.h"   // shared float constants kept in memory (0x549cdc ...)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "./cls_map.h"
#include "../../include/tableau_table.h"   // TableauTbl::read 0x4e7a70 (the tableau table is the map loader)
#include "./progress_bar.h"
#include "../../include/prsnt_table.h"   // g_presentations (0xa75f2c): used[2] is the menu kind create2 builds
#include "../terrain_a/terrain.h"   // Terrain::init_map (0x4e85d0), g_terrain
#include "../window_mgr.h"      // g_windows slot 21 (0xa739d4)
// the headers whose `// 0x...` annotations the resolution audit has to see are included directly (it scans only the direct includes)
#include "../config/config.h"
#include "../config/widget_mgr.h"
#include "../strat/gui_manager.h"
#include "../font.h"
#include "../../include/e68.h"
#include "../../include/vec_elems.h"
#include "../../include/game_data.h"
#include "../../rgl/mesh.h"

// ---- 0x4e3eb0: the progress bar widget (vtable 0x597f20) -----------------------------------------------------------------------
extern const float g_progressbar_scale;         // 0x590e0c-ish constant 0.01f (the ctor stores it as the initial value)
extern const float g_max_ratio;                 // 0x549cdc = 1.0f
extern const float g_min_ratio;                 // 0x54a6f0 = 0.01f

// @ 0x4e3eb0
progress_bar::progress_bar()
{
    rect = rgl::vector4(1.0f, 1.0f, 1.0f);
    idx = 0;
    handle = 0;
    v168 = 0.01f;
    f184 = 1;
}

// @ 0x4e3f20   (ret 0x10: a MissionUi::render call with the same four arguments after marking the widget dirty)
void progress_bar::render(rgl::scene* sc, float* count, int c, int d)
{
    b11c[2] = 1;                                // +0x11e
    b11c[0] = 1;                                // +0x11c
    update_children();                          // vtable +0xc4
    MissionUi::render(sc, count, c, d);         // 0x488720
}

// @ 0x4e3f60
void progress_bar::set168(float r)
{
    if (r < 0.01f) {
        f184 = 1;
        v168 = 0.01f;
        return;
    }
    v168 = (r > g_one) ? g_one : r;
    f184 = 1;
}

struct VDtor { virtual ~VDtor(); };   // slot 0 = the deleting destructor (delete p calls it with flag 1)

// @ 0x4e3fb0
void progress_bar::update_children()
{
    if (tracks[2] != 0) {
        if (f184 == 0)
            return;
    }
    if (tracks[2] != 0)
        delete (VDtor*)tracks[2];
    if (handle != 0) {
        rgl::mesh* c = handle->clone();
        float sx = v168;
        rgl::vector4 r = rect;
        float* d = &r.x + idx;
        *d = sx * *d;
        tracks[2] = c;
        c->set_color(r);
        f184 = 0;
    }
}

// ---- 0x4e4060..0x4e4340: the std::vector members the linker placed here ----------------------------------------------------------
// Whole-class explicit instantiations (as in cls_map.cpp): push_back<E50> 0x4e4060, resize<E68> 0x4e40f0, resize<E176> 0x4e41e0,
// the RagModel helpers 0x4e42b0 / 0x4e42f0, the Skin ones 0x4e4320 / 0x4e4340 (+ its unwinder 0x4e43b0, inside the COMDAT).
// Skin is the 0x1f8-byte skin record of src/include/skin.h (constructor 0x4d3640, destructor 0x4d3720, read 0x4e7b30): the
// whole-class instantiation below also emits vector(n) / resize(n), which default-construct one (options_panel/ragdoll_models.cpp).
#include "../module_name.h"      // g_module_name (0x8ee3c38)
#include "../../include/render_flags.h"
#include "../ui_globals.h"      // UI / core globals
template class std::vector<E50>;
template class std::vector<E68>;
template class std::vector<E176>;
template class std::vector<RagModel>;
template class std::vector<Skin>;

// ---- 0x4e43e0: gui_manager::draw_string (add one line of text to the loading screen's text mesh) -----------------------------------
// the lines are kept (line_wrap / line_height / line_arg at +0xcd0 / +0xd20 / +0xe60) so that gui_manager::rebuild (0x4da900) can replay them
extern const float g_k002;                   // 0x590cd0

// @ 0x4e43e0
void gui_manager::draw_string(tstring* s, float height, int arg, char wrap)
{
    if (overlay == 0) {
        rgl::mesh* m = new rgl::mesh;
        set_overlay(m, true);
    }
    if (fa0 >= 0x50)
        return;
    gui_manager* self = this;
    int i = self->fa0;
    self->lines[i] = *s;
    self->line_wrap[i] = wrap;
    self->line_height[i] = height;
    self->line_arg[i] = arg;
    self->fa0 = i + 1;
    unsigned flags;
    float x;
    if (wrap) {
        flags = 0x10;
        x = 0.0f;
    } else {
        flags = 0;
        x = self->fa8;
    }
    {
        tstring font("ui_font");
        self->overlay->res = g_resmgr.get_material(font);
    }
    rgl::vector4 lo, hi;
    float out[2];
    float xmin = x - 1.0f;
    float xmax = x + 1.0f;
    float scale[2];
    float& pen = (float&)self->sel_ent;
    rgl::vector4 pos(x, pen, 0.0f);                 // the pen position (x, pen, 0) (0x4e4503..0x4e4537)
    // pos never sets w: the original builds pos in the dead "ui_font" tstring's slot, so the glyph vertices get that
    // tstring's len (+0xc, which ~tstring leaves alone) = 7
    *(int*)&pos.w = 7;
    scale[0] = height;
    scale[1] = height;
    // the colour word is the caller's own `arg` slot (0x4e451b pushes its address)
    g_font->layout_text(out, s, &pos, scale, self->overlay, (int)&arg, &xmin, &xmax, flags);
    pen = pen - height;                             // one line down (0x4e454a subtracts the height argument)
    self->overlay->calc_bounds(&lo, &hi);
    float w = hi.x - lo.x;                          // the laid-out text's bounds (0x4e4569: [esp+0x30] - [esp+0x20])
    float h = hi.y - lo.y;
    float sz[2];
    float at[2];
    sz[0] = w + g_k002;
    sz[1] = h + g_k002;
    at[0] = lo.x - 0.01f;
    at[1] = lo.y - 0.01f;
    self->set_rect(at, sz);
    float v[2];
    v[0] = self->vx;                               // the view pair at +0x16e8 (0x4e45c0), not +0x16e0
    v[1] = self->vy;
    self->set_view(v);
    float& widest = (float&)self->fa4;
    float& run = (float&)self->fa8;
    // a line no wider than the widest so far (or NaN) changes nothing; a wrapped one only widens; the layout is
    // rebuilt only when the running width moves (0x4e45e0..0x4e4680)
    if (!(w > widest))
        return;
    if (wrap) {
        widest = w;
        return;
    }
    float d = widest - w;
    float n = run + d;
    run = n;
    if (!(n >= widest * g_half_ratio)) {
        n = n - d * g_ratio_a;
        run = n;
        widest = n * g_ratio_b;
    } else {
        widest = w;
    }
    rebuild();
}

// ---- 0x4e4690: the ragdoll model record's loader ---------------------------------------------------------------------------------
// E50's constructor 0x4cd7a0 is Obj7a0::Obj7a0 (rgl/mesh2/mesh_2.cpp) and its loader 0x4d3330 RagBone::load (options_panel/ragdoll_models.cpp): the
// record is that class; only the members are declared, under the defining classes
struct Obj7a0 {
    Obj7a0();   // 0x4cd7a0
};
struct RagBone {
    void load(TiXmlElement* e);   // 0x4d3330
};
// @ 0x4e4690
void RagModel::load(TiXmlElement* e)
{
    const char* n = e->Attribute("name");
    if (n != 0)
        name.assign(n);
    TiXmlElement* elem = TiXmlHandle(e).FirstChild("Bones").FirstChild().Element();
    while (elem != 0) {
        char raw[sizeof(E50)];
        bones.push_back(*(E50*)new (raw) Obj7a0);         // 0x4cd7a0: the constructor of E50 is Obj7a0::Obj7a0 (rgl/mesh2/mesh_2.cpp); it returns this
        ((E50*)raw)->~E50();
        ((RagBone*)&bones[bones.size() - 1])->load(elem);      // 0x4d3330
        elem = elem->NextSiblingElement();
    }
}

// ---- 0x4e4790: the save / load screen ----------------------------------------------------------------------------------------------
extern int g_compatible_module_version;                          // 0x5e29b8
float g_5a9be0 = 0.6f;                          // 0x5a9be0
float g_5a9f20 = 0.6f;                          // 0x5a9f20 the meeting window columns' left edge (cls_map.cpp computes the column x from it)
extern const float g_time_scale;                // 0x590e28 (60.0f; defined in cls_map.cpp: a literal here is folded to a double operand)
extern const float g_minus_one;                 // 0x549ca8 (-1.0f)
extern "C" int __cdecl sscanf(const char* s, const char* fmt, ...);   // 0x535fe4

// The screen searches the settings table by hand: it walks 0x2c bytes at a time from 0xa44240, calls tstring::equals (0x42bdd0) on each
// entry key, and stops at the first match or at the count in 0xa70240 (an inlined ini_file::find with the early exit).
static __forceinline int config_index(const tstring& key)
{
    int found = -1;
    int i = 0;
    do {
        if (i >= g_game_vars.count)
            break;
        if (g_game_vars.entries[i].key.equals(key))
            found = i;
        ++i;
    } while (found == -1);
    return found;
}

// the value of entry i as a float (atof of the string, 0 when the entry does not exist)
// a settings value keyed by name, as init_screen inlines it: the default is stored before the search
static __forceinline float config_float(const tstring& key)
{
    float r = 0.0f;
    int i = config_index(key);
    if (i >= 0) {
        char* e = (char*)&g_game_vars + 0x1c + 0x2c * i;
        char* v = *(char**)(e + 4);
        r = (float)atof(v ? v : e + 4);
    }
    return r;
}

// @ 0x4e4790
void C4c31c0::init_screen()
{
    is_load = (*(int*)(slot_as<char>(21) + 0x18) == 5);
    g_gui.reset();

    ContainerBase* w = new NodeA(0);
    window = w;

    {
        float v[2];
        v[0] = -0.001f;
        v[1] = -0.001f;
        vt(window)->d22(v);
    }
    {
        float v[2];
        v[0] = 1.002f;
        v[1] = 0.752f;
        vt(window)->d25(v);
    }
    {
        // 0x4e4844..0x4e485e: the name dies (0x42bc40) before the clone is handed to the window
        rgl::mesh* m;
        {
            tstring mesh_name("load_window");
            m = (rgl::mesh*)res_get(mesh_name)->clone();
        }
        vt(window)->d31(0, m);
    }

    list_saves();

    for (int i = 0; i < 9; ++i)
        labels[i] = 0;
    for (int i = 0; i < 9; ++i) {
        if (slots[i].seconds != 0 || is_load)
            slot_button(window, i, &labels[i]);
    }

    sel21c = -1;
    sel220 = -1;
    sel218 = -1;

    if (is_load) {
        tstring h("save_b_done_h");
        tstring caption("ui_save");
        float f = config_float(h);
        btn_done = (MissionUi*)(void*)make_short_button(window, g_strtable_c.lookup(caption), 0.0f, 0.0f, f);
    } else {
        tstring h("save_b_done_h");
        tstring caption("ui_restore");
        float f = config_float(h);
        btn_done = (MissionUi*)(void*)make_short_button(window, g_strtable_c.lookup(caption), 0.0f, 0.0f, f);
    }
    label_refresh((MissionUi*)(void*)btn_done, "save_b_done");
    vt(btn_done)->d46();

    {
        tstring h("save_b_cancel_h");
        tstring caption("ui_cancel");
        float f = config_float(h);
        btn_cancel = (MissionUi*)(void*)make_short_button(window, g_strtable_c.lookup(caption), 0.0f, 0.0f, f);
    }
    label_refresh((MissionUi*)(void*)btn_cancel, "save_b_cancel");
    {
        tstring h("save_b_delete_h");
        tstring caption("ui_delete");
        float f = config_float(h);
        btn_delete = (MissionUi*)(void*)make_short_button(window, g_strtable_c.lookup(caption), 0.0f, 0.0f, f);
    }
    label_refresh((MissionUi*)(void*)btn_delete, "save_b_delete");
    vt(btn_delete)->d46();

    {
        void* root = window;
        g_gui.dirty = 1;
        g_gui.objs.push_back((MissionUi* const&)(root));
    }
}

// ---- 0x4e4c70: the label's ini-driven setup ---------------------------------------------------------------------------------------------
// "<key>_text_color" and the other settings of the save screen's controls, read out of the ini table and pushed into the widget.
// (the key is passed by value; the setters are the vtable slots +0x98 / +0x9c / +0x58 / +0x64 / +0x6c / +0x70 / +0x74)
// @ 0x4e4c70
void label_refresh(MissionUi* self, tstring key)
{
    tstring name = key + tstring("_text_color");
    int idx = g_game_vars.find(name);
    if (idx >= 0) {
        int c = g_game_vars.get_hex(&idx);
        vt(self)->d38(&c);
    }
    name = key + tstring("_highlight_text_color");
    idx = g_game_vars.find(name);
    if (idx >= 0) {
        int c = g_game_vars.get_hex(&idx);
        vt(self)->d39(&c);
    }
    {
        float v[2];
        int idx_x, idx_y;
        bool any = false;
        v[0] = 0.0f;
        v[1] = 0.0f;
        name = key + tstring("_position_x");
        idx_x = g_game_vars.find(name);
        if (idx_x >= 0) {
            v[0] = g_game_vars.get_float(&idx_x);
            any = true;
        }
        name = key + tstring("_position_y");
        idx_y = g_game_vars.find(name);
        if (idx_y >= 0) {
            v[1] = g_game_vars.get_float(&idx_y);
            any = true;                                     // 0x4e4e38: a found y also triggers the call
        }
        if (any)
            vt(self)->d22(v);
    }
    {
        float v[2];
        int idx_x, idx_y;
        bool any = false;
        v[0] = 0.0f;
        v[1] = 0.0f;
        name = key + tstring("_size_x");
        idx_x = g_game_vars.find(name);
        if (idx_x >= 0) {
            v[0] = g_game_vars.get_float(&idx_x);
            any = true;
        }
        name = key + tstring("_size_y");
        idx_y = g_game_vars.find(name);
        if (idx_y >= 0) {
            v[1] = g_game_vars.get_float(&idx_y);
            any = true;                                     // 0x4e4e38: a found y also triggers the call
        }
        if (any)
            vt(self)->d25(v);
    }
    {
        float v[2];
        int idx_x, idx_y;
        bool any = false;
        v[0] = 0.0f;
        v[1] = 0.0f;
        name = key + tstring("_text_size_x");
        idx_x = g_game_vars.find(name);
        if (idx_x >= 0) {
            v[0] = g_game_vars.get_float(&idx_x);
            any = true;
        }
        name = key + tstring("_text_size_y");
        idx_y = g_game_vars.find(name);
        if (idx_y >= 0) {
            v[1] = g_game_vars.get_float(&idx_y);
            any = true;                                     // 0x4e4e38: a found y also triggers the call
        }
        if (any)
            vt(self)->d27(v);
    }
    {
        float v[2];
        int idx_x, idx_y;
        bool any = false;
        v[0] = 0.0f;
        v[1] = 0.0f;
        name = key + tstring("_text_position_x");
        idx_x = g_game_vars.find(name);
        if (idx_x >= 0) {
            v[0] = g_game_vars.get_float(&idx_x);
            any = true;
        }
        name = key + tstring("_text_position_y");
        idx_y = g_game_vars.find(name);
        if (idx_y >= 0) {
            v[1] = g_game_vars.get_float(&idx_y);
            any = true;                                     // 0x4e4e38: a found y also triggers the call
        }
        if (any)
            vt(self)->d28(v);
    }
    name = key + tstring("_text_flags");
    idx = g_game_vars.find(name);
    if (idx >= 0) {
        int f = g_game_vars.get_hex(&idx);
        vt(self)->d29(&f);
    }
}

// ---- 0x4e51e0 / 0x4e5230: the two number getters of the ini table (an entry index in, the entry's string out) ----------------------------
// the value pointer of entry i lives at +0x20 + 0x2c * i (a tstring's buf; a null pointer means the pointer field itself holds the string)
// @ 0x4e51e0
__declspec(noinline) int ini_file::get_hex(int* idx)    // called out of line by label_refresh (0x4e4c70)
{
    int i = *idx;
    if (i < 0 || i >= count)
        return 0;
    char* e = (char*)this + 0x1c + 0x2c * i;
    char* v = *(char**)(e + 4);
    int out;
    sscanf(v ? v : e + 4, "%x", &out);
    return out;
}

// @ 0x4e5230
__declspec(noinline) float ini_file::get_float(int* idx)
{
    int i = *idx;
    if (i < 0 || i >= count)
        return 0.0f;
    char* e = (char*)this + 0x1c + 0x2c * i;
    char* v = *(char**)(e + 4);
    return (float)atof(v ? v : e + 4);
}

// ---- 0x4e5270: list the nine save files ("sgDD.sav") and read their headers ----------------------------------------------------------------
// the header 0x4e54e0 reads out of a slot file (list_saves keeps one on its stack; fields by offset)
// the stream object 0x4e5270 builds (0x54ae48 reader -> 0x54ad68 mem_stream) and 0x4e54e0 reads
#include "../../rgl/stream_inline.h"          // bin_reader: the 0x54ae48 reader whose inline constructor and destructor 0x4e5270 expands
struct SlotReader {                             // the 0x54ae48 reader's virtuals
    virtual void r0(); virtual void r1(); virtual void r2(); virtual void r3(); virtual void r4();
    virtual void r5(); virtual void r6(); virtual void r7(); virtual void r8(); virtual void r9();
    virtual void r10();
    virtual void skip(int* bytes);              // +0x2c = entry 11
    virtual void r12(); virtual void r13();
    virtual void at(float* out);                // +0x38 = entry 14
    virtual void r15(); virtual void r16(); virtual void r17(); virtual void r18();
    virtual void get_int(int* out);             // +0x4c = entry 19
    virtual void r25();
    virtual void* sub(void* owner);             // +0x54 = entry 21
};

// @ 0x4e5270
void C4c31c0::list_saves()
{
    SlotHeader hdr;
    char dir[0x104];
    char full[0x200];
    char text[0x200];
    for (int i = 0; i < 9; ++i) {
        SlotRec* slot = &slots[i];
        char* p = g_module_name.buf ? g_module_name.buf : (char*)&g_module_name.buf;
        sprintf(dir, "");
        save_path(dir);
        mb_snprintf(full, 0x200, "%s%s\\sg%d%d.sav", dir, p, i / 10, i % 10);
        bin_reader s;
        char ok = ((text_file*)&s)->open_read(full);   // 0x415290 (ret 4; the folded text_stream::open_read)
        slot->seconds = 0;
        if (ok) {
            if (hdr.read_slot((SlotReader*)&s)) {
                slot->seconds = 1;
                double tt = hdr.time;
                int mins = (int)(tt * g_time_scale);
                int mm = mins % 0x3c;
                int hh = (int)hdr.time % 0x18;
                {
                    tstring key("ui_day_datedisplay");
                    tstring* e = g_strtable_c.lookup(key);
                    char* v = e->buf;
                    char* fmt = v ? v : (char*)&e->buf;
                    int tail = 1 - (int)(hdr.time * -0.041666668f);
                    sprintf(text, fmt, tail, hh, mm / 10, mm % 10);
                }
                slot->name = hdr.title;
                slot->date.assign(text);
                int year = hdr.year;
                int day = hdr.f30;
                slot->year = year;
                slot->month = day;
                slot->bad_type = 0;
                slot->bad_version = 0;
                slot->bad_module = 0;
                char magic[5] = "WRDR";
                if (hdr.month != *(int*)magic)
                    slot->bad_type = 1;
                if (year < 1000 || year > 0x3f3)
                    slot->bad_version = 1;
                if (day < g_compatible_module_version)
                    slot->bad_module = 1;
            }
            s.failed = 1;
        }
    }
}

// @ 0x4e54e0   bool SlotHeader::read_slot(SlotReader* r): read the slot's own header
bool SlotHeader::read_slot(SlotReader* r)
{
    r->get_int(&month);                         // +0x2c
    r->get_int(&year);                          // +0x28
    f30 = 0;                                    // +0x30
    if (year >= 0x2c0)
        r->get_int(&f30);
    if (year > 0x3c0)
        r->sub(this);
    SlotReader* inner = (SlotReader*)r->sub((char*)this + 0x10);   // the reader hung off +0x10
    inner->get_int(&f20);                       // +0x20
    if (year <= 0x3c0)
        title = body;                           // +0x00 = +0x10
    r->at(&time);                               // +0x38 -> +0x24
    return true;
}

// @ 0x4e5570   the save screen's small button: "short_button" with the "short_button_down" pressed state
UiA* make_short_button(MissionUi* parent, const tstring* text, float a, float b, float c)
{
    tstring d("short_button_down");
    tstring nm("short_button");
    return new_sized_button(parent, *text, a, b, c, nm, d, 3.0f, 0.072f, 0.085f);
}

// ---- 0x4e55f0: one slot button of the save / load screen --------------------------------------------------------------------------------
// a two-float pair (the widgets take these by pointer)
struct PairF {
    float x, y;
    __forceinline PairF(float a, float b) { x = a; y = b; }
};

// a temporary's address for a by-pointer widget call (the temporary lives to the end of the caller's full expression)
static __forceinline PairF* pair_ref(const PairF& p) { return (PairF*)&p; }

// one float setting by an existing key string (the caller's temporary)
static __forceinline float config_float_key(const tstring& n)
{
    int i = g_game_vars.find(n);
    float r = 0.0f;
    if (i >= 0) {
        char* v = g_game_vars.entries[i].value.buf;
        if (!v)
            v = (char*)&g_game_vars.entries[i].value.buf;
        r = (float)atof(v);
    }
    return r;
}

// one float setting (this body is inlined 18 times in 0x4e55f0)
static __forceinline float config_float(const char* name)
{
    tstring n(name);
    int i = g_game_vars.find(n);
    float r = 0.0f;
    if (i >= 0) {
        char* v = g_game_vars.entries[i].value.buf;
        if (!v)
            v = (char*)&g_game_vars.entries[i].value.buf;
        r = (float)atof(v);
    }
    return r;
}

// @ 0x4e55f0
// `slot` is the slot index (the geometry divides it by three: three columns); `out` receives the button
void C4c31c0::slot_button(ContainerBase* parent, int slot, MissionUi** out)
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
        *out = (MissionUi*)(void*)new_sized_button(parent, empty,
                                save_x + (float)col * save_x_a,
                                save_y - (float)row * save_y_a,
                                save_size_y, key, key_down,
                                1.5544f, save_button_h, 0.085f);
    }
    ((UiA*)(void*)*out)->f184 = 1;

    // one full expression: both lookups' key strings live until d27 returns (0x4e5734..0x4e5767 destroy them after it)
    vt(*out)->d27(pair_ref(PairF(config_float_key(tstring("save_b_save_t_s")), config_float_key(tstring("save_b_save_t_s")))));
    {
        tstring k("save_b_save_t_c");
        int v = g_game_vars.hex_field(k);
        vt(*out)->d38(&v);
    }
    {
        tstring k("save_b_save_t_h_c");
        int v = g_game_vars.hex_field(k);
        vt(*out)->d39(&v);
    }

    SlotRec* rec = &slots[slot];

    float name_x = config_float("save_t_name_x");
    float name_y = config_float("save_t_name_y");
    float info_x = config_float("save_t_info_x");
    float info_y = config_float("save_t_info_y");
    float error_x = config_float("save_t_error_x");
    float error_y = config_float("save_t_error_y");

    if (is_load != 0 && rec->seconds == 0) {
        tstring k("ui_empty_slot");
        float p[2] = { name_x, name_y };
        vt(*out)->d43(g_strtable_c.lookup(k), p);
        return;
    }
    {
        float p[2] = { name_x, name_y };
        vt(*out)->d43(&rec->name, p);
    }
    {
        float p[2] = { info_x, info_y };
        vt(*out)->d43(&rec->date, p);
    }

    if (rec->bad_type != 0) {
        tstring k("Incrct. File Type!");
        float p[2] = { error_x, error_y };
        vt(*out)->d43(&k, p);
        return;
    }
    if (rec->bad_version != 0) {
        tstring k("Incomp. File Ver.!");
        float p[2] = { error_x, error_y };
        vt(*out)->d43(&k, p);
        return;
    }
    if (rec->bad_module != 0) {
        tstring k("Incomp. Mod. Ver.!");
        float p[2] = { error_x, error_y };
        vt(*out)->d43(&k, p);
    }
}

// ---- 0x4e5d30: one "%x" setting ---------------------------------------------------------------------------------------------------------------------
// @ 0x4e5d30
// out of line, as the original keeps it (0x4e55f0 calls it; inlined, its find call replaced the hex_field call)
__declspec(noinline) int ini_file::hex_field(const tstring& key)
{
    unsigned v = 0;
    int i = find(key);
    if (i >= 0) {
        char* e = (char*)this + 0x1c + 0x2c * i;
        char* s = *(char**)(e + 4) ? *(char**)(e + 4) : e + 4;
        sscanf(s, "%x", &v);
    }
    return v;
}

// ---- 0x4e5d80..0x4e5f50: the panel that wraps a menu block --------------------------------------------------------------------------------

// @ 0x4e5d80 (a jump to 0x4e5d90; not reproducible from source, see the partial list)
void ItemB::init_screen()
{
    create();
}

// @ 0x4e5d90
void ItemB::create()
{
    g_gui.reset();
    ContainerBase* w = new NodeA(0);
    win = w;
    union { float v[2]; void* root; } u;   // the root ref shares the position pair's slot (frame 8, not 0xc)
    u.v[0] = -0.001f;
    u.v[1] = -0.001f;
    vt(win)->d22(u.v);
    u.v[0] = 1.002f;
    u.v[1] = 0.752f;
    vt(win)->d25(u.v);
    block().init(win, *(int*)((char*)g_game + 0x13b04));
    u.root = win;
    g_gui.dirty = 1;
    g_gui.objs.push_back((MissionUi* const&)(u.root));
}

// @ 0x4e5e50
void ItemC::init_screen()
{
    g_music.situation = 0x400000;
    create2();
}

// @ 0x4e5e60
void ItemC::create2()
{
    g_gui.reset();
    ContainerBase* w = new NodeA(0);
    win = w;
    {
        float v[2];
        v[0] = -0.001f;
        v[1] = -0.001f;
        vt(win)->d22(v);
        v[0] = 1.002f;
        v[1] = 0.752f;
        vt(win)->d25(v);
    }
    block().init(win, g_presentations.used[2]);
    if (block().index < 0) {
        tstring n("load_window");
        vt(win)->d31(0, res_get(n)->clone());
    }
    {
        void* root = win;
        g_gui.dirty = 1;
        g_gui.objs.push_back((MissionUi* const&)(root));
    }
}

// ---- 0x4e5f50..0x4e6050: the std::vector<Skin> / std::fill<RagModel> helpers the linker placed here ---------------------------
// 0x4e5f50 = vector<Skin>::_Destroy(first, last), 0x4e5f80 = std::fill<RagModel*, RagModel>, 0x4e5fc0 = _Uninit_fill_n<Skin*>
// (its catch(...) funclet 0x4e602b is emitted inside the body's COMDAT): all emitted by the explicit instantiations above.

// ---- 0x4e6050: gui_manager::build_scene ----------------------------------------------------------------------------------------------
namespace rgl { struct mat3; }

int g_pending;                                              // 0x5e2a14
char g_pending_text[128];                        // 0x5e25b0 (.bss: the message the next MsgLog::update adds; g_pending says it is there)

// the 12-float basis + three scalars the scene build hands to mesh::render
struct RenderXform {
    float m[12];                                // +0x00
    float f30;                                  // +0x30
    float f34;                                  // +0x34
    float f38;                                  // +0x38
};

// @ 0x4e6050
void gui_manager::build_scene()
{
    rgl::scene* s1 = &x44;
    s1->reset();                                // 0x428f30

    float r = (float)screen_h / screen_w;
    float sq = (float)sqrt(r);
    view_b = sq;
    view_a = 1.0f / sq;
    fov_k = r * 1.3333333730697632f;

    if (hud_flag)
        s1->flags |= 0x200;

    s1->f8 = screen_w;
    s1->f0 = 0;
    s1->f4 = 0;
    s1->fc = screen_h;
    s1->f10 = 0;
    s1->f14 = 1.0f;

    rgl::frustum* fr = &y3f4;
    fr->perspective = 0;
    fr->nearz = 0.0f;
    fr->farz = 501.0f;
    fr->left = -0.5f;
    fr->right = 0.5f;
    fr->bottom = -0.375f;
    fr->top = 0.375f;
    fr->build_planes();                         // 0x41e4f0

    rgl::vector4 o;
    char mslot[0x40];                           // one frame slot (the original's ES+0x28) serves as both the
    rgl::matrix* m = (rgl::matrix*)mslot;       // frustum matrix and the render xform below
    o.x = 0.5f;
    o.y = 0.375f;
    o.z = 400.0f;
    rgl::identity_pos((rgl::matrix*)mslot);     // 0x40cd00
    // o.w is never assigned (the original copies the uninitialised w slot too)
    m->o = o;
    fr->m = *m;
    fr->build_planes();
    s1->fr = *fr;

    rgl::scene* s2 = &x21c;
    s2->reset();                                // 0x428f30
    s2->fc = screen_h;
    s2->f0 = 0;
    s2->f4 = 0;
    s2->f8 = screen_w;
    s2->f10 = 0;
    s2->f14 = 1.0f;
    s2->fr = *fr;
    s2->flags |= 0x1000;

    float t = 0.0f;
    for (unsigned i = 0;; ++i) {
        MissionUi** data = objs._Myfirst;
        if (!data)
            break;
        unsigned n = (unsigned)(objs._Mylast - data);
        if (i >= n)
            break;
        float zero[2];
        float size[2];
        zero[0] = 0.0f;
        zero[1] = 0.0f;
        size[0] = 1.0f;
        size[1] = 0.75f;
        data[i]->render(s1, &t, (int)zero, (int)size);
    }
    t += 1.0f;
    render_log(s2, &t);

    float ident[12];
    RenderXform* x = (RenderXform*)mslot;

    if (m1480.mask) {
        t += 1.0f;
        memcpy(x->m, identity3((rgl::mat3*)ident), 0x30);
        x->f38 = t;
        x->f30 = 0.5f;
        x->f34 = 0.65f;
        m1220.render(s2, *(rgl::matrix*)x->m);   // 0x413750
        m1480.render(s2, *(rgl::matrix*)x->m);
    }
    if (overlay) {
        t += 1.0f;
        memcpy(x->m, identity3((rgl::mat3*)ident), 0x30);
        float k = vs;
        for (int r = 0; r < 3; ++r) {
            x->m[r * 4 + 0] *= k;
            x->m[r * 4 + 1] *= k;
            x->m[r * 4 + 2] *= k;
        }
        x->f30 = vx;
        x->f34 = vy;
        x->f38 = t;
        mfb0.render(s2, *(rgl::matrix*)x->m);
        for (int i = 0; i < 4; ++i)
            corner[i]->render(s2, *(rgl::matrix*)x->m);
        overlay->render(s2, *(rgl::matrix*)x->m);
    }
    if (label_obj) {
        t += 1.0f;
        memcpy(x->m, identity3((rgl::mat3*)ident), 0x30);
        x->f30 = box_x;
        x->f34 = box_y;
        x->f38 = t;
        m570.render(s2, *(rgl::matrix*)x->m);
        label_obj->render(s2, *(rgl::matrix*)x->m);
    }
    if (drag) {
        t += 1.0f;
        memcpy(x->m, identity3((rgl::mat3*)ident), 0x30);
        x->f30 = mouse[0];
        x->f34 = mouse[1];
        x->f38 = t;
        drag->render(s2, *(rgl::matrix*)x->m);
    }
}

// ---- 0x4e64b0 / 0x4e6510: the message log's geometry reset and per-frame update ---------------------------------------------------------
// @ 0x4e64b0
void gui_manager::reset_geometry()
{
    total_h = 0;
    cur = 0;
    for (int i = 0; i < 0x20; ++i) {
        age[i] = 1000000.0f;
        meshA[i].clear_geometry();     // 0x428e80
        meshB[i].clear_geometry();
        height[i] = 0.1f;
    }
}

// @ 0x4e6510
void gui_manager::update(float dt)
{
    if (g_pending) {
        add(tstring(g_pending_text), -1);       // 0x452c00
        g_pending = 0;
    }
    for (int i = 0; i < 0x20; ++i)
        age[i] += dt;
    float v = total_h - dt * 0.1f;
    total_h = v;
    if (v < 0.0f)
        total_h = 0.0f;
}

// ---- 0x4e65e0: Shape::build_spikes ------------------------------------------------------------------------------------------------------------
// Clears the owner mesh and, for every point record of the shape, builds a four-point spike: the record's position becomes the origin of
// a local frame (identity_pos) and four points are placed in that frame; four triangles then close them into a tetrahedron. Width and
// colour come from the shape's mode (+0x78) and the record flag; the owner's two dirty flags (+0x138 / +0x164) are set once at the end.
static inline rgl::vector4 operator+(const rgl::vector4& a, const rgl::vector4& b)
{
    return rgl::vector4(a.x + b.x, a.y + b.y, a.z + b.z);
}

// @ 0x4e65e0
void Shape::build_spikes(rgl::mesh* owner)
{
    owner->clear_geometry();                                    // 0x428e80

    rgl::matrix m;                                              // the frame, ES+0x18
    identity_pos(&m);                                           // 0x40cd00

    int off = 0;
    for (unsigned i = 0;; ++i, off += 0x18) {
        if (pts == 0)
            break;
        if (i >= (unsigned)(((char*)pts_end - (char*)pts) / 0x18))
            break;
        ShapePt* e = (ShapePt*)((char*)pts + off);

        float w;
        unsigned color;
        if (mode == 0) {
            if (e->flag != 0) {
                w = 2.0f;
                color = 0xaaff5555u;
            } else {
                w = 1.4f;
                color = 0xaa4455ddu;
            }
        } else {
            w = 1.0f;
            color = 0x77555577u;
        }

        m.o = e->pos;                                           // the record is the origin of the frame

        // the four spikes, in the frame's local coordinates
        rgl::vector4 p0, p1, p2, p3;
        p0.x = m.s.x * (w * 0.034f) + m.f.x * (w * 0.0154f) + m.u.x * (w * 0.2f) + m.o.x;
        p0.y = m.s.y * (w * 0.034f) + m.f.y * (w * 0.0154f) + m.u.y * (w * 0.2f) + m.o.y;
        p0.z = m.s.z * (w * 0.034f) + m.f.z * (w * 0.0154f) + m.u.z * (w * 0.2f) + m.o.z;
        p1.x = m.s.x * (w * -0.0602f) + m.f.x * (w * -0.148f) + m.u.x * (w * -0.0666f) + m.o.x;
        p1.y = m.s.y * (w * -0.0602f) + m.f.y * (w * -0.148f) + m.u.y * (w * -0.0666f) + m.o.y;
        p1.z = m.s.z * (w * -0.0602f) + m.f.z * (w * -0.148f) + m.u.z * (w * -0.0666f) + m.o.z;
        p2.x = m.s.x * (w * -0.0602f) + m.f.x * (w * 0.1786f) + m.u.x * (w * -0.0666f) + m.o.x;
        p2.y = m.s.y * (w * -0.0602f) + m.f.y * (w * 0.1786f) + m.u.y * (w * -0.0666f) + m.o.y;
        p2.z = m.s.z * (w * -0.0602f) + m.f.z * (w * 0.1786f) + m.u.z * (w * -0.0666f) + m.o.z;
        p3.x = m.s.x * (w * 0.2226f) + m.f.x * (w * 0.0154f) + m.u.x * (w * -0.0666f) + m.o.x;
        p3.y = m.s.y * (w * 0.2226f) + m.f.y * (w * 0.0154f) + m.u.y * (w * -0.0666f) + m.o.y;
        p3.z = m.s.z * (w * 0.2226f) + m.f.z * (w * 0.0154f) + m.u.z * (w * -0.0666f) + m.o.z;

        rgl::vector4 ring[4];
        ring[0] = p0;
        ring[1] = p1;
        ring[2] = p2;
        ring[3] = p3;

        int idx[4];
        vec2 uv;
        uv.x = 0.0f;
        uv.y = 0.0f;
        rgl::vector4 up(0.0f, 0.0f, 1.0f);
        int zero = 0;
        for (int k = 0; k < 4; ++k) {
            int p = owner->add_point(&ring[k]);
            idx[k] = owner->add_vertex(p, &uv, &color, &up, &zero);
        }
        owner->add_face(idx[0], idx[1], idx[3]);
        owner->add_face(idx[0], idx[3], idx[2]);
        owner->add_face(idx[0], idx[2], idx[1]);
        owner->add_face(idx[1], idx[2], idx[3]);
    }

    *((char*)owner + 0x164) = 1;
    *((char*)owner + 0x138) = 1;
}

// ---- 0x4e6a70: Shape::build_ribbons -----------------------------------------------------------------------------------------------------------
// Walks the shape's 0x34-byte segment records and, for each, adds two rows of four points offset along the segment normal, then stitches
// them with add_face (the segment's two end points come from the point records).
// @ 0x4e6a70
void Shape::build_ribbons(rgl::mesh* owner)
{
    owner->clear_geometry();
    int elem_off = 0;
    for (unsigned i = 0; segs != 0 && i < (unsigned)(segs_end - segs); i++) {
        ShapeSeg* e = (ShapeSeg*)((char*)segs + elem_off);
        float w;
        unsigned color;
        if (mode == 1) {
            if (e->flag != 0) {
                w = 2.0f;
                color = 0xaaff5555;
            } else {
                w = 1.4f;
                color = 0xaa66cc55;
            }
        } else {
            w = 1.0f;
            color = 0x77668866;
        }
        const rgl::vector4& p0 = pts[e->a].pos;
        const rgl::vector4& p1 = pts[e->b].pos;
        rgl::vector4 d(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z);
        float len = d.length();
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            d.x *= inv; d.y *= inv; d.z *= inv;
        } else {
            d.x = 0.0f; d.y = 1.0f; d.z = 0.0f;
        }
        // perp = d x (0,0,1) normalised (normalize leaves (0,1,0) if the length is tiny); n = perp x d (the side vector)
        rgl::vector4 perp(d.y - 0.0f * d.z, 0.0f * d.z - d.x, 0.0f * d.x - 0.0f * d.y);
        float plen = perp.length();
        if (plen > 1e-10f) {
            float pinv = 1.0f / plen;
            perp.x *= pinv; perp.y *= pinv; perp.z *= pinv;
        } else {
            perp.x = 0.0f; perp.y = 1.0f; perp.z = 0.0f;
        }
        rgl::vector4 n(perp.y * d.z - perp.z * d.y, perp.z * d.x - perp.x * d.z, perp.x * d.y - perp.y * d.x);
        // Six vertices around the segment's start point p0: perp * pa + n * nb + d * along, where the 3 at the start end are
        // pushed 0.3 along d and the 3 at the far end (len - 0.3 along d) are the arrow tail (all offsets scale with the width w).
        float near_d = 0.3f, far_d = len - 0.3f;
        float pa0 = w * 0.0f, pa1 = w * -0.07f, pa2 = w * 0.07f;
        float nb0 = w * 0.08f, nb1 = w * -0.2f, nb2 = w * -0.2f;
        rgl::vector4 v[6];
        v[0].x = p0.x + perp.x * pa0 + n.x * nb0 + d.x * pa0 + d.x * near_d;
        v[0].y = p0.y + perp.y * pa0 + n.y * nb0 + d.y * pa0 + d.y * near_d;
        v[0].z = p0.z + perp.z * pa0 + n.z * nb0 + d.z * pa0 + d.z * near_d;
        v[1].x = p0.x + perp.x * pa1 + n.x * nb1 + d.x * pa0 + d.x * near_d;
        v[1].y = p0.y + perp.y * pa1 + n.y * nb1 + d.y * pa0 + d.y * near_d;
        v[1].z = p0.z + perp.z * pa1 + n.z * nb1 + d.z * pa0 + d.z * near_d;
        v[2].x = p0.x + perp.x * pa2 + n.x * nb1 + d.x * pa0 + d.x * near_d;
        v[2].y = p0.y + perp.y * pa2 + n.y * nb1 + d.y * pa0 + d.y * near_d;
        v[2].z = p0.z + perp.z * pa2 + n.z * nb1 + d.z * pa0 + d.z * near_d;
        v[3].x = p0.x + perp.x * pa0 + n.x * nb0 + d.x * pa0 + d.x * far_d;
        v[3].y = p0.y + perp.y * pa0 + n.y * nb0 + d.y * pa0 + d.y * far_d;
        v[3].z = p0.z + perp.z * pa0 + n.z * nb0 + d.z * pa0 + d.z * far_d;
        v[4].x = p0.x + perp.x * pa1 + n.x * nb1 + d.x * pa0 + d.x * far_d;
        v[4].y = p0.y + perp.y * pa1 + n.y * nb1 + d.y * pa0 + d.y * far_d;
        v[4].z = p0.z + perp.z * pa1 + n.z * nb1 + d.z * pa0 + d.z * far_d;
        v[5].x = p0.x + perp.x * pa2 + n.x * nb2 + d.x * pa0 + d.x * far_d;
        v[5].y = p0.y + perp.y * pa2 + n.y * nb2 + d.y * pa0 + d.y * far_d;
        v[5].z = p0.z + perp.z * pa2 + n.z * nb2 + d.z * pa0 + d.z * far_d;

        int idx[6];
        vec2 uv;
        uv.x = 0.0f;
        uv.y = 0.0f;
        int zero = 0;
        rgl::vector4 nn(0.0f, 0.0f, 1.0f);
        for (int k = 0; k < 6; k++) {
            int p = owner->add_point(&v[k]);
            idx[k] = owner->add_vertex(p, &uv, &color, &nn, &zero);
        }
        owner->add_face(idx[0], idx[3], idx[4]);
        owner->add_face(idx[0], idx[4], idx[1]);
        owner->add_face(idx[0], idx[2], idx[3]);
        owner->add_face(idx[2], idx[5], idx[3]);
        elem_off += 0x34;
    }
    *((char*)owner + 0x164) = 1;
    *((char*)owner + 0x138) = 1;
}

// ---- 0x4e75a0: Shape::polygon_to_mesh -----------------------------------------------------------------------------------------------------------
// map polygon mesh -> render mesh: per 0x64-byte face record, the centroid, the corners pulled 0.1 towards it, the face normal, the tint
// from the light direction (+0x80) and the mode, then one vertex per corner and two (four for a quad) triangles per side.
rgl::vector4 tri_normal(const rgl::vector4* tri);          // 0x4612d0 (terrain_a/wind.h; hidden return pointer): n = (p1-p0) x (p2-p0), normalized

// @ 0x4e75a0
void Shape::polygon_to_mesh(rgl::mesh* out)
{
    out->clear_geometry();
    unsigned index = 0;
    int off = 0;
    while (index < (faces == 0 ? 0u : (unsigned)(faces_end - faces))) {
        ShapeFace* f = (ShapeFace*)((char*)faces + off);
        int size = f->count;
        int k;
        float cx = 0.0f, cy = 0.0f, cz = 0.0f;
        for (k = 0; k < size; k++) {
            const rgl::vector4& p = pts[f->idx[k]].pos;
            cx += p.x;
            cy += p.y;
            cz += p.z;
        }
        float inv = 1.0f / (float)size;
        cx = cx * inv;
        cy = cy * inv;
        cz = cz * inv;
        rgl::vector4 corners[4];
        for (k = 0; k < size; k++) {
            const rgl::vector4& p = pts[f->idx[k]].pos;
            float dx = cx - p.x;
            float dy = cy - p.y;
            float dz = cz - p.z;
            float len = (float)sqrt(dz * dz + dy * dy + dx * dx);
            if (len > 1e-10f) {
                inv = 1.0f / len;
                dx = dx * inv;
                dy = dy * inv;
                dz = dz * inv;
            } else {
                dx = 0.0f;
                dy = 1.0f;
                dz = 0.0f;
            }
            corners[k] = rgl::vector4(dx * 0.1f + p.x, dy * 0.1f + p.y, dz * 0.1f + p.z);
        }
        rgl::vector4 normal;
        normal = tri_normal(corners);
        rgl::vector4 c;
        if (mode == 2 && f->flag) {
            c.x = 1.0f;
            c.y = 0.5f;
            c.z = 0.4f;
            c.w = 0.7f;
        } else if (mode == 2) {
            c.x = 1.0f;
            c.y = 0.9f;
            c.z = 0.4f;
            c.w = 0.7f;
        } else {
            c.x = 0.9f;
            c.y = 0.9f;
            c.z = 0.9f;
            c.w = 0.45f;
        }
        float v = ((normal.y * ly + normal.z * lz) + normal.x * lx + 1.0f) * 0.5f;
        float w = 1.0f - v;
        c.x = (w * 0.294f + v) * c.x;
        c.y = (w * 0.0f + v) * c.y;
        c.z = (w * 0.286f + v) * c.z;
        unsigned color = rgl::pack_color(&c);          // 0x40e440
        int points[4];
        int verts[4];
        for (k = 0; k < f->count; k++) {
            int zero = 0;
            vec2 uv;
            uv.x = 0.0f;
            uv.y = 0.0f;
            rgl::vector4 nn(0.0f, 0.0f, 1.0f);
            points[k] = out->add_point(&corners[k]);
            verts[k] = out->add_vertex(points[k], &uv, &color, &nn, &zero);
        }
        out->add_face(verts[0], verts[1], verts[2]);
        out->add_face(verts[0], verts[2], verts[1]);
        if (f->count > 3) {
            out->add_face(verts[0], verts[2], verts[3]);
            out->add_face(verts[0], verts[3], verts[2]);
        }
        index++;
        off += 0x64;
    }
    *((char*)out + 0x164) = 1;
    *((char*)out + 0x138) = 1;
    compute_face_normals();
}

// ---- 0x4e7a70: the map loader's block-list reader ---------------------------------------------------------------------------------------------
// (begins and ends with a WinLicense marker that cl cannot emit, see MIGRATION.md)
// @ 0x4e7a70
void TableauTbl::read(SlotReader* stream)
{
    WL_MARKER(12)
    int n;
    stream->get_int(&n);                        // vtable +0x4c: the block count
    E68 blank;
    blank.b0 = -1;
    blank.m.items = 0;
    blank.m.count = 0;
    blank.m.done = 0;
    blank.t = 0;
    recs.resize(n, blank);                    // 0x4e40f0
    WL_MARKER(13)
    for (int i = 0; i < n; i++) {
        MapBlock* b = (MapBlock*)&recs[0] + i;
        b->read_terrain(stream);                // 0x4c83a0
        ((MapBlock*)&recs[0] + i)->b0 = i;
    }
    ::conv_map_codes((conv_rec*)this);             // 0x4c84f0
}

// ---- 0x4e85d0 / 0x4e8720: MapScreen::init and MapScreen::frame_step ------------------------------------------------------------------------
extern int      g_use_shaders;                  // 0x5e28fc
extern unsigned __int64 g_bit[];                // 0x5a8e68 (game_globals.h): g_bit[2] = 4, the terrain size mask


// @ 0x4e85d0   Terrain::init_map(void* desc)   (thiscall, ret 4)
void Terrain::init_map(void* desc)
{
    ((TerrainDesc*)desc)->init();
    int r;
    for (r = 0; r <= h; r++) {
        int i;
        for (i = 0; i <= w; i++) {
            terrain_cell& c = cell[i][r];
            c.f88 = 0.0f;
            c.f8c = -1;
            c.pz = c.f88 + c.h2;
            int k;
            for (k = 0; k < 11; k++)
                c.layer[k] = 0;
        }
    }
    rgl::vector4 box = *(rgl::vector4*)((char*)world + 0x68);
    float f = box.z + box.y;
    f = (f + box.x) * g_chunk_scale;
    int row;
    for (row = 0; row < nx; row++) {
        int col;
        for (col = 0; col < ny; col++) {
            int slot;
            for (slot = 0; slot < 0xf; slot++) {
                int type = -1;
                if (g_use_shaders && layer[1].f10) {
                    if (slot == 1)
                        type = 2;
                    else if (slot == 2)
                        continue;
                }
                emit_particle(slot, type, row, col, f);
            }
        }
    }
    fill_desc_named((TerrainDesc*)desc);
}

// @ 0x4e8720   MapScreen::frame_step()
void MapScreen::frame_step()
{
    if (!(*(unsigned*)(*(char**)((char*)g_game + 0x13a50) + f5df4 * 0x494 + 0x30) & 0x100))
        return;
    int mode = 3;
    if ((((unsigned)g_bit[2] - 1) & g_terrain.flags14) != 0 &&
        ((g_terrain.flags14 & ((unsigned)g_bit[2] - 1)) == 1 || (g_terrain.flags14 & ((unsigned)g_bit[2] - 1)) == 2 ||
         (((unsigned)g_bit[2] - 1) & g_terrain.flags14) == 3))
        mode = 2;
    mesh.build(*(void**)((char*)view + 0x64), mode);
    mesh.polygon_to_mesh(mesh.poly_mesh);
    mesh.build_ribbons(mesh.ribbon_mesh);
    mesh.build_spikes(mesh.spike_mesh);
}

// ---- 0x4e7b30: the map scene's stream reader --------------------------------------------------------------------------------------------------------
// Skin::read (src/include/skin.h: the 0x1f8-byte skin record; thiscall, the stream is the one argument, ret 4). It reads, in order,
// the tag, one dword, three lod-mesh names (the second and third also looked up with the last character toggled: 'l' -> 'r', else 'R'),
// the record's own resource (+0x28 / +0x38), the 0x34-byte pose list (ItemPage::place 0x4c6b30 fills each one), the column table (up to
// 0x40 lod meshes, a 0x38-byte ArrSet each), the material table (0x18-byte entries, an ArrSet each), two string vectors, the 0xb0-byte
// cells (vector<E176>, made from MissionRec2() 0x4ce680), the slot ids, one object resource, three scalars and the 0x204-byte faces.
void mb_error(const char* fmt, ...);            // 0x42cc80
#include "../../include/crt.h"   // canonical: operator new[] (0x535482)
namespace rgl { vector4 unpack_color(const unsigned* argb); }       // 0x40e4e0 (cdecl, hidden return pointer: shapes.cpp)
#include "../arr_set.h"
#include "../../rgl/core/resmgr.h"           // lod_group (the lod mesh resource get_lod_mesh returns)

// the scene stream is the engine's stream_base (src/rgl/stream.h): the original calls its read slots through the vtable
// (r_float +0x38, r_uint +0x40, r_int +0x4c, r_tstr +0x54)
#include "../../rgl/stream.h"
struct MsStream : stream_base {};
inline void stream_f(MsStream* s, float* v) { s->r_float(v); }
inline void stream_u(MsStream* s, unsigned* v) { s->r_uint(v); }
inline void stream_i(MsStream* s, int* v) { s->r_int(v); }
inline void stream_str(MsStream* s, tstring* v) { s->r_tstr(v); }

// 0x477880 is std::vector<ArrSet>::resize (src/game/arr_set.h; fill value ArrSet()), 0x473570 Skin::copy_troop (terrain_b/terrain_b.cpp)
// and 0x4c6b30 Skin::place (rgl/mesh2/gui_edit.cpp). 0x4ccf60 is std::vector<Constraint>::resize and 0x4cbb10
// std::vector<tstring>::resize (rgl/mesh2/mesh_2.cpp): the explicit specializations are only declared.
template<> void std::vector<ArrSet>::resize(size_t, ArrSet);     // 0x477880
template<> void std::vector<Constraint>::resize(size_t, Constraint);         // 0x4ccf60
template<> void std::vector<tstring>::resize(size_t, tstring);   // 0x4cbb10

// a 0xb0-byte cell of the [0x1bc] vector<E176>; MissionRec2 (0x4ce680) is its constructor
struct MissionRec2 {
    tstring name;                               // +0x00
    void* mat10;                                // +0x10 get_material(name)
    void* mat14;                                // +0x14 get_material(name + ".LOD")
    unsigned color18;                           // +0x18
    std::vector<int> refs;                      // +0x1c: indices into [0x19c]
    rgl::vector4 tiles[8];                      // +0x2c
    int n_ac;                                   // +0xac
    MissionRec2();                              // 0x4ce680
};

// the front table the slot ids are looked up in (0x5aa130 count, 0x5aa134 entries of 0x98 bytes, the name at +0)
#include "../../include/sound_mgr.h"   // canonical: g_sound_mgr.ngroups (0x5aa130)
struct front_rec {
    tstring name;                               // +0x00
};

// vector<E176>::resize (0x4e41e0) with its fill value built in place by MissionRec2() (the same 0xb0-byte record)
typedef void (std::vector<MissionRec2>::*cell_resize_fn)(size_t, MissionRec2);

// the original's FaceKey carries a default constructor that only zeroes +0x30; the shared header here declares none,
// so this local twin restores the new[] cookie + ctor loop codegen
struct FaceKeyExt {
    tstring name;
    tstring key;
    int f20;
    float f24;
    float f28;
    unsigned f2c;
    float* delta;
    FaceKeyExt() : delta(0) {}
};
// @ 0x4e7b30
void Skin::read(MsStream* s)
{
    tstring name;
    stream_str(s, &id);
    stream_u(s, &flags);
    stream_str(s, &name);
    body = g_resmgr.get_lod_mesh(name);
    stream_str(s, &name);
    calf_l = g_resmgr.get_lod_mesh(name);
    name.buf[name.len - 1] = (name.buf[name.len - 1] == 'l') ? 'r' : 'R';
    calf_r = g_resmgr.get_lod_mesh(name);
    stream_str(s, &name);
    hand_l = g_resmgr.get_lod_mesh(name);
    name.buf[name.len - 1] = (name.buf[name.len - 1] == 'l') ? 'r' : 'R';
    hand_r = g_resmgr.get_lod_mesh(name);
    stream_str(s, &head_name);
    head = g_resmgr.get_lod_mesh(head_name);
    if (head->count == 0)
        mb_error("Unable to find face mesh: %s", head_name.c_str());

    // the poses
    stream_i(s, &n_face_keys);
    face_keys = (FaceKey*)new FaceKeyExt[n_face_keys];
    for (int i = 0; i < n_face_keys; i++) {
        FaceKey* q = &face_keys[i];
        stream_str(s, &q->key);
        stream_i(s, &q->f20);
        stream_u(s, &q->f2c);
        stream_f(s, &q->f24);
        stream_f(s, &q->f28);
        stream_str(s, &q->name);
        q->name.underscores_to_spaces();                             // 0x42bf80
        std::vector<rgl::point20>& pts = head->lists[0][0]->get70_rec();
        face_keys[i].delta = (float*)operator new[](pts.size() << 4);
        place(i);                                // 0x4c6b30
    }

    // the columns
    stream_i(s, &n_hair);
    for (int i = 0; i < n_hair; i++) {
        tstring nm;
        stream_str(s, &nm);
        if (i < 0x40)
            hair_meshes[i] = g_resmgr.get_lod_mesh(nm);
    }
    if (n_hair > 0x40)
        n_hair = 0x40;
    hair_maps.resize(n_hair, ArrSet());                                 // 0x477880
    for (int i = 0; i < n_hair; i++)
        copy_troop(&hair_maps[i], hair_meshes[i]);   // 0x473570

    // the materials
    stream_i(s, &n_beard);
    if (n_beard != 0) {
        int n = n_beard;
        BeardMesh* m = (BeardMesh*)operator new[](n * 0x18 + 4);
        if (m != 0) {
            *(int*)m = n;
            m = (BeardMesh*)((int*)m + 1);
            BeardMesh* q = m;
            for (int i = n - 1; i >= 0; i--) {
                new (&q->name) tstring();
                q->mesh = 0;
                q++;
            }
        }
        beard_meshes = m;
        beard_maps.resize(n_beard, ArrSet());                             // 0x477880
    }
    for (int i = 0; i < n_beard; i++) {
        stream_str(s, &beard_meshes[i].name);
        beard_meshes[i].mesh = g_resmgr.get_lod_mesh(beard_meshes[i].name);
        if (head->count == 0)                                        // the original tests [+0x38], not the material
            mb_error("Unable to find beard mesh: %s", beard_meshes[i].name.c_str());
        copy_troop(&beard_maps[i], beard_meshes[i].mesh);   // 0x473570
    }

    // the two string tables
    stream_i(s, &n_hair_tex);
    hair_textures.resize(n_hair_tex, tstring());                                   // 0x4cbb10
    for (int i = 0; i < n_hair_tex; i++)
        stream_str(s, &hair_textures[i]);
    stream_i(s, &n_beard_tex);
    beard_textures.resize(n_beard_tex, tstring());                                   // 0x4cbb10
    for (int i = 0; i < n_beard_tex; i++)
        stream_str(s, &beard_textures[i]);

    // the face_textures
    stream_i(s, &n_face_tex);
    {
        cell_resize_fn fn = reinterpret_cast<cell_resize_fn>(
            static_cast<void (std::vector<E176>::*)(size_t, E176)>(&std::vector<E176>::resize));
        (((std::vector<MissionRec2>*)(void*)&face_textures)->*fn)(n_face_tex, MissionRec2());   // 0x4e41e0
    }
    for (int i = 0; i < n_face_tex; i++) {
        MissionRec2* c = (MissionRec2*)&face_textures[i];
        stream_str(s, &c->name);
        c->mat10 = g_resmgr.get_material(c->name);                // 0x418a70
        {
            tstring lod(".LOD");
            c->mat14 = g_resmgr.get_material(c->name + lod);     // 0x42c840, 0x418a70
        }
        stream_u(s, &c->color18);
        if (g_use_shaders) {
            rgl::vector4 col = rgl::unpack_color(&c->color18);
            col.x *= col.x;
            col.y *= col.y;
            col.z *= col.z;
            c->color18 = rgl::pack_color(&col);                     // 0x40e440
        }
        int nref;
        stream_i(s, &nref);
        int ntile;
        stream_i(s, &ntile);
        if (ntile >= 2)
            c->n_ac = ntile;
        for (int j = 0; j < nref; j++) {
            tstring nm;
            stream_str(s, &nm);
            int found = -1;
            for (unsigned t = 0; found == -1 && t < hair_textures.size(); t++) {
                if (hair_textures[t].equals(nm)) {                           // 0x42bdd0
                    found = t;
                    c->refs.push_back(found);                       // _Insert_n 0x43e7b0 when full
                }
            }
        }
        for (int j = 0; j < ntile; j++) {
            unsigned argb;
            stream_u(s, &argb);
            c->tiles[j] = rgl::unpack_color(&argb);
        }
    }

    // the slot ids
    int nslot;
    stream_i(s, &nslot);
    tstring nm;
    for (int i = 0; i < nslot; i++) {
        int id;
        stream_i(s, &id);
        stream_str(s, &nm);
        if (id >= 0 && id < 7) {
            int found = -1;
            for (int j = 0; found == -1 && j < g_sound_mgr.ngroups; j++) {
                if (((front_rec*)((char*)g_sound_mgr.groups + j * 0x98))->name.equals(nm))
                    found = j;
            }
            voices.v[id] = found;
        }
    }

    tstring objname;
    stream_str(s, &objname);
    skeleton = g_resmgr.get_object70(objname);                    // 0x418cf0
    stream_f(s, &scale);
    stream_i(s, &blood1);
    stream_i(s, &blood2);

    // the faces
    int nface;
    stream_i(s, &nface);
    { Constraint z; memset(&z, 0, sizeof z); constraints.resize(nface, z); }    // 0x4ccf60
    for (int i = 0; i < nface; i++) {
        Constraint* f = &constraints[i];
        stream_f(s, &f->limit);
        stream_i(s, &f->mode);
        int k;
        stream_i(s, &k);
        f->n = (k < 0x3f) ? k : 0x3f;
        for (int j = 0; j < k; j++) {
            if (j < f->n) {
                stream_f(s, &f->t[j].coef);
                stream_i(s, &f->t[j].idx);
            } else {                                                // past 0x3f: read into entry 0
                stream_f(s, &f->t[0].coef);
                stream_i(s, &f->t[0].idx);
            }
        }
    }
}
