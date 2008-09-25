
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include "capi.h"

using namespace std;


volatile int flag = 0;

static void alarm_handler(int sig)
{
  flag = 1;
  printf("received signal\n"); 
}

void *start(void *v)
{
  if(v != NULL){
    printf("setting alarm\n");
    (void)signal(SIGALRM, &alarm_handler);
    (void)alarm(2);
  }
  printf("waiting for signal at %p\n", &flag);
  while(!flag);
  return NULL;
}

int main(int argc, char* argv[])
{
  pthread_t thrd1, thrd2;
  pthread_create(&thrd1, NULL, start, NULL);
  pthread_create(&thrd2, NULL, start, (void *)1);
  pthread_join(thrd1, NULL);
  pthread_join(thrd2, NULL);
}
