from engine import (INK, auto_outline, blank, kawaii_pal, kblush, keye,
                    kmouth, px, rect, rrect)


def ktruck_pal(cab, cabsh, cabli):
    return kawaii_pal({2: cab, 3: cabsh, 4: cabli, 5: (200, 206, 214),
                       7: (150, 205, 238), 8: (255, 220, 110), 9: (52, 52, 60)})


def build_ktruck(expr, o):
    OUT, CAB, CABS, CABL, CHR, _, GLASS, HEAD, TYRE, WHT = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    g = blank()
    roof = o.get("roof")
    if roof == "stacks":
        rect(g, 6, 3, 7, 8, CHR)
        rect(g, 24, 3, 25, 8, CHR)
    elif roof == "beacon":
        rect(g, 13, 3, 18, 5, HEAD)

    rrect(g, 3, 6, 28, 24, CAB, r=4)
    for x in range(8, 24):
        px(g, x, 6, CABL)
    for y in range(9, 24):
        px(g, 3, y, CABS)
        px(g, 4, y, CABS)
        px(g, 27, y, CABS)
        px(g, 28, y, CABS)

    rrect(g, 7, 8, 24, 15, GLASS, r=2)
    rect(g, 5, 18, 26, 23, CHR)
    for gx in (9, 22):
        for y in range(19, 23):
            px(g, gx, y, INK)
    rect(g, 4, 16, 6, 18, HEAD)
    rect(g, 25, 16, 27, 18, HEAD)
    rect(g, 3, 23, 28, 24, CHR)

    for cx in (8, 23):
        for dy in range(-3, 4):
            for dx in range(-3, 4):
                if dx * dx + dy * dy <= 9:
                    px(g, cx + dx, 28 + dy, TYRE)
        for dy in range(-1, 2):
            for dx in range(-1, 2):
                px(g, cx + dx, 28 + dy, WHT)

    auto_outline(g, OUT)
    keye(g, 11, 10, expr)
    keye(g, 19, 10, expr)
    kblush(g, 8, 16)
    kblush(g, 23, 16)
    kmouth(g, 14, 20, expr)
    return g


TRUCKS = [
    {
        "id": "hauler", "label": "hauler", "cat": "trucks", "build": build_ktruck, "wheeled": True,
        "pal": ktruck_pal((222, 74, 66), (176, 46, 42), (255, 128, 118)),
        "opts": {"roof": "stacks"},
    },
    {
        "id": "towie", "label": "towie", "cat": "trucks", "build": build_ktruck, "wheeled": True,
        "pal": ktruck_pal((248, 196, 70), (208, 156, 44), (255, 228, 148)),
        "opts": {"roof": "beacon"},
    },
    {
        "id": "parcel", "label": "parcel", "cat": "trucks", "build": build_ktruck, "wheeled": True,
        "pal": ktruck_pal((36, 60, 120), (20, 36, 82), (78, 110, 178)),
        "opts": {"roof": None},
    },
]
