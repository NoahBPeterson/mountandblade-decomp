// Slice s004bc540: device-buffer pair release and the renderer's vertex-buffer format table setup
// (10 FVF slots + 2 index buffers). Names are provisional. Flags: default /O2 /MT /Gy /TP.
//
// Layout evidence: the 0x4c-byte dev_buffer record is the one verified by s00461bd0 (0x4622e0 restore:
// type 0, elem_size 4, vb 8, fvf 0xc, generation 0x14, count 0x28, loaded 0x2c, ib 0x44); this slice's
// 0x4bc830 (s004bc830) stores a vertex declaration at +0x48 and 0x4bcfb0 restores slots at 0x14 + i*0x4c.
#include <windows.h>
#include <d3d9.h>

struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
};

struct dev_buffer {
    int type;                                   // 0x00  0 vertex buffer, 1 index buffer
    int elem_size;                              // 0x04
    IDirect3DVertexBuffer9* vb;                 // 0x08
    unsigned fvf;                               // 0x0c
    int f10;                                    // 0x10
    int generation;                             // 0x14
    tstring name;                               // 0x18
    int count;                                  // 0x28
    int loaded;                                 // 0x2c
    char pad30[0x44 - 0x30];
    IDirect3DIndexBuffer9* ib;                  // 0x44
    IDirect3DVertexDeclaration9* decl;          // 0x48
};

struct buffer_pair {
    dev_buffer bufs[2];
    void release();                             // 0x4bc540
};

struct render_batch {
    char pad0[0x14];
    dev_buffer vf[10];                          // 0x14
    dev_buffer ib[2];                           // 0x30c
    char pad3a4[0x3b4 - 0x3a4];
    int cur_vf_a;                               // 0x3b4
    int cur_ib;                                 // 0x3b8
    int cur_vf_b;                               // 0x3bc
    void init_formats();                        // 0x4bc5b0
};

extern int g_mem_budget;                        // 0x5a9fe0

// @ 0x4bc540
void buffer_pair::release()
{
    dev_buffer& a = bufs[0];
    if (a.type == 1)
        a.ib->Release();
    else if (a.count != 0 && a.vb)
        a.vb->Release();
    a.generation++;
    a.loaded = 0;
    dev_buffer& b = bufs[1];
    if (b.type == 1)
        b.ib->Release();
    else if (b.count != 0 && b.vb)
        b.vb->Release();
    b.generation++;
    b.loaded = 0;
}

// @ 0x4bc5b0
void render_batch::init_formats()
{
    cur_vf_a = -1;
    cur_ib = -1;
    cur_vf_b = -1;
    int n = (g_mem_budget << 18) / 0x44;
    for (int i = 0; i < 10; i++)
        vf[i].type = 0;
    dev_buffer* p = vf;
    p[0].fvf = 0x02000013; p[0].elem_size = 0x24;
    p[1].fvf = 0x02000013; p[1].elem_size = 0x24;
    p[2].fvf = 0x02000053; p[2].elem_size = 0x28;
    p[3].fvf = 0x02000053; p[3].elem_size = 0x28;
    p[4].fvf = 0x02000153; p[4].elem_size = 0x40;
    p[5].fvf = 0x02000153; p[5].elem_size = 0x40;
    p[6].fvf = 0x01001013; p[6].elem_size = 0x44;
    p[7].fvf = 0x01001013; p[7].elem_size = 0x44;
    p[8].fvf = 0x01001113; p[8].elem_size = 0x5c;
    p[9].fvf = 0x01001113; p[9].elem_size = 0x5c;
    {
        tstring s("vdt_regular_ffp");
        vf[0].name = s;
        vf[1].name = s;
    }
    {
        tstring s("vdt_regular");
        vf[2].name = s;
        vf[3].name = s;
    }
    {
        tstring s("vdt_normal_map");
        vf[4].name = s;
        vf[5].name = s;
    }
    {
        tstring s("vdt_skinning");
        vf[6].name = s;
        vf[7].name = s;
    }
    {
        tstring s("vdt_normal_map_skinning");
        vf[8].name = s;
        vf[9].name = s;
    }
    ib[0].type = 1;
    ib[1].type = 1;
    ib[0].count = n * 3 * 8;
    ib[1].count = n / 2 * 8;
}

// 0x594628 "vdt_regular_ffp", 0x59461c "vdt_regular", 0x59460c "vdt_normal_map",
// 0x5945fc "vdt_skinning", 0x5945e4 "vdt_normal_map_skinning"
