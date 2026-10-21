// Slice s0041ee20: the world/scene container's entity bookkeeping (link pools, entity removal, teardown) and the
// VC7.1 <list>/<vector> instantiations they use (names provisional).
#include <list>
#include <vector>
#include <algorithm>
#include <string.h>
#include <new>
#include "rgl.h"

using rgl::vector4;

// ---- hand-written std::vector layout (VC7.1: empty allocator slot, first, last, end), as in s0040d7d0 ----
template<class T> struct vec {
    std::allocator<T> al;
    T* first; T* last; T* end;
    vec() : first(0), last(0), end(0) {}
    ~vec() { tidy(); }
    unsigned size() const { return first == 0 ? 0 : last - first; }
    struct iter {                                       // std::_Ptrit
        T* cur;
        iter() : cur(0) {}
        iter(T* p) : cur(p) {}
        T& operator*() const { return *cur; }
        iter& operator+=(int n) { cur += n; return *this; }
        iter operator+(int n) const { iter t = *this; return t += n; }
    };
    iter begin() { return iter(first); }
    T& operator[](unsigned i) { return *(begin() + i); }
    void tidy() { if (first != 0) operator delete(first); first = 0; last = 0; end = 0; }
    void clear();
};

template<class T> void vec<T>::clear() { if (first != 0) operator delete(first); first = 0; last = 0; end = 0; }

// ---- link pool ------------------------------------------------------------------------------------
struct link_t { link_t* next; link_t* prev; };
struct ref_t { ref_t* next; link_t* link; int a, b; };       // 0x10 bytes: one membership of an entity in a cell list
struct cell34 { char pad[0x30]; link_t* head; };             // 0x34 bytes
struct holder { char pad[0x14]; ref_t* refs; };              // entity part at +0x14
struct entity { virtual ~entity(); int f4; int id; };       // +8 id; slot 0 is the deleting destructor
struct part_t { int v[25]; };                                 // 100 bytes
struct item_t {                                               // an object with a part list
    char pad0[0xc];
    vec<part_t> parts;                                        // 0xc (first at +0x10)
    char pad1c[0x19c - 0x1c];
    int f19c;
};
struct world {
    char pad0[0x28];
    cell34** cells;              // +0x28
    char pad2c[0x3c - 0x2c];
    link_t* free_links;          // +0x3c
    char pad40[0x50 - 0x40];
    ref_t* free_refs;            // +0x50
    char pad54[0x7c - 0x54];
    std::vector<entity*> ents;   // +0x7c (first at +0x80)
    std::vector<entity*> ents2;  // +0x8c (first at +0x90)
    char pad9c[0x32c - 0x9c];
    std::vector<item_t*> items;  // +0x32c (first at +0x330)
    int live;                    // +0x33c
    void release_links(holder* e);          // 0x41ee20
    void remove_entity(int id);             // 0x41f0d0
    void partition_items();                 // 0x41efd0
};

// @ 0x41ee20
void world::release_links(holder* e)
{
    ref_t* r = e->refs;
    ref_t* first = r;
    for (; r != 0; r = r->next) {
        link_t* l = r->link;
        if (l->prev != 0)
            l->prev->next = l->next;
        else
            cells[r->a][r->b].head = l->next;
        if (l->next != 0)
            l->next->prev = l->prev;
        l->next = free_links;
        free_links = l;
    }
    e->refs = 0;
    if (first != 0) {
        ref_t* last = first;
        while (last->next != 0)
            last = last->next;
        last->next = free_refs;
        free_refs = first;
    }
}

// ---- std::list<T*> instantiations (VC7.1 <list>): _Buynode 0x41eeb0 (+ funclet 0x41ef19), _Insert 0x41f640,
// _Insert_n 0x41f680 (+ funclet 0x41f6e6), erase 0x41f310. The element is a 4-byte pointer.
typedef std::list<item_t*> item_list;

// @ 0x41f310
template item_list::iterator item_list::erase(item_list::iterator);
// @ 0x41f640
template void item_list::_Insert(item_list::iterator, item_t* const&);
// @ 0x41f680
template void item_list::_Insert_n(item_list::iterator, item_list::size_type, item_t* const&);

// ---- std::vector<T*>::_Ucopy (memmove) ----
// @ 0x41ef30
template item_t** std::vector<item_t*>::_Ucopy<item_t**>(item_t**, item_t**, item_t**);

// ---- view frustum constructor (class defined in s0041e470; planes built by 0x41e4f0) ----
struct plane { vector4 pos; vector4 normal; };
struct frustum {
    char perspective;          // +0
    char flag1;                // +1
    char pad2[2];
    int nplanes;               // +4
    plane planes[6];           // +8
    rgl::matrix m;             // +0xc8
    float left, right, top, bottom, nearz, farz;   // +0x108
    frustum();                 // 0x41ef60
    void build_planes();       // 0x41e4f0
};
__declspec(noinline) rgl::matrix* identity_pos(rgl::matrix* out);       // 0x40cd00 (cdecl, returns out)

// @ 0x41ef60
frustum::frustum()
{
    rgl::matrix t;
    m = *identity_pos(&t);
    perspective = 1;
    flag1 = 0;
    right = 1.0f;
    left = -1.0f;
    bottom = -1.0f;
    top = 1.0f;
    nearz = 1.0f;
    farz = 1000.0f;
    build_planes();
}

// @ 0x41efd0
void world::partition_items()
{
    live = items.size();
    int i = 0;
    while (i < live) {
        if (items[i]->f19c <= 0 && items[i]->parts.size() == 0) {
            item_t* tmp = items[live - 1];
            items[live - 1] = items[i];
            items[i] = tmp;
            live--;
        } else {
            i++;
        }
    }
}

// ---- entity removal ----
// raw view of a VC7.1 std::vector<entity*> (allocator slot, first, last, end)
struct rawvec { int al; entity** first; entity** last; entity** end; };

// @ 0x41f080
template std::vector<entity*>::iterator std::remove(std::vector<entity*>::iterator, std::vector<entity*>::iterator, entity* const&);

void mb_error(const char* fmt, ...);                      // 0x42cc80

static __forceinline bool remove_from(world* w, rawvec& v, int id, bool rel)
{
    bool found = false;
    entity** it = v.first;
    entity** nxt = it + 1;
    do {
        if (it >= v.last) break;
        if ((*it)->id == id) {
            if (rel) w->release_links((holder*)*it);
            delete *it;
            memmove(it, nxt, (v.last - nxt) * sizeof(entity*));
            v.last--;
            found = true;
        }
        it++;
        nxt++;
    } while (!found);
    return found;
}
// @ 0x41f0d0
void world::remove_entity(int id)
{
    rawvec& v1 = (rawvec&)ents;
    rawvec& v2 = (rawvec&)ents2;
    if (!remove_from(this, v1, id, true)) {
        if (!remove_from(this, v2, id, false))
            mb_error("unable to remove entity %d \n", id);
    }
}

// ---- scene object: sub-objects with kid lists, owned buffers, teardown ----
struct kid_t { char pad[0x1a8]; int id; };                  // +0x1a8: id
struct sub_t {
    char pad0[0xb0];
    vec<kid_t*> kids;           // +0xb0 (first at +0xb4, ends at +0xc0)
    void* data;                 // +0xc0
};
struct guard {                  // owns a heap block when `on` is set
    int on;
    void* ptr;
    ~guard() { if (on) operator delete(ptr); }
};
struct owner_t {
    virtual ~owner_t();                                      // 0x41f1d0 (vtable 0x54b454)
    char pad4[0x19c - 4];
    vec<sub_t*> subs;           // +0x19c (first at +0x1a0)
    char pad1ac[0x1b0 - 0x1ac];
    vec<void*> bufs;            // +0x1b0 (first at +0x1b4)
    char pad1c0[0x1c4 - 0x1c0];
    guard g;                    // +0x1c4
    void set_data(void* p);                                  // 0x41f250
    int find_kid(int id);                                    // 0x41f2a0
};

// @ 0x41f1d0
owner_t::~owner_t() {}

// @ 0x41f250
void owner_t::set_data(void* p)
{
    if (subs.size() != 0) {
        sub_t* s = subs[0];
        if (s->data != 0)
            operator delete(s->data);
        s->data = p;
    } else {
        operator delete(p);
    }
}

// @ 0x41f2a0
int owner_t::find_kid(int id)
{
    int r = -1;
    if (subs.size() != 0) {
        for (unsigned i = 0; i < subs[0]->kids.size(); i++) {
            if (subs[0]->kids[i]->id == id)
                r = i;
        }
    }
    return r;
}

// ---- chunked free-list pool (provisional): a list of chunk pointers, `per_chunk` cells in each, all chained ----
struct cell12 { cell12* next; int a, b; };
struct cell16 { cell16* next; int a, b, c; };
template<class T> struct pool {
    std::list<T*> chunks;       // +0 (head at +4)
    int per_chunk;              // +0xc
    T* free_head;               // +0x10
    void link_chunks();
};
template<class T> void pool<T>::link_chunks()
{
    typedef typename std::list<T*>::iterator It;
    for (It it = chunks.begin(); it != chunks.end(); ++it) {
        for (int i = 0; i < per_chunk - 1; i++) {
            (*it)[i].next = &(*it)[i + 1];
        }
        It nx = it;
        ++nx;
        if (nx != chunks.end())
            (*it)[per_chunk - 1].next = *nx;
        else
            (*it)[per_chunk - 1].next = 0;
    }
    if (chunks.begin() != chunks.end())
        free_head = *chunks.begin();
}
// @ 0x41f350
template void pool<cell12>::link_chunks();
// @ 0x41f3d0
template void pool<cell16>::link_chunks();

// ---- vec<T>::clear (the container class of the collision shapes, see s0040d7d0) and a four-vec teardown ----
struct mface { int a, flags, n, idx[4], m4; };
// @ 0x41f4b0
template void vec<mface>::clear();
struct four_vecs {
    vec<vector4> a;             // +0
    vec<vector4> b;             // +0x10
    vec<mface> c;               // +0x20
    vec<int> d;                 // +0x30
    ~four_vecs();               // 0x41f5d0
};
// @ 0x41f5d0
four_vecs::~four_vecs() {}

// ---- recursive teardown of a scene node ----
struct resmgr_t {
    void release(int handle);   // 0x4175f0
};
extern resmgr_t g_resmgr;                               // 0x5d6fa0
struct vobj { virtual ~vobj(); };
struct scene_node {
    char pad0[0x154];
    int handle;                 // +0x154 (resource handle, released through the resource manager)
    scene_node* child;          // +0x158
    char pad15c[0x194 - 0x15c];
    vec<void*> v194;            // +0x194 (first at +0x198)
    char pad1a4[0x1b4 - 0x1a4];
    vec<void*> v1b4;            // +0x1b4 (first at +0x1b8)
    char pad1c4[0x1c8 - 0x1c4];
    vec<vobj*> kids;            // +0x1c8 (first at +0x1cc)
    void destroy();             // 0x41f4e0
};

// @ 0x41f4e0
void scene_node::destroy()
{
    if (handle != 0) {
        g_resmgr.release(handle);
        handle = 0;
    }
    scene_node* c = child;
    if (c != 0) {
        c->destroy();
        operator delete(c);
    }
    for (unsigned i = 0; i < kids.size(); i++) {
        vobj* o = kids[i];
        if (o != 0)
            delete o;
    }
    kids.clear();
    v1b4.clear();
    v194.clear();
}
