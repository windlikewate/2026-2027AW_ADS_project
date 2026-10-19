/*
 * File: splay_tree.h
 * Description: Implementation of a self-adjusting Splay Tree using
 *              zig, zig-zig, and zig-zag rotations, split-based insertion,
 *              and join-based deletion.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

/*
 * Splay Tree node structure.
 * Requires no explicit height or color metadata; amortized O(log N) balance
 * is maintained dynamically by splaying accessed nodes to the root.
 */
typedef struct SplayTreeNode SplayTreeNode;

struct SplayTreeNode{
    int key;                /* Integer key stored in the node */
    SplayTreeNode* left;    /* Pointer to the left subtree */
    SplayTreeNode* right;   /* Pointer to the right subtree */
};

/*
 * Single right rotation (zig on left child): lifts root->left to root.
 */
SplayTreeNode* singleRotateLL(SplayTreeNode* root){
    SplayTreeNode* newRoot = root->left;
    root->left = newRoot->right;
    newRoot->right = root;
    return newRoot;
}

/*
 * Single left rotation (zag on right child): lifts root->right to root.
 */
SplayTreeNode* singleRotateRR(SplayTreeNode* root){
    SplayTreeNode* newRoot = root->right;
    root->right = newRoot->left;
    newRoot->left = root;
    return newRoot;
}

/*
 * Search for 'key' and splay the matching node (or last visited node) to the root.
 * Applies two-level zig-zig and zig-zag rotations along the recursion path.
 */
SplayTreeNode* findSplay(SplayTreeNode* root, int key){
    if(!root || root->key == key) return root;

    /* Target key lies in the left subtree */
    if(key < root->key){
        if(root->left == NULL) return root; /* Key is not in the tree; splay last visited */

        if(key < root->left->key){
            /* Zig-Zig (Left-Left): splay in left-left grandchild, then rotate grandparent first */
            root->left->left = findSplay(root->left->left, key);
            root = singleRotateLL(root);
        } else if(key > root->left->key){
            /* Zig-Zag (Left-Right): splay in left-right grandchild, then rotate parent first */
            root->left->right = findSplay(root->left->right, key);
            if(root->left->right != NULL)
                root->left = singleRotateRR(root->left);
        }

        /* Perform the second rotation (or single Zig at root) */
        return root->left == NULL ? root : singleRotateLL(root);
    } else {
        /* Target key lies in the right subtree */
        if(root->right == NULL) return root; /* Key is not in the tree; splay last visited */

        if(key > root->right->key){
            /* Zag-Zag (Right-Right): splay in right-right grandchild, then rotate grandparent first */
            root->right->right = findSplay(root->right->right, key);
            root = singleRotateRR(root);
        } else if(key < root->right->key){
            /* Zag-Zig (Right-Left): splay in right-left grandchild, then rotate parent first */
            root->right->left = findSplay(root->right->left, key);
            if(root->right->left != NULL)
                root->right = singleRotateLL(root->right);
        }

        /* Perform the second rotation (or single Zag at root) */
        return root->right == NULL ? root : singleRotateRR(root);
    }
}

/*
 * Insert 'key' into the Splay Tree via splay-and-split.
 * Splays the closest existing key to the root, then splits the tree around the new root.
 */
SplayTreeNode* insertSplay(SplayTreeNode* root, int key){
    /* If the tree is empty, create and return a single-node tree */
    if(!root){
        root = (SplayTreeNode*)malloc(sizeof(SplayTreeNode));
        if(root == NULL) return NULL;
        root->key = key;
        root->left = NULL;
        root->right = NULL;
        return root;
    }

    /* Splay the key (or the closest neighbor) to the root */
    root = findSplay(root, key);
    if(root->key == key) return root; /* Key already exists; ignore duplicate */

    /* Allocate the new root node and split the old root into left and right subtrees */
    SplayTreeNode* newRoot = (SplayTreeNode*)malloc(sizeof(SplayTreeNode));
    if(newRoot == NULL) return root;
    newRoot->key = key;

    if(key < root->key){
        /* New key is smaller than splayed root: old root becomes right child */
        newRoot->left = root->left;
        newRoot->right = root;
        root->left = NULL;
    } else {
        /* New key is larger than splayed root: old root becomes left child */
        newRoot->right = root->right;
        newRoot->left = root;
        root->right = NULL;
    }
    return newRoot;
}

/*
 * Delete 'key' from the Splay Tree via splay-and-join.
 * Splays target key to the root, removes it, and joins its left and right subtrees.
 */
SplayTreeNode* deleteSplay(SplayTreeNode* root, int key){
    if(!root) return NULL;

    /* Splay the target key to the root; if absent, return the splayed tree */
    root = findSplay(root, key);
    if(root->key != key) return root;

    SplayTreeNode* newRoot;
    if(root->left == NULL){
        /* No left subtree: promote right subtree directly */
        newRoot = root->right;
    } else {
        /* Splay the maximum element of the left subtree (all keys < key) to its root */
        newRoot = findSplay(root->left, key);
        /* Attach the original right subtree to the new root's empty right link */
        newRoot->right = root->right;
    }
    free(root);
    return newRoot;
}

/*
 * Recursively export Splay Tree edges in Graphviz DOT format.
 */
void exportSPLAYDOT(SplayTreeNode* root, FILE* fp) {
    if (root == NULL) return;

    if (root->left != NULL) {
        fprintf(fp, "    %d -> %d;\n",
                root->key, root->left->key);
    }

    if (root->right != NULL) {
        fprintf(fp, "    %d -> %d;\n",
                root->key, root->right->key);
    }

    exportSPLAYDOT(root->left, fp);
    exportSPLAYDOT(root->right, fp);
}

/*
 * Export the Splay Tree to a DOT file and render it as a PNG image.
 */
void visualizeSPLAY(SplayTreeNode* root) {
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

    snprintf(dotPath, sizeof(dotPath), "%s/splay.dot", dir);
    snprintf(pngPath, sizeof(pngPath), "%s/splay.png", dir);

    FILE* fp = fopen(dotPath, "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to create '%s': %s\n",
                dotPath, strerror(errno));
        return;
    }

    fprintf(fp, "digraph SPLAY {\n");
    fprintf(fp, "    node [shape=circle];\n");
    fprintf(fp, "    rankdir=TB;\n");

    exportSPLAYDOT(root, fp);

    fprintf(fp, "}\n");
    fclose(fp);

    int ret = treeRenderGraphviz(dotPath, pngPath);

    if (ret == 0) {
        printf("SPLAY visualization saved to %s\n", pngPath);
    } else {
        fprintf(stderr,
                "Graphviz rendering failed. Make sure Graphviz is installed "
                "and the 'dot' command is available in PATH.\n"
                "The DOT source was still saved to %s\n",
                dotPath);
    }
}
