// Slice s00465a70: Terrain grid cell helpers: noise sample at a cell, bilinear sample of the cell grid at a
// position, bulk set of a cell field by mask bit and neighbour smoothing of cells lacking a mask bit.
// Names are provisional. Flags: default.
#include "../s00464a60/terrain.h"

// @ 0x465a70
float Terrain::noise_at(int x, int y)
{
    float p[4];
    float n[4];
    TerrainCell* c = &cell[x][y];
    p[0] = off_a[0] + c->f[0];
    p[1] = off_a[1] + c->f[1];
    p[2] = off_a[2] + c->f[2];
    noise_fractal(noise_seed, 0.5f, 1, 0, p, n);
    return n[0] + n[1] + n[2];
}

// @ 0x465af0
vector4 Terrain::sample(const float* p)
{
    int ij[2];
    cell_of(ij, p);
    int i = ij[0], j = ij[1];
    TerrainCell* C = &cell[0][0];
    vector4* q00 = (vector4*)&C[i * 251 + j];
    vector4* q10 = (vector4*)&C[(i + 1) * 251 + j];
    vector4* q01 = (vector4*)&C[i * 251 + (j + 1)];
    vector4* q11 = (vector4*)&C[(i + 1) * 251 + (j + 1)];
    vector4 c00 = *q00;
    vector4 c10 = *q10;
    vector4 c01 = *q01;
    vector4 c11 = *q11;
    vector4 a, b;
    if (p[1] < c00.y) {
        a = *q00;
    } else if (c01.y < p[1]) {
        a = *q01;
    } else {
        float t = (p[1] - c00.y) / (c01.y - c00.y);
        float u = 1.0f - t;
        a = vector4(q01->x * t + c00.x * u, t * c01.y + c00.y * u, q01->z * t + u * q00->z);
    }
    if (p[1] < c10.y) {
        b = *q10;
    } else if (c11.y < p[1]) {
        b = *q11;
    } else {
        float t = (p[1] - c10.y) / (c11.y - c10.y);
        float u = 1.0f - t;
        b = vector4(q11->x * t + c10.x * u, t * c11.y + c10.y * u, q11->z * t + u * q10->z);
    }
    if (p[0] < c00.x) {
        return a;
    }
    if (c10.x < p[0]) {
        return b;
    }
    float t = (p[0] - c00.x) / (c10.x - c00.x);
    float u = 1.0f - t;
    vector4 r(b.x * t + a.x * u, b.y * t + a.y * u, b.z * t + u * a.z);
    r.w = q10->w;
    return r;
}

// @ 0x465e70
void Terrain::set_cells(int bit, float v)
{
    unsigned m = 1u << bit;
    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            TerrainCell* c = &cell[x][y];
            if (c->mask & m) {
                c->f84 = v;
                c->f[2] = v;
            }
        }
    }
}

// @ 0x465ee0
void Terrain::smooth_cells(int bit, float k)
{
    unsigned m = 1u << bit;
    float nv = k;
    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            TerrainCell* c = &cell[x][y];
            if ((c->mask & m) == 0) {
                int kind = 0;
                int nx = x + 1, ny = y + 1;
                if (x >= 0 && x <= w && ny >= 0 && ny <= h && (cell[x][ny].mask & m) != 0) {
                    kind = 2;
                    nv = cell[x][ny].f[2];
                }
                if (nx >= 0 && nx <= w && ny >= 0 && ny <= h && (cell[nx][ny].mask & m) != 0) {
                    if (kind == 0) kind = 1;
                    nv = cell[nx][ny].f[2];
                }
                if (nx >= 0 && nx <= w && y >= 0 && y <= h && (cell[nx][y].mask & m) != 0) {
                    kind = 2;
                    nv = cell[nx][y].f[2];
                }
                int py = y - 1;
                if (nx >= 0 && nx <= w && py >= 0 && py <= h && (cell[nx][py].mask & m) != 0) {
                    if (kind == 0) kind = 1;
                    nv = cell[nx][py].f[2];
                }
                if (x >= 0 && x <= w && py >= 0 && py <= h && (cell[x][py].mask & m) != 0) {
                    kind = 2;
                    nv = cell[x][py].f[2];
                }
                if (x - 1 >= 0 && x - 1 <= w) {
                    if (py >= 0 && py <= h && (cell[x - 1][py].mask & m) != 0) {
                        if (kind == 0) kind = 1;
                        nv = cell[x - 1][py].f[2];
                    }
                    if (y >= 0 && y <= h && (cell[x - 1][y].mask & m) != 0) {
                        kind = 2;
                        nv = cell[x - 1][y].f[2];
                    }
                }
                if (x - 1 >= 0 && x - 1 <= w && ny >= 0 && ny <= h && (cell[x - 1][ny].mask & m) != 0) {
                    if (kind == 0) kind = 1;
                    nv = cell[x - 1][ny].f[2];
                }
                if (kind != 0) {
                    float a = k;
                    if (kind == 1) a = k * 0.7f;
                    a = (nv - c->f[2]) * a + c->f[2];
                    c->f[2] = a;
                    c->f84 = a;
                }
            }
        }
    }
}
