"""Plot existing benchmark results; run: python src/plot_complexity.py.

Dependencies: numpy, matplotlib. Input/output defaults are relative to this file,
so the script works from either the project root or src directory.
K counts coupon purchases (heap loop iterations), excluding bulk purchases.
"""

import argparse
import bisect
import csv
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np


ROOT = Path(__file__).resolve().parent.parent
PROFILES = (
    "random", "duplicates", "near_limit", "unaffordable",
    "many_purchases", "ordered",
)


def coupon_purchases(row, csv_path):
    """All coupon pairs cheaper than base are bought before bulk purchases.

    Therefore K = min(total items bought, number of such pairs). Strict '<'
    matches solveShopping's getMin(heap) >= minOriginalPrice branch.
    """
    if row.get("k", "") != "":
        k = int(row["k"])
        if not 0 <= k <= int(row["items"]):
            raise ValueError(f"Invalid K: {row['file']}")
        return k
    # Backward compatibility: old CSVs did not record K in the solver.
    name = Path(row["file"])
    candidates = [
        csv_path.parent / name, ROOT / "src" / name,
        ROOT / name, ROOT / "data" / name.name,
    ]
    path = next((p for p in candidates if p.is_file()), None)
    if path is None:
        raise FileNotFoundError(f"Cannot find input for {row['file']}")
    values = list(map(int, path.read_text().split()))
    n, d = values[:2]
    if n != int(row["n"]) or d != int(row["d"]) or len(values) != 2 + 2 * n:
        raise ValueError(f"Input disagrees with CSV: {path}")
    prices, coupons = values[2:2 + n], sorted(values[2 + n:])
    base = min(prices)
    cheap_pairs = sum(n - bisect.bisect_right(coupons, p - base) for p in prices)
    return min(int(row["items"]), cheap_pairs)


def nonnegative_fit(x, y):
    """Small nonnegative least squares fit, enumerating active columns.

    T = a*N*log2(N) + b*K*log2(N) + c, with a,b,c >= 0.
    Scale columns to avoid numerical conditioning problems.
    """
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


def save(fig, output, name):
    fig.savefig(output / f"{name}.png", dpi=220, bbox_inches="tight")
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv", type=Path, default=ROOT / "output/results.csv")
    parser.add_argument("--output", type=Path, default=ROOT / "output")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    with args.csv.open(newline="", encoding="utf-8-sig") as stream:
        raw = list(csv.DictReader(stream))
    rows = []
    for row in raw:
        if row["profile"] not in PROFILES:
            continue  # Two small known-answer cases are correctness checks.
        if row["status"] != "PASS":
            raise ValueError(f"Benchmark failed: {row['file']}")
        n = int(row["n"])
        k = coupon_purchases(row, args.csv.resolve())
        t = float(row["avg_ms"])
        lo, hi = float(row["min_ms"]), float(row["max_ms"])
        if n <= 1 or not 0 < lo <= t <= hi:
            raise ValueError(f"Invalid size/timing: {row['file']}")
        rows.append(dict(profile=row["profile"], seed=row["seed"], n=n,
                         items=int(row["items"]), k=k, avg_ms=t,
                         min_ms=lo, max_ms=hi,
                         nlogn=n * np.log2(n), klogn=k * np.log2(n)))
    if not rows:
        raise ValueError("No benchmark profiles found")

    plt.rcParams.update({"font.size": 10, "axes.spines.top": False,
                         "axes.spines.right": False, "axes.grid": True,
                         "grid.alpha": 0.22})
    colors = dict(zip(PROFILES, plt.get_cmap("tab10").colors))
    fig, ax = plt.subplots(figsize=(9, 5.5), layout="constrained")
    for profile in PROFILES:
        group = [r for r in rows if r["profile"] == profile]
        ns = sorted({r["n"] for r in group})
        if not ns:
            continue
        means = [np.mean([r["avg_ms"] for r in group if r["n"] == n]) for n in ns]
        lower = [min(r["min_ms"] for r in group if r["n"] == n) for n in ns]
        upper = [max(r["max_ms"] for r in group if r["n"] == n) for n in ns]
        ax.errorbar(ns, means, yerr=[np.array(means) - lower,
                                   np.array(upper) - means],
                    marker="o", capsize=3, label=profile, color=colors[profile])
    ax.set(xscale="log", yscale="log", xlabel="N (items / coupon types)",
           ylabel="Solver time (ms)", title="Measured runtime by input profile")
    ax.legend(ncol=2)
    fig.supxlabel("Points: mean across seeds; bars: min/max across seeds and timing repetitions", fontsize=9)
    save(fig, args.output, "runtime_by_n")

    fig, axes = plt.subplots(2, 3, figsize=(15, 9), layout="constrained")
    fit_report = []
    for ax, profile in zip(axes.flat, PROFILES):
        group = sorted((r for r in rows if r["profile"] == profile),
                       key=lambda r: (r["n"], r["seed"]))
        if not group:
            ax.set_visible(False)
            continue
        x = np.array([[r["nlogn"], r["klogn"], 1] for r in group])
        y = np.array([r["avg_ms"] for r in group])
        coef = nonnegative_fit(x, y)
        pred = x @ coef
        sst = np.sum((y - y.mean()) ** 2)
        r2 = 1 - np.sum((y - pred) ** 2) / sst if sst > 0 else float("nan")
        for r, fitted in zip(group, pred):
            r["fitted_ms"] = fitted
            r["ns_per_work_unit"] = 1e6 * r["avg_ms"] / (r["nlogn"] + r["klogn"])
        ns = sorted({r["n"] for r in group})
        mean_actual = [np.mean([r["avg_ms"] for r in group if r["n"] == n]) for n in ns]
        mean_fit = [np.mean([r["fitted_ms"] for r in group if r["n"] == n]) for n in ns]
        ax.scatter([r["n"] for r in group], y, color=colors[profile], alpha=.55, s=22)
        ax.plot(ns, mean_actual, "o-", color=colors[profile], label="Measured mean")
        ax.plot(ns, mean_fit, "s--", color="black", label="Fitted mean")
        ax.set(xscale="log", yscale="log", xlabel="N", ylabel="Time (ms)",
               title=f"{profile} (R² = {r2:.3f})")
        ax.legend(fontsize=8)
        fit_report.append(dict(profile=profile, a_ms=coef[0], b_ms=coef[1],
                               c_ms=coef[2], r_squared=r2))
    fig.suptitle(r"Measured vs. fitted $T=aN\log_2N+bK\log_2N+c$ ($a,b,c\geq0$)")
    fig.supxlabel("Separate fit per profile; fitted on these same data, not an independent validation", fontsize=10)
    save(fig, args.output, "complexity_fit")

    fig, axes = plt.subplots(1, 2, figsize=(13, 5), layout="constrained")
    for profile in PROFILES:
        group = [r for r in rows if r["profile"] == profile]
        ns = sorted({r["n"] for r in group})
        if not ns:
            continue
        ratio = [np.mean([r["ns_per_work_unit"] for r in group if r["n"] == n]) for n in ns]
        ks = [np.mean([r["k"] for r in group if r["n"] == n]) for n in ns]
        axes[0].plot(ns, ratio, "o-", label=profile, color=colors[profile])
        axes[1].plot(ns, ks, "o-", label=profile, color=colors[profile])
    axes[0].set(xscale="log", yscale="log", xlabel="N",
                ylabel=r"$T / [(N+K)\log_2N]$ (ns / work unit)",
                title="Normalized runtime (no fitted scaling)")
    axes[1].set(xscale="log", yscale="symlog", xlabel="N",
                ylabel="K (coupon purchases)", title="Actual heap-loop purchase count")
    axes[1].set_ylim(bottom=0)
    axes[0].legend(fontsize=8, ncol=2)
    fig.supxlabel("K excludes original-price purchases performed by one division; zero K is shown on the right.", fontsize=10)
    save(fig, args.output, "complexity_normalized")

    for name, records in (("complexity_data.csv", rows), ("complexity_fit.csv", fit_report)):
        with (args.output / name).open("w", newline="", encoding="utf-8") as stream:
            writer = csv.DictWriter(stream, fieldnames=list(records[0]))
            writer.writeheader()
            writer.writerows(records)
    print(f"Read {len(rows)} benchmark cases from {args.csv}")
    for report in fit_report:
        print(f"{report['profile']}: R^2={report['r_squared']:.4f}")
    print(f"Saved 3 PNG plots and 2 derived CSV files to {args.output}")


if __name__ == "__main__":
    main()
