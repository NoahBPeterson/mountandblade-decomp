// Slice s004cbcd0: out-of-line std::vector<T> members for a 0x2c-byte record (E44), a 0x204-byte record
// (E516) and a 4-byte scalar (E4): _Insert_n / copy ctor, plus their catch funclets (which the original
// emits inline, after the main body's ret, so they are separate VAs) and a trivial-range helper.
// The vectors are VC7.1's <vector> instantiated explicitly; the STL's explicit try/catch keeps its frame
// and funclets while a trivial element leaves the catch path as free(_Newvec) + throw, as in the original.
#include <vector>
#include <memory>

struct E44  { int v[11]; ~E44() {} }; // 0x2c-byte record (empty, user-declared dtor: the original's
                                      // _Destroy loop survives, the call inlines away)
struct E516 { int v[0x81]; };        // 0x204-byte record
struct scalar4 { int v; };           // 4-byte scalar element (unique name: other slices call theirs E4)

template class std::vector<E44>;
template class std::vector<E516>;
template class std::vector<scalar4>;

// _Nonscalar_ptr_iterator_tag: an empty tag that cl 13.10 still passes as one 4-byte stack slot
// (which is where the last `mov eax, [esp+8]` in 0x4cc260 comes from).
struct nonscalar_tag {};

// 0x4ca7e0 (_Destroy for a 0xb0-byte record: frees +0x20, then destroys the string at +0)
void __cdecl destroy_110(void* first, void* last, void* v, nonscalar_tag tag);   // 0x4ca7e0
// 0x4c3e80 (trivial copy, element 0x2c) is reached through the instantiated _Ucopy.

// @ 0x4cc260
void __fastcall erase_110(void* self, int, void* first, void* last)
{
    nonscalar_tag tag;
    destroy_110(first, last, self, tag);
}
