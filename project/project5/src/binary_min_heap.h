/*
 * File: binary_min_heap.h
 * Description: Contiguous parallel-array Binary Min-Heap supporting 64-bit
 *              priority keys (`data[]`) paired with 32-bit item/vertex IDs
 *              (`vertices[]`), plus optional Graphviz DOT visualization.
 *
 * Structural Invariants:
 *   - 0-based complete binary tree stored in `0 .. size - 1`:
 *       parent(i) = (i - 1) / 2,  left(i) = 2*i + 1,  right(i) = 2*i + 2.
 *   - Min-heap order: `data[(i - 1) / 2] <= data[i]` for every `1 <= i < size`.
 *   - Capacity auto-doubles on overflow, though in Shopping with Coupons at
 *     most N active row heads ever coexist so pre-sizing to N avoids reallocs.
 */

#ifndef BINARY_MIN_HEAP_H
#define BINARY_MIN_HEAP_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

/* Cross-platform directory creation and file-existence macros for Graphviz output. */
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
 * @brief Invoke Graphviz `dot` to render a generated `.dot` file into a `.png`.
 * @param dotPath Input Graphviz DOT file path.
 * @param pngPath Target rendered PNG image path.
 * @return 0 on successful rendering, -1 otherwise.
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

/*
 * Parallel-array Binary Min-Heap:
 *   - `data`     : 64-bit net price keys (P_v - C_(idx_v)).
 *   - `vertices` : Associated 0-based item index v in [0, N - 1].
 *   - `size`     : Current number of active entries in the heap.
 *   - `capacity` : Allocated length of `data` and `vertices`.
 */
typedef struct {
    int64_t *data;
    int *vertices;
    int size;
    int capacity;
} BinaryMinHeap;

/*
 * @brief Allocate an empty BinaryMinHeap with initial array capacity `capacity`.
 * @param capacity Positive initial slot count (typically N).
 * @return Pointer to initialized heap, or NULL on invalid capacity or OOM.
 */
static inline BinaryMinHeap* createBinaryMinHeap(int capacity){
    if(capacity <= 0)
        return NULL;
    BinaryMinHeap* heap = (BinaryMinHeap*)malloc(sizeof(BinaryMinHeap));
    if(!heap)
        return NULL; /* Heap descriptor allocation failed. */
    heap->data = (int64_t*)malloc((size_t)capacity * sizeof(int64_t));
    heap->vertices = (int*)malloc((size_t)capacity * sizeof(int));
    if(!heap->data || !heap->vertices){ /* Backing array allocation failed. */
        free(heap->data);
        free(heap->vertices);
        free(heap);
        return NULL;
    }
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

/*
 * @brief Exchange both key and vertex payload between heap slots `a` and `b`.
 */
static inline void swapBinaryEntries(BinaryMinHeap* heap, int a, int b){
    int64_t key = heap->data[a];
    int vertex = heap->vertices[a];
    heap->data[a] = heap->data[b];
    heap->vertices[a] = heap->vertices[b];
    heap->data[b] = key;
    heap->vertices[b] = vertex;
}

/*
 * @brief Percolate slot `index` upward until `data[index] >= data[parent]`.
 *        Note: `>=` stops immediately upon hitting an equal-key ancestor,
 *        yielding O(1) early termination on heavy duplicate-key workloads.
 */
static inline void siftup(BinaryMinHeap* heap, int index){
    while(index > 0){
        int parent = (index - 1) / 2;
        if(heap->data[index] >= heap->data[parent])
            break;
        swapBinaryEntries(heap, index, parent);
        index = parent;
    }
}

/*
 * @brief Percolate slot `index` downward by swapping with the smaller child
 *        whenever a child has a strictly smaller key (`<`), restoring heap order
 *        in O(log size) comparisons.
 */
static inline void siftdown(BinaryMinHeap* heap, int index){
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

/*
 * @brief Insert a `(value, vertex)` frontier pair into `heap` in O(log size) time.
 *
 * Memory & Complexity Notes:
 *   - If `size == capacity`, both parallel arrays (`data` and `vertices`) are
 *     reallocated to `2 * capacity` to provide amortized O(1) growth.
 *   - In `solveShoppingCounted`, `capacity` is initialized to `N` and at most
 *     one frontier coupon per item resides in the heap, so reallocation never
 *     triggers during normal solver execution.
 *
 * @param heap   Target BinaryMinHeap instance (no-op if NULL).
 * @param value  64-bit discounted item price `P_v - C_(idx_v)`.
 * @param vertex 0-based item index `v` in `[0, N - 1]` (or `-1` if unused).
 */
static inline void insertBinaryMinHeapEntry(BinaryMinHeap* heap, int64_t value, int vertex){
    if (heap == NULL)
        return;
    /* Dynamic capacity guard for general-purpose reuse beyond fixed-N frontiers. */
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
    /* Place new entry at leaf `size` and percolate upward. */
    heap->data[heap->size] = value;
    heap->vertices[heap->size] = vertex;
    siftup(heap, heap->size);
    heap->size++;
}

/*
 * @brief Convenience wrapper inserting a key without an associated vertex ID.
 */
static inline void insert(BinaryMinHeap* heap, int64_t value){
    insertBinaryMinHeapEntry(heap, value, -1);
}

/*
 * @brief Peek at the minimum key at root slot 0 in O(1) time.
 * @return Minimum 64-bit key, or `INT64_MAX` sentinel if `heap` is NULL/empty.
 */
static inline int64_t getMin(const BinaryMinHeap* heap){
    if(heap == NULL || heap->size == 0)
        return INT64_MAX;
    return heap->data[0];
}

/*
 * @brief Remove the minimum root entry in O(log size) time and optionally write
 *        its key and vertex ID to `*value` and `*vertex`.
 *
 * Extraction Steps:
 *   1. Copy root key `data[0]` and item index `vertices[0]` to caller pointers.
 *   2. Decrement `size`; if elements remain, move leaf `size` into slot `0`.
 *   3. Invoke `siftdown(heap, 0)` to restore the min-heap order invariant.
 *
 * @param heap   Target BinaryMinHeap instance.
 * @param value  Optional output pointer receiving minimum 64-bit key.
 * @param vertex Optional output pointer receiving associated item index.
 * @return `true` if an element was extracted, `false` if `heap` was NULL/empty.
 */
static inline bool extractMinBinaryEntry(BinaryMinHeap* heap, int64_t* value, int* vertex){
    if(heap == NULL || heap->size == 0){
        return false;
    }
    if(value != NULL)
        *value = heap->data[0];
    if(vertex != NULL)
        *vertex = heap->vertices[0];
    /* Move last leaf into root slot 0 and percolate downward. */
    heap->size--;
    if(heap->size > 0){
        heap->data[0] = heap->data[heap->size];
        heap->vertices[0] = heap->vertices[heap->size];
        siftdown(heap, 0);
    }
    return true;
}

/*
 * @brief Discard the minimum element without capturing its key or vertex ID.
 */
static inline void extractMin(BinaryMinHeap* heap){
    (void)extractMinBinaryEntry(heap, NULL, NULL);
}

/*
 * @brief Test whether `heap` contains zero elements in O(1) time.
 */
static inline bool isEmpty(const BinaryMinHeap* heap){
    return heap == NULL || heap->size == 0;
}

/*
 * @brief Release both parallel backing arrays and the heap descriptor.
 */
static inline void destroy(BinaryMinHeap* heap){
    if(heap){
        free(heap->data);
        free(heap->vertices);
        free(heap);
    }
}

/*
 * @brief Write the active heap tree to stream `fp` in Graphviz DOT syntax,
 *        using array indices `n0 .. n{size-1}` as stable node identifiers.
 */
static inline void exportBinaryMinHeapDOT(const BinaryMinHeap* heap, FILE* fp) {
    if (heap == NULL || fp == NULL)
        return;

    /* Emit node labels showing both vertex index `v` and 64-bit key. */
    for (int i = 0; i < heap->size; ++i) {
        if(heap->vertices[i] >= 0)
            fprintf(fp, "    n%d [label=\"v%d\\n%lld\"];\n", i,
                    heap->vertices[i], (long long)heap->data[i]);
        else
            fprintf(fp, "    n%d [label=\"%lld\"];\n", i,
                    (long long)heap->data[i]);
    }

    /* Emit directed parent -> child edges in the implicit binary tree. */
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
 * @brief Export `heap` to `visualizations/binary_min_heap.dot` and invoke
 *        Graphviz `dot` to produce `binary_min_heap.png` for debugging.
 *
 * Output Path Convention:
 *   - Writes into `src/visualizations/` when invoked from the project root,
 *     or `visualizations/` when invoked directly inside `src/`.
 *   - Retains the `.dot` source file even if Graphviz `dot` is not installed.
 */
static inline void visualizeBinaryMinHeap(const BinaryMinHeap* heap) {
    /* Support launching from either the project root or `src/`. */
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

#endif /* BINARY_MIN_HEAP_H */
