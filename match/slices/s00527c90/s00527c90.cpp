// Slice s00527c90 (batch ds1): the scene/map .sco save path.
//
//   0x527c90  GameData::save_scene      -- open Modules\<mod>\SceneObj\<name>.sco and serialise the scene record
//   0x527da0  SceneData::save           -- write both object lists of a scene record
//   0x527ef0  SceneObj::save            -- write one scene object
//   0x527f90  Mission::save_scene_data  -- reset both in-memory streams and serialise the live grids
//   0x528000  GridSet::save             -- write the grid count, dimensions and every grid
//   0x528060  Grid::save                -- run-length encode one grid
//   0x528410  Mission::release_item     -- free the entity behind one per-item slot
//   0x528470  Resource::release         -- mark a resource's sub-resources for deletion
//
// Class layouts: tstring / stream_base / bin_writer come from mbbin.h (verified in s00417850..s00418a70);
// the Mission and GameData field offsets are those of s005261e0 (0x5e292c / 0x5e2930).  Names provisional.
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "mbbin.h"

// ---- engine string helpers -----------------------------------------------------------------------------
// tstring::c_str() is the inlined `buf ? buf : (char*)&buf` seen at 0x527cc1.
struct GameData;
struct Mission;

// The scene record (0x494 bytes) in the GameData scene table at +0x13a50.
struct SceneObjRec;

struct GameData {
    char pad0[0x13a50];
    SceneObjRec* scenes;                    // +0x13a50
    tstring scene_path(int idx);            // 0x463950 (returns by hidden pointer; ret 8)
    const char* module_path(const char* sub);   // 0x4639d0 (ret 4)
    void save_scene(int idx);               // 0x527c90 (ret 4)
};

extern GameData* g_game;                    // 0x5e2930
extern Mission* g_mission;                  // 0x5e292c

struct Mesh;

// Resource with a sub-resource vector at +0x12c and a flag word at +8 (0x528470).
struct Resource {
    char pad0[8];
    unsigned flags;                         // +8
};

struct ResourceOwner {
    char pad0[0x12c];
    Resource** first;                       // +0x12c
    Resource** last;                        // +0x130
    __declspec(noinline) void release();    // 0x528470
};

struct SceneObjRec {
    char pad0[0x30];
    unsigned flags30;                       // +0x30
    char pad34[0x108 - 0x34];
    char* objs_first;                       // +0x108 (vector of 0x1c8-byte scene objects)
    char* objs_last;                        // +0x10c
    char pad110[0x414 - 0x110];
    mem_stream stream_a;                    // +0x414 (0x30 bytes)
    mem_stream stream_b;                    // +0x444
    char pad474[0x494 - 0x474];
    void save(stream_base* s, int arg);     // 0x527da0 (ret 8)
};

// one 0x1c8-byte scene object
struct SceneObj {
    char pad0[0x10];
    int f10;                                // +0x10
    int f14;                                // +0x14
    char pad18[0x98 - 0x18];
    char mat[0x158 - 0x98];                 // +0x98 matrix (0x30 rotation + 3 floats)
    float f158, f15c, f160;                 // +0x158
    char pad164[0x188 - 0x164];
    int f188;                               // +0x188
    int f18c;                               // +0x18c
    tstring name;                           // +0x190
    int f1a0;                               // +0x1a0
    int f1a4;                               // +0x1a4
    char pad1a8[0x1b4 - 0x1a8];
    void* res;                              // +0x1b4 (owned resource)
    char pad1b8[0x1c8 - 0x1b8];
    void save(stream_base* s);              // 0x527ef0 (ret 4)
};

void w_matrix(stream_base* s, void* m);     // 0x415ef0 (cdecl)

// ---- grid container (Mission+0x63d0) -------------------------------------------------------------------
struct Grid {
    tstring name;                           // +0
    int type;                               // +0x10
    float* data;                            // +0x14
    void save(stream_base* s, int* dims);   // 0x528060 (ret 8)
};

struct GridSet {
    int count;                              // +0
    char pad4[0xc - 4];
    Grid grids[1];                          // +0xc
    char pad144[0x144 - 0xc - sizeof(Grid)];
    int width;                              // +0x144
    int height;                             // +0x148
    void save(stream_base* s);              // 0x528000 (ret 4)
};

// ---- Mission (0x5e292c) --------------------------------------------------------------------------------
struct MissionSub {
    char pad0[0x1c];
    int count;                              // +0x1c
    char* first;                            // +0x20
    char* last;                             // +0x24
    void save(stream_base* s);              // 0x524f00 (ret 4)
};

struct Mission {
    char pad0[0x5df4];
    int cur_scene;                          // +0x5df4 (index into GameData::scenes)
    char pad5df8[0x63d0 - 0x5df8];
    GridSet grids;                          // +0x63d0
    char pad668c[0x668c - 0x63d0 - sizeof(GridSet)];
    char* item_array;                       // +0x668c (per-item array, 0x1c8 apart)
    char pad6690[0x6698 - 0x6690];
    MissionSub other;                       // +0x6698
    void save_scene_data();                 // 0x527f90
    int release_item(int idx);              // 0x528410 (ret 4)
};

// ---- world (Mission+0) ---------------------------------------------------------------------------------
struct World {
    int remove_entity(int id);              // 0x41f0d0 (ret 4)
};

struct MissionHdr {
    World* world;                           // +0
};

// @ 0x527c90
void GameData::save_scene(int idx)
{
    tstring path = scene_path(idx);
    bin_writer w;
    const char* p = module_path(path.c_str());
    w.name.assign(p);
    w.is_open = 0;
    w.fp = fopen(p, "wb");
    if (w.fp) {
        w.failed = 0;
        w.is_open = 1;
    } else {
        w.failed = 1;
    }
    scenes[idx].save(&w, 0);
    if (w.is_open) {
        stream_base& s = w;
        if (s.flush() == 0)
            fclose(w.fp);
        w.failed = 1;
        w.is_open = 0;
    }
}

// @ 0x527da0
void SceneObjRec::save(stream_base* s, int arg)
{
    int version = -0x2cd;
    s->w_int(&version);
    int four = 4;
    s->w_int(&four);
    int count = 0;
    for (unsigned i = 0;
         objs_first && i < (unsigned)((objs_last - objs_first) / 0x1c8);
         i++) {
        if (((SceneObj*)objs_first)[i].f10 == arg)
            count++;
    }
    s->w_int(&count);
    for (unsigned i = 0;
         objs_first && i < (unsigned)((objs_last - objs_first) / 0x1c8);
         i++) {
        if (((SceneObj*)objs_first)[i].f10 == arg)
            ((SceneObj*)objs_first)[i].save(s);
    }
    stream_b.rewind();
    s->w_stream(&stream_b);
    if (!stream_a.at_end()) {
        int marker = 0xff4ad1a6;
        s->w_int(&marker);
        stream_a.rewind();
        s->w_stream(&stream_a);
    }
}

// @ 0x527ef0
void SceneObj::save(stream_base* s)
{
    s->w_int(&f188);
    s->w_int(&f18c);
    int tmp;
    s->w_int(&tmp);
    w_matrix(s, &mat[0]);
    s->w_tstr(&name);
    s->w_int(&f1a0);
    s->w_int(&f1a4);
    float* p = &f158;
    s->w_float(p).w_float(p + 1).w_float(p + 2);
}

// @ 0x527f90
void Mission::save_scene_data()
{
    SceneObjRec* rec = g_game->scenes + cur_scene;
    if (rec->flags30 & 0x100) {
        rec->stream_a.reset();
        grids.save(&rec->stream_a);
    }
    rec->stream_b.reset();
    other.save(&rec->stream_b);
}

// @ 0x528000
void GridSet::save(stream_base* s)
{
    s->w_int(&count);
    s->w_int(&width);
    s->w_int(&height);
    int i = 0;
    if (count > 0) {
        Grid* g = grids;
        do {
            g->save(s, &width);
            i++;
            g++;
        } while (i < count);
    }
}

// @ 0x528060
void Grid::save(stream_base* s, int* dims)
{
    s->w_int(&type);
    s->w_tstr(&name);
    if (data == 0) {
        int zero = 0;
        s->w_int(&zero);
        return;
    }
    int one = 1;
    s->w_int(&one);

    bool run = false;
    int count = 0;
    int gap = 0;
    int saved_row = 0;
    int saved_col = 0;
    int row = 0;
    if (dims[1] <= 0) {
        s->w_int(&gap);
        return;
    }
    for (row = 0; row < dims[1]; row++) {
        int col;
        for (col = 0; col < dims[0]; col++) {
            float v = data[col * 0x1a4 + row];
            bool transparent;
            if (type == -0x3115)
                transparent = ((*(int*)&v & 0xffffff) == 0xffffff);
            else
                transparent = (-0.001f < v && v < 0.001f);
            if (transparent) {
                if (run) {
                    s->w_int(&count);
                    int n = count;
                    int r = saved_row;
                    int c = saved_col;
                    for (int i = 0; i < n; i++) {
                        if (type < 0)
                            s->w_float(&data[c * 0x1a4 + r]);
                        else {
                            float y = data[c * 0x1a4 + r] * 255.0f;
                            double f = floor((double)y);
                            int ci = (int)f;
                            if (y - f > 0.5f)
                                ci++;
                            unsigned char b;
                            if (ci < 0)
                                b = 0;
                            else {
                                b = 0xff;
                                if (ci < 0x100)
                                    b = (unsigned char)ci;
                            }
                            s->w_byte(&b);
                        }
                        c++;
                        if (c >= dims[0]) {
                            c = 0;
                            r++;
                        }
                    }
                    count = 0;
                    run = false;
                    gap = 1;
                } else {
                    gap++;
                }
            } else {
                if (run) {
                    count++;
                } else {
                    run = true;
                    s->w_int(&gap);
                    gap = 0;
                    count = 1;
                    saved_row = row;
                    saved_col = col;
                }
            }
        }
    }
    if (run) {
        s->w_int(&count);
        int n = count;
        int r = saved_row;
        int c = saved_col;
        for (int i = 0; i < n; i++) {
            if (type < 0)
                s->w_float(&data[c * 0x1a4 + r]);
            else {
                float y = data[c * 0x1a4 + r] * 255.0f;
                double f = floor((double)y);
                int ci = (int)f;
                if (y - f > 0.5f)
                    ci++;
                unsigned char b;
                if (ci < 0)
                    b = 0;
                else {
                    b = 0xff;
                    if (ci < 0x100)
                        b = (unsigned char)ci;
                }
                s->w_byte(&b);
            }
            c++;
            if (c >= dims[0]) {
                c = 0;
                r++;
            }
        }
    } else {
        s->w_int(&gap);
    }
}

// @ 0x528410
int Mission::release_item(int idx)
{
    int off = idx * 0x1c8;
    char* p = (char*)item_array + off;
    if (*(void**)(p + 0x1b4)) {
        (*(ResourceOwner**)(p + 0x1b4))->release();
        int id = *(int*)((char*)*(void**)(item_array + off + 0x1b4) + 8);
        int r = ((MissionHdr*)this)->world->remove_entity(id);
        *(void**)(item_array + off + 0x1b4) = 0;
        return r;
    }
    return (int)p;
}

// @ 0x528470
void ResourceOwner::release()
{
    Resource** p = first;
    if (p < last) {
        do {
            (*p)->flags |= 0x8000;
            p++;
        } while (p < last);
    }
}
