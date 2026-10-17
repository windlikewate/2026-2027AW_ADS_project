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



typedef struct SplayTreeNode SplayTreeNode;

struct SplayTreeNode{
    int key;
    SplayTreeNode* left;
    SplayTreeNode* right;
};

SplayTreeNode* singleRotateLL(SplayTreeNode* root){// the left-left rotation
    SplayTreeNode* newRoot = root->left;
    root->left = newRoot->right;
    newRoot->right = root;
    return newRoot;
}

SplayTreeNode* singleRotateRR(SplayTreeNode* root){// the right-right rotation
    SplayTreeNode* newRoot = root->right;
    root->right = newRoot->left;
    newRoot->left = root;
    return newRoot;
}
SplayTreeNode* findSplay(SplayTreeNode* root, int key){ // implement the find operation with splay
    if(!root || root->key == key) return root;

    if(key < root->key){
        if(root->left == NULL) return root; // the key is not in the tree

        if(key < root->left->key){ // zig-zig: continue searching in the left-left subtree
            root->left->left = findSplay(root->left->left, key);
            root = singleRotateLL(root);
        } else if(key > root->left->key){ // zig-zag: continue searching in the left-right subtree
            root->left->right = findSplay(root->left->right, key);
            if(root->left->right != NULL)
                root->left = singleRotateRR(root->left);
        }

        // perform the final zig rotation if the searched node has a parent
        return root->left == NULL ? root : singleRotateLL(root);
    } else {
        if(root->right == NULL) return root; // the key is not in the tree

        if(key > root->right->key){ // zag-zag: continue searching in the right-right subtree
            root->right->right = findSplay(root->right->right, key);
            root = singleRotateRR(root);
        } else if(key < root->right->key){ // zag-zig: continue searching in the right-left subtree
            root->right->left = findSplay(root->right->left, key);
            if(root->right->left != NULL)
                root->right = singleRotateLL(root->right);
        }

        // perform the final zag rotation if the searched node has a parent
        return root->right == NULL ? root : singleRotateRR(root);
    }
}


SplayTreeNode* insertSplay(SplayTreeNode* root, int key){ // implement the insert operation with splay
    if(!root){
        root = (SplayTreeNode*)malloc(sizeof(SplayTreeNode));
        if(root == NULL) return NULL;
        root->key = key;
        root->left = NULL;
        root->right = NULL;
        return root;
    }

    root = findSplay(root, key); // splay the key or the last accessed node to the root
    if(root->key == key) return root; // do not insert a duplicate key

    SplayTreeNode* newRoot = (SplayTreeNode*)malloc(sizeof(SplayTreeNode));
    if(newRoot == NULL) return root;
    newRoot->key = key;

    if(key < root->key){
        newRoot->left = root->left; // split the tree at the new key
        newRoot->right = root;
        root->left = NULL;
    } else {
        newRoot->right = root->right; // split the tree at the new key
        newRoot->left = root;
        root->right = NULL;
    }
    return newRoot;
}

SplayTreeNode* deleteSplay(SplayTreeNode* root, int key){ // implement the delete operation with splay
    if(!root) return NULL;
    root = findSplay(root, key); // splay the node to be deleted to the root
    if(root->key != key) return root; // key not found

    SplayTreeNode* newRoot;
    if(root->left == NULL){
        newRoot = root->right;
    } else {
        // key is greater than every key in the left subtree, so its maximum becomes the root
        newRoot = findSplay(root->left, key);
        newRoot->right = root->right; // join the original right subtree
    }
    free(root);
    return newRoot;
}



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

void visualizeSPLAY(SplayTreeNode* root) {
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


