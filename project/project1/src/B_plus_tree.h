/*
 * File: B_plus_tree.h
 * Description: Implementation of an order-M B+ Tree (used with M = 4 in benchmarks)
 *              supporting leaf/internal node splitting, sibling key borrowing,
 *              node merging, and Graphviz visualization.
 */

#ifndef B_PLUS_TREE_H
#define B_PLUS_TREE_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <process.h>
#define BPT_ACCESS _access
#define BPT_MAKE_DIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <unistd.h>
#define BPT_ACCESS access
#define BPT_MAKE_DIR(path) mkdir((path), 0755)
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

/*
 * B+ Tree node structure.
 * All actual data keys reside in leaf nodes (isLeaf == 1) linked via 'next'.
 * Internal nodes (isLeaf == 0) store separator keys where keys[i] = min(children[i+1]).
 */
typedef struct BPlusTreeNode BPlusTreeNode;

struct BPlusTreeNode {
    int isLeaf;                 /* 1 if leaf node, 0 if internal routing node */
    int numKeys;                /* Number of keys currently stored in this node */
    int* keys;                  /* Dynamic array of keys (capacity = order) */
    BPlusTreeNode** children;   /* Dynamic array of child pointers (capacity = order + 1) */
    BPlusTreeNode* parent;      /* Pointer to the parent node */
    BPlusTreeNode* next;        /* Pointer to the next leaf in sorted order */
};

/*
 * Wrapper structure representing a B+ Tree of a given branching order M.
 */
typedef struct {
    int order;                  /* Maximum number of children per internal node (M = 4) */
    BPlusTreeNode* root;        /* Pointer to the root node */
} BPlusTree;

/*
 * Allocate and initialize a new B+ Tree node (leaf or internal).
 */
static BPlusTreeNode* createBPlusNode(const BPlusTree* tree, int isLeaf) {
    BPlusTreeNode* node = (BPlusTreeNode*)malloc(sizeof(*node));
    if(node == NULL) return NULL;

    /* Allocate space for up to M keys and M + 1 children to accommodate temporary overflow */
    node->keys = (int*)malloc((size_t)tree->order * sizeof(*node->keys));
    node->children = (BPlusTreeNode**)calloc((size_t)tree->order + 1,
                                             sizeof(*node->children));
    if(node->keys == NULL || node->children == NULL) {
        free(node->keys);
        free(node->children);
        free(node);
        return NULL;
    }

    node->isLeaf = isLeaf;
    node->numKeys = 0;
    node->parent = NULL;
    node->next = NULL;
    return node;
}

/*
 * Free a single B+ Tree node and its internal arrays.
 */
static void freeBPlusNode(BPlusTreeNode* node) {
    if(node == NULL) return;
    free(node->keys);
    free(node->children);
    free(node);
}

/*
 * Return the smallest key in the subtree rooted at 'node' by walking down children[0].
 */
static int firstBPlusKey(BPlusTreeNode* node) {
    while(node != NULL && !node->isLeaf) node = node->children[0];
    return node->keys[0];
}

/*
 * Recompute separator keys of an internal node so keys[i] equals min(children[i+1]).
 */
static void refreshBPlusNode(BPlusTreeNode* node) {
    if(node == NULL || node->isLeaf) return;
    for(int i = 0; i < node->numKeys; ++i)
        node->keys[i] = firstBPlusKey(node->children[i + 1]);
}

/*
 * Refresh separator keys for 'node' and all of its ancestors up to the root.
 */
static void refreshBPlusAncestors(BPlusTreeNode* node) {
    while(node != NULL) {
        refreshBPlusNode(node);
        node = node->parent;
    }
}

/*
 * Descend from the root to locate the leaf node where 'key' belongs.
 */
static BPlusTreeNode* findBPlusLeaf(const BPlusTree* tree, int key) {
    BPlusTreeNode* node = tree->root;
    while(node != NULL && !node->isLeaf) {
        int index = 0;
        /* Advance child index while key is >= current separator key */
        while(index < node->numKeys && key >= node->keys[index]) ++index;
        node = node->children[index];
    }
    return node;
}

/*
 * Find the index of 'child' in 'parent->children[]'.
 */
static int indexOfBPlusChild(BPlusTreeNode* parent, BPlusTreeNode* child) {
    for(int i = 0; i <= parent->numKeys; ++i)
        if(parent->children[i] == child) return i;
    return -1;
}

static int insertBPlusChild(BPlusTree* tree, BPlusTreeNode* left,
                            BPlusTreeNode* right);

/*
 * Split an overflowing internal node with M + 1 children into two internal nodes.
 */
static int splitBPlusInternal(BPlusTree* tree, BPlusTreeNode* node) {
    BPlusTreeNode* right = createBPlusNode(tree, 0);
    if(right == NULL) return -1;

    /* Divide children evenly between the left node and the new right sibling */
    int totalChildren = node->numKeys + 1;
    int leftChildren = (totalChildren + 1) / 2;
    int rightChildren = totalChildren - leftChildren;
    right->numKeys = rightChildren - 1;

    /* Move the upper half of child pointers into 'right' */
    for(int i = 0; i < rightChildren; ++i) {
        right->children[i] = node->children[leftChildren + i];
        right->children[i]->parent = right;
        node->children[leftChildren + i] = NULL;
    }
    node->numKeys = leftChildren - 1;
    refreshBPlusNode(node);
    refreshBPlusNode(right);
    /* Attach the new right sibling into the parent node */
    return insertBPlusChild(tree, node, right);
}

/*
 * Insert 'right' as the child immediately following 'left' in their shared parent.
 * Creates a new root if 'left' was the root, or splits the parent if it overflows.
 */
static int insertBPlusChild(BPlusTree* tree, BPlusTreeNode* left,
                            BPlusTreeNode* right) {
    BPlusTreeNode* parent = left->parent;
    /* If the root split, allocate a new root node above 'left' and 'right' */
    if(parent == NULL) {
        BPlusTreeNode* root = createBPlusNode(tree, 0);
        if(root == NULL) return -1;
        root->numKeys = 1;
        root->children[0] = left;
        root->children[1] = right;
        left->parent = root;
        right->parent = root;
        refreshBPlusNode(root);
        tree->root = root;
        return 1;
    }

    /* Shift existing children right to insert 'right' at index + 1 */
    int index = indexOfBPlusChild(parent, left);
    for(int i = parent->numKeys + 1; i > index + 1; --i)
        parent->children[i] = parent->children[i - 1];
    parent->children[index + 1] = right;
    right->parent = parent;
    ++parent->numKeys;
    refreshBPlusNode(parent);

    /* Propagate split upward if parent now has M + 1 children (numKeys == M) */
    if(parent->numKeys >= tree->order)
        return splitBPlusInternal(tree, parent);
    refreshBPlusAncestors(parent->parent);
    return 1;
}

/*
 * Split an overflowing leaf node containing M keys into two leaves of ceil(M/2) and floor(M/2) keys.
 */
static int splitBPlusLeaf(BPlusTree* tree, BPlusTreeNode* leaf) {
    BPlusTreeNode* right = createBPlusNode(tree, 1);
    if(right == NULL) return -1;

    /* Move the upper half of the keys into the new right leaf */
    int leftKeys = (leaf->numKeys + 1) / 2;
    right->numKeys = leaf->numKeys - leftKeys;
    for(int i = 0; i < right->numKeys; ++i)
        right->keys[i] = leaf->keys[leftKeys + i];
    leaf->numKeys = leftKeys;

    /* Maintain the singly linked list of leaf nodes */
    right->next = leaf->next;
    leaf->next = right;
    return insertBPlusChild(tree, leaf, right);
}

/*
 * Create an empty B+ Tree of the specified branching order (order >= 3).
 */
BPlusTree* createBPlusTree(int order) {
    if(order < 3) return NULL;
    BPlusTree* tree = (BPlusTree*)malloc(sizeof(*tree));
    if(tree == NULL) return NULL;
    tree->order = order;
    tree->root = NULL;
    return tree;
}

/*
 * Search for 'key' in the B+ Tree; returns 1 if found, 0 otherwise.
 */
int searchBPlus(const BPlusTree* tree, int key) {
    if(tree == NULL) return 0;
    BPlusTreeNode* leaf = findBPlusLeaf(tree, key);
    if(leaf == NULL) return 0;
    for(int i = 0; i < leaf->numKeys; ++i)
        if(leaf->keys[i] == key) return 1;
    return 0;
}

/*
 * Insert 'key' into the B+ Tree.
 * Splits the target leaf (and any overflowing ancestors) when the leaf reaches M keys.
 */
int insertBPlus(BPlusTree* tree, int key) {
    if(tree == NULL || tree->order < 3) return -1;
    /* If the tree is empty, create the initial root leaf node */
    if(tree->root == NULL) {
        tree->root = createBPlusNode(tree, 1);
        if(tree->root == NULL) return -1;
        tree->root->keys[0] = key;
        tree->root->numKeys = 1;
        return 1;
    }

    /* Locate the target leaf and find the sorted insertion position */
    BPlusTreeNode* leaf = findBPlusLeaf(tree, key);
    int index = 0;
    while(index < leaf->numKeys && leaf->keys[index] < key) ++index;
    if(index < leaf->numKeys && leaf->keys[index] == key) return 0; /* Duplicate key */

    /* Shift larger keys right and insert 'key' */
    for(int i = leaf->numKeys; i > index; --i)
        leaf->keys[i] = leaf->keys[i - 1];
    leaf->keys[index] = key;
    ++leaf->numKeys;

    /* Split leaf if it reached M keys; otherwise refresh ancestor separators */
    if(leaf->numKeys >= tree->order) return splitBPlusLeaf(tree, leaf);
    refreshBPlusAncestors(leaf->parent);
    return 1;
}

/*
 * Remove child pointer at 'index' from 'parent' and update its separator keys.
 */
static void removeBPlusChild(BPlusTreeNode* parent, int index) {
    for(int i = index; i < parent->numKeys; ++i)
        parent->children[i] = parent->children[i + 1];
    parent->children[parent->numKeys] = NULL;
    --parent->numKeys;
    refreshBPlusNode(parent);
}

/*
 * Restore minimum occupancy at an internal node after a child removal.
 * Borrows a child from an adjacent sibling if possible, or merges with a sibling.
 */
static void rebalanceBPlusInternal(BPlusTree* tree, BPlusTreeNode* node) {
    /* If root has only one child left, collapse the root level */
    if(node == tree->root) {
        if(node->numKeys == 0) {
            tree->root = node->children[0];
            if(tree->root != NULL) tree->root->parent = NULL;
            freeBPlusNode(node);
        } else {
            refreshBPlusNode(node);
        }
        return;
    }

    /* Check whether the internal node still satisfies minimum key count ceil(M/2) - 1 */
    int minKeys = (tree->order + 1) / 2 - 1;
    if(node->numKeys >= minKeys) {
        refreshBPlusAncestors(node);
        return;
    }

    BPlusTreeNode* parent = node->parent;
    int index = indexOfBPlusChild(parent, node);
    BPlusTreeNode* left = index > 0 ? parent->children[index - 1] : NULL;
    BPlusTreeNode* right = index < parent->numKeys ? parent->children[index + 1] : NULL;

    /* Case 1a: Borrow the rightmost child from the left sibling */
    if(left != NULL && left->numKeys > minKeys) {
        for(int i = node->numKeys + 1; i > 0; --i)
            node->children[i] = node->children[i - 1];
        node->children[0] = left->children[left->numKeys];
        node->children[0]->parent = node;
        left->children[left->numKeys] = NULL;
        --left->numKeys;
        ++node->numKeys;
        refreshBPlusNode(left);
        refreshBPlusAncestors(node);
        return;
    }

    /* Case 1b: Borrow the leftmost child from the right sibling */
    if(right != NULL && right->numKeys > minKeys) {
        node->children[node->numKeys + 1] = right->children[0];
        node->children[node->numKeys + 1]->parent = node;
        ++node->numKeys;
        for(int i = 0; i < right->numKeys; ++i)
            right->children[i] = right->children[i + 1];
        right->children[right->numKeys] = NULL;
        --right->numKeys;
        refreshBPlusNode(right);
        refreshBPlusAncestors(node);
        return;
    }

    /* Case 2: Merge with left sibling (or right sibling) and propagate upward */
    if(left != NULL) {
        int first = left->numKeys + 1;
        for(int i = 0; i <= node->numKeys; ++i) {
            left->children[first + i] = node->children[i];
            node->children[i]->parent = left;
        }
        left->numKeys += node->numKeys + 1;
        removeBPlusChild(parent, index);
        refreshBPlusNode(left);
        freeBPlusNode(node);
    } else {
        int first = node->numKeys + 1;
        for(int i = 0; i <= right->numKeys; ++i) {
            node->children[first + i] = right->children[i];
            right->children[i]->parent = node;
        }
        node->numKeys += right->numKeys + 1;
        removeBPlusChild(parent, index + 1);
        refreshBPlusNode(node);
        freeBPlusNode(right);
    }
    rebalanceBPlusInternal(tree, parent);
}

/*
 * Delete 'key' from the B+ Tree.
 * Handles underflow in leaf nodes (< floor(M/2) keys) by borrowing or merging.
 */
int deleteBPlus(BPlusTree* tree, int key) {
    if(tree == NULL || tree->root == NULL) return 0;
    BPlusTreeNode* leaf = findBPlusLeaf(tree, key);
    int position = 0;
    while(position < leaf->numKeys && leaf->keys[position] < key) ++position;
    if(position == leaf->numKeys || leaf->keys[position] != key) return 0; /* Key absent */

    /* Remove the key from the leaf by shifting subsequent keys left */
    for(int i = position; i + 1 < leaf->numKeys; ++i)
        leaf->keys[i] = leaf->keys[i + 1];
    --leaf->numKeys;

    /* If the root itself is a leaf, allow 0..M-1 keys and free if empty */
    if(leaf == tree->root) {
        if(leaf->numKeys == 0) {
            freeBPlusNode(leaf);
            tree->root = NULL;
        }
        return 1;
    }

    /* If the leaf still has at least floor(M/2) keys, only refresh ancestor separators */
    int minKeys = tree->order / 2;
    if(leaf->numKeys >= minKeys) {
        refreshBPlusAncestors(leaf->parent);
        return 1;
    }

    BPlusTreeNode* parent = leaf->parent;
    int index = indexOfBPlusChild(parent, leaf);
    BPlusTreeNode* left = index > 0 ? parent->children[index - 1] : NULL;
    BPlusTreeNode* right = index < parent->numKeys ? parent->children[index + 1] : NULL;

    /* Borrow a key from the left sibling leaf if it has more than minKeys */
    if(left != NULL && left->numKeys > minKeys) {
        for(int i = leaf->numKeys; i > 0; --i) leaf->keys[i] = leaf->keys[i - 1];
        leaf->keys[0] = left->keys[left->numKeys - 1];
        --left->numKeys;
        ++leaf->numKeys;
        refreshBPlusAncestors(parent);
        return 1;
    }

    /* Borrow a key from the right sibling leaf if it has more than minKeys */
    if(right != NULL && right->numKeys > minKeys) {
        leaf->keys[leaf->numKeys++] = right->keys[0];
        for(int i = 0; i + 1 < right->numKeys; ++i) right->keys[i] = right->keys[i + 1];
        --right->numKeys;
        refreshBPlusAncestors(parent);
        return 1;
    }

    /* Neither sibling can lend a key: merge 'leaf' with an adjacent sibling */
    if(left != NULL) {
        for(int i = 0; i < leaf->numKeys; ++i)
            left->keys[left->numKeys + i] = leaf->keys[i];
        left->numKeys += leaf->numKeys;
        left->next = leaf->next;
        removeBPlusChild(parent, index);
        freeBPlusNode(leaf);
    } else {
        for(int i = 0; i < right->numKeys; ++i)
            leaf->keys[leaf->numKeys + i] = right->keys[i];
        leaf->numKeys += right->numKeys;
        leaf->next = right->next;
        removeBPlusChild(parent, index + 1);
        freeBPlusNode(right);
    }
    /* Rebalance the parent internal node after removing the merged child pointer */
    rebalanceBPlusInternal(tree, parent);
    return 1;
}

/*
 * Recursively export B+ Tree nodes, child pointers, and leaf links in DOT format.
 */
static void exportBPlusDOT(BPlusTreeNode* node, FILE* fp) {
    if(node == NULL) return;
    fprintf(fp, "    n%p [label=\"", (void*)node);
    for(int i = 0; i < node->numKeys; ++i) {
        if(i != 0) fprintf(fp, " | ");
        fprintf(fp, "%d", node->keys[i]);
    }
    fprintf(fp, "\", fillcolor=\"%s\"];\n", node->isLeaf ? "palegreen" : "lightblue");

    if(!node->isLeaf) {
        for(int i = 0; i <= node->numKeys; ++i) {
            fprintf(fp, "    n%p -> n%p;\n", (void*)node, (void*)node->children[i]);
            exportBPlusDOT(node->children[i], fp);
        }
    }
    if(node->isLeaf && node->next != NULL)
        fprintf(fp, "    n%p -> n%p [style=dashed, color=darkgreen, constraint=false];\n",
                (void*)node, (void*)node->next);
}

/*
 * Export the B+ Tree to a DOT file and render it as a PNG image.
 */
void visualizeBPlus(const BPlusTree* tree) {
    if(tree == NULL) return;
    const char* dir = BPT_ACCESS("src", 0) == 0
                        ? "src/visualizations"
                        : "visualizations";
    char dotPath[512];
    char pngPath[512];

    if(BPT_MAKE_DIR(dir) != 0 && errno != EEXIST) {
        fprintf(stderr, "Failed to create output directory '%s': %s\n", dir, strerror(errno));
        return;
    }
    snprintf(dotPath, sizeof(dotPath), "%s/b_plus.dot", dir);
    snprintf(pngPath, sizeof(pngPath), "%s/b_plus.png", dir);

    FILE* fp = fopen(dotPath, "w");
    if(fp == NULL) {
        fprintf(stderr, "Failed to create '%s': %s\n", dotPath, strerror(errno));
        return;
    }
    fprintf(fp, "digraph B_PLUS_TREE {\n");
    fprintf(fp, "    graph [rankdir=TB];\n");
    fprintf(fp, "    node [shape=box, style=filled, margin=\"0.12,0.08\"];\n");
    exportBPlusDOT(tree->root, fp);
    fprintf(fp, "}\n");
    fclose(fp);

    if(treeRenderGraphviz(dotPath, pngPath) == 0)
        printf("B+ tree visualization saved to %s\n", pngPath);
    else
        fprintf(stderr, "Graphviz rendering failed. The DOT source was saved to %s\n", dotPath);
}

/*
 * Recursively free all nodes in the B+ Tree.
 */
static void destroyBPlusNodes(BPlusTreeNode* node) {
    if(node == NULL) return;
    if(!node->isLeaf)
        for(int i = 0; i <= node->numKeys; ++i) destroyBPlusNodes(node->children[i]);
    freeBPlusNode(node);
}

/*
 * Destroy the B+ Tree and release all allocated memory.
 */
void destroyBPlusTree(BPlusTree* tree) {
    if(tree == NULL) return;
    destroyBPlusNodes(tree->root);
    free(tree);
}

#endif
