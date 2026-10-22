// Slice s004759c0 (second TU): VC7.1 <vector> instantiations for three Mission record vectors (0x24, 0x88 and
// 0x1c8 bytes). Built with /EHa: the stock reserve() body's try/catch then keeps the EH frame and the
// catch(...) funclet the original has (with /EHsc cl shrinks the frame; with no EH switch it assumes _Xlen
// returns). insert() and _Buy() are the stock members; they call the out-of-line _Insert_n instantiations
// 0x424830 / 0x424db0 from s00424830.
#include <vector>
#include <stdexcept>

struct rec36  { int v[9]; };        // 0x24
struct rec136 { int v[34]; };       // 0x88
struct rec456 { int v[114]; };      // 0x1c8

// 0x476020 vector<rec36>::reserve, 0x476130 vector<rec136>::reserve (their catch(...) funclets follow at
// 0x476111 / 0x476225), 0x476240 vector<rec36>::insert, 0x4762b0 vector<rec136>::insert,
// 0x476320 vector<rec456>::_Buy.
template class std::vector<rec36>;
template class std::vector<rec136>;
template class std::vector<rec456>;
