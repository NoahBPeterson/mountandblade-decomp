// Slice s00408e60: debug polygon outline drawing.
#include "rgl.h"
// Minimal IDirect3DDevice9 vtable stub (the VC7.1 toolchain's d3d9.h needs sal.h, which is missing).
struct IDirect3DDevice9 {
    virtual long __stdcall slot0() = 0;
    virtual long __stdcall slot1() = 0;
    virtual long __stdcall slot2() = 0;
    virtual long __stdcall slot3() = 0;
    virtual long __stdcall slot4() = 0;
    virtual long __stdcall slot5() = 0;
    virtual long __stdcall slot6() = 0;
    virtual long __stdcall slot7() = 0;
    virtual long __stdcall slot8() = 0;
    virtual long __stdcall slot9() = 0;
    virtual long __stdcall slot10() = 0;
    virtual long __stdcall slot11() = 0;
    virtual long __stdcall slot12() = 0;
    virtual long __stdcall slot13() = 0;
    virtual long __stdcall slot14() = 0;
    virtual long __stdcall slot15() = 0;
    virtual long __stdcall slot16() = 0;
    virtual long __stdcall slot17() = 0;
    virtual long __stdcall slot18() = 0;
    virtual long __stdcall slot19() = 0;
    virtual long __stdcall slot20() = 0;
    virtual long __stdcall slot21() = 0;
    virtual long __stdcall slot22() = 0;
    virtual long __stdcall slot23() = 0;
    virtual long __stdcall slot24() = 0;
    virtual long __stdcall slot25() = 0;
    virtual long __stdcall slot26() = 0;
    virtual long __stdcall slot27() = 0;
    virtual long __stdcall slot28() = 0;
    virtual long __stdcall slot29() = 0;
    virtual long __stdcall slot30() = 0;
    virtual long __stdcall slot31() = 0;
    virtual long __stdcall slot32() = 0;
    virtual long __stdcall slot33() = 0;
    virtual long __stdcall slot34() = 0;
    virtual long __stdcall slot35() = 0;
    virtual long __stdcall slot36() = 0;
    virtual long __stdcall slot37() = 0;
    virtual long __stdcall slot38() = 0;
    virtual long __stdcall slot39() = 0;
    virtual long __stdcall slot40() = 0;
    virtual long __stdcall slot41() = 0;
    virtual long __stdcall slot42() = 0;
    virtual long __stdcall slot43() = 0;
    virtual long __stdcall slot44() = 0;
    virtual long __stdcall slot45() = 0;
    virtual long __stdcall slot46() = 0;
    virtual long __stdcall slot47() = 0;
    virtual long __stdcall slot48() = 0;
    virtual long __stdcall slot49() = 0;
    virtual long __stdcall slot50() = 0;
    virtual long __stdcall slot51() = 0;
    virtual long __stdcall slot52() = 0;
    virtual long __stdcall slot53() = 0;
    virtual long __stdcall slot54() = 0;
    virtual long __stdcall slot55() = 0;
    virtual long __stdcall slot56() = 0;
    virtual long __stdcall SetRenderState(unsigned state, unsigned value) = 0;      // 57
    virtual long __stdcall slot58() = 0;
    virtual long __stdcall slot59() = 0;
    virtual long __stdcall slot60() = 0;
    virtual long __stdcall slot61() = 0;
    virtual long __stdcall slot62() = 0;
    virtual long __stdcall slot63() = 0;
    virtual long __stdcall slot64() = 0;
    virtual long __stdcall SetTexture(unsigned stage, void* tex) = 0;             // 65
    virtual long __stdcall slot66() = 0;
    virtual long __stdcall slot67() = 0;
    virtual long __stdcall slot68() = 0;
    virtual long __stdcall slot69() = 0;
    virtual long __stdcall slot70() = 0;
    virtual long __stdcall slot71() = 0;
    virtual long __stdcall slot72() = 0;
    virtual long __stdcall slot73() = 0;
    virtual long __stdcall slot74() = 0;
    virtual long __stdcall slot75() = 0;
    virtual long __stdcall slot76() = 0;
    virtual long __stdcall slot77() = 0;
    virtual long __stdcall slot78() = 0;
    virtual long __stdcall slot79() = 0;
    virtual long __stdcall slot80() = 0;
    virtual long __stdcall slot81() = 0;
    virtual long __stdcall slot82() = 0;
    virtual long __stdcall DrawPrimitiveUP(unsigned type, unsigned count, const void* data, unsigned stride) = 0; // 83
    virtual long __stdcall slot84() = 0;
    virtual long __stdcall slot85() = 0;
    virtual long __stdcall slot86() = 0;
    virtual long __stdcall slot87() = 0;
    virtual long __stdcall slot88() = 0;
    virtual long __stdcall slot89() = 0;
    virtual long __stdcall slot90() = 0;
    virtual long __stdcall slot91() = 0;
    virtual long __stdcall SetVertexShader(void* shader) = 0;                    // 92
};
enum { D3DRS_ZWRITEENABLE = 14, D3DRS_ZFUNC = 23, D3DRS_FOGENABLE = 28, D3DRS_LIGHTING = 137, D3DRS_AMBIENT = 139, D3DPT_LINELIST = 2 };
using namespace rgl;

// renderer (provisional): the D3D9 device pointer lives at +0x618.
struct renderer {
    char pad[0x618];
    IDirect3DDevice9* device;
    void draw_polygon_outline(const vector4* pts, int n);
};

// @ 0x408e60
// Draws the outline of a convex polygon (at most 4 points), shrunk 1% towards its centroid, as a line list.
void renderer::draw_polygon_outline(const vector4* pts, int n)
{
    vector4 shrunk[4];
    float lines[8][3];
    vector4 c;
    c.x = 0.0f; c.y = 0.0f; c.z = 0.0f;
    int i;
    for (i = 0; i < n; i++) {
        c.x += pts[i].x;
        c.y += pts[i].y;
        c.z += pts[i].z;
    }
    float inv = 1.0f / (float)n;
    c.x = c.x * inv;
    c.y = c.y * inv;
    c.z = c.z * inv;
    for (i = 0; i < n; i++) {
        vector4 t((pts[i].x - c.x) * 0.99f + c.x, (pts[i].y - c.y) * 0.99f + c.y, (pts[i].z - c.z) * 0.99f + c.z);
        shrunk[i] = t;
    }
    for (i = 0; i < n; i++) {
        int j = (i + 1) % n;
        lines[2 * i][0] = shrunk[i].x;
        lines[2 * i][1] = shrunk[i].y;
        lines[2 * i][2] = shrunk[i].z;
        lines[2 * i + 1][0] = shrunk[j].x;
        lines[2 * i + 1][1] = shrunk[j].y;
        lines[2 * i + 1][2] = shrunk[j].z;
    }
    device->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    device->SetRenderState(D3DRS_ZFUNC, 4);
    device->SetRenderState(D3DRS_FOGENABLE, 0);
    device->SetRenderState(D3DRS_LIGHTING, 0);
    device->SetTexture(0, 0);
    device->SetRenderState(D3DRS_AMBIENT, 0xffbffff2);
    device->SetVertexShader(0);
    device->DrawPrimitiveUP(D3DPT_LINELIST, n, lines, 12);
}
