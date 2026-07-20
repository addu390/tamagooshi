from gen.features.mascots.drawing import auto_outline, blank, boxeye, kawaii_pal, rect


def build_clawd(expr, o):
    OUT, BODY, SHADE = 1, 2, 3
    g = blank()
    rect(g, 8, 8, 24, 21, BODY)
    rect(g, 5, 13, 7, 16, BODY)
    rect(g, 25, 13, 27, 16, BODY)
    for lx in (10, 13, 19, 22):
        rect(g, lx, 22, lx + 1, 25, BODY)
    rect(g, 9, 19, 23, 21, SHADE)
    rect(g, 5, 15, 7, 16, SHADE)
    rect(g, 25, 15, 27, 16, SHADE)
    for lx in (10, 13, 19, 22):
        rect(g, lx, 24, lx + 1, 25, SHADE)
    auto_outline(g, OUT)
    boxeye(g, 12, 11, expr)
    boxeye(g, 19, 11, expr)
    return g


CLAUDE = [
    {
        "id": "clawd", "label": "clawd", "cat": "claude", "build": build_clawd,
        "pal": kawaii_pal({1: (94, 51, 36), 2: (215, 119, 87), 3: (191, 96, 62),
                           11: (30, 22, 18)}),
        "opts": {},
    },
]
