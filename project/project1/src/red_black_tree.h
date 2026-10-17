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


#define BLACK 0
#define RED 1
typedef struct RedBlackTreeNode RedBlackTreeNode;

struct RedBlackTreeNode{
    int key;
    int color; // 0 for black, 1 for red
    RedBlackTreeNode* left;
    RedBlackTreeNode* right;
    RedBlackTreeNode* parent; //the insert and delete operations need to know the color of parent node
};

void exportRBDOT(RedBlackTreeNode* root, FILE* fp) {
    if (root == NULL) return;

    /* Give every node an explicit color so the PNG reflects the RB tree. */
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

void visualizeRedBlack(RedBlackTreeNode* root) {
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

RedBlackTreeNode* singlerotationLL(RedBlackTreeNode* root){ // Left-Left rotation
    RedBlackTreeNode* oldParent = root->parent;
    RedBlackTreeNode* newRoot = root->left;
    root->left = newRoot->right;
    if(root->left)
        root->left->parent = root;
    newRoot->right = root;
    newRoot->parent = oldParent;
    root->parent = newRoot;
    if (oldParent != NULL) {
        if (oldParent->left == root) oldParent->left = newRoot;
        else if (oldParent->right == root) oldParent->right = newRoot;
    }
    return newRoot;
}

RedBlackTreeNode* singlerotationRR(RedBlackTreeNode* root){ // Right-Right rotation
    RedBlackTreeNode* oldParent = root->parent;
    RedBlackTreeNode* newRoot = root->right;
    root->right = newRoot->left;
    if(root->right)
        root->right->parent = root;
    newRoot->left = root;
    newRoot->parent = oldParent;
    root->parent = newRoot;
    if (oldParent != NULL) {
        if (oldParent->left == root) oldParent->left = newRoot;
        else if (oldParent->right == root) oldParent->right = newRoot;
    }
    return newRoot;
}

static RedBlackTreeNode* insertRBNode(RedBlackTreeNode* root, int key,
                                      RedBlackTreeNode** inserted) { // record the inserted node to use for fix
    if (root == NULL) {
        RedBlackTreeNode* node = (RedBlackTreeNode*)malloc(sizeof(*node));
        if (node == NULL) return NULL;
        node->key = key;
        node->color = RED;
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

static RedBlackTreeNode* rbRoot(RedBlackTreeNode* node) {// find the root of the red-black tree from a given node
    if (node == NULL || node->parent == NULL) return node;
    return rbRoot(node->parent);
}

static RedBlackTreeNode* fixRedBlackAfterInsert(RedBlackTreeNode* node) {
    while (node != NULL && node->parent != NULL && node->parent->color == RED) { // Fix the red-black tree after insertion using iterative approach
        RedBlackTreeNode* parent = node->parent;
        RedBlackTreeNode* grandparent = parent->parent;
        if (grandparent == NULL) break;

        if (parent == grandparent->left) {
            // Case 1: Uncle is red
            RedBlackTreeNode* uncle = grandparent->right;
            if (uncle != NULL && uncle->color == RED) {
                parent->color = BLACK;
                uncle->color = BLACK;
                grandparent->color = RED;
                node = grandparent;
            } else {
                if (node == parent->right) {
                    singlerotationRR(parent);   /* Case 2: LR */
                    parent = node;
                }
                parent->color = BLACK;          /* Case 3 */
                grandparent->color = RED;
                singlerotationLL(grandparent);
                break;
            }
        } else {
            // Case 1: Uncle is red
            RedBlackTreeNode* uncle = grandparent->left;
            if (uncle != NULL && uncle->color == RED) {
                parent->color = BLACK;
                uncle->color = BLACK;
                grandparent->color = RED;
                node = grandparent;
            } else {
                if (node == parent->left) {
                    singlerotationLL(parent);   /* Case 2: RL */
                    parent = node;
                }
                parent->color = BLACK;          /* Case 3 */
                grandparent->color = RED;
                singlerotationRR(grandparent);
                break;
            }
        }
    }

    RedBlackTreeNode* root = rbRoot(node);
    if (root != NULL) root->color = BLACK;
    return root;
}

RedBlackTreeNode* insertRedBlack(RedBlackTreeNode* root, int key) {
    RedBlackTreeNode* inserted = NULL;
    RedBlackTreeNode* treeRoot = insertRBNode(root, key, &inserted);
    if (treeRoot == NULL || inserted == NULL) return treeRoot;
    return fixRedBlackAfterInsert(inserted);
}

RedBlackTreeNode* fixRedBlackAfterDelete(RedBlackTreeNode* node) {
    if(node == NULL) return NULL;

    while(node->parent !=NULL && node->color == BLACK) {
        //Case 1: the sibling is red
        RedBlackTreeNode* parent = node->parent;
        RedBlackTreeNode* sibling = (node == parent->left) ? parent->right : parent->left;
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
        //Case 2: the sibling is black and both of its children are black
        if(sibling == NULL) {
            node = parent;
            continue;
        }
        if((sibling->left == NULL || sibling->left->color == BLACK) && (sibling->right == NULL || sibling->right->color == BLACK)) {
            sibling->color = RED;
            //if the color of the parent is red
            if(parent->color == RED) {
                parent->color = BLACK;
                break;
            } 
            //if the color of the parent is black
            else {
                node = parent;
                continue;
            }
            //Case 3: the sibling is black and the inner child is red
        } else if(node == parent->left){
            if(sibling->right == NULL || sibling->right->color == BLACK){
                sibling->color = RED;
                if(sibling->left != NULL) sibling->left->color = BLACK;
                singlerotationLL(sibling);
                sibling = parent->right;
            }
            //Case 4: the sibling is black and the outer child is red
            sibling->color = parent->color;
            parent->color = BLACK;
            if(sibling->right != NULL) sibling->right->color = BLACK;
            singlerotationRR(parent);
            break;
        } else {
            if(sibling->left == NULL || sibling->left->color == BLACK){
                sibling->color = RED;
                if(sibling->right != NULL) sibling->right->color = BLACK;
                singlerotationRR(sibling);
                sibling = parent->left;
            }
            //Case 4: the sibling is black and the outer child is red
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

RedBlackTreeNode* deleteRedBlack(RedBlackTreeNode* root, int key, RedBlackTreeNode** deleted) {
    // Implementation for deleting a node from the red-black tree goes here
    RedBlackTreeNode* target = root;
    if(deleted != NULL) *deleted = NULL;

    while(target != NULL && target->key != key) { // find the node to delete
        if(key < target->key) {
            target = target->left;
        } else {
            target = target->right;
        }
    }
    if(target == NULL) return root;

    RedBlackTreeNode* removed = target;
    if(target->left != NULL && target->right != NULL) {
        removed = target->left;
        while(removed->right!=NULL) { // Find the rightmost node in the left subtree to replace the deleted node
            removed = removed->right;
        }
        RedBlackTreeNode* tept = target; // swap the deleted node with this node
        int keyToDelete = tept->key;
        tept->key = removed->key;
        removed->key = keyToDelete;
    }

    RedBlackTreeNode* replacement = removed->left != NULL ? removed->left : removed->right; // the node that will replace the removed node
    RedBlackTreeNode* parent = removed->parent;
    int removedWasLeft = parent != NULL && parent->left == removed;

    if(parent == NULL) {
        root = replacement; // if romoved is the root node, update the root to the replacement node
    } else if(removedWasLeft) {
        parent->left = replacement; // if the removed node was the left child of its parent, update the parent's left child to the replacement node
    } else {
        parent->right = replacement; // if the removed node was the right child of its parent, update the parent's right child to the replacement node
    }
    if(replacement != NULL) replacement->parent = parent; // update the parent of the replacement node to the parent of the removed node

    if(removed->color == BLACK) {
        if(replacement != NULL) {
            root = fixRedBlackAfterDelete(replacement);
        } else { // if there is no replacement node, create a temporary node to represent the removed node
            RedBlackTreeNode temporary = {0, BLACK, NULL, NULL, parent};
            if(parent == NULL) {
                root = &temporary;
            } else if(removedWasLeft) {
                parent->left = &temporary;
            } else {
                parent->right = &temporary;
            }

            root = fixRedBlackAfterDelete(&temporary);
            if(temporary.parent != NULL) {
                if(temporary.parent->left == &temporary) temporary.parent->left = NULL;
                if(temporary.parent->right == &temporary) temporary.parent->right = NULL;
            }
            if(root == &temporary) root = NULL;
        }
    }

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
