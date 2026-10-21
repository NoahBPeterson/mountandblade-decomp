// Slice s00419a30 (list part): std::list<res_base*>::_Incsize (the "list<T> too long" check). Built WITHOUT
// /EH: the original has no EH frame around the inlined length_error construction.
#include <list>

struct res_base;
template class std::list<res_base*>;
