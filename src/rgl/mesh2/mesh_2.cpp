// mesh_2.cpp: 0x4ca600..0x4ce720 (TU map 104 "mesh_2"; the start is proven by the forward call 0x4ca550 -> 0x4ca600).
// The screen-mode queue, the graph editor's edge/face operations, the big editor record releasers, the std::vector
// instantiations the linker placed in this object, the skeleton fix-up, the language tables' constructor and loader, the
// csv row and the scene object readers. Migrated from slices s004ca550 (0x4ca600..0x4ca9b0), s004caa40, s004cb3a0,
// s004cbcd0, s004cc680, s004cd010, s004cd850 and s004cdfd0 (up to 0x4ce680; 0x4ce720 onward is the next object).
#include "./mesh_2.h"

#pragma warning(disable:4234)     // __thiscall on a free function (thunk reconstruction)

const char g_none[] = "none";                                   // 0x54ac00
static const float HALF = 0.5f;                                 // 0x549d70

// @ 0x4ca600
void screen_cfg::apply()
{
    g_gui.set_o568(0, true);
    items[req.back()]->restore();
    req.pop_back();
    if (req.count) {
        unsigned char v = items[req.back()]->index();
        if (g_dev_renderer)
            ((dev_index*)g_dev_renderer)->set_index(v);
        g_5d75e0 = v;
    }
}

// @ 0x4ca6c0
void dev_list::select_windowed(int a)
{
    int found = -1;
    for (int i = 0; i < nreg; i++) {
        if (dev[i]) {
            if (dev[i]->is_fullscreen(0x10000))
                found = i;
        }
    }
    if (found != -1) {
        dev[6] = (dev_rec*)found;
        found = (int)&g_screen;
        g_screen.apply();
    }
}

// @ 0x4ca720
// Releases both hash tables of the translation table: every bucket's strings, then its buffer.
void LangTables::release_strings()
{
    lang_bucket* p = &vals[0xdf3];
    int n = 0xdf3;
    do {
        p--;
        if (p->myfirst) {
            for (tstring* s = p->myfirst; s != p->mylast; s++)
                s->~tstring();
            engine_delete(p->myfirst);
        }
        p->myfirst = 0;
        p->mylast = 0;
        p->myend = 0;
        n--;
    } while (n != 0);

    p = &keys[0xdf3];
    n = 0xdf3;
    do {
        p--;
        if (p->myfirst) {
            for (tstring* s = p->myfirst; s != p->mylast; s++)
                s->~tstring();
            engine_delete(p->myfirst);
        }
        p->myfirst = 0;
        p->mylast = 0;
        p->myend = 0;
        n--;
    } while (n != 0);
}

// @ 0x4ca830
void big_holder::release()
{
    operator delete[](px_d4);
    s.~scene_sub();
    m.~mesh_sub();
    operator delete[](px_d8);
    if (v3a8)
        operator delete(v3a8);
    v3a8 = 0;
    v3ac = 0;
    v3b0 = 0;
    tail.clear();
    for (int i = 4; i >= 0; i--)
        vec_dtor_iter(&rows[i], 0x4c, 2, &item_dtor);
}

// @ 0x4ca8c0
void graph_obj::rebuild()
{
    for (;;) {
        unsigned k;
        for (k = 0; nodes != 0 && k < (nodes ? (unsigned)((nodes_end - nodes) / 0x18) : 0); k++)
            nodes[k].flag = 0;
        for (k = 0; edges != 0 && k < (edges ? (unsigned)((edges_end - edges) / 0x34) : 0); k++) {
            nodes[edges[k].a].flag = 1;
            nodes[edges[k].b].flag = 1;
        }
        for (k = 0; nodes != 0 && k < (nodes ? (unsigned)((nodes_end - nodes) / 0x18) : 0); k++) {
            if (nodes[k].flag == 0)
                break;
        }
        if (nodes != 0 && !(k < (nodes ? (unsigned)((nodes_end - nodes) / 0x18) : 0)))
            return;
        remove_node(k);
    }
}

// @ 0x4ca9b0
void graph_obj::remove_edge(int i)
{
    int last = edges ? (int)((edges_end - edges) / 0x34) : 0;
    last--;
    if (i != last) {
        edges[i] = edges[last];
        swap_poly_normals(last, i);
    }
    edges_end--;
    rebuild();
    remove_vertex_from_polys(last);
}


#define NODES(g) ((g)->nodes ? (int)((g)->nodes_end - (g)->nodes) : 0)
#define FACES(g) ((g)->faces ? (int)((g)->faces_end - (g)->faces) : 0)
#define EDGES(g) ((g)->edges ? (int)((g)->edges_end - (g)->edges) : 0)

// @ 0x4caa40
bool graph_obj::combine(int k, int o)
{
    int count = 0;
    int picked[2];
    int sel = -1;
    bool eo = false;
    bool ek = false;
    bool twice = false;

    if (edges) {
        for (int i = 0; i < EDGES(this); i++) {
            if (edges[i].n) {
                if (count < 2) {
                    picked[count] = i;
                    o = edges[i].b;
                }
                count++;
            }
        }
    } else {
        for (int i = 0; i < NODES(this); i++) {
            if (nodes[i].used) {
                if (count < 2)
                    picked[count] = i;
                count++;
            }
        }
    }
    if (count != 2)
        return false;

    bool other_k = picked[0] == k;
    bool other_o = o == k;
    if (!other_k)
        other_o = o == o;

    for (int i = 0; i < EDGES(this); i++) {
        int hits = 0;
        if (edges[i].a == k || edges[i].b == k) {
            hits = 1;
            if (edges[i].n == 1)
                ek = true;
        }
        if (edges[i].a == o || edges[i].b == o) {
            if (edges[i].n == 1)
                eo = true;
            hits++;
        }
        if (hits == 2) {
            twice = true;
            sel = i;
        }
    }

    if (twice) {
        swap_face_vertices(o, k);
        swap_poly_colors(o, k);
        swap_poly_normals(sel, NODES(this) - 1);

        node f = *(node*)&faces[k];
        nodes[k] = nodes[NODES(this) - 1];
        *(node*)&faces[NODES(this) - 1] = f;

        edge t = edges[sel];
        edges[sel] = edges[EDGES(this) - 1];
        edges[EDGES(this) - 1] = t;

        k = NODES(this) - 1;
        int last = EDGES(this) - 1;
        if (!other_o) {
            if (!other_k) {
                nodes[o].x = (nodes[o].x + nodes[k].x) * 0.5f;
                nodes[o].y = (nodes[o].y + nodes[k].y) * 0.5f;
                nodes[o].z = (nodes[o].z + nodes[k].z) * 0.5f;
            }
        }
        for (int i = 0; i < EDGES(this); i++) {
            if (edges[i].a == k) {
                if (i != last)
                    edges[i].a = o;
            } else if (edges[i].b == k && i != last) {
                edges[i].b = o;
            }
        }
        for (int i = 0; i < FACES(this); i++) {
            for (int j = 0; j < faces[i].verts[8]; j++) {
                if (faces[i].verts[j] == k)
                    faces[i].verts[j] = o;
            }
            for (j = 0; j < faces[i].ends[8]; j++) {
                if (faces[i].ends[j] == sel) {
                    if (faces[i].ends[8] == 4) {
                        for (int m = j + 1; m < faces[i].ends[8]; m++) {
                            faces[i].ends[m - 1] = faces[i].ends[m];
                            faces[i].verts[m + 2] = faces[i].verts[m + 3];
                        }
                        faces[i].ends[8]--;
                    }
                    break;
                }
            }
        }
        remove_node(k);
    } else {
        if (!ek || !eo)
            return false;

        swap_face_vertices(o, k);
        swap_poly_colors(o, k);

        node f = *(node*)&faces[k];
        nodes[k] = nodes[NODES(this) - 1];
        *(node*)&faces[NODES(this) - 1] = f;

        k = NODES(this) - 1;
        if (!other_o) {
            if (!other_k) {
                nodes[o].x = (nodes[o].x + nodes[k].x) * 0.5f;
                nodes[o].y = (nodes[o].y + nodes[k].y) * 0.5f;
                nodes[o].z = (nodes[o].z + nodes[k].z) * 0.5f;
            }
        }
        for (int i = 0; i < EDGES(this); i++) {
            if (edges[i].a == k)
                edges[i].a = o;
            else if (edges[i].b == k)
                edges[i].b = o;
        }
        for (int i = 0; i < FACES(this); i++) {
            for (int j = 0; j < faces[i].verts[8]; j++) {
                if (faces[i].verts[j] == k)
                    faces[i].verts[j] = o;
            }
        }
        remove_node(k);
    }

    for (;;) {
        int i = 0;
        for (; i < EDGES(this); i++) {
            if (edges[i].a == o || edges[i].b == o) {
                for (int j = i + 1; j < EDGES(this); j++) {
                    int c = edges[i].a;
                    if (c == edges[j].a || c == edges[j].b) {
                        if (edges[i].b == edges[j].a || edges[i].b == edges[j].b)
                            break;
                    }
                }
                if (j < EDGES(this)) {
                    remove_edge(j);
                    break;
                }
            }
        }
        if (i >= EDGES(this))
            break;
    }
    finish();
    return true;
}

// @ 0x4cb3a0
bool graph_obj::split_edge(int e)
{
    int count = 0;
    int i;
    int idx = 0;
    if (e == -1) {
        for (i = 0; i < (edges ? (int)(edges_end - edges) : 0); i++) {
            if (edges[i].sel) {
                count++;
                e = i;
            }
        }
        if (count != 1)
            return false;
    }
    if (edges[e].n != 2)
        return false;
    face* f0 = &faces[edges[e].f0];
    if (f0->count != 3)
        return false;
    face* f1 = &faces[edges[e].f1];
    if (f1->count != 3)
        return false;

    int i0 = -1, i1 = -1;
    if (f0->ends[0] == e)
        i0 = 0;
    if (f1->ends[0] == e)
        i1 = 0;
    if (f0->ends[1] == e)
        i0 = 1;
    if (f1->ends[1] == e)
        i1 = 1;
    if (f0->ends[2] == e)
        i0 = 2;
    if (f1->ends[2] == e)
        i1 = 2;

    int xs[4];
    int ys[4];
    xs[0] = f0->verts[(i0 + 1) % 3];
    ys[0] = f0->ends[(i0 + 1) % 3];
    xs[1] = f1->verts[(i1 + 1) % 3];
    ys[1] = f1->ends[(i1 + 1) % 3];
    xs[2] = f0->verts[(i0 + 2) % 3];
    ys[2] = f0->ends[(i0 + 2) % 3];
    xs[3] = f1->verts[(i1 + 2) % 3];
    ys[3] = f1->ends[(i1 + 2) % 3];

    f0->verts[0] = xs[0];
    f0->verts[1] = xs[1];
    f0->verts[2] = xs[2];
    f0->verts[3] = xs[3];
    f0->ends[0] = ys[0];
    f0->ends[1] = ys[1];
    f0->ends[2] = ys[2];
    f0->ends[3] = ys[3];
    f0->count = 4;
    remove_edge(e);
    finish();
    return true;
}

// @ 0x4cb610

void rec_2e4::release()
{
    g_gui.reset();
    res_rec* r = p;
    if (r) {
        r->remove();
        engine_delete(r);
        p = 0;
    }
}

// @ 0x4cb650

void rec_1ac::release()
{
    g_gui.reset();
    res_rec* r = p;
    if (r) {
        r->remove();
        engine_delete(r);
    }
}

// @ 0x4cb680

void rec_178::release()
{
    void* self = this;
    g_gui.reset();
    res_rec* r = ((rec_178*)self)->p;
    if (r) {
        r->remove();
        engine_delete(r);
    }
}

// @ 0x4cb6b0

void vec3_holder::clear()
{
    ptr_tag tag;
    if (begin) {
        obj_free(begin, end, (void*)pad0, tag);
        delete_obj(begin);
    }
    begin = 0;
    end = 0;
    cap = 0;
}

// @ 0x4cb6f0
void list_node::grow(unsigned n)
{
    if (0x7ffffffu - count < n)
        throw std::length_error("list<T> too long");
    count += n;
}

// @ 0x4cb780
void graph_obj::clear_vecs()
{
    tidy(&vec6c);
    tidy(&vec5c);
    tidy(&vec4c);
    tidy(&edges);
    tidy(&faces);
    tidy(&nodes);
    tidy(&vec08);
}

// @ 0x4cb840
void graph_obj::clean()
{
    for (;;) {
        int i;
        for (i = 0; i < (edges ? (int)(edges_end - edges) : 0); i++)
            edges[i].flag = 0;
        for (i = 0; i < (faces ? (int)(faces_end - faces) : 0); i++) {
            int j;
            for (j = 0; j < faces[i].count; j++)
                edges[faces[i].ends[j]].flag = 1;
        }
        i = 0;
        for (; i < (edges ? (int)(edges_end - edges) : 0); i++) {
            if (!edges[i].flag)
                break;
        }
        if (i >= (edges ? (int)(edges_end - edges) : 0))
            return;
        remove_edge(i);
    }
}

// @ 0x4cb940
void graph_obj::remove_face(int i)
{
    int last = (faces ? (int)(faces_end - faces) : 0) - 1;
    if (i != last)
        faces[i] = faces[last];
    if ((faces ? (int)(faces_end - faces) : 0) != 0)
        faces_end--;
    clean();
}

// @ 0x4cb9e0
void big_obj::release()
{
    res_rec* r = p2e74;
    if (r) {
        r->remove();
        engine_delete(r);
    }
    gadget** s = slots;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (*s) {
                (*s)->release(1);
                *s = 0;
            }
            s++;
        }
    }
    tidy(&vec2eb0);
    tidy(&vec2ea0);
    tidy(&vec2e90);
    tidy(&vec2e80);
    for (int i = 0x7f; i >= 0; i--)
        names[i].~tstring();
    tidy(&vec44);
    ts24.~tstring();
}

// @ 0x4cbb90
bool tvec::reserve_50(unsigned n)
{
    begin = 0;
    end = 0;
    cap = 0;
    if (n == 0)
        return false;
    if (n > 0x3333333) {
        _Xlen();
    }
    tstring* p = (tstring*)operator new(n * 0x50);
    cap = p + n * 5;
    begin = p;
    end = p;
    return true;
}

// @ 0x4cbbe0
bool tvec::reserve_38(unsigned n)
{
    begin = 0;
    end = 0;
    cap = 0;
    if (n == 0)
        return false;
    if (n > 0x4924924)
        _Xlen();
    tstring* p = (tstring*)operator new(n * 0x38);
    cap = (tstring*)((char*)p + n * 0x38);
    begin = p;
    end = p;
    return true;
}

// @ 0x4cbc30
bool tvec::reserve_204(unsigned n)
{
    begin = 0;
    end = 0;
    cap = 0;
    if (n == 0)
        return false;
    if (n > 0x7f01fc)
        _Xlen();
    tstring* p = (tstring*)operator new(n * 0x204);
    cap = (tstring*)((char*)p + n * 0x204);
    begin = p;
    end = p;
    return true;
}

// @ 0x4cbc80
bool tvec::reserve_b0(unsigned n)
{
    begin = 0;
    end = 0;
    cap = 0;
    if (n == 0)
        return false;
    if (n > 0x1745d17)
        _Xlen();
    tstring* p = (tstring*)operator new(n * 0xb0);
    cap = (tstring*)((char*)p + n * 0xb0);
    begin = p;
    end = p;
    return true;
}

// ---- explicit instantiations: the std::vector members the linker placed in this object (0x4cbcd0..0x4cd720) ----
// The vectors are VC7.1's <vector> instantiated explicitly; the STL's explicit try/catch keeps its frame and funclets
// while a trivial element leaves the catch path as free(_Newvec) + throw, as in the original.
template class std::vector<E44>;
template class std::vector<E516>;
template class std::vector<scalar4>;
template class std::vector<tstring>;
template class std::vector<E56>;
template class std::vector<R44>;
template class std::vector<E50>;
template class std::vector<E68>;
template class std::vector<E176>;

// @ 0x4cc260
void __fastcall erase_110(void* self, int, void* first, void* last)
{
    nonscalar_tag tag;
    destroy_110(first, last, self, tag);
}

// @ 0x4cd720
Obj720::Obj720()
{
    u[0].g = 0;
    u[1].g = 1;
}

// @ 0x4cd7a0
Obj7a0::Obj7a0()
{
    p44 = 0; p48 = 0; p4c = 0;
    f18 = 1.0f; f1c = 1.0f;
    v30 = vec4f(0, 0, 0);
    v20 = vec4f(0, 0, 0);
    f10 = 2.0f; f14 = -0.01f;
}

// @ 0x4cd850
void fix_skeletons(std::vector<Skeleton*>* list)
{
    for (unsigned i = 0; i < (unsigned)(list->_Mylast - list->_Myfirst) && i < list->_Mylast - list->_Myfirst; i++) {
        Skeleton* sk = list->_Myfirst[i];
        SkelRes* res = g_resmgr.get_object70(sk->model);
        if (res == 0) {
            ((stream_base&)g_log).w_cstr("Fixing skeletons: Unable to find skeleton model:").w_tstr((const rgl::string*)&sk->model);
            continue;
        }
        int n = (int)((sk->end - sk->begin) / 0x50);
        int j = 0;
        if (n) {
            while (j < n && j < res->count) {
                SkelBone* d = (SkelBone*)(res->bones + j * 0xdc);
                Bone* b = sk->begin + j;
                d->name = b->name;
                copy16(&d->vcc, b->m40);
                d->fa0 = b->parent;
                d->fa4 = b->f18;
                d->f9c = b->idx;
                d->fa8 = b->f1c;
                d->vac = b->v20;
                d->vbc = b->v30;
                j++;
            }
        }
    }
}

// @ 0x4cda20
LangTables::LangTables()
{
    notfound.assign("STR NOT FOUND");
}

// @ 0x4cda90
void LangTables::load(const char* root)
{
    FILE* f = fopen("language.txt", "r");
    tstring line;
    char buf[5000];
    if (fgets(buf, 5000, f)) {
        line.assign(buf);
        if (line.buf[line.len - 1] == '\n')
            language = line.substr(0, line.len - 1);
        else
            language = line;
    } else {
        language.assign("en");
    }
    fclose(f);

    char pattern[0x2b8];
    _finddata_t* fd = (_finddata_t*)&pattern[0x80];
    sprintf(pattern, "%slanguages\\%s\\*", root, language.c_str());
    long h = _findfirst(pattern, fd);
    if (h == -1) {
        _findclose(h);
        return;
    }
    do {
        if ((fd->attrib & 0x10) != 0)
            continue;
        char path[0x100];
        sprintf(path, "%slanguages\\%s\\%s", root, language.c_str(), fd->name);
        FILE* g = fopen(path, "r");
        tstring key;
        tstring value;
        tstring text;
        bool first = true;
        while (fgets(buf, 5000, g)) {
            if (first)
                first = false;
            text.assign(buf);
            if (text.len > 2 && text.buf[0] == '\xef' && text.buf[1] == '\xbb' && text.buf[2] == '\xbf')
                text = text.substr(3, text.len);
            int i = 0;
            int len = text.len;
            while (i < len) {
                if (text.buf[i] == '|') {
                    int a = 0;
                    int stop = i;
                    int kstart = i + 1;
                    int end = len - 1;
                    if (text.buf[0] == '"') a = 1;
                    if (text.buf[i - 1] == '"') stop = i - 1;
                    if (text.buf[kstart] == '"') kstart++;
                    if (text.buf[end] == '"') end--;
                    key = text.substr(a, stop);
                    value = text.substr(kstart, end);
                    unsigned hash = 0;
                    for (int c = 0; c < key.len; c++)
                        hash = (hash * 0x3b + (unsigned)(signed char)key.buf[c]) % 0xdf3;
                    tstring* found = find(&key);
                    if (found != 0) {
                        *found = value;
                    } else {
                        lang_bucket* kb = &keys[hash];
                        if (kb->size() < (unsigned)(kb->myend - kb->myfirst))
                            vec_insert_n(kb, kb->mylast, 1, key);
                        else
                            vec_insert_n2(kb, kb->mylast, 1, key);
                        kb->mylast++;
                        lang_bucket* vb = &vals[hash];
                        if (vb->size() < (unsigned)(vb->myend - vb->myfirst))
                            vec_insert_n(vb, vb->mylast, 1, value);
                        else
                            vec_insert_n2(vb, vb->mylast, 1, value);
                        vb->mylast++;
                    }
                    len = text.len;
                    i = -1;
                }
                i++;
            }
        }
        fclose(g);
    } while (_findnext(h, fd) == 0);
    _findclose(h);
}

// @ 0x4cdf20
csv_row::csv_row()
{
    // the scalar fields are assigned in the body, not in an init list: the original emits their constant
    // stores only after the alts loop (i.e. after every implicit member construction) and shares the 1.0f
    // constant in eax across +0x40/+0x21c/+0x224, which only happens when the three stores stay adjacent.
    f40 = 1.0f;
    f1c8 = 0;
    f1d8 = 10;
    f1e0 = 0;
    f1e4 = 0;
    f44 = 0;
    f21c = 1.0f;
    f224 = 1.0f;
    f220 = 100;
    v34.clear();
}

// @ 0x4cdfd0
void ObjectList::read(stream_base* s)
{
    tstring buf;
    s->r_tstr((rgl::string*)&name);
    s->r_u64((unsigned __int64*)&f10);
    s->r_int(&count);
    int cnt = count;
    char* block = (char*)engine_alloc(cnt * 0x48 + 4);
    if (block) {
        *(int*)block = cnt;
        shape_entry* e = (shape_entry*)(block + 4);
        entries = e;
        for (int k = cnt - 1; k >= 0; k--) {
            CONSTRUCT_TSTRING(&e->name);
            e->nodes.begin = 0;
            e->nodes.end = 0;
            e->nodes.cap = 0;
            e->shape = 0;
            e->body = 0;
            e++;
        }
    } else {
        entries = 0;
    }
    if (count > 0) {
        shape_entry* c = entries;
        for (int i = 0; i < count; i++) {
            s->r_tstr((rgl::string*)&c->name);
            mesh_vector nodes;
            g_resmgr.find_meshes(&nodes, &c->name);
            vec_assign_43f0a0(&c->nodes, &nodes);
            if (nodes.begin)
                engine_delete(nodes.begin);
            c->mn = vector4(10000.0f, 10000.0f, 10000.0f);
            c->mx = vector4(-10000.0f, -10000.0f, -10000.0f);
            nodes.begin = 0;
            nodes.end = 0;
            nodes.cap = 0;
            for (unsigned j = 0; j < (unsigned)((c->nodes.end - c->nodes.begin) >> 2); j++) {
                mesh* node = c->nodes.begin[j];
                vector4 a, b;
                mesh_node_bounds(node, &a, &b);
                if (c->mn.x < a.x) ; else c->mn.x = a.x;
                if (c->mn.y < a.y) ; else c->mn.y = a.y;
                if (c->mn.z < a.z) ; else c->mn.z = a.z;
                if (c->mx.x > a.x) ; else c->mx.x = a.x;
                if (c->mx.y > a.y) ; else c->mx.y = a.y;
                if (c->mx.z > a.z) ; else c->mx.z = a.z;
            }
            s->r_tstr((rgl::string*)&buf);
            if (buf.equals(g_none)) {
                c->shape = 0;
            } else {
                c->shape = g_resmgr.get_mesh(&buf);
                c->body = body_copy_shape(c->shape);
            }
            c = c + 1;
        }
    }
    s->r_tstr((rgl::string*)&buf);
}

// @ 0x4ce2a0
MissionObj::MissionObj()
{
    v18 = vector4(0.0f, 0.0f, 0.0f);
    frame = vector4(1.0f, 1.0f, 1.0f);
    shape = 0; body = 0;
    v58 = vector4(0.0f, 0.0f, 0.0f);
    frame.w = 1.0f;
    radius = 0.1f;
}

// @ 0x4ce360
void MissionObj::read(stream_base* s)
{
    char buf[0x200];
    vector4 mn(10000.0f, 10000.0f, 10000.0f);
    vector4 mx(-10000.0f, -10000.0f, -10000.0f);
    s->r_tstr((rgl::string*)&name);
    s->r_int(&f10);
    s->r_int(&f14);
    tstring t0;
    tstring t1;
    s->r_tstr((rgl::string*)&t0);
    if (t0.len > 1 && t0.differs(g_none)) {
        mesh_vector nodes;
        g_resmgr.find_meshes(&nodes, &t0);
        vec_assign_43f0a0(&nodes_, &nodes);
        if (nodes.begin)
            engine_delete(nodes.begin);
        unsigned j = 0;
        while (1) {
            if (nodes_.begin == 0 || (nodes_.end - nodes_.begin) <= (int)j)
                break;
            vector4 a, b;
            mesh_node_bounds(nodes_.begin[j], &a, &b);
            vmax_ext(&mx, &mx, &b);
            vmin_ext(&mn, &mn, &a);
            j++;
        }
        v58.x = (mn.x + mx.x) * HALF;
        v58.y = (mn.y + mx.y) * HALF;
        v58.z = (mn.z + mx.z) * HALF;
        v58.w = 0;
        radius = rgl::length(mx - v58);
    }
    s->r_tstr((rgl::string*)&t1);
    if (t1.len > 1) {
        shape = g_resmgr.get_mesh(&t1);
        body = body_copy_shape(shape);
        vector4 b;
        vmax_ext(&mx, &mx, body->get_bounds_max(&b));
        vmin_ext(&mn, &mn, body->get_bounds_min(&b));
        v58.x = (mn.x + mx.x) * HALF;
        v58.y = (mn.y + mx.y) * HALF;
        v58.z = (mn.z + mx.z) * HALF;
        v58.w = 0;
        radius = rgl::length(mx - v58);
    }
    _snprintf(buf, 0x200, "Mission object %s", name.c_str());
    v70.resize(s, buf, 0);
    v70.stamp(2);
}

// @ 0x4ce680
MissionRec2::MissionRec2()
{
    f20 = 0; f24 = 0; f28 = 0; f14 = 0;
    frame = vector4(1.0f, 1.0f, 1.0f);
    v3c = vector4(1.0f, 1.0f, 1.0f);
    fac = 2;
}

