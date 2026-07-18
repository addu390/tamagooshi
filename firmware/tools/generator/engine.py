W = H = 32
FRAMES = ["open", "blink", "happy", "sleepy", "worried", "alert"]

BASE = {
    0: None,
    1: (40, 30, 24),
    2: (245, 168, 78),
    3: (208, 120, 46),
    4: (255, 205, 140),
    5: (253, 247, 240),
    6: (255, 170, 186),
    7: (206, 100, 112),
    8: (86, 196, 104),
    9: (26, 24, 24),
    10: (255, 255, 255),
}

INK = 11


def palette(overrides):
    p = dict(BASE)
    p.update(overrides)
    return p


def kawaii_pal(overrides):
    p = palette({1: (98, 68, 52), 6: (250, 176, 184), 10: (255, 255, 255),
                 11: (46, 34, 30)})
    p.update(overrides)
    return p


def blank():
    return [[0] * W for _ in range(H)]


def px(g, x, y, c):
    if 0 <= x < W and 0 <= y < H:
        g[y][x] = c


def rect(g, x0, y0, x1, y1, c):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            px(g, x, y, c)


def rrect(g, x0, y0, x1, y1, c, r=2):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            cxp = x0 + r if x < x0 + r else (x1 - r if x > x1 - r else x)
            cyp = y0 + r if y < y0 + r else (y1 - r if y > y1 - r else y)
            if (x - cxp) ** 2 + (y - cyp) ** 2 <= r * r + 0.5:
                px(g, x, y, c)


def ellipse(g, cx, cy, rx, ry, c, only_solid=False):
    for y in range(H):
        for x in range(W):
            if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0:
                if only_solid and g[y][x] == 0:
                    continue
                px(g, x, y, c)


def auto_outline(g, col=1):
    solid = {(x, y) for y in range(H) for x in range(W) if g[y][x] != 0}
    for (x, y) in list(solid):
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < W and 0 <= ny < H and g[ny][nx] == 0:
                g[ny][nx] = col


def _mirror(g, pts, c):
    for (x, y) in pts:
        px(g, x, y, c)
        px(g, 31 - x, y, c)


def keye(g, x, y, expr, out=INK, wht=10):
    if expr == "open":
        for yy in range(y, y + 3):
            px(g, x, yy, out)
            px(g, x + 1, yy, out)
        px(g, x, y, wht)
    elif expr == "alert":
        for yy in range(y - 1, y + 3):
            px(g, x, yy, out)
            px(g, x + 1, yy, out)
        px(g, x, y - 1, wht)
    elif expr == "worried":
        px(g, x, y, out)
        px(g, x + 1, y + 1, out)
        px(g, x + 2, y, out)
    elif expr == "sleepy":
        px(g, x, y + 2, out)
        px(g, x + 1, y + 2, out)
    elif expr == "blink":
        px(g, x, y + 1, out)
        px(g, x + 1, y + 1, out)
    elif expr == "happy":
        px(g, x, y + 1, out)
        px(g, x + 1, y, out)
        px(g, x + 2, y + 1, out)


def kblush(g, x, y, c=6):
    px(g, x, y, c)
    px(g, x + 1, y, c)
    px(g, x, y + 1, c)
    px(g, x + 1, y + 1, c)


def kmouth(g, x, y, expr, out=INK):
    if expr == "happy":
        px(g, x, y, out)
        px(g, x + 3, y, out)
        px(g, x + 1, y + 1, out)
        px(g, x + 2, y + 1, out)
    elif expr == "worried":
        px(g, x, y + 1, out)
        px(g, x + 1, y, out)
        px(g, x + 2, y, out)
        px(g, x + 3, y + 1, out)
    elif expr == "alert":
        px(g, x + 1, y, out)
        px(g, x + 2, y, out)
        px(g, x + 1, y + 1, out)
        px(g, x + 2, y + 1, out)
    elif expr == "sleepy":
        px(g, x + 1, y, out)
        px(g, x + 2, y, out)
    else:
        px(g, x + 1, y + 1, out)
        px(g, x + 2, y + 1, out)


def rgb565(rgb):
    r, gg, b = rgb
    return ((r >> 3) << 11) | ((gg >> 2) << 5) | (b >> 3)


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


CACHE = "/tmp/tama_refs"


def import_sprite(src, base_dir, target_h=30, colors=15):
    import os
    import urllib.request

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


def resolve(m, base_dir):
    if "src" in m:
        pal, frames = import_sprite(m["src"], base_dir)
        return pal, frames, len(pal)
    pal = m["pal"]
    frames = [m["build"](f, m.get("opts", {})) for f in FRAMES]
    return pal, frames, max(pal.keys()) + 1


def sprite_header(m, base_dir):
    pal, frames, ncol = resolve(m, base_dir)
    single = len(frames) == 1
    lines = ["#pragma once", "", '#include "mascots/sprite.h"', "",
             f'namespace tama::sprites::{m["id"]} {{', "",
             f"inline constexpr int kW = {W};", f"inline constexpr int kH = {H};"]
    palvals = ", ".join("0x%04X" % (rgb565(pal[i]) if pal.get(i) else 0) for i in range(ncol))
    lines.append(f"inline constexpr uint16_t kPalette[{ncol}] = {{ {palvals} }};")
    for fi, g in enumerate(frames):
        rows = [",".join(str(g[y][x]) for x in range(W)) for y in range(H)]
        body = ",\n    ".join(rows)
        lines.append(f"inline constexpr uint8_t kF{fi}[kW * kH] = {{\n    {body}\n}};")
    flist = ", ".join(f"kF{i}" for i in range(len(frames)))
    lines.append(f"inline constexpr const uint8_t* kFrames[{len(frames)}] = {{ {flist} }};")
    wheeled = "true" if m.get("wheeled") else "false"
    outline = 0 if "src" in m else 1
    idx = ("0, 0, 0, 0, 0, 0, false" if single else "0, 1, 2, 3, 4, 5, true")
    idx += f", {wheeled}, {outline}"
    lines.append(
        f'inline constexpr SpriteDef kDef{{ "{m["id"]}", "{m["label"]}", "{m["cat"]}", kW, kH, '
        f"kPalette, kFrames, {idx} }};")
    lines += ["", f"}}  // namespace tama::sprites::{m['id']}", "",
              "namespace tama::characters {",
              f"inline Character& {m['id']}() {{ static SpriteChar inst(sprites::{m['id']}::kDef);"
              " return inst; }",
              "}  // namespace tama::characters", ""]
    return "\n".join(lines)


def pack_mask(w, h, rows):
    bits = bytearray((w * h + 7) // 8)
    i = 0
    for y in range(h):
        for x in range(w):
            if rows[y][x]:
                bits[i >> 3] |= 0x80 >> (i & 7)
            i += 1
    return bits.hex()


def logo_mask(source, base_dir, target=24):
    from PIL import Image

    import os
    local = source if os.path.isabs(source) else os.path.join(base_dir, source)
    im = Image.open(local).convert("RGBA")
    bbox = im.getbbox()
    if bbox:
        im = im.crop(bbox)
    s = target / max(im.width, im.height)
    nw = max(1, round(im.width * s))
    nh = max(1, round(im.height * s))
    im = im.resize((nw, nh), Image.LANCZOS)
    px_ = list(im.getdata())
    rows = []
    for y in range(nh):
        row = []
        for x in range(nw):
            r, g, b, a = px_[y * nw + x]
            lum = 0.299 * r + 0.587 * g + 0.114 * b
            row.append(1 if (a >= 128 and lum < 200) else 0)
        rows.append(row)
    return nw, nh, rows
