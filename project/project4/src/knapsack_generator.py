"""Generate reproducible single-constraint 0/1 knapsack instances (.kp) using
only the Python standard library.

Format: item count n, capacity c, blank line, followed by n lines of "profit weight".
Examples:
  python knapsack_generator.py --n 50 --output data
  python knapsack_generator.py --start 50 --end 1000 --step 1 --instances 10
  python knapsack_generator.py --n 100 200 500 --kind strongly --seed 42

By default, weights and profits are sampled independently and uniformly from
[1, 1000], and capacity defaults to floor(0.5 * sum(weights)).
"""

import argparse
from decimal import Decimal, InvalidOperation, ROUND_FLOOR
from pathlib import Path
import random
import sys


def generate_instance(n, value_range, kind, rng, ratio=Decimal("0.5"), capacity=None):
    """Return (capacity, [(profit, weight), ...]) for one 0/1 knapsack instance."""
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
            raise ValueError(f"Unknown instance distribution: {kind}")
        items.append((profit, weight))
    if capacity is None:
        total_weight = sum(weight for _, weight in items)
        # Use exact Decimal arithmetic to avoid floating-point boundary rounding drift.
        capacity = int((ratio * total_weight).to_integral_value(rounding=ROUND_FLOOR))
    return capacity, items


def write_instance(path, capacity, items):
    """Create file exclusively ('x') so existing benchmark files are never overwritten."""
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        stream.write(f"{len(items)}\n{capacity}\n\n")
        for profit, weight in items:
            stream.write(f"{profit} {weight}\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sizes = parser.add_mutually_exclusive_group()
    sizes.add_argument("--n", type=int, nargs="+", help="one or more item counts (default: 50)")
    sizes.add_argument("--start", type=int, help="inclusive start of contiguous item-count sweep")
    parser.add_argument("--end", type=int, help="inclusive end of contiguous item-count sweep")
    parser.add_argument("--step", type=int, default=1, help="step size for item-count sweep (default: 1)")
    parser.add_argument("--instances", type=int, default=1, help="instances generated per item count")
    parser.add_argument("--range", dest="value_range", type=int, default=1000, help="upper bound R for weights/profits")
    parser.add_argument("--kind", choices=("uncorrelated", "weakly", "strongly", "subset-sum"), default="uncorrelated")
    parser.add_argument("--seed", type=int, default=42, help="deterministic master seed (default: 42)")
    capacities = parser.add_mutually_exclusive_group()
    capacities.add_argument("--ratio", default=None, help="capacity ratio of total weight (default: 0.5)")
    capacities.add_argument("--capacity", type=int, help="fixed knapsack capacity shared across all instances")
    parser.add_argument("--output", type=Path, default=Path("generated_knapsack"), help="output root directory")
    args = parser.parse_args()

    if args.step <= 0 or args.instances <= 0 or args.value_range <= 0:
        parser.error("step, instances, and range must be positive integers")
    if args.start is None:
        if args.end is not None:
            parser.error("--end requires --start")
        ns = sorted(set(args.n if args.n is not None else [50]))
    else:
        if args.end is None or args.end < args.start:
            parser.error("--start requires --end with end >= start")
        ns = range(args.start, args.end + 1, args.step)
    if any(n <= 0 for n in ns):
        parser.error("item counts must be positive integers")
    if args.capacity is not None and args.capacity < 0:
        parser.error("capacity must be non-negative")
    try:
        ratio = Decimal(args.ratio if args.ratio is not None else "0.5")
        if not ratio.is_finite() or not Decimal(0) <= ratio <= Decimal(1):
            raise ValueError()
    except (InvalidOperation, ValueError):
        parser.error("ratio must be a finite number in [0, 1]")

    # Encode generator parameters in directory names so multiple suites can coexist.
    capacity_tag = f"c{args.capacity}" if args.capacity is not None else f"ratio{ratio.normalize()}"
    root = args.output / args.kind / f"R{args.value_range:05d}_{capacity_tag}_seed{args.seed}"

    # Pre-check target paths before writing any files.
    for n in ns:
        for trial in range(args.instances):
            target = root / f"n{n:05d}" / f"s{trial:03d}.kp"
            if target.exists():
                parser.error(f"Target file already exists; aborting without overwriting: {target}")

    count = 0
    try:
        for n in ns:
            directory = root / f"n{n:05d}"
            directory.mkdir(parents=True, exist_ok=True)
            for trial in range(args.instances):
                # Seed each (n, trial) independently so changing sweep bounds preserves instances.
                instance_seed = f"{args.seed}:{args.kind}:{args.value_range}:{n}:{trial}"
                rng = random.Random(instance_seed)
                capacity, items = generate_instance(n, args.value_range, args.kind, rng, ratio, args.capacity)
                write_instance(directory / f"s{trial:03d}.kp", capacity, items)
                count += 1
    except OSError as error:
        parser.exit(1, f"Write failed (after generating {count} files): {error}\n")
    print(f"Generated {count} .kp files under: {root.resolve()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
