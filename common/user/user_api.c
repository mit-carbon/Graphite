#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "user_api.h"

void carbonInit()
{
}

void carbonFinish()
{
}

int getCoreCount()
{
    fprintf(stderr, "Got to the dummy core count function.\n");
    return -1;
}

