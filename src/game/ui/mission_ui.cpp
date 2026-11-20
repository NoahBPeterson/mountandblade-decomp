//-----------------------------------------------------------------------------
// File: mission_ui.cpp  (0x488560..0x48b2d0)
//
// Desc: the mission-UI widget base (track timers, flag / colour accessors, text placement, the constructor), its
//       list helpers, and the neighbouring records of the same object file: the item-pair sort, troop record and
//       party list helpers, selector records, a mission view, the world's party / item value queries, the replay
//       frame interpolation, per-team counters and combatant slots, and the first widget's key handler. Names
//       provisional. Functions in ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#define UIA_CTOR_OUT_OF_LINE   // UiA::UiA is defined below (0x48b1c0)
#include <math.h>
#include "../config/config_block.h"   // g_cfg
#include <string.h>
#include <stdlib.h>
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../../rgl/scene/world.h"      // rgl::world::set_fog 0x488cd0 (the fog block at +0xdc)
#include "./ui_common.h"
#include "./item_ref.h"
#include "./troop_inv.h"
#include "../party2/camera_class.h"   // Camera::draw 0x488d50
#include "./party.h"
#include "./mission_ui.h"
#include "./ui_widgets.h"
#include "./world.h"
#include "./mission.h"
#include "../window_mgr.h"      // WindowMgrW::clear_all 0x489be0
#include "../../include/dialog_rec.h"      // DialogBlock::emit 0x4b09e0 (the block runner a TimerChild's sound channel stands for)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../agent_ai.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/fx.h"             // FxDef: elem_add_psys 0x488e20 / elem_set_data 0x488e40
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../item_ref.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/item_kind.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/party.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_recs.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../rgl/options.h"   // g_options 0x5aa008
#include "../shared_vars.h"
#include "../key_names.h"
#include "../game_keys.h"   // GameKeys::is_down 0x48ac50, clicked 0x48ac90
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)
#include "../dialog_mgr.h"             // DialogMgr::current_text 0x489c30

// ---- data this module owns (.bss, no initialiser; sizes from the code) ----

SortVal g_sort_val[0x2a * 5 + 1];                   // 0xa75f90 weights per (modifier bit, 5): up to index 0x2a * 5 (mission_3.cpp), ends at g_anim_set 0xa76628


// round to nearest, as cl inlines it (0x432540 is the out-of-line copy in another object)
static __forceinline int rnd(const float* p)
{
    float fl = floorf(*p);
    int i = (int)fl;
    if (*p - fl <= 0.5f)
        return i;
    return i + 1;
}

struct Blob80 { int v[32]; };               // the party's 0x80-byte companion records (0x489540)


static __forceinline void restart_inl(Timer& t) { t.start = g_ticks[t.clock]; }     // Timer::restart (0x432bf0) as the callers inline it

// @ 0x488560
void MissionUi::animate_color(float a, int b)
{
    periodA = a;
    restart_inl(tm0);
    valA = b;
    srcA = color14c;
    c0 = 1;
}

// @ 0x4885a0
void MissionUi::animate_alpha(float a, int b)
{
    periodB = a;
    restart_inl(tm1);
    valB = b;
    srcB = color14c;
    c1 = 1;
}

// @ 0x4885e0
void MissionUi::animate_hilight_color(float a, int b)
{
    periodC = a;
    restart_inl(tm2);
    valC = b;
    srcC = color150;
    c2 = 1;
}

// @ 0x488620
void MissionUi::animate_hilight_alpha(float a, int b)
{
    periodD = a;
    restart_inl(tm3);
    valD = b;
    srcD = color150;
    c3 = 1;
}

// @ 0x488660
void MissionUi::animate_size(float a, const vec2& p)
{
    periodE = a;
    restart_inl(tm4);
    anim_size_to.x = p.x;
    anim_size_to.y = p.y;
    anim_size_from.x = cur_size.x;
    anim_size_from.y = cur_size.y;
    c4 = 1;
}

// @ 0x4886c0
void MissionUi::animate_pos(float a, const vec2& p)
{
    periodF = a;
    restart_inl(tm5);
    anim_pos_to.x = p.x;
    anim_pos_to.y = p.y;
    anim_pos_from.x = position.x;
    anim_pos_from.y = position.y;
    c5 = 1;
}

// @ 0x488720
// Per-frame track driver (ret 0x10: this + three stack words). `a` is forwarded to mesh::render, `t` is the
// caller's time accumulator, bumped by 1.0 for every child that advances. 0x40cd00 fills the 0x40-byte matrix; the
// position row (+0x30) gets (position.x, position.y, *t), and its z is the advanced time for each child.
void MissionUi::render(rgl::scene* a, float* t, float*, float*)
{
    float xf[16];
    if (f104 & 1)
        return;
    if (tracks[4])
    {
        if (f104 & 2)
            tracks[4]->color = color14c & 0x77ffffffu;
        else
            tracks[4]->color = (b121 && is_active()) ? color150 : color14c;
    }
    rgl::identity_pos((rgl::matrix*)xf);
    float pos[3] = { position.x, position.y, *t };    // an unset 4th word follows, copied whole into the position row
    *(rgl::vector4*)(xf + 12) = *(rgl::vector4*)pos;
    for (int i = 0; i < 5; i++)
    {
        if (!tracks[i])
            continue;
        if (i == 3)
        {
            if (!(b121 && is_active() && !(tracks[1] && b11c[1])))
                ;
            else
            {
                *t = *t + 1.0f;
                xf[14] = *t;
                tracks[3]->render(a, *(rgl::matrix*)xf);
                continue;
            }
        }
        else if (i == 2)
        {
            if (b121 && is_active() && tracks[3])
                continue;
        }
        if (!b11c[i])
            continue;
        *t = *t + 1.0f;
        xf[14] = *t;
        tracks[i]->render(a, *(rgl::matrix*)xf);
    }
}

// @ 0x488890
void MissionUi::set15c(int v)
{
    v15c = v;
}

// @ 0x4888a0
int MissionUi::get15c()
{
    return v15c;
}

// @ 0x4888b0
void MissionUi::set168(float v)
{
    v168 = v;
}

// @ 0x4888c0
float MissionUi::get168()
{
    return v168;
}

// @ 0x4888d0
bool MissionUi::test_c8(int m)
{
    return (unsigned char)((fC8 & m) != 0);
}

// @ 0x4888e0
void MissionUi::clear_c8()
{
    fC8 = 0;
}

// @ 0x4888f0
bool MissionUi::clear_mask(int m)
{
    bool had = (fC8 & m) != 0;
    fC8 &= ~m;
    return had;
}

// @ 0x488910
void MissionUi::set_key_flags(int m)
{
    fC8 |= m;
    fCC |= m;
}

// @ 0x488930
void MissionUi::set_d0(int v)
{
    fD0 = v;
}

// @ 0x488940
void MissionUi::set_d4(int v)
{
    fD4 = v;
}

// @ 0x488950
void MissionUi::or_d4(int m)
{
    fD4 |= m;
}

// @ 0x488970
// NOTE: 3 bytes differ from the original: identical instruction stream, but MSVC keeps `this`
// in edx and p in eax where the original keeps `this` in eax and p in edx.
void MissionUi::move_a(const vec2& p)
{
    rel_position.x = p.x;
    rel_position.y = p.y;
    if (p154)
    {
        p154->on_child_moved(this);
        return;
    }
    position.x = p.x;
    position.y = p.y;
}

// @ 0x4889b0
void MissionUi::set_xy(const vec2& p)
{
    rect_max.x = p.x;
    rect_max.y = p.y;
    if (p154)
    {
        p154->on_child_moved(this);
        return;
    }
}

// @ 0x4889e0
void MissionUi::move_f8(const vec2& p)
{
    rect_min.x = p.x;
    rect_min.y = p.y;
    if (p154)
    {
        p154->on_child_moved(this);
        return;
    }
}

// @ 0x488a10
void MissionUi::set_offset(const vec2& p)
{
    if (f148 & 1)
        f148 &= ~1u;
    text_offset.x = p.x;
    text_offset.y = p.y;
}

// @ 0x488a40
void MissionUi::set148(const unsigned& flags)
{
    f148 = flags;
    get_xy();
}

// @ 0x488a70
void MissionUi::or148(const unsigned& flags)
{
    f148 |= flags;
    get_xy();
}

// @ 0x488aa0
void MissionUi::set_mesh(int a, rgl::mesh* b)
{
    normalize_uv(b);
    set_track(a, b);
}

// @ 0x488ad0
void MissionUi::set_track(int i, rgl::mesh* p)
{
    if (tracks[i])
        delete tracks[i];
    tracks[i] = p;
    on_track_set(p);
}

// @ 0x488b10
void MissionUi::set_color(const unsigned& c)
{
    color14c = c;
}


// @ 0x488b20
void MissionUi::set_hilight_color(const unsigned& c)
{
    color150 = c;
}

// @ 0x488b30
void MissionUi::set_font(Font* f)
{
    font = f;
    get_xy();
}

// @ 0x488b50
V2 MissionUi::set_text(const tstring& s)
{
    if (f148 & 2) {
        float v = rect_max.y * 0.95f;
        text_scale.y = v;
        text_scale.x = v;
    }
    name = s;
    unsigned fl = f148;
    if (fl & 1) {
        text_offset.x = 0.005f;
        text_offset.y = rect_max.y * 0.9f - text_scale.y * 0.6f;
    }
    if ((fl & 0x200) && !(fl & 0x100))
        set_xy(V2(1.0f, 0.75f));
    V2 var = get_xy();
    fl = f148;
    if (fl & 0x200) {
        const float* src = (fl & 0x100) ? &rect_max.x : &var.x;
        set_xy(V2(*src, var.y));
        text_offset.y = var.y - text_scale.y * 0.63f;
        get_xy();
    }
    b11c[4] = 1;
    return var;
}

// @ 0x488c70
void MissionUi::get_text(tstring& out)
{
    out = name;
}

// @ 0x488c90
void MissionUi::hide()
{
    f104 |= 1;
}

// @ 0x488ca0
void MissionUi::show()
{
    f104 &= ~1u;
}

// @ 0x488cb0
void MissionUi::flag2_set()
{
    f104 |= 2;
}

// @ 0x488cc0
void MissionUi::flag2_clear()
{
    f104 &= ~2u;
}

// The fog block of the scene world (view.fog_start .. fog_density at +0xdc..+0xec): the target, the rate, the colour and the smoothing derived from the rate.
// @ 0x488cd0
void rgl::world::set_fog(const float* ref, const float* rate, const int* color)
{
    view.fog_on = 1;
    view.fog_start = *ref;
    view.fog_end = *rate;
    view.fog_color = *color;
    float f = (1.0f / view.fog_end) * 3.0f;
    if (f < 0.001f) {
        view.fog_density = 0.001f;
        return;
    }
    if (f >= 0.95f) {
        view.fog_density = 0.95f;
        return;
    }
    view.fog_density = f;
}

// @ 0x488d50
void Camera::draw()
{
    for (int i = 0; i < 2; i++) {
        slot_on[i] = 0;
        if (follow[i]) {
            follow[i]->hide();
            float d = target->rect_max[i] - target->rect_min[i];
            if (d > rect_max[i]) {
                slot_on[i] = 1;
                follow[i]->show();
                if (slot_on[i] != 0)
                    follow[i]->update_children(rect_max[i] / d);
            }
        }
    }
    target->move_a(V2(0.0f, 0.0f));
}

// @ 0x488e20
void FxDef::elem_add_psys(int i, rgl::psys* ps)
{
    elems[i].add_to_first(ps);
}

// @ 0x488e40
void FxDef::elem_set_data(int i, void* p)
{
    elems[i].set_data(p);
}

// @ 0x488e60
void PairStack::push(int a, int b)
{
    if (count >= 0 && count < 0x80) {
        pairs[count * 2] = a;
        pairs[count * 2 + 1] = b;
        count++;
    }
}

// @ 0x488e90
void TimerChild::tick(int idx)
{
    if (period < -1.0f) {
        if (idx == rnd(&period)) {
            __int64 buf[16];
            int zero = 0;
            ch.emit(idx, 0, &zero, buf, (unsigned)-1);   // 0x4b09e0
        }
    } else {
        long long d = g_ticks[last.clock] - last.start;
        if (!((float)d * 1e-5f < period) && idx >= 0) {
            restart_inl(last);
            ch.play(idx);
        }
    }
}

// @ 0x488f80
inline int __fastcall sort_value(ItemRef* e)        // inline: sl_take expands it in its equipment loop and calls it in the inventory loop
{
    int idx = e->item;
    if (idx >= 0) {
        float v = (float)g_items[idx].value;
        int mod = (signed char)(e->m4 >> 24);
        float x = v * g_sort_val[mod * 5].v;
        return rnd(&x);
    }
    return 0;
}

// @ 0x488ff0
float ItemRef::speed_scale(const float* base, const float* ref, float r)
{
    float d = fabsf(r - *ref);
    if ((r > 1.001f && *ref < 1.001f) || (r < 0.999f && *ref > 0.999f))
        d = d * 5.0f + 3.0f;
    return *base / (d + 1.0f);
}

// @ 0x489060
void __fastcall clear_slots(ItemRef* e)
{
    int i = 0;
    while (i < 0x60) {
        e[i].item = -1;
        i++;
    }
}

// @ 0x489070
int ItemRef::check_slots(int n)
{
    int i = 0;
    if (n > 0) {
        do {
            if (this[i].item == -1)
                break;
            i++;
        } while (i < n);
    }
    return i;
}

// The selection order: higher item id first, ties by the modifier's sort weight (the modifier is the top byte of the
// m4 word read as (int)m4 >> 24, a movsx of byte 7 anchored at +4).
static inline bool sel_better(const ItemRef& a, const ItemRef& b)
{
    if (a.item > b.item)
        return true;
    if (a.item < b.item)
        return false;
    return g_sort_val[((int)a.m4 >> 24) * 5].v > g_sort_val[((int)b.m4 >> 24) * 5].v;
}
// @ 0x489090
// Descending selection sort of n item references (cl unrolls the inner scan by 4, as in the original).
void ItemRef::sort_sel(int n)
{
    for (int j = 0; j < n - 1; j++) {
        int best = j;
        for (int k = j + 1; k < n; k++) {
            if (sel_better(this[k], this[best]))
                best = k;
        }
        ItemRef tmp = this[j];
        this[j] = this[best];
        this[best] = tmp;
    }
}
// @ 0x4892a0
void Troop::set_flags_low(int v)
{
    flags30 &= ~0xf;
    flags30 |= (__int64)v & 0xf;
}

// @ 0x4892d0
void Troop::add_stat(unsigned i, int amt)
{
    float v = stat[i] + (float)amt;
    stat[i] = v;
    if (v < 0.0f) {
        stat[i] = 0.0f;
        return;
    }
    if (v > 1000.0f)
        stat[i] = 1000.0f;
}

// @ 0x489320
void Troop::set_colors(int a, int b)
{
    site = (b & 0xff) << 16 | a;
}

// @ 0x489340
void Troop::remove_gold(int amt)
{
    gold -= amt;
    if (gold < 0)
        gold = 0;
}

// @ 0x489370
void Troop::set_alpha(int a)
{
    int v = a;
    *(unsigned char*)&f64 = 0;
    f64 |= v & 0xff;
}

// @ 0x489390
int __cdecl terrain_is_road(unsigned v)
{
    if ((v & 7) != 0 && v != 7)
        return 0;
    return 1;
}

// @ 0x4893b0
int terrain_is_water(unsigned x)
{
    int m = x & 7;
    if (m < 2 || m >= 7) {
        if (x != 7)
            return 0;
    }
    return 1;
}

// @ 0x4893e0
void NamedSelRec::reset()
{
    f0 = 0;
    f4 = -1;
    f8 = 0;
    name.cap = 0;
    name.pos = 0;
    name.buf = 0;
    name.len = 0;
}

// @ 0x489400
void NamedSelRec::set_name(const tstring* s)
{
    name = *s;
    flag1c = 1;
}

// @ 0x489420
void Party::set_id(int v)
{
    unsigned __int64* f = (unsigned __int64*)&flags_lo;
    ((char*)&flags_lo)[0] = 0;
    *f |= (long long)(v & 0xff);
}

// @ 0x489460
float Party::scale()
{
    unsigned u = (unsigned)flags_lo & 0x3000;
    if (u == 0)
        return 0.5f;
    if (u == 0x1000)
        return 0.75f;
    return 1.0f;
}

// @ 0x489490
int Party::sum_all()
{
    int s = 0;
    int n = nstacks;
    if (n > 0) {
        Stack* p = stacks;
        do {
            s += p->count;
            p++;
        } while (--n);
    }
    return s;
}

// @ 0x4894b0
int Party::sum_head()
{
    int s = 0;
    int i = 0;
    int n = nstacks;
    if (n > 0) {
        Stack* p = stacks;
        do {
            if (p->flags & 1)
                return s;
            s += p->count;
            i++;
            p++;
        } while (i < n);
    }
    return s;
}

// @ 0x4894e0
int Party::sum_flagged()
{
    int s = 0;
    int n = nstacks;
    if (n > 0) {
        Stack* p = stacks;
        do {
            if (p->flags & 1)
                s += p->count;
            p++;
        } while (--n);
    }
    return s;
}

// @ 0x489510
int Party::count_tail()
{
    int n = nstacks;
    int lead = 0;
    int i = 0;
    if (n > 0) {
        char* q = (char*)stacks + 8;
        do {
            if (*q & 1)
                break;
            lead++;
            i++;
            q += 0x20;
        } while (i < n);
    }
    return n - lead;
}

// @ 0x489540
int* Party::remove_row(int* out, int index, int* arr)
{
    Stack local = stacks[index];

    int i = index;
    if (index < nstacks - 1) {
        do {
            stacks[i] = stacks[i + 1];
            i++;
        } while (i < nstacks - 1);
    }

    *(int*)((char*)this + nstacks * 0x20 + 0x8c) = -1;
    *(int*)((char*)this + nstacks * 0x20 + 0x88) = 0;
    nstacks = nstacks - 1;
    dirty = 1;

    if (arr != 0) {
        for (int j = index; j < nstacks; j++)
            *(Blob80*)((char*)arr + j * 0x80) = *(Blob80*)((char*)arr + (j + 1) * 0x80);
    }

    *(Stack*)out = local;
    return out;
}

// @ 0x489620
int Party::find_or_add(unsigned troop, unsigned flag, char force)
{
    int found = -1;
    if (*(unsigned __int64*)&flags_lo & 0x100000) {
        int have, cap;
        if (flag & 1) {
            have = 0;
            int m = nstacks;
            if (m > 0) {
                Stack* p = stacks;
                do {
                    if (p->flags & 1)
                        have += p->count;
                    p++;
                } while (--m);
            }
            cap = max_prisoners();
        } else {
            have = sum_head();
            cap = max_members();
        }
        if (cap - have <= 0) {
            if (force)
                goto done;
        }
    }

    int n = nstacks;
    int i = 0;
    if (n > 0) {
        Stack* p = stacks;
        do {
            if (p->troop == (int)troop && ((p->flags ^ flag) & 1) == 0)
                found = i;
            i++;
            p++;
        } while (i < n);
        if (found != -1)
            goto done;
    }
    if (n >= 0x40)
        goto done;

    if (flag & 1) {
        if ((*(unsigned __int64*)&flags_lo & 0x100000) == 0)
            goto tail_count;
        int ct = count_tail();
        int mx = max_stacks();
        if (ct < mx) {
        tail_count: ;
            int lead = 0;
            int m = nstacks;
            for (int k = 0; k < m; k++) {
                if (stacks[k].flags & 1)
                    break;
                lead++;
            }
            if (m - lead < 0x20) {
                found = nstacks;
                goto done;
            }
        }
    }
    if (!(flag & 1)) {
        if (*(unsigned __int64*)&flags_lo & 0x100000) {
            int ff = first_flagged();
            int mx = max_stacks();
            if (!(ff < mx))
                goto done;
        }
        int lead = 0;
        int m = nstacks;
        for (int k = 0; k < m; k++) {
            if (stacks[k].flags & 1)
                break;
            lead++;
        }
        if (lead < 0x20) {
            found = nstacks;
            goto done;
        }
    }
done:   // every exit leaves through the one found-in-ebx epilogue (mov eax,ebx), as in the original
    return found;
}

// @ 0x4897a0
void PlacedObj::animate_to(const rgl::matrix* to, const float* seconds, const int* state)
{
    animate(0);
    m118 = frame;
    md8 = *to;
    m168 = *state;
    f16c = *seconds;
    tween_clock.restamp();

    last_t = 0;
}

// @ 0x489820
void PlacedObj::set_position(const rgl::matrix* m)
{
    frame = *m;
    if (ent != 0)
        ent->set_local_scaled_dirty(m, &scale);
    ((EmptySlot*)this)->EmptySlot::slot();          // 0x477b70: IfaceB's empty slot function, called directly
}

// @ 0x489860
void Sel::set_v(int v)
{
    if (rec[0].present)
        rec[0].x = v;
    if (rec[1].present)
        rec[1].x = v;
    if (rec[2].present)
        rec[2].x = v;
}

// @ 0x489890
rgl::vector4 Sel::first()
{
    int i = 0;
    int* p = &rec[0].present;
    do {
        if (*p != 0)
            return *(rgl::vector4*)((char*)this + i * 0x6c + 0x18);
        i++;
        p += 0x1b;
    } while (i < 3);
    return rgl::vector4(0.0f, 0.0f, 0.0f);
}

// @ 0x4898e0
void Sel::set_vec(const int* a, const int* b, const int* c)
{
    if (rec[0].present) {
        *(Blob16*)&rec[0].a[0] = *(const Blob16*)a;
        rec[0].b = *b;
        *(Blob16*)&rec[0].c[0] = *(const Blob16*)c;
    }
    if (rec[1].present) {
        *(Blob16*)&rec[1].a[0] = *(const Blob16*)a;
        rec[1].b = *b;
        *(Blob16*)&rec[1].c[0] = *(const Blob16*)c;
    }
    if (rec[2].present) {
        *(Blob16*)&rec[2].a[0] = *(const Blob16*)a;
        rec[2].b = *b;
        *(Blob16*)&rec[2].c[0] = *(const Blob16*)c;
    }
}

// @ 0x4899e0
void Sel::bump(int d)
{
    if (rec[0].present) {
        int v = rec[0].counter + d;
        rec[0].counter = v;
        if (v < 1)
            rec[0].counter = 1;
        else if (v > 4)
            rec[0].counter = 4;
    }
    if (rec[1].present) {
        int v = rec[1].counter + d;
        rec[1].counter = v;
        if (v < 1)
            rec[1].counter = 1;
        else if (v > 4)
            rec[1].counter = 4;
    }
    if (rec[2].present) {
        int v = rec[2].counter + d;
        rec[2].counter = v;
        if (v < 1)
            rec[2].counter = 1;
        else if (v > 4)
            rec[2].counter = 4;
    }
}

// @ 0x489a90
void NamedSelRec::set_sel(const int* src)
{
    int v = *src;
    if (v < 0 || v >= 8)
        return;
    sel = v;
}

// @ 0x489ab0
void Named16::set(const tstring* s)
{
    name = *s;
    flag10 = 1;
}

// @ 0x489ad0
void E68::quarter()
{
    if (g_cfg.texture_detail >= 0x3c) {
        b4 = b2;
        b5 = b3;
        return;
    }
    if (g_cfg.texture_detail >= 0x19) {
        b4 = b2 / 2;
        b5 = b3 / 2;
        return;
    }
    b4 = b2 / 4;
    b5 = b3 / 4;
}

// @ 0x489b20
skin* E68::make_material(const int* key)
{
    quarter();
    skin* o = g_resmgr.get_shadow_skin(b4, b5, 0x15, 1);
    o->refcount = 0;
    o->key = *key;
    o->tex[0]->rendered = 0;
    if (t != 0) {
        o->shader = t->shader;
        ((tstring*)&o->shader_name)->operator=(*(tstring*)&t->shader_name);
        o->flags = t->flags;
        ((tstring*)&o->name)->operator=(*(tstring*)&t->name);
        *(Blob16*)&o->color = *(Blob16*)&t->color;   // copied as four words, not as floats
        *(int*)&o->spec_coef = *(int*)&t->spec_coef;
        o->tex[2] = t->tex[2];
        o->tex[1] = t->tex[1];
        o->tex[3] = t->tex[3];
        o->tex[4] = t->tex[4];
    }
    return o;
}

// @ 0x489be0
void WindowMgrW::clear_all()
{
    int zero = 0, cnt = 0x63;
    int** e = (int**)&arr[2];
    int i = 5;
    do {
        int* q;
        q = e[-2]; q[1] = zero; q[2] = cnt;
        q = e[-1]; q[1] = zero; q[2] = cnt;
        q = e[0];  q[1] = zero; q[2] = cnt;
        q = e[1];  q[1] = zero; q[2] = cnt;
        q = e[2];  q[1] = zero; q[2] = cnt;
        q = e[3];  q[1] = zero; q[2] = cnt;
        e += 6;
    } while (--i);
}

// @ 0x489c30
tstring DialogMgr::current_text()
{
    return cur_text;
}

// @ 0x489c50
int GameData::count_in(int id, int idx, char flag)
{
    Troop* tr = troops;
    int c = 0;
    int n = tr[idx].inv_count();
    for (int i = 0; i < n; i++)
        if (tr[idx].inv[i].item == id)
            c++;
    if (flag) {
        for (int k = 0; k < 10; k++)
            if (tr[idx].equip[k].item == id) c++;
    }
    return c;
}

// @ 0x489d10
int GameData::sl_take(int group, int item, int want, char extras)
{
    int total = 0;
    int left = want;
    int n = troops[group].inv_count();
    for (int i = 0; i < n; i++) {
        if (left <= 0)
            break;
        if (troops[group].inv[i].item == item) {
            total += sort_value(&troops[group].inv[i]);
            troops[group].inv[i].item = -1;
            left--;
        }
    }
    if (extras) {
        for (int i = 0; i < 10; i++) {
            if (left <= 0)
                break;
            if (troops[group].equip[i].item == item) {
                total += sort_value(&troops[group].equip[i]);
                left--;
                troops[group].equip[i].item = -1;
            }
        }
    }
    return total;
}

static inline int sl_rand(int n) { return n <= 1 ? 0 : rand() % n; }
static inline bool sl_bit(unsigned v, int n) { return ((v >> n) & 1) != 0; }
static inline QuestRec* sl_quest(QuestRec* q, int i) { return q + i; }

// @ 0x489e70
int GameData::sl_roll_party(int lo, int hi)
{
    int found = -1;
    int tries = 0;
    do {
        if (tries >= 1000)
            return found;
        found = sl_rand(hi - lo) + lo;
        tries++;
        QuestRec& pi = quests[found];
        if (!sl_bit(pi.flags2, 1) || sl_bit(pi.flags, 8))
            found = -1;
    } while (found == -1);
    return found;
}

// @ 0x489ee0
int GameData::sl_roll_other(int lo, int hi)
{
    int tries = 0;
    int span = hi - lo;
    int v;
    do {
        int r;
        if (span <= 1)
            r = 0;
        else
            r = rand() % span;
        v = r + lo;
        tries++;
        if (v == excluded)
            v = -1;
        if (tries > 100000)
            return 0;
    } while (v == -1);
    return v;
}

// @ 0x489f40
// Party strength factor from the difficulty and option settings, truncated to int (the x87 value is popped by the
// _ftol call). The constants are .rdata literals of this object (0.09, 0.15, 0.2, 1.2, 0.05, 1.05, 100.01, 0.75).
int GameData::sl_value_factor()
{
    float f = 1.0f;
    if (fb4)
        f = 0.75f;
    f = (1.0f - (float)g_options.difficulty * 0.09f) * (((g_options.f74 - 0.5f) * 0.15f + 1.0f) * f);
    f = f * ((float)(g_options.dmg_to_player - 2) * 0.15f + 1.0f);
    f = f * ((float)(g_options.dmg_to_friends - 2) * 0.2f + 1.0f);
    if (g_options.stick_mode == 1)
        f = f * 1.2f;
    return (int)(((float)(g_options.agent_count - 2) * 0.05f + 1.05f) * f * 100.01f);
}

// @ 0x48a000
void GameData::sl_update_avg()
{
    struct { float dt, now; int add; } s;
    s.now = (float)g_ticks[0] * 1e-05f;
    s.dt = s.now - avg_prev_time;
    s.add = sl_value_factor();
    float av = avg_value;
    avg_value = av * avg_weight + s.add * s.dt;
    avg_prev_time = s.now;
    float wsum = s.dt + avg_weight;
    avg_weight = wsum;
    avg_value = avg_value / wsum;
}

// @ 0x48a080
int GameData::sl_pick_chain(int leader, char lancer)
{
    int chain[32];
    chain[0] = leader;
    int n = 0;
    while ((unsigned)troops[chain[n]].upgrade_troop[0] > 0) {   // an unsigned compare in the original (jbe / ja)
        chain[n + 1] = troops[chain[n]].upgrade_troop[0];
        n++;
    }
    int cnt = troops[cur_troop].level / 3 + 1;
    int r;
    if (cnt <= 1)
        r = 0;
    else
        r = rand() % cnt;
    int r2 = r + 1;
    int lo = 1;
    if (lancer)
        lo = 0;
    if (r2 < lo)
        r2 = lo;
    if (r2 > n)
        return chain[n];
    return chain[r2];
}

// @ 0x48a150
int GameData::template_count(int party, int cell, float)
{
    PartyTemplate* t = &g_party_templates[party];
    int roll = rand() % 0x3dc9;
    float f = (float)roll * 6.322312e-05f;
    int lo = range_lo;
    int hi = range_hi;
    f = (f * (hi - lo) + lo) * (t->stacks[cell].max - t->stacks[cell].min) * 0.01f;
    return t->stacks[cell].min + rnd(&f);
}

// @ 0x48a210
void GameData::sl_item_flag_all()
{
    for (int i = 0; i < g_item_kind_count; i++)
        g_items[i].price_rate = 1.0f;
}

// @ 0x48a250
void GameData::sl_item_set_all(float v)
{
    for (int i = 0; i < g_item_kind_count; i++)
        g_items[i].merchandise_probability = v;
}

// @ 0x48a290
int GameData::sl_party_value(int party)
{
    if (party < 0)
        return 0;
    else {
        g_script.trigger_result.store0 = 0;
        g_script.trigger_result.store1 = 0;
        if (emit_event(7, party, cur_party) != 0)
            return g_script.trigger_result.store0;
        if (((__int64)troops[party].flags30 & 0x10) != 0)
            return 0;
        return troops[party].level * troops[party].level / 20 + 25;
    }
}

// @ 0x48a320
int GameData::sl_party_price(int party)
{
    if (party < 0)
        return 0;
    g_script.trigger_result.store0 = 0;
    g_script.trigger_result.store1 = 0;
    if (emit_event(9, party, cur_party))
        return g_script.trigger_result.store0;
    int v = sl_party_value(party);
    return (int)((float)v * 5.0f + 2.0f);
}

// @ 0x48a380
void GameData::set_names(int slot, const tstring& a)
{
    talk_lines[slot] = a;
}

// @ 0x48a3a0
void NamedRec::setup(const tstring* a, const tstring* b)
{
    name68 = *a;
    name1c = *b;
    answer = 0;
    text = 0;
    f04 = 0;
}

// @ 0x48a3d0
void NamedRec::setup2(const tstring* a, const tstring* b, const tstring* c, int* v)
{
    name68.clear();
    name1c = *a;
    name78 = *b;
    name88 = *c;
    answer = v;
    text = 0;
    f04 = 1;
}


// @ 0x48a420
rgl::matrix Mission::get_frame()
{
    if (blend_time > 0.0f) {
        float t = (g_ticks[blend_timer.clock] - blend_timer.start) * 1e-5f / blend_time;
        if (blend_n > 0) {
            t = atan((t - 0.5f) * (float)blend_n * 3.1415927f) /
                    (atan((float)blend_n * 1.5707964f) * 2.0f) + 0.5f;
        }
        if (t >= 1.0f) {
            return frame_b;
        }
        float s = 1.0f - t;
        rgl::matrix m;
        // each row is two scaled copies added lane by lane (the vector4 operators return their products through
        // stack temporaries, which rounds them to float). The 4th lane of the sum is never written: the copy into
        // the matrix carries a stale stack dword.
        m.o = frame_b.o * t + frame_a.o * s;
        m.f = frame_b.f * t + frame_a.f * s;
        m.u = frame_b.u * t + frame_a.u * s;
        m.s = frame_b.s * t + frame_a.s * s;
        m.orthonormalize();
        return m;
    }
    return frame_a;
}

// @ 0x48a740
float Mission::blend_param()
{
    if (blend_time > 0.0f) {
        float t = (g_ticks[blend_timer.clock] - blend_timer.start) * 1e-5f;
        t = t / blend_time;
        if (t >= 1.0f)
            return blend_to;
        return (1.0f - t) * blend_from + t * blend_to;
    }
    return blend_from;
}

// @ 0x48a7d0
float Mission::blend2_param()
{
    if (time_speed_anim_time > 0.0f) {
        float t = (g_ticks[time_speed_timer.clock] - time_speed_timer.start) * 1e-5f;
        t = t / time_speed_anim_time;
        if (t >= 1.0f)
            return target_time_speed;
        return (1.0f - t) * time_speed + t * target_time_speed;
    }
    return time_speed;
}

// @ 0x48a860
void Mission::start_clock(int v)
{
    clock_set = 1;
    int d = (int)((float)-v * 100000.0f);
    clock.start = g_ticks[clock.clock] - d;
}

// @ 0x48a8b0
int Mission::team_count()
{
    int n = 0;
    if (0 < counts[0][1]) n = 1;
    if (0 < counts[1][1]) n++;
    if (0 < counts[2][1]) n++;
    if (0 < counts[3][1]) n++;
    if (0 < counts[4][1]) n++;
    if (0 < counts[5][1]) n++;
    if (0 < counts[6][1]) n++;
    if (0 < counts[7][1]) n++;
    return n;
}

// @ 0x48a910
int Mission::last_team()
{
    int r = -1;
    if (0 < counts[0][1]) r = 0;
    if (0 < counts[1][1]) r = 1;
    if (0 < counts[2][1]) r = 2;
    if (0 < counts[3][1]) r = 3;
    if (0 < counts[4][1]) r = 4;
    if (0 < counts[5][1]) r = 5;
    if (0 < counts[6][1]) r = 6;
    if (0 < counts[7][1]) r = 7;
    return r;
}

// @ 0x48a990
bool Mission::point_in_rect(const float* p)
{
    return min_x <= p[0] && p[0] < max_x &&
           min_y <= p[1] && p[1] < max_y;
}

// @ 0x48a9e0
void Mission::select_team(int i)
{
    TeamRec* r = &teams[i];
    r->cls[0].saved_use = r->cls[0].use;
    r->cls[1].saved_use = r->cls[1].use;
    r->cls[2].saved_use = r->cls[2].use;
}

// @ 0x48aa20
void Mission::unselect_team(int i)
{
    TeamRec* r = &teams[i];
    r->cls[0].use = r->cls[0].saved_use;
    r->cls[1].use = r->cls[1].saved_use;
    r->cls[2].use = r->cls[2].saved_use;
}

// @ 0x48aa60
void Mission::select_class(int i, int s)
{
    TeamRec* r = &teams[i];
    if (s >= 0 && s < 3) {
        r->cls[0].use = 0;
        r->cls[1].use = 0;
        r->cls[2].use = 0;
        r->cls[s].use = 1;
    } else {
        r->cls[0].use = 1;
        r->cls[1].use = 1;
        r->cls[2].use = 1;
    }
}

// @ 0x48aac0
int Mission::movement_order(int i)
{
    TeamRec* r = &teams[i];
    for (int k = 0; k < 3; k++) {
        if (r->cls[k].use != 0) {
            switch (r->cls[k].kind) {
            case 0: return 2;
            case 2: return 1;
            case 1: return 0;
            case 3: return 11;
            default: return -1;
            }
        }
    }
    return -1;
}

// @ 0x48ab30
int Mission::riding_order(int i)
{
    TeamRec* r = &teams[i];
    int v = 0;
    while (v < 3) {
        if (r->cls[v].use != 0) {
            v = r->cls[v].k4;
            goto sw;
        }
        v++;
    }
    v = -1;
sw:
    switch (v) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 2;
    }
    return -1;
}

// @ 0x48ab90
int Mission::weapon_usage_order(int i)
{
    int v = -1;
    for (int k = 0; k < 3; k++) {
        if (teams[i].cls[k].use) {
            v = teams[i].cls[k].a;
            break;
        }
    }
    switch (v) {
    case 0: return 0;
    case 1: return 1;
    }
    return -1;
}

// @ 0x48abf0
int Mission::hold_fire_order(int i)
{
    int v = -1;
    for (int k = 0; k < 3; k++) {
        if (teams[i].cls[k].use) {
            v = teams[i].cls[k].b;
            break;
        }
    }
    switch (v) {
    case 0: return 0;
    case 1: return 1;
    }
    return -1;
}

// @ 0x48ac50
bool GameKeys::is_down(int i)
{
    bool r = false;
    int a = entry[i][0];
    if (a >= 0 && g_keyboard.down[a] != 0)
        r = true;
    int b = entry[i][1];
    if (b >= 0 && g_keyboard.down[b] != 0)
        r = true;
    return r;
}

// @ 0x48ac90
bool GameKeys::clicked(int i)
{
    bool r = false;
    int a = entry[i][0];
    if (a >= 0 && g_keyboard.down[a] != 0 && g_keyboard.prev[a] == 0)
        r = true;
    int b = entry[i][1];
    if (b >= 0 && g_keyboard.down[b] != 0 && g_keyboard.prev[b] == 0)
        r = true;
    return r;
}

// @ 0x48ace0
int Vec68::count()
{
    if (!begin)
        return 0;
    return end - begin;
}

// @ 0x48ad00
int Map10007::lookup(unsigned id)
{
    if (id < 0x2717)
        return tab[id].value;
    int h = id % 0x2717;
    int n = 0;
    if (tab[h].key != id) {
        do {
            if (n >= 1000)
                return -1;
            h = (h + 0x4f) % 0x2717;
            n++;
        } while (tab[h].key != id);
    }
    if (n < 1000)
        return tab[h].value;
    return -1;
}

// @ 0x48ad70
void fill8(int* first, int* last, const int* val)
{
    for (; first != last; first += 2) {
        first[0] = val[0];
        first[1] = val[1];
    }
}

// @ 0x48ada0
void move_backward8(__int64* first, __int64* last, __int64* dest)
{
    int n = (int)(last - first);
    memmove(dest - n, first, n * 8);
}

// @ 0x48add0
void TimerList::update(int flag)
{
    for (int i = 0; i < count; i++) {
        items[i].tick(flag);
        if (flag > 0 && (flag & 1) && g_game->mode == 0x12)
            return;
    }
}


// a two-float temporary built from const float& (each lane goes through a stack slot: 0x48ae20)
struct UiPt : V2 { UiPt(const float& a, const float& b) { x = a; y = b; } };
// @ 0x48ae20
MissionUi::MissionUi()
{
    f100 = 1;
    position = UiPt(0.0f, 0.0f);
    rel_position = UiPt(0.0f, 0.0f);
    p154 = 0;
    f104 = 0;
    for (int i = 0; i < 5; i++) {
        tracks[i] = 0;
        b11c[i] = 0;
    }
    text_offset = UiPt(0.005f, 0.0f);
    text_scale = UiPt(0.024f, 0.024f);
    color14c = 0xff000000;
    color150 = 0xff000000;
    b11c[0] = 1;
    b11c[2] = 1;
    f164 = 1;
    rect_min = UiPt(0.0f, 0.0f);
    f148 = 0x11;
    f170 = 1.0f;
    rect_max = UiPt(0.0f, 0.0f);
    periodA = -1.0f;
    periodB = -1.0f;
    periodC = -1.0f;
    periodD = -1.0f;
    periodE = -1.0f;
    periodF = -1.0f;
    cur_size = UiPt(1.0f, 1.0f);
    font = 0;
    fC8 = 0;
    fCC = 0;
    fD0 = 0;
    fD4 = 0;
    f158 = 0;
    v15c = 0;
    f160 = 0;
    *(int*)&v168 = 0;
    *(int*)&f16c = 0;
    b121 = 0;
    b122 = 0;
    c0 = 0;
    c1 = 0;
    c2 = 0;
    c3 = 0;
    c4 = 0;
    c5 = 0;
}

// @ 0x48b090
void MissionUi::set_size(const vec2& p)
{
    if (f148 & 2)
        f148 &= ~2u;
    text_scale.x = p.x;
    text_scale.y = p.y;
    if ((f148 & 0x200) && !(f148 & 0x100))
        set_xy(V2(1.0f, 0.75f));
    if (f148 & 0x200) {
        V2 v = get_xy();
        float m = rect_max.x;
        if (v.x < m)
            m = v.x;
        set_xy(V2(m, v.y));
        text_offset.y = v.y - text_scale.y * 0.63f;
        get_xy();
    }
}

// The constructor 0x48b1c0, out of line: the TUs that see the body in ui_widgets.h (all but this one and party_window.cpp) inline it.
UiA::UiA() { f180 = 0; f184 = 0; }

// The constructor on raw memory, for the callers that build a toggle button by hand (char_sheet/view_open.cpp elem2).
void* UiA::construct()
{
    new (this) UiA;
    return this;
}

// @ 0x48b170
bool UiA::is_active() { return true; }

// @ 0x48b180
void UiA::flag2_set()
{
    b121 = 0;
    f104 |= 2;
}

// @ 0x48b1a0
void UiA::hide()
{
    b121 = 0;
    f104 |= 1;
}

// @ 0x48b1f0
int UiA::kind() { return 1; }

// @ 0x48b200
char UiA::key(unsigned p)
{
    char b = 0;
    if (f184 & 1) {
        if (!(p & 0x10))
            goto tail;
        b = 1;
        if (!(f184 & 2) || v15c == 0) {
            g_gui.play_sound();
            f180 = (f180 == 0);
            set_key_flags(0x10000);
            v15c = (v15c == 0);
        }
    } else {
        if (p & 4)
            goto clear;
        if (p & 0x10) {
            b = 1;
            g_gui.play_sound();
            f180 = 1;
            goto tail;
        }
        if (!(p & 8) || f180 != 1)
            goto tail;
        b = 1;
        set_key_flags(0x10000);
clear:
        f180 = 0;
    }
tail:
    set_key_flags(p);
    refresh();
    return b;
}
