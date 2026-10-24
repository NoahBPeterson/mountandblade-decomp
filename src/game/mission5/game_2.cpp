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
#include "./game_2.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../party/game.h"
#include "../party/externs.h"
#include "../tstring.h"
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

// ---- the map object (0xa732b8): vertex and group vectors of the ground geometry ---------------------------------------
struct MapVert { rgl::vector4 p; rgl::vector4 n; int f20; };    // 0x24 bytes
struct MapGroup {                                               // 0x88 bytes
    int n;                                                      // +0
    int vidx[4];                                                // +4
    char pad14[0x10];                                           // +0x14
    int f24;                                                    // +0x24
    int link[4];                                                // +0x28
    int f38[4];                                                 // +0x38
    int f48, f4c, f50;                                          // +0x48
    char pad54[0x10];                                           // +0x54
    float f64;                                                  // +0x64
    int f68, f6c, f70;                                          // +0x68
    rgl::vector4 c;                                             // +0x74
    int f84;                                                    // +0x84
    MapGroup();                                                 // 0x4246f0
};
struct MapVerts {                                               // vector<MapVert>, _Myfirst at +4
    char pad0[4];
    MapVert* first; MapVert* last; MapVert* end;
    void resize(unsigned n, MapVert v);                         // 0x424740
};
struct MapGroups {                                              // vector<MapGroup>, _Myfirst at +4
    char pad0[4];
    MapGroup* first; MapGroup* last; MapGroup* end;
    void resize(unsigned n, MapGroup v);                        // 0x424cc0
};
struct MapStore {                                               // the global map (0xa732b8), 0x68 bytes
    MapVerts verts;                                             // +0
    MapGroups groups;                                           // +0x10
    char pad20[0x68 - 0x20];
    MapStore();                                                 // 0x478150
    void post_load();                                           // 0x47ee60
};
extern MapStore* g_map;                                         // 0xa732b8

stream_base* read_map(stream_base* s, MapStore* map);           // 0x520c00

// @ 0x520b20
void Game::build_map()
{
    if (!g_new_game_done) {
        g_map = new MapStore();
        file_text s;
        const char* path = module_path("map.txt");
        if (!s.open_read_text(path))
            mb_error("Unable to open %s", path);
        if (!s.failed) {
            read_map(&s, g_map);
            s.failed = 1;
        }
    }
}

// @ 0x520c00
__declspec(noinline) stream_base* read_map(stream_base* s, MapStore* map)
{
    int n;
    s->r_int(&n);
    map->verts.resize(n, MapVert());
    int i;
    for (i = 0; i < n; i++) {
        char* p = (char*)map->verts.first + i * 0x24;
        s->r_float((float*)p).r_float((float*)p + 1).r_float((float*)p + 2);
        *(float*)((char*)map->verts.first + i * 0x24 + 0x20) = -1.0f;
    }
    int n2;
    s->r_int(&n2);
    map->groups.resize(n2, MapGroup());
    for (i = 0; i < n2; i++) {
        char* g = (char*)map->groups.first + i * 0x88;
        s->r_int((int*)(g + 0x24));
        s->r_int((int*)(g + 0x68));
        s->r_int((int*)g);
        int nn = *(int*)g;
        for (int j = 0; j < nn; j++)
            s->r_int((int*)(g + 4 + j * 4));
    }
    map->post_load();
    return s;
}

// @ 0x520d60
void Game::build_scene()
{
    GameMapView* self = (GameMapView*)this;
    CellGrid* world = g_level;
    if (g_new_game_done) {
        for (int i = 0; i < g_page_rows; i++) {
            char* row = g_map_pages + i * 4;
            for (int j = 0; j < g_page_cols; j++) {
                char* cell = row + j * 0x20;
                void* p = *(void**)cell;
                if (p) {
                    ((WorldX*)world)->destroy_obj(p);
                    *(void**)(cell + 0x100) = *(void**)cell;
                    *(void**)cell = 0;
                }
            }
        }
        ((rgl::world*)world)->clear_ents();
    }
    log_stream_ptr()->w_cstr("map mesh built.\n");
    rgl::vector4 tmp;
    self->fog1 = *self->fog_query_a(&tmp);
    log_stream_ptr()->w_cstr("get_ideal_sun_color.\n");
    self->fog2 = *self->fog_query_b(&tmp);
    log_stream_ptr()->w_cstr("get_ideal_fog_color.\n");
    self->fog1 = rgl::vector4(0.5f, 0.48f, 0.47f);
    self->fog2 = rgl::vector4(0.8f, 0.8f, 0.8f);
    self->fog3 = rgl::vector4(0.35f, 0.37f, 0.4f);
    world->amb = rgl::vector4(0.35f, 0.37f, 0.4f);
    world->amb.w = 1.0f;

    Wind* w = new Wind();
    {
        std::vector<Wind*>& wv = world->winds;
        if (wv.size() < wv.capacity()) {
            Wind** p = wv._Mylast;
            *p = w;
            wv._Mylast = p + 1;
        } else {
            wv.insert(wv.end(), w);
        }
    }
    self->wind = w;
    self->wind->f0 = 2;
    self->wind->pos = self->fog1;
    self->wind->f54 = 30000.0f;
    self->wind->f24 = rgl::vector4(700.0f, 200.0f, 900.0f);
    self->wind->f44 = rgl::vector4(-0.8f, -0.6f, -1.0f);
    float len = self->wind->f44.length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        self->wind->f44.x = self->wind->f44.x * inv;
        self->wind->f44.y = self->wind->f44.y * inv;
        self->wind->f44.z = self->wind->f44.z * inv;
    } else {
        self->wind->f44.x = 0.0f;
        self->wind->f44.y = 1.0f;
        self->wind->f44.z = 0.0f;
    }

    rgl::entity* track = (rgl::entity*)world->create_entity(9);
    self->track_entity = track;
    float d = 1000000.0f;
    track->set_fbc(&d);
    self->map_track = new rgl::mesh();
    self->map_track->res = g_resmgr.get_material(rgl::string("map_track"));
    ((EntityX*)self->track_entity)->add_mesh(self->map_track);

    rgl::entity* obj = (rgl::entity*)world->create_entity(9);
    self->map_obj = rgl::resmgr().get_object(g_compass_name)->clone();
    self->map_obj->scale(60.0f);
    self->map_obj->mask = 0xfffeffff;
    ((EntityX*)obj)->add_mesh(self->map_obj);
    d = 100000000.0f;
    obj->set_fbc(&d);
    world->f78 = (int)obj;

    self->compass = (rgl::entity*)world->create_entity(9);
    rgl::mesh* mc = rgl::resmgr().get_object(&rgl::string("compass"))->clone();
    mc->scale(0.28f);
    ((EntityX*)self->compass)->add_mesh(mc);

    if (!g_new_game_done) {
        build_map_trees();
    } else {
        for (int i = 0; i < g_page_rows; i++) {
            char* row = g_map_pages + i * 4;
            for (int j = 0; j < g_page_cols; j++) {
                char* cell = row + j * 0x20;
                *(void**)cell = *(void**)(cell + 0x100);
                *(void**)(cell + 0x100) = 0;
                ((rgl::world*)world)->add_pending(*(rgl::entity**)cell);
            }
        }
    }

    ((rgl::world*)world)->step(0.0f);

    unsigned id = (unsigned)((PartyMgrView*)&scene)->first_party_id();
    if (id < (unsigned)scene.f138dc) {
        do {
            self->add_order_arrow((int)id);
            ((PartyMgrView*)&scene)->next_party_id((int*)&id);
        } while (id < (unsigned)scene.f138dc);
    }
    log_stream_ptr()->w_cstr("parties added.\n");

    int idx = 0;
    if (sprites.count > 0) {
        do {
            if (*(int*)((char*)sprites.pages._Myfirst[idx >> 4] + (idx & 0xf) * 0x98 + 4) != 0)
                break;
            idx++;
        } while (idx < sprites.count);
    }
    if (idx < sprites.count) {
        do {
            self->del_order(idx);
            ((SpriteNext*)&sprites)->next(&idx);
        } while (idx < sprites.count);
    }
    release_orders();
    more_start();
    if (f13a34 == 0) {
        self->sl_query(0, 0, 0);
        f13a34 = 1;
    }
}

// @ 0x5213a0
__declspec(noinline) void Game::release_orders()
{
    TrackRec* p = tracks._Myfirst;
    if (p < tracks._Mylast) {
        do {
            ((GameMapView*)this)->destroy_order(p);
            p++;
        } while (p < tracks._Mylast);
    }
}

// @ 0x5213d0
void Game::build_map_trees()
{
    GameMapView* self = (GameMapView*)this;
    MapNav* mgr = (MapNav*)g_level->core;

    // ---- A: reset the four per-material mesh vectors -----------------------
    for (int v = 0; v < 4; v++) {
        if (g_mat_vec[v].first) ::operator delete(g_mat_vec[v].first);        // 0x535712
        g_mat_vec[v].first = 0;
        g_mat_vec[v].last = 0;
        g_mat_vec[v].end = 0;
    }

    void* cam = self->wind;                                                   // +0x139dc
    float ang_a = (float)g_ang_a * 3.1415927f * 0.0055555557f;               // deg -> rad
    float ang_b = (float)g_ang_b * 3.1415927f;

    rgl::vector4 dir;
    dir.x = -*(float*)((char*)cam + 0x44);
    dir.y = -*(float*)((char*)cam + 0x48);
    dir.z = -*(float*)((char*)cam + 0x4c);
    m5_vector4_normalize(&dir);                                                 // 0x407d60

    int npts = (int)(((char*)mgr->pts_end - (char*)mgr->pts) / 0x24);

    // ---- B: per-point view dot --------------------------------------------
    for (int i = 0; i < npts; i++) {
        MapPoint* p = &mgr->pts[i];
        float s = dir.x * p->nx + dir.y * p->ny + dir.z * p->nz;
        p->s = s;
        if (g_env2 != 0) p->s = s * s;
    }

    // ---- C: per-point flags and distance ----------------------------------
    int*   flags = new int[npts];
    float* dist  = new float[npts];
    int*   vert  = new int[npts];
    int*   scratch = new int[npts];                                          // [esp+0xc3e00]
    for (int i = 0; i < npts; i++) { flags[i] = 0; dist[i] = 1000.0f; }

    int nnodes = (int)(((char*)mgr->nodes_end - (char*)mgr->nodes) / 0x88);
    for (int i = 0; i < nnodes; i++) {
        MapNode* nd = &mgr->nodes[i];
        unsigned nt = (unsigned)nd->type;
        for (unsigned k = 0; k < 10; k++) {
            unsigned u;
            if (nt == 8)                     u = 8;
            else if (nd->type == 9)          u = 9;
            else { u = nt & 7; if (nd->type == 0xf) u = 0; }
            if (u == k) {
                unsigned mask = g_masks[k * 2];
                for (int j = 0; j < nd->count; j++) flags[nd->idx[j]] |= mask;
            }
        }
    }

    // ---- D: bbox + aggregate masks ----------------------------------------
    g_maxx = -100000.0f; g_minx = 100000.0f;
    g_miny = 100000.0f;  g_maxy = -100000.0f;
    unsigned u33 = g_masks[16] | g_masks[14];
    unsigned u36 = g_masks[6] | g_masks[2] | g_masks[8] | g_masks[10] | g_masks[22]
                 | g_masks[18] | g_masks[24] | g_masks[26] | g_masks[20] | g_masks[4];
    unsigned u19 = g_masks[0];
    unsigned u20 = g_masks[18];
    m5_grid_scan(scratch, (void*)((char*)scratch + 0x40), self);                // 0x523630

    for (int i = 0; i < npts; i++) {
        MapPoint* p = &mgr->pts[i];
        if (p->x < g_minx) g_minx = p->x;
        if (g_maxx < p->x) g_maxx = p->x;
        if (p->y < g_miny) g_miny = p->y;
        if (g_maxy < p->y) g_maxy = p->y;
        if (((u19 & (unsigned)flags[i]) != 0) && ((u36 & (unsigned)flags[i]) != 0))
            dist[i] = 0.0f;
    }

    // ---- E: ten relaxation passes over the edges --------------------------
    int nedges = (int)(((char*)mgr->edges_end - (char*)mgr->edges) / 0x24);
    for (int pass = 10; pass != 0; pass--) {
        for (int e = 0; e < nedges; e++) {
            MapEdge* ed = &mgr->edges[e];
            int a = ed->a, b = ed->b;
            float dx = mgr->pts[a].x - mgr->pts[b].x;
            float dy = mgr->pts[a].y - mgr->pts[b].y;
            float dz = mgr->pts[a].z - mgr->pts[b].z;
            float len = sqrtf(dy * dy + (dz * dz + dx * dx));
            if (len + dist[b] < dist[a])      dist[a] = len + dist[b];
            else if (len + dist[a] < dist[b]) dist[b] = len + dist[a];
        }
    }

    // ---- F: material-name tables ------------------------------------------
    char tmp[0x40];
    m5_tstring_ctor0(tmp);                                                      // 0x42bbd0
    void* map_arr[0x40];    for (int i = 0; i < 0x40; i++) map_arr[i] = 0;
    void* snow_arr[0x40];   for (int i = 0; i < 0x40; i++) snow_arr[i] = 0;
    void* steppe_arr[0x40]; for (int i = 0; i < 0x40; i++) steppe_arr[i] = 0;
    void* desert_arr[0x40]; for (int i = 0; i < 0x40; i++) desert_arr[i] = 0;
    for (int i = 0; i < 4; i++) if (g_mat_count[i] > 0x40) g_mat_count[i] = 0x40;

    for (int i = 0; i < g_mat_count[0]; i++) {
        m5_tstring_assign(tmp, "map_tree_");                                    // 0x59c0d8
        m5_tstring_push_back(tmp, (char)('a' + i));
        map_arr[i] = m5_resmgr_get_object(g_resmgr_raw, tmp);
    }
    for (int i = 0; i < g_mat_count[1]; i++) {
        m5_tstring_assign(tmp, "snow_tree_");                                   // 0x59c0cc
        m5_tstring_push_back(tmp, (char)('a' + i));
        snow_arr[i] = m5_resmgr_get_object(g_resmgr_raw, tmp);
    }
    for (int i = 0; i < g_mat_count[2]; i++) {
        m5_tstring_assign(tmp, "steppe_tree_");                                 // 0x59c0bc
        m5_tstring_push_back(tmp, (char)('a' + i));
        steppe_arr[i] = m5_resmgr_get_object(g_resmgr_raw, tmp);
    }
    for (int i = 0; i < g_mat_count[3]; i++) {
        m5_tstring_assign(tmp, "desert_tree_");                                 // 0x59c0ac
        m5_tstring_push_back(tmp, (char)('a' + i));
        desert_arr[i] = m5_resmgr_get_object(g_resmgr_raw, tmp);
    }

    // ---- G: grid dimensions and cell entities -----------------------------
    g_sizex = g_maxx - g_minx;
    g_sizey = g_maxy - g_miny;
    float stepx = (g_sizex + 5.0f) * 0.125f;                                 // 0x59393c, 0x54b8a0
    float stepy = (g_sizey + 5.0f) * 0.125f;
    g_page_cols = (int)ceil(g_sizex / stepx);                                // ceil 0x535ae0, __ftol2 0x535a64
    g_page_rows = (int)ceil(g_sizey / stepy);
    if (g_page_cols > 8) mb_error("Map size too big in x axis!");            // 0x59c090
    if (g_page_rows > 8) mb_error("Map size too big in x axis!");

    for (int row = 0; row < g_page_rows; row++) {
        for (int col = 0; col < g_page_cols; col++) {
            void* ent = m5_create_entity(9);                                    // 0x428440
            g_grid[col * 8 + row] = ent;
            (*(void(**)(void*, int))((char*)*(void**)ent + 0x18))(ent, 0);   // virtual slot +0x18
        }
    }

    // ---- H: per-cell geometry ---------------------------------------------
    for (int col = 0; col < g_page_cols; col++) {
        float x0 = (float)col * stepx + g_minx;
        float x1 = (float)(col + 1) * stepx + g_minx;
        for (int row = 0; row < g_page_rows; row++) {
            float y0 = (float)row * stepy + g_miny;
            float y1 = (float)(row + 1) * stepy + g_miny;
            int ci = col * 8 + row;
            void* cell = g_grid[ci];

            void* cellmesh = (void*)::operator new(0x260);
            cellmesh = cellmesh ? m5_mesh_ctor(cellmesh) : 0;                   // 0x412a90

            for (int mi = 0; mi < 0xd; mi++) {
                int mt;
                if (mi == 10)      mt = 0x15;
                else if (mi == 11) mt = 0x16;
                else if (mi == 12) mt = 0x17;
                else               mt = mi;
                unsigned umask = g_masks[mi * 2];

                void* m = (void*)::operator new(0x260);
                m = m ? m5_mesh_ctor(m) : 0;

                // cell centre (written every material iteration in the original)
                *(float*)((char*)cell + 0x88) = (x1 + x0) * 0.5f;
                *(float*)((char*)cell + 0x8c) = (y1 + y0) * 0.5f;
                *(int*)((char*)cell + 0x90) = 0;
                *(int*)((char*)cell + 0x94) = 0;

                for (int i = 0; i < npts; i++) vert[i] = -1;

                for (int ni = 0; ni < nnodes; ni++) {
                    MapNode* nd = &mgr->nodes[ni];
                    MapPoint* p0 = &mgr->pts[nd->idx[0]];
                    if (!(x0 <= p0->x && p0->x < x1 && y0 <= p0->y && p0->y < y1))
                        continue;

                    int nf[8];
                    for (int j = 0; j < nd->count && j < 8; j++) nf[j] = flags[nd->idx[j]];

                    // --- material / node-type filter (0x5220xx) ---
                    bool emit = false;
                    if (mt == 0) {
                        emit = !(nd->type != 0 && nd->type != 0xf);
                    } else if (mt == 8) {
                        int t = nd->type;
                        if (t == 8 || t == 7)      emit = true;
                        else if (t != 0)           emit = false;
                        else emit = !((u33 & nf[0]) == 0 && (u33 & nf[1]) == 0 && (u33 & nf[2]) == 0);
                    } else if (mt == 0x15) {
                        int t = nd->type;
                        if (t > 0 && t != 8 && t != 0xf && t != 7 && t != 1)
                            emit = (dist[nd->idx[0]] < 1.51f) || (dist[nd->idx[1]] < 1.51f)
                                || (dist[nd->idx[2]] < 1.51f);
                    } else if (mt == 0x16 || mt == 0x17) {
                        if (nd->type == 0)
                            emit = (dist[nd->idx[0]] < 0.01f) || (dist[nd->idx[1]] < 0.01f)
                                || (dist[nd->idx[2]] < 0.01f);
                    } else if (mt != 7 && mt != 0xf) {
                        unsigned u18;
                        int t = nd->type;
                        if (mt == 9) { u18 = u20; if (t == 8) emit = true; else if (t == 0) emit = true; }
                        else         { u18 = umask; if (t == 8 || t == 7) emit = true; }
                        if (emit && (u18 & nf[0]) == 0 && (u18 & nf[1]) == 0 && (u18 & nf[2]) == 0)
                            emit = false;
                    }

                    // --- geometry emission (LAB_0052213f) ---
                    if (emit) {
                        for (int k = 0; k < nd->count; k++) {
                            int pi = nd->idx[k];
                            if (vert[pi] != -1) continue;
                            float px = mgr->pts[pi].x;
                            float py = mgr->pts[pi].y;
                            float pz = mgr->pts[pi].z;

                            if (mt == 0x17 && ((u19 & (unsigned)flags[pi]) == 0
                                            || (u36 & (unsigned)flags[pi]) == 0)) {
                                int nlist[64]; int ncnt = 0;
                                m5_collect_near(mgr, pi, nlist, &ncnt);             // 0x5234e0
                                for (int a = 0; a < ncnt; ) {
                                    int p2 = nlist[a];
                                    if ((u19 & (unsigned)flags[p2]) == 0
                                     || (u36 & (unsigned)flags[p2]) == 0)
                                        nlist[a] = nlist[--ncnt];
                                    else a++;
                                }
                                if (ncnt > 0) {
                                    float sx = 0.0f, sy = 0.0f;
                                    for (int a = 0; a < ncnt; a++) {
                                        int p2 = nlist[a];
                                        float dx = px - mgr->pts[p2].x;
                                        float dy = py - mgr->pts[p2].y;
                                        float dz = pz - mgr->pts[p2].z;
                                        float d = sqrtf(dy * dy + (dz * dz + dx * dx));
                                        float nx_, ny_;
                                        if (d > 1e-10f) { nx_ = dx * (1.0f / d); ny_ = dy * (1.0f / d); }
                                        else            { nx_ = 0.0f; ny_ = 1.0f; }
                                        float qx = px, qy = py;
                                        if (d > 0.25f) {
                                            qx = mgr->pts[p2].x + nx_ * 0.25f;
                                            qy = mgr->pts[p2].y + ny_ * 0.25f;
                                        }
                                        sx += qx; sy += qy;
                                    }
                                    px = sx * (1.0f / (float)ncnt);
                                    py = sy * (1.0f / (float)ncnt);
                                }
                            }

                            // m5_add_point takes a 0x20-byte record
                            float prec[8];
                            prec[0] = px; prec[1] = py; prec[2] = pz; prec[3] = 0.0f;
                            prec[4] = mgr->pts[pi].nx; prec[5] = mgr->pts[pi].ny;
                            prec[6] = mgr->pts[pi].nz; prec[7] = 0.0f;
                            int pidx = m5_add_point(m, prec);                        // 0x412f50

                            // ---- per-material orientation + fractal noise ----------
                            // uv seeds: [esp+0x14] = px*0.1, [esp+0x18] = py*0.1
                            float uu = px * 0.1f;                                 // 0x549d7c
                            float vv = py * 0.1f;

                            float in6[4], out6[4];
                            if (mt == 0) {
                                // atan2 of the scaled point; result * 10 stored as uu
                                float l = sqrtf(px * px + py * py);
                                float ca, sa;
                                if (l > 0.0f) { float inv = 1.0f / l; ca = px * inv; sa = py * inv; }
                                else          { ca = 0.0f; sa = 1.0f; }
                                uu = 10.0f * atan2f(ca, -sa);                 // 0x549d74, fpatan
                            } else if (mt == 8 || mt == 0x16 || mt == 0x17) {
                                // rotate the uv seed pair by the fixed map angle ang_b, then noise
                                float A = ang_b * 3.1415927f * 0.0055555557f; // [esp+0x11c]
                                float sa = sinf(A), ca = cosf(A);
                                in6[0] = ca * uu - sa * vv;
                                in6[1] = sa * uu + ca * vv;
                                in6[2] = 0.0f; in6[3] = 0.0f;
                                m5_noise_fractal(6.0f, 0.7f, 3, 0, in6, out6);      // 0x414fc0
                                if (mt == 8) {
                                    uu = out6[0] * 0.5f + uu;                    // 0x549d70
                                    vv = (out6[1] * 0.5f) + vv;
                                    uu *= 0.45f;                                 // 0x54b3b8
                                    vv *= 0.45f;
                                } else if (mt == 0x16) {
                                    uu = out6[0] * 0.7f + uu;                    // 0x54b3c8
                                    vv = (out6[1] * 0.7f) + vv;
                                    uu *= 2.3f;                                  // 0x591a0c
                                    vv *= 2.3f;
                                } else {
                                    uu = out6[0] * 0.3f + uu;                    // 0x549d68
                                    vv = (out6[1] * 0.3f) + vv;
                                    uu *= 5.3f;                                  // 0x59c088
                                    vv *= 5.3f;
                                }
                            } else if (mt == 2) {
                                uu *= 0.6f; vv *= 0.6f;                       // 0x590d94
                            }

                            // common per-vertex scale: clamp(point.s*1.3 + 0.1, 1.0)
                            float sc = mgr->pts[pi].s * 1.3f + 0.1f;          // 0x590d90, 0x549d7c
                            if (sc > 1.0f) sc = 1.0f;                         // 0x549cdc

                            // --- three fractal passes (0x5227ac..0x5228a2) ---------
                            // pass 1: dither the vertex's own (px,py); its out[6]/out[7]
                            //         become the tint z / the second uv seed.
                            float na[8], nb[8];
                            na[0] = px;  na[1] = py;  na[2] = 6816.0f; na[3] = 0.0f;  // 0x45d4d000
                            m5_noise_fractal(9.0f, 0.6f, 4, 0, na, nb);         // 0x40c00000 / 0x3f19999a
                            float tz = nb[6] * 0.5f;                         // 0x549d70
                            float tseed = nb[7] * 0.5f;

                            // pass 2: dither (material, y)
                            float nc[8], nd[8];
                            nc[0] = (float)mt; nc[1] = py; nc[2] = 2917.0f; nc[3] = 0.0f; // 0x45365000
                            m5_noise_fractal(7.0f, 0.6f, 4, 0, nc, nd);         // 0x40e00000

                            // pass 3: dither the fixed view dir
                            float ne[8], nf_[8];
                            ne[0] = dir.x; ne[1] = dir.y; ne[2] = -3668.0f; ne[3] = 0.0f;  // 0xc4654000
                            m5_noise_fractal(4.0f, 0.6f, 4, 0, ne, nf_);        // 0x40800000

                            // --- colour: (base + offset) * sc (0x5228a3) -----------
                            float cr = (py + 0.86f) * sc;                    // 0x59944c
                            float cg = (tseed + 0.89f) * sc;                 // 0x59c084
                            float cb = (tz * 0.6f + 0.87f) * sc;             // 0x590d94 / 0x59c080
                            float ca_ = 1.0f;

                            if (g_env2 == 0) {
                                // modulate the rgb by the point's view dot (0x5228f6)
                                float k = mgr->pts[pi].s;
                                cr *= (1.0f + k) * 0.55f;                    // 0x54b888
                                cg *= (1.0f + k) * 0.55f;
                                cb *= (1.0f + k) * 0.55f;
                            }

                            // --- material-conditional tint / alpha (0x5229af) ------
                            if (mt != 0) {
                                if (mt == 8) {
                                    if ((umask & (unsigned)flags[pi]) == 0) ca_ = 0.0f;
                                    cr *= 0.7f; cg *= 0.7f; cb *= 0.7f;      // 0x54b3c8
                                } else if (mt == 0x15) {
                                    float dd = dist[pi];
                                    if (!(dd > 0.01f)) { cr *= 0.42f; cg *= 0.42f; cb *= 0.42f; } // 0x590c5c
                                    ca_ = 1.0f - dd * 0.66f;                 // 0x590aec
                                    if (ca_ < 0.0f) ca_ = 0.0f;
                                    if (dist[pi] > 0.01f) ca_ = 0.0f;
                                } else if (mt == 0x16 || mt == 0x17) {
                                    ca_ = (dist[pi] > 0.01f) ? 0.0f
                                        : ((umask & (unsigned)flags[pi]) ? 0.0f : 1.0f);
                                } else {
                                    if ((umask & (unsigned)flags[pi]) == 0) ca_ = 0.0f;
                                    if (mt == 4) { cr = cg = cb = 0.9f; }    // 0x3f666666
                                    else if ((umask & (unsigned)flags[pi])) {
                                        cr *= 0.65f; cg *= 0.85f; cb *= 0.7f; // 0x590b6c / 0x590c5c / 0x54b3c8
                                    }
                                }
                            }

                            if (g_env2 == 0) { cr = sqrtf(cr); cg = sqrtf(cg); cb = sqrtf(cb); }

                            float uv[2] = { uu, vv };
                            float col_rec[4] = { cr, cg, cb, ca_ };
                            unsigned col_ = m5_pack_color(col_rec);             // 0x40e440
                            vert[pi] = m5_add_vertex(m, pidx, uv, &col_, prec, 0);  // 0x413c50
                        }
                        m5_add_rec2(m, vert[nd->idx[0]], vert[nd->idx[1]], vert[nd->idx[2]]); // 0x411920
                    }

                    // --- tree-mesh selection, material 0 only (LAB_00522cb4) ---
                    if (mi == 0) {
                        void* chosen = 0;
                        int t = nd->type;
                        if (t == 0xb) {
                        lab_cdb:
                            if (t == 0xc) goto lab_ce0;
                            if (t == 10) {
                                chosen = steppe_arr[0];
                                if (g_mat_count[2] > 1) chosen = steppe_arr[m5_mb_rand() % g_mat_count[2]];
                            } else if (t == 0xd) {
                                chosen = desert_arr[0];
                                if (g_mat_count[3] > 1) chosen = desert_arr[m5_mb_rand() % g_mat_count[3]];
                            } else {
                                chosen = (g_mat_count[0] < 2) ? 0
                                       : map_arr[m5_mb_rand() % g_mat_count[0]];
                            }
                        } else {
                            if (t != 0xc) {
                                if (t != 10 && t != 0xd) goto lab_fc7;
                                goto lab_cdb;
                            }
                        lab_ce0:
                            chosen = snow_arr[0];
                            if (g_mat_count[1] > 1) chosen = snow_arr[m5_mb_rand() % g_mat_count[1]];
                        }
                        if (chosen != 0) {
                            float mat[16];
                            memcpy(mat, m5_identity3(), 12 * 4);                // 0x40cd60
                            for (int q = 12; q < 16; q++) mat[q] = 0.0f;
                            float ang = (float)m5_mb_rand() * 0.0001f;
                            m5_matrix_rotate_sf(mat, &ang);                     // 0x40e770
                            for (int q = 0; q < 3; q++) { mat[q * 4 + 3] += (float)m5_mb_rand() * 0.0001f; }
                            m5_append_skinned(cellmesh, chosen, mat, -1, 0xffffffff, 0); // 0x44eb00
                        }
                    lab_fc7: ;
                    }
                }

                // --- finalize the per-material mesh (0x522ecf) ---
                if (!m) {
                    // nothing
                } else if (*(void**)((char*)m + 0x94) == 0
                        || (*(void**)((char*)m + 0x98) == *(void**)((char*)m + 0x94))) {
                    (*(void(**)(void*))(* (void**)m))(m);                    // virtual dtor
                } else {
                    *(void**)((char*)m + 0x2c) = m5_material_from_name(mt);     // 0x523390
                    m5_mesh_set_mat(m, mt);                                     // 0x523360
                    *(int*)((char*)m + 0x10c) = 0x17fff;
                    m5_scene_add_object(m);                                     // 0x419300

                    RawVec cv;
                    cv.first = *(void***)((char*)cell + 0x11c);
                    cv.last  = *(void***)((char*)cell + 0x120);
                    cv.end   = *(void***)((char*)cell + 0x124);
                    if (cv.first == 0 || cv.end <= cv.last) {
                        m5_vec_insert_n(&cv, cv.last, 1, &m);                   // 0x410610
                        *(void***)((char*)cell + 0x11c) = cv.first;
                        *(void***)((char*)cell + 0x120) = cv.last;
                        *(void***)((char*)cell + 0x124) = cv.end;
                    } else {
                        *cv.last = m;
                        *(void***)((char*)cell + 0x120) = cv.last + 1;
                    }
                    (*(void(**)(void*))((char*)*(void**)cell + 0x10))(cell); // virtual slot +0x10

                    RawVec* mv = 0;
                    if (mt == 0)       mv = &g_mat_vec[0];
                    else if (mt == 8)  mv = &g_mat_vec[1];
                    else if (mt == 0x16) mv = &g_mat_vec[2];
                    else if (mt == 0x17) mv = &g_mat_vec[3];
                    if (mv) {
                        if (mv->first == 0 || mv->end <= mv->last)
                            m5_vec_insert_n(mv, mv->last, 1, &m);
                        else { *mv->last = m; mv->last++; }
                    }
                }
            }

            // --- finalize the cell mesh (0x522f7e) ---
            if (!cellmesh) {
                // nothing
            } else if (*(void**)((char*)cellmesh + 0x94) == 0
                    || (*(void**)((char*)cellmesh + 0x98) == *(void**)((char*)cellmesh + 0x94))) {
                (*(void(**)(void*))(*(void**)cellmesh))(cellmesh);           // virtual dtor
            } else {
                char name[0x40];
                m5_tstring_ctor1(name, "map_trees");                            // 0x42bbe0 / 0x59c070
                *(void**)((char*)cellmesh + 0x2c) = m5_resmgr_get_material(g_resmgr_raw, name); // 0x418a70
                m5_tstring_dtor(name);                                          // 0x42bc40
                m5_mesh_set_mat(cellmesh, 2);                                   // 0x523360
                *(int*)((char*)cellmesh + 0x10c) = 0x1ffff;
                m5_scene_add_object(cellmesh);                                  // 0x419300

                RawVec cv;
                cv.first = *(void***)((char*)cell + 0x11c);
                cv.last  = *(void***)((char*)cell + 0x120);
                cv.end   = *(void***)((char*)cell + 0x124);
                if (cv.first == 0 || cv.end <= cv.last) {
                    m5_vec_insert_n(&cv, cv.last, 1, &cellmesh);
                    *(void***)((char*)cell + 0x11c) = cv.first;
                    *(void***)((char*)cell + 0x120) = cv.last;
                    *(void***)((char*)cell + 0x124) = cv.end;
                } else {
                    *cv.last = cellmesh;
                    *(void***)((char*)cell + 0x120) = cv.last + 1;
                }
                (*(void(**)(void*))((char*)*(void**)cell + 0x10))(cell);

                RawVec ev;
                ev.first = self->ents;
                ev.last  = self->ents_last;
                ev.end   = self->ents_end;
                if (ev.first == 0 || ev.end <= ev.last) {
                    m5_vec_insert_n(&ev, ev.last, 1, &cellmesh);
                    self->ents = ev.first; self->ents_last = ev.last; self->ents_end = ev.end;
                } else {
                    *ev.last = cellmesh;
                    self->ents_last = ev.last + 1;
                }
            }
        }
    }

    m5_tstring_dtor(tmp);                                                       // 0x42bc40

    delete[] scratch;
    delete[] vert;
    delete[] dist;
    delete[] flags;
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
skin* __stdcall terrain_material(int type)
{
    if (type == 0)
        return g_resmgr.get_material(rgl::string("map_ocean"));
    if (type == 1)
        return g_resmgr.get_material(rgl::string("map_mountain"));
    if (type == 3)
        return g_resmgr.get_material(rgl::string("map_plain"));
    if (type == 2)
        return g_resmgr.get_material(rgl::string("map_steppe"));
    if (type == 4)
        return g_resmgr.get_material(rgl::string("map_snow"));
    if (type == 5)
        return g_resmgr.get_material(rgl::string("map_desert"));
    if (type == 8)
        return g_resmgr.get_material(rgl::string("map_river"));
    if (type == 9)
        return g_resmgr.get_material(rgl::string("map_mountain_forest"));
    if (type == 7)
        return g_resmgr.get_material(rgl::string("map_bridge"));
    if (type == 0x15)
        return g_resmgr.get_material(rgl::string("map_shore"));
    if (type == 0x16)
        return g_resmgr.get_material(rgl::string("map_foam"));
    if (type == 0x17)
        return g_resmgr.get_material(rgl::string("map_reverse_foam"));
    return g_resmgr.get_material(rgl::string("default"));
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
        Rec88* rec = (Rec88*)((char*)this->f14 + cur * 0x88);
        int n = rec->count;
        int found = -1;
        if (n <= 0)
            return;
        int* nb = (int*)((char*)rec + 0x14);
        do {
            int v = *nb;
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
void __stdcall scan_arrays(void* p, int unused)
{
    for (unsigned i = 0; i < (unsigned)((SceneArrays*)g_level->core)->a_size(); i++)
        *(int*)((char*)p + 0xc3500) = (int)0xcb189680;
    volatile char buf[100000];
    for (unsigned i = 0; i < (unsigned)((SceneArrays*)g_level->core)->b_size(); i++)
        buf[i] = 0;
    for (unsigned i = 0; i < (unsigned)((SceneArrays*)g_level->core)->b_size(); i++)
        ;
}

// ---- Game::load_game_data and the record helpers that follow it (binary order) -----------------------------------------
extern SkillFile g_08ee3100[42];                    // 0x8ee3100 (the skill table; kept as a hex name for the equivalence resolver)

// @ 0x5236f0
void Game::load_game_data(int extra_triggers)
{
    tstring str1;                                        // +0x38
    tstring str2;                                        // +0x28

    // ---- factions.txt -----------------------------------------------------------------------------------
    file_text s_factions;                                // +0x80
    {
        const char* path = module_path("factions.txt");
        if (!s_factions.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    stream_base& in_factions = s_factions;
    in_factions.r_tstr(RSTR(&str1));
    in_factions.r_tstr(RSTR(&str2));
    int version;
    in_factions.r_int(&version);
    in_factions.r_int(&nfactions);
    if (nfactions > 0x80)
        mb_error("Too Many Factions: %d (Max should be %d)", nfactions, 0x80);
    delete[] (FactionFile*)factions;
    factions = 0;
    factions = (Faction*)new FactionFile[nfactions];
    for (int i = 0; i < nfactions; i++)
        ((FactionFile*)factions)[i].read_file(s_factions, nfactions);
    s_factions.failed = 1;

    // ---- skills.txt -------------------------------------------------------------------------------------
    file_text s_skills;                                  // +0x198
    {
        const char* path = module_path("skills.txt");
        if (!s_skills.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    int skill_count;
    s_skills.r_int(&skill_count);
    int i;
    for (i = 0; i < skill_count && i < 42; i++)
        read_skill(s_skills, g_08ee3100[i]);
    for (; i < 42; i++) {
        g_08ee3100[i].f34 = 0;
        g_08ee3100[i].f38 = 0;
        g_08ee3100[i].f40 = 0x100;
    }
    s_skills.failed = 1;

    // ---- Data\skeleton_bodies.xml -----------------------------------------------------------------------
    {
        tstring path("Data\\skeleton_bodies.xml");
        g_skeletons.load_bodies(&path);
    }
    g_skeletons.fix();

    // ---- troops.txt -------------------------------------------------------------------------------------
    file_text s_troops;                                  // +0xb8
    {
        const char* path = module_path("troops.txt");
        if (!s_troops.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    s_troops.r_tstr(RSTR(&str1));
    s_troops.r_tstr(RSTR(&str2));
    s_troops.r_int(&version);
    s_troops.r_int(&ntroops);
    delete[] (TroopFile*)troops;
    troops = 0;
    troops = (Troop*)new TroopFile[ntroops];
    for (int j = 0; j < ntroops; j++)
        ((TroopFile*)troops)[j].read_file(s_troops);
    s_troops.failed = 1;

    // ---- scenes.txt -------------------------------------------------------------------------------------
    file_text s_scenes;                                  // +0xf0
    {
        const char* path = module_path("scenes.txt");
        if (!s_scenes.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    s_scenes.r_tstr(RSTR(&str1));
    s_scenes.r_tstr(RSTR(&str2));
    s_scenes.r_int(&version);
    s_scenes.r_int(&nparties);
    delete[] (SceneFile*)parties;
    parties = 0;
    parties = (char*)new SceneFile[nparties];
    for (int k = 0; k < nparties; k++)
        ((SceneFile*)parties)[k].read_file(s_scenes);
    s_scenes.failed = 1;

    // ---- quests.txt -------------------------------------------------------------------------------------
    file_text s_quests;                                  // +0x48
    {
        const char* path = module_path("quests.txt");
        if (!s_quests.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    s_quests.r_tstr(RSTR(&str1));
    s_quests.r_tstr(RSTR(&str2));
    s_quests.r_int(&version);
    s_quests.r_int(&nquests);
    delete[] (QuestFile*)quests;
    quests = 0;
    quests = (QuestRec*)new QuestFile[nquests];
    for (int n = 0; n < nquests; n++)
        ((QuestFile*)quests)[n].read_file(s_quests);
    s_quests.failed = 1;

    // ---- triggers.txt -----------------------------------------------------------------------------------
    file_text s_triggers;                                // +0x160
    {
        const char* path = module_path("triggers.txt");
        if (!s_triggers.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    s_triggers.r_tstr(RSTR(&str1));
    s_triggers.r_tstr(RSTR(&str2));
    s_triggers.r_int(&version);
    ((TriggerVec*)&triggers)->resize(&s_triggers, "Game Triggers");
    s_triggers.failed = 1;

    // ---- simple_triggers.txt ----------------------------------------------------------------------------
    file_text s_simple;                                  // +0x128
    {
        const char* path = module_path("simple_triggers.txt");
        if (!s_simple.open_read_text(path))
            mb_error("Unable to open %s", path);
    }
    s_simple.r_tstr(RSTR(&str1));
    s_simple.r_tstr(RSTR(&str2));
    s_simple.r_int(&version);
    int first = ((GameVec*)&simple)->resize(&s_simple, "Simple trigger", extra_triggers);
    s_simple.failed = 1;

    // one extra simple trigger per script emitter whose delay is non-negative
    if (g_emitter_count > 0) {
        Emitter* e = g_emitters;
        int off = first * 0x30;
        int idx = 0;
        do {
            if (e->f14 >= 0.0f) {
                SimpleTrigger* rec = (SimpleTrigger*)((char*)simple.array + off);
                rec->f0 = *(int*)&e->f14;
                rec->stmt.init(1, idx);
                off += 0x30;
            }
            idx++;
            e = (Emitter*)((char*)e + 0x34);
        } while (idx < g_emitter_count);
    }

    // ---- combat parameter records (six records of four floats at +0x13b64) ------------------------------
    float* c = (float*)recs;
    c[2] = 3.0f;      c[3] = 3.0f;                       // record 0 tail
    c[16] = -100.0f;  c[17] = -50.0f;                    // record 4 head
    c[6] = 5.0f;      c[7] = 1.0f;                       // record 1 tail
    c[18] = 100.0f;   c[19] = -50.0f;                    // record 4 tail
    c[10] = 1.0f;     c[11] = -3.0f;                     // record 2 tail
    c[20] = -100.0f;  c[21] = 50.0f;                     // record 5 head
    c[14] = -5.0f;    c[15] = -3.0f;                     // record 3 tail
    c[22] = 100.0f;   c[23] = 50.0f;                     // record 5 tail
    c[0] = 23.0f;     c[1] = 0.2f;                       // record 0 head
    c[4] = 15.0f;     c[5] = 0.1f;                       // record 1 head
    c[8] = 11.0f;     c[9] = 0.9f;                       // record 2 head
    c[12] = 3.0f;     c[13] = 0.3f;                      // record 3 head
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
stream_base& read_skill(stream_base& s, SkillFile& r)
{
    s.r_tstr(RSTR(&r.f20));
    s.r_tstr(RSTR(&r.f10)).r_int(&r.f40).r_int(&r.f34).r_tstr(RSTR(&r.f0));
    r.f10.underscores_to_spaces();
    r.f0.underscores_to_spaces();
    r.f38 = r.f34;
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
    int* pa = &pairs[0].a;
    for (int i = 0; i < 0x40; ++i, pa += 2) {
        s.r_int(pa);
        s.r_int(pa + 1);
    }
    for (int i = 0; i < 10; ++i) {
        p10[i].a = -1;
        p10[i].b = 0;
    }
    for (int i = 0; i < 4; ++i)
        s.r_int(&f58[i]);

    s.r_int(&n);
    if (n < 1)
        n = 1;
    fb0 = n;
    int base = g_level_lo[n];
    int r = rand() % 0x3dc9;
    float t = (float)r * (float)(g_level_hi[n] - base) * g_troop_rand_scale;
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
    if (b > g_clamp_max)
        b = g_clamp_max;
    fa0 = (float)(f58[0] + 0x23 + b * 2);

    for (int i = 0; i < 4; ++i)
        s.r_u64(&u3c0[i]);
    for (int i = 0; i < 4; ++i)
        s.r_u64(&u3e0[i]);
}

// @ 0x524690
void QuestFile::read_file(stream_base& s)
{
    s.r_tstr(RSTR(&f54));
    s.r_tstr(RSTR(&f0));
    s.r_int(&f50);
    s.r_tstr(RSTR(&f10));
    f0.underscores_to_spaces();
    f10.underscores_to_spaces();
    f30 = *default_name();
    tstring* p = g_name_mgr.find(&f54);
    f40 = p ? *p : f0;
}

// @ 0x524710
TroopFile::TroopFile()
{
    for (int i = 0; i < 0x60; ++i)
        ((int*)pairs)[i * 2] = -1;
    vec_ctor_iter(names_raw, 0x10, 8, (void*)0x42bbd0);
    f4a0 = -1;
    for (int i = 0; i < 10; ++i) {
        p10[i].a = -1;
        p10[i].b = 0;
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
    vec_ctor_iter(names_raw, 0x10, 8, (void*)0x42bbd0);
    f108 = -1;
    f12c = 0;
    f130 = 0;
    f134 = 0;
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
        pairs114[i].a = -1;
        ints314[i] = 0;
    }
}

// @ 0x5249f0
FactionFile::FactionFile()
{
    vec_ctor_iter(names_raw, 0x10, 8, (void*)0x42bbd0);
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
        _Xlen();
    if (capacity() >= n)
        return;
    tstring* p = (tstring*)::operator new(n * 0x10);
    try {
        ptr_tag t;
        uninit_copy_str(first, last, p, *this, t);
    }
    catch (...) {
        ::operator delete(p);
        throw;
    }
    unsigned s = first == 0 ? 0 : (unsigned)(last - first);
    if (first) {
        destroy(first, last);
        ::operator delete(first);
    }
    end = p + n;
    last = p + s;
    first = p;
}

// @ 0x524cd0
tstring* __cdecl uninit_copy_str(tstring* f, tstring* l, tstring* d, tstring_alloc& al, ptr_tag tag)
{
    tstring* s = d;
    try {
        for (; f != l; ++d, ++f)
            al.construct(d, *f);
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
        pairs114[i].a = -1;
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
        V2& e = v2_first[i];
        s.w_int(&e.n);
        for (int k = 0; k < e.n; ++k)
            s.w_int(&e.a[k]);
        for (int k = 0; k < e.n; ++k)
            s.w_int(&e.b[k]);
        s.w_int(&e.c);
    }
}
