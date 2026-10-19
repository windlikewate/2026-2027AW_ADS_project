/*
 * File: red_black_tree.h
 * Description: Implementation of a Red-Black Tree with parent pointers,
 *              iterative insertion fix-up (3 cases), iterative double-black
 *              deletion fix-up (4 cases), and Graphviz visualization.
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

/* Node color constants */
#define BLACK 0
#define RED 1
typedef struct RedBlackTreeNode RedBlackTreeNode;

/*
 * Red-Black Tree node structure.
 * Invariants:
 *   1. Every node is either RED or BLACK.
 *   2. The root and all NULL leaves are BLACK.
 *   3. A RED node cannot have a RED child.
 *   4. Every simple path from a node to a NULL leaf has the same black-height.
 */
struct RedBlackTreeNode{
    int key;                    /* Integer key stored in the node */
    int color;                  /* Node color: 0 for BLACK, 1 for RED */
    RedBlackTreeNode* left;     /* Pointer to the left child */
    RedBlackTreeNode* right;    /* Pointer to the right child */
    RedBlackTreeNode* parent;   /* Pointer to the parent node for bottom-up fix-ups */
};

/*
 * Recursively export Red-Black Tree nodes and edges in Graphviz DOT format.
 */
void exportRBDOT(RedBlackTreeNode* root, FILE* fp) {
    if (root == NULL) return;

    /* Give every node an explicit fill color matching its Red-Black color */
    fprintf(fp, "    n%p [label=\"%d\", style=filled, fillcolor=\"%s\", fontcolor=\"%s\"];\n",
            (void*)root, root->key,
            root->color == RED ? "red" : "black",
            root->color == RED ? "black" : "white");

    if (root->left != NULL) {
        fprintf(fp, "    n%p -> n%p;\n",
                (void*)root, (void*)root->left);
    }

    if (root->right != NULL) {
        fprintf(fp, "    n%p -> n%p;\n",
                (void*)root, (void*)root->right);
    }

    exportRBDOT(root->left, fp);
    exportRBDOT(root->right, fp);
}

/*
 * Export the Red-Black Tree to a DOT file and render it as a PNG image.
 */
void visualizeRedBlack(RedBlackTreeNode* root) {
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

    snprintf(dotPath, sizeof(dotPath), "%s/rb.dot", dir);
    snprintf(pngPath, sizeof(pngPath), "%s/rb.png", dir);

    FILE* fp = fopen(dotPath, "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to create '%s': %s\n",
                dotPath, strerror(errno));
        return;
    }

    fprintf(fp, "digraph RED_BLACK {\n");
    fprintf(fp, "    graph [rankdir=TB];\n");
    fprintf(fp, "    node [shape=circle];\n");

    exportRBDOT(root, fp);

    fprintf(fp, "}\n");
    fclose(fp);

    int ret = treeRenderGraphviz(dotPath, pngPath);

    if (ret == 0) {
        printf("Red-black tree visualization saved to %s\n", pngPath);
    } else {
        fprintf(stderr,
                "Graphviz rendering failed. Make sure Graphviz is installed "
                "and the 'dot' command is available in PATH.\n"
                "The DOT source was still saved to %s\n",
                dotPath);
    }
}

/*
 * Single right rotation (LL rotation) at 'root', updating parent pointers.
 */
RedBlackTreeNode* singlerotationLL(RedBlackTreeNode* root){
    RedBlackTreeNode* oldParent = root->parent;
    RedBlackTreeNode* newRoot = root->left;
    root->left = newRoot->right;
    if(root->left)
        root->left->parent = root;
    newRoot->right = root;
    newRoot->parent = oldParent;
    root->parent = newRoot;
    /* Reconnect the rotated subtree to oldParent */
    if (oldParent != NULL) {
        if (oldParent->left == root) oldParent->left = newRoot;
        else if (oldParent->right == root) oldParent->right = newRoot;
    }
    return newRoot;
}

/*
 * Single left rotation (RR rotation) at 'root', updating parent pointers.
 */
RedBlackTreeNode* singlerotationRR(RedBlackTreeNode* root){
    RedBlackTreeNode* oldParent = root->parent;
    RedBlackTreeNode* newRoot = root->right;
    root->right = newRoot->left;
    if(root->right)
        root->right->parent = root;
    newRoot->left = root;
    newRoot->parent = oldParent;
    root->parent = newRoot;
    /* Reconnect the rotated subtree to oldParent */
    if (oldParent != NULL) {
        if (oldParent->left == root) oldParent->left = newRoot;
        else if (oldParent->right == root) oldParent->right = newRoot;
    }
    return newRoot;
}

/*
 * Standard BST insertion helper that colors the new leaf RED and outputs its pointer.
 */
static RedBlackTreeNode* insertRBNode(RedBlackTreeNode* root, int key,
                                      RedBlackTreeNode** inserted) {
    if (root == NULL) {
        RedBlackTreeNode* node = (RedBlackTreeNode*)malloc(sizeof(*node));
        if (node == NULL) return NULL;
        node->key = key;
        node->color = RED;  /* New leaves are always colored RED initially */
        node->left = node->right = node->parent = NULL;
        *inserted = node;
        return node;
    }

    if (key < root->key) {
        root->left = insertRBNode(root->left, key, inserted);
        if (root->left != NULL) root->left->parent = root;
    } else if (key > root->key) {
        root->right = insertRBNode(root->right, key, inserted);
        if (root->right != NULL) root->right->parent = root;
    }
    return root;
}

/*
 * Climb parent pointers from 'node' to locate the current root of the tree.
 */
static RedBlackTreeNode* rbRoot(RedBlackTreeNode* node) {
    if (node == NULL || node->parent == NULL) return node;
    return rbRoot(node->parent);
}

/*
 * Restore Red-Black properties after inserting a RED leaf 'node'.
 * Resolves consecutive red-red violations (color(node) == RED && color(parent) == RED)
 * using at most two rotations.
 */
static RedBlackTreeNode* fixRedBlackAfterInsert(RedBlackTreeNode* node) {
    while (node != NULL && node->parent != NULL && node->parent->color == RED) {
        RedBlackTreeNode* parent = node->parent;
        RedBlackTreeNode* grandparent = parent->parent;
        if (grandparent == NULL) break;

        if (parent == grandparent->left) {
            /* Parent is the left child of grandparent */
            RedBlackTreeNode* uncle = grandparent->right;
            if (uncle != NULL && uncle->color == RED) {
                /* Case 1: Uncle is RED -> recolor parent, uncle, and grandparent */
                parent->color = BLACK;
                uncle->color = BLACK;
                grandparent->color = RED;
                node = grandparent;
            } else {
                if (node == parent->right) {
                    /* Case 2: Uncle is BLACK and node is inner (LR) child -> rotate parent */
                    singlerotationRR(parent);
                    parent = node;
                }
                /* Case 3: Uncle is BLACK and node is outer (LL) child -> recolor and rotate grandparent */
                parent->color = BLACK;
                grandparent->color = RED;
                singlerotationLL(grandparent);
                break;
            }
        } else {
            /* Symmetric cases when parent is the right child of grandparent */
            RedBlackTreeNode* uncle = grandparent->left;
            if (uncle != NULL && uncle->color == RED) {
                /* Case 1: Uncle is RED -> recolor parent, uncle, and grandparent */
                parent->color = BLACK;
                uncle->color = BLACK;
                grandparent->color = RED;
                node = grandparent;
            } else {
                if (node == parent->left) {
                    /* Case 2: Uncle is BLACK and node is inner (RL) child -> rotate parent */
                    singlerotationLL(parent);
                    parent = node;
                }
                /* Case 3: Uncle is BLACK and node is outer (RR) child -> recolor and rotate grandparent */
                parent->color = BLACK;
                grandparent->color = RED;
                singlerotationRR(grandparent);
                break;
            }
        }
    }

    /* Ensure the tree root is always colored BLACK */
    RedBlackTreeNode* root = rbRoot(node);
    if (root != NULL) root->color = BLACK;
    return root;
}

/*
 * Public API: Insert 'key' into the Red-Black Tree and return the updated root.
 */
RedBlackTreeNode* insertRedBlack(RedBlackTreeNode* root, int key) {
    RedBlackTreeNode* inserted = NULL;
    RedBlackTreeNode* treeRoot = insertRBNode(root, key, &inserted);
    if (treeRoot == NULL || inserted == NULL) return treeRoot;
    return fixRedBlackAfterInsert(inserted);
}

/*
 * Restore Red-Black properties after deleting a BLACK node.
 * The parameter 'node' carries the extra "double-black" token upward until resolved
 * via recoloring or at most three rotations across Cases 1-4.
 */
RedBlackTreeNode* fixRedBlackAfterDelete(RedBlackTreeNode* node) {
    if(node == NULL) return NULL;

    while(node->parent != NULL && node->color == BLACK) {
        RedBlackTreeNode* parent = node->parent;
        RedBlackTreeNode* sibling = (node == parent->left) ? parent->right : parent->left;

        /* Case 1: Sibling is RED -> recolor and rotate parent to obtain a BLACK sibling */
        if(sibling != NULL && sibling->color == RED) {
            sibling->color = BLACK;
            parent->color = RED;
            if(node == parent->left) {
                singlerotationRR(parent);
            } else {
                singlerotationLL(parent);
            }
            sibling = (node == parent->left) ? parent->right : parent->left;
        }

        if(sibling == NULL) {
            node = parent;
            continue;
        }

        /* Case 2: Sibling is BLACK and both nephews are BLACK -> recolor sibling RED */
        if((sibling->left == NULL || sibling->left->color == BLACK) &&
           (sibling->right == NULL || sibling->right->color == BLACK)) {
            sibling->color = RED;
            /* If parent was RED, absorb the extra black token at parent and terminate */
            if(parent->color == RED) {
                parent->color = BLACK;
                break;
            } else {
                /* Otherwise propagate the double-black token up to parent */
                node = parent;
                continue;
            }
        } else if(node == parent->left){
            /* Node is left child: check Case 3 (inner nephew RED, outer nephew BLACK) */
            if(sibling->right == NULL || sibling->right->color == BLACK){
                sibling->color = RED;
                if(sibling->left != NULL) sibling->left->color = BLACK;
                singlerotationLL(sibling);
                sibling = parent->right;
            }
            /* Case 4: Sibling is BLACK and outer (right) nephew is RED -> rotate parent and finish */
            sibling->color = parent->color;
            parent->color = BLACK;
            if(sibling->right != NULL) sibling->right->color = BLACK;
            singlerotationRR(parent);
            break;
        } else {
            /* Symmetric: Node is right child; check Case 3 (inner nephew RED, outer nephew BLACK) */
            if(sibling->left == NULL || sibling->left->color == BLACK){
                sibling->color = RED;
                if(sibling->right != NULL) sibling->right->color = BLACK;
                singlerotationRR(sibling);
                sibling = parent->left;
            }
            /* Case 4: Sibling is BLACK and outer (left) nephew is RED -> rotate parent and finish */
            sibling->color = parent->color;
            parent->color = BLACK;
            if(sibling->left != NULL) sibling->left->color = BLACK;
            singlerotationLL(parent);
            break;
        }
    }

    node->color = BLACK;
    return rbRoot(node);
}

/*
 * Delete 'key' from the Red-Black Tree and return the updated root.
 * Outputs the spliced-out node via '*deleted' so the caller can free it.
 */
RedBlackTreeNode* deleteRedBlack(RedBlackTreeNode* root, int key, RedBlackTreeNode** deleted) {
    RedBlackTreeNode* target = root;
    if(deleted != NULL) *deleted = NULL;

    /* Locate the target node containing 'key' */
    while(target != NULL && target->key != key) {
        if(key < target->key) {
            target = target->left;
        } else {
            target = target->right;
        }
    }
    if(target == NULL) return root; /* Key not found */

    /* If target has two children, swap its key with its in-order predecessor */
    RedBlackTreeNode* removed = target;
    if(target->left != NULL && target->right != NULL) {
        removed = target->left;
        while(removed->right != NULL) {
            removed = removed->right; /* Rightmost node in the left subtree */
        }
        RedBlackTreeNode* tept = target;
        int keyToDelete = tept->key;
        tept->key = removed->key;
        removed->key = keyToDelete;
    }

    /* Splice out 'removed', which now has at most one non-NULL child */
    RedBlackTreeNode* replacement = removed->left != NULL ? removed->left : removed->right;
    RedBlackTreeNode* parent = removed->parent;
    int removedWasLeft = parent != NULL && parent->left == removed;

    if(parent == NULL) {
        root = replacement;         /* Removed node was the root */
    } else if(removedWasLeft) {
        parent->left = replacement;
    } else {
        parent->right = replacement;
    }
    if(replacement != NULL) replacement->parent = parent;

    /* If a BLACK node was removed, black-height is violated and requires fix-up */
    if(removed->color == BLACK) {
        if(replacement != NULL) {
            root = fixRedBlackAfterDelete(replacement);
        } else {
            /* Use a temporary stack sentinel to represent the double-black NULL leaf */
            RedBlackTreeNode temporary = {0, BLACK, NULL, NULL, parent};
            if(parent == NULL) {
                root = &temporary;
            } else if(removedWasLeft) {
                parent->left = &temporary;
            } else {
                parent->right = &temporary;
            }

            root = fixRedBlackAfterDelete(&temporary);
            /* Detach the temporary sentinel after rebalancing */
            if(temporary.parent != NULL) {
                if(temporary.parent->left == &temporary) temporary.parent->left = NULL;
                if(temporary.parent->right == &temporary) temporary.parent->right = NULL;
            }
            if(root == &temporary) root = NULL;
        }
    }

    /* Ensure the final tree root has NULL parent and BLACK color */
    if(root != NULL) {
        root = rbRoot(root);
        root->parent = NULL;
        root->color = BLACK;
    }
    removed->left = NULL;
    removed->right = NULL;
    removed->parent = NULL;
    if(deleted != NULL) *deleted = removed;
    return root;
}
