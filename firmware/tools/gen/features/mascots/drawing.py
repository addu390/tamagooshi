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


def mirror(g, pts, c):
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


def boxeye(g, x, y, expr, out=INK, wht=10):
    if expr == "blink":
        rect(g, x, y + 1, x + 1, y + 1, out)
    elif expr == "sleepy":
        rect(g, x, y + 1, x + 1, y + 1, out)
        rect(g, x - 1, y - 1, x + 2, y - 1, out)
    elif expr == "happy":
        px(g, x, y + 1, out)
        px(g, x + 1, y, out)
        px(g, x + 2, y + 1, out)
    elif expr == "worried":
        rect(g, x, y, x + 1, y + 1, out)
        px(g, x if x < W // 2 else x + 2, y - 2, out)
    elif expr == "alert":
        rect(g, x - 1, y - 1, x + 2, y + 2, out)
        px(g, x, y, wht)
    else:
        rect(g, x, y, x + 1, y + 1, out)


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
