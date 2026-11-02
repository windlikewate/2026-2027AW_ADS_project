ifeq ($(OS),Windows_NT)
CC = gcc
PYTHON = python
EXE = .exe
else
CC = gcc
PYTHON = python3
EXE =
endif

CFLAGS = -std=c11 -O2 -Wall -Wextra

.PHONY: all solver profile benchmark plot clean

ifeq ($(OS),Windows_NT)
all: recover_the_design.exe testbench_time.exe testbench_profile
else
all: recover_the_design$(EXE) testbench_profile
endif

recover_the_design$(EXE): recover_the_design.c recover_the_design.h
	$(CC) $(CFLAGS) recover_the_design.c -o $@

testbench_profile: testbench_profile.c recover_the_design.h
	$(CC) $(CFLAGS) testbench_profile.c -o $@

testbench_time.exe: testbench_time.c
	$(CC) $(CFLAGS) testbench_time.c -o $@

solver: recover_the_design$(EXE)

profile: testbench_profile

benchmark: recover_the_design.exe testbench_time.exe
	./testbench_time.exe

plot: testbench_profile
	$(PYTHON) visualization.py

clean:
ifeq ($(OS),Windows_NT)
	-powershell -NoProfile -Command "Remove-Item -Force -ErrorAction SilentlyContinue *.exe,testbench_profile,temp_output.txt"
else
	$(RM) -r recover_the_design recover_the_design.exe testbench_time.exe testbench_profile temp_output.txt build
endif
