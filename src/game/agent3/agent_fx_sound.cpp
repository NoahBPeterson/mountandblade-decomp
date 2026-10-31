//-----------------------------------------------------------------------------
// File: agent_fx_sound.cpp  (0x441ff0..0x444c28)
//
// Desc: an object that keeps int lists, the 0x38-byte record copy / fill helpers, the world's "entities fully
//       touching a box" query, the effect element / definition lists, the emitter setup from the definition
//       table, the FMOD sound player, game-data experience (level-up bookkeeping and its messages), the mission's
//       effect spawn and dead-agent queue, the agent's target scans, sound pickers and players, the equipment
//       display and the wield-change animation / sound selection. Functions in ascending address order (cl's
//       bottom-up emission). Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include "./agent_fx_sound.h"
#include "../tstring.h"
#include "./agent_types.h"      // (named here too: the resolution audit reads the annotations of directly included headers)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../str_table.h"
#include "../../include/mat4.h"
#include "../../include/timer.h"
#include "../cell.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "../world.h"
#include "../../include/fx.h"
#include "../emit.h"
#include "../anim_slot.h"
#include "../skeleton.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../agent_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/msg_log.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/party.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/sound_mgr.h"      // (listed directly: the resolution audit scans only the source's own includes)

// 0x440b30 vector<int>::push_back (agent_combat.cpp) is kept out of line
template<> void std::vector<int>::push_back(const int&);

template<class T> __forceinline void vpush(std::vector<T>& v, const T& x)
{
    if (v.size() < v.capacity()) {
        T* p = v._Mylast;
        *p = x;
        v._Mylast = p + 1;
    } else {
        v.insert(v.end(), x);
    }
}

// @ 0x441ff0
void Poser::add_to_list(int idx, int val)
{
    vpush(lists[idx], val);
    if (idx + 1 > nlists)
        nlists = idx + 1;
}

// ---- std::_Uninit_copy for the 0x38-byte E56 record ----------------------------------------------------------
// @ 0x442060
void alloc_e56::construct(E56* p, const E56& v) { new ((void*)p) E56(v); }

// @ 0x442080 (+ catch-all funclet 0x4420ea: destroys [next, dest), then `throw;`)
E56* __cdecl uninit_copy_e56(const E56* first, const E56* last, E56* dest, alloc_e56& al)
{
    E56* next = dest;
    try {
        for (; first != last; ++dest, ++first)
            al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next)
            al.destroy((VecPair2*)next);
        throw;
    }
    return dest;
}

// @ 0x442110
void Poser::add_extra(int val)
{
    vpush(extra, val);
    changed();
}

// ---- the world's box query --------------------------------------------------------------------------------------------
static __forceinline bool box3_inl(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y && a->z <= d->z && c->z <= b->z)
        return true;
    return false;
}

// @ 0x442180
void world::collect_boxed(const vector4* bmin, const vector4* bmax, std::vector<unsigned>* out)
{
    if (out->_Myfirst != 0)
        operator delete(out->_Myfirst);
    out->_Myfirst = 0;
    out->_Mylast = 0;
    out->_Myend = 0;
    int lo[2];
    volatile int hi[2];                     // read from memory at every test (that is what the original compiled to)
    int c[2];
    ((rgl::world*)this)->to_cell(&bmin->x, lo);
    ((rgl::world*)this)->to_cell(&bmax->x, (int*)hi);
    for (c[0] = lo[0]; c[0] <= hi[0]; c[0]++) {
        for (c[1] = lo[1]; c[1] <= hi[1]; c[1]++) {
            for (enode* nd = cells[c[0]][c[1]].ents; nd != 0; nd = nd->next) {
                EntView** pe = &nd->ent;
                if (box3_inl(bmin, bmax, (*pe)->get_min(), (*pe)->get_max()))
                    out->push_back(*(unsigned*)pe);
            }
        }
    }
    std::vector<rgl::entity*>& ents = *(std::vector<rgl::entity*>*)out;      // the grid collects entity pointers
    sort_range(ents.begin(), ents.end(), (int)(out->end() - out->begin()));
    std::vector<unsigned>::iterator e = std::unique(out->begin(), out->end());
    out->erase(e, out->end());
}

// ---- effect elements / definitions ----------------------------------------------------------------------------------------
// @ 0x442390
void FxElem::add_psys(int p) { vpush(*(std::vector<int>*)&psys, p); }   // the list holds Psys* stored as ints
// @ 0x4423f0
void FxElem::add_group(int g)
{
    vpush(*(std::vector<int>*)&groups, g);
    f1ac++;
}
// @ 0x442460
void FxDef::attach_psys(int idx, Psys* p, const mat4* m)
{
    p->frame = *m;
    elems[idx].add_psys((int)p);
}
// @ 0x4424a0
void FxDef::attach_group(int idx, PsysGroup* g)
{
    elems[idx].add_group((int)g);
}

// ---- emitter setup from a definition table ----------------------------------------------------------------------------------
// @ 0x4424c0
void EmitTable::setup(Emit* e, int idx, const vector4* pos)
{
    if (idx < 0 || idx >= count)
        return;
    EmitDef* d = &defs[idx];
    res_obj* r = g_resmgr.get_object(&d->name);
    if (r == 0)
        return;
    e->init();
    e->idx = idx;
    e->obj = r;
    e->res = r->f2c;
    e->flags = d->f10;
    e->f198 = d->f24;
    e->fa0 = d->f30;
    e->f24 = d->f34;
    vector4 g(0.0f, 0.0f, -9.8f);
    e->gravity = g;
    e->fd4 = d->f38;
    e->f1c = d->f3c;
    e->f20 = d->f40;
    for (int k = 0; k < 2; k++) {
        e->t[0][k] = d->t[0][k];
        e->t[1][k] = d->t[1][k];
        e->t[2][k] = d->t[2][k];
        e->t[3][k] = d->t[3][k];
        e->t[4][k] = d->t[4][k];
    }
    e->f1b0 = d->fb8;
    e->f1b4 = d->fbc;
    mat4 m = rgl::identity_pos();
    m.o = *pos;
    ((Obj3*)e)->set_placement(&m, &d->a, &d->b, d->c);
}

// ---- the FMOD sound player ---------------------------------------------------------------------------------------------------
static __forceinline int round_i(float v) { return round_float(&v); }
// @ 0x442650
int SoundMgr::play(int idx, int flags, const float* pos, float freq, int* out_id)
{
    int ret = -1;
    if (g_sound_off == 0 && idx >= 0 && idx < ngroups) {
        int* pn = &groups[idx].nvar;
        int n = *pn;
        if (n > 0) {
            int k = 0;
            if (n > 1)
                k = rand() % *pn;
            int dn = groups[idx].var[k];
            int ch = FSOUND_PlaySoundEx(-1, defs[dn].sample, 0, 1);
            ret = ch;
            if (out_id)
                *out_id = defs[dn].id;
            if (ch >= 0) {
                int looping = groups[idx].flags & 2;
                int hi = 0x100;
                int vol = 0;
                int r = round_i(((const Agent*)&groups[idx])->volume_scale() * g_master_volume * 255.0f);
                vol = clamp_index(&r, &vol, &hi);
                int prio;
                if (flags & 1) {
                    FSOUND_3D_SetAttributes(ch, 0, 0);
                    prio = 0xff;
                } else {
                    FSOUND_3D_SetAttributes(ch, pos, 0);
                    float dist = (listener - *(const vector4*)pos).out_length();
                    float f = 3.0f / (dist + 3.0f);
                    int lo = 0;
                    int hi2 = 0x100;
                    int pr = round_i(((const Agent*)&defs[dn])->range_scale() * f * 254.0f);
                    prio = clamp_index(&pr, &lo, &hi2);
                }
                FSOUND_SetVolume(ch, vol);
                FSOUND_SetLoopMode(ch, (looping != 0) + 1);
                if (groups[idx].flags & 4)
                    FSOUND_SetCurrentPosition(ch, rand_below(&defs[dn].length));
                if (looping) {
                    int t = ch;
                    channels.push_back(t);
                }
                if (freq != 1.0f && g_no_freq == 0)
                    FSOUND_SetFrequency(ch, (int)((float)defs[dn].freq * freq));
                FSOUND_SetPriority(ch, prio);
                FSOUND_SetPaused(ch, 0);
            }
        }
    }
    return ret;
}
// @ 0x4428a0
int SoundMgr::play_slot(int n, int flags, const float* pos, float freq, int* out_id)
{
    n = slot[n];
    return play(n, flags, pos, freq, out_id);
}

// ---- experience and its messages ------------------------------------------------------------------------------------------------
// the ring-buffer append that this TU inlines (colour 0xfff000)
static __forceinline void log_add(const tstring& s)
{
    if (g_gui.enabled && g_gui.ring.count < 32) {
        int idx = (g_gui.ring.head + g_gui.ring.count) % 32;
        g_gui.ring.names[idx] = s;
        g_gui.ring.vals[idx] = 0xfff000;
        g_gui.ring.count++;
    }
}

// @ 0x4428c0
void GameData::add_experience(int troop, int amount)
{
    char buf1[512], buf2[512], buf3[512];
    if (amount < 0)
        amount = 0;
    else if (amount >= 30000)
        amount = 29999;
    if (troop == player && amount > 0 && f13a34 != 0 && g_show_messages != 0) {
        {
            tstring key("ui_you_got_number_experience");
            sprintf(buf1, g_strtable.lookup(key)->c_str(), amount);
        }
        {
            tstring msg(buf1);
            g_gui.push(msg, -1);
        }
    }
    TroopRec* c = &chars[troop];
    c->exp += amount;
    while (c->exp >= g_xp_table[c->level]) {
        c->fa4++;
        c->fa8++;
        c->fac += 10;
        c->level++;
        if (troop == player) {
            g_sound_mgr.play(g_levelup_sound, 1, ((const float*)&g_sound_mgr.listener), 1.0f, 0);
            {
                tstring key("ui_you_have_advanced_to_level_number");
                sprintf(buf2, g_strtable.lookup(key)->c_str(), c->level);
            }
            {
                tstring msg(buf2);
                log_add(msg);
            }
        } else {
            PartyMgr* pm = &parties;
            int p = ((Map10007*)pm)->lookup(f78);
            int i = 0;
            if (pm->pages._Myfirst[p >> 4][p & 0xf].count > 0) {
                do {
                    int q = ((Map10007*)pm)->lookup(f78);
                    if (pm->pages._Myfirst[q >> 4][q & 0xf].stacks[i].troop == troop) {
                        {
                            tstring key("ui_value_has_advanced_to_level_number");
                            tstring* nm = g_strtable.find(&c->key);
                            if (nm == 0)
                                nm = (tstring*)c;
                            sprintf(buf3, g_strtable.lookup(key)->c_str(), nm->c_str(), c->level);
                        }
                        {
                            tstring msg(buf3);
                            log_add(msg);
                        }
                    }
                    i++;
                    int r = ((Map10007*)pm)->lookup(f78);
                    if (!(i < pm->pages._Myfirst[r >> 4][r & 0xf].count))
                        break;
                } while (1);
            }
        }
    }
}

// ---- spawning an effect emitter ---------------------------------------------------------------------------------------------------
// @ 0x442ca0
Emit* Mission::spawn_effect(int idx, const mat4* m, int duration)
{
    Emit* e = (Emit*)((rgl::world*)scene_world)->alloc_emitter();
    vector4 v(0.0f, 0.0f, 0.0f);
    g_emit_defs.setup(e, idx, &v);
    e->world = *m * e->local;
    if (e->res != 0 && (e->res->flags & 0x40)) {
        CamRec* c = &g_env_rows[cam];
        vector4 t;
        t.x = c->dx * 0.3f;
        t.y = c->dy * 0.3f;
        t.z = c->dz * 0.3f;
        v.x = t.x + c->px;
        v.y = t.y + c->py;
        v.z = t.z + c->pz;
        e->offs = v;
    }
    e->flags &= ~1u;
    e->f19c = e->f1a0 = duration;
    e->f1a4 = 0;
    return e;
}

// @ 0x442dc0
void Mission::queue_dead(Agent* a)
{
    dead.push_back(a);
    while (dead.size() > g_max_dead) {
        dead[0]->f5830 = 4;
        Agent* o = dead[0];
        Timer* tm = &o->tm;
        tm->start = g_ticks[tm->clock];      // Timer::restart inlined
        for (unsigned i = 1; i < dead.size(); i++)
            dead[i - 1] = dead[i];
        dead.pop_back();
    }
}

// ---- agent scans -------------------------------------------------------------------------------------------------------------------
// @ 0x442ed0
bool Agent::can_target(unsigned aid, unsigned flags, float maxdist, float* out_dist)
{
    Agent* o = &g_mission->agents.pages[aid >> 4][aid & 0xf];
    if (o->f14 == 1
        && (!(flags & 1) || !is_enemy(o))
        && (!(flags & 2) || !((const AgentX*)this)->is_friend(o))
        && (!(flags & 4) || !(char)((const AgentX*)this)->is_neutral(o))
        && (!(flags & 0x10) || o->f10 == 1)
        && (!(flags & 0x20) || o->f10 != 1)
        && (!(flags & 0x200) || o->id > id)
        && o->id != id
        && (!(flags & 0x2000) || line_of_sight(aid, 0))
        && o->id != f2c) {
        vector4 d = o->pos - pos;
        float len = d.normalize();
        *out_dist = len;
        if (maxdist >= 0.0f && len > maxdist)
            return false;
        if (flags & 0x1000) {
            float dot = d.x * fx + (d.z * fz + d.y * fy);
            if (dot < -0.1f)
                return false;
        }
        return true;
    }
    return false;
}

// @ 0x443020
bool Agent::any_hostile_near()
{
    Mission* m = g_mission;
    AgentTable* t = &m->agents;
    unsigned i = t->first_used();
    while (i < (unsigned)t->count) {
        Agent* o = &m->agents.pages[i >> 4][i & 0xf];
        if (o->f14 == 1) {
            bool friendly = false;
            if (team >= 0 && team < 8 && o->team >= 0 && o->team < 8)
                friendly = m->rel_flat()[team * 0x5b + o->team] == 1;
            if (!friendly && !(char)((const AgentX*)this)->is_neutral(o) && o->f10 != 1 && o->id != id && o->id != f2c) {
                vector4 d;
                d.x = o->pos.x - pos.x;
                d.y = o->pos.y - pos.y;
                d.z = o->pos.z - pos.z;
                float len = d.normalize();
                if (!(300.0f < len) && line_of_sight(i, 0))
                    return true;
                m = g_mission;
                t = &m->agents;
            }
        }
        t->next_used(&i);
    }
    return false;
}

// @ 0x443140
void Agent::update_alarm()
{
    if (f2c4 & 1)
        return;
    bool hit = false;
    AgentTable* t = &g_mission->agents;
    unsigned i = t->first_used();
    unsigned n = t->count;
    while (i < n) {
        if (hit)
            break;
        Agent** pg = g_mission->agents.pages._Myfirst;
        Agent* o = pg[i >> 4] + (i & 0xf);
        if (o->f14 == 1 && !is_enemy(o) && !(char)((const AgentX*)this)->is_neutral(o) && o->f10 != 1 && o->id != id && o->id != f2c) {
            vector4 d = o->pos - pos;
            if (!(10.0f < d.normalize()))
                hit = (pg[i >> 4][i & 0xf].f2c4 & 1) != 0;
        }
        g_mission->agents.next_used(&i);
    }
    if (hit || any_hostile_near()) {
        f2c4 |= 1;
        f2c0 = 0;
    }
}

// ---- agent sound pickers and players ---------------------------------------------------------------------------------------------------
// @ 0x443270
unsigned Agent::pick_shout_target(int flags, float maxdist)
{
    float bestv = 0.0f;
    unsigned best = (unsigned)-1;
    int limit = *(int*)&maxdist;
    AgentTable* tbl = &g_mission->agents;
    unsigned i = tbl->first_used();
    if (i < (unsigned)tbl->count) {
        do {
            unsigned id = i;
            bool ok = can_target(id, flags, *(float*)&limit, &maxdist);
            Mission* m = g_mission;
            if (ok) {
                float x = 3.0f / (maxdist + 2.0f);
                float t = (x < 1.0f) ? x : 1.0f;
                float k = 1.0f;
                float w = 1.0f / ((float)m->agents.pages[id >> 4][id & 0xf].f55e8 + 7.0f);
                if (f2c >= 0 && maxdist < 40.0f) {
                    vector4 d;
                    const vector4& op = m->agents.pages[id >> 4][id & 0xf].pos;
                    d.x = op.x - pos.x;
                    d.y = op.y - pos.y;
                    d.z = op.z - pos.z;
                    maxdist = d.normalize();
                    const vector4& fw = body->m.f;
                    float dp = d.y * fw.y + d.z * fw.z + d.x * fw.x;
                    if (dp > 0.0f)
                        k = ((40.0f - maxdist) * dp) * dp + 1.0f;
                }
                float s = (k * w) * t;
                if (s > bestv) { bestv = s; best = id; }
            }
            tbl = &m->agents;
            tbl->next_used(&i);
        } while (i < (unsigned)tbl->count);
        return best;
    }
    return best;
}

// @ 0x443430
unsigned Agent::pick_threat()
{
    float score;
    unsigned best = (unsigned)-1;
    float bestv = -100.0f;
    unsigned id = (unsigned)-1;
    int n = 0;
    if (0 < nthreat) {
        Threat* r = threats;
        do {
            id = r->id;
            Agent* o = &g_mission->agents.pages[id >> 4][id & 0xf];
            if (o->f14 == 1 && line_of_sight(id, 0)) {
                if (fire_check(id, (int*)&score) == 1) {
                    const vector4& op = g_mission->agents.pages[id >> 4][id & 0xf].pos;
                    float dx = op.x - pos.x;
                    float dy = op.y - pos.y;
                    float dz = op.z - pos.z;
                    float v = 3.0f / (float)sqrt(dy * dy + (dx * dx + dz * dz));
                    if (!(v < 4.0f)) v = 4.0f;
                    score = v / ((float)g_mission->agents.pages[(unsigned)r->id >> 4][r->id & 0xf].f55e8 + 7.0f);
                    float s = ((float)(rand() % 0x3dc9) * 1.2644623e-05f + 1.0f) * score;
                    if (s > bestv) { bestv = s; best = r->id; }
                }
            }
            n++;
            r++;
            id = best;
        } while (n < nthreat);
    }
    return id;
}

// @ 0x4435c0
void Agent::play_hit_sound(int* kind)
{
    Mission* m = g_mission;
    if (m->terrain.muted)
        return;
    int snd;
    if (f10 == 0) {
        int k = *kind;
        if (k < 0)
            return;
        if (k > 1) {
            if (k != 3)
                return;
            snd = 0xe;
        } else if (!(pos.z < m->terrain.base_z)) {
            snd = (g_game->slots[m->f5df4].flags8 & 1) ? 0x10 : 0xf;
        } else {
            snd = 0x11;
        }
    } else {
        int k = *kind;
        unsigned c = fb8 & 7;
        if (k == 4) {
            snd = 0x61;
        } else if (pos.z > m->terrain.base_z) {
            if (c == 2 || c == 0) {
                if (k != 0 && k != 2) return;
                snd = 0x67;
            } else if (c == 3) {
                if (k != 0 && k != 2) return;
                snd = 0x68;
            } else if (c == 4) {
                if (k != 0) return;
                snd = 0x69;
            } else {
                if (c != 5) return;
                if (k != 0) return;
                snd = 0x6a;
            }
        } else {
            if (k != 2 && k != 3) return;
            snd = 0x11;
        }
    }
    int r = rand();
    float pitch = (float)(r % 0x3dc9) * 2.5289246e-05f + 0.8f;
    g_sound_mgr.play(g_sound_ids[snd], 0, &body->m.o.x, pitch, 0);
}

// @ 0x443730
void Agent::play_sound_var(int* kind, float* var)
{
    if (*kind != -1 && body != 0) {
        float pitch = 1.0f;
        if (*var > 0.001f) {
            float v = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * *var;
            v = v + v;
            if (v > 0.001f)
                pitch = v + 1.0f;
            else if (v < -0.001f)
                { float nv = -v; pitch = 1.0f / (nv + 1.0f); }
        }
        int own = 0;
        if (id == g_mission->player_id)
            own = 1;
        vector4 p = body->m.o;
        float h = (f10 == 0 && f2c != -1) ? 2.4f : 1.5f;
        p.z = (h - 0.3f) + p.z;
        g_sound_mgr.play(g_sound_ids[*kind], own, &p.x, pitch, (int*)&pitch);
    }
}

// @ 0x443870
void Agent::play_sound(int* snd)
{
    if (*snd != -1 && body != 0) {
        if (channel >= 0) {
            FSOUND_StopSound(channel);
            channel = -1;
        }
        float pitch = 1.0f;
        Mission* m = g_mission;
        if (f10 == 0 && (g_game->chars[troop].flags32 & 0xf) != 1 && id != m->player_id) {
            float v = ((float)(((f1bc * 0x244ad + (id + 0x1d) * 0x14fd + troop * 0x76ef) % 0x1b41) * 0x51f % 0x367)
                       * 0.0011481056f - 0.5f) * 0.3f;
            if (v > 0.001f)
                pitch = v + 1.0f;
            else if (v < -0.001f) {
                float nv = -v;
                pitch = 1.0f / (nv + 1.0f);
            }
        }
        int own = 0;
        if (id == m->player_id)
            own = 1;
        int ch = g_sound_mgr.play(*snd, own, &body->m.o.x, pitch, (int*)&pitch);
        if (ch >= 0) {
            channel = ch;
            stamp = g_ticks[f5860];
            f5870 = pitch;
        }
    }
}

// @ 0x443a00
void Agent::play_pain_sound(float health)
{
    health = (1.2f - health) * 0.3f;
    float v = (health < 0.0f) ? 0.0f : ((health >= 0.5f) ? 0.5f : health);
    int r = rand();
    float pitch = (float)(r % 0x3dc9) * 6.3223115e-06f + (v + 0.95f);
    g_sound_mgr.play(g_sound_ids[0x11], 0, &body->m.o.x, pitch, 0);
}

// ---- the equipment display ----------------------------------------------------------------------------------------------------------------
static inline unsigned char itype(const ItemKind* k) { return *(const unsigned char*)&k->type_flags; }

// shows `m` on the carry bone record `r` with local frame `f`
static __forceinline void attach(Agent* a, Mesh* m, int r, const pos2& f)
{
    Skeleton* g = a->body->skel;
    g->remove_shape((shape_base*)m);
    m->frame = f;
    Region& reg = ((Region*)g->recs)[r];
    vpush((std::vector<int>&)reg.shapes, (int)m);
    reg.count++;
}

// @ 0x443aa0
void Agent::update_equipment_display()
{
    char slot_used[4];
    char bone_used[11];
    int i;
    for (int q = 0; q < 11; q++)       // (an explicit loop: the `= {0}` form gives a different frame)
        bone_used[q] = 0;
    for (i = 3; i >= 0; i--) {
        if (pairs[i].item >= 0) {
            ItemKind* ik = &g_items[pairs[i].item];
            if (A[i] != 0) {
                A[i]->vis = 0;
                if (itype(ik) == itp_type_shield) {
                    Mesh* m = A[i];
                    float limit = 300.0f;
                    int k = 0;
                    if (0 < m->nlists) {
                        do {
                            m->lists[k].cap_all(&limit);
                            k++;
                        } while (k < m->nlists);
                        k = 0;
                    }
                }
            }
            if (B[i] != 0)
                B[i]->vis = 0;
            if (C[i] != 0) {
                C[i]->vis = 0;
                Mesh* m = C[i];
                float limit = 0.0f;
                int k = 0;
                if (0 < m->nlists) {
                    do {
                        m->lists[k].cap_all(&limit);
                        k++;
                    } while (k < m->nlists);
                    k = 0;
                }
            }
            slot_used[i] = 0;
        }
    }
    int j;
    for (j = 0; j < 2; j++) {
        int s = wield[j];
        if (s >= 0 && pairs[s].item >= 0 && (j == 0 || f23c == 0)) {
            ItemKind* ik = &g_items[pairs[wield[j]].item];
            unsigned t = ik->type_flags & 0xff;
            int region = -1;
            if (t == 2 || t == 3 || t == 4 || t == 9 || t == 10 || t == 0x10 || t == 0x11)
                region = 0x13;
            if (t == 8)
                region = 0xe;
            else if (t == 7)
                region = 0xc;
            vector4 z0(0.0f, 0.0f, 0.0f);
            mat3 t3;
            mat3* ip = identity3(&t3);
            pos2 M;
            *(mat3*)&M = *ip;
            M.o = z0;
            if ((g_items[pairs[s].item].flags & itcf_shoot_mask) == itcf_throw_javelin) {
                M.f = vector4(-M.f.x, -M.f.y, -M.f.z);
                M.s = vector4(-M.s.x, -M.s.y, -M.s.z);
            }
            if (A[wield[j]] != 0) {
                A[wield[j]]->vis = 0xffff;
                Mesh* m = A[wield[j]];
                float limit = 0.0f;
                int k = 0;
                if (0 < m->nlists) {
                    do {
                        m->lists[k].cap_all(&limit);
                        k++;
                    } while (k < m->nlists);
                }
                attach(this, A[wield[j]], region, M);
            }
            if (B[wield[j]] != 0) {
                B[wield[j]]->vis = 0xffff;
                attach(this, B[wield[j]], region, M);
            }
            if ((ik->flags & itcf_carry_mask) != 0 && (ik->flags & itcf_show_holster_when_drawn) == 0)
                slot_used[wield[j]] = 1;
            Mesh* h = C[wield[j]];
            if (h != 0) {
                if ((ik->flags & itcf_show_holster_when_drawn) == 0) {
                    h->vis = 0;
                } else {
                    float limit = 10.0f;
                    int k = 0;
                    if (0 < h->nlists) {
                        do {
                            h->lists[k].cap_all(&limit);
                            k++;
                        } while (k < h->nlists);
                    }
                }
            }
        }
    }
    for (i = 3; i >= 0; i--) {
        if (pairs[i].item >= 0) {
            ItemKind* ik = &g_items[pairs[i].item];
            if ((ik->flags & itcf_carry_mask) != 0) {
                pos2 M;
                int region, bone;
                ik->get_carry_pos(&M, &region, &bone);
                if (!bone_used[bone] && !slot_used[i] && (itype(ik) != itp_type_shield || (pairs[i].m4 & 0xffff) > 10)) {
                    bone_used[bone] = 1;
                    if (C[i] == 0) {
                        if (A[i] != 0) {
                            A[i]->vis = 0xffff;
                            attach(this, A[i], region, M);
                        }
                        if (B[i] != 0) {
                            B[i]->vis = 0xffff;
                            attach(this, B[i], region, M);
                        }
                    } else {
                        C[i]->vis = 0xffff;
                    }
                }
            }
        }
    }
    if (id == g_mission->player_id && g_game->f139e4 != 0) {
        for (j = 0; j < 4; j++) {
            if (pairs[j].item >= 0) {
                ItemKind* ik = &g_items[pairs[j].item];
                if ((ik->flags & itcf_carry_mask) != 0) {
                    pos2 M;
                    int bone, region;
                    ik->get_carry_pos(&M, &bone, &region);
                    if (bone == 9 || bone == 7 || bone == 8) {
                        if (wield[0] != j && (wield[1] != j || f23c != 0)) {
                            if (A[j] != 0)
                                A[j]->vis &= 0x2000;
                            if (B[j] != 0)
                                B[j]->vis &= 0x2000;
                        }
                        if (C[j] != 0)
                            C[j]->vis &= 0x2000;
                    }
                }
            }
        }
    }
    update_quiver();
}

// ---- the wield change ----------------------------------------------------------------------------------------------------------------------------
// @ 0x444370
void Agent::on_wield_changed()
{
    int anim_ = -1;
    int anim2 = -1;
    int snd2 = -1;
    int snd1 = -1;
    int j;
    for (j = 0; j < 2; j++) {
        int c = wield[j];
        int p = wield_prev[j];
        if (c != p) {
            int item;
            if (c < 0 || (item = pairs[c].item) < 0 || (j != 0 && f23c != 0)) {
                if (j != 0) {
                    if (p >= 0 && c < 0 && f23c == 0) {
                        int pi = pairs[p].item;
                        anim2 = 0x6d;
                        if (pi >= 0) {
                            ItemKind* ik = &g_items[pi];
                            unsigned t = ik->type_flags & 0xff;
                            if (t == itp_type_bow) {
                                anim2 = ((ik->flags & itcf_carry_mask) == itcf_carry_bowcase_left) ? 0x71 : 0x6f;
                                snd1 = 0x55;
                            } else if (t == itp_type_shield) {
                                anim2 = 0x6d;
                                snd1 = 0x57;
                            }
                        }
                    }
                } else {
                    if (p >= 0 && c < 0) {
                        int pi = pairs[p].item;
                        anim_ = 0x53;
                        if (pi >= 0) {
                            switch (g_items[pi].flags & itcf_carry_mask) {
                            case itcf_carry_mace_left_hip:
                            case itcf_carry_axe_left_hip:        anim_ = 0x59; snd2 = 0x49; break;
                            case itcf_carry_dagger_front_right:  anim_ = 0x61; snd2 = 0x53; break;
                            case itcf_carry_sword_left_hip:      anim_ = 0x55; snd2 = 0x45; break;
                            case itcf_carry_dagger_front_left:   anim_ = 0x5f; snd2 = 0x53; break;
                            case itcf_carry_revolver_right:      anim_ = 0x65; snd2 = 0x51; break;
                            case itcf_carry_pistol_front_left:   anim_ = 0x67; snd2 = 0x51; break;
                            case itcf_carry_spear:               anim_ = 0x5d; snd2 = 0x4d; break;
                            case itcf_carry_axe_back:            anim_ = 0x63; snd2 = 0x4b; break;
                            case itcf_carry_sword_back:          anim_ = 0x57; snd2 = 0x47; break;
                            case itcf_carry_crossbow_back:       anim_ = 0x5b; snd2 = 0x4f; break;
                            case itcf_carry_katana:              anim_ = 0x69; snd2 = 0x45; break;
                            case itcf_carry_wakizashi:           anim_ = 0x6b; snd2 = 0x53; break;
                            default:                             anim_ = 0x53; break;
                            }
                        }
                    }
                }
            } else {
                ItemKind* ik = &g_items[item];
                unsigned __int64 carry = ik->flags & itcf_carry_mask;
                unsigned t = ik->type_flags & 0xff;
                if (t == itp_type_bow) {
                    anim2 = (carry == itcf_carry_bowcase_left) ? 0x70 : 0x6e;
                    snd1 = 0x54;
                } else if (t == itp_type_shield) {
                    anim2 = 0x6c;
                    snd1 = 0x56;
                } else if (j == 0) {
                    switch (carry) {
                    case itcf_carry_mace_left_hip:
                    case itcf_carry_axe_left_hip:        anim_ = 0x58; snd2 = 0x48; break;
                    case itcf_carry_dagger_front_right:  anim_ = 0x60; snd2 = 0x52; break;
                    case itcf_carry_sword_left_hip:      anim_ = 0x54; snd2 = 0x44; break;
                    case itcf_carry_dagger_front_left:   anim_ = 0x5e; snd2 = 0x52; break;
                    case itcf_carry_revolver_right:      anim_ = 100;  snd2 = 0x50; break;
                    case itcf_carry_pistol_front_left:   anim_ = 0x66; snd2 = 0x50; break;
                    case itcf_carry_spear:               anim_ = 0x5c; snd2 = 0x4c; break;
                    case itcf_carry_axe_back:            anim_ = 0x62; snd2 = 0x4a; break;
                    case itcf_carry_sword_back:          anim_ = 0x56; snd2 = 0x46; break;
                    case itcf_carry_crossbow_back:       anim_ = 0x5a; snd2 = 0x4e; break;
                    case itcf_carry_katana:              anim_ = 0x68; snd2 = 0x44; break;
                    case itcf_carry_wakizashi:           anim_ = 0x6a; snd2 = 0x52; break;
                    default:
                        if (t == itp_type_polearm) { anim_ = 0x5c; snd2 = 0x4c; }
                        else { anim_ = 0x52; snd2 = 0x58; }
                        break;
                    }
                } else {
                    anim2 = 0x6c;
                }
            }
        }
        wield_prev[j] = c;
    }
    if (f23c != 0)
        wield_prev[1] = -1;
    if (anim[1].cur == 0x7b && defs->count > -1 && anim[1].set(-1, 0x46)) {
        anim[1].rate = 1.0f;
        int c0 = wield[0];
        if (c0 >= 0 && pairs[c0].item >= 0)
            A[c0]->vis = 0xffff;
    }
    if (anim_ != -1) {
        if (anim_ < -1 || anim_ >= defs->count || !anim[1].set(anim_, 0x5000046))
            goto skip;
    } else {
        if (anim2 == -1 || anim2 < -1 || anim2 >= defs->count || !anim[1].set(anim2, 0x5000046))
            goto skip;
    }
    anim[1].rate = 1.0f;
    body->skel->set_rec_phase(1, 0.0f);
    {
        int c0 = wield[0];
        if (c0 >= 0 && pairs[c0].item >= 0)
            A[c0]->vis = 0xffff;
    }
skip:
    if (snd1 != -1 && (float)(g_ticks[g_mission->time_idx] - g_mission->time_base) * 1e-05f > 1.0f)
        g_sound_mgr.play(g_sound_ids[snd1], 0, &pos.x, 1.0f, 0);
    if (snd2 != -1 && (float)(g_ticks[g_mission->time_idx] - g_mission->time_base) * 1e-05f > 1.0f)
        g_sound_mgr.play(g_sound_ids[snd2], 0, &pos.x, 1.0f, 0);
    f130 = 0;
    for (j = 0; j < 2; j++) {
        int s = wield[j];
        if (s >= 0 && pairs[s].item >= 0 && (j == 0 || f23c == 0)) {
            ItemKind* ik = &g_items[pairs[s].item];
            unsigned fl = ik->type_flags;
            unsigned t = fl & 0xff;
            if (t == itp_type_bow) {
                f130 = 0x10;
                variant = 0;
            } else if (t == itp_type_crossbow || t == itp_type_thrown) {
                f130 = 0x20;
                variant = 4;
            } else if (t == itp_type_two_handed_wpn && (fl & itp_two_handed)) {
                f130 = 4;
            } else if (t == itp_type_shield) {
                f130 |= 0x1000;
            } else if (t == itp_type_polearm) {
                f130 |= 8;
            } else if (t == itp_type_one_handed_wpn || t == itp_type_two_handed_wpn) {
                f130 |= 2;
            }
        }
    }
    active = 0;
    f5610 = 0;
}

// ---- std::_Uninit_fill_n for the 0x38-byte E56 record ---------------------------------------------------------------------------------------------
// @ 0x444bc0 (+ catch-all funclet 0x444c28: destroys [next, first), then `throw;`)
void __cdecl uninit_fill_n_e56(E56* first, unsigned n, const E56& val, alloc_e56& al, int tag)
{
    E56* next = first;
    try {
        for (; 0 < n; --n, ++first)
            al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next)
            al.destroy((VecPair2*)next);
        throw;
    }
}
