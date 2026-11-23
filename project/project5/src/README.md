
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