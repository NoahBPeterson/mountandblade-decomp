// Slice s00474410: VC7.1 std::stable_sort internals for 8-byte records (_Buffered_merge, _Chunked_merge back into the vector).
// Same record/predicate types as s004724f0 / s0046bc70 (P8 / E8, function-pointer predicate). Names provisional.
#include <vector>
#include <algorithm>

struct P8 { int key, val; };
typedef bool (__cdecl *Pred8)(const P8&, const P8&);
typedef std::vector<P8>::iterator It8;

// @ 0x474410 (_Buffered_merge: merge two sorted runs using the temp buffer)
template void std::_Buffered_merge(It8, It8, It8, int, int, std::_Temp_iterator<P8>&, Pred8);
// @ 0x474650 (_Chunked_merge: merge chunk pairs from the temp buffer back into the vector)
template void std::_Chunked_merge(P8*, P8*, It8, int, int, Pred8);
