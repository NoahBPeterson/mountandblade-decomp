## W8 0x4556d0 il --at  [bug]
command: c2.py il 0x4556d0 --src work/match/w8_trim.cpp --at @MscRVO@4,@Color@4  (also Inline,GoptCleanUp,... and @Schedule@4)
saw: "no c2 function @MscRVO@4" / "no c2 function Inline" after a full traced compile; empty output. icount.py's PIPE names (MscRVO, Inline, ...) are not accepted by il --at, and the doc example uses @Color@4 style. Which spelling works for pre-Color passes is not documented.
expected: --at accepts the PIPE names, or the error lists valid names, and fails before the 3-minute trace.
## W8 regs --blocks  [papercut]
command: c2.py regs 0x4556d0 --src ... --blocks
saw: usage error only after the full 3 minute compile ("--blocks: expected one argument", needs LO-HI). A bare --blocks should mean all blocks, or argparse should run before the compile.
## W8 regs table  [papercut]
saw: the pop table starts at pop 227 (`this`); the --export json holds 166 pops numbered 1.. and the text table renumbers them. --why block numbers are pass-specific, so tying a piece (pop 346) to the web it was cut from needs the json (decisions: mark/piece records with gr addresses that are REUSED across passes, so chasing a parent chain by gr is ambiguous).
expected: a `--chain imm=-1` that prints the web's history (cut at pop N by neighbour X, piece blocks) in one place.
## W8 immcand doc  [success]
saw: section 2.4 (join reached from a path that used the constant and one that did not: load hoisted to the if-test block) explained the `or eax,-1` above the jne in one read, and a dummy `if (horse == -1)` right after the join reproduced it. A tool that answers "which pop cut this constant's web and why" is the missing half.
## W8 trimmed TU  [success]
work/waves/W8/trim.py builds a trimmed agent_logic.cpp (2850 lines, keeps types/inlines/templates/update_controls, drops other out-of-line bodies) in one pass; chk.py on it gives byte-identical figures (8252/1813/124) in 7 s instead of minutes. Worth building into c2.py as --trim.
## W8 brief  [papercut]
modguard lives at tools/contrib/modguard.py, not tools/matching/modguard.py.
## W8-S slotwant  [success]
saw: --diag + the doc's wants were right; hbuf[64] x2 + namebuf[128] gave frame 1068 -> 1004 and aligned 3881 -> 3793 on the real row. The "why: refused for size" line plus the bridge idea made it a one-shot.
## W8-S slotwant wrapper  [papercut]
slotwant is not in tools/contrib/c2.py (TOOLS), so no --src/--func/flags fill-in; I wrote work/waves/W8/sw.sh. It also needs `--c2 13.10` placed before `--` (not an argparse option; c2build takes it from argv), else the cl headers differ and agent_logic.cpp fails with C3190 on vector<int>::_Xlen. Without the dxsdk /I the base compile fails the same way as for c2.py. Suggest: add slotwant to c2.py.
## W8-S slotwant noise  [papercut]
--diag on this row prints ~90 derived pair wants, nearly all shift noise ("apart:$T183407,..." etc); a --top / --min-votes default would make it readable.
## W8-S tsweep  [papercut]
tsweep (bdiff) ranked changes that chk.py confirmed only when size stayed equal; e.g. imp = vel*-0.6f scored better in bdiff while chk aligned went 3417 -> 3903. Printing both size and aligned in the metric line would help.
## W8-S blood_burst  [finding]
insdiff's "sub esp,0x14 + struct copy" vs 5 pushes was the cue: the original's callee takes vector4 by value (the old slice already had it). No c2scope tool says "argument passed as struct copy" directly.
