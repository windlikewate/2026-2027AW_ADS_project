"""
File: visualization.py
Description: Generate publication-grade vector PDF and 300-DPI PNG benchmark
             figures for ADS Project 5 in the unified report palette from
             output/results.csv:
               1) runtime_and_k_scaling.pdf:
                  Panel a: Solver runtime scaling across N in [16, 100000]
                  Panel b: Heap extractions K versus total units Q*
               2) complexity_fit_grid.pdf:
                  2x3 per-profile least-squares fit T = a*N*log2(N) + b*K*log2(N) + c
               3) normalized_work_and_regimes.pdf:
                  Panel a: Normalized time T / [(N + K)*log2(N)] in ns vs. N
                  Panel b: Mean runtime comparison across profiles at N = 100,000
"""

import argparse
import csv
from pathlib import Path
import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

plt.rcParams["font.sans-serif"] = ["DejaVu Sans"]
plt.rcParams["axes.unicode_minus"] = False

# Report Color Palette
PRIMARY = "#1C375A"  # Navy
PROG    = "#346958"  # Sage Green
ACCENT  = "#B08034"  # Amber Gold
RED     = "#A83232"  # Crimson
PLUM    = "#8156A5"  # Plum
MUTED   = "#5F6976"  # Slate

PROFILES = (
    "random",
    "near_limit",
    "ordered",
    "unaffordable",
    "duplicates",
    "many_purchases",
)

STYLE = {
    "random":         ("random",         PRIMARY, "o", "-"),
    "near_limit":     ("near_limit",     RED,     "^", "-"),
    "ordered":        ("ordered",        PLUM,    "D", "-"),
    "unaffordable":   ("unaffordable",   MUTED,   "v", "--"),
    "duplicates":     ("duplicates",     ACCENT,  "s", "-"),
    "many_purchases": ("many_purchases", PROG,    "P", "-"),
}


def nonnegative_fit(x: np.ndarray, y: np.ndarray) -> np.ndarray:
    """Exact active-set non-negative least squares for 3 columns: a, b, c >= 0."""
    scale = np.maximum(np.max(x, axis=0), 1.0)
    z = x / scale
    best = np.zeros(x.shape[1])
    best_error = float(y @ y)
    for mask in range(1, 1 << x.shape[1]):
        active = [j for j in range(x.shape[1]) if mask & (1 << j)]
        coef = np.linalg.lstsq(z[:, active], y, rcond=None)[0]
        if np.any(coef < 0):
            continue
        trial = np.zeros(x.shape[1])
        trial[active] = coef
        error = float(np.sum((z @ trial - y) ** 2))
        if error < best_error:
            best, best_error = trial, error
    return best / scale


def save_both(fig: plt.Figure, out_dir: Path, stem: str) -> None:
    fig.savefig(out_dir / f"{stem}.pdf", bbox_inches="tight")
    fig.savefig(out_dir / f"{stem}.png", dpi=300, bbox_inches="tight")
    plt.close(fig)


def main() -> None:
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv", type=Path, default=root / "output" / "results.csv")
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(__file__).resolve().parent / "visualizations",
    )
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)

    with args.csv.open("r", encoding="utf-8-sig", newline="") as f:
        raw = list(csv.DictReader(f))

    rows = []
    for r in raw:
        if r["profile"] not in PROFILES:
            continue
        if r["status"] != "PASS":
            raise ValueError(f"Non-PASS benchmark row: {r['file']}")
        n = int(r["n"])
        k = int(r["k"])
        items = int(r["items"])
        avg_ms = float(r["avg_ms"])
        min_ms = float(r["min_ms"])
        max_ms = float(r["max_ms"])
        nlogn = n * np.log2(n)
        klogn = k * np.log2(n)
        rows.append(
            dict(
                profile=r["profile"],
                seed=r["seed"],
                n=n,
                items=items,
                k=k,
                avg_ms=avg_ms,
                min_ms=min_ms,
                max_ms=max_ms,
                nlogn=nlogn,
                klogn=klogn,
                ns_per_wu=1e6 * avg_ms / (nlogn + klogn),
            )
        )

    # =========================================================================
    # Figure 1: Runtime Scaling & Heap-Purchase Frontier Count K vs. N
    # =========================================================================
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11.8, 3.65), layout="constrained")

    for prof in PROFILES:
        label, color, marker, ls = STYLE[prof]
        sub = [r for r in rows if r["profile"] == prof]
        ns = sorted({r["n"] for r in sub})
        means = np.array([np.mean([r["avg_ms"] for r in sub if r["n"] == n]) for n in ns])
        lows  = np.array([min(r["min_ms"] for r in sub if r["n"] == n) for n in ns])
        highs = np.array([max(r["max_ms"] for r in sub if r["n"] == n) for n in ns])
        ax1.errorbar(
            ns,
            means,
            yerr=[means - lows, highs - means],
            marker=marker,
            color=color,
            linestyle=ls,
            linewidth=1.9,
            markersize=5.5,
            capsize=2.5,
            label=label,
        )

        ks = np.array([np.mean([r["k"] for r in sub if r["n"] == n]) for n in ns])
        ax2.plot(
            ns,
            ks,
            marker=marker,
            color=color,
            linestyle=ls,
            linewidth=1.9,
            markersize=5.5,
            label=label,
        )

    # Overlay total items Q* for many_purchases to show O(1) bulk-cutoff savings
    mp_sub = [r for r in rows if r["profile"] == "many_purchases"]
    mp_ns = sorted({r["n"] for r in mp_sub})
    mp_q = np.array([np.mean([r["items"] for r in mp_sub if r["n"] == n]) for n in mp_ns])
    mp_k = np.array([np.mean([r["k"] for r in mp_sub if r["n"] == n]) for n in mp_ns])
    ax2.plot(
        mp_ns,
        mp_q,
        color=PROG,
        linestyle=":",
        linewidth=1.7,
        marker="o",
        markersize=4.0,
        alpha=0.85,
        label="many_purchases total Q*",
    )
    ax2.fill_between(mp_ns, mp_k, mp_q, color=PROG, alpha=0.11)
    ax2.annotate(
        "O(1) bulk-cutoff gap\nbetween total Q* and K",
        xy=(64, 501008),
        xytext=(55, 2500),
        fontsize=7.8,
        fontweight="bold",
        color=PROG,
        bbox=dict(boxstyle="round,pad=0.28", facecolor="#F4FAF7", edgecolor=PROG, lw=0.7),
        arrowprops=dict(arrowstyle="->", color=PROG, lw=1.0),
    )

    ax1.set_xscale("log")
    ax1.set_yscale("log")
    ax1.set_xlabel("Problem Size N", fontsize=9.5)
    ax1.set_ylabel("Execution Time in ms, log scale", fontsize=9.5)
    ax1.set_title("a. Measured Runtime Across Input Profiles", fontsize=10.2, fontweight="bold", color=PRIMARY)
    ax1.grid(True, which="both", linestyle="--", alpha=0.28)
    ax1.spines["top"].set_visible(False)
    ax1.spines["right"].set_visible(False)
    ax1.legend(loc="upper left", fontsize=8.0, ncol=2, frameon=True)

    ax2.set_xscale("log")
    ax2.set_yscale("symlog", linthresh=10)
    ax2.set_ylim(bottom=0, top=2.2e6)
    ax2.set_xlabel("Problem Size N", fontsize=9.5)
    ax2.set_ylabel("Purchase Count, symlog scale", fontsize=9.5)
    ax2.set_title("b. Heap Extractions K vs. Total Units Q*", fontsize=10.2, fontweight="bold", color=PRIMARY)
    ax2.grid(True, which="both", linestyle="--", alpha=0.28)
    ax2.spines["top"].set_visible(False)
    ax2.spines["right"].set_visible(False)
    ax2.legend(loc="lower right", fontsize=7.6, ncol=2, frameon=True)

    save_both(fig, args.output_dir, "runtime_and_k_scaling")

    # =========================================================================
    # Figure 2: 2x3 Per-Profile Least-Squares Complexity Fit Grid
    # =========================================================================
    fig, axes = plt.subplots(2, 3, figsize=(12.2, 5.25), layout="constrained")

    for ax, prof in zip(axes.flat, PROFILES):
        label, color, marker, _ = STYLE[prof]
        sub = sorted([r for r in rows if r["profile"] == prof], key=lambda r: (r["n"], r["seed"]))
        x = np.array([[r["nlogn"], r["klogn"], 1.0] for r in sub])
        y = np.array([r["avg_ms"] for r in sub])
        coef = nonnegative_fit(x, y)
        pred = x @ coef
        sst = float(np.sum((y - y.mean()) ** 2))
        r2 = 1.0 - float(np.sum((y - pred) ** 2)) / sst if sst > 0 else float("nan")

        ns = sorted({r["n"] for r in sub})
        mean_act = [np.mean([r["avg_ms"] for r, _ in zip(sub, pred) if r["n"] == n]) for n in ns]
        mean_fit = [np.mean([p for r, p in zip(sub, pred) if r["n"] == n]) for n in ns]

        ax.scatter([r["n"] for r in sub], y, color=color, alpha=0.45, s=20, zorder=2)
        ax.plot(ns, mean_act, marker=marker, color=color, linewidth=1.9, markersize=4.8, label="Measured mean", zorder=3)
        ax.plot(ns, mean_fit, marker="s", color="#222222", linestyle="--", linewidth=1.4, markersize=3.8, label="Least-squares fit", zorder=4)
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_title(f"{label}, $R^2 = {r2:.4f}$", fontsize=9.8, fontweight="bold", color=PRIMARY)
        ax.set_xlabel("N", fontsize=8.8)
        ax.set_ylabel("Time in ms", fontsize=8.8)
        ax.grid(True, which="both", linestyle="--", alpha=0.26)
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)
        ax.legend(loc="lower right", fontsize=7.6, frameon=True)

    fig.suptitle(
        r"Empirical Least-Squares Fit to $T(N, K) = a\,N\log_2 N + b\,K\log_2 N + c$ Across Six Input Regimes",
        fontsize=10.8,
        fontweight="bold",
        color=PRIMARY,
    )
    save_both(fig, args.output_dir, "complexity_fit_grid")

    # =========================================================================
    # Figure 3: Normalized Work Cost & N = 100,000 Regime Breakdown
    # =========================================================================
    fig, (ax1, ax2) = plt.subplots(
        1, 2, figsize=(11.8, 3.60), gridspec_kw={"width_ratios": [1.18, 1.0]}, layout="constrained"
    )

    for prof in PROFILES:
        label, color, marker, ls = STYLE[prof]
        sub = [r for r in rows if r["profile"] == prof]
        ns = sorted({r["n"] for r in sub})
        norm_vals = [np.mean([r["ns_per_wu"] for r in sub if r["n"] == n]) for n in ns]
        ax1.plot(
            ns,
            norm_vals,
            marker=marker,
            color=color,
            linestyle=ls,
            linewidth=1.9,
            markersize=5.3,
            label=label,
        )

    ax1.set_xscale("log")
    ax1.set_yscale("log")
    ax1.set_xlabel("Problem Size N", fontsize=9.5)
    ax1.set_ylabel(r"Normalized Time $T / [(N + K)\log_2 N]$ in ns", fontsize=9.5)
    ax1.set_title("a. Empirical Cost per Nominal Work Unit", fontsize=10.2, fontweight="bold", color=PRIMARY)
    ax1.grid(True, which="both", linestyle="--", alpha=0.28)
    ax1.spines["top"].set_visible(False)
    ax1.spines["right"].set_visible(False)
    ax1.legend(loc="lower left", fontsize=7.9, ncol=2, frameon=True)

    # Panel b: Horizontal bar chart of N = 100,000 mean runtime with K annotations
    order_100k = [
        "many_purchases",
        "duplicates",
        "random",
        "unaffordable",
        "ordered",
        "near_limit",
    ]
    y_pos = np.arange(len(order_100k))
    means_100k = []
    errs_lo = []
    errs_hi = []
    k_100k = []
    bar_colors = []
    for prof in order_100k:
        sub = [r for r in rows if r["profile"] == prof and r["n"] == 100000]
        m = float(np.mean([r["avg_ms"] for r in sub]))
        means_100k.append(m)
        errs_lo.append(m - min(r["min_ms"] for r in sub))
        errs_hi.append(max(r["max_ms"] for r in sub) - m)
        k_100k.append(int(round(np.mean([r["k"] for r in sub]))))
        bar_colors.append(STYLE[prof][1])

    bars = ax2.barh(
        y_pos,
        means_100k,
        xerr=[errs_lo, errs_hi],
        height=0.58,
        color=bar_colors,
        alpha=0.88,
        capsize=3,
        edgecolor=PRIMARY,
        linewidth=0.6,
    )
    ax2.set_yticks(y_pos)
    ax2.set_yticklabels(order_100k, fontsize=8.8)
    ax2.set_xlim(0, 14.3)
    ax2.set_xlabel("Mean Execution Time in ms", fontsize=9.5)
    ax2.set_title("b. Mean Runtime at N = 100,000 and D = 1,000,000", fontsize=10.2, fontweight="bold", color=PRIMARY)
    ax2.grid(True, axis="x", linestyle="--", alpha=0.28)
    ax2.spines["top"].set_visible(False)
    ax2.spines["right"].set_visible(False)

    for bar, m, k_val in zip(bars, means_100k, k_100k):
        ax2.text(
            m + 0.42,
            bar.get_y() + bar.get_height() / 2.0,
            f"{m:.2f} ms, K={k_val:,}",
            va="center",
            ha="left",
            fontsize=7.8,
            color=PRIMARY,
            fontweight="bold",
        )

    save_both(fig, args.output_dir, "normalized_work_and_regimes")
    print(f"Generated 3 vector PDF + 3 PNG figures in {args.output_dir}")


if __name__ == "__main__":
    main()
