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
static int ascending(const void *a,const void *b){
    int x=*(const int*)a,y=*(const int*)b;return (x>y)-(x<y);
}
typedef struct {int64_t count,cost;} Stats;
/*
 * Independent oracle: count all price-coupon pairs costing <= t.
 * Sorted arrays + coupon prefix sums allow each query in O(N).
 * At most D/base original-price purchases can ever be affordable.
 */
static Stats statsAt(int n,int d,const int *p,const int *c,
                     const int64_t *prefix,int t){
    Stats st={0,0};int j=0;
    for(int i=0;i<n;++i){
        while(j<n&&(int64_t)c[j]<(int64_t)p[i]-t)++j;
        int64_t k=n-j;
        st.count+=k;
        st.cost+=k*p[i]-(prefix[n]-prefix[j]);
    }
    if(t>=p[0]){
        int64_t k=d/p[0];st.count+=k;st.cost+=k*p[0];
    }
    return st;
}
static Result oracle(int n,int d,const int *prices,const int *coupons,int *expectedK){
    int *p=malloc((size_t)n*sizeof(*p)),*c=malloc((size_t)n*sizeof(*c));
    int64_t *prefix=malloc(((size_t)n+1)*sizeof(*prefix));
    if(!p||!c||!prefix){free(p);free(c);free(prefix);return (Result){-1,-1};}
    memcpy(p,prices,(size_t)n*sizeof(*p));memcpy(c,coupons,(size_t)n*sizeof(*c));
    qsort(p,(size_t)n,sizeof(*p),ascending);qsort(c,(size_t)n,sizeof(*c),ascending);
    prefix[0]=0;
    for(int i=0;i<n;++i)prefix[i+1]=prefix[i]+c[i];
    int low=0,high=d;
    while(low<high){
        int mid=low+(high-low+1)/2;
        if(statsAt(n,d,p,c,prefix,mid).cost<=d)low=mid;
        else high=mid-1;
    }
    Stats st=statsAt(n,d,p,c,prefix,low);
    int64_t extra=0;
    if(low<d){
        Stats next=statsAt(n,d,p,c,prefix,low+1);
        extra=(d-st.cost)/(low+1);
        if(extra>next.count-st.count)extra=next.count-st.count;
    }
    Result r={(int)(st.count+extra),(int)(d-st.cost-extra*(low+1))};
    /* Strictly cheaper than base: these pairs precede bulk purchases. */
    int64_t cheapPairs=statsAt(n,d,p,c,prefix,p[0]-1).count;
    *expectedK=cheapPairs<r.count?(int)cheapPairs:r.count;
    free(p);free(c);free(prefix);return r;
}
int main(int argc,char **argv){
    if(!enterProjectRoot(argv[0]))return 1;
    int repeats=3;
    if(argc>1){
        char *end;long value=strtol(argv[1],&end,10);
        if(*end||value<1||value>100){fprintf(stderr,"repeats must be 1..100\n");return 1;}
        repeats=(int)value;
    }
    FILE *manifest=fopen("data/manifest.csv","r");
    if(!manifest){perror("Run generate first");return 1;}
    if(MKDIR("output")!=0&&errno!=EEXIST){perror("output");fclose(manifest);return 1;}
    FILE *out=fopen("output/results.csv","w");
    if(!out){perror("output/results.csv");fclose(manifest);return 1;}
    fprintf(out,"file,profile,n,d,seed,repeats,items,k,remaining,expected_items,expected_remaining,status,avg_ms,min_ms,max_ms\n");
    char line[512],path[160],profile[64];int n,d,total=0,failed=0;unsigned seed;
    if(!fgets(line,sizeof(line),manifest)){fclose(out);fclose(manifest);return 1;}
    while(fgets(line,sizeof(line),manifest)){
        if(sscanf(line,"%159[^,],%63[^,],%d,%d,%u",path,profile,&n,&d,&seed)!=5)goto error;
        if(n<1||n>100000||d<1||d>1000000)goto error;
        int *p=malloc((size_t)n*sizeof(*p)),*c=malloc((size_t)n*sizeof(*c));
        int *copy=malloc((size_t)n*sizeof(*copy));
        if(!p||!c||!copy){free(p);free(c);free(copy);goto error;}
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
        int expectedK;
        Result expected=oracle(n,d,p,c,&expectedK);
        if(expected.count<0){free(p);free(c);free(copy);goto error;}
        /* Known-answer checks also validate two basic oracle cases. */
        if((strcmp(profile,"sample")==0&&(expected.count!=8||expected.money!=2))||
           (strcmp(profile,"original_price")==0&&(expected.count!=2||expected.money!=4))){
            fprintf(stderr,"Oracle self-check failed\n");free(p);free(c);free(copy);goto error;
        }
        /* Warm-up is excluded from timing. Always restore unsorted coupons. */
        memcpy(copy,c,(size_t)n*sizeof(*copy));
        int k;
        Result got=solveShoppingCounted(n,d,p,copy,&k);
        int pass=got.count==expected.count&&got.money==expected.money&&k==expectedK;
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
