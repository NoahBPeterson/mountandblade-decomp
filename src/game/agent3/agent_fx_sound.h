// agent_fx_sound.h: the classes of agent_fx_sound.cpp (0x441ff0..0x444c28): the object with five int lists
// (Poser), the world's box query, the effect element / definition lists, the particle emitter and its definition
// table, the FMOD sound player (SoundMgr), the message log, the mission's effect / dead-agent members, game-data
// experience, the agent's target / sound / equipment members (declared in agent_types.h) and the mesh objects the
// equipment display drives. Offsets are the ones read by a function of this module (the address is cited at each
// field). Names provisional.
#ifndef AGENT3_FX_SOUND_H
#define AGENT3_FX_SOUND_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <new>
#include "./agent_types.h"
#include "../mat4.h"

using rgl::mat4;

// ---- item constants (Module System header_items.py) ----------------------------------------------------------
#define itp_two_handed                       0x200000
#define itcf_throw_javelin                   0x40000i64
#define itcf_shoot_mask                      0xff000i64
#define itcf_carry_mask                      0x7f0000000i64
#define itcf_show_holster_when_drawn         0x800000000i64
#define itcf_carry_sword_left_hip            0x10000000i64
#define itcf_carry_axe_left_hip              0x20000000i64
#define itcf_carry_dagger_front_left         0x30000000i64
#define itcf_carry_dagger_front_right        0x40000000i64
#define itcf_carry_quiver_front_right        0x50000000i64
#define itcf_carry_quiver_right_vertical     0x70000000i64
#define itcf_carry_revolver_right            0x90000000i64
#define itcf_carry_pistol_front_left         0xa0000000i64
#define itcf_carry_bowcase_left              0xb0000000i64
#define itcf_carry_mace_left_hip             0xc0000000i64
#define itcf_carry_axe_back                  0x100000000i64
#define itcf_carry_sword_back                0x110000000i64
#define itcf_carry_crossbow_back             0x150000000i64
#define itcf_carry_spear                     0x170000000i64
#define itcf_carry_katana                    0x210000000i64
#define itcf_carry_wakizashi                 0x220000000i64

// ---- objects that keep int lists (0x441ff0, 0x442110) ----------------------------------------------------------------
struct Poser {
    virtual void slot0();
    virtual void slot1();
    virtual void slot2();
    virtual void slot3();
    virtual void changed();                                 // slot 4 (+0x10)
    char pad4[0x34 - 4];
    std::vector<int> lists[5];                              // +0x34 (stride 0x10)
    int nlists;                                             // +0x84
    char pad88[0x118 - 0x88];
    std::vector<int> extra;                                 // +0x118
    void add_to_list(int idx, int val);                     // 0x441ff0
    void add_extra(int val);                                // 0x442110
};

// ---- world grid (0x442180) -----------------------------------------------------------------------------------------------
struct Obj {                                            // entity base: virtual slots, 13 = max, 14 = min
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9();
    virtual void s10(); virtual void s11(); virtual void s12();
    virtual const vector4* get_max();                   // 13 (+0x34)
    virtual const vector4* get_min();                   // 14 (+0x38)
};
#include "../cell.h"                                  // enode, cell (ents +0x30: 0x442180)
typedef std::vector<unsigned>::iterator UIt;
void __cdecl sort_range(UIt first, UIt last, int ideal);        // 0x426a30 (std::_Sort)

// ---- effect elements / definitions (0x442390..0x4424a0) -----------------------------------------------------------------
struct Psys {                                           // particle system (provisional): frame at +0xcc
    char pad0[0xcc];
    mat4 frame;
};
struct PsysGroup;
struct FxElem {                                         // 0x1dc bytes
    char pad0[0x19c];
    std::vector<int> psys;                              // +0x19c (first at +0x1a0); holds Psys*
    int ngroups;                                        // +0x1ac
    std::vector<int> groups;                            // +0x1b0 (first at +0x1b4); holds PsysGroup*
    char pad1c0[0x1dc - 0x1c0];
    void add_psys(int p);                               // 0x442390
    void add_group(int g);                              // 0x4423f0
};
struct FxDef {
    int count;                                          // +0
    FxElem* elems;                                      // +4
    void attach_psys(int idx, Psys* p, const mat4* m);  // 0x442460
    void attach_group(int idx, PsysGroup* g);           // 0x4424a0
};

// ---- emitter setup from a definition table (0x4424c0) ---------------------------------------------------------------------
struct EmitRes {                                        // what Emit::res points to
    char pad0[0x1c];
    unsigned flags;                                     // +0x1c (0x40: follows the camera) (0x442ca0)
};
struct res_obj {                                        // resource returned by resmgr::get_object
    char pad0[0x1c];
    unsigned flags;                                     // +0x1c
    char pad20[0x2c - 0x20];
    EmitRes* f2c;                                       // +0x2c (0x4424c0)
};
struct P8 { int a, b; };
struct EmitDef {                                        // 0xc0-byte definition record
    char pad0[0x10];
    int f10;                                            // +0x10
    tstring name;                                       // +0x14
    int f24;                                            // +0x24
    char pad28[0x30 - 0x28];
    int f30, f34, f38, f3c, f40;                        // +0x30
    P8 t[5][2];                                         // +0x44 (rows of 0x10)
    vector4 a;                                          // +0x94
    vector4 b;                                          // +0xa4
    int c;                                              // +0xb4
    int fb8, fbc;                                       // +0xb8
};
struct Emit {                                           // particle emitter (0x1bc bytes)
    char pad0[4];
    EmitRes* res;                                       // +4 (0x4424c0 stores res_obj::f2c; 0x442ca0 reads its flags)
    res_obj* obj;                                       // +8
    char padc[0x1c - 0xc];
    int f1c, f20, f24;                                  // +0x1c
    P8 t[5][2];                                         // +0x28
    char pad78[0xa0 - 0x78];
    int fa0;                                            // +0xa0
    char pada4[0xc4 - 0xa4];
    vector4 gravity;                                    // +0xc4
    int fd4;                                            // +0xd4
    vector4 offs;                                       // +0xd8
    unsigned flags;                                     // +0xe8 (bit 0 cleared on spawn)
    char padec[0xf4 - 0xec];
    mat4 world;                                         // +0xf4
    mat4 local;                                         // +0x134
    char pad174[0x198 - 0x174];
    int f198;                                           // +0x198
    int f19c, f1a0, f1a4;                               // +0x19c
    int idx;                                            // +0x1a8
    char pad1ac[4];
    int f1b0, f1b4;                                     // +0x1b0
    char pad1b8[4];
    void init();                                        // 0x420b40
    void set_placement(const mat4* m, const vector4* a, const vector4* b, int c);   // 0x433770
};
struct EmitTable {
    EmitDef* defs;                                      // +0
    int count;                                          // +4
    void setup(Emit* e, int idx, const vector4* pos);   // 0x4424c0
};
extern EmitTable g_emit_defs;                           // 0x8ed5090

// ---- FMOD 3 imports (stdcall thunks into the import table; names from the import library) ---------------------------------
extern "C" {
int __stdcall FSOUND_PlaySoundEx(int channel, void* sample, void* dsp, int startpaused);        // 0x5342d6
int __stdcall FSOUND_3D_SetAttributes(int channel, const float* pos, const float* vel);         // 0x5342d0
int __stdcall FSOUND_SetVolume(int channel, int vol);                                           // 0x5342ca
int __stdcall FSOUND_SetLoopMode(int channel, unsigned mode);                                   // 0x5342c4
int __stdcall FSOUND_SetCurrentPosition(int channel, unsigned offset);                          // 0x5342be
int __stdcall FSOUND_SetFrequency(int channel, int freq);                                       // 0x5342b8
int __stdcall FSOUND_SetPriority(int channel, int priority);                                    // 0x5342b2
int __stdcall FSOUND_SetPaused(int channel, int paused);                                        // 0x5342ac
char __stdcall FSOUND_StopSound(int channel);                                                   // 0x5342a6
}
// small helpers seen from here
int __cdecl clamp_index(const int* v, const int* lo, const int* hi);    // 0x432510
int __cdecl round_float(const float* p);                                // 0x432540
int __cdecl rand_below(const int* n);                                   // 0x432580

struct SoundDef {                                       // 0x14 bytes
    void* sample;                                       // +0
    int f4;                                             // +4 (bits 4..7: distance range)
    int id;                                             // +8
    int freq;                                           // +0xc
    int length;                                         // +0x10
    float range_scale();                                // 0x4355f0 (fastcall/thiscall)
};
struct SoundGroup {                                     // 0x98 bytes
    char pad0[0x10];
    unsigned flags;                                     // +0x10 (2: looping, 4: random start, bits 8..11: volume)
    int var[32];                                        // +0x14 variations (def indices)
    int nvar;                                           // +0x94
    float volume_scale();                               // 0x435620
};
struct SoundMgr {                                       // the sound player (g_sound 0x5aa128)
    char pad0[4];
    SoundDef* defs;                                     // +4
    int ngroups;                                        // +8
    SoundGroup* groups;                                 // +0xc
    char pad10[0x40 - 0x10];
    vector4 listener;                                   // +0x40
    int slot[(0x248 - 0x50) / 4];                       // +0x50 group index by sound id
    std::vector<int> channels;                          // +0x248 looping channels
    int play(int idx, int flags, const float* pos, float freq, int* out_id);   // 0x442650 (ret 0x14)
    int play_slot(int n, int flags, const float* pos, float freq, int* out_id);   // 0x4428a0
};
extern SoundMgr g_sound;                                // 0x5aa128
extern int g_sound_ids[];                               // 0x5aa178 sound id table (g_sound_ids[0x11] = pain)
extern int g_levelup_sound;                             // 0x5aa17c
extern float g_levelup_pos[];                           // 0x5aa168
extern int g_show_messages;                             // 0x5aa080
extern int g_sound_off;                                 // 0x5a9fd8
extern int g_no_freq;                                   // 0x5a9fc8
extern float g_master_volume;                           // 0x5aa014
extern int g_xp_table[];                                // 0x590974: experience needed for each level
extern unsigned g_max_dead;                             // 0x5aa044

// ---- message log (object at 0x5d7080): enabled byte at +0x1714, 32-entry ring at +0xb0b8 ------------------------------------
#include "../str_table.h"
extern StrTable g_strtable;                             // 0x5e2a18
struct MsgLog {
    char pad0[0x1714];
    char enabled;                                       // +0x1714
    char pad1715[0xb0b8 - 0x1715];
    struct Ring {
        int count;                                      // +0xb0b8
        int head;                                       // +0xb0bc
        tstring names[32];                              // +0xb0c0
        int vals[32];                                   // +0xb2c0
    } ring;
    void push(const tstring& s, int v);                 // 0x4334b0
};
extern MsgLog g_msglog;                                 // 0x5d7080

// ---- effect spawn (0x442ca0) ---------------------------------------------------------------------------------------------------
struct CamRec {                                         // 0x4c bytes, array behind the pointer at 0x5e28f4
    char pad0[0x20];
    float dx, dy, dz;                                   // +0x20
    char pad2c[4];
    float px, py, pz;                                   // +0x30
    char pad3c[0x4c - 0x3c];
};
extern CamRec* g_cams;                                  // 0x5e28f4

// ---- mesh objects hanging off the agent (0x443aa0, 0x444370) ----------------------------------------------------------------------
#include "../../include/mat4.h"
using rgl::mat3;
mat3* __cdecl identity3(mat3* out);                     // 0x40cd60
struct node_list {
    int al; void** first; void** last; void** end;
    void cap_all(const float* limit);                   // 0x438f90
};
struct Mesh {
    char pad0[0x34];
    node_list lists[5];                                 // +0x34
    int nlists;                                         // +0x84
    char pad88[4];
    pos2 frame;                                         // +0x8c
    unsigned vis;                                       // +0xcc visibility mask (0xffff shown, 0 hidden)
};
struct Region {                                         // 0x1dc-byte skeleton record (Skeleton::recs), as 0x443aa0 sees it
    char pad0[0x1ac];
    int count;                                          // +0x1ac
    std::vector<int> meshes;                            // +0x1b0
    char pad1c0[0x1dc - 0x1c0];
};

#endif // AGENT3_FX_SOUND_H
