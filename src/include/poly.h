// poly.h: Poly, a polymorphic owned object of which only the virtual destructor (vtable slot 0) is ever called:
// "anything deleted through vtable slot 0". Used as the element of std::vector<Poly*> (src/game/entity.h scene cache,
// 0x41f720 / 0x41fa00) and as the owned object of the 0x48-byte OwnedRec48 record (src/rgl/xmlcfg/records.h, vector
// deleting dtor 0x431960). Not one real engine class but one stand-in type; defined once here.
#ifndef MB_POLY_H
#define MB_POLY_H

struct Poly { virtual ~Poly(); };

#endif // MB_POLY_H
