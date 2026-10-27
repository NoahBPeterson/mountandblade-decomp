#!/usr/bin/env python3
"""Audit virtual slot signatures for drift and stack-cleanup mismatches.

NOTE: this is the *textual* scan, so it reports every alias spelling (`mat3*` vs `rgl::mat3*`) as a
drift and every cdecl/stdcall declaration against a `ret N` comment. Use it to find which headers
talk about a slot at all; for an actionable list run `tools/matching/sig_vs_original.py`, which
compares our mangled names with the original's at the same VA and buckets only the real classes
(VIRTUAL / CONVENTION / ARITY). Its current output is 20 rows, not 1693.

The decompilation carries vtable slot annotations in trailing ``//`` comments,
e.g. ``// +0x98``, ``// +0xa4 (hidden return pointer ...)`` or
``// 0x487b00``.  The same slot is sometimes declared in several headers (or
in a source's local copy of a class) with disagreeing parameter lists.  This
tool parses those declarations, normalises their parameter lists and reports
two classes of finding.

Heuristics
==========

SIGNATURE DRIFT
---------------
A *cluster* is a set of declarations with the same method name and the same
slot/address annotation.  A cluster is reported when its declarations use more
than one normalised parameter list.  Parameter names are stripped; pointer,
reference and const/volatile qualifiers are kept.  For example::

    virtual void set_color(const unsigned* color);  // +0x98
    virtual void set_color(unsigned* c);            // +0x98

normalise to ``const unsigned*`` and ``unsigned*`` and therefore disagree.
Unnamed placeholder slots (``v00``, ``s08``, ``d22``, ...) are ignored, as are
declarations without a slot annotation.  Only declarations whose *name*
matches exactly are compared (``set_color98`` does not cluster with
``set_color``).

Live examples found by hand (2026-10):
  * ``set_color`` at ``+0x98`` -- ``inventory_panel/char_sheet.cpp`` declares
    ``const unsigned*`` while ``ui/mission_ui.h``, ``ui_widget.h``,
    ``mission_ui2/mission_3.h`` and ``mission_ui2/field_panel.h`` declare
    ``unsigned*``.  (The ``char_sheet.cpp`` summary comment still spells the
    slot as ``set_color(int)``, which is the bug this audit is meant to catch.)
  * ``decode_utf8`` -- ``ui/ui_widgets_a.cpp`` declares
    ``(const tstring*, int*, int*, const int*)`` while
    ``mesh_slot/slot.cpp`` declares ``(int*, const tstring*, int*, const int*)``.
    The argument order (and therefore the register assignment) disagrees.

RET MISMATCH
------------
A ``__thiscall``/``__stdcall`` callee pops exactly its stack arguments, so a
``ret N`` annotation must equal ``4 * stack_args``.  The stack arguments are
the declared parameters plus one for a hidden return pointer when the comment
mentions one (``hidden return``/``hidden-return``).  ``__cdecl`` and varargs
declarations are excluded when the comment says so.  For example::

    void add_order_arrow(int id);              // 0x49bea0 (ret 4)
    void add_order_arrow(int id, int mode);    // 0x49bea0 (ret 4)

The first is fine (4 == 4*1); the second is a mismatch (4 != 4*2).  The
current tree has only the one-argument form, so it must not be flagged.

Usage
=====
    python3 tools/matching/slot_sig_audit.py [--filter SUBSTR] [--json OUT] [--all]
    python3 tools/matching/slot_sig_audit.py --self-test

Exit status is 1 when findings exist, 0 otherwise.
"""

import argparse
import json
import os
import re
import sys

TYPE_KEYWORDS = {
    'int', 'unsigned', 'signed', 'short', 'long', 'float', 'double', 'char',
    'void', 'bool', 'const', 'volatile', 'struct', 'class', 'enum', 'union',
    'auto', 'register', 'static', 'extern', 'inline', 'virtual', 'typename',
    'template', 'mutable', 'explicit', 'friend',
}

# Names such as v00, s08, d22 are unnamed vtable placeholders, not real methods.
PLACEHOLDER_RE = re.compile(r'^[a-z][0-9a-f]+$')

# Keywords that can never start a declaration we want to audit.
SKIP_WORDS = {
    'return', 'if', 'for', 'while', 'switch', 'catch', 'sizeof', 'new',
    'delete', 'throw', 'else',
}


def split_code_comment(line):
    """Split a source line into code and its ``//`` comment."""
    in_string = False
    escape = False
    for i, ch in enumerate(line):
        if escape:
            escape = False
            continue
        if ch == '\\':
            escape = True
            continue
        if ch == '"':
            in_string = not in_string
            continue
        if not in_string and ch == '/' and i + 1 < len(line) and line[i + 1] == '/':
            return line[:i], line[i:]
    return line, ''


def split_by_semicolons(text):
    """Split on top-level semicolons, respecting (), [], {}, <> and strings."""
    parts = []
    depth = 0
    current = []
    in_string = False
    escape = False
    for ch in text:
        if escape:
            current.append(ch)
            escape = False
            continue
        if ch == '\\':
            current.append(ch)
            escape = True
            continue
        if ch == '"':
            in_string = not in_string
            current.append(ch)
            continue
        if in_string:
            current.append(ch)
            continue
        if ch in '<({[':
            depth += 1
        elif ch in '>)}]':
            depth -= 1
        if ch == ';' and depth == 0:
            parts.append(''.join(current))
            current = []
        else:
            current.append(ch)
    if current:
        parts.append(''.join(current))
    return parts


def split_args(text):
    """Split a parameter list on top-level commas."""
    args = []
    depth = 0
    current = []
    in_string = False
    escape = False
    for ch in text:
        if escape:
            current.append(ch)
            escape = False
            continue
        if ch == '\\':
            current.append(ch)
            escape = True
            continue
        if ch == '"':
            in_string = not in_string
            current.append(ch)
            continue
        if in_string:
            current.append(ch)
            continue
        if ch in '<(':
            depth += 1
        elif ch in '>)':
            depth -= 1
        if ch == ',' and depth == 0:
            args.append(''.join(current))
            current = []
        else:
            current.append(ch)
    if current:
        args.append(''.join(current))
    return [a.strip() for a in args if a.strip()]


def normalize_type(arg):
    """Strip the argument name, keep the type's pointer/ref/const qualifiers."""
    arg = re.sub(r'\s+', ' ', arg.strip())
    # Leave function-pointer parameters untouched; they are not part of the
    # slot-drift heuristics and their names are ambiguous.
    if '(*' in arg:
        return arg
    # "const tstring * s" -> base "const tstring", ptr "*", name "s".
    m = re.match(r'^(.*?)\s*([*&]+)\s*(\w+)$', arg)
    if m:
        base = m.group(1).strip()
        ptr = m.group(2)
        name = m.group(3)
        base_tokens = base.split()
        if base_tokens and base_tokens[-1] in {'struct', 'class', 'enum', 'union', 'namespace'}:
            # "struct Foo * x" -- keep the type name as part of the type.
            return base + ' ' + ptr + ' ' + name
        return (base + ' ' + ptr) if base else ptr
    # No pointer/reference: "unsigned rgb" -> "unsigned"; "unsigned long" stays.
    tokens = arg.split()
    if len(tokens) > 1 and tokens[-1] not in TYPE_KEYWORDS:
        if tokens[-2] not in {'struct', 'class', 'enum', 'union', 'namespace', 'const', 'volatile'}:
            return ' '.join(tokens[:-1])
    return ' '.join(tokens)


class Decl:
    """One parsed function/virtual declaration."""
    __slots__ = ('file', 'line', 'cls', 'slot', 'name', 'params', 'ret',
                 'params_str', 'raw')

    def __init__(self, file, line, cls, slot, name, params, ret, params_str, raw):
        self.file = file
        self.line = line
        self.cls = cls
        self.slot = slot
        self.name = name
        self.params = params
        self.ret = ret
        self.params_str = params_str
        self.raw = raw

    def __repr__(self):
        return (f"{self.file}:{self.line} {self.cls or ''}::{self.name}"
                f"({', '.join(self.params)}) slot={self.slot} ret={self.ret}")


def parse_file(filepath):
    """Parse all function declarations from a .h/.cpp file."""
    results = []
    with open(filepath, 'r', errors='replace') as f:
        lines = f.readlines()

    class_stack = []      # [(class_name, brace_depth), ...]
    depth = 0
    pending_class = None

    for i, raw_line in enumerate(lines):
        code, comment = split_code_comment(raw_line)

        # A slot/ret annotation can sit on the previous line (e.g. slot.cpp's
        # "// @ 0x487b00" above the decode_utf8 definition).
        prev_comment = ''
        if not comment and i > 0:
            _, prev_comment = split_code_comment(lines[i - 1])
        active_comment = comment if comment else prev_comment

        slot = None
        ret = None
        if active_comment:
            slot_m = re.search(r'([+]?\s*0x[0-9a-fA-F]+)', active_comment)
            if slot_m:
                slot = slot_m.group(1).replace(' ', '').lower()
            ret_m = re.search(r'\bret\s+(0x[0-9a-fA-F]+|\d+)\b', active_comment,
                              re.IGNORECASE)
            if ret_m:
                ret = ret_m.group(1).lower()

        # Track struct/class bodies.  A definition is either "struct Foo {"
        # or "struct Foo" followed by a "{" on the next line.
        for m in re.finditer(r'\b(struct|class)\s+([A-Za-z_]\w*)\b', code):
            end_pos = m.end()
            rest = code[end_pos:].strip()
            next_is_brace = (not rest and i + 1 < len(lines)
                             and lines[i + 1].strip().startswith('{'))
            if rest.startswith('{') or next_is_brace:
                pending_class = m.group(2)

        for ch in code:
            if ch == '{':
                depth += 1
                if pending_class:
                    class_stack.append((pending_class, depth))
                    pending_class = None
            elif ch == '}':
                if class_stack and class_stack[-1][1] == depth:
                    class_stack.pop()
                depth -= 1

        cls = class_stack[-1][0] if class_stack else None

        for part in split_by_semicolons(code):
            part = part.strip()
            if not part:
                continue
            # Drop trailing cv/override/noexcept/final and pure-virtual "= 0".
            part = re.sub(r'\b(const|override|noexcept|final)\s*(?:=\s*0)?$',
                          '', part).strip()
            part = re.sub(r'=\s*0\s*$', '', part).strip()

            # Find "name(params)".  Non-greedy so __declspec(noinline) does
            # not swallow the real parameter list.
            m = re.search(r'\b([A-Za-z_]\w*)\s*\(([^()]*)\)\s*(?:const)?\s*'
                          r'(?:;|\{|$)', part)
            if not m:
                continue
            name = m.group(1)
            params_str = m.group(2).strip()
            if name in SKIP_WORDS:
                continue

            prefix = part[:m.start()].strip()
            if not prefix or prefix.endswith(('=', '.', '->', '(', ',')):
                continue
            if ';' in prefix:
                continue
            if any(k in prefix.split() for k in SKIP_WORDS):
                continue

            params = []
            if params_str and params_str != 'void':
                for arg in split_args(params_str):
                    params.append(normalize_type(arg))

            results.append(Decl(filepath, i + 1, cls, slot, name, params, ret,
                                params_str, raw_line.strip()))

    return results


def analyze(decls):
    """Return (clusters, ret_mismatches) for all parsed declarations."""
    groups = {}
    for d in decls:
        if not d.name or d.slot is None:
            continue
        if PLACEHOLDER_RE.match(d.name):
            continue
        groups.setdefault((d.name, d.slot), []).append(d)

    clusters = []
    for (name, slot), ds in groups.items():
        param_sets = set(tuple(d.params) for d in ds)
        files = set(d.file for d in ds)
        clusters.append({
            'name': name,
            'slot': slot,
            'params': param_sets,
            'files': files,
            'decls': ds,
            'is_drift': len(param_sets) > 1,
        })

    # "sorted by how many files disagree" -- widest disagreement first.
    clusters.sort(key=lambda c: (-len(c['files']), -len(c['params']),
                                 c['name'], str(c['slot'])))

    mismatches = []
    for d in decls:
        if not d.ret:
            continue
        comment_lower = d.raw.lower()
        # __cdecl and varargs callers clean the stack; the annotation is not a
        # thiscall/stdcall ret N.
        if 'cdecl' in comment_lower or 'varargs' in comment_lower:
            continue
        if '...' in d.params_str or 'varargs' in d.params_str:
            continue

        stack_params = len(d.params)
        if 'hidden ret' in comment_lower or 'hidden return' in comment_lower:
            stack_params += 1

        expected = 4 * stack_params
        try:
            actual = int(d.ret, 0)
        except ValueError:
            continue

        if actual != expected:
            mismatches.append({
                'name': d.name,
                'slot': d.slot,
                'file': d.file,
                'line': d.line,
                'params': d.params,
                'ret': d.ret,
                'expected': expected,
                'decl': d,
            })

    return clusters, mismatches


def self_test():
    """Synthetic check that add_order_arrow(int) is clean and (int,int) is not."""
    import tempfile
    snippet = """
struct Test {
    virtual void add_order_arrow(int id);           // 0x49bea0 (ret 4)
    virtual void add_order_arrow(int id, int mode); // 0x49bea0 (ret 4)
    virtual void good(int id);                      // 0x49bea0 (ret 4)
    virtual void bad(int id, int mode);             // 0x49bea0 (ret 4)
};
"""
    fd, path = tempfile.mkstemp(suffix='.h')
    try:
        with os.fdopen(fd, 'w') as f:
            f.write(snippet)
        decls = parse_file(path)
        clusters, mismatches = analyze(decls)

        print("SELF-TEST:")
        for d in decls:
            if d.name == 'add_order_arrow':
                print(f"  {d.name}({', '.join(d.params)}) ret={d.ret} "
                      f"line={d.line}")

        found_one_arg_clean = False
        found_two_arg_bad = False
        for d in decls:
            if d.name == 'add_order_arrow' and d.params == ['int']:
                if not any(m['decl'] is d for m in mismatches):
                    found_one_arg_clean = True
        for m in mismatches:
            if m['name'] == 'add_order_arrow':
                print(f"  RET MISMATCH: {m['name']}"
                      f"({', '.join(m['params'])}) ret={m['ret']} "
                      f"expected={m['expected']} line={m['line']}")
                if m['expected'] == 8:
                    found_two_arg_bad = True

        found_drift = False
        for c in clusters:
            if c['name'] == 'add_order_arrow' and c['is_drift']:
                print(f"  SIGNATURE DRIFT: {c['name']} slot={c['slot']} "
                      f"params={c['params']}")
                found_drift = True

        assert found_one_arg_clean, \
            "add_order_arrow(int) with ret 4 must NOT be flagged"
        assert found_two_arg_bad, \
            "add_order_arrow(int, int) with ret 4 must be flagged"
        assert found_drift, "add_order_arrow must be flagged as SIGNATURE DRIFT"
        print("Self-test passed.")
    finally:
        os.unlink(path)


def main():
    parser = argparse.ArgumentParser(
        description='Audit virtual slot signatures for drift and ret '
                    'mismatches.')
    parser.add_argument('--filter', '-f', default='',
                        help='Filter by substring in method name, file or class')
    parser.add_argument('--json', '-j', default='',
                        help='Write findings to JSON file')
    parser.add_argument('--all', '-a', action='store_true',
                        help='Show clusters that agree too')
    parser.add_argument('--root', '-r', default='.',
                        help='Repository root (default: .)')
    parser.add_argument('--self-test', action='store_true',
                        help='Run a synthetic self-test')
    args = parser.parse_args()

    if args.self_test:
        self_test()
        return

    src_dir = os.path.join(args.root, 'src')
    if not os.path.isdir(src_dir):
        print(f"Error: {src_dir} not found", file=sys.stderr)
        sys.exit(2)

    decls = []
    for dirpath, dirnames, filenames in os.walk(src_dir):
        # Skip third-party tinyxml.
        dirnames[:] = [d for d in dirnames if d != 'tinyxml']
        for fname in filenames:
            if fname.endswith('.h') or fname.endswith('.cpp'):
                filepath = os.path.join(dirpath, fname)
                try:
                    decls.extend(parse_file(filepath))
                except Exception as exc:  # noqa: BLE001 - best-effort audit
                    print(f"Warning: error parsing {filepath}: {exc}",
                          file=sys.stderr)

    clusters, mismatches = analyze(decls)

    if args.filter:
        needle = args.filter.lower()

        def _matches_cluster(c):
            if needle in c['name'].lower():
                return True
            for d in c['decls']:
                if needle in d.file.lower() or needle in (d.cls or '').lower():
                    return True
            return False

        def _matches_mismatch(m):
            return (needle in m['name'].lower() or needle in m['file'].lower()
                    or needle in (m['decl'].cls or '').lower())

        clusters = [c for c in clusters if _matches_cluster(c)]
        mismatches = [m for m in mismatches if _matches_mismatch(m)]

    drift = [c for c in clusters if c['is_drift']]
    shown = clusters if args.all else drift
    total_drift_rows = sum(len(c['decls']) for c in drift)

    print(f"Found {len(drift)} SIGNATURE DRIFT clusters "
          f"({total_drift_rows} rows) and {len(mismatches)} RET MISMATCH rows.")
    print()

    if shown:
        print("SIGNATURE DRIFT" if not args.all else "SIGNATURE CLUSTERS")
        print("=" * 15)
        for c in shown:
            if not args.all and not c['is_drift']:
                continue
            print(f"{c['name']}  ({c['slot']})  [{len(c['files'])} files]")
            for d in sorted(c['decls'], key=lambda x: (x.file, x.line)):
                cls_str = f" {d.cls}::" if d.cls else " "
                print(f"  {d.file}:{d.line}{cls_str}{d.raw}")
            print()

    if mismatches:
        print("RET MISMATCH")
        print("=" * 15)
        for m in sorted(mismatches,
                        key=lambda x: (x['name'], x['file'], x['line'])):
            print(f"{m['name']}  ({m['file']}:{m['line']})")
            params = ', '.join(m['params']) if m['params'] else '(none)'
            print(f"  declared params: {params}  ({len(m['params'])} stack "
                  f"params)")
            print(f"  comment ret N: {m['ret']}")
            print(f"  expected N: {m['expected']}")
            print(f"  {m['decl'].raw}")
            print()

    if args.json:
        payload = {
            'signature_drift': [
                {
                    'name': c['name'],
                    'slot': c['slot'],
                    'files': sorted(c['files']),
                    'decls': [
                        {
                            'file': d.file,
                            'line': d.line,
                            'class': d.cls,
                            'params': d.params,
                            'raw': d.raw,
                        }
                        for d in sorted(c['decls'],
                                        key=lambda x: (x.file, x.line))
                    ],
                }
                for c in drift
            ],
            'ret_mismatch': [
                {
                    'name': m['name'],
                    'slot': m['slot'],
                    'file': m['file'],
                    'line': m['line'],
                    'params': m['params'],
                    'ret': m['ret'],
                    'expected': m['expected'],
                    'raw': m['decl'].raw,
                }
                for m in sorted(mismatches,
                                key=lambda x: (x['name'], x['file'], x['line']))
            ],
        }
        with open(args.json, 'w') as f:
            json.dump(payload, f, indent=2)
        print(f"JSON written to {args.json}")

    sys.exit(1 if (drift or mismatches) else 0)


if __name__ == '__main__':
    main()
