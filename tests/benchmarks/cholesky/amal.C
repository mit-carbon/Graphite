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

#include <stdio.h>
#include "matrix.h"

long *next_in_super, *member_of, *super_parent;
long *tree_firstchild, *tree_sibling;
long *tree_original_firstchild, *tree_original_sibling;
long ops_added;
double *crit;
extern long *INVP;

long OpsFromSuper(long size, long nz)
{
  long ops = 0;

  ops += size*(size+1)*(2*size+1)/6;
  ops += size*size*(nz-size);
  ops += 2*size*(nz-size)*(nz-size+1)/2;

  return(ops);
}

long CountSupers(long cols, long *node)
{
  long i, supers;

  supers = 0;
  for (i=0; i<cols; i+=node[i])
    supers++;

  return(supers);
}

void Amalgamate2(long join, SMatrix M, long *T, long *nz, long *node, long *domain, long target_size)
{
  long i, j;
  long counter, supers_before, supers_after;
  double g_ops_before;
  extern double *work_tree;
  extern long *PERM, *firstchild, *child;

  tree_firstchild = (long *) malloc((M.n+1)*sizeof(long));
  tree_sibling = (long *) malloc((M.n+1)*sizeof(long));
  tree_original_firstchild = (long *) malloc((M.n+1)*sizeof(long));
  tree_original_sibling = (long *) malloc((M.n+1)*sizeof(long));
  next_in_super = (long *) malloc((M.n+1)*sizeof(long));
  member_of = (long *) malloc((M.n+1)*sizeof(long));
  super_parent = (long *) malloc((M.n+1)*sizeof(long));

  for (i=0; i<=M.n; i++)
    tree_firstchild[i] = -1;

  /* link supernodes in child, sibling tree */
  for (i=0; i<M.n; i+=node[i]) {
    super_parent[i] = T[i+node[i]-1];
    tree_sibling[i] = tree_firstchild[super_parent[i]];
    tree_firstchild[super_parent[i]] = i;
  }
  super_parent[M.n] = M.n;
  for (i=0; i<=M.n; i++) {
    tree_original_firstchild[i] = tree_firstchild[i];
    tree_original_sibling[i] = tree_sibling[i];
  }

  /* link nodes within supernodes */
  for (i=0; i<M.n; i+=node[i]) {
    for (j=i; j<i+node[i]; j++) {
      next_in_super[j] = j+1;
      member_of[j] = i;
    }
    next_in_super[i+node[i]-1] = -1;
  }
  member_of[M.n] = M.n;

  supers_before = CountSupers(M.n, node);

  ops_added = 0;
  g_ops_before = work_tree[M.n];

  i = tree_original_firstchild[M.n];
  while (i != -1) {
    ConsiderMerge(join, i, M, nz, node, domain, target_size, 1);
    i = tree_original_sibling[i];
  }

  counter = M.n;
  ReorderMatrix(M, M.n, node, &counter, PERM);
  InvertPerm(M.n, PERM, INVP);

  FixNodeNZAndT(M, PERM, node, nz, T);

  free(tree_firstchild); free(tree_sibling);
  free(tree_original_firstchild); free(tree_original_sibling);
  free(next_in_super); free(member_of); free(super_parent);

  ParentToChild(T, M.n, firstchild, child);
  ComputeWorkTree(M, nz, work_tree);

  supers_after = CountSupers(M.n, node);

  printf("%ld/%ld supers before/after\n", supers_before, supers_after);
  printf("%.0f/%.0f (%.2f) ops before/after amalgamation\n",
	 g_ops_before, work_tree[M.n], work_tree[M.n]/(double) g_ops_before);
  if (ops_added != work_tree[M.n]-g_ops_before)
    printf("Model says %ld ops added, really %.0f\n", ops_added,
	   work_tree[M.n]-g_ops_before);
}


void ConsiderMerge(long join, long super, SMatrix M, long *nz, long *node, long *domain, long target_size, long traversal_order)
{
  long i, parent;
  long ops_before, ops_after, do_merge, do_merge_simple, possible;
  long allow_critical_to_grow;
  double time_before, time_after, dummy, simple_diff;
  double path_grows;

  super = member_of[super];

  /* merge until no longer profitable */
  for (;;) {

    if (super_parent[super] == M.n)
      break;

    parent = super_parent[super];

    ops_before = OpsFromSuper(node[super], nz[super]);
    ops_after = OpsFromSuper(node[super], nz[parent]+node[super]);

    time_before = 0;
    PDIV(node[super], nz[super], &dummy, &dummy, &time_before);
    PMOD(node[super], nz[super]-node[super], nz[super]-node[super],
	 &dummy, &dummy, &time_before);
    PADD(nz[super]-node[super], nz[super]-node[super], &dummy, &time_before);
    PDIV(node[parent], nz[parent], &dummy, &dummy, &time_before);
    PMOD(node[parent], nz[parent]-node[parent], nz[parent]-node[parent],
	 &dummy, &dummy, &time_before);
    PADD(nz[parent]-node[parent], nz[parent]-node[parent], &dummy,
	 &time_before);

    time_after = 0;
    PDIV(node[super]+node[parent], node[super]+nz[parent],
	 &dummy, &dummy, &time_after);
    PMOD(node[super]+node[parent], nz[parent]-node[parent],
	 nz[parent]-node[parent], &dummy, &dummy, &time_after);
    PADD(nz[parent]-node[parent], nz[parent]-node[parent], &dummy, &time_after);

    simple_diff = (ops_after-ops_before) -
      5.0*3*(nz[super]-node[super])*(nz[super]-node[super]+1)/2;

    possible = (!domain || domain[super] == 0 || domain[parent] != 0);

    allow_critical_to_grow = 1;
    if ((!domain || domain[super] == 0) && time_before > time_after) {
      if (allow_critical_to_grow)
	path_grows = 0;
    }
    else
      path_grows = 0.0;

    do_merge = (possible && time_before > time_after && path_grows == 0.0);
    do_merge_simple = (possible && simple_diff < 0);

    if (do_merge) {

      JoinTwoSupers2(nz, node, super, parent);
      ops_added += (ops_after-ops_before);

    }
    else
      break;
  }

    i = tree_original_firstchild[super];
    while (i != -1) {
      ConsiderMerge(join, i, M, nz, node, domain, target_size, traversal_order);
      i = tree_original_sibling[i];
    }
}

void JoinTwoSupers2(long *nz, long *node, long child, long parent)
{
  long i, child_last, member, grandparent;

  /* record new memberships */
  member = parent;
  while (member != -1) {
    member_of[member] = child;
    member = next_in_super[member];
  }

  /* find last member of child */
  child_last = child;
  while (next_in_super[child_last] != -1)
    child_last = next_in_super[child_last];

  /* link in nodes in parent */
  next_in_super[child_last] = parent;

  /* adjust sizes */
  nz[child] = nz[parent]+node[child];
  node[child] += node[parent];

  /* adjust child parent */
  super_parent[child] = super_parent[parent];

  /* children of 'parent' become children of 'child' */
  while (tree_firstchild[parent] != -1) {
    i = tree_firstchild[parent];
    tree_firstchild[parent] = tree_sibling[i];
    if (member_of[i] != child) {
      super_parent[member_of[i]] = child;
      tree_sibling[i] = tree_firstchild[child];
      tree_firstchild[child] = i;
    }
  }

  /* siblings of 'parent' become siblings of 'child' */
  grandparent = super_parent[parent];
  tree_sibling[child] = tree_sibling[parent];
  i = tree_firstchild[grandparent];
  if (i == parent)
    tree_firstchild[grandparent] = child;
  else {
    while (tree_sibling[i] != parent)
      i = tree_sibling[i];
    tree_sibling[i] = child;
  }
}


void ReorderMatrix(SMatrix M, long super, long *node, long *counter, long *PERM)
{
  long child, member, which_member;

  if (super != M.n) {
    super = member_of[super];

    member = super; which_member = 0;
    while (member != -1) {
      PERM[*counter-node[super]+which_member] = member;
      member = next_in_super[member];
      which_member++;
    }

    *counter -= node[super];
  }

  child = tree_firstchild[super];
  while (child != -1) {
    ReorderMatrix(M, child, node, counter, PERM);
    child = tree_sibling[child];
  }
}


void FixNodeNZAndT(SMatrix M, long *PERM, long *node, long *nz, long *T)
{
  long super, j;
  long *tmp;

  tmp = (long *) malloc(M.n*sizeof(long));

  for (j=0; j<M.n; j++)
    tmp[j] = node[j];
  for (j=0; j<M.n; j++)
    node[j] = tmp[PERM[j]];
  for (super=0; super<M.n; super+=node[super])
    for (j=super+1; j<super+node[super]; j++)
      node[j] = super-j;

  for (j=0; j<M.n; j++)
    tmp[j] = nz[j];
  for (super=0; super<M.n; super+=node[super]) {
    nz[super] = tmp[PERM[super]];
    for (j=super+1; j<super+node[super]; j++)
      nz[j] = nz[super]+super-j;
  }

  for (super=0; super<M.n; super+=node[super]) {
    for (j=super; j<super+node[super]; j++)
      T[j] = j+1;
    T[super+node[super]-1] = INVP[super_parent[PERM[super]]];
  }

  free(tmp);
}

void InvertPerm(long n, long *PERM, long *INVP)
{
  long i;

  for (i=0; i<=n; i++)
    INVP[i] = -1;

  for (i=0; i<=n; i++)
    INVP[PERM[i]] = i;

  for (i=0; i<=n; i++)
    if (INVP[i] == -1)
      printf("Not a valid permutation\n");
}


double PathLength(long cols, long rows, long target_panel_size)
{
  double path_length;

  path_length = 3*(cols*rows-cols*(cols-1)/2);
  path_length *= target_panel_size;

  return(path_length);
}

