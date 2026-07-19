from gen.features.mascots.drawing import INK, auto_outline, blank, kawaii_pal, px, rect

WHT = 10


def _eye(g, x, y, expr):
    if expr == "blink":
        rect(g, x, y + 1, x + 1, y + 1, INK)
    elif expr == "sleepy":
        rect(g, x, y + 1, x + 1, y + 1, INK)
        rect(g, x - 1, y - 1, x + 2, y - 1, INK)
    elif expr == "happy":
        px(g, x, y + 1, INK)
        px(g, x + 1, y, INK)
        px(g, x + 2, y + 1, INK)
    elif expr == "worried":
        rect(g, x, y, x + 1, y + 1, INK)
        px(g, x if x < 16 else x + 2, y - 2, INK)
    elif expr == "alert":
        rect(g, x - 1, y - 1, x + 2, y + 2, INK)
        px(g, x, y, WHT)
    else:
        rect(g, x, y, x + 1, y + 1, INK)


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
    _eye(g, 12, 11, expr)
    _eye(g, 19, 11, expr)
    return g


CLAUDE = [
    {
        "id": "clawd", "label": "clawd", "cat": "claude", "build": build_clawd,
        "pal": kawaii_pal({1: (94, 51, 36), 2: (215, 119, 87), 3: (191, 96, 62),
                           11: (30, 22, 18)}),
        "opts": {},
    },
]
