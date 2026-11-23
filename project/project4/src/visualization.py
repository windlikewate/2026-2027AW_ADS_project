"""
File: visualization.py
Description: Generate publication-grade vector PDF & PNG benchmark figures for
             ADS Project 4 (Improving Dynamic Programming for 0/1 Knapsack) in
             the project Navy / Amber Gold / Sage palette from existing CSVs:
               1) scaling_r10000_overview.pdf:
                  Runtime scaling & speedup ratios across n in {50,100,200,500,1000}
                  on KPLIB 00Uncorrelated (R = 10,000, 100 instances per n).
               2) runtime_vs_capacity_r10000.pdf:
                  Per-instance runtime vs. knapsack capacity c across n = 50, 100,
                  200, and 500/1000 on KPLIB 00Uncorrelated (R = 10,000).
               3) runtime_vs_n_fixed_c1000.pdf:
                  Runtime trajectory and empirical distribution across n in [50, 1000]
                  at fixed small capacity c = 1,000 (R = 1,000, 951 instances).
               4) bonus_regimes_overview.pdf:
                  Controlled small-weight scaling (W = 16, n = 50..3200), paired
                  speedups over `w3`, and weight sensitivity (n = 500, c = 1,000,
                  W = 4..256) for the deterministic O(n + W^3) bonus solver.
"""

import argparse
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
BONUS = "#8156A5"  # Proximity + SMAWK (w3)
ALGORITHMS = ("origin", "list", "bound", "w3")

ROOT = Path(__file__).resolve().parent.parent
# Optional paths permit comparing a fresh four-solver suite without replacing
# the historical report CSVs or generated figures.
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--data-dir", type=Path, default=ROOT / "data")
parser.add_argument("--output-dir", type=Path, default=Path(__file__).resolve().parent / "visualizations")
args = parser.parse_args()
DATA_DIR = args.data_dir
OUT_DIR = args.output_dir
OUT_DIR.mkdir(parents=True, exist_ok=True)


def load_csv(path: Path):
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


# =========================================================================
# Figure 1: KPLIB R=10000 Scaling Overview (n = 50, 100, 200, 500, 1000)
# =========================================================================
scales = [50, 100, 200, 500, 1000]
stats_r10k = {alg: {} for alg in ALGORITHMS}

for n in scales:
    rows = load_csv(DATA_DIR / f"all_n{n}_R10000.csv")
    for alg in ALGORITHMS:
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
    "w3": ("Proximity + SMAWK (w3)", BONUS, "D", "-"),
}

for alg in ALGORITHMS:
    label, color, marker, ls = style_map[alg]
    xs = sorted(stats_r10k[alg].keys())
    if not xs:  # Old three-solver CSVs need no fabricated bonus measurements.
        continue
    ys = [stats_r10k[alg][x]["mean"] for x in xs]
    lo = [stats_r10k[alg][x]["p25"] for x in xs]
    hi = [stats_r10k[alg][x]["p75"] for x in xs]
    ax1.plot(xs, ys, marker=marker, color=color, linewidth=2.1, markersize=6.5, linestyle=ls, label=label)
    ax1.fill_between(xs, lo, hi, color=color, alpha=0.16)

# Mark OOM boundary for DP-1 at n >= 500
ax1.axvline(350, color=RED, linestyle=":", linewidth=1.35, alpha=0.85)
ax1.text(
    375, 95.0,
    "DP-1 > 1 GiB RAM\nat n = 500, 1000",
    color=RED, fontsize=8.2, fontweight="bold",
    bbox=dict(boxstyle="round,pad=0.3", facecolor="#FFF6F6", edgecolor=RED, alpha=0.88, lw=0.7)
)

ax1.set_yscale("log")
ax1.set_xlim(0, 1060)
ax1.set_xticks([0, 200, 400, 600, 800, 1000])
ax1.set_xlabel("Number of Items n on KPLIB R = 10,000")
ax1.set_ylabel("Mean Execution Time in ms, log scale")
ax1.set_title("(a) Runtime Scaling Across Item Count n")
ax1.grid(True, which="both", linestyle="--", alpha=0.35)
ax1.legend(loc="lower right", fontsize=8.2, frameon=True)

# Panel (b): Speedup over DP-1 (n=50,100,200) + bound/list ratio across linear n
ax2_twin = ax2.twinx()
bar_ns = [50, 100, 200]
speedup_algs = [a for a in ALGORITHMS[1:]
                if any(n in stats_r10k[a] for n in bar_ns)]

# Show only measured speedups with a successful baseline at the same numeric n.
for alg in speedup_algs:
    valid_ns = [n for n in scales if n in stats_r10k["origin"] and n in stats_r10k[alg]]
    ys = [stats_r10k["origin"][n]["mean"] / stats_r10k[alg][n]["mean"] for n in valid_ns]
    ax2.plot(valid_ns, ys, color=style_map[alg][1], marker=style_map[alg][2],
             linewidth=2.0, markersize=6.2, label=f"Speedup: origin / {alg}")

ratio_bl = [stats_r10k["bound"][n]["mean"] / stats_r10k["list"][n]["mean"] for n in scales]
ax2_twin.plot(scales, ratio_bl, color=PRIMARY, marker="D", linewidth=2.0, linestyle="--",
              markersize=5.8, label="Time ratio: bound / list")
ax2_twin.axhline(1.0, color=MUTED, linestyle=":", linewidth=1.0)

ax2.set_xlim(0, 1060)
ax2.set_xticks([0, 200, 400, 600, 800, 1000])
ax2.set_xlabel("Number of Items n on KPLIB R = 10,000")
ax2.set_ylabel("Speedup Factor vs. DP-1", color=PROG)
ax2_twin.set_ylabel("Runtime Ratio: bound / list", color=PRIMARY)
ax2.set_ylim(0, 64)
ax2_twin.set_ylim(0, 7.2)
ax2.set_title("(b) Speedup over DP-1 & Upper-Bound Crossover")
ax2.grid(True, linestyle="--", alpha=0.35)

handles1, labels1 = ax2.get_legend_handles_labels()
handles2, labels2 = ax2_twin.get_legend_handles_labels()
ax2.legend(handles1 + handles2, labels1 + labels2, loc="center right", fontsize=8.0, frameon=True)

plt.tight_layout()
plt.savefig(OUT_DIR / "scaling_r10000_overview.pdf")
plt.savefig(OUT_DIR / "scaling_r10000_overview.png", dpi=300)
plt.close()


# =========================================================================
# Figure 2: Runtime vs. Knapsack Capacity c across R=10000 Scales
# =========================================================================
fig, axes = plt.subplots(2, 2, figsize=(11.6, 7.2))
panel_specs = [
    (axes[0, 0], 50, "(a) n = 50, R = 10,000 (Available Solvers)"),
    (axes[0, 1], 100, "(b) n = 100, R = 10,000 (Available Solvers)"),
    (axes[1, 0], 200, "(c) n = 200, R = 10,000 (Crossover: list < bound << origin)"),
    (axes[1, 1], 500, "(d) n = 500, R = 10,000 (list vs. bound; origin > 1 GiB)"),
]

for ax, n, title in panel_specs:
    rows = load_csv(DATA_DIR / f"all_n{n}_R10000.csv")
    for alg in ALGORITHMS:
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
series_c1k = {alg: {} for alg in ALGORITHMS}
for r in rows_c1k:
    if r["algorithm"] in series_c1k and r["status"] == "ok" and r["time_seconds"]:
        series_c1k[r["algorithm"]][int(r["n"])] = float(r["time_seconds"]) * 1000.0

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11.6, 4.35), gridspec_kw={"width_ratios": [1.48, 1.0]})

window = 25
for alg in ALGORITHMS:
    label, color, _, _ = style_map[alg]
    ns_sorted = sorted(series_c1k[alg].keys())
    ys = [series_c1k[alg][k] for k in ns_sorted]
    if not ys:
        continue
    ax1.scatter(ns_sorted, ys, s=5, color=color, alpha=0.22)
    local_window = min(window, len(ys))
    rolling = np.convolve(ys, np.ones(local_window) / local_window, mode="valid")
    ax1.plot(ns_sorted[local_window - 1:], rolling, color=color, linewidth=2.1,
             label=f"{label} ({local_window}-pt avg, mean {mean(ys):.2f} ms)")

ax1.set_xlabel("Number of Items n (50 to 1,000, step 1; Fixed Capacity c = 1,000, R = 1,000)")
ax1.set_ylabel("Execution Time (ms)")
ax1.set_title("(a) Runtime Trajectory vs. Item Count n at Small Fixed Capacity c = 1,000")
ax1.set_xlim(40, 1015)
ax1.set_ylim(bottom=0)
ax1.grid(True, linestyle="--", alpha=0.38)
ax1.legend(loc="upper left", fontsize=8.2, frameon=True)

# Panel (b): Boxplot distribution across the 951 instances
box_algs = [a for a in ALGORITHMS if series_c1k[a]]
data_box = [[series_c1k[alg][k] for k in sorted(series_c1k[alg])] for alg in box_algs]
_bp_kw = dict(
    patch_artist=True,
    widths=0.46,
    showmeans=True,
    meanprops=dict(marker="D", markerfacecolor="white", markeredgecolor="#111", markersize=5.5),
    medianprops=dict(color="white", linewidth=1.8),
    flierprops=dict(marker="o", markersize=2.8, alpha=0.35),
)
_bp_labels = [{"origin": "DP-1\n(origin)", "list": "DP-Lists\n(list)",
               "bound": "DP-Bounds\n(bound)", "w3": "SMAWK\n(w3)"}[a]
              for a in box_algs]
try:
    bp = ax2.boxplot(data_box, tick_labels=_bp_labels, **_bp_kw)
except TypeError:
    bp = ax2.boxplot(data_box, labels=_bp_labels, **_bp_kw)
for patch, color in zip(bp["boxes"], (style_map[a][1] for a in box_algs)):
    patch.set_facecolor(color)
    patch.set_alpha(0.84)

ax2.set_ylabel("Execution Time Across 951 Instances (ms)")
ax2.set_title("(b) Runtime Distribution at Fixed c = 1,000")
ax2.grid(True, axis="y", linestyle="--", alpha=0.38)

plt.tight_layout()
plt.savefig(OUT_DIR / "runtime_vs_n_fixed_c1000.pdf")
plt.savefig(OUT_DIR / "runtime_vs_n_fixed_c1000.png", dpi=300)
plt.close()


# =========================================================================
# Figure 4: Bonus O(n + W^3) Small-Weight Advantage & Weight Sensitivity
# =========================================================================
bonus_n_path = DATA_DIR / "bonus_advantage_n.csv"
bonus_w_path = DATA_DIR / "bonus_advantage_w.csv"

if bonus_n_path.exists() and bonus_w_path.exists():
    def summarize_bonus_suite(csv_path: Path, x_field: str):
        raw = load_csv(csv_path)
        by_inst = {}
        for r in raw:
            if r["status"] == "ok" and r["time_seconds"]:
                key = (int(r[x_field]), r["dataset"], r["algorithm"])
                by_inst.setdefault(key, []).append(float(r["time_seconds"]) * 1000.0)
        inst_med = {k: median(v) for k, v in by_inst.items()}
        agg = {alg: {} for alg in ALGORITHMS}
        xs_all = sorted({k[0] for k in inst_med})
        for alg in ALGORITHMS:
            for x in xs_all:
                vals = sorted(v for (kx, _, ka), v in inst_med.items() if kx == x and ka == alg)
                if vals:
                    agg[alg][x] = {
                        "median": median(vals),
                        "p25": float(np.percentile(vals, 25)),
                        "p75": float(np.percentile(vals, 75)),
                    }
        return inst_med, agg, xs_all

    inst_n, agg_n, ns_bonus = summarize_bonus_suite(bonus_n_path, "n")
    _, agg_w, ws_bonus = summarize_bonus_suite(bonus_w_path, "max_weight")

    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(13.4, 4.35))

    short_labels = {
        "origin": "DP-1: origin",
        "list": "DP-Lists: list",
        "bound": "DP-Bounds: bound",
        "w3": "Bonus: w3",
    }

    # Panel (a): Small-weight runtime scaling at W = 16
    for alg in ALGORITHMS:
        _, color, marker, ls = style_map[alg]
        xs = sorted(agg_n[alg].keys())
        ys = [agg_n[alg][x]["median"] for x in xs]
        lo = [agg_n[alg][x]["p25"] for x in xs]
        hi = [agg_n[alg][x]["p75"] for x in xs]
        lw = 2.4 if alg == "w3" else 1.95
        ax1.plot(xs, ys, marker=marker, color=color, linewidth=lw, markersize=5.8,
                 linestyle=ls, label=short_labels[alg])
        ax1.fill_between(xs, lo, hi, color=color, alpha=0.15)

    ax1.set_xscale("log", base=2)
    ax1.set_yscale("log")
    ax1.set_xticks(ns_bonus)
    ax1.set_xticklabels([str(x) for x in ns_bonus], fontsize=8.2)
    ax1.set_xlabel("Item count n, log2 scale", fontsize=9.0)
    ax1.set_ylabel("Solver time in ms, log scale", fontsize=9.0)
    ax1.set_title("(a) Small weights: W = 16", fontsize=10.0)
    ax1.grid(True, which="both", linestyle="--", alpha=0.35)
    ax1.legend(loc="upper left", fontsize=7.8, frameon=True)

    # Panel (b): Paired speedup baseline / w3 at W = 16
    for alg in ("origin", "list", "bound"):
        _, color, marker, _ = style_map[alg]
        med_sp, lo_sp, hi_sp = [], [], []
        for x in ns_bonus:
            ds_list = sorted({ds for (kx, ds, _) in inst_n if kx == x})
            ratios = [inst_n[(x, ds, alg)] / inst_n[(x, ds, "w3")] for ds in ds_list]
            med_sp.append(median(ratios))
            lo_sp.append(float(np.percentile(ratios, 25)))
            hi_sp.append(float(np.percentile(ratios, 75)))
        ax2.plot(ns_bonus, med_sp, marker=marker, color=color, linewidth=2.0, markersize=5.8,
                 label=f"{alg} / w3")
        ax2.fill_between(ns_bonus, lo_sp, hi_sp, color=color, alpha=0.14)

    ax2.axhline(1.0, color=RED, linestyle="--", linewidth=1.25, label="1.0x")
    ax2.set_xscale("log", base=2)
    ax2.set_yscale("log")
    ax2.set_xticks(ns_bonus)
    ax2.set_xticklabels([str(x) for x in ns_bonus], fontsize=8.2)
    ax2.set_xlabel("Item count n, log2 scale", fontsize=9.0)
    ax2.set_ylabel("Paired speedup: baseline / w3, log scale", fontsize=9.0)
    ax2.set_title("(b) Bonus advantage at W = 16", fontsize=10.0)
    ax2.grid(True, which="both", linestyle="--", alpha=0.35)
    ax2.legend(loc="upper left", fontsize=7.8, frameon=True)

    # Panel (c): Weight sensitivity at fixed n = 500, c = 1,000
    for alg in ALGORITHMS:
        _, color, marker, ls = style_map[alg]
        xs = sorted(agg_w[alg].keys())
        ys = [agg_w[alg][x]["median"] for x in xs]
        lo = [agg_w[alg][x]["p25"] for x in xs]
        hi = [agg_w[alg][x]["p75"] for x in xs]
        lw = 2.4 if alg == "w3" else 1.95
        ax3.plot(xs, ys, marker=marker, color=color, linewidth=lw, markersize=5.8,
                 linestyle=ls, label=short_labels[alg])
        ax3.fill_between(xs, lo, hi, color=color, alpha=0.15)

    ax3.set_xscale("log", base=2)
    ax3.set_yscale("log")
    ax3.set_xticks(ws_bonus)
    ax3.set_xticklabels([str(x) for x in ws_bonus], fontsize=8.2)
    ax3.set_xlabel("Actual maximum weight W, log2 scale", fontsize=9.0)
    ax3.set_ylabel("Solver time in ms, log scale", fontsize=9.0)
    ax3.set_title("(c) Weight sensitivity", fontsize=10.0)
    ax3.grid(True, which="both", linestyle="--", alpha=0.35)
    ax3.legend(loc="lower right", fontsize=7.6, frameon=True)

    plt.tight_layout()
    plt.savefig(OUT_DIR / "bonus_regimes_overview.pdf")
    plt.savefig(OUT_DIR / "bonus_regimes_overview.png", dpi=300)
    plt.close()

print("Generated figures in:", OUT_DIR)
