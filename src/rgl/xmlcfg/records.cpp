//-----------------------------------------------------------------------------
// File: records.cpp  (0x431400..0x4320af)
//
// Desc: compiler-generated scalar/vector deleting destructors and destructor bodies of engine record classes
//       (each is a COMDAT the linker placed here), the std::vector<Item> helpers they use, and the reset of
//       a big per-mission object. Functions in ascending address order (cl's bottom-up emission).
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include <new>      // declares operator delete[]: without it cl 13.10 frees arrays with plain operator delete
#include "records.h"
#include "../../include/poly.h"      // (named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/iface.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/rec_dtors.h"      // (listed directly: the resolution audit scans only the source's own includes)

template class std::vector<Item>;       // 0x431a30 _Destroy, 0x431a60 _Tidy

// @ 0x431400
IfaceA::IfaceA() {}

// @ 0x431440
RecD::~RecD() {}
// @ 0x431480
void delete_recd_array(RecD* p) { delete[] p; }
// @ 0x431540
void delete_recg_array(RecG* p) { delete[] p; }
// @ 0x4315d0
void delete_rech_array(RecH* p) { delete[] p; }
// @ 0x431660
RecI::~RecI() {}
// @ 0x4316b0
NameHolder::~NameHolder() {}

// The constructors only make cl emit the class's vtable and with it the vector deleting destructors
// (slot 0 of the real vtables).
RecF::RecF() {}
RecF* make_recf_array(int n) { return new RecF[n]; }     // makes cl emit ??_E (needs a `new[]`)
// @ 0x4316c0
void delete_recf_array(RecF* p) { delete[] p; }

RecJ::RecJ() {}
RecJ* make_recj_array(int n) { return new RecJ[n]; }
// @ 0x431750
void delete_recj_array(RecJ* p) { delete[] p; }

// @ 0x4317b0
void RecGList::clear()
{
    if (items) {
        delete[] items;
        items = 0;
    }
}
// @ 0x431810
void delete_reci_array(RecI* p) { delete[] p; }
// @ 0x4318c0
void delete_reck_array(RecK* p) { delete[] p; }

// @ 0x431960
void delete_reca_array(OwnedRec48* p) { delete[] p; }
// @ 0x431ab0
RecP::~RecP() {}
// @ 0x431b10
RecQ::~RecQ() {}

RecR::RecR() {}
// 0x431b90 RecR::~RecR (implicit)
RecR* make_recr_array(int n) { return new RecR[n]; }

// @ 0x431c40
RecS::~RecS() {}
// @ 0x431cb0
void delete_recr_array(RecR* p) { delete[] p; }
// @ 0x431d30
void delete_recp_array(RecP* p) { delete[] p; }

RecY::RecY() {}
// @ 0x431e10
RecY::~RecY() {}
RecY* make_recy_array(int n) { return new RecY[n]; }
// @ 0x431ec0
void delete_recy_array(RecY* p) { delete[] p; }

RecZ::RecZ() {}
RecZ* make_recz_array(int n) { return new RecZ[n]; }
// @ 0x431f40
void delete_recz_array(RecZ* p) { delete[] p; }

// @ 0x432010
RecT::~RecT() {}

// @ 0x432080
void BigObj::reset()
{
    *(int*)(pad + 0x5338) = 0;
    *(char*)(pad + 0x5330) = 0;
    *(int*)(pad + 0x532c) = 0;
    *(int*)(pad + 0x5324) = 0;
    *(int*)(pad + 0x284) = 0;
}
