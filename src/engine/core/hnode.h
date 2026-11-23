// hnode.h: the 12-byte open-list node of the navigation-mesh search (face id, cost so far, priority) and its
// min-heap comparator. ONE definition for src/game: game/agent/agent_stats.cpp (adjust_heap_node 0x438b10, the
// load-order copy constructor below) and game/agent/agent_logic.cpp (find_path 0x444c50, reachable 0x4452f0, which
// push hnode(face, g, f) and pop through PolyMesh::open, game/terrain/poly_mesh.h). The slices of agent2 called the fields a, b,
// key; the ones of agent4 id, g, f (same words). agent4 (find_path / reachable) needs the implicit copy: only
// game/agent/agent_stats.cpp defines HNODE_LOAD_ORDER_COPY.
#ifndef GAME_HNODE_H
#define GAME_HNODE_H
struct hnode {                                           // 12-byte heap node (key float at +8); user-written copy (load order)
    union { int a; int id; };                            // +0 face id
    union { int b; float g; };                           // +4 cost so far
    union { float key; float f; };                       // +8 priority (the heap orders on it)
    hnode() {}
    hnode(int id_, float g_, float f_) { id = id_; g = g_; f = f_; }
#if defined(HNODE_LOAD_ORDER_COPY) && !MB_LP64
    // agent2 (adjust_heap_node 0x438b10) matches only with a user-written copy (the loads in member order). Not in the
    // port (MB_LP64): a user-written copy makes hnode non-trivial, and the Itanium ABI then passes adjust_heap_node's
    // by-value node by address in agent_stats.cpp but in registers in its callers (party2's NavData::find_path crashed).
    hnode(const hnode& o) : a(o.a), b(o.b), key(o.key) {}
    hnode& operator=(const hnode& o) { a = o.a; b = o.b; key = o.key; return *this; }
#endif
};
// the world-map path search's open node (NavData::open, game/party/party_map.cpp find_path 0x4b6b40): the same twelve bytes under
// their own type, so its std::vector instances (insert / push_back 0x4b5bb0, _Insert_n ...) are party_2's and not the ones
// the PolyMesh search (agent4 find_path 0x444c50 / reachable 0x4452f0) gets from vector<hnode>. Binary evidence for the base:
// pop_heap_elem 0x4b2610 hands its nav_node* unadjusted to adjust_heap_node 0x438b10 (the one heap step both searches call).
struct nav_node : hnode {
    nav_node() {}
    nav_node(int id_, float g_, float f_) : hnode(id_, g_, f_) {}
};
struct node_gt { bool operator()(const hnode& a, const hnode& b) const { return a.key > b.key; } };
// the heap steps the open lists share (std::_Adjust_heap and std::_Push_heap for hnode/node_gt): game/agent/agent_stats.cpp defines
// 0x438b10, game/party/party_map.cpp 0x4b2520 (the map-party search's open list: nav_node, the same record under its own type)
void __cdecl adjust_heap_node(hnode* first, int hole, int bottom, hnode val, node_gt pred);     // 0x438b10
void __cdecl push_heap_elem(hnode* first, int hole, int top, hnode val, node_gt pred);          // 0x4b2520
#endif // GAME_HNODE_H
