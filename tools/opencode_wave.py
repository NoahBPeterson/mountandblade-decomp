#!/usr/bin/env python3
"""Drive one byte-matching wave of opencode sessions to completion, one session per source module.

usage: opencode_wave.py <batch> [--model cheaperinference/deepseek-v4.1-flash] [--prompt docs/opencode_bm_prompt.txt]
                        [--concurrent 8] [--hang 900] [--cooldown 120] [--max-nudges 3] [--interval 30] [--status]

Batch: work/batches/<batch>.json, a list of {"id": obj stem, "module": "src/.../stem", "vas": [...], "rows": "text"}; optional
"obj" (the stem, when id differs), "prompt" (another template) and "after" (an id that must have finished first, because
two sessions must not share a module).
The prompt template takes {MODULE}, {OBJ} and {ROWS}. A session is finished when a turn ends with WAVE-DONE in its text.

Each pass: starts sessions up to the concurrency limit (re-read from work/opencode/wave_<batch>.conc, so it can change
while running), re-prompts a session whose turn ended without WAVE-DONE (at most --max-nudges times, then it is abandoned),
and interrupts and re-prompts one whose model call has been open for longer than --hang seconds. State and per-session
cost go to work/opencode/wave_<batch>.json; the broker reads the session ids from work/opencode/sessions_<batch>.txt.
"""
import argparse, base64, json, os, re, subprocess, sys, time, urllib.error, urllib.parse, urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DONE = "WAVE-DONE"
NUDGE = ("Continue from the files as they are now. Finish the remaining rows or give up on them under the budget in your "
         "instructions, then end with the report and the line WAVE-DONE.")


def service():
    for _ in range(10):
        m = re.search(r"http://[0-9.:]+", subprocess.run(["opencode", "service", "status"], capture_output=True, text=True).stdout)
        if m:
            pw = json.load(open(os.path.expanduser("~/.config/opencode/service.json")))["password"]
            return m.group(0), "Basic " + base64.b64encode(("opencode:" + pw).encode()).decode()
        time.sleep(1)
    sys.exit("the opencode service is not running")


URL, AUTH = service()
LOC = "location%5Bdirectory%5D=" + urllib.parse.quote(ROOT)


def api(method, path, body=None):
    req = urllib.request.Request(URL + path + ("&" if "?" in path else "?") + LOC, method=method,
                                 data=json.dumps(body).encode() if body is not None else None,
                                 headers={"Authorization": AUTH, "content-type": "application/json"})
    with urllib.request.urlopen(req, timeout=60) as r:
        return json.loads(r.read() or b"{}")


def messages(sid, limit):
    return api("GET", "/api/session/%s/message?limit=%d" % (sid, limit)).get("data", [])


def text_of(msg):
    return "\n".join(c.get("text", "") for c in msg.get("content") or [] if c.get("type") == "text")


def usage(sid):
    """Cost and token totals over the session's whole message list (it pages by cursor)."""
    cost = calls = inp = cached = out = 0
    cursor = None
    while True:
        d = api("GET", "/api/session/%s/message?limit=200%s" % (sid, "&cursor=" + urllib.parse.quote(cursor) if cursor else ""))
        rows = d.get("data", [])
        for m in rows:
            if m.get("type") == "assistant":
                t = m.get("tokens") or {}
                cost += m.get("cost") or 0
                calls += 1
                inp += t.get("input") or 0
                out += (t.get("output") or 0) + (t.get("reasoning") or 0)
                cached += (t.get("cache") or {}).get("read") or 0
        cursor = (d.get("cursor") or {}).get("next")
        if not rows or not cursor or len(rows) < 200:
            return {"cost": cost, "calls": calls, "input": inp, "cached": cached, "output": out}


def status(state):
    tot = {"cost": 0, "calls": 0, "input": 0, "cached": 0, "output": 0}
    by = {}
    for k, s in sorted(state.items(), key=lambda kv: kv[1].get("started", 0)):
        by[s["state"]] = by.get(s["state"], 0) + 1
        u = s.get("usage") or {}
        for f in tot:
            tot[f] += u.get(f, 0)
        print("%-24s %-9s nudges %d calls %4d $%.3f" % (k, s["state"], s.get("nudges", 0), u.get("calls", 0), u.get("cost", 0)))
    share = tot["cached"] / max(1, tot["cached"] + tot["input"])
    print(by, "total $%.2f, %d calls, cache share %.0f%%, output tokens %d" % (tot["cost"], tot["calls"], 100 * share, tot["output"]))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("batch")
    ap.add_argument("--model", default="cheaperinference/deepseek-v4.1-flash")
    ap.add_argument("--prompt", default="docs/opencode_bm_prompt.txt")
    ap.add_argument("--concurrent", type=int, default=8)
    ap.add_argument("--hang", type=int, default=900)
    ap.add_argument("--cooldown", type=int, default=120)
    ap.add_argument("--max-nudges", type=int, default=3)
    ap.add_argument("--interval", type=int, default=30)
    ap.add_argument("--status", action="store_true")
    ap.add_argument("--revive", action="store_true",
                    help="put abandoned sessions back to running (nudges reset) and continue them with their whole context. "
                         "They stay on the model they were created with: the service ignores a prompt's model field and "
                         "a PATCH of the session's model (tested 2026-11-10), so a revive bills at the old model's price")
    a = ap.parse_args()
    W = lambda *p: os.path.join(ROOT, "work", "opencode", *p)
    state_file, conc_file, sess_file = W("wave_%s.json" % a.batch), W("wave_%s.conc" % a.batch), W("sessions_%s.txt" % a.batch)
    batch = json.load(open(os.path.join(ROOT, "work", "batches", a.batch + ".json")))
    state = json.load(open(state_file)) if os.path.exists(state_file) else {}
    if a.status:
        return status(state)
    if not os.path.exists(conc_file):
        open(conc_file, "w").write(str(a.concurrent))
    tmpl = open(os.path.join(ROOT, a.prompt)).read()
    prov, mid = a.model.split("/", 1)
    MODEL = {"providerID": prov, "id": mid}
    if a.revive:
        for s in state.values():
            if s.get("state") == "abandoned":
                s.update(state="running", nudges=0, last_prompt=0, revived=a.model)

    def save():
        tmp = state_file + ".tmp"
        json.dump(state, open(tmp, "w"), indent=0)
        os.replace(tmp, state_file)
        open(sess_file, "w").write("\n".join(s["sid"] for s in state.values() if s.get("sid")) + "\n")

    def start(e):
        s = api("POST", "/api/session", {"title": "bm %s" % e["id"], "location": {"directory": ROOT},
                                         "model": {"providerID": prov, "id": mid}})
        sid = s.get("id") or s.get("data", {}).get("id")
        state[e["id"]] = {"sid": sid, "state": "running", "nudges": 0, "started": time.time(), "last_prompt": time.time()}
        save()                 # the broker must know the session before its first request
        t = open(os.path.join(ROOT, e["prompt"])).read() if e.get("prompt") else tmpl
        api("POST", "/api/session/%s/prompt" % sid, {"text": t.replace("{MODULE}", e["module"]).replace("{OBJ}", e.get("obj", e["id"]))
                                                     .replace("{ROWS}", e["rows"]), "model": MODEL})
        print(time.strftime("%H:%M:%S"), "start", e["id"], sid, flush=True)

    n = 0
    while True:
        n += 1
        try:
            running = 0
            for k, s in state.items():
                if s["state"] != "running":
                    continue
                msgs = messages(s["sid"], 4)
                top = msgs[0] if msgs else {}
                last = next((m for m in msgs if m.get("type") == "assistant"), None)
                ended = top.get("type") == "idle"
                now = time.time()
                if ended and last is not None and DONE in text_of(last):
                    s["state"] = "done"
                    s["usage"] = usage(s["sid"])
                    s["report"] = text_of(last)[-6000:]
                    print(time.strftime("%H:%M:%S"), "done", k, "$%.3f" % s["usage"]["cost"], flush=True)
                elif ended and now - s["last_prompt"] > a.cooldown:
                    if s["nudges"] >= a.max_nudges:
                        s["state"] = "abandoned"
                        s["usage"] = usage(s["sid"])
                        print(time.strftime("%H:%M:%S"), "abandoned", k, flush=True)
                    else:
                        s["nudges"] += 1
                        s["last_prompt"] = now
                        api("POST", "/api/session/%s/prompt" % s["sid"], {"text": NUDGE, "model": MODEL})
                        print(time.strftime("%H:%M:%S"), "nudge", k, s["nudges"], flush=True)
                elif last and not (last.get("time") or {}).get("completed") and now - (last["time"]["created"] / 1000) > a.hang:
                    print(time.strftime("%H:%M:%S"), "hung", k, flush=True)
                    api("POST", "/api/session/%s/interrupt" % s["sid"], {})
                    s["last_prompt"] = now
                    api("POST", "/api/session/%s/prompt" % s["sid"], {"text": NUDGE, "model": MODEL})
                if s["state"] == "running":
                    running += 1
                    if n % 10 == 0:
                        s["usage"] = usage(s["sid"])
            try:
                limit = int(open(conc_file).read().strip())
            except (OSError, ValueError):
                limit = a.concurrent
            for e in batch:
                if running >= limit:
                    break
                if e["id"] not in state and state.get(e.get("after"), {"state": "done"})["state"] in ("done", "abandoned"):
                    start(e)
                    running += 1
            save()
            if not running and all(e["id"] in state for e in batch):
                print(time.strftime("%H:%M:%S"), "wave finished", flush=True)
                status(state)
                return
        except (urllib.error.URLError, OSError, KeyError, ValueError) as ex:
            print(time.strftime("%H:%M:%S"), "pass failed:", repr(ex)[:200], flush=True)
        time.sleep(a.interval)


if __name__ == "__main__":
    main()
