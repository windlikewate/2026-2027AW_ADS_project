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
    executable = ROOT / "src" / "testbench.exe"
    if not executable.exists():
        raise ValueError("Compile src/testbench.c to src/testbench.exe first")
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
    lines = ["# Bonus 小重量优势实验", "", "## 方法", "",
             "使用未修改的四个算法和现有 Windows 隔离 benchmark；编译采用 `gcc -std=c11 -O2`。",
             "计时只包含求解函数（含其内部预处理、分配与释放），不包含读取和进程启动。",
             "价值独立均匀取自 1..1000，不随 W 改变；重量为 1..W 的均匀样本，首件强制达到 W。",
             "三个固定种子为 20261121、20261122、20261123。同一种子使用共同随机数；n 实验取同一序列前缀。",
             "先对同实例的重复计时取中位数，再对三个实例取中位数；阴影表示实例中位数的四分位范围。",
             "加速比先在同一实例配对计算，再取中位数。", "",
             "- n 实验：W=16，n=50/100/200/400/800/1600/3200，c=floor(总重量/2)。",
             "- W 实验：n=500、c=1000，W=4/8/16/32/64/128/256。",
             "- 单次超时 15 秒、内存限制 1 GiB，全部实例都不能将所有物品装入。",
             f"- 每实例每算法重复 {count_n//(len(SIZES)*len(SEEDS)*len(ALGORITHMS))} 次。",
             f"- 原始计时记录 {count_n+count_w} 条，全部 status=ok、agreement=match。", "",
             "## 小重量结果", "", f"W=16、n={largest} 时：", "",
             "| 算法 | 中位耗时（ms） | 相对 w3 的配对加速比 |", "| --- | ---: | ---: |"]
    for a in ALGORITHMS:
        ratio = next((r["median_speedup"] for r in speedups if r["n"]==largest and r["baseline"]==a),1)
        lines.append(f"| {a} | {last[a]:.6f} | {ratio:.2f}× |")
    lines += ["", "w3 在所有三个实例中均快于该基线的最小受测 n：", ""]
    for a in ALGORITHMS[:-1]:
        wins = [x for x in SIZES if min(ratios[(x,a)])>1]
        lines.append(f"- {a}: {min(wins) if wins else '本次未观测到'}（仅针对本次受测规模，不代表普遍阈值）。")
    lines += ["", "## 重量敏感性结果", "", "| W | origin（ms） | list（ms） | bound（ms） | w3（ms） |",
              "| ---: | ---: | ---: | ---: | ---: |"]
    for W in WEIGHTS:
        times = {r["algorithm"]:r["median_ms"] for r in summary_w if r["x"]==W}
        lines.append(f"| {W} | "+" | ".join(f"{times[a]:.6f}" for a in ALGORITHMS)+" |")
    lines += ["", "## 解释和适用范围", "",
              "w3 每侧修正数组长约 2W²，并对每种实际出现的重量执行一次结构化卷积。",
              "小 n、大 W 时，这一状态范围远大于实际容量，BFPRT、SMAWK 递归及其内存分配常数也较高。",
              "固定小 W、增大 n 时，修正 DP 的大小不随 n 增长，前处理仍为线性；当容量随 n 增长时，",
              "普通 DP 的 n*c 开销持续增加。list 的状态规模与数据分布有关；当前 bound 还包含选择排序和逐状态扫描。",
              "因此本实验展示的是当前四种实现的小重量优势区间，不能说明所有数据上 w3 都更快，",
              "也不能把 bound 的全部差距归因于邻近性技术：其实现本身仍有额外开销。",
              "W 对照中 n 和 c 保持不变，但重量分布仍会改变可达状态和剪枝效果，因此曲线不是单独的复杂度证明。",
              "本次未优化算法实现、未排除正常的慢样本，也未把失败或超时当作完成计时。", "",
              "## 复现", "", "```powershell",
              "gcc -std=c11 -O2 -Wall -Wextra src/testbench.c -o src/testbench.exe",
              "python src/experiment_bonus.py", "# 只用已保存的 CSV 重画：",
              "python src/experiment_bonus.py --plot-only", "```", "",
              "![实验结果](../plots/bonus_advantage.png)", "",
              "原始数据：`bonus_advantage_n.csv`、`bonus_advantage_w.csv`；输入保存在 `bonus_advantage_inputs/`。",
              "汇总与配对加速比：`../plots/bonus_advantage_summary.csv`、`../plots/bonus_advantage_speedups.csv`。"]
    initial = ROOT / "data" / "bonus_advantage_n_initial_512mb.csv"
    if initial.exists():
        with initial.open(newline="") as f:
            initial_rows = list(csv.DictReader(f))
        failures = [r for r in initial_rows if r["status"] != "ok"]
        lines += ["", "## 初次资源限制试验", "",
                  f"初次使用 512 MiB 限制，共 {len(initial_rows)} 条记录，其中 {len(failures)} 条失败。",
                  "失败均为 n=3200 的同一实例的 list 工作进程（3 次），与内存不足相符。",
                  "原始结果保留为 `bonus_advantage_n_initial_512mb.csv`。随后对全部算法统一使用 1 GiB 完整重跑，",
                  "正文与图表只使用这次统一配置的结果；没有将初次失败当作计时或加速比。"]
    old = ROOT / "data" / "all_n50_R1000.csv"
    if old.exists():
        with old.open(newline="") as f:
            old_rows = [r for r in csv.DictReader(f) if r["status"] == "ok"]
        means = {a: sum(float(r["time_seconds"]) for r in old_rows if r["algorithm"]==a)
                    /sum(r["algorithm"]==a for r in old_rows) for a in ALGORITHMS
                    if any(r["algorithm"]==a for r in old_rows)}
        lines += ["", "## 已有 KPLIB 结果的解释", "",
                  "用户原有 `all_n50_R1000.csv` 中，n=50，实际 W 约 1000。", "",
                  "| 算法 | 平均耗时（ms） |", "| --- | ---: |"]
        lines += [f"| {a} | {means[a]*1000:.6f} |" for a in ALGORITHMS if a in means]
        lines += ["", "此时每侧修正数组约 200 万项，而原容量中位数仅 12340；当前实现没有",
                  "按前缀可删除总重量进一步收紧这个保守窗口。对实际出现的每种重量都扫描该窗口，",
                  "再叠加 SMAWK 与分配开销，因此显著慢于原有 DP。O(n+W³) 是参数化的渐近优势，",
                  "不能单凭公式认为它在 n 小、W 大时更优。"]
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
