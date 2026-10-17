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



typedef struct AVLTreeNode AVLTreeNode; //define the AVL tree node
struct AVLTreeNode{
    int val;
    int height; //height of the node in the AVL tree
    int BF; // balance factor of the node in the AVL tree
    AVLTreeNode* Left;
    AVLTreeNode* Right;
};

int CalculateHeight(AVLTreeNode* node) {
    return node == NULL ? -1 : node->height;
}

int CalculateBF(AVLTreeNode* node) {
    if (node == NULL) {
        return 0;
    }
    return CalculateHeight(node->Left) - CalculateHeight(node->Right);
}

AVLTreeNode* singleRotationLL(AVLTreeNode* root){
    AVLTreeNode* L = root->Left;
    root->Left = L->Right;
    L->Right = root;
    root->height = (CalculateHeight(root->Left) > CalculateHeight(root->Right)
                    ? CalculateHeight(root->Left) : CalculateHeight(root->Right)) + 1;
    root->BF = CalculateBF(root);
    L->height = (CalculateHeight(L->Left) > CalculateHeight(L->Right)
                 ? CalculateHeight(L->Left) : CalculateHeight(L->Right)) + 1;
    L->BF = CalculateBF(L);
    return L;
}

AVLTreeNode* singleRotationRR(AVLTreeNode* root){
    AVLTreeNode* R = root->Right;
    root->Right = R->Left;
    R->Left = root;
    root->height = (CalculateHeight(root->Left) > CalculateHeight(root->Right)
                    ? CalculateHeight(root->Left) : CalculateHeight(root->Right)) + 1;
    root->BF = CalculateBF(root);
    R->height = (CalculateHeight(R->Left) > CalculateHeight(R->Right)
                 ? CalculateHeight(R->Left) : CalculateHeight(R->Right)) + 1;
    R->BF = CalculateBF(R);
    return R;
}

AVLTreeNode* doubleRotationLR(AVLTreeNode* root){
    root->Left = singleRotationRR(root->Left);
    return singleRotationLL(root);
}

AVLTreeNode* doubleRotationRL(AVLTreeNode* root){
    root->Right = singleRotationLL(root->Right);
    return singleRotationRR(root);
}

AVLTreeNode* insertAVL(AVLTreeNode* root,int val){
    if (root == NULL) {
        AVLTreeNode* newNode = (AVLTreeNode*)malloc(sizeof(AVLTreeNode));
        newNode->val = val;
        newNode->height = 0;
        newNode->BF = 0;
        newNode->Left = NULL;
        newNode->Right = NULL;
        return newNode;
    }

    if (val < root->val) {
        root->Left = insertAVL(root->Left, val);
    } else if (val > root->val) {
        root->Right = insertAVL(root->Right, val);
    } else {
        // 此时说明已经存在相同的值，不需要插入，直接返回当前节点即可
        return root;
    }

    // Update height and balance factor
    root->height = (CalculateHeight(root->Left) > CalculateHeight(root->Right)
                    ? CalculateHeight(root->Left) : CalculateHeight(root->Right)) + 1;
    root->BF = CalculateBF(root);

    // Perform rotations if necessary
    if (root->BF > 1) {
        if (CalculateBF(root->Left) < 0) {
            return doubleRotationLR(root);
        } else {
            return singleRotationLL(root);
        }
    }

    if (root->BF < -1) {
        if (CalculateBF(root->Right) > 0) {
            return doubleRotationRL(root);
        } else {
            return singleRotationRR(root);
        }
    }

    return root;
}

AVLTreeNode* deleteAVL(AVLTreeNode* root, int val){
    if (root == NULL){
        return NULL;
    }

    if (val < root->val) {
        root->Left = deleteAVL(root->Left, val);
    } else if (val > root->val) {
        root->Right = deleteAVL(root->Right, val);
    } else {
        // Node to be deleted found
        if (root->Left == NULL) { // Node has no left child, replace with right child
            AVLTreeNode* temp = root->Right;
            free(root);
            return temp;
        } else if (root->Right == NULL) { // Node has no right child, replace with left child
            AVLTreeNode* temp = root->Left;
            free(root);
            return temp;
        } else { // Node has both left and right children, replace with inorder successor
            AVLTreeNode* temp = root->Right;
            while (temp->Left != NULL) {
                temp = temp->Left;
            }
            root->val = temp->val;
            root->Right = deleteAVL(root->Right, temp->val);
        }
    }

    // Update height and balance factor
    root->height = (CalculateHeight(root->Left) > CalculateHeight(root->Right)
                    ? CalculateHeight(root->Left) : CalculateHeight(root->Right)) + 1;
    root->BF = CalculateBF(root);

    // Perform rotations if necessary
    if (root->BF > 1) {
        if (CalculateBF(root->Left) < 0) {
            return doubleRotationLR(root);
        } else {
            return singleRotationLL(root);
        }
    }

    if (root->BF < -1) {
        if (CalculateBF(root->Right) > 0) {
            return doubleRotationRL(root);
        } else {
            return singleRotationRR(root);
        }
    }

    return root;
}


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

void visualizeAVL(AVLTreeNode* root) {
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
