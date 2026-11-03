// cell_of.cpp: Terrain::cell_of (0x464b40), the world position -> cell index lookup. Its own translation unit: the original's
// callers (Terrain::emit_particle 0x466cd0, Terrain::sample 0x465af0) were compiled without seeing the body, so cl must not see it
// either (a body visible in the caller's unit changes the scheduling of the stores that follow the call).
#include "./terrain.h"

#define IFLOOR(e) ((int)((e) > 0.0f ? (e) : (e) - 0.99999988f))

// @ 0x464b40
int* Terrain::cell_of(int* out, const float* p)
{
    float s = 1.0f / scale;
    int x = IFLOOR(s * p[0]);
    int ix;
    if (x < 0) ix = 0; else { ix = w - 1; if (x < w) ix = x; }
    int y = IFLOOR(s * p[1]);
    int iy;
    if (y < 0) iy = 0; else if (y >= h) iy = h - 1; else iy = y;
    out[0] = ix; out[1] = iy;
    return out;
}
