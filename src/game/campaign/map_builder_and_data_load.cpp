//-----------------------------------------------------------------------------
// File: map_builder_and_data_load.cpp  (0x520b20..0x525160)
//
// Desc: the map and world-map scene builders (map.txt reader, the party / compass / track entities, the campaign map's
//       tree meshes), the module-file loader Game::load_game_data (factions, skills, skeleton bodies, troops, scenes,
//       quests, triggers, simple triggers) and the record constructors / readers / vector helpers it calls (all of
//       them follow Game::load_game_data in the binary: caller first, "T regime"). Names provisional. The file keeps
//       the binary order; helpers that follow their caller are __declspec(noinline) where the original calls them.
//       Flags: /O2 /MT /Gy /TP.
//
// Boundary: class change only at 0x520b20 (Troop | Game); the map's next boundary (0x5249f0, QuestRec | Faction) is NOT
//       kept: 0x5236f0 calls 0x524360..0x524d60 in turn and they are its record helpers (see MIGRATION.md).
//-----------------------------------------------------------------------------
#include <math.h>
#include "engine/core/layout32.h"
#include <string.h>
#include "game/campaign/map_builder_and_data_load.h"
#include "game/data/env_row.h"                // EnvRow, g_env_rows (0x5e28f4)
#include "game/data/troop.h"          // g_level_xp
#include "game/data/trigger_vecs.h"           // (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "game/campaign/game_state_load.h"
#include "game/campaign/externs.h"
#include "engine/core/tstring.h"
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "engine/core/rgl_string.h"
#include "engine/scene/scene.h"
#include "engine/io/stream.h"
#include "engine/core/ptr_tag.h"
#include "game/terrain/cell_grid.h"
#include "engine/render/mesh.h"
#include "engine/scene/scene_entity.h"
#include "engine/resource/resource.h"
#include "engine/math/rgl_math.h"
#include "game/common/iface.h"
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)
#include "engine/render/render_flags.h"
#include "platform/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers

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


// ---- the map object (0xa732b8): the ground geometry NavData (game/terrain/nav_data.h: constructor 0x478150, build 0x47ee60;
// its vectors' resize 0x424740 / 0x424cc0 and the cell constructor 0x4246f0 are engine/physics/world_grid.cpp's) ---------
stream_base* read_map(stream_base* s, NavData* map);            // 0x520c00

// @ 0x520b20
void Game::build_map()
{
    if (!g_script.new_game_done) {
        g_script.nav = new NavData();
        tstream3 s;
        const char* path = MODULE_PATH("map.txt");
        if (!OPEN_READ_TEXT(s, path))
            mb_error("Unable to open %s", path);
        if (!s.failed) {
            read_map(&s, g_script.nav);
            s.failed = 1;
        }
    }
}

// @ 0x520c00
__declspec(noinline) stream_base* read_map(stream_base* s, NavData* map)
{
    int n;
    s->r_int(&n);
    map->verts.resize(n, NavVert());
    int i;
    for (i = 0; i < n; i++) {
        NavVert& v = map->verts[i];
        s->r_float(&v.p.x).r_float(&v.p.y).r_float(&v.p.z);
        map->verts[i].level = -1.0f;
    }
    int n2;
    s->r_int(&n2);
    // link[] (+0x28) and the weights (+0x38) stay as the group constructor left them: NavData::build fills link[k]
    // for every k < n, and nothing reads k >= n (score_triggers 0x51dac0 and solve 0x51d3a0 stop at count).
    map->cells.resize(n2, NavCell());
    for (i = 0; i < n2; i++) {
        s->r_int(&map->cells[i].mask());
        s->r_int(&map->cells[i].f68);
        s->r_int(&map->cells[i].npts);
        for (int j = 0; j < map->cells[i].npts; j++)
            s->r_int(&map->cells[i].verts()[j]);
    }
    map->build();
    return s;
}

static inline int map_first_sprite(Pool<Event>& t)
{
    int i;
    for (i = 0; i < t.count; i++)
        if (t.atu(i)->in_use != 0)
            break;
    return i;
}
static inline void map_set_fbc(rgl::entity* e, const float& f) { e->set_fbc(&f); }

// @ 0x520d60
void Game::build_scene()
{
    Game* self = this;
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
        self->fog1 = *self->sun_color(&tmp);
    }
    log_stream_ptr()->w_cstr("get_ideal_sun_color.\n");
    {
        rgl::vector4 tmp;
        self->fog2 = *self->fog_color(&tmp);
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

    Light* wv;
    {
        rgl::world* world = g_script.level;
        wv = new Light;                                        // the 0x7c-byte wind object (constructor 0x461480)
        world->lights.push_back(wv);                           // the pointer vectors share one _Insert_n (0x410610)
    }
    Light* w = wv;
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
    self->track_entity->add_mesh(self->map_track);
    WL_MARKER(13)

    rgl::entity* obj = (rgl::entity*)g_script.level->create_entity(9);
    self->map_obj = ((rgl::mesh*)g_resmgr.get_object(&g_env_rows->name))->clone();
    self->map_obj->scale(60.0f);
    self->map_obj->mask = 0xfffeffff;
    obj->add_mesh(self->map_obj);
    map_set_fbc(obj, 100000000.0f);
    g_script.level->sky = (rgl::entity*)obj;

    self->compass = (rgl::entity*)g_script.level->create_entity(9);
    rgl::mesh* mc = ((rgl::mesh*)g_resmgr.get_object(&RStr("compass")))->clone();
    mc->scale(0.28f);
    self->compass->add_mesh((rgl::mesh*)mc);

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
        unsigned id = (unsigned)parties.first_party_id();
        while (id < (unsigned)parties.serial) {
            self->add_order_arrow((int)id);
            parties.next_party_id((int*)&id);
        }
    }
    log_stream_ptr()->w_cstr("parties added.\n");

    {
        unsigned idx = map_first_sprite(events);
        while (idx < (unsigned)events.count) {
            self->spawn_battle_icon(idx);
            event_pool(events).next((int*)&idx);
        }
    }
    release_orders();
    more_start();
    if (f13a34 == 0) {
        self->emit_event(0, 0, 0);
        f13a34 = 1;
    }
}

// @ 0x5213a0
__declspec(noinline) void Game::release_orders()
{
    std::vector<TrackRec>::iterator p = tracks.begin();
    if (p < tracks.end()) {
        do {
            init_event(&*p);
            p++;
        } while (p < tracks.end());
    }
}

// Bodies the original's object saw (header inlines left out of line; row owners collision_shapes.cpp 0x40e440, 0x40e770 and particle_system.cpp 0x413c50):
// an object whose address only reaches a visible body doesn't escape (docs/codegen.md section 7)
namespace rgl {
static inline unsigned min255(unsigned v) { return v < 255 ? v : 255; }
inline __declspec(noinline) unsigned pack_color(const vector4* c)
{
    unsigned a = (unsigned)(c->w * 256.0f);
    unsigned r = (unsigned)(c->x * 256.0f);
    unsigned g = (unsigned)(c->y * 256.0f);
    unsigned b = (unsigned)(c->z * 256.0f);
    return (((min255(a) << 8 | min255(r)) << 8) | min255(g)) << 8 | min255(b);
}
static __forceinline vector4 ru_mul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 ru_add(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 ru_sub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
inline __declspec(noinline) void mat3::rotate_about_u(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 ns, nf;
    ns = ru_add(ru_mul(s, cos((double)*angle)), ru_mul(f, sn));
    nf = ru_sub(ru_mul(f, cos((double)*angle)), ru_mul(s, sn));
    s = ns;
    f = nf;
}
inline __declspec(noinline) int mesh::add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c)
{
    ensure_local();
    int n = (int)vertices.size();
    vertices.resize(n + 1);
    (&*vertices.begin())[n].index = index;
    (&*vertices.begin())[n].uv = *uv;
    (&*vertices.begin())[n].color = *color;
    (&*vertices.begin())[n].f5c = *f5c;
    vertex60* vv = &vertices[n];
    vv->normal = *pos;
    vv->pos = *pos;
    return n;
}
inline __declspec(noinline) int mesh::add_point(const vector4* p)
{
    ensure_local();
    int n = (int)points.size();
    points.resize(n + 1);
    points[n].a = *p;
    points[n].b = *p;
    return n;
}
}

// ---- helpers of the campaign map's tree builder (above its marker) ---------------------------------------------------
struct map_v2 {                                                     // the two-float value the map's bounds are assigned from
    float x, y;
    map_v2() {}
    map_v2(float a, float b) : x(a), y(b) {}
    float length() const { float l = x * x; l += y * y; return (float)sqrt(l); }
};
static inline float map_len(const rgl::vector4& v) { float l = v.x * v.x; l += v.z * v.z; l += v.y * v.y; return (float)sqrt(l); }
static inline map_v2 map_xy(const rgl::vector4& p) { return map_v2(p.x, p.y); }
static inline int map_add_vertex(rgl::mesh* m, int index, const vec2& uv, const unsigned& color, const rgl::vector4& n, const int& f5c)
{
    return m->add_vertex(index, &uv, &color, &n, &f5c);
}
static inline void map_add(rgl::vector4& a, const rgl::vector4& b) { a.x += b.x; a.y += b.y; a.z += b.z; }
static inline void map_mul(rgl::vector4& a, const rgl::vector4& b) { a.x *= b.x; a.y *= b.y; a.z *= b.z; }   // per-channel product: lit.z stays on the x87 stack
// vector4::normalize (0x407d60) as the original inlines it here: returns the old length, (0,1,0) if <= 1e-10. As a member
// (x *= inv through this) it gives the original's fld inv; fmul d.y; with d's fields scaled in the caller cl loads d.y first.
// The length sums x, z, y (map_len): the header's length() order moves the fsqrt operand shuffle.
struct map_vn : rgl::vector4 {
    __forceinline float normalize_inl()
    {
        float len = map_len(*this);
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            x *= inv; y *= inv; z *= inv;
        } else {
            x = 0.0f; y = 1.0f; z = 0.0f;
        }
        return len;
    }
};
// the uv rotation as a member (through this): cl evaluates sa * x before ca * y like the original; written in the
// caller, the y sum's operands are canonicalised to ca * y first whatever the source order
struct map_uv : vec2 {
    __forceinline float rot(float ca, float sa) { float rx = ca * x - sa * y; y = ca * y + sa * x; return rx; }
    // the river's noise offset (t.x + rx in the original's operand order; the sea arms add in the caller)
    __forceinline void add(float rx, const rgl::vector4& t) { x = rx + t.x; y = t.y + y; }
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
// the cell's mesh list and its recalculation (the same steps as rgl::entity::add_mesh 0x442110, expanded here)
static inline void add_cell_mesh(rgl::entity* e, rgl::mesh* m)
{
    e->meshes.push_back(m);
    e->recalc();
}

// @ 0x5213d0
void Game::build_map_trees()
{
    g_script.mat_type0.clear();
    g_script.mat_river.clear();
    g_script.mat_sea.clear();
    g_script.mat_sea2.clear();

    float sea_ang = g_module_settings.map_sea_direction * 3.1415927f;                       // deg -> rad
    sea_ang *= 0.0055555557f;
    float river_ang = g_module_settings.map_river_direction * 3.1415927f;
    river_ang *= 0.0055555557f;

    // the lighting vectors the fixed-function vertex colour uses
    rgl::vector4 lightL, lightM;
    rgl::world* lvl;     // re-read at the end of each pass of the dot loop: the original keeps it in edi and reloads it at the latch
    {
        const rgl::vector4& sun_dir = wind->dir;
        rgl::vector4 dir;
        dir.x = -sun_dir.x;
        dir.y = -sun_dir.y;
        dir.z = -sun_dir.z;
        dir.normalize();                                                 // 0x407d60

        lightL = g_script.level->ambient;
        lightM = g_script.level->lights[0]->color;
        lvl = g_script.level;

        // ---- per-point view dot --------------------------------------------
        for (unsigned i = 0; i < lvl->ground->verts.size(); i++) {
            NavVert& v = lvl->ground->verts[i];
            float s = v.n.y * dir.y;
            s += v.n.z * dir.z;
            s += v.n.x * dir.x;
            v.level = s;
            if (g_use_shaders != 0)
                v.level = s * s;
            lvl = g_script.level;
        }
    }

    // ---- per-point flags and distance ----------------------------------
    // 100000 entries each; the exe reserves 4 MB of stack (/STACK:0x3e8480), the same as the relink.
    map_arrays fr;

    NavData* ground = lvl->ground;
    // each loop has its own counter: the original keeps the edge loop's and the cells loop's counters in different slots
    for (unsigned i = 0; i < ground->verts.size(); i++) {
        fr.flags[i] = 0;
        fr.dist[i] = 1000.0f;
    }
    int ni;              // one counter for this loop and the per-cell node loop below: the original gives both one slot
    for (ni = 0; (unsigned)ni < ground->cells.size(); ni++) {
        NavCell& c = ground->cells[ni];
        for (int k = 0; k <= 9; k++) {
            int kind = c.kind & 7;
            int t = c.kind;
            if (t == 8)
                kind = t;
            else {
                int t2 = c.kind;
                if (t2 == 9)
                    kind = t2;
                else if (t2 == 0xf)
                    kind = 0;
            }
            if (kind == k) {
                unsigned mask = (unsigned)g_bit[k];
                for (int j = 0; j < c.npts; j++) {
                    int& f = fr.flags[c.vidx[j]];
                    f |= mask;
                }
            }
        }
    }

    // ---- bbox + aggregate masks ----------------------------------------
    // the bounds are reset before the masks are formed: the original interleaves these stores with the mask loads
    g_script.minx = 100000.0f;
    g_script.miny = 100000.0f;
    g_script.maxx = -100000.0f;
    g_script.maxy = -100000.0f;
    unsigned water = (unsigned)(g_bit[8] | g_bit[7]);
    unsigned land = (unsigned)(g_bit[4] | g_bit[5] | g_bit[11] | g_bit[9] | g_bit[12] | g_bit[13] | g_bit[10] | g_bit[2] | g_bit[3] | g_bit[1]);
    unsigned sea = (unsigned)g_bit[0];
    unsigned wood = (unsigned)g_bit[9];
    scan_arrays(fr.scan_a, fr.scan_b);

    for (unsigned i = 0; i < g_script.level->ground->verts.size(); i++) {
        rgl::vector4& p = g_script.level->ground->verts[i].p;
        if (p.x < g_script.minx) g_script.minx = p.x;
        if (p.x > g_script.maxx) g_script.maxx = p.x;
        if (p.y < g_script.miny) g_script.miny = p.y;
        if (p.y > g_script.maxy) g_script.maxy = p.y;
        if ((sea & fr.flags[i]) != 0 && (land & fr.flags[i]) != 0)
            fr.dist[i] = 0.0f;
    }

    // ---- ten relaxation passes over the edges --------------------------
    NavData* g2 = g_script.level->ground;
    for (int pass = 10; pass != 0; pass--) {
        for (unsigned i = 0; i < g2->edges.size(); i++) {
            int a = g2->edges[i].a;
            int b = g2->edges[i].b;
            // the edge endpoints are read from g_nav's vertex vector (map.txt's store), not the ground's
            std::vector<NavVert>& nv = g_script.nav->verts;
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
    g_script.sizex = g_script.maxx - g_script.minx;
    g_script.sizey = g_script.maxy - g_script.miny;
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

            rgl::mesh* cmv = new rgl::mesh();
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

                rgl::mesh* mv = new rgl::mesh();               // pushed onto the std::vector<rgl::mesh*> material lists (_Insert_n 0x410610)
                g_script.map_cell[ci]->local.o = center;

                for (unsigned i = 0; i < g_script.level->ground->verts.size(); i++)
                    fr.vert[i] = -1;

                for (ni = 0; ; ni++) {
                    NavData* g = g_script.level->ground;
                    if ((unsigned)ni >= g->cells.size())
                        break;
                    NavCell& nd = g_script.level->ground->cells[ni];
                    const rgl::vector4& p0 = g->verts[nd.vidx[0]].p;
                    if (!(lo.x <= p0.x && p0.x < hi.x && lo.y <= p0.y && p0.y < hi.y))
                        continue;

                    int nf[3];
                    for (int j = 0; j < nd.npts; j++)
                        nf[j] = fr.flags[nd.vidx[j]];
                    
                    // ---- material / node-type filter ----
                    {
                        if (mt == 0) {
                            if (nd.kind == 0 || nd.kind == 0xf)
                                goto emit;
                            goto no_emit;
                        } else if (mt == 8) {
                            if (nd.kind == 8 || nd.kind == 7)
                                goto emit;
                            if (nd.kind != 0)
                                goto no_emit;
                            if ((water & nf[0]) != 0)
                                goto emit;
                            if ((water & nf[1]) != 0)
                                goto emit;
                            if ((water & nf[2]) != 0)
                                goto emit;
                            goto no_emit;
                        } else if (mt == 0x15) {
                            if (nd.kind <= 0 || nd.kind == 8 || nd.kind == 0xf || nd.kind == 7 || nd.kind == 1)
                                goto no_emit;
                            if (fr.dist[nd.vidx[0]] < 1.51f || fr.dist[nd.vidx[1]] < 1.51f || fr.dist[nd.vidx[2]] < 1.51f)
                                goto emit;
                            goto no_emit;
                        } else if (mt != 0x16 && mt != 0x17) {
                            if (mt == 7 || mt == 0xf)
                                goto no_emit;
                            if (mt == 9) {
                                if (nd.kind == 8 || nd.kind == 0)
                                    goto no_emit;
                                if ((wood & nf[0]) == 0 && (wood & nf[1]) == 0 && (wood & nf[2]) == 0)
                                    goto no_emit;
                                goto emit;
                            } else {
                                if (nd.kind == 8 || nd.kind == 7 || nd.kind == 0)
                                    goto no_emit;
                                if ((umask & nf[0]) == 0 && (umask & nf[1]) == 0 && (umask & nf[2]) == 0)
                                    goto no_emit;
                                goto emit;
                            }
                        } else {
                            if (nd.kind != 0)
                                goto no_emit;
                            if (fr.dist[nd.vidx[0]] < 0.01f || fr.dist[nd.vidx[1]] < 0.01f || fr.dist[nd.vidx[2]] < 0.01f)
                                goto emit;
                            goto no_emit;
                        }
                    }
                emit:
                    int k = 0;
                    if (nd.npts > 0) {
                        rgl::world* w = g_script.level;
                        do {
                        int& id = nd.vidx[k];
                        if (fr.vert[id] != -1)
                            continue;
                        rgl::vector4 p = w->ground->verts[id].p;
                        const rgl::vector4& normal = w->ground->verts[id].n;

                        if (mt == 0x17) {
                            bool both = (sea & fr.flags[id]) != 0 && (land & fr.flags[id]) != 0;
                            if (!both) {
                                int chain[128];
                                int ncnt;
                                w->ground->build_chain(id, ni, chain, &ncnt);   // 0x5234e0 (target vertex, start node)
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
                                        float len = ((map_vn&)d).normalize_inl();
                                        if (len > 0.25f) {
                                            map_add(sum, q + d * 0.25f);     // the helper's reference: fld r.x; faddp like the original
                                        } else {
                                            sum.x += p.x;
                                            sum.y += p.y;
                                            sum.z += p.z;
                                        }
                                    }
                                    float inv = 1.0f / (float)ncnt;
                                    sum.x = sum.x * inv;
                                    sum.y = sum.y * inv;
                                    sum.z = sum.z * inv;
                                    p = sum;
                                }
                                p.z += 0.03f;
                            } else {
                                p.z += 0.01f;
                            }
                        }

                        rgl::vector4 rel(p.x - center.x, p.y - center.y, p.z);
                        int pidx = mv->add_point(&rel);

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
                            float rx = ((map_uv&)uv).rot(ca, sa);
                            noise_fractal(6.0f, 0.7f, 3, 0, (const float*)&p, (float*)&nz);
                            rgl::vector4 t = nz * 0.5f;
                            ((map_uv&)uv).add(rx, t);
                            uv.x *= 0.4f;
                            uv.y *= 0.4f;
                        } else if (mt == 0x16) {
                            float ca = (float)cos(sea_ang), sa = (float)sin(sea_ang);
                            rgl::vector4 nz;
                            float rx = ((map_uv&)uv).rot(ca, sa);
                            noise_fractal(6.0f, 0.7f, 3, 0, (const float*)&p, (float*)&nz);
                            rgl::vector4 t = nz * 0.7f;
                            uv.x = rx + t.x;
                            uv.y = t.y + uv.y;
                            uv.x *= 2.3f;
                            uv.y *= 2.3f;
                        } else if (mt == 0x17) {
                            float ca = (float)cos(sea_ang), sa = (float)sin(sea_ang);
                            rgl::vector4 nz;
                            float rx = ((map_uv&)uv).rot(ca, sa);
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
                            map_mul(c, lightL + lightM * g_script.level->ground->verts[id].level * 1.1f);
                        }
                        c.w = 1.0f;

                        if (mi == 0) {
                            // sqrt and clamp at the head of both arms (cl merges the heads and leaves the original's dead lea of
                            // &dist[id]); the scale written out per channel: its common subexpression stays on the x87 stack
                            // past the third product (fld c.z; fmul st(1); fstp; fstp st(0)), a by-value helper's k is consumed
                            float& dd = fr.dist[id];
                            if (g_use_shaders != 0) {
                                float sd = (float)sqrt(dd);
                                if (!(sd < 5.0f)) sd = 5.0f;
                                c.x = c.x * ((6.0f - sd) * 0.16666667f);
                                c.y = c.y * ((6.0f - sd) * 0.16666667f);
                                c.z = c.z * ((6.0f - sd) * 0.16666667f);
                            } else {
                                float sd = (float)sqrt(dd);
                                if (!(sd < 5.0f)) sd = 5.0f;
                                c.x = c.x * ((8.0f - sd) * 0.125f);
                                c.y = c.y * ((8.0f - sd) * 0.125f);
                                c.z = c.z * ((8.0f - sd) * 0.125f);
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
                            if (nd.kind == 0 && dd > 0.01f)
                                c.w = 0.0f;
                        } else if (mt == 0x16) {
                            // two arms with one body: cl merges them into the `mt == 0x16 || mt == 0x17` test, but the
                            // c.w stores stay apart (with one arm, cl hoists c.w = 0 and moves the block last)
                            if (fr.dist[id] > 0.01f)
                                c.w = 0.0f;
                            else
                                c.w = (water & fr.flags[id]) != 0 ? 0.0f : 1.0f;
                        } else if (mt == 0x17) {
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

                        fr.vert[id] = map_add_vertex((rgl::mesh*)mv, pidx, uv, rgl::pack_color(&c), normal, 0);
                        w = g_script.level;
                                            } while (++k < nd.npts);
                    }
                    mv->add_face(fr.vert[nd.vidx[0]], fr.vert[nd.vidx[1]], fr.vert[nd.vidx[2]]);

                no_emit:
                    // ---- tree-mesh selection, material 0 only ----
                    if (mi == 0) {
                        int t = nd.kind;
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
                                    const rgl::vector4& tp = g_script.level->ground->verts[nd.vidx[q]].p;
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
                                cmv->append_skinned(chosen, &frame, -1, 0xffffffff, 0); // 0x44eb00
                            }
                        }
                    }
                }

                // ---- finalize the per-material mesh ----
                if (mv->faces.size() != 0) {
                    mv->res = terrain_material(mt);
                    ((NibbleObj*)mv)->set_nibble(mi - 6);
                    mv->mask = 0x17fff;
                    mv->compute_bounds();
                    add_cell_mesh(g_script.map_cell[ci], mv);
                    if (mt == 0)
                        g_script.mat_type0.push_back(mv);
                    else if (mt == 8)
                        g_script.mat_river.push_back(mv);
                    else if (mt == 0x16)
                        g_script.mat_sea.push_back(mv);
                    else if (mt == 0x17)
                        g_script.mat_sea2.push_back(mv);
                } else {
                    delete mv;
                }
            }

            // ---- finalize the cell mesh ----
            if (cmv->faces.size() != 0) {
                cmv->res = g_resmgr.get_material(RStr("map_trees"));
                ((NibbleObj*)cmv)->set_nibble(2);
                cmv->mask = 0x1ffff;
                cmv->compute_bounds();
                add_cell_mesh(g_script.map_cell[ci], cmv);
                ents.push_back(cmv);
            } else {
                delete cmv;
            }
        }
    }
}


// @ 0x523360
void NibbleObj::set_nibble(int v)
{
    if (this->res) {
        this->res->flags &= 0xf0ffffff;
        if (v < 0)
            v += 0x10;
        this->res->flags |= (unsigned)v << 24;
    }
}

// @ 0x523390
skin* Game::terrain_material(int type)
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
void NavData::build_chain(int target, int start, int* out, int* count)
{
    int cur = start;
    int prev = -1;
    int next;
    *count = 0;
    do {
        NavCell& cell = cells[cur];
        int n = cell.npts;
        int found = -1;
        if (n <= 0)
            return;
        int* nb = cell.edge_ids();
        while (n > 0) {
            int v = *nb;
            NavEdge& e = edges[v];
            if (e.count > 1 && e.cell != prev && e.other != prev
                && (e.a == target || e.b == target))
                found = v;
            nb++;
            --n;
        }
        if (found < 0)
            return;
        NavEdge& e = edges[found];
        next = (e.cell == cur) ? e.other : e.cell;
        bool dup = false;
        if (e.a == target) {
            for (int i = 0; i < *count; i++)
                if (out[i] == e.b)
                    dup = true;
            if (!dup) {
                out[*count] = e.b;
                (*count)++;
            }
        } else {
            for (int i = 0; i < *count; i++)
                if (out[i] == e.a)
                    dup = true;
            if (!dup) {
                out[*count] = e.a;
                (*count)++;
            }
        }
        prev = cur;
        cur = next;
    } while (next != start);
}

// @ 0x523630
void Game::scan_arrays(void* p, void* unused)
{
    unsigned i;
    for (i = 0; i < g_script.level->ground->verts.size(); i++)
        ((float*)p)[200000] = -10000000.0f;
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
    in_factions.r_int(&n_factions);
    if (n_factions > 0x80)
        mb_error("Too Many Factions: %d (Max should be %d)", n_factions, 0x80);
    if (factions) {
        delete[] factions;
        factions = 0;
    }
    factions = new FactionRec[n_factions];
    for (int i = 0; i < n_factions; i++)
        factions[i].read_file(in_factions, n_factions);
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
        g_skeletons.load(path);
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
    if (troops) {
        delete[] troops;
        troops = 0;
    }
    troops = new Troop[c_troops];
    for (int j = 0; j < ntroops; j++)
        troops[j].read_file(in_troops);
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
    int& c_scenes = num_scenes;
    in_scenes.r_int(&c_scenes);
    if (scenes) {
        delete[] scenes;
        scenes = 0;
    }
    scenes = new SceneFile[c_scenes];
    for (int k = 0; k < num_scenes; k++)
        scenes[k].read_file(in_scenes);
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
    int& c_quests = n_quests;
    in_quests.r_int(&c_quests);
    if (quests) {
        delete[] quests;
        quests = 0;
    }
    quests = new QuestRec[c_quests];
    for (int n = 0; n < n_quests; n++)
        quests[n].read_file(in_quests);
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
    int first = simple.resize(in_simple, "Simple trigger", extra_triggers);
    s_simple.failed = 1;

    // one extra simple trigger per script emitter whose delay is non-negative
    for (int idx = 0; idx < g_scripts.count; idx++) {
        if (g_scripts[idx].f14 >= 0.0f) {
            trigger_entry& rec = simple.data[first];
            rec.interval = g_scripts[idx].f14;
            rec.block.init(1, idx);
            first++;
        }
    }

    // ---- the wind anchors: four weight records (head, tail) at +0x13b64 and their xy positions at +0x13ba4 ---------------
    val[0].q = Vec2(3.0f, 3.0f);
    pos[0] = Vec2(-100.0f, -50.0f);
    val[1].q = Vec2(5.0f, 1.0f);
    pos[1] = Vec2(100.0f, -50.0f);
    val[2].q = Vec2(1.0f, -3.0f);
    pos[2] = Vec2(-100.0f, 50.0f);
    val[3].q = Vec2(-5.0f, -3.0f);
    val[0].p.x = 23.0f;     val[0].p.y = 0.2f;
    val[1].p.x = 15.0f;     val[1].p.y = 0.1f;
    val[2].p.x = 11.0f;     val[2].p.y = 0.9f;
    val[3].p.x = 3.0f;      val[3].p.y = 0.3f;
    pos[3] = Vec2(100.0f, 50.0f);
}

static inline void clear_ref(ItemRef& r) { r.item = -1; r.m4 = 0; }
static inline void r_float2(stream_base& s, float* v) { s.r_float(v).r_float(v + 1); }
// the engine's fast float floor: bias non-positive values by (1 - 2^-24), then truncate
static int ftoi_floor(float t)
{
    if (!(t > 0.0f))
        t -= 0.99999988f;
    int r = (int)t;
    return r;
}

// @ 0x524360
void DialogBlock::init(int op, int idx)
{
    done = 0;
    if (items)
        delete[] items;
    items = new Elem[1];
    items->id = op;
    items->n = 1;
    items->vals[0] = idx;
    count = 1;
}

// @ 0x5243c0
stream_base& read_skill(stream_base& s, SkillRec& r)
{
    s.r_tstr(RSTR(&r.key));
    s.r_tstr(RSTR(&r.name)).r_uint(&r.attr).r_int(&r.max_rank).r_tstr(RSTR(&r.desc));
    r.name.underscores_to_spaces();
    r.desc.underscores_to_spaces();
    r.cap = r.max_rank;
    return s;
}

// @ 0x524420
void Troop::read_file(stream_base& s)
{
    int n;
    s.r_tstr(RSTR(&key));
    s.r_tstr(RSTR(&name));
    name.underscores_to_spaces();
    s.r_tstr(RSTR(&plural));
    plural.underscores_to_spaces();
    s.r_u64(&flags);
    s.r_int(&site);
    s.r_int(&tuple_reserved);
    s.r_int(&faction);
    s.r_int(&upgrade_troop[0]);
    s.r_int(&upgrade_troop[1]);
    int* pa = &inv[0].item;
    for (int i = 0; i < 0x40; ++i, pa += 2) {
        s.r_int(pa);
        s.r_int(pa + 1);
    }
    for (int i = 0; i < 10; ++i)
        clear_ref(equip[i]);
    for (int i = 0; i < 4; ++i)
        s.r_int(&attributes[i]);

    s.r_int(&n);
    if (n <= 0)
        n = 1;
    int lv = n;
    level = lv;
    int base = g_level_xp[lv];
    float t = (float)(rand() % 0x3dc9) * (float)(g_level_xp[lv + 1] - base) * (1.0f / 0x3dc9);
    base += ftoi_floor(t);
    exp = base;

    for (int i = 0; i < 7; ++i) {
        int v;
        s.r_int(&v);
        prof[i] = (float)v;
    }
    for (int i = 0; i < 6; ++i)
        s.r_uint(&nib[i]);

    int str = attributes[0];
    int b = (nib[4] >> 16) & 0xf;
    if (b < 0)
        b = 0;
    else if (b > g_skill_tbl[36].max_rank)              // skill 36 (ironflesh): its rank limit
        b = g_skill_tbl[36].max_rank;
    hp = (float)(str + 0x23 + b * 2);

    for (int i = 0; i < 4; ++i)
        s.r_u64(&face_keys.word[i]);
    for (int i = 0; i < 4; ++i)
        s.r_u64(&face_keys_b.word[i]);
}

// @ 0x524690
__declspec(noinline) void QuestRec::read_file(stream_base& s)
{
    s.r_tstr(RSTR(&key));
    s.r_tstr(RSTR(&name));
    s.r_int(&f50);
    s.r_tstr(RSTR(&desc));
    name.underscores_to_spaces();
    desc.underscores_to_spaces();
    title = *raw_title();       // 0x48d750
    tstring* p = g_strtable.find(&key);
    text = p ? *p : name;
}

// @ 0x524710
Troop::Troop()
{
    for (int i = 0; i < 0x60; ++i)
        inv[i].item = -1;
    notes.tableau = -1;
    for (int i = 0; i < 10; ++i) {
        equip[i].item = -1;
        equip[i].m4 = 0;
    }
    flags = 0;
    site = -1;
    level48 = 0;
    f44 = 0;
    gold = 0;
    exp = 0;
    skill_points = 0;
    prof_points = 0;
    attr_points = 0;
    level = 1;
    for (int i = 0; i < 4; ++i)
        attributes[i] = 0;
    for (int i = 0; i < 7; ++i)
        prof[i] = 0;
}

// @ 0x524860
QuestRec::QuestRec()
{
    notes.tableau = -1;
    f50 = 0;
    flags = 0;
    f11c = 0;
    giver_troop = -1;
}

// @ 0x5248e0
SceneFile::SceneFile()
{
    loaded = 0;
    for (int i = 0; i < 0x40; ++i) {
        visitors[i].troop = -1;
        visitor_counts[i] = 0;
    }
}

// @ 0x431e10
SceneFile::~SceneFile() {}

// @ 0x5249f0
FactionRec::FactionRec()
{
    notes.tableau = -1;
    f230 = 0;
    f10 = 0;
    f244 = -1.0f;
    max_rating = 1.0f;
}

// @ 0x524a70
void FactionRec::read_file(stream_base& s, int count)
{
    s.r_tstr(RSTR(&key));
    s.r_tstr(RSTR(&name));
    name.underscores_to_spaces();
    s.r_int(&f24);
    s.r_uint(&color);
    color |= 0xff000000;
    max_rating = (float)(100 - ((f24 >> 8) & 0xff)) * 0.01f;

    if (count > 0) {
        float* p = relations;
        for (int i = count; i > 0; --i) {
            s.r_float(p);
            ++p;
        }
    }
    s.r_int(&nranks);
    if (nranks != 0) {
        tstring tmp;
        ranks.reserve(nranks);
        for (int i = 0; i < nranks; ++i) {
            s.r_tstr(RSTR(&tmp));
            ranks.push_back(tmp);
        }
    }
}

// @ 0x524bc0
template void std::vector<tstring>::reserve(size_t);

// tstring's character accessor: falls back to the buf field's own address when empty
static inline const char* cstr(const tstring& t)
{
    return t.buf ? t.buf : (const char*)&t.buf;
}

// @ 0x524d60
void SceneFile::read_file(stream_base& s)
{
    for (int i = 0; i < 0x40; ++i) {
        visitors[i].troop = -1;
        visitor_counts[i] = 0;
    }
    s.r_tstr(RSTR(&name10));
    s.r_tstr(RSTR(&name));
    s.r_int(&flags);
    s.r_tstr(RSTR(&mesh_name));
    s.r_tstr(RSTR(&body_name));
    r_float2(s, &rect[0]);
    r_float2(s, &rect[2]);
    s.r_float(&base_z);
    s.r_tstr(RSTR(&terrain_code));
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
    PolyMesh tmp;
    tmp.save(s444);
}

// @ 0x524f00
// The nav mesh into the scene record's stream (the reader is PolyMesh::load 0x4781d0): the vertex positions, the edges (face count,
// endpoints, faces), the polygons (corner count, corner vertices, corner edges, f5c).
void PolyMesh::save(stream_base& s)
{
    {
        int n = (int)verts.size();
        s.w_uint((unsigned*)&n);
    }
    for (unsigned i = 0; i < verts.size(); ++i) {
        MVert& e = verts[i];
        s.w_float(&e.pos.x).w_float(&e.pos.y).w_float(&e.pos.z);
    }

    {
        int n = (int)edges.size();
        s.w_uint((unsigned*)&n);
    }
    for (unsigned i = 0; i < edges.size(); ++i) {
        MEdge& e = edges[i];
        s.w_int(&e.nfaces);
        s.w_int(&e.v0);
        s.w_int(&e.v1);
        s.w_int(&e.face[0]);
        s.w_int(&e.face[1]);
    }

    {
        int n = (int)faces.size();
        s.w_uint((unsigned*)&n);
    }
    for (unsigned i = 0; i < faces.size(); ++i) {
        MPoly& e = faces[i];
        s.w_int(&e.n);
        for (int k = 0; k < e.n; ++k)
            s.w_int(&e.vid[k]);
        for (int k = 0; k < e.n; ++k)
            s.w_int(&e.eid[k]);
        s.w_int(&e.f5c);
    }
}
