ADS Project 1: Compare Search Trees
===================================

1. File Organization in 'code/'
-------------------------------
- binaray_search_tree.h : Iterative unbalanced Binary Search Tree (BST) implementation.
- AVL_tree.h            : Self-balancing AVL Tree implementation (LL, RR, LR, RL rotations).
- splay_tree.h          : Self-adjusting Splay Tree implementation (zig, zig-zig, zig-zag).
- red_black_tree.h      : Red-Black Tree implementation (iterative insert and delete fix-ups).
- B_plus_tree.h         : Order-4 B+ Tree implementation (splitting, borrowing, merging).
- testbench_time.c      : Performance benchmark driver for all five search trees.
- testbench_figure.c    : Small-scale structural verification and Graphviz DOT/PNG exporter.
- visualization.py      : Python script to plot runtime curves from data_set.csv.
- Makefile              : Build and automation script.

2. How to Build and Run
-----------------------
Option A: Using Make
  make time       # Compile testbench_time.c and generate data_set.csv (N = 1000..50000)
  make time_pdf   # Run visualization.py to plot the 6 runtime figures in visualizations/
  make show       # Compile testbench_figure.c and render tree topologies via Graphviz dot
  make clean      # Remove generated executables and output files

Option B: Direct Compilation with GCC
  gcc -std=c11 -O2 testbench_time.c -o testbench_time
  ./testbench_time [startN] [endN] [step] [repetitions] [output.csv]

  (Default parameters: startN=1000, endN=50000, step=1000, repetitions=3, output=data_set.csv)

  gcc -std=c11 -O2 testbench_figure.c -o testbench_figure
  ./testbench_figure
