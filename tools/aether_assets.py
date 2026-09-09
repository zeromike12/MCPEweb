#!/usr/bin/env python3
"""Generate every Aether texture and the matching C++ constants header.

Writes:
  data/images/terrain.png      block textures into cells that no tile references
  data/images/gui/items.png    item icons into unused cells
  data/images/mob/{moa,sentry,mimic}.png  mob skins (64x32)
  src/world/aether/AetherTex.h  atlas indices (tex = row*16+col) used by the C++ code

The cell pools below were computed by evaluating every tile's getTexture() for all
faces/data values (plus the few hard-coded references: fire frames, door tops, glass pane
edge, snowy grass side, crack overlay row, water/lava/portal dynamic textures).  Only cells
in these pools are touched, so nothing that already renders is affected.

Run from the repository root:  python3 tools/aether_assets.py
"""
import os, random, math
import gear_sprites as g

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TERRAIN = os.path.join(ROOT, 'data/images/terrain.png')
ITEMS = os.path.join(ROOT, 'data/images/gui/items.png')
MOBDIR = os.path.join(ROOT, 'data/images/mob')
HEADER = os.path.join(ROOT, 'src/world/aether/AetherTex.h')

# (col,row) cells in terrain.png that nothing references.  Blank cells first.
TERRAIN_POOL = [
    (14, 1), (3, 7), (9, 9), (5, 10), (10, 10), (11, 10), (12, 10), (13, 10), (5, 11), (8, 11), (9, 11), (10, 11),
    (11, 11), (12, 11), (13, 11), (14, 11), (15, 11), (7, 12), (8, 12), (9, 12), (10, 12), (11, 12), (7, 13), (8, 13),
    (9, 13), (7, 14),
    (6, 7), (7, 7), (8, 7), (9, 7), (10, 7), (11, 7), (12, 7), (13, 7), (14, 7), (15, 7), (10, 8), (11, 8), (12, 8),
    (13, 8), (14, 8), (15, 8), (10, 9), (11, 9), (12, 9), (13, 9), (14, 9), (15, 9), (14, 10), (15, 10), (6, 11), (7, 11),
    (10, 4), (11, 4), (12, 4), (13, 4), (14, 4), (10, 6), (11, 6), (12, 6), (13, 6), (14, 6), (12, 13), (13, 13), (14, 13),
    (15, 13), (3, 10), (4, 10), (6, 10), (7, 10), (3, 11), (15, 12), (0, 3), (5, 3), (1, 4), (5, 5), (14, 2), (0, 8), (3, 8),
    (5, 8), (3, 9), (2, 14), (3, 14), (4, 14), (12, 14), (15, 14), (14, 15), (15, 15),
]
# icon indices in items.png that are fully transparent and referenced by no item
ITEM_POOL = [102, 118, 119, 120, 134, 144, 145, 146, 147, 148, 150, 152, 160, 161, 162, 163, 164, 166, 168, 169, 170,
             171, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 204, 205, 206, 207, 217, 218, 219, 220,
             221, 222, 223]

rng = random.Random(0xAE7)


# ----------------------------------------------------------------------------- helpers
def clamp(v):
    return max(0, min(255, int(v)))


def shade(c, f):
    return (clamp(c[0] * f), clamp(c[1] * f), clamp(c[2] * f))


def mix(a, b, t):
    return (clamp(a[0] + (b[0] - a[0]) * t), clamp(a[1] + (b[1] - a[1]) * t), clamp(a[2] + (b[2] - a[2]) * t))


def blank(alpha=0):
    return [[(0, 0, 0, alpha) for _ in range(16)] for _ in range(16)]


def put(s, x, y, c, a=255):
    if 0 <= x < 16 and 0 <= y < 16:
        s[y][x] = (c[0], c[1], c[2], a)


def noise_block(base, var=0.12, speck=None, speck_p=0.06, alpha=255):
    s = blank()
    for y in range(16):
        for x in range(16):
            f = 1.0 + rng.uniform(-var, var)
            c = shade(base, f)
            if speck and rng.random() < speck_p:
                c = speck
            s[y][x] = (c[0], c[1], c[2], alpha)
    return s


def stone(base, var=0.10):
    """Cobbly stone: noise plus a few darker cracks."""
    s = noise_block(base, var)
    for _ in range(5):
        x, y = rng.randrange(16), rng.randrange(16)
        L = rng.randrange(2, 5)
        dx, dy = rng.choice([(1, 0), (0, 1), (1, 1), (1, -1)])
        for i in range(L):
            put(s, (x + dx * i) % 16, (y + dy * i) % 16, shade(base, 0.72))
    return s


def ore(base_sprite, gem, count=9):
    s = [row[:] for row in base_sprite]
    pts = set()
    while len(pts) < count:
        x, y = rng.randrange(1, 15), rng.randrange(1, 15)
        pts.add((x, y))
        if rng.random() < 0.5:
            pts.add((x + rng.choice([-1, 1]), y))
        if rng.random() < 0.5:
            pts.add((x, y + rng.choice([-1, 1])))
    for (x, y) in pts:
        put(s, x, y, shade(gem, rng.uniform(0.85, 1.15)))
    # highlight
    for (x, y) in list(pts)[::4]:
        put(s, x, y, mix(gem, (255, 255, 255), 0.5))
    return s


def planks(base):
    s = blank()
    for y in range(16):
        for x in range(16):
            f = 1.0 + rng.uniform(-0.06, 0.06)
            c = shade(base, f)
            if y % 4 == 3:
                c = shade(base, 0.62)
            elif (y // 4 % 2 == 0 and x == 7) or (y // 4 % 2 == 1 and x == 15):
                c = shade(base, 0.62)
            s[y][x] = c + (255,)
    return s


def log_side(bark, dark):
    s = blank()
    for y in range(16):
        for x in range(16):
            c = bark if (x + (y // 5)) % 3 else dark
            c = shade(c, 1.0 + rng.uniform(-0.08, 0.08))
            s[y][x] = c + (255,)
    return s


def log_top(bark, inner):
    s = blank()
    for y in range(16):
        for x in range(16):
            d = max(abs(x - 7.5), abs(y - 7.5))
            if d > 6.5:
                c = bark
            else:
                c = shade(inner, 0.8 if int(d) % 2 else 1.0)
            s[y][x] = shade(c, 1.0 + rng.uniform(-0.06, 0.06)) + (255,)
    return s


def bricks(base, mortar):
    s = blank()
    for y in range(16):
        for x in range(16):
            row = y // 4
            off = 4 if row % 2 else 0
            c = base
            if y % 4 == 3 or (x + off) % 8 == 7:
                c = mortar
            s[y][x] = shade(c, 1.0 + rng.uniform(-0.07, 0.07)) + (255,)
    return s


def leaves(base, hole_p=0.18):
    s = blank()
    for y in range(16):
        for x in range(16):
            if rng.random() < hole_p:
                s[y][x] = (0, 0, 0, 0)
            else:
                s[y][x] = shade(base, rng.uniform(0.7, 1.2)) + (255,)
    return s


def cloud(base, alpha=215):
    s = blank()
    for y in range(16):
        for x in range(16):
            w = 0.5 + 0.5 * math.sin(x * 0.9 + y * 0.4) * math.cos(y * 0.8 - x * 0.3)
            c = mix(base, (255, 255, 255), 0.35 * w)
            c = shade(c, 1.0 + rng.uniform(-0.04, 0.04))
            s[y][x] = c + (alpha,)
    return s


def glass(tint, border):
    s = blank()
    for y in range(16):
        for x in range(16):
            if x in (0, 15) or y in (0, 15):
                s[y][x] = border + (255,)
            elif (x + y) in (5, 6, 12, 13):
                s[y][x] = mix(tint, (255, 255, 255), 0.6) + (140,)
            else:
                s[y][x] = tint + (60,)
    return s


def wool(base):
    s = blank()
    for y in range(16):
        for x in range(16):
            f = 1.0 + rng.uniform(-0.08, 0.08)
            if (x * 3 + y * 5) % 7 == 0:
                f *= 0.9
            s[y][x] = shade(base, f) + (255,)
    return s


def framed(base_sprite, frame, inner=None):
    """A block face with a 1px frame (machines / workbench)."""
    s = [row[:] for row in base_sprite]
    for i in range(16):
        for (x, y) in ((i, 0), (i, 15), (0, i), (15, i)):
            put(s, x, y, frame)
    if inner:
        for y in range(3, 13):
            for x in range(3, 13):
                put(s, x, y, shade(inner, 1.0 + rng.uniform(-0.08, 0.08)))
    return s


def template(rows, pal):
    """16 rows of 16 chars.  '.' transparent."""
    assert len(rows) == 16
    s = blank()
    for y, r in enumerate(rows):
        assert len(r) == 16, r
        for x, ch in enumerate(r):
            if ch != '.':
                c = pal[ch]
                s[y][x] = c + (255,)
    return s


def symbol(s, rows, color, ox=0, oy=0):
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            if ch == '#':
                put(s, x + ox, y + oy, color)
    return s


# ----------------------------------------------------------------------------- palettes
HOLY = (196, 194, 188)
DIRT_A = (150, 118, 90)
GRASS_A = (116, 196, 120)
ENCH_GRASS = (170, 220, 110)
SKY_WOOD = (168, 146, 106)
SKY_BARK = (118, 102, 80)
GOLD_BARK = (150, 118, 62)
GOLD_INNER = (236, 196, 88)
ICE = (172, 214, 240)
QUICK = (232, 210, 130)
AEROGEL = (130, 200, 220)
AMBRO = (255, 208, 64)
ZANITE = (96, 92, 200)
GRAVITITE = (210, 120, 180)
CARVED = (150, 150, 158)
SENTRY = (108, 108, 120)
ANGELIC = (240, 236, 214)
HELLFIRE = (150, 52, 44)
CLOUD_COLD = (236, 240, 250)
CLOUD_BLUE = (140, 176, 236)
CLOUD_GOLD = (245, 214, 120)
CLOUD_PURPLE = (190, 140, 230)
CLOUD_GREEN = (150, 220, 150)
CLOUD_STORM = (120, 118, 140)

SAPLING = [
    "................",
    "................",
    ".......gg.......",
    "......gggg......",
    ".....gggggg.....",
    "....gggGgggg....",
    "....ggGGGggg....",
    ".....gggggg.....",
    "......gGgg......",
    ".......bb.......",
    ".......bb.......",
    ".......bb.......",
    ".......bb.......",
    "......bbbb......",
    "................",
    "................",
]
FLOWER = [
    "................",
    "................",
    ".....pp.........",
    "....pccp........",
    "....pccp........",
    ".....pp....pp...",
    "......g...pccp..",
    "......g...pccp..",
    "......g....pp...",
    "......gg....g...",
    "..g...g.g...g...",
    "...g..g..g.g....",
    "....g.g...g.....",
    ".....gg...g.....",
    "......g...g.....",
    "................",
]
BUSH = [
    "................",
    "................",
    "................",
    "......gg.gg.....",
    ".....gggggggg...",
    "....gggrggggg...",
    "...ggggggggrgg..",
    "...ggrggggggggg.",
    "..gggggggrggggg.",
    "..ggggggggggggg.",
    "...ggrggggggrg..",
    "....gggggrggg...",
    ".....gggggg.....",
    ".......bb.......",
    ".......bb.......",
    "................",
]
TORCH = [
    "................",
    "................",
    ".......yy.......",
    "......yYYy......",
    "......yYYy......",
    ".......yy.......",
    ".......bb.......",
    ".......bb.......",
    ".......bb.......",
    ".......bb.......",
    ".......bb.......",
    ".......bb.......",
    ".......bb.......",
    ".......bb.......",
    "................",
    "................",
]
PARACHUTE = [
    "................",
    ".....cccccc.....",
    "...cccCCCCccc...",
    "..ccCCCCCCCCcc..",
    ".ccCCCCCCCCCCcc.",
    ".cCCCcCCCCcCCCc.",
    ".cccccccccccccc.",
    "..s....ss....s..",
    "..s....ss....s..",
    "...s...ss...s...",
    "...s...ss...s...",
    "....s..ss..s....",
    "....s..ss..s....",
    ".....shhhhs.....",
    ".....hhhhhh.....",
    "................",
]
KEY = [
    "................",
    "................",
    "....kkk.........",
    "...k...k........",
    "...k...k........",
    "...k...k........",
    "....kkk.........",
    ".....k..........",
    ".....k..........",
    ".....k..........",
    ".....kk.........",
    ".....k..........",
    ".....kk.........",
    ".....k..........",
    "................",
    "................",
]
GEM = [
    "................",
    "................",
    "................",
    "......dddd......",
    ".....dLLLLd.....",
    "....dLLmmmmd....",
    "....dLmmmmmd....",
    "....dmmmmmmd....",
    "....dmmmmmdd....",
    ".....dmmmdd.....",
    "......dddd......",
    "................",
    "................",
    "................",
    "................",
    "................",
]
SHARD = [
    "................",
    "................",
    "..........dd....",
    ".........dLd....",
    "........dLmd....",
    ".......dLmmd....",
    "......dLmmd.....",
    ".....dLmmd......",
    "....dLmmd.......",
    "....dmmd........",
    "....ddd.........",
    "................",
    "................",
    "................",
    "................",
    "................",
]
PLATE = [
    "................",
    "................",
    "................",
    "...dddddddddd...",
    "...dLLLLLLLLd...",
    "...dLmmmmmmmd...",
    "...dLmmmmmmmd...",
    "...dLmmmmmmmd...",
    "...dLmmmmmmmd...",
    "...dmmmmmmmmd...",
    "...dddddddddd...",
    "................",
    "................",
    "................",
    "................",
    "................",
]
BERRY = [
    "................",
    "................",
    "................",
    "................",
    "......g.........",
    ".....dd.g.......",
    "....dLmd.dd.....",
    "....dmmd.dLmd...",
    ".....dd..dmmd...",
    "..........dd....",
    ".....dd.........",
    "....dLmd........",
    "....dmmd........",
    ".....dd.........",
    "................",
    "................",
]
EGG = [
    "................",
    "................",
    "................",
    ".......dd.......",
    "......dLLd......",
    ".....dLmmmd.....",
    ".....dmmsmd.....",
    "....dmmmmmmd....",
    "....dmsmmmmd....",
    "....dmmmmsmd....",
    "....dmmmmmmd....",
    ".....dmmmmd.....",
    ".....dmmmmd.....",
    "......dddd......",
    "................",
    "................",
]
DOOR_ICON = [
    "................",
    "....dddddddd....",
    "....dmmmmmmd....",
    "....dmLLLLmd....",
    "....dmLmmLmd....",
    "....dmLLLLmd....",
    "....dmmmmmmd....",
    "....dmmmmmmd....",
    "....dmmmsmmd....",
    "....dmLLLLmd....",
    "....dmLmmLmd....",
    "....dmLLLLmd....",
    "....dmmmmmmd....",
    "....dmmmmmmd....",
    "....dddddddd....",
    "................",
]
BED_ICON = [
    "................",
    "................",
    "................",
    "................",
    "................",
    "..LL............",
    ".LLLLsssssssss..",
    ".LLLLssssssssss.",
    ".dddddddddddddd.",
    ".dmmmmmmmmmmmmd.",
    ".dmmmmmmmmmmmmd.",
    ".dd..........dd.",
    ".dd..........dd.",
    "................",
    "................",
    "................",
]
SIGN_ICON = [
    "................",
    "................",
    "..dddddddddddd..",
    "..dmmmmmmmmmmd..",
    "..dmsssmmsssmd..",
    "..dmmmmmmmmmmd..",
    "..dmssmmssssmd..",
    "..dmmmmmmmmmmd..",
    "..dddddddddddd..",
    ".......dd.......",
    ".......dd.......",
    ".......dd.......",
    ".......dd.......",
    ".......dd.......",
    "................",
    "................",
]
PICK = [
    "................",
    ".......hhhh.....",
    ".....hhhmmhh....",
    "....hmmm..mhh...",
    "...hmm.....mh...",
    "...hm......hmh..",
    "...h......h.mh..",
    ".........h..mh..",
    "........h....h..",
    ".......h........",
    "......h.........",
    ".....h..........",
    "....h...........",
    "...h............",
    "..h.............",
    "................",
]
AXE = [
    "................",
    "......hhhh......",
    ".....hmmmmh.....",
    "....hmmmmmmh....",
    "....hmmhhmmh....",
    "....hmmh.hmh....",
    ".....hhh..hh....",
    "........h.......",
    ".......h........",
    "......h.........",
    ".....h..........",
    "....h...........",
    "...h............",
    "..h.............",
    "................",
    "................",
]
SHOVEL = [
    "................",
    "........hhh.....",
    ".......hmmmh....",
    ".......hmmmh....",
    "........hmh.....",
    ".........h......",
    "........h.......",
    ".......h........",
    "......h.........",
    ".....h..........",
    "....h...........",
    "...h............",
    "..h.............",
    "................",
    "................",
    "................",
]
SWORD = [
    "................",
    "...........hh...",
    "..........hmmh..",
    ".........hmmh...",
    "........hmmh....",
    ".......hmmh.....",
    "......hmmh......",
    ".....hmmh.......",
    "..h.hmmh........",
    "...hhmh.........",
    "...hhh..........",
    "..hhhh..........",
    ".h..hh..........",
    "h....h..........",
    "................",
    "................",
]


# ----------------------------------------------------------------------------- textures
def build_terrain():
    t = {}
    t['AETHER_GRASS_TOP'] = noise_block(GRASS_A, 0.10)
    side = noise_block(DIRT_A, 0.10)
    for y in range(3):
        for x in range(16):
            if y < 2 or rng.random() < 0.5:
                side[y][x] = shade(GRASS_A, 0.9 + rng.uniform(-0.08, 0.08)) + (255,)
    t['AETHER_GRASS_SIDE'] = side
    t['ENCH_GRASS_TOP'] = noise_block(ENCH_GRASS, 0.10, speck=(250, 250, 170), speck_p=0.05)
    side = noise_block(DIRT_A, 0.10)
    for y in range(3):
        for x in range(16):
            if y < 2 or rng.random() < 0.5:
                side[y][x] = shade(ENCH_GRASS, 0.9 + rng.uniform(-0.08, 0.08)) + (255,)
    t['ENCH_GRASS_SIDE'] = side
    t['AETHER_DIRT'] = noise_block(DIRT_A, 0.12, speck=shade(DIRT_A, 0.7), speck_p=0.08)
    holy = stone(HOLY)
    t['HOLYSTONE'] = holy
    mossy = [row[:] for row in holy]
    for y in range(16):
        for x in range(16):
            if rng.random() < 0.33:
                mossy[y][x] = shade((110, 150, 90), rng.uniform(0.8, 1.15)) + (255,)
    t['MOSSY_HOLYSTONE'] = mossy
    t['HOLYSTONE_BRICKS'] = bricks(HOLY, shade(HOLY, 0.66))
    ice = noise_block(ICE, 0.06)
    for i in range(16):
        put(ice, i, (i * 3) % 16, mix(ICE, (255, 255, 255), 0.6))
        put(ice, (i * 5) % 16, i, shade(ICE, 0.85))
    t['ICESTONE'] = ice
    t['QUICKSOIL'] = noise_block(QUICK, 0.07, speck=mix(QUICK, (255, 255, 255), 0.5), speck_p=0.1)
    t['AERCLOUD_COLD'] = cloud(CLOUD_COLD, 200)
    t['AERCLOUD_BLUE'] = cloud(CLOUD_BLUE, 205)
    t['AERCLOUD_GOLDEN'] = cloud(CLOUD_GOLD, 215)
    t['AERCLOUD_PURPLE'] = cloud(CLOUD_PURPLE, 205)
    t['AERCLOUD_GREEN'] = cloud(CLOUD_GREEN, 205)
    storm = cloud(CLOUD_STORM, 225)
    symbol(storm, ["...#", "..#.", ".###", "..#.", ".#.."], (255, 240, 120), 6, 5)
    t['AERCLOUD_STORM'] = storm
    t['AEROGEL'] = glass(AEROGEL, shade(AEROGEL, 0.7))
    t['SKYROOT_LOG_SIDE'] = log_side(SKY_BARK, shade(SKY_BARK, 0.75))
    t['SKYROOT_LOG_TOP'] = log_top(SKY_BARK, SKY_WOOD)
    t['GOLDEN_OAK_LOG_SIDE'] = log_side(GOLD_BARK, shade(GOLD_BARK, 0.72))
    t['GOLDEN_OAK_LOG_TOP'] = log_top(GOLD_BARK, GOLD_INNER)
    t['SKYROOT_PLANKS'] = planks(SKY_WOOD)
    t['SKYROOT_LEAVES'] = leaves((88, 168, 104))
    t['GOLDEN_OAK_LEAVES'] = leaves((222, 178, 70))
    t['SKYROOT_SAPLING'] = template(SAPLING, {'g': (88, 168, 104), 'G': (130, 200, 140), 'b': SKY_BARK})
    t['GOLDEN_OAK_SAPLING'] = template(SAPLING, {'g': (222, 178, 70), 'G': (250, 220, 130), 'b': GOLD_BARK})
    t['WHITE_FLOWER'] = template(FLOWER, {'p': (245, 245, 250), 'c': (250, 220, 90), 'g': (90, 170, 100)})
    t['PURPLE_FLOWER'] = template(FLOWER, {'p': (170, 90, 220), 'c': (250, 230, 120), 'g': (90, 170, 100)})
    t['BERRY_BUSH'] = template(BUSH, {'g': (70, 150, 90), 'r': (70, 150, 90), 'b': SKY_BARK})
    t['BERRY_BUSH_RIPE'] = template(BUSH, {'g': (70, 150, 90), 'r': (70, 80, 200), 'b': SKY_BARK})
    t['AMBROSIUM_ORE'] = ore(holy, AMBRO)
    t['ZANITE_ORE'] = ore(holy, ZANITE)
    t['GRAVITITE_ORE'] = ore(holy, GRAVITITE, 7)
    t['AMBROSIUM_BLOCK'] = framed(noise_block(AMBRO, 0.08), shade(AMBRO, 0.7))
    t['ZANITE_BLOCK'] = framed(noise_block(ZANITE, 0.08), shade(ZANITE, 0.7))
    t['ENCH_GRAVITITE'] = framed(noise_block(GRAVITITE, 0.10, speck=(255, 220, 250), speck_p=0.08), shade(GRAVITITE, 0.65))
    t['CARVED_STONE'] = framed(noise_block(CARVED, 0.05), shade(CARVED, 0.7), shade(CARVED, 0.9))
    sentry = framed(noise_block(SENTRY, 0.05), shade(SENTRY, 0.7), shade(SENTRY, 0.85))
    t['SENTRY_STONE'] = sentry
    lit = [row[:] for row in sentry]
    symbol(lit, ["#####", "#...#", "#...#", "#...#", "#####"], (120, 220, 255), 5, 5)
    t['LIGHT_SENTRY_STONE'] = lit
    t['ANGELIC_STONE'] = framed(noise_block(ANGELIC, 0.04), shade(ANGELIC, 0.75), mix(ANGELIC, (255, 215, 120), 0.3))
    la = framed(noise_block(ANGELIC, 0.04), shade(ANGELIC, 0.75))
    symbol(la, ["..#..", ".###.", "#####", ".###.", "..#.."], (255, 230, 140), 5, 5)
    t['LIGHT_ANGELIC_STONE'] = la
    t['HELLFIRE_STONE'] = framed(noise_block(HELLFIRE, 0.08), shade(HELLFIRE, 0.6), shade(HELLFIRE, 0.85))
    lh = framed(noise_block(HELLFIRE, 0.08), shade(HELLFIRE, 0.6))
    symbol(lh, ["..#..", ".#.#.", "#...#", ".#.#.", "..#.."], (255, 170, 60), 5, 5)
    t['LIGHT_HELLFIRE_STONE'] = lh
    ps = noise_block(ANGELIC, 0.03)
    for y in range(16):
        for x in (0, 1, 14, 15):
            put(ps, x, y, shade(ANGELIC, 0.7))
        for x in (5, 10):
            put(ps, x, y, shade(ANGELIC, 0.85))
    t['PILLAR_SIDE'] = ps
    t['PILLAR_TOP'] = framed(noise_block(ANGELIC, 0.03), shade(ANGELIC, 0.7))
    lock = ["..###..", ".#...#.", ".#...#.", "#######", "#######", "###.###", "#######"]
    lc = framed(noise_block(CARVED, 0.05), shade(CARVED, 0.7))
    symbol(lc, lock, (200, 130, 60), 4, 4)
    t['LOCKED_CARVED'] = lc
    la2 = framed(noise_block(ANGELIC, 0.04), shade(ANGELIC, 0.75))
    symbol(la2, lock, (200, 200, 210), 4, 4)
    t['LOCKED_ANGELIC'] = la2
    lh2 = framed(noise_block(HELLFIRE, 0.08), shade(HELLFIRE, 0.6))
    symbol(lh2, lock, (255, 210, 70), 4, 4)
    t['LOCKED_HELLFIRE'] = lh2
    door_l = planks(SKY_WOOD)
    door_u = planks(SKY_WOOD)
    for s in (door_l, door_u):
        for i in range(16):
            for (x, y) in ((i, 0), (i, 15), (0, i), (15, i)):
                put(s, x, y, shade(SKY_WOOD, 0.6))
        for y in range(3, 13):
            for x in range(3, 13):
                if x in (3, 12) or y in (3, 12):
                    put(s, x, y, shade(SKY_WOOD, 0.75))
    for y in range(5, 11):
        for x in range(5, 11):
            put(door_u, x, y, mix(ICE, (255, 255, 255), 0.3), 255)
    put(door_l, 13, 7, (60, 60, 60)); put(door_l, 13, 8, (60, 60, 60))
    t['SKYROOT_DOOR_LOWER'] = door_l
    t['SKYROOT_DOOR_UPPER'] = door_u
    trap = planks(SKY_WOOD)
    for i in range(16):
        for (x, y) in ((i, 0), (i, 15), (0, i), (15, i)):
            put(trap, x, y, shade(SKY_WOOD, 0.6))
    t['SKYROOT_TRAPDOOR'] = trap
    t['QUICKSOIL_GLASS'] = glass(QUICK, shade(QUICK, 0.75))
    t['CLOUDWOOL'] = wool((240, 240, 245))
    for name, col, sym in (
            ('ALTAR', (120, 110, 150), ["#...#", ".#.#.", "..#..", ".#.#.", "#...#"]),
            ('FREEZER', (150, 190, 230), [".#.#.", "..#..", "#####", "..#..", ".#.#."]),
            ('INCUBATOR', (200, 150, 110), ["..#..", ".###.", "#####", "#####", ".###."]),
            ('SUN_ALTAR', (230, 190, 90), ["#.#.#", ".###.", "#####", ".###.", "#.#.#"])):
        top = framed(noise_block(HOLY, 0.06), shade(HOLY, 0.65), col)
        symbol(top, sym, mix(col, (255, 255, 255), 0.7), 5, 5)
        t[name + '_TOP'] = top
        sd = framed(noise_block(HOLY, 0.06), shade(HOLY, 0.65))
        for y in range(4, 12):
            for x in range(4, 12):
                put(sd, x, y, shade(col, 0.9 + rng.uniform(-0.08, 0.08)))
        t[name + '_SIDE'] = sd
        if name != 'SUN_ALTAR':
            litside = [row[:] for row in sd]
            glow = (255, 230, 120) if name != 'FREEZER' else (200, 240, 255)
            for y in range(6, 10):
                for x in range(6, 10):
                    put(litside, x, y, glow)
            t[name + '_SIDE_LIT'] = litside
    wb_top = planks(SKY_WOOD)
    for y in range(2, 14):
        for x in range(2, 14):
            if (x + y) % 2 == 0:
                put(wb_top, x, y, shade(SKY_WOOD, 0.8))
    t['SKYROOT_WORKBENCH_TOP'] = wb_top
    wbs = planks(SKY_WOOD)
    symbol(wbs, ["##..##", "##..##"], shade(SKY_WOOD, 0.55), 5, 3)
    t['SKYROOT_WORKBENCH_SIDE'] = wbs
    wbf = planks(SKY_WOOD)
    symbol(wbf, ["..##..", ".####.", "..##.."], (150, 150, 160), 5, 3)
    t['SKYROOT_WORKBENCH_FRONT'] = wbf
    bedc = (120, 160, 230)
    bf = wool(bedc)
    for x in range(16):
        put(bf, x, 15, shade(SKY_WOOD, 0.8)); put(bf, x, 14, shade(SKY_WOOD, 0.8))
    t['SKYROOT_BED_TOP_FOOT'] = bf
    bh = wool(bedc)
    for y in range(2, 7):
        for x in range(2, 14):
            put(bh, x, y, (240, 240, 245))
    for x in range(16):
        put(bh, x, 0, shade(SKY_WOOD, 0.8)); put(bh, x, 1, shade(SKY_WOOD, 0.8))
    t['SKYROOT_BED_TOP_HEAD'] = bh
    bs = planks(SKY_WOOD)
    for y in range(0, 7):
        for x in range(16):
            put(bs, x, y, shade(bedc, 0.9 + rng.uniform(-0.06, 0.06)))
    for y in range(10, 16):
        for x in range(2, 14):
            put(bs, x, y, (0, 0, 0), 0)
    t['SKYROOT_BED_SIDE'] = bs
    be = [row[:] for row in bs]
    t['SKYROOT_BED_END'] = be
    ct = planks(SKY_WOOD)
    for i in range(16):
        for (x, y) in ((i, 0), (i, 15), (0, i), (15, i)):
            put(ct, x, y, shade(SKY_WOOD, 0.6))
    t['SKYROOT_CHEST_TOP'] = ct
    cs = planks(SKY_WOOD)
    for i in range(16):
        for (x, y) in ((i, 0), (i, 15), (0, i), (15, i), (i, 6)):
            put(cs, x, y, shade(SKY_WOOD, 0.6))
    t['SKYROOT_CHEST_SIDE'] = cs
    cf = [row[:] for row in cs]
    symbol(cf, ["##", "##", "##"], (150, 150, 160), 7, 5)
    t['SKYROOT_CHEST_FRONT'] = cf
    t['AMBROSIUM_TORCH'] = template(TORCH, {'y': (255, 210, 70), 'Y': (255, 250, 200), 'b': SKY_BARK})
    portal = blank()
    for y in range(16):
        for x in range(16):
            portal[y][x] = (120, 190, 255, 200)
    t['AETHER_PORTAL'] = portal
    return t


def build_items():
    it = {}
    it['AMBROSIUM_SHARD'] = template(SHARD, {'d': shade(AMBRO, 0.6), 'm': AMBRO, 'L': mix(AMBRO, (255, 255, 255), 0.6)})
    it['ZANITE_GEM'] = template(GEM, {'d': shade(ZANITE, 0.6), 'm': ZANITE, 'L': mix(ZANITE, (255, 255, 255), 0.6)})
    it['GRAVITITE_PLATE'] = template(PLATE, {'d': shade(GRAVITITE, 0.55), 'm': GRAVITITE, 'L': mix(GRAVITITE, (255, 255, 255), 0.5)})
    it['GOLDEN_AMBER'] = template(GEM, {'d': (150, 100, 20), 'm': (240, 180, 50), 'L': (255, 230, 150)})
    it['BLUEBERRY'] = template(BERRY, {'d': (40, 40, 120), 'm': (70, 80, 200), 'L': (150, 160, 240), 'g': (70, 150, 90)})
    it['MOA_EGG'] = template(EGG, {'d': (120, 130, 150), 'm': (200, 215, 235), 'L': (240, 245, 255), 's': (140, 170, 220)})
    it['BRONZE_KEY'] = template(KEY, {'k': (200, 130, 60)})
    it['SILVER_KEY'] = template(KEY, {'k': (210, 210, 220)})
    it['GOLD_KEY'] = template(KEY, {'k': (250, 210, 70)})
    it['SKYROOT_DOOR_ITEM'] = template(DOOR_ICON, {'d': shade(SKY_WOOD, 0.55), 'm': SKY_WOOD, 'L': shade(SKY_WOOD, 0.8), 's': (60, 60, 60)})
    it['SKYROOT_BED_ITEM'] = template(BED_ICON, {'d': shade(SKY_WOOD, 0.6), 'm': SKY_WOOD, 'L': (240, 240, 245), 's': (120, 160, 230)})
    it['SKYROOT_SIGN_ITEM'] = template(SIGN_ICON, {'d': shade(SKY_WOOD, 0.55), 'm': SKY_WOOD, 's': shade(SKY_WOOD, 0.7)})
    for mat, col in (('SKYROOT', SKY_WOOD), ('ZANITE', ZANITE), ('GRAVITITE', GRAVITITE)):
        pal = {'h': SKY_BARK, 'm': col}
        it[mat + '_PICKAXE'] = template(PICK, pal)
        it[mat + '_AXE'] = template(AXE, pal)
        it[mat + '_SHOVEL'] = template(SHOVEL, pal)
        it[mat + '_SWORD'] = template(SWORD, pal)
    # appended last so earlier icon indices stay stable
    it['COLD_PARACHUTE'] = template(PARACHUTE, {'c': (225, 235, 250), 'C': (250, 252, 255), 's': (120, 120, 130), 'h': shade(SKY_WOOD, 0.8)})
    return it


# ----------------------------------------------------------------------------- mob skins
def recolor_png(src, dst, fn):
    w, h, rows = g.read_png(src)
    out = []
    for y in range(h):
        line = bytearray(rows[y])
        for x in range(w):
            r, gg, b, a = line[x * 4:x * 4 + 4]
            if a:
                r, gg, b = fn(r, gg, b, x, y)
                line[x * 4:x * 4 + 3] = bytes((clamp(r), clamp(gg), clamp(b)))
        out.append(bytes(line))
    g.write_png(dst, w, h, out)


def moa_color(r, gg, b, x, y):
    lum = (r * 0.3 + gg * 0.59 + b * 0.11) / 255.0
    if r > 180 and gg < 100:          # red wattle -> golden beak/crest
        return (250, 200, 80)
    if r > 200 and gg > 150 and b < 120:  # yellow beak -> gold
        return (240, 190, 70)
    base = (170, 205, 245)
    return (base[0] * lum * 1.15, base[1] * lum * 1.15, base[2] * lum * 1.15)


def sentry_color(r, gg, b, x, y):
    lum = (r * 0.3 + gg * 0.59 + b * 0.11) / 255.0
    # glowing eyes on the head face (8..16, 8..16 in the 64x32 humanoid layout)
    if 8 <= x < 16 and 8 <= y < 16 and y in (11, 12) and x in (9, 10, 13, 14):
        return (120, 220, 255)
    base = (115, 115, 128)
    return (base[0] * (0.6 + lum * 0.7), base[1] * (0.6 + lum * 0.7), base[2] * (0.6 + lum * 0.7))


def mimic_color(r, gg, b, x, y):
    lum = (r * 0.3 + gg * 0.59 + b * 0.11) / 255.0
    if 8 <= x < 16 and 8 <= y < 16 and y in (11, 12) and x in (9, 10, 13, 14):
        return (255, 60, 60)
    base = SKY_WOOD
    f = 0.55 + lum * 0.8
    if (y % 4) == 3:
        f *= 0.7
    return (base[0] * f, base[1] * f, base[2] * f)


# ----------------------------------------------------------------------------- main
def main():
    tex = build_terrain()
    icons = build_items()
    assert len(tex) <= len(TERRAIN_POOL), (len(tex), len(TERRAIN_POOL))
    assert len(icons) <= len(ITEM_POOL), (len(icons), len(ITEM_POOL))

    w, h, rows = g.read_png(TERRAIN)
    assert (w, h) == (256, 256)
    tex_index = {}
    for i, (name, sprite) in enumerate(tex.items()):
        col, row = TERRAIN_POOL[i]
        g.blit(rows, col, row, sprite)
        tex_index[name] = row * 16 + col
    g.write_png(TERRAIN, w, h, rows)

    w, h, rows = g.read_png(ITEMS)
    assert (w, h) == (256, 256)
    icon_index = {}
    for i, (name, sprite) in enumerate(icons.items()):
        idx = ITEM_POOL[i]
        g.blit(rows, idx % 16, idx // 16, sprite)
        icon_index[name] = idx
    g.write_png(ITEMS, w, h, rows)

    recolor_png(os.path.join(MOBDIR, 'chicken.png'), os.path.join(MOBDIR, 'moa.png'), moa_color)
    recolor_png(os.path.join(MOBDIR, 'zombie.png'), os.path.join(MOBDIR, 'sentry.png'), sentry_color)
    recolor_png(os.path.join(MOBDIR, 'zombie.png'), os.path.join(MOBDIR, 'mimic.png'), mimic_color)

    os.makedirs(os.path.dirname(HEADER), exist_ok=True)
    with open(HEADER, 'w') as f:
        f.write('// GENERATED by tools/aether_assets.py -- do not edit by hand.\n')
        f.write('// terrain.png cells (tex = row*16 + col) and items.png icon indices used by the Aether.\n')
        f.write('#ifndef AETHER_TEX_H__\n#define AETHER_TEX_H__\n\nnamespace AetherTex {\n')
        for name, idx in tex_index.items():
            f.write('\tstatic const int %s = %d; // (%d,%d)\n' % (name, idx, idx % 16, idx // 16))
        f.write('}\n\nnamespace AetherIcon {\n')
        for name, idx in icon_index.items():
            f.write('\tstatic const int %s = %d; // (%d,%d)\n' % (name, idx, idx % 16, idx // 16))
        f.write('}\n\n#endif // AETHER_TEX_H__\n')
    print('terrain cells used: %d/%d, icons used: %d/%d' % (len(tex), len(TERRAIN_POOL), len(icons), len(ITEM_POOL)))


if __name__ == '__main__':
    main()
