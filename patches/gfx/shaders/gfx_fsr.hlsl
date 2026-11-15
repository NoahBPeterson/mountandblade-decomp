// patches/gfx/shaders/gfx_fsr.hlsl: the gfx set's full-screen passes (vs_3_0 / ps_3_0, Direct3D 9).
//
//   vs_quad   pass-through of a clip-space quad (the CPU applies D3D9's half-pixel offset)
//   ps_copy   one bilinear tap: the plain upscale (gfx_fsr = 0) and the copy of the backbuffer into the scene target
//   ps_easu   FSR 1 EASU (edge adaptive spatial upsampling): render size -> output size
//   ps_rcas   FSR 1 RCAS (robust contrast adaptive sharpening): output size -> backbuffer
//
// EASU and RCAS are ports of the 32-bit (non-packed) FsrEasuF / FsrRcasF of AMD FidelityFX Super Resolution 1
// (ffx_fsr1.h "v1.20210629", FidelityFX-FSR 1.0.2, MIT licence: LICENSE-FidelityFX-FSR.txt next to this file). The
// algorithm and its constants (FsrEasuCon / FsrRcasCon, computed on the CPU in gfx_scale.cpp) are AMD's. Differences
// forced by shader model 3, which has no integer instructions and no Gather:
//   - each Gather4 of FsrEasuRF/GF/BF is four point taps around the gather point, in Gather4's x/y/z/w order, and the
//     two half-used gathers fetch only their used halves, so EASU keeps its 12 taps;
//   - the integer-trick approximations APrxLoRcpF1 / APrxLoRsqF1 / APrxMedRcpF1 are plain rcp / rsqrt; reciprocals
//     that can see 0 are clamped away from 0 (the approximations return a large finite value there, an IEEE rcp
//     returns inf and 0 * inf is NaN);
//   - the FsrEasuCon / FsrRcasCon values arrive as floats, not as bit-cast uints;
//   - ip, the integer output pixel, comes from VPOS (floor(VPOS + 0.25) takes either half-pixel convention).
// Compiled by build_shaders.sh into gfx_shaders.h (the build never needs a shader compiler).

float4 c_con0 : register(c0);       // FsrEasuCon con0..con3
float4 c_con1 : register(c1);
float4 c_con2 : register(c2);
float4 c_con3 : register(c3);
float4 c_rcas : register(c4);       // x: FsrRcasCon con[0] (sharpness as a linear factor), zw: 1 / RCAS input size
sampler2D s_src : register(s0);     // point sampled, clamped (ps_easu, ps_rcas); linear (ps_copy)

struct quad_out {
    float4 pos : POSITION;
    float2 uv : TEXCOORD0;
};

quad_out vs_quad(float4 pos : POSITION, float2 uv : TEXCOORD0)
{
    quad_out o;
    o.pos = pos;
    o.uv = uv;
    return o;
}

float4 ps_copy(float2 uv : TEXCOORD0) : COLOR
{
    return float4(tex2Dlod(s_src, float4(uv, 0, 0)).rgb, 1);
}

// ---- EASU ---------------------------------------------------------------------------------------------------------------
float3 easu_tap(float2 p)
{
    return tex2Dlod(s_src, float4(p, 0, 0)).rgb;
}

// Gather4 at p, a texel corner: x = (-,+), y = (+,+), z = (+,-), w = (-,-) half a texel away. EASU uses only x/y of
// the first gather and z/w of the last ('zz' in bczz / zzon), so those two fetch only the halves used.
void easu_gather(float2 p, out float4 r, out float4 g, out float4 b)
{
    float2 h = 0.5 * c_con1.xy;
    float3 x = easu_tap(p + float2(-h.x,  h.y));
    float3 y = easu_tap(p + float2( h.x,  h.y));
    float3 z = easu_tap(p + float2( h.x, -h.y));
    float3 w = easu_tap(p + float2(-h.x, -h.y));
    r = float4(x.r, y.r, z.r, w.r);
    g = float4(x.g, y.g, z.g, w.g);
    b = float4(x.b, y.b, z.b, w.b);
}

void easu_gather_xy(float2 p, out float4 r, out float4 g, out float4 b)
{
    float2 h = 0.5 * c_con1.xy;
    float3 x = easu_tap(p + float2(-h.x,  h.y));
    float3 y = easu_tap(p + float2( h.x,  h.y));
    r = float4(x.r, y.r, 0, 0);
    g = float4(x.g, y.g, 0, 0);
    b = float4(x.b, y.b, 0, 0);
}

void easu_gather_zw(float2 p, out float4 r, out float4 g, out float4 b)
{
    float2 h = 0.5 * c_con1.xy;
    float3 z = easu_tap(p + float2( h.x, -h.y));
    float3 w = easu_tap(p + float2(-h.x, -h.y));
    r = float4(0, 0, z.r, w.r);
    g = float4(0, 0, z.g, w.g);
    b = float4(0, 0, z.b, w.b);
}

float rcp_nz(float x)
{
    return 1.0 / max(x, 1.0 / 65536.0);
}

void FsrEasuTapF(inout float3 aC, inout float aW, float2 off, float2 dir, float2 len, float lob, float clp, float3 c)
{
    float2 v;
    v.x = (off.x * ( dir.x)) + (off.y * dir.y);
    v.y = (off.x * (-dir.y)) + (off.y * dir.x);
    v *= len;
    float d2 = v.x * v.x + v.y * v.y;
    d2 = min(d2, clp);
    float wB = (2.0 / 5.0) * d2 + (-1.0);
    float wA = lob * d2 + (-1.0);
    wB *= wB;
    wA *= wA;
    wB = (25.0 / 16.0) * wB + (-(25.0 / 16.0 - 1.0));
    float w = wB * wA;
    aC += c * w;
    aW += w;
}

void FsrEasuSetF(inout float2 dir, inout float len, float w, float lA, float lB, float lC, float lD, float lE)
{
    float dc = lD - lC;
    float cb = lC - lB;
    float lenX = max(abs(dc), abs(cb));
    lenX = rcp_nz(lenX);
    float dirX = lD - lB;
    dir.x += dirX * w;
    lenX = saturate(abs(dirX) * lenX);
    lenX *= lenX;
    len += lenX * w;
    float ec = lE - lC;
    float ca = lC - lA;
    float lenY = max(abs(ec), abs(ca));
    lenY = rcp_nz(lenY);
    float dirY = lE - lA;
    dir.y += dirY * w;
    lenY = saturate(abs(dirY) * lenY);
    lenY *= lenY;
    len += lenY * w;
}

float4 ps_easu(float2 vpos : VPOS) : COLOR
{
    float2 ip = floor(vpos + 0.25);
    float2 pp = ip * c_con0.xy + c_con0.zw;
    float2 fp = floor(pp);
    pp -= fp;
    float2 p0 = fp * c_con1.xy + c_con1.zw;
    float2 p1 = p0 + c_con2.xy;
    float2 p2 = p0 + c_con2.zw;
    float2 p3 = p0 + c_con3.xy;
    float4 bczzR, bczzG, bczzB, ijfeR, ijfeG, ijfeB, klhgR, klhgG, klhgB, zzonR, zzonG, zzonB;
    easu_gather_xy(p0, bczzR, bczzG, bczzB);
    easu_gather(p1, ijfeR, ijfeG, ijfeB);
    easu_gather(p2, klhgR, klhgG, klhgB);
    easu_gather_zw(p3, zzonR, zzonG, zzonB);
    float4 bczzL = bczzB * 0.5 + (bczzR * 0.5 + bczzG);
    float4 ijfeL = ijfeB * 0.5 + (ijfeR * 0.5 + ijfeG);
    float4 klhgL = klhgB * 0.5 + (klhgR * 0.5 + klhgG);
    float4 zzonL = zzonB * 0.5 + (zzonR * 0.5 + zzonG);
    float bL = bczzL.x;
    float cL = bczzL.y;
    float iL = ijfeL.x;
    float jL = ijfeL.y;
    float fL = ijfeL.z;
    float eL = ijfeL.w;
    float kL = klhgL.x;
    float lL = klhgL.y;
    float hL = klhgL.z;
    float gL = klhgL.w;
    float oL = zzonL.z;
    float nL = zzonL.w;
    float2 dir = 0;
    float len = 0;
    // the bilinear weights of FsrEasuSetF's four calls (s t / u v)
    FsrEasuSetF(dir, len, (1.0 - pp.x) * (1.0 - pp.y), bL, eL, fL, gL, jL);
    FsrEasuSetF(dir, len,        pp.x  * (1.0 - pp.y), cL, fL, gL, hL, kL);
    FsrEasuSetF(dir, len, (1.0 - pp.x) *        pp.y , fL, iL, jL, kL, nL);
    FsrEasuSetF(dir, len,        pp.x  *        pp.y , gL, jL, kL, lL, oL);
    float2 dir2 = dir * dir;
    float dirR = dir2.x + dir2.y;
    bool zro = dirR < (1.0 / 32768.0);
    dirR = rsqrt(max(dirR, 1.0 / 32768.0));
    dirR = zro ? 1.0 : dirR;
    dir.x = zro ? 1.0 : dir.x;
    dir *= dirR;
    len = len * 0.5;
    len *= len;
    float stretch = (dir.x * dir.x + dir.y * dir.y) * rcp_nz(max(abs(dir.x), abs(dir.y)));
    float2 len2 = float2(1.0 + (stretch - 1.0) * len, 1.0 + (-0.5) * len);
    float lob = 0.5 + ((1.0 / 4.0 - 0.04) - 0.5) * len;
    float clp = 1.0 / lob;
    float3 min4 = min(min(min(float3(ijfeR.z, ijfeG.z, ijfeB.z), float3(klhgR.w, klhgG.w, klhgB.w)),
                          float3(ijfeR.y, ijfeG.y, ijfeB.y)), float3(klhgR.x, klhgG.x, klhgB.x));
    float3 max4 = max(max(max(float3(ijfeR.z, ijfeG.z, ijfeB.z), float3(klhgR.w, klhgG.w, klhgB.w)),
                          float3(ijfeR.y, ijfeG.y, ijfeB.y)), float3(klhgR.x, klhgG.x, klhgB.x));
    float3 aC = 0;
    float aW = 0;
    FsrEasuTapF(aC, aW, float2( 0.0, -1.0) - pp, dir, len2, lob, clp, float3(bczzR.x, bczzG.x, bczzB.x));   // b
    FsrEasuTapF(aC, aW, float2( 1.0, -1.0) - pp, dir, len2, lob, clp, float3(bczzR.y, bczzG.y, bczzB.y));   // c
    FsrEasuTapF(aC, aW, float2(-1.0,  1.0) - pp, dir, len2, lob, clp, float3(ijfeR.x, ijfeG.x, ijfeB.x));   // i
    FsrEasuTapF(aC, aW, float2( 0.0,  1.0) - pp, dir, len2, lob, clp, float3(ijfeR.y, ijfeG.y, ijfeB.y));   // j
    FsrEasuTapF(aC, aW, float2( 0.0,  0.0) - pp, dir, len2, lob, clp, float3(ijfeR.z, ijfeG.z, ijfeB.z));   // f
    FsrEasuTapF(aC, aW, float2(-1.0,  0.0) - pp, dir, len2, lob, clp, float3(ijfeR.w, ijfeG.w, ijfeB.w));   // e
    FsrEasuTapF(aC, aW, float2( 1.0,  1.0) - pp, dir, len2, lob, clp, float3(klhgR.x, klhgG.x, klhgB.x));   // k
    FsrEasuTapF(aC, aW, float2( 2.0,  1.0) - pp, dir, len2, lob, clp, float3(klhgR.y, klhgG.y, klhgB.y));   // l
    FsrEasuTapF(aC, aW, float2( 2.0,  0.0) - pp, dir, len2, lob, clp, float3(klhgR.z, klhgG.z, klhgB.z));   // h
    FsrEasuTapF(aC, aW, float2( 1.0,  0.0) - pp, dir, len2, lob, clp, float3(klhgR.w, klhgG.w, klhgB.w));   // g
    FsrEasuTapF(aC, aW, float2( 1.0,  2.0) - pp, dir, len2, lob, clp, float3(zzonR.z, zzonG.z, zzonB.z));   // o
    FsrEasuTapF(aC, aW, float2( 0.0,  2.0) - pp, dir, len2, lob, clp, float3(zzonR.w, zzonG.w, zzonB.w));   // n
    float3 pix = min(max4, max(min4, aC * (1.0 / aW)));
    return float4(pix, 1);
}

// ---- RCAS ---------------------------------------------------------------------------------------------------------------
#define FSR_RCAS_LIMIT (0.25 - (1.0 / 16.0))

float3 rcas_load(float2 p)
{
    return tex2Dlod(s_src, float4((p + 0.5) * c_rcas.zw, 0, 0)).rgb;
}

float4 ps_rcas(float2 vpos : VPOS) : COLOR
{
    float2 sp = floor(vpos + 0.25);
    float3 b = rcas_load(sp + float2( 0, -1));
    float3 d = rcas_load(sp + float2(-1,  0));
    float3 e = rcas_load(sp);
    float3 f = rcas_load(sp + float2( 1,  0));
    float3 h = rcas_load(sp + float2( 0,  1));
    // (FsrRcasInputF: no input conversion; the noise detection only feeds FSR_RCAS_DENOISE, which is off)
    float3 mn4 = min(min(min(b, d), f), h);
    float3 mx4 = max(max(max(b, d), f), h);
    float2 peakC = float2(1.0, -1.0 * 4.0);
    // hitMin = min(mn4, e) / (4 mx4), hitMax = (1 - max(mx4, e)) / (4 mn4 - 4): denominators kept off 0
    float3 hitMin = min(mn4, e) / max(4.0 * mx4, 1.0 / 65536.0);
    float3 hitMax = (peakC.x - max(mx4, e)) / min(4.0 * mn4 + peakC.y, -1.0 / 65536.0);
    float3 lobeRGB = max(-hitMin, hitMax);
    float lobe = max(-FSR_RCAS_LIMIT, min(max(max(lobeRGB.r, lobeRGB.g), lobeRGB.b), 0.0)) * c_rcas.x;
    float rcpL = 1.0 / (4.0 * lobe + 1.0);
    float3 pix = (lobe * b + lobe * d + lobe * h + lobe * f + e) * rcpL;
    return float4(pix, 1);
}
