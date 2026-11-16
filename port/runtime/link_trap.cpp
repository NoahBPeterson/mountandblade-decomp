// port/runtime/link_trap.cpp: what port/link/traps.cpp's weak placeholders call (ticket 004 milestone 2). Each placeholder
// stands for a name src/ calls that has no definition for this ABI yet (port/link/tables.json "trapped"); reaching
// one is a port bug to fix, never something to continue past.
#include <stdio.h>
#include <stdlib.h>

extern "C" __attribute__((noreturn, used)) void port_link_trap(const char* name)
{
    fprintf(stderr, "mb: reached an unresolved link name (port/link/traps.cpp): %s\n", name);
    fflush(stderr);
    abort();
}
