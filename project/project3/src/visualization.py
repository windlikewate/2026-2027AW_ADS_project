# File: visualization.py
# Description: Generate benchmark and layout figures for Project 3 (Recover the Design):
#              1) End-to-end runtime across the 360 Hashi_100 instances (benchmark_results_100.csv)
#              2) Scaling of candidate edges, recursive calls, and pure solver time across L05-L20
#              3) Showcase of recovered non-crossing fence layouts on L08, L12, and Instance #1

import csv
import os
import re
import subprocess
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

plt.rcParams["font.sans-serif"] = ["DejaVu Sans"]
plt.rcParams["axes.unicode_minus"] = False

PRIMARY = "#1C375A"
ACCENT = "#B08034"
PROG = "#346958"
MUTED = "#5F6976"
RED = "#A83232"

os.makedirs("visualizations", exist_ok=True)

# ---------------------------------------------------------
# 1. Figure: Runtime trajectory + histogram across 360 instances
# ---------------------------------------------------------
bench_rows = []
with open("benchmark_results_100.csv", newline="", encoding="utf-8") as f:
    for r in csv.DictReader(f):
        bench_rows.append((int(r["ID"]), r["Size"], float(r["Runtime(s)"]) * 1000.0))

ids = [r[0] for r in bench_rows]
runtimes_ms = [r[2] for r in bench_rows]

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11.5, 4.4), gridspec_kw={"width_ratios": [1.55, 1.0]})

ax1.scatter(ids, runtimes_ms, s=14, color=PRIMARY, alpha=0.68, label="Instance runtime (ms)")
window = 15
rolling = np.convolve(runtimes_ms, np.ones(window) / window, mode="valid")
ax1.plot(range(window, len(runtimes_ms) + 1), rolling, color=ACCENT, linewidth=2.0, label="15-instance moving average")
ax1.axhline(np.mean(runtimes_ms), color=PROG, linestyle="--", linewidth=1.3, label=f"Mean = {np.mean(runtimes_ms):.2f} ms")

ax1.set_xlabel("Benchmark Instance ID (1 to 360, 16x16 Grid, 100 Connectors)")
ax1.set_ylabel("Execution Time (ms)")
ax1.set_title("(a) Per-Instance Execution Time Across 360 Benchmark Cases")
ax1.set_xlim(0, 365)
ax1.set_ylim(35, 198)
ax1.legend(loc="upper left", fontsize=8.5, frameon=True)
ax1.grid(True, linestyle="--", alpha=0.4)

ax2.hist(runtimes_ms, bins=24, color=PRIMARY, edgecolor="white", alpha=0.85)
ax2.axvline(np.median(runtimes_ms), color=ACCENT, linewidth=2.0, linestyle="-", label=f"Median = {np.median(runtimes_ms):.2f} ms")
ax2.axvline(np.mean(runtimes_ms), color=PROG, linewidth=2.0, linestyle="--", label=f"Mean = {np.mean(runtimes_ms):.2f} ms")
ax2.axvline(np.percentile(runtimes_ms, 95), color=RED, linewidth=1.6, linestyle=":", label=f"95th pct = {np.percentile(runtimes_ms, 95):.2f} ms")
ax2.set_xlabel("Execution Time (ms)")
ax2.set_ylabel("Number of Instances")
ax2.set_title("(b) Empirical Runtime Distribution (N = 360)")
ax2.legend(loc="upper right", fontsize=8.5, frameon=True)
ax2.grid(True, linestyle="--", alpha=0.4)

plt.tight_layout()
plt.savefig("visualizations/hashi100_runtime_overview.pdf")
plt.savefig("visualizations/hashi100_runtime_overview.png", dpi=300)
plt.close()

# ---------------------------------------------------------
# 2. Figure: Scaling on L05-L20 (5x5 to 20x20 grids)
# ---------------------------------------------------------
with open("../data/Hashi_Puzzles/recover_the_design_large_tests.txt", encoding="utf-8") as f:
    text = f.read().replace("\r\n", "\n")
pattern = r"\u6837\u4f8b (L\d+)\uff1a([^\n]+)\n(?:[^\n]+\n)?\u8f93\u5165\uff1a\n((?:\d+[^\n]*\n)+)"
cases = re.findall(pattern, text)

def profile(inp, reps=7):
    # Run testbench_profile multiple times and keep the minimum time.
    runs = [subprocess.run(["./testbench_profile"], input=inp, text=True,
                           capture_output=True).stdout.split() for _ in range(reps)]
    return min(runs, key=lambda o: float(o[6]))

dims, verts, cand_edges, sel_edges, rec_calls, solve_us = [], [], [], [], [], []
for name, desc, inp in cases:
    if not name.startswith("L"):
        continue
    r, c, v, e, s, calls, us, ok = profile(inp)
    dims.append(int(r))
    verts.append(int(v))
    cand_edges.append(int(e))
    sel_edges.append(int(s))
    rec_calls.append(int(calls))
    solve_us.append(float(us) / 1000.0)

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11.5, 4.3))

ax1.plot(dims, verts, marker="o", color=PRIMARY, linewidth=2.0, label=r"Connectors $|V|$")
ax1.plot(dims, cand_edges, marker="s", color=ACCENT, linewidth=2.0, label=r"Candidate edges $|E|$")
ax1.plot(dims, sel_edges, marker="^", color=PROG, linewidth=2.0, label=r"Selected fences $|E^*|$")
ax1.set_xlabel(r"Grid Dimension $n \times n$ ($n \in [5, 20]$)")
ax1.set_ylabel("Count")
ax1.set_title("(a) Structural Growth of Graph Vertices and Edges (L05-L20)")
ax1.set_xticks(dims)
ax1.legend(frameon=True, fontsize=9)
ax1.grid(True, linestyle="--", alpha=0.4)

ax2_twin = ax2.twinx()
l1 = ax2.plot(dims, rec_calls, marker="o", color=PRIMARY, linewidth=2.0, label="Recursive calls (log scale)")
l2 = ax2_twin.plot(dims, solve_us, marker="D", color=ACCENT, linewidth=2.0, linestyle="--", label="Pure search time (ms, log scale)")
ax2.set_yscale("log")
ax2_twin.set_yscale("log")
ax2.set_xlabel(r"Grid Dimension $n \times n$ ($n \in [5, 20]$)")
ax2.set_ylabel("Backtrack Recursive Calls (log scale)", color=PRIMARY)
ax2_twin.set_ylabel("Pure Backtracking Time (ms, log scale)", color=ACCENT)
ax2.set_title("(b) Search Tree Nodes and Runtime Scaling (L05-L20)")
ax2.set_xticks(dims)
ax2.grid(True, linestyle="--", alpha=0.4)

lines = l1 + l2
labels = [l.get_label() for l in lines]
ax2.legend(lines, labels, loc="upper left", frameon=True, fontsize=8.5)

plt.tight_layout()
plt.savefig("visualizations/large_tests_scaling.pdf")
plt.savefig("visualizations/large_tests_scaling.png", dpi=300)
plt.close()

# ---------------------------------------------------------
# 3. Figure: Showcase of recovered fence layouts
# ---------------------------------------------------------
subprocess.run(["gcc", "-O2", "-o", "/tmp/rtd_vis", "recover_the_design.c"], check=True)

def parse_grid_text(txt):
    nums = list(map(int, txt.split()))
    n, m = nums[0], nums[1]
    grid = [nums[2 + i * m : 2 + (i + 1) * m] for i in range(n)]
    return n, m, grid

def solve_and_draw(ax, txt, title_str, node_size, font_size, lw_fence):
    n, m, grid = parse_grid_text(txt)
    proc = subprocess.run(["/tmp/rtd_vis"], input=txt, text=True, capture_output=True, check=True)
    status = {}
    for line in proc.stdout.splitlines():
        parts = line.strip().split()
        if len(parts) == 6 and all(p.lstrip("-").isdigit() for p in parts):
            r, c, up, down, left, right = map(int, parts)
            status[(r - 1, c - 1)] = (up, down, left, right)

    # Background grid lines
    for i in range(n):
        ax.plot([0, m - 1], [i, i], color="#E5E9F0", linewidth=0.6, zorder=1)
    for j in range(m):
        ax.plot([j, j], [0, n - 1], color="#E5E9F0", linewidth=0.6, zorder=1)

    # Candidate edges (dashed) and selected fences (solid)
    sel_count = 0
    cand_count = 0
    for i in range(n):
        for j in range(m):
            if grid[i][j] > 0:
                nj = j + 1
                while nj < m and grid[i][nj] == 0:
                    nj += 1
                if nj < m:
                    cand_count += 1
                    if status.get((i, j), (0, 0, 0, 0))[3] == 1:
                        ax.plot([j, nj], [i, i], color=ACCENT, linewidth=lw_fence, solid_capstyle="round", zorder=3)
                        sel_count += 1
                    else:
                        ax.plot([j, nj], [i, i], color="#B8C2CC", linewidth=0.85, linestyle="--", zorder=2)
                ni = i + 1
                while ni < n and grid[ni][j] == 0:
                    ni += 1
                if ni < n:
                    cand_count += 1
                    if status.get((i, j), (0, 0, 0, 0))[1] == 1:
                        ax.plot([j, j], [i, ni], color=ACCENT, linewidth=lw_fence, solid_capstyle="round", zorder=3)
                        sel_count += 1
                    else:
                        ax.plot([j, j], [i, ni], color="#B8C2CC", linewidth=0.85, linestyle="--", zorder=2)

    xs, ys, degs = [], [], []
    for i in range(n):
        for j in range(m):
            if grid[i][j] > 0:
                xs.append(j)
                ys.append(i)
                degs.append(grid[i][j])
    ax.scatter(xs, ys, s=node_size, facecolor="#F4F6FA", edgecolor=PRIMARY, linewidth=1.35, zorder=4)
    for x, y, d in zip(xs, ys, degs):
        ax.text(x, y, str(d), ha="center", va="center", fontsize=font_size, fontweight="bold", color=PRIMARY, zorder=5)

    ax.set_xlim(-0.7, m - 0.3)
    ax.set_ylim(n - 0.3, -0.7)
    ax.set_aspect("equal")
    ax.set_xticks(range(0, m, 2 if m > 10 else 1))
    ax.set_xticklabels([str(x + 1) for x in range(0, m, 2 if m > 10 else 1)], fontsize=7.5)
    ax.set_yticks(range(0, n, 2 if n > 10 else 1))
    ax.set_yticklabels([str(y + 1) for y in range(0, n, 2 if n > 10 else 1)], fontsize=7.5)
    ax.set_title(f"{title_str}\n(|V|={len(xs)}, |E|={cand_count}, |E*|={sel_count})", fontsize=9.2, fontweight="bold", color=PRIMARY)

case_dict = {name: inp for name, desc, inp in cases}
with open("../data/Hashi_Puzzles/100_revised/Hs_16_100_25_00_001.has", encoding="utf-8") as f:
    hashi_001_txt = f.read()

fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(11.8, 4.1))
solve_and_draw(ax1, case_dict["L08"], "(a) Constructed Board L08 (8x8)", node_size=145, font_size=7.5, lw_fence=2.3)
solve_and_draw(ax2, case_dict["L12"], "(b) Constructed Board L12 (12x12)", node_size=85, font_size=6.5, lw_fence=1.9)
solve_and_draw(ax3, hashi_001_txt, "(c) Benchmark Instance #1 (16x16)", node_size=55, font_size=5.8, lw_fence=1.7)

plt.tight_layout()
plt.savefig("visualizations/recovered_layouts_showcase.pdf")
plt.savefig("visualizations/recovered_layouts_showcase.png", dpi=300)
plt.close()
print("All figures generated successfully.")
