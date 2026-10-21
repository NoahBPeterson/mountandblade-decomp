// Slice s00417850, part 2: the std::list<T*> node allocator (3-argument _Buynode), which needs an EH frame
// (built with /EHa, like s0040f9a0; with /EHsc cl drops the try region because nothing in it can throw).
// The 21-byte block at 0x4180b9 is the catch(...) funclet of 0x418050 (operator delete + rethrow); cl emits it
// after the parent in the same COMDAT with no symbol of its own.
#include <new>

struct res_obj;                      // list element type unknown (a 4-byte pointer-sized value)

// std::list<res_obj*> node: the VC7.1 _Node ctor is effectively nothrow here, so the placement new adds no
// nested EH state (a plain throwing ctor gives an extra try level the original lacks).
struct list_node {
    list_node* next;
    list_node* prev;
    res_obj* val;
    list_node(list_node* n, list_node* p, res_obj* const& v) throw() : next(n), prev(p), val(v) {}
};

struct list_res {
    list_node* _Buynode(list_node* next, list_node* prev, res_obj* const& val);       // 0x418050
};

// @ 0x418050
list_node* list_res::_Buynode(list_node* next, list_node* prev, res_obj* const& val)
{
    list_node* p = (list_node*)operator new(sizeof(list_node));
    try {
        new ((void*)p) list_node(next, prev, val);
    } catch (...) {
        operator delete(p);
        throw;
    }
    return p;
}
