#include "binaray_search_tree.h"
#include "AVL_tree.h"
#include "splay_tree.h"
#include "red_black_tree.h"
#include "B_plus_tree.h"
int main(){
    BSTreeNode* bstRoot = NULL;
    for(int i = 0; i < 10; i++){
        bstRoot = insertBST(bstRoot, i);
    }
    visualizeBST(bstRoot);
    AVLTreeNode* root = NULL;
    for(int i = 0; i < 10; i++){
        root = insertAVL(root, i);
    }
    visualizeAVL(root);

    SplayTreeNode* splayRoot = NULL;
    for(int i = 0; i < 10; i++){
        splayRoot = insertSplay(splayRoot, i);
    }
    visualizeSPLAY(splayRoot);

    RedBlackTreeNode* rbRoot = NULL;
    for(int i = 10; i < 20; i++){
        rbRoot = insertRedBlack(rbRoot, i);
    }
    RedBlackTreeNode* deleted = NULL;
    rbRoot = deleteRedBlack(rbRoot, 15, &deleted); // delete a node from the red-black tree
    free(deleted);
    visualizeRedBlack(rbRoot);

    BPlusTree* tree = createBPlusTree(4);
    for(int i = 0; i < 10; i++){
        insertBPlus(tree, i);
    }

      deleteBPlus(tree, 20);

      visualizeBPlus(tree);
      destroyBPlusTree(tree);
    return 0;
}
