#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <stddef.h>
#include <stdint.h>

/* A directed road stored in the graph's adjacency-list array. */
typedef struct {
    int to;
    int weight;
    int next;
} RoadEdge;

/* Graph metadata together with its compact adjacency-list representation. */
typedef struct {
    int vertexCount;
    int edgeCount;
    int loadedEdgeCount;
    int* head;
    RoadEdge* edges;
} RoadGraph;

/* Heap implementations supported by the Dijkstra runner. */
typedef enum {
    DIJKSTRA_BINARY_HEAP,
    DIJKSTRA_FIBONACCI_HEAP
} DijkstraHeapType;

/* Summary returned after one shortest-path query. */
typedef struct {
    int reachable;
    int64_t distance;
    int settledVertices;
    double elapsedSeconds;
} DijkstraResult;

/* Load a DIMACS shortest-path graph from disk. */
RoadGraph* loadDIMACSGraph(const char* path);
/* Release all memory owned by a graph. */
void destroyRoadGraph(RoadGraph* graph);

/* Run one source-to-target query with the selected priority queue. */
DijkstraResult runDijkstra(const RoadGraph* graph, int source, int target,
                           DijkstraHeapType heapType);

/* Accept either a .gr path or one of the aliases NY, BAY, and COL. */
int resolveRoadDataset(const char* input, char* output, size_t outputSize);

#endif
