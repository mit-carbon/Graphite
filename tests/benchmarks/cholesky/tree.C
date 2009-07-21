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

long *firstchild, *child;
double *work_tree;

void EliminationTreeFromA(SMatrix A, long *T, long *P, long *INVP)
{
  long *subtree;
  long i, nd, nabor, j, r, nextr, root;

  subtree = (long *) malloc((A.n+1)*sizeof(long));

  for (i=0; i<=A.n; i++)
    T[i] = subtree[i] = A.n;

  for (j=0; j<A.n; j++) {
    nd = P[j];

    for (i=A.col[nd]; i<A.col[nd+1]; i++) {

      nabor = INVP[A.row[i]];

      if (nabor < j) {

	for (r=nabor; subtree[r] != A.n; r = subtree[r])
	  ;
	if (r != j)
	  T[r] = subtree[r] = root = j;
	else root = r;

	r = nabor;
	while (subtree[r] != A.n) {
	  nextr = subtree[r];
	  subtree[r] = root;
	  r = nextr;
	}
      }
    }
  }

  free(subtree);
}


void ParentToChild(long *T, long n, long *firstchild, long *child)
{
  long i, k, parent, count=0;
  long *next;

  next = (long *) malloc((n+1)*sizeof(long));

  for (i=0; i<=n; i++)
    firstchild[i] = next[i] = -1;

  for (i=n; i>=0; i--) {
    parent = T[i];
    if (parent != i) {
	next[i] = firstchild[parent];
	firstchild[parent] = i;
    }
  }

  for (i=0; i<=n; i++) {
    k = firstchild[i];
    firstchild[i] = count;
    while (k != -1) {
      child[count++] = k;
      k = next[k];
    }
  }
  firstchild[n+1] = count;

  free(next);

}


void ComputeNZ(SMatrix A, long *T, long *nz, long *PERM, long *INVP)
{
  long i, j, nd, nabor, k;
  long *marker;

  marker = (long *) malloc(A.n*sizeof(long));
  for (i=0; i<A.n; i++)
    nz[i] = 1;
  nz[A.n] = 0;

  for (i=0; i<A.n; i++) {
    nd = PERM[i];
    marker[i] = -1;
    for (j=A.col[nd]; j<A.col[nd+1]; j++) {
      nabor = INVP[A.row[j]];
      k = nabor;
      while (marker[k] != i && k < i) {
        nz[k]++;
	marker[k] = i;
	k = T[k];
      }
    }
  }

  free(marker);
}


void FindSupernodes(SMatrix A, long *T, long *nz, long *node)
{
  long i;
  long *nchild;
  long supers, current, size, max_super = 0;

  nchild = (long *) malloc((A.n+1)*sizeof(long));
  for (i=0; i<=A.n; i++)
    nchild[i] = 0;

  for (i=0; i<A.n; i++)
    nchild[T[i]]++;

  current = 0;
  supers = 1;
  size = 1;
  for (i=1; i<A.n; i++) {
    if (nz[i] == nz[i-1]-1 && T[i-1] == i && nchild[i] == 1) {
      node[i] = current-i;
      size++;
    }
    else {
      node[current] = size;
      if (size > max_super)
	max_super = size;
      supers++;
      current = i;
      size = 1;
    }
  }

  node[current] = size;
  if (size > max_super)
    max_super = size;
  node[A.n] = 0;

  printf("%ld supers, %4.2f nodes/super, %ld max super\n",
	 supers, A.n/(double) supers, max_super);  

  free(nchild);
}


void ComputeWorkTree(SMatrix A, long *nz, double *work_tree)
{
  long i, j, nzj;

  for (j=0; j<=A.n; j++) {
    if (j != A.n) {
      nzj = nz[j];
      work_tree[j] = nzj+nzj*(nzj-1);
    }
    else
      work_tree[j] = 0;

    for (i=firstchild[j]; i<firstchild[j+1]; i++)
      work_tree[j] += work_tree[child[i]];
  }
}

