// Slice s0052bf40: the terrain brush/grid update.
//   0x52bf40  Terrain::update   -- clear the brush grid, drive the brush over the grid cells around
//                                 the cursor (0x52c2b0), regenerate the mode-0/1/2 objects and
//                                 rebuild every layer chunk of the touched cells (0x47ff60).
//   0x52c290  Terrain::clear    -- zero the 0x177-byte brush grid at this+0x845e030.
// Layouts follow the neighbouring slices (s0052c2b0 brush, s0052f250 map editor, s0052b5c0 mission).
#include <windows.h>
#include <string.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

extern "C" int _fltused = 0;

struct CampaignGame {
    char pad0[0x1a094];
    int game_mode;                                      // +0x1a094
    char pad1a098[0x1a0d4 - 0x1a098];
    float f1a0d4;                                       // +0x1a0d4
    float f1a0d8;                                       // +0x1a0d8
    float f1a0dc;                                       // +0x1a0dc
};
extern CampaignGame* g_game;                            // 0x5e2930
extern int g_5e28fc;                                    // 0x5e28fc

struct Terrain {
    char pad0[0x60];
    int f60;                                            // +0x60
    char pad64[0x278 - 0x64];
    int f278;                                           // +0x278
    int f27c;                                           // +0x27c
    char pad280[0x288 - 0x280];
    char cells[0xc3b0dc - 0x288];                       // +0x288
    void* f_c3b0dc;                                     // +0xc3b0dc
    float f_c3b0e0;                                     // +0xc3b0e0
    char pad_c3b0e4[0x845d984 - 0xc3b0e4];
    int f845d984;                                       // +0x845d984
    int f845d988;                                       // +0x845d988
    char pad845d98c[0x845e030 - 0x845d98c];
    char f845e030[0x64];                                // +0x845e030
    char f845e094[0x10000];                             // +0x845e094

    __declspec(noinline) void clear();                  // 0x52c290
    void update(int* objs, vector4* pos, int sel, int mode, int flag);  // 0x52bf40
    void gen_object(int* objs, int index);                              // 0x467250
    void brush(int* objs, int* sel, int* col, float* d, int* mode, int* flag, char* place); // 0x52c2b0
    void build_chunk(int layer, int blend, int cx, int cy, float h);     // 0x47ff60
};
extern Terrain g_terrain;                               // 0xa76630

// =====================================================================================================
//  Terrain::update
// =====================================================================================================
// @ 0x52bf40
void Terrain::update(int* objs, vector4* pos, int sel, int mode, int flag)
{
    clear();

    int m = mode;
    if (m == 1)
        sel = objs[1];
    else if (m == 2)
        sel = objs[2];

    float speed;
    if (g_game->game_mode == 1)
        speed = g_game->f1a0d4;
    else if (g_game->game_mode == 3)
        speed = g_game->f1a0dc;
    else
        speed = g_game->f1a0d8;

    float r1 = speed * speed;
    float r2 = this->f_c3b0e0 * 10.0f + speed;
    r2 = r2 * r2;

    int row = 0;
    if (this->f27c >= 0) {
        do {
            int col = 0;
            if (this->f278 >= 0) {
                do {
                    int off = (col * 0xfb + row) * 0x94;
                    float dx = *(float*)((char*)this + off + 0x288) - pos->x;
                    float dy = *(float*)((char*)this + off + 0x28c) - pos->y;
                    float dz = *(float*)((char*)this + off + 0x290) - pos->z;
                    float d2 = dx * dx + (dz * dz + dy * dy);
                    if ((m == 0 && d2 <= r2) || d2 <= r1) {
                        char place = 1;
                        float d = (float)sqrt(d2);
                        if (!(d <= r1))
                            place = 0;
                        if (sel < 0)
                            return;
                        if (sel >= objs[0])
                            return;
                        brush(objs, &sel, &col, &d, &m, &flag, &place);
                    }
                    col++;
                } while (col <= this->f278);
            }
            row++;
        } while (row <= this->f27c);
    }

    if (m == 0) {
        if (objs[0] - 2 > 0) {
            int i = 0;
            int rowbase = 0;
            do {
                char found = 0;
                if (this->f845d984 >= 0) {
                    int j = 0;
                    do {
                        if (this->f845d988 >= 0) {
                            int k = 0;
                            if (i + 4 < 0xf) {
                                do {
                                    if (this->f845e094[k + (rowbase + j) * 5] != 0)
                                        found = 1;
                                    k++;
                                } while (k <= this->f845d988);
                            }
                        }
                        j++;
                    } while (j <= this->f845d984);
                }
                if (found)
                    this->gen_object(objs, i);
                i++;
                rowbase += 5;
            } while (i < objs[0] - 2);
        }
    } else if (m == 1) {
        this->gen_object(objs, objs[1]);
    } else if (m == 2) {
        this->gen_object(objs, objs[2]);
    }

    char* rec = (char*)this->f_c3b0dc;
    float a = *(float*)(rec + 0x68);
    float b = *(float*)(rec + 0x6c);
    float c = *(float*)(rec + 0x70);
    float h = (c + b + a) * 0.3f;

    if (this->f845d984 > 0) {
        for (int bx = 0; bx < this->f845d984; bx++) {
            if (this->f845d988 > 0) {
                for (int by = 0; by < this->f845d988; by++) {
                    for (int layer = 0; layer < 0xf; layer++) {
                        if (this->f845e030[by + (bx + layer * 5) * 5] != 0) {
                            int blend = -1;
                            if (g_5e28fc != 0 && this->f60 != 0) {
                                if (layer == 1)
                                    blend = 2;
                                else if (layer == 2)
                                    continue;
                            }
                            this->build_chunk(layer, blend, bx, by, h);
                        }
                    }
                }
            }
        }
    }
}

// =====================================================================================================
//  Terrain::clear
// =====================================================================================================
// @ 0x52c290
__declspec(noinline) void Terrain::clear()
{
    // written as a byte loop (not memset) so cl expands it exactly like the
    // original: lea edi,[ecx+0x845e030]; xor eax,eax; mov ecx,0x5d;
    // rep stosd; stosw; stosb.
    char* p = (char*)this + 0x845e030;
    for (int i = 0; i < 0x177; i++)
        p[i] = 0;
}
