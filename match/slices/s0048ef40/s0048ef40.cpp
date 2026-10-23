// Slice s0048ef40: Mission camera / frustum helpers, two agent-slot accessors and one item count.
//   0x48ef40 Mission::free_objects        -- destroy and free the object vector at +0x17c
//   0x48efa0 ItemRef::shares              -- do two item references name the same item (compare-functor)
//   0x48f020 Mission::cam_follow          -- place the camera object from the parallax offset
//   0x48f120 Mission::set_view_offset     -- scale the offset by 0.02 into the camera object and refresh
//   0x48f180 Mission::init_view           -- field initialiser (calloc'd record), four random noise values
//   0x48f450 Mission::last_frustum        -- last element of the object vector
//   0x48f480 Mission::set_frustum_matrix  -- copy 16 floats into the last frustum, rebuild its planes
//   0x48f4c0 Mission::set_frustum         -- fill the last frustum (flag, halves, ids) and rebuild
//   0x48f570 Mission::set_frustum_flag    -- store +0x48c of the last frustum
//   0x48f5a0 Mission::set_frustum_mat4    -- store 4 floats at +0x490 of the last frustum
//   0x48f5f0 Mission::delete_agent        -- forward a colour to an agent, follow for kind 7
//   0x48f6b0 Mission::move_agent          -- place an agent (or the camera) from its position, halved
//   0x48f790 Mission::count_item          -- sum the amounts of matching inventory rows
// Names provisional. The page-table layout (pages[i >> 4][i & 0xf]) is the one verified in
// s00474e60 / s004759c0; the script-side agent table (0x5ac8-byte rows) is the one used by 0x48eb50.
#include <windows.h>

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    tstring(const tstring& o);                          // 0x42c110
    ~tstring();                                         // 0x42bc40
    void operator=(const tstring& o);                   // 0x42bc50
    bool equals(const tstring& o) const;                // 0x42bdd0
};

// ---- the mission-side object an agent is built on (vtable slots used by this slice) ------------------------------
struct ObjVt {
    virtual void v04(); virtual void v08(); virtual void v0c(); virtual void v10(); virtual void v14();
    virtual void v18(); virtual void v1c(); virtual void v20(); virtual void v24(); virtual void v28();
    virtual void v2c();
    virtual void v30();
    virtual void v34();
    virtual void v38(); virtual void v3c(); virtual void v40(); virtual void v44(); virtual void v48();
    virtual void v4c(); virtual void v50();
    virtual void v54();
    virtual void set_pos(float* pt);                    // +0x58
    virtual void v5c();
    virtual void v60();
    virtual void set_mat(float* m);                     // +0x64
    virtual void v68();
    virtual void set_mat2(float* m);                    // +0x6c (scale by 0.02)
    virtual void v70(); virtual void v74(); virtual void v78(); virtual void v7c();
    virtual void v80(); virtual void v84(); virtual void v88(); virtual void v8c();
    virtual void get_pos(float* out);                   // +0x94
    virtual void v98(); virtual void v9c(); virtual void va0();
    virtual void offset(float* out, int arg);           // +0xa4
    virtual void va8(); virtual void vac(); virtual void vb0(); virtual void vb4(); virtual void vb8();
    virtual void vbc();
    virtual int type();                                 // +0xc0
};

// the frustum record an object vector entry points at (+0x36c from the object)
struct Frustum {
    char pad0[0x108];
    float x108, x10c, x110, x114;                       // +0x108.. (+0x110 = +0x114)
    float x118, x11c;                                   // +0x118, +0x11c
    char pad120[0x1f8 - 0x120];
    float m1f8[16];                                     // +0x1f8 (matrix at +0x1f8; +0x36c - 0x174 below)
    char pad238[0x48c - 0x238];
    int flag48c;                                        // +0x48c
    float m490[4];                                      // +0x490

    void rebuild_planes();                              // 0x41e4f0 (thiscall: ecx = the frustum)
};
struct CamObj : ObjVt {
    char pad4[0x13c - 4];
    float x13c;                                         // +0x13c
    char pad140[0x14c - 0x140];
    int x14c, x150;                                     // +0x14c, +0x150
    Frustum* frustum;                                   // +0x17c (bogus: see Mission::obj_last)
};

// ---- ItemRef (8 bytes) and a rider row (0x88 bytes) of the agent at +0x14 -----------------------------------------
struct ItemRef {
    int item;
    unsigned m4;
};

struct Rider {
    char pad0[0x48];
    int item;                                           // +0x48
    int rider;                                          // +0x4c
    int agent;                                          // +0x50
};

// ---- agent record (0x1c8) of the mission page table --------------------------------------------------------------
struct Agent {
    ObjVt* vt;                                          // +0
    char pad4[4];
    char pad8[0x14 - 8];
    Rider* riders;                                      // +0x14 (array of 0x88-byte rows)
    char pad18[0xf0 - 0x18];
    float half_f0;                                      // +0xf0 (riding half width)
    float half_f4;                                      // +0xf4
    char padf8[0x148 - 0xf8];
    int flags148;                                       // +0x148 (bit 0x10, 0x1000)

    int shares(const ItemRef& a, const ItemRef& b) const; // 0x48efa0

    int type() { return vt->type(); }
    void set_pos(float* p) { vt->set_pos(p); }
    void get_pos(float* p) { vt->get_pos(p); }
    void offset(float* o, int a) { vt->offset(o, a); }
};

struct AgentTable {
    Agent** pages;                                      // +0
    int count;                                          // +4
    int pad8;                                           // +8
    int padc;                                           // +0xc
};

// a row of the script-side agent (0x5ac8 bytes) used by count_item
struct InvRow {
    int amount;                                         // +0
    int key;                                            // +4 (item id)
    unsigned char usable;                               // +8 (bit 0 = usable flag)
    char pad9[3];
    int pad[5];
};
struct ScriptAgent {
    char pad0[0xa8];
    InvRow inv[64];                                     // +0xa8
    int inv_count;                                      // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
};
struct SlotTable {
    char pad0[0x138d0];
    ScriptAgent** pages;                                // +0x138d0
    int table_slot(int id);                             // 0x48ad00
    ScriptAgent*& page(int i) { return pages[i]; }
    ScriptAgent& row(int id) { int s = table_slot(id); return *(ScriptAgent*)((char*)page(s >> 4) + (s & 0xf) * 0x5ac8); }
};
struct ScriptAgentTable {
    char pad0[0x139c8];
    ScriptAgent** pages;                                // +0x139c8

    int count_item(int key, int id, bool only_usable);   // 0x48f790
};

struct TableSlotAPI { int table_slot(int id); };        // 0x48ad00 (thiscall: ecx = the table base)
int   __cdecl rand_();                                  // 0x536a9f
void  __cdecl op_delete(void* p);                       // 0x535712 (operator delete)

extern const float k_half;                              // 0x549d70 (0.5f)
extern const float k_quarter;                           // 0x549d78 (0.25f)
extern const float k_zero;                              // 0x549ca8 (0.0f)
extern const float k_view_scale;                        // 0x590cd0 (0.02f)
extern const float k_cam_scale;                         // 0x591d44
extern const float k_cam_sub;                           // 0x54a704
extern const float k_noise_scale;                       // 0x54a6bc (6.32231e-05)
extern const float k_noise_w;                           // 0x54b3c8 (0.7f)
extern const float k_noise_c;                           // 0x549d70 (0.5f)

struct Mission {
    int scene;                                          // +0
    AgentTable agents;                                  // +4
    void** objects;                                     // +0x14 (vector: begin)
    void** objects_end;                                 // +0x18 (vector: end)
    void** objects_cap;                                 // +0x1c (vector: capacity)
    char pad20[0x30 - 0x20];
    int x30_0;                                          // +0x30
    int x30_1;                                          // +0x34
    int x30_2;                                          // +0x38
    int x30_3;                                          // +0x3c
    char pad40[0xd8 - 0x40];
    int view_x;                                         // +0xd8
    int view_y;                                         // +0xdc
    char pade0[0x144 - 0xe0];
    float x144;                                         // +0x144
    char pad148[0x17c - 0x148];
    void** objv_begin;                                  // +0x17c (a second object vector, freed by free_objects)
    void** objv_end;                                    // +0x180
    void** objv_cap;                                    // +0x184
    char pad188[0x490 - 0x188];
    void* camera;                                       // +0x490 (scene object whose +0x17c is the camera obj)
    char pad494[0x49c - 0x494];
    float par_x;                                        // +0x49c
    float par_y;                                        // +0x4a0

    Agent* agent(unsigned index) const { return &agents.pages[index >> 4][index & 0xf]; }
    void* last_object() const { return objects[objects_end - objects]; }
    CamObj* camera_obj() const { return *(CamObj**)((char*)camera + 0x17c); }

    void free_objects();                                // 0x48ef40
    void cam_follow(int arg);                           // 0x48f020
    void set_view_offset(int* off);                     // 0x48f120
    void init_view();                                   // 0x48f180
    void update_view();                                 // 0x48d470
    void* last_frustum();                               // 0x48f450
    void set_frustum_matrix(int* m);                    // 0x48f480
    void set_frustum(char* flag, float* p1, float* p2, float* p3, float* p4); // 0x48f4c0
    void set_frustum_flag(int v);                       // 0x48f570
    void set_frustum_mat4(int* m);                      // 0x48f5a0
    void delete_agent(unsigned index, int color);       // 0x48f5f0
    void move_agent(unsigned index, float* pos);        // 0x48f6b0
    int  count_item(int key, int id, bool only_usable);   // 0x48f790
};

struct VObj { virtual ~VObj(); };                        // slot 0: scalar deleting dtor (this, flags=1)

// @ 0x48ef40
void Mission::free_objects()
{
    void** p = objv_begin;
    while (p != objv_end) {
        VObj* o = (VObj*)*p;
        if (o) delete o;
        p++;
    }
    if (objv_begin) op_delete(objv_begin);
    objv_begin = 0;
    objv_end = 0;
    objv_cap = 0;
}

// @ 0x48efa0
int Agent::shares(const ItemRef& a, const ItemRef& b) const
{
    const Rider* ra = (const Rider*)(a.item * 0x88 + (int)riders);
    int ai = ra->item;
    int aj = -1;
    if (ai == 0x5f5e0ff) {
        ai = ra->rider;
        aj = ra->agent;
    }
    const Rider* rb = (const Rider*)(b.item * 0x88 + (int)riders);
    int bi = rb->item;
    int bj = -2;
    if (bi == 0x5f5e0ff) {
        bj = rb->agent;
        bi = rb->rider;
        if (bj < 0) bj = -2;
    }
    if (ai == bi || ai == bj || aj == bi || aj == bj) return 1;
    return 0;
}

// @ 0x48f020
void Mission::cam_follow(int arg)
{
    float v[3];
    camera_obj()->offset(v, arg);
    float px = par_x;
    float py = par_y;
    if (!(px * px + py * py >= k_zero)) {
        CamObj* cam = camera_obj();
        cam->set_mat(&par_x);
        camera_obj()->get_pos(&v[1]);
        camera_obj()->set_mat(v);
        CamObj* o = camera_obj();
        o->x13c = v[2] - x144 * k_cam_scale - k_cam_sub;
    }
    update_view();
}

// @ 0x48f120
void Mission::set_view_offset(int* off)
{
    float v[2];
    v[0] = k_view_scale * *(float*)off;
    v[1] = k_view_scale * *((float*)off + 1);
    camera_obj()->set_mat2(v);
    view_x = off[0];
    view_y = off[1];
    update_view();
}

// @ 0x48f450
void* Mission::last_frustum()
{
    int n = objects ? (int)(objects_end - objects) : 0;
    return objects[n - 1];
}

// @ 0x48f480
void Mission::set_frustum_matrix(int* m)
{
    int n = objects ? (int)(objects_end - objects) : 0;
    Frustum* f = (Frustum*)((char*)objects[n - 1] + 0x36c);
    memcpy((char*)f + 0xc8, m, 64);
    f->rebuild_planes();
}

// @ 0x48f4c0
void Mission::set_frustum(char* flag, float* p1, float* p2, float* p3, float* p4)
{
    int n = objects ? (int)(objects_end - objects) : 0;
    float x1 = *p1, x2 = *p2, x3 = *p3, x4 = *p4;
    Frustum* f = (Frustum*)((char*)objects[n - 1] + 0x36c);
    ((char*)f)[0] = *flag;
    *(float*)((char*)f + 0x108) = x1 * k_quarter;
    *(float*)((char*)f + 0x10c) = x1 * k_half;
    *(float*)((char*)f + 0x110) = x2 * k_half;
    *(float*)((char*)f + 0x114) = x2 * k_quarter;
    *(float*)((char*)f + 0x118) = x3;
    *(float*)((char*)f + 0x11c) = x4;
    f->rebuild_planes();
}

// @ 0x48f570
void Mission::set_frustum_flag(int v)
{
    int n = objects ? (int)(objects_end - objects) : 0;
    *(int*)((char*)objects[n - 1] + 0x48c) = v;
}

// @ 0x48f5a0
void Mission::set_frustum_mat4(int* m)
{
    int n = objects ? (int)(objects_end - objects) : 0;
    int* dst = (int*)((char*)objects[n - 1] + 0x490);
    dst[0] = m[0];
    dst[1] = m[1];
    dst[2] = m[2];
    dst[3] = m[3];
}

// @ 0x48f5f0
void Mission::delete_agent(unsigned index, int color)
{
    Agent* a = agent(index);
    int t = a->type();
    if (t == 0 || t == 1 || t == 9 || t == 6) {
        float v[2];
        agent(index)->offset(v, color);
    } else if (t == 7) {
        cam_follow(color);
    }
}

// @ 0x48f6b0
void Mission::move_agent(unsigned index, float* pos)
{
    Agent* a = agent(index);
    if (a->type() == 7) {
        a->set_pos(pos);
        cam_follow(0);
        return;
    }
    float wx = (a->flags148 & 0x10) ? a->half_f0 * k_half : k_zero;
    float wy = (a->flags148 & 0x1000) ? a->half_f4 * k_half : k_zero;
    float v[2];
    v[0] = pos[0] - wx;
    v[1] = pos[1] - wy;
    a->set_pos(v);
    a->get_pos(v);
}

// @ 0x48f790
// `this` is the script agent table owner: the slot table lives at +0xf8 (table_slot, thiscall) with the
// page table at +0x138d0 of it; rows are 0x5ac8 bytes at pages[slot >> 4][slot & 0xf]. The original
// re-evaluates the row lookup for every read (no CSE across the thiscall).
int ScriptAgentTable::count_item(int key, int id, bool only_usable)
{
    SlotTable* t = (SlotTable*)((char*)this + 0xf8);
    int total = 0;
    int i = 0;
    if (t->row(id).inv_count > 0) {
        do {
            if (t->row(id).inv[i].key == key) {
                if (only_usable ? (t->row(id).inv[i].usable & 1) : !(t->row(id).inv[i].usable & 1))
                    total += t->row(id).inv[i].amount;
            }
            i++;
        } while (i < t->row(id).inv_count);
    }
    return total;
}

// @ 0x48f180
void Mission::init_view()
{
    const int zero = 0;
    const float f1 = 1.0f, f01 = 0.1f, f150 = -15000.0f, f60000 = -1000000.0f, f015 = 0.15f;
    const float f05 = 0.5f, f10 = 10.0f, f1000 = -1000.0f;
    int u_w; x30_0 = zero; x30_1 = zero; x30_2 = zero; x30_3 = u_w; // +0x30..+0x3c (arg at [esp+4])
    *(int*)((char*)this + 0x960) = zero;
    *(int*)((char*)this + 0x964) = zero;
    *(int*)((char*)this + 0x95c) = zero;
    *(int*)((char*)this + 0x8f0) = zero;
    *(int*)((char*)this + 0x40) = *(int*)&f1;
    *(int*)((char*)this + 0x68) = *(int*)&f1000;
    *(int*)((char*)this + 0x6c) = zero;
    *(int*)((char*)this + 0x968) = zero;
    *(int*)((char*)this + 0x8ec) = zero;
    *(int*)((char*)this + 0x44) = zero;
    *(int*)((char*)this + 0x48) = *(int*)&f01;
    *(int*)((char*)this + 0x4c) = zero;
    *(int*)((char*)this + 0x50) = zero;
    *(int*)((char*)this + 0x88) = -1;
    *(int*)((char*)this + 0x59c0) = zero;
    *(int*)((char*)this + 0x8c) = zero;
    *(int*)((char*)this + 0x90) = zero;
    *(int*)((char*)this + 0x70) = -1;
    *(int*)((char*)this + 0x74) = zero;
    *(int*)((char*)this + 0x934) = zero;
    *(int*)((char*)this + 0x8e0) = zero;
    *(int*)((char*)this + 0x8e4) = zero;
    *(int*)((char*)this + 0x918) = zero;
    *(int*)((char*)this + 0x920) = zero;
    *(int*)((char*)this + 0x64) = *(int*)&f015;
    *(int*)((char*)this + 0x8ac) = zero;
    *(int*)((char*)this + 0x8b0) = *(int*)&f1;
    *(int*)((char*)this + 0xa0) = zero;
    *(int*)((char*)this + 0xa4) = zero;
    *(int*)((char*)this + 0x596c) = zero;
    *(int*)((char*)this + 0x5970) = zero;
    *(int*)((char*)this + 0x5974) = zero;
    *(int*)((char*)this + 0x8a8) = zero;
    *(int*)((char*)this + 0x7c) = *(int*)&f05;
    *(int*)((char*)this + 0x80) = zero;
    *(int*)((char*)this + 0x84) = zero;
    *(int*)((char*)this + 0x5980) = zero;
    *(int*)((char*)this + 0x5984) = -1;
    *(int*)((char*)this + 0x5988) = zero;
    *(int*)((char*)this + 0x598c) = zero;
    *(int*)((char*)this + 0x5998) = zero;
    *(int*)((char*)this + 0x8f4) = *(int*)&f10;
    *(int*)((char*)this + 0x5990) = *(int*)&f150;
    *(int*)((char*)this + 0x5994) = *(int*)&f60000;
    *(char*)((char*)this + 0x8b4) = 1;
    *(char*)((char*)this + 0x8b5) = 1;
    *(char*)((char*)this + 0x1c) = 0;
    if (*(void**)((char*)this + 0x59dc)) op_delete(*(void**)((char*)this + 0x59dc));
    *(int*)((char*)this + 0x59dc) = zero;
    *(int*)((char*)this + 0x59e0) = zero;
    *(int*)((char*)this + 0x59e4) = zero;
    *(char*)((char*)this + 0x8cc) = 0;
    *(int*)((char*)this + 0x8c8) = -1;
    if (*(void**)((char*)this + 0x8bc)) op_delete(*(void**)((char*)this + 0x8bc));
    *(int*)((char*)this + 0x8bc) = zero;
    *(int*)((char*)this + 0x8c0) = zero;
    *(int*)((char*)this + 0x8c4) = zero;
    *(char*)((char*)this + 0x5a04) = 0;
    *(int*)((char*)this + 0x94) = zero;
    *(int*)((char*)this + 0x98) = *(int*)&f1;
    *(int*)((char*)this + 0x958) = *(int*)&f1;
    *(float*)((char*)this + 0x59c4) = ((rand_() % 0x3dc9) * k_noise_scale + (rand_() % 0x3dc9) * k_noise_scale) * k_noise_w + k_noise_c;
    *(float*)((char*)this + 0x92c) = ((rand_() % 0x3dc9) * k_noise_scale - k_noise_c) * 2.0f;
    *(float*)((char*)this + 0x930) = ((rand_() % 0x3dc9) * k_noise_scale - k_noise_c) * 2.0f;
    *(int*)((char*)this + 0x59e8) = zero;
    *(int*)((char*)this + 0x59ec) = zero;
    *(int*)((char*)this + 0x59f0) = zero;
}

