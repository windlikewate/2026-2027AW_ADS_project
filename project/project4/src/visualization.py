"""
File: visualization.py
Description: Generate publication-grade vector PDF & PNG benchmark figures for
             ADS Project 4 (Improving Dynamic Programming for 0/1 Knapsack) in
             the Group 15 Navy / Amber Gold / Sage palette from existing CSVs:
               1) scaling_r10000_overview.pdf:
                  Runtime scaling & speedup ratios across n in {50,100,200,500,1000}
                  on KPLIB 00Uncorrelated (R = 10,000, 100 instances per n).
               2) runtime_vs_capacity_r10000.pdf:
                  Per-instance runtime vs. knapsack capacity c across n = 50, 100,
                  200, and 500/1000 on KPLIB 00Uncorrelated (R = 10,000).
               3) runtime_vs_n_fixed_c1000.pdf:
                  Runtime trajectory and empirical distribution across n in [50, 1000]
                  at fixed small capacity c = 1,000 (R = 1,000, 951 instances).
"""

import csv
from pathlib import Path
from statistics import mean, median, stdev
import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

plt.rcParams["font.sans-serif"] = ["DejaVu Sans"]
plt.rcParams["axes.unicode_minus"] = False

PRIMARY = "#1C375A"  # DP-1 (origin)
PROG = "#346958"     # DP-with-Lists (list)
ACCENT = "#B08034"   # DP-with-Upper-Bounds (bound)
MUTED = "#5F6976"
RED = "#A83232"

ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = ROOT / "data"
OUT_DIR = Path(__file__).resolve().parent / "visualizations"
OUT_DIR.mkdir(parents=True, exist_ok=True)


def load_csv(path: Path):
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


# =========================================================================
# Figure 1: KPLIB R=10000 Scaling Overview (n = 50, 100, 200, 500, 1000)
# =========================================================================
scales = [50, 100, 200, 500, 1000]
stats_r10k = {alg: {} for alg in ("origin", "list", "bound")}

for n in scales:
    rows = load_csv(DATA_DIR / f"all_n{n}_R10000.csv")
    for alg in ("origin", "list", "bound"):
        vals = sorted(
            float(r["time_seconds"]) * 1000.0
            for r in rows
            if r["algorithm"] == alg and r["status"] == "ok" and r["time_seconds"]
        )
        if vals:
            stats_r10k[alg][n] = {
                "mean": mean(vals),
                "median": median(vals),
                "p25": np.percentile(vals, 25),
                "p75": np.percentile(vals, 75),
                "sd": stdev(vals) if len(vals) > 1 else 0.0,
                "count": len(vals),
            }

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11.6, 4.45), gridspec_kw={"width_ratios": [1.22, 1.0]})

style_map = {
    "origin": ("DP-1 (origin)", PRIMARY, "o", "-"),
    "list": ("DP-with-Lists (list)", PROG, "s", "-"),
    "bound": ("DP-with-Upper-Bounds (bound)", ACCENT, "^", "-"),
}

for alg in ("origin", "list", "bound"):
    label, color, marker, ls = style_map[alg]
    xs = sorted(stats_r10k[alg].keys())
    ys = [stats_r10k[alg][x]["mean"] for x in xs]
    lo = [stats_r10k[alg][x]["p25"] for x in xs]
    hi = [stats_r10k[alg][x]["p75"] for x in xs]
    ax1.plot(xs, ys, marker=marker, color=color, linewidth=2.1, markersize=6.5, linestyle=ls, label=label)
    ax1.fill_between(xs, lo, hi, color=color, alpha=0.16)

# Mark OOM boundary for DP-1 at n >= 500
ax1.axvline(350, color=RED, linestyle=":", linewidth=1.35, alpha=0.85)
ax1.text(
    365, 95.0,
    "DP-1 > 1 GiB RAM\n(worker_failed at\nn = 500, 1000)",
    color=RED, fontsize=8.2, fontweight="bold",
    bbox=dict(boxstyle="round,pad=0.3", facecolor="#FFF6F6", edgecolor=RED, alpha=0.88, lw=0.7)
)

ax1.set_xscale("log")
ax1.set_yscale("log")
ax1.set_xticks(scales)
ax1.get_xaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())
ax1.set_xlabel("Number of Items n (KPLIB 00Uncorrelated, R = 10,000, c ~ 0.5 sum(w))")
ax1.set_ylabel("Mean Execution Time (ms, log scale)")
ax1.set_title("(a) Runtime Scaling Across Item Count n (Shaded: IQR)")
ax1.grid(True, which="both", linestyle="--", alpha=0.35)
ax1.legend(loc="upper left", fontsize=8.5, frameon=True)

# Panel (b): Speedup over DP-1 (n=50,100,200) + bound/list ratio across all n
ax2_twin = ax2.twinx()
bar_ns = [50, 100, 200]
x_idx = np.arange(len(scales))
width = 0.30

sp_list = [stats_r10k["origin"][n]["mean"] / stats_r10k["list"][n]["mean"] if n in stats_r10k["origin"] else 0 for n in scales]
sp_bound = [stats_r10k["origin"][n]["mean"] / stats_r10k["bound"][n]["mean"] if n in stats_r10k["origin"] else 0 for n in scales]

b1 = ax2.bar(x_idx[:3] - width / 2, sp_list[:3], width, color=PROG, alpha=0.86, label="Speedup: origin / list")
b2 = ax2.bar(x_idx[:3] + width / 2, sp_bound[:3], width, color=ACCENT, alpha=0.86, label="Speedup: origin / bound")

for rect in list(b1) + list(b2):
    h = rect.get_height()
    ax2.annotate(f"{h:.1f}x", xy=(rect.get_x() + rect.get_width() / 2, h),
                 xytext=(0, 2.5), textcoords="offset points", ha="center", va="bottom", fontsize=7.6, color="#222")

ratio_bl = [stats_r10k["bound"][n]["mean"] / stats_r10k["list"][n]["mean"] for n in scales]
l_rat = ax2_twin.plot(x_idx, ratio_bl, color=PRIMARY, marker="D", linewidth=2.0, linestyle="--",
                      markersize=5.8, label="Time ratio: bound / list")
ax2_twin.axhline(1.0, color=MUTED, linestyle=":", linewidth=1.0)

ax2.set_xticks(x_idx)
ax2.set_xticklabels([str(n) for n in scales])
ax2.set_xlabel("Number of Items n (KPLIB 00Uncorrelated, R = 10,000)")
ax2.set_ylabel("Speedup Factor vs. DP-1 (x)", color=PROG)
ax2_twin.set_ylabel("Runtime Ratio: bound / list", color=PRIMARY)
ax2.set_ylim(0, 66)
ax2_twin.set_ylim(0, 7.2)
ax2.set_title("(b) Speedup over DP-1 & Upper-Bound Crossover")
ax2.grid(True, axis="y", linestyle="--", alpha=0.35)

handles1, labels1 = ax2.get_legend_handles_labels()
handles2, labels2 = ax2_twin.get_legend_handles_labels()
ax2.legend(handles1 + handles2, labels1 + labels2, loc="upper right", fontsize=8.0, frameon=True)

plt.tight_layout()
plt.savefig(OUT_DIR / "scaling_r10000_overview.pdf")
plt.savefig(OUT_DIR / "scaling_r10000_overview.png", dpi=300)
plt.close()


# =========================================================================
# Figure 2: Runtime vs. Knapsack Capacity c across R=10000 Scales
# =========================================================================
fig, axes = plt.subplots(2, 2, figsize=(11.6, 7.2))
panel_specs = [
    (axes[0, 0], 50, "(a) n = 50, R = 10,000 (All 3 Solvers)"),
    (axes[0, 1], 100, "(b) n = 100, R = 10,000 (All 3 Solvers)"),
    (axes[1, 0], 200, "(c) n = 200, R = 10,000 (Crossover: list < bound << origin)"),
    (axes[1, 1], 500, "(d) n = 500, R = 10,000 (list vs. bound; origin > 1 GiB)"),
]

for ax, n, title in panel_specs:
    rows = load_csv(DATA_DIR / f"all_n{n}_R10000.csv")
    for alg in ("origin", "list", "bound"):
        pts = sorted(
            (int(r["capacity"]) / 1000.0, float(r["time_seconds"]) * 1000.0)
            for r in rows
            if r["algorithm"] == alg and r["status"] == "ok" and r["time_seconds"]
        )
        if not pts:
            continue
        label, color, marker, _ = style_map[alg]
        xs, ys = [p[0] for p in pts], [p[1] for p in pts]
        ax.scatter(xs, ys, s=15, color=color, alpha=0.62, marker=marker, label=f"{label} (mean {mean(ys):.2f} ms)")
        # Linear trend line
        coef = np.polyfit(xs, ys, 1)
        x_line = np.linspace(min(xs), max(xs), 50)
        ax.plot(x_line, np.polyval(coef, x_line), color=color, linewidth=1.7)

    if n in (50, 100, 200):
        ax.set_yscale("log")
        ax.set_ylabel("Runtime (ms, log scale)")
    else:
        ax.set_ylabel("Runtime (ms)")
    ax.set_xlabel(r"Knapsack Capacity $c$ ($\times 10^3$)")
    ax.set_title(title, fontsize=10.2)
    ax.grid(True, which="both", linestyle="--", alpha=0.35)
    ax.legend(loc="best", fontsize=7.8, frameon=True)

plt.tight_layout()
plt.savefig(OUT_DIR / "runtime_vs_capacity_r10000.pdf")
plt.savefig(OUT_DIR / "runtime_vs_capacity_r10000.png", dpi=300)
plt.close()


# =========================================================================
# Figure 3: Fixed Small Capacity c = 1,000, Varied n in [50, 1000] (951 inst)
# =========================================================================
rows_c1k = load_csv(DATA_DIR / "all_nvariable_C1000.csv")
series_c1k = {alg: {} for alg in ("origin", "list", "bound")}
for r in rows_c1k:
    if r["status"] == "ok" and r["time_seconds"]:
        series_c1k[r["algorithm"]][int(r["n"])] = float(r["time_seconds"]) * 1000.0

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11.6, 4.35), gridspec_kw={"width_ratios": [1.48, 1.0]})

window = 25
for alg in ("origin", "list", "bound"):
    label, color, _, _ = style_map[alg]
    ns_sorted = sorted(series_c1k[alg].keys())
    ys = [series_c1k[alg][k] for k in ns_sorted]
    ax1.scatter(ns_sorted, ys, s=5, color=color, alpha=0.22)
    rolling = np.convolve(ys, np.ones(window) / window, mode="valid")
    ax1.plot(ns_sorted[window - 1:], rolling, color=color, linewidth=2.1,
             label=f"{label} (25-pt avg, mean {mean(ys):.2f} ms)")

ax1.set_xlabel("Number of Items n (50 to 1,000, step 1; Fixed Capacity c = 1,000, R = 1,000)")
ax1.set_ylabel("Execution Time (ms)")
ax1.set_title("(a) Runtime Trajectory vs. Item Count n at Small Fixed Capacity c = 1,000")
ax1.set_xlim(40, 1015)
ax1.set_ylim(0, 4.55)
ax1.grid(True, linestyle="--", alpha=0.38)
ax1.legend(loc="upper left", fontsize=8.2, frameon=True)

# Panel (b): Boxplot distribution across the 951 instances
data_box = [[series_c1k[alg][k] for k in sorted(series_c1k[alg].keys())] for alg in ("origin", "list", "bound")]
bp = ax2.boxplot(
    data_box,
    tick_labels=["DP-1\n(origin)", "DP-with-Lists\n(list)", "DP-with-Bounds\n(bound)"],
    patch_artist=True,
    widths=0.46,
    showmeans=True,
    meanprops=dict(marker="D", markerfacecolor="white", markeredgecolor="#111", markersize=5.5),
    medianprops=dict(color="white", linewidth=1.8),
    flierprops=dict(marker="o", markersize=2.8, alpha=0.35),
)
for patch, color in zip(bp["boxes"], (PRIMARY, PROG, ACCENT)):
    patch.set_facecolor(color)
    patch.set_alpha(0.84)

ax2.set_ylabel("Execution Time Across 951 Instances (ms)")
ax2.set_title("(b) Runtime Distribution at Fixed c = 1,000")
ax2.grid(True, axis="y", linestyle="--", alpha=0.38)

plt.tight_layout()
plt.savefig(OUT_DIR / "runtime_vs_n_fixed_c1000.pdf")
plt.savefig(OUT_DIR / "runtime_vs_n_fixed_c1000.png", dpi=300)
plt.close()

print("Generated figures in:", OUT_DIR)
