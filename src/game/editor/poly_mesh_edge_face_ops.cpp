// poly_mesh_edge_face_ops.cpp: 0x4ca600..0x4ce720 (TU map 104 "mesh_2"; the start is proven by the forward call 0x4ca550 -> 0x4ca600).
// The screen-mode queue, the graph editor's edge/face operations, the big editor record releasers, the std::vector
// instantiations the linker placed in this object, the skeleton fix-up, the language tables' constructor and loader, the
// csv row and the scene object readers. Migrated from slices s004ca550 (0x4ca600..0x4ca9b0), s004caa40, s004cb3a0,
// s004cbcd0, s004cc680, s004cd010, s004cd850 and s004cdfd0 (up to 0x4ce680; 0x4ce720 onward is the next object).
#include "game/editor/poly_mesh_edge_face_ops.h"
#include "engine/core/tstring.h"   // (listed directly: the difftest resolver reads only the source's own includes; without it the
                                  // unscoped vector<mesh*>::operator= annotation in poly_mesh_edge_face_ops.h also bound tstring::operator=)
#include "engine/core/arr_set.h"
#include "game/script/script_state.h"   // ScriptState: ~ScriptState 0x4cb9e0 (g_script 0xa70440)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/scene/frustum.h"
#include "engine/math/mat4_include.h"
#include "engine/core/ptr_tag.h"
#include "game/ui/screens/face_generator/face_gen.h"               // FaceGen: slot 1 (0x4cb680) deletes its preview scene
#include "game/editor/terrain_page/terrain_page_game.h"            // TerrainPage: slot 1 (0x4cb650) deletes its preview scene
#include "game/ui/screens/character_sheet/char_sheet_panel.h"      // CharSheet: slot 1 (0x4cb610) releases its preview world
#include "game/ui/screens/screen_records.h"     // C4c3050: slot 3 (0x4ca6c0) is its pressed-button finder
#include "game/ui/widgets/widget_base.h"
#include "game/data/trigger_vecs.h"        // SimpleTriggerVec (resize 0x4c1e10, stamp 0x4c1ee0)
#include "game/terrain/poly_mesh.h"   // PolyMesh: its destructor 0x4cb780
#include "engine/scene/world.h"                // rgl::world: ~world 0x475d50 (the grid's destructor body, called without the delete)
#include "game/terrain/terrain.h"      // Terrain (the 0x845e7c8-byte world-map terrain object): its constructor 0x4ce720 ends this object
#include "platform/wl_marker.h"            // WL_MARKER: the original's inline WinLicense region markers

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
SkeletonBody g_skeletons;                        // 0xa180e8 (constructed and destroyed by the compiler)


#pragma warning(disable:4234)     // __thiscall on a free function (thunk reconstruction)


const char g_none[] = "none";                                   // 0x54ac00
static const float HALF = 0.5f;                                 // 0x549d70

// @ 0x4ca600
void WindowMgrW::apply()
{
    g_gui.set_overlay(0, true);
    arr[dq.back()]->reset_screen();
    dq.pop_back();
    if (!dq.empty()) {
        unsigned char v = arr[dq.back()]->always_true();  // the screen shows the cursor
        if (g_render.dev)
            g_render.dev->ShowCursor(v);
        g_gui.show_cursor = v;
    }
}

// @ 0x4ca6c0
void C4c3050::frame_update(float)
{
    int found = -1;
    for (int i = 0; i < count; i++) {
        if (rows[i]) {
            if (rows[i]->clear_mask(0x10000))
                found = i;
        }
    }
    if (found != -1) {
        sel = found;
        g_windows.apply();
    }
}

// @ 0x4ca720
// The table's destructor, out of line (the atexit thunk 0x548470 tail-calls it). The members go first (language, then
// notfound), then the base's tables. It used to be a release_strings() helper that also destroyed the two strings, called
// from an inline ~StrTable, so the compiler then destroyed them a second time at exit.
StrTable::~StrTable()
{
}

// @ 0x4ca8c0
void PolyMesh::rebuild()
{
again:
    unsigned i;
    for (i = 0; i < verts.size(); i++)
        verts[i].mark = 0;
    for (i = 0; i < edges.size(); i++) {
        verts[edges[i].v0].mark = 1;
        verts[edges[i].v1].mark = 1;
    }
    for (i = 0; i < verts.size(); i++) {
        if (verts[i].mark == 0) {
            remove_node(i);
            goto again;
        }
    }
}

// @ 0x4ca9b0
void PolyMesh::remove_edge(int i)
{
    int last = edges.size() - 1;
    if (i != last) {
        edges[i] = edges[last];
        swap_poly_normals(last, i);
    }
    edges.pop_back();
    rebuild();
    remove_vertex_from_polys(last);
}



// @ 0x4caa40
// Welds the two selected vertices (or the two ends of the one selected edge) into one: the second is swapped to the
// end of the node table, its position merged into the first, every edge / face reference redirected, the last node
// removed; then edges that now run between the same two nodes are merged.
bool PolyMesh::combine()
{
    int sel;
    int count;
    bool ek;
    bool eo;
    bool twice;
    int p[2];
    unsigned i;
    count = 0;
    ek = false;
    eo = false;
    twice = false;
    sel = -1;
    if (mode == 0) {
        for (i = 0; i < verts.size(); i++) {
            if (verts[i].flag) {
                if (count < 2)
                    p[count] = i;
                count++;
            }
        }
    } else {
        for (i = 0; i < edges.size(); i++) {
            if (edges[i].flag) {
                if (count < 2) {
                    p[0] = edges[i].v0;
                    p[1] = edges[i].v1;
                }
                count += 2;
            }
        }
    }
    if (count != 2)
        return false;
    int o = p[1];
    bool pk = false;
    bool po = false;
    if (p[0] == cur)
        pk = true;
    else if (o == cur)
        po = true;
    for (i = 0; i < edges.size(); i++) {
        int hits = 0;
        if (edges[i].v0 == p[0] || edges[i].v1 == p[0]) {
            hits = 1;
            if (edges[i].nfaces == 1)
                ek = true;
        }
        if (edges[i].v0 == o || edges[i].v1 == o) {
            if (edges[i].nfaces == 1)
                eo = true;
            hits++;
        }
        if (hits == 2) {
            twice = true;
            sel = i;
        }
    }
    if (twice) {
        swap_face_vertices(o, verts.size() - 1);
        swap_poly_colors(o, verts.size() - 1);
        swap_poly_normals(sel, edges.size() - 1);
        MVert t = verts[o];
        verts[o] = verts[verts.size() - 1];
        verts[verts.size() - 1] = t;
        MEdge te = edges[sel];
        edges[sel] = edges[edges.size() - 1];
        edges[edges.size() - 1] = te;
        o = verts.size() - 1;
        sel = edges.size() - 1;
        if (po) {
            verts[p[0]].pos = verts[o].pos;
        } else if (!pk) {
            verts[p[0]].pos = (verts[p[0]].pos + verts[o].pos) * HALF;
        }
        for (i = 0; i < edges.size(); i++) {
            if (edges[i].v0 == o) {
                if ((int)i != sel)
                    edges[i].v0 = p[0];
            } else if (edges[i].v1 == o && (int)i != sel) {
                edges[i].v1 = p[0];
            }
        }
        for (i = 0; i < faces.size(); i++) {
            int j;
            for (j = 0; j < faces[i].n; j++) {
                if (faces[i].vid[j] == o)
                    faces[i].vid[j] = p[0];
            }
            for (j = 0; j < faces[i].n; j++) {
                if (faces[i].eid[j] == sel) {
                    if (faces[i].n == 4) {
                        for (int m = j + 1; m < faces[i].n; m++) {
                            faces[i].eid[m - 1] = faces[i].eid[m];
                            faces[i].vid[m - 1] = faces[i].vid[m];
                        }
                        faces[i].n--;
                    }
                    break;
                }
            }
        }
    } else {
        if (!ek || !eo)
            return false;
        swap_face_vertices(o, verts.size() - 1);
        swap_poly_colors(o, verts.size() - 1);
        MVert t = verts[o];
        verts[o] = verts[verts.size() - 1];
        verts[verts.size() - 1] = t;
        o = verts.size() - 1;
        if (po) {
            verts[p[0]].pos = verts[o].pos;
        } else if (!pk) {
            verts[p[0]].pos = (verts[p[0]].pos + verts[o].pos) * HALF;
        }
        for (i = 0; i < edges.size(); i++) {
            if (edges[i].v0 == o)
                edges[i].v0 = p[0];
            else if (edges[i].v1 == o)
                edges[i].v1 = p[0];
        }
        for (i = 0; i < faces.size(); i++) {
            for (int j = 0; j < faces[i].n; j++) {
                if (faces[i].vid[j] == o)
                    faces[i].vid[j] = p[0];
            }
        }
    }
    remove_node(o);
    unsigned a;
    unsigned b;
again:
    for (a = 0; a < edges.size(); a++) {
        for (b = a + 1; b < edges.size(); b++) {
            if ((edges[a].v0 == edges[b].v0 || edges[a].v0 == edges[b].v1) &&
                (edges[a].v1 == edges[b].v0 || edges[a].v1 == edges[b].v1)) {
                for (i = 0; i < faces.size(); i++) {
                    for (int j = 0; j < faces[i].n; j++) {
                        if (faces[i].eid[j] == (int)b)
                            faces[i].eid[j] = a;
                    }
                }
                remove_edge(b);
                goto again;
            }
        }
    }
    build_adjacency();                      // 0x470920 (game/terrain/terrain_scene_renderer.cpp)
    return true;
}

// @ 0x4cb3a0
bool PolyMesh::split_edge(int e)
{
    int count = 0;
    if (e == -1) {
        e = -1;
        for (unsigned i = 0; i < edges.size(); i++) {
            if (edges[i].flag) {
                count++;
                e = i;
            }
        }
        if (count != 1)
            return false;
    }
    if (edges[e].nfaces != 2)
        return false;
    MPoly* f0 = &faces[edges[e].face[0]];
    if (f0->n != 3)
        return false;
    int f1 = edges[e].face[1];
    if (faces[f1].n != 3)
        return false;

    int i0 = -1;
    int i1 = -1;
    if (f0->eid[0] == e)
        i0 = 0;
    if (faces[f1].eid[0] == e)
        i1 = 0;
    if (f0->eid[1] == e)
        i0 = 1;
    if (faces[f1].eid[1] == e)
        i1 = 1;
    if (f0->eid[2] == e)
        i0 = 2;
    if (faces[f1].eid[2] == e)
        i1 = 2;

    // ring order (0x4cb530..0x4cb5c3): f0's next two entries, then f1's next two
    int xs[4];
    int ys[4];
    xs[0] = f0->vid[(i0 + 1) % 3];
    ys[0] = f0->eid[(i0 + 1) % 3];
    xs[2] = faces[f1].vid[(i1 + 1) % 3];
    ys[2] = faces[f1].eid[(i1 + 1) % 3];
    xs[1] = f0->vid[(i0 + 2) % 3];
    ys[1] = f0->eid[(i0 + 2) % 3];
    xs[3] = faces[f1].vid[(i1 + 2) % 3];
    ys[3] = faces[f1].eid[(i1 + 2) % 3];

    f0->eid[0] = ys[0];
    f0->vid[0] = xs[0];
    f0->eid[1] = ys[1];
    f0->vid[1] = xs[1];
    f0->eid[2] = ys[2];
    f0->vid[2] = xs[2];
    f0->eid[3] = ys[3];
    f0->vid[3] = xs[3];
    f0->n = 4;
    remove_edge(e);
    build_adjacency();                      // 0x470920 (game/terrain/terrain_scene_renderer.cpp)
    return true;
}

// @ 0x4cb610

void CharSheet::reset_screen()
{
    g_gui.reset();
    rgl::world* r = scene;
    if (r) {
        r->~world();                  // 0x475d50: the grid destructor, called before the delete
        operator delete(r);
        scene = 0;
    }
}

// @ 0x4cb650

void TerrainPage::reset_screen()
{
    g_gui.reset();
    rgl::world* r = world;
    if (r) {
        r->~world();                  // 0x475d50: the grid destructor, called before the delete
        operator delete(r);
    }
}

// @ 0x4cb680

void FaceGen::reset_screen()
{
    g_gui.reset();
    rgl::world* r = world;
    if (r) {
        r->~world();                  // 0x475d50: the grid destructor, called before the delete
        operator delete(r);
    }
}

// @ 0x4cb6f0
template void GlyphList::_Incsize(size_t);

// @ 0x4cb780 (the members' destructors: the seven tables in reverse order)
PolyMesh::~PolyMesh()
{
}

// @ 0x4cb840
void PolyMesh::clean()
{
again:
    unsigned i;
    for (i = 0; i < edges.size(); i++)
        edges[i].mark = 0;
    for (i = 0; i < faces.size(); i++) {
        for (int j = 0; j < faces[i].n; j++)
            edges[faces[i].eid[j]].mark = 1;
    }
    for (i = 0; i < edges.size(); i++) {
        if (edges[i].mark == 0) {
            remove_edge(i);
            goto again;
        }
    }
}

// @ 0x4cb940
void PolyMesh::remove_face(int i)
{
    int last = faces.size() - 1;
    if (i != last)
        faces[i] = faces[faces.size() - 1];
    faces.pop_back();
    clean();
}

// @ 0x4cb9e0
ScriptState::~ScriptState()
{
    rgl::world* lv = level;
    if (lv) {
        lv->~world();   // 0x475d50: the grid destructor, called before the delete
        operator delete(lv);
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (map_saved[i * 8 + j]) {
                delete map_saved[i * 8 + j];
                map_saved[i * 8 + j] = 0;
            }
        }
    }
    // the material lists (std::vector<rgl::mesh*>) are destroyed by their own destructors: mat_sea2 first, in reverse declaration order
}

// ---- explicit instantiations: the std::vector members the linker placed in this object (0x4cbcd0..0x4cd720) ----
// The vectors are VC7.1's <vector> instantiated explicitly; the STL's explicit try/catch keeps its frame and funclets
// while a trivial element leaves the catch path as free(_Newvec) + throw, as in the original.
template class std::vector<E44>;
template class std::vector<Constraint>;
template class std::vector<GlyphIt>;
// 0x4cc280 is std::vector<skin*>'s copy constructor (E68::v, emitted with vector<E68> below); FaceTexture::hair_materials'
// vector<int> one (0x4cd170) is the same code from this object, folded into it (link 7.10 folds EH-framed COMDATs within
// one object). The GlyphIt bucket vector above keeps _Insert_n 0x4cc430, resize 0x4cd010 and _Construct_n 0x4cd1c0.
template class std::vector<tstring>;
template class std::vector<ArrSet>;
template class std::vector<CapDef>;
template class std::vector<RagdollBone>;
template class std::vector<E68>;

// @ 0x4cd080
RagdollBone::RagdollBone(const RagdollBone& o)
    : name(o.name), bone_mass(o.bone_mass), cone_min(o.cone_min), cone_max(o.cone_max), twist_limit(o.twist_limit),
      rotate(o.rotate), socket_dir(o.socket_dir), bodies(o.bodies)
{
}

// @ 0x4cd270
// memberwise, spelled out: the implicit one would not compile (the shared tstring::operator= returns void)
RagdollBone& RagdollBone::operator=(const RagdollBone& o)
{
    name = o.name;
    bone_mass = o.bone_mass; cone_min = o.cone_min; cone_max = o.cone_max; twist_limit = o.twist_limit;
    rotate = o.rotate;
    socket_dir = o.socket_dir;
    bodies = o.bodies;
    return *this;
}

// @ 0x4cd100
E68::E68(const E68& o)
    : s(o.s), b0(o.b0), b1(o.b1), b2(o.b2), b3(o.b3), b4(o.b4), b5(o.b5), b6(o.b6), b7(o.b7), b8(o.b8), b9(o.b9),
      m(o.m), v(o.v), t(o.t)
{
}

// @ 0x4cd2f0
E68& E68::operator=(const E68& o)
{
    s = o.s;
    b0 = o.b0; b1 = o.b1; b2 = o.b2; b3 = o.b3; b4 = o.b4;
    b5 = o.b5; b6 = o.b6; b7 = o.b7; b8 = o.b8; b9 = o.b9;
    m = o.m;
    v = o.v;
    t = o.t;
    return *this;
}
// @ 0x4cc260
template class std::vector<FaceTexture>;

// 0x4cc260 is std::vector<FaceTexture>::_Destroy (the explicit instantiation above; _Destroy_range 0x4ca7e0)

// @ 0x4cd7a0
RagdollBone::RagdollBone()
{
    cone_max = 1.0f; twist_limit = 1.0f;
    socket_dir = rgl::vector4(0, 0, 0);
    rotate = rgl::vector4(0, 0, 0);
    bone_mass = 2.0f; cone_min = -0.01f;
}

// @ 0x4cd850
void __fastcall fix_skeletons(std::vector<RagModel>* list)
{
    for (unsigned i = 0; i < list->size(); i++) {
        RagModel* sk = &(*list)[i];
        frame_set* res = g_resmgr.get_object70(sk->name);
        if (res != 0) {
            for (unsigned j = 0; j < sk->bones.size() && (int)j < res->nbones; j++) {
                res->bones[j].name = sk->bones[j].name;
                res->bones[j].bodies = sk->bones[j].bodies;
                res->bones[j].cone_min = sk->bones[j].cone_min;
                res->bones[j].cone_max = sk->bones[j].cone_max;
                res->bones[j].bone_mass = sk->bones[j].bone_mass;
                res->bones[j].twist_limit = sk->bones[j].twist_limit;
                res->bones[j].rotate = sk->bones[j].rotate;
                res->bones[j].socket_dir = sk->bones[j].socket_dir;
            }
        } else {
            ((stream_base&)g_log).w_cstr("Fixing skeletons: Unable to find skeleton model:").w_tstr(&sk->name);
        }
    }
}

// @ 0x4cda20
StrTable::StrTable()
{
    notfound.assign("STR NOT FOUND");
}

// The UI string table: a real definition, built by the compiler's dynamic initialiser (the ctor above) and torn down by its
// atexit destructor (~StrTable 0x4ca720); it replaces static_init's hand stubs 0x547a80 / 0x548470.
StrTable g_strtable;   // 0x5e2a18

// @ 0x4cda90
void StrTable::load(const tstring* root_t)
{
    FILE* f = fopen("language.txt", "r");
    tstring line;
    char buf[5012];
    ptr_tag tag;
    if (fgets(buf, 5000, f)) {
        line.assign(buf);
        (void)line.len;     // lane device (R1-B): a first reference to the length view before the buffer view, so cl orders the address
                            // add base-first ([edx+eax-1]); symkeys levers: swap:_line+12,_line+4
        if (line.buf[line.len - 1] == '\n')
            language = line.substr(0, line.len - 1);
        else
            language = line;
    } else {
        language.assign("en");
    }
    fclose(f);

    long h;
    char pattern[0x200];
    _finddata_t fdata;
    _finddata_t* fd = &fdata;
    sprintf(pattern, "%slanguages\\%s\\*", root_t->c_str(), language.c_str());
    h = _findfirst(pattern, fd);
    if (h != -1) {
    do {
        if ((fd->attrib & 0x10) != 0)
            continue;
        char path[0x200];
        sprintf(path, "%slanguages\\%s\\%s", root_t->c_str(), language.c_str(), fd->name);
        FILE* g = fopen(path, "r");
        int i = 0;          // lane device (R1-B): i / end first referenced before `text`, which gives the original's [text.buf+i] base order
        int end = 0;        // (symkeys levers: swap:_i,_text / swap:_end,_text)
        tstring text;
        tstring key;
        tstring value;
        bool first = true;
        while (fgets(buf, 5000, g)) {
            bool first_line = first;            // the BOM is only looked for on the file's first line ([esp+0x13] at 0x4cdc71)
            text.assign(buf);
            if (first_line && text.len > 2 && text.buf[0] == '\xef' && text.buf[1] == '\xbb' && text.buf[2] == '\xbf')
                text = text.substr(3, text.len);
            first = false;
            bool done = false;                  // the line is split at its first '|' only ([esp+0x12] in the original)
            int len = text.len;
            i = 0;
            while (i < len && !done) {
                if (text.buf[i] == '|') {
                    done = true;
                    int a = 0;
                    int stop = i;
                    int kstart = i + 1;
                    end = len - 1;
                    if (text.buf[0] == '"') a = 1;
                    if (text.buf[i - 1] == '"') stop = i - 1;
                    if (text.buf[kstart] == '"') kstart++;
                    if (text.buf[end - 1] == '"') end--;   // 0x4cdd1e: the byte before the last (the last is the newline)
                    key = text.substr(a, stop);
                    value = text.substr(kstart, end);
                    unsigned hash = 0;
                    for (unsigned c = 0; c < (unsigned)key.len; c++)
                        hash = (hash * 0x3b + (unsigned)(signed char)key.buf[c]) % 0xdf3;
                    tstring* found = find(&key);
                    if (found == 0) {
                        keys[hash].push_back(key);
                        vals[hash].push_back(value);
                    } else {
                        *found = value;
                    }
                }
                i++;
            }
        }
        fclose(g);
    } while (_findnext(h, fd) == 0);
    }
    _findclose(h);
}

// @ 0x4cdf20
ItemKind::ItemKind()
{
    // the scalar fields are assigned in the body, not in an init list: the original emits their constant
    // stores only after the meshes loop (i.e. after every implicit member construction) and shares the 1.0f
    // constant in eax across +0x40/+0x21c/+0x224, which only happens when the three stores stay adjacent.
    f40 = 1.0f;
    type_flags = 0;
    value = 10;
    modifiers = 0;
    mesh_count = 0;
    price_rate = 1.0f;
    merchandise_probability = 1.0f;
    abundance = 100;
    slots.tidy();
}

static inline void keep_min(float& d, const float& v) { float t = d; d = (t < v) ? t : v; }
static inline void keep_max(float& d, const float& v) { float t = d; d = (t > v) ? t : v; }
static inline void merge_min(vector4& d, const vector4& v) { keep_min(d.x, v.x); keep_min(d.y, v.y); keep_min(d.z, v.z); }
static inline void merge_max(vector4& d, const vector4& v) { keep_max(d.x, v.x); keep_max(d.y, v.y); keep_max(d.z, v.z); }

// @ 0x4cdfd0
void ptype::read(stream_base* s)
{
    tstring buf;
    s->r_tstr(&name);
    s->r_u64(&flags);
    s->r_int(&nframes);
    boxes = new AnimBox[nframes];
    for (int i = 0; i < nframes; i++) {
        vector4 big(10000000.0f, 10000000.0f, 10000000.0f);
        vector4 neg(-10000000.0f, -10000000.0f, -10000000.0f);
        s->r_tstr(&boxes[i].name);
        boxes[i].meshes = g_resmgr.find_meshes(&boxes[i].name);
        boxes[i].mn = big;
        boxes[i].mx = neg;
        for (unsigned j = 0; j < boxes[i].meshes.size(); j++) {
            vector4 a, b;
            mesh_node_bounds(boxes[i].meshes[j], &a, &b);
            merge_min(boxes[i].mn, a);
            merge_max(boxes[i].mx, b);
        }
        s->r_tstr(&buf);
        if (buf.equals("0")) {
            boxes[i].res = 0;
        } else {
            boxes[i].res = (body_part*)g_resmgr.get_object60(buf);
            boxes[i].body = boxes[i].res->copy_shape();
        }
    }
}

// @ 0x4ce2a0
MissionObj::MissionObj()
{
    v18 = vector4(0.0f, 0.0f, 0.0f);
    frame = vector4(1.0f, 1.0f, 1.0f);
    shape = 0; body = 0;
    center = vector4(0.0f, 0.0f, 0.0f);
    frame.w = 1.0f;
    radius = 0.1f;
}

// 0x4ce360 inlines the length of (mx - v58) as x*x + (y*y + z*z) (the z lane is squared first, then y, then x); 0x407d30
// (rgl::length) adds them as (x*x + y*y) + z*z, which rounds differently
static __forceinline float center_dist(const vector4& a, const vector4& c)
{
    float dx = a.x - c.x;
    float dy = a.y - c.y;
    float dz = a.z - c.z;
    return (float)sqrt(dx * dx + (dy * dy + dz * dz));
}

// @ 0x4ce360
void MissionObj::read(stream_base* s)
{
    char buf[0x200];
    s->r_tstr(&name);
    s->r_int(&flags);
    s->r_int(&hit_points);
    tstring t0;
    tstring t1;
    s->r_tstr(&t0);
    vector4 mn(10000.0f, 10000.0f, 10000.0f);
    vector4 mx(-10000.0f, -10000.0f, -10000.0f);
    if (t0.len > 1 && t0.differs(g_none)) {
        meshes = g_resmgr.find_meshes(&t0);
        unsigned j = 0;
        while (1) {
            if (j >= meshes.size())
                break;
            vector4 a, b;
            mesh_node_bounds(meshes[j], &a, &b);
            vmax(&mx, &mx, &b);
            vmin(&mn, &mn, &a);
            j++;
        }
        center = (mn + mx) * HALF;                       // w: the temporaries' unset lane
        radius = center_dist(mx, center);
    }
    s->r_tstr(&t1);
    if (t1.len > 1) {
        shape = (body_part*)g_resmgr.get_object60(t1);
        body = shape->copy_shape();
        vector4 b;
        vmax(&mx, &mx, body->get_max(&b));
        vmin(&mn, &mn, body->get_min(&b));
        center = (mn + mx) * HALF;                       // w: the temporaries' unset lane
        radius = center_dist(mx, center);
    }
    _snprintf(buf, 0x200, "Mission object %s", name.c_str());
    triggers.resize(*s, buf, 0);         // 0x4c1e10 (SimpleTriggerVec is SimpleTriggerVec's three words: count, 0x30-byte entries, spare)
    triggers.stamp(2);                   // 0x4c1ee0
}

// @ 0x4ce680
FaceTexture::FaceTexture()
{
    material_lod = 0;
    hair_colors[0] = vector4(1.0f, 1.0f, 1.0f);
    hair_colors[1] = vector4(1.0f, 1.0f, 1.0f);
    n_hair_colors = 2;
}

// @ 0x4ce720  (the slices called it SceneGen::SceneGen(int); the original takes no argument and the w lanes of the two
// colours copy an uninitialised stack word, as rgl::vector4's three-float constructor leaves w alone).
// Two 18-byte WinLicense markers (jmp +0x10 over "WL  " data) sit inside the original; cl cannot emit them.
Terrain::Terrain()
{
    WL_MARKER(12)
    sun_ambient = rgl::vector4(0.2f, 0.2f, 0.2f);
    scale = 2.0f;
    block = 0x28;
    sun_diffuse = rgl::vector4(0.6f, 0.55f, 0.5f);       // the w lane (+0x843bd48) is the uninitialised w of the temporary
    flag843bd75 = 0;
    flag843bd74 = 0;
    water_layer_type = 0;
    n284 = 2;
    WL_MARKER(13)
    reset();
}
