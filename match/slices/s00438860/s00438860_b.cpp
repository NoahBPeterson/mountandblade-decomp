// Slice s00438860, part 2: std::remove_copy over vector<int>::iterator (0x438910), compiled apart from the rest of the
// slice because <vector> includes <new> (see s00438860.cpp).
#include <vector>
#include <algorithm>

// @ 0x438910
template std::vector<int>::iterator std::remove_copy(std::vector<int>::iterator, std::vector<int>::iterator,
                                                     std::vector<int>::iterator, const int&);
