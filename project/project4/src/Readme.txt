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
5) testbench_demo.c           : Cross-platform sanity-check driver verifying that all
                                three solvers return z* = 18 on a 5-item toy instance.
6) testbench.c                : Windows/MinGW C11 batch benchmark controller & Job-Object
                                worker (`--worker`) with 1 GiB RAM cap, 60 s timeout,
                                QueryPerformanceCounter timing, and agreement checking.
7) knapsack_generator.py      : Standard-library Python generator for reproducible `.kp`
                                instances across item counts, capacities, and correlations.
8) plot_benchmark.py          : CLI plotting utility for aggregating arbitrary `.csv` runs.
9) visualization.py           : Script generating the three publication-grade PDF/PNG
                                report figures in `visualizations/`.

2. How to Build and Run
-----------------------
A) Cross-Platform Toy Verification (Linux / macOS / Windows):
     gcc -std=c11 -O2 -Wall -Wextra testbench_demo.c -o testbench_demo
     ./testbench_demo

B) Windows / MinGW-w64 Batch Benchmark Harness:
     gcc -std=c11 -O2 -Wall -Wextra testbench.c -o testbench.exe

   # Run all three algorithms on a KPLIB R10000 directory and verify agreement:
     .\testbench.exe --algorithm all --root ..\kplib\00Uncorrelated\n00100\R10000 --output ..\data\all_n100_R10000.csv

   # Customize per-worker timeout (seconds) and committed-memory cap (MiB):
     .\testbench.exe --algorithm all --root ..\kplib --max-n 200 --timeout 60 --memory-mb 1024 --output benchmark.csv

C) Generate Reproducible Synthetic `.kp` Suites:
     python3 knapsack_generator.py --start 50 --end 1000 --step 1 --range 1000 --capacity 1000 --kind uncorrelated --seed 42 --output ..\generated_knapsack

D) Regenerate Report Figures from Recorded CSVs (`../data/*.csv`):
     python3 visualization.py
