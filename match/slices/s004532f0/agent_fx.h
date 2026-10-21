// Shared declarations of slices s004529e0 / s004532f0 / s00454320 (Agent item drops, shield damage, message log
// and the agent's skeleton visibility). Copied into each slice dir under the same name; keep identical.
// Names are provisional; every offset comes from the member functions at 0x4529e0..0x454391.
#ifndef AGENT_FX_H
#define AGENT_FX_H
#include <stddef.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

#define MB_NOTHROW throw()

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() {}
    tstring(const char* s) MB_NOTHROW;              // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;           // 0x42c110
    ~tstring() MB_NOTHROW;                          // 0x42bc40
    void operator=(const tstring& o) MB_NOTHROW;    // 0x42bc50
    int find(const char& c, int start) const;       // 0x42c080
    tstring substr(int start, int end) const;       // 0x42c4a0
    tstring strip_tags() const;                     // 0x42c5d0
};

struct mat4 {
    vector4 s, f, u, o;
    mat4 operator*(const mat4& b) const;            // 0x40e870 (this = left operand)
    vector4 transform_point(const vector4& p) const;    // 0x408460
};
struct pos : matrix {};
pos* identity_pos(pos* out);                        // 0x40cd00 (cdecl)

// ---- resources ------------------------------------------------------------------------------------------
struct resource;
struct mesh {                                       // 0x260 bytes (rgl::mesh of s0044eb00/mesh.h)
    char pad0[0x2c];
    resource* res;                                  // +0x2c
    char pad30[0x260 - 0x30];
    void clear_geometry();                          // 0x428e80
    void append_skinned(mesh* other, const mat4* m, int bone, unsigned color, char flip);   // 0x44eb00
    void calc_bounds(vector4* mn, vector4* mx);     // 0x438d80 (mesh_node_calc_bounds)
};
struct resmgr { resource* get_material(const tstring& name); };    // 0x418a70, global at 0x5d6fa0
extern resmgr g_resmgr;                             // 0x5d6fa0
struct vec2 { float x, y; };

// ---- the text-mesh builder (object at *0x5e2934): lays out `text` into `m` -------------------------------
struct Font {
    // 0x4520d0 (thiscall, ret 0x24)
    void layout(vector4* out_extent, const tstring* text, const vector4* pos, const vec2* scale, mesh* m,
                const int* a, const int* b, const float* c, int flags);
    // 0x452bb0 (thiscall, ret 0x24): clears the mesh, lays the text out into it, returns out_extent
    vector4* layout_clear(vector4* out_extent, const tstring* text, const vector4* pos, const vec2* scale, mesh* m,
                       const int* a, const int* b, const float* c, int flags);
};
extern Font* g_font;                                // 0x5e2934

// ---- string table / message log ------------------------------------------------------------------------
struct StrTable { tstring* lookup(const tstring& key) MB_NOTHROW; };   // 0x43a1e0
extern StrTable g_strtable;                         // 0x5e2a18

// message log (object at 0x5d7080, see s004333c0 / s004428c0): 32 text meshes with backgrounds, a line history
struct MsgLog {
    char pad0[0x1714];
    char enabled;                                   // +0x1714
    char pad1715[0x171c - 0x1715];
    float total_h;                                  // +0x171c
    int cur;                                        // +0x1720
    char pad1724[4];
    mesh text[32];                                  // +0x1728
    mesh bg[32];                                    // +0x6328
    int age[32];                                    // +0xaf28
    float height[32];                               // +0xafa8
    char padb028[4];
    int color[32];                                  // +0xb02c
    char padb0ac[0xb340 - 0xb0ac];
    std::vector<tstring> lines;                     // +0xb340 (first at +0xb344)
    void add(tstring msg, int color);               // 0x452c00 (thiscall, ret 0x14)
};
extern MsgLog g_msglog;                             // 0x5d7080

// ---- items ---------------------------------------------------------------------------------------------
struct ItemKind {                                   // 0x238 bytes (array at 0x5e29d0)
    char pad0[0x1c8];
    unsigned type_flags;                            // +0x1c8 (low byte = itp_type_*)
    char pad1cc[4];
    __int64 capability_flags;                       // +0x1d0 itcf_*
    char pad1d8[0x238 - 0x1d8];
    void get_carry_pos(pos* out, int* bone_flags, int* bone);   // 0x433e40
    int wield_bone();                               // 0x433df0 (s00433cb0 calls it ammo_skill)
};
#define itcf_carry_mask 0x00000007f0000000i64
extern ItemKind* g_items;                           // 0x5e29d0

struct ItemRef {                                    // 8 bytes: item index + amount/modifier word
    int item;
    union {
        unsigned m4;
        struct { unsigned char amount; unsigned char pad[2]; signed char mod; } b;
    };
    int stat_200();                                 // 0x434eb0
    void reduce(int n);                             // 0x437510
};

// ---- animator ("FxDef" of s00421aa0, "Skeleton" of s0043edf0) -------------------------------------------
struct Psys;
struct lod_mesh2;
struct Skeleton {
    int count;                                      // +0
    char* elems;                                    // +4 (0x1dc-byte records, m84 at +0x84)
    char pad8[0x20 - 8];
    std::vector<Psys*> psys;                        // +0x20 (first at +0x24)
    std::vector<lod_mesh2*> groups;                 // +0x30 (first at +0x34)
    char pad40[0x50 - 0x40];
    char done;                                      // +0x50
    char pad51[0x58 - 0x51];
    char frozen;                                    // +0x58
    char pad59[0x64 - 0x59];
    int has_poses;                                  // +0x64
    void update();                                  // 0x422130
    void update_poses();                            // 0x422080
    void skin(Psys* p, char flag);                  // 0x4291c0
    void skin_group(lod_mesh2* g, char flag);       // 0x452530
    void set_visible(char on);                      // 0x452f80
    char* rec(int i);                               // 0x43f260
    __forceinline char* rec_inl(int i)
    {
        if (!done) {
            if (has_poses)
                update_poses();
            else
                update();
        }
        return elems + i * 0x1dc + 0x84;
    }
};

struct AgentBody {
    char pad0[0x58];
    mat4 m;                                         // +0x58
    char pad98[0x304 - 0x98];
    Skeleton* skel;                                 // +0x304
};

// ---- the mission ---------------------------------------------------------------------------------------
struct TroopRec {                                   // 0x508 bytes
    char pad0[0x4a4];
    ItemRef items[10];                              // +0x4a4
    char pad4f4[0x508 - 0x4f4];
};
struct GameData {
    char pad0[0x88];
    int dummy88;
    int player;                                     // +0x8c
    char pad90[0x13a00 - 0x90];
    TroopRec* chars;                                // +0x13a00
    int skill_level(int troop, int skill);          // 0x437b20 (thiscall, ret 8)
};
extern GameData* g_game;                            // 0x5e2930

struct Agent;
struct Mission {
    char pad0[0x5c];
    int local_id;                                   // +0x5c
    char pad60[0x5df0 - 0x60];
    int cur_rec;                                    // +0x5df0
    void queue_dead(Agent* a);                      // 0x442dc0
    int add_placed(const int* kind, const int* id, const int* value, const matrix* m, int* spawn_id);   // 0x44f870
};
extern Mission* g_mission;                          // 0x5e292c
struct Rec58 { char pad[0x20]; unsigned flags; char pad24[0x58 - 0x24]; };
extern Rec58* g_rec58;                              // 0x5e2a00

struct PhysSet { void destroy_world(int idx); };    // 0x441360
extern PhysSet g_phys;                              // 0xa181f8

struct SlotLink { char pad[0xcc]; int m_cc; };

// ---- the agent (0x5928 bytes) --------------------------------------------------------------------------
struct Agent {
    char pad0[8];
    int id;                                         // +8
    char padc[4];
    int f10;                                        // +0x10
    int f14;                                        // +0x14
    char pad18[0x2c - 0x18];
    int f2c;                                        // +0x2c
    char pad30[0x50 - 0x30];
    float px, py, pz;                               // +0x50
    char pad5c[0x80 - 0x5c];
    float f80;                                      // +0x80
    char pad84[4];
    int troop;                                      // +0x88
    char pad8c[0x14c - 0x8c];
    SlotLink* links[3][4];                          // +0x14c
    char pad17c[0x1d0 - 0x17c];
    ItemRef items[10];                              // +0x1d0
    char b220[10];                                  // +0x220
    char pad22a[0x22c - 0x22a];
    int wield_a;                                    // +0x22c
    int wield_b;                                    // +0x230
    char pad234[0x23c - 0x234];
    int f23c;                                       // +0x23c
    char pad240[0x250 - 0x240];
    AgentBody* body;                                // +0x250
    char pad254[0x5834 - 0x254];
    char queued;                                    // +0x5834
    char pad5835[0x5880 - 0x5835];
    int f5880;                                      // +0x5880
    char pad5884[0x58dc - 0x5884];
    int phys_idx;                                   // +0x58dc

    void on_wield_reset();                          // 0x445a20
    int move_anim(int mode);                        // 0x43cc30
    void set_order(int slot, int id, unsigned prio, float rate);   // 0x4379c0
    void play_sound_var(int* kind, float* var);     // 0x443730
    void drop_held();                               // 0x4529e0
    void shield_hit(int damage);                    // 0x4532f0
    void get_label_box(void* out);                  // 0x453050
    void on_removed();                              // 0x454320
};
#endif
