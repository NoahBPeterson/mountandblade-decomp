#!/usr/bin/env python3
"""Make a runnable copy of the decrypted analysis image: entry point moved from the SteamStub
stub to the original entry point (OEP from the unpacker's JSON sidecar), checksum refreshed.
Every other byte is identical to the analysis image.

usage: runnable.py [<analysis.bin> <analysis.json> <out.exe>]
"""
import json, os, sys
import pefile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
W = lambda p: os.path.join(ROOT, p)


def main(src=W("work/mb.analysis.bin"), meta=W("work/mb.analysis.json"), dst=W("work/hybrid/mb.runnable.exe")):
    oep = json.load(open(meta))["oep"]
    pe = pefile.PE(src)
    pe.OPTIONAL_HEADER.AddressOfEntryPoint = oep - pe.OPTIONAL_HEADER.ImageBase
    pe.OPTIONAL_HEADER.CheckSum = pe.generate_checksum()
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    pe.write(dst)
    print("wrote %s (entry 0x%x)" % (dst, oep))


if __name__ == "__main__":
    main(*sys.argv[1:4])
