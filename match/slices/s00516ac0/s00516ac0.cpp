// Slice s00516ac0: the campaign-map frame tick and two small Game helpers.
//   0x516ac0  Game::tick          - per-frame campaign update: advance the game clock by the current
//                                 movement speed (walk / scripted speed / distance to the selected
//                                 party's map marker), fan that out to the sound + party systems,
//                                 run the second/minute/hour tickers, tick every party and map
//                                 marker, and finally handle the "resume" countdown.
//   0x517110  Game::resume_strings - store the two strings captured when the resume countdown ends.
//   0x517140  Game::update_markers - integrate the four global marker lists by the frame step.
// Layouts are provisional; every offset comes from this slice's disassembly.
#pragma warning(disable:4234)
#include <math.h>
#include <windows.h>
#include "rgl.h"

extern "C" int _fltused = 0;

// ---- engine / game globals -------------------------------------------------------------------------
extern float    g_step;                 // 0x5a9c20  frame step (seconds)
extern int      g_clock_step;           // 0x5937b4  clock index used by set_clock_step
extern __int64  g_time[];               // 0x5e2948  per-clock tick counters (100 kHz)
extern const float g_speed_scale;       // 0xa18110  map speed scale
extern const float g_speed_eps;         // 0x591390  "the map is moving" threshold
extern const float g_rate_marker;       // 0x590cd0  marker decay per second
extern const float g_rate_second;       // 0x596774  ticker 2 rate
extern const float g_rate_third;        // 0x59b998  ticker 3 rate
extern const float g_rate_fourth;       // 0x59b994  ticker 4 rate
extern const float g_rate_fifth;        // 0x59b990  ticker 5 rate
extern const float g_anim_x;            // 0xa18148  marker drift rate x
extern const float g_anim_y;            // 0xa1814c  marker drift rate y
extern const float g_anim_z;            // 0xa18154  marker drift rate x (second set)
extern const float g_anim_w;            // 0xa18158  marker drift rate y (second set)
extern int      g_msg_gate;             // 0xa181a8  0: allow the "loading" clock message
extern int      g_in_gate;              // 0x5a9fbc  input gate
extern int      g_in_a;                 // 0xa17124  input flags
extern int      g_in_b;                 // 0xa17524
extern int      g_in_c;                 // 0xa17148
extern int      g_in_d;                 // 0xa17348
extern const char g_resume_text[];      // 0x54a480  resume marker string
extern const char g_msg_key[];          // 0x59b99c  message log key

// ---- rgl string / string table / message log (16-byte tstring, as in s00516700) --------------------
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring(const char* s) throw();         // 0x42bbe0
    tstring(const tstring& s) throw();      // 0x42c110
    ~tstring() throw();                     // 0x42bc40
    void operator=(const tstring& s);       // 0x42bc50
};
struct StrTable { tstring* lookup(const tstring& key); };     // 0x43a1e0 (object 0x5e2a18)
struct MsgLog   { void add(tstring msg, int color); };        // 0x452c00 (object 0x5d7080)
extern StrTable g_strtable;             // 0x5e2a18

// ---- the 100 kHz gui timer at this+0x13a80 / this+0x1a298 -----------------------------------------
struct gui_timer {
    int clock;                          // +0x00 index into g_time
    int pad4;                           // +0x04
    __int64 t;                          // +0x08 last tick value
    void set_delay(float seconds);      // 0x42cd20
};

// ---- records ---------------------------------------------------------------------------------------
struct Troop {                          // 0x508 bytes
    char pad0[0x4ec];
    int  f4ec;                          // +0x4ec (-1: no special troop)
    char pad4f0[0x508 - 0x4f0];
};

struct Party {                          // 0x5ac8 bytes (PartyMgr pages)
    char pad0[0x30];
    float f30;                          // +0x30  map x
    float f34;                          // +0x34  map y
    char pad38[0xa0 - 0x38];
    __int64 flags;                      // +0xa0
    char pad_a8[0x8b4 - 0xa8];
    char f8b4;                          // +0x8b4 "seen this frame"
    char pad8b5[0x918 - 0x8b5];
    int  type;                          // +0x918
    char pad91c[0x964 - 0x91c];
    float f964;                         // +0x964 selected-party marker x
    float f968;                         // +0x968 selected-party marker y
    void update(float dt);              // 0x4b80d0
};

struct MapRec {                         // 0x98-byte record in the pool at this+0xcc
    char pad0[0x10];
    int  type;                          // +0x10
    char pad14[0x44 - 0x14];
    float opacity;                      // +0x44
    char pad48[0x50 - 0x48];
    void* obj;                          // +0x50
    int  f54;                           // +0x54
    int  f58;                           // +0x58
    char pad5c[0x80 - 0x5c];
    int  anim;                          // +0x80
    char pad84[4];
    __int64 stamp;                      // +0x88
    int  sound;                         // +0x90
    void update_nav_anim(float speed);  // 0x4b71e0
};

struct Pool {                           // embedded at this+0xcc
    unsigned count;                     // +0x00
    char pad4[0x14];
    void** pages;                       // +0x18
    int  first();                       // 0x5179b0
    void next(int* index);              // 0x517960
    MapRec* slot(unsigned index);       // 0x48ef20
};

struct PartyMgr {                       // embedded at this+0xf8
    char pad0[0x138d0];
    Party** pages;                      // +0x138d0
    char pad2[8];
    unsigned count;                     // +0x138dc
    int   first_party_id();             // 0x460d20
    int   next_party_id(int* id);       // 0x460ca0
    int   index_of(int id);             // 0x48ad00
    Party* get(int id);                 // 0x51ee50
};

// ---- global marker lists (pairs of begin/end pointers, elements at 0x1f4..0x208) --------------------
struct Marker {
    char pad0[0x1f4];
    float f1f4;                         // +0x1f4
    float f1f8;                         // +0x1f8
    float f1fc;                         // +0x1fc
    float f200;                         // +0x200
    float f204;                         // +0x204
    float f208;                         // +0x208
};
struct List {
    Marker** first;
    Marker** last;
    int count() const { return first ? (int)(last - first) : 0; }
    Marker* operator[](unsigned i) const { return first[i]; }
};
extern List g_list_a;                   // 0xa732c0
extern List g_list_b;                   // 0xa732e0
extern List g_list_c;                   // 0xa732f0
extern List g_list_d;                   // 0xa732d0

// ---- the campaign object (g_game) ------------------------------------------------------------------
struct UpdateObj {                       // this+0x139e8
    void update(int flags);             // 0x462760
};
struct SoundObj {                        // this+0x139f4
    void update(int flags);             // 0x48add0
};

struct Game {
    char pad0[0x78];
    int  cur_agent;                     // +0x78
    char pad7c[0x8c - 0x7c];
    int  f8c;                           // +0x8c
    char pad90[0x94 - 0x90];
    char f94;                           // +0x94 "map frozen" gate
    char f95;                           // +0x95 input toggle
    char pad96[0xcc - 0x96];
    Pool pool;                          // +0xcc
    char pad_e8[0xf8 - (0xcc + 0x1c)];
    PartyMgr parties;                   // +0xf8
    char pad_139d8[0x13a00 - 0x139d8];
    Troop* troops;                      // +0x13a00
    char pad_13a04[0x13a70 - 0x13a04];
    tstring resume_old;                 // +0x13a70
    gui_timer resume_timer;             // +0x13a80
    char pad_13a90[0x13aa0 - 0x13a90];
    char f13aa0;                        // +0x13aa0 resume countdown active
    char pad_13aa1[3];
    int  f13aa4;                        // +0x13aa4 speed mode (0 / 2 / other)
    tstring f13aa8;                     // +0x13aa8
    tstring f13ab8;                     // +0x13ab8
    char f13ac8;                        // +0x13ac8
    char pad_13ac9[0x13ad0 - 0x13ac9];
    float f13ad0;                       // +0x13ad0 countdown / remaining distance
    char pad_13ad4[0x13adc - 0x13ad4];
    int  f13adc;                        // +0x13adc speed multiplier
    char pad_13ae0[0x13af8 - 0x13ae0];
    int  f13af8;                        // +0x13af8
    char pad_13afc[0x13b18 - 0x13afc];
    volatile int  f13b18;               // +0x13b18 ticker clock index
    int  pad_13b1c;
    volatile __int64 f13b20;            // +0x13b20 ticker baseline
    int  f13b28;                        // +0x13b28 last second
    int  f13b2c;                        // +0x13b2c last 1/2 rate
    int  f13b30;                        // +0x13b30 last 1/3 rate
    int  f13b34;                        // +0x13b34 last 1/4 rate
    int  f13b38;                        // +0x13b38 last 1/5 rate
    char pad_13b3c[0x14058 - 0x13b3c];
    float speed;                        // +0x14058
    char pad_1405c[0x1a298 - 0x1405c];
    gui_timer msg_timer;                // +0x1a298

    void tick(float dt);                                // 0x516ac0
    __declspec(noinline) void resume_strings(tstring* a, tstring* b);   // 0x517110
    void update_markers();                              // 0x517140
    void update_colors();                               // 0x517350
    void update_second();                               // 0x517e70
    void update_minute(float speed);                    // 0x518680
    void update_tracks();                               // 0x5189e0
    void update_train();                                // 0x518d70
    void update_cursor(float speed);                    // 0x5179f0
    void dissolve(int id);                              // 0x4915d0
};
extern Game* g_game;                    // 0x5e2930

struct Scene { void step(float dt); };  // 0x427f40
extern Scene* g_scene;                  // 0xa732b4

void set_clock_step(int* clock, float step);            // 0x461420

static Party* party_at(PartyMgr* m, int idx)
{
    return (Party*)((char*)m->pages[idx >> 4] + (idx & 0xf) * 0x5ac8);
}

// @ 0x516ac0
void Game::tick(float dt)
{
    int flags = 1;
    if (f94) {
        speed = 0.0f;
        return;
    }

    if (f13aa0 || f13ad0 > 0.0f) {
        int n = f13adc;
        float v = (n ? (float)n : 10.0f) * g_speed_scale * dt;
        flags = 0x10001;
        speed = v;
        f13ad0 -= v;
    } else if (f13aa4 == 0) {
        speed = 0.0f;
    } else if (f13aa4 == 2) {
        float v = g_speed_scale * dt;
        speed = v;
        if (g_in_c || g_in_d)
            speed = v * 10.0f;
    } else {
        Party* a = parties.get(cur_agent);
        Party* b = parties.get(cur_agent);
        float dx = a->f30 - b->f964;
        float dy = a->f34 - b->f968;
        float d = sqrtf(dx * dx + dy * dy);
        if (d == 0.0f)
            speed = 0.0f;
        else
            speed = g_speed_scale * dt;
    }

    set_clock_step(&g_clock_step, speed);
    f13af8 = 0;
    ((SoundObj*)((char*)this + 0x139f4))->update(flags);
    ((UpdateObj*)((char*)this + 0x139e8))->update(flags);

    int target = -1;

    if (g_msg_gate == 0 &&
        ((Troop*)((char*)troops + f8c * 0x508))->f4ec == -1) {
        float d = (g_time[msg_timer.clock] - msg_timer.t) * 1e-5f;
        if (d > 1.0f) {
            msg_timer.set_delay(-2.0f);
            tstring msg(g_msg_key);
            ((MsgLog*)0x5d7080)->add(*g_strtable.lookup(msg), 0xffdd4444);
        }
    }

    party_at(&parties, parties.index_of(cur_agent))->f8b4 = 1;

    {
        int v = (int)((g_time[f13b18] - f13b20) * 1e-5f);
        if (v > f13b28) {
            update_second();
            f13b28 = v;
        }
    }
    {
        int v = (int)((g_time[f13b18] - f13b20) * 1e-5f * g_rate_second);
        if (v > f13b2c) {
            update_train();
            f13b2c = v;
        }
    }
    {
        int v = (int)((g_time[f13b18] - f13b20) * 1e-5f * g_rate_third);
        if (v > f13b30)
            f13b30 = v;
    }
    {
        int v = (int)((g_time[f13b18] - f13b20) * 1e-5f * g_rate_fourth);
        if (v > f13b34)
            f13b34 = v;
    }
    {
        int v = (int)((g_time[f13b18] - f13b20) * 1e-5f * g_rate_fifth);
        if (v > f13b38)
            f13b38 = v;
    }

    if (speed > g_speed_eps) {
        update_minute(speed);
        update_tracks();
    }
    update_cursor(speed);

    int id = parties.first_party_id();
    while ((unsigned)id < parties.count) {
        Party* p = party_at(&parties, parties.index_of(id));
        p->update(speed);
        if (p->type == 8 && (p->flags & 0x20000))
            target = id;
        parties.next_party_id(&id);
    }
    if (target >= 0)
        dissolve(target);

    {
        Pool* pool = &this->pool;
        int idx = pool->first();
        while (idx < pool->count) {
            pool->slot(idx)->update_nav_anim(speed);
            pool->next(&idx);
        }
    }

    g_scene->step(g_step);

    if (g_in_gate && g_in_a && !g_in_b && g_in_c)
        f95 = !f95;

    update_colors();
    update_markers();

    if (f13aa0) {
        float d = (g_time[resume_timer.clock] - resume_timer.t) * 1e-5f;
        if (d >= 0.0f) {
            f13aa0 = 0;
            Party* p = party_at(&parties, parties.index_of(cur_agent));
            p->flags &= ~(__int64)0x100;
            tstring s(g_resume_text);
            resume_strings(&s, &resume_old);
        }
    }
}

// @ 0x517110
void Game::resume_strings(tstring* a, tstring* b)
{
    f13ab8 = *a;
    f13aa8 = *b;
    f13ac8 = 1;
}

// @ 0x517140
void Game::update_markers()
{
    for (unsigned i = 0; i < (unsigned)g_list_a.count(); i++) {
        g_list_a[i]->f1f8 -= g_step * g_rate_marker;
        g_list_a[i]->f208 = g_list_a[i]->f1f8;
    }
    for (unsigned i = 0; i < (unsigned)g_list_b.count(); i++) {
        g_list_b[i]->f1f4 += g_step * g_anim_x * 3.0f;
        g_list_b[i]->f1f8 += g_step * g_anim_y * 3.0f;
        g_list_b[i]->f204 = g_list_b[i]->f1f4;
        g_list_b[i]->f208 = g_list_b[i]->f1f8;
    }
    for (unsigned i = 0; i < (unsigned)g_list_c.count(); i++) {
        g_list_c[i]->f1f4 -= g_step * g_anim_x * 2.0f;
        g_list_c[i]->f1f8 -= g_step * g_anim_y * 2.0f;
        g_list_c[i]->f204 = g_list_c[i]->f1f4;
        g_list_c[i]->f208 = g_list_c[i]->f1f8;
    }
    for (unsigned i = 0; i < (unsigned)g_list_d.count(); i++) {
        g_list_d[i]->f1f4 += g_step * g_anim_z;
        g_list_d[i]->f1f8 += g_step * g_anim_w;
        g_list_d[i]->f204 = g_list_d[i]->f1f4;
        g_list_d[i]->f208 = g_list_d[i]->f1f8;
    }
}
