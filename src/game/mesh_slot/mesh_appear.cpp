// mesh_appear.cpp: TU 63 "mesh", part 6 (0x482240..0x4832b0): the agent appearance builder. See MIGRATION.md.
#include "./mesh_slot.h"
#include "./mesh_build.h"          // build_appearance (0x482240), setup_item_meshes (0x47cfe0)
#include "../../include/fx.h"
#include "../../include/troop.h"
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"   // GameData, g_game (0x5e2930)
#include "../ui/timer_list.h"          // TimerList: the sound holder (update 0x48add0)
#include "../../include/item_kind.h"   // g_item_kind_count 0x5e29d4
#include "../../include/render_flags.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

namespace rgl { struct mat3; }

using namespace rgl;
// src/game/terrain_b/poly_mesh.h (pulled in through mesh_slot.h) declares a global ::vat with the same signature
#define vat rgl::vat

// the identity's 3x3 (src/rgl/rgl_math.h; defined in shapes/shapes.cpp)
mat3* identity3(mat3* out);                     // 0x40cd60

// ---- views of shared classes (members defined elsewhere in the image) --------------------------------------------------
// The members below are the ones of the classes that define them, which this TU's headers do not declare (or declare under another layout):
// lod_group (src/rgl/core/resmgr.h, the 0xe0-byte lod record; ctor 0x41c2a0), Poser (agent3/agent_fx_sound.h: the object with five int lists,
// add_to_list 0x441ff0), tmesh (clone 0x47a370, declared in mesh_slot.h), lod_mesh_x (game/lod_mesh_x.h: set_color / transform / append / apply),
// lod_mesh_a (mesh_scene.cpp, apply_anim_all 0x47dd90), and TimerList (ui/timer_list.h, update
// 0x48add0). Only the members are declared; lod2_a forwards to them.
struct lod_group {
    lod_group();   // 0x41c2a0
};
struct Poser {
    void add_to_list(int idx, rgl::mesh* val);   // 0x441ff0
};
struct lod_mesh_a {
    void apply_anim_all(const float* t);   // 0x47dd90
};
struct lod2_a : lod_mesh2 {                      // the 0xe0-byte lod record
    void add_to_list(int level, mesh* m) { ((Poser*)this)->add_to_list(level, (rgl::mesh*)m); }     // 0x441ff0 (thiscall, ret 8)
    lod_mesh2* clone() { return (lod_mesh2*)((tmesh*)this)->clone(); }                        // 0x47a370
    void set_color(unsigned color) { ((lod_mesh_x*)this)->set_color(color); }                 // 0x47b5b0 (ret 4)
    void transform(const matrix* m) { ((lod_mesh_x*)this)->transform(*m); }                  // 0x47b610 (ret 4)
    void append(lod_mesh2* src, int bone, const matrix* m, const unsigned* color) { ((lod_mesh_x*)this)->append((lod_mesh*)src, bone, m, color); }   // 0x47b690 (ret 16)
    void apply(const matrix* m) { ((lod_mesh_x*)this)->apply(m); }                           // 0x46c9f0 (ret 4)
    void apply_anim_all(const void* color) { ((lod_mesh_a*)this)->apply_anim_all((const float*)color); }   // 0x47dd90 (ret 4)
};
Skin* append_mesh(int colour, int kind, void** ap, void** a, void** b, char carried, int* out);   // 0x481c40 (mesh_terrain.cpp)
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

// the name an item contributes (its carry position), or the error string when the item is invalid. The original builds the
// name straight into one 16-byte slot from either branch (0x433bf0's hidden return pointer, or 0x42bbe0 on the literal) and
// destroys it at scope end: no default construction, no assignment, no copy. A holder with placement construction gives the
// same (a conditional expression, or `*out = ...`, copies a temporary: an extra 0x80-byte buffer).
struct ItemName {
    char raw[sizeof(tstring)];
    ItemName(ItemRef* r, int modifier)
    {
        if (r->item >= 0 && r->item < g_item_kind_count)
            new (raw) tstring(g_items[r->item].mesh_name_for(modifier));       // 0x433bf0
        else
            new (raw) tstring("Error: Invalid item");                          // 0x42bbe0
    }
    ~ItemName() { get().~tstring(); }
    tstring& get() { return *(tstring*)raw; }
};

// the lod record that a single equipped item contributes (named, made into a lod mesh)
static __forceinline lod_mesh2* make_item_lod(ItemRef* r, int modifier)
{
    ItemName name(r, modifier);
    return (rgl::lod_mesh2*)g_resmgr.make_lod_mesh(name.get(), false);
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

// a clone of one of the skin's stored meshes (body, hand_l, hand_r, calf_l, calf_r: fields +0x14..+0x24); the original bounds-checks
// the colour and then calls clone() on the (possibly null) pointer
static __forceinline lod_mesh2* clone_char_mesh(int colour, lod_group* Skin::* mesh)
{
    lod2_a* src;
    if ((unsigned)colour < (unsigned)g_anim.char_count)          // 0x482852/0x48297f: cmp + jae (unsigned)
        src = (lod2_a*)(g_anim.skin_vec._Myfirst[colour].*mesh);
    else
        src = 0;
    return src->clone();
}


// @ 0x482240
void build_appearance(strategic_entity* a, int hand, int troop, int kind, ItemRef* hweapon,
                      lod_mesh2** ap, int* stats, lod_mesh2** extra, int* extra_count,
                      lod_mesh2** out_body, EquipSet* eq, int hit_troop)
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

    lod2_a* rec = (lod2_a*)operator new(sizeof(lod2_a));
    if (rec)
        ((lod_group*)rec)->lod_group::lod_group();      // 0x41c2a0 (the lod record's constructor)
    rec->add_to_list(0, body);
    vpush_i(a->lods, (lod_mesh2*)rec);
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

        char* tr = g_game->chars_c + troop * 0x508;
        int colour = *(unsigned*)(tr + 0x30) & 0xf;
        append_mesh(colour, kind, (void**)ap, (void**)(tr + 0x3c0), (void**)(tr + 0x3c0 + 4 * 8), (char)carried, &mesh_color);

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
            ((lod2_a*)ap[0])->transform((matrix*)&mtx);
            a->fx->elems[9].add_group(ap[0]);
            if (ap[2]) {
                ((lod2_a*)ap[2])->transform((matrix*)&mtx);
                a->fx->elems[9].add_group(ap[2]);
            }
            if (ap[1]) {
                ((lod2_a*)ap[1])->transform((matrix*)&mtx);
                a->fx->elems[9].add_group(ap[1]);
            }
        } else {
            lod_mesh2* wl = make_item_lod(w, w->m4 >> 24);
            unsigned wflags = g_items[w->item].type_flags;
            if ((wflags & 0x80000000) == 0)
                g_anim.skin_vec._Myfirst[g_anim.cur].use_carry((lod_mesh*)wl, g_anim.weights);      // 0x47de60
            ((lod2_a*)wl)->apply((matrix*)&mtx);
            if ((wflags & 0xf00) == 0xf00)
                a->fx->add_group(wl);                       // 0x478fb0
            else
                a->fx->elems[9].add_group(wl);
            int c3 = (wflags & 0x80000000) ? 1 : legs_covered;
            ap[3] = wl;
            if (!(c3 == 0)) {
                if (ap[0]) lod_release(ap[0]);
                if (ap[1]) lod_release(ap[1]);
                if (ap[2]) lod_release(ap[2]);
                ap[0] = ap[1] = ap[2] = 0;
            } else {
                ((lod2_a*)ap[0])->transform((matrix*)&mtx);
                a->fx->elems[9].add_group(ap[0]);
                if (ap[2]) {
                    ((lod2_a*)ap[2])->transform((matrix*)&mtx);
                    a->fx->elems[9].add_group(ap[2]);
                }
                if (ap[1]) {
                    ((lod2_a*)ap[1])->transform((matrix*)&mtx);
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
            lod2_a* body_rec = (lod2_a*)operator new(sizeof(lod2_a));
            if (body_rec)
                ((lod_group*)body_rec)->lod_group::lod_group();      // 0x41c2a0
            *out_body = body_rec;

            if (h->item < 0) {
                lod_mesh2* c = clone_char_mesh(colour, &Skin::body);
                ((lod2_a*)c)->set_color((unsigned)mesh_color);
                vpush_i(a->fx->groups, c);
                legs_covered = 1;
            } else {
                if (g_items[h->item].type_flags & 0x1000000)
                    legs_covered = 1;
                lod_mesh2* hl = make_item_lod(h, h->m4 >> 24);
                if ((unsigned)colour < (unsigned)g_anim.char_count) {
                    int cv = (int)((g_anim.skin_vec._Myfirst[colour].flags & 7) * 10);
                    if (cv != 0) {
                        float cf = (float)cv;
                        ((lod2_a*)hl)->apply_anim_all(&cf);
                    }
                }
                if (*(int*)((char*)&g_items[h->item] + 0x228) > 0) {
                    g_game->event_ctx.agent = hit_troop;
                    g_game->event_ctx.troop = troop;
                    g_game->event_ctx.slot = 5;
                    g_game->event_ctx.key_a = hl;
                    g_game->event_ctx.mesh = 0;
                    g_script.param1 = hit_troop;
                    g_script.param2 = troop;
                    ((TimerList*)((char*)&g_items[h->item] + 0x228))->update(-50);      // 0x48add0 (the item's sound holder)
                }
                vpush_i(a->fx->groups, hl);
            }
        }

        // ---- the boots (the L/R pair plus the lit Lx/Rx pair) ------------------------------------
        {
            ItemRef* b = &eq->boots;
            if (b->item >= 0) {
                stats[0] += b->head_armor();
                stats[1] += b->body_armor();
                stats[2] += b->leg_armor();

                ItemName nmh(b, b->m4 >> 24);
                tstring& nm = nmh.get();
                nm.buf[nm.len - 1] = 'L';
                ap[17] = (rgl::lod_mesh2*)g_resmgr.make_lod_mesh(nm, false);

                tstring lx(nm);
                lx.push_back('x');
                if (g_resmgr.find_mesh(lx)) {
                    if (g_use_shaders == 0) {
                        lod_mesh2* Lx = (rgl::lod_mesh2*)g_resmgr.make_lod_mesh(lx, false);
                        ap[19] = Lx;
                        a->fx->elems[12].add_group(Lx);
                    } else {
                        unsigned col = 0xffffffff;
                        ((lod2_a*)*out_body)->append((rgl::lod_mesh2*)g_resmgr.get_lod_mesh(lx), 12,
                                            (matrix*)&a->fx->elems[12].m4, &col);
                    }
                }

                nm.buf[nm.len - 1] = 'R';
                ap[18] = (rgl::lod_mesh2*)g_resmgr.make_lod_mesh(nm, false);

                tstring rx(nm);
                rx.push_back('x');
                if (g_resmgr.find_mesh(rx)) {
                    if (g_use_shaders == 0) {
                        lod_mesh2* Rx = (rgl::lod_mesh2*)g_resmgr.make_lod_mesh(rx, false);
                        ap[20] = Rx;
                        a->fx->elems[17].add_group(Rx);
                    } else {
                        unsigned col = 0xffffffff;
                        ((lod2_a*)*out_body)->append((rgl::lod_mesh2*)g_resmgr.get_lod_mesh(rx), 17,
                                            (matrix*)&a->fx->elems[17].m4, &col);
                    }
                }
            } else {
                ap[17] = clone_char_mesh(colour, &Skin::hand_l);
                ap[18] = clone_char_mesh(colour, &Skin::hand_r);
                ((lod2_a*)ap[17])->set_color((unsigned)mesh_color);
                ((lod2_a*)ap[18])->set_color((unsigned)mesh_color);
            }
            a->fx->elems[13].add_group(ap[17]);
            a->fx->elems[18].add_group(ap[18]);
        }

        // ---- default trousers when nothing covers the legs ---------------------------------------
        if (!legs_covered) {
            lod_mesh2* tr2 = (rgl::lod_mesh2*)g_resmgr.make_lod_mesh(tstring("def_trousers"), false);
            vpush_i(a->fx->groups, tr2);
        }

        // ---- the body armour ---------------------------------------------------------------------
        {
            ItemRef* bd = &eq->body;
            if (bd->item < 0) {
                lod_mesh2* c1 = clone_char_mesh(colour, &Skin::calf_l);
                lod_mesh2* c2 = clone_char_mesh(colour, &Skin::calf_r);
                ((lod2_a*)c1)->set_color((unsigned)mesh_color);
                ((lod2_a*)c2)->set_color((unsigned)mesh_color);
                a->fx->elems[2].add_group(c1);
                a->fx->elems[5].add_group(c2);
            } else {
                stats[0] += bd->head_armor();
                stats[1] += bd->body_armor();
                stats[2] += bd->leg_armor();

                ItemName nmh(bd, bd->m4 >> 24);
                tstring& nm = nmh.get();
                if (nm.buf[nm.len - 1] == 'L') {
                    lod_mesh2* L = (rgl::lod_mesh2*)g_resmgr.get_lod_mesh(nm);
                    nm.buf[nm.len - 1] = 'R';
                    lod_mesh2* R = (rgl::lod_mesh2*)g_resmgr.get_lod_mesh(nm);
                    int b1 = 2, b2 = 5;
                    if ((g_items[bd->item].type_flags & 0xf00) != 0) {
                        b1 = -1;
                        b2 = -1;
                    }
                    unsigned col = 0xffffffff;
                    ((lod2_a*)*out_body)->append(L, b1, (matrix*)&a->fx->elems[b1].m4, &col);
                    col = 0xffffffff;
                    ((lod2_a*)*out_body)->append(R, b2, (matrix*)&a->fx->elems[b2].m4, &col);
                } else {
                    lod_mesh2* c = ((lod2_a*)g_resmgr.get_lod_mesh(nm))->clone();
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
        vpush_i(a->fx->groups, (rgl::lod_mesh2*)g_resmgr.make_lod_mesh(hweapon->item >= 0 && hweapon->item < g_item_kind_count
                   ? g_items[hweapon->item].mesh_name_for(((signed char*)hweapon)[7])    // 0x433bf0
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
    for (int i = 0; i < *extra_count; i++)
        vpush_i(a->fx->psys, (mesh*)extra[i]);

    {
        lod_mesh2* br = *out_body;
        if (br) {
            if (!(br->count == 0)) { vpush_i(a->fx->groups, br); } else { lod_release(br); }
        }
    }

    for (int i = 0; i < 0x15; i++) {
        if (ap[i])
            ap[i]->mask = 0xffff;
    }
}
