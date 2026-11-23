ADS Project 4: Improving Dynamic Programming for the 0/1 Knapsack Problem
=========================================================================

1. File Organization in 'code/' (Recommended Browsing Order)
------------------------------------------------------------
 1) Knapsack_origin.h          : Baseline table-based dynamic programming solver (DP-1,
                                 `knapsack_origin`) filling an (n + 1) x (c + 1) matrix
                                 in Theta(n * c) time and Theta(n * c) heap space.
 2) Knapsack_List.h            : List-based dynamic programming solver (`knapsack_list`,
                                 Kellerer et al. Section 3.4) with sentinel-terminated
                                 two-way merging (`Mergelist`) and Pareto dominance pruning.
 3) Fraction_Knapsack.h        : Unit-profit ratio sorting (`selection_sort`) and greedy
                                 floored LP-relaxation upper bound (`fraction_knapsack`).
 4) Knapsack_List_with_bound.h : Hybrid list-based DP with branch-and-bound LP fathoming
                                 (`knapsack_list_with_bound` and `Mergelist_with_bound`,
                                 Kellerer et al. Section 3.5).
 5) Knapsack_w3.h              : Bonus deterministic O(n + W^3) exact solver (`knapsack_w3`,
                                 Polak, Rohwedder, & Wegrzycki, ICALP 2021) combining BFPRT
                                 linear-time maximal prefix selection, 2W proximity bounds,
                                 and weight-grouped concave (max, +)-convolution via SMAWK.
 6) testbench_demo.c           : Cross-platform sanity-check driver verifying that all four
                                 solvers return z* = 18 on a 5-item toy instance.
 7) test_w3.c                  : Standalone deterministic regression suite verifying SMAWK
                                 concave convolutions, 2^n exhaustive search, 5,000 random
                                 1D-DP comparisons, n = 100,000 tie handling, and errno guards.
 8) testbench.c                : Windows/MinGW C11 batch benchmark controller & Job-Object
                                 worker (`--worker`) supporting `--algorithm origin|list|bound|w3|all`,
                                 `--max-weight`, 1 GiB RAM cap, 60 s timeout, QPC timing,
                                 and cross-solver agreement checking.
 9) knapsack_generator.py      : Standard-library Python generator for reproducible `.kp`
                                 instances across item counts, capacities, and correlations.
10) experiment_bonus.py        : Controlled common-random-number experiment runner & plotter
                                 evaluating small-weight scaling (W = 16, n = 50..3200) and
                                 weight sensitivity (n = 500, c = 1000, W = 4..256).
11) plot_benchmark.py          : CLI plotting utility for aggregating `.csv` runs by n, c, or W.
12) visualization.py           : Script generating the four publication-grade PDF/PNG
                                 report figures in `visualizations/`.

2. Test Data Organization in '../data/'
---------------------------------------
 - bonus_advantage_inputs/     : 42 reproducible `.kp` knapsack test instances across
                                 small-weight scaling (W = 16, n = 50..3200) and weight
                                 sensitivity (n = 500, c = 1000, W = 4..256).
 - all_n*_R10000.csv           : Recorded isolated KPLIB R = 10,000 benchmark CSVs.
 - all_nvariable_C1000.csv     : Recorded fixed-capacity (c = 1,000, n = 50..1000) CSV.
 - bonus_advantage_*.csv       : Recorded four-solver small-W and W-sweep benchmark CSVs.
 - all_n50_R1000.csv           : Recorded four-solver KPLIB n = 50, R = 1,000 CSV.

3. How to Build and Run
-----------------------
A) Cross-Platform Toy & Bonus Regression Verification (Linux / macOS / Windows):
     gcc -std=c11 -O2 -Wall -Wextra testbench_demo.c -o testbench_demo && ./testbench_demo
     gcc -std=c11 -O2 -Wall -Wextra -Werror test_w3.c -o test_w3 && ./test_w3

B) Windows / MinGW-w64 Batch Benchmark Harness:
     gcc -std=c11 -O2 -Wall -Wextra testbench.c -o testbench.exe

   # Run all four algorithms on the bundled `.kp` test instances and verify agreement:
     .\testbench.exe --algorithm all --root ..\data\bonus_advantage_inputs\n\R00016 --output benchmark.csv

   # Or run on a full KPLIB directory with custom timeout (s), RAM cap (MiB), or max-weight:
     .\testbench.exe --algorithm all --root ..\kplib --max-n 200 --max-weight 32 --timeout 60 --memory-mb 1024 --output benchmark.csv

C) Reproduce Controlled Bonus Regime Experiments:
     python3 experiment_bonus.py
     python3 experiment_bonus.py --plot-only

D) Generate Reproducible Synthetic `.kp` Suites & Report Figures:
     python3 knapsack_generator.py --start 50 --end 1000 --step 1 --range 1000 --capacity 1000 --kind uncorrelated --seed 42 --output ..\generated_knapsack
     python3 visualization.py
