// Slice s0051cfa0: the Game constructor and the world/nav containers it owns.
//   0x51cfa0  Game::Game()                 -- build the 0x1a2b0 object, then reset()
//   0x51d1c0  Game::alloc(count)           -- reserve the 0x5c-byte record vector (at +0x13a60)
//   0x51d220  Game::ctor_a()               -- construct the container at game+0xcc
//   0x51d250  Game::ctor_b()               -- construct the container at game+0xf8 (0x2717 id slots)
//   0x51d2b0  Game::init_map()             -- build the scene/nav graph once
//   0x51d3a0  Game::solve(...)             -- worklist shortest-path over the 0x88-byte nodes
//   0x51d7c0  Game::walk(...)              -- recursive neighbour walk used by solve()
// Offsets and names are provisional.
#include <windows.h>
#include <new>

// ---- rgl string (16 bytes) ---------------------------------------------------------------------------
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    tstring(const tstring& o);                          // 0x42c110
    ~tstring();                                         // 0x42bc40
    tstring& operator=(const tstring& o);               // 0x42bc50
    tstring& assign(const char* s);                     // 0x42bcb0
    void clear();                                       // 0x42be50
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// ---- the game object (0x5e2930, 0x1a2b0 bytes) -------------------------------------------------------
struct Node {                                           // 0x88 bytes, vector at game+0x14
    int count;                                          // +0x00  number of neighbours
    char pad04[0x24 - 0x04];
    int id;                                             // +0x24
    int nb[8];                                          // +0x28  neighbour ids
    int cost;                                           // +0x48
    int prev;                                           // +0x4c
    int next;                                           // +0x50
    char pad54[0x88 - 0x54];
};

struct Game {
    char data[0x1a2b0];                                 // operator new size

    Game();                                             // 0x51cfa0
    void reset();                                       // 0x51cc80 (other slice, declared only)
    __declspec(noinline) bool alloc(int count);         // 0x51d1c0
    __declspec(noinline) Game* ctor_a();                // 0x51d220
    __declspec(noinline) Game* ctor_b();                // 0x51d250
    void init_map();                                    // 0x51d2b0
    void solve(int* arg1, int* arg2, int* arg3);          // 0x51d3a0
    int* walk(int* a, int* idx, int* seen, int* n);     // 0x51d7c0
    int count() { int b = *(int*)((char*)this + 0x14); return b ? (*(int*)((char*)this + 0x18) - b) / 0x88 : 0; }
    Node* nodes() { return *(Node**)((char*)this + 0x14); }
};

// ---- helper objects -----------------------------------------------------------------------------------
struct C4638b0 { C4638b0(); };                          // 0x4638b0 (0x10-byte record ctor)
struct BuyVec  { bool buy(unsigned n); };               // 0x43e760 (std::vector<int>::_Buy)
struct XlenVec { void xlen(); };                        // 0x43e2a0 (std::vector::_Xlen)
struct Obj51dac0 { void run(int* a, int* b, int* c); }; // 0x51dac0
struct Obj4704b0 { void run(); };                       // 0x4704b0
struct Obj4fd490 { void run(int v); };                  // 0x4fd490
struct SceneCtor { int Init(int a, int b, int* c, int* d, int* e); };  // 0x478ad0
struct NavStore { void build(int world); };             // 0x479980
struct Helper51d920 { int* call(int*, int*, int*, int*, int*, int*, int*); };  // 0x51d920

double g_1e9d;                                          // 0x59be1c (1e9f stored as a dword)

void* __cdecl operator new(size_t n);
void* __cdecl operator new[](size_t n);
void  __cdecl operator delete(void* p);
void  __cdecl operator delete[](void* p);

struct CtorCall { void call(); };                        // any 0-arg thiscall ctor
#define CTOR(p) (((CtorCall*)(p))->call())

// @ 0x51cfa0
Game::Game()
{
    char* base = (char*)this;
    CTOR(base + 0x0008);
    CTOR(base + 0x001c);
    *(int*)(base + 0x0038) = 0;
    *(int*)(base + 0x003c) = 0;
    *(int*)(base + 0x0040) = 0;
    CTOR(base + 0x00b8);
    ((Game*)(base + 0x00cc))->ctor_a();
    ((Game*)(base + 0x00f8))->ctor_b();
    *(int*)(base + 0x139ec) = 0;
    *(int*)(base + 0x139f0) = 1;
    *(int*)(base + 0x139e8) = 0;
    *(int*)(base + 0x139f4) = 0;
    *(int*)(base + 0x139f8) = 0;
    *(int*)(base + 0x139fc) = 1;
    ((Game*)(base + 0x13a60))->alloc(0);
    CTOR(base + 0x13a70);
    *(int*)(base + 0x13a80) = 1;
    *(int*)(base + 0x13a88) = *(int*)0x5e2950;
    *(int*)(base + 0x13a8c) = *(int*)0x5e2954;
    *(int*)(base + 0x13a90) = 1;
    *(int*)(base + 0x13a98) = *(int*)0x5e2950;
    *(int*)(base + 0x13a9c) = *(int*)0x5e2954;
    CTOR(base + 0x13aa8);
    CTOR(base + 0x13ab8);
    *(int*)(base + 0x13b18) = 1;
    *(int*)(base + 0x13b20) = *(int*)0x5e2950;
    *(int*)(base + 0x13b24) = *(int*)0x5e2954;
    CTOR(base + 0x13b50);

    for (int i = 0; i < 4; i++)
        CTOR(base + 0x13b64 + i * 0x10);
    for (int i = 0; i < 0x32; i++)
        CTOR(base + 0x13bcc + i * 0x10);

    CTOR(base + 0x1a06c);
    CTOR(base + 0x1a080);
    *(int*)(base + 0x1a150) = 0;
    *(int*)(base + 0x1a158) = *(int*)0x5e2948;
    *(int*)(base + 0x1a15c) = *(int*)0x5e294c;
    *(int*)(base + 0x1a1bc) = 0;
    *(int*)(base + 0x1a1c0) = 0;
    *(int*)(base + 0x1a1c4) = 0;
    CTOR(base + 0x1a210);
    for (int i = 0; i < 5; i++)
        CTOR(base + 0x1a240 + i * 0x10);
    *(int*)(base + 0x1a298) = 1;
    *(int*)(base + 0x1a2a0) = *(int*)0x5e2950;
    *(int*)(base + 0x1a2a4) = *(int*)0x5e2954;

    *(char*)(base + 0x1a05c) = 0;
    ((Game*)base)->reset();
    *(int*)(base + 0x1a060) = 0;
    *(int*)(base + 0x1a064) = 0;
    *(int*)(base + 0x1a090) = 0;
    *(int*)(base + 0x1a160) = 0;
    *(int*)(base + 0x13a48) = 0;
    *(int*)(base + 0x13a00) = 0;
    *(int*)(base + 0x13a50) = 0;
    *(int*)(base + 0x13a58) = 0;
}

// @ 0x51d1c0
bool Game::alloc(int count)
{
    *(int*)((char*)this + 0x04) = 0;
    *(int*)((char*)this + 0x08) = 0;
    *(int*)((char*)this + 0x0c) = 0;
    if (count == 0)
        return false;
    if ((unsigned)count > 0x2c8590b) {
        ((XlenVec*)this)->xlen();
        return true;
    }
    struct E100 { char b[0x5c]; };
    E100* p = (E100*)operator new(count * 0x5c);
    *(E100**)((char*)this + 0x0c) = p + count;
    *(E100**)((char*)this + 0x04) = p;
    *(E100**)((char*)this + 0x08) = p;
    return true;
}

// @ 0x51d220
Game* Game::ctor_a()
{
    char* p = (char*)this;
    *(int*)(p + 0x08) = 0;
    *(int*)(p + 0x0c) = 0;
    *(int*)(p + 0x10) = 0;
    ((BuyVec*)(p + 0x14))->buy(0);
    *(int*)(p + 0x00) = 0;
    *(int*)(p + 0x24) = 0;
    *(int*)(p + 0x28) = 0;
    return this;
}

// @ 0x51d250
Game* Game::ctor_b()
{
    char* p = (char*)this;
    *(int*)(p + 0x138c0) = 0;
    *(int*)(p + 0x138c4) = 0;
    *(int*)(p + 0x138c8) = 0;
    ((Obj4fd490*)(p + 0x138cc))->run(0);
    int i = 0;
    do {
        *(int*)(p + i * 8) = -1;
        *(int*)(p + i * 8 + 4) = -1;
        i++;
    } while (i < 0x2717);
    *(int*)(p + 0x138b8) = 0;
    *(int*)(p + 0x138dc) = 0;
    *(int*)(p + 0x138e0) = 0;
    return this;
}

// @ 0x51d2b0
void Game::init_map()
{
    struct Tri { int a, b, c; };
    Tri t;

    if (*(char*)0xa73090 != 0)
        return;
    ((Obj4704b0*)*(int*)0xa732b8)->run();
    void* mem = operator new(0x344);
    if (mem != 0) {
        t.b = 5;
        t.c = 5;
        t.a = 0;
        *(int*)0xa732b4 = ((SceneCtor*)mem)->Init(*(int*)0xa732b8 + 0x44, *(int*)0xa732b8 + 0x54, &t.c, &t.b, &t.a);
    } else {
        *(int*)0xa732b4 = 0;
    }
    t.c = 1;
    t.b = 0;
    t.a = 7;
    ((Obj51dac0*)*(int*)0xa732b8)->run(&t.a, &t.b, &t.c);
    t.c = 7;
    t.b = 0;
    t.a = 7;
    ((Game*)*(int*)0xa732b8)->solve(&t.a, &t.b, &t.c);
    ((NavStore*)*(int*)0xa732b4)->build(*(int*)0xa732b8);
}

// @ 0x51d3a0
void Game::solve(int* arg1, int* arg2, int* arg3)
{
    int iVar1, iVar4, iVar6, iVar8, iVar10;
    unsigned uVar9, uVar11;
    int *piVar2, *piVar3;
    int *local_1c, *local_4;
    int local_30 = 0, local_2c, local_28, local_24, local_20, local_18, local_14, local_10, local_c, local_8;
    int* pb;
    int puVar5, puVar7;

    local_20 = -1;
    iVar4 = 0;
    for (uVar9 = 0; ; uVar9++) {
        pb = *(int**)((char*)this + 0x14);
        if (pb == 0)
            break;
        if ((unsigned)((*(int*)((char*)this + 0x18) - (int)pb) / 0x88) <= uVar9)
            break;
        *(int*)((char*)pb + iVar4 + 0x48) = -1;
        *(int*)((char*)pb + iVar4 + 0x4c) = -1;
        *(int*)((char*)pb + iVar4 + 0x50) = -1;
        iVar4 += 0x88;
    }
    iVar4 = 0;
    pb = *(int**)((char*)this + 0x14);
    if (pb != 0)
        iVar4 = (*(int*)((char*)this + 0x18) - (int)pb) / 0x88;
    local_1c = new int[iVar4];
    iVar4 = 0;
    pb = *(int**)((char*)this + 0x14);
    if (pb != 0)
        iVar4 = (*(int*)((char*)this + 0x18) - (int)pb) / 0x88;
    local_4 = new int[iVar4];

    local_2c = -1;
    local_18 = 0;
    for (;;) {
        for (;;) {
            if (*(int*)((char*)this + 0x14) == 0 ||
                (unsigned)((*(int*)((char*)this + 0x18) - *(int*)((char*)this + 0x14)) / 0x88) <= (unsigned)local_18) {
                for (iVar4 = 0; ; iVar4++) {
                    if (*(int*)((char*)this + 0x14) == 0 ||
                        (unsigned)((*(int*)((char*)this + 0x18) - *(int*)((char*)this + 0x14)) / 0x88) <= (unsigned)iVar4)
                        break;
                    if (*(int*)(*(int*)((char*)this + 0x14) + iVar4 * 0x88 + 0x48) == 0x5f5e0ff) {
                        local_14 = 0;
                        *local_4 = iVar4;
                        walk(&iVar4, &iVar4, local_4, &local_14);
                    }
                }
                operator delete[](local_1c);
                operator delete[](local_4);
                return;
            }
            iVar4 = 0;
            uVar9 = (unsigned)local_18;
            if (local_2c >= 0) {
                uVar9 = (unsigned)local_1c[local_2c];
                local_18--;
                local_2c--;
            }
            iVar6 = *(int*)((char*)this + 0x14);
            piVar3 = (int*)(uVar9 * 0x88 + iVar6);
            local_30 = (int)uVar9;
            if (piVar3[0x12] >= 0) {
                local_18++;
                continue;
            }
            break;
        }

        if (piVar3[9] != *arg3) {
            local_24 = -1;
            local_8 = 0;
            if (*piVar3 < 1)
                goto lab67a;
            {
                local_28 = 0x38;
                do {
                    piVar2 = arg3;
                    local_10 = *(int*)(local_28 - 0x10 + (int)piVar3);
                    puVar5 = local_24;
                    if (local_10 >= 0) {
                        puVar7 = -1;
                        if (*(int*)(local_10 * 0x88 + iVar6 + 0x24) == *arg3) {
                            local_14 = 0;
                            *local_4 = (int)uVar9;
                            puVar7 = (int)((Helper51d920*)this)->call(&local_10, &local_30, local_4, &local_14, arg1, arg2, arg3);
                            uVar9 = (unsigned)local_30;
                        }
                        iVar4 = *(int*)((char*)this + 0x14);
                        if (*(float*)(uVar9 * 0x88 + iVar4 + local_28) < 1e9f ||
                            (uVar9 = (unsigned)local_30, puVar5 = local_24,
                             *(int*)(local_10 * 0x88 + iVar4 + 0x24) == *piVar2)) {
                            if (puVar7 < 0 && *(int*)(local_10 * 0x88 + iVar4 + 0x24) != *piVar2)
                                puVar7 = *(int*)(local_10 * 0x88 + iVar4 + 0x48);
                            if (*(int*)(local_10 * 0x88 + iVar4 + 0x48) < 0) {
                                local_2c++;
                                *(int*)((char*)local_1c + local_2c * 4) = local_10;
                            }
                            uVar9 = (unsigned)local_30;
                            puVar5 = puVar7;
                            if (local_24 >= 0 && puVar7 >= 0 && local_24 != puVar7) {
                                local_20--;
                                if (puVar7 < local_24) {
                                    local_c = local_24;
                                    local_24 = puVar7;
                                } else {
                                    local_c = puVar7;
                                }
                                iVar4 = 0;
                                uVar11 = 0;
                                while (*(int*)((char*)this + 0x14) != 0 &&
                                       uVar11 < (unsigned)((*(int*)((char*)this + 0x18) - *(int*)((char*)this + 0x14)) / 0x88)) {
                                    puVar5 = *(int*)(*(int*)((char*)this + 0x14) + iVar4 + 0x48);
                                    iVar6 = *(int*)((char*)this + 0x14) + iVar4;
                                    if (local_c < puVar5) {
                                        if (puVar5 == 0x5f5e0ff) {
                                            puVar5 = *(int*)(iVar6 + 0x48);
                                            goto lab725;
                                        }
                                        uVar11++;
                                        *(int*)(iVar6 + 0x48) = puVar5 - 1;
                                        iVar4 += 0x88;
                                    } else {
lab725:
                                        if (puVar5 == local_c)
                                            *(int*)(iVar6 + 0x48) = local_24;
                                        uVar11++;
                                        iVar4 += 0x88;
                                    }
                                }
                            }
                        }
                    }
                    local_24 = puVar5;
                    local_28 += 4;
                    iVar6 = *(int*)((char*)this + 0x14);
                    piVar3 = (int*)(uVar9 * 0x88 + iVar6);
                    local_8++;
                } while (local_8 < *piVar3);
                puVar5 = local_24;
                if (local_24 < 0)
                    goto lab67a;
                *(int*)(uVar9 * 0x88 + *(int*)((char*)this + 0x14) + 0x48) = puVar5;
                goto lab693;
            lab67a:
                puVar5 = local_20 + 1;
                local_20 = puVar5;
                *(int*)(uVar9 * 0x88 + *(int*)((char*)this + 0x14) + 0x48) = puVar5;
            lab693:
                local_18++;
            }
        } else {
            piVar3[0x12] = 0x5f5e0ff;
            iVar6 = *(int*)((char*)this + 0x14);
            iVar10 = (int)uVar9 * 0x88;
            if (*(int*)(iVar10 + iVar6) >= 1) {
                iVar8 = 0x28;
                iVar4 = 0;
                do {
                    iVar1 = *(int*)(iVar10 + iVar6 + iVar8);
                    if (iVar1 >= 0 && *(int*)(iVar1 * 0x88 + iVar6 + 0x48) < 0) {
                        local_2c++;
                        *(int*)((char*)local_1c + local_2c * 4) = iVar1;
                    }
                    iVar6 = *(int*)((char*)this + 0x14);
                    iVar4++;
                    iVar8 += 4;
                } while (iVar4 < *(int*)(iVar10 + iVar6));
            }
            local_18++;
        }
    }
}

// @ 0x51d7c0
int* Game::walk(int* a, int* idx, int* seen, int* n)
{
    int* base = *(int**)((char*)this + 0x14);
    int* node = (int*)(*idx * 0x88 + (int)base);
    int local_4 = 0;
    if (*node < 1)
        return node;
    int local_8 = 0x28;
    for (;;) {
        int child = *(int*)(local_8 + (int)node);
        int num = *n;
        int k = 0;
        bool skip = false;
        if (num >= 0) {
            do {
                if (child == *(int*)((char*)seen + k * 4)) {
                    skip = true;
                    break;
                }
                k++;
            } while (k <= *n);
        }
        if (!skip && child >= 0) {
            int* c = (int*)(child * 0x88 + (int)base);
            if (c[0x48 / 4] == 0x5f5e0ff) {
                *n = num + 1;
                *(int*)((char*)seen + (num + 1) * 4) = *idx;
                walk(a, &child, seen, n);
                *n = num;
                if (*(int*)(*a * 0x88 + 0x50 + *(int*)((char*)this + 0x14)) != -1)
                    return (int*)(*a * 0x88 + *(int*)((char*)this + 0x14));
            } else {
                int* s = (int*)(*(int*)((char*)this + 0x14) + *a * 0x88);
                if (s[0x4c / 4] == -1) {
                    s[0x4c / 4] = c[0x48 / 4];
                } else if (s[0x4c / 4] != c[0x48 / 4]) {
                    s[0x50 / 4] = c[0x48 / 4];
                    return (int*)(child * 0x88 + *(int*)((char*)this + 0x14));
                }
            }
        }
        base = *(int**)((char*)this + 0x14);
        local_8 += 4;
        node = (int*)(*idx * 0x88 + (int)base);
        local_4++;
        if (*(int*)(*idx * 0x88 + (int)base) <= local_4)
            return node;
    }
}
