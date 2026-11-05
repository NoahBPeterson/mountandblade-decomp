// list_int.h: the std::list<int>-shaped list the glyph table (Font::map, game/font.h) keeps: a self-linked sentinel node and a count.
// ONE definition for src/rgl/mesh2/gui_edit.cpp (clear 0x4c4d80, _Buynode 0x4c4dc0, the node maker 0x4c4e50) and
// src/game/cls_map/cls_map.cpp (the font's inlined destructor clears it).
#ifndef MB_LIST_INT_H
#define MB_LIST_INT_H
struct list_node_int {
    list_node_int* next;
    list_node_int* prev;
    int value;
};

struct list_int {
    list_node_int* head;   // +0
    list_node_int* w04;    // +4  sentinel (self-linked when empty)
    int            count;  // +8
    void clear();          // 0x4c4d80
};
#endif // MB_LIST_INT_H
