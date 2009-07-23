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

EXTERN_ENV

#include <math.h>
#include "matrix.h"

#define MISS_COST 4.74
#define ALPHA 100
#define BETA 10
#define TIME(ops,misses) (ops+(misses)*MISS_COST)/(1.0+MISS_COST/BS)

extern BMatrix LB;
extern long P;
extern long BS;
double *opStats = NULL;
double seq_time, seq_ops, seq_misses;

void PDIV(long src_col, long src_nz, double *ops, double *misses, double *runtime)
{
  long super_size, passes;
  double this_ops, this_misses;

  super_size = src_col*src_nz - src_col*(src_col-1)/2;
  this_ops = 1.0*src_col*(src_col+1)*(2*src_col+1)/6;
  this_ops += 1.0*src_col*src_col*(src_nz-src_col);
  *ops += this_ops;

  passes = (src_col+BS-1)/BS;
  this_misses = 2.0*passes/3*super_size;
  *misses += this_misses;

  *runtime += TIME(this_ops, this_misses);
}


void PMOD(long src_col, long dest_col, long dest_nz, double *ops, double *misses, double *runtime)
{
  long update_size, passes_src, passes_dest;
  double this_ops, this_misses;

  update_size = dest_col*dest_nz - dest_col*(dest_col-1)/2;
  this_ops = 2.0*src_col*update_size;
  *ops += this_ops;

  passes_src = (src_col+BS-1)/BS;
  passes_dest =  (dest_col+BS-1)/BS;
  if (passes_src == 1 && passes_dest == 1)
    /* just miss on source and dest once */
    this_misses = 1.0*src_col*dest_nz + 1.0*update_size;
  else
    this_misses = 1.0*passes_dest*src_col*dest_nz + 1.0*passes_src*update_size;
  *misses += this_misses;

  *runtime += TIME(this_ops, this_misses);
}

void PADD(long cols, long rows, double *misses, double *runtime)
{
  double this_misses;

  this_misses = 2*(rows*cols-cols*(cols-1)/2);
  *misses += this_misses;

  *runtime += TIME(0, this_misses);
}


void AssignBlocksNow()
{
  long i, j;

  if (P == 1) {
    for (j=0; j<LB.n; j+=LB.partition_size[j])
      if (!LB.domain[j])
	for (i=LB.col[j]; i<LB.col[j+1]; i++)
	  BLOCK(i)->owner = 0;
  } else {
    EmbedBlocks();
  }
}


void EmbedBlocks()
{
  long j, block;
  extern long scatter_decomposition;

  /* number the block partitions */

  for (j=0; j<LB.n; j+=LB.partition_size[j])
    if (!LB.domain[j]) {
      for (block=LB.col[j]; block<LB.col[j+1]; block++) {
	BLOCK(block)->owner = EmbeddedOwner(block);
      }
    }

  scatter_decomposition = 1;
}

