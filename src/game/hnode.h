// hnode.h: the 12-byte open-list node of the navigation-mesh search (face id, cost so far, priority) and its
// min-heap comparator. ONE definition for src/game: agent2/agent_stats.cpp (adjust_heap_node 0x438b10, the
// load-order copy constructor below) and agent4/agent_think.cpp (find_path 0x444c50, reachable 0x4452f0, which
// push hnode(face, g, f) and pop through NavMesh::open, nav_mesh.h). The slices of agent2 called the fields a, b,
// key; the ones of agent4 id, g, f (same words). agent4 (find_path / reachable) needs the implicit copy: only
// agent2/agent_types.h defines HNODE_LOAD_ORDER_COPY.
#ifndef GAME_HNODE_H
#define GAME_HNODE_H
struct hnode {                                           // 12-byte heap node (key float at +8); user-written copy (load order)
    union { int a; int id; };                            // +0 face id
    union { int b; float g; };                           // +4 cost so far
    union { float key; float f; };                       // +8 priority (the heap orders on it)
    hnode() {}
    hnode(int id_, float g_, float f_) { id = id_; g = g_; f = f_; }
#ifdef HNODE_LOAD_ORDER_COPY
    // agent2 (adjust_heap_node 0x438b10) matches only with a user-written copy (the loads in member order)
    hnode(const hnode& o) : a(o.a), b(o.b), key(o.key) {}
    hnode& operator=(const hnode& o) { a = o.a; b = o.b; key = o.key; return *this; }
#endif
};
struct node_gt { bool operator()(const hnode& a, const hnode& b) const { return a.key > b.key; } };
#endif // GAME_HNODE_H
