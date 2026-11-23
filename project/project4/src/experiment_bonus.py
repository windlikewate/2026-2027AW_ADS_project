"""Reproduce the small-weight advantage and large-weight cost of the bonus solver.

Run with a Python environment containing matplotlib and numpy. Uses the existing
isolated C benchmark, without changing any solver or including input/startup in
solver timings. --plot-only regenerates figures from the recorded measurements.
"""
import argparse
import csv
import random
import subprocess
from collections import defaultdict
from pathlib import Path
from statistics import median

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parent.parent
ALGORITHMS = ("origin", "list", "bound", "w3")
COLORS = dict(zip(ALGORITHMS, ("#2563eb", "#059669", "#e76f00", "#9333ea")))
SIZES = (50, 100, 200, 400, 800, 1600, 3200)
WEIGHTS = (4, 8, 16, 32, 64, 128, 256)
SEEDS = (20261121, 20261122, 20261123)


def generate(root):
    """Common random draws isolate n/W effects; profits never scale with W.

    Every instance is nontrivial (not all items fit). One item attains W so the
    generation bound and actual maximum coincide. All inputs remain on disk.
    """
    for suite, values in (("n", SIZES), ("w", WEIGHTS)):
        for seed in SEEDS:
            rng = random.Random(seed)
            raw = [(rng.randint(1, 1000), rng.random()) for _ in range(max(SIZES))]
            for x in values:
                n, W = (x, 16) if suite == "n" else (500, x)
                items = [(p, 1+int(u*W)) for p, u in raw[:n]]
                items[0] = (items[0][0], W)
                capacity = sum(w for _, w in items)//2 if suite == "n" else 1000
                assert 0 < capacity < sum(w for _, w in items)
                path = root / suite / f"R{W:05d}" / f"n{n:05d}" / f"seed{seed}.kp"
                path.parent.mkdir(parents=True, exist_ok=True)
                text = f"{n}\n{capacity}\n\n" + "".join(f"{p} {w}\n" for p, w in items)
                # Refuse to silently change a previous experiment's input.
                if path.exists() and path.read_text() != text:
                    raise ValueError(f"Existing input differs: {path}")
                path.write_text(text, encoding="utf-8")


def run_benchmarks(root, repeat):
    script_dir = Path(__file__).resolve().parent
    executable = script_dir / "testbench.exe"
    if not executable.exists():
        raise ValueError(f"Compile {script_dir / 'testbench.c'} to {executable} first")
    (ROOT / "data").mkdir(parents=True, exist_ok=True)
    for suite in ("n", "w"):
        subprocess.run([str(executable), "--algorithm", "all", "--root", str(root/suite),
                        "--repeat", str(repeat), "--timeout", "15", "--memory-mb", "1024",
                        "--output", str(ROOT/"data"/f"bonus_advantage_{suite}.csv")],
                       cwd=ROOT, check=True)


def read_suite(suite):
    """First take median repeats per instance; retain pairing for speedups."""
    path = ROOT / "data" / f"bonus_advantage_{suite}.csv"
    with path.open(encoding="utf-8-sig", newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows or any(r["status"] != "ok" or r["agreement"] != "match" for r in rows):
        raise ValueError(f"{path}: expected all runs successful and agreeing; inspect raw CSV")
    groups = defaultdict(list)
    for r in rows:
        x = int(r["n"] if suite == "n" else r["max_weight"])
        groups[(x, r["dataset"], r["algorithm"])].append(float(r["time_seconds"])*1000)
    measurements = {key: median(times) for key, times in groups.items()}
    # Require all four solvers for every instance before aggregating comparisons.
    for x, dataset, _ in measurements:
        if any((x, dataset, a) not in measurements for a in ALGORITHMS):
            raise ValueError(f"Incomplete paired measurement: {dataset}")
    return measurements, len(rows)


def summarize(suite, measurements):
    groups = defaultdict(list)
    for (x, _, algorithm), time in measurements.items():
        groups[(x, algorithm)].append(time)
    return [dict(suite=suite, x=x, algorithm=a, median_ms=median(v),
                 q25_ms=float(np.percentile(v, 25)), q75_ms=float(np.percentile(v, 75)),
                 instances=len(v)) for (x, a), v in sorted(groups.items())]


def draw_runtime(ax, summary, xlabel, title):
    for a in ALGORITHMS:
        pts = sorted((r for r in summary if r["algorithm"] == a), key=lambda r: r["x"])
        xs = [r["x"] for r in pts]
        ax.plot(xs, [r["median_ms"] for r in pts], "o-", label=a, color=COLORS[a], lw=2, ms=4)
        ax.fill_between(xs, [r["q25_ms"] for r in pts], [r["q75_ms"] for r in pts],
                        color=COLORS[a], alpha=0.14)
    ax.set(xscale="log", yscale="log", xlabel=xlabel, ylabel="Solver time (ms, log scale)", title=title)
    ax.grid(True, which="both", alpha=0.2)
    ax.legend(fontsize=9)


def plot_and_report():
    data_n, count_n = read_suite("n")
    data_w, count_w = read_suite("w")
    summary_n, summary_w = summarize("n", data_n), summarize("w", data_w)
    output = ROOT / "plots"
    output.mkdir(exist_ok=True)
    summaries = summary_n+summary_w
    with (output/"bonus_advantage_summary.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(summaries[0]))
        writer.writeheader(); writer.writerows(summaries)

    # Ratios are paired on the SAME input, not ratios of unpaired averages.
    ratios = defaultdict(list)
    for (x, dataset, a), time in data_n.items():
        if a != "w3":
            ratios[(x, a)].append(time/data_n[(x, dataset, "w3")])
    speedups = [dict(n=x, baseline=a, median_speedup=median(v),
                     q25=float(np.percentile(v,25)), q75=float(np.percentile(v,75)))
                for (x,a), v in sorted(ratios.items())]
    with (output/"bonus_advantage_speedups.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(speedups[0]))
        writer.writeheader(); writer.writerows(speedups)

    fig, axes = plt.subplots(1, 3, figsize=(17, 5.3), layout="constrained")
    draw_runtime(axes[0], summary_n, "Item count n (log scale)", "(a) Small weights: W = 16\nc = floor(sum(weights) / 2)")
    for a in ALGORITHMS[:-1]:
        pts = [r for r in speedups if r["baseline"] == a]
        xs = [r["n"] for r in pts]
        axes[1].plot(xs, [r["median_speedup"] for r in pts], "o-", color=COLORS[a], label=f"{a} / w3", ms=4)
        axes[1].fill_between(xs, [r["q25"] for r in pts], [r["q75"] for r in pts], color=COLORS[a], alpha=.14)
    axes[1].axhline(1, color="#333333", linestyle="--", lw=1)
    axes[1].set(xscale="log", yscale="log", xlabel="Item count n (log scale)",
                ylabel="Paired speedup (baseline time / w3 time)",
                title="(b) Bonus advantage at W = 16\nAbove 1: w3 is faster")
    axes[1].legend(fontsize=9); axes[1].grid(True, which="both", alpha=.2)
    draw_runtime(axes[2], summary_w, "Actual maximum weight W (log scale)", "(c) Weight sensitivity\nFixed n = 500 and c = 1,000")
    fig.suptitle("Measured regimes of the O(n + W^3) knapsack solver", fontsize=15)
    fig.supxlabel("3 fixed seeds; median of repeated runs per instance, then median across instances; shading: IQR. All four answers agree.", fontsize=9)
    for extension in ("png", "pdf"):
        fig.savefig(output/f"bonus_advantage.{extension}", dpi=200)
    plt.close(fig)

    # Emit a compact, measured report; timings are never theoretical estimates.
    largest = max(r["x"] for r in summary_n)
    last = {r["algorithm"]: r["median_ms"] for r in summary_n if r["x"] == largest}
    lines = ["# Bonus Small-Weight Advantage Experiment", "", "## Method", "",
             "Uses the four unmodified C11 solvers under the isolated Windows Job Object benchmark (`gcc -std=c11 -O2`).",
             "Timings cover only the in-memory solver call (including internal preprocessing, allocation, and cleanup).",
             "Profits are drawn i.i.d. uniformly from 1..1000 independent of W; weights are uniform in 1..W with item 0 set to W.",
             "Three fixed seeds (20261121, 20261122, 20261123) share common random draws across scales.",
             "Repeats are aggregated by per-instance median, then median across seeds (shaded bands: IQR).",
             "Speedup ratios are paired on identical instances before taking medians.", "",
             "- n-suite: W=16, n=50/100/200/400/800/1600/3200, c=floor(sum(weights)/2).",
             "- W-suite: n=500, c=1000, W=4/8/16/32/64/128/256.",
             "- Per-run timeout: 15 s; Job memory cap: 1 GiB; all instances are nontrivial.",
             f"- Repeats per (instance, solver): {count_n//(len(SIZES)*len(SEEDS)*len(ALGORITHMS))}.",
             f"- Total recorded runs: {count_n+count_w}, all with status=ok and agreement=match.", "",
             "## Small-Weight Results", "", f"At W=16, n={largest}:", "",
             "| Algorithm | Median Time (ms) | Paired Speedup vs. w3 |", "| --- | ---: | ---: |"]
    for a in ALGORITHMS:
        ratio = next((r["median_speedup"] for r in speedups if r["n"]==largest and r["baseline"]==a),1)
        lines.append(f"| {a} | {last[a]:.6f} | {ratio:.2f}x |")
    lines += ["", "Smallest tested n where w3 beats baseline on all 3 seeds:", ""]
    for a in ALGORITHMS[:-1]:
        wins = [x for x in SIZES if min(ratios[(x,a)])>1]
        lines.append(f"- {a}: {min(wins) if wins else 'not observed'} (for this benchmark grid only).")
    lines += ["", "## Weight-Sensitivity Results", "", "| W | origin (ms) | list (ms) | bound (ms) | w3 (ms) |",
              "| ---: | ---: | ---: | ---: | ---: |"]
    for W in WEIGHTS:
        times = {r["algorithm"]:r["median_ms"] for r in summary_w if r["x"]==W}
        lines.append(f"| {W} | "+" | ".join(f"{times[a]:.6f}" for a in ALGORITHMS)+" |")
    lines += ["", "## Interpretation and Regime Scope", "",
              "Each correction DP in w3 spans length ~2*W^2 and performs structured SMAWK convolutions per distinct weight.",
              "When n is small and W is large, 2*W^2 exceeds c and BFPRT/SMAWK constants dominate.",
              "When W is small and n grows, the O(W^2) correction window stays fixed while O(n*c) and O(n^2*B) baselines grow.", "",
              "## Reproduction", "", "```powershell",
              "gcc -std=c11 -O2 -Wall -Wextra src/testbench.c -o src/testbench.exe",
              "python src/experiment_bonus.py", "# Regenerate plots from recorded CSVs only:",
              "python src/experiment_bonus.py --plot-only", "```", "",
              "![Bonus Regimes](../plots/bonus_advantage.png)", "",
              "Raw CSVs: `bonus_advantage_n.csv`, `bonus_advantage_w.csv`; inputs in `bonus_advantage_inputs/`.",
              "Summaries: `../plots/bonus_advantage_summary.csv`, `../plots/bonus_advantage_speedups.csv`."]
    initial = ROOT / "data" / "bonus_advantage_n_initial_512mb.csv"
    if initial.exists():
        with initial.open(newline="") as f:
            initial_rows = list(csv.DictReader(f))
        failures = [r for r in initial_rows if r["status"] != "ok"]
        lines += ["", "## Initial 512 MiB Resource-Cap Pilot", "",
                  f"An initial 512 MiB pilot recorded {len(initial_rows)} runs with {len(failures)} worker_failed rows",
                  "(all 3 repeats of `list` on n=3200 seed20261122.kp due to retaining L_0..L_n). Rerunning all solvers",
                  "under the standard 1 GiB cap succeeded cleanly across all 504 runs."]
    old = ROOT / "data" / "all_n50_R1000.csv"
    if old.exists():
        with old.open(newline="") as f:
            old_rows = [r for r in csv.DictReader(f) if r["status"] == "ok"]
        means = {a: sum(float(r["time_seconds"]) for r in old_rows if r["algorithm"]==a)
                    /sum(r["algorithm"]==a for r in old_rows) for a in ALGORITHMS
                    if any(r["algorithm"]==a for r in old_rows)}
        lines += ["", "## KPLIB n=50, R=1000 Contrast", "",
                  "On `all_n50_R1000.csv` (n=50, actual W ~ 989, median c = 12340):", "",
                  "| Algorithm | Mean Time (ms) |", "| --- | ---: |"]
        lines += [f"| {a} | {means[a]*1000:.6f} |" for a in ALGORITHMS if a in means]
        lines += ["", "Here 2*W^2 ~ 2e6 vastly exceeds median capacity c = 12340, making unclipped w3 slower than list/bound."]
    report = ROOT / "data" / "bonus_advantage_report.md"
    report.write_text("\n".join(lines)+"\n", encoding="utf-8")
    print(f"Report: {report}\nFigure: {output/'bonus_advantage.png'}")
    print("Largest-n timings (ms):", last)
    print("Largest-n paired speedups:", [r for r in speedups if r['n']==largest])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plot-only", action="store_true")
    parser.add_argument("--repeat", type=int, default=3)
    args = parser.parse_args()
    if args.repeat <= 0:
        parser.error("--repeat must be positive")
    if not args.plot_only:
        root = ROOT / "data" / "bonus_advantage_inputs"
        generate(root)
        run_benchmarks(root, args.repeat)
    plot_and_report()


if __name__ == "__main__":
    main()
