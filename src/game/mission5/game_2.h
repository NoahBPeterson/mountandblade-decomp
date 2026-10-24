// game_2.h: declarations of src/game/mission5/game_2.cpp (TU 115 "game_2", 0x520b20..0x525160): the map and world-map scene
// builders (map.txt, the tree meshes of the campaign map), the module-file loader Game::load_game_data (factions.txt,
// skills.txt, troops.txt, scenes.txt, quests.txt, triggers.txt, simple_triggers.txt) and the record constructors / readers
// it calls. The shared classes are used where they exist (Game: src/game/party/game.h; the Data\*.txt record classes are
// the destructor-side classes of src/rgl/xmlcfg/records.h: RecS = troop, RecQ = quest, RecT = faction, RecY = scene);
// this header adds only views of their padding (named ...View) and the classes that exist nowhere else. Offsets cite the
// function that proves them.
#ifndef MISSION5_GAME_2_H
#define MISSION5_GAME_2_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <new>
#include "../party/game.h"
#include "../../include/iface.h"        // IfaceB (the polymorphic base of the scene record)
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../../rgl/scene/scene_entity.h"
#include "../terrain_b/cell_grid.h"      // CellGrid (the level, 0xa732b4), NavData, Wind
#include "../../rgl/scene/world.h"            // rgl::world

void __cdecl mb_error(const char* fmt, ...);        // 0x42cc80

#define RSTR(p) ((rgl::string*)(p))                 // the stream's string slot takes rgl::string* (same 16-byte tstring)

// ---- the text reader the loaders build on their stack (vtable 0x54ac88; three inline constructor levels) -----------
// vtable 0x54a890 level: the class with a user-declared (empty) destructor, as in src/rgl/core/resmgr.h
struct file_text_b : stream_base {
    FILE* fp;                                       // +0x30
    char failed;                                    // +0x34
    STREAM_SLOTS
    __forceinline file_text_b() {}
    ~file_text_b() {}
};
struct file_text_base : file_text_b {
    __forceinline file_text_base() { failed = 1; fp = 0; state = 0; }
};
struct file_text : file_text_base {                 // vtable 0x54ac88: text reader
    __forceinline file_text() {}
    STREAM_SLOTS
    bool open_read_text(const char* path);          // 0x415d60 (ret 4)
};

// ---- std::vector<tstring> internals (VC7.1, 0x10-byte element) ---------------------------------------------------
#include "../../include/ptr_tag.h"             // ptr_tag: std::_Nonscalar_ptr_iterator_tag (empty, passed by value)
struct tstring_alloc {
    int pad;                                        // +0 allocator subobject
    void destroy(tstring* f, tstring* l);           // 0x42d360
    void construct(tstring* p, const tstring& v);   // 0x436c90
};
void __cdecl uninit_fill_n_str(tstring* f, unsigned n, const tstring& v, tstring_alloc& al, ptr_tag tag);   // 0x438f00
tstring* __cdecl uninit_copy_str(tstring* f, tstring* l, tstring* d, tstring_alloc& al, ptr_tag tag);       // 0x524cd0
void __stdcall vec_ctor_iter(void* dst, unsigned size, unsigned count, void* ctorfn);                       // 0x401800

struct tstring_vec : tstring_alloc {
    tstring* first;                                 // +4
    tstring* last;                                  // +8
    tstring* end;                                   // +0xc
    tstring_vec() : first(0), last(0), end(0) {}
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
    unsigned capacity() const { return first == 0 ? 0 : (unsigned)(end - first); }
    tstring* last_pos() const { return last; }
    void _Xlen() const;                                         // 0x43e2a0
    void insert_n(tstring* pos, unsigned n, const tstring& v);  // 0x43e310
    void reserve(unsigned n);                                   // 0x524bc0
    void push_back(const tstring& v)
    {
        if (size() < capacity()) {
            ptr_tag t;
            tstring* p = last;
            uninit_fill_n_str(p, 1, v, *this, t);
            last = p + 1;
        } else {
            tstring* pos = last_pos();
            insert_n(pos, 1, v);
        }
    }
};

// ---- the data tables and their globals ---------------------------------------------------------------------------
struct NameMgr { tstring* find(tstring* key); };    // 0x4c6010
extern NameMgr g_name_mgr;                          // 0x5e2a18
extern int g_level_lo[];                            // 0x5937c8
extern int g_level_hi[];                            // 0x5937cc
extern float g_troop_rand_scale;                    // 0x54a6bc
extern int g_clamp_max;                             // 0x8ee3ac4

struct Pair { int a; int b; };

// triggers.txt statement (0x98-byte element, allocated with new[])
struct StmtRec {
    int kind;                       // +0x00
    char pad04[4];
    int f08;                        // +0x08
    char pad0c[4];
    __int64 f10;                    // +0x10
    char pad18[0x78];
    int f90;                        // +0x90
    char pad94[4];
    StmtRec() : f90(-1) {}
};
struct TriggerStmt {                // the 0x2c-byte statement of a simple trigger (at +4 of its 0x30-byte record)
    StmtRec* p;                     // +0
    int n;                          // +4
    char flag;                      // +8
    void init(int op, int idx);     // 0x524360 (ret 8)
};
struct SimpleTrigger {              // 0x30 bytes: SimpleRec of game.h
    int f0;                         // +0 (the emitter delay, bit-copied)
    TriggerStmt stmt;               // +4
};

// skills.txt record (0x44 bytes, table g_skill_tbl 0x8ee3100)
struct SkillFile {
    tstring f0;                     // 0x00
    tstring f10;                    // 0x10
    tstring f20;                    // 0x20
    char pad30[4];
    int f34;                        // 0x34
    int f38;                        // 0x38
    char pad3c[4];
    int f40;                        // 0x40
};
extern SkillFile g_skill_file[42];  // 0x8ee3100
stream_base& __cdecl read_skill(stream_base& s, SkillFile& r);     // 0x5243c0

// ---- the Data\*.txt record classes (full layouts; records.h's RecS / RecQ / RecT / RecY are the destructor-side views of the
// same records: padded, without constructors, RecS / RecT with the wrong size, RecY without the embedded streams) ---------
struct TroopFile {                  // troops.txt record (0x508 bytes); dtor 0x431c40
    tstring f0;                     // 0x000
    tstring f10;                    // 0x010
    tstring f20;                    // 0x020
    unsigned __int64 u30;           // 0x030
    int f38;                        // 0x038
    int f3c;
    int f40;
    int f44;
    int f48;
    int f4c;
    int f50;
    int f54;
    int f58[4];                     // 0x058
    unsigned f68[6];                // 0x068
    float f80[7];                   // 0x080
    int f9c;                        // 0x09c
    float fa0;                      // 0x0a0
    int fa4;
    int fa8;
    int fac;
    int fb0;
    Pair pairs[0x60];               // 0x0b4 .. 0x3b3
    int f3b4;                       // 0x3b4
    int f3b8;                       // 0x3b8
    unsigned __int64 u3c0[4];       // 0x3c0
    unsigned __int64 u3e0[4];       // 0x3e0
    char pad400[0x20];              // 0x400
    char names_raw[0x80];           // 0x420 tstring[8]
    int f4a0;                       // 0x4a0
    Pair p10[10];                   // 0x4a4
    tstring_vec v;                  // 0x4f4
    TroopFile();                    // 0x524710
    ~TroopFile();                   // 0x431c40
    void read_file(stream_base& s); // 0x524420
};
struct QuestFile {                  // quests.txt record (0x138 bytes); dtor 0x431b10
    tstring f0, f10, f20, f30, f40; // 0x000 .. 0x040
    int f50;                        // 0x050
    tstring f54;                    // 0x054
    int f64;                        // 0x064
    char pad68[0x20];               // 0x068
    char names_raw[0x80];           // 0x088 tstring[8]
    int f108;                       // 0x108
    char pad10c[0xc];
    int f118;                       // 0x118
    int f11c;                       // 0x11c
    char pad120[0xc];
    int f12c;                       // 0x12c
    int f130;                       // 0x130
    int f134;                       // 0x134
    QuestFile();                    // 0x524860
    ~QuestFile();                   // 0x431b10
    void read_file(stream_base& s); // 0x524690
    tstring* default_name();        // 0x48d750
};
struct FactionFile {                // factions.txt record (0x300 bytes); dtor 0x432010
    tstring name;                   // 0x000
    char b10;                       // 0x010
    char pad11[3];
    tstring f14;                    // 0x014
    int f24;                        // 0x024
    unsigned color;                 // 0x028
    float arr[0x80];                // 0x02c .. 0x22b
    int f22c;                       // 0x22c
    int f230;                       // 0x230
    tstring_vec v;                  // 0x234
    float f244;                     // 0x244
    float f248;                     // 0x248
    tstring_vec v2;                 // 0x24c
    char pad25c[0x20];              // 0x25c
    char names_raw[0x80];           // 0x27c tstring[8]
    int f2fc;                       // 0x2fc
    FactionFile();                  // 0x5249f0
    ~FactionFile();                 // 0x432010
    void read_file(stream_base& s, int count);   // 0x524a70
};
// the scene record's embedded passage / site writer stream (vtable slots 3, 7 and 8 of stream_base)
struct V0 { int a, b, c; char pad[0xc]; };                      // 0x18
struct V1 { int p0, p1, p2, p3, p4; char pad[0x20]; };          // 0x34
struct V2 {                                                     // 0x64
    char pad[0x28];
    int a[4];                                                   // +0x28
    int b[4];                                                   // +0x38
    int n;                                                      // +0x48
    char pad2[0x10];
    int c;                                                      // +0x5c
    char pad3[4];
};
#include "../terrain_b/scene_file.h"       // SceneFile (the 0x494-byte scene record; shared with terrain_b/mission.cpp)
// the temporary collection SceneRec::read fills through the embedded stream (0x524f00)
struct Local {
    char pad0[0x1c];
    V0* v0_first; V0* v0_last;                                  // +0x1c / +0x20
    char pad1[8];
    V2* v2_first; V2* v2_last;                                  // +0x2c / +0x30
    char pad2[8];
    V1* v1_first; V1* v1_last;                                  // +0x3c / +0x40
    char pad3[0x94 - 0x44];
    unsigned size0() const { return v0_first == 0 ? 0 : (unsigned)(v0_last - v0_first); }
    unsigned size1() const { return v1_first == 0 ? 0 : (unsigned)(v1_last - v1_first); }
    unsigned size2() const { return v2_first == 0 ? 0 : (unsigned)(v2_last - v2_first); }
    Local();                                                    // 0x4d1360
    ~Local();                                                   // 0x4cb780
    void read(stream_base& s);                                  // 0x524f00
};

// ---- script emitters (0xa75eb0 array, count 0xa75eb4) ------------------------------------------------------------
#include "../emitter_rec.h"
extern int g_emitter_count;                         // 0xa75eb4

struct SkeletonList {                               // skeleton-body table at 0xa180e8
    void load_bodies(tstring* path);                // 0x50fc60 (ret 4)
    void fix();                                     // 0x4cd850
};
extern SkeletonList g_skeletons;                    // 0xa180e8

// trigger vectors at Game +0x139e8 / +0x139f4 (game.h: TriggerTag / SimpleTag, whose padding these resize)
struct TriggerVec {
    int count; void* array; int f8;
    void resize(stream_base* s, const char* name);                  // 0x4c1c90 (ret 8)
};
struct GameVec {
    int count; void* array; int f8;
    int resize(stream_base* s, const char* name, int extra);        // 0x4c1e10 (ret 0xc)
};


// ---- the campaign map's tree builder (0x5213d0) -------------------------------------------------------------------
// Views of the ground graph (CellGrid::core, a NavData): NavVert / NavCell of cell_grid.h with the flat field names this
// function reads, and the third vector (+0x20, 0x24-byte edge records) that NavData declares as ints.
struct MapPoint {                    // 0x24 bytes = NavVert
    float x, y, z;                   // +0x00
    char  pad0c[4];
    float nx, ny, nz;                // +0x10 normal
    char  pad1c[4];
    float s;                         // +0x20 dot with the view direction
};
struct MapNode {                     // 0x88 bytes = NavCell
    int   count;                     // +0x00
    int   idx[8];                    // +0x04 vertex indices
    int   type;                      // +0x24 terrain mask (NavRec::mask)
    char  pad[0x88 - 0x28];
};
struct MapEdge {                     // 0x24 bytes
    char  pad0c[0xc];
    int   a;                         // +0x0c
    int   b;                         // +0x10
    char  pad18[0x24 - 0x18];
};
struct MapNav {                      // NavData
    int      pad0;
    MapPoint* pts;                   // +0x04
    MapPoint* pts_end;               // +0x08
    char     padc[8];
    MapNode* nodes;                  // +0x14
    MapNode* nodes_end;              // +0x18
    char     pad1c[8];
    MapEdge* edges;                  // +0x24
    MapEdge* edges_end;              // +0x28
};
struct RawVec { void** first; void** last; void** end; };      // std::vector<T*> without its allocator word
// The parts of Game that game.h leaves as padding and the build_scene / build_map_trees members that use them.
struct GameMapView {
    char   pad0[0x2c];
    rgl::mesh* map_track;            // +0x2c  0x520d60
    rgl::entity* track_entity;       // +0x30
    char   pad34[4];
    void** ents;                     // +0x38  std::vector<mesh*> (first, last, end; 0x5213d0)
    void** ents_last;                // +0x3c
    void** ents_end;                 // +0x40
    rgl::mesh* map_obj;              // +0x44
    rgl::vector4 fog1;               // +0x48
    rgl::vector4 fog2;               // +0x58
    rgl::vector4 fog3;               // +0x68
    char   pad78[0x139dc - 0x78];
    Wind*  wind;                     // +0x139dc (the fog / sun object; +0x44 = its direction)
    char   pad139e0[0x1a2a8 - 0x139e0];
    rgl::entity* compass;            // +0x1a2a8
    rgl::vector4* fog_query_a(rgl::vector4* out);           // 0x5176e0
    rgl::vector4* fog_query_b(rgl::vector4* out);           // 0x517820
    void add_order_arrow(int id);                           // 0x49bea0
    void del_order(int id);                                 // 0x49bbb0
    void sl_query(int a, int b, int c);                     // 0x435790
    void destroy_order(void* p);                            // 0x4b36a0
};
extern char g_map_pages[];           // 0xa73094 page grid of the map (cell = 0x20 bytes per column, 4 per row)
extern int  g_page_cols;             // 0xa73294
extern int  g_page_rows;             // 0xa73298
extern void* g_grid[64];             // 0xa73094 cell entities, index col*8+row
extern float g_minx;                 // 0xa7329c
extern float g_miny;                 // 0xa732a0
extern float g_maxx;                 // 0xa732a4
extern float g_maxy;                 // 0xa732a8
extern float g_sizex;                // 0xa732ac
extern float g_sizey;                // 0xa732b0
extern RawVec g_mat_vec[4];          // 0xa732c0 / 0xa732d0 / 0xa732e0 / 0xa732f0
extern int    g_mat_count[4];        // 0xa1815c map, 0xa18160 snow, 0xa18164 steppe, 0xa18168 desert
extern int    g_ang_a;               // 0xa18144
extern int    g_ang_b;               // 0xa18150
extern unsigned g_masks[28];         // 0x5a8e68 .. 0x5a8ed0 (14 pairs, indexed [i*2])
extern int    g_env2;                // 0x5e28fc
extern void*  g_resmgr_raw;          // 0x5d6fa0
float    m5_vector4_normalize(rgl::vector4* self);                                   // 0x407d60
void     m5_grid_scan(void* a, void* b, void* c);                                    // 0x523630
void*    m5_tstring_ctor0(void* self);                                               // 0x42bbd0
void*    m5_tstring_ctor1(void* self, const char* s);                                // 0x42bbe0
void     m5_tstring_assign(void* self, const char* s);                               // 0x42bcb0
void     m5_tstring_push_back(void* self, char c);                                   // 0x42be70
void     m5_tstring_dtor(void* self);                                                // 0x42bc40
void*    m5_resmgr_get_object(void* resmgr, void* name);                             // 0x419540
void*    m5_resmgr_get_material(void* resmgr, void* name);                           // 0x418a70
void*    m5_create_entity(int type);                                                 // 0x428440
void*    m5_mesh_ctor(void* p);                                                      // 0x412a90
int      m5_add_point(void* self, void* p);                                          // 0x412f50
int      m5_add_vertex(void* self, int idx, void* uv, void* color, void* pos, void* f5c); // 0x413c50
int      m5_add_rec2(void* self, int a, int b, int c);                               // 0x411920
void     m5_append_skinned(void* self, void* other, const void* m, int bone, unsigned color, char flag); // 0x44eb00
void     m5_noise_fractal(float a, float b, int c, int d, const float* in, float* out); // 0x414fc0
unsigned m5_pack_color(const float* rgba);                                           // 0x40e440
void*    m5_identity3();                                                             // 0x40cd60
void     m5_matrix_rotate_sf(void* m, float* ang);                                   // 0x40e770
int      m5_mb_rand();                                                               // 0x536a9f
void     m5_scene_add_object(void* obj);                                             // 0x419300
void     m5_mesh_set_mat(void* mesh, int type);                                      // 0x523360
void*    m5_material_from_name(int type);                                            // 0x523390
void     m5_collect_near(void* mgr, int idx, int* out, int* count);                  // 0x5234e0
void     m5_vec_insert_n(RawVec* v, void** pos, unsigned n, void** val);             // 0x410610

// ---- 0x523360..0x523630: the nibble setter, the terrain material table, the chain builder, the array scan ---------------
struct FlagNode { char pad[0x1c]; unsigned f1c; };              // +0x1c: bits 24..27 are a 4-bit colour / alpha nibble
struct NibbleObj {
    char pad[0x2c];
    FlagNode* f2c;                                              // +0x2c
    void set_nibble(int v);                                     // 0x523360 (thiscall, ret 4)
};
struct Rec88 { int count; char pad4[0x14 - 4]; int nb[1]; };    // the 0x88-byte graph node (NavCell): neighbour ids at +0x14
struct Node24 { int f00, f04, f08, f0c, f10; char pad14[0x24 - 0x14]; };   // the 0x24-byte edge record
struct ChainGraph {
    char pad0[0x14];
    Rec88* f14;                                                 // +0x14
    char pad18[0x24 - 0x18];
    Node24* f24;                                                // +0x24
    void build_chain(int target, int start, int* out, int* count);   // 0x5234e0 (ret 0x10)
};
struct SceneArrays {
    char pad0[4];
    char* a_first;                                              // +0x04 (elements of 0x24)
    char* a_last;                                               // +0x08
    char padc[0x14 - 0xc];
    char* b_first;                                              // +0x14 (elements of 0x88)
    char* b_last;                                               // +0x18
    int a_size() const { return a_first ? (int)((a_last - a_first) / 0x24) : 0; }
    int b_size() const { return b_first ? (int)((b_last - b_first) / 0x88) : 0; }
};

// ---- thin views for build_scene (0x520d60) -------------------------------------------------------------------------
struct WorldX : rgl::world { void destroy_obj(void* o); };          // 0x41f450 (ret 4)
struct EntityX : rgl::entity { void add_mesh(rgl::mesh* m); };      // 0x442110 (ret 4)
struct PartyMgrView {                                               // Game +0xf8 (game.h Scene)
    int first_party_id();                                           // 0x460d20
    void next_party_id(int* id);                                    // 0x460ca0 (ret 4)
};
struct SpriteNext { void next(int* idx); };                         // 0x517960 (ret 4: SpriteTable at Game +0xcc)
extern CellGrid* g_level;                                           // 0xa732b4 (terrain_b.h declares it too; that header cannot be included next to game.h)
extern rgl::string* g_compass_name;                                 // 0x5e28f4

#endif // MISSION5_GAME_2_H
