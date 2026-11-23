/*
 * File: generate.c
 * Description: Deterministic benchmark dataset generator for ADS Project 5
 *              ("Shopping with Coupons"). Synthesizes 158 reproducible `.in`
 *              test files and `data/manifest.csv` across:
 *                - 13 sizes N in {16, 32, 64, 100, 256, 512, 1000, 2000,
 *                  5000, 10000, 20000, 50000, 100000} at D = 1,000,000;
 *                - 6 structural profiles (`random`, `duplicates`, `near_limit`,
 *                  `unaffordable`, `many_purchases`, `ordered`) x 2 seeds;
 *                - 2 known-answer unit cases (`sample.in`, `original_price.in`).
 */

#include "project_paths.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

/* Internal 32-bit state for Marsaglia xorshift32 PRNG. */
static uint32_t state;

/*
 * @brief Advance xorshift32 state and return the next pseudo-random 32-bit word.
 */
static uint32_t random32(void){
    state^=state<<13;state^=state>>17;state^=state<<5;return state;
}

/*
 * @brief Draw an unbiased integer uniformly from `[low, high]` via rejection
 *        sampling, eliminating `2^32 mod bound` modulo bias.
 */
static int between(int low,int high){
    uint32_t bound=(uint32_t)(high-low)+1u;
    uint32_t threshold=(uint32_t)(0u-bound)%bound,x;
    do{x=random32();}while(x<threshold);
    return low+(int)(x%bound);
}

/*
 * @brief Serialize one `(n, d, prices, coupons)` instance to disk in the exact
 *        three-line assignment input format with I/O error checking.
 */
static void writeCase(const char *path,int n,int d,const int *p,const int *c){
    FILE *f=fopen(path,"w");
    if(!f){perror(path);exit(1);}
    fprintf(f,"%d %d\n",n,d);
    for(int i=0;i<n;++i)fprintf(f,"%d%c",p[i],i==n-1?'\n':' ');
    for(int i=0;i<n;++i)fprintf(f,"%d%c",c[i],i==n-1?'\n':' ');
    if(ferror(f)){fprintf(stderr,"Write failed: %s\n",path);fclose(f);exit(1);}
    if(fclose(f)!=0){perror(path);exit(1);}
}

/*
 * @brief Entry point: resolve project root, generate all 156 scaling cases plus
 *        2 known-answer cases under `data/`, and write `data/manifest.csv`.
 */
int main(int argc,char **argv){
    (void)argc;
    if(!enterProjectRoot(argv[0]))return 1;

    /* Problem sizes spanning four orders of magnitude up to N = 100,000. */
    const int sizes[]={16,32,64,100,256,512,1000,2000,5000,10000,20000,50000,100000};
    const char *names[]={"random","duplicates","near_limit","unaffordable",
                         "many_purchases","ordered"};

    if(MKDIR("data")!=0&&errno!=EEXIST){perror("data");return 1;}
    FILE *manifest=fopen("data/manifest.csv","w");
    if(!manifest){perror("data/manifest.csv");return 1;}
    fprintf(manifest,"file,profile,n,d,seed\n");

    /* Reusable scratch buffers sized for the maximum N = 100,000. */
    int *p=malloc(100000*sizeof(*p)),*c=malloc(100000*sizeof(*c));
    if(!p||!c){free(p);free(c);fclose(manifest);return 1;}

    int cases=0;
    for(size_t s=0;s<sizeof(sizes)/sizeof(sizes[0]);++s)for(int mode=0;mode<6;++mode)for(int trial=1;trial<=2;++trial){
        int n=sizes[s],d=1000000;
        /* Preserve historical seed slots for {16, 100, 1000, 10000, 100000}. */
        int slot=n==16?0:n==100?1:n==1000?2:n==10000?3:n==100000?4:10+(int)s;
        uint32_t seed=20261123u+(uint32_t)(slot*100+mode*10+trial);
        state=seed;

        /* Populate prices `p[]` and coupons `c[]` with max(c) < min(p). */
        for(int i=0;i<n;++i){
            switch(mode){
            /* Mode 0 (`random`): general uncorrelated prices and discounts. */
            case 0:p[i]=between(1000,2000);c[i]=between(1,999);break;
            /* Mode 1 (`duplicates`): narrow value bands causing heavy ties. */
            case 1:p[i]=between(100,105);c[i]=between(95,99);break;
            /* Mode 2 (`near_limit`): values near 10^9 with small net differences. */
            case 2:p[i]=between(999999900,1000000000);
                   c[i]=between(999999800,999999899);break;
            /* Mode 3 (`unaffordable`): net costs > D forcing K = 0 purchases. */
            case 3:p[i]=between(500000000,1000000000);
                   c[i]=between(1,1000000);break;
            /* Mode 4 (`many_purchases`): identical coupons and up to 10^6 units. */
            case 4:p[i]=between(2,3);c[i]=1;break;
            /* Mode 5 (`ordered`): ascending prices with monotone coupons. */
            default:p[i]=100001+i;
                    c[i]=trial==1?i+1:n-i;break;
            }
        }
        /* Write `.in` file and append its metadata row to `data/manifest.csv`. */
        char path[160];
        snprintf(path,sizeof(path),"data/%s_n%d_s%u.in",names[mode],n,(unsigned)seed);
        writeCase(path,n,d,p,c);
        fprintf(manifest,"%s,%s,%d,%d,%u\n",path,names[mode],n,d,(unsigned)seed);
        ++cases;
    }

    /* Known-answer Case 1: Problem statement sample (expected output: 8 2). */
    {int prices[]={12,20,15,10},coupons[]={9,6,8,7};
     writeCase("data/sample.in",4,30,prices,coupons);
     fprintf(manifest,"data/sample.in,sample,4,30,0\n");++cases;}

    /* Known-answer Case 2: Unlimited full-price rule (expected output: 2 4). */
    {int prices[]={5},coupons[]={4};
     writeCase("data/original_price.in",1,10,prices,coupons);
     fprintf(manifest,"data/original_price.in,original_price,1,10,0\n");++cases;}

/* Release scratch buffers, flush manifest, and print total case count. */
    free(p);free(c);
    if(ferror(manifest)){fclose(manifest);return 1;}
    if(fclose(manifest)!=0)return 1;
    printf("Generated %d cases and data/manifest.csv\n",cases);
    return 0;
}
