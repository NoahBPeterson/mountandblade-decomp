// gui_edit.cpp: 0x4c48d0..0x4ca600 (TU map 102 "gui_manager_2" + 103 "item_page", merged: nothing in the range calls
// forward out of it and every class change inside it recurs on both sides, see MIGRATION.md; the end is proven by
// 0x4ca550 -> 0x4ca600). The editor's record loaders and head objects, the item records, the map editor dialogs,
// the mesh block reader, the layout/tableau group and the language-file exporter.
// Migrated from slices s004c46d0, s004c5080, s004c5a20, s004c6330, s004c6ca0, s004c7290, s004c7bf0, s004c84f0,
// s004c8ca0 and s004ca550 (0x4ca550).
#include "../../include/image_addrs.h"
#include "./gui_edit.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
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
#include "../../include/skin_set.h"      // (listed directly: the resolution audit scans only the source's own includes)
rgl::mat3* __cdecl identity3(rgl::mat3* out);                 // 0x40cd60 (rgl/shapes/shapes.h; matrix3 of gui_edit.h is a view of it)
// canonical declarations of the callees this module reaches through its layout views (definitions in other modules)
#include "../../game/config/config.h"     // Config (init_defaults, load_file), rec34_table (load_used 0x4be560)
#include "../../game/trigger_vecs.h"        // GameVec (resize 0x4c1e10)
#include "./rec_tables.h"                      // the _Uninit_copy helpers 0x4c3d90..0x4c3ef0 (Alloc2c, Alloc4, Alloc204)
struct OptionsDlg {
    void choose_dx_settings(struct Config* cfg, bool overwrite);   // 0x4b9b90 (game/config/options_dlg.h, which cannot be mixed with windows.h here)
};
struct rgl_config {
    void save(FILE* f);   // 0x415fb0 (rgl/resource_manager.h clashes on g_log)
};
struct mnode {
    void calc_bounds(rgl::vector4* lo, rgl::vector4* hi);   // 0x438d80 (game/agent2/agent_types.h)
};
struct PresentationRec {
    void read(stream_base& s);   // 0x4c23f0 (game/triggers/trigger_vec.h)
};
namespace rgl { struct keyframes { void find(int* lo, int* hi, float* frac, const float* t); }; }   // 0x40f0d0 (include/keyframes.h, which needs rgl::frame_t first)
struct StrTable {
    tstring* lookup(const tstring& key);   // 0x43a1e0 (game/agent2/agent_stats.h): the UI string table
};
struct clip_pair {
    clip_pair(void* a_, void* b_, const int* src);   // 0x4be9c0 (game/triggers/trigger_vec.h)
};

#pragma warning(disable:4234)     // __thiscall on a free function (thunk reconstruction)
#pragma warning(disable:4700)     // 0x4c77c0: the matrix is left unread when no record is selected

// @ 0x4c48d0
// Loader (ret 4): reads two tstrings and an int into locals, then the count at +4, rebuilds the 0x34-byte
// records and counts the ones whose 0x4be470 load succeeded; finishes the list.
void rec34_list::load(stream_base& s)
{
    tstring a;
    tstring b;
    int dummy;
    s.r_tstr((rgl::string*)&a);
    s.r_tstr((rgl::string*)&b);
    s.r_int(&dummy);
    s.r_int(&count);
    if (v != 0) {
        delete[] (RecK*)v;                                      // 0x4318c0 is the vector deleting destructor of RecK (rec34 has the same 0x34-byte layout)
        v = 0;
    }
    nok = 0;
    v = new rec34[count];           // the compiler's vector-ctor-iterator expansion: cookie, then name/body per record
    for (int i = 0; i < count; i++) {
        if ((char)v[i].read(&s))        // rec34::read returns a char (config.cpp declares int for the same code)
            nok++;
    }
    ((rec34_table*)this)->load_used();   // 0x4be560: rec34_list has rec34_table's records / count / success head
}                                       // a and b are destroyed by the epilogue, b first

// @ 0x4c49f0
head_cam::head_cam()
{
    fr.perspective = 1;
    fr.nearz = 0.2f;
    fr.farz = 300.0f;
    fr.left = -0.1f;
    fr.right = 0.1f;
    fr.bottom = -0.07f;
    fr.top = 0.07f;
    fr.build_planes();
    fr.m.o = vector4(0, 0, 10);
    p290 = vector4(0, 0, 0);
    p2a0 = vector4(0, 0, 0);
    w2b0 = 0;
    flag64 = 0;
    f2b4 = 1.0f;
    f2b8 = 5.0f;
}

// @ 0x4c4b70
// (ret 4, this in ecx) Writes the vector4s part by part, each through the writer returned by the previous
// call (p290, p2a0, then the int at +0x2b0, the two floats).
void head_cam::write_stream(w_stream* w)
{
    vector4* p = &p290;
    w->w_float(&p->x)->w_float(&p->y)->w_float(&p->z);
    p = &p2a0;
    w->w_float(&p->x)->w_float(&p->y)->w_float(&p->z);
    w->w_float((float*)&w2b0);
    w->w_float(&f2b4);
    w->w_float(&f2b8);
}

// @ 0x4c4c00
// Reader counterpart of 0x4c4b70 (ret 4, this in ecx): the same chained calls through slot 14 (+0x38).
void head_cam::read_stream(w_stream* w)
{
    vector4* p = &p290;
    w->r_float(&p->x)->r_float(&p->y)->r_float(&p->z);
    p = &p2a0;
    w->r_float(&p->x)->r_float(&p->y)->r_float(&p->z);
    w->r_float((float*)&w2b0);
    w->r_float(&f2b4);
    w->r_float(&f2b8);
}

// @ 0x4c4c90
// Deleting destructor (ret 4): the head vtable, the string at +0x1c, then the object itself for bit 0.
void* __fastcall head_rec_delete(head_rec* self, void*, unsigned flags)
{
    *(void**)self = (void*)vtbl_5947d8;
    self->name.~tstring();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c4cc0
head_obj2::head_obj2()
{
    w2dc = -1;
    w2e4 = 0;
}

// @ 0x4c4d10
// Tail2::~Tail2: first drains the +0x10 reference window through the inlined release() (clearing +0xc when it
// reaches zero), then frees the owned pointers back to front through operator delete and drops the array.
Tail2::~Tail2()
{
    while (refcap != 0)
        release();
    unsigned int i = count;
    while (i > 0) {
        void* p = arr[i - 1];
        --i;
        if (p != 0)
            operator delete(p);
    }
    if (arr != 0)
        operator delete(arr);
    count = 0;
    arr = 0;
}

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
// std::list<int>::_Buynode: allocates a 0x28-byte node and links it to itself on both sides.
void* __fastcall list_int_buynode(list_int* self)
{
    list_node_int* node = (list_node_int*)operator new(0x28);
    if (node != 0)
        node->next = node;
    if ((list_node_int*)((char*)node + 4) != 0)
        node->prev = node;
    return node;
}

// @ 0x4c4e2f
// EH funclet of 0x4c4dc0: frees the node under construction and rethrows.
void list_int_buynode_funclet(list_node_int* node)
{
    operator delete(node);
    __CxxThrowException(0, 0);
}

// @ 0x4c4e50
// std::list<int> node constructor allocator (ret 0xc): allocates a 0x28-byte node and initialises it through
// 0x4be9c0 with the three arguments of the caller.
void* __fastcall list_int_make_node(list_int* self, void*, void* a, void* b, void* c)
{
    void* node = operator new(0x28);
    if (node != 0)
        new (node) clip_pair(a, b, (const int*)c);
    return node;
}

// @ 0x4c4ebd
// EH funclet of 0x4c4e50: frees the node under construction and rethrows.
void list_int_make_node_funclet(void* node)
{
    operator delete(node);
    __CxxThrowException(0, 0);
}

// @ 0x4c4ee0
// Frees the owned block at +0x845e7bc, clears the three words there; the member destructors then destroy the
// string at +0x845e784 and the fifteen 0x28-byte string records from +0x248 back down to +0x18.
world_obj::~world_obj()
{
    if (blk != 0)
        operator delete(blk);
    blk = 0;
    blk2 = 0;
    blk3 = 0;
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
void __cdecl uninit_fill_n(void* first, size_t n, const void* src, block_copier* cop)
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

// @ 0x4c5074
// EH funclet of 0x4c5010.
void fill_records_funclet()
{
    __CxxThrowException(0, 0);
}

// @ 0x4c5080
// copy-construct a range of 0x38-byte records (the original's auto-object range ctor; its EH pad is the
// next function). Arguments: first, last, dst (esi = [ebp+8], [ebp+0xc] end, edi = [ebp+0x10]); the 4th is the record ctor's context.
item38* copy_range38(const item38* first, const item38* last, item38* dst, editor* self)
{
    for (; first != last; first++) {
        ((allocArr*)self)->construct((ArrSet*)dst, *(const ArrSet*)first);   // item38 is the 0x38-byte ArrSet (terrain_a.cpp 0x46b6c0)
        dst++;
    }
    return dst;
}

// SEH cleanup pad of 0x4c5080: destroys the target records it had built and rethrows (the compiler-generated
// __catch$ …,0 of the original's auto-object range constructor; not expressible as C++ in cl 13.10 source,
// so it is left as an empty function).
// @ 0x4c50ea
void copy_range38_catch(void)
{
}

// @ 0x4c5110
void editor::clear_vec38()
{
    if (v38)
        operator delete((void*)v38);
    v38 = 0;
    v3c = 0;
    v40 = 0;
    name.~tstring();
}

// @ 0x4c5150
void editor::free_items()
{
    char* p = (char*)this + 0xd0;
    int i = 2;
    do {
        p -= 0x4c;
        void** v = (void**)p;
        if (v[0])
            operator delete(v[0]);
        v[0] = 0;
        v[1] = 0;
        v[2] = 0;
        ((tstring*)(p - 0x20))->~tstring();
    } while (--i);
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
void gui_view::set_view(const float* v)
{
    vector4 lo;
    vector4 hi;
    fx = v[0];
    fy = v[1];
    fs = 1.0f;
    if (fx > 0.95f)
        fx = 0.95f;
    if (fx < 0.05f)
        fx = 0.05f;
    if (!o568)
        return;
    ((mnode*)o568)->calc_bounds(&lo, &hi);   // o568 is a scene node: mesh_bnd is a view of it, the callee 0x438d80 is mnode::calc_bounds
    if (lo.x + fx < g_f011) {
        fx = g_f011 - lo.x;
    } else if (hi.x + fx > g_f989) {
        fx = g_f989 - hi.x;
    }
    if (lo.y + fy < g_f011) {
        fy = g_f011 - lo.y;
    } else if (hi.y + fy > g_f739) {
        fy = g_f739 - hi.y;
    }
    if (hi.y - lo.y > g_f728) {
        float s = g_f728 / (hi.y - lo.y);
        fs = s;
        fy = g_f011 - s * lo.y;
    }
}

// Wheel step: forward the mask to the active child (+0x10), else to the last element of the child vector.
// @ 0x4c52f0
void gui_view::wheel_step(int mask)
{
    if (active) {
        active->on_event(mask);
    } else if (!kids.empty()) {
        kids[kids.size() - 1]->on_event(mask);
    }
}

// @ 0x4c5330
void gui_view::clear_selection()
{
    f1708 = 1;
    if (active) {
        active->on_event(0x20000);
    } else if (!kids.empty()) {
        kids[kids.size() - 1]->on_event(0x20000);
    }
    if (kids[kids.size() - 1] != 0) {
        delete kids[kids.size() - 1];
        kids[kids.size() - 1] = 0;
    }
    f1709 = 0;
    if (!kids.empty())
        kids.pop_back();
}

// @ 0x4c53f0
void graph_obj::select_mode(int mode, char on)
{
    int i, n;
    if (on)
        cur = nodes ? (int)(nodes_end - nodes) - 1 : -1;
    else
        cur = -1;
    if (mode == 0) {
        n = nodes ? (int)(nodes_end - nodes) : 0;
        for (i = 0; (unsigned)i < (unsigned)n; i++)
            nodes[i].used = on;
    } else if (mode == 1) {
        n = edges ? (int)(edges_end - edges) : 0;
        for (i = 0; (unsigned)i < (unsigned)n; i++)
            edges[i].sel = on;
    } else if (mode == 2) {
        n = faces ? (int)(faces_end - faces) : 0;
        for (i = 0; (unsigned)i < (unsigned)n; i++)
            faces[i].used = on;
    }
}

// @ 0x4c54f0
void graph_obj::calc_vertex_mode(int mode)
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
void graph_obj::calc_edge_mode(int mode)
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
void graph_obj::swap_face_vertices(int a, int b)
{
    for (unsigned i = 0; edges && i < edges_n(); i++) {
        edge* e = edges + i;
        if (e->a == a)
            e->a = b;
        else if (e->a == b)
            e->a = a;
        if (e->b == a)
            e->b = b;
        else if (e->b == b)
            e->b = a;
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
    for (unsigned i = 0; edges && i < edges_n(); i++) {
        edge* e = edges + i;
        if (e->a == v || e->b == v) {
            remove_edge(i);
            i--;
        }
    }
}

// @ 0x4c5a20
void graph_obj::update_vertex_mode()
{
    cur = -1;
    int n = nodes_n();
    for (int i = 0; i < n; i++)
        nodes[i].used = 0;
    if (mode == 1) {
        int ne = edges ? (int)(edges_end - edges) : 0;
        for (int i = 0; i < ne; i++) {
            if (edges[i].sel) {
                nodes[edges[i].a].used = 1;
                nodes[edges[i].b].used = 1;
                cur = edges[i].b;
            }
        }
    } else {
        int np = faces ? (int)(faces_end - faces) : 0;
        for (int i = 0; i < np; i++) {
            graph_face* p = &faces[i];
            if (p->used) {
                for (int k = 0; k < p->count; k++) {
                    nodes[p->verts[k]].used = 1;
                    cur = p->verts[k];
                }
            }
        }
    }
}

// @ 0x4c5b60
void graph_obj::update_edge_mode()
{
    cur = -1;
    int ne = edges_n();
    for (int i = 0; i < ne; i++)
        edges[i].sel = 0;
    if (mode == 0) {
        for (int i = 0; i < ne; i++) {
            if (nodes[edges[i].a].used || nodes[edges[i].b].used)
                edges[i].sel = 1;
        }
    } else {
        int np = faces ? (int)(faces_end - faces) : 0;
        for (int i = 0; i < np; i++) {
            graph_face* p = &faces[i];
            if (p->used) {
                for (int k = 0; k < p->count; k++)
                    edges[p->ends[k]].sel = 1;
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
        int ne = edges ? (int)(edges_end - edges) : 0;
        for (int i = 0; i < ne; i++) {
            if (nodes[edges[i].a].flag) {
                nodes[edges[i].b].used = 1;
                cur = edges[i].b;
            }
            if (nodes[edges[i].b].flag) {
                nodes[edges[i].a].used = 1;
                cur = edges[i].a;
            }
        }
    } else if (mode == 1) {
        calc_vertex_mode(mode);
        int ne = edges ? (int)(edges_end - edges) : 0;
        for (int i = 0; i < ne; i++) {
            if (nodes[edges[i].a].flag || nodes[edges[i].b].flag)
                edges[i].sel = 1;
        }
    } else {
        calc_edge_mode(mode);
        int np = faces ? (int)(faces_end - faces) : 0;
        for (int i = 0; i < np; i++) {
            graph_face* p = &faces[i];
            for (int k = 0; k < p->count; k++)
                if (edges[p->ends[k]].flag)
                    p->used = 1;
        }
    }
}

// @ 0x4c5f60
// For the edge *e: the ring position of the edge in its first face, and the face's node there (*a) and at the next
// ring position (*b).
void graph_obj::find_triangle(const int* e, int* a, int* b)
{
    int ei = *e;
    graph_face* f = &faces[edges[ei].f0];
    int idx = -1;
    for (int k = 0; k < f->count; k++) {
        if (ei == f->ends[k]) {
            idx = k;
            break;
        }
    }
    *a = f->verts[idx];
    graph_face* g = &faces[edges[*e].f0];
    *b = g->verts[(idx + 1) % g->count];
}

// @ 0x4c6010
// Looks `key` up in the hash bucket of its hash (0x433bb0): the keys of the bucket are one 0x10-stride vector (+4),
// the translations the vector of the same bucket in the second table (+0xdf34).
tstring* LangTables::find(const tstring* key) throw()
{
    unsigned h = hash_string(key);
    lang_bucket* b = &keys[h];
    int n = b->myfirst == 0 ? 0 : (int)(b->mylast - b->myfirst);
    for (int i = 0; i < n; i++) {
        if (b->myfirst[i].equals(*key))
            return vals[h].myfirst + i;
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
        tstring door(s__door);
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
        tstring door(s__door);
        s.append(door);
    }
    return s;
}

// @ 0x4c6190
tstring* SkillRec::desc_name()
{
    tstring* r;
    {
        tstring prefix(s_prefix_desc);
        r = g_strtable.find(&(key + prefix));
    }
    return r ? r : (tstring*)this;
}

// @ 0x4c61f0
tstring SkillRec::translated_desc()
{
    tstring* r;
    {
        tstring prefix(s_prefix_desc);
        r = g_strtable.find(&(key + prefix));
    }
    if (r)
        return *r;
    {
        tstring s = desc;
        {
            tstring door(s__door);
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
        tstring door(s__door);
        s.append(door);
    }
    return s;
}

// @ 0x4c6330
tstring RecA::translated_plural()
{
    tstring* r;
    {
        tstring suffix(s__pl);
        r = g_strtable.find(&(field + suffix));
    }
    if (r)
        return *r;
    {
        tstring s = text;
        {
            tstring door(s__door);
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
        tstring door(s__door);
        s.append(door);
    }
    return s;
}

// @ 0x4c6470
tstring RecA::translated_text()
{
    tstring* r;
    {
        tstring suffix(s__text);
        r = g_strtable.find(&(key + suffix));
    }
    if (r)
        return *r;
    {
        tstring s = text;
        {
            tstring door(s__door);
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
        tstring door(s__door);
        s.append(door);
    }
    return s;
}

// @ 0x4c65d0
void ItemPage::reset()
{
    if (*(void**)((char*)this + 0x54))
        (*(RefRecord**)((char*)this + 0x54))->deleting_dtor(1);
    ((RecGList*)((char*)this + 0x6c))->clear();   // 0x4317b0: frees the keyframe track array and clears the pointer
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
        tstring key(s_entry_point);
        _snprintf(out, 0x80, ((StrTable&)g_strtable).lookup(key)->c_str(), text_ptr);
    } else if (kind == 5) {
        tstring key(s_passage);
        _snprintf(out, 0x80, ((StrTable&)g_strtable).lookup(key)->c_str(), text_ptr2);
    } else if (kind == 4) {
        if (index < 0 || index >= g_ptype_count) {
            _snprintf(out, 0x80, s_invalid_plant);
            return;
        }
        tstring key(s_plant);
        _snprintf(out, 0x80, ((StrTable&)g_strtable).lookup(key)->c_str(), cs(&g_ptypes[index].name));
    } else if (kind == 2 || kind == 6) {
        if (index < 0 || index >= g_item_kind_count) {
            _snprintf(out, 0x80, s_invalid_item);
            return;
        }
        tstring key(s_item_name);
        _snprintf(out, 0x80, ((StrTable&)g_strtable).lookup(key)->c_str(), g_items[index].display_name()->c_str());
    } else if (kind == 0) {
        tstring key(s_prop);
        _snprintf(out, 0x80, ((StrTable&)g_strtable).lookup(key)->c_str(), cs(&field.buf));
    } else {
        tstring key(s_unknown);
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
        tstring door(s__door);
        s.append(door);
    }
    return s;
}

// @ 0x4c6900
void RefList::retire()
{
    if (records) {
        delete[] records;
        records = 0;
    }
}

// @ 0x4c6950
void ItemPage::fill_records(RefRecord* rec)
{
    tstring a, b;
    rec->v54(&a);
    rec->v54(&b);
    int version;
    rec->v4c(&version);                                  // the file's version word (unused)
    rec->v4c((void*)((char*)this + 4));                  // the record count, +4
    RefList* rl = (RefList*)this;
    if (rl->records) {                                  // retire(), open-coded as in the original 0x4c69a5..0x4c69e2
        delete[] rl->records;
        rl->records = 0;
    }
    rl->records = new RefItem[*(int*)((char*)this + 4)];  // 0x4c69e9: the array-new expansion builds each entry
    int n = *(int*)((char*)this + 4);
    for (int i = 0; i < n; i++)
        ((PresentationRec*)((char*)rl->records + i * 0x24))->read(*(stream_base*)rec);   // 0x4c23f0: a member call on the item, the stream as its argument
    ((loaded_prsnt_table*)this)->load_used();           // 0x4bdec0: the presentation codes
    // void: the original sets no eax, so its "return value" is whatever the last local destructor (tstring
    // 0x42bc40) leaves in eax, and no caller reads it (inventory_panel/module_files.cpp discards it).
}

// @ 0x4c6aa0
void ItemPage::clear()
{
    if (*(void**)((char*)this + 0x4)) {
        delete[] *(char**)((char*)this + 0x4);
        *(void**)((char*)this + 0x4) = 0;
    }
    Pad98*& p = *(Pad98**)((char*)this + 0xc);
    if (p) {
        delete[] p;
        p = 0;
    }
    if (extra)
        operator delete(extra);
    extra = 0;
    extra_end = 0;
    extra_cap = 0;
}

// @ 0x4c6b30
// Morph-target pose: clears the target's per-point offsets, finds the key frame nearest to the record's key time and
// stores (frame vertex - rest vertex) for every point.
void ItemPage::place(int e)
{
    PMesh* m = *owner->mesh;
    for (unsigned i = 0; i < m->npts(); i++) {
        vector4 z(0.0f, 0.0f, 0.0f);
        poses[e].vecs[i] = z;
    }
    float t = (float)poses[e].key;
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
            poses[e].vecs[i] = d;
        }
    }
}

// @ 0x4c6ca0
tstring* MapObject::mesh_ptr()
{
    tstring* r;
    {
        tstring door(s_suffix_door);
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
        tstring suffix(s_suffix_door);
        r = g_strtable.find(&(key + suffix));
    }
    if (r)
        return *r;
    {
        tstring s = text;
        {
            tstring door(s__door);
            s.append(door);
        }
        return s;
    }
}

// @ 0x4c6dc0
void MapDialog::set_object_target(int n)
{
    MapRecord* o = &records[n];
    if (o->target)
        o->target->set_name(&o->mesh_name, &o->view);
}

// @ 0x4c6e00
void reset_gui()
{
    g_gui.reset();
}

// @ 0x4c6e10
void MapDialog::reset_state()
{
    g_gui.reset();
    f2c = 0;
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
void copy_uninit_204(char* dst, unsigned n, const void* arg, Alloc204& al);                  // 0x4c3d90 (rec_tables.cpp)
struct blk204;
char* uninit_copy_204(blk204* first, blk204* last, char* dst, Alloc204* al, ptr_tag tag);    // 0x4c1b60 (game/triggers/trigger_vec.cpp)

// @ 0x4c6eb0
MapRecord* MapDialog::insert_a(MapRecord* arr, int index, int z)
{
    ((void (__cdecl*)(char*, unsigned, const void*, Alloc204&, int))copy_uninit_204)((char*)arr, (unsigned)index, *(const void* volatile*)&z, *(Alloc204*)this, z);
    return (MapRecord*)((char*)arr + index * 0x204);
}

// @ 0x4c6ee0
int* MapDialog::insert_b(int* arr, int index, int z)
{
    ((void (__cdecl*)(char*, unsigned, const void*, Alloc4&, int))copy_uninit_4)((char*)arr, (unsigned)index, *(const void* volatile*)&z, *(Alloc4*)this, z);
    return &arr[index];
}

// @ 0x4c6f10
void MapDialog::place_a(MapRecord* arr, int index, int z)
{
    ((void* (__cdecl*)(char*, char*, char*, Alloc2c&, int))copy_uninit_2c)((char*)arr, (char*)index, (char*)z, *(Alloc2c*)this, z);
}

// @ 0x4c6f30
void MapDialog::place_b(MapRecord* arr, int index, int z)
{
    ((void* (__cdecl*)(char*, char*, char*, Alloc4&, int))copy_uninit_4_range)((char*)arr, (char*)index, (char*)z, *(Alloc4*)this, z);
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
    ((char* (__cdecl*)(blk204*, blk204*, char*, Alloc204*, int))uninit_copy_204)((blk204*)arr, (blk204*)index, (char*)z, (Alloc204*)this, z);
}

// @ 0x4c6f90
void MapDialog::refresh_list(char use_sel)
{
    HWND hWnd = GetDlgItem(hwnd, 0x465);
    if (!hWnd)
        return;
    SendMessageA(hWnd, 0x14b, 0, 0);                    // 0x184
    int count = list->get_count();
    for (int i = 0; i < count; ++i) {
        int kind;
        if (use_sel) {
            kind = g_dlg_display_bits;
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
                tstring sub = tmp.substr(p + 1, p + 3);
                kind = atoi(sub.buf ? sub.buf : (char*)&sub.buf);
            }
        }
        int data = 0x16 + (kind == 0x10);
        {
            tstring off("ui_off");
            tstring* r = ((StrTable&)g_strtable).lookup(off);
            if (SendMessageA(hWnd, 0x14c, 0, (LPARAM)(r->buf ? r->buf : (char*)&r->buf)) < 0) {   // LB_GETTEXT
                tstring off2("ui_off");
                tstring* r2 = ((StrTable&)g_strtable).lookup(off2);
                int idx = SendMessageA(hWnd, 0x14a, 0, (LPARAM)(r2->buf ? r2->buf : (char*)&r2->buf));  // 0x180
                SendMessageA(hWnd, 0x151, idx, 1);                                          // 0x19a
            }
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
    int cur = 0;
    if (use_sel) {
        if (g_dlg_multisample == 2)
            cur = 1;
        else if (g_dlg_multisample == 4)
            cur = 2;
    }
    SendMessageA(hWnd, 0x14e, cur, 0);                  // 0x186
}

// @ 0x4c7290
// (the original brackets the two SetDlgItemTextA groups with 18-byte WinLicense markers: jmp +0x10; "WL  ", 12 / 13, 0, "WL  ")
void ConfigDialog::init()
{
    RenderConfig cfg;
    ((Config*)&cfg)->init_defaults();            // RenderConfig is Config's 0x70-byte layout (first_time at +0)
    FILE* f = mb_open_user_file(g_rgl_name, g_mode_rb);
    if (f) {
        ((Config*)&cfg)->load_file(f);
        fclose(f);
    }
    if (cfg.first_time) {
        cfg.first_time = 0;
        ((OptionsDlg*)this)->choose_dx_settings((Config*)&cfg, true);   // ConfigDialog::init runs on the OptionsDlg object
        f = mb_open_user_file(g_rgl_name, g_mode_wb);
        ((rgl_config*)&cfg)->save(f);
        fclose(f);
    }
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
    sprintf(skin, ".\\Skins\\mb_skin_%s.smf", g_skin_name.c_str());
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(skin, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        FindClose(h);
        SetDlgItemTextA(hwnd, 0x410, g_empty);
        SetDlgItemTextA(hwnd, 0x41f, g_empty);
        SetDlgItemTextA(hwnd, 0x46b, g_empty);
        SetDlgItemTextA(hwnd, 0x405, g_empty);
        SetDlgItemTextA(hwnd, 0x406, g_empty);
        SetDlgItemTextA(hwnd, 0x469, g_empty);
        SetDlgItemTextA(hwnd, 0x425, g_empty);
    } else {
        {
            tstring key(g_ui_play);
            SetDlgItemTextA(hwnd, 0x410, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
        {
            tstring key(g_ui_configure);
            SetDlgItemTextA(hwnd, 0x405, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
        {
            tstring key(g_ui_check_for_new_version);
            SetDlgItemTextA(hwnd, 0x41f, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
        {
            tstring key(g_ui_download_new_settings);
            SetDlgItemTextA(hwnd, 0x46b, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
        {
            tstring key(g_ui_website);
            SetDlgItemTextA(hwnd, 0x406, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
        {
            tstring key(g_ui_cancel2);
            SetDlgItemTextA(hwnd, 0x469, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
        {
            tstring key(g_ui_current_setting);
            SetDlgItemTextA(hwnd, 0x425, ((StrTable&)g_strtable).lookup(key)->c_str());
        }
    }

    tstring file(g_module_path);
    file.append(tstring(g_bmp_main));
    HBITMAP logo = (HBITMAP)LoadImageA(0, file.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    *(HBITMAP*)((char*)this + 0x354) = logo;
    if (!logo)
        *(HBITMAP*)((char*)this + 0x354) = (HBITMAP)LoadImageA(0, g_bmp_main, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (g_trial_mode)
        ShowWindow(GetDlgItem(hwnd, 0x419), 5);
}

// @ 0x4c77c0
void MapEditorDialog::refresh()
{
    Frame* g = ((Frame*)g_game);
    MapRecord* r = 0;
    int var2_no = 0;
    char name[0xc8];
    float matrix[0x10];
    float x, y, z;
    int entry_no = 0;
    int var_no = 0;
    int mode = 0;

    if (g->object_index) {
        memcpy(matrix, (*(Pawn**)&g_mission->f65e4)->matrix, 0x28);
        matrix[12] += g->keep;
        mode = g->mode;
        x = g->eye.x;
        y = g->eye.y;
        z = g->eye.z;
        var_no = 0;
        strcpy(name, "New Object");
        entry_no = 0;
    } else if (g->record >= 0) {
        r = ((Map*)g_mission)->records + g->record;
        memcpy(matrix, &r->mesh_name, sizeof(matrix));
        entry_no = r->entry_no;
        var_no = r->var_no;
        ((ItemPage*)r)->format(name);
    }

    if (!(g->record < 0 && g->object_index == 0)) {
        SetDlgItemTextA(hwnd, 0x439, name);
        _snprintf(name, 0x80, "%3f", matrix[0]);
        SetDlgItemTextA(hwnd, 0x435, name);
        _snprintf(name, 0x80, "%3f", matrix[1]);
        SetDlgItemTextA(hwnd, 0x436, name);
        _snprintf(name, 0x80, "%3f", matrix[2]);
        SetDlgItemTextA(hwnd, 0x437, name);
        _snprintf(name, 0x80, "%3f", x);
        SetDlgItemTextA(hwnd, 0x48b, name);
        _snprintf(name, 0x80, "%3f", y);
        SetDlgItemTextA(hwnd, 0x48c, name);
        _snprintf(name, 0x80, "%3f", z);
        SetDlgItemTextA(hwnd, 0x48d, name);
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
    sprintf(name, "");
    SetDlgItemTextA(hwnd, 0x439, name);
    SetDlgItemTextA(hwnd, 0x435, name);
    SetDlgItemTextA(hwnd, 0x436, name);
    SetDlgItemTextA(hwnd, 0x437, name);
    SetDlgItemTextA(hwnd, 0x48b, name);
    SetDlgItemTextA(hwnd, 0x48c, name);
    SetDlgItemTextA(hwnd, 0x48d, name);
}

// @ 0x4c7bf0
void MapEditorDialog::update_object_type()
{
    if (!hwnd)
        return;
    HWND type = GetDlgItem(hwnd, 0x431);
    HWND list = GetDlgItem(hwnd, 0x433);
    int type_sel = SendMessageA(type, 0x147, 0, 0);
    int list_type = SendMessageA(type, WM_USER_LIST, type_sel, 0);
    SendMessageA(list, 0x184, 0, 0);
    if (!list_type) {
        int count = orig::g_propkind_count;
        for (int i = 0, off = 0; i < count; i++, off += 0x78) {
            char* e = (char*)(*(int*)&orig::g_propkinds + off);
            // the substring is a temporary of the add-string call (destroyed once, right after it: 0x4c7caa)
            int item = SendMessageA(list, 0x180, 0, (LPARAM)((tstring*)e)->substr(4, *(int*)(e + 0xc)).c_str());
            SendMessageA(list, 0x19a, item, i);
        }
    } else if (list_type == 2) {
        int count = orig::g_item_kind_count;
        for (int i = 0, off = 0; i < count; i++, off += 0x238) {
            char* rec = (char*)(*(int*)&orig::g_items + off);
            tstring* text = g_strtable.find((tstring*)(rec + 0x20));     // the translation of the key at +0x20,
            if (!text)
                text = (tstring*)rec;                                   // else the record's name at +0 (0x4c7cfa)
            int item = SendMessageA(list, 0x180, 0, (LPARAM)text->c_str());
            SendMessageA(list, 0x19a, item, i);
        }
    } else if (list_type == 4) {
        int count = orig::g_ptype_count;
        for (int i = 0, off = 0; i < count; i++, off += 0x20) {
            tstring* s = (tstring*)(*(int*)&orig::g_ptypes + off);          // the 0x20-byte record starts with its name (0x4c7d76 reads its buf at +4)
            int item = SendMessageA(list, 0x180, 0, (LPARAM)s->c_str());
            SendMessageA(list, 0x19a, item, i);
        }
    } else if (list_type == 5) {
        SendMessageA(list, 0x180, 0, (LPARAM)"Passage");
        SendMessageA(list, 0x19a, 0, 0);
    } else if (list_type == 1) {
        SendMessageA(list, 0x180, 0, (LPARAM)"Entry Point");
        SendMessageA(list, 0x19a, 0, 0);
    }
    SendMessageA(list, 0x186, 0, 0);
    // the selection's item data (LB_GETITEMDATA of LB_GETCURSEL, 0x4c7de2..0x4c7df7): the object index stored by 0x19a
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
    int i = 0;
    int off = 0;
    do {
        char* base = (char*)((Map*)g_mission)->records;
        if (!base)
            break;
        if ((unsigned)i >= (unsigned)(((Map*)g_mission)->records_end - ((Map*)g_mission)->records))
            break;
        char name[128];
        ((ItemPage*)(base + off))->format(name);
        int item = SendMessageA(list, 0x180, 0, (LPARAM)name);
        SendMessageA(list, 0x19a, item, i);
        i++;
        off += 0x1c8;
    } while ((char*)((Map*)g_mission)->records);
}

// @ 0x4c7ee0
void* __fastcall destroy_record_array(void* p, int, unsigned char flags)
{
    if (flags & 2) {
        int n = *((int*)p - 1);
        Rec78* cur = (Rec78*)p + n;
        while (--n >= 0) {
            --cur;
            rec78_tail(cur).~Rec78Tail();
            cur->name.~tstring();
        }
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
    for (int i = 0, off = 0;; i++, off += 0x18) {
        char* base = *(char**)((char*)this + 0x1c);
        if (!base)
            break;
        int count = (*(char**)((char*)this + 0x20) - base) / 0x18;
        if (i >= count)
            break;
        float* p = (float*)(*(char**)((char*)this + 0x1c) + off);
        if (p[5] != 0.0f) {
            p[0] += d[0];
            p[1] += d[1];
            p[2] += d[2];
        }
    }
}

// @ 0x4c8000
// Plane normal of the face that owns edge *e, scaled by the triangle's first edge (node b -> node a).
rgl::vector4 graph_obj::normal(const int* e)
{
    int b, a;
    find_triangle(e, &a, &b);
    node* v = &nodes[b];
    node* w = &nodes[a];
    float dx = v->x - w->x;
    float dy = v->y - w->y;
    float dz = v->z - w->z;
    float* m = (float*)&faces[edges[*e].f0];
    rgl::vector4 res;
    res.x = dy * m[8] - dz * m[7];
    res.y = dz * m[6] - dx * m[8];
    res.z = dx * m[7] - dy * m[6];
    res.normalize();
    return res;
}



// @ 0x4c80c0
void Block::clear()
{
    int* p = (int*)this;
    p[0] = 0;
    p[0xd] = -1;
    p[0xe] = 0;
    p[0xf] = -1;
    p[2] = 0xfffe7960;
    p[3] = -1;
    p[4] = 0xfffe7960;
    p[5] = -1;
    p[6] = 0;
    p[7] = 0;
    p[8] = 100;
    tidy3(&p[0x11]);
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
    int n = *(int*)((char*)this + 0x44);
    for (int i = 0; i < n; i++) {
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
void Block::free_materials()
{
    for (unsigned i = 0; i < mats().size(); i++) {
        mats()[i]->owned = 0;                           // the block no longer owns the material
        ReleaseFn rel;
        rel.clear_f14 = clear_f14;
        (g_res_manager.*rel.as_member)(mats()[i]);
    }
    tidy3((char*)this + 0x58);
    *(int*)((char*)this + 0x64) = 0;
}

// @ 0x4c83a0
void Block::read_terrain(stream_base* stream)
{
    tstring name;
    this->free_materials();
    stream->r_tstr((rgl::string*)((char*)this + 0x00));
    stream->r_int((int*)((char*)this + 0x14));
    stream->r_tstr((rgl::string*)&name);
    stream->r_int((int*)((char*)this + 0x18));
    stream->r_int((int*)((char*)this + 0x1c));
    stream->r_float((float*)((char*)this + 0x28));
    float* f2 = (float*)((char*)this + 0x2c);
    stream->r_float(f2);
    stream->r_float((float*)((char*)this + 0x30));
    stream->r_float((float*)((char*)this + 0x34));
    *(float*)((char*)this + 0x28) *= 0.001;
    *f2 *= 0.001;
    *(float*)((char*)this + 0x30) *= 0.001;
    *(float*)((char*)this + 0x34) *= 0.001;
    ((DialogBlock*)((char*)this + 0x38))->read(stream);   // 0x4607e0
    char buf[0x200];
    _snprintf(buf, 0x200, "Tableau material %s", ((tstring*)this)->c_str());
    {
        tstring mat(buf);
        ((tstring*)((char*)this + 0x44))->operator=(mat);
    }
    if (name.len > 1) {
        if (name.differs("none"))
            *(void**)((char*)this + 0x64) = ((resource_manager*)&g_res_manager)->get_material(*(const rgl::string*)&name);
    }
}



// @ 0x4c84f0
void __fastcall conv_map_codes(conv_rec* self)
{
    char* p = (char*)self;
    const char** names = g_conv_names;
    int* out = (int*)(p + 0x24);
    do {
        ConvItem16* begin = *(ConvItem16**)(p + 4);
        ConvItem16* end = *(ConvItem16**)(p + 8);
        const char* nm = *names;
        int found = -1;
        unsigned i = 0;
        do {
            if (begin == 0 || i >= (unsigned)((int)((char*)end - (char*)begin) / 0x68)) {
                log_cstr("WARNING: UNABLE TO MAP GAME TABLEAU CODE: ").w_cstr(nm).w_cstr(g_empty_b);
                break;
            }
            if (((ConvItem16*)((char*)begin + i * 0x68))->name.equals(nm))
                found = i;
            i++;
        } while (found == -1);
        *out = found;
        out++;
        names++;
    } while (names < g_conv_names + 3);
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
void __fastcall conv_build_scene(conv_rec* self)
{
    char* c = (char*)self;
    int gy = 0;
    if (*(int*)(c + 0x27c) >= 0) {
        tab_cell* q = (tab_cell*)(c + 0x288);
        do {
            int x = 0;
            if (*(int*)(c + 0x278) >= 0) {
                int y16 = gy * 16;
                tab_cell* qq = q;
                do {
                    int* cell = (*(world_root**)(c + 0xc3b0dc))->codes->cols[x];
                    *(int*)(y16 + (int)cell) = *(int*)&qq->v.z;
                    if (qq->vert >= 0)
                        (*(world_root**)(c + 0xc3b0dc))->sub->verts[qq->vert].pos = qq->v;
                    x++;
                    qq = (tab_cell*)((char*)qq + 0x911c);
                } while (x <= *(int*)(c + 0x278));
            }
            gy++;
            q = (tab_cell*)((char*)q + 0x94);
        } while (gy <= *(int*)(c + 0x27c));
    }
    world_sub* sub = (*(world_root**)(c + 0xc3b0dc))->sub;
    for (unsigned k = 0; k < sub->items.size(); k++) {
        scene_obj* r = sub->items[k].obj;
        if (r) {
            for (int j = 0; j < sub->items[k].n; j++)
                r->pts[j] = sub->verts[sub->items[k].idx[j]].pos;
            vector4 z0(0.0f, 0.0f, 0.0f);
            matrix3 t;
            matrix3* ip = (matrix3*)identity3((rgl::mat3*)&t);
            rgl::matrix m;
            *(matrix3*)&m = *ip;
            m.o = z0;
            r->set_frame(&m);
        }
    }
}

// @ 0x4c8810
edit_vert* edit_vec::ufill(edit_vert* dst, int n, const edit_vert* src)
{
    ((void (__cdecl*)(void*, size_t, const void*, block_copier*, const void*))uninit_fill_n)(dst, (unsigned)n, *(const void* volatile*)&src, (block_copier*)this, src);
    return dst + n;
}

// @ 0x4c8840
tstr_iter tstr_vec::erase_range(tstring* first, tstring* lastp)
{
    if (first != lastp) {
        tstring* end = std::_Copy_opt<tstring*, tstring*>(lastp, last, first, std::_Nonscalar_ptr_iterator_tag());
        for (tstring* it = end; it != last; it++)
            it->~tstring();
        last = end;
    }
    tstr_iter r;
    r.p = first;
    return r;
}

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
    ((item38* (__cdecl*)(const item38*, const item38*, item38*, editor*, item38*))copy_range38)(first, last, dst, (editor*)this, dst);
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
void __fastcall conv_group_dtor(conv_rec* self)
{
    char* p = (char*)self;
    ((RecGList*)(p + 0x228))->clear();   // 0x4317b0
    int i = 16;
    char* q = p + 0x1c8;
    do {
        q -= 0x18;
        ((tstring*)q)->~tstring();
    } while (--i != 0);
    if (*(void**)(p + 0x34) != 0)
        operator delete(*(void**)(p + 0x34));
    *(void**)(p + 0x34) = 0;
    *(void**)(p + 0x38) = 0;
    *(void**)(p + 0x3c) = 0;
    ((tstring*)(p + 0x20))->~tstring();
    ((tstring*)(p + 0x10))->~tstring();
    ((tstring*)p)->~tstring();
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
    int last = (int)nodes_n() - 1;
    if (idx != last) {
        nodes[idx] = nodes[last];
        swap_face_vertices(last, idx);
        swap_poly_colors(last, idx);
    }
    if (nodes) {
        if (nodes_n() != 0)
            nodes_end--;
    }
    remove_vertex_from_edges(last);
}

// @ 0x4c8ba0
void __fastcall conv_list_dtor(conv_rec* self)
{
    char* p = (char*)self;
    ((Block*)self)->free_materials();   // conv_rec is the mesh block (Block) here; 0x4c8330
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
void __fastcall conv_clear(void** self)
{
    g_presentation = 0;
    void** v = &self[1];
    if (v[0] != 0)
        operator delete(v[0]);
    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
}

// @ 0x4c8c20
void* __fastcall conv_zero_node(void* self)
{
    unsigned* s = (unsigned*)self;
    *(unsigned*)((char*)s + 0x7c) = 0;
    *(unsigned*)((char*)s + 0x80) = 0;
    *(unsigned*)((char*)s + 0x84) = 0;
    *(unsigned*)((char*)s + 0x88) = 0;
    for (int i = 0; i < 0x1e; i++)
        s[i] = 0;
    return self;
}


// The accessors below are shared by classes with the same field layout (identical code folded by the linker): 0x4c6080 is
// PartyTemplate's for DialogRec / GameString / ModRec / QuickString / SkinElem (name +0, key +0x10), 0x4c62b0 MenuDoor's
// for TroopRec / ItemKind, 0x4c6530 RecB's for Party, 0x4c6860 RecC's for FactionRec, 0x4c63f0 / 0x4c6330 / 0x4c6470 RecA's
// for QuestRec (and translated_plural of TroopRec / ItemKind). The calls go to the class that defines the function.
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
                ((ini_file&)g_strtable).write(f, chars[i].key, *find_or(chars[i].key, chars[i].name));
                tstring suffix("_pl");                                  // built before plural_name() runs, as in the original
                ((ini_file&)g_strtable).write(f, chars[i].key + suffix, *chars[i].plural_name());
            }
        }
        fclose(f);
    }

    if (parties.n_parties != 0) {
        FILE* f = fopen("new_language\\parties.csv", "wt+");            // 0x596034
        unsigned i = parties.first_party_id();
        while (i < parties.n_parties) {
            if (parties.get_party(i)->owner <= 0 && i != (unsigned)party_id_78) {
                if (translated) {
                    write_row(f, parties.get_party(i)->key, ((RecB*)parties.get_party(i))->translated());
                } else {
                    Party* p = parties.get_party(i);
                    const tstring* v;
                    if (g_show_party_ids_instead_of_names)
                        v = &p->key;
                    else if (p->flag)
                        v = &p->name;
                    else
                        v = find_or(p->key, p->name);
                    write_row(f, parties.get_party(i)->key, *v);
                }
            }
            parties.next_party_id(&i);
        }
        fclose(f);
    }

    if (n_factions > 0) {
        FILE* f = fopen("new_language\\factions.csv", "wt+");           // 0x596018
        for (int i = 0; i < n_factions; i++) {
            if (translated) {
                tstring v = ((RecC*)&factions[i])->translated();
                write_row(f, factions[i].key, v);
            } else {
                const tstring* v = &factions[i].name;
                if (!factions[i].flag)
                    v = find_or(factions[i].key, factions[i].name);
                write_row(f, factions[i].key, *v);
            }
        }
        fclose(f);
    }

    if (g_item_kind_count > 0) {
        FILE* f = fopen("new_language\\item_kinds.csv", "wt+");         // 0x595ffc
        for (int i = 0; i < g_item_kind_count; i++) {
            if (translated) {
                {
                    tstring v = ((MenuDoor*)&g_items[i])->translated(); // dies before the suffix is built
                    write_row(f, g_items[i].key, v);
                }
                tstring suffix("_pl");
                write_row(f, g_items[i].key + suffix, ((RecA*)&g_items[i])->translated_plural());
            } else {
                const tstring* v = find_or(g_items[i].key, g_items[i].name);
                write_row(f, g_items[i].key, *v);
                tstring suffix("_pl");                                  // built before plural_name() runs, as in the original
                write_row(f, g_items[i].key + suffix, *g_items[i].plural_name());
            }
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\item_modifiers.csv", "wt+");     // 0x595fdc
        for (ModRec* r = g_item_modifiers; r < g_item_modifiers + 43; r++) {
            if (translated) {
                tstring v = ((PartyTemplate*)r)->translated();
                write_row(f, r->key, v);
            } else {
                write_row(f, r->key, *find_or(r->key, r->name));
            }
        }
        fclose(f);
    }

    if (g_party_template_count > 0) {
        FILE* f = fopen("new_language\\party_templates.csv", "wt+");    // 0x595fb8
        for (int i = 0; i < g_party_template_count; i++) {
            if (translated) {
                tstring v = g_party_templates[i].translated();
                write_row(f, g_party_templates[i].key, v);
            } else {
                write_row(f, g_party_templates[i].key,
                          *find_or(g_party_templates[i].key, g_party_templates[i].name));
            }
        }
        fclose(f);
    }

    if (g_n_game_strings > 0) {
        FILE* f = fopen("new_language\\game_strings.csv", "wt+");       // 0x595f98
        for (int i = 0; i < g_n_game_strings; i++) {
            if (translated) {
                tstring v = ((PartyTemplate*)&g_strings[i])->translated();
                write_row(f, g_strings[i].key, v);
            } else {
                write_row(f, g_strings[i].key,
                          *find_or(g_strings[i].key, g_strings[i].name));
            }
        }
        fclose(f);
    }

    if (g_n_quick_strings > 0) {
        FILE* f = fopen("new_language\\quick_strings.csv", "wt+");      // 0x595f78
        for (int i = 0; i < g_n_quick_strings; i++) {
            if (translated) {
                tstring v = ((PartyTemplate*)&g_quick_strings[i])->translated();
                write_row(f, g_quick_strings[i].key, v);
            } else {
                write_row(f, g_quick_strings[i].key,
                          *find_or(g_quick_strings[i].key, g_quick_strings[i].name));
            }
        }
        fclose(f);
    }

    if (n_quests > 0) {
        FILE* f = fopen("new_language\\quests.csv", "wt+");             // 0x595f60
        for (int i = 0; i < n_quests; i++) {
            if (translated) {
                {
                    tstring v = ((RecA*)&quests[i])->translated();      // dies before the suffix is built
                    write_row(f, quests[i].key, v);
                }
                tstring suffix("_text");                                // 0x5923b4
                write_row(f, quests[i].key + suffix, ((RecA*)&quests[i])->translated_text());
            } else {
                write_row(f, quests[i].key, *find_or(quests[i].key, quests[i].name));
                tstring suffix("_text");                                // built before text_name() runs, as in the original
                write_row(f, quests[i].key + suffix, *quests[i].text_name());
            }
        }
        fclose(f);
    }

    if (g_n_dialogs > 0) {
        FILE* f = fopen("new_language\\dialogs.csv", "wt+");            // 0x595f44
        for (int i = 0; i < g_n_dialogs; i++) {
            for (int j = 0; j < i; j++) {
                if (g_dialogs[i].id.equals(g_dialogs[j].id))
                    goto next_dialog;
            }
            if (translated) {
                tstring v = ((PartyTemplate*)&g_dialogs[i])->translated();
                write_row(f, g_dialogs[i].id, v);
            } else {
                write_row(f, g_dialogs[i].id,
                          *find_or(g_dialogs[i].id, g_dialogs[i].text));
            }
        next_dialog:
            ;
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\skins.csv", "wt+");              // 0x595f2c
        for (unsigned i = 0; g_skins_begin && i < (unsigned)(g_skins_end - g_skins_begin); i++) {
            for (int j = 0; j < g_skins_begin[i].n_elems; j++) {
                if (translated) {
                    tstring v = ((PartyTemplate*)&g_skins_begin[i].elems[j])->translated();
                    write_row(f, g_skins_begin[i].elems[j].key, v);
                } else {
                    write_row(f, g_skins_begin[i].elems[j].key,
                              *find_or(g_skins_begin[i].elems[j].key, g_skins_begin[i].elems[j].name));
                }
            }
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\skills.csv", "wt+");             // 0x595f14
        for (SkillRec* r = g_skill_tbl; r < g_skill_tbl + 42; r++) {
            if (translated) {
                {
                    tstring v = r->translated();                        // dies before the suffix is built
                    write_row(f, r->key, v);
                }
                tstring suffix("_desc");                                // 0x595ca4
                write_row(f, r->key + suffix, r->translated_desc());
            } else {
                write_row(f, r->key, *find_or(r->key, r->name));
                tstring suffix("_desc");                                // built before desc_name() runs, as in the original
                write_row(f, r->key + suffix, *r->desc_name());
            }
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\game_menus.csv", "wt+");         // 0x595ef8
        for (int i = 0; i < g_menu_count; i++) {
            write_row(f, g_menus[i].key, *find_or(g_menus[i].key, g_menus[i].name));
            for (int j = 0; j < g_menus[i].n_doors; j++) {
                if (translated) {
                    tstring v = g_menus[i].doors[j].translated();
                    write_row(f, g_menus[i].doors[j].key, v);
                    tstring suffix("_door");                            // 0x595d64
                    tstring lookup = g_menus[i].doors[j].key + suffix;
                    const tstring* p = g_strtable.find(&lookup);
                    if (!p)
                        p = &g_menus[i].doors[j].text;
                    if (p->len > 2) {
                        tstring suffix2("_door");
                        tstring v2 = g_menus[i].doors[j].translated_door();
                        write_row(f, g_menus[i].doors[j].key + suffix2, v2);
                    }
                } else {
                    write_row(f, g_menus[i].doors[j].key,
                              *find_or(g_menus[i].doors[j].key, g_menus[i].doors[j].name));
                    const tstring* p = suffix_or(g_menus[i].doors[j].key, "_door",
                                                 g_menus[i].doors[j].text);
                    if (p->len > 2)
                        write_row(f, g_menus[i].doors[j].key + tstring("_door"),
                                  *suffix_or(g_menus[i].doors[j].key, "_door", g_menus[i].doors[j].text));
                }
            }
        }
        fclose(f);
    }
}

// @ 0x4ca550
void screen_text::refresh(int handle)
{
    if (dev1->is_fullscreen(0x10000))
        goto off;
    if (dev2) {
        if (dev2->is_fullscreen(0x10000))
            goto on;
        return;
    } else {
        if (!g_windowed_setting || g_windowed_other)
            return;
        goto off;   // 0x4ca573/0x4ca57d: windowed setting on and other off -> the 'off' tail at 0x4ca5c7
    }
on:
    if (flag)
        *flag = 1;
    if (text && win)
        text->assign("");
    g_windows.apply();
    return;
off:
    if (flag)
        *flag = 0;
    if (text && win)
        *text = *(tstring*)((char*)win + 0x128);
    g_windows.apply();
}

