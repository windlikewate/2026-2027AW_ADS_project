/*
 * File: project_paths.h
 * Description: Cross-platform working-directory resolver ensuring that both
 *              `generate` and `testbench` locate the project root reliably
 *              regardless of the shell directory from which they are launched.
 *
 * Supported Platforms:
 *   - Windows (`_WIN32`): resolves executable path via `GetModuleFileNameA`.
 *   - Linux (`__linux__`): resolves `/proc/self/exe` via POSIX `readlink`.
 *   - Fallback POSIX/macOS: inspects `argv[0]` relative path prefix.
 */

#ifndef PROJECT_PATHS_H
#define PROJECT_PATHS_H

/* Expose POSIX.1-2008 `readlink` declaration before including standard headers. */
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

/*
 * @brief Change the process working directory to the project root folder
 *        (the parent directory containing `src/generate.c`).
 *
 * Resolution Strategy:
 *   1. Obtain the absolute or launch-relative path of the current executable.
 *   2. `chdir` into the directory containing that executable.
 *   3. Check whether `src/generate.c` exists directly (executable in root) or
 *      whether `generate.c` exists locally (executable in `src/`, requiring
 *      one `chdir("..")` step up to the project root).
 *
 * @param argv0 Fallback `argv[0]` string used on non-Linux, non-Windows targets.
 * @return 1 on success (current working directory is now project root), 0 on error.
 */
static int enterProjectRoot(const char *argv0){
    char executable[4096];
#ifdef _WIN32
    /* Query full Win32 module path of the running `.exe` binary. */
    DWORD length=GetModuleFileNameA(NULL,executable,(DWORD)sizeof(executable));
    (void)argv0;
    if(length==0||length>=sizeof(executable)){
        fprintf(stderr,"Cannot locate executable.\n");return 0;
    }
#elif defined(__linux__)
    /* Resolve symbolic link `/proc/self/exe` on Linux / WSL2 environments. */
    ssize_t length=readlink("/proc/self/exe",executable,sizeof(executable)-1);
    (void)argv0;
    if(length<0){perror("/proc/self/exe");return 0;}
    if((size_t)length>=sizeof(executable)-1)return 0;
    executable[length]='\0';
#else
    /* Fallback to `argv[0]` path prefix when OS-specific procfs is unavailable. */
    if(strlen(argv0)>=sizeof(executable))return 0;
    strcpy(executable,argv0);
#endif
    /* Strip the binary filename so `executable` holds its parent directory. */
    char *separator=NULL;
    for(char *p=executable;*p;++p)if(*p=='/'||*p=='\\')separator=p;
    if(separator){
        *separator='\0';
        if(PROJECT_CHDIR(executable)!=0){perror(executable);return 0;}
    }
    /* Case A: Binary was placed directly in the project root directory. */
    FILE *marker=fopen("src/generate.c","r");
    if(marker){fclose(marker);return 1;}
    /* Case B: Binary was built inside `src/`; step one directory up (`..`). */
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

#endif /* PROJECT_PATHS_H */
