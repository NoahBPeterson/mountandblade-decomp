// Slice s00472dc0: the Newton physics set's per-frame step (PhysSet::update, 0x472dc0), the per-probe light accumulation
// of the scene (0x472ed0), the big "reset" of the 0x5928-byte agent record (0x472f90) and a per-troop-record copy
// helper (0x473570). All names provisional; PhysSet / PhysRec layouts are shared with s004413e0 / s0044f3c0.
#include <vector>
#include "rgl.h"
using rgl::vector4;

struct NewtonWorld; struct NewtonBody; struct NewtonRagDollBone;
extern "C" {
__declspec(dllimport) void __cdecl NewtonUpdate(const NewtonWorld* w, float dt);                                 // 0x549204
}

struct PhysRec {                                  // 0x38 bytes
    int f0;
    const NewtonWorld* world;                     // +4
    char pad8[4];
    std::vector<const NewtonRagDollBone*> bones;  // +0xc (first at +0x10, last at +0x14)
    std::vector<const NewtonBody*> bodies;        // +0x1c (first at +0x20, last at +0x24)
    char pad2c[0x38 - 0x2c];
};
struct PhysSet {
    std::vector<PhysRec> recs;                    // +0 (first at +4)
    char pad10[0x3c - 0x10];
    int nworlds;                                  // +0x3c
    void clamp_rec(int i);                        // 0x472c50 (thiscall, ret 4)
    void update(float dt);                        // 0x472dc0
};

// @ 0x472dc0
void PhysSet::update(float dt)
{
    float step = 0.018f;
    if (nworlds > 1) {
        step = 0.021f;
        if (nworlds > 2) {
            step = 0.024f;
            if (nworlds > 3)
                step = 0.026f;
        }
    }
    if (dt > 0.075f)
        step *= 1.5f;
    while (dt > 0.0001f) {
        float s;
        if (dt < step)
            s = dt;
        else
            s = step;
        dt -= s;
        for (unsigned i = 0; i < recs.size(); i++) {
            if (recs[i].world) {
                clamp_rec(i);
                NewtonUpdate(recs[i].world, s);
            }
        }
    }
}

// ---- scene light probes: 0x24-byte records (position, normal, accumulated brightness) --------------------------------
struct LightProbe {
    vector4 p;                                     // +0
    vector4 n;                                     // +0x10
    float level;                                   // +0x20
};
struct ProbeSet { int pad0; LightProbe* first; LightProbe* last; };
struct Scene {
    char pad0[0xb0];
    ProbeSet* probes;                              // +0xb0
    void light_at(vector4 p, vector4 n, vector4* out_dir, vector4* out_col, unsigned flags, float scale);   // 0x43ac40 (thiscall, ret 0x30)
    void update_probes(unsigned flags);            // 0x472ed0
};
// @ 0x472ed0
void Scene::update_probes(unsigned flags)
{
    for (LightProbe* pr = probes->first; pr < probes->last; pr++) {
        if (pr->level < -0.1f) {
            vector4 dir, col;
            light_at(pr->p, pr->n, &dir, &col, flags, 1.0f);
            pr->level = (col.x + col.y + col.z) * 0.33f;
        }
    }
}

// ---- 0x472f90: reset of the agent record ---------------------------------------------------------------------------------
struct ViewState {
    void init();                                   // 0x463370
    char body[0x5374];
};
extern __int64 g_ticks[];                          // 0x5e2948 (100 kHz clocks)
struct Rec2c {                                     // 0x2c-byte sub-record (provisional), two of them in the agent
    int a, b, c, d, e, f, g;
    float h, i, j;
    int k;
    void clear()
    {
        b = -1; a = -1; c = -1; d = 0; e = -1; f = 0; g = 0;
        h = 1.0f; i = 1.0f; j = -1.0f; k = 100;
    }
};
struct AgentReset {
    char pad0[0x14];
    int f14;   // +0x14
    int f18;   // +0x18
    char pad1c[0x4];
    int* f20;   // +0x20
    int f24;   // +0x24
    int f28;   // +0x28
    int f2c;   // +0x2c
    int f30;   // +0x30
    int f34;   // +0x34
    int f38;   // +0x38
    int f3c;   // +0x3c
    int f40;   // +0x40
    char pad44[0x4];
    __int64 f48;   // +0x48
    vector4 f50;   // +0x50
    vector4 f60;   // +0x60
    int f70;   // +0x70
    int f74;   // +0x74
    float f78;   // +0x78
    int f7c;   // +0x7c
    float f80;   // +0x80
    int f84;   // +0x84
    int f88;   // +0x88
    char pad8c[0x8];
    int f94;   // +0x94
    vector4 f98;   // +0x98
    int fa8;   // +0xa8
    int fac;   // +0xac
    float fb0;   // +0xb0
    int fb4;   // +0xb4
    int fb8;   // +0xb8
    int fbc;   // +0xbc
    int fc0;   // +0xc0
    vector4 fc4;   // +0xc4
    int fd4;   // +0xd4
    int fd8;   // +0xd8
    int fdc;   // +0xdc
    int fe0;   // +0xe0
    int fe4;   // +0xe4
    int fe8;   // +0xe8
    int fec;   // +0xec
    float ff0;   // +0xf0
    float ff4;   // +0xf4
    float ff8;   // +0xf8
    int ffc;   // +0xfc
    int f100;   // +0x100
    int f104;   // +0x104
    int f108;   // +0x108
    int f10c;   // +0x10c
    int f110;   // +0x110
    int f114;   // +0x114
    int f118;   // +0x118
    float f11c;   // +0x11c
    float f120;   // +0x120
    float f124;   // +0x124
    int f128;   // +0x128
    int f12c;   // +0x12c
    int f130;   // +0x130
    int f134;   // +0x134
    int f138;   // +0x138
    int f13c;   // +0x13c
    int f140;   // +0x140
    int f144;   // +0x144
    int f148;   // +0x148
    int f14c;   // +0x14c
    int f150;   // +0x150
    int f154;   // +0x154
    int f158;   // +0x158
    int f15c;   // +0x15c
    int f160;   // +0x160
    int f164;   // +0x164
    int f168;   // +0x168
    int f16c;   // +0x16c
    int f170;   // +0x170
    int f174;   // +0x174
    int f178;   // +0x178
    int f17c;   // +0x17c
    int f180;   // +0x180
    int f184;   // +0x184
    int f188;   // +0x188
    int f18c;   // +0x18c
    int f190;   // +0x190
    char pad194[0x10];
    int f1a4;   // +0x1a4
    int f1a8;   // +0x1a8
    int f1ac;   // +0x1ac
    int f1b0;   // +0x1b0
    int f1b4;   // +0x1b4
    int f1b8;   // +0x1b8
    int f1bc;   // +0x1bc
    int f1c0;   // +0x1c0
    char pad1c4[0x68];
    int f22c;   // +0x22c
    int f230;   // +0x230
    int f234;   // +0x234
    int f238;   // +0x238
    char pad23c[0x14];
    int f250;   // +0x250
    char pad254[0x4];
    ViewState view;   // +0x258
    char pad55cc[0x4];
    int f55d0;   // +0x55d0
    char pad55d4[0x4];
    vector4 f55d8;   // +0x55d8
    int f55e8;   // +0x55e8
    char pad55ec[0x20];
    int f560c;   // +0x560c
    int f5610;   // +0x5610
    int f5614;   // +0x5614
    char pad5618[0xc];
    vector4 f5624;   // +0x5624
    int f5634;   // +0x5634
    int f5638;   // +0x5638
    char pad563c[0x68];
    vector4 f56a4;   // +0x56a4
    vector4 f56b4;   // +0x56b4
    int f56c4;   // +0x56c4
    int f56c8;   // +0x56c8
    int f56cc;   // +0x56cc
    int f56d0;   // +0x56d0
    int f56d4;   // +0x56d4
    int f56d8;   // +0x56d8
    int f56dc;   // +0x56dc
    float f56e0;   // +0x56e0
    float f56e4;   // +0x56e4
    char pad56e8[0x4];
    int f56ec;   // +0x56ec
    int f56f0;   // +0x56f0
    char pad56f4[0x20];
    int f5714;   // +0x5714
    int f5718;   // +0x5718
    char pad571c[0xc];
    int f5728;   // +0x5728
    int f572c;   // +0x572c
    int f5730;   // +0x5730
    char pad5734[0x80];
    float f57b4;   // +0x57b4
    char pad57b8[0x20];
    char f57d8;   // +0x57d8
    char pad57d9[0x7];
    int f57e0;   // +0x57e0
    char pad57e4[0x4];
    __int64 f57e8;   // +0x57e8
    int f57f0;   // +0x57f0
    char pad57f4[0x4];
    __int64 f57f8;   // +0x57f8
    int f5800;   // +0x5800
    char pad5804[0x4];
    __int64 f5808;   // +0x5808
    int f5810;   // +0x5810
    char pad5814[0x4];
    __int64 f5818;   // +0x5818
    int f5820;   // +0x5820
    char pad5824[0x4];
    __int64 f5828;   // +0x5828
    int f5830;   // +0x5830
    char f5834;   // +0x5834
    char pad5835[0x3];
    int f5838;   // +0x5838
    int f583c;   // +0x583c
    char pad5840[0x4];
    int f5844;   // +0x5844
    char pad5848[0x8];
    float f5850;   // +0x5850
    char pad5854[0x4];
    int f5858;   // +0x5858
    char pad585c[0x4];
    int f5860;   // +0x5860
    char pad5864[0x4];
    __int64 f5868;   // +0x5868
    char pad5870[0x4];
    int f5874;   // +0x5874
    char f5878;   // +0x5878
    char pad5879[0x3];
    int f587c;   // +0x587c
    int f5880;   // +0x5880
    char pad5884[0x40];
    char f58c4;   // +0x58c4
    char f58c5;   // +0x58c5
    char pad58c6[0x12];
    char f58d8;   // +0x58d8
    char pad58d9[0x3];
    int f58dc;   // +0x58dc
    vector4 f58e0;   // +0x58e0
    vector4 f58f0;   // +0x58f0
    int f5900;   // +0x5900
    char pad5904[0x14];
    int f5918;   // +0x5918
    char pad591c[0x4];
    __int64 f5920;   // +0x5920
    char pad5928[0x5928 - 0x5928];
    void reset();                                  // 0x472f90
};
// @ 0x472f90
void AgentReset::reset()
{
    fd8 = -1;
    fd4 = -1;
    fdc = -1;
    fe0 = 0;
    fe4 = -1;
    fe8 = 0;
    fec = 0;
    ff0 = 1.0f;
    ff4 = 1.0f;
    ff8 = -1.0f;
    ffc = 100;
    f104 = -1;
    f100 = -1;
    f108 = -1;
    f10c = 0;
    f110 = -1;
    f114 = 0;
    f118 = 0;
    f11c = 1.0f;
    f120 = 1.0f;
    f124 = -1.0f;
    f128 = 100;
    view.init();
    f50 = vector4(0.0f, 0.0f, 0.0f);
    f55d8 = vector4(0.0f, 1.0f, 0.0f);
    f60 = vector4(0.0f, 1.0f, 0.0f);
    f14 = 0;
    f18 = 0;
    f88 = -1;
    f34 = 1;
    f38 = 9;
    f55d0 = 0;
    f70 = 0;
    f74 = 0;
    f78 = 0.15f;
    fa8 = 0;
    fac = 0;
    f58e0 = vector4(-10000.0f, -100000.0f, -100000.0f);
    f98 = vector4(0.0f, 0.0f, 0.0f);
    f5624 = vector4(0.0f, 0.0f, 0.0f);
    f56a4 = vector4(0.0f, 0.0f, 0.0f);
    fb0 = 1.0f;
    f250 = 0;
    fbc = 0;
    f84 = 0;
    f5880 = 0;
    f58c4 = 1;
    f58c5 = 1;
    f58d8 = 1;
    f58dc = -1000;
    f5838 = -1;
    f583c = 0;
    f80 = 1.0f;
    f1bc = -1;
    f1c0 = -1;
    f94 = -1;
    f5878 = 1;
    f7c = 0;
    f12c = 0;
    f138 = -1;
    f134 = -1;
    f2c = -1;
    f30 = -1;
    f130 = 0;
    f56c4 = 0;
    f56c8 = 0;
    f56cc = 0;
    f560c = 0;
    f5610 = 0;
    f5614 = -1;
    f5634 = 0;
    f5638 = 0;
    fe0 = -1;
    f10c = 0;
    f56d0 = 0;
    f3c = 1;
    fb8 = 1;
    fb4 = 0;
    f5858 = -1;
    f56b4 = vector4(0.0f, 0.0f, 0.0f);
    fc4 = vector4(100000.0f, 10000.0f, 100000.0f);
    f13c = 0; f140 = 0; f144 = 0; f148 = 0; f14c = 0; f150 = 0; f154 = 0; f158 = 0;
    f15c = 0; f160 = 0; f164 = 0; f168 = 0; f16c = 0; f170 = 0; f174 = 0; f178 = 0;
    f17c = 0; f180 = 0; f184 = 0; f188 = 0; f18c = 0; f190 = 0;
    f1a4 = 0;
    f1a8 = 0;
    f5714 = 0;
    f56f0 = 0;
    f5718 = -1;
    f22c = -1;
    f230 = -1;
    f234 = -1;
    f238 = -1;
    f56d8 = 0;
    f56dc = 0;
    f56e0 = 1.0f;
    f56e4 = 1.0f;
    f56d4 = 0;
    f5874 = 0;
    f5730 = 0;
    f1ac = 0;
    f1b0 = 0;
    f1b4 = 0;
    f1b8 = 0;
    f5830 = 2;
    f5844 = 0;
    f5850 = 90000.0f;
    if (f20)
        delete f20;
    f20 = 0;
    f24 = 0;
    f28 = 0;
    f587c = -1;
    f57b4 = 1.0f;
    fc0 = 0;
    f57d8 = 0;
    f57e8 = g_ticks[f57e0];
    f57f8 = g_ticks[f57f0];
    f5808 = g_ticks[f5800];
    f5828 = g_ticks[f5820];
    f48 = g_ticks[f40];
    f5868 = g_ticks[f5860];
    f5920 = g_ticks[f5918];
    f5818 = g_ticks[f5810];
    f58f0 = vector4(0.0f, 0.0f, 0.0f);
    f55e8 = 0;
    f5834 = 0;
    f5900 = 0;
    f56ec = 0;
    f5728 = 0;
    f572c = 0;
}

// ---- 0x473570 ----------------------------------------------------------------------------------------------------------------
struct tstring {
    int pos; char* buf; int cap; int len;
    void operator=(const tstring& o);              // 0x42bc50
};
struct TroopRec {                                  // reduced
    char pad0[0x34];
    std::vector<int> groups[5];                    // +0x34 (first at +0x38)
    int ngroups;                                   // +0x84
    char pad88[0xd0 - 0x88];
    tstring name;                                  // +0xd0
};
struct TroopMgr {
    void apply_group(tstring* dst, int idx, int first);   // 0x470db0 (thiscall, ret 0xc)
    void copy_troop(tstring* dst, const TroopRec* r);     // 0x473570
};
// @ 0x473570
void TroopMgr::copy_troop(tstring* dst, const TroopRec* r)
{
    *dst = r->name;
    for (int i = 0; i < r->ngroups; i++) {
        if (r->groups[i].size() != 0)
            apply_group(dst, i, r->groups[i][0]);
    }
}
