/*
 * File: testbench_figure.c
 * Description: Small-scale structural verification and Graphviz topology
 *              generator for BST, AVL, Splay, Red-Black, and B+ Trees.
 */

#include "binaray_search_tree.h"
#include "AVL_tree.h"
#include "splay_tree.h"
#include "red_black_tree.h"
#include "B_plus_tree.h"

int main(){
    /* 1. Build and visualize an unbalanced BST with sequential keys 0..9 */
    BSTreeNode* bstRoot = NULL;
    for(int i = 0; i < 10; i++){
        bstRoot = insertBST(bstRoot, i);
    }
    visualizeBST(bstRoot);

    /* 2. Build and visualize an AVL Tree with sequential keys 0..9 */
    AVLTreeNode* root = NULL;
    for(int i = 0; i < 10; i++){
        root = insertAVL(root, i);
    }
    visualizeAVL(root);

    /* 3. Build and visualize a Splay Tree with sequential keys 0..9 */
    SplayTreeNode* splayRoot = NULL;
    for(int i = 0; i < 10; i++){
        splayRoot = insertSplay(splayRoot, i);
    }
    visualizeSPLAY(splayRoot);

    /* 4. Build a Red-Black Tree with keys 10..19, delete key 15, and visualize */
    RedBlackTreeNode* rbRoot = NULL;
    for(int i = 10; i < 20; i++){
        rbRoot = insertRedBlack(rbRoot, i);
    }
    RedBlackTreeNode* deleted = NULL;
    rbRoot = deleteRedBlack(rbRoot, 15, &deleted); /* Delete key 15 to test RB deletion fix-up */
    free(deleted);
    visualizeRedBlack(rbRoot);

    /* 5. Build an order-4 B+ Tree with keys 0..9, test deleting absent key 20, and visualize */
    BPlusTree* tree = createBPlusTree(4);
    for(int i = 0; i < 10; i++){
        insertBPlus(tree, i);
    }
    deleteBPlus(tree, 20); /* Deleting a non-existent key should leave the tree intact */
    visualizeBPlus(tree);
    destroyBPlusTree(tree);

    return 0;
}
