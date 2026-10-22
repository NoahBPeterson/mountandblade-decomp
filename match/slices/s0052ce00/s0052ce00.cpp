// Slice s0052ce00: the per-cell value block helpers of the world-map terrain brush.
//   0x52ce00  clear the 0xac440-byte block (0x1a4 x 0x1a4 floats) of one brush element.
//   0x52ce70  lazily allocate that block (filled with -1) and store one cell.
//   0x52cef0  scale a grid coordinate pair into the scene's cell range (divide by the cell size at
//             this+0x843bd70, clamp to [0, size-1] using this+0x845d984 / this+0x845d988).
// The element record is 24 bytes; its block pointer lives at +0x14 and the block is read as float
// cells by the brush (0x52c2b0) and written as int -1 when it is first allocated. Layout and offsets
// are read off this slice's disassembly.
#include <windows.h>

void* __cdecl alloc_arr_535482(unsigned _Size);          // 0x535482  operator new[] (??_U@YAPAXI@Z)

struct Elem {
    int f0;
    int f1;
    int f2;
    int f3;
    int f4;
    float* v;                                            // +0x14  0xac440-byte value block
    void clear();                                        // 0x52ce00
    void put(int* ix, int* iy, int* value);              // 0x52ce70
};

struct Scene {
    int* clamp(int* out, int* in);                       // 0x52cef0
};

// @ 0x52ce00
void Elem::clear()
{
    if (v != 0) {
        for (int x = 0; x < 0x1a4; x++)
            for (int y = 0; y < 0x1a4; y++)
                v[x * 0x1a4 + y] = 0.0f;
    }
}

// @ 0x52ce70
void Elem::put(int* ix, int* iy, int* value)
{
    if (v == 0) {
        if (*value == -1)
            return;
        v = (float*)alloc_arr_535482(0xac440);
        for (int x = 0; x < 0x1a4; x++)
            for (int y = 0; y < 0x1a4; y++)
                ((int*)v)[x * 0x1a4 + y] = -1;
    }
    ((int*)v)[*ix * 0x1a4 + *iy] = *value;
}

// @ 0x52cef0
int* Scene::clamp(int* out, int* in)
{
    int ox;
    int ix = in[0] / *(int*)((char*)this + 0x843bd70);
    if (ix < 0) {
        ox = 0;
    } else {
        int m = *(int*)((char*)this + 0x845d984);
        ox = m - 1;
        if (ix < m)
            ox = ix;
    }

    int iy = in[1] / *(int*)((char*)this + 0x843bd70);
    if (iy < 0) {
        out[0] = ox;
        out[1] = 0;
        return out;
    }
    int mx = *(int*)((char*)this + 0x845d988);
    if (iy >= mx) {
        out[0] = ox;
        out[1] = mx - 1;
        return out;
    }
    out[0] = ox;
    out[1] = iy;
    return out;
}
