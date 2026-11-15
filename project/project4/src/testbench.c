/* Windows/MinGW C11 benchmark for the existing knapsack implementations.
 * The controller discovers .kp files below R<digits> directories. Each solve
 * runs in a separate, time/memory-limited process so one failure cannot abort
 * the batch. Only the algorithm call is timed; parsing and process startup
 * are excluded. See doc/testbench.md for commands and CSV field definitions.
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

#define PATH_BUFFER 32768
typedef int (*Solver)(int, int, int *, int *);
static const char *algorithm_names[] = {"origin", "list", "bound"};
static Solver solvers[] = {knapsack_origin, knapsack_list, knapsack_list_with_bound};

typedef struct {
    int n, capacity, max_weight;
    int *profits, *weights;
} Instance;

typedef struct {
    char **paths;
    size_t size, allocated;
} FileList;

typedef struct {
    const char *status;
    int profit;
    double seconds;
    DWORD exit_code;
} Result;

/* Return a status rather than terminating the whole batch on invalid input.
 * Existing solvers use int arithmetic, so unsupported ranges are rejected.
 */
static const char *read_instance(const char *path, Instance *in) {
    FILE *f = fopen(path, "r");
    long long n, capacity, profit, weight, sum = 0;
    const char *status = "invalid_input";
    memset(in, 0, sizeof(*in));
    if (!f) return "input_open_error";
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
    in->profits = (int *)malloc((size_t)n * sizeof(int));
    in->weights = (int *)malloc((size_t)n * sizeof(int));
    if (!in->profits || !in->weights) {
        status = "allocation_error";
        goto done;
    }
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
        if (weight > in->max_weight) in->max_weight = (int)weight;
    }
    /* Whitespace, including the blank line after capacity, is allowed. */
    {
        char extra;
        if (fscanf(f, " %c", &extra) == 1) goto done;
    }
    status = "ok";
done:
    fclose(f);
    return status;
}

static void free_instance(Instance *in) {
    free(in->profits);
    free(in->weights);
}

static int algorithm_id(const char *name) {
    for (int i = 0; i < 3; ++i)
        if (!strcmp(name, algorithm_names[i])) return i;
    return -1;
}

/* Internal worker protocol: a completed worker writes profit and elapsed time
 * to its private temporary file. A crash cannot produce a successful row.
 */
static int worker(const char *algorithm, const char *input, const char *output) {
    Instance in;
    LARGE_INTEGER frequency, start, end;
    int id = algorithm_id(algorithm);
    const char *status = read_instance(input, &in);
    if (id < 0 || strcmp(status, "ok")) {
        free_instance(&in);
        return 2;
    }
    if (!QueryPerformanceFrequency(&frequency) ||
        !QueryPerformanceCounter(&start)) {
        free_instance(&in);
        return 3;
    }
    int profit = solvers[id](in.n, in.capacity, in.profits, in.weights);
    QueryPerformanceCounter(&end);
    double elapsed = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
    free_instance(&in);
    FILE *f = fopen(output, "w");
    if (!f) return 4;
    int written = fprintf(f, "%d %.12f\n", profit, elapsed);
    int closed = fclose(f);
    return written < 0 || closed != 0 ? 4 : 0;
}

static int is_r_directory(const char *name) {
    if (*name++ != 'R' || !*name) return 0;
    for (; *name; ++name) if (*name < '0' || *name > '9') return 0;
    return 1;
}

/* Also recognize an R directory when the selected root is inside it. */
static int path_under_r(const char *path) {
    const char *begin = path;
    for (const char *p = path; ; ++p) {
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

static int join_path(char *out, const char *directory, const char *name) {
    int n = snprintf(out, PATH_BUFFER, "%s\\%s", directory, name);
    return n >= 0 && n < PATH_BUFFER;
}

static int add_file(FileList *files, const char *path) {
    if (files->size == files->allocated) {
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

/* Traverse real directories only; junctions/symlinks are skipped to avoid cycles.
 * .kp is the data extension documented by the bundled kplib README.
 */
static int discover(const char *directory, int under_r, FileList *files) {
    char pattern[PATH_BUFFER], path[PATH_BUFFER];
    WIN32_FIND_DATAA entry;
    if (!join_path(pattern, directory, "*")) return 0;
    HANDLE search = FindFirstFileA(pattern, &entry);
    if (search == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) return 1;
        fprintf(stderr, "Cannot scan: %s (Windows error %lu)\n", directory, GetLastError());
        return 0;
    }
    int ok = 1;
    do {
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
        if (!join_path(path, directory, entry.cFileName)) { ok = 0; break; }
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!discover(path, under_r || is_r_directory(entry.cFileName), files)) {
                ok = 0;
                break;
            }
        } else {
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

static int compare_paths(const void *a, const void *b) {
    return strcmp(*(const char * const *)a, *(const char * const *)b);
}

/* Jobs limit memory and kill the worker when the controller closes the job.
 * CREATE_SUSPENDED ensures the solver cannot start before limits are applied.
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
    DWORD length = GetTempPathA(MAX_PATH, temp_dir);
    if (!length || length >= MAX_PATH || !GetTempFileNameA(temp_dir, "kpb", 0, temp_file))
        return result;
    size_t bytes = strlen(exe) + strlen(path) + strlen(temp_file) + 100;
    command = (char *)malloc(bytes);
    if (!command) goto cleanup;
    /* Windows filenames cannot contain quotes. These file paths have no
     * trailing slash, so quoted arguments follow Windows command-line rules. */
    snprintf(command, bytes, "\"%s\" --worker %s \"%s\" \"%s\"",
             exe, algorithm_names[algorithm], path, temp_file);
    job = CreateJobObjectA(NULL, NULL);
    if (!job) goto cleanup;
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (memory_mb) {
        limits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
        limits.ProcessMemoryLimit = (SIZE_T)memory_mb * 1024 * 1024;
    }
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
        goto cleanup;
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
    DWORD wait = WaitForSingleObject(process.hProcess, timeout_ms);
    if (wait == WAIT_TIMEOUT) {
        result.status = "timeout";
        TerminateJobObject(job, 1);
        WaitForSingleObject(process.hProcess, INFINITE);
        goto cleanup;
    }
    if (wait != WAIT_OBJECT_0) goto cleanup;
    GetExitCodeProcess(process.hProcess, &result.exit_code);
    if (result.exit_code != 0) {
        result.status = "worker_failed";
        goto cleanup;
    }
    FILE *f = fopen(temp_file, "r");
    if (f) {
        if (fscanf(f, "%d%lf", &result.profit, &result.seconds) == 2)
            result.status = "ok";
        fclose(f);
    }
cleanup:
    if (job) CloseHandle(job);
    if (process.hThread) CloseHandle(process.hThread);
    if (process.hProcess) CloseHandle(process.hProcess);
    free(command);
    DeleteFileA(temp_file);
    return result;
}

/* Always quote paths, doubling embedded quotes for standard CSV readers. */
static void csv_string(FILE *f, const char *value) {
    fputc('"', f);
    for (; *value; ++value) {
        if (*value == '"') fputc('"', f);
        fputc(*value, f);
    }
    fputc('"', f);
}

static int parse_unsigned(const char *text, unsigned *value) {
    char *end;
    errno = 0;
    if (!*text || *text == '-') return 0;
    unsigned long n = strtoul(text, &end, 10);
    if (errno || *end || n > UINT_MAX) return 0;
    *value = (unsigned)n;
    return 1;
}

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

int main(int argc, char **argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    /* Worker has four arguments after the executable. */
    if (argc == 5 && !strcmp(argv[1], "--worker"))
        return worker(argv[2], argv[3], argv[4]);
    const char *root = "kplib", *output = "benchmark.csv", *algorithm = NULL;
    unsigned repeat = 1, timeout = 60, memory = 1024;
    unsigned max_n = UINT_MAX, max_capacity = UINT_MAX, limit = UINT_MAX;
    int dry_run = 0;
    for (int i = 1; i < argc; ++i) {
        const char *key = argv[i];
        if (!strcmp(key, "--help")) { usage(argv[0]); return 0; }
        if (!strcmp(key, "--dry-run")) { dry_run = 1; continue; }
        if (++i == argc) { fprintf(stderr, "Missing value for %s\n", key); return 2; }
        if (!strcmp(key, "--root")) root = argv[i];
        else if (!strcmp(key, "--output")) output = argv[i];
        else if (!strcmp(key, "--algorithm")) algorithm = argv[i];
        else {
            unsigned *value = NULL;
            if (!strcmp(key, "--repeat")) value = &repeat;
            else if (!strcmp(key, "--timeout")) value = &timeout;
            else if (!strcmp(key, "--memory-mb")) value = &memory;
            else if (!strcmp(key, "--max-n")) value = &max_n;
            else if (!strcmp(key, "--max-capacity")) value = &max_capacity;
            else if (!strcmp(key, "--limit")) value = &limit;
            if (!value || !parse_unsigned(argv[i], value)) {
                fprintf(stderr, "Invalid option/value: %s %s\n", key, argv[i]);
                return 2;
            }
        }
    }
    int selected = algorithm ? algorithm_id(algorithm) : -1;
    int all = algorithm && !strcmp(algorithm, "all");
    if ((!all && selected < 0) || !repeat || !timeout || timeout > (INFINITE - 1) / 1000 ||
        (uint64_t)memory * 1024 * 1024 > SIZE_MAX) {
        usage(argv[0]);
        return 2;
    }
    char root_path[PATH_BUFFER], exe[PATH_BUFFER], output_path[PATH_BUFFER];
    DWORD n = GetFullPathNameA(root, PATH_BUFFER, root_path, NULL);
    if (!n || n >= PATH_BUFFER || GetFileAttributesA(root_path) == INVALID_FILE_ATTRIBUTES ||
        !(GetFileAttributesA(root_path) & FILE_ATTRIBUTE_DIRECTORY)) {
        fprintf(stderr, "Invalid input directory: %s\n", root);
        return 2;
    }
    n = GetModuleFileNameA(NULL, exe, PATH_BUFFER);
    if (!n || n >= PATH_BUFFER) return 2;
    n = GetFullPathNameA(output, PATH_BUFFER, output_path, NULL);
    if (!n || n >= PATH_BUFFER) return 2;
    FileList files = {0};
    int exit_status = 0;
    FILE *csv = NULL;
    if (!discover(root_path, path_under_r(root_path), &files) || !files.size) {
        fprintf(stderr, "No data files found, or directory traversal failed.\n");
        exit_status = 2;
        goto finish;
    }
    qsort(files.paths, files.size, sizeof(*files.paths), compare_paths);
    /* Prevent an accidental --output pointing at one of the input datasets. */
    for (size_t i = 0; i < files.size; ++i) {
        if (!_stricmp(output_path, files.paths[i]) || !_stricmp(output_path, exe)) {
            fprintf(stderr, "Output must not overwrite input data or executable.\n");
            exit_status = 2;
            goto finish;
        }
    }
    csv = fopen(output, "w");
    if (!csv) { perror(output); exit_status = 2; goto finish; }
    fprintf(csv, "dataset,algorithm,n,capacity,max_weight,trial,profit,time_seconds,status,exit_code,agreement\n");
    size_t count = files.size < limit ? files.size : limit;
    size_t ok_rows = 0, failed_rows = 0, skipped_rows = 0;
    printf("Discovered %zu .kp files; selected %zu.\n", files.size, count);
    for (size_t i = 0; i < count; ++i) {
        Instance in;
        const char *input_status = read_instance(files.paths[i], &in);
        if (!strcmp(input_status, "ok") && (unsigned)in.n > max_n) input_status = "skipped_n";
        if (!strcmp(input_status, "ok") && (unsigned)in.capacity > max_capacity) input_status = "skipped_capacity";
        free_instance(&in);
        printf("[%zu/%zu] %s\n", i + 1, count, files.paths[i]);
        fflush(stdout);
        for (unsigned trial = 1; ; ++trial) {
            Result results[3];
            int first = all ? 0 : selected, last = all ? 2 : selected;
            int reference = 0, have_reference = 0, mismatch = 0, successes = 0;
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
            const char *agreement = successes > 1 ? (mismatch ? "mismatch" : "match") : "not_checked";
            if (mismatch) exit_status = 1;
            for (int a = first; a <= last; ++a) {
                Result *r = &results[a];
                csv_string(csv, files.paths[i]);
                fprintf(csv, ",%s,%d,%d,%d,%u,", algorithm_names[a], in.n, in.capacity, in.max_weight, trial);
                if (!strcmp(r->status, "ok")) {
                    fprintf(csv, "%d,%.9f", r->profit, r->seconds);
                    ++ok_rows;
                } else {
                    fputc(',', csv); /* Leave result and algorithm time empty on failure. */
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
    if (csv && fclose(csv) != 0) exit_status = 2;
    for (size_t i = 0; i < files.size; ++i) free(files.paths[i]);
    free(files.paths);
    return exit_status;
}
