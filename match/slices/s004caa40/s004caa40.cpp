// Slice s004caa40: one graph method.
//   0x4caa40  graph_obj::combine - take the two picked nodes k and o, split the selected edge into the two
//            nodes of a triangle, merge the picked nodes, rewrite the edge endpoints and each face's vertex
//            lists, drop the merged node and the degenerate edges. Returns true when it changed the graph.
// Offsets are read off the disassembly; the classes are provisional.
#include <windows.h>
#include "rgl.h"

struct node {                                    // 0x18 bytes
    float x, y, z, w;                            // +0x00
    unsigned char used;                          // +0x10
    char pad11[0x7];
};

struct edge {                                    // 0x34 bytes
    int a;                                       // +0x00
    int b;                                       // +0x04
    char pad08[0x8];
    int n;                                       // +0x10
    char pad14[0x20];
};

struct face {                                    // 0x64 bytes
    char pad00[0x28];
    int  verts[8];                               // +0x28 (count at +0x48)
    int  ends[8];                                // +0x38 (count at +0x58)
    int  other[2];                               // +0x58 (its first word doubles as the terminator)
};

struct graph_obj {
    bool combine(int k, int o);                  // 0x4caa40
    char pad0[0x1c];
    node* nodes;                                 // +0x1c
    node* nodes_end;                             // +0x20
    char pad24[0x8];
    face* faces;                                 // +0x2c
    face* faces_end;                             // +0x30
    char pad34[0x8];
    edge* edges;                                 // +0x3c
    edge* edges_end;                             // +0x40
};

void g_split(graph_obj* g, int a, int b);        // 0x4c57d0
void g_split2(graph_obj* g, int a, int b);       // 0x4c58c0
void g_changed(graph_obj* g, int last, int i);   // 0x4c5840
void g_remove_edge(graph_obj* g, int i);         // 0x4ca9b0
void g_remove_node(graph_obj* g, int i);         // 0x4c8af0
void g_finish(graph_obj* g);                     // 0x470920

#define NODES(g) ((g)->nodes ? (int)((g)->nodes_end - (g)->nodes) : 0)
#define FACES(g) ((g)->faces ? (int)((g)->faces_end - (g)->faces) : 0)
#define EDGES(g) ((g)->edges ? (int)((g)->edges_end - (g)->edges) : 0)

// @ 0x4caa40
bool graph_obj::combine(int k, int o)
{
    int count = 0;
    int picked[2];
    int sel = -1;
    bool eo = false;
    bool ek = false;
    bool twice = false;

    if (edges) {
        for (int i = 0; i < EDGES(this); i++) {
            if (edges[i].n) {
                if (count < 2) {
                    picked[count] = i;
                    o = edges[i].b;
                }
                count++;
            }
        }
    } else {
        for (int i = 0; i < NODES(this); i++) {
            if (nodes[i].used) {
                if (count < 2)
                    picked[count] = i;
                count++;
            }
        }
    }
    if (count != 2)
        return false;

    bool other_k = picked[0] == k;
    bool other_o = o == k;
    if (!other_k)
        other_o = o == o;

    for (int i = 0; i < EDGES(this); i++) {
        int hits = 0;
        if (edges[i].a == k || edges[i].b == k) {
            hits = 1;
            if (edges[i].n == 1)
                ek = true;
        }
        if (edges[i].a == o || edges[i].b == o) {
            if (edges[i].n == 1)
                eo = true;
            hits++;
        }
        if (hits == 2) {
            twice = true;
            sel = i;
        }
    }

    if (twice) {
        g_split(this, o, k);
        g_split2(this, o, k);
        g_changed(this, sel, NODES(this) - 1);

        node f = *(node*)&faces[k];
        nodes[k] = nodes[NODES(this) - 1];
        *(node*)&faces[NODES(this) - 1] = f;

        edge t = edges[sel];
        edges[sel] = edges[EDGES(this) - 1];
        edges[EDGES(this) - 1] = t;

        k = NODES(this) - 1;
        int last = EDGES(this) - 1;
        if (!other_o) {
            if (!other_k) {
                nodes[o].x = (nodes[o].x + nodes[k].x) * 0.5f;
                nodes[o].y = (nodes[o].y + nodes[k].y) * 0.5f;
                nodes[o].z = (nodes[o].z + nodes[k].z) * 0.5f;
            }
        }
        for (int i = 0; i < EDGES(this); i++) {
            if (edges[i].a == k) {
                if (i != last)
                    edges[i].a = o;
            } else if (edges[i].b == k && i != last) {
                edges[i].b = o;
            }
        }
        for (int i = 0; i < FACES(this); i++) {
            for (int j = 0; j < faces[i].verts[8]; j++) {
                if (faces[i].verts[j] == k)
                    faces[i].verts[j] = o;
            }
            for (j = 0; j < faces[i].ends[8]; j++) {
                if (faces[i].ends[j] == sel) {
                    if (faces[i].ends[8] == 4) {
                        for (int m = j + 1; m < faces[i].ends[8]; m++) {
                            faces[i].ends[m - 1] = faces[i].ends[m];
                            faces[i].verts[m + 2] = faces[i].verts[m + 3];
                        }
                        faces[i].ends[8]--;
                    }
                    break;
                }
            }
        }
        g_remove_node(this, k);
    } else {
        if (!ek || !eo)
            return false;

        g_split(this, o, k);
        g_split2(this, o, k);

        node f = *(node*)&faces[k];
        nodes[k] = nodes[NODES(this) - 1];
        *(node*)&faces[NODES(this) - 1] = f;

        k = NODES(this) - 1;
        if (!other_o) {
            if (!other_k) {
                nodes[o].x = (nodes[o].x + nodes[k].x) * 0.5f;
                nodes[o].y = (nodes[o].y + nodes[k].y) * 0.5f;
                nodes[o].z = (nodes[o].z + nodes[k].z) * 0.5f;
            }
        }
        for (int i = 0; i < EDGES(this); i++) {
            if (edges[i].a == k)
                edges[i].a = o;
            else if (edges[i].b == k)
                edges[i].b = o;
        }
        for (int i = 0; i < FACES(this); i++) {
            for (int j = 0; j < faces[i].verts[8]; j++) {
                if (faces[i].verts[j] == k)
                    faces[i].verts[j] = o;
            }
        }
        g_remove_node(this, k);
    }

    for (;;) {
        int i = 0;
        for (; i < EDGES(this); i++) {
            if (edges[i].a == o || edges[i].b == o) {
                for (int j = i + 1; j < EDGES(this); j++) {
                    int c = edges[i].a;
                    if (c == edges[j].a || c == edges[j].b) {
                        if (edges[i].b == edges[j].a || edges[i].b == edges[j].b)
                            break;
                    }
                }
                if (j < EDGES(this)) {
                    g_remove_edge(this, j);
                    break;
                }
            }
        }
        if (i >= EDGES(this))
            break;
    }
    g_finish(this);
    return true;
}
