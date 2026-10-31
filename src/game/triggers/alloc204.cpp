// alloc204.cpp: the allocator of trigger_vec's vector of 0x204-byte records, on its own. The original compiled
// Alloc204::construct apart from its callers (uninit_copy_204 0x4c1b60, rec_tables' copy_uninit_204 0x4c3d90): with
// the body visible, cl keeps the copy loop's pointers in edx/eax across the call, registers the original leaves alone.
#include "./trigger_vec.h"

// @ 0x4c1b40
void Alloc204::construct(void* dst, const void* src)
{
    if (dst)
        *(blk204*)dst = *(const blk204*)src;
}
