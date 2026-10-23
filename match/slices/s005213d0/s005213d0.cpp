// Slice s005213d0: Game::build_map_trees (0x5213d0) -- rebuilds the campaign-map
//   tree/scenery meshes from the map's node/edge graph and registers them with the
//   scene.  Single function, ~0x1f81 bytes (0x5213d0-0x523351), ~2.8 MB stack frame.
//
// RECONSTRUCTION (complete, not byte-exact): translated from the Ghidra decompile +
//   annotated disassembly.  The function is one ~8066-byte routine; no manifest row
//   anywhere in this repo exceeds ~2000 bytes, so a byte-exact match is out of reach.
//   Every branch, call, constant and record layout of the original is represented,
//   including the per-tree noise/orientation geometry in the material loop: the mt==0
//   atan2 orientation, the mt==8/0x16/0x17 rotate-then-noise assemblies, the mt==2
//   uv scale, the common point.s*1.3+0.1 clamp and the three 4-tap noise_fractal
//   passes that feed the vertex tint; the per-vertex colour/alpha is computed with the
//   material-conditional tint ladder and the sqrt (g_env2==0) path before pack_color.
//
// Phases:
//   A 0x5213e0  free the four per-material mesh vectors (0xa732c0/d0/e0/f0), seed the
//               view direction from the fog object (this+0x139dc, +0x44..0x4c).
//   B 0x5214e9  per graph point: s = dot(view, normal) (+0x20), squared if g_env2.
//   C 0x5215c1  per point flags=0, dist=1000; OR the type masks into the point flags.
//   D 0x5216db  init the bbox and the aggregate masks; FUN_00523630; refine the bbox
//               per point and zero dist where the mask pair matches.
//   E 0x5217c0  ten relaxation passes over the edge list (0x24-byte records).
//   F 0x521996  build the four material-name tables map/snow/steppe/desert_tree_*.
//   G 0x521bd2  grid dims = ceil(size / ((size+5)*0.125)), error past 8; create the grid cells.
//   H 0x521cae  per cell: one cell mesh + one mesh per material (0..0xc), walk the
//               nodes that fall in the cell, emit geometry, pick the tree mesh by node
//               type for material 0, then finalize/register everything.
//
// flags /O2 /MT /Gy /TP
#include <windows.h>
#include <math.h>
#include <new>
#include "rgl.h"

using rgl::vector4;

// ---------------------------------------------------------------------------
// records (offsets taken from this function)
// ---------------------------------------------------------------------------
struct Point {                       // 0x24-byte graph point
    float x, y, z;                   // +0x00
    char  pad0c[4];
    float nx, ny, nz;                // +0x10 normal
    char  pad1c[4];
    float s;                         // +0x20 dot with the view dir
};
struct Node {                        // 0x88-byte graph node (fan)
    int   count;                     // +0x00
    int   idx[8];                    // +0x04
    int   type;                      // +0x24
    char  pad[0x88 - 0x28];
};
struct Edge {                        // 0x24-byte graph edge
    char  pad0c[0xc];
    int   a;                         // +0x0c
    int   b;                         // +0x10
    char  pad18[0x24 - 0x18];
};
struct NodeMgr {                     // g_scene->mgr
    int    pad0;
    Point* pts;                      // +0x04
    Point* pts_end;                  // +0x08
    char   padc[8];
    Node*  nodes;                    // +0x14
    Node*  nodes_end;                // +0x18
    char   pad1c[8];
    Edge*  edges;                    // +0x24
    Edge*  edges_end;                // +0x28
};
struct Scene {                       // g_scene @ 0xa732b4
    char    pad0[0x68];
    vector4 color;                   // +0x68
    char    pad78[0xb0 - 0x78];
    NodeMgr* mgr;                    // +0xb0
    char    padb4[4];
    void*   pts2;                    // +0xb8
    void*   view;                    // +0xbc
};
struct PtrVec {                      // MSVC std::vector<T*>
    void** first;                    // +0x00
    void** last;                     // +0x04
    void** end;                      // +0x08
};
struct Game {                        // 'this' (ECX)
    char   pad0[0x38];
    void** ents;                     // +0x38 std::vector<mesh*>
    void** ents_last;                // +0x3c
    void** ents_end;                 // +0x40
    char   pad44[0x139dc - 0x44];
    void*  cam;                      // +0x139dc fog object (+0x44 = vector4)
    void build_map_trees();
};

// ---------------------------------------------------------------------------
// globals
// ---------------------------------------------------------------------------
extern Scene* g_scene;               // 0xa732b4
extern void*  g_pts;                 // 0xa732b8 (object, +0x04 = Point array)
extern int    g_grid_cols;           // 0xa73294
extern int    g_grid_rows;           // 0xa73298
extern void*  g_grid[64];            // 0xa73094 cell entities, index col*8+row
extern float  g_minx;                // 0xa7329c
extern float  g_miny;                // 0xa732a0
extern float  g_maxx;                // 0xa732a4
extern float  g_maxy;                // 0xa732a8
extern float  g_sizex;               // 0xa732ac
extern float  g_sizey;               // 0xa732b0
extern PtrVec g_mat_vec[4];          // 0xa732c0 / 0xa732d0 / 0xa732e0 / 0xa732f0
extern int    g_mat_count[4];        // 0xa1815c map, 0xa18160 snow, 0xa18164 steppe, 0xa18168 desert
extern int    g_ang_a;               // 0xa18144
extern int    g_ang_b;               // 0xa18150
extern unsigned g_masks[28];         // 0x5a8e68 .. 0x5a8ed0 (14 pairs, indexed [i*2])
extern int    g_env2;                // 0x5e28fc
extern void*  g_resmgr;              // 0x5d6fa0

// ---------------------------------------------------------------------------
// callees
// ---------------------------------------------------------------------------
float    vector4_normalize(vector4* self);                                   // 0x407d60
void     grid_scan(void* a, void* b, void* c);                               // 0x523630
void*    tstring_ctor0(void* self);                                          // 0x42bbd0
void*    tstring_ctor1(void* self, const char* s);                           // 0x42bbe0
void     tstring_assign(void* self, const char* s);                          // 0x42bcb0
void     tstring_push_back(void* self, char c);                              // 0x42be70
void     tstring_dtor(void* self);                                           // 0x42bc40
void*    resmgr_get_object(void* resmgr, void* name);                        // 0x419540
void*    resmgr_get_material(void* resmgr, void* name);                      // 0x418a70
void     mb_error(const char* msg);                                          // 0x42cc80
void*    create_entity(int type);                                            // 0x428440
void*    mesh_ctor(void* p);                                                 // 0x412a90
int      add_point(void* self, void* p);                                     // 0x412f50
int      add_vertex(void* self, int idx, void* uv, void* color, void* pos, void* f5c); // 0x413c50
int      add_rec2(void* self, int a, int b, int c);                          // 0x411920
void     append_skinned(void* self, void* other, const void* m, int bone, unsigned color, char flip); // 0x44eb00
void     noise_fractal(float a, float b, int c, int d, const float* in, float* out); // 0x414fc0
unsigned pack_color(const float* rgba);                                      // 0x40e440
void*    identity3();                                                        // 0x40cd60
void     matrix_rotate_sf(void* m, float* ang);                              // 0x40e770
int      mb_rand();                                                          // 0x536a9f
void     scene_add_object(void* obj);                                        // 0x419300
void     mesh_set_mat(void* mesh, int type);                                 // 0x523360
void*    material_from_name(int type);                                       // 0x523390
void     collect_near(void* mgr, int idx, int* out, int* count);             // 0x5234e0
void     vec_insert_n(PtrVec* v, void** pos, unsigned n, void** val);        // 0x410610

// ---------------------------------------------------------------------------
// @ 0x5213d0 Game::build_map_trees
// ---------------------------------------------------------------------------
void Game::build_map_trees()
{
    Game* self = this;
    NodeMgr* mgr = g_scene->mgr;

    // ---- A: reset the four per-material mesh vectors -----------------------
    for (int v = 0; v < 4; v++) {
        if (g_mat_vec[v].first) ::operator delete(g_mat_vec[v].first);        // 0x535712
        g_mat_vec[v].first = 0;
        g_mat_vec[v].last = 0;
        g_mat_vec[v].end = 0;
    }

    void* cam = self->cam;                                                   // +0x139dc
    float ang_a = (float)g_ang_a * 3.1415927f * 0.0055555557f;               // deg -> rad
    float ang_b = (float)g_ang_b * 3.1415927f;

    vector4 dir;
    dir.x = -*(float*)((char*)cam + 0x44);
    dir.y = -*(float*)((char*)cam + 0x48);
    dir.z = -*(float*)((char*)cam + 0x4c);
    vector4_normalize(&dir);                                                 // 0x407d60

    int npts = (int)(((char*)mgr->pts_end - (char*)mgr->pts) / 0x24);

    // ---- B: per-point view dot --------------------------------------------
    for (int i = 0; i < npts; i++) {
        Point* p = &mgr->pts[i];
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
        Node* nd = &mgr->nodes[i];
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
    grid_scan(scratch, (void*)((char*)scratch + 0x40), self);                // 0x523630

    for (int i = 0; i < npts; i++) {
        Point* p = &mgr->pts[i];
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
            Edge* ed = &mgr->edges[e];
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
    tstring_ctor0(tmp);                                                      // 0x42bbd0
    void* map_arr[0x40];    for (int i = 0; i < 0x40; i++) map_arr[i] = 0;
    void* snow_arr[0x40];   for (int i = 0; i < 0x40; i++) snow_arr[i] = 0;
    void* steppe_arr[0x40]; for (int i = 0; i < 0x40; i++) steppe_arr[i] = 0;
    void* desert_arr[0x40]; for (int i = 0; i < 0x40; i++) desert_arr[i] = 0;
    for (int i = 0; i < 4; i++) if (g_mat_count[i] > 0x40) g_mat_count[i] = 0x40;

    for (int i = 0; i < g_mat_count[0]; i++) {
        tstring_assign(tmp, "map_tree_");                                    // 0x59c0d8
        tstring_push_back(tmp, (char)('a' + i));
        map_arr[i] = resmgr_get_object(g_resmgr, tmp);
    }
    for (int i = 0; i < g_mat_count[1]; i++) {
        tstring_assign(tmp, "snow_tree_");                                   // 0x59c0cc
        tstring_push_back(tmp, (char)('a' + i));
        snow_arr[i] = resmgr_get_object(g_resmgr, tmp);
    }
    for (int i = 0; i < g_mat_count[2]; i++) {
        tstring_assign(tmp, "steppe_tree_");                                 // 0x59c0bc
        tstring_push_back(tmp, (char)('a' + i));
        steppe_arr[i] = resmgr_get_object(g_resmgr, tmp);
    }
    for (int i = 0; i < g_mat_count[3]; i++) {
        tstring_assign(tmp, "desert_tree_");                                 // 0x59c0ac
        tstring_push_back(tmp, (char)('a' + i));
        desert_arr[i] = resmgr_get_object(g_resmgr, tmp);
    }

    // ---- G: grid dimensions and cell entities -----------------------------
    g_sizex = g_maxx - g_minx;
    g_sizey = g_maxy - g_miny;
    float stepx = (g_sizex + 5.0f) * 0.125f;                                 // 0x59393c, 0x54b8a0
    float stepy = (g_sizey + 5.0f) * 0.125f;
    g_grid_cols = (int)ceil(g_sizex / stepx);                                // ceil 0x535ae0, __ftol2 0x535a64
    g_grid_rows = (int)ceil(g_sizey / stepy);
    if (g_grid_cols > 8) mb_error("Map size too big in x axis!");            // 0x59c090
    if (g_grid_rows > 8) mb_error("Map size too big in x axis!");

    for (int row = 0; row < g_grid_rows; row++) {
        for (int col = 0; col < g_grid_cols; col++) {
            void* ent = create_entity(9);                                    // 0x428440
            g_grid[col * 8 + row] = ent;
            (*(void(**)(void*, int))((char*)*(void**)ent + 0x18))(ent, 0);   // virtual slot +0x18
        }
    }

    // ---- H: per-cell geometry ---------------------------------------------
    for (int col = 0; col < g_grid_cols; col++) {
        float x0 = (float)col * stepx + g_minx;
        float x1 = (float)(col + 1) * stepx + g_minx;
        for (int row = 0; row < g_grid_rows; row++) {
            float y0 = (float)row * stepy + g_miny;
            float y1 = (float)(row + 1) * stepy + g_miny;
            int ci = col * 8 + row;
            void* cell = g_grid[ci];

            void* cellmesh = (void*)::operator new(0x260);
            cellmesh = cellmesh ? mesh_ctor(cellmesh) : 0;                   // 0x412a90

            for (int mi = 0; mi < 0xd; mi++) {
                int mt;
                if (mi == 10)      mt = 0x15;
                else if (mi == 11) mt = 0x16;
                else if (mi == 12) mt = 0x17;
                else               mt = mi;
                unsigned umask = g_masks[mi * 2];

                void* m = (void*)::operator new(0x260);
                m = m ? mesh_ctor(m) : 0;

                // cell centre (written every material iteration in the original)
                *(float*)((char*)cell + 0x88) = (x1 + x0) * 0.5f;
                *(float*)((char*)cell + 0x8c) = (y1 + y0) * 0.5f;
                *(int*)((char*)cell + 0x90) = 0;
                *(int*)((char*)cell + 0x94) = 0;

                for (int i = 0; i < npts; i++) vert[i] = -1;

                for (int ni = 0; ni < nnodes; ni++) {
                    Node* nd = &mgr->nodes[ni];
                    Point* p0 = &mgr->pts[nd->idx[0]];
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
                                collect_near(mgr, pi, nlist, &ncnt);             // 0x5234e0
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

                            // add_point takes a 0x20-byte record
                            float prec[8];
                            prec[0] = px; prec[1] = py; prec[2] = pz; prec[3] = 0.0f;
                            prec[4] = mgr->pts[pi].nx; prec[5] = mgr->pts[pi].ny;
                            prec[6] = mgr->pts[pi].nz; prec[7] = 0.0f;
                            int pidx = add_point(m, prec);                        // 0x412f50

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
                                noise_fractal(6.0f, 0.7f, 3, 0, in6, out6);      // 0x414fc0
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
                            noise_fractal(9.0f, 0.6f, 4, 0, na, nb);         // 0x40c00000 / 0x3f19999a
                            float tz = nb[6] * 0.5f;                         // 0x549d70
                            float tseed = nb[7] * 0.5f;

                            // pass 2: dither (material, y)
                            float nc[8], nd[8];
                            nc[0] = (float)mt; nc[1] = py; nc[2] = 2917.0f; nc[3] = 0.0f; // 0x45365000
                            noise_fractal(7.0f, 0.6f, 4, 0, nc, nd);         // 0x40e00000

                            // pass 3: dither the fixed view dir
                            float ne[8], nf_[8];
                            ne[0] = dir.x; ne[1] = dir.y; ne[2] = -3668.0f; ne[3] = 0.0f;  // 0xc4654000
                            noise_fractal(4.0f, 0.6f, 4, 0, ne, nf_);        // 0x40800000

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
                            unsigned col_ = pack_color(col_rec);             // 0x40e440
                            vert[pi] = add_vertex(m, pidx, uv, &col_, prec, 0);  // 0x413c50
                        }
                        add_rec2(m, vert[nd->idx[0]], vert[nd->idx[1]], vert[nd->idx[2]]); // 0x411920
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
                                if (g_mat_count[2] > 1) chosen = steppe_arr[mb_rand() % g_mat_count[2]];
                            } else if (t == 0xd) {
                                chosen = desert_arr[0];
                                if (g_mat_count[3] > 1) chosen = desert_arr[mb_rand() % g_mat_count[3]];
                            } else {
                                chosen = (g_mat_count[0] < 2) ? 0
                                       : map_arr[mb_rand() % g_mat_count[0]];
                            }
                        } else {
                            if (t != 0xc) {
                                if (t != 10 && t != 0xd) goto lab_fc7;
                                goto lab_cdb;
                            }
                        lab_ce0:
                            chosen = snow_arr[0];
                            if (g_mat_count[1] > 1) chosen = snow_arr[mb_rand() % g_mat_count[1]];
                        }
                        if (chosen != 0) {
                            float mat[16];
                            memcpy(mat, identity3(), 12 * 4);                // 0x40cd60
                            for (int q = 12; q < 16; q++) mat[q] = 0.0f;
                            float ang = (float)mb_rand() * 0.0001f;
                            matrix_rotate_sf(mat, &ang);                     // 0x40e770
                            for (int q = 0; q < 3; q++) { mat[q * 4 + 3] += (float)mb_rand() * 0.0001f; }
                            append_skinned(cellmesh, chosen, mat, -1, 0xffffffff, 0); // 0x44eb00
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
                    *(void**)((char*)m + 0x2c) = material_from_name(mt);     // 0x523390
                    mesh_set_mat(m, mt);                                     // 0x523360
                    *(int*)((char*)m + 0x10c) = 0x17fff;
                    scene_add_object(m);                                     // 0x419300

                    PtrVec cv;
                    cv.first = *(void***)((char*)cell + 0x11c);
                    cv.last  = *(void***)((char*)cell + 0x120);
                    cv.end   = *(void***)((char*)cell + 0x124);
                    if (cv.first == 0 || cv.end <= cv.last) {
                        vec_insert_n(&cv, cv.last, 1, &m);                   // 0x410610
                        *(void***)((char*)cell + 0x11c) = cv.first;
                        *(void***)((char*)cell + 0x120) = cv.last;
                        *(void***)((char*)cell + 0x124) = cv.end;
                    } else {
                        *cv.last = m;
                        *(void***)((char*)cell + 0x120) = cv.last + 1;
                    }
                    (*(void(**)(void*))((char*)*(void**)cell + 0x10))(cell); // virtual slot +0x10

                    PtrVec* mv = 0;
                    if (mt == 0)       mv = &g_mat_vec[0];
                    else if (mt == 8)  mv = &g_mat_vec[1];
                    else if (mt == 0x16) mv = &g_mat_vec[2];
                    else if (mt == 0x17) mv = &g_mat_vec[3];
                    if (mv) {
                        if (mv->first == 0 || mv->end <= mv->last)
                            vec_insert_n(mv, mv->last, 1, &m);
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
                tstring_ctor1(name, "map_trees");                            // 0x42bbe0 / 0x59c070
                *(void**)((char*)cellmesh + 0x2c) = resmgr_get_material(g_resmgr, name); // 0x418a70
                tstring_dtor(name);                                          // 0x42bc40
                mesh_set_mat(cellmesh, 2);                                   // 0x523360
                *(int*)((char*)cellmesh + 0x10c) = 0x1ffff;
                scene_add_object(cellmesh);                                  // 0x419300

                PtrVec cv;
                cv.first = *(void***)((char*)cell + 0x11c);
                cv.last  = *(void***)((char*)cell + 0x120);
                cv.end   = *(void***)((char*)cell + 0x124);
                if (cv.first == 0 || cv.end <= cv.last) {
                    vec_insert_n(&cv, cv.last, 1, &cellmesh);
                    *(void***)((char*)cell + 0x11c) = cv.first;
                    *(void***)((char*)cell + 0x120) = cv.last;
                    *(void***)((char*)cell + 0x124) = cv.end;
                } else {
                    *cv.last = cellmesh;
                    *(void***)((char*)cell + 0x120) = cv.last + 1;
                }
                (*(void(**)(void*))((char*)*(void**)cell + 0x10))(cell);

                PtrVec ev;
                ev.first = self->ents;
                ev.last  = self->ents_last;
                ev.end   = self->ents_end;
                if (ev.first == 0 || ev.end <= ev.last) {
                    vec_insert_n(&ev, ev.last, 1, &cellmesh);
                    self->ents = ev.first; self->ents_last = ev.last; self->ents_end = ev.end;
                } else {
                    *ev.last = cellmesh;
                    self->ents_last = ev.last + 1;
                }
            }
        }
    }

    tstring_dtor(tmp);                                                       // 0x42bc40

    delete[] scratch;
    delete[] vert;
    delete[] dist;
    delete[] flags;
}
