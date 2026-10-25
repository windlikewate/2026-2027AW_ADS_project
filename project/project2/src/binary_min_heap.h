#ifndef BINARY_MIN_HEAP_H
#define BINARY_MIN_HEAP_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>


#include <errno.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <process.h>
#define ACCESS _access
#define MAKE_DIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <unistd.h>
#define ACCESS access
#define MAKE_DIR(path) mkdir((path), 0755)
#endif

#ifndef TREE_GRAPHVIZ_COMMAND
#define TREE_GRAPHVIZ_COMMAND
/*
 * Invoke Graphviz 'dot' to render a DOT file into a PNG diagram.
 */
static int treeRenderGraphviz(const char* dotPath, const char* pngPath) {
    const char* exe = getenv("GRAPHVIZ_DOT"); if (exe == NULL || exe[0] == '\0') exe = "dot";
#ifdef _WIN32
    const char* const argv[] = {exe, "-Tpng", dotPath, "-o", pngPath, NULL};
    return _spawnvp(_P_WAIT, exe, argv) == 0 ? 0 : -1;
#else
    char command[1200]; snprintf(command, sizeof(command), "\"%s\" -Tpng \"%s\" -o \"%s\"", exe, dotPath, pngPath); return system(command) == 0 ? 0 : -1;
#endif
}
#endif


typedef struct {
    int64_t *data;
    int *vertices;
    int size;
    int capacity;
} BinaryMinHeap; // using dynamic array to implement the binary min heap

BinaryMinHeap* createBinaryMinHeap(int capacity){
    if(capacity <= 0)
        return NULL;
    BinaryMinHeap* heap = (BinaryMinHeap*)malloc(sizeof(BinaryMinHeap));
    if(!heap)
        return NULL; //if allocate memory is failed
    heap->data = (int64_t*)malloc((size_t)capacity * sizeof(int64_t));
    heap->vertices = (int*)malloc((size_t)capacity * sizeof(int));
    if(!heap->data || !heap->vertices){ //if allocate memory for the arrays is failed
        free(heap->data);
        free(heap->vertices);
        free(heap);
        return NULL;
    }
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

static void swapBinaryEntries(BinaryMinHeap* heap, int a, int b){
    int64_t key = heap->data[a];
    int vertex = heap->vertices[a];
    heap->data[a] = heap->data[b];
    heap->vertices[a] = heap->vertices[b];
    heap->data[b] = key;
    heap->vertices[b] = vertex;
}

static void siftup(BinaryMinHeap* heap, int index){ // sift up the element at the given index to maintain the binary min heap property
    while(index > 0){
        int parent = (index - 1) / 2;
        if(heap->data[index] >= heap->data[parent])
            break;
        swapBinaryEntries(heap, index, parent);
        index = parent;
    }
}

static void siftdown(BinaryMinHeap* heap, int index){ // sift down the element at the given index to maintain the binary min heap property
    while(1){
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int smallest = index;
        if(left < heap->size && heap->data[left] < heap->data[smallest])
            smallest = left;
        if(right < heap->size && heap->data[right] < heap->data[smallest])
            smallest = right;
        if(smallest == index)
            break;
        swapBinaryEntries(heap, index, smallest);
        index = smallest;
    }
}

static void insertBinaryMinHeapEntry(BinaryMinHeap* heap, int64_t value, int vertex){
    if (heap == NULL)
        return;
    if(heap->size == heap->capacity){
        int newCapacity = heap->capacity * 2;
        int64_t *newData = (int64_t*)malloc((size_t)newCapacity * sizeof(int64_t));
        int *newVertices = (int*)malloc((size_t)newCapacity * sizeof(int));
        if(!newData || !newVertices){
            free(newData);
            free(newVertices);
            return;
        }
        memcpy(newData, heap->data, (size_t)heap->size * sizeof(int64_t));
        memcpy(newVertices, heap->vertices, (size_t)heap->size * sizeof(int));
        free(heap->data);
        free(heap->vertices);
        heap->data = newData;
        heap->vertices = newVertices;
        heap->capacity = newCapacity;
    }
    heap->data[heap->size] = value;
    heap->vertices[heap->size] = vertex;
    siftup(heap, heap->size);
    heap->size++;
}

static void insert(BinaryMinHeap* heap, int64_t value){
    insertBinaryMinHeapEntry(heap, value, -1);
}

static int64_t getMin(const BinaryMinHeap* heap){ //get the minimum element from the binary min heap
    if(heap == NULL || heap->size == 0)
        return INT64_MAX; // if the heap is empty, return a large value
    return heap->data[0]; // the root element is the minimum
}

static bool extractMinBinaryEntry(BinaryMinHeap* heap, int64_t* value, int* vertex){
    if(heap == NULL || heap->size == 0){
        return false;
    }
    if(value != NULL)
        *value = heap->data[0];
    if(vertex != NULL)
        *vertex = heap->vertices[0];
    heap->size--;
    if(heap->size > 0){
        heap->data[0] = heap->data[heap->size];
        heap->vertices[0] = heap->vertices[heap->size];
        siftdown(heap, 0);
    }
    return true;
}

static void extractMin(BinaryMinHeap* heap){ // extract the minimum element from the binary min heap
    (void)extractMinBinaryEntry(heap, NULL, NULL);
}

static void destroy(BinaryMinHeap* heap){ // destroy the binary min heap
    if(heap){
        free(heap->data);
        free(heap->vertices);
        free(heap);
    }
}


/*
 * Export an array-backed binary heap in Graphviz DOT format.  Array indices
 * are stable, portable node identifiers; heap values are labels only.
 */
static void exportBinaryMinHeapDOT(const BinaryMinHeap* heap, FILE* fp) {
    if (heap == NULL || fp == NULL)
        return;

    for (int i = 0; i < heap->size; ++i) {
        if(heap->vertices[i] >= 0)
            fprintf(fp, "    n%d [label=\"v%d\\n%lld\"];\n", i,
                    heap->vertices[i], (long long)heap->data[i]);
        else
            fprintf(fp, "    n%d [label=\"%lld\"];\n", i,
                    (long long)heap->data[i]);
    }

    for (int parent = 0; parent < heap->size; ++parent) {
        int left = 2 * parent + 1;
        int right = left + 1;
        if (left < heap->size)
            fprintf(fp, "    n%d -> n%d;\n", parent, left);
        if (right < heap->size)
            fprintf(fp, "    n%d -> n%d;\n", parent, right);
    }
}

/*
 * Export the binary min heap to a DOT file and render it as a PNG image.
 */
static void visualizeBinaryMinHeap(const BinaryMinHeap* heap) {
    /* Support launching the testbench from either the project root or src */
    const char* dir = ACCESS("src", 0) == 0
                        ? "src/visualizations"
                        : "visualizations";
    char dotPath[512];
    char pngPath[512];

    if (MAKE_DIR(dir) != 0 && errno != EEXIST) {
        fprintf(stderr, "Failed to create output directory '%s': %s\n",
                dir, strerror(errno));
        return;
    }

    snprintf(dotPath, sizeof(dotPath), "%s/binary_min_heap.dot", dir);
    snprintf(pngPath, sizeof(pngPath), "%s/binary_min_heap.png", dir);

    FILE* fp = fopen(dotPath, "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to create '%s': %s\n",
                dotPath, strerror(errno));
        return;
    }

    fprintf(fp, "digraph BINARY_MIN_HEAP {\n");
    fprintf(fp, "    graph [rankdir=TB];\n");
    fprintf(fp, "    node [shape=circle, style=filled, fillcolor=\"lightblue\"];\n");

    exportBinaryMinHeapDOT(heap, fp);

    fprintf(fp, "}\n");
    fclose(fp);

    int ret = treeRenderGraphviz(dotPath, pngPath);

    if (ret == 0) {
        printf("Binary min heap visualization saved to %s\n", pngPath);
    } else {
        fprintf(stderr,
                "Graphviz rendering failed. Make sure Graphviz is installed "
                "and the 'dot' command is available in PATH.\n"
                "The DOT source was still saved to %s\n",
                dotPath);
    }
}

#endif // BINARY_MIN_HEAP_H
