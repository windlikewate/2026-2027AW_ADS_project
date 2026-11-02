/*
 * File: testbench_time.c
 * Description: Windows driver. It runs recover_the_design.exe once per
 *              .has file in TEST_DIR and writes ID, size, and seconds
 *              to benchmark_results_100.csv. This file includes windows.h
 *              and is compiled on Windows, not on Linux.
 */


#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h> /* FindFirstFile, CreateProcess, QueryPerformanceCounter. */


#define PROGRAM_PATH "recover_the_design.exe" /* Solver launched for every instance. */
#define TEST_DIR "../data/Hashi_Puzzles/100" /* Directory of .has boards. */
#define RESULT_FILE "benchmark_results_100.csv" /* One row per board. */

#define MAX_PATH_LEN 1024 /* Capacity of the path and command buffers. */
#define TIMEOUT_MS 60000 /* Kill a run that is still going after 60 seconds. */

/* Rows and columns read from the first line of one .has file. */
typedef struct {
    int rows;       /* Board height. */
    int columns;    /* Board width. */
} InstanceSize;


/*
 * Open one puzzle file and read its first two integers.
 * A missing file yields rows = 0 and columns = 0.
 */
InstanceSize get_instance_size(const char* filename) {
    InstanceSize size = {0, 0};

    FILE* fp = fopen(filename, "r");

    if (fp == NULL) {
        return size; /* Caller prints the size as Unknown. */
    }

    fscanf(fp, "%d %d", &size.rows, &size.columns); /* Header only; the grid is left for the solver. */

    fclose(fp);

    return size;
}


/*
 * Run the solver on one file and return the wall-clock seconds.
 * stdin is the puzzle file and stdout is discarded to temp_output.txt.
 * The return is negative when the process cannot be started, when the
 * wait fails, or when the run exceeds TIMEOUT_MS.
 */
double run_test(const char* filename) {
    char command[MAX_PATH_LEN * 3];
    char output_file[MAX_PATH_LEN];

    snprintf(output_file, sizeof(output_file),
             "temp_output.txt"); /* Solver stdout is thrown away after timing. */

    snprintf(command, sizeof(command),
             "cmd.exe /C \"\"%s\" < \"%s\" > \"%s\"\"",
             PROGRAM_PATH, filename, output_file);

    STARTUPINFOA startup;
    PROCESS_INFORMATION process;

    ZeroMemory(&startup, sizeof(startup)); /* CreateProcess requires a zeroed STARTUPINFO. */
    ZeroMemory(&process, sizeof(process));

    startup.cb = sizeof(startup);

    LARGE_INTEGER frequency, start, end;

    QueryPerformanceFrequency(&frequency); /* Ticks per second of the performance counter. */
    QueryPerformanceCounter(&start); /* Timestamp just before the child is created. */

    BOOL success = CreateProcessA(
        NULL,              /* The executable is the first token of command. */
        command,           /* cmd.exe redirects stdin from the puzzle file. */
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,  /* No extra console for the child. */
        NULL,
        NULL,
        &startup,
        &process
    );

    if (!success) {
        printf("Failed to launch: %s\n", filename);
        return -1.0; /* CreateProcess failed; nothing to wait for. */
    }

    DWORD wait_result = WaitForSingleObject(
        process.hProcess,
        TIMEOUT_MS       /* Give up once the limit has passed. */
    );

    if (wait_result == WAIT_TIMEOUT) {
        TerminateProcess(process.hProcess, 1); /* The board did not finish in time. */
        WaitForSingleObject(process.hProcess, INFINITE);

        CloseHandle(process.hProcess);
        CloseHandle(process.hThread);

        remove(output_file); /* The redirected stdout is not part of the result. */

        return -1.0; /* Counted as Timeout or Error in the CSV. */
    }

    QueryPerformanceCounter(&end); /* Timestamp after the child has exited. */

    double runtime =
        (double)(end.QuadPart - start.QuadPart)
        / frequency.QuadPart;

    CloseHandle(process.hProcess);
    CloseHandle(process.hThread);

    remove(output_file);

    if (wait_result != WAIT_OBJECT_0) {
        return -1.0; /* The wait ended for a reason other than exit or timeout. */
    }

    return runtime; /* Elapsed seconds, including process creation. */
}


/*
 * Walk every .has file in TEST_DIR, time the solver, and append a CSV row.
 * Directory entries are skipped. A negative runtime is written as
 * Timeout/Error instead of a number.
 */
int main(void) {
    WIN32_FIND_DATAA find_data;
    HANDLE handle;

    char search_path[MAX_PATH_LEN];

    snprintf(search_path, sizeof(search_path),
             "%s\\*.has", TEST_DIR); /* Non-recursive: only this directory. */

    handle = FindFirstFileA(search_path, &find_data);

    if (handle == INVALID_HANDLE_VALUE) {
        printf("No .has files found in %s\n", TEST_DIR); /* Nothing to time. */
        return 1;
    }

    /* Truncate any previous CSV so this run stands alone. */
    FILE* csv = fopen(RESULT_FILE, "w");

    if (csv == NULL) {
        printf("Cannot create CSV file.\n"); /* Leave the directory handle closed. */
        FindClose(handle); /* End the directory walk. */
        return 1;
    }

    fprintf(csv, "ID,Size,Runtime(s)\n"); /* Header consumed by the plotting script. */

    int id = 0; /* CSV identifier, starting at 1 for the first file. */

    /* Files arrive in the order FindFirstFile / FindNextFile return them. */
    do {
        if (find_data.dwFileAttributes &
            FILE_ATTRIBUTE_DIRECTORY) {
            continue; /* FindFirstFile also returns the directory itself. */
        }

        /* Join the directory and the file name with a Windows separator. */
        char filename[MAX_PATH_LEN];

        snprintf(filename, sizeof(filename),
                 "%s\\%s", TEST_DIR, find_data.cFileName);

        InstanceSize size = get_instance_size(filename); /* Used only for the size column. */

        double runtime = run_test(filename); /* Negative means timeout or launch failure. */

        id++; /* One identifier per file, including timeouts. */

        /* Printed both on the console and in the size column. */
        char size_string[64];

        if (size.rows > 0 && size.columns > 0) {
            snprintf(size_string, sizeof(size_string),
                     "%dx%d", size.rows, size.columns);
        }
        else {
            strcpy(size_string, "Unknown"); /* The header could not be read. */
        }

        printf(
            "ID: %d | File: %s | Size: %s | Time: ",
            id, find_data.cFileName, size_string
        );

        /* A negative runtime is a launch failure or a 60 second timeout. */
        if (runtime >= 0) {
            printf("%.6f s\n", runtime); /* Same clock reading that goes into the CSV. */

            fprintf(csv, "%d,%s,%.9f\n",
                    id, size_string, runtime);
        }
        else {
            printf("Timeout or Error\n"); /* The CSV stores the words, not a sentinel number. */

            fprintf(csv, "%d,%s,Timeout/Error\n",
                    id, size_string);
        }

    } while (FindNextFileA(handle, &find_data));

    FindClose(handle);
    fclose(csv); /* Flush every row before the process exits. */

    printf("\nAll tests completed.\n"); /* Every file in the directory has a row. */
    printf("Results saved to: %s\n", RESULT_FILE); /* Path of the CSV just closed. */

    return 0;
}
