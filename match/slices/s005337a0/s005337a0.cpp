// Slice s005337a0 (batch ds1): three methods of the party/roster screen.  The widget
// host reads its child widgets from raw offsets; the value sources are the usual
// UiDsv objects at widget+0x484.  Names are provisional.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <stdlib.h>
#include "rgl.h"

struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10();
    virtual void select(int flag);                       // +0x2c
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16();
    virtual bool hit(int mask);                          // +0x44
    virtual void refresh(int mask);                      // +0x48
    virtual void v19(); virtual void v20(); virtual void v21();
    void get3(int* a, int* b, int* c);                   // 0x533a60 (ret 0xc)
};

struct IntVec {
    char pad0[4];
    int* first;                                          // +0x04
    int* last;                                           // +0x08
    int* end;                                            // +0x0c
    void push_back(const int& v);                        // 0x440b30 (ret 4)
    void resize(unsigned n, int v);                      // 0x43f310 (ret 8)
};

struct MenuItem {
    char pad0[4];
    int f4;                                              // +0x04
    char pad8[0x10 - 8];
    int f10;                                             // +0x10
    int f14;                                             // +0x14
    int f18;                                             // +0x18
};

struct MainMenu {
    void show(int code);                                 // 0x495700 (ret 4)
};

struct FlagSet {
    void commit(void* slot);                             // 0x513710 (ret 4)
};

struct ValueSource {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17();
    virtual void refresh(int mask);                      // +0x48
    void on_value_changed(float v);                      // 0x4d5940 (ret 4)
    void set_value(float v, void* src);                  // 0x48baa0 (ret 8)
};

extern void* g_game;                                     // 0x5e2930 (opaque game object)
extern int   g_world_time;                               // 0x5d8790
extern float g_ui_alpha;                                 // 0x5d7094
extern float g_scroll;                                   // 0x590d7c
extern int   g_a170d8;                                   // 0xa170d8
extern int   g_a174d8;                                   // 0xa174d8
extern FlagSet g_flags5e291c;                            // 0x5e291c
extern MainMenu g_a73980;                                // 0xa73980
extern MenuItem* g_a739e4;                               // 0xa739e4
extern MenuItem* g_a739e8;                               // 0xa739e8

struct Screen {
    char data[0x5ac8];

    void on_tick(int code);                              // 0x5337a0 (ret 4)
    void on_input(int code);                             // 0x533aa0 (ret 4)
    void refresh_anim(int a, int b);                     // 0x50a1c0 (ret 8)
    void method_4fc820(int arg);                         // 0x4fc820 (ret 4)
};

struct VecIter {
    int* first;                                          // +0x00
    int* second;                                         // +0x04
    VecIter* advance(int** out, int idx);                // 0x534160 (ret 8)
};

#define F(p, off, type) (*(type*)((char*)(p) + (off)))
#define W(p, off) (*(Widget**)((char*)(p) + (off)))
#define V(p, off) ((IntVec*)((char*)(p) + (off)))

inline float anim_value(ValueSource* vs)
{
    float* base = (float*)((char*)vs + 0xf0 + F(vs, 0x178, int) * 4);
    return ((float)g_world_time / base[0]) * F(vs, 0x17c, float) * g_scroll
         + (base[0] - F(vs, 0x17c, float))
         * ((F(vs, 0x168, float) - F(vs, 0x16c, float))
          / (F(vs, 0x170, float) - F(vs, 0x16c, float)));
}

inline void set_anim(Screen* s, int push_off, int resize_off)
{
    IntVec* v = V(s, push_off);
    v->push_back(F(s, 0xa8, int));
    v->push_back(F(s, 0xac, int));
    IntVec* d = V(s, resize_off);
    unsigned n = d->first ? (unsigned)(d->last - d->first) : 0;
    d->resize(n - 2, 0);
}

inline void clear_anim(Screen* s)
{
    if (F(s, 0x7c, void*) != 0)
        free(F(s, 0x7c, void*));
    F(s, 0x7c, void*) = 0;
    F(s, 0x80, void*) = 0;
    F(s, 0x84, void*) = 0;
    if (F(s, 0x8c, void*) != 0)
        free(F(s, 0x8c, void*));
    F(s, 0x8c, void*) = 0;
    F(s, 0x90, void*) = 0;
    F(s, 0x94, void*) = 0;
}

// @ 0x5337a0
void Screen::on_tick(int code)
{
    if (W(this, 0x40)->hit(0x10000))
        method_4fc820(1);
    if (W(this, 0x44)->hit(0x10000))
        method_4fc820(0);

    if (g_world_time != 0) {
        ValueSource* vs = *(ValueSource**)((char*)W(this, 0x38) + 0x484);
        vs->set_value(anim_value(vs), vs);
        ValueSource* vs2 = *(ValueSource**)((char*)W(this, 0x38) + 0x484);
        vs2->refresh(0x10000);
    }

    if (W(this, 0x30)->hit(0x10000)) {
        int a, b, c;
        W(this, 0x30)->get3(&a, &b, &c);
        g_a739e8->f18 = g_a739e4->f18;
        F(g_game, 0x13b08, int) = a;
        F(g_game, 0x13b0c, int) = b;
        g_a73980.show(0x1a);
        return;
    }
    if (W(this, 0x48)->hit(0x10000)) {
        g_a739e8->f18 = g_a739e4->f18;
        F(g_game, 0x13b08, int) = 1;
        F(g_game, 0x13b0c, int) = 0;
        g_a73980.show(0x1a);
        return;
    }
    if (W(this, 0x4c)->hit(0x10000)) {
        g_a739e8->f18 = g_a739e4->f18;
        F(g_game, 0x13b08, int) = 3;
        F(g_game, 0x13b0c, int) = 0;
        g_a73980.show(0x1a);
        return;
    }
    if (W(this, 0x50)->hit(0x10000)) {
        g_a739e8->f18 = g_a739e4->f18;
        F(g_game, 0x13b08, int) = 2;
        F(g_game, 0x13b0c, int) = 0;
        g_a73980.show(0x1a);
        return;
    }
    if (W(this, 0x54)->hit(0x10000)) {
        g_a739e8->f18 = g_a739e4->f18;
        F(g_game, 0x13b08, int) = 4;
        F(g_game, 0x13b0c, int) = 0;
        g_a73980.show(0x1a);
        return;
    }
    if (W(this, 0x3c)->hit(0x10000) || (g_a170d8 == 0 && g_a174d8 != 0)) {
        if (F(this, 0x18, int) == 100) {
            int slot;
            g_flags5e291c.commit(&slot);
            return;
        }
        g_a73980.show(F(this, 0x18, int));
    }
}

__declspec(noinline)
// @ 0x533a60
void Widget::get3(int* a, int* b, int* c)
{
    int* p = F(this, 0x19c, int*);
    p += F(this, 0x1cc, int);
    *a = *p;
    p = F(this, 0x1ac, int*);
    p += F(this, 0x1cc, int);
    *b = *p;
    *c = F(this, 0x1cc, int);
}

// @ 0x533aa0
void Screen::on_input(int code)
{
    if (g_world_time != 0) {
        if (g_ui_alpha < 0.7f
            && F(W(this, 0x4c), 0xf4, float) < F(W(this, 0x44), 0xf4, float)) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x4c) + 0x484);
            vs->on_value_changed((float)g_world_time);
            ValueSource* vs2 = *(ValueSource**)((char*)W(this, 0x4c) + 0x484);
            vs2->refresh(0x10000);
        } else {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            vs->set_value(anim_value(vs), vs);
            ValueSource* vs2 = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            vs2->refresh(0x10000);
        }
    }

    if (W(this, 0x6c)->hit(0x10000)) {
        set_anim(this, 0x88, 0x78);
        if (F(this, 0xa8, int) == F(this, 0x98, int)) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            F(this, 0xb0, float) = anim_value(vs);
        }
        refresh_anim(F(this, 0xa8, int), F(this, 0x9c, int));
    }

    if (W(this, 0x70)->hit(0x10000)) {
        set_anim(this, 0x78, 0x88);
        if (F(this, 0xa8, int) == F(this, 0xa0, int)) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            F(this, 0xb0, float) = anim_value(vs);
        }
        refresh_anim(F(this, 0xa8, int), F(this, 0xa4, int));
    }

    if (W(this, 0x34)->hit(0x10000)) {
        Widget* w = W(this, 0x34);
        VecIter it;
        it.first = F(w, 0x19c, int*);
        it.second = 0;
        VecIter* r = it.advance(&it.second, F(w, 0x1cc, int));
        int iv2 = *r->first;
        int iv1 = F(w, 0x1ac, int*)[F(w, 0x1cc, int)];
        if (F(this, 0x8c, void*) != 0)
            free(F(this, 0x8c, void*));
        F(this, 0x8c, void*) = 0;
        F(this, 0x90, void*) = 0;
        F(this, 0x94, void*) = 0;
        if (F(this, 0xa8, int) >= 0
            && (F(this, 0xa8, int) != iv2 || F(this, 0xac, int) != iv1)) {
            V(this, 0x78)->push_back(F(this, 0xa8, int));
            V(this, 0x78)->push_back(F(this, 0xac, int));
        }
        if (F(this, 0xa8, int) == iv2) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            F(this, 0xb0, float) = anim_value(vs);
        }
        refresh_anim(iv2, iv1);
    }

    if (W(this, 0x3c)->hit(0x10000)) {
        Widget* w = W(this, 0x3c);
        VecIter it;
        it.first = F(w, 0x19c, int*);
        it.second = 0;
        VecIter* r = it.advance(&it.second, F(w, 0x1cc, int));
        int iv2 = *r->first;
        int iv1 = F(w, 0x1ac, int*)[F(w, 0x1cc, int)];
        if (F(this, 0x8c, void*) != 0)
            free(F(this, 0x8c, void*));
        F(this, 0x8c, void*) = 0;
        F(this, 0x90, void*) = 0;
        F(this, 0x94, void*) = 0;
        if (F(this, 0xa8, int) >= 0
            && (F(this, 0xa8, int) != iv2 || F(this, 0xac, int) != iv1)) {
            V(this, 0x78)->push_back(F(this, 0xa8, int));
            V(this, 0x78)->push_back(F(this, 0xac, int));
        }
        if (F(this, 0xa8, int) == iv2) {
            ValueSource* vs = *(ValueSource**)((char*)W(this, 0x48) + 0x484);
            F(this, 0xb0, float) = anim_value(vs);
        }
        refresh_anim(iv2, iv1);
    }

    if (W(this, 0x58)->hit(0x10000)) {
        clear_anim(this);
        refresh_anim(1, 0);
    }
    if (W(this, 0x60)->hit(0x10000)) {
        clear_anim(this);
        refresh_anim(2, 0);
    }
    if (W(this, 0x64)->hit(0x10000)) {
        clear_anim(this);
        refresh_anim(3, 0);
    }
    if (W(this, 0x5c)->hit(0x10000)) {
        clear_anim(this);
        refresh_anim(4, 0);
    }
    if (W(this, 0x50)->hit(0x10000)) {
        g_a739e4->f18 = g_a739e8->f18;
        g_a739e4->f4 = 1;
        g_a73980.show(0x19);
        return;
    }
    if (W(this, 0x74)->hit(0x10000)) {
        F(g_game, 0x88, char) = 1;
        F(g_game, 0x84, int) = F(this, 0xac, int);
        g_a73980.show(F(this, 0x18, int));
        return;
    }
    if (W(this, 0x54)->hit(0x10000)) {
        g_a739e4->f18 = g_a739e8->f18;
        g_a739e4->f4 = 0;
        g_a73980.show(0x19);
        return;
    }
    if (W(this, 0x68)->hit(0x10000) || (g_a170d8 == 0 && g_a174d8 != 0)) {
        if (F(this, 0x18, int) == 100) {
            char slot;
            g_flags5e291c.commit(&slot);
            return;
        }
        g_a73980.show(F(this, 0x18, int));
    }
}
