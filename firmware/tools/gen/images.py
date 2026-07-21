import os
import urllib.request

from gen.features.mascots.drawing import FRAMES, H, W, blank

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


def _fetch(src, base_dir):
    if "://" not in src:
        return src if os.path.isabs(src) else os.path.join(base_dir, src)
    os.makedirs(CACHE, exist_ok=True)
    local = os.path.join(CACHE, os.path.basename(src))
    if not os.path.exists(local):
        urllib.request.urlretrieve(src, local)
    return local


def _tiles(im):
    n = len(FRAMES)
    if im.width == im.height * n:
        tw = im.width // n
        return [im.crop((i * tw, 0, (i + 1) * tw, im.height)) for i in range(n)]
    return [im]


def _native_scale(im):
    from PIL import Image

    for k in (8, 6, 4, 3, 2):
        if im.width % k or im.height % k:
            continue
        small = im.resize((im.width // k, im.height // k), Image.NEAREST)
        if small.resize(im.size, Image.NEAREST).tobytes() == im.tobytes():
            return small
    return im


def _is_palette_art(tiles):
    colors = {(r, g, b) for t in tiles for (r, g, b, a) in t.getdata() if a >= 128}
    return len(colors) <= 64


# Accepts a single image or a horizontal strip of one tile per frame in FRAMES
# order. Tiles share one crop box and palette so features stay aligned.
def import_sprite(src, base_dir, target_h=30, colors=15):
    from PIL import Image

    im = _native_scale(Image.open(_fetch(src, base_dir)).convert("RGBA"))
    tiles = _tiles(im)

    boxes = [b for b in (t.getbbox() for t in tiles) if b]
    if not boxes:
        raise SystemExit(f"sprite {src} is fully transparent")
    bbox = (min(b[0] for b in boxes), min(b[1] for b in boxes),
            max(b[2] for b in boxes), max(b[3] for b in boxes))
    tiles = [t.crop(bbox) for t in tiles]

    tw, th = tiles[0].size
    pixel = _is_palette_art(tiles)
    if pixel and tw <= W and th <= H:
        nw, nh = tw, th
    else:
        s = min((W - 2) / tw, target_h / th)
        nw = max(1, round(tw * s))
        nh = max(1, round(th * s))
        resample = Image.NEAREST if pixel else Image.LANCZOS
        tiles = [t.resize((nw, nh), resample) for t in tiles]

    datas = []
    for t in tiles:
        canvas = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        canvas.paste(t, ((W - nw) // 2, max(0, (H - 1) - nh)))
        datas.append(list(canvas.getdata()))

    opaque = [(r, g, b) for data in datas for (r, g, b, a) in data if a >= 128]
    palcolors = _quantize(opaque, colors)

    grids = []
    for data in datas:
        grid = blank()
        for y in range(H):
            for x in range(W):
                r, g, b, a = data[y * W + x]
                if a >= 128:
                    grid[y][x] = _nearest(palcolors, r, g, b) + 1
        grids.append(grid)

    pal = {0: None}
    for i, c in enumerate(palcolors):
        pal[i + 1] = c
    return pal, grids


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
