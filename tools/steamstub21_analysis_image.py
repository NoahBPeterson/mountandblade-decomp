#!/usr/bin/env python3
"""Produce a NON-RUNNABLE analysis image of a SteamStub v2.1 x86 protected PE.

Port of Steamless' Variant21.x86 unpacker (header -> payload -> XTEA SteamDRMP.dll ->
DRMP offset block -> AES-CBC code section). The code section is decrypted in place;
the entry point is left on the stub and the original entry point is written to a
JSON sidecar. Output is for local analysis only.
"""
import json, re, struct, sys
import pefile
from Crypto.Cipher import AES


def steam_xor(buf: bytearray, key: int, start: int = 0) -> None:
    for x in range(start, len(buf) - 3, 4):
        v = struct.unpack_from("<I", buf, x)[0]
        struct.pack_into("<I", buf, x, v ^ key)
        key = v


def xtea_pass1(data: bytearray, keys) -> None:
    delta, m = 0x9E3779B9, 0xFFFFFFFF
    p1 = p2 = 0x55555555
    for x in range(0, len(data) - 7, 8):
        d1, d2 = struct.unpack_from("<II", data, x)
        v1, v2, s = d1, d2, (delta * 32) & m
        for _ in range(32):
            v2 = (v2 - ((((v1 << 4) & m ^ v1 >> 5) + v1) ^ (s + keys[s >> 11 & 3]))) & m
            s = (s - delta) & m
            v1 = (v1 - ((((v2 << 4) & m ^ v2 >> 5) + v2) ^ (s + keys[s & 3]))) & m
        struct.pack_into("<II", data, x, v1 ^ p1, v2 ^ p2)
        p1, p2 = d1, d2


def find(data: bytes, pat: str) -> int:
    rx = b"".join(b"." if t == "??" else re.escape(bytes([int(t, 16)])) for t in pat.split())
    m = re.search(rx, data, re.S)
    return m.start() if m else -1


def main(src, dst, meta_path):
    pe = pefile.PE(src)
    raw = open(src, "rb").read()
    base = pe.OPTIONAL_HEADER.ImageBase
    foff = lambda va: pe.get_offset_from_rva(va - base)
    ep = pe.OPTIONAL_HEADER.AddressOfEntryPoint
    epo = pe.get_offset_from_rva(ep)
    assert struct.unpack_from("<I", raw, epo - 4)[0] == 0xC0DEC0DE, "not SteamStub v2"
    # Stub prologue: mov [ebp-x], <header VA>; mov esi,..; mov ecx, <dwords>; ...; mov [ebp-y], <xor key>
    code = raw[epo:epo + 0x80]
    hdr_va = struct.unpack_from("<I", code, code.index(b"\xc7\x85") + 6)[0]
    ndw = struct.unpack_from("<I", code, code.index(b"\xb9") + 1)[0]
    i = code.index(b"\xc7\x85", code.index(b"\xb9"))
    key = struct.unpack_from("<I", code, i + 6)[0]
    hdr = bytearray(raw[foff(hdr_va):foff(hdr_va) + ndw * 4])
    steam_xor(hdr, key)
    (pay_key, pay_va, pay_size, appid) = struct.unpack_from("<IIII", hdr, 0x20)
    drmp_va_off, drmp_size_off, xtea_off = struct.unpack_from("<III", hdr, 0x3C)

    pay = bytearray(raw[foff(pay_va):foff(pay_va) + pay_size])
    steam_xor(pay, pay_key, 4)
    drmp_va = struct.unpack_from("<I", pay, drmp_va_off)[0]
    drmp_size = struct.unpack_from("<I", pay, drmp_size_off)[0]
    keys = struct.unpack_from("<%dI" % ((len(pay) - xtea_off) // 4), pay, xtea_off)
    drmp = bytearray(raw[foff(drmp_va):foff(drmp_va) + drmp_size])
    xtea_pass1(drmp, keys)

    fallback = False
    o = find(drmp, "8B ?? ?? ?? ?? ?? 89 ?? ?? ?? ?? ?? " * 5 + "8D ?? ?? ?? ?? ?? 05")
    if o < 0:
        o = find(drmp, "8B ?? ?? ?? ?? ?? 89 ?? ?? ?? ?? ?? " * 4 + "8B ?? ?? ?? ?? ?? 89 ?? ?? ?? ?? ?? 8B")
    if o < 0:
        o = find(drmp, "8B ?? ?? ?? ?? ?? 89 ?? ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? A3 ?? ?? ?? ?? "
                       "8B ?? ?? ?? ?? ?? A3 ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? A3 ?? ?? ?? ?? 8B")
        fallback = True
    assert o >= 0, "DRMP offset block not found"
    blk = bytes(drmp[o:o + 1024])
    at = lambda k: struct.unpack_from("<i", blk, k)[0]
    offs = [at(2), at(14), at(25 if fallback else 26), at(36 if fallback else 38),
            at(47 if fallback else 50), at(61 if fallback else 62), at(72 if fallback else 67)]
    offs.append(offs[6] + 16)
    u32 = lambda k: struct.unpack_from("<I", pay, k)[0]
    flags, oep, code_va, enc_size = u32(offs[0]), u32(offs[2]), u32(offs[3]), u32(offs[4])
    aes_key, aes_iv, stolen = bytes(pay[offs[5]:offs[5] + 32]), bytes(pay[offs[6]:offs[6] + 16]), bytes(pay[offs[7]:offs[7] + 16])

    out = bytearray(raw)
    if not flags & 0x04:
        iv = AES.new(aes_key, AES.MODE_ECB).decrypt(aes_iv)
        co = foff(code_va)
        plain = AES.new(aes_key, AES.MODE_CBC, iv).decrypt(stolen + raw[co:co + enc_size])
        out[co:co + enc_size] = plain[:enc_size]
    open(dst, "wb").write(out)
    meta = dict(app_id=appid, flags=flags, oep=oep, code_va=code_va, encrypted_size=enc_size,
                drmp_va=drmp_va, drmp_size=drmp_size, fallback_offsets=fallback)
    json.dump(meta, open(meta_path, "w"), indent=1)
    print({k: hex(v) if isinstance(v, int) and not isinstance(v, bool) else v for k, v in meta.items()})


if __name__ == "__main__":
    main(*sys.argv[1:4])
