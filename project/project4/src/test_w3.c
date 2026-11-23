/* Standalone, deterministic correctness checks; not included in benchmark time.
 * Covers SMAWK concave convolution, corner cases, exhaustive enumeration on
 * small instances, random 1D DP comparisons, tie handling, and errno checks.
 * Compile: gcc -std=c11 -O2 -Wall -Wextra src/test_w3.c -o src/test_w3.exe
 */
#include "Knapsack_w3.h"
#include <stdio.h>
#include <string.h>

/* Fixed xorshift32 seed for deterministic test generation. */
static uint32_t state = 20261121;

/* Return a pseudo-random integer in [0, n - 1] using xorshift32 updates. */
static unsigned draw(unsigned n) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return state % n;
}

/* Independent capacity DP oracle, using descending updates for 0/1 items. */
static int reference(int n, int c, int *p, int *w) {
    /* Allocate zero-initialized 1D DP table dp[0..c]. */
    int *dp = calloc((size_t)c+1, sizeof(*dp));
    if (!dp) exit(2);
    /* Update 1D DP array from c down to w[i] for each item i. */
    for (int i = 0; i < n; ++i)
        for (int t = c; t >= w[i]; --t)
            if (dp[t-w[i]]+p[i] > dp[t]) dp[t] = dp[t-w[i]]+p[i];
    /* Save optimal profit at capacity c and free temporary buffer. */
    int result = dp[c]; free(dp); return result;
}

/* Compare knapsack_w3 against reference DP and check that inputs stay unchanged. */
static int check(int n, int c, int *p, int *w) {
    /* Copy caller profit and weight arrays before calling knapsack_w3. */
    int pc[256], wc[256];
    memcpy(pc, p, (size_t)n*sizeof(int)); memcpy(wc, w, (size_t)n*sizeof(int));
    int want = reference(n, c, p, w), got = knapsack_w3(n, c, p, w);
    /* Report mismatch if optimal value differs or input arrays were modified. */
    if (got != want || memcmp(pc,p,(size_t)n*sizeof(int)) || memcmp(wc,w,(size_t)n*sizeof(int))) {
        fprintf(stderr, "Mismatch: n=%d c=%d expected=%d got=%d\n", n,c,want,got);
        for (int i=0;i<n;++i) fprintf(stderr,"%d %d\n",p[i],w[i]);
        return 0;
    }
    return 1;
}

/* Directly test SMAWK against quadratic enumeration. Sparse old arrays,
 * band boundaries, negative gains and equal maxima exercise padding/ties. */
static int convolution_checks(void) {
    /* Run 3,000 randomized trials with varying row counts and band widths. */
    for (int trial=0; trial<3000; ++trial) {
        int64_t old[80], gain[80]; size_t rows[80], cols[80], ans[80];
        size_t n=1+draw(80), k=draw((unsigned)n), nc=0;
        int slope=(int)draw(30);
        /* Build a concave gain array by non-increasing step slopes. */
        gain[0]=0;
        for (size_t i=1;i<=k;++i) { slope-=(int)draw(4); gain[i]=gain[i-1]+slope; }
        /* Fill old[] with random values and unreachable KW3_NEG entries. */
        for (size_t i=0;i<n;++i) {
            rows[i]=i; old[i]=draw(4) ? (int)draw(201)-100 : KW3_NEG;
            if(old[i]!=KW3_NEG) cols[nc++]=i;
        }
        /* Skip degenerate trials where every input state was unreachable. */
        if(!nc) continue;
        /* Run kw3_smawk on the implicit matrix with step h = 1, residue = 0. */
        KW3Matrix m={old,gain,1,0,k};
        if(!kw3_smawk(&m,rows,n,cols,nc,ans)) return 0;
        /* Verify each row maximum against direct enumeration over reachable cols. */
        for(size_t i=0;i<n;++i) {
            int64_t want=KW3_NEG, got; size_t d;
            for(size_t j=0;j<nc;++j)
                if(cols[j]<=i && i-cols[j]<=k && old[cols[j]]+gain[i-cols[j]]>want)
                    want=old[cols[j]]+gain[i-cols[j]];
            /* Convert out-of-band SMAWK answers back to KW3_NEG before comparing. */
            kw3_entry(&m,i,ans[i],&d,&got);
            if(d) got=KW3_NEG;
            if(got!=want) { fprintf(stderr,"SMAWK mismatch trial=%d row=%zu\n",trial,i); return 0; }
        }
    }
    return 1;
}

/* Run convolution checks, corner cases, exhaustive checks, random DP, and errno tests. */
int main(void) {
    int p[256], w[256];
    /* Stage 1: verify SMAWK concave convolution helper directly. */
    if(!convolution_checks()) return 1;
    /* Empty, zero capacity, all fit, all too heavy, and an exchange needed. */
    if(knapsack_w3(0,0,NULL,NULL)!=0) return 1;
    /* Test item exchange across the prefix break item on a 4-item example. */
    int ep[]={10,9,9,0}, ew[]={6,5,5,1};
    if(!check(4,10,ep,ew) || !check(4,0,ep,ew) || !check(4,100,ep,ew)) return 1;
    /* Check large profits near INT_MAX without 32-bit overflow. */
    int hp[]={INT_MAX-1,1}, hw[]={2,1};
    if(!check(2,2,hp,hw)) return 1;
    /* Tiny instances: capacity DP and exhaustive enumeration must agree. */
    for(int t=0;t<500;++t) {
        int n=1+(int)draw(12), c=(int)draw(70), best=0;
        /* Generate random small weights and profits for exhaustive subset search. */
        for(int i=0;i<n;++i) { w[i]=1+(int)draw(15); p[i]=(int)draw(50); }
        /* Enumerate all 2^n subsets to get the ground-truth optimum. */
        for(unsigned mask=0;mask<(1u<<n);++mask) {
            int weight=0, profit=0;
            /* Sum weight and profit of items selected by the current bitmask. */
            for(int i=0;i<n;++i) if(mask&(1u<<i)) { weight+=w[i];profit+=p[i]; }
            /* Update best profit whenever the subset fits within capacity c. */
            if(weight<=c && profit>best) best=profit;
        }
        /* Require both 1D DP and knapsack_w3 to match exhaustive enumeration. */
        if(reference(n,c,p,w)!=best || !check(n,c,p,w)) return 1;
    }
    /* Many equal weights/densities, gcd gaps, zero profits, and large n/W. */
    for(int t=0;t<5000;++t) {
        int n=1+(int)draw(256), W=1+(int)draw(24), total=0;
        /* Sample random weights, periodic multiples of 3, and equal-density cases. */
        for(int i=0;i<n;++i) {
            w[i]=1+(int)draw((unsigned)W);
            if(t%5==0) w[i]*=3;
            p[i]=t%4==0 ? w[i] : (int)draw(200);
            total+=w[i];
        }
        /* Cross-check knapsack_w3 against 1D capacity DP on random capacity. */
        if(!check(n,(int)draw((unsigned)total+10),p,w)) return 1;
    }
    /* Large n, tiny W and identical densities exercise BFPRT tie handling
     * without requiring an O(n*c) oracle: optimum is exactly capacity. */
    int *large_p=malloc(100000*sizeof(int)), *large_w=malloc(100000*sizeof(int));
    if(!large_p || !large_w) return 2;
    /* Fill 100,000 unit items with identical profit 1 and weight 1. */
    for(int i=0;i<100000;++i) large_p[i]=large_w[i]=1;
    int large_result=knapsack_w3(100000,50000,large_p,large_w);
    free(large_p); free(large_w);
    if(large_result!=50000) return 1;
    /* Error contracts must not turn invalid inputs into plausible optima.
     * Check non-positive weights (EINVAL) and INT_MAX profit sum (EOVERFLOW). */
    int bad_w=0, good_p=1, overflow_p[]={INT_MAX,1}, good_w[]={1,1};
    if(knapsack_w3(1,1,&good_p,&bad_w)!=-1 || errno!=EINVAL) return 1;
    if(knapsack_w3(2,1,overflow_p,good_w)!=-1 || errno!=EOVERFLOW) return 1;
    /* Print summary after all checks succeed. */
    puts("Passed: 3000 sparse concave convolutions, 500 exhaustive instances, 5000 random DP comparisons, edge cases.");
    return 0;
}
