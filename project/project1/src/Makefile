ifeq ($(OS),Windows_NT)
# Keep the UCRT64 runtime DLLs ahead of Conda/Git MinGW DLLs on Windows.
MSYS2_UCRT64 ?= C:/msys64/ucrt64/bin
define COMPILE
	powershell.exe -NoProfile -Command '$$env:PATH = "C:\msys64\ucrt64\bin;" + $$env:PATH; gcc $(CFLAGS) $< -o $@'
endef
else
CC = gcc
define COMPILE
	$(CC) $(CFLAGS) $< -o $@
endef
endif
CFLAGS = -std=c11 -O2 -Wall -Wextra -pedantic
PYTHON = python

TIME_EXE = testbench_time.exe
SHOW_EXE = testbench_figure.exe
TIME_DATA = data_set.csv
HEADERS = binaray_search_tree.h AVL_tree.h splay_tree.h red_black_tree.h B_plus_tree.h

.PHONY: time time_pdf show clean

# Compile and run the performance test.
time: $(TIME_EXE)
	./$(TIME_EXE)

$(TIME_EXE): testbench_time.c $(HEADERS)
	$(COMPILE)

# Generate the CSV automatically if it is missing or older than the executable.
$(TIME_DATA): $(TIME_EXE)
	./$(TIME_EXE)

# Generate all time figures from the benchmark CSV.
time_pdf: $(TIME_DATA) visualization.py
	$(PYTHON) visualization.py

# Compile and run the tree-structure visualization test.
show: $(SHOW_EXE)
	./$(SHOW_EXE)

$(SHOW_EXE): testbench_figure.c $(HEADERS)
	$(COMPILE)

# Remove generated executables, data, object files and all generated figures.
clean:
	$(RM) *.exe *.csv *.o *.obj *.png *.pdf *.dot
	$(RM) visualizations/*.png visualizations/*.pdf visualizations/*.dot
	$(RM) -r __pycache__
	-rmdir visualizations
