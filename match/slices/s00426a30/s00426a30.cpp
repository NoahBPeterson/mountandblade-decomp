// Slice s00426a30: the VC7.1 <algorithm> instantiations behind std::sort(v.begin(), v.end()) for a
// std::vector<unsigned int> (introsort: _Sort / _Unguarded_partition / _Median / heap fallback /
// insertion sort / _Rotate). The iterator is the class std::vector<unsigned>::iterator (VC7.1), which is
// what puts the address-taken iterator temporaries in _Rotate/_Unguarded_partition. The element is a pointer:
// the neighbour s004260a0 sorts its std::vector<Obj*> query result with this (codegen is the same for unsigned and
// pointer elements; the instances here use unsigned). _Adjust_heap at 0x426df0 is the plain-pointer instance.
#include <vector>
#include <algorithm>

typedef unsigned int u32;
typedef std::vector<u32>::iterator It;

// @ 0x426a30
// std::_Sort<It,int> with sort_heap/pop_heap spelled out: in the binary they are inlined into the heap-sort
// tail (only _Pop_heap_0 is called), while cl 13.10 keeps std::sort_heap out of line here.
// std::_Pop_heap_0 (0x426e90) seen from its caller: kept out of line like the original
void pop_heap_0_ext(It first, It last, u32* tag);                   // 0x426e90

void sort_range(It first, It last, int ideal)
{
    int count;
    for (; 32 < (count = last - first) && 0 < ideal; ) {
        std::pair<It, It> mid = std::_Unguarded_partition(first, last);
        ideal /= 2, ideal += ideal / 2;
        if (mid.first - first < last - mid.second)
            sort_range(first, mid.first, ideal), first = mid.second;
        else
            sort_range(mid.second, last, ideal), last = mid.first;
    }
    if (32 < count) {
        std::make_heap(first, last);
        for (; 1 < last - first; --last) {
            if (1 < last - first)
                pop_heap_0_ext(first, last, (u32*)0);
        }
    } else if (1 < count)
        std::_Insertion_sort(first, last);
}
// @ 0x426b20
template std::pair<It, It> std::_Unguarded_partition<It>(It, It);
// @ 0x426c70
template void std::_Median<It>(It, It, It);
// @ 0x426db0
template void std::_Make_heap<It, int, u32>(It, It, int*, u32*);
// @ 0x426df0
template void std::_Adjust_heap<u32*, int, u32>(u32*, int, int, u32);  // the pointer instance is what lives at 0x426df0
// @ 0x426e50
template void std::_Push_heap<It, int, u32>(It, int, int, u32);
// @ 0x426e90
template void std::_Pop_heap_0<It, u32>(It, It, u32*);
// @ 0x426ec0
template void std::_Insertion_sort<It>(It, It);
// @ 0x426f40
template void std::_Rotate<It, int, u32>(It, It, It, int*, u32*);
