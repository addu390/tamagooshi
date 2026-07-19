import os
import urllib.request

from gen.features.mascots.drawing import H, W, blank

CACHE = "/tmp/tama_refs"


def _quantize(opaque, colors):
    from PIL import Image

    ncol = min(colors, max(1, len(set(opaque))))
    strip = Image.new("RGB", (max(1, len(opaque)), 1))
    strip.putdata(opaque)
    raw = strip.quantize(colors=ncol, method=Image.MEDIANCUT).getpalette()
    return [(raw[i * 3], raw[i * 3 + 1], raw[i * 3 + 2]) for i in range(ncol)]


def _nearest(palcolors, r, g, b):
    best, bd = 0, 1 << 30
    for i, (pr, pg, pb) in enumerate(palcolors):
        d = (pr - r) ** 2 + (pg - g) ** 2 + (pb - b) ** 2
        if d < bd:
            bd, best = d, i
    return best


def import_sprite(src, base_dir, target_h=30, colors=15):
    from PIL import Image

    if "://" not in src:
        local = src if os.path.isabs(src) else os.path.join(base_dir, src)
    else:
        os.makedirs(CACHE, exist_ok=True)
        local = os.path.join(CACHE, os.path.basename(src))
        if not os.path.exists(local):
            urllib.request.urlretrieve(src, local)

    im = Image.open(local).convert("RGBA")
    bbox = im.getbbox()
    if bbox:
        im = im.crop(bbox)
    s = min((W - 2) / im.width, target_h / im.height)
    nw = max(1, round(im.width * s))
    nh = max(1, round(im.height * s))
    im = im.resize((nw, nh), Image.LANCZOS)

    canvas = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    canvas.paste(im, ((W - nw) // 2, (H - 1) - nh))
    data = list(canvas.getdata())

    opaque = [(r, g, b) for (r, g, b, a) in data if a >= 128]
    palcolors = _quantize(opaque, colors)

    grid = blank()
    for y in range(H):
        for x in range(W):
            r, g, b, a = data[y * W + x]
            if a >= 128:
                grid[y][x] = _nearest(palcolors, r, g, b) + 1

    pal = {0: None}
    for i, c in enumerate(palcolors):
        pal[i + 1] = c
    return pal, [grid]


def logo_mask(source, base_dir, target=24):
    from PIL import Image

    local = source if os.path.isabs(source) else os.path.join(base_dir, source)
    im = Image.open(local).convert("RGBA")
    bbox = im.getbbox()
    if bbox:
        im = im.crop(bbox)
    s = target / max(im.width, im.height)
    nw = max(1, round(im.width * s))
    nh = max(1, round(im.height * s))
    im = im.resize((nw, nh), Image.LANCZOS)
    px = list(im.getdata())
    rows = []
    for y in range(nh):
        row = []
        for x in range(nw):
            r, g, b, a = px[y * nw + x]
            lum = 0.299 * r + 0.587 * g + 0.114 * b
            row.append(1 if (a >= 128 and lum < 200) else 0)
        rows.append(row)
    return nw, nh, rows


def pack_mask(w, h, rows):
    bits = bytearray((w * h + 7) // 8)
    i = 0
    for y in range(h):
        for x in range(w):
            if rows[y][x]:
                bits[i >> 3] |= 0x80 >> (i & 7)
            i += 1
    return bits.hex()
