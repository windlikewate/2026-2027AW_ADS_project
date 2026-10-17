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
/* Use PATH lookup (CreateProcess search on Windows) so IDE and shell launchers
   work alike. GRAPHVIZ_DOT can override the executable when needed. */
static int treeRenderGraphviz(const char* dotPath, const char* pngPath) {
    const char* exe = getenv("GRAPHVIZ_DOT");
    if (exe == NULL || exe[0] == '\0') exe = "dot";
#ifdef _WIN32
    /* system() invokes cmd.exe, whose quoting rules break quoted executable
       paths. Use _spawnvp to search PATH and pass arguments without a shell. */
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


typedef struct BSTreeNode BSTreeNode; // Define the binary search tree node structure
struct BSTreeNode{
    int val;
    BSTreeNode* Left;
    BSTreeNode* Right;
};

/*
 * Recursive implementations kept for reference.
 * WARNING: An ordered input makes an ordinary BST degenerate into a chain.
 * These recursive functions may then create O(N) call frames and cause a
 * stack overflow when N is large (around 26000 with the current Windows stack).
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
                temp = temp->Left; //find the smallest node in the right subtree
            }
            root->val = temp->val;
            root->Right = deleteBST(root->Right, temp->val); // Delete the inorder successor from the right subtree
            return root;
        }
    }
    return root; // Return the updated root after deletion
}
*/

BSTreeNode* insertBST(BSTreeNode* root, int val) { // Insert a value into the binary search tree iteratively
    BSTreeNode* parent = NULL;
    BSTreeNode* current = root;

    while (current != NULL) { // Find the empty position for the new node
        parent = current;
        if (val < current->val) {
            current = current->Left;
        } else if (val > current->val) {
            current = current->Right;
        } else {
            return root; // Duplicate value, do nothing
        }
    }

    BSTreeNode* newNode = (BSTreeNode*)malloc(sizeof(BSTreeNode));
    if (newNode == NULL) return root; // Memory allocation failed
    newNode->val = val;
    newNode->Left = NULL;
    newNode->Right = NULL;

    if (parent == NULL) {
        return newNode; // The original tree was empty
    } else if (val < parent->val) {
        parent->Left = newNode;
    } else {
        parent->Right = newNode;
    }
    return root;
}

BSTreeNode* deleteBST(BSTreeNode* root, int val) { // Delete a value from the binary search tree iteratively
    BSTreeNode* parent = NULL;
    BSTreeNode* current = root;

    while (current != NULL && current->val != val) { // Find the node and record its parent
        parent = current;
        if (val < current->val) {
            current = current->Left;
        } else {
            current = current->Right;
        }
    }
    if (current == NULL) return root; // Value not found

    if (current->Left != NULL && current->Right != NULL) {
        // Replace the node with its inorder successor, which has at most one child
        BSTreeNode* successorParent = current;
        BSTreeNode* successor = current->Right;
        while (successor->Left != NULL) {
            successorParent = successor;
            successor = successor->Left;
        }
        current->val = successor->val;
        parent = successorParent;
        current = successor;
    }

    // Connect the only child, if any, directly to the deleted node's parent
    BSTreeNode* child = current->Left != NULL ? current->Left : current->Right;
    if (parent == NULL) {
        root = child; // Delete the root node
    } else if (parent->Left == current) {
        parent->Left = child;
    } else {
        parent->Right = child;
    }
    free(current);
    return root;
}

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

void visualizeBST(BSTreeNode* root) {
    /* Support launching the testbench from either the project root or src. */
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

