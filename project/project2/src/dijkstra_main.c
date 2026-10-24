#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dijkstra.h"

static int parseVertex(const char* text, int* value){
    /* Parse strictly so malformed IDs cannot silently become vertex zero. */
    char* end = NULL;
    errno = 0;
    long parsed = strtol(text, &end, 10);
    if(errno != 0 || end == text || *end != '\0' || parsed < 1 ||
       parsed > INT32_MAX)
        return 0;
    *value = (int)parsed;
    return 1;
}

static void printResult(const char* name, int source, int target,
                        DijkstraResult result){
    /* Keep output for both heap implementations in the same readable format. */
    printf("Heap: %s\n", name);
    printf("Query: %d -> %d\n", source, target);
    if(result.reachable)
        printf("Shortest distance: %" PRId64 "\n", result.distance);
    else
        printf("Shortest distance: unreachable\n");
    printf("Settled vertices: %d\n", result.settledVertices);
    printf("Dijkstra time: %.6f seconds\n", result.elapsedSeconds);
}

int main(int argc, char** argv){
    /* Command-line form: dataset, heap selection, source ID, target ID. */
    if(argc != 5){
        fprintf(stderr,
                "Usage: %s <NY|BAY|COL|graph.gr> "
                "<binary|fib|both> <source> <target>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    int source;
    int target;
    if(!parseVertex(argv[3], &source) || !parseVertex(argv[4], &target)){
        fprintf(stderr, "Source and target must be positive integer IDs.\n");
        return EXIT_FAILURE;
    }

    char path[1024];
    if(!resolveRoadDataset(argv[1], path, sizeof(path))){
        fprintf(stderr, "Cannot resolve dataset '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }
    RoadGraph* graph = loadDIMACSGraph(path);
    if(graph == NULL)
        return EXIT_FAILURE;
    if(source > graph->vertexCount || target > graph->vertexCount){
        fprintf(stderr, "Vertex IDs must be within 1..%d.\n", graph->vertexCount);
        destroyRoadGraph(graph);
        return EXIT_FAILURE;
    }

    printf("Dataset: %s\n", path);
    printf("Vertices: %d, arcs: %d\n", graph->vertexCount,
           graph->loadedEdgeCount);

    /* Run the requested implementation(s), optionally checking both results. */
    int success = 1;
    if(strcmp(argv[2], "binary") == 0){
        printResult("binary min heap", source, target,
                    runDijkstra(graph, source, target,
                                DIJKSTRA_BINARY_HEAP));
    }else if(strcmp(argv[2], "fib") == 0){
        printResult("Fibonacci heap", source, target,
                    runDijkstra(graph, source, target,
                                DIJKSTRA_FIBONACCI_HEAP));
    }else if(strcmp(argv[2], "both") == 0){
        DijkstraResult binary = runDijkstra(graph, source, target,
                                             DIJKSTRA_BINARY_HEAP);
        DijkstraResult fibonacci = runDijkstra(graph, source, target,
                                                DIJKSTRA_FIBONACCI_HEAP);
        printResult("binary min heap", source, target, binary);
        printResult("Fibonacci heap", source, target, fibonacci);
        if(binary.reachable != fibonacci.reachable ||
           binary.distance != fibonacci.distance){
            fprintf(stderr, "ERROR: the two implementations disagree.\n");
            success = 0;
        }
    }else{
        fprintf(stderr, "Heap must be binary, fib, or both.\n");
        success = 0;
    }

    destroyRoadGraph(graph);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
