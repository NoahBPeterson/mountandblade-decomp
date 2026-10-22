// Slice s0048e5d0
//   0x48e5d0..0x48e850 : six script-API slot wrappers.  Each forwards (a, b) to one vtable
//                        slot of the object looked up in the plain slot table at [this+4],
//                        then folds the 100 kHz clock delta since [this+0x20] into [this+0x28].
//   0x48e8d0           : NameRec::resolve - look the record's key up in the string table,
//                        falling back to the record's own name string.
//   0x48e8f0           : NameTable::get_string - string register / local / global variable.
//   0x48e9e0           : World::string_value - format a string register through the string table.
//   0x48ea80           : cell_search - spatial-grid candidate query (see the section note below).
//   0x48eb50           : InvOwner::inventory_add - add an item to the active agent's inventory.
//   0x48ed60/0x48ee20/0x48ee90 : inventory_tag / inventory_count / inventory_find, walking the
//                        0x1c8-byte view records at +0x668c.
//   0x48ef20           : AgentPages::slot - pages[index >> 4] + (index & 0xf) * 0x98.
//
// The functions are contiguous in the binary but are methods of several different objects
// (a tiny slot façade, a 0x20-byte name record, a name table, the big script/world object, the
// inventory owner and the agent page table).  Each shape is modelled with its own stub struct.
// Names provisional; offsets and call shapes are taken from the disassembly.
#include <windows.h>

// ---- rgl string (16 bytes): pos / buffer / capacity / length --------------------------------
// Layout and method addresses verified in s0042c0f0/tstring.h.
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    tstring(const tstring& o);                          // 0x42c110
    ~tstring();                                         // 0x42bc40
    void operator=(const tstring& o);                   // 0x42bc50
    void assign(const char* s);                         // 0x42bcb0
    char equals(const tstring& o) const;                // 0x42bdd0
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// String table / reserved-name table.  find() returns 0 when the key is absent; lookup()
// returns a shared default string instead.  Both live on the object at 0x5e2a18.
struct StrTable {
    tstring* find(const tstring* key);                  // 0x4c6010
    tstring* lookup(const tstring* key);                // 0x43a1e0
};
extern StrTable g_strtable;                             // 0x5e2a18

struct MsgLog { void push(const tstring& s, int colour); };   // 0x4334b0
extern MsgLog g_msglog;                                 // 0x5d7080

void mb_error(const char* fmt, ...);                    // 0x42cc80
int  __cdecl spr(char* buf, const char* fmt, ...);      // 0x536f8f (CRT _sprintf)

extern __int64 g_ticks[];                               // 0x5e2948 (100 kHz clock)
extern tstring g_string_registers[128];                 // 0xa72890
extern tstring g_script_string;                         // 0xa70464 (last formatted script string)

// Item-kind table (0x238-byte records) and the tag table (0x78-byte records); both globals
// hold a *pointer* to the array, not the array itself.
extern char* g_items;                                   // 0x5e29d0
extern char* g_tags;                                    // 0x5e29dc

// ------------------------------------------------------------------------------------------
// 0x20-byte name record: name at +0, lookup key at +0x10.
// ------------------------------------------------------------------------------------------
struct NameRec {
    tstring name;                                       // +0
    tstring key;                                        // +0x10
    tstring* resolve();                                 // 0x48e8d0
};

// @ 0x48e8d0
tstring* NameRec::resolve()
{
    tstring* r = g_strtable.find(&key);
    return r ? r : &name;
}

// ------------------------------------------------------------------------------------------
// Slot façade used by the six wrappers.
// ------------------------------------------------------------------------------------------
struct SlotVt {
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10(float a, int b);                   // +0x10
    virtual void v14(float a, int b);                   // +0x14 (b = colour << 24)
    virtual void v18(float a, int b);                   // +0x18
    virtual void v1c(float a, int b);                   // +0x1c (b = colour << 24)
    virtual void v20(float a, int b);                   // +0x20
    virtual void v24(float a, int b);                   // +0x24
};
struct Slot { SlotVt* vt; };

struct SlotAPI {
    char     pad0[4];
    Slot**   slots;                                     // +4  (linear array of Slot*)
    char     pad8[0x18 - 8];
    int      timer_index;                               // +0x18
    char     pad1c[0x20 - 0x1c];
    int      tick_lo;                                   // +0x20
    int      tick_hi;                                   // +0x24
    float    time;                                      // +0x28

    // 100 kHz delta -> seconds, plus the op's own increment, folded into `time`.
    __forceinline void bump(float a)
    {
        float t = (float)(g_ticks[timer_index] - *(__int64*)&tick_lo) * 1e-5f + a + 1.0f;
        time = (t < time) ? time : t;
    }

    void slot70(unsigned index, float a, int b);        // 0x48e5d0
    void slot71(unsigned index, float a, int b);        // 0x48e650
    void slot72(unsigned index, float a, int b);        // 0x48e6d0
    void slot73(unsigned index, float a, int b);        // 0x48e750
    void slot74(unsigned index, float a, int b);        // 0x48e7d0
    void slot75(unsigned index, float a, int b);        // 0x48e850
};

// @ 0x48e5d0
void SlotAPI::slot70(unsigned index, float a, int b)
{
    Slot* s = slots[index];
    s->vt->v24(a, b);
    bump(a);
}

// @ 0x48e650
void SlotAPI::slot71(unsigned index, float a, int b)
{
    Slot* s = slots[index];
    s->vt->v10(a, b);
    bump(a);
}

// @ 0x48e6d0
void SlotAPI::slot72(unsigned index, float a, int b)
{
    Slot* s = slots[index];
    s->vt->v14(a, b << 24);
    bump(a);
}

// @ 0x48e750
void SlotAPI::slot73(unsigned index, float a, int b)
{
    Slot* s = slots[index];
    s->vt->v18(a, b);
    bump(a);
}

// @ 0x48e7d0
void SlotAPI::slot74(unsigned index, float a, int b)
{
    Slot* s = slots[index];
    s->vt->v1c(a, b << 24);
    bump(a);
}

// @ 0x48e850
void SlotAPI::slot75(unsigned index, float a, int b)
{
    Slot* s = slots[index];
    s->vt->v20(a, b);
    bump(a);
}

// ------------------------------------------------------------------------------------------
// Name table: two 0x20-byte record arrays with independent bounds.
//   kind 0    -> string register `index` (or an error string)
//   kind 0x16 -> record in base2 bound by count2
//   otherwise -> record in base  bound by count
// ------------------------------------------------------------------------------------------
struct NameTable {
    NameRec* base;                                      // +0
    NameRec* base2;                                     // +4
    int      count;                                     // +8
    int      count2;                                    // +0xc
    tstring  get_string(int index, int kind);           // 0x48e8f0
};

// @ 0x48e8f0
tstring NameTable::get_string(int index, int kind)
{
    if (kind == 0) {
        if (index >= 0 && index < 0x80)
            return g_string_registers[index];
        return tstring((const char*)0x59243c);
    }
    if (kind == 0x16) {
        if (index >= 0 && index < count2)
            return *((NameRec*)((char*)base2 + index * 0x20))->resolve();
        return tstring((const char*)0x592424);
    }
    if (index >= 0 && index < count)
        return *((NameRec*)((char*)base + index * 0x20))->resolve();
    return tstring((const char*)0x592410);
}

// ------------------------------------------------------------------------------------------
// The big script/world object.  Only the method used here is declared; the offsets are opaque.
// ------------------------------------------------------------------------------------------
struct World {
    char pad[0x1a070];
    char fmt16(int a, int b, int c);                    // 0x435790
    tstring string_value(int index);                    // 0x48e9e0
};

// @ 0x48e9e0
tstring World::string_value(int index)
{
    g_script_string.assign((const char*)0x54a480);
    if (!fmt16(0x10, index, 0)) {
        char buf[0x200];
        {
            tstring key((const char*)0x592458);
            tstring* value = g_strtable.lookup(&key);
            spr(buf, value->c_str(), index);
        }
        g_script_string.assign(buf);
    }
    return g_script_string;
}

// ------------------------------------------------------------------------------------------
// Spatial-grid candidate query.
//
// The original allocates a 0x8020-byte frame, has the cell grid (0xa732b4) map the query point
// to a cell index pair via 0x41ed00 (locate) and fill up to 0x100 candidate node pointers of the
// target cell via 0x439780 (collect_point), then returns the first candidate whose 0x34-byte
// node passes the predicate 0x4333c0, or 0.  0xa732b4 is a *pointer* to the grid, and collect_point
// is called on the 0x34-byte cell (its +4/+8 hold the node vector).  Reconstructed; the residual
// byte differences are cl 13.10 register allocation only (see partial.txt / nonmatching.txt).
// ------------------------------------------------------------------------------------------
// The cell grid at 0xa732b4 and its 0x34-byte cells.  A cell holds a vector of node
// pointers at +4/+8; locate() maps a world point to a cell index pair.
// ------------------------------------------------------------------------------------------
struct CellNode {
    int      next;                                      // +0
    int      pad4;
    unsigned mask;                                      // +8
    float    bbox[2][2];                                // +0xc
};

struct Cell {
    int        pad0;
    CellNode** nodes;                                   // +4
    CellNode** nodes_end;                               // +8
    char       padc[0x34 - 0xc];
    void collect_point(const float* q, const int* flag, unsigned mask,
                       void** dst, int* count);         // 0x439780
};

struct CellGrid {
    float  min[2];                                      // +0
    char   pad8[8];
    void** buckets;                                     // +0x10
    char   pad14[0x28 - 0x14];
    char** cells;                                       // +0x28
    void locate(const float* p, int* out);              // 0x41ed00
};
extern CellGrid* g_cellgrid;                            // 0xa732b4 (pointer to the grid)

char __fastcall rec_applies(CellNode* c, const float* p);   // 0x4333c0

// @ 0x48ea80
void* __stdcall cell_search(const float* p)
{
    int    idx[2];
    void*  cand[0x2000];
    int    count;
    float  q[2];
    int    flag;

    q[0] = p[0];
    q[1] = p[1];
    g_cellgrid->locate(q, idx);
    flag = 0;
    Cell* cell = (Cell*)(g_cellgrid->cells[idx[0]] + idx[1] * 0x34);
    cell->collect_point(q, &flag, 0x100, cand, &count);
    for (int k = 0; k < count; k++) {
        CellNode* c = (CellNode*)cand[k];
        if (c->mask < 0)
            continue;
        if (rec_applies(c, p))
            return c;
    }
    return 0;
}

// ------------------------------------------------------------------------------------------
// Inventory.
// ------------------------------------------------------------------------------------------
struct InvRow {                                         // 8 bytes
    int item;                                           // +0
    int amount;                                         // +4
    void update(int v);                                 // 0x437510
};

struct Agent {                                          // 0x508 bytes
    char    pad[0xb4];
    InvRow* rows() { return (InvRow*)((char*)this + 0xb4); }
    int     count();                                    // 0x462b60
    void    add(int item, int slot, int amount);        // 0x48c7f0
};

struct ViewRec {                                        // 0x1c8 bytes
    char    pad0[0x188];
    int     flag;                                       // +0x188
    int     key;                                        // +0x18c
    tstring name;                                       // +0x190
    char    pad1a0[0x1c8 - 0x1a0];
};

struct InvOwner {
    char      pad0[0x8c];
    int       agent_index;                              // +0x8c
    char      pad90[0x668c - 0x90];
    ViewRec*  view_begin;                               // +0x668c
    ViewRec*  view_end;                                 // +0x6690
    char      pad6694[0x13a00 - 0x6694];
    char*     agent_base;                               // +0x13a00
    char      pad13a04[0x30];
    int       x13a34;                                   // +0x13a34

    void inventory_add(int item, int amount);           // 0x48eb50
    void inventory_tag(int a, int b);                   // 0x48ed60
    int  inventory_count(int tag);                      // 0x48ee20
    int  inventory_find(int tag, int nth);              // 0x48ee90
};

// @ 0x48eb50
void InvOwner::inventory_add(int item, int amount)
{
    Agent* agent = (Agent*)(agent_base + agent_index * 0x508);
    InvRow* rows = agent->rows();
    int n = agent->count();

    int free_slot = -1;
    for (int i = 0; i < n; i++) {
        if (rows[i].item == -1) {
            free_slot = i;
            break;
        }
    }

    if (free_slot >= 0) {
        agent->add(item, free_slot, amount);
        if (x13a34 == 0)
            return;

        char buf[0x200];
        {
            tstring key((const char*)0x59248c);
            char* kind = g_items + item * 0x238;
            tstring* nm = g_strtable.find((tstring*)(kind + 0x20));
            const char* name = (nm ? nm : (tstring*)kind)->c_str();
            tstring* fmt = g_strtable.lookup(&key);
            spr(buf, fmt->c_str(), name);
        }
        {
            tstring msg(buf);
            g_msglog.push(msg, (int)0xff80ff80);
        }
        return;
    }

    // No free slot: only a unique item may displace a non-unique one.
    unsigned flags = *(unsigned*)(g_items + item * 0x238 + 0x1c8);
    if (!((flags >> 12) & 1))
        return;

    int replace = -1;
    int m = agent->count();
    int i;
    for (i = 0; i < m; i++) {
        if (replace != -1)
            break;
        unsigned f = *(unsigned*)(g_items + rows[i].item * 0x238 + 0x1c8);
        if (!((f >> 12) & 1))
            replace = i;
    }
    if (replace == -1) {
        mb_error((const char*)0x592468);
        return;
    }

    // The original indexes the old row with the loop counter (one past `replace`).
    int old_item = rows[i].item;
    rows[replace].item = item;
    rows[replace].amount = 0;
    rows[replace].update(0);
    *(char*)((char*)&rows[replace] + 7) = 0;
    rows[replace].update(0);
    inventory_add(old_item, amount);
}

// @ 0x48ed60
void InvOwner::inventory_tag(int a, int b)
{
    ViewRec* base = view_begin;
    if (!base)
        return;
    int n = (int)(((char*)view_end - (char*)base) / 0x1c8);
    for (int i = 0; i < n; i++) {
        ViewRec* r = base + i;
        if (r->flag)
            continue;
        if (!r->name.equals(*(tstring*)(g_tags + a * 0x78)))
            continue;
        r->key = b;
        r->name = *(tstring*)(g_tags + b * 0x78);
    }
}

// @ 0x48ee20
int InvOwner::inventory_count(int tag)
{
    int count = 0;
    ViewRec* base = view_begin;
    if (!base)
        return 0;
    unsigned n = (unsigned)(((char*)view_end - (char*)base) / 0x1c8);
    for (unsigned i = 0; i < n; i++) {
        ViewRec* r = base + i;
        if (!r->flag && r->key == tag)
            count++;
    }
    return count;
}

// @ 0x48ee90
int InvOwner::inventory_find(int tag, int nth)
{
    int found = 0;
    ViewRec* base = view_begin;
    if (!base)
        return -1;
    unsigned n = (unsigned)(((char*)view_end - (char*)base) / 0x1c8);
    for (unsigned i = 0; i < n; i++) {
        ViewRec* r = base + i;
        if (!r->flag && r->key == tag) {
            if (found == nth)
                return i;
            found++;
        }
    }
    return -1;
}

// ------------------------------------------------------------------------------------------
// Agent page table (a different object from the slot façade).
// ------------------------------------------------------------------------------------------
struct AgentPages {
    char   pad0[0x18];
    void** pages;                                       // +0x18
    void*  slot(unsigned index);                        // 0x48ef20
};

// @ 0x48ef20
void* AgentPages::slot(unsigned index)
{
    return (char*)pages[index >> 4] + (index & 0xf) * 0x98;
}
