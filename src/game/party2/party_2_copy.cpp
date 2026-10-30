// party_2_copy.cpp: 0x4b2160, the E92 record copy of the party_2 object's ElemCopier.
// It is a thiscall member in the original (ret 8, the ecx context unused); it lives in its own translation unit so that the
// callers in party_2.cpp keep treating it as an opaque thiscall (cl would otherwise give them the callee's register knowledge).
#include "./party_2.h"

// @ 0x4b2160
void ElemCopier::copy_event(void* dst, void* src)
{
    if (dst) {
        E92 tmp = *(E92*)src;
        *(E92*)dst = tmp;
    }
}
