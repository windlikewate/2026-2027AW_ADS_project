#include "dijkstra.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#include "binary_min_heap.h"
#include "Fibonacci_heap.h"

static int fileExists(const char* path){
    /* Opening the file is portable and avoids depending on filesystem APIs. */
    FILE* fp = fopen(path, "r");
    if(fp == NULL)
        return 0;
    fclose(fp);
    return 1;
}

static int equalsIgnoreCase(const char* left, const char* right){
    /* Dataset aliases are accepted without making their case significant. */
    while(*left != '\0' && *right != '\0'){
        if(toupper((unsigned char)*left) != toupper((unsigned char)*right))
            return 0;
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
}

int resolveRoadDataset(const char* input, char* output, size_t outputSize){
    /* Resolve either a caller-supplied path or a known DIMACS dataset alias. */
    if(input == NULL || output == NULL || outputSize == 0)
        return 0;
    if(fileExists(input)){
        snprintf(output, outputSize, "%s", input);
        return 1;
    }

    const char* filename = NULL;
    if(equalsIgnoreCase(input, "NY")){
        filename = "USA-road-d.NY.gr";
    }else if(equalsIgnoreCase(input, "BAY")){
        filename = "USA-road-d.BAY.gr";
    }else if(equalsIgnoreCase(input, "COL")){
        filename = "USA-road-d.COL.gr";
    }else{
        return 0;
    }

    /* Search both project-root and src-relative locations for the downloaded data. */
    const char* prefixes[] = {
        "../data/DIMACS_USA_road_distance_NY_BAY_COL/"
        "DIMACS_USA_road_distance_NY_BAY_COL",
        "data/DIMACS_USA_road_distance_NY_BAY_COL/"
        "DIMACS_USA_road_distance_NY_BAY_COL"
    };
    for(size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i){
#ifdef _WIN32
        char pattern[1024];
        snprintf(pattern, sizeof(pattern), "%s/*", prefixes[i]);
        struct _finddata_t entry;
        intptr_t search = _findfirst(pattern, &entry);
        if(search != -1){
            do{
                if((entry.attrib & _A_SUBDIR) != 0 &&
                   strcmp(entry.name, ".") != 0 &&
                   strcmp(entry.name, "..") != 0){
                    snprintf(output, outputSize, "%s/%s/%s", prefixes[i],
                             entry.name, filename);
                    if(fileExists(output)){
                        _findclose(search);
                        return 1;
                    }
                }
            }while(_findnext(search, &entry) == 0);
            _findclose(search);
        }
#else
        DIR* directory = opendir(prefixes[i]);
        if(directory != NULL){
            struct dirent* entry;
            while((entry = readdir(directory)) != NULL){
                if(strcmp(entry->d_name, ".") == 0 ||
                   strcmp(entry->d_name, "..") == 0)
                    continue;
                snprintf(output, outputSize, "%s/%s/%s", prefixes[i],
                         entry->d_name, filename);
                if(fileExists(output)){
                    closedir(directory);
                    return 1;
                }
            }
            closedir(directory);
        }
#endif
    }
    output[0] = '\0';
    return 0;
}

RoadGraph* loadDIMACSGraph(const char* path){
    /* DIMACS files contain comments, one problem line, and directed arc lines. */
    if(path == NULL)
        return NULL;
    FILE* fp = fopen(path, "r");
    if(fp == NULL){
        fprintf(stderr, "Cannot open graph file: %s\n", path);
        return NULL;
    }

    RoadGraph* graph = NULL;
    char line[256];
    while(fgets(line, sizeof(line), fp) != NULL){
        if(line[0] == 'c' || line[0] == '\n' || line[0] == '\r')
            continue;
        if(line[0] == 'p'){
            int vertices = 0;
            int edges = 0;
            if(sscanf(line, "p sp %d %d", &vertices, &edges) != 2 ||
               vertices <= 0 || edges < 0){
                fprintf(stderr, "Invalid DIMACS problem line.\n");
                fclose(fp);
                return NULL;
            }
            graph = (RoadGraph*)calloc(1, sizeof(RoadGraph));
            if(graph == NULL){
                fclose(fp);
                return NULL;
            }
            graph->vertexCount = vertices;
            graph->edgeCount = edges;
            /* Vertex IDs are one-based, so index zero is kept as a sentinel. */
            graph->head = (int*)malloc((size_t)(vertices + 1) * sizeof(int));
            graph->edges = (RoadEdge*)malloc((size_t)edges * sizeof(RoadEdge));
            if(graph->head == NULL || (edges > 0 && graph->edges == NULL)){
                destroyRoadGraph(graph);
                fclose(fp);
                return NULL;
            }
            for(int i = 0; i <= vertices; ++i)
                graph->head[i] = -1;
        }else if(line[0] == 'a'){
            int from = 0;
            int to = 0;
            int weight = 0;
            if(graph == NULL ||
               sscanf(line, "a %d %d %d", &from, &to, &weight) != 3 ||
               from < 1 || from > graph->vertexCount ||
               to < 1 || to > graph->vertexCount || weight < 0 ||
               graph->loadedEdgeCount >= graph->edgeCount){
                fprintf(stderr, "Invalid DIMACS arc line: %s", line);
                destroyRoadGraph(graph);
                fclose(fp);
                return NULL;
            }
            /* Prepend each arc to its source vertex's adjacency list. */
            int index = graph->loadedEdgeCount++;
            graph->edges[index].to = to;
            graph->edges[index].weight = weight;
            graph->edges[index].next = graph->head[from];
            graph->head[from] = index;
        }
    }
    fclose(fp);

    if(graph == NULL || graph->loadedEdgeCount != graph->edgeCount){
        fprintf(stderr, "Graph edge count does not match its DIMACS header.\n");
        destroyRoadGraph(graph);
        return NULL;
    }
    return graph;
}

void destroyRoadGraph(RoadGraph* graph){
    /* free(NULL) is safe, which keeps cleanup simple on partial failures. */
    if(graph == NULL)
        return;
    free(graph->head);
    free(graph->edges);
    free(graph);
}

static int64_t* createDistanceArray(int vertexCount){
    /* INT64_MAX represents infinity for vertices not reached yet. */
    int64_t* distance = (int64_t*)malloc((size_t)(vertexCount + 1) *
                                         sizeof(int64_t));
    if(distance == NULL)
        return NULL;
    for(int i = 0; i <= vertexCount; ++i)
        distance[i] = INT64_MAX;
    return distance;
}

static DijkstraResult dijkstraBinary(const RoadGraph* graph, int source,
                                     int target){
    /* The binary heap uses lazy insertion instead of an explicit decrease-key. */
    DijkstraResult result = {0, INT64_MAX, 0, 0.0};
    int64_t* distance = createDistanceArray(graph->vertexCount);
    BinaryMinHeap* queue = createBinaryMinHeap(64);
    if(distance == NULL || queue == NULL){
        free(distance);
        destroy(queue);
        return result;
    }

    clock_t start = clock();
    distance[source] = 0;
    insertBinaryMinHeapEntry(queue, 0, source);
    while(queue->size > 0){
        int64_t currentDistance;
        int vertex;
        extractMinBinaryEntry(queue, &currentDistance, &vertex);
        if(currentDistance != distance[vertex])
            continue; /* Ignore an obsolete lazy-queue entry. */
        /* The first current entry removed for a vertex is its final distance. */
        result.settledVertices++;
        if(vertex == target)
            break;

        for(int edge = graph->head[vertex]; edge != -1;
            edge = graph->edges[edge].next){
            int next = graph->edges[edge].to;
            int weight = graph->edges[edge].weight;
            /* Relax every outgoing edge while guarding against integer overflow. */
            if(currentDistance <= INT64_MAX - weight &&
               currentDistance + weight < distance[next]){
                distance[next] = currentDistance + weight;
                insertBinaryMinHeapEntry(queue, distance[next], next);
            }
        }
    }
    result.elapsedSeconds = (double)(clock() - start) / CLOCKS_PER_SEC;
    result.distance = distance[target];
    result.reachable = result.distance != INT64_MAX;
    free(distance);
    destroy(queue);
    return result;
}

static DijkstraResult dijkstraFibonacci(const RoadGraph* graph, int source,
                                        int target){
    /* Handles allow the Fibonacci heap to update an existing queue entry. */
    DijkstraResult result = {0, INT64_MAX, 0, 0.0};
    int64_t* distance = createDistanceArray(graph->vertexCount);
    FibNode** handles = (FibNode**)calloc((size_t)graph->vertexCount + 1,
                                          sizeof(FibNode*));
    unsigned char* settled = (unsigned char*)calloc(
        (size_t)graph->vertexCount + 1, sizeof(unsigned char));
    FibonacciHeap* queue = createFibonacciHeap();
    if(distance == NULL || handles == NULL || settled == NULL || queue == NULL){
        free(distance);
        free(handles);
        free(settled);
        destroyFibonacciHeap(queue);
        return result;
    }

    clock_t start = clock();
    distance[source] = 0;
    handles[source] = insertFibonacciWithValue(queue, 0, source);
    while(queue->n > 0){
        FibNode* minimum = extractMinFibonacci(queue);
        if(minimum == NULL)
            break;
        int vertex = minimum->vertex;
        int64_t currentDistance = minimum->key;
        handles[vertex] = NULL;
        free(minimum);
        if(settled[vertex])
            continue;
        settled[vertex] = 1;
        result.settledVertices++;
        if(vertex == target)
            break;

        /* Relax outgoing edges and decrease keys for improved estimates. */
        for(int edge = graph->head[vertex]; edge != -1;
            edge = graph->edges[edge].next){
            int next = graph->edges[edge].to;
            int weight = graph->edges[edge].weight;
            if(settled[next] || currentDistance > INT64_MAX - weight)
                continue;
            int64_t candidate = currentDistance + weight;
            if(candidate < distance[next]){
                distance[next] = candidate;
                if(handles[next] == NULL){
                    handles[next] = insertFibonacciWithValue(queue, candidate,
                                                              next);
                }else{
                    decreaseKeyFibonacci(queue, handles[next], candidate);
                }
            }
        }
    }
    result.elapsedSeconds = (double)(clock() - start) / CLOCKS_PER_SEC;
    result.distance = distance[target];
    result.reachable = result.distance != INT64_MAX;
    free(distance);
    free(handles);
    free(settled);
    destroyFibonacciHeap(queue);
    return result;
}

DijkstraResult runDijkstra(const RoadGraph* graph, int source, int target,
                           DijkstraHeapType heapType){
    /* Invalid inputs return an unreachable result instead of dereferencing data. */
    DijkstraResult invalid = {0, INT64_MAX, 0, 0.0};
    if(graph == NULL || source < 1 || source > graph->vertexCount ||
       target < 1 || target > graph->vertexCount)
        return invalid;
    if(heapType == DIJKSTRA_FIBONACCI_HEAP)
        return dijkstraFibonacci(graph, source, target);
    return dijkstraBinary(graph, source, target);
}
