// agent_appearance.cpp: TU 63 "mesh", part 6 (0x482240..0x4832b0): the agent appearance builder. See MIGRATION.md.
#include "game/mission/mesh_slot.h"
#include "game/mission/mesh_build.h"          // build_appearance (0x482240), setup_item_meshes (0x47cfe0)
#include "engine/fx/fx.h"
#include "game/data/troop.h"
#include "game/agent/animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "game/data/game_data.h"   // GameData, g_game (0x5e2930)
#include "game/data/trigger_vecs.h"          // SimpleTriggerVec: the item's sound triggers (update 0x48add0)
#include "game/data/item_kind_include.h"   // g_item_kind_count 0x5e29d4
#include "engine/render/render_flags.h"
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object

namespace rgl { struct mat3; }

using namespace rgl;
// src/game/terrain/poly_mesh.h (pulled in through mesh_slot.h) declares a global ::vat with the same signature
#define vat rgl::vat

// the identity's 3x3 (src/engine/math/rgl_math.h; defined in engine/physics/collision_shapes.cpp)
mat3* identity3(mat3* out);                     // 0x40cd60

// (the lod mesh and its members are engine/render/mesh.h's rgl::lod_mesh; the item sound holder is game/data/trigger_vecs.h's
// SimpleTriggerVec, update 0x48add0)
Skin* append_mesh(int colour, int kind, lod_mesh** ap, const PackedState* a, const PackedState* b, char carried, int* out);   // 0x481c40 (terrain_water_regen.cpp)
extern int g_item_kind_count;                         // g_item_kind_count 0x5e29d4
extern float g_lod_scale;                        // 0x54b890 (0.95)

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


// the name an item contributes (its carry position), or the error string when the item is invalid. The original builds the
// name straight into the caller's slot from either branch (0x433bf0's hidden return pointer, or 0x42bbe0 on the literal):
// each return constructs the result in place, no copy.
static __forceinline tstring item_name(ItemRef* r, int modifier)
{
    if (r->item >= 0 && r->item < g_item_kind_count)
        return g_items[r->item].mesh_name_for(modifier);       // 0x433bf0
    return tstring("Error: Invalid item");                     // 0x42bbe0
}

// the lod record that a single equipped item contributes (named, made into a lod mesh)
static __forceinline lod_mesh* make_item_lod(ItemRef* r, int modifier)
{
    tstring name = item_name(r, modifier);
    return g_resmgr.make_lod_mesh(name, false);
}

// a clone of one of the skin's stored meshes (body, hand_l, hand_r, calf_l, calf_r: fields +0x14..+0x24); the original bounds-checks
// the colour and then calls clone() on the (possibly null) pointer
static __forceinline lod_mesh* clone_char_mesh(int colour, lod_mesh* Skin::* mesh)
{
    lod_mesh* src;
    if ((unsigned)colour < (unsigned)g_anim.char_count)          // 0x482852/0x48297f: cmp + jae (unsigned)
        src = g_anim.skin_vec[colour].*mesh;
    else
        src = 0;
    return src->clone();
}


// @ 0x482240
void build_appearance(strategic_entity* a, int hand, int troop, int kind, ItemRef* hweapon,
                      lod_mesh** ap, int* stats, mesh** extra, int* extra_count,
                      lod_mesh** out_body, EquipSet* eq, int hit_troop)
{
    *extra_count = 0;
    stats[0] = stats[1] = stats[2] = 0;

    char legs_covered = 0;

    mesh* body = new mesh;
    body->billboard = 2;
    {
        tstring nm("agent_name");
        body->res = g_resmgr.get_material(nm);
    }
    body->mask = 1;

    lod_mesh* rec = new lod_mesh;
    rec->add_mesh(0, body);
    a->lods.push_back(rec);
    ap[21] = rec;

    int mesh_color = 0;                             // written by append_mesh, tints the cloned meshes

    if (!(hand != 0)) {
        // ---- the character's weapon ---------------------------------------------------------------
        ItemRef* w = &eq->weapon;
        if (w->item >= 0) {
            stats[0] += w->head_armor();
            stats[1] += w->body_armor();
            stats[2] += w->leg_armor();
        }
        int carried = 0;
        if (w->item >= 0 && (g_items[w->item].type_flags & 0x1000000) == 0)
            carried = 1;

        Troop* tr = &g_game->troops[troop];
        int colour = tr->flags32 & 0xf;
        append_mesh(colour, kind, ap, &tr->face_keys, &tr->face_keys_b, (char)carried, &mesh_color);

        // the identity's 3x3, scaled by 0.95 (the item mesh frame): identity3 fills a mat3 temporary, which is
        // copied into the frame's first 48 bytes (12 dwords); the origin is a vector4(0,0,0) temporary
        mat3 id;
        matrix mtx;
        rgl::identity3(&id);
        *((mat3*)&mtx) = id;
        mtx.o = vector4(0.0f, 0.0f, 0.0f);
        mtx.s.x *= 0.95f;  mtx.s.y *= 0.95f;  mtx.s.z *= 0.95f;
        mtx.f.x *= 0.95f;  mtx.f.y *= 0.95f;  mtx.f.z *= 0.95f;
        mtx.u.x *= 0.95f;  mtx.u.y *= 0.95f;  mtx.u.z *= 0.95f;

        if (w->item < 0) {
            // no weapon: just place the previously-owned meshes on bone 9
            ap[0]->transform(mtx);
            a->fx->elems[9].add_group(ap[0]);
            if (ap[2]) {
                ap[2]->transform(mtx);
                a->fx->elems[9].add_group(ap[2]);
            }
            if (ap[1]) {
                ap[1]->transform(mtx);
                a->fx->elems[9].add_group(ap[1]);
            }
        } else {
            lod_mesh* wl = make_item_lod(w, w->m4 >> 24);
            unsigned wflags = g_items[w->item].type_flags;
            if ((wflags & 0x80000000) == 0)
                g_anim.skin_vec[g_anim.cur].use_carry(wl, g_anim.weights);      // 0x47de60
            wl->apply(&mtx);
            if ((wflags & 0xf00) == 0xf00)
                a->fx->add_group(wl);                       // 0x478fb0
            else
                a->fx->elems[9].add_group(wl);
            int c3 = (wflags & 0x80000000) ? 1 : legs_covered;
            ap[3] = wl;
            if (!(c3 == 0)) {
                delete ap[0];
                delete ap[1];
                delete ap[2];
                ap[0] = ap[1] = ap[2] = 0;
            } else {
                ap[0]->transform(mtx);
                a->fx->elems[9].add_group(ap[0]);
                if (ap[2]) {
                    ap[2]->transform(mtx);
                    a->fx->elems[9].add_group(ap[2]);
                }
                if (ap[1]) {
                    ap[1]->transform(mtx);
                    a->fx->elems[9].add_group(ap[1]);
                }
            }
        }

        // ---- the helmet --------------------------------------------------------------------------
        {
            ItemRef* h = &eq->helmet;
            if (h->item >= 0) {
                stats[0] += h->head_armor();
                stats[1] += h->body_armor();
                stats[2] += h->leg_armor();
            }
            *out_body = new lod_mesh;

            if (h->item < 0) {
                lod_mesh* c = clone_char_mesh(colour, &Skin::body);
                c->set_color((unsigned)mesh_color);
                a->fx->groups.push_back(c);
                legs_covered = 1;
            } else {
                if (g_items[h->item].type_flags & 0x1000000)
                    legs_covered = 1;
                lod_mesh* hl = make_item_lod(h, h->m4 >> 24);
                if ((unsigned)colour < (unsigned)g_anim.char_count) {
                    int cv = (int)((g_anim.skin_vec[colour].flags & 7) * 10);
                    if (cv != 0) {
                        float cf = (float)cv;
                        hl->apply_anim_all(&cf);
                    }
                }
                if (g_items[h->item].sound.count > 0) {
                    g_game->event_ctx.agent = hit_troop;
                    g_game->event_ctx.troop = troop;
                    g_game->event_ctx.slot = 5;
                    g_game->event_ctx.key_a = hl;
                    g_game->event_ctx.mesh = 0;
                    g_script.param1 = hit_troop;
                    g_script.param2 = troop;
                    g_items[h->item].sound.update(-50);      // 0x48add0 (the item's sound set)
                }
                a->fx->groups.push_back(hl);
            }
        }

        // ---- the boots (the L/R pair plus the lit Lx/Rx pair) ------------------------------------
        {
            ItemRef* b = &eq->boots;
            if (b->item >= 0) {
                stats[0] += b->head_armor();
                stats[1] += b->body_armor();
                stats[2] += b->leg_armor();

                tstring nm = item_name(b, b->m4 >> 24);
                nm.buf[nm.len - 1] = 'L';
                ap[17] = g_resmgr.make_lod_mesh(nm, false);

                tstring lx(nm);
                lx.push_back('x');
                if (g_resmgr.find_mesh(lx)) {
                    if (g_use_shaders == 0) {
                        lod_mesh* Lx = g_resmgr.make_lod_mesh(lx, false);
                        ap[19] = Lx;
                        a->fx->elems[12].add_group(Lx);
                    } else {
                        unsigned col = 0xffffffff;
                        (*out_body)->append(g_resmgr.get_lod_mesh(lx), 12,
                                            (matrix*)&a->fx->elems[12].m4, &col);
                    }
                }

                nm.buf[nm.len - 1] = 'R';
                ap[18] = g_resmgr.make_lod_mesh(nm, false);

                tstring rx(nm);
                rx.push_back('x');
                if (g_resmgr.find_mesh(rx)) {
                    if (g_use_shaders == 0) {
                        lod_mesh* Rx = g_resmgr.make_lod_mesh(rx, false);
                        ap[20] = Rx;
                        a->fx->elems[17].add_group(Rx);
                    } else {
                        unsigned col = 0xffffffff;
                        (*out_body)->append(g_resmgr.get_lod_mesh(rx), 17,
                                            (matrix*)&a->fx->elems[17].m4, &col);
                    }
                }
            } else {
                ap[17] = clone_char_mesh(colour, &Skin::hand_l);
                ap[18] = clone_char_mesh(colour, &Skin::hand_r);
                ap[17]->set_color((unsigned)mesh_color);
                ap[18]->set_color((unsigned)mesh_color);
            }
            a->fx->elems[13].add_group(ap[17]);
            a->fx->elems[18].add_group(ap[18]);
        }

        // ---- default trousers when nothing covers the legs ---------------------------------------
        if (!legs_covered) {
            lod_mesh* tr2 = g_resmgr.make_lod_mesh(tstring("def_trousers"), false);
            a->fx->groups.push_back(tr2);
        }

        // ---- the body armour ---------------------------------------------------------------------
        {
            ItemRef* bd = &eq->body;
            if (bd->item < 0) {
                lod_mesh* c1 = clone_char_mesh(colour, &Skin::calf_l);
                lod_mesh* c2 = clone_char_mesh(colour, &Skin::calf_r);
                c1->set_color((unsigned)mesh_color);
                c2->set_color((unsigned)mesh_color);
                a->fx->elems[2].add_group(c1);
                a->fx->elems[5].add_group(c2);
            } else {
                stats[0] += bd->head_armor();
                stats[1] += bd->body_armor();
                stats[2] += bd->leg_armor();

                tstring nm = item_name(bd, bd->m4 >> 24);
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
                    (*out_body)->append(L, b1, (matrix*)&a->fx->elems[b1].m4, &col);
                    col = 0xffffffff;
                    (*out_body)->append(R, b2, (matrix*)&a->fx->elems[b2].m4, &col);
                } else {
                    lod_mesh* c = g_resmgr.get_lod_mesh(nm)->clone();
                    a->fx->add_group(c);
                }
            }
        }

        float f = 3.1f;
        a->set_fbc(&f);
        a->shadow_radius = 1.1f;
        a->center = vector4(0.0f, 0.0f, 1.1f);      // the 3-argument constructor leaves w unset; the copied w lane is never read
    } else {
        // ---- only the hand-held weapon ------------------------------------------------------------
        // one full-expression: the name is a temporary built straight into make_lod_mesh's argument (0x48254d:
        // tstring(const char*) on [esp+0x38], no copy) and destroyed after the push (0x48256e); a named `nm`
        // initialised from the conditional was a copy of a temporary (an extra 0x80-byte buffer and destructor)
        a->fx->groups.push_back(g_resmgr.make_lod_mesh(hweapon->item >= 0 && hweapon->item < g_item_kind_count
                   ? g_items[hweapon->item].mesh_name_for(hweapon->b.mod)    // 0x433bf0
                   : tstring("Error: Invalid item"), false));
        FxElem* e = a->fx->elems;
        e[4].blend_rate = 0.2f;
        e[5].blend_rate = 0.25f;
        e[6].blend_rate = 0.25f;
        e[7].blend_rate = 0.3f;
        float f = 3.1f;
        a->set_fbc(&f);
        a->shadow_radius = 2.1f;
        a->center = vector4(0.0f, 0.0f, 1.3f);      // the 3-argument constructor leaves w unset; the copied w lane is never read
    }

    // ---- common tail -----------------------------------------------------------------------------
    for (int i = 0; i < *extra_count; i++) {
        mesh* m = extra[i];
        a->fx->psys.push_back(m);
    }

    {
        lod_mesh* br = *out_body;
        if (br) {
            if (!(br->count == 0)) { a->fx->groups.push_back(br); } else { delete br; }
        }
    }

    for (int i = 0; i < 0x15; i++) {
        if (ap[i])
            ap[i]->mask = 0xffff;
    }
}
