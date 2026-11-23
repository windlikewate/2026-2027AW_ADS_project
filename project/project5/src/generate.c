#include "project_paths.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
static uint32_t state;
static uint32_t random32(void){
    state^=state<<13;state^=state>>17;state^=state<<5;return state;
}
/* Rejection sampling avoids modulo bias. */
static int between(int low,int high){
    uint32_t bound=(uint32_t)(high-low)+1u;
    uint32_t threshold=(uint32_t)(0u-bound)%bound,x;
    do{x=random32();}while(x<threshold);
    return low+(int)(x%bound);
}
static void writeCase(const char *path,int n,int d,const int *p,const int *c){
    FILE *f=fopen(path,"w");
    if(!f){perror(path);exit(1);}
    fprintf(f,"%d %d\n",n,d);
    for(int i=0;i<n;++i)fprintf(f,"%d%c",p[i],i==n-1?'\n':' ');
    for(int i=0;i<n;++i)fprintf(f,"%d%c",c[i],i==n-1?'\n':' ');
    if(ferror(f)){fprintf(stderr,"Write failed: %s\n",path);fclose(f);exit(1);}
    if(fclose(f)!=0){perror(path);exit(1);}
}
int main(int argc,char **argv){
    (void)argc;
    if(!enterProjectRoot(argv[0]))return 1;
    const int sizes[]={16,32,64,100,256,512,1000,2000,5000,10000,20000,50000,100000};
    const char *names[]={"random","duplicates","near_limit","unaffordable",
                         "many_purchases","ordered"};
    if(MKDIR("data")!=0&&errno!=EEXIST){perror("data");return 1;}
    FILE *manifest=fopen("data/manifest.csv","w");
    if(!manifest){perror("data/manifest.csv");return 1;}
    fprintf(manifest,"file,profile,n,d,seed\n");
    int *p=malloc(100000*sizeof(*p)),*c=malloc(100000*sizeof(*c));
    if(!p||!c){free(p);free(c);fclose(manifest);return 1;}
    int cases=0;
    for(size_t s=0;s<sizeof(sizes)/sizeof(sizes[0]);++s)for(int mode=0;mode<6;++mode)for(int trial=1;trial<=2;++trial){
        int n=sizes[s],d=1000000;
        /* Keep the original five sizes' seeds and filenames unchanged. */
        int slot=n==16?0:n==100?1:n==1000?2:n==10000?3:n==100000?4:10+(int)s;
        uint32_t seed=20261123u+(uint32_t)(slot*100+mode*10+trial);
        state=seed;
        for(int i=0;i<n;++i){
            switch(mode){
            case 0:p[i]=between(1000,2000);c[i]=between(1,999);break;
            case 1:p[i]=between(100,105);c[i]=between(95,99);break;
            case 2:p[i]=between(999999900,1000000000);
                   c[i]=between(999999800,999999899);break;
            case 3:p[i]=between(500000000,1000000000);
                   c[i]=between(1,1000000);break;
            case 4:p[i]=between(2,3);c[i]=1;break;
            default:p[i]=100001+i;
                    c[i]=trial==1?i+1:n-i;break;
            }
        }
        char path[160];
        snprintf(path,sizeof(path),"data/%s_n%d_s%u.in",names[mode],n,(unsigned)seed);
        writeCase(path,n,d,p,c);
        fprintf(manifest,"%s,%s,%d,%d,%u\n",path,names[mode],n,d,(unsigned)seed);
        ++cases;
    }
    {int prices[]={12,20,15,10},coupons[]={9,6,8,7};
     writeCase("data/sample.in",4,30,prices,coupons);
     fprintf(manifest,"data/sample.in,sample,4,30,0\n");++cases;}
    {int prices[]={5},coupons[]={4};
     writeCase("data/original_price.in",1,10,prices,coupons);
     fprintf(manifest,"data/original_price.in,original_price,1,10,0\n");++cases;}
    free(p);free(c);
    if(ferror(manifest)){fclose(manifest);return 1;}
    if(fclose(manifest)!=0)return 1;
    printf("Generated %d cases and data/manifest.csv\n",cases);
    return 0;
}
