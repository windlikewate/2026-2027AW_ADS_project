/*
 * File: testbench.c
 * Description: Automated verification and high-resolution benchmark harness for
 *              ADS Project 5 ("Shopping with Coupons").
 *
 * Responsibilities:
 *   1. Read all 158 test cases listed in `data/manifest.csv` and validate domain
 *      constraints (`1 <= N <= 10^5`, `1 <= D <= 10^6`, `max C < min P`).
 *   2. Compute independent ground-truth `(Q*, R*, K*)` via an O(N log N + N log D)
 *      value-threshold binary search + monotone two-pointer oracle (`oracle`).
 *   3. Execute one untimed warm-up call followed by `repeats` timed runs of
 *      `solveShoppingCounted`, restoring unsorted `coupons[]` before every call.
 *   4. Verify exact equality on `(count, money, k)` across every run and record
 *      `avg_ms`, `min_ms`, and `max_ms` into `output/results.csv`.
 */

#include "project_paths.h"
#include "bench.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shopping_with_cunpons.h"
#include <time.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#endif

/*
 * @brief Query monotonic / high-resolution wall-clock timestamp in milliseconds.
 */
static double milliseconds(void){
#ifdef _WIN32
    LARGE_INTEGER now,freq;
    QueryPerformanceFrequency(&freq);QueryPerformanceCounter(&now);
    return (double)now.QuadPart*1000.0/(double)freq.QuadPart;
#else
    struct timespec ts;
    timespec_get(&ts,TIME_UTC);
    return (double)ts.tv_sec*1000.0+(double)ts.tv_nsec/1000000.0;
#endif
}

/*
 * @brief Overflow-safe ascending comparator for standard C `qsort` inside `oracle`.
 */
static int ascending(const void *a,const void *b){
    int x=*(const int*)a,y=*(const int*)b;return (x>y)-(x<y);
}

/* Aggregate count and 64-bit total expenditure of all options with cost <= t. */
typedef struct {int64_t count,cost;} Stats;

/*
 * @brief Count all couponed pairs `p[i] - c[j] <= t` (plus affordable full-price
 *        units at `p[0]` when `t >= p[0]`) and their cumulative dollar cost in O(N)
 *        time using sorted arrays `p[]`, `c[]` and coupon prefix sums `prefix[]`.
 *
 * Monotone Two-Pointer Invariant:
 *   - Input arrays satisfy `p[0] <= ... <= p[n-1]` and `c[0] <= ... <= c[n-1]`.
 *   - Condition `p[i] - c[j] <= t` rearranges to `c[j] >= p[i] - t`.
 *   - As item index `i` advances from `0` to `n - 1`, threshold `p[i] - t` is
 *     non-decreasing, so the first qualifying coupon index `j` is also
 *     non-decreasing (`0 <= j <= n`), bounding total `while` steps across the
 *     entire `for` loop to at most `n`.
 *   - For fixed `i`, the `k = n - j` qualifying pairs `c[j .. n-1]` have exact
 *     total cost `k * p[i] - (prefix[n] - prefix[j])` in O(1) time.
 */
static Stats statsAt(int n,int d,const int *p,const int *c,
                     const int64_t *prefix,int t){
    Stats st={0,0};int j=0;
    /* As `i` increases, `p[i] - t` is non-decreasing, so cursor `j` only moves right. */
    for(int i=0;i<n;++i){
        while(j<n&&(int64_t)c[j]<(int64_t)p[i]-t)++j;
        int64_t k=n-j;
        st.count+=k;
        st.cost+=k*p[i]-(prefix[n]-prefix[j]);
    }
    /* Include up to `floor(D / p[0])` full-price units of cheapest item `p[0]`. */
    if(t>=p[0]){
        int64_t k=d/p[0];st.count+=k;st.cost+=k*p[0];
    }
    return st;
}

/*
 * @brief Independent O(N log N + N log D) verification oracle computing exact
 *        `(Q*, R*)` and expected coupon-purchase count `*expectedK` without
 *        using any priority queue or per-item simulation loop.
 *
 * Algorithmic Steps:
 *   1. Sort independent copies of `prices` and `coupons` in ascending order and
 *      build 64-bit coupon prefix sums `prefix[0..n]`.
 *   2. Binary search `t in [0, D]` (at most ceil(log2(10^6 + 1)) = 20 probes)
 *      for the largest marginal price `low` whose cumulative cost `<= D`.
 *   3. Purchase all `st.count` units costing `<= low`, then purchase as many
 *      boundary units at marginal price `low + 1` as `(D - st.cost)` allows.
 *   4. Compute `*expectedK = min(Q*, count_of_pairs_strictly_below_p[0])` to
 *      cross-check the solver's heap-extraction counter `k`.
 */
static Result oracle(int n,int d,const int *prices,const int *coupons,int *expectedK){
    int *p=malloc((size_t)n*sizeof(*p)),*c=malloc((size_t)n*sizeof(*c));
    int64_t *prefix=malloc(((size_t)n+1)*sizeof(*prefix));
    if(!p||!c||!prefix){free(p);free(c);free(prefix);return (Result){-1,-1};}
    memcpy(p,prices,(size_t)n*sizeof(*p));memcpy(c,coupons,(size_t)n*sizeof(*c));
    qsort(p,(size_t)n,sizeof(*p),ascending);qsort(c,(size_t)n,sizeof(*c),ascending);

    /* Build 64-bit prefix sums of ascending coupon discounts. */
    prefix[0]=0;
    for(int i=0;i<n;++i)prefix[i+1]=prefix[i]+c[i];

    /* Binary search over marginal unit price `t in [0, D]` for largest feasible threshold. */
    int low=0,high=d;
    while(low<high){
        int mid=low+(high-low+1)/2;
        if(statsAt(n,d,p,c,prefix,mid).cost<=d)low=mid;
        else high=mid-1;
    }

    /* Buy all options costing `<= low`, then greedily fill boundary options at `low + 1`. */
    Stats st=statsAt(n,d,p,c,prefix,low);
    int64_t extra=0;
    if(low<d){
        Stats next=statsAt(n,d,p,c,prefix,low+1);
        extra=(d-st.cost)/(low+1);
        if(extra>next.count-st.count)extra=next.count-st.count;
    }
    Result r={(int)(st.count+extra),(int)(d-st.cost-extra*(low+1))};

    /* Pairs strictly cheaper than `p[0]` are redeemed before any full-price bulk buy. */
    int64_t cheapPairs=statsAt(n,d,p,c,prefix,p[0]-1).count;
    *expectedK=cheapPairs<r.count?(int)cheapPairs:r.count;
    free(p);free(c);free(prefix);return r;
}

/*
 * @brief CLI entry point: `./testbench [repeats]` (default `repeats = 3`).
 *
 * Execution Protocol:
 *   - Parses optional repetition count `repeats in [1, 100]`.
 *   - Iterates over all 158 entries in `data/manifest.csv`.
 *   - Performs strict input domain validation, oracle verification, one untimed
 *     warm-up call, and `repeats` timed calls with fresh unsorted `coupons[]`.
 *   - Emits per-case CSV rows to `output/results.csv` and returns non-zero if
 *     any case fails verification or encounters an I/O error.
 */
int main(int argc,char **argv){
    if(!enterProjectRoot(argv[0]))return 1;
    int repeats=3;
    if(argc>1){
        char *end;long value=strtol(argv[1],&end,10);
        if(*end||value<1||value>100){fprintf(stderr,"repeats must be 1..100\n");return 1;}
        repeats=(int)value;
    }

    /* Open manifest index and prepare CSV output stream. */
    FILE *manifest=fopen("data/manifest.csv","r");
    if(!manifest){perror("Run generate first");return 1;}
    if(MKDIR("output")!=0&&errno!=EEXIST){perror("output");fclose(manifest);return 1;}
    FILE *out=fopen("output/results.csv","w");
    if(!out){perror("output/results.csv");fclose(manifest);return 1;}
    fprintf(out,"file,profile,n,d,seed,repeats,items,k,remaining,expected_items,expected_remaining,status,avg_ms,min_ms,max_ms\n");

    char line[512],path[160],profile[64];int n,d,total=0,failed=0;unsigned seed;
    if(!fgets(line,sizeof(line),manifest)){fclose(out);fclose(manifest);return 1;}

    /* Iterate through every test case registered in `data/manifest.csv`. */
    while(fgets(line,sizeof(line),manifest)){
        if(sscanf(line,"%159[^,],%63[^,],%d,%d,%u",path,profile,&n,&d,&seed)!=5)goto error;
        if(n<1||n>100000||d<1||d>1000000)goto error;
        int *p=malloc((size_t)n*sizeof(*p)),*c=malloc((size_t)n*sizeof(*c));
        int *copy=malloc((size_t)n*sizeof(*copy));
        if(!p||!c||!copy){free(p);free(c);free(copy);goto error;}

        /* Load `.in` file and validate positivity and `max(C) < min(P)` invariant. */
        FILE *in=fopen(path,"r");
        if(!in){perror(path);free(p);free(c);free(copy);goto error;}
        int actualN,actualD,valid=1,minP=1000000000,maxC=0;
        if(fscanf(in,"%d%d",&actualN,&actualD)!=2||actualN!=n||actualD!=d)valid=0;
        for(int i=0;i<n&&valid;++i){
            if(fscanf(in,"%d",&p[i])!=1||p[i]<1||p[i]>1000000000)valid=0;
            else if(p[i]<minP)minP=p[i];
        }
        for(int i=0;i<n&&valid;++i){
            if(fscanf(in,"%d",&c[i])!=1||c[i]<1||c[i]>1000000000)valid=0;
            else if(c[i]>maxC)maxC=c[i];
        }
        fclose(in);
        if(!valid||maxC>=minP){fprintf(stderr,"Invalid input: %s\n",path);
            free(p);free(c);free(copy);goto error;}

        /* Compute independent oracle answer and self-check known-answer cases. */
        int expectedK;
        Result expected=oracle(n,d,p,c,&expectedK);
        if(expected.count<0){free(p);free(c);free(copy);goto error;}
        if((strcmp(profile,"sample")==0&&(expected.count!=8||expected.money!=2))||
           (strcmp(profile,"original_price")==0&&(expected.count!=2||expected.money!=4))){
            fprintf(stderr,"Oracle self-check failed\n");free(p);free(c);free(copy);goto error;
        }

        /* Untimed warm-up run; copy `c[]` each time because `quickSort` mutates in place. */
        memcpy(copy,c,(size_t)n*sizeof(*copy));
        int k;
        Result got=solveShoppingCounted(n,d,p,copy,&k);
        int pass=got.count==expected.count&&got.money==expected.money&&k==expectedK;

        /* Timed measurement loop across `repeats` fresh solver invocations. */
        double sum=0,best=1e100,worst=0;
        for(int r=0;r<repeats;++r){
            memcpy(copy,c,(size_t)n*sizeof(*copy));
            double start=milliseconds();
            got=solveShoppingCounted(n,d,p,copy,&k);
            double elapsed=milliseconds()-start;
            sum+=elapsed;if(elapsed<best)best=elapsed;if(elapsed>worst)worst=elapsed;
            if(got.count!=expected.count||got.money!=expected.money||k!=expectedK)pass=0;
        }

        fprintf(out,"%s,%s,%d,%d,%u,%d,%d,%d,%d,%d,%d,%s,%.6f,%.6f,%.6f\n",
                path,profile,n,d,seed,repeats,got.count,k,got.money,
                expected.count,expected.money,pass?"PASS":"FAIL",sum/repeats,best,worst);
        ++total;if(!pass)++failed;
        printf("[%d] %s: %s (%.3f ms)\n",total,path,pass?"PASS":"FAIL",sum/repeats);
        free(p);free(c);free(copy);
    }

    {int ioError=ferror(manifest)||ferror(out);
     if(fclose(manifest)!=0)ioError=1;
     if(fclose(out)!=0)ioError=1;
     if(ioError)return 1;}
    printf("Completed: %d cases, %d failures. See output/results.csv\n",total,failed);
    return failed?1:0;
error:
    fprintf(stderr,"Benchmark aborted: input, allocation or oracle error.\n");
    fclose(manifest);fclose(out);return 1;
}
