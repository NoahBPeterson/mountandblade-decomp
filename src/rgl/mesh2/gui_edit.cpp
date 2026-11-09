// gui_edit.cpp: 0x4c48d0..0x4ca600 (TU map 102 "gui_manager_2" + 103 "item_page", merged: nothing in the range calls
// forward out of it and every class change inside it recurs on both sides, see MIGRATION.md; the end is proven by
// 0x4ca550 -> 0x4ca600). The editor's record loaders and head objects, the item records, the map editor dialogs,
// the mesh block reader, the layout/tableau group and the language-file exporter.
// Migrated from slices s004c46d0, s004c5080, s004c5a20, s004c6330, s004c6ca0, s004c7290, s004c7bf0, s004c84f0,
// s004c8ca0 and s004ca550 (0x4ca550).
#include "../../include/image_addrs.h"
#include "../../game/animator.h"      // Animator g_anim (0x8ed4f60)
#include "../../game/trial_mode.h"   // g_trial_mode (canonical)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "./gui_edit.h"
#include "../../game/options_panel/mission_item_recs.h"   // ItemA::reset_screen (0x4c6e10)
#include "../../game/terrain_a/terrain.h"   // Terrain::conv_build_scene (0x4c8630)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../game/char_sheet_panel.h"   // CharSheet (0x4c4cc0)
#include "../../game/campaign_screen.h"   // CampaignScreen (0x4c49f0, write_stream 0x4c4b70, read_stream 0x4c4c00)
#include "../../include/mat4.h"
#include "../../include/ptr_vec.h"
#include "../../include/vec.h"
#include "../../include/ptr_tag.h"
#include "../../include/phys_set.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/item_kind.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/party.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_recs.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/dialog_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/skill_rec.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/skin.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/sound_mgr.h"      // SoundMgr g_sound_mgr (0x5aa128): defined here, with the destructor 0x4c6aa0
// canonical declarations of the callees this module reaches through its layout views (definitions in other modules)
#include "../../game/config/config.h"     // Config (init_defaults, load_file), rec34_table (load_used 0x4be560)
#include "../../game/trigger_vecs.h"        // GameVec (resize 0x4c1e10)
#include "../../game/ui/mission_ui.h"      // MissionUi (ResultWindow's buttons)
#include "./rec_tables.h"                      // the _Uninit_copy helpers 0x4c3d90..0x4c3ef0 (Alloc2c, Alloc4, Alloc204)
#include "../../game/menu_rec.h"   // g_menus 0x5e29b0
struct Presentation;
extern Presentation* g_presentation;                        // 0x5e299c
extern int g_propkind_count;                                // 0x5e29e0
#include "../../game/shared_vars.h"
#include "../../game/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../game/animator.h"
#include "../../game/dialog_mgr.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
char g_loading = 1;                                // 0x5a9c38 (loader thread runs while set)
const char* g_conv_names[3] = {"tab_game_character_sheet", "tab_game_inventory_window", "tab_game_party_window"};  // 0x5a9c98
HANDLE g_mt_event;                                 // 0x5e28d8 (.bss)
CRITICAL_SECTION g_mt_cs;                          // 0x5e28dc (.bss)
extern const float g_54ab2c = 0.0625f;             // 0x54ab2c (rdata: the sample volume scale of music_set::load; a literal here is reassociated by cl)

struct OptionsDlg {
    void choose_dx_settings(struct Config* cfg, bool overwrite);   // 0x4b9b90 (game/config/options_dlg.h, which cannot be mixed with windows.h here)
};
struct rgl_config {
    void save(FILE* f);   // 0x415fb0 (rgl/resource_manager.h clashes on g_log)
};
struct PresentationRec {
    void read(stream_base& s);   // 0x4c23f0 (game/triggers/trigger_vec.h)
};
struct clip_pair {
    clip_pair(void* a_, void* b_, const int* src);   // 0x4be9c0 (game/triggers/trigger_vec.h)
};

#pragma warning(disable:4234)     // __thiscall on a free function (thunk reconstruction)
#pragma warning(disable:4700)     // 0x4c77c0: the matrix is left unread when no record is selected

// @ 0x4c48d0
// Loader (ret 4): reads two tstrings and an int into locals, then the count at +4, rebuilds the 0x34-byte
// records and counts the ones whose 0x4be470 load succeeded; finishes the list.
rec34_table g_scripts;                                 // 0xa75eb0 (.bss; constructed by the compiler: rec_table::rec_table 0x4be520, destructor below)

// the atexit destructor of g_scripts (the original's stub 0x5483c0)
rec34_table::~rec34_table()
{
    RecK* r = (RecK*)records;
    if (r) {
        delete[] r;
        records = 0;
    }
}

void rec34_table::load(stream_base& s)
{
    tstring a;
    tstring b;
    int dummy;
    s.r_tstr(&a);
    s.r_tstr(&b);
    s.r_int(&dummy);
    s.r_int(&count);
    if (records != 0) {
        delete[] (RecK*)records;                                // 0x4318c0 is the vector deleting destructor of RecK (rec34 has the same 0x34-byte layout)
        records = 0;
    }
    success = 0;
    records = new rec34[count];     // the compiler's vector-ctor-iterator expansion: cookie, then name/body per record
    for (int i = 0; i < count; i++) {
        if ((char)(*this)[i].read(&s))      // rec34::read returns a char (config.cpp declares int for the same code)
            success++;
    }
    load_used();   // 0x4be560
}                                       // a and b are destroyed by the epilogue, b first

// @ 0x4c49f0
CampaignScreen::CampaignScreen()
{
    rgl::frustum* f = &cam;
    WL_MARKER(12)
    f->perspective = 1;
    f->nearz = 0.2f;
    f->farz = 300.0f;
    f->left = -0.1f;
    f->right = 0.1f;
    f->bottom = -0.07f;
    f->top = 0.07f;
    f->build_planes();
    cam.m.o = vector4(0, 0, 10);
    target = vector4(0, 0, 0);
    velocity = vector4(0, 0, 0);
    f2b0 = 0;
    f64 = 0;
    f2b4 = 1.0f;
    f2b8 = 5.0f;
    WL_MARKER(13)
}

// @ 0x4c4b70
// (ret 4, this in ecx) Writes the vector4s part by part, each through the writer returned by the previous
// call (the target and velocity triples, then the int at +0x2b0, the two floats).
void CampaignScreen::write_stream(w_stream* w)
{
    vector4* p = &target;
    w->w_float(&p->x)->w_float(&p->y)->w_float(&p->z);
    p = &velocity;
    w->w_float(&p->x)->w_float(&p->y)->w_float(&p->z);
    w->w_float(&f2b0);
    w->w_float(&f2b4);
    w->w_float(&f2b8);
}

// @ 0x4c4c00
// Reader counterpart of 0x4c4b70 (ret 4, this in ecx): the same chained calls through slot 14 (+0x38).
void CampaignScreen::read_stream(w_stream* w)
{
    vector4* p = &target;
    w->r_float(&p->x)->r_float(&p->y)->r_float(&p->z);
    p = &velocity;
    w->r_float(&p->x)->r_float(&p->y)->r_float(&p->z);
    w->r_float(&f2b0);
    w->r_float(&f2b4);
    w->r_float(&f2b8);
}

// @ 0x4c4cc0
CharSheet::CharSheet()
{
    f_2dc = -1;
    scene = 0;
}

// @ 0x4c4d10
// std::deque<int>::_Tidy: what ~deque tail-calls (pops every element, frees the blocks and the map). The window manager's id stack
// (g_windows.dq, 0xa739f8: the atexit stub is `mov ecx, 0xa739f8; jmp 0x4c4d10`) and the agent view's tail (Tail2) run it out of line.
template void std::deque<int>::_Tidy();

// @ 0x4c4d80
void list_int::clear()
{
    list_node_int* h = w04;
    list_node_int* first = h->next;
    h->next = h;
    w04->prev = w04;
    count = 0;
    while (first != w04) {
        list_node_int* nx = first->next;
        operator delete(first);
        first = nx;
    }
}

// @ 0x4c4dc0
// std::list::_Buynode() of a list with 0x20-byte values: allocates a 0x28-byte node and links it to itself on both sides.
void* __fastcall list_int_buynode(list_int* self)
{
    std::allocator<list_node_int*> al;
    list_node_int* node = (list_node_int*)operator new(0x28);
    int linkcnt = 0;
    try {
        al.construct(&node->next, node);
        ++linkcnt;
        al.construct(&node->prev, node);
    } catch (...) {
        if (0 < linkcnt)
            al.destroy(&node->next);
        operator delete(node);
        throw;
    }
    return node;
}

// @ 0x4c4e50
// std::list::_Buynode(next, prev, value) of the same list (ret 0xc): the node's constructor is 0x4be9c0.
void* __fastcall list_int_make_node(list_int* self, void*, void* a, void* b, void* c)
{
    void* node = operator new(0x28);
    try {
        new (node) clip_pair(a, b, (const int*)c);
    } catch (...) {
        operator delete(node);
        throw;
    }
    return node;
}

// @ 0x4c4ee0
// The destructor of the world-map Terrain: the std::vector member frees its block and clears first/last/end, then the member
// destructors destroy the string at +0x845e784 and LayerNames' base destructor the fifteen 0x28-byte layer-name strings.
Terrain::~Terrain()
{
}

// @ 0x4c4f40
// Deleting destructor (ret 4). Bit 1 destroys a vector of elements back to front from the count stored before
// `this` (plus the array itself for bit 0); without bit 1 it destroys the single element.
void* __fastcall elem60_vector_delete(elem60* self, void*, unsigned flags)
{
    if (flags & 2) {
        void* base = (char*)self - 4;
        int n = *(int*)base;
        elem60* e = (elem60*)((char*)self + n * 0x60);
        while (--n >= 0) {
            e = (elem60*)((char*)e - 0x60);
            if (e->p58 != 0)
                ((RecI*)e->p58)->destroy(3);
            if (e->p38 != 0)
                operator delete[](e->p38);
            e->s44.~tstring();
            e->s28.~tstring();
            e->s10.~tstring();
            e->s0.~tstring();
        }
        if (flags & 1)
            operator delete[](base);
        return base;
    }
    if (self->p58 != 0)
        ((RecI*)self->p58)->destroy(3);
    if (self->p38 != 0)
        operator delete[](self->p38);
    self->s44.~tstring();
    self->s28.~tstring();
    self->s10.~tstring();
    self->s0.~tstring();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c5010
// Fills `n` 0x2c-byte records with the value `src` through 0x4c1b20 (the record copy); `cop` is the record
// that owns the copies. The loop is wrapped in the EH frame that guards the copy.
void __cdecl uninit_fill_n(void* first, size_t n, const void* src, block_copier* cop, ptr_tag)
{
    try {
        while (n > 0) {
            ((Alloc2c*)cop)->construct(first, src);   // block_copier is the empty Alloc2c tag (0x4c1b20)
            --n;
            first = (char*)first + 0x2c;
        }
    } catch (...) {
        throw;
    }
}

// @ 0x4c5080
// copy-construct a range of 0x38-byte records (the original's auto-object range ctor; its catch funclet 0x4c50ea is part of it). Arguments: first, last, dst (esi = [ebp+8], [ebp+0xc] end, edi = [ebp+0x10]); the 4th is the record ctor's context.
item38* copy_range38(const item38* first, const item38* last, item38* dst, editor* self)
{
    item38* next = dst;
    try {
        for (; first != last; dst++, first++)
            ((allocArr*)self)->construct((ArrSet*)dst, *(const ArrSet*)first);   // item38 is the 0x38-byte ArrSet (terrain_
    } catch (...) {
        for (; next != dst; next++)
            ((ArrSet*)next)->~ArrSet();
        throw;
    }
    return dst;
}

static inline void tidy3(void* base)
{
    void** q = (void**)base;
    if (q[0])
        operator delete(q[0]);
    q[0] = 0;
    q[1] = 0;
    q[2] = 0;
}

// @ 0x4c5190
void gui_manager::set_view(const float* v)
{
    vector4 lo;
    vector4 hi;
    vx = v[0];
    vy = v[1];
    vs = 1.0f;
    if (vx > 0.95f)
        vx = 0.95f;
    if (vx < 0.05f)
        vx = 0.05f;
    if (!overlay)
        return;
    overlay->calc_bounds(&lo, &hi);   // 0x438d80
    if (lo.x + vx < 0.011f) {
        vx = 0.011f - lo.x;
    } else if (hi.x + vx > 0.989f) {
        vx = 0.989f - hi.x;
    }
    if (lo.y + vy < 0.011f) {
        vy = 0.011f - lo.y;
    } else if (hi.y + vy > 0.739f) {
        vy = 0.739f - hi.y;
    }
    if (hi.y - lo.y > 0.728f) {
        float s = 0.728f / (hi.y - lo.y);
        vs = s;
        vy = 0.011f - s * lo.y;
    }
}

// Wheel step: forward the mask to the active child (+0x10), else to the last element of the child vector.
// @ 0x4c52f0
void gui_manager::wheel_step(int mask)
{
    if (ui_active) {
        ui_active->key(mask);
    } else if (!objs.empty()) {
        objs[objs.size() - 1]->key(mask);
    }
}

// @ 0x4c5330
void gui_manager::clear_selection()
{
    dirty = 1;
    if (ui_active) {
        ui_active->key(0x20000);
    } else if (!objs.empty()) {
        objs[objs.size() - 1]->key(0x20000);
    }
    if (objs[objs.size() - 1] != 0) {
        delete objs[objs.size() - 1];
        objs[objs.size() - 1] = 0;
    }
    f1709 = 0;
    if (!objs.empty())
        objs.pop_back();
}

// @ 0x4c53f0
void graph_obj::select_mode(int mode, char on)
{
    if (on)
        cur = nodev().size() - 1;
    else
        cur = -1;
    if (mode == 0) {
        for (unsigned i = 0; i < nodev().size(); i++)
            nodev()[i].used = on;
    } else if (mode == 1) {
        for (unsigned i = 0; i < edgev().size(); i++)
            edgev()[i].sel = on;
    } else if (mode == 2) {
        for (unsigned i = 0; i < facev().size(); i++)
            facev()[i].used = on;
    }
}

// @ 0x4c54f0
inline __declspec(noinline) void graph_obj::calc_vertex_mode(int mode)
{
    for (unsigned i = 0; i < nodev().size(); i++)
        nodev()[i].flag = 0;
    if (mode == 0) {
        for (unsigned i = 0; i < nodev().size(); i++)
            if (nodev()[i].used)
                nodev()[i].flag = 1;
    } else if (mode == 1) {
        for (unsigned i = 0; i < edgev().size(); i++)
            if (edgev()[i].sel) {
                nodev()[edgev()[i].a].flag = 1;
                nodev()[edgev()[i].b].flag = 1;
            }
    } else {
        for (unsigned i = 0; i < facev().size(); i++)
            if (facev()[i].used)
                for (int k = 0; k < facev()[i].count; k++)
                    nodev()[facev()[i].verts[k]].flag = 1;
    }
}

// @ 0x4c5670
inline __declspec(noinline) void graph_obj::calc_edge_mode(int mode)
{
    for (unsigned i = 0; i < edgev().size(); i++)
        edgev()[i].flag = 0;
    if (mode == 0) {
        for (unsigned i = 0; i < edgev().size(); i++)
            if (nodev()[edgev()[i].a].used || nodev()[edgev()[i].b].used)
                edgev()[i].flag = 1;
    } else if (mode == 1) {
        for (unsigned i = 0; i < edgev().size(); i++)
            if (edgev()[i].sel)
                edgev()[i].flag = 1;
    } else {
        for (unsigned i = 0; i < facev().size(); i++)
            if (facev()[i].used)
                for (int k = 0; k < facev()[i].count; k++)
                    edgev()[facev()[i].ends[k]].flag = 1;
    }
}

// @ 0x4c57d0
inline __declspec(noinline) void graph_obj::swap_face_vertices(int a, int b)
{
    for (unsigned i = 0; i < edgev().size(); i++) {
        if (edgev()[i].a == a)
            edgev()[i].a = b;
        else if (edgev()[i].a == b)
            edgev()[i].a = a;
        if (edgev()[i].b == a)
            edgev()[i].b = b;
        else if (edgev()[i].b == b)
            edgev()[i].b = a;
    }
}

// @ 0x4c5840
void graph_obj::swap_poly_normals(int a, int b)
{
    for (unsigned i = 0; i < facev().size(); i++) {
        for (int j = 0; j < facev()[i].count; j++) {
            if (facev()[i].ends[j] == a)
                facev()[i].ends[j] = b;
            else if (facev()[i].ends[j] == b)
                facev()[i].ends[j] = a;
        }
    }
}

// @ 0x4c58c0
void graph_obj::swap_poly_colors(int a, int b)
{
    for (unsigned i = 0; i < facev().size(); i++) {
        for (int j = 0; j < facev()[i].count; j++) {
            if (facev()[i].verts[j] == a)
                facev()[i].verts[j] = b;
            else if (facev()[i].verts[j] == b)
                facev()[i].verts[j] = a;
        }
    }
}

// @ 0x4c5940
void graph_obj::remove_vertex_from_polys(int v)
{
    for (unsigned i = 0; i < facev().size(); i++) {
        for (int k = 0; k < facev()[i].count; k++) {
            if (facev()[i].ends[k] == v) {
                remove_face(i);
                i--;
                break;
            }
        }
    }
}


// @ 0x4c59c0
void graph_obj::remove_vertex_from_edges(int v)
{
    for (unsigned i = 0; i < edgev().size(); i++) {
        if (edgev()[i].a == v || edgev()[i].b == v) {
            remove_edge(i);
            i--;
        }
    }
}

// @ 0x4c5a20
void graph_obj::update_vertex_mode()
{
    cur = -1;
    for (unsigned i = 0; i < nodev().size(); i++)
        nodev()[i].used = 0;
    if (mode == 1) {
        for (unsigned i = 0; i < edgev().size(); i++) {
            if (edgev()[i].sel) {
                nodev()[edgev()[i].a].used = 1;
                nodev()[edgev()[i].b].used = 1;
                cur = edgev()[i].b;
            }
        }
    } else {
        for (unsigned i = 0; i < facev().size(); i++) {
            if (facev()[i].used) {
                for (int k = 0; k < facev()[i].count; k++) {
                    nodev()[facev()[i].verts[k]].used = 1;
                    cur = facev()[i].verts[k];
                }
            }
        }
    }
}

// @ 0x4c5b60
void graph_obj::update_edge_mode()
{
    cur = -1;
    for (unsigned i = 0; i < edgev().size(); i++)
        edgev()[i].sel = 0;
    if (mode == 0) {
        for (unsigned i = 0; i < edgev().size(); i++) {
            if (nodev()[edgev()[i].a].used || nodev()[edgev()[i].b].used)
                edgev()[i].sel = 1;
        }
    } else {
        for (unsigned i = 0; i < facev().size(); i++) {
            if (facev()[i].used) {
                for (int k = 0; k < facev()[i].count; k++)
                    edgev()[facev()[i].ends[k]].sel = 1;
            }
        }
    }
}

// @ 0x4c5c80
void graph_obj::update_poly_mode()
{
    cur = -1;
    for (unsigned i = 0; i < facev().size(); i++)
        facev()[i].used = 0;
    if (mode == 0) {
        for (unsigned i = 0; i < facev().size(); i++) {
            for (int k = 0; k < facev()[i].count; k++)
                if (nodev()[facev()[i].verts[k]].used)
                    facev()[i].used = 1;
        }
    } else {
        for (unsigned i = 0; i < facev().size(); i++) {
            for (int k = 0; k < facev()[i].count; k++)
                if (edgev()[facev()[i].ends[k]].sel)
                    facev()[i].used = 1;
        }
    }
}

// @ 0x4c5dc0
void graph_obj::refresh_selection()
{
    if (mode == 0) {
        calc_vertex_mode(mode);
        for (unsigned i = 0; i < edgev().size(); i++) {
            if (nodev()[edgev()[i].a].flag) {
                nodev()[edgev()[i].b].used = 1;
                cur = edgev()[i].b;
            }
            if (nodev()[edgev()[i].b].flag) {
                nodev()[edgev()[i].a].used = 1;
                cur = edgev()[i].a;
            }
        }
    } else if (mode == 1) {
        calc_vertex_mode(mode);
        for (unsigned i = 0; i < edgev().size(); i++) {
            if (nodev()[edgev()[i].a].flag || nodev()[edgev()[i].b].flag)
                edgev()[i].sel = 1;
        }
    } else {
        calc_edge_mode(mode);
        for (unsigned i = 0; i < facev().size(); i++) {
            for (int k = 0; k < facev()[i].count; k++)
                if (edgev()[facev()[i].ends[k]].flag)
                    facev()[i].used = 1;
        }
    }
}

// @ 0x4c5f60
// For the edge *e: the ring position of the edge in its first face, and the face's node there (*a) and at the next
// ring position (*b).
void graph_obj::find_triangle(const int* e, int* a, int* b)
{
    int ei = *e;
    int idx = -1;
    for (int k = 0; k < facev()[edgev()[ei].f0].count; k++) {
        if (ei == facev()[edgev()[ei].f0].ends[k]) {
            idx = k;
            break;
        }
    }
    *a = facev()[edgev()[ei].f0].verts[idx];
    *b = facev()[edgev()[*e].f0].verts[(idx + 1) % facev()[edgev()[*e].f0].count];
}

// @ 0x4c6010
// Looks `key` up in the hash bucket of its hash (0x433bb0): the keys of the bucket are one 0x10-stride vector (+4),
// the translations the vector of the same bucket in the second table (+0xdf34).
tstring* StrTable::find(const tstring* key)
{
    unsigned h = hash_string(key);
    lang_bucket* b = &keys[h];
    int n = (int)b->v().size();
    for (int i = 0; i < n; i++) {
        if (b->v()[i].equals(*key))
            return &vals[h].v()[i];
    }
    return 0;
}

// @ 0x4c6080
// Translated name: the table's text for `key`, else the name with the "{***}" placeholder appended.
tstring PartyTemplate::translated()
{
    tstring* r = g_strtable.find(&key);
    if (r)
        return *r;
    tstring s = name;
    {
        tstring door("{***}");
        s.append(door);
    }
    return s;
}

// @ 0x4c6100
tstring SkillRec::translated()
{
    tstring* r = g_strtable.find(&key);
    if (r)
        return *r;
    tstring s = name;
    {
        tstring door("{***}");
        s.append(door);
    }
    return s;
}

// @ 0x4c6190
tstring* SkillRec::desc_name()
{
    tstring* r;
    {
        tstring prefix("_desc");
        r = g_strtable.find(&(key + prefix));
    }
    return r ? r : (tstring*)this;
}

// @ 0x4c61f0
tstring SkillRec::translated_desc()
{
    tstring* r;
    {
        tstring prefix("_desc");
        r = g_strtable.find(&(key + prefix));
    }
    if (r)
        return *r;
    {
        tstring s = desc;
        {
            tstring door("{***}");
            s.append(door);
        }
        return s;
    }
}

// @ 0x4c62b0
tstring MenuDoor::translated()
{
    tstring* r = g_strtable.find(&key);
    if (r)
        return *r;
    tstring s = name;
    {
        tstring door("{***}");
        s.append(door);
    }
    return s;
}

// @ 0x4c6330
tstring RecA::translated_plural()
{
    tstring* r;
    {
        tstring suffix("_pl");
        r = g_strtable.find(&(field + suffix));
    }
    if (r)
        return *r;
    {
        tstring s = text;
        {
            tstring door("{***}");
            s.append(door);
        }
        return s;
    }
}

// @ 0x4c63f0
tstring RecA::translated()
{
    tstring* r = g_strtable.find(&key);
    if (r)
        return *r;
    tstring s = name;
    {
        tstring door("{***}");
        s.append(door);
    }
    return s;
}

// @ 0x4c6470
tstring RecA::translated_text()
{
    tstring* r;
    {
        tstring suffix("_text");
        r = g_strtable.find(&(key + suffix));
    }
    if (r)
        return *r;
    {
        tstring s = text;
        {
            tstring door("{***}");
            s.append(door);
        }
        return s;
    }
}

// @ 0x4c6530
tstring RecB::translated()
{
    if (flag)
        return field;
    tstring* r = g_strtable.find(&name);
    if (r)
        return *r;
    tstring s = field;
    {
        tstring door("{***}");
        s.append(door);
    }
    return s;
}

// @ 0x4c65d0
void ItemPage::reset()
{
    if (*(void**)((char*)this + 0x54))
        (*(RefRecord**)((char*)this + 0x54))->deleting_dtor(1);
    ((TimerList*)((char*)this + 0x6c))->~TimerList();   // 0x4317b0: frees the sound set's voices and clears the pointer
    if (*(void**)((char*)this + 0x44))
        operator delete(*(void**)((char*)this + 0x44));
    *(int*)((char*)this + 0x44) = 0;
    *(int*)((char*)this + 0x48) = 0;
    *(int*)((char*)this + 0x4c) = 0;
    name.~tstring();
}

static inline const char* cs(char** pbuf) { return *pbuf ? *pbuf : (const char*)pbuf; }

// @ 0x4c6620
void ItemPage::format(char* out)
{
    if (kind == 1) {
        tstring key("ui_entry_point_entrypointname");
        _snprintf(out, 0x80, ((StrTable&)g_strtable).lookup(key)->c_str(), text_ptr);
    } else if (kind == 5) {
        tstring key("ui_passage_menu_item_passagename");
        _snprintf(out, 0x80, ((StrTable&)g_strtable).lookup(key)->c_str(), text_ptr2);
    } else if (kind == 4) {
        if (index >= 0 && index < g_ptype_count) {
            tstring key("ui_plant_plantname");
            const char* name = g_ptypes[index].name.c_str();
            _snprintf(out, 0x80, ((StrTable&)g_strtable).lookup(key)->c_str(), name);
        } else {
            _snprintf(out, 0x80, "INVALID PLANT");
        }
    } else if (kind == 2 || kind == 6) {
        if (index < 0 || index >= g_item_kind_count) {
            _snprintf(out, 0x80, "INVALID ITEM");
            return;
        }
        tstring key("ui_item_itemname");
        _snprintf(out, 0x80, ((StrTable&)g_strtable).lookup(key)->c_str(), g_items[index].display_name()->c_str());
    } else if (kind == 0) {
        tstring key("ui_prop_propname");
        _snprintf(out, 0x80, ((StrTable&)g_strtable).lookup(key)->c_str(), cs(&field.buf));
    } else {
        tstring key("ui_unknown_unknownname");
        _snprintf(out, 0x80, ((StrTable&)g_strtable).lookup(key)->c_str(), field.c_str());
    }
}

// @ 0x4c6860
tstring RecC::translated()
{
    if (flag)
        return name;
    tstring* r = g_strtable.find(&field);
    if (r)
        return *r;
    tstring s = name;
    {
        tstring door("{***}");
        s.append(door);
    }
    return s;
}

// @ 0x4c6900
// the atexit destructor of g_presentations (the original's stub 0x548400 calls this)
PresentationTable g_presentations;                      // 0xa75f2c (.bss; constructed by the compiler: rec_table::rec_table 0x4be520)

PresentationTable::~PresentationTable()
{
    if (records) {
        delete[] (RefItem*)records;
        records = 0;
    }
}

// @ 0x4c6950
void PresentationTable::fill_records(RefRecord* rec)
{
    tstring a, b;
    rec->v54(&a);
    rec->v54(&b);
    int version;
    rec->v4c(&version);                                  // the file's version word (unused)
    rec->v4c(&count);                                    // the record count, +4
    if (records) {                                      // retire(), open-coded as in the original 0x4c69a5..0x4c69e2
        delete[] (RefItem*)records;
        records = 0;
    }
    records = new RefItem[count];                       // 0x4c69e9: the array-new expansion builds each entry
    for (int i = 0; i < count; i++)
        ((PresentationRec*)&((RefItem*)records)[i])->read(*(stream_base*)rec);   // 0x4c23f0: a member call on the item
    load_used();                                        // 0x4bdec0: the presentation codes
    // void: the original sets no eax, so its "return value" is whatever the last local destructor (tstring
    // 0x42bc40) leaves in eax, and no caller reads it (inventory_panel/module_files.cpp discards it).
}

// @ 0x4c6aa0
// The sound player's destructor (the original's atexit function 0x548410 for g_sound_mgr jumps here; its sound-definition and
// group arrays are freed here, the channel vector by the member destructor).
SoundMgr::SoundMgr()
{
    slot[125] = 32;                 // the original image's dword at 0x5aa36c (g_sound_mgr + 0x244); the rest is zero
}
SoundMgr::~SoundMgr()
{
    if (defs) {
        delete[] defs;
        defs = 0;
    }
    if (groups) {
        delete[] groups;
        groups = 0;
    }
}
SoundMgr g_sound_mgr;                                       // 0x5aa128 (.bss; the compiler registers ~SoundMgr with atexit)

// @ 0x4c6b30
// Morph-target pose: clears the target's per-point offsets, finds the key frame nearest to the record's key time and
// stores (frame vertex - rest vertex) for every point.
void Skin::place(int e)
{
    PMesh* m = *((POwner*)head)->mesh;
    vector4* vecs = (vector4*)face_keys[e].delta;
    for (unsigned i = 0; i < m->npts(); i++) {
        vector4 z(0.0f, 0.0f, 0.0f);
        vecs[i] = z;
    }
    float t = (float)face_keys[e].f20;
    int lo, hi;
    float frac;
    ((rgl::keyframes*)&m->kf)->find(&lo, &hi, &frac, &t);
    if (!(frac < 0.5f))
        lo = hi;
    if (lo >= 0 && lo < m->kf.count) {
        for (unsigned i = 0; i < m->npts(); i++) {
            const vector4& f = m->kf.frames[lo].v[i];
            const vector4& r = m->pts[i].a;
            vector4 d(f.x - r.x, f.y - r.y, f.z - r.z);
            vecs[i] = d;
        }
    }
}

// @ 0x4c6ca0
tstring* MapObject::mesh_ptr()
{
    tstring* r;
    {
        tstring door("_door");
        r = g_strtable.find(&(mesh + door));
    }
    return r ? r : &name;
}

// @ 0x4c6d00
// Translated "_door" text of a menu door: the table's text for key + "_door", else the door's text with the
// "{***}" placeholder appended.
tstring MenuDoor::translated_door()
{
    tstring* r;
    {
        tstring suffix("_door");
        r = g_strtable.find(&(key + suffix));
    }
    if (r)
        return *r;
    {
        tstring s = text;
        {
            tstring door("{***}");
            s.append(door);
        }
        return s;
    }
}

// @ 0x4c6dc0
void MapDialog::set_object_target(int n)
{
    if (records[n].target) {
        MapRecord* o = &records[n];
        o->target->set_name(&o->mesh_name, &o->view);
    }
}

// @ 0x4c6e00
void reset_gui()
{
    g_gui.reset();
}

// @ 0x4c6e10 (slot 1 of the game menu record ItemA: drop the window)
void ItemA::reset_screen()
{
    g_gui.reset();
    win = 0;
}

// @ 0x4c6e30
void MapDialog::clear_objects()
{
    for (ListObj** p = list_start; p < list_end; ++p) {
        delete[] *p;
        *p = 0;
    }
    tidy3(&list_start);
    tidy3(&raw);
}

// The helpers below are cdecl and the definitions have no tag parameter; the original pushes its slot (here the fifth argument
// repeats z), so each call goes to the definition's own declaration through the five-parameter type of the original's call.
void copy_uninit_204(char* dst, unsigned n, const void* arg, Alloc204& al, ptr_tag tag);                  // 0x4c3d90 (rec_tables.cpp)
struct blk204;
char* uninit_copy_204(blk204* first, blk204* last, char* dst, Alloc204* al, ptr_tag tag);    // 0x4c1b60 (game/triggers/trigger_vec.cpp)

// @ 0x4c6eb0
MapRecord* MapDialog::insert_a(MapRecord* arr, int index, int z)
{
    ptr_tag tag;
    copy_uninit_204((char*)arr, (unsigned)index, (const void*)z, *(Alloc204*)this, tag);
    return (MapRecord*)((char*)arr + index * 0x204);
}

// @ 0x4c6ee0
int* MapDialog::insert_b(int* arr, int index, int z)
{
    ptr_tag tag;
    copy_uninit_4((char*)arr, (unsigned)index, (const void*)z, *(Alloc4*)this, tag);
    return &arr[index];
}

// @ 0x4c6f10
void MapDialog::place_a(MapRecord* arr, int index, int z)
{
    ptr_tag tag;
    ((void* (__cdecl*)(char*, char*, char*, Alloc2c&, ptr_tag))copy_uninit_2c)((char*)arr, (char*)index, (char*)z, *(Alloc2c*)this, tag);
}

// @ 0x4c6f30
void MapDialog::place_b(MapRecord* arr, int index, int z)
{
    ptr_tag tag;
    ((void* (__cdecl*)(char*, char*, char*, Alloc4&, ptr_tag))copy_uninit_4_range)((char*)arr, (char*)index, (char*)z, *(Alloc4*)this, tag);
}

// @ 0x4c6f50
void MapDialog::place_c(MapRecord* arr, int index, int z)
{
    ptr_tag tag;
    uninit_copy_str((tstring*)arr, (tstring*)index, (tstring*)z, *(tstring_alloc*)this, tag);   // MapDialog is the vector (and its allocator) of tstrings here
}

// @ 0x4c6f70
void MapDialog::place_d(MapRecord* arr, int index, int z)
{
    ptr_tag tag;
    ((char* (__cdecl*)(blk204*, blk204*, char*, Alloc204*, ptr_tag))uninit_copy_204)((blk204*)arr, (blk204*)index, (char*)z, (Alloc204*)this, tag);
}

// @ 0x4c6f90
void MapDialog::refresh_list(char use_sel)
{
    HWND hWnd = GetDlgItem(hwnd, 0x465);
    if (!hWnd)
        return;
    SendMessageA(hWnd, 0x14b, 0, 0);                    // 0x184
    unsigned count = list->get_count();
    for (unsigned i = 0; i < count; ++i) {
        int kind;
        if (use_sel) {
            kind = g_launcher_cfg.display_bits;
            if (kind == 0) {
                int m = this->mode;
                if (m == 0x17 || m == 0x19 || m == 0x1a || m == 0x18 || m == 0x1e)
                    kind = 0x10;
                else
                    kind = 0x20;
            }
        } else {
            int sel = SendMessageA(GetDlgItem(hwnd, 0x45f), 0x147, 0, 0);   // 0x147
            if (sel == 0) {
                int m = this->mode;
                if (m == 0x17 || m == 0x19 || m == 0x1a || m == 0x18 || m == 0x1e)
                    kind = 0x10;
                else
                    kind = 0x20;
            } else {
                char buf[0x80];
                SendMessageA(GetDlgItem(hwnd, 0x45f), 0x148, sel, (LPARAM)buf);   // CB_GETLBTEXT
                tstring tmp(buf);
                char c = ' ';
                int p = tmp.find(c, 0);
                kind = atoi(tmp.substr(p + 1, p + 3).c_str());
            }
        }
        int data = 0x16 + (kind == 0x10);
        bool missing;
        {
            tstring off("ui_off");
            missing = SendMessageA(hWnd, 0x14c, 0, (LPARAM)((StrTable&)g_strtable).lookup(off)->c_str()) < 0;   // LB_GETTEXT
        }
        if (missing) {
            int idx;
            {
                tstring off("ui_off");
                idx = SendMessageA(hWnd, 0x14a, 0, (LPARAM)((StrTable&)g_strtable).lookup(off)->c_str());  // 0x180
            }
            SendMessageA(hWnd, 0x151, idx, 1);                                          // 0x19a
        }
        int flag;
        if (list->get_sel(i, 1, data, 0, 2, &flag) >= 0) {
            if ((unsigned)flag > 0) {
                if (SendMessageA(hWnd, 0x14c, 0, (LPARAM)"2x") < 0) {
                    int idx = SendMessageA(hWnd, 0x14a, 1, (LPARAM)"2x");
                    SendMessageA(hWnd, 0x151, idx, 2);
                }
            }
        }
        if (list->get_sel(i, 1, data, 0, 4, &flag) >= 0) {
            if ((unsigned)flag > 0) {
                if (SendMessageA(hWnd, 0x14c, 0, (LPARAM)"4x") < 0) {
                    int idx = SendMessageA(hWnd, 0x14a, 2, (LPARAM)"4x");
                    SendMessageA(hWnd, 0x151, idx, 4);
                }
            }
        }
    }
    if (!use_sel) {
        SendMessageA(hWnd, 0x14e, 0, 0);
    } else {
        switch (g_launcher_cfg.antialiasing) {
        case 1: SendMessageA(hWnd, 0x14e, 0, 0); break;
        case 2: SendMessageA(hWnd, 0x14e, 1, 0); break;
        case 4: SendMessageA(hWnd, 0x14e, 2, 0); break;
        default: SendMessageA(hWnd, 0x14e, 0, 0); break;
        }
    }
}

// @ 0x4c7290
// (the original brackets the two SetDlgItemTextA groups with 18-byte WinLicense markers: jmp +0x10; "WL  ", 12 / 13, 0, "WL  ")
void ConfigDialog::init()
{
    RenderConfig cfg;
    ((Config*)&cfg)->init_defaults();            // RenderConfig is Config's 0x70-byte layout (first_time at +0)
    FILE* f = mb_open_user_file("rgl_config.txt", "r");
    if (f) {
        ((Config*)&cfg)->load_file(f);
        fclose(f);
    }
    if (cfg.first_time) {
        cfg.first_time = 0;
        ((OptionsDlg*)this)->choose_dx_settings((Config*)&cfg, true);   // ConfigDialog::init runs on the OptionsDlg object
        f = mb_open_user_file("rgl_config.txt", "w");
        ((rgl_config*)&cfg)->save(f);
        fclose(f);
    }
    WL_MARKER(12)
    SetWindowPos(hwnd, 0, 0, 0, 0x280, 0x1e0, 0x16);
    SetWindowPos(GetDlgItem(hwnd, 0x410), 0, 0x14, 0x46, 0xe6, 0x28, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x405), 0, 0x14, 0x6e, 0xe6, 0x28, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x41f), 0, 0x14, 0x96, 0xe6, 0x28, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x46b), 0, 0x14, 0xbe, 0xe6, 0x28, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x406), 0, 0x14, 0xe6, 0xe6, 0x28, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x469), 0, 0x14, 0x10e, 0xe6, 0x28, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x425), 0, 0x14, 0, 0x96, 0x46, 0x14);
    SetWindowPos(GetDlgItem(hwnd, 0x420), 0, 0xb4, 0x1c, 0x19a, 0xa, 0x14);

    char skin[128];
    sprintf(skin, ".\\Skins\\mb_skin_%s.smf", g_strtable.language.c_str());
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(skin, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        FindClose(h);
        SetDlgItemTextA(hwnd, 0x410, "");
        SetDlgItemTextA(hwnd, 0x41f, "");
        SetDlgItemTextA(hwnd, 0x46b, "");
        SetDlgItemTextA(hwnd, 0x405, "");
        SetDlgItemTextA(hwnd, 0x406, "");
        SetDlgItemTextA(hwnd, 0x469, "");
        SetDlgItemTextA(hwnd, 0x425, "");
    } else {
        {
            tstring key("ui_play");
            SetDlgItemTextA(hwnd, 0x410, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
        {
            tstring key("ui_configure");
            SetDlgItemTextA(hwnd, 0x405, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
        {
            tstring key("ui_check_for_new_version");
            SetDlgItemTextA(hwnd, 0x41f, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
        {
            tstring key("ui_download_new_settings");
            SetDlgItemTextA(hwnd, 0x46b, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
        {
            tstring key("ui_website");
            SetDlgItemTextA(hwnd, 0x406, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
        {
            tstring key("ui_cancel2");
            SetDlgItemTextA(hwnd, 0x469, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
        {
            tstring key("ui_current_setting");
            SetDlgItemTextA(hwnd, 0x425, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
    }

    tstring file(g_module_path);
    {
        tstring name("main.bmp");
        file.append(name);
    }
    HBITMAP logo = (HBITMAP)LoadImageA(0, file.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    *(HBITMAP*)((char*)this + 0x354) = logo;
    if (!logo)
        *(HBITMAP*)((char*)this + 0x354) = (HBITMAP)LoadImageA(0, "main.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (g_trial_mode)
        ShowWindow(GetDlgItem(hwnd, 0x419), 5);
    WL_MARKER(13)
}

// @ 0x4c77c0
void MapEditorDialog::refresh()
{
    MapRecord* r = 0;
    int var_no;
    vector4 pos;
    char buf[0x80];
    float matrix[0x10];
    char name[0x80];
    int entry_no;
    int mode;

    if (((Frame*)g_game)->object_index) {
        memcpy(matrix, (*(Pawn**)&g_mission->f65e4)->matrix, sizeof(matrix));
        matrix[14] += ((Frame*)g_game)->keep;
        pos = *(vector4*)((char*)((Frame*)g_game) + 0x1a09c);
        mode = ((Frame*)g_game)->mode;
        entry_no = 0;
        var_no = 0;
        sprintf(name, "New Object");
    } else if (((Frame*)g_game)->record >= 0) {
        r = ((Map*)g_mission)->records + ((Frame*)g_game)->record;
        entry_no = r->entry_no;
        memcpy(matrix, &r->mesh_name, sizeof(matrix));
        pos = *(vector4*)((char*)r + 0x158);
        mode = ((ItemPage*)r)->kind;
        var_no = r->var_no;
        ((ItemPage*)r)->format(name);
    }

    if (!(((Frame*)g_game)->record < 0 && ((Frame*)g_game)->object_index == 0)) {
        SetDlgItemTextA(hwnd, 0x439, name);
        _snprintf(buf, 0x80, "%3f", matrix[12]);
        SetDlgItemTextA(hwnd, 0x435, buf);
        _snprintf(buf, 0x80, "%3f", matrix[13]);
        SetDlgItemTextA(hwnd, 0x436, buf);
        _snprintf(buf, 0x80, "%3f", matrix[14]);
        SetDlgItemTextA(hwnd, 0x437, buf);
        _snprintf(buf, 0x80, "%3f", pos.x);
        SetDlgItemTextA(hwnd, 0x48b, buf);
        _snprintf(buf, 0x80, "%3f", pos.y);
        SetDlgItemTextA(hwnd, 0x48c, buf);
        _snprintf(buf, 0x80, "%3f", pos.z);
        SetDlgItemTextA(hwnd, 0x48d, buf);
        SetDlgItemInt(hwnd, 0x43a, entry_no, 0);
        SetDlgItemInt(hwnd, 0x43b, var_no, 0);

        if (mode == 1) {
            SetWindowTextA(GetDlgItem(hwnd, 0x44e), "Entry No:");
            SetWindowTextA(GetDlgItem(hwnd, 0x44f), "");
        } else if (mode == 5) {
            SetWindowTextA(GetDlgItem(hwnd, 0x44e), "Entry No:");
            SetWindowTextA(GetDlgItem(hwnd, 0x44f), "Menu Item No:");
        } else {
            SetWindowTextA(GetDlgItem(hwnd, 0x44e), "Var No:");
            SetWindowTextA(GetDlgItem(hwnd, 0x44f), "Var2 No:");
        }
        return;
    }
    sprintf(buf, "");
    SetDlgItemTextA(hwnd, 0x439, buf);
    SetDlgItemTextA(hwnd, 0x435, buf);
    SetDlgItemTextA(hwnd, 0x436, buf);
    SetDlgItemTextA(hwnd, 0x437, buf);
    SetDlgItemTextA(hwnd, 0x48b, buf);
    SetDlgItemTextA(hwnd, 0x48c, buf);
    SetDlgItemTextA(hwnd, 0x48d, buf);
}

// The record's name through the translation table, or the record's own name (the first member): each record class's
// inline accessor, written once.
template <class R>
inline const tstring* rec_name(R* r)
{
    const tstring* p = g_strtable.find(&r->key);
    return p ? p : &r->name;
}

// @ 0x4c7bf0
void MapEditorDialog::update_object_type()
{
    if (!hwnd)
        return;
    HWND type = GetDlgItem(hwnd, 0x431);
    HWND list = GetDlgItem(hwnd, 0x433);
    int list_type = SendMessageA(type, WM_USER_LIST, SendMessageA(type, 0x147, 0, 0), 0);
    SendMessageA(list, 0x184, 0, 0);
    if (!list_type) {
        for (int i = 0; i < g_propkind_count; i++) {
            // the substring is a temporary of the add-string call (destroyed once, right after it: 0x4c7caa)
            int item = SendMessageA(list, 0x180, 0, (LPARAM)((Rec78*)g_propkinds)[i].name.substr(4, ((Rec78*)g_propkinds)[i].name.len).c_str());
            SendMessageA(list, 0x19a, item, i);
        }
    } else if (list_type == 2) {
        for (int i = 0; i < g_item_kind_count; i++) {
            int item = SendMessageA(list, 0x180, 0, (LPARAM)rec_name(&g_items[i])->c_str());
            SendMessageA(list, 0x19a, item, i);
        }
    } else if (list_type == 4) {
        for (int i = 0; i < g_ptype_count; i++) {
            int item = SendMessageA(list, 0x180, 0, (LPARAM)cs(&g_ptypes[i].name.buf));
            SendMessageA(list, 0x19a, item, i);
        }
    } else if (list_type == 1) {
        SendMessageA(list, 0x180, 0, (LPARAM)"Entry Point");
        SendMessageA(list, 0x19a, 0, 0);
    } else if (list_type == 5) {
        SendMessageA(list, 0x180, 0, (LPARAM)"Passage");
        SendMessageA(list, 0x19a, 0, 0);
    }
    SendMessageA(list, 0x186, 0, 0);
    // the selection's item data (LB_GETITEMDATA of LB_GETCURSEL, 0x4c7de2..0x4c7df7): the object index stored
    ((Frame*)g_game)->sel = SendMessageA(list, 0x199, SendMessageA(list, 0x188, 0, 0), 0);
    ((Frame*)g_game)->mode = list_type;
}

// @ 0x4c7e20
void MapEditorDialog::update_objects()
{
    if (!hwnd)
        return;
    HWND list = GetDlgItem(hwnd, 0x438);
    SendMessageA(list, 0x184, 0, 0);
    for (unsigned i = 0; i < (*(std::vector<MapRecord>*)((char*)g_mission + 0x6688)).size(); i++) {
        char name[128];
        ((ItemPage*)&(*(std::vector<MapRecord>*)((char*)g_mission + 0x6688))[i])->format(name);
        int item = SendMessageA(list, 0x180, 0, (LPARAM)name);
        SendMessageA(list, 0x19a, item, i);
    }
}

// the whole record as its array destructor runs it (the same layout as module_files.cpp's prop record, whose
// compiler-generated `delete[]` is this function: symbols/slices/gui_edit.txt names it ??_EMissionObj)
struct Rec78Owner {
    virtual ~Rec78Owner();              // slot 0
};
struct Rec78Whole {
    tstring name;                       // +0x00
    char pad10[0x30];                   // +0x10
    std::vector<char> buf;              // +0x40
    char pad50[4];
    Rec78Owner* owner;                  // +0x54
    char pad58[0x14];
    TimerList groups;                   // +0x6c
    ~Rec78Whole() { delete owner; }
};

// @ 0x4c7ee0
void* __fastcall destroy_record_array(void* p, int, unsigned char flags)
{
    if (flags & 2) {
        int n = *((int*)p - 1);
        Rec78Whole* cur = (Rec78Whole*)p + n;
        while (--n >= 0)
            (--cur)->~Rec78Whole();
        if (flags & 1)
            operator delete[]((char*)p - 4);
        return (char*)p - 4;
    }
    ((ItemPage*)p)->reset();   // 0x4c65d0
    if (flags & 1)
        operator delete(p);
    return p;
}

// the apron: after rebuilding the terrain, every vertex not pinned by a flag follows the drag
// @ 0x4c7f90
void graph_obj::update_terrain(int idx, const float* d)
{
    calc_vertex_mode(idx);
    for (unsigned i = 0; i < nodev().size(); i++) {
        if (nodev()[i].flag) {
            node& n = nodev()[i];
            n.x += d[0];
            n.y += d[1];
            n.z += d[2];
        }
    }
}

// @ 0x4c8000
// Plane normal of the face that owns edge *e, scaled by the triangle's first edge (node b -> node a).
rgl::vector4 graph_obj::normal(const int* e)
{
    int a, b;
    find_triangle(e, &a, &b);
    node* w = &nodes[a];
    node* v = &nodes[b];
    float dx = v->x - w->x;
    float dy = v->y - w->y;
    float dz = v->z - w->z;
    float* m = (float*)&faces[(*(std::vector<edge>*)((char*)this + 0x38))[*e].f0];   // the edge through vector::operator[] (its reference is materialised)
    rgl::vector4 res;
    res.x = dy * m[8] - dz * m[7];
    res.y = dz * m[6] - dx * m[8];
    res.z = dx * m[7] - dy * m[6];
    res.normalize();
    return res;
}



// @ 0x4c8110
void Block::read(stream_base* stream, int version)
{
    if (*(char**)((char*)this + 0x34))
        operator delete(*(char**)((char*)this + 0x34));
    *(int*)((char*)this + 0x34) = 0;
    *(int*)((char*)this + 0x38) = 0;
    *(int*)((char*)this + 0x3c) = 0;
    stream->r_tstr((rgl::string*)((char*)this + 0x20));         // the original reads +0x20 first, then +0, +0x10
    stream->r_tstr((rgl::string*)((char*)this + 0x00));
    stream->r_tstr((rgl::string*)((char*)this + 0x10));
    ((tstring*)((char*)this + 0x00))->underscores_to_spaces();
    ((tstring*)((char*)this + 0x10))->underscores_to_spaces();
    stream->r_int((int*)((char*)this + 0x44));
    if (*(int*)((char*)this + 0x44) > 0x10)
        mb_error("Error too many variation on mesh %s", ((tstring*)((char*)this + 0x00))->c_str());
    for (int i = 0; i < *(int*)((char*)this + 0x44); i++) {
        char* v = (char*)this + 0x58 + i * 0x18;
        stream->r_tstr((rgl::string*)(v - 0x10));
        stream->r_i64((__int64*)v);                                 // slot +0x48
    }
    stream->r_uint((unsigned*)((char*)this + 0x1c8));              // +0x40: the item flags
    stream->r_u64((unsigned __int64*)((char*)this + 0x1d0));       // +0x44: the capabilities
    stream->r_int((int*)((char*)this + 0x1d8));
    stream->r_u64((unsigned __int64*)((char*)this + 0x1e0));       // +0x44
    stream->r_float((float*)((char*)this + 0x208));
    stream->r_int((int*)((char*)this + 0x220));
    stream->r_int((int*)((char*)this + 0x1fc));
    stream->r_int((int*)((char*)this + 0x200));
    stream->r_int((int*)((char*)this + 0x204));
    stream->r_int((int*)((char*)this + 0x1ec));
    stream->r_int((int*)((char*)this + 0x1e8));
    stream->r_int((int*)((char*)this + 0x1f0));
    stream->r_int((int*)((char*)this + 0x1f4));
    stream->r_int((int*)((char*)this + 0x20c));
    stream->r_int((int*)((char*)this + 0x210));
    stream->r_int((int*)((char*)this + 0x214));
    stream->r_int((int*)((char*)this + 0x218));
    if (version > 1) {                                              // itemsfile version 2: each item has triggers
        char buf[0x200];
        // the original passes the tstring itself through the varargs (a 16-byte bitwise copy at 0x4c82c4), so %s
        // reads its first dword (the read cursor), not the text: reproduced as is
        _snprintf(buf, 0x200, "Item %s", *(tstring*)((char*)this + 0x20));
        ((GameVec*)((char*)this + 0x228))->resize(*stream, buf, 0);   // 0x4c1e10: a member call on the sub-object at +0x228
    }
    *(int*)((char*)this + 0x40) = 0x3f800000;
    *(int*)((char*)this + 0x21c) = 0x3f800000;
}



// @ 0x4c8330
void E68::free_materials()
{
    for (unsigned i = 0; i < v.size(); i++) {
        ((BlockMat*)v[i].v)->owned = 0;                 // the block no longer owns the material
        ReleaseFn rel;
        rel.clear_f14 = clear_f14;
        (g_resmgr.*rel.as_member)((void*)v[i].v);
    }
    tidy3(&v._Myfirst);
    t = 0;
}

// @ 0x4c83a0
void E68::read_terrain(stream_base* stream)
{
    tstring name;
    free_materials();
    stream->r_tstr(&s);
    stream->r_int(&b1);
    stream->r_tstr(&name);
    stream->r_int(&b2);
    stream->r_int(&b3);
    stream->r_float(&b6);
    stream->r_float(&b7);
    stream->r_float(&b8);
    stream->r_float(&b9);
    b6 *= 0.001;
    b7 *= 0.001;
    b8 *= 0.001;
    b9 *= 0.001;
    m.read(stream);   // 0x4607e0
    char buf[0x200];
    _snprintf(buf, 0x200, "Tableau material %s", s.c_str());
    {
        tstring mat(buf);
        m.text.operator=(mat);
    }
    if (name.len > 1) {
        if (name.differs("none"))
            t = g_resmgr.get_material(name);
    }
}



// one of the layout's 0x68-byte tableau entries, as the code mapper reads it (the name is first)
struct ConvCode {
    tstring name;
    char pad10[0x58];
};

// @ 0x4c84f0
void __fastcall conv_map_codes(conv_rec* self)
{
    WL_MARKER(12)
    for (int k = 0; k < 3; k++) {
        int found = -1;
        unsigned i = 0;
        do {
            if (i >= ((std::vector<ConvCode>*)self)->size()) {
                log_cstr("WARNING: UNABLE TO MAP GAME TABLEAU CODE: ").w_cstr(g_conv_names[k]).w_cstr("\n");
                break;
            }
            if ((*(std::vector<ConvCode>*)self)[i].name.equals(g_conv_names[k]))
                found = i;
            i++;
        } while (found == -1);
        ((int*)((char*)self + 0x24))[k] = found;
    }
    WL_MARKER(13)
}

// @ 0x4c85e0
void __fastcall conv_free_one(conv_rec* self)
{
    ConvItem16* p = (ConvItem16*)self;
    if (p->a4 != 0)
        operator delete((void*)p->a4);
    p->a4 = 0;
    p->a8 = 0;
    p->ac = 0;
    ((tstring*)((char*)p + 0x10))->~tstring();
    ((tstring*)p)->~tstring();
}

// @ 0x4c8630
void Terrain::conv_build_scene()
{
    rgl::matrix m;
    for (int gy = 0; gy <= h; gy++) {
        for (int x = 0; x <= w; x++) {
            ((world_root*)world)->codes->cols[x][gy * 4] = *(int*)&cell[x][gy].pz;   // the tableau code rides in the cell's z lane
            int vi = cell[x][gy].f14;
            if (vi >= 0)
                ((world_root*)world)->sub->verts[vi].pos = *(vector4*)&cell[x][gy];
        }
    }
    world_sub* sub = ((world_root*)world)->sub;
    for (unsigned k = 0; k < sub->items.size(); k++) {
        scene_obj* r = sub->items[k].obj;
        if (r) {
            for (int j = 0; j < sub->items[k].n; j++)
                r->pts[j] = sub->verts[sub->items[k].idx[j]].pos;
            vector4 z0(0.0f, 0.0f, 0.0f);
            rgl::mat3 t;
            rgl::mat3* ip = identity3(&t);
            *(rgl::mat3*)&m = *ip;
            m.o = z0;
            r->set_frame(&m);
        }
    }
}

// @ 0x4c8810
edit_vert* edit_vec::ufill(edit_vert* dst, int n, const edit_vert* src)
{
    ptr_tag tag;
    uninit_fill_n(dst, (unsigned)n, src, (block_copier*)this, tag);
    return dst + n;
}

// @ 0x4c8840
// std::vector<tstring>::erase itself: the row is this instantiation (mesh_2.cpp's calls go to it too).
template std::vector<tstring>::iterator std::vector<tstring>::erase(std::vector<tstring>::iterator, std::vector<tstring>::iterator);

// @ 0x4c88a0
ptr_iter ptr_vec::erase_range(int* first, int* last)
{
    if (first != last) {
        int* end = this->last;
        int* d = first;
        while (last != end) {
            *d = *last;
            last++;
            d++;
        }
        this->last = d;
    }
    ptr_iter r;
    r.p = first;
    return r;
}

// @ 0x4c88e0
void item38_vec::copy_range(const item38* first, const item38* last, item38* dst)
{
    ptr_tag tag;
    ((item38* (__cdecl*)(const item38*, const item38*, item38*, editor*, ptr_tag))copy_range38)(first, last, dst, (editor*)this, tag);
}

// @ 0x4c8900
void __stdcall loader_thread_body()
{
    while (g_loading) {
        WaitForSingleObject(g_mt_event, INFINITE);
        EnterCriticalSection(&g_mt_cs);
        if (g_paused_time > 0.0f) {
            g_phys.update(g_paused_time);
            g_paused_time = 0.0f;
        }
        LeaveCriticalSection(&g_mt_cs);
    }
}

// @ 0x4c8980
void* __fastcall conv_item_array_dtor(conv_rec* self, int, unsigned char flags)
{
    if (flags & 2) {
        int n = *((int*)self - 1);
        Rec0xb0* cur = (Rec0xb0*)self + n;
        while (--n >= 0) {
            --cur;
            cur->tail.~Rec0xb0Tail();
            cur->name.~tstring();
        }
        if (flags & 1)
            operator delete[]((char*)self - 4);
        return (char*)self - 4;
    }
    conv_free_one(self);
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c8a40
ItemKind::~ItemKind()
{
}

// @ 0x4c8ab0
void __stdcall conv_sub_dtor(void** p)
{
    if (p[8] != 0)
        operator delete(p[8]);
    p[8] = 0;
    p[9] = 0;
    p[10] = 0;
    ((tstring*)p)->~tstring();
}

// @ 0x4c8af0
// Moves the last node over node `idx`, renames the last node's references and drops it.
void graph_obj::remove_node(int idx)
{
    int last = (int)nodev().size() - 1;
    if (idx != last) {
        nodev()[idx] = nodev()[last];
        swap_face_vertices(last, idx);
        swap_poly_colors(last, idx);
    }
    nodev().pop_back();
    remove_vertex_from_edges(last);
}

// @ 0x4c8ba0
void __fastcall conv_list_dtor(conv_rec* self)
{
    char* p = (char*)self;
    ((E68*)self)->free_materials();   // conv_rec is the mesh block (E68) here; 0x4c8330
    void** v = (void**)(p + 0x58);
    if (v[0] != 0)
        operator delete(v[0]);
    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
    if (*(void**)(p + 0x38) != 0)
        operator delete[](*(void**)(p + 0x38));
    ((tstring*)(p + 0x44))->~tstring();
    ((tstring*)p)->~tstring();
}

// @ 0x4c8bf0
owned_block::~owned_block()
{
    g_presentation = 0;
    if (begin != 0)
        operator delete(begin);
    begin = 0;
    end = 0;
    cap = 0;
}

// @ 0x4c8c20
// The window manager's constructor: the id stack's empty state (the deque's own constructor), then the 30 screen slots cleared.
WindowMgrW::WindowMgrW()
{
    for (int i = 0; i < 30; i++)
        arr[i] = 0;
}
WindowMgrW g_windows;                                       // 0xa73980 (compiler-constructed; the compiler registers ~deque for the id stack)


// The accessors below are shared by classes with the same field layout (identical code folded by the linker): 0x4c6080 is
// PartyTemplate's for DialogRec / GameString / ModRec / QuickString / FaceKey (name +0, key +0x10), 0x4c62b0 MenuDoor's
// for TroopRec / ItemKind, 0x4c6530 RecB's for Party, 0x4c6860 RecC's for FactionRec, 0x4c63f0 / 0x4c6330 / 0x4c6470 RecA's
// for QuestRec (and translated_plural of TroopRec / ItemKind). The calls go to the class that defines the function.
// the same for the records with an id and a text (dialogs, menus)
template <class R>
inline const tstring* id_text(R* r)
{
    const tstring* p = g_strtable.find(&r->id);
    return p ? p : &r->text;
}

inline Skin& skin_at(SkinVec& v, unsigned i) { return *(v.first + i); }

// the door text of a menu item: key + "_door" through the translation table, or the item's own text
inline const tstring* door_text(MenuDoor* d)
{
    const tstring* r;
    {
        tstring suffix("_door");
        r = g_strtable.find(&(d->key + suffix));
    }
    return r ? r : &d->text;
}

inline const tstring* faction_name(FactionRec* r)
{
    if (r->flag)
        return &r->name;
    const tstring* q = g_strtable.find(&r->key);
    return q ? q : &r->name;
}

inline const tstring* party_name(Party* r)
{
    if (g_module_settings.show_party_ids_instead_of_names)
        return &r->key;
    if (r->flag)
        return &r->name;
    const tstring* p = g_strtable.find(&r->key);
    return p ? p : &r->name;
}

// The party accessors, visible here as header inlines in the original (the same definitions as ui/mission_ui.cpp and
// dialog_manager.cpp; the row owners win the link). The id stays in a register across lookup only if cl sees it can't change.
inline int Map10007::lookup(unsigned id)
{
    if (id < 0x2717)
        return tab[id].value;
    int h = id % 0x2717;
    int n = 0;
    if (tab[h].key != id) {
        do {
            if (n >= 1000)
                return -1;
            h = (h + 0x4f) % 0x2717;
            n++;
        } while (tab[h].key != id);
    }
    if (n < 1000)
        return tab[h].value;
    return -1;
}
__declspec(noinline) void PartyMgr::next_party_id(int* id)
{
    unsigned i = lookup(*id) + 1;
    while ((int)i < count && pages._Myfirst[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        *id = deflt;
    else
        *id = pages._Myfirst[i >> 4][i & 0xf].id;
}
__declspec(noinline) int PartyMgr::first_party_id()
{
    unsigned i = 0;
    while ((int)i < count && pages._Myfirst[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        return deflt;
    return pages._Myfirst[i >> 4][i & 0xf].id;
}

// @ 0x4c8ca0
void GameData::save_language_files(bool translated)
{
    CreateDirectoryA("new_language", 0);                                // 0x59606c

    if (n_chars > 0) {
        FILE* f = fopen("new_language\\troops.csv", "wt+");             // 0x596050 / 0x596068
        for (int i = 0; i < n_chars; i++) {
            if (i == player_troop)
                continue;
            if (translated) {
                ((ini_file&)g_strtable).write(f, chars[i].key, ((MenuDoor*)&chars[i])->translated());
                tstring suffix("_pl");                                  // 0x5923b0
                ((ini_file&)g_strtable).write(f, chars[i].key + suffix, ((RecA*)&chars[i])->translated_plural());
            } else {
                ((ini_file&)g_strtable).write(f, chars[i].key, *rec_name(&chars[i]));
                tstring suffix("_pl");                                  // built before plural_name() runs, as in the original
                ((ini_file&)g_strtable).write(f, chars[i].key + suffix, *chars[i].plural_name());
            }
        }
        fclose(f);
    }

    if (parties.n_parties > 0) {
        FILE* f = fopen("new_language\\parties.csv", "wt+");            // 0x596034
        unsigned i = parties.first_party_id();
        while (i < parties.n_parties) {
            if (parties.get_w(i)->owner <= 0 && i != (unsigned)party_id_78) {
                if (translated) {
                    write_row(f, parties.get_w(i)->key, ((RecB*)parties.get_w(i))->translated());
                } else {
                    write_row(f, parties.get_w(i)->key, *party_name(parties.get_w(i)));
                }
            }
            parties.next_party_id((int*)&i);
        }
        fclose(f);
    }

    if (n_factions > 0) {
        FILE* f = fopen("new_language\\factions.csv", "wt+");           // 0x596018
        for (int i = 0; i < n_factions; i++) {
            if (translated) {
                write_row(f, factions[i].key, ((RecC*)&factions[i])->translated());
            } else {
                write_row(f, factions[i].key, *faction_name(&factions[i]));
            }
        }
        fclose(f);
    }

    if (g_item_kind_count > 0) {
        FILE* f = fopen("new_language\\item_kinds.csv", "wt+");         // 0x595ffc
        for (int i = 0; i < g_item_kind_count; i++) {
            if (translated) {
                {
                    write_row(f, g_items[i].key, ((MenuDoor*)&g_items[i])->translated());
                }
                tstring suffix("_pl");
                write_row(f, g_items[i].key + suffix, ((RecA*)&g_items[i])->translated_plural());
            } else {
                const tstring* v = rec_name(&g_items[i]);
                write_row(f, g_items[i].key, *v);
                tstring suffix("_pl");                                  // built before plural_name() runs, as in the original
                write_row(f, g_items[i].key + suffix, *g_items[i].plural_name());
            }
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\item_modifiers.csv", "wt+");     // 0x595fdc
        for (int i = 0; i < 43; i++) {
            if (translated) {
                write_row(f, ((ModRec*)g_item_mods)[i].key, ((PartyTemplate*)&((ModRec*)g_item_mods)[i])->translated());
            } else {
                write_row(f, ((ModRec*)g_item_mods)[i].key, *rec_name(&((ModRec*)g_item_mods)[i]));
            }
        }
        fclose(f);
    }

    if (g_party_template_count > 0) {
        FILE* f = fopen("new_language\\party_templates.csv", "wt+");    // 0x595fb8
        for (int i = 0; i < g_party_template_count; i++) {
            if (translated) {
                write_row(f, g_party_templates[i].key, g_party_templates[i].translated());
            } else {
                write_row(f, g_party_templates[i].key,
                          *rec_name(&g_party_templates[i]));
            }
        }
        fclose(f);
    }

    if (g_strings.count > 0) {
        FILE* f = fopen("new_language\\game_strings.csv", "wt+");       // 0x595f98
        for (int i = 0; i < g_strings.count; i++) {
            if (translated) {
                write_row(f, g_strings.strings[i].key, ((PartyTemplate*)&g_strings.strings[i])->translated());
            } else {
                write_row(f, g_strings.strings[i].key,
                          *rec_name(&g_strings.strings[i]));
            }
        }
        fclose(f);
    }

    if (g_strings.quick_count > 0) {
        FILE* f = fopen("new_language\\quick_strings.csv", "wt+");      // 0x595f78
        for (int i = 0; i < g_strings.quick_count; i++) {
            if (translated) {
                write_row(f, g_strings.quick[i].key, ((PartyTemplate*)&g_strings.quick[i])->translated());
            } else {
                write_row(f, g_strings.quick[i].key,
                          *rec_name(&g_strings.quick[i]));
            }
        }
        fclose(f);
    }

    if (n_quests > 0) {
        FILE* f = fopen("new_language\\quests.csv", "wt+");             // 0x595f60
        for (int i = 0; i < n_quests; i++) {
            if (translated) {
                {
                    write_row(f, quests[i].key, ((RecA*)&quests[i])->translated());
                }
                tstring suffix("_text");                                // 0x5923b4
                write_row(f, quests[i].key + suffix, ((RecA*)&quests[i])->translated_text());
            } else {
                write_row(f, quests[i].key, *rec_name(&quests[i]));
                tstring suffix("_text");                                // built before text_name() runs, as in the original
                write_row(f, quests[i].key + suffix, *quests[i].text_name());
            }
        }
        fclose(f);
    }

    if (g_dialogs.count > 0) {
        FILE* f = fopen("new_language\\dialogs.csv", "wt+");            // 0x595f44
        for (int i = 0; i < g_dialogs.count; i++) {
            for (int j = 0; j < i; j++) {
                if (g_dialogs.recs[i].id.equals(g_dialogs.recs[j].id))
                    goto next_dialog;
            }
            if (translated) {
                write_row(f, g_dialogs.recs[i].id, ((PartyTemplate*)&g_dialogs.recs[i])->translated());
            } else {
                write_row(f, g_dialogs.recs[i].id, *id_text(&g_dialogs.recs[i]));
            }
        next_dialog:
            ;
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\skins.csv", "wt+");              // 0x595f2c
        for (unsigned i = 0; i < (unsigned)(g_anim.skin_vec.first ? g_anim.skin_vec.last - g_anim.skin_vec.first : 0); i++) {
            for (int j = 0; j < g_anim.skin_vec.first[i].n_face_keys; j++) {
                if (translated) {
                    Skin& s = skin_at(g_anim.skin_vec, i);
                    write_row(f, skin_at(g_anim.skin_vec, i).face_keys[j].key, ((PartyTemplate*)&s.face_keys[j])->translated());
                } else {
                    write_row(f, g_anim.skin_vec.first[i].face_keys[j].key,
                              *rec_name(&g_anim.skin_vec.first[i].face_keys[j]));
                }
            }
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\skills.csv", "wt+");             // 0x595f14
        for (int i = 0; i < 42; i++) {
            if (translated) {
                {
                    write_row(f, g_skill_tbl[i].key, g_skill_tbl[i].translated());
                }
                tstring suffix("_desc");                                // 0x595ca4
                write_row(f, g_skill_tbl[i].key + suffix, g_skill_tbl[i].translated_desc());
            } else {
                write_row(f, g_skill_tbl[i].key, *rec_name(&g_skill_tbl[i]));
                tstring suffix("_desc");                                // built before desc_name() runs, as in the original
                write_row(f, g_skill_tbl[i].key + suffix, *g_skill_tbl[i].desc_name());
            }
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\game_menus.csv", "wt+");         // 0x595ef8
        for (int i = 0; i < g_menu_count; i++) {
            write_row(f, g_menus[i].id, *id_text(&g_menus[i]));
            for (int j = 0; j < g_menus[i].count; j++) {
                if (translated) {
                    write_row(f, ((MenuDoor*)g_menus[i].items)[j].key, ((MenuDoor*)g_menus[i].items)[j].translated());
                    if (door_text(&((MenuDoor*)g_menus[i].items)[j])->len > 2) {
                        tstring suffix("_door");                        // 0x595d64
                        write_row(f, ((MenuDoor*)g_menus[i].items)[j].key + suffix, ((MenuDoor*)g_menus[i].items)[j].translated_door());
                    }
                } else {
                    write_row(f, ((MenuDoor*)g_menus[i].items)[j].key, *rec_name(&((MenuDoor*)g_menus[i].items)[j]));
                    if (door_text(&((MenuDoor*)g_menus[i].items)[j])->len > 2) {
                        tstring suffix("_door");
                        write_row(f, ((MenuDoor*)g_menus[i].items)[j].key + suffix, *door_text(&((MenuDoor*)g_menus[i].items)[j]));
                    }
                }
            }
        }
        fclose(f);
    }
}

// @ 0x4ca550
void ResultWindow::frame_update(float)
{
    // the Enter key counts as "off" only while there is no second button (0x4ca573/0x4ca57d)
    if (b3c->clear_mask(0x10000) || (!b40 && g_keyboard.down[0x1c] && !g_keyboard.prev[0x1c]))
        goto off;
    if (!b40 || !b40->clear_mask(0x10000))
        return;
    if (flag)
        *flag = 1;
    if (text && box38)
        text->assign("");
    g_windows.apply();
    return;
off:
    if (flag)
        *flag = 0;
    if (text && box38)
        *text = *(tstring*)((char*)box38 + 0x128);
    g_windows.apply();
}

