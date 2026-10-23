// gui_edit.cpp: 0x4c48d0..0x4ca600 (TU map 102 "gui_manager_2" + 103 "item_page", merged: nothing in the range calls
// forward out of it and every class change inside it recurs on both sides, see MIGRATION.md; the end is proven by
// 0x4ca550 -> 0x4ca600). The editor's record loaders and head objects, the item records, the map editor dialogs,
// the mesh block reader, the layout/tableau group and the language-file exporter.
// Migrated from slices s004c46d0, s004c5080, s004c5a20, s004c6330, s004c6ca0, s004c7290, s004c7bf0, s004c84f0,
// s004c8ca0 and s004ca550 (0x4ca550).
#include "./gui_edit.h"

#pragma warning(disable:4234)     // __thiscall on a free function (thunk reconstruction)
#pragma warning(disable:4700)     // 0x4c77c0: the matrix is left unread when no record is selected

// @ 0x4c48d0
// Loader (ret 4): reads two tstrings and an int into locals, then the count at +4, rebuilds the 0x34-byte
// records and counts the ones whose 0x4be470 load succeeded; finishes the list.
void __fastcall rec34_list::load(stream_base& s)
{
    tstring a;
    tstring b;
    int dummy;
    s.r_tstr((rgl::string*)&a);
    s.r_tstr((rgl::string*)&b);
    s.r_int(&dummy);
    s.r_int(&count);
    if (v != 0) {
        free_vec_4318c0(v, 3);
        v = 0;
    }
    nok = 0;
    int n = count;
    rec34* r = (rec34*)engine_alloc(n * 0x34 + 4);
    if (r != 0) {
        *(int*)((char*)r - 4) = n;
        for (int i = 0; i < n; i++) {
            new (&r[i].name) tstring();
            new (&r[i].path) tstring();
            r[i].w14 = 0;
            r[i].w10 = 0;
            r[i].flag = 0;
        }
    } else
        r = 0;
    v = r;
    for (int i = 0; i < count; i++) {
        if (rec34_load(&r[i], &s))
            nok++;
    }
    rec34_list_finish(this);
    b.~tstring();
    a.~tstring();
}

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
    *(void**)self = (void*)0x5947d8;
    self->name.~tstring();
    if (flags & 1)
        engine_delete(self);
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
            engine_delete(p);
    }
    if (arr != 0)
        engine_delete(arr);
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
        engine_delete(first);
        first = nx;
    }
}

// @ 0x4c4dc0
// std::list<int>::_Buynode: allocates a 0x28-byte node and links it to itself on both sides.
void* __fastcall list_int_buynode(list_int* self)
{
    list_node_int* node = (list_node_int*)engine_new(0x28);
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
    engine_delete(node);
    __CxxThrowException(0, 0);
}

// @ 0x4c4e50
// std::list<int> node constructor allocator (ret 0xc): allocates a 0x28-byte node and initialises it through
// 0x4be9c0 with the three arguments of the caller.
void* __fastcall list_int_make_node(list_int* self, void*, void* a, void* b, void* c)
{
    void* node = engine_new(0x28);
    if (node != 0)
        list_node_init_4be9c0(node, 0, a, b, c);
    return node;
}

// @ 0x4c4ebd
// EH funclet of 0x4c4e50: frees the node under construction and rethrows.
void list_int_make_node_funclet(void* node)
{
    engine_delete(node);
    __CxxThrowException(0, 0);
}

// @ 0x4c4ee0
// Frees the owned block at +0x845e7bc, clears the three words there; the member destructors then destroy the
// string at +0x845e784 and the fifteen 0x28-byte string records from +0x248 back down to +0x18.
world_obj::~world_obj()
{
    if (blk != 0)
        engine_delete(blk);
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
                engine_free(e->p38);
            e->s44.~tstring();
            e->s28.~tstring();
            e->s10.~tstring();
            e->s0.~tstring();
        }
        if (flags & 1)
            engine_free(base);
        return base;
    }
    if (self->p58 != 0)
        ((RecI*)self->p58)->destroy(3);
    if (self->p38 != 0)
        engine_free(self->p38);
    self->s44.~tstring();
    self->s28.~tstring();
    self->s10.~tstring();
    self->s0.~tstring();
    if (flags & 1)
        engine_delete(self);
    return self;
}

// @ 0x4c5010
// Fills `n` 0x2c-byte records with the value `src` through 0x4c1b20 (the record copy); `cop` is the record
// that owns the copies. The loop is wrapped in the EH frame that guards the copy.
void __cdecl fill_records(void* first, size_t n, const void* src, block_copier* cop)
{
    try {
        while (n > 0) {
            cop->copy(first, src);
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
// copy-assign a range of 0x38-byte records (the original's auto-object range ctor; its EH pad is the
// next function). Arguments: dst, first, last; the 4th is the record ctor's context.
item38* copy_range38(item38* dst, const item38* first, const item38* last, editor* self)
{
    for (; first != last; first++) {
        copy_item38(dst, first);
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
        free_mem((void*)v38);
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
            free_mem(v[0]);
        v[0] = 0;
        v[1] = 0;
        v[2] = 0;
        ((tstring*)(p - 0x20))->~tstring();
    } while (--i);
}

// @ 0x4c5190
void gui_view::set_view(const float* v)
{
    float bx[2];
    float by[2];
    fx = v[0];
    fy = v[1];
    fs = 1.0f;
    if (fx > g_f95)
        fx = 0.95f;
    if (fx < g_f05)
        fx = 0.05f;
    if (!o568)
        return;
    mesh_bounds(bx, by);
    if (bx[0] + fx < g_f011) {
        fx = g_f011 - bx[0];
    } else if (bx[1] + fx > g_f989) {
        fx = g_f989 - bx[1];
    }
    if (by[0] + fy < g_f011) {
        fy = g_f011 - by[0];
    } else if (by[1] + fy > g_f739) {
        fy = g_f739 - by[1];
    }
    if (by[1] - by[0] > g_f728) {
        float s = g_f728 / (by[1] - by[0]);
        fs = s;
        fy = g_f011 - s * by[0];
    }
}

// Wheel step: forward the mask to the active child (the pointer at +4), else to the last item of the object's
// first vector; both are tail calls.
// @ 0x4c52f0
void gui_view::wheel_step(int mask)
{
    gui_child* a = *(gui_child**)((char*)this + 0x10);
    if (a) {
        a->on_event(mask);
    } else {
        gui_child** first = (gui_child**)((char*)this + 4);
        gui_child** last = (gui_child**)((char*)this + 8);
        if (first && last - first)
            first[last - first - 1]->on_event(mask);
    }
}

// @ 0x4c5330
void gui_view::clear_selection()
{
    gui_child** first = (gui_child**)((char*)this + 4);
    gui_child** last = (gui_child**)((char*)this + 8);
    gui_child* a = *(gui_child**)((char*)this + 0x10);
    f1708 = 1;
    if (a) {
        a->on_event(0x20000);
    } else if (first && last != first) {
        first[last - first - 1]->on_event(0x20000);
    }
    if (first && last - first) {
        gui_child* e = first[last - first - 1];
        if (e)
            e->d0();
        first[last - first - 1] = 0;
    }
    f1709 = 0;
    if (first && last - first)
        last -= 1;
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
int graph_obj::calc_vertex_mode(int mode)
{
    for (unsigned i = 0; nodes && i < nodes_n(); i++)
        nodes[i].flag = 0;
    if (mode == 0) {
        for (unsigned i = 0; nodes && i < nodes_n(); i++)
            if (nodes[i].used)
                nodes[i].flag = 1;
    } else if (mode == 1) {
        for (unsigned i = 0; edges && i < edges_n(); i++)
            if (edges[i].sel) {
                nodes[edges[i].a].flag = 1;
                nodes[edges[i].b].flag = 1;
            }
    } else {
        for (unsigned i = 0; faces && i < faces_n(); i++)
            if (faces[i].used)
                for (int k = 0; k < faces[i].count; k++)
                    nodes[faces[i].verts[k]].flag = 1;
    }
    return 0;
}

// @ 0x4c5670
int graph_obj::calc_edge_mode(int mode)
{
    for (unsigned i = 0; edges && i < edges_n(); i++)
        edges[i].flag = 0;
    if (mode == 0) {
        for (unsigned i = 0; edges && i < edges_n(); i++)
            if (nodes[edges[i].a].used || nodes[edges[i].b].used)
                edges[i].flag = 1;
    } else if (mode == 1) {
        for (unsigned i = 0; edges && i < edges_n(); i++)
            if (edges[i].sel)
                edges[i].flag = 1;
    } else {
        for (unsigned i = 0; faces && i < faces_n(); i++)
            if (faces[i].used)
                for (int k = 0; k < faces[i].count; k++)
                    edges[faces[i].ends[k]].flag = 1;
    }
    return 0;
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
    for (unsigned i = 0; faces && i < faces_n(); i++) {
        face* p = faces + i;
        for (int j = 0; j < p->count; j++) {
            if (p->verts[j] == a)
                p->verts[j] = b;
            else if (p->verts[j] == b)
                p->verts[j] = a;
        }
    }
}

// @ 0x4c58c0
void graph_obj::swap_poly_colors(int a, int b)
{
    for (unsigned i = 0; faces && i < faces_n(); i++) {
        face* p = faces + i;
        for (int j = 0; j < p->count; j++) {
            if (p->ends[j] == a)
                p->ends[j] = b;
            else if (p->ends[j] == b)
                p->ends[j] = a;
        }
    }
}

// @ 0x4c5940
void graph_obj::remove_vertex_from_polys(int v)
{
    for (unsigned i = 0; faces && i < faces_n(); i++) {
        face* p = faces + i;
        for (int k = 0; k < p->count; k++) {
            if (p->verts[k] == v) {
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
            face* p = &faces[i];
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
            face* p = &faces[i];
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
    int np = faces_n();
    for (int i = 0; i < np; i++)
        faces[i].used = 0;
    if (mode == 0) {
        for (int i = 0; i < np; i++) {
            face* p = &faces[i];
            for (int k = 0; k < p->count; k++)
                if (nodes[p->verts[k]].used)
                    p->used = 1;
        }
    } else {
        for (int i = 0; i < np; i++) {
            face* p = &faces[i];
            for (int k = 0; k < p->count; k++)
                if (edges[p->ends[k]].sel)
                    p->used = 1;
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
            face* p = &faces[i];
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
    face* f = &faces[edges[ei].f0];
    int idx = -1;
    for (int k = 0; k < f->count; k++) {
        if (ei == f->ends[k]) {
            idx = k;
            break;
        }
    }
    *a = f->verts[idx];
    face* g = &faces[edges[*e].f0];
    *b = g->verts[(idx + 1) % g->count];
}

// @ 0x4c6010
// Looks `key` up in the hash bucket of its hash (0x433bb0): the keys of the bucket are one 0x10-stride vector (+4),
// the translations the vector of the same bucket in the second table (+0xdf34).
tstring* LangTables::find(const tstring* key) throw()
{
    unsigned h = hash(key);
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
tstring RecA::text_by_kind()
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
tstring RecA::src_text()
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
tstring RecA::src_id_name()
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
tstring RecB::name_by_state()
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
    kf_arr_dtor((char*)this + 0x6c);
    if (*(void**)((char*)this + 0x44))
        free_mem(*(void**)((char*)this + 0x44));
    *(int*)((char*)this + 0x44) = 0;
    *(int*)((char*)this + 0x48) = 0;
    *(int*)((char*)this + 0x4c) = 0;
    name.~tstring();
}

// @ 0x4c6620
void ItemPage::format(char* out)
{
    if (kind == 1) {
        tstring key(s_entry_point);
        tstring* r = g_strtable.lookup(key);
        sprintf_c(out, 0x80, r->buf ? r->buf : (char*)&r->buf, text_ptr);
    } else if (kind == 5) {
        tstring key(s_passage);
        tstring* r = g_strtable.lookup(key);
        sprintf_c(out, 0x80, r->buf ? r->buf : (char*)&r->buf, text_ptr2);
    } else if (kind == 4) {
        if (index < 0 || index >= g_ground_spec_count) {
            sprintf_c(out, 0x80, s_invalid_plant);
            return;
        }
        tstring key(s_plant);
        GroundSpec& gs = g_ground_specs[index];
        tstring* r = g_strtable.lookup(key);
        sprintf_c(out, 0x80, r->buf ? r->buf : (char*)&r->buf, gs.name ? gs.name : (char*)&gs.name);
    } else if (kind == 2 || kind == 6) {
        if (index < 0 || index >= g_n_items) {
            sprintf_c(out, 0x80, s_invalid_item);
            return;
        }
        tstring key(s_item_name);
        tstring* v = g_items[index].display_name();
        tstring* r = g_strtable.lookup(key);
        sprintf_c(out, 0x80, r->buf ? r->buf : (char*)&r->buf, v->buf ? v->buf : (char*)&v->buf);
    } else if (kind == 0) {
        tstring key(s_prop);
        tstring* v = &field;
        tstring* r = g_strtable.lookup(key);
        sprintf_c(out, 0x80, r->buf ? r->buf : (char*)&r->buf, v->buf ? v->buf : (char*)&v->buf);
    } else {
        tstring key(s_unknown);
        tstring* v = &field;
        tstring* r = g_strtable.lookup(key);
        sprintf_c(out, 0x80, r->buf ? r->buf : (char*)&r->buf, v->buf ? v->buf : (char*)&v->buf);
    }
}
// @ 0x4c6860
tstring RecC::field_name()
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
void ItemPage::retire()
{
    if (!records)
        return;
    int n = ((int*)records)[-1];
    RefItem* it = (RefItem*)((char*)records + n * 0x24);
    while (n-- >= 0) {
        it = (RefItem*)((char*)it - 0x24);
        kf_arr_dtor((void*)&it->kf);
        it->name.~tstring();
    }
    engine_free((char*)records - 4);
    records = 0;
}

// @ 0x4c6950
RefRecord* ItemPage::fill_records(RefRecord* rec)
{
    tstring a, b;
    rec->v54(&a);
    rec->v54(&b);
    rec->v4c(&field);
    rec->v4c((void*)((char*)this + 0x1a0));
    retire();
    records = 0;
    int n = rec_count;
    RefItem* p = (RefItem*)engine_alloc(n * 0x24 + 4);
    if (p) {
        p = (RefItem*)((char*)p + 4);
        ((int*)p)[-1] = n;
        RefItem* it = p;
        for (int i = 0; i < n; i++, it = (RefItem*)((char*)it + 0x24)) {
            tstring t;
            it->name = t;
            it->refs = 0;
            it->kf = 0;
            it->f18 = 0;
        }
    }
    records = p;
    for (int i = 0; i < n; i++)
        ref_item_ctor((char*)records + i * 0x24, i, rec, i);
    return rec;
}

// @ 0x4c6aa0
void ItemPage::clear()
{
    if (*(void**)((char*)this + 0x4)) {
        engine_free(*(void**)((char*)this + 0x4));
        *(void**)((char*)this + 0x4) = 0;
    }
    Pad98* p = *(Pad98**)((char*)this + 0xc);
    if (p) {
        int n = ((int*)p)[-1];
        Pad98* it = (Pad98*)((char*)p + n * 0x98);
        for (int i = n; i >= 0; i--) {
            it = (Pad98*)((char*)it - 0x98);
            it->name.~tstring();
        }
        engine_free((void*)((char*)p - 4));
        *(void**)((char*)this + 0xc) = 0;
    }
    if (extra)
        free_mem(extra);
    extra = 0;
    extra_end = 0;
}

// @ 0x4c6b30
void ItemPage::place(int e)
{
    void* hdr = *(void**)((char*)records + 0x38);
    char* frames = *(char**)((char*)hdr + 0x38);
    char* v74 = *(char**)frames;
    unsigned n = *(unsigned*)(v74 + 0x74);
    if (n && 0u < (unsigned)(*(int*)(v74 + 0x78) - (int)n) >> 5) {
        for (unsigned i = 0; i < (unsigned)(*(int*)(v74 + 0x78) - (int)n) >> 5; i++) {
            float* dst = (float*)((*(char**)((char*)records + 0x40) + e * 0x34 + 0x30) + i * 0x10);
            dst[0] = 0.0f;
            dst[1] = 0.0f;
            dst[2] = 0.0f;
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
void MapScreen::set_object_target(int n)
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
void MapScreen::reset_state()
{
    g_gui.reset();
    f2c = 0;
}

// @ 0x4c6e30
void MapScreen::clear_objects()
{
    for (ListObj** p = list_start; p < list_end; ++p) {
        delete *p;
        *p = 0;
    }
    if (list_start)
        engine_delete(list_start);
    list_start = 0;
    list_end = 0;
    list_cap = 0;
    if (raw)
        engine_delete(raw);
    raw = 0;
    raw_end = 0;
    raw_cap = 0;
}

// @ 0x4c6eb0
MapRecord* MapScreen::insert_a(MapRecord* arr, int index, int z)
{
    ctor_4c3d90(arr, index, z, this, z);
    return (MapRecord*)((char*)arr + index * 0x204);
}

// @ 0x4c6ee0
int* MapScreen::insert_b(int* arr, int index, int z)
{
    ctor_4c3e10((MapRecord*)arr, index, z, this, z);
    return &arr[index];
}

// @ 0x4c6f10
void MapScreen::place_a(MapRecord* arr, int index, int z)
{
    ctor_4c3e80(arr, index, z, this, z);
}

// @ 0x4c6f30
void MapScreen::place_b(MapRecord* arr, int index, int z)
{
    ctor_4c3ef0(arr, index, z, this, z);
}

// @ 0x4c6f50
void MapScreen::place_c(MapRecord* arr, int index, int z)
{
    ctor_524cd0(arr, index, z, this, z);
}

// @ 0x4c6f70
void MapScreen::place_d(MapRecord* arr, int index, int z)
{
    ctor_4c1b60(arr, index, z, this, z);
}

// @ 0x4c6f90
void MapScreen::refresh_list(char use_sel)
{
    HWND hWnd = GetDlgItem(hwnd, 0x465);
    if (!hWnd)
        return;
    SendMessageA(hWnd, 0x14b, 0, 0);                    // 0x184
    int count = list->get_count(list);
    for (int i = 0; i < count; ++i) {
        int kind;
        if (use_sel) {
            kind = g_dlg_sel;
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
            tstring* r = g_strtable.lookup(off);
            if (SendMessageA(hWnd, 0x14c, 0, (LPARAM)(r->buf ? r->buf : (char*)&r->buf)) < 0) {   // LB_GETTEXT
                tstring off2("ui_off");
                tstring* r2 = g_strtable.lookup(off2);
                int idx = SendMessageA(hWnd, 0x14a, 0, (LPARAM)(r2->buf ? r2->buf : (char*)&r2->buf));  // 0x180
                SendMessageA(hWnd, 0x151, idx, 1);                                          // 0x19a
            }
        }
        int flag;
        if (list->get_sel(list, i, 1, data, 0, 2, &flag) >= 0) {
            if (flag > 0) {
                if (SendMessageA(hWnd, 0x14c, 0, (LPARAM)"2x") < 0) {
                    int idx = SendMessageA(hWnd, 0x14a, 1, (LPARAM)"2x");
                    SendMessageA(hWnd, 0x151, idx, 2);
                }
            }
        }
        if (list->get_sel(list, i, 1, data, 0, 4, &flag) >= 0) {
            if (flag > 0) {
                if (SendMessageA(hWnd, 0x14c, 0, (LPARAM)"4x") < 0) {
                    int idx = SendMessageA(hWnd, 0x14a, 2, (LPARAM)"4x");
                    SendMessageA(hWnd, 0x151, idx, 4);
                }
            }
        }
    }
    int cur = 0;
    if (use_sel) {
        if (g_map_state == 2)
            cur = 1;
        else if (g_map_state == 4)
            cur = 2;
    }
    SendMessageA(hWnd, 0x14e, cur, 0);                  // 0x186
}

// @ 0x4c7290
void ConfigDialog::init()
{
    Settings s;
    FILE* f = mb_open_user_file(g_rgl_name, "rb");
    if (f) {
        s.cfg.read(f);
        fclose(f);
    }
    if (*(int*)&s.cfg != 0) {
        *(int*)&s.cfg = 0;
        s.cfg.apply(1, NULL);
        f = mb_open_user_file(g_rgl_name, "wb");
        s.cfg.write(f);
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

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(g_skin_path, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        SetDlgItemTextA(hwnd, 0x410, text(g_ui_play));
        SetDlgItemTextA(hwnd, 0x405, text(g_ui_configure));
        SetDlgItemTextA(hwnd, 0x41f, text(g_ui_check_for_new_version));
        SetDlgItemTextA(hwnd, 0x46b, text(g_ui_download_new_settings));
        SetDlgItemTextA(hwnd, 0x406, text(g_ui_website));
        SetDlgItemTextA(hwnd, 0x469, text(g_ui_cancel2));
        SetDlgItemTextA(hwnd, 0x425, text(g_ui_current_setting));
    } else {
        FindClose(h);
        SetDlgItemTextA(hwnd, 0x410, g_empty);
        SetDlgItemTextA(hwnd, 0x41f, g_empty);
        SetDlgItemTextA(hwnd, 0x46b, g_empty);
        SetDlgItemTextA(hwnd, 0x405, g_empty);
        SetDlgItemTextA(hwnd, 0x406, g_empty);
        SetDlgItemTextA(hwnd, 0x469, g_empty);
        SetDlgItemTextA(hwnd, 0x425, g_empty);
    }

    tstring file(&g_path_prefix);
    file.append(tstring(g_bmp_main));
    HANDLE logo = LoadImageA(0, file.buf ? file.buf : (char*)&file, 0, 0, 0, LR_LOADFROMFILE);
    if (!logo)
        logo = LoadImageA(0, g_bmp_main, 0, 0, 0, LR_LOADFROMFILE);
    if (g_show_download)
        ShowWindow(GetDlgItem(hwnd, 0x419), 5);
}

// @ 0x4c77c0
void MapEditorDialog::refresh()
{
    Frame* g = g_frame;
    MapRecord* r = 0;
    int var2_no = 0;
    char name[0xc8];
    float matrix[0x10];
    float x, y, z;
    int entry_no = 0;
    int var_no = 0;
    int mode = 0;

    if (g->object_index) {
        memcpy(matrix, g_edited_object->matrix, 0x28);
        matrix[12] += g->keep;
        mode = g->mode;
        x = g->eye.x;
        y = g->eye.y;
        z = g->eye.z;
        var_no = 0;
        strcpy(name, "New Object");
        entry_no = 0;
    } else if (g->record >= 0) {
        r = g_map->records + g->record;
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
        int count = *(int*)0x5e29e0;
        for (int i = 0, off = 0; i < count; i++, off += 0x78) {
            char* e = (char*)(*(int*)0x5e29dc + off);
            tstring sub = ((tstring*)e)->substr(4, *(int*)(e + 0xc));
            int item = SendMessageA(list, 0x180, 0, (LPARAM)sub.c_str());
            sub.~tstring();
            SendMessageA(list, 0x19a, item, i);
        }
    } else if (list_type == 2) {
        int count = *(int*)0x5e29d4;
        for (int i = 0, off = 0; i < count; i++, off += 0x238) {
            tstring* key = (tstring*)(*(int*)0x5e29d0 + off + 0x20);
            tstring* text = g_strtable.find(key);
            if (text)
                key = text;
            int item = SendMessageA(list, 0x180, 0, (LPARAM)key->c_str());
            SendMessageA(list, 0x19a, item, i);
        }
    } else if (list_type == 4) {
        int count = *(int*)0x5e29e8;
        for (int i = 0, off = 0; i < count; i++, off += 0x20) {
            tstring* s = (tstring*)(*(int*)0x5e28f8 + off + 4);
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
    g_frame->sel = SendMessageA(list, 0x188, 0, 0);
    g_frame->mode = list_type;
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
        char* base = (char*)g_map->records;
        if (!base)
            break;
        if ((unsigned)i >= (unsigned)(g_map->records_end - g_map->records))
            break;
        char name[128];
        ((ItemPage*)(base + off))->format(name);
        int item = SendMessageA(list, 0x180, 0, (LPARAM)name);
        SendMessageA(list, 0x19a, item, i);
        i++;
        off += 0x1c8;
    } while ((char*)g_map->records);
}

// @ 0x4c7ee0
void* __fastcall destroy_record_array(void* p, int, unsigned int flags)
{
    int* head;
    int n;
    if (!(flags & 2)) {
        record_free(p);
        if (flags & 1)
            engine_delete(p);
        return p;
    }
    n = ((RecordArrayHead*)p)[-1].count;
    head = (int*)p - 1;
    char* e = (char*)head + 0x48 + n * 0x78;            // &elem[n-1] + 0x48
    for (; n > 0; n--) {
        if (*(void**)(e - 0x30)) {                      // owner at elem+0x18
            void** vt = *(void***)(e - 0x30);
            ((void (__cdecl*)(void*, int))vt[0])(*(void**)(e - 0x30), 1);
        }
        record_array_clear(e - 0x20);                   // sub-object at elem+0x28
        if (*(void**)(e)) {
            engine_delete(*(void**)(e));
            *(void**)(e) = 0;
        }
        *(void**)(e + 4) = 0;
        *(void**)(e + 8) = 0;
        ((tstring*)(e - 0x44))->~tstring();             // elem's name
        e -= 0x78;
    }
    if (flags & 1)
        engine_free(head);
    return head;
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
    int a, b;
    find_triangle(e, &a, &b);
    node* v = &nodes[b];
    node* w = &nodes[a];
    float dx = w->x - v->x;
    float dy = w->y - v->y;
    float dz = w->z - v->z;
    float* m = (float*)&faces[edges[*e].f0];
    rgl::vector4 res;
    res.x = dy * m[8] - dz * m[7];
    res.y = dz * m[6] - dx * m[8];
    res.z = dx * m[7] - dy * m[6];
    res.normalize();
    res.w = m[9];                                       // the caller's 4th lane lands here
    return res;
}



// @ 0x4c80c0
void Block::clear()
{
    tstring* var = (tstring*)*(int*)((char*)this + 0x44);
    int z = 0, k = 0xfffe7960;
    this->first = z;
    *(int*)((char*)this + 0x34) = -1;
    *(int*)((char*)this + 0x38) = z;
    *(int*)((char*)this + 0x3c) = -1;
    *(int*)((char*)this + 0x08) = k;
    *(int*)((char*)this + 0x0c) = -1;
    *(int*)((char*)this + 0x10) = k;
    *(int*)((char*)this + 0x14) = -1;
    *(int*)((char*)this + 0x18) = z;
    *(int*)((char*)this + 0x1c) = z;
    *(int*)((char*)this + 0x20) = 100;
    if (var)
        engine_delete(var);
    *(int*)((char*)this + 0x44) = 0;
    *(int*)((char*)this + 0x48) = 0;
    *(int*)((char*)this + 0x4c) = 0;
}



// @ 0x4c8110
void Block::read(stream_base* stream)
{
    if (*(char**)((char*)this + 0x34))
        engine_delete(*(char**)((char*)this + 0x34));
    *(int*)((char*)this + 0x34) = 0;
    *(int*)((char*)this + 0x38) = 0;
    *(int*)((char*)this + 0x3c) = 0;
    stream->r_tstr((rgl::string*)((char*)this + 0x00));
    stream->r_tstr((rgl::string*)((char*)this + 0x10));
    stream->r_tstr((rgl::string*)((char*)this + 0x20));
    ((tstring*)((char*)this + 0x00))->underscores_to_spaces();
    ((tstring*)((char*)this + 0x10))->underscores_to_spaces();
    stream->r_int((int*)((char*)this + 0x44));
    if (*(int*)((char*)this + 0x44) > 0x10)
        mb_error("Error too many variation on mesh %s", ((tstring*)((char*)this + 0x00))->c_str());
    int n = *(int*)((char*)this + 0x44);
    for (int i = 0; i < n; i++) {
        char* v = (char*)this + 0x58 + i * 0x18;
        stream->r_tstr((rgl::string*)(v - 0x10));
        stream->r_float((float*)v);
    }
    stream->r_float((float*)((char*)this + 0x1c8));
    stream->r_float((float*)((char*)this + 0x1d0));
    stream->r_int((int*)((char*)this + 0x1d8));
    stream->r_float((float*)((char*)this + 0x1e0));
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
    if (*(int*)((char*)this + 0x20) > 1) {
        char buf[0x200];
        _snprintf(buf, 0x200, "Item %s", ((tstring*)((char*)this + 0x20))->c_str());
        add_frame(stream, buf, 0);
    }
    *(int*)((char*)this + 0x40) = 0x3f800000;
    *(int*)((char*)this + 0x21c) = 0x3f800000;
}



// @ 0x4c8330
void Block::free_materials()
{
    for (unsigned i = 0;; i++) {
        char* b = *(char**)((char*)this + 0x58);
        if (!b)
            break;
        unsigned n = (unsigned)(*(char**)((char*)this + 0x5c) - b) >> 2;
        if (i >= n)
            break;
        char* e = *(char**)(*(char**)((char*)this + 0x58) + i * 4);
        *(int*)(e + 0x64) = 0;                          // the block no longer owns the material
        g_res_manager.release(e);
    }
    if (*(char**)((char*)this + 0x58))
        engine_delete(*(char**)((char*)this + 0x58));
    *(int*)((char*)this + 0x58) = 0;
    *(int*)((char*)this + 0x5c) = 0;
    *(int*)((char*)this + 0x60) = 0;
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
    *(float*)((char*)this + 0x28) *= 0.001f;
    *f2 *= 0.001f;
    *(float*)((char*)this + 0x30) *= 0.001f;
    *(float*)((char*)this + 0x34) *= 0.001f;
    block_read_base(this, stream);
    char buf[0x200];
    _snprintf(buf, 0x200, "Tableau material %s", ((tstring*)((char*)this + 0x44))->c_str());
    tstring mat(buf);
    ((tstring*)((char*)this + 0x44))->operator=(mat);
    mat.~tstring();
    if (name.len > 1) {
        if (name.differs("none"))
            *(void**)((char*)this + 0x64) = g_res_manager.get_material(name);
    }
    name.~tstring();
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
        engine_delete((void*)p->a4);
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
        grid_slot* slot = (grid_slot*)(c + 0x29c);
        do {
            int x = 0;
            if (*(int*)(c + 0x278) >= 0) {
                int y16 = gy * 16;
                do {
                    int** codes = (*(world_root**)(c + 0xc3b0dc))->codes;
                    int* cell = codes[x];
                    *(int*)(y16 + (int)cell) = slot->f0;
                    if (slot->f14 >= 0) {
                        scene_vert* verts = (*(world_root**)(c + 0xc3b0dc))->sub->verts;
                        scene_vert* v = &verts[slot->f14 * 9];
                        v->pos = *(vector4*)((char*)slot + 4);
                    }
                    x++;
                    slot = (grid_slot*)((char*)slot + 0x911c);
                } while (x <= *(int*)(c + 0x278));
            }
            gy++;
            slot = (grid_slot*)((char*)slot + 0x94);
        } while (gy <= *(int*)(c + 0x27c));
    }
    scene_sub* sub = (*(world_root**)(c + 0xc3b0dc))->sub;
    for (unsigned k = 0;
         sub->first != 0 && k < (unsigned)((int)((char*)sub->last - (char*)sub->first) / 0x88);
         k++) {
        var_rec* it = (var_rec*)((char*)sub->first + k * 0x88);
        var_rec* r = *(var_rec**)&it->verts;
        if (r != 0) {
            int j = 0;
            if (r->n > 0) {
                scene_vert* dst = r->verts;
                do {
                    int t = it->idx[j];
                    dst->pos = sub->verts[t].pos;
                    j++;
                    dst++;
                } while (j < r->n);
            }
            matrix3 loc;
            loc.m[0][0] = 0;
            loc.m[0][1] = 0;
            loc.m[0][2] = 0;
            matrix3 m = *identity3(&loc);
            ((void (__fastcall*)(void*, int, matrix3*))*(void**)r)(r, 0, &m);
        }
    }
}

// @ 0x4c8810
edit_vert* fill_verts(edit_vert* dst, int n, const edit_vert* src)
{
    uninit_fill_n(dst, (unsigned)n, src, 0, src);
    return dst + n;
}

// @ 0x4c8840
void __fastcall conv_assign_back(conv_rec* self, int, conv_rec** out, conv_rec* first, conv_rec* last)
{
    if (first != last) {
        tstring* end = assign_range((tstring*)first, (tstring*)last, *(tstring**)((char*)self + 8), *(tstring**)((char*)self + 8));
        tstring* it = *(tstring**)((char*)self + 8);
        while (end != it) {
            ((tstring*)it)->~tstring();
            it = (tstring*)((char*)it + 0x10);
        }
        *(tstring**)((char*)self + 8) = end;
    }
    *out = first;
}

// @ 0x4c88a0
ptr_iter ptr_vec::erase_range(int* first, int* last)
{
    if (first != last) {
        int* end = (int*)this->end;
        int* d = first;
        while (last != end) {
            *d = *last;
            last++;
            d++;
        }
        this->end = d;
    }
    ptr_iter r;
    r.p = first;
    return r;
}

// @ 0x4c88e0
void item38_vec::copy_range(item38* dst, const item38* first, const item38* last)
{
    copy_range38(dst, first, last, (conv_rec*)this, last);
}

// @ 0x4c8900
void __stdcall loader_thread_body()
{
    while (g_loading) {
        WaitForSingleObject(g_load_event, INFINITE);
        EnterCriticalSection(&g_load_cs);
        if (g_load_dt > 0.0f) {
            g_phys.update(g_load_dt);
            g_load_dt = 0.0f;
        }
        LeaveCriticalSection(&g_load_cs);
    }
}

// @ 0x4c8980
void* __fastcall conv_item_array_dtor(conv_rec* self, int, unsigned flag)
{
    void* p = self;
    if (!(flag & 2)) {
        conv_free_one(self);
        if (flag & 1)
            engine_delete(p);
        return p;
    }
    int n = *(int*)((char*)p - 4);
    for (int i = n - 1; i >= 0; i--) {
        ConvItem16* e = (ConvItem16*)((char*)p + i * 0xb0);
        if (e->a4 != 0)
            engine_delete((void*)e->a4);
        e->a4 = 0;
        e->a8 = 0;
        e->ac = 0;
        ((tstring*)((char*)e + 0x10))->~tstring();
        ((tstring*)e)->~tstring();
    }
    if (flag & 1)
        engine_free((char*)p - 4);
    return (char*)p - 4;
}

// @ 0x4c8a40
void __fastcall conv_group_dtor(conv_rec* self)
{
    char* p = (char*)self;
    self->clear_groups();
    int i = 16;
    char* q = p + 0x1c8;
    do {
        q -= 0x18;
        ((tstring*)q)->~tstring();
    } while (--i != 0);
    if (*(void**)(p + 0x34) != 0)
        engine_delete(*(void**)(p + 0x34));
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
        engine_delete(p[8]);
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
    self->reset_groups();
    void** v = (void**)(p + 0x58);
    if (v[0] != 0)
        engine_delete(v[0]);
    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
    if (*(void**)(p + 0x38) != 0)
        engine_free(*(void**)(p + 0x38));
    ((tstring*)(p + 0x44))->~tstring();
    ((tstring*)p)->~tstring();
}

// @ 0x4c8bf0
void __fastcall conv_clear(void** self)
{
    g_conv_scratch = 0;
    void** v = &self[1];
    if (v[0] != 0)
        engine_delete(v[0]);
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
                g_strtable.write(f, chars[i].key, chars[i].translated());
                tstring suffix("_pl");                                  // 0x5923b0
                g_strtable.write(f, chars[i].key + suffix, chars[i].translated_plural());
            } else {
                g_strtable.write(f, chars[i].key, *chars[i].display_name());
                g_strtable.write(f, chars[i].key + tstring("_pl"),
                             *chars[i].plural_name());
            }
        }
        fclose(f);
    }

    if (pm.n_parties != 0) {
        FILE* f = fopen("new_language\\parties.csv", "wt+");            // 0x596034
        unsigned i = pm.first_party_id();
        while (i < pm.n_parties) {
            PartyRec* p = pm.get_party(i);
            if (p->f8c <= 0 && i != (unsigned)party_id_78) {
                if (translated) {
                    tstring v = pm.get_party(i)->translated();
                    write_row(f, pm.get_party(i)->key, v);
                } else {
                    const tstring* v;
                    if (g_lang_on)
                        v = &p->key;
                    else if (p->flag)
                        v = &p->name;
                    else
                        v = find_or(p->key, p->name);
                    write_row(f, pm.get_party(i)->key, *v);
                }
            }
            pm.next_party_id(&i);
        }
        fclose(f);
    }

    if (n_factions > 0) {
        FILE* f = fopen("new_language\\factions.csv", "wt+");           // 0x596018
        for (int i = 0; i < n_factions; i++) {
            if (translated) {
                tstring v = factions[i].translated();
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

    if (g_n_items > 0) {
        FILE* f = fopen("new_language\\item_kinds.csv", "wt+");         // 0x595ffc
        for (int i = 0; i < g_n_items; i++) {
            if (translated) {
                tstring v = g_items[i].translated();
                write_row(f, g_items[i].key, v);
                tstring suffix("_pl");
                write_row(f, g_items[i].key + suffix, g_items[i].translated_plural());
            } else {
                const tstring* v = find_or(g_items[i].key, g_items[i].name);
                write_row(f, g_items[i].key, *v);
                write_row(f, g_items[i].key + tstring("_pl"),
                          *g_items[i].plural_name());
            }
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\item_modifiers.csv", "wt+");     // 0x595fdc
        for (ModRec* r = g_item_mods; r < g_item_mods + 43; r++) {
            if (translated) {
                tstring v = r->translated();
                write_row(f, r->key, v);
            } else {
                write_row(f, r->key, *find_or(r->key, r->name));
            }
        }
        fclose(f);
    }

    if (g_n_party_templates > 0) {
        FILE* f = fopen("new_language\\party_templates.csv", "wt+");    // 0x595fb8
        for (int i = 0; i < g_n_party_templates; i++) {
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
                tstring v = g_game_strings[i].translated();
                write_row(f, g_game_strings[i].key, v);
            } else {
                write_row(f, g_game_strings[i].key,
                          *find_or(g_game_strings[i].key, g_game_strings[i].name));
            }
        }
        fclose(f);
    }

    if (g_n_quick_strings > 0) {
        FILE* f = fopen("new_language\\quick_strings.csv", "wt+");      // 0x595f78
        for (int i = 0; i < g_n_quick_strings; i++) {
            if (translated) {
                tstring v = g_quick_strings[i].translated();
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
                tstring v = quests[i].translated();
                write_row(f, quests[i].key, v);
                tstring suffix("_text");                                // 0x5923b4
                write_row(f, quests[i].key + suffix, quests[i].translated_text());
            } else {
                write_row(f, quests[i].key, *find_or(quests[i].key, quests[i].name));
                write_row(f, quests[i].key + tstring("_text"),
                          *quests[i].text_name());
            }
        }
        fclose(f);
    }

    if (g_n_dialogs > 0) {
        FILE* f = fopen("new_language\\dialogs.csv", "wt+");            // 0x595f44
        for (int i = 0; i < g_n_dialogs; i++) {
            for (int j = 0; j < i; j++) {
                if (g_dialogs[i].key.equals(g_dialogs[j].key))
                    goto next_dialog;
            }
            if (translated) {
                tstring v = g_dialogs[i].translated();
                write_row(f, g_dialogs[i].key, v);
            } else {
                write_row(f, g_dialogs[i].key,
                          *find_or(g_dialogs[i].key, g_dialogs[i].name));
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
                    tstring v = g_skins_begin[i].elems[j].translated();
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
        for (SkillRec* r = g_skills; r < g_skills + 42; r++) {
            if (translated) {
                tstring v = r->translated();
                write_row(f, r->key, v);
                tstring suffix("_desc");                                // 0x595ca4
                write_row(f, r->key + suffix, r->translated_desc());
            } else {
                write_row(f, r->key, *find_or(r->key, r->name));
                write_row(f, r->key + tstring("_desc"),
                          *r->desc_name());
            }
        }
        fclose(f);
    }

    {
        FILE* f = fopen("new_language\\game_menus.csv", "wt+");         // 0x595ef8
        for (int i = 0; i < g_n_menus; i++) {
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
                        write_row(f, g_menus[i].doors[j].key + tstring("_door"), *p);
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
    }
on:
    if (g->flag)
        *g->flag = 1;
    if (text && g->text)
        g->text->assign("");
    g_screen.apply();
    return;
off:
    if (g->flag)
        *g->flag = 0;
    if (text && g->text)
        *text = *(tstring*)((char*)win + 0x128);
    g_screen.apply();
}

