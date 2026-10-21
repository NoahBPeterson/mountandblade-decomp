// Slice s0043db60: Agent path goal setters / path follower, the game font's glyph-advance helpers, container
// helpers (E1c8 fill, tstring vector insert) and the two "too long" length_error throwers.
#include <vector>
#include <memory>
#include <stdlib.h>
#include <stdexcept>
#include "agent_nav.h"

// rgl string with a read cursor (16 bytes; see match/slices/s0042aaf0/tstring.h): out-of-line members only.
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring(const tstring& o);                       // 0x42c110
    ~tstring();                                      // 0x42bc40
    tstring& operator=(const tstring& o);            // 0x42bc50
};

static inline Agent* AGENT(unsigned id)
{
    Agent** pp = &g_mission->pages[id >> 4];
    return *pp + (id & 0xf);
}

// ---- E1c8 fill (VC7.1 _Uninitialized_fill_n for a polymorphic 0x1c8-byte record) ----------------------
struct ptr_tag {};
struct E1c8 { virtual ~E1c8(); char body[0x1c8 - 4]; };
struct allocE1c8 {};
void __cdecl uninit_fill_n1c8(E1c8* f, unsigned n, const E1c8& v, allocE1c8& al, ptr_tag tag);     // 0x43ab50
struct vec1c8 : allocE1c8 { E1c8* first; E1c8* last; E1c8* end; E1c8* ufill(E1c8* f, unsigned n, const E1c8& v); };

// @ 0x43db60
E1c8* vec1c8::ufill(E1c8* f, unsigned n, const E1c8& v)
{
    ptr_tag t;
    uninit_fill_n1c8(f, n, v, *this, t);
    return f + n;
}

// @ 0x43db90
void Agent::set_goal_agent(unsigned id)
{
    int a = AGENT(id)->update_nav_face();
    if (update_nav_face() != a) {
        if (path_active != 0 && AGENT(id)->update_nav_face() == goal_face) {
            goal_changed = 0;
            goto done;
        }
        goal_changed = 1;
    }
    path_active = 0;
done:
    f318 = id;
    goal = *(vector4*)&AGENT(id)->pos;
    goal_dir = *(vector4*)(((char*)AGENT(id)) + 0x56b4);
    goal_face = AGENT(id)->update_nav_face();
    path_flag = 0;
    path_state = 2;
}

// @ 0x43dcb0
void Agent::set_goal_point(vector4 p, int face)
{
    if (update_nav_face() != face) {
        if (path_active != 0 && face == goal_face) {
            goal_changed = 0;
            goto done;
        }
        goal_changed = 1;
    }
    path_active = 0;
done:
    goal = p;
    p.x = 0.0f; p.y = 0.0f; p.z = 0.0f;
    goal_dir = p;
    f318 = -1;
    goal_face = face;
    path_state = 1;
    path_flag = 0;
}

// @ 0x43dd70
void Agent::set_goal_near(const vector4* center, const float* radius)
{
    int i = 0;
    float r = *radius;
    vector4 p;
    Mission* m;
    do {
        p = *center;
        p.x = ((rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * r * 2.0f + p.x;
        p.y = ((rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * r * 2.0f + p.y;
        m = g_mission;
        if (m->min_x <= p.x && p.x < m->max_x && m->min_y <= p.y && p.y < m->max_y)
            break;
        i++;
        r *= 0.9f;
    } while (i < 0x20);
    vector4 d(goal.x - p.x, goal.y - p.y, goal.z - p.z);
    if (d.z * d.z + (d.y * d.y + d.x * d.x) < 1e-06f) {
        path_state = 1;
        return;
    }
    int cur = goal_face;
    int face;
    if (cur >= 0 && m->nav.inside(&goal_face, &p.x)) {
        face = cur;
    } else {
        int mode = 2;
        if (cur >= 0 && (face = m->scene->locate_near(&p, &goal_face, &mode)) != -1) {
        } else {
            face = g_mission->scene->locate(&p);
        }
    }
    set_goal_point(p, face);
}

// @ 0x43df60
void Agent::advance_path()
{
    if (path_active != 0 && (path_state & 3) != 0) {
        int face = update_nav_face();
        int n = path_count;
        if (face == path_face[n]) {
            path_active = 0;
            goal_changed = 0;
            return;
        }
        if (face >= 0) {
            NavMesh* nav = &g_mission->nav;
            if (nav->inside(&face, &pos.x)) {
                int next = path_cur + 1;
                int nface;
                vector4 np;
                if (next < n) {
                    nface = path_face[path_cur + 2];
                    np = path_pos[path_cur];
                } else {
                    nface = goal_face;
                    np = goal;
                }
                int steps = 10;
                vector4 d;
                d.x = pos.x - np.x;
                d.y = pos.y - np.y;
                d.z = pos.z - np.z;
                if (d.out_length_squared() < 900.0f) {
                    if (nav->trace(face, pos, nface, &np.x, &steps))
                        path_cur = next;
                }
            }
        }
        if (path_cur >= path_count) {
            path_active = 0;
            goal_changed = 0;
        }
    }
}

// @ 0x43e0e0
void deque_xlen() { throw std::length_error("deque<T> too long"); }

// ---- two-vector tidy ---------------------------------------------------------------------------------
struct PtrVec {                                      // (first, last, end) of an engine vector
    void *first, *last, *end;
    void tidy()
    {
        if (first != 0)
            operator delete(first);
        first = 0; last = 0; end = 0;
    }
};
struct VecPair2 {                                    // two vectors at +0x10 and +0x20
    char pad0[0x10];
    PtrVec a;
    char pad1c[4];
    PtrVec b;
};
// @ 0x43e150
void __stdcall tidy_vec_pair(VecPair2* p)
{
    p->b.tidy();
    p->a.tidy();
}

// ---- font glyph metrics (provisional) ---------------------------------------------------------------------
struct Glyph {                                       // 0x1c bytes: key + 6 floats (std::map<int, ...> value_type)
    int key;
    float f4, f8, fc, f10, f14, f18;
};
struct GlyphNode { GlyphNode *left, *parent, *right; Glyph val; };
struct GlyphMap {                                    // std::map<int, glyph> (comparator, head, size)
    int pad0[2];
    GlyphNode* head;                                 // +8
    void lookup(GlyphNode** out, const int* key);    // 0x43b210 (lower_bound/find with an out iterator)
};
struct Font {
    char pad0[0x28];
    Glyph dflt;                                      // +0x28 (returned for unknown characters)
    GlyphMap map;                                    // +0x44
    char pad50[0x6c - 0x50];
    Glyph ascii[0x200];                              // +0x6c
    __declspec(noinline) float advance(int ch);      // 0x43e190
    float measure(const int* text, int len, int* start, int* end);   // 0x43e200 (ret 0x10)
};

// @ 0x43e190
float Font::advance(int ch)
{
    if (ch >= 0x200) {
        GlyphNode* it;
        map.lookup(&it, &ch);
        if (it != map.head) {
            Glyph* g = &it->val;
            return g->f18 + g->f10;
        }
        return dflt.f18 + dflt.f10;
    }
    Glyph* g = &ascii[ch];
    return g->f18 + g->f10;
}

// @ 0x43e200
float Font::measure(const int* text, int len, int* start, int* end)
{
    float sum = 0.0f;
    bool in_tag = false;
    int i = *start;
    if (i < *end) {
        do {
            if (i >= len)
                break;
            if (in_tag) {
                if (text[i] == 0x3e)
                    in_tag = false;
            } else {
                int c = text[i];
                if (c == 0x3c)
                    in_tag = true;
                else if (c == 9)
                    sum = (float)(int)(sum * 4.0f) * 0.25f + 0.4f;
                else
                    sum += advance(c);
            }
            i++;
        } while (i < *end);
    }
    return sum;
}

// @ 0x43e2a0
void vec_xlen() { throw std::length_error("vector<T> too long"); }

// ---- std::vector<tstring>::_Insert_n (VC7.1 <vector>; built with /EHa like the other _Insert_n slices; its
// catch(...) funclet 0x43e47a and the cold branches (0x43e49c...) live in the same COMDAT) -----------------
// @ 0x43e310
template void std::vector<tstring>::_Insert_n(std::vector<tstring>::iterator, size_t, const tstring&);
