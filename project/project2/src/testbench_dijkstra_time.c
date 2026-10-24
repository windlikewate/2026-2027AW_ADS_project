/*
 * Benchmark Dijkstra on reproducible source/target pairs.
 * Usage: testbench_dijkstra_time [dataset] [maxQueries] [step] [repetitions] [output.csv]
 * Defaults: NY 1000 100 3 dijkstra_runtime.csv
 */

#include "dijkstra.h"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct {
    /* One source/target pair used by the reproducible benchmark workload. */
    int source;
    int target;
} Query;

static int parsePositive(const char* text, int* value){
    /* Reject malformed, zero, negative, and overflowing numeric arguments. */
    char* end = NULL;
    errno = 0;
    long parsed = strtol(text, &end, 10);
    if(errno != 0 || end == text || *end != '\0' || parsed <= 0 ||
       parsed > INT32_MAX)
        return 0;
    *value = (int)parsed;
    return 1;
}

static uint32_t nextRandom(uint32_t* state){
    /* Small deterministic xorshift generator; no platform-specific rand(). */
    uint32_t value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static Query* createQueries(int count, int vertexCount, uint32_t seed){
    /* Generate a fixed sequence of valid, non-self source/target queries. */
    Query* queries = (Query*)malloc((size_t)count * sizeof(Query));
    if(queries == NULL)
        return NULL;
    for(int i = 0; i < count; ++i){
        queries[i].source = 1 + (int)(nextRandom(&seed) % (uint32_t)vertexCount);
        queries[i].target = 1 + (int)(nextRandom(&seed) % (uint32_t)vertexCount);
        if(queries[i].target == queries[i].source)
            queries[i].target = queries[i].target % vertexCount + 1;
    }
    return queries;
}

static double benchmarkTime(void){
    /* Use a high-resolution monotonic-style wall-clock source per platform. */
#ifdef _WIN32
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return (double)now.tv_sec + (double)now.tv_nsec / 1e9;
#endif
}

static void runBatch(const RoadGraph* graph, const Query* queries, int count,
                    DijkstraHeapType heapType, double* seconds,
                    uint64_t* checksum){
    /* Time only complete Dijkstra calls; fold results into a cheap consistency check. */
    double start = benchmarkTime();
    uint64_t sum = 0;
    for(int i = 0; i < count; ++i){
        DijkstraResult result = runDijkstra(graph, queries[i].source,
                                             queries[i].target, heapType);
        if(result.reachable)
            sum += (uint64_t)result.distance;
        else
            sum ^= UINT64_C(0x9e3779b97f4a7c15) + (uint64_t)i;
    }
    *seconds = benchmarkTime() - start;
    *checksum = sum;
}

int main(int argc, char** argv){
    /* Run the same query prefixes for each heap and query-count measurement. */
    setvbuf(stdout, NULL, _IOLBF, 0);
    if(argc > 6){
        fprintf(stderr,
                "Usage: %s [NY|BAY|COL|graph.gr] [maxQueries] [step] "
                "[repetitions] [output.csv]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    const char* dataset = argc > 1 ? argv[1] : "NY";
    int maxQueries = 1000;
    int step = 100;
    int repetitions = 3;
    const char* csvPath = argc > 5 ? argv[5] : "dijkstra_runtime.csv";
    if((argc > 2 && !parsePositive(argv[2], &maxQueries)) ||
       (argc > 3 && !parsePositive(argv[3], &step)) ||
       (argc > 4 && !parsePositive(argv[4], &repetitions)) ||
       maxQueries < 1000 || step > maxQueries){
        fprintf(stderr,
                "maxQueries must be >= 1000; step and repetitions must be positive.\n");
        return EXIT_FAILURE;
    }

    /* Resolve and load the graph once, outside the timed benchmark region. */
    char graphPath[1024];
    if(!resolveRoadDataset(dataset, graphPath, sizeof(graphPath))){
        fprintf(stderr, "Cannot resolve dataset '%s'.\n", dataset);
        return EXIT_FAILURE;
    }
    RoadGraph* graph = loadDIMACSGraph(graphPath);
    if(graph == NULL)
        return EXIT_FAILURE;

    FILE* csv = fopen(csvPath, "w");
    if(csv == NULL){
        fprintf(stderr, "Cannot create CSV '%s'.\n", csvPath);
        destroyRoadGraph(graph);
        return EXIT_FAILURE;
    }
    /* Write a self-describing CSV header before recording measurements. */
    fprintf(csv,
            "dataset,query_pairs,heap,repetitions,avg_seconds,total_seconds,"
            "avg_ms_per_query,checksum\n");
    printf("Dataset: %s (%d vertices, %d arcs)\n", graphPath,
           graph->vertexCount, graph->loadedEdgeCount);
    printf("Writing benchmark results to %s\n", csvPath);

    /* A fixed seed makes runs reproducible across platforms and heap types. */
    uint32_t seed = UINT32_C(0x6d2b79f5);
    int status = EXIT_SUCCESS;
    for(int queryCount = step; queryCount <= maxQueries;){
        Query* queries = createQueries(queryCount, graph->vertexCount, seed);
        if(queries == NULL){
            fprintf(stderr, "Failed to allocate query pairs.\n");
            status = EXIT_FAILURE;
            break;
        }

        /* Run identical query batches for the binary and Fibonacci heaps. */
        const DijkstraHeapType heapTypes[] = {
            DIJKSTRA_BINARY_HEAP, DIJKSTRA_FIBONACCI_HEAP
        };
        const char* heapNames[] = {"binary", "fibonacci"};
        uint64_t binaryChecksum = 0;
        for(int h = 0; h < 2; ++h){
            double totalSeconds = 0.0;
            uint64_t checksum = 0;
            for(int repetition = 0; repetition < repetitions; ++repetition){
                double elapsed;
                uint64_t currentChecksum;
                runBatch(graph, queries, queryCount, heapTypes[h], &elapsed,
                         &currentChecksum);
                totalSeconds += elapsed;
                checksum ^= currentChecksum + (uint64_t)repetition;
            }
            double averageSeconds = totalSeconds / repetitions;
            if(h == 0)
                binaryChecksum = checksum;
            else if(checksum != binaryChecksum){
                fprintf(stderr, "ERROR: heap distance checksums disagree.\n");
                status = EXIT_FAILURE;
                break;
            }
            double averageMsPerQuery =
                averageSeconds * 1000.0 / queryCount;
            fputc('"', csv);
            for(const char* character = dataset; *character != '\0'; ++character){
                if(*character == '"')
                    fputc('"', csv);
                fputc(*character, csv);
            }
            fprintf(csv, "\",%d,%s,%d,%.9f,%.9f,%.9f,\"%" PRIu64 "\"\n",
                    queryCount, heapNames[h], repetitions,
                    averageSeconds, totalSeconds, averageMsPerQuery, checksum);
            fflush(csv);
            printf("%s: %d queries, average %.6f s (%.3f ms/query)\n",
                   heapNames[h], queryCount, averageSeconds,
                   averageMsPerQuery);
        }
        free(queries);
        if(status != EXIT_SUCCESS || queryCount == maxQueries)
            break;
        queryCount = queryCount > maxQueries - step ? maxQueries : queryCount + step;
    }

    fclose(csv);
    destroyRoadGraph(graph);
    return status;
}
