// nav_graph.h: the node graph the new-game sequence builds on the strategic map (the object behind the global
// 0xa732b8) and the helpers that score and search it (game.cpp, 0x51d3a0..0x51db70). The slices put these members
// under `Game`; the calls show another `this` (0x51d2b0 loads ECX from 0xa732b8 before 0x51dac0 and 0x51d3a0, and
// every access is to +0x14/+0x18, a vector of 0x88-byte nodes). Names provisional.
#ifndef GAME_PARTY_NAV_GRAPH_H
#define GAME_PARTY_NAV_GRAPH_H

struct NavNode {                            // 0x88 bytes
    int count;                              // +0x00  number of neighbours (at most 4)           0x51d3a0, 0x51d7c0
    char pad04[0x24 - 0x04];
    unsigned mask;                          // +0x24  class mask, compared with an argument      0x51d3a0, 0x51db70
    int nb[4];                              // +0x28  neighbour node ids (-1 = none)             0x51d7c0, 0x51dac0
    float weight[4];                        // +0x38  edge weights (1e9 = blocked)               0x51d3a0, 0x51dac0
    int cost;                               // +0x48  component label (-1 unlabelled)            0x51d3a0
    int prev;                               // +0x4c                                             0x51d7c0
    int next;                               // +0x50                                             0x51d7c0
    char pad54[0x88 - 0x54];
};

struct NavGraph {
    char pad0[0x14];
    NavNode* nodes;                         // +0x14  vector of nodes (null while empty)          0x51d3a0, 0x51dac0
    NavNode* nodes_end;                     // +0x18                                              0x51dac0

    void solve(int* arg1, int* arg2, int* arg3);                // 0x51d3a0 (ret 0xc)
    int* walk(int* a, int* idx, int* seen, int* n);             // 0x51d7c0 (ret 0x10)
    int find_trigger(int* idp, int* a2, int* arr, int* cnt, unsigned* m1, unsigned* m2, int* flag);   // 0x51d920 (ret 0x1c)
    void score_triggers(int* a, int* b, int* c);                // 0x51dac0 (ret 0xc)
    float trigger_score(int row, int idx, unsigned a, unsigned b, unsigned c);                         // 0x51db70 (ret 0x14)
};
extern NavGraph* g_nav;                     // 0xa732b8 (the map object; also +0x44/+0x54 are handed to the scene constructor)

#endif // GAME_PARTY_NAV_GRAPH_H
