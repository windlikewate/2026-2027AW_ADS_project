Shopping with Coupons — Source Code Guide (Readme.txt)
======================================================

1. Recommended File Browsing Order
----------------------------------
  1) bench.h                  : Shared `Result { count, money }` return structure.
  2) quick_sort.h             : In-place descending median-of-three 3-way QuickSort
                                with small-subarray selection-sort cutoff (<= 16).
  3) binary_min_heap.h        : Contiguous parallel-array 64-bit Binary Min-Heap
                                (`data[]`, `vertices[]`) + Graphviz DOT exporter.
  4) shopping_with_cunpons.h  : Core O((N + K) log N) greedy frontier solver with
                                O(1) original-price bulk-purchase cutoff (`B = min P_i`).
  5) testbench_demo.c         : Interactive standard-input CLI solver matching the
                                problem statement's exact stdin/stdout format.
  6) project_paths.h          : Cross-platform project-root path resolver (Win32/POSIX).
  7) generate.c               : Deterministic generator for all 158 `.in` test cases
                                and `data/manifest.csv`.
  8) testbench.c              : Automated verification & timing harness with an
                                independent O(N log N + N log D) binary-search oracle.
  9) plot_complexity.py       : Non-negative least-squares fit (`complexity_fit.csv`).
 10) visualization.py         : Vector PDF & 300-DPI PNG report plot generator.

2. Test Data Organization in '../data/'
---------------------------------------
  - sample.in                 : Assignment sample (N = 4, D = 30 -> expected: 8 2).
  - original_price.in         : Unlimited full-price check (N = 1, D = 10 -> expected: 2 4).
  - <profile>_n<N>_s<seed>.in : 156 scaling cases across 13 sizes (N = 16 .. 100,000)
                                and 6 profiles (`random`, `duplicates`, `near_limit`,
                                `unaffordable`, `many_purchases`, `ordered`).
  - manifest.csv              : Master index read by `testbench`.

3. How to Build and Run (C11 / GCC)
-----------------------------------
Run from the `src/` (or `code/`) directory:

A) Interactive Standard-Input Solver (`testbench_demo`):
     gcc -O2 -std=c11 -Wall -Wextra -pedantic testbench_demo.c -o testbench_demo
     ./testbench_demo < ../data/sample.in
     # Output: 8 2

B) Dataset Generator & Full 158-Case Verification Benchmark (`generate`, `testbench`):
     gcc -O2 -std=c11 -Wall -Wextra -pedantic generate.c -o generate
     gcc -O2 -std=c11 -Wall -Wextra -pedantic testbench.c -o testbench

   # (Optional) Regenerate all 158 `.in` inputs and `../data/manifest.csv`:
     ./generate

   # Verify all 158 cases against the independent oracle and time 3 repetitions
   # (Note: overwrites `../output/results.csv` when run):
     ./testbench 3

C) Regenerate Report Figures (Python 3 + NumPy + Matplotlib):
     python3 visualization.py
