def _565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def _hex(s):
    s = s.lstrip("#")
    return int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16)


def _mix(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))


# Role order: bg, fg, hi, dim, dimmer, warn, crit, ink, blush
THEMES = {
    "slate":    [0xE75C, 0x2146, 0x2146, 0x7BF1, 0xBE18, 0xD445, 0xC1C5, 0x2146, 0xFCB0],
    "espresso": [0xF739, 0x4962, 0x4962, 0x93CB, 0xD613, 0xC3C3, 0xB1A5, 0x4962, 0xFCB0],
    "midnight": [0x10C4, 0xE73D, 0xE73D, 0x7C12, 0x3A0A, 0xF547, 0xF2CA, 0x0000, 0xFCB0],
    "timex":    [0xEF19, 0x18C2, 0x18C2, 0x7B8C, 0xC5F5, 0xCBC3, 0xB144, 0x18C2, 0xFCB0],
}


def derive(colors):
    surface = _hex(colors["surface"])
    ink = _hex(colors["ink"])
    hi = _hex(colors["accent"]) if colors.get("accent") else ink
    roles = [surface, ink, hi, _mix(ink, surface, 0.45), _mix(ink, surface, 0.72),
             (230, 150, 40), (200, 70, 66), ink, (252, 176, 184)]
    return [_565(*c) for c in roles]
