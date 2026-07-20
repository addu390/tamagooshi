ROLES = ("bg", "fg", "hi", "dim", "dimmer", "warn", "crit", "ink", "blush")


def _565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def _hex(s):
    s = s.lstrip("#")
    return int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16)


def _mix(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))


def derive(colors):
    surface = _hex(colors["surface"])
    ink = _hex(colors["ink"])
    hi = _hex(colors["accent"]) if colors.get("accent") else ink
    roles = [surface, ink, hi, _mix(ink, surface, 0.45), _mix(ink, surface, 0.72),
             (230, 150, 40), (200, 70, 66), ink, (252, 176, 184)]
    return [_565(*c) for c in roles]


def _tuned(*roles):
    assert len(roles) == len(ROLES)
    return {"roles": [_565(*_hex(c)) for c in roles]}


def _derived(colors):
    return {"roles": derive(colors)}


# Role order follows ROLES. Hand-tuned themes pick every role, derived themes
# come from surface/ink/accent.
THEMES = {
    "slate":    _tuned("#E0E8E0", "#202830", "#202830", "#787C88", "#B8C0C0",
                       "#D08828", "#C03828", "#202830", "#F89480"),
    "espresso": _tuned("#F0E4C8", "#482C10", "#482C10", "#907858", "#D0C098",
                       "#C07818", "#B03428", "#482C10", "#F89480"),
    "midnight": _tuned("#101820", "#E0E4E8", "#E0E4E8", "#788090", "#384050",
                       "#F0A838", "#F05850", "#000000", "#F89480"),
    "timex":    _tuned("#E8E0C8", "#181810", "#181810", "#787060", "#C0BCA8",
                       "#C87818", "#B02820", "#181810", "#F89480"),
    "terra":    _derived({"surface": "#F0EEE6", "ink": "#191919", "accent": "#D97757"}),
}
