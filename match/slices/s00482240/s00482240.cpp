// Slice s00482240: the agent appearance builder (0x482240, 4206 bytes).
//
// Given `troop` (an index into the active character table 0x8ed4f7c), the carried-item set `eq`
// (weapon +0x20, helmet +0x28, body +0x30, boots +0x38) and, when `hand` is non-zero, a weapon to
// put in the hand, it builds the body mesh with its 0xe0-byte lod record, makes one lod mesh per
// equipped item (colouring them with the character's colour and registering them on the skeleton),
// and hands the resulting lod table back through `ap` / `out_body`.  ALL NAMES ARE PROVISIONAL.
//
// Args (cdecl, 12 of them; offsets from the return address):
//   A0  E+0x04  BodyMesh*   a            the agent's body (skel at +0x304, a lod list at +0x108)
//   A1  E+0x08  int         hand         != 0: only put `hweapon` in the hand, skip the whole body
//   A2  E+0x0c  int         troop        index into g_game->chars (record size 0x508)
//   A3  E+0x10  int         kind         forwarded to 0x481c40
//   A4  E+0x14  ItemRef*    hweapon      the item the hand should hold (hand != 0 only)
//   A5  E+0x18  lod_mesh**  ap           22-slot lod table (index 3 weapon, 17..20 boots, 21 body)
//   A6  E+0x1c  int*        stats        three zeroed skill words, accumulated from each item
//   A7  E+0x20  lod_mesh**  extra        lod meshes pushed onto the skeleton's particle list
//   A8  E+0x24  int*        extra_count  count for `extra` (zeroed at entry)
//   A9  E+0x28  lod_mesh**  out_body     receives the new body lod record
//   A10 E+0x2c  EquipSet*   eq           the carried-item set
//   A11 E+0x30  int         hit_troop    written into the game's last-hit record (with A2)
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <new>
#include "rgl.h"

// ---------------------------------------------------------------------------------------------------
// engine types (layouts recovered from this function and the neighbour slices; see the slice header)

struct tstring {                                    // 16 bytes
    int pos; char* buf; int cap; int len;
    tstring();                                      // 0x42bbd0
    tstring(const char* s);                         // 0x42bbe0
    tstring(const tstring& o);                      // 0x42c110
    ~tstring();                                     // 0x42bc40
    tstring& operator=(const tstring& o);           // 0x42bc50
    void push_back(char c);                         // 0x42be70
};

// rgl's owned pointer list: a 4-byte tag followed by the std::vector, so the vector starts at +4
// (its insert/erase run on the *container* address -- see ~mesh_list 0x419420 and insert 0x41a2c0).
template<class T> struct ptr_vec {                  // 0x10 bytes
    int tag;                                        // +0
    std::vector<T> v;                               // +4
    __forceinline void push_back(const T& x)        // the fast path the original always inlines
    {
        if (v.size() < v.capacity()) {
            T* p = v._Mylast;
            *p = x;
            v._Mylast = p + 1;
        } else {
            v.insert(v.end(), x);
        }
    }
};

struct lod_mesh;
struct mesh;

struct mesh_list : ptr_vec<mesh*> {                 // 0x10 bytes: a lod record's owned mesh list
    ~mesh_list();                                   // 0x419420 (deletes each mesh, frees the array)
};

struct lod_mesh {                                   // 0xe0 bytes (ctor 0x41c2a0)
    char pad0[0x34];
    mesh_list lists[5];                             // +0x34 (each 0x10, vector at +0x38...)
    int count;                                      // +0x84
    int cur;                                        // +0x88
    char pad8c[0xd0 - 0x8c];
    tstring name;                                   // +0xd0
    lod_mesh();
    void add(int level, mesh* m);                   // 0x441ff0 (thiscall, ret 8)
    lod_mesh* clone();                              // 0x47a370
    void set_color(unsigned color);                 // 0x47b5b0 (ret 4)
    void transform(const float* m);                 // 0x47b610 (ret 4)
    void append(lod_mesh* src, int bone, const float* m, const unsigned* color);   // 0x47b690 (ret 16)
    void apply(const float* m);                     // 0x46c9f0 (ret 4)
    void set_mesh_color(const void* color);         // 0x47dd90 (ret 4)
};

struct mesh {                                       // the 0x260-byte resource mesh (ctor 0x412a90)
    virtual ~mesh();                                // 0x417730
    char pad0[0x2c - 0x04];                         // +0x04 (base name, name2, parent)
    void* res;                                      // +0x2c
    int f30;                                        // +0x30
    int f34;                                        // +0x34 (set to 2)
    char pad38[0x3c - 0x38];
    unsigned color;                                 // +0x3c
    char pad40[0x10c - 0x40];
    int mask;                                       // +0x10c (set to 1)
    char pad110[0x260 - 0x110];
    mesh();                                         // 0x412a90
};

struct Skeleton;

// the agent body object A0: polymorphic, a lod list at +0x108, the animator at +0x304
struct BodyMesh {
    virtual void v00(); virtual void v04(); virtual void v08();
    virtual void v0c(); virtual void v10(); virtual void v14();
    virtual void set_anim(float* f);                // vtable slot 0x18
    char pad0[0xc0 - 0x04];
    float f_c0;                                     // +0xc0
    float c4, c8, cc, d0;                           // +0xc4: a vector4
    char pad2[0x108 - 0xd4];
    ptr_vec<lod_mesh*> lods;                        // +0x108 (vector at +0x10c)
    char pad3[0x304 - 0x118];
    Skeleton* skel;                                 // +0x304
};

struct SkelElem {                                   // one 0x1dc-byte animator element
    char pad[0x1dc];
    void add_group(void* p);                        // 0x4423f0 (thiscall, ret 4)
};

struct Skeleton {
    int count;                                      // +0
    char* elems;                                    // +4 (0x1dc-byte records)
    char pad8[0x20 - 8];
    ptr_vec<lod_mesh*> psys;                        // +0x20 (vector at +0x24)
    ptr_vec<lod_mesh*> groups;                      // +0x30 (vector at +0x34)
    void attach(lod_mesh* l);                       // 0x478fb0 (thiscall, ret 4)
    __forceinline SkelElem* elem(int i) { return (SkelElem*)(elems + i * 0x1dc); }
};

struct ItemKind {                                   // 0x238 bytes (array at 0x5e29d0)
    char pad0[0x1c8];
    unsigned type_flags;                            // +0x1c8
    char pad1cc[0x228 - 0x1cc];
    int sound;                                      // +0x228
    char pad22c[0x238 - 0x22c];
    void get_carry_pos(tstring* out, int modifier); // 0x433bf0 (thiscall, ret 8)
};

struct ItemSound { void play(int id); };            // 0x48add0 (this = &ItemKind + 0x228)

struct ItemRef {                                    // 8 bytes
    int item;
    int m4;
    int stat_1fc();                                 // 0x462960 (thiscall, ret 0)
    int stat_200();                                 // 0x434eb0
    int stat_204();                                 // 0x462a10
};

struct EquipSet {                                   // the carried-item set A10
    char pad0[0x20];
    ItemRef weapon;                                 // +0x20
    ItemRef helmet;                                 // +0x28
    ItemRef body;                                   // +0x30
    ItemRef boots;                                  // +0x38
};

struct EChar {                                      // 0x1f8-byte character record (table 0x8ed4f7c)
    char pad0[0x10];
    unsigned flags10;                               // +0x10 (low 3 bits: colour index)
    lod_mesh* l14;                                  // +0x14 the character's own body parts
    lod_mesh* l18;                                  // +0x18
    lod_mesh* l1c;                                  // +0x1c
    lod_mesh* l20;                                  // +0x20
    lod_mesh* l24;                                  // +0x24
    char pad28[0x1f8 - 0x28];
    void use_carry(lod_mesh* l, const float* m);    // 0x47de60 (thiscall, ret 8)
};

struct TroopRec {                                   // 0x508 bytes (g_game->chars)
    char pad0[0x30];
    unsigned f30;                                   // +0x30
    char pad34[0x3c0 - 0x34];
    ItemRef items[10];                              // +0x3c0 (items[0]/items[4] go to 0x481c40)
};

struct GameData {
    char pad0[0x13a00];
    TroopRec* chars;                                // +0x13a00
    char pad13a04[0x1a1f4 - 0x13a04];
    int last_hit_troop;                             // +0x1a1f4
    int last_hit_troop2;                            // +0x1a1f8
    int last_hit_mesh;                              // +0x1a1fc
    int last_hit_zero;                              // +0x1a200
    int last_hit_type;                              // +0x1a204
};

struct ResMgr {
    void* lookup(const tstring& name);              // 0x418a70 (thiscall, ret 4)
    mesh* find(const tstring* name);                // 0x418bd0
    lod_mesh* make_lod_mesh(const tstring& name, bool b);   // 0x41ca60 (ret 8)
    lod_mesh* get_lod_mesh(const tstring& name);    // 0x41cdc0 (ret 4)
};

extern ResMgr g_resmgr;                             // 0x5d6fa0
extern ItemKind* g_items;                           // 0x5e29d0
extern int g_item_count;                            // 0x5e29d4
extern int g_resource_flag;                         // 0x5e28fc
extern GameData* g_game;                            // 0x5e2930
extern int g_cur;                                   // 0x8ed4f68
extern EChar* g_chars;                              // 0x8ed4f7c
extern int g_char_count;                            // 0x8ed4f88
extern float g_dummy_mat;                           // 0x8ed4f8c
extern float g_lod_scale;                           // 0x54b890 (0.95)
extern __int64 g_hit_a;                             // 0xa70448
extern __int64 g_hit_b;                             // 0xa70450

float* identity3(float* out);                       // 0x40cd60 (cdecl; the 3x4 identity)
void append_mesh(int colour, int kind, lod_mesh** ap, ItemRef* a, ItemRef* b, int carried, int* out);   // 0x481c40

// ---------------------------------------------------------------------------------------------------
// helpers

// the name an item contributes (its carry position), or the error string when the item is invalid
static __forceinline void item_name(tstring* out, ItemRef* r, int modifier)
{
    if (r->item >= 0 && r->item < g_item_count)
        g_items[r->item].get_carry_pos(out, modifier);
    else
        *out = tstring("Error: Invalid item");
}

// the lod record that a single equipped item contributes (named, made into a lod mesh)
static __forceinline lod_mesh* make_item_lod(ItemRef* r, int modifier)
{
    tstring name;
    item_name(&name, r, modifier);
    return g_resmgr.make_lod_mesh(name, false);
}

// tear down one owned lod record: the name, the five lists (which delete the meshes they own) and the
// block itself.  Both the ap[] cleanup and the empty-*out_body prune in the original are this exact
// sequence (~mesh_list called out of line in one place and inlined in the other).
static __forceinline void lod_release(lod_mesh* l)
{
    if (!l)
        return;
    l->name.~tstring();
    for (int i = 4; i >= 0; i--)
        l->lists[i].~mesh_list();
    ::operator delete(l);
}

// a clone of one of the character's stored meshes (fields at +0x14..+0x24); the original bounds-checks
// the colour and then calls clone() on the (possibly null) pointer
static __forceinline lod_mesh* clone_char_mesh(int colour, int off)
{
    lod_mesh* src;
    if (colour < g_char_count)
        src = *(lod_mesh**)((char*)g_chars + colour * 0x1f8 + off);
    else
        src = 0;
    return src->clone();
}

// ---------------------------------------------------------------------------------------------------
// @ 0x482240
void build_appearance(BodyMesh* a, int hand, int troop, int kind, ItemRef* hweapon,
                      lod_mesh** ap, int* stats, lod_mesh** extra, int* extra_count,
                      lod_mesh** out_body, EquipSet* eq, int hit_troop)
{
    *extra_count = 0;
    stats[0] = 0;
    stats[1] = 0;
    stats[2] = 0;

    char legs_covered = 0;

    mesh* body = new mesh;
    body->f34 = 2;
    {
        tstring nm("agent_name");
        body->res = g_resmgr.lookup(nm);
    }
    body->mask = 1;

    lod_mesh* rec = new lod_mesh;
    rec->add(0, body);
    a->lods.push_back(rec);
    ap[21] = rec;

    int mesh_color = 0;                             // written by append_mesh, tints the cloned meshes

    if (hand != 0) {
        // ---- only the hand-held weapon ------------------------------------------------------------
        tstring nm;
        if (hweapon->item >= 0 && hweapon->item < g_item_count)
            g_items[hweapon->item].get_carry_pos(&nm, ((signed char*)hweapon)[7]);
        else
            nm = tstring("Error: Invalid item");
        lod_mesh* l = g_resmgr.make_lod_mesh(nm, false);
        a->skel->groups.push_back(l);
        float* e = (float*)a->skel->elems;
        *(float*)((char*)e + 0x944) = 0.2f;
        *(float*)((char*)e + 0xb20) = 0.25f;
        *(float*)((char*)e + 0xcfc) = 0.25f;
        *(float*)((char*)e + 0xed8) = 0.3f;
        float f = 3.1f;
        a->set_anim(&f);
        a->f_c0 = 2.1f;
        a->c4 = 0.0f;
        a->c8 = 0.0f;
        a->cc = 1.3f;
        a->d0 = 0.0f;
    } else {
        // ---- the character's weapon ---------------------------------------------------------------
        ItemRef* w = &eq->weapon;
        if (w->item >= 0) {
            stats[0] += w->stat_1fc();
            stats[1] += w->stat_200();
            stats[2] += w->stat_204();
        }
        int carried = 0;
        if (w->item >= 0 && (g_items[w->item].type_flags & 0x1000000) == 0)
            carried = 1;

        TroopRec* tr = &g_game->chars[troop];
        int colour = tr->f30 & 0xf;
        append_mesh(colour, kind, ap, &tr->items[0], &tr->items[4], carried, &mesh_color);

        // the identity's 3x3, scaled by 0.95 (the item mesh frame)
        float mtx[16];
        identity3(mtx);
        mtx[12] = mtx[13] = mtx[14] = mtx[15] = 0.0f;
        mtx[0] *= g_lod_scale;  mtx[1] *= g_lod_scale;  mtx[2]  *= g_lod_scale;
        mtx[4] *= g_lod_scale;  mtx[5] *= g_lod_scale;  mtx[6]  *= g_lod_scale;
        mtx[8] *= g_lod_scale;  mtx[9] *= g_lod_scale;  mtx[10] *= g_lod_scale;

        if (w->item < 0) {
            // no weapon: just place the previously-owned meshes on bone 9
            ap[0]->transform(mtx);
            a->skel->elem(9)->add_group(ap[0]);
            if (ap[2]) {
                ap[2]->transform(mtx);
                a->skel->elem(9)->add_group(ap[2]);
            }
            if (ap[1]) {
                ap[1]->transform(mtx);
                a->skel->elem(9)->add_group(ap[1]);
            }
        } else {
            lod_mesh* wl = make_item_lod(w, w->m4 >> 24);
            unsigned wflags = g_items[w->item].type_flags;
            if ((wflags & 0x80000000) == 0)
                g_chars[g_cur].use_carry(wl, &g_dummy_mat);
            wl->apply(mtx);
            if ((wflags & 0xf00) == 0xf00)
                a->skel->attach(wl);
            else
                a->skel->elem(9)->add_group(wl);
            int c3 = (wflags & 0x80000000) ? 1 : legs_covered;
            ap[3] = wl;
            if (c3 == 0) {
                ap[0]->transform(mtx);
                a->skel->elem(9)->add_group(ap[0]);
                if (ap[2]) {
                    ap[2]->transform(mtx);
                    a->skel->elem(9)->add_group(ap[2]);
                }
                if (ap[1]) {
                    ap[1]->transform(mtx);
                    a->skel->elem(9)->add_group(ap[1]);
                }
            } else {
                if (ap[0]) lod_release(ap[0]);
                if (ap[1]) lod_release(ap[1]);
                if (ap[2]) lod_release(ap[2]);
                ap[0] = ap[1] = ap[2] = 0;
            }
        }

        // ---- the helmet --------------------------------------------------------------------------
        {
            ItemRef* h = &eq->helmet;
            if (h->item >= 0) {
                stats[0] += h->stat_1fc();
                stats[1] += h->stat_200();
                stats[2] += h->stat_204();
            }
            lod_mesh* body_rec = new lod_mesh;
            *out_body = body_rec;

            if (h->item < 0) {
                lod_mesh* c = clone_char_mesh(colour, 0x14);
                c->set_color((unsigned)mesh_color);
                a->skel->groups.push_back(c);
                legs_covered = 1;
            } else {
                if (g_items[h->item].type_flags & 0x1000000)
                    legs_covered = 1;
                lod_mesh* hl = make_item_lod(h, h->m4 >> 24);
                if (colour < g_char_count) {
                    int cv = (int)((g_chars[colour].flags10 & 7) * 10);
                    if (cv != 0) {
                        float cf = (float)cv;
                        hl->set_mesh_color(&cf);
                    }
                }
                if (g_items[h->item].sound > 0) {
                    g_game->last_hit_troop = hit_troop;
                    g_game->last_hit_troop2 = troop;
                    g_game->last_hit_type = 5;
                    g_game->last_hit_mesh = (int)hl;
                    g_game->last_hit_zero = 0;
                    g_hit_a = hit_troop;
                    g_hit_b = troop;
                    ((ItemSound*)((char*)&g_items[h->item] + 0x228))->play(-50);
                }
                a->skel->groups.push_back(hl);
            }
        }

        // ---- the boots (the L/R pair plus the lit Lx/Rx pair) ------------------------------------
        {
            ItemRef* b = &eq->boots;
            if (b->item >= 0) {
                stats[0] += b->stat_1fc();
                stats[1] += b->stat_200();
                stats[2] += b->stat_204();

                tstring nm;
                item_name(&nm, b, b->m4 >> 24);
                nm.buf[nm.len - 1] = 'L';
                ap[17] = g_resmgr.make_lod_mesh(nm, false);

                tstring lx(nm);
                lx.push_back('x');
                if (g_resmgr.find(&lx)) {
                    if (g_resource_flag == 0) {
                        lod_mesh* Lx = g_resmgr.make_lod_mesh(lx, false);
                        ap[19] = Lx;
                        a->skel->elem(12)->add_group(Lx);
                    } else {
                        unsigned col = 0xffffffff;
                        (*out_body)->append(g_resmgr.get_lod_mesh(lx), 12,
                                            (float*)(a->skel->elems + 0x1654), &col);
                    }
                }

                nm.buf[nm.len - 1] = 'R';
                ap[18] = g_resmgr.make_lod_mesh(nm, false);

                tstring rx(nm);
                rx.push_back('x');
                if (g_resmgr.find(&rx)) {
                    if (g_resource_flag == 0) {
                        lod_mesh* Rx = g_resmgr.make_lod_mesh(rx, false);
                        ap[20] = Rx;
                        a->skel->elem(17)->add_group(Rx);
                    } else {
                        unsigned col = 0xffffffff;
                        (*out_body)->append(g_resmgr.get_lod_mesh(rx), 17,
                                            (float*)(a->skel->elems + 0x1fa0), &col);
                    }
                }
            } else {
                ap[17] = clone_char_mesh(colour, 0x18);
                ap[18] = clone_char_mesh(colour, 0x1c);
                ap[17]->set_color((unsigned)mesh_color);
                ap[18]->set_color((unsigned)mesh_color);
            }
            a->skel->elem(13)->add_group(ap[17]);
            a->skel->elem(18)->add_group(ap[18]);
        }

        // ---- default trousers when nothing covers the legs ---------------------------------------
        if (!legs_covered) {
            lod_mesh* tr = g_resmgr.make_lod_mesh(tstring("def_trousers"), false);
            a->skel->groups.push_back(tr);
        }

        // ---- the body armour ---------------------------------------------------------------------
        {
            ItemRef* bd = &eq->body;
            if (bd->item < 0) {
                lod_mesh* c1 = clone_char_mesh(colour, 0x20);
                lod_mesh* c2 = clone_char_mesh(colour, 0x24);
                c1->set_color((unsigned)mesh_color);
                c2->set_color((unsigned)mesh_color);
                a->skel->elem(2)->add_group(c1);
                a->skel->elem(5)->add_group(c2);
            } else {
                stats[0] += bd->stat_1fc();
                stats[1] += bd->stat_200();
                stats[2] += bd->stat_204();

                tstring nm;
                item_name(&nm, bd, bd->m4 >> 24);
                if (nm.buf[nm.len - 1] == 'L') {
                    lod_mesh* L = g_resmgr.get_lod_mesh(nm);
                    nm.buf[nm.len - 1] = 'R';
                    lod_mesh* R = g_resmgr.get_lod_mesh(nm);
                    int b1 = 2, b2 = 5;
                    if ((g_items[bd->item].type_flags & 0xf00) != 0) {
                        b1 = -1;
                        b2 = -1;
                    }
                    unsigned col = 0xffffffff;
                    (*out_body)->append(L, b1, (float*)(a->skel->elems + 4 + b1 * 0x1dc), &col);
                    col = 0xffffffff;
                    (*out_body)->append(R, b2, (float*)(a->skel->elems + 4 + b2 * 0x1dc), &col);
                } else {
                    lod_mesh* c = g_resmgr.get_lod_mesh(nm)->clone();
                    a->skel->attach(c);
                }
            }
        }

        float f = 3.1f;
        a->set_anim(&f);
        a->f_c0 = 1.1f;
        a->c4 = 0.0f;
        a->c8 = 0.0f;
        a->cc = 1.1f;
        a->d0 = 0.0f;
    }

    // ---- common tail -----------------------------------------------------------------------------
    for (int i = 0; i < *extra_count; i++)
        a->skel->psys.push_back(extra[i]);

    {
        lod_mesh* br = *out_body;
        if (br) {
            if (br->count == 0)
                lod_release(br);
            else
                a->skel->groups.push_back(br);
        }
    }

    for (int i = 0; i < 0x15; i++) {
        if (ap[i])
            *(unsigned*)((char*)ap[i] + 0xcc) = 0xffff;
    }
}
