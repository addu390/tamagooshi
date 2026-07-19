from gen.features.mascots.drawing import (INK, auto_outline, blank, ellipse, kawaii_pal,
                                          kblush, keye, kmouth, mirror, px)


def build_kcat(expr, o):
    OUT, BODY, PATCH = 1, 2, 4
    g = blank()
    ellipse(g, 16, 20, 10, 8, BODY)
    ellipse(g, 16, 13, 7, 6, BODY)
    lear = [(11, 3),
            (10, 4), (11, 4), (12, 4),
            (10, 5), (11, 5), (12, 5),
            (9, 6), (10, 6), (11, 6), (12, 6), (13, 6),
            (9, 7), (10, 7), (11, 7), (12, 7), (13, 7)]
    mirror(g, lear, BODY)
    mirror(g, [(11, 5), (11, 6)], PATCH)
    ellipse(g, 21, 11, 3, 3, PATCH, only_solid=True)
    ellipse(g, 10, 23, 4, 3, PATCH, only_solid=True)
    for (x, y) in [(25, 21), (26, 20), (27, 19), (27, 18), (26, 17)]:
        px(g, x, y, BODY)
    auto_outline(g, OUT)
    keye(g, 12, 15, expr)
    keye(g, 18, 15, expr)
    kblush(g, 10, 18)
    kblush(g, 20, 18)
    kmouth(g, 14, 18, expr)
    return g


def build_kdog(expr, o):
    OUT, BODY, WHITE, PLIGHT = 1, 2, 4, 5
    g = blank()
    ellipse(g, 16, 20, 10, 8, BODY)
    ellipse(g, 16, 13, 7, 6, BODY)
    ellipse(g, 16, 23, 8, 5, WHITE, only_solid=True)
    ellipse(g, 16, 16, 4, 3, WHITE, only_solid=True)
    lear = [(9, 3),
            (9, 4), (10, 4),
            (9, 5), (10, 5), (11, 5),
            (9, 6), (10, 6), (11, 6), (12, 6)]
    mirror(g, lear, BODY)
    mirror(g, [(10, 5), (10, 6)], PLIGHT)
    for (x, y) in [(25, 21), (26, 20), (27, 19), (27, 18), (26, 17), (25, 17)]:
        px(g, x, y, BODY)
    auto_outline(g, OUT)
    keye(g, 12, 14, expr)
    keye(g, 18, 14, expr)
    kblush(g, 10, 17)
    kblush(g, 20, 17)
    px(g, 15, 16, INK)
    px(g, 16, 16, INK)
    kmouth(g, 14, 18, expr)
    return g


def build_kduck(expr, o):
    OUT, BODY, SHADE, BEAK = 1, 2, 3, 4
    g = blank()
    ellipse(g, 16, 19, 10, 10, BODY)
    for (x, y) in [(15, 4), (16, 3), (16, 4), (17, 4), (14, 5), (15, 5),
                   (16, 5), (17, 5), (18, 5)]:
        px(g, x, y, BODY)
    ellipse(g, 8, 20, 3, 4, SHADE, only_solid=True)
    for (x, y) in [(19, 16), (20, 16), (21, 16), (19, 17), (20, 17)]:
        px(g, x, y, BEAK)
    for (x, y) in [(12, 28), (13, 28), (19, 28), (20, 28)]:
        px(g, x, y, BEAK)
    auto_outline(g, OUT)
    keye(g, 13, 13, expr)
    keye(g, 17, 13, expr)
    kblush(g, 10, 16)
    kblush(g, 21, 16)
    return g


PALS = [
    {
        "id": "cat", "label": "mochi", "cat": "pals", "build": build_kcat,
        "pal": kawaii_pal({2: (250, 250, 252), 3: (225, 226, 234), 4: (246, 190, 120),
                           5: (255, 214, 150)}),
        "opts": {},
    },
    {
        "id": "dog", "label": "shiba", "cat": "pals", "build": build_kdog,
        "pal": kawaii_pal({2: (245, 184, 112), 3: (214, 150, 82), 4: (250, 250, 252),
                           5: (255, 210, 150)}),
        "opts": {},
    },
    {
        "id": "duck", "label": "ducky", "cat": "pals", "build": build_kduck,
        "pal": kawaii_pal({2: (255, 214, 88), 3: (236, 186, 56), 4: (250, 160, 58),
                           5: (255, 232, 150)}),
        "opts": {},
    },
]
