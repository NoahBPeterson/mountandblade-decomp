#include "./save_state.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../out_stream.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../list8.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../rec5c.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4e8f10    int GameData::save_state(OutStream* out)   (thiscall, ret 4)
int GameData::save_state(OutStream* out)
{
    __int64 counter = g_time1;
    out->w_int8((int*)&counter);
    out->w_raw((char*)this + 4);
    out->w_raw((char*)this + 0xb4);
    write_clock(out);
    g_list_a70480.write8(out);
    ((Rec78Tag*)((char*)this + 0x139e8))->write(out);
    ((Rec30Tag*)((char*)this + 0x139f4))->write(out);
    write_list138(out);
    write_list_party(out);
    write_list300(out);
    write_vec5c(out);
    out->w_raw(&g_party_template_count);
    for (int i = 0; i < g_party_template_count; i++)
        g_party_templates[i].write(out);
    write_other2(out);
    ((Table2*)((char*)this + 0xcc))->write_table(out);
    write_records(out);
    write_items(out);
    out->w_int8((int*)&records[f8c].f3c0);
    out->w_int8((int*)&records[f8c].f3c8);
    out->w_raw((char*)this + 0x98);
    out->w_raw((char*)this + 0xa0);
    out->w_raw((char*)this + 0x9c);
    out->w_raw((char*)this + 0xa4);
    return 0;
}

// @ 0x4e90d0    void Rec78Tag::write(OutStream* out)   (thiscall, ret 4)
void Rec78Tag::write(OutStream* out)
{
    out->w_raw(this);
    for (int i = 0; i < count; i++)
        array[i].write(out);
}

// @ 0x4e9110    void Rec78::write(OutStream* out)   (thiscall, ret 4)
void Rec78::write(OutStream* out)
{
    out->w_raw((char*)this + 0xc);
    out->w_int1(&f50);
    out->w_int1(&f60);
    out->w_int1(&f70);
}

// @ 0x4e9150    void Rec30Tag::write(OutStream* out)   (thiscall, ret 4)
void Rec30Tag::write(OutStream* out)
{
    out->w_raw(this);
    for (int i = 0; i < count; i++)
        out->w_int1((int*)(array + i * 0x30 + 0x28));
}

// @ 0x4e9190    int GameData::write_clock(OutStream* out)   (thiscall, ret 4)
int GameData::write_clock(OutStream* out)
{
    char* p = (char*)this;
    out->w_int1((int*)&f13b20);
    out->w_raw(p + 0x13b28);
    out->w_raw(p + 0x13b2c);
    out->w_raw(p + 0x13b30);
    out->w_raw(p + 0x13b34);
    out->w_raw(p + 0x13b38);
    out->w_raw(p + 0x13b60);
    out->w_int(&i13b3c);
    out->w_int(&i13b40);
    out->w_int(&f13b44);
    out->w_int(&f13b48);
    out->w_struct(p + 0x13a70);
    out->w_int4(&f13aa0);
    out->w_raw(p + 0x13ae8);
    out->w_raw(p + 0x13a20);
    out->w_raw(p + 0x13a0c);
    out->w_raw(p + 0x13a10);
    out->w_raw(p + 0x13aec);
    out->w_raw(p + 0x13af4);
    out->w_raw(p + 0x13af8);
    out->w_raw(p + 0x13afc);
    out->w_raw(p + 0x13ae0);
    out->w_raw(p + 0x13ae4);
    tstring tmp = f13b50;
    tmp.clear();
    if (!tmp.len) {
        static const char kEmpty[] = "";
        out->w_cstr(kEmpty);
    } else {
        out->w_struct(&tmp);
    }
    out->w_raw(p + 0x1a1d8);
    out->w_raw(p + 0x1a1dc);
    out->w_raw(p + 0x1a1e0);
    out->w_raw(p + 0x1a1e4);
    out->w_raw(p + 0x1a1e8);
    out->w_raw(p + 0x1a1ec);
    out->w_int1(&f13a88);
    out->w_int(&f13ad0);
    out->w_raw(p + 0x13adc);
    out->w_raw(p + 0x13ad8);
    out->w_raw(p + 0x13ad4);
    return 0;
}

// @ 0x4e93f0    void GameData::write_vec5c(OutStream* out)   (thiscall, ret 4)   vector at +0x13a64, stride 0x5c
void GameData::write_vec5c(OutStream* out)
{
    int n;
    if (!f13a64)
        n = 0;
    else
        n = (int)(f13a68 - f13a64) / 0x5c;
    out->w_count(&n);
    for (char* r = f13a64; r < f13a68; r += 0x5c)
        ((Rec5c*)r)->write(out);
}

// @ 0x4e9460    void Rec5c::write(OutStream* s)   (thiscall, ret 4)
void Rec5c::write(OutStream* s)
{
    char* p = (char*)this;
    s->w_int((int*)(p + 0)).w_int((int*)(p + 4)).w_int((int*)(p + 8));
    s->w_int((int*)(p + 0x10));
    s->w_int((int*)(p + 0x14)).w_raw(p + 0x18);
}

// @ 0x4e94b0    void List8::write8(OutStream* out)   (thiscall, ret 4)
void List8::write8(OutStream* out)
{
    int n;
    if (!begin)
        n = 0;
    else
        n = (int)(end - begin);
    out->w_count(&n);
    for (unsigned i = 0; i < (!begin ? 0u : (unsigned)(end - begin)); i++)
        out->w_int1((int*)&begin[i]);
}
void write_vec8(List8* v, OutStream* out) { v->write8(out); }

// @ 0x4e9510    void RecB0::write(OutStream* out)   (thiscall, ret 4)
void RecB0::write(OutStream* out)
{
    out->w_raw(&f34);
    out->w_raw(&f38);
    out->w_raw(&f3c);
    f_a0.write8(out);
}

// @ 0x4e9550    void GameData::write_list138(OutStream* out)   (thiscall, ret 4)
void GameData::write_list138(OutStream* out)
{
    out->w_raw(&n13a5c);
    for (int i = 0; i < n13a5c; i++)
        f13a58[i].write(out);
}

// @ 0x4e95a0    void Rec138::write(OutStream* out)   (thiscall, ret 4)
void Rec138::write(OutStream* out)
{
    out->w_raw(&f64);
    out->w_raw(&f118);
    out->w_raw(&f114);
    out->w_int(&f11c);
    out->w_struct(&f40);
    out->w_struct(&f30);
    out->w_struct(&f20);
    write_soa(out, (char*)this + 0x68);
    f128.write8(out);
}

// @ 0x4e9620    OutStream* write_soa(OutStream* out, char* base)   (free function, ret 0)
// Eight iterations: the struct at base+0x20+i*0x10, the int at base+i*4 and the block at base+0xa0.
struct Soa8 { int v[8]; char s[8][16]; int blk[1]; };
OutStream* write_soa(OutStream* out, char* base)
{
    Soa8* b = (Soa8*)base;
    int* blk = b->blk;
    for (int i = 0; i < 8; i++) {
        out->w_struct(b->s[i]);
        out->w_raw(&b->v[i]);
        out->w_raw(blk);
    }
    return out;
}

// @ 0x4e9670    void GameData::write_list_party(OutStream* out)   (thiscall, ret 4)
void GameData::write_list_party(OutStream* out)
{
    out->w_raw(&nparties);
    for (int i = 0; i < nparties; i++)
        ((List8*)((char*)party_recs + i * 0x494 + 0x474))->write8(out);
}

// @ 0x4e96c0    void GameData::write_list300(OutStream* out)   (thiscall, ret 4)
void GameData::write_list300(OutStream* out)
{
    out->w_raw(&n13a4c);
    for (int i = 0; i < n13a4c; i++)
        f13a48[i].write300(out, n13a4c);
}

// @ 0x4e9710    void Rec300::write300(OutStream* out, int n)   (thiscall, ret 8)
void Rec300::write300(OutStream* out, int n)
{
    f24c.write8(out);
    for (int i = 0; i < n; i++)
        out->w_int((int*)(f2c + i * 4));
    out->w_struct(this);
    out->w_int4(&f10);
    out->w_count(&f28);
    out->w_raw((char*)this + 0x230);
    write_soa(out, (char*)this + 0x25c);
}

// @ 0x4e9790    void GameData::write_items(OutStream* out)   (thiscall, ret 4)   item kinds at 0x5e29d0
void GameData::write_items(OutStream* out)
{
    out->w_raw(&g_item_kind_count);
    for (int i = 0; i < g_item_kind_count; i++)
        write_vec8((List8*)g_items[i].l30, out);
}

// @ 0x4e97e0    void GameData::write_records(OutStream* out)   (thiscall, ret 4)
void GameData::write_records(OutStream* out)
{
    out->w_raw(&nrecords);
    for (int i = 0; i < nrecords; i++)
        records[i].write_save(out);
    char* q = (char*)0x8ee3138;
    int   qend = 0x8ee3c60;
    while ((int)q < qend) {
        out->w_raw(q);
        q += 0x44;
    }
}