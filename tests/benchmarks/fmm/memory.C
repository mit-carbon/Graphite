/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

#include <float.h>
#include "defs.h"
#include "memory.h"

MAIN_ENV

g_mem *G_Memory;
local_memory Local[MAX_PROCS];

/*
 *  InitGlobalMemory ()
 *
 *  Args : none.
 *
 *  Returns : nothing.
 *
 *  Side Effects : Allocates all the global storage for G_Memory.
 *
 */
void
InitGlobalMemory ()
{
   G_Memory = (g_mem *) G_MALLOC(sizeof(g_mem));
   G_Memory->i_array = (long *) G_MALLOC(Number_Of_Processors * sizeof(long));
   G_Memory->d_array = (double *) G_MALLOC(Number_Of_Processors * sizeof(double));
   if (G_Memory == NULL) {
      printf("Ran out of global memory in InitGlobalMemory\n");
      exit(-1);
   }
   G_Memory->count = 0;
   G_Memory->id = 0;
   LOCKINIT(G_Memory->io_lock);
   LOCKINIT(G_Memory->mal_lock);
   LOCKINIT(G_Memory->single_lock);
   LOCKINIT(G_Memory->count_lock);
   ALOCKINIT(G_Memory->lock_array, MAX_LOCKS);
   BARINIT(G_Memory->synch, Number_Of_Processors);
   G_Memory->max_x = -MAX_REAL;
   G_Memory->min_x = MAX_REAL;
   G_Memory->max_y = -MAX_REAL;
   G_Memory->min_y = MAX_REAL;
}


