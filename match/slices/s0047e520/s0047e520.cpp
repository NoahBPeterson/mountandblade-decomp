// Slice s0047e520: the engine's per-pass render driver (one huge method of the global renderer state at
// 0x5fe898). Names provisional; offsets and call targets are those of the original.
#include <vector>
#include <algorithm>
#include <new>

// ---- engine text stream (same model as s00478830) ----
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
};

// ---- an 8-byte render record: object id/handle at +0, an extra word at +4 ----
struct E8 {
    int obj;                                    // +0
    int val;                                    // +4
    E8(int o, int v) : obj(o), val(v) {}
};
bool rec_less(const E8& a, const E8& b);        // 0x4622a0

struct Obj;

// A render-target / texture descriptor (only the fields this function touches).
struct TexRec {
    virtual int QueryInterface();
    virtual int AddRef();
    virtual int Release();
    char pad4[0x18 - 4];
    char f18;                                   // +0x18
    char pad19[0x20 - 0x19];
    int f20;                                    // +0x20
    char pad24[0x44 - 0x24];
    int f44;                                    // +0x44
    int f48;                                    // +0x48
    int f50;                                    // +0x50
};

struct Obj {
    virtual void v0();
    virtual void v1();
    virtual void v2();                          // called through the third slot
    char pad4[0x18 - 4];
    Obj* f18;                                   // +0x18
    char pad1c[0x2c - 0x1c];
    int f2c;                                    // +0x2c
    char pad30[0xa0 - 0x30];
    char f0a0;                                  // +0xa0
    char padA1[0x110 - 0xa1];
    int f110;                                   // +0x110
    int f114;                                   // +0x114
    char pad118[0x139 - 0x118];
    char f139;                                  // +0x139
    char pad13a[0x140 - 0x13a];
    int f140;                                   // +0x140
    char pad144[0x150 - 0x144];
    int f150;                                   // +0x150
    char pad154[0x15c - 0x154];
    int f15c;                                   // +0x15c
    char pad160[0x165 - 0x160];
    char f165;                                  // +0x165
    char pad166[0x16c - 0x166];
    int f16c;                                   // +0x16c
    char pad170[0x18c - 0x170];
    int f18c;                                   // +0x18c
    char pad190[0x191 - 0x190];
    char f191;                                  // +0x191
    char pad192[0x1a4 - 0x192];
    char f1a4;                                  // +0x1a4
    char pad1a5[0x1ac - 0x1a5];
    char f1ac;                                  // +0x1ac
    char pad1ad[0x1b4 - 0x1ad];
    std::vector<E8> v1b4;                       // +0x1b4
    int f1c4;                                   // +0x1c4
    std::vector<int> v1cc;                      // +0x1c8
    void clear_geometry();                      // 0x428e80
    void apply_state();                         // 0x46ca50
};

// 0x4c-byte bucket; its int vector sits at +0x34 (two buckets make up a 0x98-byte pair).
struct Bucket {
    char pad0[0x34];
    std::vector<int> v;                         // +0x34
    char pad44[0x4c - 0x44];
    int upload(void* geom, int count);          // 0x46d060
};
struct GeoPair {                                // 0x98
    Bucket b[2];
    void restore_all();                         // 0x4623c0
};

// ---- Direct3D 9 device (only the vtable slots used here) ----
struct D3DDevice {
    virtual int QueryInterface();
    virtual int AddRef();
    virtual int Release();
    virtual int TestCooperativeLevel();
    virtual int GetAvailableTextureMem();
    virtual int EvictManagedResources();
    virtual int GetDirect3D();
    virtual int GetDeviceCaps();
    virtual int GetDisplayMode();
    virtual int GetCreationParameters();
    virtual int SetCursorProperties();
    virtual int SetCursorPosition();
    virtual int ShowCursor();
    virtual int CreateAdditionalSwapChain();
    virtual int GetSwapChain();
    virtual int GetNumberOfSwapChains();
    virtual int Reset();
    virtual int Present();
    virtual int GetBackBuffer();
    virtual int GetRasterStatus();
    virtual int SetDialogBoxMode();
    virtual int SetGammaRamp();
    virtual int GetGammaRamp();
    virtual int CreateTexture();
    virtual int CreateVolumeTexture();
    virtual int CreateCubeTexture();
    virtual int CreateVertexBuffer();
    virtual int CreateIndexBuffer();
    virtual int CreateRenderTarget();
    virtual int CreateDepthStencilSurface();
    virtual int UpdateSurface();
    virtual int UpdateTexture();
    virtual int GetRenderTargetData();
    virtual int GetFrontBufferData();
    virtual int StretchRect();
    virtual int ColorFill();
    virtual int CreateOffscreenPlainSurface();
    virtual int SetRenderTarget(unsigned index, void* surf);        // +0x94
    virtual int GetRenderTarget(unsigned index, void** surf);       // +0x98
    virtual int SetDepthStencilSurface(void* surf);                 // +0x9c
    virtual int GetDepthStencilSurface(void** surf);                // +0xa0
    virtual int BeginScene();                                       // +0xa4
    virtual int EndScene();                                         // +0xa8
    virtual int Clear();
    virtual int SetTransform();
    virtual int GetTransform();
    virtual int MultiplyTransform();
    virtual int SetViewport();
    virtual int GetViewport();
    virtual int SetMaterial();
    virtual int GetMaterial();
    virtual int SetLight();
    virtual int GetLight();
    virtual int LightEnable();
    virtual int GetLightEnable();
    virtual int SetClipPlane();
    virtual int GetClipPlane();
    virtual int SetRenderState(unsigned state, unsigned value);     // +0xe4
    virtual int GetRenderState();
    virtual int CreateStateBlock();
    virtual int BeginStateBlock();
    virtual int EndStateBlock();
    virtual int SetClipStatus();
    virtual int GetClipStatus();
    virtual int GetTexture();
    virtual int SetTexture(unsigned stage, void* tex);              // +0x104
    virtual int GetTextureStageState();
    virtual int SetTextureStageState();
    virtual int GetSamplerState();
    virtual int SetSamplerState();
    virtual int ValidateDevice();
    virtual int SetPaletteEntries();
    virtual int GetPaletteEntries();
    virtual int SetCurrentTexturePalette();
    virtual int GetCurrentTexturePalette();
    virtual int SetScissorRect();
    virtual int GetScissorRect();
    virtual int SetSoftwareVertexProcessing();
    virtual int GetSoftwareVertexProcessing();
    virtual int SetNPatchMode();
    virtual int GetNPatchMode();
    virtual int DrawPrimitive();
    virtual int DrawIndexedPrimitive();
    virtual int DrawPrimitiveUP();
    virtual int DrawIndexedPrimitiveUP();
    virtual int ProcessVertices();
    virtual int CreateVertexDeclaration();
    virtual int SetVertexDeclaration();
    virtual int GetVertexDeclaration();
    virtual int SetFVF();
    virtual int GetFVF();
    virtual int CreateVertexShader();
    virtual int SetVertexShader();
    virtual int GetVertexShader();
    virtual int SetVertexShaderConstantF();
    virtual int GetVertexShaderConstantF();
    virtual int SetVertexShaderConstantI();
    virtual int GetVertexShaderConstantI();
    virtual int SetVertexShaderConstantB();
    virtual int GetVertexShaderConstantB();
    virtual int SetStreamSource(unsigned stream, void* data, unsigned off, unsigned stride);  // +0x190
};

// A D3D surface (only Release is used).
struct ISurface {
    virtual int QueryInterface();
    virtual int AddRef();
    virtual int Release();
};

struct Renderer {
    void reset_state();                         // 0x4617b0
};

struct ResMgr {
    int get_material(const tstring& s);         // 0x418a70
};

extern Renderer g_render;                       // 0xa17910
extern D3DDevice* g_device;                     // 0xa17f28
extern ResMgr g_resmgr;                         // 0x5d6fa0
extern char g_5e2938;                           // 0x5e2938
extern char g_scene_open;                       // 0xa17e14
extern int g_5e28fc;                            // 0x5e28fc
void gfx_restore();                             // 0x4c1b00
void mb_error(const char* msg);                 // 0x42cc80

// The renderer state singleton (0x5fe898).
struct Big {
    char pad0[0x14];
    GeoPair bp[5];                              // +0x14  (five 0x98-byte pairs)
    GeoPair bp5;                                // +0x30c
    std::vector<int> v3a4;                      // +0x3a4
    int f3b4;                                   // +0x3b4
    int f3b8;                                   // +0x3b8
    char pad3bc[0x3c0 - 0x3bc];
    char geom3c0[0x4183c0 - 0x3c0];             // the geometry-buffer arena at +0x3c0
    int f4183c0;                                // +0x4183c0
    int f4183c4;                                // +0x4183c4
    int f4183c8;                                // +0x4183c8
    int f4183cc;                                // +0x4183cc
    int f4183d0;                                // +0x4183d0
    char pad4183d4[0x418828 - 0x4183d4];
    int f418828;                                // +0x418828

    bool render(int pass);                      // 0x47e520
    void register_obj(Obj* o);                  // 0x412040
    void draw_a(Obj* o);                        // 0x46e590
    void draw_b(Obj* o);                        // 0x46d8c0
};

// @ 0x47e520
bool Big::render(int pass)
{
    for (int i = 0; i < 5; i++)
        bp[i].restore_all();
    bp5.restore_all();

    std::vector<int> tmp;
    if (pass == 0)
        tmp = v3a4;
    else
        tmp.push_back(pass);

    f4183c8 = 0;
    f4183d0 = 0;
    g_render.reset_state();

    for (std::vector<int>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
        for (unsigned i = 0; i < ((Obj*)*it)->v1cc.size(); i++) {
            ((Obj*)((Obj*)*it)->v1cc[i])->f114 = 1;
            ((Obj*)((Obj*)*it)->v1cc[i])->f140 = 1;
            ((Obj*)((Obj*)*it)->v1cc[i])->f16c = 1;
            if (((Obj*)*it)->f1a4 == 0 && ((Obj*)*it)->f1ac != 0)
                register_obj((Obj*)*it);
            ((Obj*)*it)->v1b4.push_back(E8((int)((Obj*)*it)->v1cc[i], 0));
        }
    }

again:
    if (g_5e2938) {
        g_5e2938 = 0;
        gfx_restore();
    }

    for (std::vector<int>::iterator c = tmp.begin(); c != tmp.end(); ++c) {
        for (std::vector<E8>::iterator r = ((Obj*)*c)->v1b4.begin(); r != ((Obj*)*c)->v1b4.end(); ++r) {
            Obj* x = (Obj*)r->obj;
            for (int k = 0; k < 3; k++)
                ((char*)x)[0x139 + k * 0x2c] = 0;
            if (x->f18) {
                x->f18->f139 = 0;
                x->f18->f165 = 0;
                x->f18->f191 = 0;
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        bp[i].b[0].v.resize(0);
        bp[i].b[1].v.resize(0);
    }
    bp5.b[0].v.resize(0);
    bp5.b[1].v.resize(0);

    {
        Bucket* buckets10 = &bp5.b[0];
        Bucket* buckets0 = &bp[0].b[0];
        for (std::vector<int>::iterator f = tmp.begin(); f != tmp.end(); ++f) {
            for (std::vector<E8>::iterator r = ((Obj*)*f)->v1b4.begin(); r != ((Obj*)*f)->v1b4.end(); ++r) {
                Obj* x = (Obj*)r->obj;
                int rec2 = x->f0a0 ? (int)x : (int)x->f18;
                if (((Obj*)rec2)->f165 == 0) {
                    buckets10[((Obj*)rec2)->f140].v.push_back(rec2);
                    ((Obj*)rec2)->f165 = 1;
                }
                if (((Obj*)r->obj)->f139 == 0) {
                    buckets0[((Obj*)r->obj)->f114 + ((Obj*)r->obj)->f110 * 2].v.push_back(r->obj);
                    ((Obj*)r->obj)->f139 = 1;
                }
            }
        }
    }

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 2; j++)
            if (bp[i].b[j].upload(geom3c0, f4183c0) == (int)0xfff3ebc3)
                goto again;
    for (int j = 0; j < 2; j++)
        if (bp5.b[j].upload(geom3c0, f4183c0) == (int)0xfff3ebc3)
            goto again;

    for (std::vector<int>::iterator s = tmp.begin(); s != tmp.end(); ++s) {
        if (((Obj*)*s)->f18c & 1)
            continue;
        for (unsigned k = 0; k < ((Obj*)*s)->v1b4.size(); k++) {
            if (((Obj*)((Obj*)*s)->v1b4[k].obj)->f2c == 0) {
                tstring ts("default");
                ((Obj*)((Obj*)*s)->v1b4[k].obj)->f2c = g_resmgr.get_material(ts);
                ts.~tstring();
            }
            ((Obj*)((Obj*)*s)->v1b4[k].obj)->v2();
        }
        std::stable_sort(((Obj*)*s)->v1b4.begin(), ((Obj*)*s)->v1b4.end(), rec_less);
    }

    ISurface* rt = 0;
    ISurface* ds = 0;
    if (g_device->GetRenderTarget(0, (void**)&rt) < 0)
        mb_error("GetRenderTarget failed");
    if (g_device->GetDepthStencilSurface((void**)&ds) < 0)
        mb_error("GetDepthStencilSurface failed");
    f3b4 = -1;
    f3b8 = -1;

    int last_tex = 0;
    for (std::vector<int>::iterator d = tmp.begin(); d != tmp.end(); ++d) {
        Obj* o = (Obj*)*d;
        if (o->f150 != (int)&g_render || o->f15c != 0) {
            if (g_scene_open)
                g_device->EndScene();
            g_device->BeginScene();
            g_scene_open = 1;
        }
        if (o->f150 != (int)&g_render) {
            if (o->f150 != 0) {
                TexRec* t = (TexRec*)o->f150;
                void* surf = (void*)(t->f20 == 1 ? t->f48 : t->f44);
                if (g_device->SetRenderTarget(0, surf) < 0)
                    mb_error("Unable to set render target texture! Try turning off shadows");
                ((TexRec*)o->f150)->f50 = f418828;
            } else {
                g_device->SetRenderTarget(0, rt);
            }
        }
        last_tex = o->f150;
        float bias = 0.0f;
        g_device->SetRenderState(0xc3, *(unsigned*)&bias);
        g_device->SetRenderState(0xaf, *(unsigned*)&bias);
        if (o->f15c != 0 && ((TexRec*)o->f15c)->f18 != 0) {
            TexRec* d = (TexRec*)o->f15c;
            if (d->f20 == 2) {
                g_device->SetDepthStencilSurface((void*)d->f44);
            } else {
                void* surf = (void*)(d->f20 == 1 ? d->f48 : d->f44);
                g_device->SetDepthStencilSurface(surf);
                bias = 0.0002f;
                g_device->SetRenderState(0xc3, *(unsigned*)&bias);
                bias = 2.0f;
                g_device->SetRenderState(0xaf, *(unsigned*)&bias);
            }
            ds = (ISurface*)o->f15c;
        }
        o->apply_state();
        if (g_5e28fc)
            draw_a(o);
        else
            draw_b(o);
    }

    if (last_tex)
        g_device->SetRenderTarget(0, rt);
    rt->Release();

    if (ds)
        g_device->SetDepthStencilSurface(ds);
    ds->Release();

    g_device->SetStreamSource(0, 0, 0, 0);
    if (g_scene_open)
        g_device->EndScene();
    g_scene_open = 0;

    for (std::vector<int>::iterator e = tmp.begin(); e != tmp.end(); ++e) {
        Obj* o = (Obj*)*e;
        o->v1b4.clear();
        o->f1a4 = 0;
        for (unsigned i = 0; i < o->v1cc.size(); i++)
            ((Obj*)o->v1cc[i])->clear_geometry();
    }

    if (pass == 0)
        v3a4.resize(0);
    for (int i = 0; i < 3; i++)
        g_device->SetTexture(i, 0);
    return true;
}
