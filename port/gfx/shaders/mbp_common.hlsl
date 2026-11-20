// port/gfx/shaders/mbp_common.hlsl: the interface every generated shader of the gfx lane shares.
//   - the varying block MBP_VSOUT (locations fixed: COLOR0, COLOR1, TEXCOORD0..7, fog factor, clip distance), written in
//     full by every vertex shader so any pixel shader can pair with any vertex shader (D3D9 matches by semantic);
//   - the device blocks (uniform slot 1 of each stage): D3D9 rasterisation conventions (half-pixel offset), the user clip
//     plane, and the fixed-function stages that follow the pixel shader in D3D9 (alpha test, fog blend).
// C mirrors: VsDevBlock / PsDevBlock in gfx_internal.h.

struct MBP_VSOUT {
    float4 pos : SV_Position;
    [[vk::location(0)]] float4 v0 : MBPV0;     // COLOR0
    [[vk::location(1)]] float4 v1 : MBPV1;     // COLOR1
    [[vk::location(2)]] float4 t0 : MBPT0;     // TEXCOORD0..7
    [[vk::location(3)]] float4 t1 : MBPT1;
    [[vk::location(4)]] float4 t2 : MBPT2;
    [[vk::location(5)]] float4 t3 : MBPT3;
    [[vk::location(6)]] float4 t4 : MBPT4;
    [[vk::location(7)]] float4 t5 : MBPT5;
    [[vk::location(8)]] float4 t6 : MBPT6;
    [[vk::location(9)]] float4 t7 : MBPT7;
    [[vk::location(10)]] float fog : MBPFOG;   // D3D9 fog factor (1 = no fog)
    [[vk::location(11)]] float clipd : MBPCLIP; // user clip plane distance (< 0 = clipped)
};

#ifdef MBP_VERTEX
[[vk::binding(1, 1)]] cbuffer MbpVsDev {
    float4 mbp_halfpix;     // x 1/vpW, y -1/vpH, z 2/vpW, w -2/vpH
    float4 mbp_vp;          // X, Y, MinZ, MaxZ-MinZ
    float4 mbp_clip_plane;
    float4 mbp_vsflags;     // x clip plane enabled
};

// clip distance, then D3D9's pixel-centre convention (pixel centres on integer coordinates): shift by half a pixel
void mbp_vs_finish(inout MBP_VSOUT o)
{
    o.clipd = mbp_vsflags.x > 0.5 ? dot(o.pos, mbp_clip_plane) : 1.0;
    o.pos.xy += mbp_halfpix.xy * o.pos.w;
}
#endif

#ifdef MBP_PIXEL
[[vk::binding(1, 3)]] cbuffer MbpPsDev {
    float4 mbp_fog_color;
    float4 mbp_alpha;       // x alpha func (0 = test off), y alpha ref (0..255), z fog enabled
};

bool mbp_cmp(int f, float a, float r)
{
    if (f == 1) return false;
    if (f == 2) return a < r;
    if (f == 3) return a == r;
    if (f == 4) return a <= r;
    if (f == 5) return a > r;
    if (f == 6) return a != r;
    if (f == 7) return a >= r;
    return true;
}

// the fixed stages after the pixel shader: user clip plane, alpha test, fog blend
float4 mbp_ps_finish(float4 c, MBP_VSOUT i)
{
    if (i.clipd < 0.0) discard;
    int f = (int)mbp_alpha.x;
    if (f != 0) {
        float a = floor(saturate(c.a) * 255.0 + 0.5);
        if (!mbp_cmp(f, a, mbp_alpha.y)) discard;
    }
    if (mbp_alpha.z > 0.5) c.rgb = lerp(mbp_fog_color.rgb, c.rgb, saturate(i.fog));
    return c;
}
#endif
