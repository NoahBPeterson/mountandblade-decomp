// Slice s0052c2b0: the terrain "brush" of the world-map editor (0x52c2b0, 2887 bytes, __thiscall,
// ret 0x1c = 7 stack args). It maps the brush amount (arg4) through the per-map-mode response curve
// selected by g_game->game_mode (+0x1a094), then either rewrites the height/colour values of one or
// many map tiles (the 24-byte "element" records reached from arg1, whose per-cell value array at +0x14
// is a 0xac440-byte lazily allocated block) or marks the touched grid cells in this[0x845e030].
// Reconstruction of the original control flow (verified against the disassembly); NOT a byte-exact
// match: the original's x87 evaluation order and the compiler's 4x unrolling of the 0.98 scan are
// reproduced only structurally. See nonmatching.txt.
#include <windows.h>

typedef unsigned int uint;

// ---------------------------------------------------------------------------------------------------
// globals (addresses from the annotations in the disassembly)
extern char* g_game;                 // 0x5e2930  game/mission data block
extern float g_00;                   // 0x549ca8  0.0f
extern float g_10;                   // 0x549cdc  1.0f
extern float g_098;                  // 0x5911d8  0.98f (scan threshold)
extern double g_001;                 // 0x59c480  unused here (other slices)

void* __cdecl mb_new_array(uint size);      // 0x535482  ??_U@YAPAXI@Z operator new[]
unsigned __cdecl pack_color(const float* c);           // 0x40e440
float* __cdecl unpack_color(float* out, const unsigned* argb);  // 0x40e4e0

// ---------------------------------------------------------------------------------------------------
// brush element record: 24 bytes. The array header (arg1) is {int count; ...}, element i starts at
// (char*)arg1 + 0xc + i*0x18 and holds the per-cell value pointer at +0x14.
struct BrushElem {
    int    f0, f1, f2, f3, f4;
    float* v;                        // +0x14  0xac440-byte value block (0x1a4*0x2a1 dwords), or 0
    void zero();                     // 0x52ce00  clear the whole block
    void put(int* a, int* b, int* value);   // 0x52ce70  lazy-alloc + store one cell
};

struct SceneBrush {
    char pad0[0x278];
    int  w;                          // 0x278  grid width
    int  h;                          // 0x27c  grid height
    char pad280[0x30c - 0x280];
    // per-cell terrain records (0x94 bytes, x stride 0xfb rows) live at +0x30c; two floats used.
    void clamp(int* out, int* in);   // 0x52cef0  scale a cell coordinate into the grid
    void brush(int* elems, int* pidx, int* cell, float* amt, int* op, int* fA, char* fB);  // 0x52c2b0
};

// @ 0x52c2b0
void SceneBrush::brush(int* elems, int* pidx, int* cell, float* amt, int* op, int* fA, char* fB)
{
    char* me = (char*)this;
    BrushElem* const e = (BrushElem*)((char*)elems + 0xc);
    const int ci = cell[0] * 0x1a4 + cell[1];

    int mode = *(int*)(g_game + 0x1a094);

    float sel;
    if (mode == 1)      sel = *(float*)(g_game + 0x1a0d4);
    else if (mode == 3) sel = *(float*)(g_game + 0x1a0dc);
    else                sel = *(float*)(g_game + 0x1a0d8);

    float lo;
    if (mode == 1)      lo = *(float*)(g_game + 0x1a0ec);
    else if (mode == 3) lo = *(float*)(g_game + 0x1a0f4);
    else                lo = *(float*)(g_game + 0x1a0f0);

    float t = *amt / sel;
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;

    float hi;
    if (mode == 1)      hi = *(float*)(g_game + 0x1a0e0);
    else if (mode == 3) hi = *(float*)(g_game + 0x1a0e8);
    else                hi = *(float*)(g_game + 0x1a0e4);

    const float amount = hi * (1.0f - (1.0f - lo) * t);

    int stampbase = 0;

    if (*(char*)fA == 0 || *fB == 0) {
        // ---- one of the two flags is clear: normal brush -------------------------------------------
        BrushElem* E = &e[*pidx];
        float v = 0.0f;
        if (E->v) v = E->v[ci];

        if (*op == 0) {
            v = v + amount;
            if (v > 1.0f) v = 1.0f;

            int any = 0;
            for (int i = *pidx + 1; i < *elems - 2; i++) {
                float x = 0.0f;
                if (e[i].v) x = e[i].v[ci];
                if (x > 0.98f) any = 1;
            }

            if (any) {
                if (E->v == 0) {
                    E->v = (float*)mb_new_array(0xac440);
                    E->zero();
                }
                E->v[ci] = 1.0f;
            } else {
                if (*fB == 0) goto done;
                if (E->v == 0) {
                    if (!(v > 0.0f)) goto decayed;
                    E->v = (float*)mb_new_array(0xac440);
                    E->zero();
                }
                E->v[ci] = v;
            }

        decayed:
            if (*fB != 0) {
                for (int i = *pidx + 1; i < *elems - 2; i++) {
                    if (e[i].v) {
                        float old = e[i].v[ci];
                        if (old > 0.0f) {
                            float nv = old - amount;
                            if (nv < 0.0f) nv = 0.0f;
                            else if (nv > old) nv = old;
                            if (e[i].v == 0) {
                                if (!(nv > 0.0f)) continue;
                                e[i].v = (float*)mb_new_array(0xac440);
                                e[i].zero();
                            }
                            e[i].v[ci] = nv;
                        }
                    }
                }
            }
        } else if (*op == 1) {
            if (*(int*)(g_game + 0x1a108) == 1) {
                v = v - amount;
            } else if (*(int*)(g_game + 0x1a108) == 0) {
                v = v + amount;
            } else {
                // response = average of the nine surrounding terrain tiles, blended by `amount`
                int x = cell[0];
                int yp1 = cell[1] + 1;
                int cnt = 0;
                float sum = 0.0f;
                if (x >= 0 && x < w && yp1 >= 0 && yp1 < h) {
                    sum = *(float*)(me + (x * 0xfb + yp1) * 0x94 + 0x310)
                        + *(float*)(me + (x * 0xfb + yp1) * 0x94 + 0x30c);
                    cnt = 1;
                }
                int xp1 = x + 1;
                if (xp1 >= 0) {
                    if (xp1 < w && yp1 >= 0 && yp1 < h) {
                        sum += *(float*)(me + (xp1 * 0xfb + yp1) * 0x94 + 0x310)
                             + *(float*)(me + (xp1 * 0xfb + yp1) * 0x94 + 0x30c);
                        cnt++;
                    }
                    if (xp1 >= 0 && xp1 < w && cell[1] >= 0 && cell[1] < h) {
                        cnt++;
                        sum += *(float*)(me + (xp1 * 0xfb + cell[1]) * 0x94 + 0x310)
                             + *(float*)(me + (xp1 * 0xfb + cell[1]) * 0x94 + 0x30c);
                    }
                }
                int ym1 = cell[1] - 1;
                if (xp1 >= 0 && xp1 < w && ym1 >= 0 && ym1 < h) {
                    sum += *(float*)(me + (xp1 * 0xfb + ym1) * 0x94 + 0x310)
                         + *(float*)(me + (xp1 * 0xfb + ym1) * 0x94 + 0x30c);
                    cnt++;
                }
                if (x >= 0 && x < w && ym1 >= 0 && ym1 < h) {
                    sum += *(float*)(me + (x * 0xfb + ym1) * 0x94 + 0x310)
                         + *(float*)(me + (x * 0xfb + ym1) * 0x94 + 0x30c);
                    cnt++;
                }
                int xm1 = x - 1;
                if (xm1 >= 0) {
                    if (xm1 < w && ym1 >= 0 && ym1 < h) {
                        cnt++;
                        sum += *(float*)(me + (xm1 * 0xfb + ym1) * 0x94 + 0x310)
                             + *(float*)(me + (xm1 * 0xfb + ym1) * 0x94 + 0x30c);
                    }
                    if (xm1 >= 0 && xm1 < w && cell[1] >= 0 && cell[1] < h) {
                        cnt++;
                        sum += *(float*)(me + (xm1 * 0xfb + cell[1]) * 0x94 + 0x310)
                             + *(float*)(me + (xm1 * 0xfb + cell[1]) * 0x94 + 0x30c);
                    }
                    if (xm1 >= 0 && xm1 < w && yp1 >= 0 && yp1 < h) {
                        sum += *(float*)(me + (xm1 * 0xfb + yp1) * 0x94 + 0x310)
                             + *(float*)(me + (xm1 * 0xfb + yp1) * 0x94 + 0x30c);
                        cnt++;
                    }
                }
                if (x >= 0 && x < w && cell[1] >= 0 && cell[1] < h) {
                    cnt++;
                    sum += *(float*)(me + (x * 0xfb + cell[1]) * 0x94 + 0x310)
                         + *(float*)(me + (x * 0xfb + cell[1]) * 0x94 + 0x30c);
                }
                float centre = *(float*)(me + (x * 0xfb + cell[1]) * 0x94 + 0x310)
                             + *(float*)(me + (x * 0xfb + cell[1]) * 0x94 + 0x30c);
                v = ((1.0f - amount) * centre + ((centre + sum) * amount) / (float)(cnt + 1)) + v - centre;
            }
            if (E->v == 0) {
                if (!(v > 0.0f)) goto done;
                E->v = (float*)mb_new_array(0xac440);
                E->zero();
            }
            E->v[ci] = v;
        } else if (*op == 2) {
            // blend the tile colour toward the brush colour
            unsigned argb = 0xffffffff;
            if (E->v) argb = *(unsigned*)&E->v[ci];
            float col[4];
            unpack_color(col, &argb);
            float k = 1.0f - amount;
            float out[4];
            out[0] = col[0] * k + amount * *(float*)(g_game + 0x1a0f8);
            out[1] = col[1] * k + amount * *(float*)(g_game + 0x1a0fc);
            out[2] = col[2] * k + amount * *(float*)(g_game + 0x1a100);
            out[3] = 1.0f;   // original copies an uninitialised slot here; it saturates alpha to 255
            unsigned c = pack_color(out);
            E->put(cell, cell + 1, (int*)&c);
        }
    } else {
        // ---- both flags set ------------------------------------------------------------------------
        if (*op == 0) {
            for (int i = 0; i < *elems - 2; i++) {
                float v = 0.0f;
                if (e[i].v) {
                    v = e[i].v[ci] - amount;
                    if (v < 0.0f) v = 0.0f;
                }
                if (e[i].v == 0) {
                    if (!(v > 0.0f)) continue;
                    e[i].v = (float*)mb_new_array(0xac440);
                    e[i].zero();
                }
                e[i].v[ci] = v;
            }
            stampbase = 4;
        } else if (*op == 1) {
            BrushElem* E = &e[*pidx];
            float v = 0.0f;
            if (E->v) v = E->v[ci];
            float nv = (1.0f - amount) * v;
            if (nv < 0.0f) nv = 0.0f;
            if (E->v == 0) {
                if (!(nv > 0.0f)) goto done;
                E->v = (float*)mb_new_array(0xac440);
                E->zero();
            }
            E->v[ci] = nv;
        } else if (*op == 2) {
            BrushElem* E = &e[*pidx];
            unsigned argb = 0xffffffff;
            if (E->v) argb = *(unsigned*)&E->v[ci];
            float col[4];
            unpack_color(col, &argb);
            float k = 1.0f - amount;
            float out[4];
            out[0] = col[0] * k + amount;
            out[1] = col[1] * k + amount;
            out[2] = col[2] * k + amount;
            out[3] = 1.0f;   // original copies an uninitialised slot here; it saturates alpha to 255
            unsigned c = pack_color(out);
            E->put(cell, cell + 1, (int*)&c);
        }
    }

done:
    int P1[2], P2[2], P3[2];
    this->clamp(P1, cell);
    int up[2]   = { cell[0] + 1, cell[1] + 1 };
    int down[2] = { cell[0] - 1, cell[1] - 1 };
    this->clamp(P2, up);
    this->clamp(P3, down);

    if (stampbase < 0xf) {
        int k = stampbase * 5;
        do {
            me[0x845e030 + (P1[0] + k) * 5 + P1[1]] = 1;
            me[0x845e030 + (P2[0] + k) * 5 + P2[1]] = 1;
            me[0x845e030 + (P3[0] + k) * 5 + P3[1]] = 1;
            k += 5;
        } while (k < 0x4b);
    }
}
