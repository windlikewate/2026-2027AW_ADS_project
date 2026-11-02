ADS Project 3: Recover the Design
=================================

1. File Organization in 'code/'
-------------------------------
- recover_the_design.h     : Core data structures (Vertex, Edge, Graph, result), row-major
                             candidate-graph construction (init_Graph), lookahead degree
                             counting (count_remaining_edges), orthogonal crossing detection
                             (is_crossing), edge validation (check_edge), and directional
                             output formatting (show_result).
- recover_the_design.c     : Recursive depth-first backtracking solver (backtrack) with root
                             handshaking parity pruning, inclusion-first branching, and
                             exclusion lookahead pruning, plus the CLI entry point (main).
- testbench_time.c         : Windows batch timing driver that runs recover_the_design.exe on
                             all 360 .has instances via CreateProcessA and logs wall-clock
                             runtimes to benchmark_results_100.csv.
- testbench_profile.c      : Cross-platform instrumented profiler that counts recursive calls
                             and measures pure in-memory backtracking search time.
- verify_output.py         : Independent structural verifier that checks row-major ordering,
                             connector degree satisfaction, nearest-neighbor symmetry, and
                             non-crossing constraints on solver outputs.
- visualization.py         : Python script to generate the scaling, runtime distribution, and
                             recovered fence layout figures in visualizations/.
- benchmark_results_100.csv: Recorded end-to-end execution times across the 360 revised
                             100-island benchmark instances.
- Makefile                 : Build and automation script.

2. How to Build and Run
-----------------------
Option A: Using Make
  make all        # Build the solver and profiler (and testbench_time.exe on Windows)
  make plot       # Run visualization.py to generate all PDF/PNG figures in visualizations/
  make benchmark  # Run the Windows batch timing driver (Windows only)
  make clean      # Remove compiled binaries and temporary build files

Option B: Direct Compilation with GCC
  # 1. Interactive / single-instance fence recovery solver
  gcc -std=c11 -O2 -Wall -Wextra recover_the_design.c -o recover_the_design
  ./recover_the_design < input.txt > output.txt

  # 2. Verify a recovered layout against all problem constraints
  python3 verify_output.py input.txt output.txt

  # 3. Pure backtracking profiler (outputs: rows cols |V| |E| |E*| calls time_us status)
  gcc -std=c11 -O2 -Wall -Wextra testbench_profile.c -o testbench_profile
  ./testbench_profile < ../data/Hashi_Puzzles/100_revised/Hs_16_100_25_00_001.has

  # 4. Windows end-to-end batch benchmark driver (requires Windows / MinGW)
  gcc -std=c11 -O2 -Wall -Wextra recover_the_design.c -o recover_the_design.exe
  gcc -std=c11 -O2 -Wall -Wextra testbench_time.c -o testbench_time.exe
  ./testbench_time.exe

  # 5. Generate benchmark and layout figures
  python3 visualization.py

3. Datasets and Notes
---------------------
- Constructed scaling boards (L05-L20 solvable, U05-U20 odd-degree unsolvable) are located in
  ../data/Hashi_Puzzles/recover_the_design_large_tests.txt.
- The 360 revised 100-island solvable single-fence instances (16x16 grid) are located in
  ../data/Hashi_Puzzles/100_revised/, adapted from the public Hashiwokakero benchmark of
  Coelho et al. (https://w1.cirrelt.ca/~vidalt/en/research-data.html), with reference outputs
  in ../data/Hashi_Puzzles/100_solutions/.
- Input format: first line "n m" (1 <= n, m <= 50), followed by n rows of m integers in {0..4}.
- Output format: one line "row col up down left right" (1-based) per connector in row-major
  order, or "No Solution" if no valid non-crossing fence layout exists.
