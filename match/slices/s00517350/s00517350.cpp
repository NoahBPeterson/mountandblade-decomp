// Slice s00517350: the daily colour refresh of the campaign map (Game methods).
//  0x517350  Game::update_colors - blends the three colour fields (base +0x48, fog +0x58, sky +0x68)
//            against the per-time-of-day colours from 0x5176e0 / 0x517820 / 0x517770 with the factor
//            g_step*0.9 (0x5a9c20 * 0.9), lifts them to linear space (pow 1/2.2) when the "cheap" path
//            0x5e28fc is clear, packs the sun colour (0x40e440) and pushes it into every entity colour
//            (+0x3c) and the sun entity (+0x44), and copies the results into the fog object (+0x139dc
//            vector at +4) and the scene (+0x68 colour, +0x74 alpha = 1).
//  0x5176e0  base/sun colour from the fog parameters at +0x13b3c / +0x13b40.
//  0x517770  sky colour from the same two parameters.
//  0x517820  fog colour, square-rooted when 0x5e28fc is clear.
//  0x5178d0  writes a colour into every cell record of the 0xa73094 host grid.
//  0x517960  advances a slot cursor to the next used 0x98-byte pool record.
//  0x5179b0  returns the first used pool slot index.
// flags /O2 /MT /Gy /TP
#include <math.h>
#include <windows.h>
#include "rgl.h"

using rgl::vector4;

// ---- records -------------------------------------------------------------------------------------------
struct Entity {                                      // a scene object; only the colour is used here
    char pad0[0x3c];
    unsigned color;                                  // +0x3c
};

struct ColorTarget {                                 // this+0x139dc
    char pad0[4];
    vector4 v;                                       // +4
};

struct Model {                                       // a grid cell: a scene object with an entity list
    char pad0[0x11c];
    Entity** ents;                                   // +0x11c
    Entity** ents_end;                               // +0x120
};

struct Scene {                                       // g_scene at 0xa732b4
    char pad0[0x68];
    vector4 color;                                   // +0x68 (its w lane is the +0x74 alpha)
};

// ---- the game object (g_game) --------------------------------------------------------------------------
struct Game {
    char pad0[0x38];
    Entity** ents;                                   // +0x38  begin
    Entity** ents_end;                               // +0x3c  end
    char pad40[4];
    Entity* sun_ent;                                 // +0x44
    vector4 color_base;                              // +0x48
    vector4 color_fog;                               // +0x58
    vector4 color_sky;                               // +0x68
    char pad78[0x139dc - 0x78];
    ColorTarget* target;                             // +0x139dc
    char pad139e0[0x13b3c - 0x139e0];
    float f13b3c;                                    // +0x13b3c
    float f13b40;                                    // +0x13b40

    void update_colors();                            // 0x517350
    vector4* sun_color(vector4* out);                // 0x5176e0
    vector4* sky_color(vector4* out);                // 0x517770
    vector4* fog_color(vector4* out);                // 0x517820
    void set_grid_color(unsigned* c);                // 0x5178d0
};

// ---- the paged pool of 0x98-byte records ---------------------------------------------------------------
struct Pool {
    int count;                                       // +0
    char pad4[0x14];
    char** pages;                                    // +0x18  page = 16 records of 0x98 bytes
    void next(int* index);                           // 0x517960
    int first();                                     // 0x5179b0
};

// ---- globals -------------------------------------------------------------------------------------------
extern float g_step;                                 // 0x5a9c20  per-frame step (seconds)
extern int g_env2;                                   // 0x5e28fc  0 = linear colour path
extern Scene* g_scene;                               // 0xa732b4
extern int g_grid_rows;                              // 0xa73298
extern int g_grid_cols;                              // 0xa73294
extern char g_grid[];                                // 0xa73094  host grid (array of cell pointers)

unsigned pack_color(const float* c);                 // 0x40e440
bool is_day();                                        // 0x435700  true in the 4..20 h window

#define GAMMA (0.45454543828964233f)                 // 0x5a9f1c  1 / 2.2 (the compiler's rounding)

// lerp: dst = dst*s + src*t (the 4th lane rides along uninitialised)
static void blend_color(vector4& dst, const vector4& src, float t, float s)
{
    vector4 r;
    r.x = src.x * t + dst.x * s;
    r.y = src.y * t + dst.y * s;
    r.z = src.z * t + dst.z * s;
    dst = r;
}

// @ 0x517350
void Game::update_colors()
{
    float t = g_step * 0.9f;
    float s = 1.0f - t;

    vector4 c;
    sun_color(&c);
    blend_color(color_base, c, t, s);

    fog_color(&c);
    blend_color(color_fog, c, t, s);

    sky_color(&c);
    blend_color(color_sky, c, t, s);

    vector4 sun;
    sun.x = color_base.x + color_sky.x;
    sun.y = color_base.y + color_sky.y;
    sun.z = color_base.z + color_sky.z;
    sun.w = 1.0f;

    if (g_env2 == 0) {
        sun.x = (float)pow(sun.x, GAMMA);
        sun.y = (float)pow(sun.y, GAMMA);
        sun.z = (float)pow(sun.z, GAMMA);
        unsigned col = pack_color(&sun.x);
        set_grid_color(&col);
        sun_ent->color = col;
    } else {
        unsigned col = pack_color(&sun.x);
        Entity** e = ents;
        while (e < ents_end) {
            (*e)->color = col;
            e++;
        }
        sun_ent->color = col;
    }

    if (g_env2 == 0) {
        target->v = vector4((float)pow(color_base.x, GAMMA),
                            (float)pow(color_base.y, GAMMA),
                            (float)pow(color_base.z, GAMMA));
        target->v.w = 1.0f;                      // the original copies the sun's w lane here
        g_scene->color = vector4((float)pow(color_sky.x, GAMMA),
                                 (float)pow(color_sky.y, GAMMA),
                                 (float)pow(color_sky.z, GAMMA));
    } else {
        target->v = color_base;
        g_scene->color = color_sky;
    }
    g_scene->color.w = 1.0f;                         // +0x74 is the scene alpha
}

// @ 0x5176e0
vector4* Game::sun_color(vector4* out)
{
    vector4 c;
    if (!is_day()) {
        c = vector4(0.095f, 0.116f, 0.24f);
    } else {
        float d = 3.3f - (f13b3c + f13b3c) - f13b40;
        c = vector4(d * 0.33900001645088196f, d * 0.3f, d * 0.1980000138282776f);
    }
    *out = c;
    return out;
}

// @ 0x517770
vector4* Game::sky_color(vector4* out)
{
    vector4 c;
    if (!is_day()) {
        c = vector4(0.035f, 0.04f, 0.14f);
    } else {
        float f = f13b40 + f13b3c;
        float t = (f + f + 1.0f) * 0.08f;
        float x = t + 0.24f;
        float y = t + 0.28f;
        float z = t + 0.44f;
        c = vector4(x, y, z);
    }
    *out = c;
    return out;
}

// @ 0x517820
vector4* Game::fog_color(vector4* out)
{
    vector4 c(0.65f, 0.68f, 0.71f);
    if (!is_day())
        c = vector4(0.11f, 0.18f, 0.25f);
    if (g_env2 == 0) {
        c.x = (float)sqrt(c.x);
        c.y = (float)sqrt(c.y);
        c.z = (float)sqrt(c.z);
    }
    *out = c;
    return out;
}

// @ 0x5178d0
void Game::set_grid_color(unsigned* c)
{
    int i = 0;
    if (g_grid_rows > 0) {
        int* row = (int*)g_grid;
        do {
            int j = 0;
            if (g_grid_cols > 0) {
                do {
                    Model** pp = (Model**)(row + j * 8);
                    for (Entity** e = (*pp)->ents; e < (*pp)->ents_end; e++)
                        (*e)->color = *c;
                    j++;
                } while (j < g_grid_cols);
            }
            i++;
            row = (int*)((char*)row + 4);
        } while (i < g_grid_rows);
    }
}

// @ 0x517960
void Pool::next(int* index)
{
    int i = ++*index;
    if (i >= count)
        return;
    do {
        if (*(int*)(pages[(unsigned)i >> 4] + (i & 0xf) * 0x98 + 4) != 0)
            return;
        *index = ++i;
    } while (i < count);
}

// @ 0x5179b0
int Pool::first()
{
    int i = 0;
    if (count > 0) {
        do {
            char* rec = pages[(unsigned)i >> 4] + (i & 0xf) * 0x98;
            if (*(int*)(rec + 4) != 0)
                break;
            i++;
        } while (i < count);
    }
    return i;
}
