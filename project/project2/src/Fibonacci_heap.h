# ifndef FIBONACCI_HEAP_H
# define FIBONACCI_HEAP_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <process.h>
#define FIB_ACCESS _access
#define FIB_MAKE_DIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <unistd.h>
#define FIB_ACCESS access
#define FIB_MAKE_DIR(path) mkdir((path), 0755)
#endif

typedef struct FibNode{
    int64_t key;
    int vertex;
    int degree;
    bool mark;
    struct FibNode* parent;
    struct FibNode* child;
    struct FibNode* left;
    struct FibNode* right;
} FibNode; // the node structure for the Fibonacci heap

typedef struct {
    FibNode* min;
    int n;
} FibonacciHeap; // the structure for the Fibonacci heap

FibonacciHeap* createFibonacciHeap(){
    FibonacciHeap* heap = (FibonacciHeap*)malloc(sizeof(FibonacciHeap));
    if(!heap)
        return NULL;
    heap->min = NULL;
    heap->n = 0;
    return heap;
} // create a new Fibonacci heap

FibNode *createFibNode(int64_t key){
    FibNode* node = (FibNode*)malloc(sizeof(FibNode));
    if(!node)
        return NULL;
    node->key = key;
    node->vertex = -1;
    node->degree = 0;
    node->mark = false;
    node->parent = NULL;
    node->child = NULL;
    node->left = node;
    node->right = node;
    return node;
} // create a new Fibonacci node

void removeFromList(FibNode* node){
    if(node == NULL)
        return;
    node->left->right = node->right;
    node->right->left = node->left;
    node->left = node;
    node->right = node;
} // remove a node from its list (either root list or child list)

void insertBefore(FibNode *pos , FibNode *node){
    if(pos == NULL || node == NULL)
        return;
    node->left = pos->left;
    node->right = pos;
    pos->left->right = node;
    pos->left = node;
} // insert a node before a specified position in the list

static void addFibRoot(FibonacciHeap* heap, FibNode* node){
    node->parent = NULL;
    node->mark = false;
    if(heap->min == NULL){
        node->left = node;
        node->right = node;
        heap->min = node;
    }else{
        insertBefore(heap->min, node);
        if(node->key < heap->min->key)
            heap->min = node;
    }
}

/* Insert a key and return its node handle for decrease-key operations. */
FibNode* insertFibonacciWithValue(FibonacciHeap* heap, int64_t key, int vertex){
    if(heap == NULL)
        return NULL;
    FibNode* node = createFibNode(key);
    if(node == NULL)
        return NULL;
    node->vertex = vertex;
    addFibRoot(heap, node);
    heap->n++;
    return node;
}

FibNode* insertFibonacci(FibonacciHeap* heap, int64_t key){
    return insertFibonacciWithValue(heap, key, -1);
}

int64_t getMinFibonacci(const FibonacciHeap* heap){
    return (heap == NULL || heap->min == NULL) ? INT64_MAX : heap->min->key;
}

/* Move all nodes from source into destination in O(1). */
void meldFibonacci(FibonacciHeap* destination, FibonacciHeap* source){
    if(destination == NULL || source == NULL || destination == source ||
       source->min == NULL)
        return;

    if(destination->min == NULL){
        destination->min = source->min;
        destination->n = source->n;
    }else{
        FibNode* aRight = destination->min->right;
        FibNode* bLeft = source->min->left;
        destination->min->right = source->min;
        source->min->left = destination->min;
        bLeft->right = aRight;
        aRight->left = bLeft;
        if(source->min->key < destination->min->key)
            destination->min = source->min;
        destination->n += source->n;
    }
    source->min = NULL;
    source->n = 0;
}

static void linkFibTrees(FibNode* child, FibNode* parent){
    removeFromList(child);
    child->parent = parent;
    child->mark = false;
    if(parent->child == NULL){
        parent->child = child;
    }else{
        insertBefore(parent->child, child);
    }
    parent->degree++;
}

static void consolidateFibonacci(FibonacciHeap* heap){
    if(heap == NULL || heap->min == NULL)
        return;

    int rootCount = 0;
    FibNode* current = heap->min;
    do{
        rootCount++;
        current = current->right;
    }while(current != heap->min);

    FibNode** roots = (FibNode**)malloc((size_t)rootCount * sizeof(FibNode*));
    FibNode** degrees = (FibNode**)calloc((size_t)heap->n + 1, sizeof(FibNode*));
    if(roots == NULL || degrees == NULL){
        free(roots);
        free(degrees);
        /* The heap remains valid but unconsolidated; restore the true min. */
        FibNode* smallest = heap->min;
        current = heap->min->right;
        while(current != heap->min){
            if(current->key < smallest->key)
                smallest = current;
            current = current->right;
        }
        heap->min = smallest;
        return;
    }

    current = heap->min;
    for(int i = 0; i < rootCount; ++i){
        roots[i] = current;
        current = current->right;
    }

    for(int i = 0; i < rootCount; ++i){
        FibNode* x = roots[i];
        if(x->parent != NULL)
            continue; /* It was linked under an earlier root. */
        int degree = x->degree;
        while(degrees[degree] != NULL){
            FibNode* y = degrees[degree];
            if(x->key > y->key){
                FibNode* temporary = x;
                x = y;
                y = temporary;
            }
            linkFibTrees(y, x);
            degrees[degree] = NULL;
            degree++;
        }
        degrees[degree] = x;
    }

    heap->min = NULL;
    for(int degree = 0; degree <= heap->n; ++degree){
        if(degrees[degree] != NULL){
            degrees[degree]->left = degrees[degree];
            degrees[degree]->right = degrees[degree];
            addFibRoot(heap, degrees[degree]);
        }
    }

    free(roots);
    free(degrees);
}

/* Remove and return the minimum node. The caller owns the returned node. */
FibNode* extractMinFibonacci(FibonacciHeap* heap){
    if(heap == NULL || heap->min == NULL)
        return NULL;

    FibNode* minimum = heap->min;
    while(minimum->child != NULL){
        FibNode* child = minimum->child;
        if(child->right == child)
            minimum->child = NULL;
        else
            minimum->child = child->right;
        removeFromList(child);
        minimum->degree--;
        addFibRoot(heap, child);
    }

    if(minimum->right == minimum){
        heap->min = NULL;
    }else{
        FibNode* nextRoot = minimum->right;
        removeFromList(minimum);
        heap->min = nextRoot;
    }
    heap->n--;
    if(heap->min != NULL)
        consolidateFibonacci(heap);

    minimum->parent = NULL;
    minimum->child = NULL;
    minimum->degree = 0;
    minimum->mark = false;
    minimum->left = minimum;
    minimum->right = minimum;
    return minimum;
}

static void cutFibNode(FibonacciHeap* heap, FibNode* node, FibNode* parent){
    if(parent->child == node){
        parent->child = (node->right == node) ? NULL : node->right;
    }
    removeFromList(node);
    parent->degree--;
    addFibRoot(heap, node);
}

static void cascadingCutFibonacci(FibonacciHeap* heap, FibNode* node){
    FibNode* parent = node->parent;
    if(parent == NULL)
        return;
    if(!node->mark){
        node->mark = true;
    }else{
        cutFibNode(heap, node, parent);
        cascadingCutFibonacci(heap, parent);
    }
}

/* Return false if the handle/key is invalid; otherwise decrease in O(1) amortized. */
bool decreaseKeyFibonacci(FibonacciHeap* heap, FibNode* node, int64_t newKey){
    if(heap == NULL || node == NULL || newKey > node->key)
        return false;
    node->key = newKey;
    FibNode* parent = node->parent;
    if(parent != NULL && node->key < parent->key){
        cutFibNode(heap, node, parent);
        cascadingCutFibonacci(heap, parent);
    }
    if(heap->min == NULL || node->key < heap->min->key)
        heap->min = node;
    return true;
}

bool deleteFibonacci(FibonacciHeap* heap, FibNode* node){
    if(heap == NULL || node == NULL)
        return false;
    FibNode* parent = node->parent;
    node->key = INT64_MIN;
    if(parent != NULL){
        cutFibNode(heap, node, parent);
        cascadingCutFibonacci(heap, parent);
    }
    /* Select this exact handle even if another root also has INT_MIN. */
    heap->min = node;
    FibNode* removed = extractMinFibonacci(heap);
    free(removed);
    return true;
}

static void destroyFibList(FibNode* start){
    if(start == NULL)
        return;
    FibNode* current = start;
    do{
        FibNode* next = current->right;
        destroyFibList(current->child);
        free(current);
        current = next;
    }while(current != start);
}

void destroyFibonacciHeap(FibonacciHeap* heap){
    if(heap == NULL)
        return;
    destroyFibList(heap->min);
    free(heap);
}

static void exportFibListDOT(const FibNode* start, const FibNode* minimum,
                             FILE* fp){
    if(start == NULL)
        return;
    const FibNode* node = start;
    do{
        const char* color = node == minimum ? "gold" :
                            (node->mark ? "lightcoral" : "lightblue");
        if(node->vertex >= 0){
            fprintf(fp,
                    "    n%p [label=\"v%d\\n%lld\\ndegree=%d\", "
                    "fillcolor=\"%s\"%s];\n",
                    (const void*)node, node->vertex, (long long)node->key,
                    node->degree, color,
                    node == minimum ? ", peripheries=2" : "");
        }else{
            fprintf(fp,
                    "    n%p [label=\"%lld\\ndegree=%d\", "
                    "fillcolor=\"%s\"%s];\n",
                    (const void*)node, (long long)node->key, node->degree,
                    color, node == minimum ? ", peripheries=2" : "");
        }

        if(node->child != NULL){
            const FibNode* child = node->child;
            do{
                fprintf(fp, "    n%p -> n%p;\n",
                        (const void*)node, (const void*)child);
                child = child->right;
            }while(child != node->child);
            exportFibListDOT(node->child, minimum, fp);
        }
        node = node->right;
    }while(node != start);
}

void exportFibonacciDOT(const FibonacciHeap* heap, FILE* fp){
    if(heap == NULL || fp == NULL || heap->min == NULL)
        return;

    exportFibListDOT(heap->min, heap->min, fp);

    /* Dashed, non-constraining arrows make the circular root list visible. */
    const FibNode* root = heap->min;
    fprintf(fp, "    { rank=same;");
    do{
        fprintf(fp, " n%p;", (const void*)root);
        root = root->right;
    }while(root != heap->min);
    fprintf(fp, " }\n");
    root = heap->min;
    do{
        fprintf(fp, "    n%p -> n%p [style=dashed, color=gray, "
                    "constraint=false];\n",
                (const void*)root, (const void*)root->right);
        root = root->right;
    }while(root != heap->min);
}

static int renderFibonacciGraphviz(const char* dotPath, const char* pngPath){
    const char* executable = getenv("GRAPHVIZ_DOT");
    if(executable == NULL || executable[0] == '\0')
        executable = "dot";
#ifdef _WIN32
    const char* const arguments[] = {
        executable, "-Tpng", dotPath, "-o", pngPath, NULL
    };
    return _spawnvp(_P_WAIT, executable, arguments) == 0 ? 0 : -1;
#else
    char command[1200];
    snprintf(command, sizeof(command), "\"%s\" -Tpng \"%s\" -o \"%s\"",
             executable, dotPath, pngPath);
    return system(command) == 0 ? 0 : -1;
#endif
}

void visualizeFibonacciHeap(const FibonacciHeap* heap){
    const char* directory = FIB_ACCESS("src", 0) == 0
                                ? "src/visualizations"
                                : "visualizations";
    char dotPath[512];
    char pngPath[512];

    if(FIB_MAKE_DIR(directory) != 0 && errno != EEXIST){
        fprintf(stderr, "Failed to create output directory '%s': %s\n",
                directory, strerror(errno));
        return;
    }
    snprintf(dotPath, sizeof(dotPath), "%s/fibonacci_heap.dot", directory);
    snprintf(pngPath, sizeof(pngPath), "%s/fibonacci_heap.png", directory);

    FILE* fp = fopen(dotPath, "w");
    if(fp == NULL){
        fprintf(stderr, "Failed to create '%s': %s\n", dotPath, strerror(errno));
        return;
    }
    fprintf(fp, "digraph FIBONACCI_HEAP {\n");
    fprintf(fp, "    graph [rankdir=TB];\n");
    fprintf(fp, "    node [shape=circle, style=filled];\n");
    exportFibonacciDOT(heap, fp);
    fprintf(fp, "}\n");
    fclose(fp);

    if(renderFibonacciGraphviz(dotPath, pngPath) == 0){
        printf("Fibonacci heap visualization saved to %s\n", pngPath);
    }else{
        fprintf(stderr, "Graphviz rendering failed; DOT source saved to %s\n",
                dotPath);
    }
}

#endif // FIBONACCI_HEAP_H
