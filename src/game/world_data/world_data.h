// world_data.h: the party / quest / experience members of the game object (g_game, 0x5e2930), the mission-side helpers
// that sit between them (0x490030..0x4958f0: translation units 83 world_2 and 84 game_data_2 of the TU map), and the
// small engine objects those functions reach (message log, sound player, window manager, scene list, panel family).
// Names provisional; every offset cites the function that proves it.
//
// The shared classes are included, not redefined: GameData / PartyMgr / Party / Stack / Troop / QuestRec / tstring /
// MsgLog / StrTable / Vec2 / MissionUi / ContainerBase / DropDown come from src/include and src/game. What those headers
// lack is added here as *views*: a derived class with no data members (`GameW : GameData`, `PartyW : Party`, ...) whose
// inline accessors read the raw offsets this module's functions use (the technique of script/script_interpreter.h).
// Pure additions to canonical headers: none.
#ifndef GAME_WORLD_DATA_H
#define GAME_WORLD_DATA_H
#include <new>
#include <vector>
#include <deque>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../tstring.h"
#include "../game_globals.h"      // canonical g_player_wounded_treshold
#include "../str_table.h"
#include "../../rgl/rgl_math.h"
#include "../../include/vec.h"
#include "../../include/game_data.h"
#include "../../include/msg_log.h"
#include "../config/widget_mgr.h"
#include "../ui/mission.h"
#include "../../include/rec_dtors.h"
#include "../../include/fx.h"        // FxElem (add_group 0x4423f0)
#include "../mission.h"
#include "../../include/rec1c8.h"

#include "./raw_place.h"            // raw_place: constructing a canonical class in raw storage

// raw field of a view class
#define WD_FLD(T, off) (*(T*)((char*)this + (off)))

// ---- party record (include/party.h Party, 0x5ac8 bytes) ---------------------------------------------------------
struct PartyW : Party {
    int& kind70() { return WD_FLD(int, 0x70); }                 // +0x70 faction / kind (0x490990)
    float& x30() { return WD_FLD(float, 0x30); }                // +0x30 map position (0x491e50)
    float& y34() { return WD_FLD(float, 0x34); }                // +0x34
    char& flag8b5() { return WD_FLD(char, 0x8b5); }             // +0x8b5 (0x490990)
    unsigned char& f8b4() { return WD_FLD(unsigned char, 0x8b4); }     // +0x8b4 follower list dirty (0x490a30, 0x4915d0, 0x4949d0)
    std::vector<int>& list() { return WD_FLD(std::vector<int>, 0x8b8); }   // +0x8b8 the follower ids as a std::vector<int> (0x4949d0)
    int*& fol_first() { return WD_FLD(int*, 0x8bc); }           // +0x8bc follower id vector (0x490a30; allocator word at +0x8b8)
    int*& fol_last() { return WD_FLD(int*, 0x8c0); }            // +0x8c0
    int*& fol_end() { return WD_FLD(int*, 0x8c4); }             // +0x8c4
    int& leader() { return WD_FLD(int, 0x8c8); }                // +0x8c8 attached-to party id, -1 = none
    unsigned char& f8cc() { return WD_FLD(unsigned char, 0x8cc); }     // +0x8cc
    int& f8e4() { return WD_FLD(int, 0x8e4); }                  // +0x8e4 (0x4915d0)
    int& f8e8() { return WD_FLD(int, 0x8e8); }                  // +0x8e8
    int& f918() { return WD_FLD(int, 0x918); }                  // +0x918
    int& f91c() { return WD_FLD(int, 0x91c); }                  // +0x91c
    // add_members 0x48c840, find_worth 0x4b1d30, clear_map_flag 0x4b32f0, skill_level 0x4b2370, place_on_map 0x4b2880,
    // add_xp_to_stack 0x4b1d80, init 0x48f180 and max_stacks 0x4b1700 are Party's own (include/party.h)
    PartyW* construct();                                        // 0x4934d0 (the constructor: returns this)
    void thunk_4936a0();                                        // 0x4936a0 (tail jump to the reset 0x48f180)
};

// ---- the party manager embedded in the game object at +0xf8 (include/party.h PartyMgr) --------------------------------
struct PartyMgrW : PartyMgr {
    unsigned& limit() { return WD_FLD(unsigned, 0x138dc); }     // +0x138dc id bound (PartyMgr::n_parties)
    PartyW* at(int idx) { return (PartyW*)((char*)pages[idx >> 4] + (idx & 0xf) * 0x5ac8); }
    PartyW* atu(unsigned idx) { return (PartyW*)((char*)pages[idx >> 4] + (idx & 0xf) * 0x5ac8); }
    PartyW* get_w(unsigned id) { int i = index_of(id); return at(i); }   // the inline lookup: index_of, then the page (by-value parameter)
    int is_live(unsigned id);                                   // 0x4903e0 (thiscall, ret 4)
    // PartyMgr::index_of(int) (include/party.h) is declared but not defined: the lookup 0x48ad00 is ui/mission.h's
    // Map10007::lookup(unsigned) (the id -> index hash table at the start of the manager), called as ((Map10007*)mgr)->lookup(id)
    void clear();                                               // 0x490340 (thiscall, ret 0)
    std::vector<int>& free_ids() { return WD_FLD(std::vector<int>, 0x138bc); }   // +0x138bc free party indices (first at +0x138c0)
    PartyMgrW* grow();                                          // 0x493830 (thiscall, ret 0)
    int add_slot();                                             // 0x493960 (thiscall, ret 0: the new party's id)
    int alloc_id();                                             // 0x493700 (the 0x98-byte record pool; see RecPool)
};


// ---- virtual views ------------------------------------------------------------------------------------------------
struct PartyVt {                                                // the party record's vtable (0x592644): slot 0 = vector deleting destructor
    virtual void v00(int flags) = 0;
};

struct EntVt {                                                  // vtable view of the container and of its children (vtable 0x5924a0 / MissionUi)
    virtual void v00(int f);                                    // +0x00 deleting destructor
    virtual void v01();
    virtual void v02(int p);                                    // +0x08 update (0x4908c0)
    virtual void v03();
    virtual void v04();
    virtual void v05();
    virtual void v06();
    virtual void v07();
    virtual void v08();
    virtual void v09();
    virtual void v10(int* a, float* b, float* c, float* d);     // +0x28 bounds (0x490710)
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();                                         // +0x40 reset (0x4906e0)
    virtual void v17();
    virtual void v18();
    virtual void v19();
    virtual void v20();
    virtual void v21();
    virtual void v22();
    virtual void v23(void* child);                              // +0x5c child moved (0x490570, 0x4906b0)
    virtual void v24();                                         // +0x60 (0x490570, 0x4905c0)
};

// ---- the container widget (MissionUi-derived, vtable 0x5924a0; widget_mgr.h ContainerBase, size 0x190) -------------------------
struct ContainerW : ContainerBase {
    std::vector<MissionUi*>& children() { return WD_FLD(std::vector<MissionUi*>, 0x178); }   // +0x178 the child widgets (first at +0x17c; 0x493a30)
    void**& ents_first() { return WD_FLD(void**, 0x17c); }      // +0x17c std::vector<child*> (0x4906b0, 0x490520)
    void**& ents_last() { return WD_FLD(void**, 0x180); }       // +0x180
    void**& ents_end() { return WD_FLD(void**, 0x184); }        // +0x184
    // release 0x48ef40 is mission_ui2's FrameBox::release; the base destructor 0x487c00 is Buffer's (see world_data.cpp)
    void dtor_common();                                         // 0x490520 (~ContainerBase)
    void place(int* a);                                         // 0x490570 (ret 4)
    void track_bounds(void* p);                                 // 0x4905c0 (ret 4)
    void activate_all();                                        // 0x4906b0
    void reset_all();                                           // 0x4906e0
    void bounds_of(int* a, float* p3, float* p4, float* p5);    // 0x490710 (ret 0x10)
    void update_all(int p);                                     // 0x4908c0 (ret 4)
    ContainerW* destroy(unsigned flag);                         // 0x490930 (scalar deleting destructor, ret 4)
    void* construct();                                          // 0x4931a0 (the constructor body: returns this)
    void add_child(MissionUi* child);                           // 0x493a30 (thiscall, ret 4)
};

inline int& troop_f54(Troop* t) { return *(int*)((char*)t + 0x54); }   // +0x54 second upgrade troop (0x494e40)

// ---- the game object (include/game_data.h GameData) -----------------------------------------------------------------
// mission_4's RiderTable::shares (0x48efa0) is what the level object's face-sharing check calls
struct RiderTable { int shares(const int& a, const int& b) const; };
struct LevelObj {                                               // g_level->obj (CellGrid +0xb0)
    char pad0[0x14];
    char* kinds;                                                // +0x14 0x88-byte face records (+0x24: surface kind)
    char pad18[0xb0 - 0x18];
};
struct LevelRef {
    char pad0[0xb0];
    LevelObj* obj;                                              // +0xb0 (0x490030)
};
extern LevelRef* g_level;                                   // 0xa732b4 (terrain_b.h calls it g_level)
struct FaceId {
    int m0;
    int m4;                                                     // +4 face index (-1: none)
};

int __fastcall trigger_entity(int context);                     // 0x490430 (the scene entity of a trigger context)
int trigger_item_mesh(int context);                             // 0x4904a0 (context in EAX in the original: a static function with a register convention)

extern char* g_party_templates;                                     // 0x5e29f8 (0xb0-byte party-template rows, +0x38 counter: 0x4915d0)
extern float g_k15;                                             // 0x590da0 (15.0f)
extern float g_k065;                                            // 0x590b6c (0.65f)

struct GameW : GameData {
    rgl::vector4& cur_color() { return WD_FLD(rgl::vector4, 0x13fb4); }     // +0x13fb4 (0x4952f0)
    rgl::vector4& pos_delta() { return WD_FLD(rgl::vector4, 0x13fc4); }     // +0x13fc4
    rgl::vector4& trig_row(int i) { return ((rgl::vector4*)((char*)this + 0x14014))[i]; }   // +0x14014 the trigger frame s, f, u, o
    int& encountered_a() { return WD_FLD(int, 0x13a0c); }       // +0x13a0c (0x494ad0)
    int& encountered_b() { return WD_FLD(int, 0x13a10); }       // +0x13a10
    char& objectives_on() { return WD_FLD(char, 0x13a18); }     // +0x13a18 (0x494ad0)
    char& scene_open() { return WD_FLD(char, 0x1a05c); }        // +0x1a05c (0x492a20, 0x492f40)
    int& cur_party_idx() { return WD_FLD(int, 0x1a1f0); }       // +0x1a1f0 (0x490430)
    unsigned& cur_agent_idx() { return WD_FLD(unsigned, 0x1a1f4); }   // +0x1a1f4
    int& free_slot() { return WD_FLD(int, 0x1a204); }           // +0x1a204 (0x4904a0)
        PartyMgrW& pm() { return *(PartyMgrW*)&parties; }
    PartyW* party(int id) { PartyMgrW& m = pm(); return m.at(m.index_of(id)); }

    // find_at 0x48ea80, agent_low 0x4638f0 and add_experience 0x4428c0 are GameData's own (include/game_data.h). add_experience is
    // void there, but add_random_experience (0x4952a0, a byte-exact row returning int) returns the callee's eax: that one call
    // goes through GameData::add_experience viewed as returning int (a pointer-to-member of the same function, see there)

    void random_position_around(Vec2 pos, float r, Vec2* out);               // 0x490030 (ret 0x10)
    char position_around(Vec2 pos, float r, Vec2* out, char any);    // 0x490160 (ret 0x14)
    void random_position_around(float x, float y, float radius, float* out);    // 0x490030 (the same function, called with the pair as two floats: 0x491e50)
    int stack_dna(unsigned id, int stack, int a3, int a4);      // 0x4902a0 (ret 0x10)
    void mark_kind(int kind);                                   // 0x490990 (ret 4)
    void detach_party(unsigned id);                             // 0x490a30 (ret 4)
    void count_units(unsigned id, int* a, int* b, int* c, int* d);  // 0x490ce0 (ret 0x14)
    void list_members(unsigned id, int* out, int* n);          // 0x490fa0 (ret 0xc)
    void recruit(int id, int count, PartyW* dest);              // 0x4910a0 (ret 0xc)
    void handout(PartyW* p, int id);                            // 0x491430 (ret 8)
    void remove_party(int id);                                      // 0x4915d0 (ret 4)
    bool can_transfer(unsigned id, int other, char flag);       // 0x4918d0 (ret 0xc)
    int gate_rating(unsigned p1, unsigned p2, int p3);          // 0x491b20 (ret 0xc)
    void set_pos(float x, float y, int id, float radius);       // 0x491e50 (ret 0x10)
    int token_lookup(int a, int b, int c, int d);               // 0x491f20 (ret 0x10)
    void transfer_stack(int a, int b, char flag);               // 0x492530 (ret 0xc)
    void free_arrays();                                         // 0x492a20 (ret 0)
    void world_free();                                          // 0x492f40 (ret 0)
    void quest_done(int quest);                                 // 0x494410 (ret 4)
    void quest_succeeded(int quest);                            // 0x494560 (ret 4)
    void quest_failed(int quest);                               // 0x494680 (ret 4)
    void set_quest_concluded(int quest, int value);             // 0x4947a0 (ret 8)
    void set_quest_cancelled(int quest);                        // 0x4948c0 (ret 4)
    void set_objective(int party, int value);                   // 0x4949d0 (ret 8)
    void switch_objective(int party, int mode);                 // 0x494ad0 (ret 8)
    void share_experience(int party, float amount);             // 0x494b60 (ret 8)
    void award_experience(int party, float amount, int mode);   // 0x494e40 (ret 0xc)
    int add_random_experience(int amount);                      // 0x4952a0 (ret 4)
    void spread_party_experience(int amount);                   // 0x4957d0 (ret 4)
};

// ---- the mission object (game/mission.h Mission) and its agent records -----------------------------------------------
struct AgentW : Agent {
    int& f56d4() { return WD_FLD(int, 0x56d4); }                // +0x56d4 weapon mode (0x492230)
};
struct MissionW : Mission {
    Agent**& pages_raw() { return WD_FLD(Agent**, 0x1c); }      // +0x1c the agent page table as a raw pointer (0x492230)
    int& f6520() { return WD_FLD(int, 0x6520); }                // +0x6520 (0x491f50: the signed speed term)
    AgentW* rec(unsigned i) { return (AgentW*)((char*)agents.pages[i >> 4] + (i & 0xf) * 0x5928); }   // agent record i (page of 16, no validity check)
    int count_agents(int team);                                 // 0x491f50 (ret 4)
    bool all_defeated(int team);                                // 0x4920e0 (ret 4)
    bool hero_down();                                           // 0x492170
    unsigned find_agent_by_troop(int troop);                    // 0x4921b0 (ret 4)
    void add_wind_effect(int a1, int a2);                       // 0x4952f0 (ret 8)
    void set_weapon_order_1(int team, unsigned value);          // 0x492230 (ret 8)
    void set_weapon_order_2(int team, unsigned value);          // 0x492350 (ret 8)
};

// ---- the battle / order object (g_mission_ui 0xa73300): two party ids at +4 / +8, side, tier, rating ----------------------
struct BattleW {
    int ids[2];                                                 // +0x00 / +0x04 the two party ids (0x492470, 0x492750: indexed [sel * 4])
    char pad8[0x10 - 0x08];
    int flags;                                                  // +0x10
    int mode;                                                   // +0x14
    char pad18[4];
    int rating;                                                 // +0x1c (0x492470)
    char pad20[0x24 - 0x20];
    int side;                                                   // +0x24
    int tier;                                                   // +0x28
    char pad2c[0x138 - 0x2c];
    int sel;                                                    // +0x138
    char pad13c[0x144 - 0x13c];
    int f144;                                                   // +0x144
    void refresh_rating();                                      // 0x492470
    void select_side(int side);                                 // 0x492750 (ret 4)
};


// ---- vectors of 8-byte integers, 4-byte-integer vectors (VC7.1 layout: allocator word first) -----------------------------
#include "../../include/pod_elems.h"
struct FPair { float a, b; };

struct I64VecW : I64Vec {                                       // include/i64_vec.h; the slot-array / quest-record vector
    E8*& first() { return WD_FLD(E8*, 4); }
    E8*& last() { return WD_FLD(E8*, 8); }
    E8*& end() { return WD_FLD(E8*, 0xc); }
    unsigned size() const { return *(E8* const*)((const char*)this + 4) == 0 ? 0 : (unsigned)(*(E8* const*)((const char*)this + 8) - *(E8* const*)((const char*)this + 4)); }
    unsigned capacity() const { return (unsigned)(*(E8* const*)((const char*)this + 0xc) - *(E8* const*)((const char*)this + 4)); }
    unsigned max_size() const { return (unsigned)(-1) / sizeof(E8); }
    __declspec(noinline) void reserve(int* newsize);            // 0x493c40 (thiscall, ret 4; callers keep it a call)
};
struct I64VecOp {                                               // the same vector as an opaque callee (the callers reload size after reserve)
    int pad0;
    long long* _Myfirst;
    E8* _Mylast;
    E8* _Myend;
    void reserve(int* newsize);                                 // 0x493c40
};
struct SlotHolder40 {                                           // an object with the 8-byte vector at +0x40 (0x493db0)
    char pad0[0x40];
    void set40(int idx, int val);                               // 0x493db0 (thiscall, ret 8)
};

struct IntVecW {                                                // std::vector<int> with the leading allocator dword (game/int_vec.h IntVec)
    int pad0;
    int* _Myfirst;
    int* _Mylast;
    int* _Myend;
    unsigned size() const { return _Myfirst == 0 ? 0 : (unsigned)(_Mylast - _Myfirst); }
    void insert_a(int* where, unsigned count, const int& val);     // 0x4927d0 (the instance below; also used for pointer vectors)
    void insert_n(int* where, unsigned count, const int& val);     // 0x43e7b0
    void insert_c(int* where, unsigned count, const int& val);     // 0x41a2c0
    void resize(unsigned n, int v);                                 // 0x43f310
    void kill_range(int n, int pad);                                // 0x41ad20
    void push_back(const int& v);                                   // 0x4936b0 (out of line)
};


// ---- opaque records whose out-of-line destructors the game object's teardown calls (0x492a20) ---------------------------------
// struct RecS : src/include/rec_dtors.h
// struct RecT : src/include/rec_dtors.h
// struct RecQ : src/include/rec_dtors.h
struct RecPoly { virtual ~RecPoly(); };                         // a scene record (game_data.h Level494 behind a vtable): virtual destructor in slot 0

// ---- the container / list panels (MissionUi family) ---------------------------------------------------------------------------
struct PanelVt {                                                // vtable slots the list panel reaches (slot n at +4n)
    virtual void v00(void* p = 0); virtual void v01(void* p = 0); virtual void v02(void* p = 0); virtual void v03(void* p = 0);
    virtual void v04(void* p = 0); virtual void v05(void* p = 0); virtual void v06(void* p = 0); virtual void v07(void* p = 0);
    virtual void v08(void* p = 0); virtual void v09(void* p = 0); virtual void v10(void* p = 0);
    virtual void v11(void* p = 0);                              // +0x2c (0x493400)
    virtual void v12(void* p = 0); virtual void v13(void* p = 0); virtual void v14(void* p = 0); virtual void v15(void* p = 0);
    virtual void v16(void* p = 0); virtual void v17(void* p = 0); virtual void v18(void* p = 0); virtual void v19(void* p = 0);
    virtual void v20(void* p = 0); virtual void v21(void* p = 0); virtual void v22(void* p = 0); virtual void v23(void* p = 0);
    virtual void v24(void* p = 0); virtual void v25(void* p = 0); virtual void v26(void* p = 0);
    virtual void v27(void* p = 0);                              // +0x6c (0x493400)
    virtual void v28(void* p = 0); virtual void v29(void* p = 0); virtual void v30(void* p = 0); virtual void v31(void* p = 0);
    virtual void v32(void* p = 0); virtual void v33(void* p = 0); virtual void v34(void* p = 0); virtual void v35(void* p = 0);
    virtual void v36(void* p = 0); virtual void v37(void* p = 0); virtual void v38(void* p = 0); virtual void v39(void* p = 0);
    virtual void v40(void* p = 0);
    virtual void v41(void* p = 0, void* q = 0);                 // +0xa4 (0x493310)
};
typedef void (__stdcall* PanelFn)(void* self, unsigned char v);
struct E16 { int a, b, c, d; };
struct Vec16W {                                                 // the 16-byte-element vector at +0x18c (allocator word, then first/last/end)
    int pad0;
    E16* _Myfirst;
    E16* _Mylast;
    E16* _Myend;
    unsigned size() const { return _Myfirst == 0 ? 0 : (unsigned)(_Mylast - _Myfirst); }
};
struct ListPanelW : MissionUi {                                 // the text list panel (vector of rows at +0x18c)
    void set_mode();                                            // 0x4932a0
    void set_index(int v);                                      // 0x493310 (ret 4)
    void reset_tail();                                          // 0x4933b0
    void* destroy(unsigned flags);                              // 0x4933d0 (scalar deleting destructor, ret 4)
    void append(const tstring* name);                           // 0x493400 (ret 4)
};
struct DropDownW : DropDown {
    DropDownW* construct();                                     // 0x4931d0 (the constructor body: returns this)
};
// the vector<tstring> at ListPanelW+0x18c: its out-of-line _Tidy (0x42d390, protected: reached through a derived class)
namespace std { template<> void vector<tstring, allocator<tstring> >::_Tidy(); }   // 0x42d390
struct TailVec : std::vector<tstring> { using std::vector<tstring>::_Tidy; };
int panel_count_493300();                                       // 0x493300 (returns 5)
extern char vtbl_5924a0[];                                      // 0x5924a0
extern char vtbl_592570[];                                      // 0x592570
extern char vtbl_591d58[];                                      // 0x591d58
extern char vtbl_592644[];                                      // 0x592644

struct PtrStack14 {                                             // an object with a pointer vector (no allocator word) at +0x14
    char pad0[0x14];
    void** _Myfirst;                                            // +0x14
    void** _Mylast;                                             // +0x18
    unsigned size() const { return (unsigned)(_Mylast - _Myfirst); }
    void pop_back_delete();                                     // 0x492ee0
};

// ---- the party pool (0x98-byte records, pages of 16) and the cache group (E68 members) ----------------------------------------
#include "../rec98.h"                                             // Rec98: the 0x98-byte pool record, constructor 0x48c210
struct RecPool {                                                // an object with the free-id vector at +4 and the page vector at +0x14
    int next_id;                                                // +0
    std::vector<int> free_ids;                                  // +4 (allocator word, first at +8)
    std::vector<Rec98*> pages;                                  // +0x14 (first at +0x18)
    int serial;                                                 // +0x24
    void alloc_record();                                        // 0x493700 (thiscall, ret 0)
};
// The pool record's constructor is stat_panel_x's Event::Event (0x48c210), declared under the defining class's name; a party record
// as `new Party[16]` builds it: vtable 0x592644 at +0, constructor 0x4934d0 (nothrow), whose body is PartyW::construct below (the
// constructor forwards to it, so the array new binds to the one definition).
// Its destructor has no code in the binary (no caller, no definition): it only makes `new[]` keep the count cookie, so it is
// an empty inline one here (an out-of-line ~Rec98 would be an external no object defines); the pages are used as Rec98.
struct Event {
    char body[0x98];
    Event();   // 0x48c210
    ~Event() {}
};
struct PartyNew {
    ~PartyNew() {}                                              // (non-virtual here: the vtable is stored by PartyW::construct; the destructor only makes `new[]` keep a count cookie: no code in the binary)
    char body[0x5ac8];
    PartyNew() throw() { ((PartyW*)(void*)this)->construct(); }   // 0x4934d0 (forwards to PartyW::construct)
};
#include "./thiscall_free.h"
#include "../../rgl/resource.h"                               // resource_manager g_resmgr 0x5d6fa0
struct ResSlot {                                                // a resource slot of the cache group (0x493df0, 0x493e90)
    char pad0[0x5c];
    int f5c;                                                    // +0x5c
    int key;                                                    // +0x60
    int refs;                                                   // +0x64 reference count
};
struct E68W : E68 {
    std::vector<ResSlot*>& slots() { return WD_FLD(std::vector<ResSlot*>, 0x54); }   // +0x54 (first at +0x58)                                             // the vector at +0x54 holds resource slots (refcount at +0x64)
    void purge();                                               // 0x493e90 (thiscall, ret 0)
    void* find_or_create(int* key);                             // 0x493df0 (thiscall, ret 4)
};
struct Vec68W : Vec68 {
    void purge_all();                                           // 0x493f30 (thiscall, ret 0)
};

struct ViewParams {
    ViewParams();   // 0x46b940
};
struct SceneBaseCtor {                                          // the scene base object (0x36c bytes): its constructor body, kept opaque from 0x495650
    SceneBaseCtor* construct();                                 // 0x493ab0 (returns this)
};

// ---- the tableau table (g_tableaus 0xa75f40; the shared header tableau.h has the whole class) -------------------------------------
#include "./tableau.h"

#include "./widget_mgr_w.h"

// ---- the window manager (g_windows 0xa73980: 30 screen objects, the open-screen deque at +0x78) --------------------------------
#include "../screen_obj.h"                                    // ScreenObj: the vtable view of a screen (slot 0 = deleting destructor)
struct WindowMgrW {
    ScreenObj* arr[30];                                         // +0x00
    std::deque<int> dq;                                         // +0x78 (allocator word at +0x78; map at +0x7c)
    void open(int window);                                      // 0x494260 (ret 4)
    void set_state(int state);                                  // 0x495700 (ret 4)
};
struct GlobVec {                                                // the running presentation's list holder (global 0x5e299c)
    int f0;
    void* first;                                                // +4
    int f8;                                                     // +8
    int fc;                                                     // +0xc
};
extern GlobVec* g_presentation;                            // 0x5e299c

// ---- the sound player (g_sound_mgr 0x5aa128; agent3/agent_fx_sound.h has the full class under src/rgl-based headers) ----------------
// agent3/agent_fx_sound.h's SoundMgr::play (0x442650); the header does not go with this one
struct SoundMgr { int play(int idx, int flags, const float* pos, float freq, int* out_id); };
struct SoundW {
    char pad0[0x40];
    rgl::vector4 listener;                                      // +0x40
    char pad50[0x248 - 0x50];
    std::vector<int> channels;                                  // +0x248 looping channels (allocator word, first at +0x24c)
    rgl::matrix& listener_frame() { return WD_FLD(rgl::matrix, 0x10); }   // +0x10 the listener frame (position at +0x40: 0x494310)
    void play_ui(int sound, unsigned flags);                    // 0x4942f0 (ret 8)
    void stop_all();                                            // 0x494310
};
extern SoundW g_sound_mgr;                                          // 0x5aa128
extern int g_quest_sounds[];                                    // 0x5aa184 .. (completed 0x5aa184, succeeded 0x5aa188, concluded 0x5aa18c, failed 0x5aa190, cancelled 0x5aa194)

#define g_msglog (*(MsgLog*)&g_gui)                          // the on-screen message log view of the UI root g_gui (0x5d7080)
extern StrTable g_strtable;                                     // 0x5e2a18
void mb_snprintf(char* out, unsigned size, const char* fmt, ...);   // 0x415100
#include "../config/config.h"                                  // g_cfg 0x5a9f98
#include "../strat/gui_manager.h"                             // gui_manager g_gui 0x5d7080 (UI root: set_o568, reset, f560)
#include "../../include/renderer.h"                              // g_render 0xa17910 (dev at +0x618 = 0xa17f28)
struct RotRows { rgl::vector4 s, f, u; };                       // the rotation part of a frame (0x30 bytes)
#include "../../include/mat4.h"                              // rgl::mat3
rgl::mat3* __cdecl identity3(rgl::mat3* out);                  // 0x40cd60 (cdecl; returns out)
extern "C" void __stdcall FSOUND_StopSound(int handle);                            // 0x5342a6 _FSOUND_StopSound@4
extern "C" void __stdcall FSOUND_3D_Listener_SetAttributes(void* pos, int, int, int, int, int, int, int);   // 0x5342dc _FSOUND_3D_Listener_SetAttributes@32

// ---- the carried-item placement of an agent body (0x495430): views of the item kind, mesh, skeleton ---------------------------------------
struct CarryKind {                                              // include/item_kind.h ItemKind (0x238 bytes, g_items 0x5e29d0)
    char pad0[0x1c8];
    unsigned char type;                                         // +0x1c8 (7 = shield)
    char pad1c9[0x1d0 - 0x1c9];
    unsigned f1d0;                                              // +0x1d0
    unsigned f1d4;                                              // +0x1d4
    char pad1d8[0x238 - 0x1d8];
};
struct CarryMesh {                                              // rgl::mesh seen by the placement: visibility mask and local frame
    char pad0[0x8c];
    rgl::matrix frame;                                          // +0x8c (m_cc)
    unsigned vis;                                               // +0xcc (mask)
};
struct CarryRef { int item; unsigned m4; };                     // an 8-byte carried item ref (include/item_ref.h ItemRef)
struct CarryCtx {                                               // the agent's three per-slot mesh arrays
    char pad0[0x10];
    CarryMesh* c1[4];                                           // +0x10
    CarryMesh* c2[4];                                           // +0x20
    CarryMesh* c3[4];                                           // +0x30
};
struct CarryElem {                                              // 0x1dc-byte runtime state of one bone (include/fx.h FxElem: add_group 0x4423f0)
    char pad0[0x1dc];
};
struct CarrySkel {                                              // game/skeleton.h Skeleton (remove_shape 0x439ec0)
    int count;                                                  // +0
    CarryElem* elems;                                           // +4
};
// The members the placement calls, on the classes that define them (the headers game/skeleton.h and include/shape.h do not go with
// this one): include/fx.h FxElem::add_group(int) (fx.h is included), game/skeleton.h Skeleton::remove_shape(shape_base*), include/item_kind.h
// ItemKind::get_carry_pos, rgl/xmlcfg/game_util.h AgentX::agent_class.
struct shape_base;
struct Skeleton {
    void remove_shape(shape_base* s);   // 0x439ec0
};
struct AgentX {
    int agent_class() const;   // 0x433140
};
struct CarryBody {
    char pad0[0x304];
    CarrySkel* skel;                                            // +0x304
};
void __cdecl place_scene_agents(CarryBody* holder, CarryRef* table, CarryCtx* ctx);   // 0x495430

// ---- 0x4952f0: the effect object the mission's scene container hands out (the wind / light record, 0x7c bytes) ----------------------------
#include "../eff_obj.h"                                         // EffObj / EffMgr

// ---- the particle-system table and the emitter control (0x4955e0) -----------------------------------------------------------------------------
struct Emitter;
struct EmitTarget {                                             // virtual object at mission +0x59c0
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23();
    virtual void s24();
    virtual void add(Emitter* e);                               // +0x64
};
#include "../emit.h"
struct EmitterCtl {
    char pad0[0x59c0];
    EmitTarget* target;                                         // +0x59c0
    char pad59c4[0x59f4 - 0x59c4];
    std::vector<int> list;                                      // +0x59f4
    void add(int e);                                            // 0x4955e0 (ret 4)
};

// ---- 0x495650: the scene list ------------------------------------------------------------------------------------------------------
#include "../../include/frustum.h"                              // rgl::frustum (0x120 bytes; the ctor 0x41ef60); the stand-in "Frustum" was 0x134 bytes with 0x14 unknown after it
struct SceneBase {
    char d[0x36c];
    inline SceneBase();                                         // 0x493ab0 (forwards to SceneBaseCtor::construct, the one definition)
};
inline SceneBase::SceneBase() { ((SceneBaseCtor*)this)->construct(); }
struct SceneObject : SceneBase { rgl::frustum frustum; char tail[0x14]; };   // 0x4a0 total
struct SceneList {
    char pad0[0x10];
    std::vector<SceneObject*> v;                                // +0x10
    SceneObject* push();                                        // 0x495650
};

// Views of classes defined in other translation units, declared here under the defining class's name (their headers do not go with this
// one): terrain_a's Mission3::speed_factor (0x467740, ignores this), terrain_b's
// CellGrid::spawn_wind (0x478f30).
struct Mission3 { float speed_factor(float x); };
struct Wind;
struct CellGrid {
    ~CellGrid();   // 0x475d50
    Wind* spawn_wind();   // 0x478f30
    void init(const float* lo, const float* hi, int dimx, int dimy);   // 0x478830
};

extern WindowMgrW g_windows; // 0xa73980
#endif // GAME_WORLD_DATA_H
