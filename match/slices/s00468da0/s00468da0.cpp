// Slice s00468da0: Terrain::apply_brush, a cosine-falloff height brush over the 251x251 terrain grid
// (layout from match/slices/s00464a60/terrain.h and s004661e0). Names provisional. Flags: default /O2 /MT /Gy /TP.
#include <math.h>

extern unsigned g_field_mask;       // 0x5a8ea0 (field values are clamped to g_field_mask - 1)

struct TerrainCell {                // 0x94 bytes
    float px, py, pz;               // +0   position; pz is the height (+8)
    char pad0c[0x18 - 0xc];
    float weight;                   // +0x18
    char pad1c[0x84 - 0x1c];
    float h2;                       // +0x84 copy of the height
    char pad88[0x94 - 0x88];
};

struct Terrain {
    char pad0[0x10];
    unsigned flags;                 // +0x10 bits 7..13 are a field
    char pad14[0x278 - 0x14];
    int w;                          // +0x278
    int h;                          // +0x27c
    char pad280[8];
    TerrainCell cell[251 * 251];    // +0x288  index x * 251 + y
    char pad_b[0xc3b0e0 - 0x288 - 251 * 251 * 0x94];
    float scale;                    // +0xc3b0e0 world size of a cell
    void apply_brush(const float* center, float radius, float strength);    // 0x468da0
};

// @ 0x468da0
void Terrain::apply_brush(const float* center, float radius, float strength)
{
    float fv = (float)((g_field_mask - 1) & (flags >> 7));
    int lo[2], hi[2];
    const int* lim = &w;
    for (int k = 0; k < 2; ++k) {
        float a = (float)ceil((center[k] - radius) / scale);
        float fl = (float)floor(a);
        int i0 = (int)fl;
        int r = i0;
        if (!(a - fl <= 0.5f)) r = i0 + 1;
        lo[k] = r > 0 ? r : 0;
        float b = (float)floor((center[k] + radius) / scale);
        float fl2 = (float)floor(b);
        int i1 = (int)fl2;
        int r2 = i1;
        if (!(b - fl2 <= 0.5f)) r2 = i1 + 1;
        int h = lim[k];
        if (r2 < h) h = r2;
        hi[k] = h;
    }
    for (int j = lo[1]; j <= hi[1]; ++j) {
        for (int i = lo[0]; i <= hi[0]; ++i) {
            TerrainCell& c = cell[i * 251 + j];
            float dx = (float)i * scale - center[0];
            float dy = (float)j * scale - center[1];
            float d = (1.0f / radius) * (float)sqrt(dy * dy + dx * dx);
            if (d < 1.0f) {
                float v = ((((float)cos(d * 3.1415927f) + 1.0f) * strength) * 0.5f * c.weight * fv) * 0.01f + c.pz;
                c.pz = v;
                c.h2 = v;
            }
        }
    }
}
