// Slice s004b80d0: agent::tick (0x4b80d0, ~3.6 KB) -- the per-tick update of a mission agent.
// A state machine on this+0x918 (action) / this+0x91c (target) plus the movement, the scene probe
// (0x48ea80/0x48ef20), the model matrix/colour refresh and the two action entities at this+0x59ec/0x59f0.
// Class and field names are provisional; the layout is the union of this function's member accesses and the
// neighbouring slices s004b4a00 / s004b7840 / s0048ef40 / s0049bea0.
#include <math.h>
#include <stddef.h>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// ---- engine helpers -----------------------------------------------------------------------------------------
float __cdecl frand();                                   // 0x40e400  (rand() % 15817)/15817
unsigned __cdecl pack_color(const vector4* c);           // 0x40e440
struct mat3 { vector4 s, f, u; void rotate_sf(const float* a); };   // 0x30 bytes
mat3* __cdecl identity3(mat3* out);                      // 0x40cd60
char f489390(int kind);                                  // 0x489390
char f48efa0(const int* a, const int* b);                // 0x48efa0

struct Vector2 {                                         // 0x4367c0 (normalises, returns the old length)
    float x, y;
    float normalize();
};

// ---- the per-agent clock (0x432c10 elapsed, 0x42cd20 set_delay) ----------------------------------------------
struct GameClock {
    int idx;                                             // +0 slot in g_ticks
    int pad4;
    __int64 start;                                       // +8
    float elapsed() const;                               // 0x432c10
    void  set_delay(float sec);                          // 0x42cd20
};

// ---- scene / model records ---------------------------------------------------------------------------------
struct Agent;
struct Entity {                                          // agent+0x59ec / +0x59f0
    char pad0[0x18];
    Entity* owner;                                       // +0x18
    char pad1c[0x3c - 0x1c];
    int color;                                           // +0x3c
    char pad40[0xac - 0x40];
    int f_ac;                                            // +0xac
    char padb0[0x114 - 0xb0];
    int f114;                                            // +0x114
    char in_use();                                       // 0x4b17d0
    void set_time(const float* t);                       // 0x433190
};

struct SceneObj {                                        // 0x48ea80 / 0x48ef20 result
    char pad0[4];
    int anim;                                            // +4  index into the 0x88-byte anim records
    char pad8[0x24 - 8];
    float x;                                             // +0x24
    float y;                                             // +0x28
    char pad2c[0x3c - 0x2c];
    float z;                                             // +0x3c
    char pad40[0x48 - 0x40];
    SceneObj* sub;                                       // +0x48
    float height(const float* p);                        // 0x433440
    void  reset_anim();                                  // 0x4b6180
};

struct Model {                                           // agent+0x59c0
    char pad0[0x58];
    mat3 basis;                                          // +0x58
    vector4 origin;                                      // +0x88
    char pad98[0x11c - 0x98];
    Entity** ents;                                       // +0x11c (vector<Entity*>: begin/end)
    Entity** ents_end;                                   // +0x120
    char pad124[0x140 - 0x124];
    char visible;                                        // +0x140
    char pad141[0x294 - 0x141];
    float f294;                                          // +0x294
};

struct AnimSet { char pad0[0x14]; void* recs; };         // g_scene+0xb0
struct AnimRec { char pad0[0x24]; int f24; char pad28[0x88 - 0x28]; };
struct Scene {
    char pad0[0xb0];
    AnimSet* anims;                                      // +0xb0
    char padb4[0x31c - 0xb4];
    float x, y, z;                                       // +0x31c
};
extern Scene* g_scene;                                   // 0xa732b4

struct EmitDef { char pad0[0x28]; int sound; char pad2c[0x48 - 0x2c]; };
extern EmitDef* g_emit_defs;                             // 0x5e29f0
extern vector4 g_emit_pos;                               // 0xa708c0

struct SoundMgr { void play(int sound, int flags, const vector4* pos, float pitch, int* out); };
extern SoundMgr g_sound;                                 // 0x5aa128

struct AgentMgr {                                        // g_game+0xf8
    bool exists(int id);                                 // 0x4903e0
    Agent* get(int id);                                  // 0x51ee50
};
struct EntTable {                                        // g_game+0xcc
    SceneObj* get_at(int id);                            // 0x48ef20
};
struct GameData {
    char pad0[0x78];
    int camera_id;                                       // +0x78
    char pad7c[0xcc - 0x7c];
    EntTable ents;                                       // +0xcc
    char padd0[0xe4 - 0xd0];
    void* cells;                                         // +0xe4 (id>>4 page, (id&0xf) 0x98 record)
    char pade8[0xf8 - 0xe8];
    AgentMgr agents;                                     // +0xf8
    char pad100[0x13a90 - 0x100];
    GameClock clock;                                     // +0x13a90
    char pad13aa0[0x13ad0 - 0x13aa0];
    float zoom;                                          // +0x13ad0
    int f13ad4;                                          // +0x13ad4
    SceneObj* find_at(const float* p);                   // 0x48ea80
    void emit_event(int a, int b, int c);                // 0x435790
    void agent_change(int a, int b);                     // 0x49f060
    void push_party_event(int id, const vector4* pos, const float* angle);   // 0x4b6070
};
extern GameData* g_game;                                 // 0x5e2930
extern __int64 g_ticks[];                                // 0x5e2948

// ---- the agent ---------------------------------------------------------------------------------------------
struct Agent {
    char pad0[8];
    int id;                                              // +0x08
    char padc[0x30 - 0xc];
    float x;                                             // +0x30
    float y;                                             // +0x34
    float z;                                             // +0x38
    float w;                                             // +0x3c
    float dirx;                                          // +0x40
    float diry;                                          // +0x44
    float oldx;                                          // +0x48
    float oldy;                                          // +0x4c
    float angle;                                         // +0x50
    char pad54[0x64 - 0x54];
    float radius;                                        // +0x64
    float scale;                                         // +0x68
    char pad6c[0x88 - 0x6c];
    int troop;                                           // +0x88 (-1 when unattached)
    char pad8c[0xa0 - 0x8c];
    unsigned flags;                                      // +0xa0
    int fa4;                                             // +0xa4
    char pada8[0x8c8 - 0xa8];
    int f8c8;                                            // +0x8c8
    GameClock clk8d0;                                    // +0x8d0
    float interval;                                      // +0x8e0
    int state;                                           // +0x8e4
    char pad8e8[0x908 - 0x8e8];
    GameClock clk908;                                    // +0x908
    int action;                                          // +0x918
    int target;                                          // +0x91c
    char pad920[0x934 - 0x920];
    float t934;                                          // +0x934
    int idx938;                                          // +0x938
    char pad93c[4];
    __int64 t940;                                        // +0x940
    GameClock clk948;                                    // +0x948
    char pad958[0x964 - 0x958];
    float dest_x;                                        // +0x964
    float dest_y;                                        // +0x968
    char pad96c[0x5974 - 0x96c];
    SceneObj* mesh;                                      // +0x5974
    int anim;                                            // +0x5978
    float f597c;                                         // +0x597c
    int f5980;                                           // +0x5980
    int f5984;                                           // +0x5984
    int f5988;                                           // +0x5988
    int f598c;                                           // +0x598c
    float f5990;                                         // +0x5990
    float f5994;                                         // +0x5994
    int f5998;                                           // +0x5998
    char pad599c[0x59a8 - 0x599c];
    int f59a8;                                           // +0x59a8
    char pad59ac[0x59c0 - 0x59ac];
    Model* model;                                        // +0x59c0
    float f59c4;                                         // +0x59c4
    float f59c8;                                         // +0x59c8
    float f59cc;                                         // +0x59cc
    float f59d0;                                         // +0x59d0
    float f59d4;                                         // +0x59d4
    char pad59d8[0x59ec - 0x59d8];
    Entity* e59ec;                                       // +0x59ec
    Entity* e59f0;                                       // +0x59f0

    void tick(float dt);                                 // 0x4b80d0
    void step_timing();                                  // 0x4b4a00
    void f4b3ce0();                                      // 0x4b3ce0
    void f4b7460();                                      // 0x4b7460
    void update_hud();                                   // 0x4b7e50
    void setup();                                        // 0x4b7840
    float speed();                                       // 0x4b20b0
};

// @ 0x4b80d0
void Agent::tick(float dt)
{
    // ---- early outs ---------------------------------------------------------------------------------------
    if (flags & 0x100) {
        model->visible = 0;
        return;
    }
    if ((id == g_game->camera_id && g_game->zoom > 0.0f && g_game->f13ad4 != 0) || f8c8 >= 0) {
        model->visible = 0;
        return;
    }

    // ---- periodic re-planning (never for the camera agent) -------------------------------------------------
    if (!(flags & 0x400)) {
        if (id != g_game->camera_id) {
            if (interval <= clk8d0.elapsed()) {
                f598c = 0;
                step_timing();
                clk8d0.start = g_ticks[clk8d0.idx];
                float nv = frand() * 0.15f + 0.25f;
                interval = nv;
                if (action == 0xb)
                    interval = nv * 0.1f;
                t934 = 0.0f;
            }
        }
        float d1 = (float)(g_ticks[idx938] - t940) * 1e-05f;
        if (t934 < d1) {
            f4b3ce0();
            clk948.set_delay(0.1f);
            t940 = g_ticks[idx938];
            t934 = (frand() + 1.0f) * 0.1f;
            if (f597c < 5.0f)
                f597c = f597c + 0.5f;
        }
        float d2 = (float)(g_ticks[clk948.idx] - clk948.start) * 1e-05f;
        if (d2 > 0.05f || id == g_game->camera_id) {
            f4b7460();
            clk948.start = g_ticks[clk948.idx];
        }
        if (f5980 == 0 && f5998 == 2) {
            int oid = f59a8;
            if (g_game->agents.exists(oid)) {
                Agent* o = g_game->agents.get(oid);
                dest_x = o->x;
                dest_y = o->y;
            }
        }
    }

    // ---- movement -----------------------------------------------------------------------------------------
    model->visible = 1;
    if (action == 8) {
        model->visible = 0;
        return;
    }
    update_hud();

    Vector2 dir2;
    dir2.x = dest_x - x;
    dir2.y = dest_y - y;
    float dist = dir2.normalize();
    bool moved = false;
    float step = speed() * dt;

    bool skip_heading = false;
    if (f5980 == 0 && f5998 == 2) {
        float lim = dist - radius * 1.2f;
        if (lim < step) {
            step = lim;
            if (lim < 0.0f)
                step = 0.0f;
            dist = lim;
            if (lim < 0.0f)
                skip_heading = true;
        }
    } else {
        if (dist < step)
            step = dist;
    }

    if (!skip_heading && dist > 0.0f) {
        dirx = dir2.x;
        diry = dir2.y;
        Vector2 t;
        t.x = dirx - oldx;
        t.y = diry - oldy;
        t.normalize();
        float want = (float)atan2(-t.x, t.y);
        if (angle > 6.2831855f)
            angle = (float)fmod(angle, 6.2831855f);
        else if (angle < 0.0f)
            angle = 6.2831855f - (float)fmod(-angle, 6.2831855f);
        if (want > 6.2831855f)
            want = want - 6.2831855f;
        else if (want < 0.0f)
            want = want + 6.2831855f;
        float diff = want - angle;
        if (diff > 3.1415927f)
            diff = -(6.2831855f - diff);
        if (diff < -3.1415927f)
            diff = diff + 6.2831855f;
        float k = dt * 50.0f;
        if (k > 1.0f)
            k = 1.0f;
        moved = true;
        angle = k * diff + angle;
    }

    // ---- scene probe at the candidate position ------------------------------------------------------------
    float px = step * dirx + x;
    float py = step * diry + y;
    SceneObj* node = g_game->find_at(&px);
    if (node != 0) {
        AnimRec* recs = (AnimRec*)((AnimSet*)g_scene->anims)->recs;
        int animfield = recs[node->anim].f24;
        bool near = f48efa0(&node->anim, &mesh->anim) != 0;
        bool commit = false;
        if (flags & 0x200) {
            if (f489390(animfield)) {
                commit = true;
            } else if (animfield != 0xf && id == g_game->camera_id && 0.05f < g_game->clock.elapsed()) {
                g_emit_pos.x = px;
                g_emit_pos.y = py;
                g_emit_pos.z = 0.0f;
                g_game->emit_event(0x14, 0, 0);
                g_game->clock.start = g_ticks[g_game->clock.idx];
                f5998 = 0;
                f5980 = 0;
                f5988 = 0;
            }
        }
        if (!commit) {
            if (near) {
                commit = true;
            } else {
                f597c = 0;
                if (f598c == 0)
                    f5988 = 1;
            }
        }
        if (commit) {
            x = px;
            y = py;
            mesh = node;
            anim = animfield;
            z = node->height(&x);
        }
    }

    // ---- action state machine -----------------------------------------------------------------------------
    if (action == 4 && target >= 0 && g_game->agents.exists(target) &&
        !(g_game->agents.get(target)->flags & 0x100) &&
        g_game->agents.get(target)->action != 8 && dt > 0.0f) {
        Agent* o = g_game->agents.get(target);
        float ox = o->x;
        float oy = o->y;
        float ax = ox - x;
        float ay = oy - y;
        float ad = sqrtf(ax * ax + ay * ay);
        Agent* o2 = g_game->agents.get(target);
        if (ad < o2->radius + radius) {
            Agent* o3 = g_game->agents.get(target);
            if (id == g_game->camera_id && !(o3->flags & 0x400) && o3->troop < 0 &&
                o3->action == 4 && o3->target == id) {
                g_game->agent_change(target, id);
            } else {
                g_game->agent_change(id, target);
            }
            Agent* o4 = g_game->agents.get(target);
            o4->f5998 = 0;
            o4->f5980 = 0;
            o4->f5988 = 0;
            f5998 = 0;
            f5980 = 0;
            f5988 = 0;
        }
    } else if (action == 9) {
        int cell = target;
        char* page = *(char**)((char*)g_game->cells + (cell >> 4) * 4);
        char* rec = page + (cell & 0xf) * 0x98;
        float cx = *(float*)(rec + 0x2c);
        float cy = *(float*)(rec + 0x30);
        float ax = cx - x;
        float ay = cy - y;
        float ad = sqrtf(ax * ax + ay * ay);
        float cr = *(float*)(rec + 0x40);
        if (ad < cr + radius) {
            SceneObj* n = g_game->ents.get_at(cell);
            x = n->x;
            y = n->y;
            z = g_game->ents.get_at(cell)->z;
            mesh = g_game->ents.get_at(cell)->sub;
            flags |= 0x200;
            g_game->ents.get_at(cell)->reset_anim();
            action = 0;
            f5998 = 0;
            f5980 = 0;
            f5988 = 0;
            setup();
        }
    } else if (action == 1) {
        if (target >= 0 && g_game->agents.exists(target)) {
            Agent* o = g_game->agents.get(target);
            float ox = o->x;
            float oy = o->y;
            float ax = ox - x;
            float ay = oy - y;
            float ad = sqrtf(ax * ax + ay * ay);
            Agent* o2 = g_game->agents.get(target);
            if (ad < o2->radius + radius) {
                action = 8;
                f5998 = 0;
                f5980 = 0;
                f5988 = 0;
            }
        }
    }

    // ---- party-event trail on large moves -----------------------------------------------------------------
    {
        vector4 delta;
        delta.x = x - f59c8;
        delta.y = y - f59cc;
        delta.z = z - f59d0;
        float mlen = delta.normalize();
        if (mlen > f59c4 && !(flags & 0x200) && anim != 7) {
            float midx = x * 0.5f + f59c8 * 0.5f;
            float midy = y * 0.5f + f59cc * 0.5f;
            float midz = z * 0.5f + f59d0 * 0.5f;
            float ang = (float)atan2(-delta.x, delta.y);
            vector4 mid(midx, midy, midz);
            g_game->push_party_event(id, &mid, &ang);
            float r1 = frand();
            float r2 = frand();
            f59c8 = x;
            f59cc = y;
            f59d0 = z;
            f59d4 = w;
            f59c4 = (r2 + r1) * 0.7f + 0.5f;
        }
    }

    // ---- model refresh ------------------------------------------------------------------------------------
    mat3 m;
    identity3(&m);
    model->basis = m;
    model->basis.rotate_sf(&angle);
    model->origin = *(vector4*)&x;

    unsigned color;
    if (id == g_game->camera_id) {
        color = (flags & 0x1000000) ? 0x88ffffffu : 0xffffffffu;
    } else {
        vector4 c(1.0f, 1.0f, 1.0f);
        c.w = scale;
        color = pack_color(&c);
        model->visible = (scale > 0.01f);
        model->f294 = scale * 0.5f;
    }
    for (int i = 0; i < (int)(model->ents_end - model->ents); i++)
        if (model->ents[i] != 0)
            model->ents[i]->color = (int)color;
    if (!model->visible)
        return;
    if (e59ec != 0)
        e59ec->f114 = 0;
    if (e59f0 != 0)
        e59f0->f114 = 0;

    // ---- the agent's first action entity ------------------------------------------------------------------
    Entity* e1 = e59ec;
    if (!moved) {
        if (e1 != 0 && e1->in_use()) {
            float one = 1.0f;
            e1->set_time(&one);
        }
    } else {
        if (e1 != 0 && e1->in_use()) {
            float ph = (float)fmod(((double)g_ticks[0] * 1e-05f + (double)id) * 2.3f, 1.0) * 40.0f + 100.0f;
            e1->set_time(&ph);
            e59ec->f114 = 1;
            if (clk908.elapsed() > 0.5f && dt > 0.0f) {
                clk908.start = g_ticks[clk908.idx];
                int snd = g_emit_defs[flags & 0xff].sound;
                if (snd != 0)
                    g_sound.play(snd, 0, &model->origin, 1.0f, 0);
            }
        }
    }

    // ---- the agent's second action entity -----------------------------------------------------------------
    Entity* e2 = e59f0;
    if (e2 == 0)
        return;
    int v = e2->owner ? e2->owner->f_ac : e2->f_ac;
    if (!(v > 0))
        return;
    float sx = g_scene->x - x;
    float sy = g_scene->y - y;
    float sz = g_scene->z - z;
    float dd = sx * sx + sy * sy + sz * sz;
    if (!(dd < 2000.0f))
        return;
    float ph2 = (float)fmod((double)id * 0.05f + (double)g_ticks[0] * 1e-05f, 1.0) * 40.0f + 100.0f;
    e2->set_time(&ph2);
    e59f0->f114 = 1;
}
