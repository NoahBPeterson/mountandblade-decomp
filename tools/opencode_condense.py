"""Condense an `opencode session export` JSON into an experiment log: the model's reasoning text, every source edit
(old -> new), and the measurement lines that followed (check.py / bm5o2_c.py / equiv.py verdicts).
usage: tools/opencode_condense.py <export.json> > <out.txt>"""
import json, re, sys

KEEP = re.compile(r"MATCH|closer|further|BROKE|aligned|identical \d+ / 590|^\s*(PASS|WEAK|FAIL|UNSUPPORTED)|mismatch|"
                  r"Where:|Values:|byte-exact", re.I)


def clip(s, n):
    s = s if isinstance(s, str) else json.dumps(s)
    return s if len(s) <= n else s[:n] + " ...[%d more chars]" % (len(s) - n)


d = json.load(open(sys.argv[1]))
out = []
for m in d["messages"]:
    if m.get("type") == "user":
        t = m.get("text") or ""
        out.append("=== USER: " + clip(t, 300))
        continue
    if m.get("type") != "assistant":
        continue
    for c in m.get("content") or []:
        if c.get("type") in ("text", "reasoning"):
            t = (c.get("text") or "").strip()
            if t:
                out.append(("THINK: " if c["type"] == "reasoning" else "SAY: ") + clip(t, 1500))
        elif c.get("type") == "tool":
            st = c.get("state") or {}
            inp = st.get("input") or {}
            res = st.get("output") or st.get("result") or "\n".join(x.get("text", "") for x in (st.get("content") or []) if isinstance(x, dict))
            if not isinstance(res, str):
                res = json.dumps(res)
            name = c.get("name")
            if name == "edit":
                out.append("EDIT %s\n--- old:\n%s\n+++ new:\n%s" % ((inp.get("filePath") or inp.get("path", "")), clip(inp.get("oldString", ""), 2500),
                                                                 clip(inp.get("newString", ""), 2500)))
                if res and "success" not in res.lower():
                    out.append("  edit result: " + clip(res, 200))
            elif name == "write":
                out.append("WRITE %s (%d chars)" % ((inp.get("filePath") or inp.get("path", "")), len(inp.get("content", "") or "")))
            elif name in ("bash", "shell"):
                cmd = inp.get("command", "")
                lines = [l for l in res.splitlines() if KEEP.search(l)]
                if any(k in cmd for k in ("check.py", "bm5o2_c.py", "equiv.py", "run_all.py", "task.py")):
                    out.append("RUN %s\n%s" % (clip(cmd, 300), "\n".join("  | " + clip(l, 220) for l in lines[:14])))
                else:
                    out.append("RUN %s" % clip(cmd, 200))
            elif name == "read":
                out.append("READ %s" % (inp.get("filePath") or inp.get("path", "")))
print("\n".join(out))
