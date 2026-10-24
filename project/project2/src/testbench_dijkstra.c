#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "dijkstra.h"

int main(void){
    /* Build a small graph whose known shortest paths exercise both heaps. */
    const char* path = "test_dijkstra_graph.gr";
    FILE* fp = fopen(path, "w");
    assert(fp != NULL);
    fprintf(fp, "c small directed test graph\n");
    fprintf(fp, "p sp 7 10\n");
    fprintf(fp, "a 1 2 7\n");
    fprintf(fp, "a 1 3 9\n");
    fprintf(fp, "a 1 6 14\n");
    fprintf(fp, "a 2 3 10\n");
    fprintf(fp, "a 2 4 15\n");
    fprintf(fp, "a 3 4 11\n");
    fprintf(fp, "a 3 6 2\n");
    fprintf(fp, "a 6 5 9\n");
    fprintf(fp, "a 4 5 6\n");
    fprintf(fp, "a 5 1 20\n");
    fclose(fp);

    /* Reload the generated DIMACS file through the production parser. */
    RoadGraph* graph = loadDIMACSGraph(path);
    assert(graph != NULL);
    assert(graph->vertexCount == 7);
    assert(graph->loadedEdgeCount == 10);

    /* Both implementations must agree on the reachable shortest path. */
    DijkstraResult binary = runDijkstra(graph, 1, 5,
                                         DIJKSTRA_BINARY_HEAP);
    DijkstraResult fibonacci = runDijkstra(graph, 1, 5,
                                            DIJKSTRA_FIBONACCI_HEAP);
    assert(binary.reachable && fibonacci.reachable);
    assert(binary.distance == 20);
    assert(fibonacci.distance == 20);
    assert(binary.distance == fibonacci.distance);

    /* Vertex 7 has no incoming path, so this query must be unreachable. */
    binary = runDijkstra(graph, 1, 7, DIJKSTRA_BINARY_HEAP);
    fibonacci = runDijkstra(graph, 1, 7, DIJKSTRA_FIBONACCI_HEAP);
    assert(!binary.reachable && !fibonacci.reachable);

    /* A vertex always has a zero-length path to itself. */
    binary = runDijkstra(graph, 4, 4, DIJKSTRA_BINARY_HEAP);
    fibonacci = runDijkstra(graph, 4, 4, DIJKSTRA_FIBONACCI_HEAP);
    assert(binary.distance == 0 && fibonacci.distance == 0);

    destroyRoadGraph(graph);
    assert(remove(path) == 0);
    puts("All Dijkstra tests passed.");
    return EXIT_SUCCESS;
}
