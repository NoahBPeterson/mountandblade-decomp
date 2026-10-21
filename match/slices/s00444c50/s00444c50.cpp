// Slice s00444c50: NavMesh::find_path, the A*-style face search used by the agents' path planner. Expands faces from an
// open list (std::priority_queue of 12-byte nodes, min-heap on f = (dist-to-goal / dist-from-start + 1) * cost), weighing
// each portal by its clearance (narrow portals cost more, below the caller's width even more), then walks the parent
// links back from the goal, fills the face list (start first, at most 0x400 entries) and the portal midpoints.
// Names provisional; the std heap helpers it calls (_Pop_heap, _Push_heap, push_heap, vector::push_back) live elsewhere.
#include "nav_search.h"

// ---- A*-style path search: cheapest face chain from `from` to `to`; fills the face list and the portal midpoints ---------
// @ 0x444c50
bool NavMesh::find_path(int from, int to, int* out_faces, vector4* out_pos, int* out_count, int* out_steps,
                        const float* w0, const float* w1, const float* w2)
{
    int chain[0x2004];
    while (!open.empty())
        open.pop_inl();
    next_stamp();
    faces[from].stamp = stamp + 1;
    faces[from].parent = from;
    hnode n;
    n.g = 0.0f;
    n.f = 0.0f;
    n.id = from;
    open.push(n);
    *out_steps = 0;
    float maxg = 0.0f;
    while (faces[to].stamp <= stamp) {
        if (open.empty() || !(n.f < 1e+06f))
            break;
        (*out_steps)++;
        n = open.top();
        open.pop_inl();
        int cur = n.id;
        for (int k = 0; k < faces[cur].nverts; k++) {
            int nb = faces[cur].nbr_[k];
            if (nb != -1 && faces[nb].stamp <= stamp) {
                float cost = 1.0f;
                faces[nb].stamp = stamp + 1;
                faces[nb].parent = cur;
                NavEdge* e = &edges[faces[cur].edge_[k]];
                float w = e->face == nb ? e->w_a : e->w_b;
                if (w < 0.65f)
                    cost = (0.65f - w) * 20.0f + 1.0f;
                if (w <= *w0)
                    cost = (1.0f - w) * 50.0f + cost;
                vector4 dg = vsub(faces[to].c, faces[nb].c);
                vector4 ds = vsub(faces[from].c, faces[nb].c);
                float ls = ds.length();
                float lg = dg.length();
                cost = cost + n.g;
                if (n.g > maxg)
                    maxg = n.g;
                open.push(hnode(nb, cost, (lg / ls + 1.0f) * cost));
            }
        }
        faces[cur].stamp = stamp + 2;
    }
    if (faces[to].stamp <= stamp) {
        *out_count = 0;
        return false;
    }
    int len = 0;
    int f = to;
    while (f != from && len < 0x2000) {
        chain[len++] = f;
        f = faces[f].parent;
    }
    chain[len++] = from;
    *out_count = len < 0x400 ? len : 0x400;
    int prev = from;
    for (int i = 0; i < *out_count; i++) {
        int cur = chain[len - 1 - i];
        out_faces[i] = cur;
        int k = 0;
        for (int j = 0; j < faces[prev].nverts; j++) {
            if (faces[prev].nbr_[j] == cur) {
                k = j;
                break;
            }
        }
        out_pos[i] = edges[faces[prev].edge_[k]].mid;
        prev = out_faces[i];
    }
    for (int i = 1; i < *out_count; i++)
        out_pos[i - 1] = out_pos[i];
    out_pos[*out_count - 1] = faces[out_faces[*out_count - 1]].c;
    return true;
}

