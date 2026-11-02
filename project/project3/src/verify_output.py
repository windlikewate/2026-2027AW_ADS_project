# File: verify_output.py
# Description: Independent checker for one solver output.
#   Usage: python3 verify_output.py <input file> <output file>
#   Checks the output format and row-major order, that every printed side
#   points at the nearest connector and is matched from the other end,
#   that every degree is met, and that no two fences cross in an empty cell.
#   "No Solution" is accepted only when the degree sum is odd, or when
#   exhaustive search is affordable (<= 22 candidate edges) and finds nothing.

import itertools
import sys


def read_grid(path):
    nums = list(map(int, open(path).read().split()))
    n, m = nums[0], nums[1]
    return n, m, [nums[2 + i * m: 2 + (i + 1) * m] for i in range(n)]


def nearest(g, n, m, i, j, di, dj):
    # First connector strictly beyond (i, j) in direction (di, dj), or None.
    i, j = i + di, j + dj
    while 0 <= i < n and 0 <= j < m:
        if g[i][j] > 0:
            return (i, j)
        i, j = i + di, j + dj
    return None


def segments_cross(h, v):
    # h = (row, c1, c2), v = (col, r1, r2); open-interior intersection.
    return h[1] < v[0] < h[2] and v[1] < h[0] < v[2]


def check_drawing(n, m, g, lines):
    conns = [(i, j) for i in range(n) for j in range(m) if g[i][j] > 0]
    if len(lines) != len(conns):
        return "wrong number of lines"
    status = {}
    for (i, j), line in zip(conns, lines):
        if line != line.strip() or "  " in line:
            return "bad spacing"
        r, c, up, down, left, right = map(int, line.split())
        if (r - 1, c - 1) != (i, j):
            return "not row-major"
        status[(i, j)] = (up, down, left, right)
    hs, vs = [], []
    dirs = [(-1, 0), (1, 0), (0, -1), (0, 1)]
    for (i, j), s in status.items():
        if sum(s) != g[i][j]:
            return f"degree mismatch at {(i + 1, j + 1)}"
        for k, (di, dj) in enumerate(dirs):
            if not s[k]:
                continue
            t = nearest(g, n, m, i, j, di, dj)
            if t is None or not status[t][k ^ 1]:
                return f"dangling fence at {(i + 1, j + 1)}"
            if k == 3:
                hs.append((i, j, t[1]))
            if k == 1:
                vs.append((j, i, t[0]))
    for h in hs:
        for v in vs:
            if segments_cross(h, v):
                return "fences cross"
    return None


def has_solution(n, m, g):
    # Exhaustive search over candidate edges; only used for tiny inputs.
    edges = []
    for i in range(n):
        for j in range(m):
            if g[i][j] > 0:
                for di, dj in [(0, 1), (1, 0)]:
                    t = nearest(g, n, m, i, j, di, dj)
                    if t:
                        edges.append(((i, j), t))
    if len(edges) > 22:
        return None
    for mask in range(1 << len(edges)):
        sel = [edges[k] for k in range(len(edges)) if mask >> k & 1]
        deg = {}
        for a, b in sel:
            deg[a] = deg.get(a, 0) + 1
            deg[b] = deg.get(b, 0) + 1
        if any(deg.get((i, j), 0) != g[i][j] for i in range(n) for j in range(m)):
            continue
        hs = [(a[0], a[1], b[1]) for a, b in sel if a[0] == b[0]]
        vs = [(a[1], a[0], b[0]) for a, b in sel if a[1] == b[1]]
        if not any(segments_cross(h, v) for h, v in itertools.product(hs, vs)):
            return True
    return False


def main():
    n, m, g = read_grid(sys.argv[1])
    lines = [
        l for l in open(sys.argv[2]).read().splitlines()
        if l.strip() != "The answer is:"
    ]
    if lines == ["No Solution"]:
        if sum(map(sum, g)) % 2 == 1:
            print("OK (odd degree sum)")
        elif has_solution(n, m, g) is False:
            print("OK (exhaustive check)")
        else:
            print("UNVERIFIED No Solution")
            sys.exit(1)
        return
    err = check_drawing(n, m, g, lines)
    print("OK" if err is None else "FAIL: " + err)
    sys.exit(err is not None)


if __name__ == "__main__":
    main()
