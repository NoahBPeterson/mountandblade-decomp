//-----------------------------------------------------------------------------
// File: game_2.cpp  (0x520b20..0x525160)
//
// Desc: the map and world-map scene builders (map.txt reader, the party / compass / track entities, the campaign map's
//       tree meshes), the module-file loader Game::load_game_data (factions, skills, skeleton bodies, troops, scenes,
//       quests, triggers, simple triggers) and the record constructors / readers / vector helpers it calls (all of
//       them follow Game::load_game_data in the binary: caller first, "T regime"). Names provisional. The file keeps
//       the binary order; helpers that follow their caller are __declspec(noinline) where the original calls them.
//       Flags: /O2 /MT /Gy /TP.
//
// Boundary: class change only at 0x520b20 (TroopRec | Game); the map's next boundary (0x5249f0, QuestRec | Faction) is NOT
//       kept: 0x5236f0 calls 0x524360..0x524d60 in turn and they are its record helpers (see MIGRATION.md).
//-----------------------------------------------------------------------------
#include <math.h>
#include <string.h>
#include "./game_2.h"
#include "../env_row.h"                // EnvRow, g_env_rows (0x5e28f4)
#include "../../include/troop.h"          // g_level_xp
#include "../trigger_vecs.h"           // (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../party/game.h"
#include "../party/externs.h"
#include "../tstring.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "../../rgl/rgl_string.h"
#include "../../rgl/scene/scene.h"
#include "../../rgl/stream.h"
#include "../../include/ptr_tag.h"
#include "../terrain_b/cell_grid.h"
#include "../../rgl/mesh.h"
#include "../../rgl/scene/scene_entity.h"
#include "../../rgl/resource.h"
#include "../../rgl/rgl_math.h"
#include "../../include/iface.h"
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../party/nav_graph.h"      // g_nav (0xa732b8)
#include "../../include/render_flags.h"
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
SkillRec g_skill_tbl[42];                                   // 0x8ee3100 (constructed and destroyed by the compiler; skills.txt: description, name, key, attribute, rank limit)



extern const int g_level_xp[64] = {  // 0x5937c8 experience bound per level (rdata)
    0, 0, 600, 1360, 2296, 3426, 4768, 6345,
    8179, 10297, 13010, 16161, 19806, 24007, 28832, 34362,
    40682, 47892, 56103, 65441, 77233, 90809, 106425, 124371,
    144981, 168636, 195769, 226879, 262533, 303381, 350164, 412091,
    484440, 568947, 667638, 782877, 917424, 1074494, 1257843, 1471851,
    1721626, 2070551, 2489361, 2992033, 3595340, 4319408, 5188389, 6231267,
    7482821, 8984785, 11236531, 14051314, 17569892, 21968215, 27466219, 34338823,
    42929679, 53668349, 67091786, 83871183, 160204600, 320304600, 644046000, 2050460000
};


// ---- the map object (0xa732b8): vertex and group vectors of the ground geometry ---------------------------------------
// The records and vectors are the ones world_grid.cpp (rgl/dynamic) defines: ground_v36 (0x24 bytes: position, normal, a flag) and ground_g136
// (0x88 bytes), std::vector of each, the group constructor (0x4246f0), the container constructor NavCore
// (0x478150) and PolyGeom::build (mesh_slot/mesh_pass.cpp 0x47ee60). They are declared here in the shape this module needs.
struct ground_v36  { float x, y, z; int pad0c[5]; float f20; ground_v36() {} };   // 0x24 bytes
struct ground_g136 {                                            // 0x88 bytes
    int n;                                                      // +0x00
    int vidx[8];                                                // +0x04 vertex ids
    int f24;                                                    // +0x24 mask
    int pad28[16];
    int f68;                                                    // +0x68
    int pad6c[7];
    ground_g136();                                              // ctor 0x4246f0
};
typedef char ground_g136_size_chk[sizeof(ground_g136) == 0x88 ? 1 : -1];
namespace std {
template<> void vector<ground_v36, allocator<ground_v36> >::resize(size_t, ground_v36);     // 0x424740
}
struct vec136 : std::vector<ground_g136> {                      // world_grid.cpp's written-out sibling of vector<ground_g136>
    void resize(unsigned n, ground_g136 v);                     // 0x424cc0
};
struct PolyGeom { void build(); };                              // 0x47ee60
typedef char nav_core_size_chk[sizeof(NavCore) == 0x68 ? 1 : -1];     // new NavCore allocates the map store's 0x68 bytes
struct MapStore {                                               // the global map (0xa732b8), 0x68 bytes
    std::vector<ground_v36> verts;                              // +0
    vec136 groups;                                              // +0x10
    char pad20[4];
    ground_v36* edge_first; ground_v36* edge_last; ground_v36* edge_end;   // +0x20 vector<GeomEdge> (0x24-byte edges; PolyGeom::edges in mesh_pass.cpp)
    char pad30[0x68 - 0x30];
};

stream_base* read_map(stream_base* s, MapStore* map);           // 0x520c00

// @ 0x520b20
void Game::build_map()
{
    if (!g_script.new_game_done) {
        g_script.nav = (NavGraph*)new NavCore();
        tstream3 s;
        const char* path = MODULE_PATH("map.txt");
        if (!OPEN_READ_TEXT(s, path))
            mb_error("Unable to open %s", path);
        if (!s.failed) {
            read_map(&s, (MapStore*)g_script.nav);
            s.failed = 1;
        }
    }
}

// @ 0x520c00
__declspec(noinline) stream_base* read_map(stream_base* s, MapStore* map)
{
    int n;
    s->r_int(&n);
    map->verts.resize(n, ground_v36());
    int i;
    for (i = 0; i < n; i++) {
        ground_v36& v = map->verts[i];
        s->r_float(&v.x).r_float(&v.y).r_float(&v.z);
        map->verts[i].f20 = -1.0f;
    }
    int n2;
    s->r_int(&n2);
    // link[] (+0x28) and the weights (+0x38) stay as the group constructor left them: PolyGeom::build fills link[k]
    // for every k < n, and nothing reads k >= n (score_triggers 0x51dac0 and solve 0x51d3a0 stop at count).
    map->groups.resize(n2, ground_g136());
    for (i = 0; i < n2; i++) {
        s->r_int(&map->groups[i].f24);
        s->r_int(&map->groups[i].f68);
        s->r_int(&map->groups[i].n);
        for (int j = 0; j < map->groups[i].n; j++)
            s->r_int(&map->groups[i].vidx[j]);
    }
    ((PolyGeom*)map)->build();
    return s;
}

static inline int map_first_sprite(SpriteTable& t)
{
    int i;
    for (i = 0; i < t.count; i++)
        if (*(int*)((char*)t.pages._Myfirst[(unsigned)i >> 4] + (i & 0xf) * 0x98 + 4) != 0)
            break;
    return i;
}
static inline void map_set_fbc(rgl::entity* e, const float& f) { e->set_fbc(&f); }

// @ 0x520d60
void Game::build_scene()
{
    GameMapView* self = (GameMapView*)this;
    WL_MARKER(12)
    if (g_script.new_game_done) {
        for (int i = 0; i < g_script.page_rows; i++) {
            for (int j = 0; j < g_script.page_cols; j++) {
                if (g_script.map_cell[j * 8 + i]) {
                    g_script.level->erase_entity(g_script.map_cell[j * 8 + i]);
                    g_script.map_saved[j * 8 + i] = g_script.map_cell[j * 8 + i];
                    g_script.map_cell[j * 8 + i] = 0;
                }
            }
        }
        g_script.level->clear_ents();
    }
    log_stream_ptr()->w_cstr("map mesh built.\n");
    {
        rgl::vector4 tmp;
        self->fog1 = *((GameG*)self)->sun_color(&tmp);
    }
    log_stream_ptr()->w_cstr("get_ideal_sun_color.\n");
    {
        rgl::vector4 tmp;
        self->fog2 = *((GameG*)self)->fog_color(&tmp);
    }
    log_stream_ptr()->w_cstr("get_ideal_fog_color.\n");
    self->fog1 = rgl::vector4(0.5f, 0.48f, 0.47f);
    self->fog2 = rgl::vector4(0.8f, 0.8f, 0.8f);
    self->fog3 = rgl::vector4(0.35f, 0.37f, 0.4f);
    {
        rgl::world* wd = g_script.level;
        wd->ambient = rgl::vector4(0.35f, 0.37f, 0.4f);
        wd->ambient.w = 1.0f;
    }

    Light* w;
    {
        rgl::world* world = g_script.level;
        w = new Light;                                         // the 0x7c-byte wind object (constructor 0x461480)
        world->lights.push_back(w);
    }
    self->wind = w;
    self->wind->type = 2;
    self->wind->color = self->fog1;
    self->wind->range = 30000.0f;
    self->wind->pos = rgl::vector4(700.0f, 200.0f, 900.0f);
    self->wind->dir = rgl::vector4(-0.8f, -0.6f, -1.0f);
    rgl::vector4& dr = self->wind->dir;
    float len = dr.length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        dr.x = dr.x * inv;
        dr.y = dr.y * inv;
        dr.z = dr.z * inv;
    } else {
        dr.x = 0.0f;
        dr.y = 1.0f;
        dr.z = 0.0f;
    }

    self->track_entity = (rgl::entity*)g_script.level->create_entity(9);
    map_set_fbc(self->track_entity, 1000000.0f);
    self->map_track = new rgl::mesh();
    self->map_track->res = g_resmgr.get_material(RStr("map_track"));
    ((Poser*)self->track_entity)->add_extra((int)self->map_track);
    WL_MARKER(13)

    rgl::entity* obj = (rgl::entity*)g_script.level->create_entity(9);
    self->map_obj = ((rgl::mesh*)g_resmgr.get_object(&g_env_rows->name))->clone();
    self->map_obj->scale(60.0f);
    self->map_obj->mask = 0xfffeffff;
    ((Poser*)obj)->add_extra((int)self->map_obj);
    map_set_fbc(obj, 100000000.0f);
    g_script.level->sky = (rgl::entity*)obj;

    self->compass = (rgl::entity*)g_script.level->create_entity(9);
    rgl::mesh* mc = ((rgl::mesh*)g_resmgr.get_object(&RStr("compass")))->clone();
    mc->scale(0.28f);
    ((Poser*)self->compass)->add_extra((int)mc);

    if (!g_script.new_game_done) {
        build_map_trees();
    } else {
        for (int i = 0; i < g_script.page_rows; i++) {
            for (int j = 0; j < g_script.page_cols; j++) {
                g_script.map_cell[j * 8 + i] = g_script.map_saved[j * 8 + i];
                g_script.map_saved[j * 8 + i] = 0;
                g_script.level->add_pending(g_script.map_cell[j * 8 + i]);
            }
        }
    }

    g_script.level->step(0.0f);

    {
        unsigned id = (unsigned)scene.first_party_id();
        while (id < (unsigned)scene.n_parties) {
            ((c49bea0::MissionAgents*)self)->add_order_arrow((int)id);
            scene.next_party_id((int*)&id);
        }
    }
    log_stream_ptr()->w_cstr("parties added.\n");

    {
        unsigned idx = map_first_sprite(sprites);
        while (idx < (unsigned)sprites.count) {
            ((c49b630::Mission*)self)->spawn_battle_icon(idx);
            ((GamePool*)&sprites)->next((int*)&idx);
        }
    }
    release_orders();
    more_start();
    if (f13a34 == 0) {
        ((GameData*)self)->emit_event(0, 0, 0);
        f13a34 = 1;
    }
}

// @ 0x5213a0
__declspec(noinline) void Game::release_orders()
{
    TrackRec* p = tracks._Myfirst;
    if (p < tracks._Mylast) {
        do {
            ((GameData*)this)->init_event((E92*)p);
            p++;
        } while (p < tracks._Mylast);
    }
}

// ---- helpers of the campaign map's tree builder (above its marker) ---------------------------------------------------
struct map_v2 {                                                     // the two-float value the map's bounds are assigned from
    float x, y;
    map_v2() {}
    map_v2(float a, float b) : x(a), y(b) {}
    float length() const { return (float)sqrt(x * x + y * y); }
};
static inline float map_len(const rgl::vector4& v) { float l = v.x * v.x; l += v.z * v.z; l += v.y * v.y; return (float)sqrt(l); }
static inline map_v2 map_xy(const rgl::vector4& p) { return map_v2(p.x, p.y); }
static inline int map_add_vertex(rgl::mesh* m, int index, const vec2& uv, const unsigned& color, const rgl::vector4& n, const int& f5c)
{
    return m->add_vertex(index, &uv, &color, &n, &f5c);
}
static inline void map_add(rgl::vector4& a, const rgl::vector4& b) { a.x += b.x; a.y += b.y; a.z += b.z; }
static inline void map_scale(rgl::vector4& v, float k) { v.x *= k; v.y *= k; v.z *= k; }
struct map_ground {                                                 // NavData with its edge vector typed
    rgl::vec<NavVert> verts;
    rgl::vec<NavCell> cells;
    rgl::vec<MapEdge> edges;
    unsigned ncells() const { return cells.first == 0 ? 0 : (unsigned)(cells.last - cells.first); }
};
struct map_arrays {                                                 // per-point tables of the map builder
    float dist[100000];
    int   vert[100000];
    float scan_b[2][100000];
    int   flags[100000];
    float scan_a[2][100000];
};
struct map_frame : rgl::matrix {                                    // a frame from a rotation and an origin
    map_frame(const rgl::mat3& r, const rgl::vector4& p) { *(rgl::mat3*)this = r; o = p; }
};
// the cell's mesh list and its recalculation (the same steps as Poser::add_extra 0x442110, expanded here)
static inline void add_cell_mesh(rgl::entity* e, rgl::mesh* m)
{
    e->meshes.push_back(m);
    e->recalc();
}

// @ 0x5213d0
void Game::build_map_trees()
{
    ((rgl::vec<void*>&)g_script.mat_type0).clear();
    ((rgl::vec<void*>&)g_script.mat_river).clear();
    ((rgl::vec<void*>&)g_script.mat_sea).clear();
    ((rgl::vec<void*>&)g_script.mat_sea2).clear();

    float sea_ang = g_module_settings.map_sea_direction * 3.1415927f * 0.0055555557f;                         // deg -> rad
    float river_ang = g_module_settings.map_river_direction * 3.1415927f * 0.0055555557f;

    // the lighting vectors the fixed-function vertex colour uses
    rgl::vector4 lightL, lightM;
    unsigned i;
    {
        const rgl::vector4& sun_dir = ((GameMapView*)this)->wind->dir;
        rgl::vector4 dir;
        dir.x = -sun_dir.x;
        dir.y = -sun_dir.y;
        dir.z = -sun_dir.z;
        dir.normalize();                                                 // 0x407d60

        lightL = g_script.level->ambient;
        lightM = g_script.level->lights[0]->color;

        // ---- per-point view dot --------------------------------------------
        for (i = 0; i < g_script.level->ground->verts.size(); i++) {
            NavVert& v = g_script.level->ground->verts[i];
            float s = v.n.y * dir.y;
            s += v.n.z * dir.z;
            s += v.n.x * dir.x;
            v.level = s;
            if (g_use_shaders != 0)
                v.level = s * s;
        }
    }

    // ---- per-point flags and distance ----------------------------------
    // 100000 entries each; the exe reserves 4 MB of stack (/STACK:0x3e8480), the same as the relink.
    map_arrays fr;

    NavData* ground = g_script.level->ground;
    for (i = 0; i < ground->verts.size(); i++) {
        fr.flags[i] = 0;
        fr.dist[i] = 1000.0f;
    }
    for (i = 0; i < ground->cells.size(); i++) {
        NavCell& c = ground->cells[i];
        for (int k = 0; k <= 9; k++) {
            int kind = c.ids[8] & 7;
            if (c.ids[8] == 8)
                kind = c.ids[8];
            else if (c.ids[8] == 9)
                kind = c.ids[8];
            else if (c.ids[8] == 0xf)
                kind = 0;
            if (kind == k) {
                unsigned mask = (unsigned)g_bit[k];
                for (int j = 0; j < c.npts; j++) {
                    int& f = fr.flags[c.ids[j]];
                    f |= mask;
                }
            }
        }
    }

    // ---- bbox + aggregate masks ----------------------------------------
    *(map_v2*)&g_script.minx = map_v2(100000.0f, 100000.0f);
    *(map_v2*)&g_script.maxx = map_v2(-100000.0f, -100000.0f);
    unsigned water = (unsigned)(g_bit[8] | g_bit[7]);
    unsigned land = (unsigned)(g_bit[4] | g_bit[5] | g_bit[11] | g_bit[9] | g_bit[12] | g_bit[13] | g_bit[10] | g_bit[2] | g_bit[3] | g_bit[1]);
    unsigned sea = (unsigned)g_bit[0];
    unsigned wood = (unsigned)g_bit[9];
    ((GameMapView*)this)->scan_arrays(fr.scan_a, fr.scan_b);

    for (i = 0; i < g_script.level->ground->verts.size(); i++) {
        rgl::vector4& p = g_script.level->ground->verts[i].p;
        if (p.x < g_script.minx) g_script.minx = p.x;
        if (p.x > g_script.maxx) g_script.maxx = p.x;
        if (p.y < g_script.miny) g_script.miny = p.y;
        if (p.y > g_script.maxy) g_script.maxy = p.y;
        if ((sea & fr.flags[i]) != 0 && (land & fr.flags[i]) != 0)
            fr.dist[i] = 0.0f;
    }

    // ---- ten relaxation passes over the edges --------------------------
    map_ground* g2 = (map_ground*)g_script.level->ground;
    for (int pass = 10; pass != 0; pass--) {
        for (i = 0; i < g2->edges.size(); i++) {
            int a = g2->edges[i].a;
            int b = g2->edges[i].b;
            // the edge endpoints are read from g_nav's vertex vector (map.txt's store), not the ground's
            NavVert* nv = ((NavData*)g_script.nav)->verts.first;
            float len = map_len(nv[a].p - nv[b].p);
            float& da = fr.dist[a];
            float& db = fr.dist[b];
            float t = len + db;
            if (t < da)
                da = t;
            else {
                t = len + da;
                if (t < db)
                    db = t;
            }
        }
    }

    // ---- material-name tables ------------------------------------------
    tstring tmp;
    rgl::mesh* map_arr[0x40];
    rgl::mesh* snow_arr[0x40];
    rgl::mesh* steppe_arr[0x40];
    rgl::mesh* desert_arr[0x40];
    memset(desert_arr, 0, sizeof(desert_arr));
    memset(steppe_arr, 0, sizeof(steppe_arr));
    memset(snow_arr, 0, sizeof(snow_arr));
    memset(map_arr, 0, sizeof(map_arr));
    if (g_module_settings.tree_types[0] > 0x40) g_module_settings.tree_types[0] = 0x40;
    if (g_module_settings.tree_types[1] > 0x40) g_module_settings.tree_types[1] = 0x40;
    if (g_module_settings.tree_types[2] > 0x40) g_module_settings.tree_types[2] = 0x40;
    if (g_module_settings.tree_types[3] > 0x40) g_module_settings.tree_types[3] = 0x40;

    int n;
    for (n = 0; n < g_module_settings.tree_types[0]; n++) {
        tmp.assign("map_tree_");
        tmp.push_back((char)((unsigned char)n + 'a'));
        map_arr[n] = g_resmgr.get_object(&tmp);
    }
    for (n = 0; n < g_module_settings.tree_types[1]; n++) {
        tmp.assign("snow_tree_");
        tmp.push_back((char)((unsigned char)n + 'a'));
        snow_arr[n] = g_resmgr.get_object(&tmp);
    }
    for (n = 0; n < g_module_settings.tree_types[2]; n++) {
        tmp.assign("steppe_tree_");
        tmp.push_back((char)((unsigned char)n + 'a'));
        steppe_arr[n] = g_resmgr.get_object(&tmp);
    }
    for (n = 0; n < g_module_settings.tree_types[3]; n++) {
        tmp.assign("desert_tree_");
        tmp.push_back((char)((unsigned char)n + 'a'));
        desert_arr[n] = g_resmgr.get_object(&tmp);
    }

    // ---- grid dimensions and cell entities -----------------------------
    *(map_v2*)&g_script.sizex = map_v2(g_script.maxx - g_script.minx, g_script.maxy - g_script.miny);
    map_v2 step((g_script.sizex + 5.0f) * 0.125f, (g_script.sizey + 5.0f) * 0.125f);
    g_script.page_cols = (int)ceil(g_script.sizex / step.x);
    g_script.page_rows = (int)ceil(g_script.sizey / step.y);
    if (g_script.page_cols > 8) mb_error("Map size too big in x axis!");
    if (g_script.page_rows > 8) mb_error("Map size too big in x axis!");

    int row, col;
    for (row = 0; row < g_script.page_rows; row++) {
        for (col = 0; col < g_script.page_cols; col++) {
            rgl::entity* e = (rgl::entity*)g_script.level->create_entity(9);
            g_script.map_cell[col * 8 + row] = e;
            float zero = 0.0f;
            e->set_fbc(&zero);
        }
    }

    // ---- per-cell geometry ---------------------------------------------
    for (col = 0; col < g_script.page_cols; col++) {
        map_v2 lo, hi;
        lo.x = (float)col * step.x + g_script.minx;
        hi.x = (float)(col + 1) * step.x + g_script.minx;
        for (row = 0; row < g_script.page_rows; row++) {
            lo.y = (float)row * step.y + g_script.miny;
            hi.y = (float)(row + 1) * step.y + g_script.miny;

            rgl::mesh* cellmesh = new rgl::mesh();
            rgl::vector4 center;
            *(map_v2*)&center = map_v2((hi.x + lo.x) * 0.5f, (hi.y + lo.y) * 0.5f);
            center.z = 0.0f;
            int ci = col * 8 + row;

            for (int mi = 0; mi < 0xd; mi++) {
                int mt = mi;
                if (mi == 10)      mt = 0x15;
                else if (mi == 11) mt = 0x16;
                else if (mi == 12) mt = 0x17;
                unsigned umask = (unsigned)g_bit[mi];

                rgl::mesh* m = new rgl::mesh();
                g_script.map_cell[ci]->local.o = center;

                for (i = 0; i < g_script.level->ground->verts.size(); i++)
                    fr.vert[i] = -1;

                for (int ni = 0; ; ni++) {
                    map_ground* g = (map_ground*)g_script.level->ground;
                    if ((unsigned)ni >= g->ncells())
                        break;
                    NavCell& nd = g_script.level->ground->cells[ni];
                    const rgl::vector4& p0 = g->verts[nd.ids[0]].p;
                    if (!(lo.x <= p0.x && p0.x < hi.x && lo.y <= p0.y && p0.y < hi.y))
                        continue;

                    int nf[3];
                    for (int j = 0; j < nd.npts; j++)
                        nf[j] = fr.flags[nd.ids[j]];
                    int f0 = nf[0], f2 = nf[2], f1 = nf[1];

                    // ---- material / node-type filter ----
                    bool add = false;
                    if (mt == 0) {
                        if (nd.ids[8] == 0 || nd.ids[8] == 0xf)
                            add = true;
                    } else if (mt == 8) {
                        if (nd.ids[8] == 8 || nd.ids[8] == 7)
                            add = true;
                        else if (nd.ids[8] == 0 && ((water & f0) != 0 || (water & f1) != 0 || (water & f2) != 0))
                            add = true;
                    } else if (mt == 0x15) {
                        if (nd.ids[8] > 0 && nd.ids[8] != 8 && nd.ids[8] != 0xf && nd.ids[8] != 7 && nd.ids[8] != 1
                            && (fr.dist[nd.ids[0]] < 1.51f || fr.dist[nd.ids[1]] < 1.51f || fr.dist[nd.ids[2]] < 1.51f))
                            add = true;
                    } else if (mt == 0x16 || mt == 0x17) {
                        if (nd.ids[8] == 0 && (fr.dist[nd.ids[0]] < 0.01f || fr.dist[nd.ids[1]] < 0.01f || fr.dist[nd.ids[2]] < 0.01f))
                            add = true;
                    } else if (mt != 7 && mt != 0xf) {
                        if (mt == 9) {
                            if (nd.ids[8] != 8 && nd.ids[8] != 0 && ((wood & f0) != 0 || (wood & f1) != 0 || (wood & f2) != 0))
                                add = true;
                        } else {
                            if (nd.ids[8] != 8 && nd.ids[8] != 7 && nd.ids[8] != 0 && ((umask & f0) != 0 || (umask & f1) != 0 || (umask & f2) != 0))
                                add = true;
                        }
                    }
                    if (!add)
                        goto no_emit;
                    rgl::world* w = g_script.level;
                    for (int k = 0; k < nd.npts; k++) {
                        int& id = nd.ids[k];
                        if (fr.vert[id] != -1)
                            continue;
                        rgl::vector4 p = w->ground->verts[id].p;
                        const rgl::vector4& normal = w->ground->verts[id].n;

                        if (mt == 0x17) {
                            if ((sea & fr.flags[id]) == 0 || (land & fr.flags[id]) == 0) {
                                int chain[128];
                                int ncnt;
                                ((ChainGraph*)w->ground)->build_chain(id, ni, chain, &ncnt);   // 0x5234e0 (target vertex, start node)
                                int a = 0;
                                while (a < ncnt) {
                                    int f = fr.flags[chain[a]];
                                    if ((sea & f) != 0 && (land & f) != 0)
                                        a++;
                                    else
                                    {
                                        chain[a] = chain[ncnt - 1];
                                        ncnt--;
                                    }
                                }
                                if (ncnt > 0) {
                                    rgl::vector4 sum(0.0f, 0.0f, 0.0f);
                                    for (a = 0; a < ncnt; a++) {
                                        const rgl::vector4& q = g_script.level->ground->verts[chain[a]].p;
                                        rgl::vector4 d = p - q;
                                        float len = map_len(d);
                                        if (len > 1e-10f) {
                                            float inv = 1.0f / len;
                                            d.x *= inv;
                                            d.y = inv * d.y;
                                            d.z = inv * d.z;
                                        } else {
                                            d.x = 0.0f;
                                            d.y = 1.0f;
                                            d.z = 0.0f;
                                        }
                                        if (len > 0.25f) {
                                            rgl::vector4 r = q + d * 0.25f;
                                            sum.x += r.x;
                                            sum.y += r.y;
                                            sum.z += r.z;
                                        } else {
                                            sum.x += p.x;
                                            sum.y += p.y;
                                            sum.z += p.z;
                                        }
                                    }
                                    float inv = 1.0f / (float)ncnt;
                                    sum.x *= inv;
                                    sum.y = inv * sum.y;
                                    sum.z = inv * sum.z;
                                    p = sum;
                                }
                                p.z += 0.03f;
                            } else {
                                p.z += 0.01f;
                            }
                        }

                        rgl::vector4 rel(p.x - center.x, p.y - center.y, p.z);
                        int pidx = m->add_point(&rel);

                        // ---- per-material uv ----------
                        vec2 uv;
                        uv.x = p.x * 0.1f;
                        uv.y = p.y * 0.1f;
                        if (mt == 0) {
                            map_v2 q = *(const map_v2*)&p;
                            float l = q.length();
                            float nx_, ny_;
                            if (l > 0.0f) {
                                float inv = 1.0f / l;
                                nx_ = q.x * inv;
                                ny_ = inv * q.y;
                            } else {
                                nx_ = 0.0f;
                                ny_ = 1.0f;
                            }
                            uv.y = l * 0.1f;
                            uv.x = (float)atan2((double)-nx_, (double)ny_) * 10.0f;
                        } else if (mt == 8) {
                            float ca = (float)cos(river_ang), sa = (float)sin(river_ang);
                            rgl::vector4 nz;
                            float rx = ca * uv.x - sa * uv.y;
                            uv.y = ca * uv.y + sa * uv.x;
                            noise_fractal(6.0f, 0.7f, 3, 0, (const float*)&p, (float*)&nz);
                            rgl::vector4 t = nz * 0.5f;
                            uv.x = t.x + rx;
                            uv.y = t.y + uv.y;
                            uv.x *= 0.4f;
                            uv.y *= 0.4f;
                        } else if (mt == 0x16) {
                            float ca = (float)cos(sea_ang), sa = (float)sin(sea_ang);
                            rgl::vector4 nz;
                            float rx = ca * uv.x - sa * uv.y;
                            uv.y = ca * uv.y + sa * uv.x;
                            noise_fractal(6.0f, 0.7f, 3, 0, (const float*)&p, (float*)&nz);
                            rgl::vector4 t = nz * 0.7f;
                            uv.x = rx + t.x;
                            uv.y = t.y + uv.y;
                            uv.x *= 2.3f;
                            uv.y *= 2.3f;
                        } else if (mt == 0x17) {
                            float ca = (float)cos(sea_ang), sa = (float)sin(sea_ang);
                            rgl::vector4 nz;
                            float rx = ca * uv.x - sa * uv.y;
                            uv.y = ca * uv.y + sa * uv.x;
                            noise_fractal(6.0f, 0.7f, 3, 0, (const float*)&p, (float*)&nz);
                            rgl::vector4 t = nz * 0.3f;
                            uv.x = rx + t.x;
                            uv.y = t.y + uv.y;
                            uv.x *= 5.3f;
                            uv.y *= 5.3f;
                        } else if (mt == 2) {
                            uv.x *= 0.6f;
                            uv.y *= 0.6f;
                        }

                        // common per-vertex scale: clamp(point.level * 1.3 + 0.1, 1.0)
                        float sc = g_script.level->ground->verts[id].level * 1.3f + 0.1f;
                        if (sc > 1.0f)
                            sc = 1.0f;

                        // three fractal passes at the point; only out.x of each is used
                        rgl::vector4 no;
                        noise_fractal(9.0f, 0.6f, 4, 0, (const float*)&rgl::vector4(p.x, p.y, 6810.0f), (float*)&no);
                        float g0 = no.x * 0.5f;
                        noise_fractal(7.0f, 0.6f, 4, 0, (const float*)&rgl::vector4(p.x, p.y, 2917.0f), (float*)&no);
                        float h0 = no.x * 2.1f;
                        noise_fractal(4.0f, 0.6f, 4, 0, (const float*)&rgl::vector4(p.x, p.y, -917.0f), (float*)&no);

                        // ---- colour ----
                        rgl::vector4 c;
                        c.x = (g0 + 0.86f) * sc;
                        c.y = (h0 + 0.89f) * sc;
                        c.z = (no.x * 0.6f + 0.87f) * sc;
                        if (g_use_shaders == 0) {
                            // the fixed-function lighting factor per channel
                            rgl::vector4 lit = lightL + lightM * g_script.level->ground->verts[id].level * 1.1f;
                            c.x *= lit.x;
                            c.y *= lit.y;
                            c.z *= lit.z;
                        }
                        c.w = 1.0f;

                        if (mi == 0) {
                            float& dd = fr.dist[id];
                            float sd = (float)sqrt(dd);
                            if (g_use_shaders != 0) {
                                if (!(sd < 5.0f)) sd = 5.0f;
                                map_scale(c, (6.0f - sd) * 0.16666667f);
                            } else {
                                if (!(sd < 5.0f)) sd = 5.0f;
                                map_scale(c, (8.0f - sd) * 0.125f);
                            }
                        } else if (mt == 8) {
                            if ((nf[k] & water) == 0)
                                c.w = 0.0f;
                            c.x *= 0.7f;
                            c.y *= 0.7f;
                            c.z *= 0.7f;
                        } else if (mt == 0x15) {
                            const float& dd = fr.dist[id];
                            if (dd > 0.01f) {
                                c.x *= 0.85f;
                                c.y *= 0.85f;
                                c.z *= 0.85f;
                            }
                            float al = 1.0f - dd * 0.6666667f;
                            if (al > 0.0f)
                                c.w = al;
                            else
                                c.w = 0.0f;
                            if (nd.ids[8] == 0 && dd > 0.01f)
                                c.w = 0.0f;
                        } else if (mt == 0x16 || mt == 0x17) {
                            if (fr.dist[id] > 0.01f)
                                c.w = 0.0f;
                            else
                                c.w = (water & fr.flags[id]) != 0 ? 0.0f : 1.0f;
                        } else {
                            unsigned fl = nf[k];
                            if ((fl & umask) == 0)
                                c.w = 0.0f;
                            if (mt == 4) {
                                c.x = 0.9f;
                                c.y = 0.9f;
                                c.z = 0.9f;
                            } else if ((fl & water) != 0) {
                                c.x *= 0.65f;
                                c.y *= 0.85f;
                                c.z *= 0.7f;
                            }
                        }

                        if (g_use_shaders == 0) {
                            c.x = (float)sqrt(c.x);
                            c.y = (float)sqrt(c.y);
                            c.z = (float)sqrt(c.z);
                        }

                        fr.vert[id] = map_add_vertex(m, pidx, uv, rgl::pack_color(&c), normal, 0);
                        w = g_script.level;
                    }
                    m->add_face(fr.vert[nd.ids[0]], fr.vert[nd.ids[1]], fr.vert[nd.ids[2]]);

                no_emit:
                    // ---- tree-mesh selection, material 0 only ----
                    if (mi == 0) {
                        int t = nd.ids[8];
                        if (t == 0xb || t == 0xc || t == 10 || t == 0xd) {
                            rgl::mesh* chosen;
                            if (t == 0xc) {
                                chosen = snow_arr[g_module_settings.tree_types[1] <= 1 ? 0 : rand() % g_module_settings.tree_types[1]];
                            } else if (t == 10) {
                                chosen = steppe_arr[g_module_settings.tree_types[2] <= 1 ? 0 : rand() % g_module_settings.tree_types[2]];
                            } else if (t == 0xd) {
                                chosen = desert_arr[g_module_settings.tree_types[3] <= 1 ? 0 : rand() % g_module_settings.tree_types[3]];
                            } else {
                                chosen = map_arr[g_module_settings.tree_types[0] <= 1 ? 0 : rand() % g_module_settings.tree_types[0]];
                            }
                            if (chosen != 0) {
                                rgl::mat3 rot;
                                rgl::matrix frame = map_frame(*rgl::identity3(&rot), rgl::vector4(0.0f, 0.0f, 0.0f));
                                float ang = (float)(rand() % 0x3dc9) * 0.00039724255f;
                                frame.rotate_about_u(&ang);
                                // a random point of the node's first three vertices: weights t = r * 6.3223e-5 + 0.2
                                rgl::vector4 s(0.0f, 0.0f, 0.0f);
                                float tw = 0.0f;
                                for (int q = 0; q < 3; q++) {
                                    float w = (float)(rand() % 0x3dc9) * 6.3223117e-05f + 0.2f;
                                    const rgl::vector4& tp = g_script.level->ground->verts[nd.ids[q]].p;
                                    map_add(s, tp * w);
                                    tw += w;
                                }
                                float tinv = 1.0f / tw;
                                s.x *= tinv;
                                s.y *= tinv;
                                s.z *= tinv;
                                rgl::vector4 ofs;
                                ofs.x = s.x - center.x;
                                ofs.y = s.y - center.y;
                                ofs.z = s.z;
                                frame.o = ofs;
                                cellmesh->append_skinned(chosen, &frame, -1, 0xffffffff, 0); // 0x44eb00
                            }
                        }
                    }
                }

                // ---- finalize the per-material mesh ----
                if (m->v90.size() != 0) {
                    m->res = ((GameMapView*)this)->terrain_material(mt);
                    ((NibbleObj*)m)->set_nibble(mi - 6);
                    m->mask = 0x17fff;
                    m->compute_bounds();
                    add_cell_mesh(g_script.map_cell[ci], m);
                    if (mt == 0)
                        ((std::vector<rgl::mesh*>&)g_script.mat_type0).push_back(m);
                    else if (mt == 8)
                        ((std::vector<rgl::mesh*>&)g_script.mat_river).push_back(m);
                    else if (mt == 0x16)
                        ((std::vector<rgl::mesh*>&)g_script.mat_sea).push_back(m);
                    else if (mt == 0x17)
                        ((std::vector<rgl::mesh*>&)g_script.mat_sea2).push_back(m);
                } else {
                    delete m;
                }
            }

            // ---- finalize the cell mesh ----
            if (cellmesh->v90.size() != 0) {
                cellmesh->res = g_resmgr.get_material(RStr("map_trees"));
                ((NibbleObj*)cellmesh)->set_nibble(2);
                cellmesh->mask = 0x1ffff;
                cellmesh->compute_bounds();
                add_cell_mesh(g_script.map_cell[ci], cellmesh);
                ((GameMapView*)this)->ents.push_back(cellmesh);
            } else {
                delete cellmesh;
            }
        }
    }
}


// @ 0x523360
void NibbleObj::set_nibble(int v)
{
    if (this->f2c) {
        this->f2c->f1c &= 0xf0ffffff;
        if (v < 0)
            v += 0x10;
        this->f2c->f1c |= (unsigned)v << 24;
    }
}

// @ 0x523390
skin* GameMapView::terrain_material(int type)
{
    if (type == 0)
        return g_resmgr.get_material(RStr("map_ocean"));
    if (type == 1)
        return g_resmgr.get_material(RStr("map_mountain"));
    if (type == 3)
        return g_resmgr.get_material(RStr("map_plain"));
    if (type == 2)
        return g_resmgr.get_material(RStr("map_steppe"));
    if (type == 4)
        return g_resmgr.get_material(RStr("map_snow"));
    if (type == 5)
        return g_resmgr.get_material(RStr("map_desert"));
    if (type == 8)
        return g_resmgr.get_material(RStr("map_river"));
    if (type == 9)
        return g_resmgr.get_material(RStr("map_mountain_forest"));
    if (type == 7)
        return g_resmgr.get_material(RStr("map_bridge"));
    if (type == 0x15)
        return g_resmgr.get_material(RStr("map_shore"));
    if (type == 0x16)
        return g_resmgr.get_material(RStr("map_foam"));
    if (type == 0x17)
        return g_resmgr.get_material(RStr("map_reverse_foam"));
    return g_resmgr.get_material(RStr("default"));
}

// @ 0x5234e0
// Follows the 0x24-node chain from `start`, picking (at each step) the last neighbour that links to `target`, is not the
// node we came from, and appends the far end of that link once.
void ChainGraph::build_chain(int target, int start, int* out, int* count)
{
    int cur = start;
    int prev = -1;
    int next;
    *count = 0;
    do {
        if (cur < 0)                    // the neighbour lists can carry -1; rec at f14 - 0x88 faults (0x49d47a)
            return;
        Rec88* rec = (Rec88*)((char*)this->f14 + cur * 0x88);
        int n = rec->count;
        int found = -1;
        if (n <= 0)
            return;
        int* nb = (int*)((char*)rec + 0x14);
        do {
            int v = *nb;
            if (v < 0) {                // same for a neighbour id: node at f24 - 0x24 faulted with v = -1
                nb++;
                continue;
            }
            Node24* node = (Node24*)((char*)this->f24 + v * 0x24);
            if (node->f00 > 1 && node->f04 != prev && node->f08 != prev
                && (node->f0c == target || node->f10 == target))
                found = v;
            nb++;
        } while (--n);
        if (found < 0)
            return;
        Node24* node = (Node24*)((char*)this->f24 + found * 0x24);
        next = (node->f04 == cur) ? node->f08 : node->f04;
        // (no early-out on next < 0: the original appends first; the loop-top cur < 0 guard stops the walk)
        int c = *count;
        bool dup = false;
        if (node->f0c == target) {
            for (int i = 0; i < c; i++)
                if (out[i] == node->f10)
                    dup = true;
            if (!dup) {
                out[c] = node->f10;
                (*count)++;
            }
        } else {
            for (int i = 0; i < *count; i++)
                if (out[i] == node->f0c)
                    dup = true;
            if (!dup) {
                out[c] = node->f0c;
                (*count)++;
            }
        }
        prev = cur;
        cur = next;
    } while (next != start);
}

// @ 0x523630
void GameMapView::scan_arrays(void* p, void* unused)
{
    unsigned i;
    for (i = 0; i < g_script.level->ground->verts.size(); i++)
        *(volatile int*)((char*)p + 0xc3500) = (int)0xcb189680;
    char buf[100000];
    for (i = 0; i < g_script.level->ground->cells.size(); i++)
        buf[i] = 0;
    for (i = 0; i < g_script.level->ground->cells.size(); i++)
        ;
}

// ---- Game::load_game_data and the record helpers that follow it (binary order) -----------------------------------------

// @ 0x5236f0
void Game::load_game_data(int extra_triggers)
{
    tstring str1;                                        // +0x38
    tstring str2;                                        // +0x28

    // ---- factions.txt -----------------------------------------------------------------------------------
    tstream3 s_factions;                                // +0x80
    {
        const char* path = MODULE_PATH("factions.txt");
        if (!OPEN_READ_TEXT(s_factions, path))
            mb_error("Unable to open %s", path);
    }
    stream_base& in_factions = s_factions;
    in_factions.r_tstr(RSTR(&str1));
    in_factions.r_tstr(RSTR(&str2));
    int version;
    in_factions.r_int(&version);
    int& nf = nfactions;
    in_factions.r_int(&nf);
    if (nf > 0x80)
        mb_error("Too Many Factions: %d (Max should be %d)", nf, 0x80);
    delete[] (FactionFile*)factions;
    factions = 0;
    factions = (Faction*)new FactionFile[nf];
    for (int i = 0; i < nf; i++)
        ((FactionFile*)factions)[i].read_file(in_factions, nf);
    s_factions.failed = 1;

    // ---- skills.txt -------------------------------------------------------------------------------------
    tstream3 s_skills;                                  // +0x198
    {
        const char* path = MODULE_PATH("skills.txt");
        if (!OPEN_READ_TEXT(s_skills, path))
            mb_error("Unable to open %s", path);
    }
    stream_base& in_skills = s_skills;
    int skill_count;
    in_skills.r_int(&skill_count);
    int i;
    for (i = 0; i < (skill_count > 42 ? 42 : skill_count); i++)
        read_skill(in_skills, g_skill_tbl[i]);
    for (; i < 42; i++) {
        g_skill_tbl[i].max_rank = 0;
        g_skill_tbl[i].cap = 0;
        g_skill_tbl[i].attr = 0x100;
    }
    s_skills.failed = 1;

    // ---- Data\skeleton_bodies.xml -----------------------------------------------------------------------
    {
        tstring path("Data\\skeleton_bodies.xml");
        ((SkeletonBody*)&g_skeletons)->load(path);
    }
    fix_skeletons(&g_skeletons);

    // ---- troops.txt -------------------------------------------------------------------------------------
    tstream3 s_troops;                                  // +0xb8
    {
        const char* path = MODULE_PATH("troops.txt");
        if (!OPEN_READ_TEXT(s_troops, path))
            mb_error("Unable to open %s", path);
    }
    stream_base& in_troops = s_troops;
    in_troops.r_tstr(RSTR(&str1));
    in_troops.r_tstr(RSTR(&str2));
    in_troops.r_int(&version);
    int& c_troops = ntroops;
    in_troops.r_int(&c_troops);
    delete[] (TroopFile*)troops;
    troops = 0;
    troops = (Troop*)new TroopFile[c_troops];
    for (int j = 0; j < ntroops; j++)
        ((TroopFile*)troops)[j].read_file(in_troops);
    s_troops.failed = 1;

    // ---- scenes.txt -------------------------------------------------------------------------------------
    tstream3 s_scenes;                                  // +0xf0
    {
        const char* path = MODULE_PATH("scenes.txt");
        if (!OPEN_READ_TEXT(s_scenes, path))
            mb_error("Unable to open %s", path);
    }
    stream_base& in_scenes = s_scenes;
    in_scenes.r_tstr(RSTR(&str1));
    in_scenes.r_tstr(RSTR(&str2));
    in_scenes.r_int(&version);
    int& c_scenes = nparties;
    in_scenes.r_int(&c_scenes);
    delete[] (SceneFile*)parties;
    parties = 0;
    parties = (char*)new SceneFile[c_scenes];
    for (int k = 0; k < nparties; k++)
        ((SceneFile*)parties)[k].read_file(in_scenes);
    s_scenes.failed = 1;

    // ---- quests.txt -------------------------------------------------------------------------------------
    tstream3 s_quests;                                  // +0x48
    {
        const char* path = MODULE_PATH("quests.txt");
        if (!OPEN_READ_TEXT(s_quests, path))
            mb_error("Unable to open %s", path);
    }
    stream_base& in_quests = s_quests;
    in_quests.r_tstr(RSTR(&str1));
    in_quests.r_tstr(RSTR(&str2));
    in_quests.r_int(&version);
    int& c_quests = nquests;
    in_quests.r_int(&c_quests);
    delete[] (QuestFile*)quests;
    quests = 0;
    quests = (QuestRec*)new QuestFile[c_quests];
    for (int n = 0; n < nquests; n++)
        ((QuestFile*)quests)[n].read_file(in_quests);
    s_quests.failed = 1;

    // ---- triggers.txt -----------------------------------------------------------------------------------
    tstream3 s_triggers;                                // +0x160
    {
        const char* path = MODULE_PATH("triggers.txt");
        if (!OPEN_READ_TEXT(s_triggers, path))
            mb_error("Unable to open %s", path);
    }
    stream_base& in_triggers = s_triggers;
    in_triggers.r_tstr(RSTR(&str1));
    in_triggers.r_tstr(RSTR(&str2));
    in_triggers.r_int(&version);
    ((TriggerVec*)&triggers)->resize(in_triggers, "Game Triggers");
    s_triggers.failed = 1;

    // ---- simple_triggers.txt ----------------------------------------------------------------------------
    tstream3 s_simple;                                  // +0x128
    {
        const char* path = MODULE_PATH("simple_triggers.txt");
        if (!OPEN_READ_TEXT(s_simple, path))
            mb_error("Unable to open %s", path);
    }
    stream_base& in_simple = s_simple;
    in_simple.r_tstr(RSTR(&str1));
    in_simple.r_tstr(RSTR(&str2));
    in_simple.r_int(&version);
    int first = ((GameVec*)&simple)->resize(in_simple, "Simple trigger", extra_triggers);
    s_simple.failed = 1;

    // one extra simple trigger per script emitter whose delay is non-negative
    for (int idx = 0; idx < g_scripts.count; idx++) {
        int off = first * 0x30;
        if (((rec34*)g_scripts.records)[idx].f14 >= 0.0f) {
            SimpleTrigger* rec = (SimpleTrigger*)((char*)simple.array + off);
            rec->f0 = *(int*)&((rec34*)g_scripts.records)[idx].f14;
            rec->stmt.init(1, idx);
            first++;
        }
    }

    // ---- combat parameter records (six records of four floats at +0x13b64) ------------------------------
    float* c = (float*)recs;
    *(map_v2*)&c[2] = map_v2(3.0f, 3.0f);                // record 0 tail
    *(map_v2*)&c[16] = map_v2(-100.0f, -50.0f);          // record 4 head
    *(map_v2*)&c[6] = map_v2(5.0f, 1.0f);                // record 1 tail
    *(map_v2*)&c[18] = map_v2(100.0f, -50.0f);           // record 4 tail
    *(map_v2*)&c[10] = map_v2(1.0f, -3.0f);              // record 2 tail
    *(map_v2*)&c[20] = map_v2(-100.0f, 50.0f);           // record 5 head
    *(map_v2*)&c[14] = map_v2(-5.0f, -3.0f);             // record 3 tail
    c[0] = 23.0f;     c[1] = 0.2f;                       // record 0 head
    c[4] = 15.0f;     c[5] = 0.1f;                       // record 1 head
    c[8] = 11.0f;     c[9] = 0.9f;                       // record 2 head
    c[12] = 3.0f;     c[13] = 0.3f;                      // record 3 head
    *(map_v2*)&c[22] = map_v2(100.0f, 50.0f);            // record 5 tail
}

// the engine's fast float floor: bias non-positive values by (1 - 2^-24), then truncate
static int ftoi_floor(float t)
{
    if (!(t > 0.0f))
        t -= 0.99999988f;
    return (int)t;
}

// @ 0x524360
void TriggerStmt::init(int op, int idx)
{
    flag = 0;
    if (p)
        delete[] p;
    p = new StmtRec[1];
    p->kind = op;
    p->f08 = 1;
    p->f10 = idx;
    n = 1;
}

// @ 0x5243c0
stream_base& read_skill(stream_base& s, SkillRec& r)
{
    s.r_tstr(RSTR(&r.key));
    s.r_tstr(RSTR(&r.name)).r_int((int*)&r.attr).r_int(&r.max_rank).r_tstr(RSTR(&r.desc));
    r.name.underscores_to_spaces();
    r.desc.underscores_to_spaces();
    r.cap = r.max_rank;
    return s;
}

// @ 0x524420
void TroopFile::read_file(stream_base& s)
{
    int n;
    s.r_tstr(RSTR(&f20));
    s.r_tstr(RSTR(&f0));
    f0.underscores_to_spaces();
    s.r_tstr(RSTR(&f10));
    f10.underscores_to_spaces();
    s.r_u64(&u30);
    s.r_int(&f3b8);
    s.r_int(&f38);
    s.r_int(&f4c);
    s.r_int(&f50);
    s.r_int(&f54);
    int* pa = &pairs[0].item;
    for (int i = 0; i < 0x40; ++i, pa += 2) {
        s.r_int(pa);
        s.r_int(pa + 1);
    }
    for (int i = 0; i < 10; ++i) {
        p10[i].item = -1;
        p10[i].m4 = 0;
    }
    for (int i = 0; i < 4; ++i)
        s.r_int(&f58[i]);

    s.r_int(&n);
    if (n < 1)
        n = 1;
    fb0 = n;
    int base = g_level_xp[n];
    float t = (float)(rand() % 0x3dc9) * (float)(g_level_xp[n + 1] - base) * (1.0f / 0x3dc9);
    f9c = base + ftoi_floor(t);

    for (int i = 0; i < 7; ++i) {
        int v;
        s.r_int(&v);
        f80[i] = (float)v;
    }
    for (int i = 0; i < 6; ++i)
        s.r_uint(&f68[i]);

    int b = (f68[4] >> 16) & 0xf;
    if (b < 0)
        b = 0;
    if (b > (*(int*)((char*)&g_skill_tbl + 0x9c4)))
        b = (*(int*)((char*)&g_skill_tbl + 0x9c4));
    fa0 = (float)(f58[0] + 0x23 + b * 2);

    for (int i = 0; i < 4; ++i)
        s.r_u64(&u3c0[i]);
    for (int i = 0; i < 4; ++i)
        s.r_u64(&u3e0[i]);
}

// @ 0x524690
__declspec(noinline) void QuestFile::read_file(stream_base& s)
{
    s.r_tstr(RSTR(&f54));
    s.r_tstr(RSTR(&f0));
    s.r_int(&f50);
    s.r_tstr(RSTR(&f10));
    f0.underscores_to_spaces();
    f10.underscores_to_spaces();
    f30 = *((QuestRec*)this)->raw_title();       // QuestRec (include/game_recs.h) is this record: raw_title 0x48d750
    tstring* p = g_strtable.find(&f54);
    f40 = p ? *p : f0;
}

// @ 0x524710
TroopFile::TroopFile()
{
    for (int i = 0; i < 0x60; ++i)
        ((int*)pairs)[i * 2] = -1;
    f4a0 = -1;
    for (int i = 0; i < 10; ++i) {
        p10[i].item = -1;
        p10[i].m4 = 0;
    }
    u30 = 0;
    f3b8 = -1;
    f48 = 0;
    f44 = 0;
    f3b4 = 0;
    f9c = 0;
    fa4 = 0;
    fac = 0;
    fa8 = 0;
    fb0 = 1;
    for (int i = 0; i < 4; ++i)
        f58[i] = 0;
    for (int i = 0; i < 7; ++i)
        f80[i] = 0;
}

// @ 0x524860
QuestFile::QuestFile()
{
    f108 = -1;
    f50 = 0;
    f64 = 0;
    f11c = 0;
    f118 = -1;
}

// @ 0x5248e0
SceneFile::SceneFile()
{
    loaded = 0;
    for (int i = 0; i < 0x40; ++i) {
        pairs114[i].item = -1;
        ints314[i] = 0;
    }
}

// @ 0x431e10
SceneFile::~SceneFile() {}

// @ 0x5249f0
FactionFile::FactionFile()
{
    f2fc = -1;
    f230 = 0;
    b10 = 0;
    f244 = -1.0f;
    f248 = 1.0f;
}

// @ 0x524a70
void FactionFile::read_file(stream_base& s, int count)
{
    s.r_tstr(RSTR(&f14));
    s.r_tstr(RSTR(&name));
    name.underscores_to_spaces();
    s.r_int(&f24);
    s.r_uint(&color);
    color |= 0xff000000;
    f248 = (float)(100 - ((f24 >> 8) & 0xff)) * 0.01f;

    if (count > 0) {
        float* p = arr;
        for (int i = count; i > 0; --i) {
            s.r_float(p);
            ++p;
        }
    }
    s.r_int(&f22c);
    if (f22c != 0) {
        tstring tmp;
        v.reserve(f22c);
        for (int i = 0; i < f22c; ++i) {
            s.r_tstr(RSTR(&tmp));
            v.push_back(tmp);
        }
    }
}

// @ 0x524bc0
void tstring_vec::reserve(unsigned n)
{
    if (n > 0x0fffffff)
        VEC_XLEN_CALL(this);
    if (capacity() >= n)
        return;
    tstring* p = (tstring*)::operator new(n * 0x10);
    try {
        ptr_tag t;
        uninit_copy_str(_Myfirst, _Mylast, p, *(tstring_alloc*)this, t);
    }
    catch (...) {
        ::operator delete(p);
        throw;
    }
    unsigned s = _Myfirst == 0 ? 0 : (unsigned)(_Mylast - _Myfirst);
    if (_Myfirst) {
        _Destroy(_Myfirst, _Mylast);
        ::operator delete(_Myfirst);
    }
    _Myend = p + n;
    _Mylast = p + s;
    _Myfirst = p;
}

// @ 0x524cd0
tstring* __cdecl uninit_copy_str(tstring* f, tstring* l, tstring* d, tstring_alloc& al, ptr_tag tag)
{
    tstring* s = d;
    try {
        for (; f != l; ++d, ++f)
            ((alloc_tstring*)&al)->construct(d, *f);
    }
    catch (...) {
        for (; s != d; ++s)
            s->~tstring();
        throw;
    }
    return d;
}

// tstring's character accessor: falls back to the buf field's own address when empty
static inline const char* cstr(const tstring& t)
{
    return t.buf ? t.buf : (const char*)&t.buf;
}

// @ 0x524d60
void SceneFile::read_file(stream_base& s)
{
    for (int i = 0; i < 0x40; ++i) {
        pairs114[i].item = -1;
        ints314[i] = 0;
    }
    s.r_tstr(RSTR(&name10));
    s.r_tstr(RSTR(&name));
    s.r_int(&f30);
    s.r_tstr(RSTR(&f34));
    s.r_tstr(RSTR(&f44));
    s.r_float(&f54).r_float(&f58);
    s.r_float(&f5c).r_float(&f60);
    s.r_float(&f64);
    s.r_tstr(RSTR(&f68));
    s.r_int(&fbc);
    if (fbc > 0x10)
        mb_error("Number of passages for site %s exceeds %d.", cstr(name10), 0x10);
    for (int i = 0; i < fbc; ++i)
        s.r_int(&f7c[i]);
    s.r_int(&f100);
    for (int i = 0; i < f100; ++i)
        if (i < 0x10)
            s.r_int(&fc0[i]);
    s.r_tstr(RSTR(&f484));
    Local tmp;
    tmp.read(s444);
}

// @ 0x524f00
void Local::read(stream_base& s)
{
    int n0 = (int)size0();
    s.w_uint((unsigned*)&n0);
    for (unsigned i = 0; i < size0(); ++i)
        s.w_float((float*)&v0_first[i].a).w_float((float*)&v0_first[i].b).w_float((float*)&v0_first[i].c);

    int n1 = (int)size1();
    s.w_uint((unsigned*)&n1);
    for (unsigned i = 0; i < size1(); ++i) {
        V1& e = v1_first[i];
        s.w_int(&e.p4);
        s.w_int(&e.p0);
        s.w_int(&e.p1);
        s.w_int(&e.p2);
        s.w_int(&e.p3);
    }

    int n2 = (int)size2();
    s.w_uint((unsigned*)&n2);
    for (unsigned i = 0; i < size2(); ++i) {
        Rec64& e = v2_first[i];
        s.w_int(&e.n);
        for (int k = 0; k < e.n; ++k)
            s.w_int(&e.a[k]);
        for (int k = 0; k < e.n; ++k)
            s.w_int(&e.b[k]);
        s.w_int(&e.c);
    }
}
