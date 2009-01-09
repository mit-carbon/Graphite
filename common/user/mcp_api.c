#include "mcp_api.h"
#include "capi.h"
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>


// Globals required for the syscall server
static pthread_t mcp_thread;


void initMCP()
{
   // Create the syscall server thread
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   pthread_create(&mcp_thread, &attr, mcp_thread_func, (void *) 0);
}

void* mcp_thread_func(void *dummy)
{
   fprintf(stderr, "Made it to the dummy mcp_thread_func() function.\n");
//   assert(false);
   return NULL;
}

void runMCP()
{
   fprintf(stderr, "Made it to the dummy runMCP() function.\n");
//   assert(false);
}

void finishMCP()
{
   fprintf(stderr, "Made it to the dummy finishMCP() function.\n");
}

void quitMCP()
{
   finishMCP();
   pthread_join(mcp_thread, NULL);
   fprintf(stderr, "MCP Thread has joined...\n");
}

