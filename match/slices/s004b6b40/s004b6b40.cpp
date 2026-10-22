// Slice s004b6b40: the world-map navigation-object path finder and its per-party drivers.
//   0x4b6b40  WorldMap::find_path -- A* over the map's 0x88-byte formation nodes; fills the node-id chain,
//             the per-node positions and the stall limit of the target node
//   0x4b71e0  Party::update_nav_anim -- per-frame visibility/sound update of the party's map object
//   0x4b73c0  Party::start_path -- resolves the target agent and kicks off find_path for it
// Names provisional; layouts from these disassemblies and the neighbouring slices s004b3420 (WorldMap,
// Party order fields), s004b54a0 (0xc-byte vector element) and s00438860 (the 0xc-byte heap helpers).
#pragma warning(disable:4234)     // __thiscall on a free function
#include "rgl.h"

using rgl::vector4;

// ---- navigation node ------------------------------------------------------------------------------------
struct NavRec {                                     // 0x88-byte formation record of the world map
    int n;                                          // +0x00 neighbour/vertex count
    int pad4[4];
    int agents[10];                                 // +0x14 vertex index of edge i
    unsigned mask;                                  // +0x24 per-edge flags (roads, fords, ...)
    int nbr[10];                                    // +0x28 neighbour record (-1: none)
    float dist[10];                                 // +0x38 per-edge keep distance
    int pad60[3];
    int g;                                          // +0x6c A* cost stamp (compare with WorldMap::gen)
    int parent;                                     // +0x70 A* parent record
    float x, y, z;                                  // +0x74 node position
    float f80;                                      // +0x80 keep-distance/time limit of the node
};

struct MapVert { char pad0[0x14]; float x, y, z, w; };  // 0x24-byte position entry of the map mesh

// ---- open list (0xc-byte element: node id, cost so far, priority) ---------------------------------------
struct OpenItem {                                   // 0x0c
    int node;
    float cost;
    float dist;
};

// the VC7.1 heap helpers of the 0xc-byte element (cdecl; the comparator orders by +8 and is ignored here)
extern void adjust_heap(OpenItem* first, int hole, int bottom, OpenItem val);   // 0x438b10
extern void shift_heap_into(OpenItem* out, OpenItem* first, int hole, int top, OpenItem val, int flag, int tag); // 0x4b2610
extern void pop_heap_repair(OpenItem* first, OpenItem* last, OpenItem* dest, OpenItem val);  // 0x4b2520
extern void heap_copy(OpenItem* begin, OpenItem* end, int flag);               // 0x4b2710
extern void __thiscall vec_push_back(void* vec, const OpenItem* item);         // 0x4b5bb0

struct MapGlobal {                                  // object at 0xa732b4
    char pad0[0xb0];
    struct WorldMap* world;                         // +0xb0
};
extern MapGlobal* g_map;                            // 0xa732b4

struct WorldMap {
    char pad0[0x14];
    NavRec* recs;                                   // +0x14 A* records (a vector: begin/end here)
    char pad18[0x24 - 0x18];
    MapVert* detail_verts;                          // +0x24 vertex table of the returned positions
    char pad28[0x34 - 0x28];
    OpenItem* open_begin;                           // +0x34
    OpenItem* open_end;                             // +0x38
    OpenItem* open_cap;                             // +0x3c
    char cmp;                                       // +0x40 comparator state
    char pad41[0x64 - 0x41];
    int gen;                                        // +0x64 A* generation stamp

    void reset(void);                               // 0x4b33c0
    int find_path(int from, int to, int* path, float* detail, int* count, int* visited, int* guard,
                  unsigned* mask_req, unsigned* mask_ban, char* avoid);      // 0x4b6b40
};

// ---- parties ----------------------------------------------------------------------------------------------
struct GObj {
    char pad0[0x88];
    float pos[4];                                   // +0x88 sound/visual position
    char pad98[0x11c - 0x98];
    int* begin;                                     // +0x11c
    int* end;                                       // +0x120
    char pad124[0x140 - 0x124];
    char visible;                                   // +0x140
};

struct Party {
    char pad0[4];
    int agent;                                      // +0x04
    int id;                                         // +0x08
    char padc[0x10 - 0xc];
    int type;                                       // +0x10 (== 1 on the world map)
    char pad14[0x30 - 0x14];
    float x;                                        // +0x30
    float y;                                        // +0x34
    char pad38[0x44 - 0x38];
    float opacity;                                  // +0x44
    char pad48[0x50 - 0x48];
    GObj* obj;                                      // +0x50
    int f54;                                        // +0x54
    int f58;                                        // +0x58
    char pad5c[0x60 - 0x5c];
    int clock;                                      // +0x60
    char pad64[0x68 - 0x64];
    __int64 stamp;                                  // +0x68 (time of the last commit)
    float wait;                                     // +0x70
    char pad74[0x80 - 0x74];
    int anim;                                       // +0x80
    char pad84[0x88 - 0x84];
    __int64 anim_stamp;                             // +0x88 (time the sound was started)
    int sound;                                      // +0x90
    char pad94[0x95c - 0x94];
    float f95c;                                     // +0x95c
    float f960;                                     // +0x960
    char pad964[0x96c - 0x964];
    int path[0x400];                                // +0x96c node-id chain
    float detail[0x400 * 4];                        // +0x196c per-node positions
    int cursor;                                     // +0x596c path cursor
    int count;                                      // +0x5970 path length
    Party* target;                                  // +0x5974
    char pad5978[0x5ac0 - 0x5978];
    int visited;                                    // +0x5ac0

    void update_nav_anim(int unused);               // 0x4b71e0
    int start_path(int from, float tx, float ty);   // 0x4b73c0
};

struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                                  // +0x138d0
    char pad138d4[0x138dc - 0x138d4];
    unsigned count;                                 // +0x138dc
};

struct GameData {
    char pad0[0x78];
    int cur_party;                                  // +0x78
    char pad7c[0x95 - 0x7c];
    char f95;                                       // +0x95
    char pad96[0xf8 - 0x96];
    PartyMgr parties;                               // +0xf8
};
extern GameData* g_game;                            // 0x5e2930
extern __int64 g_ticks[];                           // 0x5e2948

extern float frand();                               // 0x40e400 (st0)
extern float __fastcall anim_elapsed(int* anim);    // 0x432c10 (the anim pointer in ecx, result in st0)
struct SoundMgr {                                    // object at 0x5aa128
    int play(int sound, int priority, const float* pos, float pitch, float* out);   // 0x442650 (ret 0x14)
};
extern SoundMgr g_sound_mgr;                        // 0x5aa128
extern int g_sound_bank;                            // 0x5aa304 (the sound index, passed by value)
extern void __stdcall stop_sound(int id);           // 0x5342a6 (FSOUND_StopSound@4)
extern unsigned pack_color(const float* rgba);      // 0x40e440
extern void __fastcall commit_party(Party* self);   // 0x4b6870

extern float g_half;        // 0x549d70 = 0.5f
extern float g_one;         // 0x549cdc = 1.0f
extern float g_105;         // 0x59364c = 10.5f
extern float g_10;          // 0x549d74 = 10.0f
extern float g_005;         // 0x54b4d0 = 0.05f
extern float g_vis;         // 0x54a6f0 = 0.01f
extern float g_scale;       // 0x54a6d0 = 1e-5f

// ---- the path finder ------------------------------------------------------------------------------------
// @ 0x4b6b40
int WorldMap::find_path(int from, int to, int* path, float* detail, int* count, int* visited,
                        int* guard, unsigned* mask_req, unsigned* mask_ban, char* avoid)
{
    (void)guard;

    // drain whatever the previous search left on the open list
    if (open_begin != 0) {
        for (;;) {
            int n = (int)((open_end - open_begin) / (int)sizeof(OpenItem));
            if (n == 0)
                break;
            if (n > 1) {
                OpenItem last = open_end[-1];
                open_end[-1] = open_begin[0];
                adjust_heap(open_begin, 0, n - 1, last);
            }
            if (open_begin != 0) {
                n = (int)((open_end - open_begin) / (int)sizeof(OpenItem));
                if (n != 0)
                    open_end -= 1;
            }
        }
    }

    reset();

    recs[from].g = gen + 1;
    recs[from].parent = from;
    {
        OpenItem root;
        root.node = from;
        root.cost = 0.0f;
        root.dist = 0.0f;
        vec_push_back((char*)this + 0x30, &root);
    }
    heap_copy(open_begin, open_end, cmp);
    *visited = 0;
    float best = 0.0f;
    float limit = 10000.0f;
    if (*avoid != 0)
        limit = 1e6f;
    int so = from * 0x88;

    if (recs[to].g <= gen) {
        for (;;) {
            if (open_begin == 0)
                break;
            if ((int)((open_end - open_begin) / (int)sizeof(OpenItem)) == 0)
                break;
            if (!(open_begin[0].dist < limit))
                break;

            *visited = *visited + 1;
            OpenItem cur = open_begin[0];
            if ((int)((open_end - open_begin) / (int)sizeof(OpenItem)) > 1) {
                OpenItem last = open_end[-1];
                shift_heap_into(open_end - 1, open_begin, 0, (int)((open_end - 1 - open_begin) /
                                                                   (int)sizeof(OpenItem)), last, cmp, 0);
            }
            if ((int)((open_end - open_begin) / (int)sizeof(OpenItem)) != 0)
                open_end -= 1;

            int node = cur.node;
            int sn = node * 0x88;
            int nidx = node * 0x22;
            int nb = 0;
            if (recs[node].n > 0) {
                int off = sn + 0x28;
                do {
                    int nbr = *(int*)((char*)recs + off);
                    if (nbr != -1) {
                        int sb = nbr * 0x88;
                        if (recs[nbr].g <= gen) {
                            recs[nbr].g = gen + 1;
                            recs[nbr].parent = node;

                            unsigned fl = *(unsigned*)((char*)recs + sb + 0x24);
                            float cost = *(float*)((char*)recs + (nidx + nb) * 4 + 0x38);
                            if ((*mask_req & fl) != *mask_req)
                                cost += 10000.0f;
                            if ((*mask_ban & fl) != 0)
                                cost += 10000.0f;

                            float* pb = (float*)((char*)recs + sb + 0x74);
                            float tz = *(float*)((char*)recs + to * 0x88 + 0x7c) - pb[2];
                            float ty = *(float*)((char*)recs + to * 0x88 + 0x78) - pb[1];
                            float tx = *(float*)((char*)recs + to * 0x88 + 0x74) - pb[0];
                            float fz = *(float*)((char*)recs + so + 0x7c) - pb[2];
                            float fy = *(float*)((char*)recs + so + 0x78) - pb[1];
                            float fx = *(float*)((char*)recs + so + 0x74) - pb[0];
                            float d_from = (float)sqrt(fx * fx + (fy * fy + fz * fz));
                            float d_to = (float)sqrt(tx * tx + (ty * ty + tz * tz));

                            cost += cur.cost;
                            if (best < cur.cost)
                                best = cur.cost;

                            OpenItem add;
                            add.node = nbr;
                            add.cost = cost;
                            add.dist = (d_to / d_from + 1.0f) * cost;
                            vec_push_back((char*)this + 0x30, &add);
                            if (open_begin != open_end) {
                                OpenItem last = open_end[-1];
                                pop_heap_repair(open_begin, open_end - 1, open_end - 1, last);
                            }
                        }
                    }
                    off += 4;
                    nb++;
                } while (nb < recs[node].n);
            }
            recs[node].g = gen + 2;
        }
    }

    if (recs[to].g <= gen) {
        *count = 0;
        return 0;
    }

    char reachable = best < 10000.0f;

    int chain[0x2000];
    int k = 0;
    int cur = to;
    while (cur != from && k < 0x2000) {
        chain[k++] = cur;
        cur = recs[cur].parent;
    }
    chain[k++] = from;
    int m = k;
    if (k > 0x400)
        m = 0x400;
    *count = m;

    if (m > 0) {
        for (int i = 0; i < m; i++) {
            int id = chain[k - 1 - i];
            path[i] = id;
            int cnt = recs[from].n;
            int idx = 0;
            while (idx < cnt && recs[from].nbr[idx] != id)
                idx++;
            int v = recs[from].agents[idx];
            MapVert* vv = &detail_verts[v];
            detail[i * 4 + 0] = vv->x;
            detail[i * 4 + 1] = vv->y;
            detail[i * 4 + 2] = vv->z;
            detail[i * 4 + 3] = vv->w;
        }
    }

    for (int i = 1; i < *count; i++) {
        detail[i * 4 - 4] = detail[i * 4 + 0];
        detail[i * 4 - 3] = detail[i * 4 + 1];
        detail[i * 4 - 2] = detail[i * 4 + 2];
        detail[i * 4 - 1] = detail[i * 4 + 3];
    }
    {
        int lastid = path[*count - 1];
        float* e = (float*)((char*)recs + lastid * 0x88 + 0x74);
        float* o = detail + *count * 4 - 4;
        o[0] = e[0];
        o[1] = e[1];
        o[2] = e[2];
        o[3] = *(float*)((char*)recs + lastid * 0x88 + 0x80);
    }
    return reachable ? 1 : 0;
}

extern float g_half;        // 0x549d70 = 0.5f
extern float g_one;         // 0x549cdc = 1.0f
extern float g_105;         // 0x59364c = 10.5f
extern float g_10;          // 0x549d74 = 10.0f
extern float g_005;         // 0x54b4d0 = 0.05f
extern float g_vis;         // 0x54a6f0 = 0.01f
extern float g_scale;       // 0x54a6d0 = 1e-5f

// @ 0x4b71e0
void Party::update_nav_anim(int)
{
    if (f54 == g_game->cur_party || f58 == g_game->cur_party) {
        obj->visible = 0;
        return;
    }

    if (type == 1) {
        if (opacity > g_half && g_game->f95 == 0) {
            if (anim_elapsed(&anim) > g_105) {
                sound = g_sound_mgr.play(g_sound_bank, 0, obj->pos, 1.0f, 0);
                anim_stamp = g_ticks[anim];
            }
        }
        if (opacity < g_005 && sound != -1) {
            if (anim_elapsed(&anim) < g_10)
                stop_sound(sound);
        }
        {
            __int64 elapsed = g_ticks[clock] - stamp;
            float secs = (float)elapsed * g_scale;
            if (wait < secs) {
                wait = frand() + g_one;
                stamp = g_ticks[clock];
                commit_party(this);
            }
        }
    }

    GObj* o = obj;
    if (o) {
        float rgba[4];
        rgba[0] = 1.0f;
        rgba[1] = 1.0f;
        rgba[2] = 1.0f;
        rgba[3] = opacity;
        unsigned color = pack_color(rgba);
        o->visible = (opacity > g_vis) ? 1 : 0;
        unsigned i = 0;
        for (;;) {
            int* b = obj->begin;
            if (b == 0)
                break;
            if (i >= (unsigned)(((char*)obj->end - (char*)b) >> 2))
                break;
            *(unsigned*)(b[i] + 0x3c) = color;
            i++;
        }
    }
}

// @ 0x4b73c0
int Party::start_path(int from, float tx, float ty)
{
    int mask_req = 0;
    int mask_ban = 0;
    char mine = (id == g_game->cur_party);
    int ok = g_map->world->find_path(target->agent, from, path, detail, &count, &visited, &from,
                                     (unsigned*)&mask_req, (unsigned*)&mask_ban, &mine);
    f95c = tx;
    f960 = ty;
    cursor = 0;
    return ok;
}
