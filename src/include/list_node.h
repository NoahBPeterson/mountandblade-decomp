// list_node.h: the VC7.1 std::list layout (allocator word, head node, size) of the hash table's node list
// (game/options_panel/world_app.cpp HashTab::pool) and of the list whose size check 0x4cb6f0 (std::list::_Incsize,
// rgl/mesh2/mesh_2.cpp) every such list calls. ONE definition for both TUs.
#ifndef MB_LIST_NODE_H
#define MB_LIST_NODE_H
#include <stddef.h>

// a node of the hash table's list of unsigned keys
struct HNode {
    HNode* next;                                    // +0
    HNode* prev;                                    // +4
    unsigned key;                                   // +8
};

struct list_node {
    int    alloc;                                   // +0 the allocator word
    HNode* head;                                    // +4 the sentinel node
    size_t size;                                    // +8
    void   grow(unsigned n);                        // 0x4cb6f0 (throws length_error past 0x7ffffff)
};
#endif // MB_LIST_NODE_H
