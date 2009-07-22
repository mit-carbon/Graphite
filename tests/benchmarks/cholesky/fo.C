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

extern struct GlobalMemory *Global;
extern BMatrix LB;
extern long P;
extern long BS;
extern long *node;  /* global */
extern long scatter_decomposition, P_dimi, P_dimj;
struct BlockList ***AllBlocks, ***DiagBlock;
long **ToReceive, **NReceived;


void PreProcessFO(long MyNum)
{
  InitRemainingFO(MyNum);
  InitReceivedFO(MyNum);
}

void PreAllocate1FO()
{
  long i;

  AllBlocks = (struct BlockList ***) MyMalloc(P*sizeof(struct BlockList **),
					       DISTRIBUTED);
  DiagBlock = (struct BlockList ***) MyMalloc(P*sizeof(struct BlockList **),
					       DISTRIBUTED);
  ToReceive = (long **) MyMalloc(P*sizeof(long *), DISTRIBUTED);
  NReceived = (long **) MyMalloc(P*sizeof(long *), DISTRIBUTED);
  for (i=0; i<P; i++) {
    ToReceive[i] = (long *) MyMalloc(LB.n_partitions*sizeof(long), i);
    NReceived[i] = (long *) MyMalloc(LB.n_partitions*sizeof(long), i);
  }
}


void PreAllocateFO(long MyNum, struct LocalCopies *lc)
{
  long i, j, stor_size, root, update_size;

  lc->link = (long *) MyMalloc((LB.n+1)*sizeof(long), MyNum);
  lc->first = (long *) MyMalloc(LB.n*sizeof(long), MyNum);

  for (i=0; i<LB.n; i++)
    lc->first[i] = lc->link[i] = 0;

  lc->max_panel = 0;
  for (i=0; i<LB.n; i+=node[i])
    if (LB.domain[i]) {
      if (LB.col[i+node[i]]-LB.col[i] > lc->max_panel)
	lc->max_panel = LB.col[i+node[i]]-LB.col[i];
      j = LB.col[i+1]-LB.col[i]-node[i];
      if (j*(j+1)/2 > lc->max_panel)
	lc->max_panel = j*(j+1)/2;
    }

  if (LB.max_partition > BS) {  /* if blocks need to be copied */
    stor_size = BS*BS;
    lc->blktmp = (double *) MyMalloc(stor_size*sizeof(double), MyNum);
    for (i=0; i<stor_size; i++)
      lc->blktmp[i] = 0.0;
  }
  else {
    lc->blktmp = NULL;
  }
  stor_size = LB.max_partition*LB.max_partition;
  lc->updatetmp = (double *) MyMalloc(stor_size*sizeof(double), MyNum);
  for (i=0; i<stor_size; i++)
    lc->updatetmp[i] = 0.0;
  lc->relative = (long *) MyMalloc(LB.max_partition*sizeof(long), MyNum);

  AllBlocks[MyNum] = (struct BlockList **)
    MyMalloc(LB.n_partitions*sizeof(struct BlockList *), MyNum);
  DiagBlock[MyNum] = (struct BlockList **)
    MyMalloc(LB.n_partitions*sizeof(struct BlockList *), MyNum);
  for (i=0; i<LB.n_partitions; i++)
    AllBlocks[MyNum][i] = DiagBlock[MyNum][i] = NULL;

  /* allocate domain update matrices */
    for (i=LB.proc_domains[MyNum]; i<LB.proc_domains[MyNum+1]; i++) {
      root = LB.domains[i];
      update_size = LB.col[root+1]-LB.col[root]-1;
      update_size = update_size*(update_size+1)/2;
      if (update_size) {
	LB.proc_domain_storage[i] = (double *) MyMalloc(update_size*
							 sizeof(double),
							 MyNum);
        for (j=0; j<update_size; j++)
	  LB.proc_domain_storage[i][j] = 0.0;
      }
      else
	LB.proc_domain_storage[i] = NULL;
    }
}


void BNumericSolveFO(long MyNum, struct LocalCopies *lc)
{
  long i;

  for (i=0; i<LB.n; i++)
    lc->link[i] = -1;

  lc->storage = (double *) MyMalloc(lc->max_panel*sizeof(double), MyNum);
  for (i=0; i<lc->max_panel; i++)
    lc->storage[i] = 0.0;

  for (i=LB.proc_domains[MyNum]; i<LB.proc_domains[MyNum+1]; i++)
    FactorLLDomain(i, MyNum, lc);
  MyFree(lc->storage);
  lc->storage = NULL;
    DriveParallelFO(MyNum, lc);
}


void DriveParallelFO(long MyNum, struct LocalCopies *lc)
{
  long some, j;

  some = 0;
  for (j=0; j<LB.n; j+=node[j])
    if (!LB.domain[j]) {
      some = 1;
      if (BLOCK(LB.col[j])->owner == MyNum &&
	  BLOCK(LB.col[j])->remaining == 0)
	BlockReadyFO(LB.col[j], MyNum, lc);
    }

  if (some) 
    while (HandleTaskFO(MyNum, lc))
      ;

  if (TaskWaiting(MyNum))
    printf("**** Termination error ***\n");
}


long HandleTaskFO(long MyNum, struct LocalCopies *lc)
{
  long desti, destj, src;
  struct Update *update;

  GetBlock(&desti, &destj, &src, &update, MyNum, lc);
  if (desti == -1) /* terminate */
    return(0);
  else if (update == (struct Update *) -19)
    HandleUpdate2FO(src, desti, destj, MyNum, lc);
  else if (update != NULL) {
  }
  else {
    if (BLOCKROW(src) == BLOCKCOL(src))
      DiagReceived(src, MyNum, lc);
    else
      BlockReceived(src, MyNum, lc);

    NReceived[MyNum][LB.renumbering[BLOCKCOL(src)]]--;
    if (NReceived[MyNum][LB.renumbering[BLOCKCOL(src)]] == 0)
      FreeColumnListFO(MyNum, LB.renumbering[BLOCKCOL(src)]);
  }

  return(1);
}


/*  Receive a block.  Use it to produce updates to other blocks. */

void DiagReceived(long diag, long MyNum, struct LocalCopies *lc)
{
  long i, column;
  struct BlockList *diagbl;

  diagbl = CopyOneBlock(diag, MyNum);
  column = LB.renumbering[BLOCKCOL(diag)];
  diagbl->next = NULL;
  DiagBlock[MyNum][column] = diagbl;

  column = BLOCKCOL(diag);
  for (i=LB.col[column]+1; i<LB.col[column+1]; i++)
    if (BLOCK(i)->owner == MyNum &&
	BLOCK(i)->remaining == 0) {
      BDiv(diagbl->length, BLOCK(i)->length, diagbl->nz, BLOCK(i)->nz, lc);
      BlockDoneFO(i, MyNum, lc);
    }

  /* terminate */
  if (BLOCKCOL(diag)+LB.partition_size[BLOCKCOL(diag)] == LB.n &&
      OWNER(diag) == MyNum)
    for (i=0; i<P; i++)
      Send(-1, -1, -1, -1, (struct Update *) NULL, i, MyNum, lc);
}


/*  Receive a block.  Use it to produce updates to other blocks. */

void BlockReceived(long block, long MyNum, struct LocalCopies *lc)
{
  long column;
  struct BlockList *thisbl, *bl;

  column = LB.renumbering[BLOCKCOL(block)];

  /* add block to list for 'column' */
  thisbl = CopyOneBlock(block, MyNum);
  thisbl->next = AllBlocks[MyNum][column];
  AllBlocks[MyNum][column] = thisbl;

  /* perform related block updates */
  bl = AllBlocks[MyNum][column]->next;
  while (bl) {
    if (block < bl->theBlock)
      PerformUpdate(thisbl, bl, MyNum, lc);
    else
      PerformUpdate(bl, thisbl, MyNum, lc);

    bl = bl->next;
  }

  /* perform diagonal update */
  PerformUpdate(thisbl, thisbl, MyNum, lc);
}


/* create a structure to record receipt of 'block' */
/* if 'copy_across' is set, all relevant info is copied across */
/* otherwise, structure points to info in home */

struct BlockList *CopyOneBlock(long block, long MyNum)
{
  struct BlockList *bl;

    bl = (struct BlockList *) MyMalloc(sizeof(struct BlockList), MyNum);

  bl->theBlock = block;
  bl->row = BLOCKROW(block); bl->col = BLOCKCOL(block);
  bl->length = BLOCK(block)->length;
  
    bl->structure = BLOCK(block)->structure;
    bl->nz = BLOCK(block)->nz;

  return(bl);
}


void FreeColumnListFO(long p, long col)
{
  struct BlockList *bl;
  
  while (AllBlocks[p][col]) {
    bl = AllBlocks[p][col];
    AllBlocks[p][col] = bl->next;
    MyFree(bl);
  }
  while (DiagBlock[p][col]) {
    bl = DiagBlock[p][col];
    DiagBlock[p][col] = bl->next;
    MyFree(bl);
  }
}


void DecrementRemaining(long dest_block, long MyNum, struct LocalCopies *lc)
{
  BLOCK(dest_block)->remaining--;
  if (BLOCK(dest_block)->remaining == 0)
    BlockReadyFO(dest_block, MyNum, lc);
  else if (BLOCK(dest_block)->remaining == -1)
    printf("*** Error rem ***\n");  
}


void PerformUpdate(struct BlockList *above_bl, struct BlockList *below_bl, long MyNum, struct LocalCopies *lc)
{
  long above, below;
  long desti, destj, dest_block, is_diag;
  long *relative_i, *relative_j;
  double *destination;

  above = above_bl->theBlock;
  below = below_bl->theBlock;

  desti = below_bl->row;
  destj = above_bl->row;

  is_diag = (desti == destj);

  dest_block = FindBlock(desti, destj);
  if (dest_block == -1)
    printf("Couldn't find %ld,%ld\n", desti, destj);
  else if (BLOCK(dest_block)->owner != MyNum)
    return; /* not my block */

  if (is_diag) {

    if (!below_bl->structure)
      destination = BLOCK(dest_block)->nz;
    else
      destination = lc->updatetmp;

    /* modify diagonal block */
    BLMod(below_bl->length, LB.partition_size[below_bl->col], below_bl->nz, destination, lc);

    if (destination == lc->updatetmp) {

      ScatterUpdateFO(below_bl->length, below_bl->structure,
		      below_bl->length, below_bl->structure,
		      BLOCK(dest_block)->length,
		      lc->updatetmp, BLOCK(dest_block)->nz);
    }
  }

  else {
    /* modify off-diagonal block */

    if (below_bl->length == BLOCK(dest_block)->length)
      relative_i = NULL;
    else if (!BLOCK(dest_block)->structure)
      relative_i = below_bl->structure;
    else {
      FindRelativeIndices(below_bl->structure, below_bl->length, BLOCK(dest_block)->structure, lc->relative);
      relative_i = lc->relative;
    }
      
    if (above_bl->structure)
      relative_j = above_bl->structure;
    else
      relative_j = NULL;

    if (!relative_i && !relative_j)
      destination = BLOCK(dest_block)->nz;
    else
      destination = lc->updatetmp;

    BMod(above_bl->length, LB.partition_size[above_bl->col], below_bl->length, above_bl->nz, below_bl->nz, destination, lc);

    if (destination == lc->updatetmp) {

      ScatterUpdateFO(below_bl->length, relative_i,
		      above_bl->length, relative_j,
		      BLOCK(dest_block)->length,
		      lc->updatetmp, BLOCK(dest_block)->nz);

    }
  }
  DecrementRemaining(dest_block, MyNum, lc);
}



void DistributeUpdateFO(long which_domain, long MyNum, struct LocalCopies *lc)
{
  long bi, bj, desti, destj, dest_block;

  for (bi=LB.col[LB.n+which_domain]; bi<LB.col[LB.n+which_domain+1]; bi++) {
    for (bj=LB.col[LB.n+which_domain]; bj<=bi; bj++) {
      desti = BLOCKROW(bi);
      destj = BLOCKROW(bj);

      dest_block = FindBlock(desti, destj);
      Send(which_domain, dest_block, bi, bj, (struct Update *) -19,
	   OWNER(dest_block), MyNum, lc);
    }
  }
}


void HandleUpdate2FO(long which_domain, long bli, long blj, long MyNum, struct LocalCopies *lc)
{
  long dest_block, desti, destj;
  long *relative_i, *relative_j;
  long stride;
  double *update;

  desti = BLOCKROW(bli); destj = BLOCKROW(blj);
  dest_block = FindBlock(desti, destj);

  if (dest_block == -1)
    printf("Couldn't find %ld,%ld\n", desti, destj);
  else if (BLOCK(dest_block)->owner != MyNum)
    printf("Sent to wrong PE\n");

  FindBlockUpdate(which_domain, bli, blj, &update, &stride);

  if (BLOCK(bli)->structure && BLOCK(dest_block)->structure) {
    if (BLOCK(bli)->length != BLOCK(dest_block)->length) {
      FindRelativeIndices(BLOCK(bli)->structure, BLOCK(bli)->length, BLOCK(dest_block)->structure, lc->relative);
      relative_i = lc->relative;
    }
    else
      relative_i = NULL;
  }
  else if (BLOCK(bli)->structure)
    relative_i = BLOCK(bli)->structure;
  else
    relative_i = NULL;

  if (BLOCK(blj)->structure)
    relative_j = BLOCK(blj)->structure;
  else
    relative_j = NULL;

  ScatterUpdateFO2(BLOCK(bli)->length, relative_i,
		  BLOCK(blj)->length, relative_j,
		  stride, BLOCK(dest_block)->length,
		  update, BLOCK(dest_block)->nz);

  DecrementRemaining(dest_block, MyNum, lc);
}


void FindRelativeIndices(long *src_structure, long src_len, long *dest_structure, long *relative)
{
  long srci, desti;
  long *leftRow, *rightRow, *last;

  leftRow = src_structure;
  rightRow = dest_structure;
  last = &src_structure[src_len];

  srci = desti = 0;
  while (leftRow != last) {
    while (*rightRow != *leftRow) {
      rightRow++;
      desti++;
    }
    relative[srci] = desti;
    leftRow++; rightRow++; srci++; desti++;
  }

}


void BlockReadyFO(long block, long MyNum, struct LocalCopies *lc)
{
  long column;
  struct BlockList *diagbl;

  if (BLOCKROW(block) == BLOCKCOL(block)) {
    BFac(block, lc);
    BlockDoneFO(block, MyNum, lc);
  }
  else {
    column = LB.renumbering[BLOCKCOL(block)];
    if (DiagBlock[MyNum][column]) {
      diagbl = DiagBlock[MyNum][column];
      BDiv(diagbl->length, BLOCK(block)->length, diagbl->nz, BLOCK(block)->nz, lc);
      BlockDoneFO(block, MyNum, lc);
    }

  }
}


void BlockDoneFO(long block, long MyNum, struct LocalCopies *lc)
{
  long i;
  long P_row, P_col;

  if (scatter_decomposition) {
    P_row = LB.mapI[LB.renumbering[BLOCKROW(block)]]%P_dimi;
    P_col = LB.mapJ[LB.renumbering[BLOCKROW(block)]]%P_dimj;

    /* send to row */
    for (i=0; i<P_dimj; i++)
      Send(block, block, 0, 0, (struct Update *) NULL, P_row + i*P_dimi, MyNum, lc);

    /* send to column */
    for (i=0; i<P_dimi; i++)
      if (i != P_row)
	Send(block, block, 0, 0, (struct Update *) NULL, i + P_col*P_dimi, MyNum, lc);
  }
  else {
    for (i=0; i<P; i++)
      Send(block, block, 0, 0, (struct Update *) NULL, i, MyNum, lc);
  }
}


void CheckRemaining()
{
  long i, j, bogus=0;
  
  for (j=0; j<LB.n; j+=LB.partition_size[j])
    if (!LB.domain[j]) {
      for (i=LB.col[j]; i<LB.col[j+1]; i++)
	if (BLOCK(i)->remaining) {
	  bogus = 1;
	}
    }

  if (bogus)
    printf("Bad remaining\n");

}


void CheckReceived()
{
  long p, i, bogus=0;

  for (p=0; p<P; p++)
    for (i=0; i<LB.n_partitions; i++) {
      if (AllBlocks[p][i])
	bogus = 1;
      if (DiagBlock[p][i])
	bogus = 1;
    }
  if (bogus)
    printf("Bad received\n");

}


/* determine number of updates that must be performed on each block */

void ComputeRemainingFO()
{
  long i, j, k;
  long desti, destj, dest_block;

  for (j=0; j<LB.n; j++)
    if (!LB.domain[j]) {
      for (i=LB.col[j]; i<LB.col[j+1]; i++)
	BLOCK(i)->nmod = 0;
    }

  /* domain updates */
  for (k=0; k<LB.proc_domains[P]; k++) {
    for (i=LB.col[LB.n+k]; i<LB.col[LB.n+k+1]; i++) {
      for (j=LB.col[LB.n+k]; j<i; j++) {
	destj = BLOCKROW(j);
	desti = BLOCKROW(i);
	dest_block = FindBlock(desti, destj);
	BLOCK(dest_block)->nmod++;
      }
      desti = BLOCKROW(i);
      dest_block = FindBlock(desti, desti);
      BLOCK(dest_block)->nmod++;
    }
  }

  /* block updates */
  for (k=0; k<LB.n; k++)
    if (!LB.domain[k]) {
      for (i=LB.col[k]+1; i<LB.col[k+1]; i++) {
	for (j=LB.col[k]+1; j<i; j++) {
	  destj = BLOCKROW(j);
	  desti = BLOCKROW(i);
	  dest_block = FindBlock(desti, destj);
	  BLOCK(dest_block)->nmod++;
	}
	desti = BLOCKROW(i);
	dest_block = FindBlock(desti, desti);
	BLOCK(dest_block)->nmod++;
      }
    }
}


void InitRemainingFO(long MyNum)
{
  long i, k;

  /* block updates */
  for (k=0; k<LB.n; k++)
    if (!LB.domain[k])
      for (i=LB.col[k]; i<LB.col[k+1]; i++)
	if (MyNum == -1 || BLOCK(i)->owner == MyNum)
	  BLOCK(i)->remaining = BLOCK(i)->nmod;
}


void ComputeReceivedFO()
{
  long p, i, k, block;
  long P_row, P_col, destp;

  for (p=0; p<P; p++)
    for (i=0; i<LB.n_partitions; i++)
      ToReceive[p][i] = 0;

  for (k=0; k<LB.n; k+=LB.partition_size[k])
    if (!LB.domain[k]) {
      for (block=LB.col[k]; block<LB.col[k+1]; block++) {
	
	/* should be identical to BlockDoneFO() */
	if (scatter_decomposition) {
	  P_row = LB.mapI[LB.renumbering[BLOCKROW(block)]]%P_dimi;
	  P_col = LB.mapJ[LB.renumbering[BLOCKROW(block)]]%P_dimj;

	  /* send to row */
	  for (i=0; i<P_dimj; i++) {
	    destp = P_row + i*P_dimi;
	    ToReceive[destp][LB.renumbering[BLOCKCOL(block)]]++;
	  }

	  /* send to column */
	  for (i=0; i<P_dimi; i++) 
	    if (i != P_row) {
	      destp = i + P_col*P_dimi;
	      ToReceive[destp][LB.renumbering[BLOCKCOL(block)]]++;
	    }

	}
	else {
	  for (i=0; i<P; i++)
	    ToReceive[i][LB.renumbering[BLOCKCOL(block)]]++;
	}
      }
    }
}


void InitReceivedFO(long MyNum)
{
  long i;

  for (i=0; i<LB.n_partitions; i++)
    NReceived[MyNum][i] = ToReceive[MyNum][i];
}



void ScatterUpdateFO(long dimi, long *structi, long dimj, long *structj, long destdim, double *oldupdate, double *newupdate)
{
  long i, j;
  double *srccol, *destcol;

  for (j=0; j<dimj; j++) {
    if (structj)
      destcol = &newupdate[structj[j]*destdim];
    else
      destcol = &newupdate[j*destdim];
    srccol = &oldupdate[j*dimi];
    if (structi)
      for (i=0; i<dimi; i++) {
	destcol[structi[i]] += srccol[i];
	srccol[i] = 0.0;
      }
    else
      for (i=0; i<dimi; i++) {
	destcol[i] += srccol[i];
	srccol[i] = 0.0;
      }
  }
}



void ScatterUpdateFO2(long dimi, long *structi, long dimj, long *structj, long stride, long destdim, double *oldupdate, double *newupdate)
{
  long i, j, top_of_srccol, top_of_destcol;

  top_of_srccol = 0;
  for (j=0; j<dimj; j++) {
    if (structj)
      top_of_destcol = structj[j]*destdim;
    else
      top_of_destcol = j*destdim;
    if (structi)
      for (i=0; i<dimi; i++) {
	newupdate[structi[i]+top_of_destcol] += oldupdate[i+top_of_srccol];
      }
    else
      for (i=0; i<dimi; i++) {
	newupdate[i+top_of_destcol] += oldupdate[i+top_of_srccol];
      }
    top_of_srccol += (stride-j);
  }
}

