// ===========================================================================
// mount&blade.exe (MSVC 7.1 / cl 13.10, x86-32) -- frozen batch ds1, slice 263
//
// The game-loop event handler: one big frame function that drives the
// per-frame camera/sky update, the mission "show/hide" party switches, the
// per-frame scene item pass and the main-menu result dispatch.
// ===========================================================================
typedef unsigned int uint;

// ---------------------------------------------------------------------------
// the engine's generic polymorphic object: only the virtual slots this
// function calls are given real signatures (slot = byte offset / 4)
// ---------------------------------------------------------------------------
struct VObj {
    virtual void v0();  virtual void v1();  virtual void v2();  virtual void v3();
    virtual void v4();                                          // 0x10
    virtual void v5();  virtual void v6();  virtual void v7();  virtual void v8();
    virtual void v9();  virtual void v10(); virtual void v11(); virtual void v12();
    virtual void v13(); virtual void v14(); virtual void v15(); virtual void v16();
    virtual char v17(int a);                                    // 0x44
    virtual void v18(int a);                                    // 0x48
    virtual void v19(); virtual void v20(); virtual void v21();
    virtual void v22(void* p);                                  // 0x58
    virtual void v23(); virtual void v24(); virtual void v25(); virtual void v26();
    virtual void v27(); virtual void v28(); virtual void v29(); virtual void v30();
    virtual void v31(); virtual void v32(); virtual void v33(); virtual void v34();
    virtual void v35(); virtual void v36();
    virtual void v37(void* p);                                  // 0x94
    virtual void v38(); virtual void v39(); virtual void v40();
    virtual void v41(void* a, void* b);                         // 0xa4
    virtual void v42(); virtual void v43();
    virtual void v44();                                         // 0xb0
    virtual void v45();                                         // 0xb4
};

struct Mgr { void update(); };                                  // 0x4f1390
struct Dlg { void open(int a); };                               // 0x495700
struct Mgr513710 { void m(void* p); };                           // 0x513710
struct Chunk { void m_4464d0(int a); void m_4f8bc0(); };
struct Troop { void m_4630d0(int i); };
struct Big5e292c { void m_483f60(int a, int b); };

void __stdcall f_48baa0(float v);                               // 0x48baa0

#define g_mgr       (*(Mgr*)0x5d7080)
#define g_dlg       (*(Dlg*)0xa73980)
#define g_513710    (*(Mgr513710*)0x5e291c)
#define g_big       (*(Big5e292c**)0x5e292c)

// ---------------------------------------------------------------------------
// the game object (only the members this handler touches)
// ---------------------------------------------------------------------------
struct Game {
    char pad0[0x18];                                            // +0x00
    int f18;                                                    // +0x18
    char pad0b[0x30 - 0x1c];
    char* f30;                                                  // +0x30 mission object
    char pad1[0x188 - 0x34];
    char* f188;                                                 // +0x188
    char pad2[0x190 - 0x18c];
    VObj* arr190[0x400];                                        // +0x190
    int f1190;                                                  // +0x1190
    char pad3[0x11e0 - 0x1194];
    VObj* f11e0;                                                // +0x11e0
    VObj* f11e4;                                                // +0x11e4
    VObj* f11e8;                                                // +0x11e8
    VObj* f11ec[5];                                             // +0x11ec
    int f1200;                                                  // +0x1200
    int f1204;                                                  // +0x1204
    void m_4f73c0(int i, int* out);                             // 0x4f73c0
    void m_5257f0();                                            // 0x5257f0
    void handle_event(int code);                                // 0x525160
};

// ===========================================================================
// @ 0x525160
void Game::handle_event(int code)
{
    g_mgr.update();

    if (*(int*)0x5d8790 != 0)
    {
        // camera/sky interpolation over the current site's passage table
        char* o = *(char**)(f188 + 0x484);
        float* pa = (float*)(o + *(int*)(o + 0x178) * 4 + 0xf0);
        float v = (*(float*)(o + 0x168) - *(float*)(o + 0x16c))
                / (*(float*)(o + 0x170) - *(float*)(o + 0x16c))
                * (*pa - *(float*)(o + 0x17c))
                + (float)*(int*)0x5d8790 / *pa * *(float*)(o + 0x17c)
                  * *(float*)0x590d7c;
        f_48baa0(v);
        ((VObj*)o)->v18(0x10000);
    }

    char* b = *(char**)0x5e2930;
    if (f1204 == *(int*)(b + 0x8c) || *(int*)(b + 0x1a23c) == 0)
        goto hide;
    if (*(float*)0x5d7094 >= *(float*)(f30 + 0xf0))
        goto hide;
    if (*(float*)(f30 + 0xe4) + *(float*)0x549d7c >= *(float*)0x5d7098)
        goto hide;
    {
        f11e0->v45();
        int s = *(int*)(b + 0x1a238);
        if (s < -0x65 || s > 0x65)
        {
            f11e8->v44();
            f11e4->v44();
        }
        else
        {
            float pair[2];
            pair[0] = (float)(s + 0x64) * *(float*)0x59c268 + *(float*)0x590cd0;
            pair[1] = 0.02f;
            f11e4->v22(pair);
            f11e8->v45();
            f11e4->v45();
        }
    }
    goto shown;
hide:
    f11e0->v44();
shown:

    for (int i = 0; i < 5; ++i)
    {
        b = *(char**)0x5e2930;
        if (*(char*)(b + 0x1a290 + i) == 0)
            continue;
        int l1;
        int l2;
        f11ec[i]->v41((void*)(b + 0x1a240 + i * 0x10), &l1);
        f11ec[i]->v37(&l2);
        *(char*)(b + 0x1a290 + i) = 0;
    }

    *(char*)(g_big + 0x6554) = 1;

    int res = -1;
    if (*(int*)0x8ed50dc >= 0)
    {
        *(int*)(*(char**)((void**)*(void***)((char*)g_big + 0x1c))[*(int*)0x8ed50dc >> 4] + 0x31c)
            = *(int*)((char*)g_big + 0x5c);
        *(int*)(*(char**)((void**)*(void***)((char*)g_big + 0x1c))[*(int*)0x8ed50dc >> 4] + 0x533c) = 2;
        *(int*)(*(char**)((void**)*(void***)((char*)g_big + 0x1c))[*(int*)0x8ed50dc >> 4] + 0x5338) = 0;
        ((Chunk*)*(char**)((void**)*(void***)((char*)g_big + 0x1c))[*(int*)0x8ed50dc >> 4])
            ->m_4464d0(0);
    }

    g_big->m_483f60(code, 0);

    {
        if (*(int*)((char*)g_big + 0x5c) >= 0)
        {
            ((Chunk*)*(char**)((void**)*(void***)((char*)g_big + 0x1c))[*(int*)((char*)g_big + 0x5c) >> 4])
                ->m_4f8bc0();
            *(int*)(*(char**)((void**)*(void***)((char*)g_big + 0x1c))[*(int*)((char*)g_big + 0x5c) >> 4] + 0x55d0) = 0;
        }
    }

    if (f1190 > 1)
    {
        int i;
        for (i = 0; i < 0x400; ++i)
        {
            if (!arr190[i]->v17(0x10000))
                continue;
            if (i >= f1190)
                continue;
            m_4f73c0(i, &res);
        }
    }
    else if (*(int*)0xa17454 == 0 && *(int*)0xa17854 != 0)
    {
        m_4f73c0(-2, &res);
    }

    m_5257f0();

    int v = res;

    if (v == 1)
        goto case1;
    if (*(int*)0x5a9fb0 != 0 && *(int*)0xa17110 != 0 && *(int*)0xa17510 == 0)
        goto case1;

    if (v == 0x10)
    {
        char* t = *(char**)0xa739e8;
        *(int*)(t + 0x18) = 0x10;
        g_dlg.open(0x1a);
        return;
    }

    if (v == 4)
    {
        char* b2 = *(char**)0x5e2930;
        char* rec = *(char**)(b2 + 0x13a00) + *(int*)0x8ed50d4 * 0x508;
        int save = *(int*)(rec + 0x34);
        int flags = *(int*)(rec + 0x30);
        (void)save;
        if (!(flags & 0x10) || (flags & 0x1000))
        {
            for (int i = 0; i < 0xa; ++i)
                ((Troop*)rec)->m_4630d0(i);
        }
        char* t = *(char**)0xa739a0;
        *(int*)(t + 0x18) = 0x10;
        *(int*)(t + 0x14) = *(int*)0x8ed50d4;
        *(int*)(t + 4) = 2;
        g_dlg.open(8);
        return;
    }

    if (v == 3)
        f1200 = *(int*)0x8ed50d4;
    if (v == 2 || v == 3)
    {
        char* t = *(char**)0xa739a0;
        *(int*)(t + 0x18) = 0x10;
        int a = *(int*)(*(char**)0x5e2930 + 0x13af0);
        if (a == 0)
            a = *(int*)0x8ed50d4;
        *(int*)(t + 0x14) = a;
        *(int*)(t + 4) = ((v != 3) ? 0 : 3) + 1;
        g_dlg.open(8);
        return;
    }

    if (v == 5 || v == 7 || v == 6 || v == 9 || v == 0xa)
    {
        char* t = *(char**)0xa739b0;
        *(int*)(t + 0x18) = 0x10;
        if (v == 5 || v == 6 || v == 7)
        {
            *(int*)(t + 0x10) = *(int*)0x8ed50d8;
        }
        else
        {
            *(int*)(t + 0x10) = -1;
            *(int*)(t + 0x14) = *(int*)0x8ed50d4;
        }
        if (v == 5)
        {
            *(int*)(t + 4) = 1;
            g_dlg.open(0xc);
            return;
        }
        if (v == 7)
        {
            *(int*)(t + 4) = 7;
            g_dlg.open(0xc);
            return;
        }
        if (v == 6)
        {
            *(int*)(t + 4) = 6;
            g_dlg.open(0xc);
            return;
        }
        if (v == 9)
        {
            *(int*)(t + 4) = 2;
            g_dlg.open(0xc);
            return;
        }
        *(int*)(t + 4) = 3;
        g_dlg.open(0xc);
        return;
    }

    if (v == 0xb || v == 0xc)
    {
        char* t = *(char**)0xa739bc;
        *(int*)(t + 0x18) = 0x10;
        *(int*)(t + 0x14) = *(int*)0x8ed50d4;
        *(int*)(t + 4) = (v != 0xb) + 1;
        ((VObj*)t)->v4();
        g_dlg.open(0xf);
        return;
    }

    return;

case1:
    {
        int x = f18;
        if (x == 0x64)
            g_513710.m(&code);
        else
            g_dlg.open(x);
    }
}
