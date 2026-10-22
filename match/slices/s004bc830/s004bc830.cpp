// Slice s004bc830: renderer vertex-declaration setup, buffer restore + shader load, declaration release,
// and a small 7-dword zeroing constructor. Names are provisional. Flags: default /O2 /MT /Gy /TP.
//
// Layout evidence: 0x4bc830 keeps five declaration pointers at this+0..0x10 and writes a declaration into
// slot+0x48 of ten 0x4c-byte dev_buffer records at this+0x14 (the layout verified by s004bc540/s00461bd0);
// 0x4bcfb0 restores them pair-wise at 0x98 stride and touches the shader pointers at 0x418818.. and the
// counter at 0x418828. 0x4bd0c0 releases the five declarations. 0x4bd120 (different class) zeroes seven
// dwords and returns this.
#include <windows.h>
#include <d3d9.h>

struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
};

struct dev_buffer {
    int type;                                   // 0x00
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
    void restore();                             // 0x4622e0
};

struct buffer_pair {
    dev_buffer bufs[2];
    void restore() { bufs[0].restore(); bufs[1].restore(); }
};

struct resource_manager {
    void* get_shader(const tstring& name);      // 0x418b30
};

struct renderer {
    IDirect3DVertexDeclaration9* decls[5];      // 0x00
    buffer_pair vf[5];                          // 0x14
    buffer_pair ib;                             // 0x30c
    char pad[0x418818 - 0x3a4];
    void* sh0;                                  // 0x418818
    void* sh1;                                  // 0x41881c
    void* sh2;                                  // 0x418820
    void* sh3;                                  // 0x418824
    int counter;                                // 0x418828
    int init_decls(int* a0, int* a1, int* a2, int* a3, int* a4,
                   int* a5, int* a6, int* a7, int* a8, int* a9);  // 0x4bc830
    void init_shaders();                        // 0x4bcfb0
    int release_decls();                        // 0x4bd0c0
};

struct font_details {
    int a, b, c, d, e, f, g;
    font_details();                             // 0x4bd120
};

extern int g_use_shaders;                       // 0x5e28fc
extern IDirect3DDevice9* g_dev;                 // 0xa17f28
extern resource_manager g_res;                  // 0x5d6fa0
void mb_error(const char* msg, ...);            // 0x42cc80

// @ 0x4bc830
int renderer::init_decls(int* a0, int* a1, int* a2, int* a3, int* a4,
                         int* a5, int* a6, int* a7, int* a8, int* a9)
{
    if (g_use_shaders) {
        int c1 = *a1 ? *a1 : 16000;
        int c0 = *a0;
        vf[0].bufs[1].count = c1;
        vf[0].bufs[0].count = c0;
        int c3 = *a3 ? *a3 : 35000;
        int c2 = *a2 ? *a2 : 290000;
        vf[1].bufs[1].count = c3;
        vf[1].bufs[0].count = c2;
        int c5 = *a5;
        int c4 = *a4 ? *a4 : 90000;
        vf[3].bufs[1].count = c5;
        vf[3].bufs[0].count = c4;
        int c7 = *a7 ? *a7 : 12000;
        int c6 = *a6 ? *a6 : 120000;
        vf[2].bufs[0].count = c6;
        vf[2].bufs[1].count = c7;
    } else {
        int c9 = *a9 ? *a9 : 30000;
        int c8 = *a8 ? *a8 : 290000;
        vf[0].bufs[0].count = c8;
        vf[0].bufs[1].count = c9;
    }

    D3DVERTEXELEMENT9 ffp[5];
    D3DVERTEXELEMENT9 regular[6];
    D3DVERTEXELEMENT9 skinning[7];
    D3DVERTEXELEMENT9 normalmap[8];
    D3DVERTEXELEMENT9 normalmap_skin[9];

    ffp[0].Stream = 0; ffp[0].Offset = 0; ffp[0].Type = D3DDECLTYPE_FLOAT3;
    ffp[0].Method = 0; ffp[0].Usage = D3DDECLUSAGE_POSITION; ffp[0].UsageIndex = 0;
    ffp[1].Stream = 0; ffp[1].Offset = 0xc; ffp[1].Type = D3DDECLTYPE_FLOAT3;
    ffp[1].Method = 0; ffp[1].Usage = D3DDECLUSAGE_NORMAL; ffp[1].UsageIndex = 0;
    ffp[2].Stream = 0; ffp[2].Offset = 0x18; ffp[2].Type = D3DDECLTYPE_D3DCOLOR;
    ffp[2].Method = 0; ffp[2].Usage = D3DDECLUSAGE_COLOR; ffp[2].UsageIndex = 0;
    ffp[3].Stream = 0; ffp[3].Offset = 0x1c; ffp[3].Type = D3DDECLTYPE_FLOAT2;
    ffp[3].Method = 0; ffp[3].Usage = D3DDECLUSAGE_TEXCOORD; ffp[3].UsageIndex = 0;
    ffp[4].Stream = 0xff; ffp[4].Offset = 0; ffp[4].Type = D3DDECLTYPE_UNUSED;
    ffp[4].Method = 0; ffp[4].Usage = 0; ffp[4].UsageIndex = 0;

    regular[0].Stream = 0; regular[0].Offset = 0; regular[0].Type = D3DDECLTYPE_FLOAT3;
    regular[0].Method = 0; regular[0].Usage = D3DDECLUSAGE_POSITION; regular[0].UsageIndex = 0;
    regular[1].Stream = 0; regular[1].Offset = 0xc; regular[1].Type = D3DDECLTYPE_FLOAT3;
    regular[1].Method = 0; regular[1].Usage = D3DDECLUSAGE_NORMAL; regular[1].UsageIndex = 0;
    regular[2].Stream = 0; regular[2].Offset = 0x18; regular[2].Type = D3DDECLTYPE_D3DCOLOR;
    regular[2].Method = 0; regular[2].Usage = D3DDECLUSAGE_COLOR; regular[2].UsageIndex = 0;
    regular[3].Stream = 0; regular[3].Offset = 0x1c; regular[3].Type = D3DDECLTYPE_D3DCOLOR;
    regular[3].Method = 0; regular[3].Usage = D3DDECLUSAGE_COLOR; regular[3].UsageIndex = 1;
    regular[4].Stream = 0; regular[4].Offset = 0x20; regular[4].Type = D3DDECLTYPE_FLOAT2;
    regular[4].Method = 0; regular[4].Usage = D3DDECLUSAGE_TEXCOORD; regular[4].UsageIndex = 0;
    regular[5].Stream = 0xff; regular[5].Offset = 0; regular[5].Type = D3DDECLTYPE_UNUSED;
    regular[5].Method = 0; regular[5].Usage = 0; regular[5].UsageIndex = 0;

    normalmap[0].Stream = 0; normalmap[0].Offset = 0; normalmap[0].Type = D3DDECLTYPE_FLOAT3;
    normalmap[0].Method = 0; normalmap[0].Usage = D3DDECLUSAGE_POSITION; normalmap[0].UsageIndex = 0;
    normalmap[1].Stream = 0; normalmap[1].Offset = 0xc; normalmap[1].Type = D3DDECLTYPE_FLOAT3;
    normalmap[1].Method = 0; normalmap[1].Usage = D3DDECLUSAGE_NORMAL; normalmap[1].UsageIndex = 0;
    normalmap[2].Stream = 0; normalmap[2].Offset = 0x18; normalmap[2].Type = D3DDECLTYPE_D3DCOLOR;
    normalmap[2].Method = 0; normalmap[2].Usage = D3DDECLUSAGE_COLOR; normalmap[2].UsageIndex = 0;
    normalmap[3].Stream = 0; normalmap[3].Offset = 0x1c; normalmap[3].Type = D3DDECLTYPE_D3DCOLOR;
    normalmap[3].Method = 0; normalmap[3].Usage = D3DDECLUSAGE_COLOR; normalmap[3].UsageIndex = 1;
    normalmap[4].Stream = 0; normalmap[4].Offset = 0x20; normalmap[4].Type = D3DDECLTYPE_FLOAT2;
    normalmap[4].Method = 0; normalmap[4].Usage = D3DDECLUSAGE_TEXCOORD; normalmap[4].UsageIndex = 0;
    normalmap[5].Stream = 0; normalmap[5].Offset = 0x28; normalmap[5].Type = D3DDECLTYPE_FLOAT3;
    normalmap[5].Method = 0; normalmap[5].Usage = D3DDECLUSAGE_TANGENT; normalmap[5].UsageIndex = 0;
    normalmap[6].Stream = 0; normalmap[6].Offset = 0x34; normalmap[6].Type = D3DDECLTYPE_FLOAT3;
    normalmap[6].Method = 0; normalmap[6].Usage = D3DDECLUSAGE_BINORMAL; normalmap[6].UsageIndex = 0;
    normalmap[7].Stream = 0xff; normalmap[7].Offset = 0; normalmap[7].Type = D3DDECLTYPE_UNUSED;
    normalmap[7].Method = 0; normalmap[7].Usage = 0; normalmap[7].UsageIndex = 0;

    skinning[0].Stream = 0; skinning[0].Offset = 0; skinning[0].Type = D3DDECLTYPE_FLOAT3;
    skinning[0].Method = 0; skinning[0].Usage = D3DDECLUSAGE_POSITION; skinning[0].UsageIndex = 0;
    skinning[1].Stream = 0; skinning[1].Offset = 0xc; skinning[1].Type = D3DDECLTYPE_FLOAT3;
    skinning[1].Method = 0; skinning[1].Usage = D3DDECLUSAGE_NORMAL; skinning[1].UsageIndex = 0;
    skinning[2].Stream = 0; skinning[2].Offset = 0x18; skinning[2].Type = D3DDECLTYPE_D3DCOLOR;
    skinning[2].Method = 0; skinning[2].Usage = D3DDECLUSAGE_COLOR; skinning[2].UsageIndex = 0;
    skinning[3].Stream = 0; skinning[3].Offset = 0x1c; skinning[3].Type = D3DDECLTYPE_FLOAT2;
    skinning[3].Method = 0; skinning[3].Usage = D3DDECLUSAGE_TEXCOORD; skinning[3].UsageIndex = 0;
    skinning[4].Stream = 0; skinning[4].Offset = 0x24; skinning[4].Type = D3DDECLTYPE_UBYTE4;
    skinning[4].Method = 0; skinning[4].Usage = D3DDECLUSAGE_BLENDWEIGHT; skinning[4].UsageIndex = 0;
    skinning[5].Stream = 0; skinning[5].Offset = 0x34; skinning[5].Type = D3DDECLTYPE_UBYTE4;
    skinning[5].Method = 0; skinning[5].Usage = D3DDECLUSAGE_BLENDINDICES; skinning[5].UsageIndex = 0;
    skinning[6].Stream = 0xff; skinning[6].Offset = 0; skinning[6].Type = D3DDECLTYPE_UNUSED;
    skinning[6].Method = 0; skinning[6].Usage = 0; skinning[6].UsageIndex = 0;

    normalmap_skin[0].Stream = 0; normalmap_skin[0].Offset = 0; normalmap_skin[0].Type = D3DDECLTYPE_FLOAT3;
    normalmap_skin[0].Method = 0; normalmap_skin[0].Usage = D3DDECLUSAGE_POSITION; normalmap_skin[0].UsageIndex = 0;
    normalmap_skin[1].Stream = 0; normalmap_skin[1].Offset = 0xc; normalmap_skin[1].Type = D3DDECLTYPE_FLOAT3;
    normalmap_skin[1].Method = 0; normalmap_skin[1].Usage = D3DDECLUSAGE_NORMAL; normalmap_skin[1].UsageIndex = 0;
    normalmap_skin[2].Stream = 0; normalmap_skin[2].Offset = 0x18; normalmap_skin[2].Type = D3DDECLTYPE_D3DCOLOR;
    normalmap_skin[2].Method = 0; normalmap_skin[2].Usage = D3DDECLUSAGE_COLOR; normalmap_skin[2].UsageIndex = 0;
    normalmap_skin[3].Stream = 0; normalmap_skin[3].Offset = 0x1c; normalmap_skin[3].Type = D3DDECLTYPE_FLOAT2;
    normalmap_skin[3].Method = 0; normalmap_skin[3].Usage = D3DDECLUSAGE_TEXCOORD; normalmap_skin[3].UsageIndex = 0;
    normalmap_skin[4].Stream = 0; normalmap_skin[4].Offset = 0x24; normalmap_skin[4].Type = D3DDECLTYPE_FLOAT3;
    normalmap_skin[4].Method = 0; normalmap_skin[4].Usage = D3DDECLUSAGE_TANGENT; normalmap_skin[4].UsageIndex = 0;
    normalmap_skin[5].Stream = 0; normalmap_skin[5].Offset = 0x30; normalmap_skin[5].Type = D3DDECLTYPE_FLOAT3;
    normalmap_skin[5].Method = 0; normalmap_skin[5].Usage = D3DDECLUSAGE_BINORMAL; normalmap_skin[5].UsageIndex = 0;
    normalmap_skin[6].Stream = 0; normalmap_skin[6].Offset = 0x3c; normalmap_skin[6].Type = D3DDECLTYPE_UBYTE4;
    normalmap_skin[6].Method = 0; normalmap_skin[6].Usage = D3DDECLUSAGE_BLENDWEIGHT; normalmap_skin[6].UsageIndex = 0;
    normalmap_skin[7].Stream = 0; normalmap_skin[7].Offset = 0x4c; normalmap_skin[7].Type = D3DDECLTYPE_UBYTE4;
    normalmap_skin[7].Method = 0; normalmap_skin[7].Usage = D3DDECLUSAGE_BLENDINDICES; normalmap_skin[7].UsageIndex = 0;
    normalmap_skin[8].Stream = 0xff; normalmap_skin[8].Offset = 0; normalmap_skin[8].Type = D3DDECLTYPE_UNUSED;
    normalmap_skin[8].Method = 0; normalmap_skin[8].Usage = 0; normalmap_skin[8].UsageIndex = 0;

    if (g_dev->CreateVertexDeclaration(ffp, &decls[0]) < 0)
        mb_error("Unable to create vertex declaration.");
    if (g_dev->CreateVertexDeclaration(regular, &decls[1]) < 0)
        mb_error("Unable to create vertex declaration.");
    if (g_dev->CreateVertexDeclaration(normalmap, &decls[2]) < 0)
        mb_error("Unable to create vertex declaration.");
    if (g_dev->CreateVertexDeclaration(skinning, &decls[3]) < 0)
        mb_error("Unable to create vertex declaration.");
    if (g_dev->CreateVertexDeclaration(normalmap_skin, &decls[4]) < 0)
        mb_error("Unable to create vertex declaration.");

    vf[0].bufs[0].decl = decls[0];
    vf[0].bufs[1].decl = decls[0];
    vf[1].bufs[0].decl = decls[1];
    vf[1].bufs[1].decl = decls[1];
    vf[2].bufs[0].decl = decls[2];
    vf[2].bufs[1].decl = decls[2];
    vf[3].bufs[0].decl = decls[3];
    vf[3].bufs[1].decl = decls[3];
    vf[4].bufs[0].decl = decls[4];
    vf[4].bufs[1].decl = decls[4];

    return 0;
}

// @ 0x4bcfb0
void renderer::init_shaders()
{
    buffer_pair* p = vf;
    for (int i = 0; i < 5; i++, p++)
        p->restore();
    ib.restore();

    counter++;
    {
        tstring name("render_to_depth_shader");
        sh0 = g_res.get_shader(name);
    }
    {
        tstring name("render_to_depth_skin_shader");
        sh1 = g_res.get_shader(name);
    }
    {
        tstring name("render_character_shadow");
        sh2 = g_res.get_shader(name);
    }
    {
        tstring name("render_character_shadow_with_skin");
        sh3 = g_res.get_shader(name);
    }
}

// @ 0x4bd0c0
int renderer::release_decls()
{
    if (decls[0]) { decls[0]->Release(); decls[0] = 0; }
    if (decls[1]) { decls[1]->Release(); decls[1] = 0; }
    if (decls[2]) { decls[2]->Release(); decls[2] = 0; }
    if (decls[3]) { decls[3]->Release(); decls[3] = 0; }
    if (decls[4]) { decls[4]->Release(); decls[4] = 0; }
    return 0;
}

// @ 0x4bd120
font_details::font_details()
{
    a = 0; b = 0; c = 0; d = 0; e = 0; f = 0; g = 0;
}
