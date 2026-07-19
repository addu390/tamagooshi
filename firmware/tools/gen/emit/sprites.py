from gen.features.mascots.drawing import FRAMES, H, W
from gen.images import import_sprite


def rgb565(rgb):
    r, g, b = rgb
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


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
