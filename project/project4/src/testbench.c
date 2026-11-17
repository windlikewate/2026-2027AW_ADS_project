/*
 * File: testbench.c
 * Description: Windows/MinGW C11 batch benchmark harness for evaluating the three
 *              0/1 knapsack solvers (`origin`, `list`, `bound`) across KPLIB and
 *              synthetic `.kp` directory hierarchies.
 *
 * Architecture & Design Notes:
 *   1. Controller Mode:
 *      - Recursively discovers `.kp` instance files beneath `R<digits>` directories.
 *      - Sorts discovered file paths lexicographically for deterministic execution.
 *      - Spawns an isolated child worker per (dataset, algorithm, trial) run.
 *      - Verifies cross-algorithm profit agreement (`match` vs. `mismatch`).
 *      - Writes RFC-4180 CSV rows and flushes after every trial.
 *   2. Worker Mode (`--worker <alg> <input> <tmp_out>`):
 *      - Runs inside a Windows Job Object with configurable wall-clock timeout
 *        (default 60 s) and committed-memory cap (default 1024 MiB).
 *      - Isolates out-of-memory faults (e.g., 0xC0000005 when DP-1 exceeds 1 GiB
 *        at n >= 500, R = 10000) so the remaining algorithms continue cleanly.
 *   3. Timing Protocol:
 *      - Uses `QueryPerformanceCounter` strictly around the in-memory solver call,
 *        excluding directory traversal, file parsing, and process startup.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "Knapsack_origin.h"
#include "Knapsack_List_with_bound.h"

/* Maximum buffer length for extended Windows paths and command lines. */
#define PATH_BUFFER 32768

/* Unified function-pointer signature shared by all three knapsack solvers. */
typedef int (*Solver)(int, int, int *, int *);

/* Canonical CLI algorithm identifiers and corresponding solver functions. */
static const char *algorithm_names[] = {"origin", "list", "bound"};
static Solver solvers[] = {knapsack_origin, knapsack_list, knapsack_list_with_bound};

/**
 * @brief In-memory representation of a single parsed `.kp` knapsack instance.
 */
typedef struct {
    int n, capacity, max_weight; /* Item count n, capacity c, and max_i w_i. */
    int *profits, *weights;      /* Heap-allocated arrays p[0..n-1], w[0..n-1]. */
} Instance;

/**
 * @brief Dynamic string array storing discovered `.kp` dataset paths.
 */
typedef struct {
    char **paths;                /* Heap-allocated array of NUL-terminated paths. */
    size_t size, allocated;      /* Number of stored paths and allocated slots. */
} FileList;

/**
 * @brief Outcome record returned by one isolated child-worker invocation.
 */
typedef struct {
    const char *status;          /* Status label: ok, timeout, worker_failed, etc. */
    int profit;                  /* Optimal profit reported when status == "ok". */
    double seconds;              /* Pure solver runtime in seconds (QPC). */
    DWORD exit_code;             /* Raw Windows process exit code (0 on success). */
} Result;

/**
 * @brief Reads and validates a `.kp` instance file without aborting on bad input.
 *
 * @details Existing C solvers use 32-bit signed `int` arithmetic. To prevent
 *          silent undefined behavior, this parser rejects instances where:
 *            - `n` or `capacity` falls outside 32-bit positive `int` bounds;
 *            - any weight is non-positive or `weight + capacity > INT_MAX`;
 *            - any profit is negative or total profit sum exceeds `INT_MAX`;
 *            - extra non-whitespace tokens trail the expected `n` item lines.
 *
 * @param path Path to the `.kp` file on disk.
 * @param in   Output struct populated on return (caller must call `free_instance`).
 * @return Status literal ("ok", "input_open_error", "invalid_input",
 *         "unsupported_input", or "allocation_error").
 */
static const char *read_instance(const char *path, Instance *in) {
    FILE *f = fopen(path, "r");
    long long n, capacity, profit, weight, sum = 0;
    const char *status = "invalid_input";
    memset(in, 0, sizeof(*in));
    if (!f) return "input_open_error";

    /* Parse header tokens: item count n and knapsack capacity c. */
    if (fscanf(f, "%lld%lld", &n, &capacity) != 2) goto done;
    if (n < 1 || n >= INT_MAX || capacity < 0 || capacity >= INT_MAX) {
        status = "unsupported_input";
        goto done;
    }
    in->n = (int)n;
    in->capacity = (int)capacity;
    if ((size_t)n > SIZE_MAX / sizeof(int)) {
        status = "unsupported_input";
        goto done;
    }

    /* Allocate contiguous profit and weight arrays for the n items. */
    in->profits = (int *)malloc((size_t)n * sizeof(int));
    in->weights = (int *)malloc((size_t)n * sizeof(int));
    if (!in->profits || !in->weights) {
        status = "allocation_error";
        goto done;
    }

    /* Parse each `(profit, weight)` line and enforce 32-bit overflow guards. */
    for (int i = 0; i < in->n; ++i) {
        if (fscanf(f, "%lld%lld", &profit, &weight) != 2) goto done;
        if (profit < 0 || profit > INT_MAX || weight <= 0 ||
            weight > INT_MAX - capacity || sum > INT_MAX - profit) {
            status = "unsupported_input";
            goto done;
        }
        sum += profit;
        in->profits[i] = (int)profit;
        in->weights[i] = (int)weight;
        /* Track largest single-item weight max_i w_i for CSV metadata. */
        if (weight > in->max_weight) in->max_weight = (int)weight;
    }

    /* Allow optional blank lines/spaces, but reject unexpected extra tokens. */
    {
        char extra;
        if (fscanf(f, " %c", &extra) == 1) goto done;
    }
    status = "ok";
done:
    fclose(f);
    return status;
}

/**
 * @brief Frees heap memory owned by `Instance` (safe even on partial reads).
 */
static void free_instance(Instance *in) {
    free(in->profits);
    free(in->weights);
}

/**
 * @brief Maps an algorithm name ("origin", "list", "bound") to index 0..2.
 * @return Solver index in {0, 1, 2}, or -1 if `name` is unrecognized.
 */
static int algorithm_id(const char *name) {
    for (int i = 0; i < 3; ++i)
        if (!strcmp(name, algorithm_names[i])) return i;
    return -1;
}

/**
 * @brief Executes a single isolated benchmark trial inside a child worker process.
 *
 * @details Invoked via `testbench.exe --worker <alg> <input_kp> <temp_out>`.
 *          Reads the input file first, brackets only `solvers[id](...)` with
 *          `QueryPerformanceCounter`, and writes `<profit> <seconds>` to `output`.
 */
static int worker(const char *algorithm, const char *input, const char *output) {
    Instance in;
    LARGE_INTEGER frequency, start, end;
    int id = algorithm_id(algorithm);
    const char *status = read_instance(input, &in);
    /* Abort with code 2 if input failed validation or algorithm ID is invalid. */
    if (id < 0 || strcmp(status, "ok")) {
        free_instance(&in);
        return 2;
    }
    /* Query high-resolution performance counter frequency and start timestamp. */
    if (!QueryPerformanceFrequency(&frequency) ||
        !QueryPerformanceCounter(&start)) {
        free_instance(&in);
        return 3;
    }

    /* Execute selected knapsack solver (includes internal sort/malloc/free). */
    int profit = solvers[id](in.n, in.capacity, in.profits, in.weights);

    /* Capture end timestamp immediately upon return and convert ticks to seconds. */
    QueryPerformanceCounter(&end);
    double elapsed = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
    free_instance(&in);

    /* Write `<profit> <elapsed>` to private temp file for parent controller. */
    FILE *f = fopen(output, "w");
    if (!f) return 4;
    int written = fprintf(f, "%d %.12f\n", profit, elapsed);
    int closed = fclose(f);
    return written < 0 || closed != 0 ? 4 : 0;
}

/**
 * @brief Checks whether a directory basename matches KPLIB's `R<digits>` pattern
 *        (e.g., `R01000` or `R10000`).
 */
static int is_r_directory(const char *name) {
    if (*name++ != 'R' || !*name) return 0;
    for (; *name; ++name) if (*name < '0' || *name > '9') return 0;
    return 1;
}

/**
 * @brief Checks whether `path` already lies inside an `R<digits>` ancestor folder,
 *        supporting `--root` paths aimed directly at `R10000` or a subfolder.
 */
static int path_under_r(const char *path) {
    const char *begin = path;
    for (const char *p = path; ; ++p) {
        /* Split path on forward slashes, backslashes, or string terminator. */
        if (*p == '/' || *p == '\\' || !*p) {
            size_t length = (size_t)(p - begin);
            if (length > 1 && begin[0] == 'R') {
                size_t i = 1;
                while (i < length && begin[i] >= '0' && begin[i] <= '9') ++i;
                if (i == length) return 1;
            }
            if (!*p) break;
            begin = p + 1;
        }
    }
    return 0;
}

/**
 * @brief Joins `directory` and `name` with `\` into fixed-capacity buffer `out`.
 */
static int join_path(char *out, const char *directory, const char *name) {
    int n = snprintf(out, PATH_BUFFER, "%s\\%s", directory, name);
    return n >= 0 && n < PATH_BUFFER;
}

/**
 * @brief Appends a newly duplicated path string to `files`, doubling buffer capacity.
 */
static int add_file(FileList *files, const char *path) {
    if (files->size == files->allocated) {
        /* Start with 256 slots and double geometrically on overflow. */
        size_t size = files->allocated ? files->allocated * 2 : 256;
        char **p = (char **)realloc(files->paths, size * sizeof(*p));
        if (!p) return 0;
        files->paths = p;
        files->allocated = size;
    }
    char *copy = (char *)malloc(strlen(path) + 1);
    if (!copy) return 0;
    strcpy(copy, path);
    files->paths[files->size++] = copy;
    return 1;
}

/**
 * @brief Recursively traverses `directory` using Win32 `FindFirstFileA` to
 *        collect `.kp` files located beneath any `R<digits>` directory.
 *
 * @details Skips `.` / `..` and NTFS reparse points (junctions/symlinks) to
 *          prevent infinite directory loops. Non-`.kp` README files are ignored.
 */
static int discover(const char *directory, int under_r, FileList *files) {
    char pattern[PATH_BUFFER], path[PATH_BUFFER];
    WIN32_FIND_DATAA entry;
    if (!join_path(pattern, directory, "*")) return 0;
    HANDLE search = FindFirstFileA(pattern, &entry);
    if (search == INVALID_HANDLE_VALUE) {
        /* Empty directory is not an error; report other Win32 scan errors. */
        if (GetLastError() == ERROR_FILE_NOT_FOUND) return 1;
        fprintf(stderr, "Cannot scan: %s (Windows error %lu)\n", directory, GetLastError());
        return 0;
    }
    int ok = 1;
    do {
        /* Skip self/parent entries and symbolic links or directory junctions. */
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
        if (!join_path(path, directory, entry.cFileName)) { ok = 0; break; }
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            /* Recurse into subdirectory, marking `under_r` once inside R<digits>. */
            if (!discover(path, under_r || is_r_directory(entry.cFileName), files)) {
                ok = 0;
                break;
            }
        } else {
            /* Record `.kp` instance files only when `under_r` is active. */
            const char *ext = strrchr(entry.cFileName, '.');
            if (under_r && ext && !_stricmp(ext, ".kp") && !add_file(files, path)) {
                ok = 0;
                break;
            }
        }
    } while (FindNextFileA(search, &entry));
    if (ok && GetLastError() != ERROR_NO_MORE_FILES) ok = 0;
    FindClose(search);
    return ok;
}

/**
 * @brief `qsort` callback ordering path strings in deterministic ASCII order.
 */
static int compare_paths(const void *a, const void *b) {
    return strcmp(*(const char * const *)a, *(const char * const *)b);
}

/**
 * @brief Runs one solver in a sandboxed Windows Job Object with timeout & RAM cap.
 *
 * @details Uses `CREATE_SUSPENDED` so `AssignProcessToJobObject` binds the
 *          `JOB_OBJECT_LIMIT_PROCESS_MEMORY` cap before the child begins executing.
 *          If the child exceeds `timeout_ms`, `TerminateJobObject` kills it cleanly.
 */
static Result run_solver(const char *exe, const char *path, int algorithm,
                         unsigned timeout_ms, unsigned memory_mb) {
    Result result = {"process_error", 0, 0.0, 0};
    char temp_dir[MAX_PATH], temp_file[MAX_PATH];
    char *command = NULL;
    HANDLE job = NULL;
    PROCESS_INFORMATION process = {0};
    STARTUPINFOA startup = {0};
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {0};
    startup.cb = sizeof(startup);

    /* Allocate a temporary IPC output file `kpb*.tmp` in the user temp folder. */
    DWORD length = GetTempPathA(MAX_PATH, temp_dir);
    if (!length || length >= MAX_PATH || !GetTempFileNameA(temp_dir, "kpb", 0, temp_file))
        return result;
    size_t bytes = strlen(exe) + strlen(path) + strlen(temp_file) + 100;
    command = (char *)malloc(bytes);
    if (!command) goto cleanup;

    /* Format quoted worker command line following Win32 argv parsing rules. */
    snprintf(command, bytes, "\"%s\" --worker %s \"%s\" \"%s\"",
             exe, algorithm_names[algorithm], path, temp_file);

    /* Create Job Object with kill-on-close and optional committed-RAM limit. */
    job = CreateJobObjectA(NULL, NULL);
    if (!job) goto cleanup;
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (memory_mb) {
        limits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
        limits.ProcessMemoryLimit = (SIZE_T)memory_mb * 1024 * 1024;
    }
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
        goto cleanup;

    /* Launch child suspended, attach to Job Object, then resume primary thread. */
    if (!CreateProcessA(exe, command, NULL, NULL, FALSE,
                        CREATE_SUSPENDED | CREATE_NO_WINDOW, NULL, NULL, &startup, &process))
        goto cleanup;
    if (!AssignProcessToJobObject(job, process.hProcess)) {
        TerminateProcess(process.hProcess, 1);
        goto cleanup;
    }
    if (ResumeThread(process.hThread) == (DWORD)-1) {
        TerminateProcess(process.hProcess, 1);
        goto cleanup;
    }

    /* Block up to `timeout_ms` ms; kill job and mark "timeout" if exceeded. */
    DWORD wait = WaitForSingleObject(process.hProcess, timeout_ms);
    if (wait == WAIT_TIMEOUT) {
        result.status = "timeout";
        TerminateJobObject(job, 1);
        WaitForSingleObject(process.hProcess, INFINITE);
        goto cleanup;
    }
    if (wait != WAIT_OBJECT_0) goto cleanup;

    /* Non-zero exit code signals crash/OOM (e.g., 3221225477 = 0xC0000005). */
    GetExitCodeProcess(process.hProcess, &result.exit_code);
    if (result.exit_code != 0) {
        result.status = "worker_failed";
        goto cleanup;
    }

    /* Parse profit and elapsed time produced by a successful worker exit. */
    FILE *f = fopen(temp_file, "r");
    if (f) {
        if (fscanf(f, "%d%lf", &result.profit, &result.seconds) == 2)
            result.status = "ok";
        fclose(f);
    }
cleanup:
    /* Always release OS handles, command buffer, and temporary IPC file. */
    if (job) CloseHandle(job);
    if (process.hThread) CloseHandle(process.hThread);
    if (process.hProcess) CloseHandle(process.hProcess);
    free(command);
    DeleteFileA(temp_file);
    return result;
}

/**
 * @brief Emits a double-quoted CSV string field, doubling any embedded quotes.
 */
static void csv_string(FILE *f, const char *value) {
    fputc('"', f);
    for (; *value; ++value) {
        if (*value == '"') fputc('"', f);
        fputc(*value, f);
    }
    fputc('"', f);
}

/**
 * @brief Parses a base-10 non-negative integer into `*value` with range checks.
 */
static int parse_unsigned(const char *text, unsigned *value) {
    char *end;
    errno = 0;
    if (!*text || *text == '-') return 0;
    unsigned long n = strtoul(text, &end, 10);
    if (errno || *end || n > UINT_MAX) return 0;
    *value = (unsigned)n;
    return 1;
}

/**
 * @brief Prints command-line usage instructions to standard output.
 */
static void usage(const char *exe) {
    printf("Usage: %s --algorithm origin|list|bound|all [options]\n"
           "  --root DIR          kplib root, type/n subtree, or Rxxxxx directory (default kplib)\n"
           "  --output FILE       CSV destination (default benchmark.csv)\n"
           "  --repeat N          independent trials per file/algorithm (default 1)\n"
           "  --timeout SECONDS   worker wall-time limit, including input/startup (default 60)\n"
           "  --memory-mb N       worker committed-memory limit (default 1024; 0 = unlimited)\n"
           "  --max-n N           skip larger n (default unlimited)\n"
           "  --max-capacity N    skip larger capacities (default unlimited)\n"
           "  --limit N           first N files in sorted order (default all)\n"
           "  --dry-run           scan/parse only, no solver calls\n"
           "  --help              show this message\n", exe);
}

/**
 * @brief Entry point handling both internal `--worker` mode and batch controller mode.
 */
int main(int argc, char **argv) {
    /* Disable Windows GPF crash popups so unattended batch jobs never stall. */
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

    /* Fast path: child worker process executes one solver call and exits. */
    if (argc == 5 && !strcmp(argv[1], "--worker"))
        return worker(argv[2], argv[3], argv[4]);

    /* Initialize controller configuration defaults. */
    const char *root = "kplib", *output = "benchmark.csv", *algorithm = NULL;
    unsigned repeat = 1, timeout = 60, memory = 1024;
    unsigned max_n = UINT_MAX, max_capacity = UINT_MAX, limit = UINT_MAX;
    int dry_run = 0;

    /* Parse CLI flags and numeric limits. */
    for (int i = 1; i < argc; ++i) {
        const char *key = argv[i];
        /* Print usage banner and exit 0 immediately on `--help`. */
        if (!strcmp(key, "--help")) { usage(argv[0]); return 0; }
        /* Enable inventory-only scan mode without spawning solver workers. */
        if (!strcmp(key, "--dry-run")) { dry_run = 1; continue; }
        /* All remaining options require an explicit value argument. */
        if (++i == argc) { fprintf(stderr, "Missing value for %s\n", key); return 2; }
        /* String-valued options: input root, CSV path, and solver selector. */
        if (!strcmp(key, "--root")) root = argv[i];
        else if (!strcmp(key, "--output")) output = argv[i];
        else if (!strcmp(key, "--algorithm")) algorithm = argv[i];
        else {
            /* Unsigned integer options: repetition count, timeout, memory, filters. */
            unsigned *value = NULL;
            if (!strcmp(key, "--repeat")) value = &repeat;
            else if (!strcmp(key, "--timeout")) value = &timeout;
            else if (!strcmp(key, "--memory-mb")) value = &memory;
            else if (!strcmp(key, "--max-n")) value = &max_n;
            else if (!strcmp(key, "--max-capacity")) value = &max_capacity;
            else if (!strcmp(key, "--limit")) value = &limit;
            /* Reject unknown flags or malformed/negative integer arguments. */
            if (!value || !parse_unsigned(argv[i], value)) {
                fprintf(stderr, "Invalid option/value: %s %s\n", key, argv[i]);
                return 2;
            }
        }
    }

    /*
     * Validate algorithm selection and numerical resource parameters:
     *   - `--algorithm` must be one of `origin`, `list`, `bound`, or `all`.
     *   - `repeat` and `timeout` must be positive, and `timeout * 1000` must
     *     fit strictly below `INFINITE` for `WaitForSingleObject`.
     *   - `memory * 1024 * 1024` must fit inside `SIZE_T` on the target ABI.
     */
    int selected = algorithm ? algorithm_id(algorithm) : -1;
    int all = algorithm && !strcmp(algorithm, "all");
    if ((!all && selected < 0) || !repeat || !timeout || timeout > (INFINITE - 1) / 1000 ||
        (uint64_t)memory * 1024 * 1024 > SIZE_MAX) {
        usage(argv[0]);
        return 2;
    }

    /*
     * Resolve canonical full paths for:
     *   1) `--root` directory to scan for `.kp` benchmarks;
     *   2) the running `testbench.exe` binary (used to spawn `--worker`);
     *   3) `--output` CSV path (checked against discovered inputs for safety).
     */
    char root_path[PATH_BUFFER], exe[PATH_BUFFER], output_path[PATH_BUFFER];
    DWORD n = GetFullPathNameA(root, PATH_BUFFER, root_path, NULL);
    if (!n || n >= PATH_BUFFER || GetFileAttributesA(root_path) == INVALID_FILE_ATTRIBUTES ||
        !(GetFileAttributesA(root_path) & FILE_ATTRIBUTE_DIRECTORY)) {
        fprintf(stderr, "Invalid input directory: %s\n", root);
        return 2;
    }
    /* Query path of current executable image for child worker command lines. */
    n = GetModuleFileNameA(NULL, exe, PATH_BUFFER);
    if (!n || n >= PATH_BUFFER) return 2;
    /* Canonicalize destination CSV path for case-insensitive collision check. */
    n = GetFullPathNameA(output, PATH_BUFFER, output_path, NULL);
    if (!n || n >= PATH_BUFFER) return 2;

    /*
     * Discover `.kp` files under `R<digits>` directories and sort paths
     * alphabetically so `--limit N` selects a reproducible prefix across runs.
     */
    FileList files = {0};
    int exit_status = 0;
    FILE *csv = NULL;
    if (!discover(root_path, path_under_r(root_path), &files) || !files.size) {
        fprintf(stderr, "No data files found, or directory traversal failed.\n");
        exit_status = 2;
        goto finish;
    }
    qsort(files.paths, files.size, sizeof(*files.paths), compare_paths);

    /*
     * Safety check: refuse to overwrite any discovered `.kp` dataset file or
     * the benchmark executable itself if `--output` was misconfigured.
     */
    for (size_t i = 0; i < files.size; ++i) {
        if (!_stricmp(output_path, files.paths[i]) || !_stricmp(output_path, exe)) {
            fprintf(stderr, "Output must not overwrite input data or executable.\n");
            exit_status = 2;
            goto finish;
        }
    }

    /*
     * Open output CSV stream and emit the 11-column schema consumed by
     * `plot_benchmark.py` and `visualization.py`.
     */
    csv = fopen(output, "w");
    if (!csv) { perror(output); exit_status = 2; goto finish; }
    fprintf(csv, "dataset,algorithm,n,capacity,max_weight,trial,profit,time_seconds,status,exit_code,agreement\n");

    /*
     * Main batch loop: iterate through the first `min(files.size, limit)`
     * datasets, check scale filters (`--max-n`, `--max-capacity`), and run
     * `repeat` independent worker trials per selected algorithm.
     */
    size_t count = files.size < limit ? files.size : limit;
    size_t ok_rows = 0, failed_rows = 0, skipped_rows = 0;
    printf("Discovered %zu .kp files; selected %zu.\n", files.size, count);
    for (size_t i = 0; i < count; ++i) {
        /* Read instance metadata (`n`, `capacity`, `max_weight`) and check skip filters. */
        Instance in;
        const char *input_status = read_instance(files.paths[i], &in);
        if (!strcmp(input_status, "ok") && (unsigned)in.n > max_n) input_status = "skipped_n";
        if (!strcmp(input_status, "ok") && (unsigned)in.capacity > max_capacity) input_status = "skipped_capacity";
        free_instance(&in);
        printf("[%zu/%zu] %s\n", i + 1, count, files.paths[i]);
        fflush(stdout);

        /* Execute `repeat` independent trials per dataset. */
        for (unsigned trial = 1; ; ++trial) {
            Result results[3];
            int first = all ? 0 : selected, last = all ? 2 : selected;
            int reference = 0, have_reference = 0, mismatch = 0, successes = 0;

            /*
             * Invoke each requested solver (`origin` -> `list` -> `bound`) in
             * its own Job-Object-isolated worker and compare returned optimal
             * profits across all solvers that completed with status == "ok".
             */
            for (int a = first; a <= last; ++a) {
                results[a] = (Result){input_status, 0, 0.0, 0};
                if (!strcmp(input_status, "ok")) {
                    if (dry_run) results[a].status = "dry_run";
                    else results[a] = run_solver(exe, files.paths[i], a, timeout * 1000, memory);
                }
                if (!strcmp(results[a].status, "ok")) {
                    if (have_reference && reference != results[a].profit) mismatch = 1;
                    reference = results[a].profit;
                    have_reference = 1;
                    ++successes;
                }
            }

            /* Set `agreement` column when >= 2 solvers return "ok" on the same trial. */
            const char *agreement = successes > 1 ? (mismatch ? "mismatch" : "match") : "not_checked";
            if (mismatch) exit_status = 1;

            /* Write one CSV row per solver and flush stream immediately. */
            for (int a = first; a <= last; ++a) {
                Result *r = &results[a];
                csv_string(csv, files.paths[i]);
                fprintf(csv, ",%s,%d,%d,%d,%u,", algorithm_names[a], in.n, in.capacity, in.max_weight, trial);
                if (!strcmp(r->status, "ok")) {
                    fprintf(csv, "%d,%.9f", r->profit, r->seconds);
                    ++ok_rows;
                } else {
                    fputc(',', csv); /* Omit profit and seconds when run did not succeed. */
                    if (!strncmp(r->status, "skipped_", 8) || !strcmp(r->status, "dry_run")) ++skipped_rows;
                    else { ++failed_rows; exit_status = 1; }
                }
                fprintf(csv, ",%s,%lu,%s\n", r->status, r->exit_code, agreement);
            }
            if (fflush(csv) != 0) { perror("CSV write"); exit_status = 2; goto finish; }
            if (trial == repeat) break;
        }
    }
    printf("CSV: %s\nRows: %zu ok, %zu failed, %zu skipped/dry-run.\n",
           output, ok_rows, failed_rows, skipped_rows);
finish:
    /* Close CSV file and free discovered path list before exiting. */
    if (csv && fclose(csv) != 0) exit_status = 2;
    for (size_t i = 0; i < files.size; ++i) free(files.paths[i]);
    free(files.paths);
    return exit_status;
}
