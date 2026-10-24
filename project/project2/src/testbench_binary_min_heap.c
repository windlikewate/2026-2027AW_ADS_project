/*
 * Functional and visualization testbench for binary_min_heap.h.
 * Compile: gcc -std=c11 -Wall -Wextra -pedantic testbench_binary_min_heap.c -o testbench_binary_min_heap
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "binary_min_heap.h"

static int obeysMinHeapProperty(const BinaryMinHeap* heap) {
    /* Every parent must be no larger than either of its children. */
    for (int child = 1; child < heap->size; ++child) {
        int parent = (child - 1) / 2;
        if (heap->data[parent] > heap->data[child])
            return 0;
    }
    return 1;
}

static void verifyDOT(const char* path, int expectedNodes, int expectedEdges) {
    /* Count Graphviz nodes and edges to verify the visualization output. */
    FILE* fp = fopen(path, "r");
    assert(fp != NULL);

    int nodes = 0;
    int edges = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "[label=\"") != NULL)
            ++nodes;
        if (strstr(line, " -> ") != NULL)
            ++edges;
    }
    fclose(fp);

    assert(nodes == expectedNodes);
    assert(edges == expectedEdges);
}

int main(void) {
    /* Use unsorted, duplicate-free values so extraction order is observable. */
    const int64_t input[] = {75, 30, 925, -20, 45, 125, 80, 0};
    const int64_t sorted[] = {-20, 0, 30, 45, 75, 80, 125, 925};
    const int count = (int)(sizeof(input) / sizeof(input[0]));

    /* Check defensive behavior for invalid and NULL heap operations. */
    assert(createBinaryMinHeap(0) == NULL);
    assert(getMin(NULL) == INT64_MAX);
    insert(NULL, 1.0);
    extractMin(NULL);

    /* Start with a small capacity so insertion also exercises growth. */
    BinaryMinHeap* heap = createBinaryMinHeap(2);
    assert(heap != NULL);
    assert(getMin(heap) == INT64_MAX);

    /* Insertion must preserve the heap invariant after every operation. */
    for (int i = 0; i < count; ++i) {
        insert(heap, input[i]);
        assert(heap->size == i + 1);
        assert(obeysMinHeapProperty(heap));
    }
    assert(heap->capacity >= count); /* Exercises dynamic growth. */

    /* Verify that the exported DOT file describes the complete heap tree. */
    visualizeBinaryMinHeap(heap);
    const char* dotPath = ACCESS("src", 0) == 0
                            ? "src/visualizations/binary_min_heap.dot"
                            : "visualizations/binary_min_heap.dot";
    verifyDOT(dotPath, count, count - 1);

    /* Repeated extraction must produce values in nondecreasing order. */
    for (int i = 0; i < count; ++i) {
        assert(getMin(heap) == sorted[i]);
        extractMin(heap);
        assert(obeysMinHeapProperty(heap));
    }
    assert(heap->size == 0);
    assert(getMin(heap) == INT64_MAX);
    extractMin(heap); /* Extracting from an empty heap is a no-op. */

    destroy(heap);
    destroy(NULL);
    puts("All binary min heap tests passed.");
    return EXIT_SUCCESS;
}
