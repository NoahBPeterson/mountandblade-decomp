# TinyXML migration (0x42d820-0x4313ff)

The 8 slices that cover the statically linked TinyXML (`s0042d360` (its TinyXML half), `s0042dcb0`,
`s0042e4f0`, `s0042ee90`, `s0042f840`, `s00430190`, `s00430860`, `s00431050` (its first function)) are
merged into TinyXML's three translation units. The slices under `match/slices/` are unchanged and stay
the reference.

The library is **TinyXML 2.4.x, non-STL build** (`TIXML_USE_STL` undefined, `TIXML_SAFE` defined, so
`TIXML_SNPRINTF` = `_snprintf`). It is 2.4 and not 2.5, which the slice headers claimed. The evidence:
- `TiXmlBase`'s vtable is `~TiXmlBase, Print, Parse, StreamOut`, and the code has `TiXmlOutStream`.
  2.5 replaced `StreamOut` with `TiXmlPrinter`/`Accept`.
- `LoadFile()` uses the `StringToBuffer` helper (STL_STRING_BUG).
- `Attribute`/`Query*Attribute`/`SetAttribute` build `TIXML_STRING` temporaries for
  `Find(const TIXML_STRING&)`.
- The non-const `GetDocument`/`NextSiblingElement`/`FirstChildElement`/`FirstChild(const char*)` have
  bodies of their own. In 2.5 they forward to the const versions (see "Header conflicts").

The 16 error strings (with `TIXML_ERROR_PARSING_CDATA`), CDATA text nodes, `useMicrosoftBOM`,
`LoadFile(FILE*)` and the `utf8ByteTable` that starts its 2-byte range at 0xc2 put it at 2.4.2/2.4.3.

All three modules build with the default flags `/O2 /MT /Gy /TP`. The region has no EH frames, so
there is no `/EHsc`. Each .cpp defines `NDEBUG` (Release project setting: TinyXML's `assert`s
compile to nothing) and includes `<new>` before the TinyXML headers. Without `<new>`, cl 13.10 lowers
`new int[]`/`delete[]` of the string reps to the scalar operators, while the original calls
0x535482/0x53570d. That replaces the slices' `TX_NEW_ARRAY`/`TX_DELETE_ARRAY` macros. The headers
are TinyXML's own `tinystr.h`/`tinyxml.h`, trimmed to what the linked code uses (see below), and are
included as `"./x.h"` so that equiv.py reads their `// 0x` annotations.

Verification:
- `chk.py` on every module VA.
- `run_all.py --manifest` on each `<module>.manifest.txt`.
- The resolution audit (`resaudit_all.py --slices src:src/tinyxml/<m>.manifest.txt,...`, log
  `work/resaudit/mig_tinyxml.jsonl`).
- The equivalence harness on the formerly nonmatching function.

## Summary

| module | functions | byte-exact before (slices) | byte-exact after | improved | regressed |
|---|---|---|---|---|---|
| tinystr.cpp | 4 | 4 | 4 | 0 | 0 |
| tinyxml.cpp | 66 | 65 | 66 | 1 | 0 |
| tinyxmlparser.cpp | 21 | 21 | 21 | 0 | 0 |
| **total** | **91** | **90** | **91** | **1** | **0** |

- Resolution audit: 91 / 91 rows `agree` (0 disagree, 0 unverified, 0 not audited).
- No `nonmatching.txt` or `partial.txt` files: every VA of the range is a manifest row.
- Improved: 0x42f210 `TiXmlElement::SetAttribute(const char*, const char*)` went from nonmatching
  (58 bytes: the slice put the second `TiXmlString` temporary in a fresh local instead of the dead
  parameter home slot) to MATCH. It compiles byte-exact as TinyXML's literal source inside the full
  tinyxml.cpp TU with the real headers. The cause was not isolated further. Equivalence harness (module
  flags): WEAK with no mismatch, 238 valid inputs. Both sides miss 14 of 20 edges (6/20 covered on each side),
  mostly the allocation-failure and found-attribute paths that random inputs do not reach.
  `TiXmlElement::Attribute` (0x42f0b0) and `TiXmlAttributeSet::Find` (0x42df40): WEAK, no mismatch,
  1 edge missed on each side. Log: `work/match/scratch_mig_tinyxml_equiv.log`.
- Regressions: none. Every function that was byte-exact in its slice is byte-exact in its module.

## Boundaries
The brief's boundaries are right. They are confirmed by which file each function lives in, in the
TinyXML 2.4 source:
- **tinystr.cpp, 0x42d820-0x42d99f:** `reserve`, `assign` and `append`, preceded by the out-of-line
  copy of the inline `TiXmlString::quit()` (identical to `~TiXmlString`; ICF). Compiling tinystr.cpp
  emits these plus unreferenced inline COMDATs.
- **tinyxml.cpp, 0x42d9a0-0x42f71f:** from `TiXmlNode::LinkEndChild` to
  `TiXmlDocument::LoadFile(TiXmlEncoding)`.
- **tinyxmlparser.cpp, 0x42f720-0x4313ff:** from `TiXmlBase::ConvertUTF32ToUTF8` (a tinyxmlparser.cpp
  function) to `TiXmlDeclaration::Parse` (0x431050).
- 0x431400 is not TinyXML. It is the engine record destructors (`??_GIfaceA`, ...) that s00431050
  also covers.

Inside each file the functions appear in the binary's order, which is cl's bottom-up emission order,
not TinyXML's source order. Each .cpp lists its functions by ascending address. A helper with no copy
of its own in the binary (inlined into every caller) sits just before its first caller. Examples:
`TiXmlNode::Clear`/`CopyTo`, `TiXmlAttributeSet` ctor/dtor/`Remove`, `TiXmlAttribute::Next`/
`QueryIntValue`/`QueryDoubleValue`, `StringToBuffer`, `PutString(.., TIXML_OSTREAM*)`,
`TiXmlBase::IsAlphaNum`, `~TiXmlElement` and the `Comment`/`Text`/`Unknown::CopyTo`.

Out-of-line copies of header-inline members and compiler-generated functions have no definition at
their address. A plain `// 0x...:` comment marks the spot (no `// @` marker, see "Tooling" below).
These are:
- 0x42d820 `TiXmlString::quit`.
- 0x42da20 / 0x42da50 `TiXmlString::operator=` / `operator+=` (`const char*`).
- 0x42daa0 `TiXmlAttribute::SetValue`.
- 0x42e070 `TiXmlAttribute(name, value)` and 0x42e0f0 the implicit `~TiXmlAttribute`.
- 0x42ed00 `TiXmlString(const char*)`.
- The inline constructors 0x42edc0 / 0x42ee30 / 0x42eec0 / 0x42ef70.
- 0x42ef20 `~TiXmlDeclaration`.
- 0x42fa40 `IsWhiteSpace(char)` and 0x430360 `GetChar`.
- The scalar deleting destructors 0x42da80, 0x42ed70, 0x42ee00, 0x42ee90, 0x42ef00, 0x42efb0,
  0x42efe0, 0x42f010 and 0x42f660.

All of them come out of the real source by themselves: no stand-ins and no dummy callers are needed.

## Removed slice-local tricks
- `TX_NEW_ARRAY` / `TX_DELETE_ARRAY` (s0042e4f0's header): replaced by the real `new int[]` /
  `delete []` with `<new>` included.
- `#pragma inline_depth(1)` around `~TiXmlDocument` (s0042ee90). The deleting destructor keeps
  `~TiXmlString` out of line by itself in the real TU.
- `__declspec(noinline)` on `TiXmlString::assign`/`append` (s0042d360's header) and on
  `~TiXmlDeclaration` (s0042e4f0's header).
- The dummy `destroy_attribute()` that forced the implicit `~TiXmlAttribute` out of line (s0042dcb0).
  `~TiXmlAttributeSet`/the deleting destructor emit it.
- Out-of-line declarations of TinyXML's inline members, made so that a slice could call them:
  `operator=`, `operator+=`, `TiXmlAttribute::SetValue`, `TiXmlAttributeSet::Add`, `LinkEndChild`, the
  `TiXmlNode` ctor, `TiXmlComment`/`TiXmlText` ctors, `IsWhiteSpace`, `IsAlpha`, `StringEqual`,
  `SetError`. They are back where TinyXML has them, inline in the header or in the .cpp, and cl's own
  inlining reproduces every call and every inlined copy.
- Duplicate definitions in several slices: `Stamp` (3x), `IsAlpha`, `GetDocument`, the
  `TiXmlAttribute` ctor and `~TiXmlNode`, `quit`, `operator=`.
- The stand-in externs `errorString_` and `::entity` (s0042f840, s0042d360) are now the class statics
  `TiXmlBase::errorString` and `TiXmlBase::entity`.

## Data
Defined in the modules with their addresses: `TiXmlString::npos` (0x54b8f4), `TiXmlString::nullrep_`
(0x5e23d0), `TiXmlBase::condenseWhiteSpace = true` (0x5a9068), `TiXmlBase::entity` (0x5a90b0) and
`TiXmlBase::utf8ByteTable` (0x54bd88). The tables were checked against the binary's bytes.
`TiXmlBase::errorString` (0x5a9070, 16 strings) is only declared. Its definition is in
tinyxmlerror.cpp, which has no code and was not created.

Not linked, so left out: the `operator+` overloads (tinystr.cpp) and most of the tinyxml.cpp API:
`Insert*`/`Replace*`/`RemoveChild`, `SaveFile`, `SetAttribute(int)`, `IterateChildren`, the const
twins of the accessors, `TiXmlHandle::Child*` and the stream operators. cl also emits unreferenced
COMDATs of small inline helpers, which `/OPT:REF` drops.

## Canonical names (changed from the slices)
| slices' symbol | module (TinyXML 2.4) |
|---|---|
| `?ReadValue@TiXmlNode@@...` (0x430190) | `TiXmlElement::ReadValue` (protected) |
| `GetDocument() const` (0x42d9d0) | `TiXmlDocument* TiXmlNode::GetDocument()` (non-const; it calls the non-const `ToDocument`, slot 4) |
| `FirstChild(const char*) const` returning non-const (0x42dc10) | `TiXmlNode* TiXmlNode::FirstChild(const char*)` |
| `NextSiblingElement() const` returning non-const (0x42dc80) | `TiXmlElement* TiXmlNode::NextSiblingElement()` |
| `ClearThis` (public) | protected (`?ClearThis@TiXmlElement@@IAEXXZ`) |
| `TiXmlDocument::CopyTo` (public/protected) | private |
| `StreamOut` (public) | protected in the node classes (`M` access), public in `TiXmlAttribute` |
| `PutString` (public) | protected (`?PutString@TiXmlBase@@KAX...`) |
| `TiXmlText::Clone`, `Blank` (public) | protected |
| `TiXmlDocument::Clone` (public) | protected |
| `errorString_`, `::entity` | `TiXmlBase::errorString`, `TiXmlBase::entity` |
| `destroy_attribute` | (implicit `TiXmlAttribute::~TiXmlAttribute`) |

0x42d820 keeps the slice's name `TiXmlString::quit`. It is the same code as `~TiXmlString` (ICF).

## Header conflicts resolved (one tinystr.h + tinyxml.h)
- **ToXxx vtable order.** The slice headers disagree. s0042d360/s0042dcb0 declare each pair
  non-const first, and s0042e4f0 (with s00430190/s00430860/s00431050) declares it const first. Each
  slice also called a different overload: both sets of byte-exact code call slot 4 for `ToDocument`
  and slot 6 for `ToElement`. The module uses TinyXML's declaration (the six const casts, then the six
  non-const ones). cl puts each overload pair non-const first, so slot 4 is `ToDocument()`. The
  original's `GetDocument`/`NextSiblingElement`/`FirstChildElement` call the non-const casts, which is
  why they are the non-const 2.4 bodies (with the 2.5-style const forwarding, 4 functions were 2 bytes
  off).
- **`Parse` signature:** `void*` in s0042d360/s0042dcb0, `(TiXmlParsingData*, TiXmlEncoding)` in the
  others. The module uses the real signature.
- **Error ids:** a global enum in the slices, `TiXmlBase`'s nested enum in TinyXML 2.4 (module). The
  values are the same.
- **`TiXmlString::size_type`:** `unsigned` vs `size_t`; it is `size_t`.
- **`TiXmlAttributeSet::Find`:** one `TiXmlAttribute* Find(const TIXML_STRING&) const`.
  `TiXmlElement::Parse` passes `attrib->NameTStr()`.
- **Class layouts:** all slices agree. Node 0x2c bytes (`parent` +0x10 ... `next` +0x28), attribute
  0x24, element `attributeSet` +0x2c, document `error` +0x2c ... `useMicrosoftBOM` +0x44.
- **`TiXmlParsingData`:** private to tinyxmlparser.cpp, as in TinyXML. Its constructor is private, so
  only the friend `TiXmlDocument` can build one.
- **`TiXmlHandle`:** 2.4's accessors (`Node()`, `Element()`, `Text()`, `Unknown()`). The game slice
  s0050fc60 calls a `ToElement()` (2.5 spelling) that is inline in its own header.
- The game-code slices that carry their own copies (s004d2df0, s0050fc60) only use members declared
  here. They were not changed.

## Tooling notes
- **Audit symbol lookup.** `resaudit_all.py` finds our symbol for a `src:` row through the module's
  `// @ 0x` markers or `symbols/slices/<module>.txt`, not through the module manifest. A `// @` marker
  followed by a comment instead of a definition silently resolved to the *next* definition, which gave
  false disagree rows, and `??4`/`??Y`/`??_G` cannot be named by a marker. So those addresses carry
  plain `// 0x...:` comments, and the exact names are in `symbols/slices/tinystr.txt`, `tinyxml.txt`
  and `tinyxmlparser.txt` (new files, outside src/tinyxml; matching.md prescribes symbols lines for
  compiler-generated functions). Logged in docs/Papercuts.md.
- **chk.py pairing.** For byte-identical siblings chk.py may pair by "other" with the wrong name:
  `??4` for `??Y`, `??_GTiXmlComment` for `??_GTiXmlText`/`??_GTiXmlUnknown`, `??_GTiXmlDeclaration`
  for `??_GTiXmlNode`. The manifest rows name the right symbol for each address, and run_all and the
  audit confirm them.
