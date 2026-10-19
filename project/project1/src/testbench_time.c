/*
 * File: testbench_time.c
 * Description: Automated performance benchmark for comparing Unbalanced BST,
 *              AVL Tree, Splay Tree, Red-Black Tree, and Order-4 B+ Tree
 *              under three insertion/deletion workloads.
 * Usage: testbench_time [startN] [endN] [step] [repetitions] [output.csv]
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "binaray_search_tree.h"
#include "AVL_tree.h"
#include "splay_tree.h"
#include "red_black_tree.h"
#include "B_plus_tree.h"

/* Function pointer type and adapter entry for uniform benchmarking */
typedef void (*RunFunc)(const int*, const int*, int, double*, double*);
typedef struct { const char *name; RunFunc run; } TreeAdapter;

/*
 * High-resolution wall-clock timer returning elapsed time in seconds.
 */
static double nowSeconds(void) {
#ifdef _WIN32
    LARGE_INTEGER counter, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    return (double)t.tv_sec + (double)t.tv_nsec / 1000000000.0;
#endif
}

/*
 * Generate a uniform random permutation of {0, 1, ..., n - 1} using Fisher-Yates shuffle.
 */
static int *permutation(int n, unsigned seed) {
    int *v = (int*)malloc((size_t)n * sizeof(*v));
    if(!v) return NULL;
    for(int i = 0; i < n; i++) v[i] = i;
    srand(seed);
    for(int i = n - 1; i > 0; i--){
        int j = rand() % (i + 1), t = v[i];
        v[i] = v[j];
        v[j] = t;
    }
    return v;
}

/* Helper routines to free any remaining nodes after a test run */
static void freeBST(BSTreeNode*p){if(p){freeBST(p->Left);freeBST(p->Right);free(p);}}
static void freeAVL(AVLTreeNode*p){if(p){freeAVL(p->Left);freeAVL(p->Right);free(p);}}
static void freeSplay(SplayTreeNode*p){if(p){freeSplay(p->left);freeSplay(p->right);free(p);}}
static void freeRB(RedBlackTreeNode*p){if(p){freeRB(p->left);freeRB(p->right);free(p);}}

/*
 * Per-structure benchmark runners: measure total time for N insertions and N deletions.
 */
static void runBST(const int*i,const int*d,int n,double*ti,double*td){BSTreeNode*r=NULL;double a=nowSeconds();for(int k=0;k<n;k++)r=insertBST(r,i[k]);*ti=nowSeconds()-a;a=nowSeconds();for(int k=0;k<n;k++)r=deleteBST(r,d[k]);*td=nowSeconds()-a;freeBST(r);}
static void runAVL(const int*i,const int*d,int n,double*ti,double*td){AVLTreeNode*r=NULL;double a=nowSeconds();for(int k=0;k<n;k++)r=insertAVL(r,i[k]);*ti=nowSeconds()-a;a=nowSeconds();for(int k=0;k<n;k++)r=deleteAVL(r,d[k]);*td=nowSeconds()-a;freeAVL(r);}
static void runSplay(const int*i,const int*d,int n,double*ti,double*td){SplayTreeNode*r=NULL;double a=nowSeconds();for(int k=0;k<n;k++)r=insertSplay(r,i[k]);*ti=nowSeconds()-a;a=nowSeconds();for(int k=0;k<n;k++)r=deleteSplay(r,d[k]);*td=nowSeconds()-a;freeSplay(r);}
static void runRB(const int*i,const int*d,int n,double*ti,double*td){RedBlackTreeNode*r=NULL;double a=nowSeconds();for(int k=0;k<n;k++)r=insertRedBlack(r,i[k]);*ti=nowSeconds()-a;a=nowSeconds();for(int k=0;k<n;k++){RedBlackTreeNode*x=NULL;r=deleteRedBlack(r,d[k],&x);free(x);}*td=nowSeconds()-a;freeRB(r);}
static void runBPlus(const int*i,const int*d,int n,double*ti,double*td){BPlusTree*r=createBPlusTree(4);double a=nowSeconds();for(int k=0;k<n;k++)insertBPlus(r,i[k]);*ti=nowSeconds()-a;a=nowSeconds();for(int k=0;k<n;k++)deleteBPlus(r,d[k]);*td=nowSeconds()-a;destroyBPlusTree(r);}

/*
 * Run all five trees under a given workload scenario and record average timings to CSV.
 */
static void benchmark(FILE*out,const char*scenario,int n,int repeats,const int*ins,const int*del){
    TreeAdapter trees[]={{"BST",runBST},{"AVL",runAVL},{"Splay",runSplay},{"RedBlack",runRB},{"BPlus",runBPlus}};
    for(size_t t=0;t<sizeof(trees)/sizeof(trees[0]);t++){
        double it=0,dt=0,a,b;
        for(int r=0;r<repeats;r++){trees[t].run(ins,del,n,&a,&b);it+=a;dt+=b;}
        fprintf(out,"%s,%s,%d,%.9f,%.9f\n",scenario,trees[t].name,n,it/repeats,dt/repeats);
    }
}

/*
 * Main entry point: iterate over N = start..end and evaluate the three scenarios:
 *   1. ordered:         insert 0..N-1, delete 0..N-1
 *   2. ordered_reverse: insert 0..N-1, delete N-1..0
 *   3. random:          insert random permutation, delete random permutation
 */
int main(int argc,char**argv){
    int start=argc>1?atoi(argv[1]):1000, end=argc>2?atoi(argv[2]):50000;
    int step=argc>3?atoi(argv[3]):1000, repeats=argc>4?atoi(argv[4]):3;
    const char*path=argc>5?argv[5]:"data_set.csv";
    if(start<=0||end<start||step<=0||repeats<=0){fprintf(stderr,"Invalid range or repetitions.\n");return 1;}
    FILE*out=fopen(path,"w");if(!out){perror(path);return 1;}
    fprintf(out,"scenario,tree,n,insert_seconds,delete_seconds\n");
    for(int n=start;n<=end;n+=step){
        int*ordered= (int*)malloc((size_t)n*sizeof(*ordered));
        int*reversed= (int*)malloc((size_t)n*sizeof(*reversed));
        int*randomInsert=permutation(n,12345u+(unsigned)n);
        int*randomDelete=permutation(n,67890u+(unsigned)n);
        if(!ordered||!reversed||!randomInsert||!randomDelete){fprintf(stderr,"Out of memory at N=%d.\n",n);free(ordered);free(reversed);free(randomInsert);free(randomDelete);fclose(out);return 1;}
        for(int i=0;i<n;i++){
            ordered[i]=i;           /* Increasing sequence 0, 1, ..., n - 1 */
            reversed[i]=n-1-i;      /* Decreasing sequence n - 1, ..., 1, 0 */
        }
        fprintf(stderr,"Testing N=%d/%d\n",n,end);
        benchmark(out,"ordered",n,repeats,ordered,ordered);
        benchmark(out,"ordered_reverse",n,repeats,ordered,reversed);
        benchmark(out,"random",n,repeats,randomInsert,randomDelete);
        free(ordered);free(reversed);free(randomInsert);free(randomDelete);
        if(end-n<step) break; /* Avoid integer overflow at the upper bound */
    }
    fclose(out);printf("Benchmark written to %s\n",path);return 0;
}
