/*
 * File: AVL_tree.h
 * Description: Implementation of a self-balancing AVL Tree supporting
 *              single rotations (LL, RR), double rotations (LR, RL),
 *              insertion, deletion, and Graphviz visualization.
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
    const char* exe = getenv("GRAPHVIZ_DOT");
    if (exe == NULL || exe[0] == '\0') exe = "dot";
#ifdef _WIN32
    const char* const argv[] = {exe, "-Tpng", dotPath, "-o", pngPath, NULL};
    return _spawnvp(_P_WAIT, exe, argv) == 0 ? 0 : -1;
#else
    char command[1200];
    snprintf(command, sizeof(command), "\"%s\" -Tpng \"%s\" -o \"%s\"", exe, dotPath, pngPath);
    return system(command) == 0 ? 0 : -1;
#endif
}
#endif

/*
 * AVL Tree node definition.
 * Maintains subtree height and balance factor BF = height(Left) - height(Right)
 * so that |BF| <= 1 holds at every node after rebalancing.
 */
typedef struct AVLTreeNode AVLTreeNode;
struct AVLTreeNode{
    int val;            /* Integer key stored in the node */
    int height;         /* Height of the subtree rooted at this node (leaf = 0) */
    int BF;             /* Balance factor: height(Left) - height(Right) */
    AVLTreeNode* Left;  /* Pointer to the left subtree */
    AVLTreeNode* Right; /* Pointer to the right subtree */
};

/*
 * Return the height of a node, or -1 for an empty (NULL) subtree.
 */
int CalculateHeight(AVLTreeNode* node) {
    return node == NULL ? -1 : node->height;
}

/*
 * Compute the balance factor BF(node) = height(Left) - height(Right).
 */
int CalculateBF(AVLTreeNode* node) {
    if (node == NULL) {
        return 0;
    }
    return CalculateHeight(node->Left) - CalculateHeight(node->Right);
}

/*
 * LL Single Right Rotation:
 * Restores balance when the left-left subtree becomes too heavy (BF = +2, left BF >= 0).
 * Lifts the left child L as the new subtree root and updates heights and balance factors.
 */
AVLTreeNode* singleRotationLL(AVLTreeNode* root){
    AVLTreeNode* L = root->Left;
    root->Left = L->Right;  /* Move L's right subtree to root's left */
    L->Right = root;        /* Place old root as L's right child */

    /* Recompute height and balance factor of the demoted root first */
    root->height = (CalculateHeight(root->Left) > CalculateHeight(root->Right)
                    ? CalculateHeight(root->Left) : CalculateHeight(root->Right)) + 1;
    root->BF = CalculateBF(root);

    /* Recompute height and balance factor of the new subtree root L */
    L->height = (CalculateHeight(L->Left) > CalculateHeight(L->Right)
                 ? CalculateHeight(L->Left) : CalculateHeight(L->Right)) + 1;
    L->BF = CalculateBF(L);
    return L;
}

/*
 * RR Single Left Rotation:
 * Restores balance when the right-right subtree becomes too heavy (BF = -2, right BF <= 0).
 * Lifts the right child R as the new subtree root and updates heights and balance factors.
 */
AVLTreeNode* singleRotationRR(AVLTreeNode* root){
    AVLTreeNode* R = root->Right;
    root->Right = R->Left;  /* Move R's left subtree to root's right */
    R->Left = root;         /* Place old root as R's left child */

    /* Recompute height and balance factor of the demoted root first */
    root->height = (CalculateHeight(root->Left) > CalculateHeight(root->Right)
                    ? CalculateHeight(root->Left) : CalculateHeight(root->Right)) + 1;
    root->BF = CalculateBF(root);

    /* Recompute height and balance factor of the new subtree root R */
    R->height = (CalculateHeight(R->Left) > CalculateHeight(R->Right)
                 ? CalculateHeight(R->Left) : CalculateHeight(R->Right)) + 1;
    R->BF = CalculateBF(R);
    return R;
}

/*
 * LR Double Rotation (Left-Right):
 * Left-rotates the left child to convert LR imbalance into LL, then right-rotates root.
 */
AVLTreeNode* doubleRotationLR(AVLTreeNode* root){
    root->Left = singleRotationRR(root->Left);
    return singleRotationLL(root);
}

/*
 * RL Double Rotation (Right-Left):
 * Right-rotates the right child to convert RL imbalance into RR, then left-rotates root.
 */
AVLTreeNode* doubleRotationRL(AVLTreeNode* root){
    root->Right = singleRotationLL(root->Right);
    return singleRotationRR(root);
}

/*
 * Insert a key into the AVL tree and rebalance along the return path.
 */
AVLTreeNode* insertAVL(AVLTreeNode* root, int val){
    /* Base case: allocate a new leaf node with height 0 and BF 0 */
    if (root == NULL) {
        AVLTreeNode* newNode = (AVLTreeNode*)malloc(sizeof(AVLTreeNode));
        newNode->val = val;
        newNode->height = 0;
        newNode->BF = 0;
        newNode->Left = NULL;
        newNode->Right = NULL;
        return newNode;
    }

    /* Recursively descend into the left or right subtree */
    if (val < root->val) {
        root->Left = insertAVL(root->Left, val);
    } else if (val > root->val) {
        root->Right = insertAVL(root->Right, val);
    } else {
        /* Duplicate key already exists; no insertion needed */
        return root;
    }

    /* Update height and balance factor of the current ancestor */
    root->height = (CalculateHeight(root->Left) > CalculateHeight(root->Right)
                    ? CalculateHeight(root->Left) : CalculateHeight(root->Right)) + 1;
    root->BF = CalculateBF(root);

    /* Left-heavy imbalance (BF == +2): apply LR or LL rotation */
    if (root->BF > 1) {
        if (CalculateBF(root->Left) < 0) {
            return doubleRotationLR(root);  /* Left-Right case */
        } else {
            return singleRotationLL(root);  /* Left-Left case */
        }
    }

    /* Right-heavy imbalance (BF == -2): apply RL or RR rotation */
    if (root->BF < -1) {
        if (CalculateBF(root->Right) > 0) {
            return doubleRotationRL(root);  /* Right-Left case */
        } else {
            return singleRotationRR(root);  /* Right-Right case */
        }
    }

    return root;
}

/*
 * Delete a key from the AVL tree and rebalance all affected ancestors.
 */
AVLTreeNode* deleteAVL(AVLTreeNode* root, int val){
    if (root == NULL){
        return NULL;    /* Key not found */
    }

    /* Search for the node to delete in the appropriate subtree */
    if (val < root->val) {
        root->Left = deleteAVL(root->Left, val);
    } else if (val > root->val) {
        root->Right = deleteAVL(root->Right, val);
    } else {
        /* Target node found */
        if (root->Left == NULL) {
            /* Only right child or leaf: replace node with its right child */
            AVLTreeNode* temp = root->Right;
            free(root);
            return temp;
        } else if (root->Right == NULL) {
            /* Only left child: replace node with its left child */
            AVLTreeNode* temp = root->Left;
            free(root);
            return temp;
        } else {
            /* Two children: replace value with in-order successor (minimum of right subtree) */
            AVLTreeNode* temp = root->Right;
            while (temp->Left != NULL) {
                temp = temp->Left;
            }
            root->val = temp->val;
            root->Right = deleteAVL(root->Right, temp->val);
        }
    }

    /* Update height and balance factor after deletion in subtree */
    root->height = (CalculateHeight(root->Left) > CalculateHeight(root->Right)
                    ? CalculateHeight(root->Left) : CalculateHeight(root->Right)) + 1;
    root->BF = CalculateBF(root);

    /* Restore AVL balance if the current node became left-heavy (BF == +2) */
    if (root->BF > 1) {
        if (CalculateBF(root->Left) < 0) {
            return doubleRotationLR(root);
        } else {
            return singleRotationLL(root);
        }
    }

    /* Restore AVL balance if the current node became right-heavy (BF == -2) */
    if (root->BF < -1) {
        if (CalculateBF(root->Right) > 0) {
            return doubleRotationRL(root);
        } else {
            return singleRotationRR(root);
        }
    }

    return root;
}

/*
 * Recursively output AVL edges in Graphviz DOT format.
 */
void exportAVLDOT(AVLTreeNode* root, FILE* fp) {
    if (root == NULL) return;

    if (root->Left != NULL) {
        fprintf(fp, "    %d -> %d;\n",
                root->val, root->Left->val);
    }

    if (root->Right != NULL) {
        fprintf(fp, "    %d -> %d;\n",
                root->val, root->Right->val);
    }

    exportAVLDOT(root->Left, fp);
    exportAVLDOT(root->Right, fp);
}

/*
 * Export the AVL tree to DOT and render it as a PNG image.
 */
void visualizeAVL(AVLTreeNode* root) {
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

    snprintf(dotPath, sizeof(dotPath), "%s/avl.dot", dir);
    snprintf(pngPath, sizeof(pngPath), "%s/avl.png", dir);

    FILE* fp = fopen(dotPath, "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to create '%s': %s\n",
                dotPath, strerror(errno));
        return;
    }

    fprintf(fp, "digraph AVL {\n");
    fprintf(fp, "    node [shape=circle];\n");
    fprintf(fp, "    rankdir=TB;\n");

    exportAVLDOT(root, fp);

    fprintf(fp, "}\n");
    fclose(fp);

    int ret = treeRenderGraphviz(dotPath, pngPath);

    if (ret == 0) {
        printf("AVL visualization saved to %s\n", pngPath);
    } else {
        fprintf(stderr,
                "Graphviz rendering failed. Make sure Graphviz is installed "
                "and the 'dot' command is available in PATH.\n"
                "The DOT source was still saved to %s\n",
                dotPath);
    }
}
