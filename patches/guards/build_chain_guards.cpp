// patches/guards/build_chain_guards.cpp: bounds guards on NavData::build_chain (patch set "guards", linked by default).
//
// What it changes. Two checks the original doesn't have, so the faithful function in src/ can byte-match:
//   cur < 0  stop the walk: the cell before cells.first faulted (0x49d47a), since the neighbour lists can carry -1
//   v < 0    skip the neighbour: the edge before edges.first faulted with v = -1
// Changes behaviour on purpose, only where the original reads out of bounds. On every valid input it does what the
// faithful version does.
//
// Open question: whether the original can reach those reads at all, or whether -1 only arrived because of a bug of ours
// that has since been fixed. Running a --faithful build through a world map answers it; if it never faults, this patch
// can go.
#include "../../src/game/campaign/map_builder_and_data_load.h"

// @ replaces 0x5234e0
void NavData::build_chain(int target, int start, int* out, int* count)
{
    int cur = start;
    int prev = -1;
    int next;
    *count = 0;
    do {
        if (cur < 0)
            return;
        NavCell& cell = cells[cur];
        int n = cell.npts;
        int found = -1;
        if (n <= 0)
            return;
        int* nb = cell.edge_ids();
        do {
            int v = *nb;
            if (v < 0) {
                nb++;
                continue;
            }
            NavEdge& e = edges[v];
            if (e.count > 1 && e.cell != prev && e.other != prev
                && (e.a == target || e.b == target))
                found = v;
            nb++;
        } while (--n);
        if (found < 0)
            return;
        NavEdge& e = edges[found];
        next = (e.cell == cur) ? e.other : e.cell;
        // (no early-out on next < 0: the original appends first; the loop-top cur < 0 guard stops the walk)
        int c = *count;
        bool dup = false;
        if (e.a == target) {
            for (int i = 0; i < c; i++)
                if (out[i] == e.b)
                    dup = true;
            if (!dup) {
                out[c] = e.b;
                (*count)++;
            }
        } else {
            for (int i = 0; i < *count; i++)
                if (out[i] == e.a)
                    dup = true;
            if (!dup) {
                out[c] = e.a;
                (*count)++;
            }
        }
        prev = cur;
        cur = next;
    } while (next != start);
}
