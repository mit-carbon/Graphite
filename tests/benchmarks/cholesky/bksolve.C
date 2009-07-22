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

#include "matrix.h"
#include <math.h>

double *TriBSolve(BMatrix LB, double *b, long *PERM)
{
  long i, j, i1, j1, bl, row;
  double *y, *xp, *x, *bt;

  x = (double *) malloc(LB.n*sizeof(double));
  xp = (double *) malloc(LB.n*sizeof(double));
  y = (double *) malloc(LB.n*sizeof(double));
  bt = (double *) malloc(LB.n*sizeof(double));

  for (j=0; j<LB.n; j++)
    bt[j] = b[j];

  /* forward solve */

  for (j=0; j<LB.n; j+=LB.partition_size[j]) {
    if (LB.domain[j]) {
      y[j] = bt[PERM[j]]/LB.entry[LB.col[j]].nz;
      for (i=LB.col[j]+1; i<LB.col[j+1]; i++)
        bt[PERM[LB.row[i]]] -= LB.entry[i].nz*y[j];
    }
    else {
      /* diagonal block */
      bl = LB.col[j];
      for (j1=j; j1<j+LB.partition_size[j]; j1++) {
	y[j1] = bt[PERM[j1]]/
	        LB.entry[bl].block->nz[(j1-j)+(j1-j)*LB.entry[bl].block->length];
        for (i1=j1-j+1; i1<LB.entry[bl].block->length; i1++) {
	  if (LB.entry[bl].block->structure)
	    row = LB.row[bl] + LB.entry[bl].block->structure[i1];
	  else row = LB.row[bl] + i1;
	  bt[PERM[row]] -= LB.entry[bl].block->nz[
		i1+(j1-j)*LB.entry[bl].block->length] * y[j1];
	}
      }
      /* blocks below diagonal */
      for (bl=LB.col[j]+1; bl<LB.col[j+1]; bl++) {
	for (i1=0; i1<LB.entry[bl].block->length; i1++) {
	  if (LB.entry[bl].block->structure)
	    row = LB.row[bl] + LB.entry[bl].block->structure[i1];
	  else row = LB.row[bl] + i1;
	  for (j1=j; j1<j+LB.partition_size[j]; j1++) {
	    bt[PERM[row]] -= LB.entry[bl].block->nz[
		i1+(j1-j)*LB.entry[bl].block->length] * y[j1];
	  }
	}
      }
    }
  }

  /* back solve */

  j = LB.n-1;
  if (LB.partition_size[j] < 0)
    j += LB.partition_size[j];

  while (j >= 0) {
    if (LB.domain[j]) {
      for (i=LB.col[j]+1; i<LB.col[j+1]; i++)
        y[j] -= LB.entry[i].nz*xp[LB.row[i]];
      xp[j] = y[j]/LB.entry[LB.col[j]].nz;
    }
    else {
      /* blocks below diagonal */
      for (bl=LB.col[j]+1; bl<LB.col[j+1]; bl++) {
        for (i1=0; i1<LB.entry[bl].block->length; i1++) {
	  if (LB.entry[bl].block->structure)
	    row = LB.row[bl] + LB.entry[bl].block->structure[i1];
	  else row = LB.row[bl] + i1;
          for (j1=j; j1<j+LB.partition_size[j]; j1++) {
	    y[j1] -= LB.entry[bl].block->nz[
		i1+(j1-j)*LB.entry[bl].block->length] * xp[row];
	  }
	}
      }
      /* diagonal block */
      bl = LB.col[j];
      for (j1=j+LB.partition_size[j]-1; j1>=j; j1--) {
        for (i1=j1-j+1; i1<LB.entry[bl].block->length; i1++) {
	  if (LB.entry[bl].block->structure)
	    row = LB.row[bl] + LB.entry[bl].block->structure[i1];
	  else row = LB.row[bl] + i1;
	  y[j1] -= LB.entry[bl].block->nz[
		i1+(j1-j)*LB.entry[bl].block->length] * xp[row];
	}
	xp[j1] = y[j1]/
	        LB.entry[bl].block->nz[(j1-j)+(j1-j)*LB.entry[bl].block->length];
      }
    }

    j--;
    if (j>=0 && LB.partition_size[j] < 0)
      j += LB.partition_size[j];
  }


  for (j=0; j<LB.n; j++)
    x[j] = xp[PERM[j]];

  free(xp);
  free(y);
  free(bt);

  return(x);
}

double ComputeNorm(double *x, long n)
{
  double tmp = 0.0;
  long i;

  for (i=0; i<n; i++)
    if (fabs(x[i]-1.0) > tmp)
      tmp = fabs(x[i]-1.0);

  return(tmp);
}

double *CreateVector(SMatrix M)
{
  long i, j;
  double *b;

  b = NewVector(M.n);

  for (j=0; j<M.n; j++)
    b[j] = 0.0;

  if (M.nz) {
    for (j=0; j<M.n; j++)
      for (i=M.col[j]; i<M.col[j+1]; i++) {
	b[M.row[i]] += M.nz[i];
      }
  }
  else {
    for (j=0; j<M.n; j++)
      for (i=M.col[j]; i<M.col[j+1]; i++) {
	b[M.row[i]] += Value(M.row[i], j);
      }
  }

  return(b);
}

