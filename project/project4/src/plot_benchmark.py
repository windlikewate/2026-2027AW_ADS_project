"""Plot testbench CSV results against item count n or knapsack capacity c.

Uses the standard csv module and matplotlib. Repeated trials are first
averaged per dataset/algorithm; datasets with the same x then receive equal
weight. Failed or timed-out trials are never treated as completed timings.
"""

import argparse
import csv
import math
from collections import Counter, defaultdict
from pathlib import Path
from statistics import mean, median

import matplotlib

matplotlib.use("Agg")  # Work without a GUI, including batch/remote sessions.
import matplotlib.pyplot as plt


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_files", nargs="+", type=Path, help="one or more testbench CSV files")
    parser.add_argument("--mode", choices=("n", "c"), required=True, help="x axis: n or capacity")
    parser.add_argument("--output", type=Path, help="PNG/PDF/SVG path; default plots/time_vs_MODE.png")
    parser.add_argument("--algorithms", nargs="+", help="include only these algorithms, e.g. origin list bound")
    parser.add_argument("--aggregate", choices=("mean", "median"), default="mean", help="aggregate datasets at equal x")
    parser.add_argument("--n", type=int, help="include only this item count (useful in c mode)")
    parser.add_argument("--capacity", type=int, help="include only this capacity")
    parser.add_argument("--category", help="kplib category, e.g. 00Uncorrelated")
    parser.add_argument("--r", help="kplib range directory, e.g. R01000")
    parser.add_argument("--exclude-mismatch", action="store_true", help="exclude rows with agreement=mismatch")
    parser.add_argument("--log-x", action="store_true")
    parser.add_argument("--log-y", action="store_true")
    parser.add_argument("--title", help="custom chart title")
    return parser, parser.parse_args()


def read_rows(args):
    # Key identifies a dataset independently of the selected x axis.
    timings = defaultdict(list)
    skipped = Counter()
    seen = set()
    required = {"dataset", "algorithm", "n", "capacity", "time_seconds", "status"}
    for path in args.csv_files:
        with path.open("r", encoding="utf-8-sig", newline="") as stream:
            reader = csv.DictReader(stream)
            missing = required - set(reader.fieldnames or [])
            if missing:
                raise ValueError(f"{path}: missing columns: {', '.join(sorted(missing))}")
            for line, row in enumerate(reader, start=2):
                if row["status"] != "ok":
                    skipped[row["status"] or "empty_status"] += 1
                    continue
                if args.algorithms and row["algorithm"] not in args.algorithms:
                    skipped["filter"] += 1
                    continue
                # Normalize separators so Windows CSVs can also be plotted on Linux.
                dataset = row["dataset"].replace("\\", "/")
                components = dataset.split("/")
                if ((args.category and args.category not in components)
                        or (args.r and args.r not in components)):
                    skipped["filter"] += 1
                    continue
                if args.exclude_mismatch and row.get("agreement") == "mismatch":
                    skipped["mismatch"] += 1
                    continue
                try:
                    n, capacity = int(row["n"]), int(row["capacity"])
                    seconds = float(row["time_seconds"])
                except (TypeError, ValueError) as exc:
                    raise ValueError(f"{path}:{line}: invalid n/capacity/time") from exc
                if n <= 0 or capacity < 0 or seconds < 0 or not math.isfinite(seconds):
                    raise ValueError(f"{path}:{line}: n/capacity/time outside valid range")
                if ((args.n is not None and n != args.n)
                        or (args.capacity is not None and capacity != args.capacity)):
                    skipped["filter"] += 1
                    continue
                if (args.log_x and (n if args.mode == "n" else capacity) <= 0) or (args.log_y and seconds <= 0):
                    skipped["nonpositive_log_value"] += 1
                    continue
                # Avoid weighting identical rows twice when CSV inputs overlap.
                identity = (dataset, row["algorithm"], n, capacity,
                            row.get("trial", ""), seconds)
                if identity in seen:
                    skipped["duplicate"] += 1
                    continue
                seen.add(identity)
                timings[(dataset, row["algorithm"], n, capacity)].append(seconds)
    return timings, skipped


def summarize(timings, mode, method):
    groups = defaultdict(list)
    trial_counts = Counter()
    for (_, algorithm, n, capacity), trials in timings.items():
        key = (algorithm, n if mode == "n" else capacity)
        groups[key].append(mean(trials))
        trial_counts[key] += len(trials)
    aggregate = mean if method == "mean" else median
    return [
        {"algorithm": algorithm, "x": x, "time_seconds": aggregate(values),
         "dataset_count": len(values), "trial_count": trial_counts[(algorithm, x)]}
        for (algorithm, x), values in sorted(groups.items())
    ]


def draw(summary, args, pair_count):
    output = args.output or Path("plots") / f"time_vs_{args.mode}.png"
    if output.suffix.lower() not in {".png", ".pdf", ".svg"}:
        raise ValueError("--output must end with .png, .pdf or .svg")
    output.parent.mkdir(parents=True, exist_ok=True)
    fig, ax = plt.subplots(figsize=(9, 5.5), layout="constrained")
    names = [name for name in ("origin", "list", "bound")
             if any(row["algorithm"] == name for row in summary)]
    names += sorted({row["algorithm"] for row in summary} - set(names))
    colors = ["#2563eb", "#059669", "#e76f00", "#9333ea"]
    markers = ["o", "s", "^"]
    for index, algorithm in enumerate(names):
        points = sorted((r for r in summary if r["algorithm"] == algorithm), key=lambda r: r["x"])
        ax.plot([r["x"] for r in points], [r["time_seconds"] for r in points],
                label=algorithm, color=colors[index % len(colors)],
                marker=markers[index % len(markers)], markersize=4, linewidth=1.5)
    ax.set_xlabel("Number of items n" if args.mode == "n" else "Knapsack capacity c")
    ax.set_ylabel("Runtime (seconds)")
    title = args.title or f"Knapsack runtime vs {'item count n' if args.mode == 'n' else 'capacity c'}"
    ax.set_title(title)
    if args.log_x:
        ax.set_xscale("log")
    if args.log_y:
        ax.set_yscale("log")
    else:
        ax.set_ylim(bottom=0)
        ax.ticklabel_format(axis="y", style="sci", scilimits=(-3, 3))
    ax.grid(True, alpha=0.25)
    ax.legend(title=f"Algorithm ({args.aggregate})")
    subtitle = f"Successful trials only; {pair_count} dataset/algorithm pairs"
    ax.text(0, -0.17, subtitle, transform=ax.transAxes, fontsize=9, color="#555555")
    fig.savefig(output, dpi=200)
    plt.close(fig)
    # Export exactly the aggregated values used by the figure.
    summary_path = output.with_name(output.stem + "_summary.csv")
    with summary_path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=["algorithm", "x", "time_seconds", "dataset_count", "trial_count"])
        writer.writeheader()
        writer.writerows(summary)
    return output, summary_path


def main():
    parser, args = arguments()
    try:
        # Protect source CSVs from being overwritten by either plot artifact.
        output = args.output or Path("plots") / f"time_vs_{args.mode}.png"
        summary_path = output.with_name(output.stem + "_summary.csv")
        sources = {p.resolve() for p in args.csv_files}
        if output.resolve() in sources or summary_path.resolve() in sources:
            raise ValueError("output must not overwrite an input CSV")
        timings, skipped = read_rows(args)
        if skipped:
            print("Excluded rows: " + ", ".join(f"{key}={value}" for key, value in sorted(skipped.items())))
        if not timings:
            raise ValueError("no successful timing rows remain after filtering")
        summary = summarize(timings, args.mode, args.aggregate)
        if len({row["x"] for row in summary}) == 1:
            print("Note: only one distinct x value is available; more scales are needed for a trend.")
        if any(row["x"] == 0 for row in summary):
            print("Note: zero capacity is shown on the linear x axis.")
        figure, table = draw(summary, args, len(timings))
        print(f"Figure: {figure}\nAggregated data: {table}")
    except (OSError, ValueError, csv.Error) as exc:
        parser.exit(1, f"Error: {exc}\n")


if __name__ == "__main__":
    main()
