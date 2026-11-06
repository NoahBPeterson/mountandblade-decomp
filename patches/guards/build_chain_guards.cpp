// patches/guards/build_chain_guards.cpp: bounds guards on ChainGraph::build_chain (patch set "guards", linked by default).
//
// What it changes. Two checks the original doesn't have, so the faithful function in src/ can byte-match:
//   cur < 0  stop the walk: the record at f14 - 0x88 faulted (0x49d47a), since the neighbour lists can carry -1
//   v < 0    skip the neighbour: the node at f24 - 0x24 faulted with v = -1
// Changes behaviour on purpose, only where the original reads out of bounds. On every valid input it does what the
// faithful version does.
//
// Open question: whether the original can reach those reads at all, or whether -1 only arrived because of a bug of ours
// that has since been fixed. Running a --faithful build through a world map answers it; if it never faults, this patch
// can go.
#include "../../src/game/mission5/game_2.h"

// @ replaces 0x5234e0
void ChainGraph::build_chain(int target, int start, int* out, int* count)
{
    int cur = start;
    int prev = -1;
    int next;
    *count = 0;
    do {
        if (cur < 0)
            return;
        Rec88* rec = (Rec88*)((char*)this->f14 + cur * 0x88);
        int n = rec->count;
        int found = -1;
        if (n <= 0)
            return;
        int* nb = (int*)((char*)rec + 0x14);
        do {
            int v = *nb;
            if (v < 0) {
                nb++;
                continue;
            }
            Node24* node = (Node24*)((char*)this->f24 + v * 0x24);
            if (node->f00 > 1 && node->f04 != prev && node->f08 != prev
                && (node->f0c == target || node->f10 == target))
                found = v;
            nb++;
        } while (--n);
        if (found < 0)
            return;
        Node24* node = (Node24*)((char*)this->f24 + found * 0x24);
        next = (node->f04 == cur) ? node->f08 : node->f04;
        // (no early-out on next < 0: the original appends first; the loop-top cur < 0 guard stops the walk)
        int c = *count;
        bool dup = false;
        if (node->f0c == target) {
            for (int i = 0; i < c; i++)
                if (out[i] == node->f10)
                    dup = true;
            if (!dup) {
                out[c] = node->f10;
                (*count)++;
            }
        } else {
            for (int i = 0; i < *count; i++)
                if (out[i] == node->f0c)
                    dup = true;
            if (!dup) {
                out[c] = node->f0c;
                (*count)++;
            }
        }
        prev = cur;
        cur = next;
    } while (next != start);
}
