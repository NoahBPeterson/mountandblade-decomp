// port/gfx/shaders/ffp_vs.hlsl: the Direct3D 9 fixed-function vertex pipeline as a shader.
// Transform (world/view/projection) or pretransformed (XYZRHW) positions, D3D9 lighting in camera space (directional,
// point, spot; material colour sources; specular with local / infinite viewer), texture coordinate generation and
// texture transforms per stage, vertex fog (linear / exp / exp2, range or depth based).
// The generated entry point (ffp.cpp) fills FfpIn from the vertex declaration and defines:
//   FFP_RHW, FFP_HAS_NORMAL, FFP_HAS_C0, FFP_HAS_C1, FFP_TC_DIM0..7 (0 = absent)
// C mirror of the block: FfpVsBlock in ffp.cpp.

[[vk::binding(0, 1)]] cbuffer FfpVs {
    row_major float4x4 ffp_world;
    row_major float4x4 ffp_wv;
    row_major float4x4 ffp_wvp;
    row_major float4x4 ffp_wv_it;          // transpose(inverse(world*view)): normals
    row_major float4x4 ffp_texm[4];
    float4 ffp_tci[4];                     // x coord index, y texgen (0 none 1 normal 2 position 3 reflection 4 sphere), z count (0 = no transform), w projected
    float4 ffp_mat_diffuse;
    float4 ffp_mat_ambient;
    float4 ffp_mat_specular;
    float4 ffp_mat_emissive;
    float4 ffp_misc;                       // x power, y light count
    float4 ffp_global_ambient;
    float4 ffp_flags0;                     // x lighting, y normalize, z specular, w local viewer
    float4 ffp_flags1;                     // x colorvertex, y diffuse src, z specular src, w ambient src
    float4 ffp_flags2;                     // x emissive src, y fog vertex mode, z fog table mode, w range fog
    float4 ffp_fog;                        // x start, y end, z density, w fog enabled
    float4 ffp_l_diffuse[8];
    float4 ffp_l_specular[8];
    float4 ffp_l_ambient[8];
    float4 ffp_l_pos[8];                   // camera space position, w = type
    float4 ffp_l_dir[8];                   // camera space direction (normalised)
    float4 ffp_l_atten[8];                 // a0 a1 a2 range
    float4 ffp_l_spot[8];                  // falloff, cos(theta/2), cos(phi/2)
};

struct FfpIn {
    float4 pos;
    float3 normal;
    float4 c0;
    float4 c1;
    float4 tc[8];
};

float4 ffp_material_color(int src, float4 mat, FfpIn i)
{
    if (ffp_flags1.x > 0.5) {
#if FFP_HAS_C0
        if (src == 1) return i.c0;
#endif
#if FFP_HAS_C1
        if (src == 2) return i.c1;
#endif
    }
    return mat;
}

float ffp_fog_factor(int mode, float d)
{
    if (mode == 1) return 1.0 / exp(d * ffp_fog.z);
    if (mode == 2) { float x = d * ffp_fog.z; return 1.0 / exp(x * x); }
    if (mode == 3) return saturate((ffp_fog.y - d) / (ffp_fog.y - ffp_fog.x));
    return 1.0;
}

float4 ffp_tex_input(int idx, FfpIn i)
{
    float4 r = i.tc[0];
    [unroll] for (int k = 1; k < 8; k++) if (idx == k) r = i.tc[k];
    return r;
}

MBP_VSOUT ffp_vs_main(FfpIn i)
{
    MBP_VSOUT o = (MBP_VSOUT)0;
#if FFP_RHW
    // pretransformed: x, y in render-target pixels (D3D9 pixel centres), z in viewport depth, w = 1/w
    float w = i.pos.w != 0.0 ? 1.0 / i.pos.w : 1.0;
    float2 ndc = float2((i.pos.x + 0.5 - mbp_vp.x) * mbp_halfpix.z - 1.0, (i.pos.y + 0.5 - mbp_vp.y) * mbp_halfpix.w + 1.0);
    float z = (i.pos.z - mbp_vp.z) / mbp_vp.w;
    o.pos = float4(ndc * w, z * w, w);
    o.v0 = i.c0;
    o.v1 = i.c1;
#if FFP_HAS_C1
    o.fog = i.c1.a;
#else
    o.fog = 1.0;
#endif
    o.t0 = i.tc[0]; o.t1 = i.tc[1]; o.t2 = i.tc[2]; o.t3 = i.tc[3];
    o.t4 = i.tc[4]; o.t5 = i.tc[5]; o.t6 = i.tc[6]; o.t7 = i.tc[7];
    o.clipd = 1.0;
    return o;
#else
    float4 wpos = mul(i.pos, ffp_world);
    float3 vpos = mul(i.pos, ffp_wv).xyz;
    o.pos = mul(i.pos, ffp_wvp);
    float3 n = float3(0, 0, 0);
#if FFP_HAS_NORMAL
    n = mul(i.normal, (float3x3)ffp_wv_it);
    if (ffp_flags0.y > 0.5) n = normalize(n);
#endif
    float3 eye = ffp_flags0.w > 0.5 ? -normalize(vpos) : float3(0, 0, -1);

    // ---- colours ----
    float4 diffuse = float4(1, 1, 1, 1), specular = float4(0, 0, 0, 0);
#if FFP_HAS_C0
    diffuse = i.c0;
#endif
#if FFP_HAS_C1
    specular = i.c1;
#endif
    if (ffp_flags0.x > 0.5) {
        float4 md = ffp_material_color((int)ffp_flags1.y, ffp_mat_diffuse, i);
        float4 ms = ffp_material_color((int)ffp_flags1.z, ffp_mat_specular, i);
        float4 ma = ffp_material_color((int)ffp_flags1.w, ffp_mat_ambient, i);
        float4 me = ffp_material_color((int)ffp_flags2.x, ffp_mat_emissive, i);
        float3 amb = ffp_global_ambient.rgb;
        float3 dif = float3(0, 0, 0);
        float3 spc = float3(0, 0, 0);
        int nl = (int)ffp_misc.y;
        for (int k = 0; k < 8; k++) {
            if (k >= nl) break;
            int type = (int)ffp_l_pos[k].w;
            float3 L;
            float att = 1.0;
            if (type == 3) {
                L = -ffp_l_dir[k].xyz;
            } else {
                float3 d = ffp_l_pos[k].xyz - vpos;
                float dist = length(d);
                L = d / max(dist, 1e-6);
                if (dist > ffp_l_atten[k].w) att = 0.0;
                else att = 1.0 / max(ffp_l_atten[k].x + ffp_l_atten[k].y * dist + ffp_l_atten[k].z * dist * dist, 1e-6);
                if (type == 2) {
                    float rho = dot(-L, ffp_l_dir[k].xyz);
                    float ct = ffp_l_spot[k].y, cp = ffp_l_spot[k].z;
                    float spot = rho > ct ? 1.0 : (rho <= cp ? 0.0 : pow(saturate((rho - cp) / max(ct - cp, 1e-6)), ffp_l_spot[k].x));
                    att *= spot;
                }
            }
            amb += ffp_l_ambient[k].rgb * att;
            float ndl = dot(n, L);
            if (ndl > 0.0) {
                dif += ffp_l_diffuse[k].rgb * ndl * att;
                if (ffp_flags0.z > 0.5) {
                    float3 h = normalize(L + eye);
                    float ndh = dot(n, h);
                    if (ndh > 0.0) spc += ffp_l_specular[k].rgb * pow(ndh, ffp_misc.x) * att;
                }
            }
        }
        diffuse = float4(saturate(me.rgb + ma.rgb * amb + md.rgb * dif), md.a);
        specular = float4(saturate(ms.rgb * spc), ms.a);
    }
    o.v0 = saturate(diffuse);
    o.v1 = saturate(specular);

    // ---- texture coordinates ----
    float4 tcs[4];
    [unroll] for (int s = 0; s < 4; s++) {
        int gen = (int)ffp_tci[s].y;
        float4 tc = ffp_tex_input((int)ffp_tci[s].x, i);
        if (gen == 1) tc = float4(n, 1);
        else if (gen == 2) tc = float4(vpos, 1);
        else if (gen == 3 || gen == 4) {
            float3 e = normalize(vpos);
            float3 r = e - 2.0 * dot(e, n) * n;
            if (gen == 3) tc = float4(r, 1);
            else {
                float m = 2.0 * sqrt(r.x * r.x + r.y * r.y + (r.z + 1.0) * (r.z + 1.0));
                tc = float4(r.x / m + 0.5, -r.y / m + 0.5, 0, 1);
            }
        }
        int cnt = (int)ffp_tci[s].z;
        if (cnt > 0) {
            tc = mul(tc, ffp_texm[s]);
            if (ffp_tci[s].w > 0.5) {
                float q = cnt == 2 ? tc.y : cnt == 3 ? tc.z : tc.w;
                tc /= q;
            }
        }
        tcs[s] = tc;
    }
    o.t0 = tcs[0]; o.t1 = tcs[1]; o.t2 = tcs[2]; o.t3 = tcs[3];

    // ---- fog ----
    int vmode = (int)ffp_flags2.y;
    int tmode = (int)ffp_flags2.z;
    float d = ffp_flags2.w > 0.5 ? length(vpos) : abs(vpos.z);
    o.fog = tmode != 0 ? ffp_fog_factor(tmode, abs(vpos.z)) : ffp_fog_factor(vmode, d);
    mbp_vs_finish(o);
    return o;
#endif
}
