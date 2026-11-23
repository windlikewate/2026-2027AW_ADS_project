
# Shopping With Coupons — Batch Experiments

## 1. Rules

The purchasing rules used in these experiments are:

- At most **one coupon can be applied to each purchased unit**.
- Each item-type/coupon-type pair can be used at most once.
- Original-price purchases are unlimited.

## 2. Project Files

| File | Description |
|---|---|
| `generate.c` | Generates 158 individual `.in` files and `data/manifest.csv` in the project root. |
| `shopping_with_cunpons.h` | Current greedy solver, including direct K counting. |
| `project_paths.h` | Locates the project root from the executable location. |
| `bench.h` | Solver interface, returning the maximum item count and remaining money. |
| `testbench.c` | Loads test cases, validates input, checks independent expected answers, warms up once, and times repeated solver calls. |
| `../output/results.csv` | Actual timing results and measured K, one row per case. |

## 3. Build and Run

Use GCC in the `src` directory. From the project root, run `cd src` first.

### Step 1: Compile the Generator

```bash
gcc -O2 -std=c11 -Wall -Wextra -pedantic generate.c -o generate.exe
```

### Step 2: Compile the Testbench

```bash
gcc -O2 -std=c11 -Wall -Wextra -pedantic testbench.c -o testbench.exe
```

### Step 3: Generate Test Cases

```bash
.\generate.exe
```

### Step 4: Run the Benchmark

```bash
.\testbench.exe 3
```

The argument specifies the number of timing repetitions.

- Use `3` for a quick benchmark.
- Use `10` for more timing repetitions.

**Note:** Keep the executables in `src/` (or the project root). Both programs locate the project root from the executable location, independent of the launching directory. All inputs go to the project root's `data/`, and results go to `output/results.csv`. Manifest paths have exactly one `data/` prefix and are resolved from the project root. Re-running overwrites the same output files rather than appending rows or creating additional output directories.

## 4. Benchmark Configuration

### 4.1 Test Profiles

The experiments cover six profiles at the following input sizes:

\[
N \in \{16,32,64,100,256,512,1000,2000,5000,10000,20000,50000,100000\}
\]

Each profile uses two fixed random seeds.

Sizes are configured in `generate.c`, not hard-coded in the testbench. The testbench reads every case listed in `data/manifest.csv`: 13 sizes × 6 profiles × 2 seeds + 2 known-answer cases = 158 cases. The original five sizes retain their previous seeds and filenames.

All test cases use:

\[
D = 1,000,000
\]

| Profile | Price Range | Coupon Range | Description |
|---|---|---|---|
| `random` | 1,000–2,000 | 1–999 | Random prices and coupon values. |
| `duplicates` | 100–105 | 95–99 | Many duplicate and closely spaced values. |
| `near_limit` | 999,999,900–1,000,000,000 | 999,999,800–999,999,899 | Values close to the upper input limit. |
| `unaffordable` | 500,000,000–1,000,000,000 | 1–1,000,000 | Most purchases are unaffordable. |
| `many_purchases` | 2–3 | 1 | Allows up to 1,000,000 purchases. |
| `ordered` | Ascending prices | Ascending / Descending coupons | Tests ordered input configurations. |

### 4.2 Additional Known-Answer Cases

Two additional test cases have independently specified expected answers.

**Case 1: Sample**

```text
N = 4
D = 30

Prices:  12 20 15 10
Coupons: 9 6 8 7

Expected Output:
8 2
```

**Case 2: Original-Price Purchases**

```text
N = 1
D = 10

Prices:  5
Coupons: 4

Expected Output:
2 4
```

This case verifies that purchasing items at their original prices is unlimited.

## 5. Reference Checker

The testbench uses an independent reference algorithm to verify the solver's results.

### 5.1 Algorithm

The reference checker uses:

- Standard C `qsort` for sorting.
- Prefix sums for efficient cost calculations.
- Binary search on the paid price threshold.

For a given threshold, it counts the cost and quantity of **all item-type/coupon-type pairs** satisfying:

\[
price_i-coupon_j \leq threshold
\]

When necessary, it completes the remaining purchases at the next original-price threshold.

The reference implementation does not use the tested heap or quick-sort implementation.

### 5.2 Complexity

| Resource | Complexity |
|---|---|
| Time | \(O(N\log N+N\log D)\) |
| Space | \(O(N)\) |

All accumulated sums use 64-bit integers to avoid integer overflow.

The two known-answer cases are additionally checked against their fixed expected answers.

## 6. Solver Interface and K

The current implementation is in `shopping_with_cunpons.h`.

Your implementation must provide the following interface:

```c
Result solveShopping(
    int n,
    int d,
    int prices[],
    int coupons[]
);
```

### 6.1 Requirements

1. Return the maximum number of items that can be purchased and the remaining money:

   ```c
   return (Result){maxItems, remainingMoney};
   ```

2. Remove all `printf` statements from the timed solver code.
3. Release all dynamically allocated memory before returning.
4. Do not modify the `prices` array.
5. The `coupons` array may be sorted in place.

### 6.2 Current Solver Implementation

The bundled solver uses a heap-based implementation.

Its main characteristics are:

- Stores actual prices and item IDs in the heap, with coupon indices stored per item.
- Builds the heap using N insertions.
- Performs one extraction and at most one insertion for each coupon purchase.

The testbench calls `solveShoppingCounted(n, d, prices, coupons, &k)`. Each invocation initializes a local counter to zero and increments it immediately after a successful coupon purchase extracts the heap minimum. Bulk original-price purchases and an unaffordable final attempt do not increment K. The original `solveShopping()` entry point remains available.

K is checked for the warm-up and every timed repetition against an independent reference count, and written as the `k` column in `output/results.csv`. All repetitions must produce the same expected K; it is not accumulated or multiplied by `repeats`. The reference count is `min(expected_items, M)`, where M counts all price/coupon pairs strictly cheaper than the minimum original price. Sorting and a two-pointer scan compute M outside the timed region, without enumerating N² pairs. For example, the sample has `items=8, k=8`, while `original_price` has `items=2, k=1`.

The plotting script reads this measured `k` column directly; only older CSV files without `k` use reconstruction from input data.

## 7. Timing Methodology

The benchmark measures only the execution time of the solver.

**Included in timing:**

- Sorting.
- Memory allocation.
- Heap construction.
- Purchasing operations.

**Excluded from timing:**

- Input generation.
- File reading and writing.
- Input validation.
- Reference-answer calculation.
- Coupon copying.
- Compiler execution.

The testbench performs one warm-up run before the repeated timing measurements.

## 8. Notes and Limitations

- All datasets are synthetic and reproducible.
- The test cases are not official online judge data.
- Timing results depend on CPU, compiler options, and system load.
- `results.csv` contains actual measurements from the current machine.

When comparing different implementations, use the same generated datasets, compiler options, and repetition count to ensure a fair comparison.
