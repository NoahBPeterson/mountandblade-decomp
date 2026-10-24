// mesh_appear.cpp: TU 63 "mesh", part 6 (0x482240..0x4832b0): the agent appearance builder. See MIGRATION.md.
#include "./mesh_slot.h"
#include "../../include/fx.h"
#include "../../include/troop.h"
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)

using namespace rgl;
// src/game/terrain_b/poly_mesh.h (pulled in through mesh_slot.h) declares a global ::vat with the same signature
#define vat rgl::vat

// ---- views of shared classes (members defined elsewhere in the image) --------------------------------------------------
struct lod2_a : lod_mesh2 {                      // the 0xe0-byte lod record
    lod2_a();                                    // 0x41c2a0
    void add(int level, mesh* m);                // 0x441ff0 (thiscall, ret 8)
    lod_mesh2* clone();                          // 0x47a370
    void set_color(unsigned color);              // 0x47b5b0 (ret 4)
    void transform(const matrix* m);             // 0x47b610 (ret 4)
    void append(lod_mesh2* src, int bone, const matrix* m, const unsigned* color);   // 0x47b690 (ret 16)
    void apply(const matrix* m);                 // 0x46c9f0 (ret 4)
    void set_mesh_color(const void* color);      // 0x47dd90 (ret 4)
};
struct ItemKindA : ItemKind {
    void get_carry_pos_n(tstring* out, int modifier);    // 0x433bf0 (thiscall, ret 8; the hidden-return mesh_name_for)
};
struct ItemSndA {
    void play(int id);                           // 0x48add0 (this = &ItemKind + 0x228)
};
struct FxDefA : FxDef {
    void attach(lod_mesh2* l);                   // 0x478fb0 (thiscall, ret 4: pushes onto the int list at +0x30)
};
struct EChar_a : EChar {
    void use_carry_a(lod_mesh2* l, const float* m);   // 0x47de60 (ret 8)
};
void append_mesh(int colour, int kind, lod_mesh2** ap, ItemRef* a, ItemRef* b, int carried, int* out);   // 0x481c40
extern int g_item_count;                         // g_item_kind_count 0x5e29d4
extern int g_cur;                                // 0x8ed4f68
extern EChar* g_chars;                           // 0x8ed4f7c
extern int g_char_count;                         // 0x8ed4f88
extern float g_dummy_mat;                        // 0x8ed4f8c
extern float g_lod_scale;                        // 0x54b890 (0.95)
extern __int64 g_hit_a;                          // 0xa70448
extern __int64 g_hit_b;                          // 0xa70450

// the player-visible fields of the agent body: a strategic entity (vtable 0x54b5d0) whose effect definition (+0x304) holds
// the skeleton elements; slot 6 of its vtable is set_fbc (the "anim" argument of the slice)

// the item set A10 of the caller: the carried items at +0x20.. (weapon, helmet, body, boots)
struct EquipSet {
    char pad0[0x20];
    ItemRef weapon;                              // +0x20
    ItemRef helmet;                              // +0x28
    ItemRef body;                                // +0x30
    ItemRef boots;                               // +0x38
};

template<class T> __forceinline void vpush_i(std::vector<T>& v, const T& x)
{
    if (v.size() < v.capacity()) {
        T* p = v._Mylast;
        *p = x;
        v._Mylast = p + 1;
    } else {
        v.insert(v.end(), x);
    }
}

// the name an item contributes (its carry position), or the error string when the item is invalid
static __forceinline void item_name(tstring* out, ItemRef* r, int modifier)
{
    if (r->item >= 0 && r->item < g_item_count)
        ((ItemKindA*)&g_items[r->item])->get_carry_pos_n(out, modifier);
    else
        *out = tstring("Error: Invalid item");
}

// the lod record that a single equipped item contributes (named, made into a lod mesh)
static __forceinline lod_mesh2* make_item_lod(ItemRef* r, int modifier)
{
    tstring name;
    item_name(&name, r, modifier);
    return g_rms.make_lod_mesh(name, false);
}

// tear down one owned lod record: the name, the five lists (which delete the meshes they own) and the block itself.
// Both the ap[] cleanup and the empty-*out_body prune in the original are this exact sequence (~mesh_list called out of
// line in one place and inlined in the other).
static __forceinline void lod_release(lod_mesh2* l)
{
    if (!l)
        return;
    l->name.~string();
    for (int i = 4; i >= 0; i--)
        l->lists[i].~mesh_list2();
    ::operator delete(l);
}

// a clone of one of the character's stored meshes (fields at +0x14..+0x24); the original bounds-checks the colour and then
// calls clone() on the (possibly null) pointer
static __forceinline lod_mesh2* clone_char_mesh(int colour, int off)
{
    lod2_a* src;
    if (colour < g_char_count)
        src = *(lod2_a**)((char*)g_chars + colour * 0x1f8 + off);
    else
        src = 0;
    return src->clone();
}

#define ELEM(i) ((FxElem*)((char*)a->fx->elems + (i) * 0x1dc))

// @ 0x482240
void build_appearance(strategic_entity* a, int hand, int troop, int kind, ItemRef* hweapon,
                      lod_mesh2** ap, int* stats, lod_mesh2** extra, int* extra_count,
                      lod_mesh2** out_body, EquipSet* eq, int hit_troop)
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
        body->res = g_resmgr.get_material(*(rgl::string*)&nm);
    }
    body->mask = 1;

    lod2_a* rec = new lod2_a;
    rec->add(0, body);
    vpush_i(a->lods, (lod_mesh2*)rec);
    ap[21] = rec;

    int mesh_color = 0;                             // written by append_mesh, tints the cloned meshes

    if (hand != 0) {
        // ---- only the hand-held weapon ------------------------------------------------------------
        tstring nm;
        if (hweapon->item >= 0 && hweapon->item < g_item_count)
            ((ItemKindA*)&g_items[hweapon->item])->get_carry_pos_n(&nm, ((signed char*)hweapon)[7]);
        else
            nm = tstring("Error: Invalid item");
        lod_mesh2* l = g_rms.make_lod_mesh(nm, false);
        vpush_i(a->fx->groups, l);
        float* e = (float*)a->fx->elems;
        *(float*)((char*)e + 0x944) = 0.2f;
        *(float*)((char*)e + 0xb20) = 0.25f;
        *(float*)((char*)e + 0xcfc) = 0.25f;
        *(float*)((char*)e + 0xed8) = 0.3f;
        float f = 3.1f;
        a->set_fbc(&f);
        a->fc0 = 2.1f;
        a->vc4.x = 0.0f;
        a->vc4.y = 0.0f;
        a->vc4.z = 1.3f;
        a->vc4.w = 0.0f;
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

        char* tr = *(char**)(g_game_base + 0x13a00) + troop * 0x508;
        int colour = *(unsigned*)(tr + 0x30) & 0xf;
        append_mesh(colour, kind, ap, (ItemRef*)(tr + 0x3c0), (ItemRef*)(tr + 0x3c0 + 4 * 8), carried, &mesh_color);

        // the identity's 3x3, scaled by 0.95 (the item mesh frame)
        float mtx[16];
        identity3((mat3*)mtx);
        mtx[12] = mtx[13] = mtx[14] = mtx[15] = 0.0f;
        mtx[0] *= g_lod_scale;  mtx[1] *= g_lod_scale;  mtx[2]  *= g_lod_scale;
        mtx[4] *= g_lod_scale;  mtx[5] *= g_lod_scale;  mtx[6]  *= g_lod_scale;
        mtx[8] *= g_lod_scale;  mtx[9] *= g_lod_scale;  mtx[10] *= g_lod_scale;

        if (w->item < 0) {
            // no weapon: just place the previously-owned meshes on bone 9
            ((lod2_a*)ap[0])->transform((matrix*)mtx);
            ELEM(9)->add_group((int)ap[0]);
            if (ap[2]) {
                ((lod2_a*)ap[2])->transform((matrix*)mtx);
                ELEM(9)->add_group((int)ap[2]);
            }
            if (ap[1]) {
                ((lod2_a*)ap[1])->transform((matrix*)mtx);
                ELEM(9)->add_group((int)ap[1]);
            }
        } else {
            lod_mesh2* wl = make_item_lod(w, w->m4 >> 24);
            unsigned wflags = g_items[w->item].type_flags;
            if ((wflags & 0x80000000) == 0)
                ((EChar_a*)&g_chars[g_cur])->use_carry_a(wl, &g_dummy_mat);
            ((lod2_a*)wl)->apply((matrix*)mtx);
            if ((wflags & 0xf00) == 0xf00)
                ((FxDefA*)a->fx)->attach(wl);
            else
                ELEM(9)->add_group((int)wl);
            int c3 = (wflags & 0x80000000) ? 1 : legs_covered;
            ap[3] = wl;
            if (c3 == 0) {
                ((lod2_a*)ap[0])->transform((matrix*)mtx);
                ELEM(9)->add_group((int)ap[0]);
                if (ap[2]) {
                    ((lod2_a*)ap[2])->transform((matrix*)mtx);
                    ELEM(9)->add_group((int)ap[2]);
                }
                if (ap[1]) {
                    ((lod2_a*)ap[1])->transform((matrix*)mtx);
                    ELEM(9)->add_group((int)ap[1]);
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
            lod2_a* body_rec = new lod2_a;
            *out_body = body_rec;

            if (h->item < 0) {
                lod_mesh2* c = clone_char_mesh(colour, 0x14);
                ((lod2_a*)c)->set_color((unsigned)mesh_color);
                vpush_i(a->fx->groups, c);
                legs_covered = 1;
            } else {
                if (g_items[h->item].type_flags & 0x1000000)
                    legs_covered = 1;
                lod_mesh2* hl = make_item_lod(h, h->m4 >> 24);
                if (colour < g_char_count) {
                    int cv = (int)((*(unsigned*)((char*)&g_chars[colour] + 0x10) & 7) * 10);
                    if (cv != 0) {
                        float cf = (float)cv;
                        ((lod2_a*)hl)->set_mesh_color(&cf);
                    }
                }
                if (*(int*)((char*)&g_items[h->item] + 0x228) > 0) {
                    *(int*)(g_game_base + 0x1a1f4) = hit_troop;
                    *(int*)(g_game_base + 0x1a1f8) = troop;
                    *(int*)(g_game_base + 0x1a204) = 5;
                    *(int*)(g_game_base + 0x1a1fc) = (int)hl;
                    *(int*)(g_game_base + 0x1a200) = 0;
                    g_hit_a = hit_troop;
                    g_hit_b = troop;
                    ((ItemSndA*)((char*)&g_items[h->item] + 0x228))->play(-50);
                }
                vpush_i(a->fx->groups, hl);
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
                ap[17] = g_rms.make_lod_mesh(nm, false);

                tstring lx(nm);
                lx.push_back('x');
                if (g_rms.find_v20(*(rgl::string*)&lx)) {
                    if (g_use_shaders == 0) {
                        lod_mesh2* Lx = g_rms.make_lod_mesh(lx, false);
                        ap[19] = Lx;
                        ELEM(12)->add_group((int)Lx);
                    } else {
                        unsigned col = 0xffffffff;
                        ((lod2_a*)*out_body)->append(g_rms.get_lod_mesh(lx), 12,
                                            (matrix*)((char*)a->fx->elems + 0x1654), &col);
                    }
                }

                nm.buf[nm.len - 1] = 'R';
                ap[18] = g_rms.make_lod_mesh(nm, false);

                tstring rx(nm);
                rx.push_back('x');
                if (g_rms.find_v20(*(rgl::string*)&rx)) {
                    if (g_use_shaders == 0) {
                        lod_mesh2* Rx = g_rms.make_lod_mesh(rx, false);
                        ap[20] = Rx;
                        ELEM(17)->add_group((int)Rx);
                    } else {
                        unsigned col = 0xffffffff;
                        ((lod2_a*)*out_body)->append(g_rms.get_lod_mesh(rx), 17,
                                            (matrix*)((char*)a->fx->elems + 0x1fa0), &col);
                    }
                }
            } else {
                ap[17] = clone_char_mesh(colour, 0x18);
                ap[18] = clone_char_mesh(colour, 0x1c);
                ((lod2_a*)ap[17])->set_color((unsigned)mesh_color);
                ((lod2_a*)ap[18])->set_color((unsigned)mesh_color);
            }
            ELEM(13)->add_group((int)ap[17]);
            ELEM(18)->add_group((int)ap[18]);
        }

        // ---- default trousers when nothing covers the legs ---------------------------------------
        if (!legs_covered) {
            lod_mesh2* tr2 = g_rms.make_lod_mesh(tstring("def_trousers"), false);
            vpush_i(a->fx->groups, tr2);
        }

        // ---- the body armour ---------------------------------------------------------------------
        {
            ItemRef* bd = &eq->body;
            if (bd->item < 0) {
                lod_mesh2* c1 = clone_char_mesh(colour, 0x20);
                lod_mesh2* c2 = clone_char_mesh(colour, 0x24);
                ((lod2_a*)c1)->set_color((unsigned)mesh_color);
                ((lod2_a*)c2)->set_color((unsigned)mesh_color);
                ELEM(2)->add_group((int)c1);
                ELEM(5)->add_group((int)c2);
            } else {
                stats[0] += bd->stat_1fc();
                stats[1] += bd->stat_200();
                stats[2] += bd->stat_204();

                tstring nm;
                item_name(&nm, bd, bd->m4 >> 24);
                if (nm.buf[nm.len - 1] == 'L') {
                    lod_mesh2* L = g_rms.get_lod_mesh(nm);
                    nm.buf[nm.len - 1] = 'R';
                    lod_mesh2* R = g_rms.get_lod_mesh(nm);
                    int b1 = 2, b2 = 5;
                    if ((g_items[bd->item].type_flags & 0xf00) != 0) {
                        b1 = -1;
                        b2 = -1;
                    }
                    unsigned col = 0xffffffff;
                    ((lod2_a*)*out_body)->append(L, b1, (matrix*)((char*)a->fx->elems + 4 + b1 * 0x1dc), &col);
                    col = 0xffffffff;
                    ((lod2_a*)*out_body)->append(R, b2, (matrix*)((char*)a->fx->elems + 4 + b2 * 0x1dc), &col);
                } else {
                    lod_mesh2* c = ((lod2_a*)g_rms.get_lod_mesh(nm))->clone();
                    ((FxDefA*)a->fx)->attach(c);
                }
            }
        }

        float f = 3.1f;
        a->set_fbc(&f);
        a->fc0 = 1.1f;
        a->vc4.x = 0.0f;
        a->vc4.y = 0.0f;
        a->vc4.z = 1.1f;
        a->vc4.w = 0.0f;
    }

    // ---- common tail -----------------------------------------------------------------------------
    for (int i = 0; i < *extra_count; i++)
        vpush_i(a->fx->psys, (mesh*)extra[i]);

    {
        lod_mesh2* br = *out_body;
        if (br) {
            if (br->count == 0)
                lod_release(br);
            else
                vpush_i(a->fx->groups, br);
        }
    }

    for (int i = 0; i < 0x15; i++) {
        if (ap[i])
            *(unsigned*)((char*)ap[i] + 0xcc) = 0xffff;
    }
}
