/*
 * File: binaray_search_tree.h
 * Description: Implementation of an unbalanced Binary Search Tree (BST)
 *              with iterative insertion, iterative deletion, and Graphviz
 *              DOT/PNG topology export utilities.
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
 * Render a Graphviz DOT file to a PNG image using the 'dot' executable.
 * Uses PATH lookup (or GRAPHVIZ_DOT environment variable if specified).
 */
static int treeRenderGraphviz(const char* dotPath, const char* pngPath) {
    const char* exe = getenv("GRAPHVIZ_DOT");
    if (exe == NULL || exe[0] == '\0') exe = "dot";
#ifdef _WIN32
    /* Use _spawnvp on Windows to pass arguments directly without cmd.exe quoting issues */
    const char* const argv[] = {exe, "-Tpng", dotPath, "-o", pngPath, NULL};
    return _spawnvp(_P_WAIT, exe, argv) == 0 ? 0 : -1;
#else
    char command[1200];
    snprintf(command, sizeof(command), "\"%s\" -Tpng \"%s\" -o \"%s\"",
             exe, dotPath, pngPath);
    return system(command) == 0 ? 0 : -1;
#endif
}
#endif

/*
 * Node structure for an unbalanced Binary Search Tree.
 * Stores an integer key and pointers to the left and right subtrees.
 */
typedef struct BSTreeNode BSTreeNode;
struct BSTreeNode{
    int val;            /* Integer key stored in the node */
    BSTreeNode* Left;   /* Pointer to left child (keys < val) */
    BSTreeNode* Right;  /* Pointer to right child (keys > val) */
};

/*
 * Recursive implementations kept for reference.
 * WARNING: An ordered input makes an ordinary BST degenerate into a chain.
 * These recursive functions may then create O(N) call frames and cause a
 * stack overflow when N is large (around 26000 with the default call stack).
 *
BSTreeNode* insertBST(BSTreeNode* root, int val) { // Insert a value into the binary search tree
    if (root == NULL) {
        root = (BSTreeNode*)malloc(sizeof(BSTreeNode)); // Allocate memory for the new node
        root->val = val;
        root->Left = NULL;
        root->Right = NULL;
        return root;
    }
    if (val < root->val) { // If the value is smaller, insert into the left subtree
        root->Left = insertBST(root->Left, val);
    } else if (val > root->val) { // If the value is larger, insert into the right subtree
        root->Right = insertBST(root->Right, val);
    } else {
        // Duplicate value, do nothing (BST does not allow duplicates)
    }
    return root;
}

BSTreeNode* deleteBST(BSTreeNode* root, int val) { // Delete a value from the binary search tree
    if (root == NULL) {
        return root; // Value not found
    }
    if (val < root->val) { // If the value is smaller, search in the left subtree
        root->Left = deleteBST(root->Left, val);
    } else if (val > root->val) { // If the value is larger, search in the right subtree
        root->Right = deleteBST(root->Right, val);
    } else { // Found the node to delete
        if (root->Left == NULL) { // Node has no left child
            BSTreeNode* temp = root->Right;
            free(root);
            root = temp;
            return root;
        } else if (root->Right == NULL) { // Node has no right child
            BSTreeNode* temp = root->Left;
            free(root);
            root = temp;
            return root;
        } else { // Node has two children, find the inorder successor (smallest node in the right subtree)
            BSTreeNode* temp = root->Right;
            while (temp->Left != NULL) {
                temp = temp->Left; // find the smallest node in the right subtree
            }
            root->val = temp->val;
            root->Right = deleteBST(root->Right, temp->val); // Delete the inorder successor from the right subtree
            return root;
        }
    }
    return root; // Return the updated root after deletion
}
*/

/*
 * Insert a key into the BST iteratively.
 * Uses O(1) auxiliary stack space to prevent stack overflow on skewed inputs.
 */
BSTreeNode* insertBST(BSTreeNode* root, int val) {
    BSTreeNode* parent = NULL;
    BSTreeNode* current = root;

    /* Walk down the tree from the root to locate the insertion point */
    while (current != NULL) {
        parent = current;
        if (val < current->val) {
            current = current->Left;    /* Move to left child */
        } else if (val > current->val) {
            current = current->Right;   /* Move to right child */
        } else {
            return root;                /* Duplicate key found; ignore insertion */
        }
    }

    /* Allocate and initialize the new leaf node */
    BSTreeNode* newNode = (BSTreeNode*)malloc(sizeof(BSTreeNode));
    if (newNode == NULL) return root;   /* Memory allocation failed */
    newNode->val = val;
    newNode->Left = NULL;
    newNode->Right = NULL;

    /* Attach the new node to its parent (or set as root if tree was empty) */
    if (parent == NULL) {
        return newNode;                 /* The original tree was empty */
    } else if (val < parent->val) {
        parent->Left = newNode;
    } else {
        parent->Right = newNode;
    }
    return root;
}

/*
 * Delete a key from the BST iteratively.
 * Handles leaf nodes, one-child nodes, and two-child nodes (via in-order successor).
 */
BSTreeNode* deleteBST(BSTreeNode* root, int val) {
    BSTreeNode* parent = NULL;
    BSTreeNode* current = root;

    /* Search for the target node while keeping track of its parent */
    while (current != NULL && current->val != val) {
        parent = current;
        if (val < current->val) {
            current = current->Left;
        } else {
            current = current->Right;
        }
    }
    if (current == NULL) return root;   /* Target key not found in the tree */

    /* If the target node has two children, reduce to deleting its in-order successor */
    if (current->Left != NULL && current->Right != NULL) {
        /* Find the minimum node in the right subtree (which has no left child) */
        BSTreeNode* successorParent = current;
        BSTreeNode* successor = current->Right;
        while (successor->Left != NULL) {
            successorParent = successor;
            successor = successor->Left;
        }
        /* Copy the successor's key into the current node and splice out the successor */
        current->val = successor->val;
        parent = successorParent;
        current = successor;
    }

    /* Connect the single child (or NULL) directly to the removed node's parent */
    BSTreeNode* child = current->Left != NULL ? current->Left : current->Right;
    if (parent == NULL) {
        root = child;                   /* Deleting the root node */
    } else if (parent->Left == current) {
        parent->Left = child;
    } else {
        parent->Right = child;
    }
    free(current);
    return root;
}

/*
 * Recursively write directed edges of the BST in Graphviz DOT format.
 */
void exportBSTDOT(BSTreeNode* root, FILE* fp) {
    if (root == NULL) return;

    if (root->Left != NULL) {
        fprintf(fp, "    %d -> %d;\n",
                root->val, root->Left->val);
    }

    if (root->Right != NULL) {
        fprintf(fp, "    %d -> %d;\n",
                root->val, root->Right->val);
    }

    exportBSTDOT(root->Left, fp);
    exportBSTDOT(root->Right, fp);
}

/*
 * Export the BST structure to a DOT file and render it as a PNG image.
 */
void visualizeBST(BSTreeNode* root) {
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

    snprintf(dotPath, sizeof(dotPath), "%s/bst.dot", dir);
    snprintf(pngPath, sizeof(pngPath), "%s/bst.png", dir);

    FILE* fp = fopen(dotPath, "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to create '%s': %s\n",
                dotPath, strerror(errno));
        return;
    }

    fprintf(fp, "digraph BST {\n");
    fprintf(fp, "    node [shape=circle];\n");
    fprintf(fp, "    rankdir=TB;\n");

    exportBSTDOT(root, fp);

    fprintf(fp, "}\n");
    fclose(fp);

    int ret = treeRenderGraphviz(dotPath, pngPath);

    if (ret == 0) {
        printf("BST visualization saved to %s\n", pngPath);
    } else {
        fprintf(stderr,
                "Graphviz rendering failed. Make sure Graphviz is installed "
                "and the 'dot' command is available in PATH.\n"
                "The DOT source was still saved to %s\n",
                dotPath);
    }
}
