/*
 * Functional and visualization testbench for Fibonacci_heap.h.
 * Compile: gcc -std=c11 -Wall -Wextra -Werror -pedantic testbench_fibonacci_heap.c -o testbench_fibonacci_heap
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Fibonacci_heap.h"

static int countList(const FibNode* start){
    /* Count one circular doubly linked list and detect cycles that never close. */
    if(start == NULL)
        return 0;
    int count = 0;
    const FibNode* node = start;
    do{
        count++;
        node = node->right;
        assert(count < 1000); /* Detect a broken circular list. */
    }while(node != start);
    return count;
}

static int validateList(const FibNode* start, const FibNode* parent){
    /* Recursively check links, parent pointers, heap order, and node degrees. */
    if(start == NULL)
        return 0;
    int total = 0;
    const FibNode* node = start;
    do{
        assert(node->left->right == node);
        assert(node->right->left == node);
        assert(node->parent == parent);
        if(parent != NULL)
            assert(parent->key <= node->key);
        assert(node->degree == countList(node->child));
        total += 1 + validateList(node->child, node);
        node = node->right;
        assert(total < 1000);
    }while(node != start);
    return total;
}

static void validateHeap(const FibonacciHeap* heap){
    /* Validate every root and child list, including the global minimum pointer. */
    assert(heap != NULL);
    assert(validateList(heap->min, NULL) == heap->n);
    if(heap->min == NULL){
        assert(heap->n == 0);
        return;
    }
    const FibNode* root = heap->min;
    do{
        assert(heap->min->key <= root->key);
        root = root->right;
    }while(root != heap->min);
}

static void verifyDOT(const char* path){
    /* Ensure the Graphviz output contains a heap graph, minimum styling, and an edge. */
    FILE* fp = fopen(path, "r");
    assert(fp != NULL);
    int hasGraph = 0;
    int hasMinimumStyle = 0;
    int hasEdge = 0;
    char line[512];
    while(fgets(line, sizeof(line), fp) != NULL){
        hasGraph |= strstr(line, "digraph FIBONACCI_HEAP") != NULL;
        hasMinimumStyle |= strstr(line, "peripheries=2") != NULL;
        hasEdge |= strstr(line, " -> ") != NULL;
    }
    fclose(fp);
    assert(hasGraph && hasMinimumStyle && hasEdge);
}

int main(void){
    /* This sequence forces consolidation and creates a nontrivial child list. */
    const int input[] = {7, 3, 18, 38, 24, 17, 23, 21, 39, 41, 52, 30, 26, 46};
    const int inputCount = (int)(sizeof(input) / sizeof(input[0]));
    FibNode* handles[14];

    /* Check safe behavior for operations on a NULL heap. */
    assert(getMinFibonacci(NULL) == INT64_MAX);
    assert(extractMinFibonacci(NULL) == NULL);

    FibonacciHeap* heap = createFibonacciHeap();
    FibonacciHeap* other = createFibonacciHeap();
    assert(heap != NULL && other != NULL);

    /* Keep handles so decrease-key and deletion can target exact nodes later. */
    for(int i = 0; i < inputCount; ++i){
        handles[i] = insertFibonacci(heap, input[i]);
        assert(handles[i] != NULL);
    }
    /* Meld must transfer all roots and leave the source heap empty. */
    assert(insertFibonacci(other, -4) != NULL);
    assert(insertFibonacci(other, 11) != NULL);
    meldFibonacci(heap, other);
    assert(other->n == 0 && other->min == NULL);
    assert(heap->n == inputCount + 2);
    assert(getMinFibonacci(heap) == -4);
    validateHeap(heap);

    /* Extracting the minimum triggers consolidation of the remaining roots. */
    FibNode* removed = extractMinFibonacci(heap);
    assert(removed != NULL && removed->key == -4);
    free(removed);
    validateHeap(heap);

    /* Consolidation creates children; decreasing one forces a cut. */
    FibNode* childHandle = NULL;
    for(int i = 0; i < inputCount; ++i){
        if(handles[i]->parent != NULL){
            childHandle = handles[i];
            break;
        }
    }
    assert(childHandle != NULL);
    assert(!decreaseKeyFibonacci(heap, childHandle, childHandle->key + 1));
    assert(decreaseKeyFibonacci(heap, childHandle, -10));
    assert(childHandle->parent == NULL);
    assert(getMinFibonacci(heap) == -10);
    validateHeap(heap);

    assert(deleteFibonacci(heap, handles[2]));
    validateHeap(heap);

    /* Check that the exported DOT graph contains the expected heap structure. */
    visualizeFibonacciHeap(heap);
    const char* dotPath = FIB_ACCESS("src", 0) == 0
                            ? "src/visualizations/fibonacci_heap.dot"
                            : "visualizations/fibonacci_heap.dot";
    verifyDOT(dotPath);

    /* Removing all nodes must return keys in sorted order. */
    int64_t previous = INT64_MIN;
    while(heap->n > 0){
        removed = extractMinFibonacci(heap);
        assert(removed != NULL);
        assert(previous <= removed->key);
        previous = removed->key;
        free(removed);
        validateHeap(heap);
    }
    assert(extractMinFibonacci(heap) == NULL);
    assert(getMinFibonacci(heap) == INT64_MAX);

    destroyFibonacciHeap(other);
    destroyFibonacciHeap(heap);
    destroyFibonacciHeap(NULL);
    puts("All Fibonacci heap tests passed.");
    return EXIT_SUCCESS;
}
