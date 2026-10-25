ifeq ($(OS),Windows_NT)
export PATH := C:/msys64/ucrt64/bin;$(PATH)
define COMPILE
	gcc $(CFLAGS) $(SOURCES) -o $@
endef
else
CC = gcc
define COMPILE
	$(CC) $(CFLAGS) $(SOURCES) -o $@
endef
endif

CFLAGS = -std=c11 -O2 -Wall -Wextra -pedantic
PYTHON = python

DIJKSTRA_EXE = dijkstra.exe
BENCH_EXE = testbench_dijkstra_time.exe
DATASET ?= NY
MAX_QUERIES ?= 1000
MIN_QUERIES ?= 100
STEP ?= 100
REPETITIONS ?= 1
CSV ?= dijkstra_runtime_$(DATASET).csv

.PHONY: all benchmark plot test clean

all: $(DIJKSTRA_EXE) $(BENCH_EXE)

$(DIJKSTRA_EXE): dijkstra.c dijkstra_main.c dijkstra.h binary_min_heap.h Fibonacci_heap.h
	$(eval SOURCES = dijkstra.c dijkstra_main.c)
	$(COMPILE)

$(BENCH_EXE): dijkstra.c testbench_dijkstra_time.c dijkstra.h binary_min_heap.h Fibonacci_heap.h
	$(eval SOURCES = dijkstra.c testbench_dijkstra_time.c)
	$(COMPILE)

benchmark: $(BENCH_EXE)
	./$(BENCH_EXE) $(DATASET) $(MAX_QUERIES) $(MIN_QUERIES) $(STEP) $(REPETITIONS) $(CSV)

plot:
	$(PYTHON) visualization.py $(CSV)

testbench_dijkstra.exe: dijkstra.c testbench_dijkstra.c dijkstra.h binary_min_heap.h Fibonacci_heap.h
	$(eval SOURCES = dijkstra.c testbench_dijkstra.c)
	$(COMPILE)

test: testbench_dijkstra.exe
	./testbench_dijkstra.exe

clean:
ifeq ($(OS),Windows_NT)
	-powershell -NoProfile -Command "Remove-Item -Force -ErrorAction SilentlyContinue *.exe,*.csv,visualizations/dijkstra_runtime_*.png"
else
	$(RM) *.exe *.csv
	$(RM) visualizations/dijkstra_runtime_*.png
endif
