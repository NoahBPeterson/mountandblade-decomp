#!/bin/sh
# usage: sw.sh SRC [slotwant args...]   (run from anywhere; one compile at a time)
R=/Users/noahpeterson/Documents/Projects/mountandblade-decomp
SRC=$1; shift
export MB_RC_ALLOW_LOCAL=1 C2S_OFF=1
cd $R/../vs2005-cpp
exec .venv/bin/python tools/slotwant.py $R/$SRC --func '?process_blow@Agent@@QAEXM@Z' --image $R/work/mb.analysis.bin --va 0x457b40 "$@" --jobs 1 --c2 13.10 -- /O2 /MT /Gy /TP /I$R/match/include /I$R/src /I$R/work/ext/dxsdk_jun10/DXSDK/Include
