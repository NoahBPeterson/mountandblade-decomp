// Slice s00443aa0: Agent::update_equipment_display (0x443aa0, 2243 bytes). Re-evaluates what the agent carries:
// clears the visibility mask of every slot's mesh objects, then for the (up to two) wielded items shows their
// meshes (with the javelin flip), then attaches the carried-but-not-wielded items to their carry bones, then
// hides the weapon meshes of the other slots in the player's first-person mode, and finally refreshes the quiver
// ammo display. All names provisional.
#include <vector>
#include "rgl.h"
using rgl::vector4;

// ---- item records (array at 0x5e29d0, 0x238 bytes each; see s00433e40 / s0043a280) -------------------------------
struct pos2 : rgl::matrix {};
struct item_kind {
    char pad0[0x1c8];
    union { unsigned type_flags; unsigned char type; };      // 0x1c8: low byte = itp_type_*
    char pad1cc[4];
    __int64 capability_flags;                                // 0x1d0 itcf_*
    char pad1d8[0x238 - 0x1d8];
    void get_carry_pos(pos2* out, int* bone_flags, int* bone);   // 0x433e40
};
extern item_kind* g_items;                                   // 0x5e29d0

#define itp_type_horse           1
#define itp_type_one_handed_wpn  2
#define itp_type_two_handed_wpn  3
#define itp_type_polearm         4
#define itp_type_shield          7
#define itp_type_bow             8
#define itp_type_crossbow        9
#define itp_type_thrown          10
#define itp_type_pistol          0x10
#define itp_type_musket          0x11

#define itcf_throw_javelin                0x40000i64
#define itcf_shoot_mask                   0xff000i64
#define itcf_carry_mask                   0x7f0000000i64
#define itcf_show_holster_when_drawn      0x800000000i64

// ---- mesh objects hanging off the agent (provisional) ------------------------------------------------------------
struct node_list {                                           // see s00438860
    int al; void** first; void** last; void** end;
    void cap_all(const float* limit);                        // 0x438f90
};
struct Mesh {
    char pad0[0x34];
    node_list lists[5];                                      // +0x34
    int nlists;                                              // +0x84
    char pad88[4];
    pos2 frame;                                              // +0x8c
    unsigned vis;                                            // +0xcc visibility mask (0xffff shown, 0 hidden)
};
struct Region {                                              // 0x1dc bytes (see s00439a50)
    char pad0[0x1ac];
    int count;                                               // +0x1ac
    std::vector<int> meshes;                                 // +0x1b0
    char pad1c0[0x1dc - 0x1c0];
};
struct Group {
    int count;
    Region* regions;                                         // +4
    void remove_shape(Mesh* m);                              // 0x439ec0
};
struct AgentBody {
    char pad0[0x304];
    Group* skel;                                             // +0x304
};

struct mat3 { vector4 s, f, u; };
mat3* __cdecl identity3(mat3* out);                          // 0x40cd60

struct Mission { char pad[0x5c]; int player_id; };           // +0x5c
extern Mission* g_mission;                                   // 0x5e292c
struct GameData { char pad[0x139e4]; int f139e4; };
extern GameData* g_game;                                     // 0x5e2930

struct ItemRef { int item; unsigned m4; };
struct Agent {
    char pad0[8];
    int id;                                                  // +8
    char padc[0x14c - 0xc];
    Mesh* A[4];                                              // +0x14c
    Mesh* B[4];                                              // +0x15c
    Mesh* C[4];                                              // +0x16c (ammo/holster display)
    char pad17c[0x1d0 - 0x17c];
    ItemRef slots[8];                                        // +0x1d0
    char pad210[0x22c - 0x210];
    int cur[2];                                              // +0x22c
    char pad234[0x23c - 0x234];
    int f23c;                                                // +0x23c
    char pad240[0x250 - 0x240];
    AgentBody* body;                                         // +0x250
    char pad254[0x5928 - 0x254];

    void update_quiver();                                    // 0x43a800
    void update_equipment_display();                         // 0x443aa0
};

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

// shows `m` on the carry bone region `r` with local frame `f`
static __forceinline void attach(Agent* a, Mesh* m, int r, const pos2& f)
{
    Group* g = a->body->skel;
    g->remove_shape(m);
    m->frame = f;
    Region& reg = g->regions[r];
    vpush(reg.meshes, (int)m);
    reg.count++;
}

// @ 0x443aa0
void Agent::update_equipment_display()
{
    char slot_used[4];
    char bone_used[11] = {0};
    int i;
    for (i = 3; i >= 0; i--) {
        if (slots[i].item >= 0) {
            item_kind* ik = &g_items[slots[i].item];
            if (A[i] != 0) {
                A[i]->vis = 0;
                if (ik->type == itp_type_shield) {
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
        int s = cur[j];
        if (s >= 0 && slots[s].item >= 0 && (j == 0 || f23c == 0)) {
            item_kind* ik = &g_items[slots[cur[j]].item];
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
            if ((g_items[slots[s].item].capability_flags & itcf_shoot_mask) == itcf_throw_javelin) {
                M.f = vector4(-M.f.x, -M.f.y, -M.f.z);
                M.s = vector4(-M.s.x, -M.s.y, -M.s.z);
            }
            if (A[cur[j]] != 0) {
                A[cur[j]]->vis = 0xffff;
                Mesh* m = A[cur[j]];
                float limit = 0.0f;
                int k = 0;
                if (0 < m->nlists) {
                    do {
                        m->lists[k].cap_all(&limit);
                        k++;
                    } while (k < m->nlists);
                }
                attach(this, A[cur[j]], region, M);
            }
            if (B[cur[j]] != 0) {
                B[cur[j]]->vis = 0xffff;
                attach(this, B[cur[j]], region, M);
            }
            if ((ik->capability_flags & itcf_carry_mask) != 0 && (ik->capability_flags & itcf_show_holster_when_drawn) == 0)
                slot_used[cur[j]] = 1;
            Mesh* h = C[cur[j]];
            if (h != 0) {
                if ((ik->capability_flags & itcf_show_holster_when_drawn) == 0) {
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
        if (slots[i].item >= 0) {
            item_kind* ik = &g_items[slots[i].item];
            if ((ik->capability_flags & itcf_carry_mask) != 0) {
                pos2 M;
                int region, bone;
                ik->get_carry_pos(&M, &region, &bone);
                if (!bone_used[bone] && !slot_used[i] && (ik->type != itp_type_shield || (slots[i].m4 & 0xffff) > 10)) {
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
            if (slots[j].item >= 0) {
                item_kind* ik = &g_items[slots[j].item];
                if ((ik->capability_flags & itcf_carry_mask) != 0) {
                    pos2 M;
                    int bone, region;
                    ik->get_carry_pos(&M, &bone, &region);
                    if (bone == 9 || bone == 7 || bone == 8) {
                        if (cur[0] != j && (cur[1] != j || f23c != 0)) {
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
