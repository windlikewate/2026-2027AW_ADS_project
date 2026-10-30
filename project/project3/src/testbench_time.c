
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define PROGRAM_PATH "recover_the_design.exe"
#define TEST_DIR "../data/Hashi_Puzzles/100"
#define RESULT_FILE "benchmark_results_100.csv"

#define MAX_PATH_LEN 1024
#define TIMEOUT_MS 60000

typedef struct {
    int rows;
    int columns;
} InstanceSize;


/* Read the first two integers as rows and columns. */
InstanceSize get_instance_size(const char* filename) {
    InstanceSize size = {0, 0};

    FILE* fp = fopen(filename, "r");

    if (fp == NULL) {
        return size;
    }

    fscanf(fp, "%d %d", &size.rows, &size.columns);

    fclose(fp);

    return size;
}


/* Run one instance and measure its execution time. */
double run_test(const char* filename) {
    char command[MAX_PATH_LEN * 3];
    char output_file[MAX_PATH_LEN];

    snprintf(output_file, sizeof(output_file),
             "temp_output.txt");

    snprintf(command, sizeof(command),
             "cmd.exe /C \"\"%s\" < \"%s\" > \"%s\"\"",
             PROGRAM_PATH, filename, output_file);

    STARTUPINFOA startup;
    PROCESS_INFORMATION process;

    ZeroMemory(&startup, sizeof(startup));
    ZeroMemory(&process, sizeof(process));

    startup.cb = sizeof(startup);

    LARGE_INTEGER frequency, start, end;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    BOOL success = CreateProcessA(
        NULL,
        command,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &startup,
        &process
    );

    if (!success) {
        printf("Failed to launch: %s\n", filename);
        return -1.0;
    }

    DWORD wait_result = WaitForSingleObject(
        process.hProcess,
        TIMEOUT_MS
    );

    if (wait_result == WAIT_TIMEOUT) {
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, INFINITE);

        CloseHandle(process.hProcess);
        CloseHandle(process.hThread);

        remove(output_file);

        return -1.0;
    }

    QueryPerformanceCounter(&end);

    double runtime =
        (double)(end.QuadPart - start.QuadPart)
        / frequency.QuadPart;

    CloseHandle(process.hProcess);
    CloseHandle(process.hThread);

    remove(output_file);

    if (wait_result != WAIT_OBJECT_0) {
        return -1.0;
    }

    return runtime;
}


int main(void) {
    WIN32_FIND_DATAA find_data;
    HANDLE handle;

    char search_path[MAX_PATH_LEN];

    snprintf(search_path, sizeof(search_path),
             "%s\\*.has", TEST_DIR);

    handle = FindFirstFileA(search_path, &find_data);

    if (handle == INVALID_HANDLE_VALUE) {
        printf("No .has files found in %s\n", TEST_DIR);
        return 1;
    }

    FILE* csv = fopen(RESULT_FILE, "w");

    if (csv == NULL) {
        printf("Cannot create CSV file.\n");
        FindClose(handle);
        return 1;
    }

    fprintf(csv, "ID,Size,Runtime(s)\n");

    int id = 0;

    do {
        if (find_data.dwFileAttributes &
            FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        char filename[MAX_PATH_LEN];

        snprintf(filename, sizeof(filename),
                 "%s\\%s", TEST_DIR, find_data.cFileName);

        InstanceSize size = get_instance_size(filename);

        double runtime = run_test(filename);

        id++;

        char size_string[64];

        if (size.rows > 0 && size.columns > 0) {
            snprintf(size_string, sizeof(size_string),
                     "%dx%d", size.rows, size.columns);
        }
        else {
            strcpy(size_string, "Unknown");
        }

        printf(
            "ID: %d | File: %s | Size: %s | Time: ",
            id, find_data.cFileName, size_string
        );

        if (runtime >= 0) {
            printf("%.6f s\n", runtime);

            fprintf(csv, "%d,%s,%.9f\n",
                    id, size_string, runtime);
        }
        else {
            printf("Timeout or Error\n");

            fprintf(csv, "%d,%s,Timeout/Error\n",
                    id, size_string);
        }

    } while (FindNextFileA(handle, &find_data));

    FindClose(handle);
    fclose(csv);

    printf("\nAll tests completed.\n");
    printf("Results saved to: %s\n", RESULT_FILE);

    return 0;
}
