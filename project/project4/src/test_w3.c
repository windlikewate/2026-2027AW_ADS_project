/* Standalone, deterministic correctness checks; not included in benchmark time.
 * Compile: gcc -std=c11 -O2 -Wall -Wextra src/test_w3.c -o src/test_w3.exe
 */
#include "Knapsack_w3.h"
#include <stdio.h>
#include <string.h>

static uint32_t state = 20261121;
static unsigned draw(unsigned n) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return state % n;
}
/* Independent capacity DP oracle, using descending updates for 0/1 items. */
static int reference(int n, int c, int *p, int *w) {
    int *dp = calloc((size_t)c+1, sizeof(*dp));
    if (!dp) exit(2);
    for (int i = 0; i < n; ++i)
        for (int t = c; t >= w[i]; --t)
            if (dp[t-w[i]]+p[i] > dp[t]) dp[t] = dp[t-w[i]]+p[i];
    int result = dp[c]; free(dp); return result;
}
static int check(int n, int c, int *p, int *w) {
    int pc[256], wc[256];
    memcpy(pc, p, (size_t)n*sizeof(int)); memcpy(wc, w, (size_t)n*sizeof(int));
    int want = reference(n, c, p, w), got = knapsack_w3(n, c, p, w);
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
    for (int trial=0; trial<3000; ++trial) {
        int64_t old[80], gain[80]; size_t rows[80], cols[80], ans[80];
        size_t n=1+draw(80), k=draw((unsigned)n), nc=0;
        int slope=(int)draw(30);
        gain[0]=0;
        for (size_t i=1;i<=k;++i) { slope-=(int)draw(4); gain[i]=gain[i-1]+slope; }
        for (size_t i=0;i<n;++i) {
            rows[i]=i; old[i]=draw(4) ? (int)draw(201)-100 : KW3_NEG;
            if(old[i]!=KW3_NEG) cols[nc++]=i;
        }
        if(!nc) continue;
        KW3Matrix m={old,gain,1,0,k};
        if(!kw3_smawk(&m,rows,n,cols,nc,ans)) return 0;
        for(size_t i=0;i<n;++i) {
            int64_t want=KW3_NEG, got; size_t d;
            for(size_t j=0;j<nc;++j)
                if(cols[j]<=i && i-cols[j]<=k && old[cols[j]]+gain[i-cols[j]]>want)
                    want=old[cols[j]]+gain[i-cols[j]];
            kw3_entry(&m,i,ans[i],&d,&got);
            if(d) got=KW3_NEG;
            if(got!=want) { fprintf(stderr,"SMAWK mismatch trial=%d row=%zu\n",trial,i); return 0; }
        }
    }
    return 1;
}
int main(void) {
    int p[256], w[256];
    if(!convolution_checks()) return 1;
    /* Empty, zero capacity, all fit, all too heavy, and an exchange needed. */
    if(knapsack_w3(0,0,NULL,NULL)!=0) return 1;
    int ep[]={10,9,9,0}, ew[]={6,5,5,1};
    if(!check(4,10,ep,ew) || !check(4,0,ep,ew) || !check(4,100,ep,ew)) return 1;
    int hp[]={INT_MAX-1,1}, hw[]={2,1};
    if(!check(2,2,hp,hw)) return 1;
    /* Tiny instances: capacity DP and exhaustive enumeration must agree. */
    for(int t=0;t<500;++t) {
        int n=1+(int)draw(12), c=(int)draw(70), best=0;
        for(int i=0;i<n;++i) { w[i]=1+(int)draw(15); p[i]=(int)draw(50); }
        for(unsigned mask=0;mask<(1u<<n);++mask) {
            int weight=0, profit=0;
            for(int i=0;i<n;++i) if(mask&(1u<<i)) { weight+=w[i];profit+=p[i]; }
            if(weight<=c && profit>best) best=profit;
        }
        if(reference(n,c,p,w)!=best || !check(n,c,p,w)) return 1;
    }
    /* Many equal weights/densities, gcd gaps, zero profits, and large n/W. */
    for(int t=0;t<5000;++t) {
        int n=1+(int)draw(256), W=1+(int)draw(24), total=0;
        for(int i=0;i<n;++i) {
            w[i]=1+(int)draw((unsigned)W);
            if(t%5==0) w[i]*=3;
            p[i]=t%4==0 ? w[i] : (int)draw(200);
            total+=w[i];
        }
        if(!check(n,(int)draw((unsigned)total+10),p,w)) return 1;
    }
    /* Large n, tiny W and identical densities exercise BFPRT tie handling
     * without requiring an O(n*c) oracle: optimum is exactly capacity. */
    int *large_p=malloc(100000*sizeof(int)), *large_w=malloc(100000*sizeof(int));
    if(!large_p || !large_w) return 2;
    for(int i=0;i<100000;++i) large_p[i]=large_w[i]=1;
    int large_result=knapsack_w3(100000,50000,large_p,large_w);
    free(large_p); free(large_w);
    if(large_result!=50000) return 1;
    /* Error contracts must not turn invalid inputs into plausible optima. */
    int bad_w=0, good_p=1, overflow_p[]={INT_MAX,1}, good_w[]={1,1};
    if(knapsack_w3(1,1,&good_p,&bad_w)!=-1 || errno!=EINVAL) return 1;
    if(knapsack_w3(2,1,overflow_p,good_w)!=-1 || errno!=EOVERFLOW) return 1;
    puts("Passed: 3000 sparse concave convolutions, 500 exhaustive instances, 5000 random DP comparisons, edge cases.");
    return 0;
}
