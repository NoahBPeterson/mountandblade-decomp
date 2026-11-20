// port/gfx/shaders/ffp_ps.hlsl: the Direct3D 9 texture stage cascade (D3DTSS_COLOROP / ALPHAOP and their arguments,
// D3DTA_* with COMPLEMENT / ALPHAREPLICATE, TEMP result, texture factor, per-stage constants), then the specular add.
// Four stages (the game uses at most three). A stage whose texture is unbound reads (1,1,1,1) as its texture.
// C mirror of the block: FfpPsBlock in ffp.cpp.

[[vk::binding(0, 3)]] cbuffer FfpPs {
    float4 ffp_cop[4];       // colour: op, arg1, arg2, arg0
    float4 ffp_aop[4];       // alpha:  op, arg1, arg2, arg0
    float4 ffp_smisc[4];     // x texture bound, y result to TEMP
    float4 ffp_sconst[4];    // D3DTSS_CONSTANT
    float4 ffp_tfactor;
    float4 ffp_pflags;       // x specular enable
};

[[vk::binding(0, 2)]] sampler2D ffp_s0;
[[vk::binding(1, 2)]] sampler2D ffp_s1;
[[vk::binding(2, 2)]] sampler2D ffp_s2;
[[vk::binding(3, 2)]] sampler2D ffp_s3;

float4 ffp_arg(int a, float4 diffuse, float4 cur, float4 tex, float4 temp, float4 spec, float4 cst)
{
    int sel = a & 15;
    float4 r = diffuse;
    if (sel == 1) r = cur;
    else if (sel == 2) r = tex;
    else if (sel == 3) r = ffp_tfactor;
    else if (sel == 4) r = spec;
    else if (sel == 5) r = temp;
    else if (sel == 6) r = cst;
    if (a & 16) r = 1.0 - r;
    if (a & 32) r = r.aaaa;
    return r;
}

float4 ffp_op(int op, float4 a1, float4 a2, float4 a0, float4 diffuse, float4 tex, float4 cur)
{
    if (op == 2) return a1;
    if (op == 3) return a2;
    if (op == 4) return a1 * a2;
    if (op == 5) return a1 * a2 * 2.0;
    if (op == 6) return a1 * a2 * 4.0;
    if (op == 7) return a1 + a2;
    if (op == 8) return a1 + a2 - 0.5;
    if (op == 9) return (a1 + a2 - 0.5) * 2.0;
    if (op == 10) return a1 - a2;
    if (op == 11) return a1 + a2 - a1 * a2;
    if (op == 12) return lerp(a2, a1, diffuse.a);
    if (op == 13) return lerp(a2, a1, tex.a);
    if (op == 14) return lerp(a2, a1, ffp_tfactor.a);
    if (op == 15) return a1 + a2 * (1.0 - tex.a);
    if (op == 16) return lerp(a2, a1, cur.a);
    if (op == 17) return a1;
    if (op == 18) return float4(a1.rgb + a1.a * a2.rgb, a1.a);
    if (op == 19) return float4(a1.rgb * a2.rgb + a1.a, a1.a);
    if (op == 20) return float4((1.0 - a1.a) * a2.rgb + a1.rgb, a1.a);
    if (op == 21) return float4((1.0 - a1.rgb) * a2.rgb + a1.a, a1.a);
    if (op == 24) return saturate(4.0 * dot(a1.rgb - 0.5, a2.rgb - 0.5)).xxxx;
    if (op == 25) return a0 + a1 * a2;
    if (op == 26) return lerp(a2, a1, a0);
    return cur;   // 22 / 23 (bump env map): pass through
}

float4 mbp_ps(MBP_VSOUT i) : SV_Target0
{
    float4 diffuse = i.v0, spec = i.v1;
    float4 cur = diffuse, temp = float4(0, 0, 0, 0);
    float4 tcs[4] = {i.t0, i.t1, i.t2, i.t3};
    [unroll] for (int s = 0; s < 4; s++) {
        int cop = (int)ffp_cop[s].x;
        if (cop == 1) break;
        float4 tex = float4(1, 1, 1, 1);
        if (ffp_smisc[s].x > 0.5) {
            if (s == 0) tex = tex2D(ffp_s0, tcs[s].xy);
            else if (s == 1) tex = tex2D(ffp_s1, tcs[s].xy);
            else if (s == 2) tex = tex2D(ffp_s2, tcs[s].xy);
            else tex = tex2D(ffp_s3, tcs[s].xy);
        }
        float4 cst = ffp_sconst[s];
        float4 c1 = ffp_arg((int)ffp_cop[s].y, diffuse, cur, tex, temp, spec, cst);
        float4 c2 = ffp_arg((int)ffp_cop[s].z, diffuse, cur, tex, temp, spec, cst);
        float4 c0 = ffp_arg((int)ffp_cop[s].w, diffuse, cur, tex, temp, spec, cst);
        float3 rgb = saturate(ffp_op(cop, c1, c2, c0, diffuse, tex, cur)).rgb;
        int aop = (int)ffp_aop[s].x;
        float alpha = cur.a;
        if (aop != 1) {
            float4 x1 = ffp_arg((int)ffp_aop[s].y, diffuse, cur, tex, temp, spec, cst);
            float4 x2 = ffp_arg((int)ffp_aop[s].z, diffuse, cur, tex, temp, spec, cst);
            float4 x0 = ffp_arg((int)ffp_aop[s].w, diffuse, cur, tex, temp, spec, cst);
            alpha = saturate(ffp_op(aop, x1, x2, x0, diffuse, tex, cur)).a;
        }
        if (cop == 24) alpha = rgb.r;    // DOTPRODUCT3 replicates into alpha
        if (ffp_smisc[s].y > 0.5) temp = float4(rgb, alpha);
        else cur = float4(rgb, alpha);
    }
    if (ffp_pflags.x > 0.5) cur.rgb = saturate(cur.rgb + spec.rgb);
    return mbp_ps_finish(cur, i);
}
