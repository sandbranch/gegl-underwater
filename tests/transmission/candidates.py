"""Candidate transmission estimates for tests/transmission/compare.py.

Each takes the small linear copy (h x w x 3, floats 0 to 1), the water
colour of the operation (3 floats) and the operation's own transmission,
and returns a transmission map (h x w, 0 far to 1 near).
"""
import numpy as np

CANDIDATES = {}


def candidate(func):
    CANDIDATES[func.__name__] = func
    return func


def min_filter(a, r):
    from numpy.lib.stride_tricks import sliding_window_view
    p = np.pad(a, r, mode='edge')
    return sliding_window_view(p, (2 * r + 1, 2 * r + 1)).min(axis=(-1, -2))


def box(a, r):
    c = np.pad(a, ((r + 1, r), (r + 1, r)), mode='edge').cumsum(0).cumsum(1)
    k = 2 * r + 1
    return (c[k:, k:] - c[:-k, k:] - c[k:, :-k] + c[:-k, :-k]) / (k * k)


def guided(I, p, r, eps):
    mI, mp = box(I, r), box(p, r)
    cov = box(I * p, r) - mI * mp
    var = box(I * I, r) - mI * mI
    a = cov / (var + eps)
    b = mp - a * mI
    return box(a, r) * I + box(b, r)


def luma(img):
    return img @ np.array([0.2126, 0.7152, 0.0722], np.float32)


@candidate
def udcp(img, water, t_op):
    """The operation's rule without the water map: a check of the harness."""
    v = min_filter(np.minimum(img[..., 1] / water[1], img[..., 2] / water[2]), 1)
    t = 1 - 0.9 * v
    return np.clip(guided(luma(img), t, max(4, img.shape[1] // 40), 1e-3), 0.1, 1)


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


def local_cv(l, r):
    m1 = box(l, r)
    m2 = box(l * l, r)
    return np.sqrt(np.maximum(m2 - m1 * m1, 0)) / np.maximum(m1, 1e-4)


def open_water(img, water):
    """How much each pixel looks like open water: the water's colour
    direction (not its brightness) and smooth at two scales."""
    dot = img @ water
    cos = dot / np.maximum(np.linalg.norm(img, axis=2) * np.linalg.norm(water), 1e-6)
    l = luma(img)
    cv = np.maximum(local_cv(l, 2), local_cv(l, 6))
    return smoothstep(0.97, 0.995, cos) * (1 - smoothstep(0.03, 0.10, cv))


@candidate
def openwater_score(img, water, t_op):
    """Not a transmission: the open water score itself (white = water)."""
    return open_water(img, water)


@candidate
def openwater(img, water, t_op):
    """The operation's t, lowered to open water's where the pixel is open water."""
    w = open_water(img, water)
    t = t_op * (1 - w) + 0.1 * w
    return np.clip(guided(luma(img), t, max(4, img.shape[1] // 40), 1e-3), 0.1, 1)


def open_water2(img, water):
    """open_water, with smoothness on gamma-encoded brightness (noise in
    dark water counts about the same as in bright water) and the score
    softened over a few pixels."""
    dot = img @ water
    cos = dot / np.maximum(np.linalg.norm(img, axis=2) * np.linalg.norm(water), 1e-6)
    l = np.sqrt(luma(img))
    sd = lambda r: np.sqrt(np.maximum(box(l * l, r) - box(l, r) ** 2, 0))
    rough = np.maximum(sd(2), sd(6)) / np.maximum(box(l, 6), 1e-3)
    w = smoothstep(0.97, 0.995, cos) * (1 - smoothstep(0.015, 0.05, rough))
    return box(w, 3)


@candidate
def openwater2_score(img, water, t_op):
    return open_water2(img, water)


@candidate
def openwater2(img, water, t_op):
    w = open_water2(img, water)
    t = t_op * (1 - w) + 0.1 * w
    return np.clip(guided(luma(img), t, max(4, img.shape[1] // 40), 1e-3), 0.1, 1)


def background_surface(img, water, min_area=0.03):
    """The water's own colour B(x), fitted per channel as a quadratic
    surface to the open water: smooth, water-coloured pixels in large
    connected regions, with outliers rejected twice. The constant water
    colour where there is too little open water."""
    from scipy import ndimage
    h, w = img.shape[:2]
    score = open_water2(img, water)
    mask = score > 0.5
    lab, n = ndimage.label(mask)
    if n:
        sizes = ndimage.sum(mask, lab, range(1, n + 1))
        keep = np.isin(lab, 1 + np.flatnonzero(sizes >= min_area * h * w))
    else:
        keep = mask
    if keep.sum() < 0.02 * h * w:
        return np.broadcast_to(water, img.shape).copy(), keep
    yy, xx = np.mgrid[0:h, 0:w]
    X = (2 * xx / max(w - 1, 1) - 1).astype(np.float64)
    Y = (2 * yy / max(h - 1, 1) - 1).astype(np.float64)
    basis = np.stack([np.ones_like(X), X, Y, X * X, X * Y, Y * Y], -1)
    B = np.empty_like(img)
    # the curved terms are damped (ridge), so that a fit to one side of the
    # photo does not run away on the other side
    ridge = np.diag([0, 0, 0, 1, 1, 1]).astype(np.float64) * 0.05 * keep.sum()
    for c in range(3):
        sel = keep.copy()
        for _ in range(3):
            A = basis[sel]
            coef = np.linalg.solve(A.T @ A + ridge, A.T @ img[..., c][sel])
            fit = basis @ coef
            res = img[..., c] - fit
            mad = np.median(np.abs(res[sel])) + 1e-6
            sel = keep & (np.abs(res) < 3.0 * 1.4826 * mad)
        # no colour the open water does not have
        lo, hi = np.percentile(img[..., c][sel], [1, 99])
        B[..., c] = np.clip(fit, max(lo * 0.8, 1e-4), min(hi * 1.1, 1.0))
    return B, keep


@candidate
def bsurf_samples(img, water, t_op):
    """Not a transmission: the open water used for the fit (white)."""
    return background_surface(img, water)[1].astype(np.float32)


@candidate
def bsurf_colour(img, water, t_op):
    """Not a transmission: the fitted water colour, gamma encoded."""
    B = background_surface(img, water)[0]
    return (np.clip(B, 0, 1) ** (1 / 2.2) * 255).astype(np.uint8)


@candidate
def bsurf(img, water, t_op):
    """Boundary constraint against B(x) (Meng 2013, GDCP Eq. 9): darker or
    brighter than the water behind it, in green or blue, is near."""
    B, _ = background_surface(img, water)
    tb = np.zeros(img.shape[:2], np.float32)
    for c in (1, 2):
        d = (B[..., c] - img[..., c]) / B[..., c]
        u = (img[..., c] - B[..., c]) / np.maximum(1 - B[..., c], 1e-3)
        tb = np.maximum(tb, np.maximum(d, u))
    from scipy import ndimage
    tb = ndimage.maximum_filter(np.clip(tb / 0.9, 0, 1), size=3)
    return np.clip(guided(luma(img), tb, max(4, img.shape[1] // 40), 1e-3), 0.1, 1)


@candidate
def bsurf_dark(img, water, t_op):
    """The operation's rule (darker than the water) against B(x) only."""
    B, _ = background_surface(img, water)
    from scipy import ndimage
    v = ndimage.minimum_filter(np.minimum(img[..., 1] / B[..., 1], img[..., 2] / B[..., 2]), size=3)
    t = 1 - 0.9 * v
    return np.clip(guided(luma(img), t, max(4, img.shape[1] // 40), 1e-3), 0.1, 1)
