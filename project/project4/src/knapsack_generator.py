"""生成单约束 0/1 背包实例，仅使用 Python 标准库。

格式：物品数量 n，容量 c，空行，然后 n 行“价值 重量”。
示例：
  python knapsack_generator.py --n 50 --output data
  python knapsack_generator.py --start 50 --end 1000 --step 1 --instances 10
  python knapsack_generator.py --n 100 200 500 --kind strongly --seed 42

默认重量和价值独立均匀采样于 [1, 1000]，容量为总重量的一半向下取整。
生成的是自定义可复现实例，不是原版 KPLIB 实例；不提供参考最优值。
"""

import argparse
from decimal import Decimal, InvalidOperation, ROUND_FLOOR
from pathlib import Path
import random
import sys


def generate_instance(n, value_range, kind, rng, ratio=Decimal("0.5"), capacity=None):
    """返回 (容量, [(价值, 重量), ...])；每件物品只能选择一次。"""
    items = []
    offset = value_range // 10
    for _ in range(n):
        weight = rng.randint(1, value_range)
        if kind == "uncorrelated":
            profit = rng.randint(1, value_range)
        elif kind == "weakly":
            profit = rng.randint(max(1, weight - offset), weight + offset)
        elif kind == "strongly":
            profit = weight + offset
        elif kind == "subset-sum":
            profit = weight
        else:
            raise ValueError(f"未知数据类型：{kind}")
        items.append((profit, weight))
    if capacity is None:
        total_weight = sum(weight for _, weight in items)
        # 使用十进制计算，避免 0.3 等浮点比例在取整边界产生误差。
        capacity = int((ratio * total_weight).to_integral_value(rounding=ROUND_FLOOR))
    return capacity, items


def write_instance(path, capacity, items):
    """使用 exclusive create，避免覆盖同名文件。"""
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        stream.write(f"{len(items)}\n{capacity}\n\n")
        for profit, weight in items:
            stream.write(f"{profit} {weight}\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sizes = parser.add_mutually_exclusive_group()
    sizes.add_argument("--n", type=int, nargs="+", help="指定一个或多个物品数量，默认 50")
    sizes.add_argument("--start", type=int, help="连续规模的起始值，包含该值")
    parser.add_argument("--end", type=int, help="连续规模上限，包含该值（若步长可到达）")
    parser.add_argument("--step", type=int, default=1, help="规模步长，默认 1")
    parser.add_argument("--instances", type=int, default=1, help="每个规模生成的实例数")
    parser.add_argument("--range", dest="value_range", type=int, default=1000, help="重量上限 R；无相关价值上限也为 R")
    parser.add_argument("--kind", choices=("uncorrelated", "weakly", "strongly", "subset-sum"), default="uncorrelated")
    parser.add_argument("--seed", type=int, default=42, help="固定随机种子，默认 42")
    capacities = parser.add_mutually_exclusive_group()
    capacities.add_argument("--ratio", default=None, help="容量占总重量的比例，默认 0.5")
    capacities.add_argument("--capacity", type=int, help="所有实例使用相同的固定容量")
    parser.add_argument("--output", type=Path, default=Path("generated_knapsack"), help="输出目录")
    args = parser.parse_args()

    if args.step <= 0 or args.instances <= 0 or args.value_range <= 0:
        parser.error("step、instances、range 必须为正整数")
    if args.start is None:
        if args.end is not None:
            parser.error("--end 必须与 --start 一起使用")
        ns = sorted(set(args.n if args.n is not None else [50]))
    else:
        if args.end is None or args.end < args.start:
            parser.error("使用 --start 时需要 --end，且 end >= start")
        ns = range(args.start, args.end + 1, args.step)
    if any(n <= 0 for n in ns):
        parser.error("物品数量必须为正整数")
    if args.capacity is not None and args.capacity < 0:
        parser.error("capacity 不能为负数")
    try:
        ratio = Decimal(args.ratio if args.ratio is not None else "0.5")
        if not ratio.is_finite() or not Decimal(0) <= ratio <= Decimal(1):
            raise ValueError()
    except (InvalidOperation, ValueError):
        parser.error("ratio 必须是 [0, 1] 内的有限数值")

    # 参数进入目录名，不同种子、分布和容量设置可在同一输出目录并存。
    capacity_tag = f"c{args.capacity}" if args.capacity is not None else f"ratio{ratio.normalize()}"
    root = args.output / args.kind / f"R{args.value_range:05d}_{capacity_tag}_seed{args.seed}"

    # 仅检查将要写入的文件是否存在，不读取任何已有文件内容。
    for n in ns:
        for trial in range(args.instances):
            target = root / f"n{n:05d}" / f"s{trial:03d}.kp"
            if target.exists():
                parser.error(f"文件已存在，未开始生成；请更换输出目录或参数：{target}")

    count = 0
    try:
        for n in ns:
            directory = root / f"n{n:05d}"
            directory.mkdir(parents=True, exist_ok=True)
            for trial in range(args.instances):
                # 每个实例独立播种：更改生成顺序或增加其他规模不会影响它。
                instance_seed = f"{args.seed}:{args.kind}:{args.value_range}:{n}:{trial}"
                rng = random.Random(instance_seed)
                capacity, items = generate_instance(n, args.value_range, args.kind, rng, ratio, args.capacity)
                write_instance(directory / f"s{trial:03d}.kp", capacity, items)
                count += 1
    except OSError as error:
        parser.exit(1, f"写入失败（此前可能已生成 {count} 个文件）：{error}\n")
    print(f"已生成 {count} 个 .kp 文件：{root.resolve()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
