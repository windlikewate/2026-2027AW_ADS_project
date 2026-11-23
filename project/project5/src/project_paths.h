#ifndef PROJECT_PATHS_H
#define PROJECT_PATHS_H
/* Include before other headers for the POSIX readlink declaration. */
#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#define PROJECT_CHDIR(p) _chdir(p)
#else
#include <unistd.h>
#include <sys/stat.h>
#define MKDIR(p) mkdir(p,0755)
#define PROJECT_CHDIR(p) chdir(p)
#endif

/* Binaries live in src/ (or the project root). Ignore the launch directory. */
static int enterProjectRoot(const char *argv0){
    char executable[4096];
#ifdef _WIN32
    DWORD length=GetModuleFileNameA(NULL,executable,(DWORD)sizeof(executable));
    (void)argv0;
    if(length==0||length>=sizeof(executable)){
        fprintf(stderr,"Cannot locate executable.\n");return 0;
    }
#elif defined(__linux__)
    ssize_t length=readlink("/proc/self/exe",executable,sizeof(executable)-1);
    (void)argv0;
    if(length<0){perror("/proc/self/exe");return 0;}
    if((size_t)length>=sizeof(executable)-1)return 0;
    executable[length]='\0';
#else
    if(strlen(argv0)>=sizeof(executable))return 0;
    strcpy(executable,argv0);
#endif
    char *separator=NULL;
    for(char *p=executable;*p;++p)if(*p=='/'||*p=='\\')separator=p;
    if(separator){
        *separator='\0';
        if(PROJECT_CHDIR(executable)!=0){perror(executable);return 0;}
    }
    FILE *marker=fopen("src/generate.c","r");
    if(marker){fclose(marker);return 1;}
    marker=fopen("generate.c","r");
    if(marker){
        fclose(marker);
        if(PROJECT_CHDIR("..")!=0){perror("project root");return 0;}
        marker=fopen("src/generate.c","r");
        if(marker){fclose(marker);return 1;}
    }
    fprintf(stderr,"Cannot locate project root; build binaries in src/.\n");
    return 0;
}
#endif
