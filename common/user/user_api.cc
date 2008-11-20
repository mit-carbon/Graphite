#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "user_api.h"

//FIXME: This will not work -- we need to store  this pointer 
//in the pin tool as well as managing the memory

static pthread_t *sh_mem_threads;

void carbonInit()
{

    //Start the shared memory threads
    int core_count = getCoreCount();
    printf("Creating [%d] shared memory threads\n", core_count);

    sharedMemThreadsInit();
    initMCP();
}

void carbonFinish()
{
    quitMCP();
    sharedMemThreadsFinish();
}

int getCoreCount()
{
    fprintf(stderr, "Got to the dummy core count function.\n");
    return -1;
}

void sharedMemThreadsInit()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Subtract 1 for the MCP
    int core_count = getCoreCount();
    sh_mem_threads = (pthread_t *)malloc(core_count * sizeof(pthread_t));

    // Now spawn each shared memory thread
    fprintf(stderr, "core count is: %d\n", core_count);
    int i;
    for(i = 0; i < core_count; i++)
    {
        pthread_create(&sh_mem_threads[i], &attr, shmem_thread_func, (void *) 0);
    }
}

void sharedMemThreadsFinish()
{
    //Sets the flag to quit these threads
    sharedMemQuit();

    // Subtract 1 for the MCP
    int core_count = getCoreCount();
    int i;

    fprintf(stderr, "core count is: %d\n", core_count);
    for(i = 0; i < core_count; i++)
    {
        pthread_join(sh_mem_threads[i], NULL);
    }
    free(sh_mem_threads);
}

//dummy functions to be replaced by the simulator
void sharedMemQuit()
{
    fprintf(stderr, "Got to the dummy shared memory quit func.\n");
}

void* shmem_thread_func(void *)
{
    fprintf(stderr, "Got to the dummy shared memory thread func.\n");
    return NULL;
}

