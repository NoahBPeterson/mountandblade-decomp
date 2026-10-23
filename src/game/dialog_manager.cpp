//-----------------------------------------------------------------------------
// File: dialog_manager.cpp  (0x460160..0x461200)
//
// Desc: the conversation manager. The {token} expander of the string registers (ScriptState::expand_token /
//       expand), DialogMgr's accessors, the script-block reader (Block::read, with the Elem constructor the
//       `new Elem[]` passes to the vector constructor iterator), the dialog records read from the module file
//       (DialogRec::read, DialogMgr::read), the availability test, line collection (up to 0x400 lines), and the
//       start / advance loops. Game code. Names provisional (Module System: dialogs, dlga_ ids, repeat_for_*
//       flags, anyone / party speaker flags). Functions in ascending address order (cl's bottom-up emission).
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <stddef.h>
#include <new>                                             // declares operator new[]/delete[]: without it cl calls operator new/delete for arrays
#include "./tstring.h"
#include "./dialog_manager.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "./str_table.h"

// @ 0x460160
tstring ScriptState::expand_token(const tstring& tok_)
{
    char buf[0x10000];
    tstring& tok = (tstring&)tok_;
    buf[0] = 0;
    if (tok.len == 3 && tok.equals("***"))
        return tstring("");
    for (int i = 1; i < tok.len; i++) {
        if (tok.buf[i] == '{') {
            tstring inner("");
            int depth = 0;
            int j = i + 1;
            for (; j < tok.len; j++) {
                if (tok.buf[j] == '}' && depth <= 0)
                    break;
                if (tok.buf[j] == '{')
                    depth++;
                else if (tok.buf[j] == '}')
                    depth--;
                inner.push_back(tok.buf[j]);
            }
            if (j > i && j < tok.len) {
                tok.erase_range(i, j + 1);
                tok.insert(i, expand_token(inner));
            }
        }
    }
    if ((tok.len == 4 || tok.len == 5) && tok.buf[0] == 'r' && tok.buf[1] == 'e' && tok.buf[2] == 'g' &&
        tok.buf[3] >= '0' && tok.buf[3] < ':') {
        int n = tok.buf[3] - '0';
        if (tok.len > 4)
            n = tok.buf[4] - '0' + n * 10;
        if (n >= 0 && n < 0x80)
            mb_snprintf(buf, 0x10000, "%d", reg[n]);
        else
            mb_snprintf(buf, 0x10000, "TOKEN EXCEEDS MAX REGISTERS: <%s>", tok.c_str());
    } else if (tok.len > 5 && tok.buf[0] == 'r' && tok.buf[1] == 'e' && tok.buf[2] == 'g' && tok.buf[3] >= '0' &&
               tok.buf[3] < ':' && (tok.buf[4] == '?' || tok.buf[5] == '?')) {
        int n = tok.buf[3] - '0';
        int start = 5;
        if (tok.buf[4] >= '0' && tok.buf[4] < ':') {
            n = tok.buf[4] - '0' + n * 10;
            start = 6;
        }
        if (n >= 0 && n < 0x80) {
            int colon = -1;
            for (int k = 0; k < tok.len; k++)
                if (tok.buf[k] == ':')
                    colon = k;
            if (colon >= 0) {
                if (reg[n] == 0)
                    mb_snprintf(buf, 0x10000, "%s", tok.substr(colon + 1, tok.len).c_str());
                else
                    mb_snprintf(buf, 0x10000, "%s", tok.substr(start, colon).c_str());
            } else {
                mb_snprintf(buf, 0x10000, "UNRECOGNIZED TOKEN <%s>", tok.c_str());
            }
        } else {
            mb_snprintf(buf, 0x10000, "TOKEN EXCEEDS MAX REGISTERS: <%s>", tok.c_str());
        }
    } else if ((tok.len == 2 || tok.len == 3) && tok.buf[0] == 's' && tok.buf[1] >= '0' && tok.buf[1] < ':') {
        int n = tok.buf[1] - '0';
        if (tok.len > 2)
            n = tok.buf[2] - '0' + n * 10;
        if (n >= 0 && n < 0x80)
            mb_snprintf(buf, 0x10000, "%s", str[n].c_str());
        else
            mb_snprintf(buf, 0x10000, "TOKEN EXCEEDS MAX REGISTERS: <%s>", tok.c_str());
    } else if (tok.equals("playername")) {
        TroopRec* p = &g_game->chars[g_game->player];
        tstring* t = g_strtable.find(&p->id);
        if (!t)
            t = (tstring*)p;
        mb_snprintf(buf, 0x10000, "%s", t->c_str());
    } else {
        int slash = -1;
        for (int k = 0; k < tok.len; k++)
            if (tok.buf[k] == '/')
                slash = k;
        if (slash >= 0) {
            if ((g_game->chars[g_game->player].flags & 0xf) != 1)
                mb_snprintf(buf, 0x10000, "%s", tok.substr(0, slash).c_str());
            else
                mb_snprintf(buf, 0x10000, "%s", tok.substr(slash + 1, tok.len).c_str());
        } else {
            mb_snprintf(buf, 0x10000, "UNRECOGNIZED TOKEN <%s>", tok.c_str());
        }
    }
    if (buf[0]) {
        char* p = buf;
        do {
            if (*p == '^')
                *p = '\n';
        } while (*++p);
    }
    tstring res(buf);
    return res;
}

// @ 0x460620
int DialogMgr::get_c8()
{
    if (f8 == 6)
        return 1;
    return g_game->fc8;
}

// @ 0x460640
int DialogMgr::speaker()
{
    unsigned fl = recs[cur].flags;
    if (fl & 0x10000)
        return g_game->player;
    if (fl & 0xfff00000)
        return (int)fl >> 20;
    return fc;
}

// @ 0x460670
int DialogMgr::listener()
{
    if (recs[cur].flags & 0x10000)
        return fc;
    return g_game->player;
}

// @ 0x4606a0
tstring DialogMgr::line_text(int i)
{
    return list[i].text;
}

// @ 0x4606c0
int DialogMgr::line_flags(int i)
{
    return list[i].flags;
}

// @ 0x4606d0
tstring ScriptState::expand(const tstring& in)
{
    int i = 0;
    tstring out;
    tstring inner;
    for (; i < in.len; i++) {
        if (in.buf[i] != '{') {
            if (in.buf[i] == '^')
                out.push_back('\n');
            else
                out.push_back(in.buf[i]);
        } else {
            inner.assign("");
            i++;
            int depth = 0;
            for (; i < in.len; i++) {
                if (in.buf[i] == '}' && depth <= 0)
                    break;
                if (in.buf[i] == '{')
                    depth++;
                else if (in.buf[i] == '}')
                    depth--;
                inner.push_back(in.buf[i]);
            }
            out.append(expand_token(inner));
        }
    }
    return out;
}

// 0x4607d0 Elem::Elem (defined inline in dialog_manager.h; emitted for the vector constructor iterator of Block::read)

// @ 0x4607e0
void Block::read(stream_base* s)
{
    done = 0;
    if (items) {
        delete[] items;
        items = 0;
    }
    s->r_int(&count);
    if (count)
        items = new Elem[count];
    for (int i = 0; i < count; i++) {
        Elem* e = &items[i];
        s->r_uint(&e->id);
        s->r_int(&e->n);
        for (int j = 0; j < e->n; j++)
            s->r_i64(&e->vals[j]);
    }
}

// @ 0x4608d0
void DialogRec::read(stream_base* s)
{
    char buf[0x200];
    s->r_tstr(&id);
    s->r_int((int*)&flags).r_int(&f24);
    cond.read(s);
    _snprintf(buf, 0x200, "dialog condition: %s", id.c_str());
    {
        tstring t(buf);
        cond.text = t;
    }
    s->r_tstr(&text);
    text.underscores_to_spaces();
    s->r_int(&f60);
    conseq.read(s);
    _snprintf(buf, 0x200, "dialog consequence: %s", id.c_str());
    {
        tstring t(buf);
        conseq.text = t;
    }
}

// 0x4609d0: DialogRec's vector deleting destructor (??_EDialogRec), generated by the delete[] in DialogMgr::read

// @ 0x460ab0
void DialogMgr::read(stream_base* s)
{
    s->r_int(&count);
    if (recs) {
        delete[] recs;
        recs = 0;
    }
    recs = new DialogRec[count];
    for (int i = 0; i < count; i++)
        recs[i].read(s);
}

// @ 0x460b90
bool DialogMgr::available(int idx, char flag)
{
    unsigned char bad = 0;
    DialogRec* r = &recs[idx];
    if (r->f24 == f8) {
        if (flag) {
            bad = ~(unsigned char)(r->flags >> 16) & 1;
            if (bad)
                goto done;
        }
        unsigned who = r->flags & 0xfff;
        if (who == 0xfff)
            goto done;
        if (r->flags & 0x20000) {
            if (f10 < 0)
                goto fail;
            if (who == g_game->parties.get(f10)->owner)
                goto done;
        } else if (who == fc) {
            goto done;
        }
    }
fail:
    bad = 1;
done:
    return bad == 0;
}

// @ 0x460c10
void DialogMgr::refresh_text()
{
    tstring name;
    if (cur >= 0) {
        DialogRec* r = &recs[cur];
        tstring* t = g_strtable.find(&r->id);
        if (!t)
            t = &r->text;
        name = *t;
    } else {
        name.assign("Error");
    }
    cur_text = g_script.expand(name);
}

// @ 0x460ca0
void PartyMgr::next_party_id(int* id)
{
    unsigned i = index_of(*id) + 1;
    while ((int)i < count && pages[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        *id = deflt;
    else
        *id = pages[i >> 4][i & 0xf].id;
}

// @ 0x460d20
int PartyMgr::first_party_id()
{
    unsigned i = 0;
    while ((int)i < count && pages[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        return deflt;
    return pages[i >> 4][i & 0xf].id;
}

// @ 0x460d80
void DialogMgr::select(DlgLine* l)
{
    g_game->fc8 = 0;
    f8 = recs[l->dialog].f60;
    cur = l->dialog;
    cur_idx = l->idx;
    g_game->f13a1c = l->idx;
    recs[l->dialog].conseq.exec(4);
    refresh_text();
}

// @ 0x460de0
void DialogMgr::collect(DlgLine* out, int max, int* n, char flag)
{
    *n = 0;
    if (max > 0x400)
        max = 0x400;
    for (int i = 0; i < count; i++) {
        if (*n >= max)
            return;
        if (available(i, flag)) {
            int start = 0;
            int reps = 1;
            unsigned mode = recs[i].flags & 0xf000;
            if (mode != 0) {
                if (mode == 0x1000) {
                    reps = g_game->f13a4c;
                } else if (mode == 0x3000) {
                    reps = g_game->f13a04;
                } else if (mode == 0x2000) {
                    GameData* g = g_game;
                    start = g_game->parties.first_party_id();
                    reps = g->parties.deflt;
                } else if (mode == 0x4000) {
                    reps = 100;
                } else if (mode == 0x5000) {
                    reps = 1000;
                }
            }
            int j = start;
            while (j < reps && *n < max) {
                g_game->f13a1c = j;
                if (recs[i].cond.exec(2)) {
                    out[*n].dialog = i;
                    out[*n].idx = j;
                    DialogRec* r = &recs[i];
                    tstring* t = g_strtable.find(&r->id);
                    if (!t)
                        t = &r->text;
                    out[*n].text = g_script.expand(*t);
                    out[*n].flags = recs[i].flags;
                    (*n)++;
                }
                if ((recs[i].flags & 0xf000) == 0x2000)
                    g_game->parties.next_party_id(&j);
                else
                    j++;
            }
        }
    }
}

// @ 0x460ff0
void DialogMgr::advance()
{
    int n;
    collect(sel, 1, &n, 0);
    while (n > 0 && (recs[sel[0].dialog].flags & 0x40000)) {
        select(sel);
        collect(sel, 1, &n, 0);
    }
    if (n > 0)
        select(sel);
}

// @ 0x4610b0
// Starts a conversation: stores the parties / troop, resets the dialog globals and runs the automatic lines.
// The body (0x4610c2) is bracketed by two 18-byte inline tags of the original: `jmp +0x10; "WL  ", id, 0, "WL  "`
// with id 0x0c at the start (the 0x4610b0 "thunk") and 0x0d at the end (before the epilogue).
// The tags cannot be written in C++ (the inline assembler here accepts only _emit for data, which the rules
// forbid), so this source omits them and the row is nonmatching.
void DialogMgr::start(int a, int b, int c, int d, char f)
{
    fc = a;
    f10 = c;
    fe024 = f;
    g_game->fb0 = 0;
    g_game->fc8 = 0;
    g_game->f1a23c = 0;
    f8 = d;
    f14 = b;
    cur = -1;
    cur_idx = 0;
    advance();
}

// @ 0x461140
void DialogMgr::advance_all(int* n)
{
    collect(list, 0x400, n, 1);
    while (*n > 0 && (recs[list[0].dialog].flags & 0x40000)) {
        select(list);
        collect(list, 0x400, n, 1);
    }
    nres = *n;
}

