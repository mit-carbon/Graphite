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

#define AddMember(set, new) { long s, n; s = set; n = new; \
			       lc->link[n] = lc->link[s]; lc->link[s] = n; }

extern BMatrix LB;
extern struct GlobalMemory *Global;
extern long *node; /* global */


void FactorLLDomain(long which_domain, long MyNum, struct LocalCopies *lc)
{
  long i, start, root;
  long j, j_last, j_len, dest_super;
  long k, k_length, update_size;
  long theFirst, theLast;
  long *relative, *indices;
  double *domain_update;
  extern long *firstchild, *child;

  relative = (long *) MyMalloc(LB.n*sizeof(long), MyNum);
  indices = (long *) MyMalloc(LB.n*sizeof(long), MyNum);

  root = LB.domains[which_domain];

  start = root;
  while (firstchild[start] != firstchild[start+1])
    start = child[firstchild[start]];

  lc->link[root+1] = -1;

  /* compute domain columns */

  for (j=start; j<=root; j+=node[j]) {

    j_last = j+node[j];
    j_len = LB.col[j+1]-LB.col[j];

    SetDestIndices(j, indices);

    while (lc->link[j] != -1) {
      k = lc->link[j];
      lc->link[j] = lc->link[k];

      k_length = LB.col[k+1]-LB.col[k];

      theFirst = lc->first[k];
      theLast = theFirst+1;
      while (theLast < k_length && LB.row[LB.col[k]+theLast] < j_last)
	theLast++;

      if (theLast-theFirst == node[j] &&
	  k_length-theFirst == j_len) {
	ModifySuperBySuper(k, theFirst, theLast, k_length,
			   (double *) &LB.entry[LB.col[j]].nz);
      }
      else if (node[k] > 1) {
	ModifySuperBySuper(k, theFirst, theLast, k_length, lc->storage);
	FindRelativeIndicesLeft(&LB.row[LB.col[k]+theFirst],
				k_length-theFirst, 0, indices, relative);
	ScatterSuperUpdate(lc->storage, theLast-theFirst, k_length-theFirst,
			   (double *) &LB.entry[LB.col[j]].nz,
			   j_len, relative);
      }
      else {
	FindRelativeIndicesLeft(&LB.row[LB.col[k]+theFirst],
				k_length-theFirst, 0, indices, relative);
	ModifySuperByColumn((double *) &LB.entry[LB.col[k]+theFirst].nz,
			    theLast-theFirst, k_length-theFirst,
			    (double *) &LB.entry[LB.col[j]].nz,
			    j_len, relative);
      }

      lc->first[k] = theLast;  /* move on */
      if (theLast < k_length) {
	dest_super = LB.row[LB.col[k]+theLast];
	if (dest_super > root)
	  dest_super = root+1;
	else if (node[dest_super] < 0)
	  dest_super += node[dest_super];
	AddMember(dest_super, k);
      }
    }

    CompleteSupernodeB(j);

    lc->first[j] = node[j];
    if (lc->first[j] < j_len) {
      dest_super = LB.row[LB.col[j]+lc->first[j]];
      if (dest_super > root)
	dest_super = root+1;
      else if (node[dest_super] < 0)
	dest_super += node[dest_super];
      AddMember(dest_super, j);
    }
  }


  /* compute update matrix */

  j_len = LB.col[root+1]-LB.col[root]-1;
  update_size = j_len*(j_len+1)/2;

  if (update_size == 0)
    domain_update = NULL;
  else if (LB.proc_domain_storage[which_domain])
    domain_update = LB.proc_domain_storage[which_domain];
  else {
    domain_update = (double *) MyMalloc(update_size*sizeof(double),
					 MyNum);
    LB.proc_domain_storage[which_domain] = domain_update;
  }
  for (i=0; i<update_size; i++)
    domain_update[i] = 0.0;

  SetDomainIndices(root, indices);

  while (lc->link[root+1] != -1) {
    k = lc->link[root+1];
    lc->link[root+1] = lc->link[k];

    k_length = LB.col[k+1] - LB.col[k];

    theFirst = lc->first[k];
    theLast = k_length;

    if (theLast-theFirst == j_len) {
      ModifySuperBySuper(k, theFirst, theLast, k_length, domain_update);
    }
    else if (node[k] > 1) {
      ModifySuperBySuper(k, theFirst, theLast, k_length, lc->storage);
      FindRelativeIndicesLeft(&LB.row[LB.col[k]+theFirst], k_length-theFirst,
			      0, indices, relative);
      ScatterSuperUpdate(lc->storage, k_length-theFirst, k_length-theFirst,
			 domain_update, j_len, relative);
    }
    else {
      FindRelativeIndicesLeft(&LB.row[LB.col[k]+theFirst],
			      k_length-theFirst, 0, indices, relative);
      ModifySuperByColumn((double *) &LB.entry[LB.col[k]+theFirst].nz,
			  theLast-theFirst, k_length-theFirst,
			  domain_update, j_len, relative);
    }
  }

  if (domain_update) {
      DistributeUpdateFO(which_domain, MyNum, lc);
  }

  MyFree(relative); MyFree(indices);
}


void CompleteSupernodeB(long super)
{
  long i, length, fits, first, last;

  if (node[super] == 1) {
    CompleteColumnB(super);
    return;
  }

  length = LB.col[super+1]-LB.col[super];
  fits = FitsInCache/length;
  if (fits > 4)
    fits &= 0xfffffffc;
  else if (fits < 2)
    fits = node[super];

  first = super;

  while (first<super+node[super]) {
    last = first+fits;
    if (last > super+node[super])
      last = super+node[super];

    i = first;
    for (; i<last-1; i+=2) {
      ModifyTwoBySupernodeB(first, i, i-first,
			 (double *) &LB.entry[LB.col[i]].nz,
			 (double *) &LB.entry[LB.col[i+1]].nz);
      CompleteColumnB(i);
      ModifyBySupernodeB(i, i+1, 1, (double *) &LB.entry[LB.col[i+1]].nz);
      CompleteColumnB(i+1);
    }
    for (; i<last; i++) {
      ModifyBySupernodeB(first, i, i-first,
			 (double *) &LB.entry[LB.col[i]].nz);
      CompleteColumnB(i);
    }

    i = last;
    for (; i<super+node[super]-1; i+=2)
      ModifyTwoBySupernodeB(first, last, i-first,
			 (double *) &LB.entry[LB.col[i]].nz,
			 (double *) &LB.entry[LB.col[i+1]].nz);
    for (; i<super+node[super]; i++)
      ModifyBySupernodeB(first, last, i-first,
			 (double *) &LB.entry[LB.col[i]].nz);
			 

    first = last;
  }
}



void CompleteColumnB(long j)
{
  double recip, diag, *theNZ, *last;

  theNZ = &LB.entry[LB.col[j]].nz;
  last = &LB.entry[LB.col[j+1]].nz;

  diag = *theNZ;
  if (diag <= 0.0) {
    printf("Negative pivot, d[%ld] = %f\n", j, diag);
    exit(0);
  }
  diag = sqrt(diag);
  *theNZ++ = diag;
  recip = 1.0/diag;

  while (theNZ != last) 
    *theNZ++ *= recip;
}


/* given a src_structure non-zero structure with given length,
   and assuming that global 'indices' contains structure of dest,
   find relative indices */

void FindRelativeIndicesLeft(long *src_structure, long rows_in_update, long offset, long *indices, long *relative)
{
  long i, *leftRow, *last;

  leftRow = src_structure;
  last = &src_structure[rows_in_update];

  i = 0;
  while (leftRow != last)
    relative[i++] = indices[*leftRow++] - offset;
}



/* given a panel update 'update' with given size and relative indices,
   scatter into into dest_nz */

void ScatterSuperUpdate(double *update, long cols_in_update, long rows_in_update, double *dest_nz, long dest_len, long *relative)
{
  long i, dest, *last, *leftRow;
  double *theTmp, *rightNZ;

  theTmp = update;

  for (i=0; i<cols_in_update; i++) {
    leftRow = relative+i;
    last = relative + rows_in_update;
    dest = leftRow[0];
    rightNZ = dest_nz + dest*dest_len - dest*(dest+1)/2;
    while (leftRow != last) {
      rightNZ[*leftRow] += *theTmp;
      *theTmp = 0.0;
      theTmp++; leftRow++;
    }
  }
}


/* given a panel update 'update' with given size and relative indices,
   scatter into into dest_nz */

void ModifySuperByColumn(double *src_nz, long cols_in_update, long rows_in_update, double *dest_nz, long dest_len, long *relative)
{
  long i, dest, *last, *leftRow;
  double ljk, *theTmp, *rightNZ;

  for (i=0; i<cols_in_update; i++) {
    leftRow = relative+i;
    last = relative + rows_in_update;
    dest = leftRow[0];
    rightNZ = dest_nz + dest*dest_len - dest*(dest+1)/2;
    theTmp = src_nz+i;
    ljk = *theTmp;
    while (leftRow != last) {
      rightNZ[*leftRow] -= ljk*(*theTmp);
      theTmp++; leftRow++;
    }
  }
}


void SetDestIndices(long super, long *indices)
{
  long i, *rightRow, *lastRow;

  rightRow = &LB.row[LB.col[super]];
  lastRow = rightRow + (LB.col[super+1]-
	LB.col[super]);
  i = 0;
  while (rightRow != lastRow)
    indices[*rightRow++] = i++;
}


void SetDomainIndices(long super, long *indices)
{
  long i, *rightRow, *lastRow;

  rightRow = &LB.row[LB.col[super]+1];
  lastRow = rightRow-1 + (LB.col[super+1]-LB.col[super]);
  i = 0;
  while (rightRow != lastRow)
    indices[*rightRow++] = i++;
}


void ModifySuperBySuper(long src, long theFirst, long theLast, long length, double *dest)
{
  long i, fits;
  long first, last, lastcol;
  long this_length;
  double *destination;

  fits = FitsInCache/length;
  if (fits > 4)
    fits &= 0xfffffffc;
  else if (fits < 2)
    fits = node[src];

  lastcol = src+node[src];
  last = src;

  while (last < lastcol) {

    first = last;
    last = first + fits;
    if (last > lastcol)
      last = lastcol;

    destination = dest;
    this_length = length-theFirst;
    i = theFirst;
    for (; i<theLast-1; i+=2) {
      ModifyTwoBySupernodeB(first, last, i-(first-src), 
			    destination, destination+this_length);
      destination += this_length + this_length - 1; this_length-=2;
    }
    for (; i<theLast; i++) {
      ModifyBySupernodeB(first, last, i-(first-src), destination);
      destination += this_length; this_length--;
    }
  }
}


void ModifyTwoBySupernodeB(long super, long lastcol, long theFirst, double *destination0, double *destination1)
{
  long col, increment;
  double ljk0_0, ljk0_1, ljk1_0, ljk1_1, ljk2_0, ljk2_1, ljk3_0, ljk3_1;
  double ljk4_0, ljk4_1, ljk5_0, ljk5_1, ljk6_0, ljk6_1, ljk7_0, ljk7_1;
  double d0, d1, tmp0, tmp1;
  double *last, *dest0, *dest1;
  double *srcNZ0, *srcNZ1, *srcNZ2, *srcNZ3;
  double  *srcNZ4, *srcNZ5, *srcNZ6, *srcNZ7;

  col = super;
  while (col < lastcol-7) {

    dest0 = destination0; dest1 = destination1;

    last = &LB.entry[LB.col[col+1]].nz;
    increment = LB.col[col+1] - LB.col[col];

    srcNZ0 = &LB.entry[LB.col[col] + (super-col) + theFirst].nz;
    srcNZ1 = srcNZ0 + increment - 1;
    srcNZ2 = srcNZ1 + increment - 2;
    srcNZ3 = srcNZ2 + increment - 3;
    srcNZ4 = srcNZ3 + increment - 4;
    srcNZ5 = srcNZ4 + increment - 5;
    srcNZ6 = srcNZ5 + increment - 6;
    srcNZ7 = srcNZ6 + increment - 7;

    ljk0_0 = *srcNZ0++; ljk0_1 = *srcNZ0++;
    ljk1_0 = *srcNZ1++; ljk1_1 = *srcNZ1++;
    ljk2_0 = *srcNZ2++; ljk2_1 = *srcNZ2++;
    ljk3_0 = *srcNZ3++; ljk3_1 = *srcNZ3++;
    ljk4_0 = *srcNZ4++; ljk4_1 = *srcNZ4++;
    ljk5_0 = *srcNZ5++; ljk5_1 = *srcNZ5++;
    ljk6_0 = *srcNZ6++; ljk6_1 = *srcNZ6++;
    ljk7_0 = *srcNZ7++; ljk7_1 = *srcNZ7++;

    *dest0++ -= ljk0_0*ljk0_0 + ljk1_0*ljk1_0 + ljk2_0*ljk2_0 + ljk3_0*ljk3_0
      + ljk4_0*ljk4_0 + ljk5_0*ljk5_0 + ljk6_0*ljk6_0 + ljk7_0*ljk7_0;
    *dest1++ -= ljk0_1*ljk0_1 + ljk1_1*ljk1_1 + ljk2_1*ljk2_1 + ljk3_1*ljk3_1
      + ljk4_1*ljk4_1 + ljk5_1*ljk5_1 + ljk6_1*ljk6_1 + ljk7_1*ljk7_1;

    *dest0++ -= ljk0_0*ljk0_1 + ljk1_0*ljk1_1 + ljk2_0*ljk2_1 + ljk3_0*ljk3_1
      + ljk4_0*ljk4_1 + ljk5_0*ljk5_1 + ljk6_0*ljk6_1 + ljk7_0*ljk7_1;

    while (srcNZ0 != last) {
      d0 = *dest0; d1 = *dest1;
      tmp0 = *srcNZ0++; d0 -= ljk0_0*tmp0; d1 -= ljk0_1*tmp0;
      tmp1 = *srcNZ1++; d0 -= ljk1_0*tmp1; d1 -= ljk1_1*tmp1;
      tmp0 = *srcNZ2++; d0 -= ljk2_0*tmp0; d1 -= ljk2_1*tmp0;
      tmp1 = *srcNZ3++; d0 -= ljk3_0*tmp1; d1 -= ljk3_1*tmp1;
      tmp0 = *srcNZ4++; d0 -= ljk4_0*tmp0; d1 -= ljk4_1*tmp0;
      tmp1 = *srcNZ5++; d0 -= ljk5_0*tmp1; d1 -= ljk5_1*tmp1;
      tmp0 = *srcNZ6++; d0 -= ljk6_0*tmp0; d1 -= ljk6_1*tmp0;
      tmp1 = *srcNZ7++; d0 -= ljk7_0*tmp1; d1 -= ljk7_1*tmp1;
      *dest0++ = d0; *dest1++ = d1;
    }

    col += 8;
  }
  while (col < lastcol-3) {

    dest0 = destination0; dest1 = destination1;

    last = &LB.entry[LB.col[col+1]].nz;
    increment = LB.col[col+1] - LB.col[col];

    srcNZ0 = &LB.entry[LB.col[col] + (super-col) + theFirst].nz;
    srcNZ1 = srcNZ0 + increment - 1;
    srcNZ2 = srcNZ1 + increment - 2;
    srcNZ3 = srcNZ2 + increment - 3;

    ljk0_0 = *srcNZ0++; ljk0_1 = *srcNZ0++;
    ljk1_0 = *srcNZ1++; ljk1_1 = *srcNZ1++;
    ljk2_0 = *srcNZ2++; ljk2_1 = *srcNZ2++;
    ljk3_0 = *srcNZ3++; ljk3_1 = *srcNZ3++;

    *dest0++ -= ljk0_0*ljk0_0 + ljk1_0*ljk1_0 + ljk2_0*ljk2_0 + ljk3_0*ljk3_0;
    *dest1++ -= ljk0_1*ljk0_1 + ljk1_1*ljk1_1 + ljk2_1*ljk2_1 + ljk3_1*ljk3_1;

    *dest0++ -= ljk0_0*ljk0_1 + ljk1_0*ljk1_1 + ljk2_0*ljk2_1 + ljk3_0*ljk3_1;

    while (srcNZ0 != last) {
      d0 = *dest0; d1 = *dest1;
      tmp0 = *srcNZ0++; d0 -= ljk0_0*tmp0; d1 -= ljk0_1*tmp0;
      tmp1 = *srcNZ1++; d0 -= ljk1_0*tmp1; d1 -= ljk1_1*tmp1;
      tmp0 = *srcNZ2++; d0 -= ljk2_0*tmp0; d1 -= ljk2_1*tmp0;
      tmp1 = *srcNZ3++; d0 -= ljk3_0*tmp1; d1 -= ljk3_1*tmp1;
      *dest0++ = d0; *dest1++ = d1;
    }

    col+=4;
  }
  while (col < lastcol-1) {
    last = &LB.entry[LB.col[col+1]].nz;
    dest0 = destination0; dest1 = destination1;

    increment = LB.col[col+1] - LB.col[col];

    srcNZ0 = &LB.entry[LB.col[col] + (super-col) + theFirst].nz;
    srcNZ1 = srcNZ0 + increment - 1;

    ljk0_0 = *srcNZ0++; ljk0_1 = *srcNZ0++;
    ljk1_0 = *srcNZ1++; ljk1_1 = *srcNZ1++;

    *dest0++ -= ljk0_0*ljk0_0 + ljk1_0*ljk1_0;
    *dest1++ -= ljk0_1*ljk0_1 + ljk1_1*ljk1_1;

    *dest0++ -= ljk0_0*ljk0_1 + ljk1_0*ljk1_1;

    while (srcNZ0 != last) {
      tmp0 = *srcNZ0++; tmp1 = *srcNZ1++;
      *dest0++ -= ljk0_0*tmp0 + ljk1_0*tmp1;
      *dest1++ -= ljk0_1*tmp0 + ljk1_1*tmp1;
    }
    col+=2;
  }
  while (col < lastcol) {
    last = &LB.entry[LB.col[col+1]].nz;
    dest0 = destination0; dest1 = destination1;

    srcNZ0 = &LB.entry[LB.col[col] + (super-col) + theFirst].nz;
    ljk0_0 = *srcNZ0++;
    ljk0_1 = *srcNZ0++;

    *dest0++ -= ljk0_0*ljk0_0;
    *dest1++ -= ljk0_1*ljk0_1;

    *dest0++ -= ljk0_0*ljk0_1;

    while (srcNZ0 != last) {
      tmp0 = *srcNZ0++;
      *dest0++ -= ljk0_0*tmp0; *dest1++ -= ljk0_1*tmp0;
    }

    col++;
  }
}


void ModifyBySupernodeB(long super, long lastcol, long theFirst, double *destination)
{
  double t0, ljk0, ljk1, ljk2, ljk3, ljk4, ljk5, ljk6, ljk7;
  long increment;
  double *dest, *last;
  double *theNZ0, *theNZ1, *theNZ2, *theNZ3, *theNZ4, *theNZ5, *theNZ6,*theNZ7;
  long j, col;

  j = LB.row[LB.col[super]+theFirst];

  col = super;
  while (col < lastcol - 7) {

    /* eight source columns */

    last = &LB.entry[LB.col[col+1]].nz;
    dest = destination;

    increment = LB.col[col+1] - LB.col[col];

    theNZ0 = &LB.entry[LB.col[col] + (super-col) + theFirst].nz;
    theNZ1 = theNZ0 + increment - 1;
    theNZ2 = theNZ1 + increment - 2;
    theNZ3 = theNZ2 + increment - 3;
    theNZ4 = theNZ3 + increment - 4;
    theNZ5 = theNZ4 + increment - 5;
    theNZ6 = theNZ5 + increment - 6;
    theNZ7 = theNZ6 + increment - 7;

    ljk0 = *theNZ0++; ljk1 = *theNZ1++; ljk2 = *theNZ2++; ljk3 = *theNZ3++;
    ljk4 = *theNZ4++; ljk5 = *theNZ5++; ljk6 = *theNZ6++; ljk7 = *theNZ7++;

    *dest++ -= ljk0*ljk0 + ljk1*ljk1 + ljk2*ljk2 + ljk3*ljk3
      + ljk4*ljk4 + ljk5*ljk5 + ljk6*ljk6 + ljk7*ljk7;

    while (theNZ0 != last) {
      t0 = *dest;
      t0 -= ljk0*(*theNZ0++);
      t0 -= ljk1*(*theNZ1++);
      t0 -= ljk2*(*theNZ2++);
      t0 -= ljk3*(*theNZ3++);
      t0 -= ljk4*(*theNZ4++);
      t0 -= ljk5*(*theNZ5++);
      t0 -= ljk6*(*theNZ6++);
      t0 -= ljk7*(*theNZ7++);
      *dest++ = t0;
    }

    col += 8;
  }
  while (col < lastcol - 3) {

    /* four source columns */

    last = &LB.entry[LB.col[col+1]].nz;
    dest = destination;

    increment = LB.col[col+1] - LB.col[col];

    theNZ0 = &LB.entry[theFirst + LB.col[col] + (super-col)].nz;
    theNZ1 = theNZ0 + increment - 1;
    theNZ2 = theNZ1 + increment - 2;
    theNZ3 = theNZ2 + increment - 3;

    ljk0 = *theNZ0++; ljk1 = *theNZ1++; ljk2 = *theNZ2++; ljk3 = *theNZ3++;

    *dest++ -= ljk0*ljk0 + ljk1*ljk1 + ljk2*ljk2 + ljk3*ljk3;

    while (theNZ0 != last) {
      t0 = *dest;
      t0 -= ljk0*(*theNZ0++);
      t0 -= ljk1*(*theNZ1++);
      t0 -= ljk2*(*theNZ2++);
      t0 -= ljk3*(*theNZ3++);
      *dest++ = t0;
    }

    col += 4;
  }
  while (col < lastcol) {

    /* one source column */

    last = &LB.entry[LB.col[col+1]].nz;
    dest = destination;

    theNZ0 = &LB.entry[theFirst + LB.col[col] + (super-col)].nz;
    
    ljk0 = *theNZ0++;

    *dest++ -= ljk0*ljk0;

    while (theNZ0 < last - 3) {
      *dest -= ljk0*(*theNZ0);
      *(dest+1) -= ljk0*(*(theNZ0+1));
      *(dest+2) -= ljk0*(*(theNZ0+2));
      *(dest+3) -= ljk0*(*(theNZ0+3));
      dest += 4; theNZ0 += 4;
    }
    while (theNZ0 != last)
      *dest++ -= ljk0*(*theNZ0++);

    col++;
  }
}

