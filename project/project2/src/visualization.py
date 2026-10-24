# Plot Dijkstra benchmark CSV files using the plotting style from Project 1.
import argparse
import csv
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def main():
    parser = argparse.ArgumentParser(
        description="Plot Dijkstra runtime against the number of query pairs."
    )
    parser.add_argument("csv", nargs="?", default="dijkstra_runtime.csv")
    parser.add_argument("--output-dir", default="visualizations")
    args = parser.parse_args()

    required = {"dataset", "query_pairs", "heap", "avg_seconds"}
    with open(args.csv, newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream)
        missing = required.difference(reader.fieldnames or [])
        data = list(reader)
    if missing:
        raise SystemExit(f"CSV is missing required columns: {', '.join(sorted(missing))}")
    if not data:
        raise SystemExit("CSV contains no measurements.")

    plt.rcParams["font.sans-serif"] = ["Microsoft YaHei", "DejaVu Sans"]
    plt.rcParams["axes.unicode_minus"] = False
    os.makedirs(args.output_dir, exist_ok=True)

    for dataset in dict.fromkeys(row["dataset"] for row in data):
        subset = [row for row in data if row["dataset"] == dataset]
        plt.figure(figsize=(9, 6))
        for heap in dict.fromkeys(row["heap"] for row in subset):
            curve = sorted((row for row in subset if row["heap"] == heap),
                           key=lambda row: int(row["query_pairs"]))
            plt.plot(
                [int(row["query_pairs"]) for row in curve],
                [float(row["avg_seconds"]) for row in curve],
                marker="o",
                label=heap,
            )

        plt.xlabel("Number of source-target query pairs")
        plt.ylabel("Average Dijkstra runtime (seconds)")
        plt.title(f"Dijkstra Runtime - {dataset}")
        plt.legend(title="Priority queue")
        plt.grid(True, linestyle="--", alpha=0.5)
        plt.tight_layout()
        safe_name = "".join(c if c.isalnum() or c in "_-" else "_" for c in dataset)
        filename = f"dijkstra_runtime_{safe_name}.png"
        plt.savefig(os.path.join(args.output_dir, filename), dpi=300)
        plt.savefig(os.path.join(args.output_dir, f"dijkstra_runtime_{safe_name}.pdf"))
        plt.close()
        print(f"Saved {os.path.join(args.output_dir, filename)}")


if __name__ == "__main__":
    main()
