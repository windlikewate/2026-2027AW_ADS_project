#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <stddef.h> // 提供标准类型和宏
#include <stdint.h> // 提供固定宽度的整数类型

/* A directed road stored in the graph's adjacency-list array. */
typedef struct {
    int to;
    int weight;
    int next;
} RoadEdge;

/* Graph metadata together with its compact adjacency-list representation. */
typedef struct {
    int vertexCount; //顶点数
    int edgeCount; //边数 
    int loadedEdgeCount; //已加载的边数
    int* head; //每个顶点的邻接表头
    RoadEdge* edges; //所有边的数组
} RoadGraph;

/* Heap implementations supported by the Dijkstra runner. */
typedef enum {
    DIJKSTRA_BINARY_HEAP,
    DIJKSTRA_FIBONACCI_HEAP
} DijkstraHeapType; //定义Type中所有可能的取值

/* Summary returned after one shortest-path query. */
typedef struct {
    int reachable; //目标顶点是否可达
    int64_t distance; //最短路径距离
    int settledVertices; //已确定的顶点数
    double elapsedSeconds; //查询耗时（秒）
} DijkstraResult; //一次最短路径查询的结果

/* Load a DIMACS shortest-path graph from disk. */
RoadGraph* loadDIMACSGraph(const char* path);
/* Release all memory owned by a graph. */
void destroyRoadGraph(RoadGraph* graph);

/* Run one source-to-target query with the selected priority queue. */
DijkstraResult runDijkstra(const RoadGraph* graph, int source, int target,
                           DijkstraHeapType heapType);

/* Accept either a .gr path or one of the aliases NY, BAY, and COL. 即通过符号来加载对应的图文件 */
int resolveRoadDataset(const char* input, char* output, size_t outputSize);

#endif
