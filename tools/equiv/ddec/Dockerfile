# STOKE (DDEC data-driven equivalence checker) for tools/equiv/ddec_layer.py.
#
# Copied from spore-decomp tools/equiv/ddec/ at commit 82de4ab6c (the same owner). The three *.patch files next to
# this one are byte-identical copies of spore-decomp's (no header comment in them on purpose: identical files give
# identical build layers, so a machine that built spore's image builds this one from its cache).
#
# Build (x86-64 Linux/Windows Docker, or macOS arm64 under Rosetta/QEMU), from the repository root:
#   docker build --platform linux/amd64 -t mb-stoke-ddec:98d8a0f-1 tools/equiv/ddec
#
# Base: the STOKE project's own image (Ubuntu 14.04, source at commit 98d8a0f with Z3 prebuilt), pinned by digest.
# Patches (applied in name order):
#   1-learner-eqonly.patch   DDEC_EQ_ONLY=1 restricts the invariant learner to linear equalities + memory equality
#                            (the DDEC paper's invariants). Sound: every learned candidate is still SMT-checked.
#   2-cutpoints-trace.patch  DDEC_TRACE=1 prints one line per testcase while mining cutpoint data (debugging).
#   3-ddec-hints.patch       DDEC_HINTS=<file> rechecks a proof: cutpoints and invariants are read back from the
#                            text STOKE printed in its SUMMARY; nothing is learned, nothing is dropped, no other
#                            cutpoints are tried, so wrong hints make the check fail.
#   x87/4-x87-lifter.patch   (M&B, image tag -2) for tools/equiv/x87lift.py, which lowers x87 code to SSE2 scalar
#                            doubles that STOKE models as uninterpreted functions (add_double, comisd, ...).
#                            STOKE_ACKERMANN=1 replaces them by Ackermann's reduction with x87's algebra: equal
#                            arguments give equal results; fadd/fmul are commutative; fcom of swapped arguments
#                            exchanges less/greater, of equal arguments gives equal or unordered (Z3 4.4 hangs on
#                            f(x[63:0]) = f(y[63:0]) when only the bits say x = y). Register equalities are stated per
#                            64-bit lane (the same Z3 weakness), and the constant-propagation rules x + 0 = x and
#                            x - 0 = x are dropped (not IEEE: -0 + 0 = +0; not x87 under a narrower precision
#                            control). It sits in its own directory and is applied after the build of patches 1-3,
#                            so the earlier layers stay cached (and shared with spore).
#   Build:  docker build --platform linux/amd64 -t mb-stoke-ddec:98d8a0f-2 tools/equiv/ddec
FROM --platform=linux/amd64 stanfordpl/stoke@sha256:c50f06b36602b4282c0730da58c119d809edd69732923dd137860aaa931a599d

USER stoke
WORKDIR /home/stoke/stoke

COPY --chown=stoke:stoke *.patch /home/stoke/patches/
RUN test "$(git rev-parse --short=7 HEAD)" = 98d8a0f \
 && for p in /home/stoke/patches/*.patch; do git apply -v "$p" || exit 1; done

# Fixed configuration instead of ./configure.sh, which picks the platform from the build machine's /proc/cpuinfo:
# "nehalem" (no AVX) runs everywhere, including under Rosetta. -DSTOKE_DEBUG_DDEC makes DDEC print its SUMMARY
# (cutpoints and invariants), which the layer parses. CVC4 is not needed (Z3 is the solver).
RUN printf 'STOKE_PLATFORM="nehalem"\nBUILD_TYPE=release\nMISC_OPTIONS= -DSTOKE_DEBUG_DDEC\nNOCVC4=1\n' > .stoke_config

# x64asm generates src/opcode.enum, which the pin tool includes; with parallel make the pin tool can start before
# it exists (a race). Build x64asm alone first, then the rest of the external libraries, then the one tool we use.
RUN make x64asm EXT_OPT=release \
 && make external EXT_OPT=release \
 && make -j"$(nproc)" bin/stoke_debug_verify \
 && test -x bin/stoke_debug_verify

COPY --chown=stoke:stoke x87/*.patch /home/stoke/patches/x87/
# STOKE's Makefile does not track header dependencies: every file that includes a patched header is touched so
# that it is compiled again.
RUN for p in /home/stoke/patches/x87/*.patch; do git apply -v "$p" || exit 1; done \
 && touch $(grep -rlE '(packed_handler|simple_handler|handlers|z3solver)\.h' src tools --include=*.cc) \
 && make -j"$(nproc)" bin/stoke_debug_verify \
 && test -x bin/stoke_debug_verify

ENV PATH=/home/stoke/stoke/bin:$PATH
WORKDIR /work
CMD ["sleep", "infinity"]
