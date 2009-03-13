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

/* Performs the laplacian calculation for a subblock */

EXTERN_ENV

#include <stdio.h>
#include <math.h>
#include <time.h>
#include "decs.h"

void laplacalc(long procid, double ****x, double ****z, long psiindex, long firstrow, long lastrow, long firstcol, long lastcol)
{
   long iindex;
   long indexp1;
   long indexm1;
   long ip1;
   long im1;
   long i;
   long j;
   double **t2a;
   double **t2b;
   double *t1a;
   double *t1b;
   double *t1c;
   double *t1d;

   t2a = (double **) x[procid][psiindex];
   j = gp[procid].neighbors[UP];
   if (j != -1) {
     t1a = (double *) t2a[0];
     t1b = (double *) x[j][psiindex][im-2];
     for (i=1;i<=lastcol;i++) {
       t1a[i] = t1b[i];
     }
   }
   j = gp[procid].neighbors[DOWN];
   if (j != -1) {
     t1a = (double *) t2a[im-1];
     t1b = (double *) x[j][psiindex][1];
     for (i=1;i<=lastcol;i++) {
       t1a[i] = t1b[i];
     }
   }
   j = gp[procid].neighbors[LEFT];
   if (j != -1) {
     t2b = (double **) x[j][psiindex];
     for (i=1;i<=lastrow;i++) {
       t2a[i][0] = t2b[i][jm-2];
     }
   }
   j = gp[procid].neighbors[RIGHT];
   if (j != -1) {
     t2b = (double **) x[j][psiindex];
     for (i=1;i<=lastrow;i++) {
       t2a[i][jm-1] = t2b[i][1];
     }
   }

   t2a = (double **) x[procid][psiindex];
   t2b = (double **) z[procid][psiindex];
   for (i=firstrow;i<=lastrow;i++) {
     ip1 = i+1;
     im1 = i-1;
     t1a = (double *) t2a[i];
     t1b = (double *) t2b[i];
     t1c = (double *) t2a[ip1];
     t1d = (double *) t2a[im1];
     for (iindex=firstcol;iindex<=lastcol;iindex++) {
       indexp1 = iindex+1;
       indexm1 = iindex-1;
       t1b[iindex] = factlap*(t1c[iindex]+
			      t1d[iindex]+t1a[indexp1]+
                              t1a[indexm1]-4.*t1a[iindex]);
     }
   }

   if (gp[procid].neighbors[UP] == -1) {
     t1b = (double *) t2b[0];
     for (j=firstcol;j<=lastcol;j++) {
       t1b[j] = 0.0;
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     t1b = (double *) t2b[im-1];
     for (j=firstcol;j<=lastcol;j++) {
       t1b[j] = 0.0;
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for (j=firstrow;j<=lastrow;j++) {
       t2b[j][0] = 0.0;
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for (j=firstrow;j<=lastrow;j++) {
       t2b[j][jm-1] = 0.0;
     }
   }

}
