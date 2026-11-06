#!/usr/bin/env python3
"""make_placeholder_icons.py: write the placeholder .ico files that stand in for the game's icons.

usage: make_placeholder_icons.py [resources/manifest.json]

For every icon group in the manifest, writes its .ico (a plain coloured square with a border and two letters) containing
one image per manifest entry, with the same width, height and colour depth the original group has. Replace the .ico files
with real artwork later: rsrc_repo.py build picks, for every manifest image, the .ico image of the same size and depth
and keeps the manifest's RT_ICON / RT_GROUP_ICON ids. Pillow only draws the pixels; the DIB/ICO bytes are written here.
"""
import json, os, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
COLOURS = {"MB": ((28, 52, 110), (255, 255, 255), (255, 214, 64)),      # background, border, text
           "RC": ((24, 100, 60), (255, 255, 255), (255, 255, 255))}


def draw(w, h, bpp, label):
    from PIL import Image, ImageDraw, ImageFont
    bg, border, text = COLOURS.get(label, COLOURS["MB"])
    im = Image.new("P", (w, h), 1)
    pal = [0, 0, 0, *bg, *border, *text]
    im.putpalette(pal + [0] * (768 - len(pal)))
    d = ImageDraw.Draw(im)
    t = max(1, w // 16)
    d.rectangle([0, 0, w - 1, h - 1], fill=2)
    d.rectangle([t, t, w - 1 - t, h - 1 - t], fill=1)
    font = ImageFont.load_default(size=max(8, int(w * 0.5)))
    d.fontmode = "1"                                           # no antialiasing: only palette indices 1..3 appear
    box = d.textbbox((0, 0), label, font=font)
    d.text(((w - (box[2] - box[0])) // 2 - box[0], (h - (box[3] - box[1])) // 2 - box[1]), label, fill=3, font=font)
    return im, pal


def dib(w, h, bpp, label):
    """A 4 or 8 bit icon image as the DIB an .ico / RT_ICON holds: header, palette, XOR bitmap bottom-up, AND mask."""
    im, pal = draw(w, h, bpp, label)
    n = 1 << bpp
    palette = b"".join(bytes((pal[3 * i + 2], pal[3 * i + 1], pal[3 * i], 0)) if 3 * i < len(pal) else b"\0\0\0\0" for i in range(n))
    px = im.load()
    stride = ((w * bpp + 31) // 32) * 4
    xor = bytearray()
    for y in range(h - 1, -1, -1):
        row = bytearray(stride)
        for x in range(w):
            v = px[x, y]
            if bpp == 8:
                row[x] = v
            else:
                row[x // 2] |= v << (4 if x % 2 == 0 else 0)
        xor += row
    mask = bytes(((w + 31) // 32) * 4 * h)                       # all zero: every pixel opaque
    hdr = struct.pack("<IiiHHIIiiII", 40, w, h * 2, 1, bpp, 0, len(xor) + len(mask), 0, 0, 0, 0)
    return hdr + palette + bytes(xor) + mask


def make_ico(images, label):
    """images: [(w, h, bpp)] -> .ico bytes."""
    blobs = [(w, h, bpp, dib(w, h, bpp, label)) for w, h, bpp in images]
    out = bytearray(struct.pack("<HHH", 0, 1, len(blobs)))
    off = 6 + 16 * len(blobs)
    for w, h, bpp, b in blobs:
        out += struct.pack("<BBBBHHII", w, h, 16 if bpp == 4 else 0, 0, 1, bpp, len(b), off)
        off += len(b)
    return bytes(out) + b"".join(b for *_x, b in blobs)


def main():
    mf = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "resources", "manifest.json")
    m = json.load(open(mf))
    base = os.path.dirname(mf)
    for g in m["icon_groups"]:
        p = os.path.join(base, g["ico"])
        os.makedirs(os.path.dirname(p), exist_ok=True)
        open(p, "wb").write(make_ico([(i["w"], i["h"], i["bpp"]) for i in g["images"]], g["label"]))
        print("wrote %s (%s, %d images)" % (os.path.relpath(p, ROOT), g["label"], len(g["images"])))


if __name__ == "__main__":
    main()
