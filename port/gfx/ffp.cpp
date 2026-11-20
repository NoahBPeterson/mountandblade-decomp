// port/gfx/ffp.cpp: the fixed-function pipeline: a vertex shader generated per vertex layout (shaders/ffp_vs.hlsl)
// and one uber pixel shader (shaders/ffp_ps.hlsl), driven by uniform blocks filled from the device state per draw.
#include "gfx_internal.h"
#include "gfx_shader_sources.h"

namespace mbg {

struct FfpVsBlock {
    D3DMATRIX world, wv, wvp, wv_it;
    D3DMATRIX texm[4];
    float tci[4][4];
    float mat_diffuse[4], mat_ambient[4], mat_specular[4], mat_emissive[4];
    float misc[4];
    float global_ambient[4];
    float flags0[4], flags1[4], flags2[4];
    float fog[4];
    float l_diffuse[8][4], l_specular[8][4], l_ambient[8][4], l_pos[8][4], l_dir[8][4], l_atten[8][4], l_spot[8][4];
};
struct FfpPsBlock {
    float cop[4][4], aop[4][4], smisc[4][4], sconst[4][4];
    float tfactor[4];
    float pflags[4];
};

static inline float fv(DWORD d) { float f; memcpy(&f, &d, 4); return f; }
static inline void unpack(DWORD c, float* o)
{
    o[0] = ((c >> 16) & 255) / 255.0f;
    o[1] = ((c >> 8) & 255) / 255.0f;
    o[2] = (c & 255) / 255.0f;
    o[3] = (c >> 24) / 255.0f;
}

// HLSL type and float4 expansion of one declaration element as the fixed-function / effect wrappers read it
std::string decl_hlsl_type(BYTE t)
{
    switch (t) {
    case D3DDECLTYPE_FLOAT1: return "float";
    case D3DDECLTYPE_FLOAT2: case D3DDECLTYPE_SHORT2N: case D3DDECLTYPE_USHORT2N: case D3DDECLTYPE_FLOAT16_2: return "float2";
    case D3DDECLTYPE_FLOAT3: return "float3";
    case D3DDECLTYPE_UBYTE4: return "uint4";
    case D3DDECLTYPE_SHORT2: return "int2";
    case D3DDECLTYPE_SHORT4: return "int4";
    default: return "float4";
    }
}
std::string decl_to_float4(BYTE t, const std::string& v)
{
    switch (t) {
    case D3DDECLTYPE_FLOAT1: return "float4(" + v + ", 0, 0, 1)";
    case D3DDECLTYPE_FLOAT2: case D3DDECLTYPE_SHORT2N: case D3DDECLTYPE_USHORT2N: case D3DDECLTYPE_FLOAT16_2: return "float4(" + v + ", 0, 1)";
    case D3DDECLTYPE_FLOAT3: return "float4(" + v + ", 1)";
    case D3DDECLTYPE_D3DCOLOR: return v + ".zyxw";
    case D3DDECLTYPE_UBYTE4: return "float4(" + v + ")";
    case D3DDECLTYPE_SHORT2: return "float4(float2(" + v + "), 0, 1)";
    case D3DDECLTYPE_SHORT4: return "float4(" + v + ")";
    default: return v;
    }
}
static int decl_dims(BYTE t)
{
    switch (t) {
    case D3DDECLTYPE_FLOAT1: return 1;
    case D3DDECLTYPE_FLOAT2: case D3DDECLTYPE_SHORT2: case D3DDECLTYPE_SHORT2N: case D3DDECLTYPE_USHORT2N: case D3DDECLTYPE_FLOAT16_2: return 2;
    case D3DDECLTYPE_FLOAT3: return 3;
    default: return 4;
    }
}

struct FfpVS final : public VShader {
    Ffp::Impl* owner = nullptr;
    VsVariant* variant(const VertexDecl* decl) override;
};
struct FfpPS final : public PShader {};

struct Ffp::Impl {
    Device* dev;
    FfpVS* vs = nullptr;
    FfpPS* ps = nullptr;
    FfpVsBlock vb{};
    FfpPsBlock pb{};
    UniformSource vsrc{}, psrc{};
    uint64_t ver = 1;
};

VsVariant* FfpVS::variant(const VertexDecl* decl)
{
    auto it = variants.find(decl->hash);
    if (it != variants.end()) return &it->second;
    VsVariant& v = variants[decl->hash];
    std::string src = "#define MBP_VERTEX 1\n";
    const D3DVERTEXELEMENT9* pos = decl->find(D3DDECLUSAGE_POSITIONT, 0);
    bool rhw = pos != nullptr;
    if (!pos) pos = decl->find(D3DDECLUSAGE_POSITION, 0);
    const D3DVERTEXELEMENT9* nrm = decl->find(D3DDECLUSAGE_NORMAL, 0);
    const D3DVERTEXELEMENT9* c0 = decl->find(D3DDECLUSAGE_COLOR, 0);
    const D3DVERTEXELEMENT9* c1 = decl->find(D3DDECLUSAGE_COLOR, 1);
    src += "#define FFP_RHW " + std::to_string(rhw ? 1 : 0) + "\n";
    src += "#define FFP_HAS_NORMAL " + std::to_string(nrm ? 1 : 0) + "\n";
    src += "#define FFP_HAS_C0 " + std::to_string(c0 ? 1 : 0) + "\n";
    src += "#define FFP_HAS_C1 " + std::to_string(c1 ? 1 : 0) + "\n";
    src += k_src_mbp_common;
    src += k_src_ffp_vs;
    std::string params, body;
    uint32_t loc = 0;
    auto input = [&](const D3DVERTEXELEMENT9* e, const char* name) {
        if (!params.empty()) params += ", ";
        params += "[[vk::location(" + std::to_string(loc) + ")]] " + decl_hlsl_type(e->Type) + " " + name + " : MBPA" + std::to_string(loc);
        v.inputs.push_back(VsInput{loc, e->Stream, e->Offset, e->Type, e->Usage, e->UsageIndex});
        loc++;
    };
    body += "    FfpIn i = (FfpIn)0;\n    i.c0 = float4(1, 1, 1, 1);\n";
    if (pos) { input(pos, "a_pos"); body += "    i.pos = " + decl_to_float4(pos->Type, "a_pos") + ";\n"; }
    else body += "    i.pos = float4(0, 0, 0, 1);\n";
    if (nrm) { input(nrm, "a_nrm"); body += "    i.normal = " + decl_to_float4(nrm->Type, "a_nrm") + ".xyz;\n"; }
    if (c0) { input(c0, "a_c0"); body += "    i.c0 = " + decl_to_float4(c0->Type, "a_c0") + ";\n"; }
    if (c1) { input(c1, "a_c1"); body += "    i.c1 = " + decl_to_float4(c1->Type, "a_c1") + ";\n"; }
    for (int t = 0; t < 8; t++) {
        const D3DVERTEXELEMENT9* e = decl->find(D3DDECLUSAGE_TEXCOORD, (BYTE)t);
        if (!e) continue;
        std::string n = "a_t" + std::to_string(t);
        input(e, n.c_str());
        int d = decl_dims(e->Type);
        // D3D9 hands an n-component coordinate to the texture transform as (u, 1, 0, 0) / (u, v, 1, 0) / (u, v, w, 1)
        std::string ex = d == 1 ? "float4(" + n + ", 1, 0, 0)" : d == 2 ? "float4(" + n + ", 1, 0)" : d == 3 ? "float4(" + n + ", 1)" : decl_to_float4(e->Type, n);
        body += "    i.tc[" + std::to_string(t) + "] = " + ex + ";\n";
    }
    src += "MBP_VSOUT mbp_vs(" + params + ")\n{\n" + body + "    return ffp_vs_main(i);\n}\n";
    CompiledShader cs;
    if (!compile_shader(dev->gpu, src, "mbp_vs", Stage::Vertex, cs)) {
        log("fixed-function VS: %s", cs.error.c_str());
        if (verbose()) log("%s", src.c_str());
        return &v;
    }
    v.shader = cs.shader;
    v.refl = cs.refl;
    v.ok = true;
    return &v;
}

Ffp::Ffp(Device* d) : p(new Impl())
{
    p->dev = d;
    p->vs = new FfpVS();
    p->vs->dev = d;
    p->vs->owner = p;
    p->vs->is_ffp = true;
    p->vsrc.data = &p->vb;
    p->vsrc.size = sizeof p->vb;
    p->psrc.data = &p->pb;
    p->psrc.size = sizeof p->pb;
    p->vs->ubo0 = &p->vsrc;
    p->ps = new FfpPS();
    p->ps->dev = d;
    p->ps->is_ffp = true;
    p->ps->ubo0 = &p->psrc;
    std::string src = std::string("#define MBP_PIXEL 1\n") + k_src_mbp_common + k_src_ffp_ps;
    if (!compile_shader(d->gpu, src, "mbp_ps", Stage::Fragment, p->ps->cs)) log("fixed-function PS: %s", p->ps->cs.error.c_str());
    p->ps->stage_of_binding = p->ps->cs.refl.sampler_global;
}

Ffp::~Ffp()
{
    SDL_GPUDevice* g = p->dev->gpu;
    for (auto& kv : p->vs->variants)
        if (kv.second.shader) SDL_ReleaseGPUShader(g, kv.second.shader);
    if (p->ps->cs.shader) SDL_ReleaseGPUShader(g, p->ps->cs.shader);
    p->vs->Release();
    p->ps->Release();
    delete p;
}

VShader* Ffp::vs() { return p->vs; }
PShader* Ffp::ps() { return p->ps; }

static void transpose(D3DMATRIX& o, const D3DMATRIX& m)
{
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) o.m[r][c] = m.m[c][r];
}

static void mul3(const D3DMATRIX& m, const float* v, float w, float* o)
{
    for (int c = 0; c < 3; c++) o[c] = v[0] * m.m[0][c] + v[1] * m.m[1][c] + v[2] * m.m[2][c] + w * m.m[3][c];
}

void Ffp::update(const VertexDecl*)
{
    Device* d = p->dev;
    const DevState& st = d->st;
    FfpVsBlock vb{};
    vb.world = st.world[0];
    mat_mul(vb.wv, st.world[0], st.view);
    mat_mul(vb.wvp, vb.wv, st.proj);
    D3DMATRIX inv;
    if (mat_inverse(inv, nullptr, vb.wv)) transpose(vb.wv_it, inv);
    else mat_identity(vb.wv_it);
    for (int s = 0; s < 4; s++) {
        vb.texm[s] = st.texm[s];
        DWORD tci = st.tss[s][D3DTSS_TEXCOORDINDEX];
        DWORD ttff = st.tss[s][D3DTSS_TEXTURETRANSFORMFLAGS];
        vb.tci[s][0] = (float)(tci & 0xffff);
        DWORD gen = tci >> 16;
        vb.tci[s][1] = (float)(gen <= 4 ? gen : 0);
        vb.tci[s][2] = (float)(ttff & 0xff);
        vb.tci[s][3] = (ttff & D3DTTFF_PROJECTED) ? 1.0f : 0.0f;
    }
    memcpy(vb.mat_diffuse, &st.material.Diffuse, 16);
    memcpy(vb.mat_ambient, &st.material.Ambient, 16);
    memcpy(vb.mat_specular, &st.material.Specular, 16);
    memcpy(vb.mat_emissive, &st.material.Emissive, 16);
    vb.misc[0] = st.material.Power;
    unpack(st.rs[D3DRS_AMBIENT], vb.global_ambient);
    vb.flags0[0] = st.rs[D3DRS_LIGHTING] ? 1.0f : 0.0f;
    vb.flags0[1] = st.rs[D3DRS_NORMALIZENORMALS] ? 1.0f : 0.0f;
    vb.flags0[2] = st.rs[D3DRS_SPECULARENABLE] ? 1.0f : 0.0f;
    vb.flags0[3] = st.rs[D3DRS_LOCALVIEWER] ? 1.0f : 0.0f;
    vb.flags1[0] = st.rs[D3DRS_COLORVERTEX] ? 1.0f : 0.0f;
    vb.flags1[1] = (float)st.rs[D3DRS_DIFFUSEMATERIALSOURCE];
    vb.flags1[2] = (float)st.rs[D3DRS_SPECULARMATERIALSOURCE];
    vb.flags1[3] = (float)st.rs[D3DRS_AMBIENTMATERIALSOURCE];
    vb.flags2[0] = (float)st.rs[D3DRS_EMISSIVEMATERIALSOURCE];
    vb.flags2[1] = (float)st.rs[D3DRS_FOGVERTEXMODE];
    vb.flags2[2] = (float)st.rs[D3DRS_FOGTABLEMODE];
    vb.flags2[3] = st.rs[D3DRS_RANGEFOGENABLE] ? 1.0f : 0.0f;
    vb.fog[0] = fv(st.rs[D3DRS_FOGSTART]);
    vb.fog[1] = fv(st.rs[D3DRS_FOGEND]);
    vb.fog[2] = fv(st.rs[D3DRS_FOGDENSITY]);
    vb.fog[3] = st.rs[D3DRS_FOGENABLE] ? 1.0f : 0.0f;
    int nl = 0;
    if (st.rs[D3DRS_LIGHTING]) {
        for (const auto& kv : st.lights) {
            if (!kv.second.enabled || nl >= MAX_LIGHTS_ACTIVE) continue;
            const D3DLIGHT9& l = kv.second.l;
            memcpy(vb.l_diffuse[nl], &l.Diffuse, 16);
            memcpy(vb.l_specular[nl], &l.Specular, 16);
            memcpy(vb.l_ambient[nl], &l.Ambient, 16);
            float pos[3] = {l.Position.x, l.Position.y, l.Position.z}, dir[3] = {l.Direction.x, l.Direction.y, l.Direction.z};
            mul3(st.view, pos, 1.0f, vb.l_pos[nl]);
            vb.l_pos[nl][3] = (float)l.Type;
            mul3(st.view, dir, 0.0f, vb.l_dir[nl]);
            float len = sqrtf(vb.l_dir[nl][0] * vb.l_dir[nl][0] + vb.l_dir[nl][1] * vb.l_dir[nl][1] + vb.l_dir[nl][2] * vb.l_dir[nl][2]);
            if (len > 0) for (int i = 0; i < 3; i++) vb.l_dir[nl][i] /= len;
            vb.l_atten[nl][0] = l.Attenuation0;
            vb.l_atten[nl][1] = l.Attenuation1;
            vb.l_atten[nl][2] = l.Attenuation2;
            vb.l_atten[nl][3] = l.Range > 0 ? l.Range : 1e30f;
            vb.l_spot[nl][0] = l.Falloff;
            vb.l_spot[nl][1] = cosf(l.Theta * 0.5f);
            vb.l_spot[nl][2] = cosf(l.Phi * 0.5f);
            nl++;
        }
    }
    vb.misc[1] = (float)nl;
    if (memcmp(&vb, &p->vb, sizeof vb) != 0) {
        p->vb = vb;
        p->vsrc.version = ++p->ver;
    }

    FfpPsBlock pb{};
    for (int s = 0; s < 4; s++) {
        const DWORD* t = st.tss[s];
        pb.cop[s][0] = (float)t[D3DTSS_COLOROP];
        pb.cop[s][1] = (float)t[D3DTSS_COLORARG1];
        pb.cop[s][2] = (float)t[D3DTSS_COLORARG2];
        pb.cop[s][3] = (float)t[D3DTSS_COLORARG0];
        pb.aop[s][0] = (float)t[D3DTSS_ALPHAOP];
        pb.aop[s][1] = (float)t[D3DTSS_ALPHAARG1];
        pb.aop[s][2] = (float)t[D3DTSS_ALPHAARG2];
        pb.aop[s][3] = (float)t[D3DTSS_ALPHAARG0];
        pb.smisc[s][0] = st.tex[s] ? 1.0f : 0.0f;
        pb.smisc[s][1] = (t[D3DTSS_RESULTARG] & 15) == D3DTA_TEMP ? 1.0f : 0.0f;
        unpack(t[D3DTSS_CONSTANT], pb.sconst[s]);
    }
    // D3D9: a disabled colour stage ends the cascade; stage 0 disabled means plain diffuse
    unpack(st.rs[D3DRS_TEXTUREFACTOR], pb.tfactor);
    pb.pflags[0] = st.rs[D3DRS_SPECULARENABLE] ? 1.0f : 0.0f;
    if (memcmp(&pb, &p->pb, sizeof pb) != 0) {
        p->pb = pb;
        p->psrc.version = ++p->ver;
    }
}

} // namespace mbg
