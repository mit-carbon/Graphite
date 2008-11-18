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

/*    ****************
      subroutine slave2
      ****************  */

EXTERN_ENV

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include "decs.h"

void slave2(long procid, long firstrow, long lastrow, long numrows, long firstcol, long lastcol, long numcols)
{
   long i;
   long j;
   long iindex;
   double hh1;
   double hh3;
   double hinv;
   double h1inv;
   long istart;
   long iend;
   long jstart;
   long jend;
   long ist;
   long ien;
   long jst;
   long jen;
   double fac;
   double ressqr;
   double psiaipriv;
   double f4;
   double timst;
   long psiindex;
   long i_off;
   long j_off;
   long multi_start;
   long multi_end;
   double **t2a;
   double **t2b;
   double **t2c;
   double **t2d;
   double **t2e;
   double **t2f;
   double **t2g;
   double **t2h;
   double *t1a;
   double *t1b;
   double *t1c;
   double *t1d;
   double *t1e;
   double *t1f;
   double *t1g;
   double *t1h;

   ressqr = lev_res[numlev-1] * lev_res[numlev-1];
   i_off = gp[procid].rownum*numrows;
   j_off = gp[procid].colnum*numcols;

/*   ***************************************************************

          f i r s t     p h a s e   (of timestep calculation)

     ***************************************************************/

   t2a = (double **) ga[procid];
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[0][0]=0.0;
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[im-1][0]=0.0;
   }
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[0][jm-1]=0.0;
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[im-1][jm-1]=0.0;
   }
   if (gp[procid].neighbors[UP] == -1) {
     t1a = (double *) t2a[0];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = 0.0;
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     t1a = (double *) t2a[im-1];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = 0.0;
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][0] = 0.0;
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][jm-1] = 0.0;
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     t1a = (double *) t2a[i];
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
       t1a[iindex] = 0.0;
     }
   }

   t2a = (double **) gb[procid];
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[0][0]=0.0;
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[im-1][0]=0.0;
   }
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[0][jm-1]=0.0;
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[im-1][jm-1]=0.0;
   }
   if (gp[procid].neighbors[UP] == -1) {
     t1a = (double *) t2a[0];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = 0.0;
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     t1a = (double *) t2a[im-1];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = 0.0;
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][0] = 0.0;
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][jm-1] = 0.0;
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     t1a = (double *) t2a[i];
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
       t1a[iindex] = 0.0;
     }
   }

/* put the laplacian of psi{1,3} in work1{1,2}
   note that psi(i,j,2) represents the psi3 array in
   the original equations  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     t2a = (double **) work1[procid][psiindex];
     if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
       t2a[0][0] = 0;
     }
     if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
       t2a[im-1][0] = 0;
     }
     if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
       t2a[0][jm-1] = 0;
     }
     if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
       t2a[im-1][jm-1] = 0;
     }
     laplacalc(procid,psi,work1,psiindex,
	       firstrow,lastrow,firstcol,lastcol);
   }

/* set values of work2 array to psi1 - psi3   */

   t2a = (double **) work2[procid];
   t2b = (double **) psi[procid][0];
   t2c = (double **) psi[procid][1];
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[0][0] = t2b[0][0]-t2c[0][0];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[im-1][0] = t2b[im-1][0]-t2c[im-1][0];
   }
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[0][jm-1] = t2b[0][jm-1]-t2c[0][jm-1];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[im-1][jm-1] = t2b[im-1][jm-1] -
				 t2c[im-1][jm-1];
   }
   if (gp[procid].neighbors[UP] == -1) {
     t1a = (double *) t2a[0];
     t1b = (double *) t2b[0];
     t1c = (double *) t2c[0];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = t1b[j]-t1c[j];
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     t1a = (double *) t2a[im-1];
     t1b = (double *) t2b[im-1];
     t1c = (double *) t2c[im-1];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = t1b[j]-t1c[j];
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][0] = t2b[j][0]-t2c[j][0];
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][jm-1] = t2b[j][jm-1]-t2c[j][jm-1];
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     t1a = (double *) t2a[i];
     t1b = (double *) t2b[i];
     t1c = (double *) t2c[i];
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
         t1a[iindex] = t1b[iindex] - t1c[iindex];
     }
   }

/* set values of work3 array to h3/h * psi1 + h1/h * psi3  */

   t2a = (double **) work3[procid];
   hh3 = h3/h;
   hh1 = h1/h;
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[0][0] = hh3*t2a[0][0]+hh1*t2c[0][0];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[im-1][0] = hh3*t2a[im-1][0] +
			      hh1*t2c[im-1][0];
   }
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[0][jm-1] = hh3*t2a[0][jm-1] +
			      hh1*t2c[0][jm-1];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[im-1][jm-1] = hh3*t2a[im-1][jm-1] +
				 hh1*t2c[im-1][jm-1];
   }
   if (gp[procid].neighbors[UP] == -1) {
     for(j=firstcol;j<=lastcol;j++) {
       t2a[0][j] = hh3*t2a[0][j]+hh1*t2c[0][j];
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     for(j=firstcol;j<=lastcol;j++) {
       t2a[im-1][j] = hh3*t2a[im-1][j] +
				hh1*t2c[im-1][j];
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][0] = hh3*t2a[j][0]+hh1*t2c[j][0];
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][jm-1] = hh3*t2a[j][jm-1] +
				hh1*t2c[j][jm-1];
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     t1a = (double *) t2a[i];
     t1c = (double *) t2c[i];
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
        t1a[iindex] = hh3*t1a[iindex] + hh1*t1c[iindex];
     }
   }

/* set values of temparray{1,3} to psim{1,3}  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     t2a = (double **) temparray[procid][psiindex];
     t2b = (double **) psi[procid][psiindex];
     if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
       t2a[0][0] = t2b[0][0];
     }
     if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
       t2a[im-1][0] = t2b[im-1][0];
     }
     if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
       t2a[0][jm-1] = t2b[0][jm-1];
     }
     if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
       t2a[im-1][jm-1] = t2b[im-1][jm-1];
     }
     if (gp[procid].neighbors[UP] == -1) {
       for(j=firstcol;j<=lastcol;j++) {
         t2a[0][j] = t2b[0][j];
       }
     }
     if (gp[procid].neighbors[DOWN] == -1) {
       for(j=firstcol;j<=lastcol;j++) {
         t2a[im-1][j] = t2b[im-1][j];
       }
     }
     if (gp[procid].neighbors[LEFT] == -1) {
       for(j=firstrow;j<=lastrow;j++) {
         t2a[j][0] = t2b[j][0];
       }
     }
     if (gp[procid].neighbors[RIGHT] == -1) {
       for(j=firstrow;j<=lastrow;j++) {
         t2a[j][jm-1] = t2b[j][jm-1];
       }
     }

     for(i=firstrow;i<=lastrow;i++) {
       t1a = (double *) t2a[i];
       t1b = (double *) t2b[i];
       for(iindex=firstcol;iindex<=lastcol;iindex++) {
         t1a[iindex] = t1b[iindex];
       }
     }
   }
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_1,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
/*     *******************************************************

              s e c o n d   p h a s e

       *******************************************************

   set values of psi{1,3} to psim{1,3}   */

   for(psiindex=0;psiindex<=1;psiindex++) {
     t2a = (double **) psi[procid][psiindex];
     t2b = (double **) psim[procid][psiindex];
     if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
       t2a[0][0] = t2b[0][0];
     }
     if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
       t2a[0][jm-1] = t2b[0][jm-1];
     }
     if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
       t2a[im-1][0] = t2b[im-1][0];
     }
     if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
       t2a[im-1][jm-1] = t2b[im-1][jm-1];
     }
     if (gp[procid].neighbors[UP] == -1) {
       for(j=firstcol;j<=lastcol;j++) {
         t2a[0][j] = t2b[0][j];
       }
     }
     if (gp[procid].neighbors[DOWN] == -1) {
       for(j=firstcol;j<=lastcol;j++) {
         t2a[im-1][j] = t2b[im-1][j];
       }
     }
     if (gp[procid].neighbors[LEFT] == -1) {
       for(j=firstrow;j<=lastrow;j++) {
         t2a[j][0] = t2b[j][0];
       }
     }
     if (gp[procid].neighbors[RIGHT] == -1) {
       for(j=firstrow;j<=lastrow;j++) {
         t2a[j][jm-1] = t2b[j][jm-1];
       }
     }

     for(i=firstrow;i<=lastrow;i++) {
       t1a = (double *) t2a[i];
       t1b = (double *) t2b[i];
       for(iindex=firstcol;iindex<=lastcol;iindex++) {
         t1a[iindex] = t1b[iindex];
       }
     }
   }

/* put the laplacian of the psim array
   into the work7 array; first part of a three-laplacian
   calculation to compute the friction terms  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     t2a = (double **) work7[procid][psiindex];
     if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
       t2a[0][0] = 0;
     }
     if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
       t2a[im-1][0] = 0;
     }
     if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
       t2a[0][jm-1] = 0;
     }
     if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
       t2a[im-1][jm-1] = 0;
     }
     laplacalc(procid,psim,work7,psiindex,
	       firstrow,lastrow,firstcol,lastcol);
   }

/* to the values of the work1{1,2} arrays obtained from the
   laplacians of psi{1,2} in the previous phase, add to the
   elements of every column the corresponding value in the
   one-dimenional f array  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     t2a = (double **) work1[procid][psiindex];
     if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
       t2a[0][0] = t2a[0][0] + f[0];
     }
     if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
       t2a[im-1][0] = t2a[im-1][0] + f[0];
     }
     if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
       t2a[0][jm-1] = t2a[0][jm-1] + f[jmx[numlev-1]-1];
     }
     if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
       t2a[im-1][jm-1]=t2a[im-1][jm-1] + f[jmx[numlev-1]-1];
     }
     if (gp[procid].neighbors[UP] == -1) {
       for(j=firstcol;j<=lastcol;j++) {
         t2a[0][j] = t2a[0][j] + f[j+j_off];
       }
     }
     if (gp[procid].neighbors[DOWN] == -1) {
       for(j=firstcol;j<=lastcol;j++) {
         t2a[im-1][j] = t2a[im-1][j] + f[j+j_off];
       }
     }
     if (gp[procid].neighbors[LEFT] == -1) {
       for(j=firstrow;j<=lastrow;j++) {
         t2a[j][0] = t2a[j][0] + f[j+i_off];
       }
     }
     if (gp[procid].neighbors[RIGHT] == -1) {
       for(j=firstrow;j<=lastrow;j++) {
         t2a[j][jm-1] = t2a[j][jm-1] + f[j+i_off];
       }
     }
     for(i=firstrow;i<=lastrow;i++) {
       t1a = (double *) t2a[i];
       for(iindex=firstcol;iindex<=lastcol;iindex++) {
         t1a[iindex]=t1a[iindex] + f[iindex+j_off];
       }
     }
   }
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_2,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
/* 	*******************************************************

                 t h i r d   p h a s e

 	*******************************************************

   put the jacobian of the work1{1,2} and psi{1,3} arrays
   (the latter currently in temparray) in the work5{1,2} arrays  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     jacobcalc2(work1,temparray,work5,psiindex,procid,firstrow,lastrow,
	       firstcol,lastcol);
   }

/* set values of psim{1,3} to temparray{1,3}  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     t2a = (double **) psim[procid][psiindex];
     t2b = (double **) temparray[procid][psiindex];
     if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
       t2a[0][0] = t2b[0][0];
     }
     if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
       t2a[im-1][0] = t2b[im-1][0];
     }
     if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
       t2a[0][jm-1] = t2b[0][jm-1];
     }
     if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
       t2a[im-1][jm-1] = t2b[im-1][jm-1];
     }
     if (gp[procid].neighbors[UP] == -1) {
       t1a = (double *) t2a[0];
       t1b = (double *) t2b[0];
       for(j=firstcol;j<=lastcol;j++) {
         t1a[j] = t1b[j];
       }
     }
     if (gp[procid].neighbors[DOWN] == -1) {
       t1a = (double *) t2a[im-1];
       t1b = (double *) t2b[im-1];
       for(j=firstcol;j<=lastcol;j++) {
         t1a[j] = t1b[j];
       }
     }
     if (gp[procid].neighbors[LEFT] == -1) {
       for(j=firstrow;j<=lastrow;j++) {
         t2a[j][0] = t2b[j][0];
       }
     }
     if (gp[procid].neighbors[RIGHT] == -1) {
       for(j=firstrow;j<=lastrow;j++) {
         t2a[j][jm-1] = t2b[j][jm-1];
       }
     }
     for(i=firstrow;i<=lastrow;i++) {
       t1a = (double *) t2a[i];
       t1b = (double *) t2b[i];
       for(iindex=firstcol;iindex<=lastcol;iindex++) {
         t1a[iindex] = t1b[iindex];
       }
     }
   }

/* put the laplacian of the work7{1,2} arrays in the work4{1,2}
   arrays; second step in the three-laplacian friction calculation  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     laplacalc(procid,work7,work4,psiindex,
	       firstrow,lastrow,firstcol,lastcol);
   }
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_3,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
/*     *******************************************************

                f o u r t h   p h a s e

       *******************************************************

   put the jacobian of the work2 and work3 arrays in the work6
   array  */

   jacobcalc(work2,work3,work6,procid,firstrow,lastrow,firstcol,lastcol);

/* put the laplacian of the work4{1,2} arrays in the work7{1,2}
   arrays; third step in the three-laplacian friction calculation  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     laplacalc(procid,work4,work7,psiindex,
	       firstrow,lastrow,firstcol,lastcol);
   }
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_4,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
/*     *******************************************************

                f i f t h   p h a s e

       *******************************************************

   use the values of the work5, work6 and work7 arrays
   computed in the previous time-steps to compute the
   ga and gb arrays   */

   hinv = 1.0/h;
   h1inv = 1.0/h1;

   t2a = (double **) ga[procid];
   t2b = (double **) gb[procid];
   t2c = (double **) work5[procid][0];
   t2d = (double **) work5[procid][1];
   t2e = (double **) work7[procid][0];
   t2f = (double **) work7[procid][1];
   t2g = (double **) work6[procid];
   t2h = (double **) tauz[procid];
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[0][0] = t2c[0][0]-t2d[0][0] +
			eig2*t2g[0][0]+h1inv*t2h[0][0] +
			lf*t2e[0][0]-lf*t2f[0][0];
     t2b[0][0] = hh1*t2c[0][0]+hh3*t2d[0][0] +
			hinv*t2h[0][0]+lf*hh1*t2e[0][0] +
		        lf*hh3*t2f[0][0];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[im-1][0] = t2c[im-1][0]-t2d[im-1][0] +
	   eig2*t2g[im-1][0] + h1inv*t2h[im-1][0] +
	   lf*t2e[im-1][0] - lf*t2f[im-1][0];
     t2b[im-1][0] = hh1*t2c[im-1][0] +
	   hh3*t2d[im-1][0] + hinv*t2h[im-1][0] +
	   lf*hh1*t2e[im-1][0] + lf*hh3*t2f[im-1][0];
   }
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[0][jm-1] = t2c[0][jm-1]-t2d[0][jm-1]+
	   eig2*t2g[0][jm-1]+h1inv*t2h[0][jm-1] +
	   lf*t2e[0][jm-1]-lf*t2f[0][jm-1];
     t2b[0][jm-1] = hh1*t2c[0][jm-1] +
	   hh3*t2d[0][jm-1]+hinv*t2h[0][jm-1] +
	   lf*hh1*t2e[0][jm-1]+lf*hh3*t2f[0][jm-1];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[im-1][jm-1] = t2c[im-1][jm-1] -
	   t2d[im-1][jm-1]+eig2*t2g[im-1][jm-1] +
	   h1inv*t2h[im-1][jm-1]+lf*t2e[im-1][jm-1] -
	   lf*t2f[im-1][jm-1];
     t2b[im-1][jm-1] = hh1*t2c[im-1][jm-1] +
	   hh3*t2d[im-1][jm-1]+hinv*t2h[im-1][jm-1] +
	   lf*hh1*t2e[im-1][jm-1] +
	   lf*hh3*t2f[im-1][jm-1];
   }
   if (gp[procid].neighbors[UP] == -1) {
     t1a = (double *) t2a[0];
     t1b = (double *) t2b[0];
     t1c = (double *) t2c[0];
     t1d = (double *) t2d[0];
     t1e = (double *) t2e[0];
     t1f = (double *) t2f[0];
     t1g = (double *) t2g[0];
     t1h = (double *) t2h[0];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = t1c[j]-t1d[j] +
	   eig2*t1g[j]+h1inv*t1h[j] +
	   lf*t1e[j]-lf*t1f[j];
       t1b[j] = hh1*t1c[j] +
	   hh3*t1d[j]+hinv*t1h[j] +
	   lf*hh1*t1e[j]+lf*hh3*t1f[j];
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     t1a = (double *) t2a[im-1];
     t1b = (double *) t2b[im-1];
     t1c = (double *) t2c[im-1];
     t1d = (double *) t2d[im-1];
     t1e = (double *) t2e[im-1];
     t1f = (double *) t2f[im-1];
     t1g = (double *) t2g[im-1];
     t1h = (double *) t2h[im-1];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = t1c[j] -
	   t1d[j]+eig2*t1g[j] +
	   h1inv*t1h[j]+lf*t1e[j] -
	   lf*t1f[j];
       t1b[j] = hh1*t1c[j] +
	   hh3*t1d[j]+hinv*t1h[j] +
	   lf*hh1*t1e[j]+lf*hh3*t1f[j];
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][0] = t2c[j][0]-t2d[j][0] +
	   eig2*t2g[j][0]+h1inv*t2h[j][0] +
	   lf*t2e[j][0]-lf*t2f[j][0];
       t2b[j][0] = hh1*t2c[j][0] +
	   hh3*t2d[j][0]+hinv*t2h[j][0] +
	   lf*hh1*t2e[j][0]+lf*hh3*t2f[j][0];
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][jm-1] = t2c[j][jm-1] -
	   t2d[j][jm-1]+eig2*t2g[j][jm-1] +
	   h1inv*t2h[j][jm-1]+lf*t2e[j][jm-1] -
	   lf*t2f[j][jm-1];
       t2b[j][jm-1] = hh1*t2c[j][jm-1] +
	   hh3*t2d[j][jm-1]+hinv*t2h[j][jm-1] +
	   lf*hh1*t2e[j][jm-1]+lf*hh3*t2f[j][jm-1];
     }
   }

   for(i=firstrow;i<=lastrow;i++) {
     t1a = (double *) t2a[i];
     t1b = (double *) t2b[i];
     t1c = (double *) t2c[i];
     t1d = (double *) t2d[i];
     t1e = (double *) t2e[i];
     t1f = (double *) t2f[i];
     t1g = (double *) t2g[i];
     t1h = (double *) t2h[i];
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
       t1a[iindex] = t1c[iindex] -
	   t1d[iindex]+eig2*t1g[iindex] +
	   h1inv*t1h[iindex]+lf*t1e[iindex] -
	   lf*t1f[iindex];
       t1b[iindex] = hh1*t1c[iindex] +
	   hh3*t1d[iindex]+hinv*t1h[iindex] +
	   lf*hh1*t1e[iindex] +
	   lf*hh3*t1f[iindex];
     }
   }
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_5,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
/*     *******************************************************

               s i x t h   p h a s e

       *******************************************************  */

   istart = 1;
   iend = istart + gp[procid].rel_num_y[numlev-1] - 1;
   jstart = 1;
   jend = jstart + gp[procid].rel_num_x[numlev-1] - 1;
   ist = istart;
   ien = iend;
   jst = jstart;
   jen = jend;

   if (gp[procid].neighbors[UP] == -1) {
     istart = 0;
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     jstart = 0;
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     iend = im-1;
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     jend = jm-1;
   }
   t2a = (double **) rhs_multi[procid][numlev-1];
   t2b = (double **) ga[procid];
   t2c = (double **) oldga[procid];
   t2d = (double **) q_multi[procid][numlev-1];
   for(i=istart;i<=iend;i++) {
     t1a = (double *) t2a[i];
     t1b = (double *) t2b[i];
     for(j=jstart;j<=jend;j++) {
       t1a[j] = t1b[j] * ressqr;
     }
   }

   if (gp[procid].neighbors[UP] == -1) {
     t1d = (double *) t2d[0];
     t1b = (double *) t2b[0];
     for(j=jstart;j<=jend;j++) {
       t1d[j] = t1b[j];
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     t1d = (double *) t2d[im-1];
     t1b = (double *) t2b[im-1];
     for(j=jstart;j<=jend;j++) {
       t1d[j] = t1b[j];
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for(i=istart;i<=iend;i++) {
       t2d[i][0] = t2b[i][0];
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for(i=istart;i<=iend;i++) {
       t2d[i][jm-1] = t2b[i][jm-1];
     }
   }

   fac = 1.0 / (4.0 - ressqr*eig2);
   for(i=ist;i<=ien;i++) {
     t1d = (double *) t2d[i];
     t1c = (double *) t2c[i];
     for(j=jst;j<=jen;j++) {
       t1d[j] = t1c[j];
     }
   }

   if ((procid == MASTER) || (do_stats)) {
     CLOCK(multi_start);
   }

   multig(procid);

   if ((procid == MASTER) || (do_stats)) {
     CLOCK(multi_end);
     gp[procid].multi_time += (multi_end - multi_start);
   }

/* the shared sum variable psiai is initialized to 0 at
   every time-step  */

   if (procid == MASTER) {
     global->psiai=0.0;
   }

/*  copy the solution for use as initial guess in next time-step  */

   for(i=istart;i<=iend;i++) {
     t1b = (double *) t2b[i];
     t1c = (double *) t2c[i];
     t1d = (double *) t2d[i];
     for(j=jstart;j<=jend;j++) {
       t1b[j] = t1d[j];
       t1c[j] = t1d[j];
     }
   }
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_6,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
/*     *******************************************************

                s e v e n t h   p h a s e

       *******************************************************

   every process computes the running sum for its assigned portion
   in a private variable psiaipriv   */

   psiaipriv=0.0;
   t2a = (double **) ga[procid];
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     psiaipriv = psiaipriv + 0.25*(t2a[0][0]);
   }
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     psiaipriv = psiaipriv + 0.25*(t2a[0][jm-1]);
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     psiaipriv=psiaipriv+0.25*(t2a[im-1][0]);
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     psiaipriv=psiaipriv+0.25*(t2a[im-1][jm-1]);
   }
   if (gp[procid].neighbors[UP] == -1) {
     t1a = (double *) t2a[0];
     for(j=firstcol;j<=lastcol;j++) {
       psiaipriv = psiaipriv + 0.5*t1a[j];
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     t1a = (double *) t2a[im-1];
     for(j=firstcol;j<=lastcol;j++) {
       psiaipriv = psiaipriv + 0.5*t1a[j];
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       psiaipriv = psiaipriv + 0.5*t2a[j][0];
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       psiaipriv = psiaipriv + 0.5*t2a[j][jm-1];
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     t1a = (double *) t2a[i];
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
       psiaipriv = psiaipriv + t1a[iindex];
     }
   }

/* after computing its private sum, every process adds that to the
   shared running sum psiai  */

   LOCK(locks->psiailock)
   global->psiai = global->psiai + psiaipriv;
   UNLOCK(locks->psiailock)
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_7,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
/*      *******************************************************

                e i g h t h   p h a s e

        *******************************************************

   augment ga(i,j) with [-psiai/psibi]*psib(i,j) */

   f4 = (-global->psiai)/(global->psibi);

   t2a = (double **) ga[procid];
   t2b = (double **) psib[procid];
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[0][0] = t2a[0][0]+f4*t2b[0][0];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[im-1][0] = t2a[im-1][0]+f4*t2b[im-1][0];
   }
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[0][jm-1] = t2a[0][jm-1]+f4*t2b[0][jm-1];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[im-1][jm-1] = t2a[im-1][jm-1] +
			      f4*t2b[im-1][jm-1];
   }
   if (gp[procid].neighbors[UP] == -1) {
     t1a = (double *) t2a[0];
     t1b = (double *) t2b[0];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = t1a[j]+f4*t1b[j];
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     t1a = (double *) t2a[im-1];
     t1b = (double *) t2b[im-1];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = t1a[j]+f4*t1b[j];
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][0] = t2a[j][0]+f4*t2b[j][0];
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][jm-1] = t2a[j][jm-1]+f4*t2b[j][jm-1];
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     t1a = (double *) t2a[i];
     t1b = (double *) t2b[i];
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
       t1a[iindex] = t1a[iindex]+f4*t1b[iindex];
     }
   }

   t2a = (double **) rhs_multi[procid][numlev-1];
   t2b = (double **) gb[procid];
   t2c = (double **) oldgb[procid];
   t2d = (double **) q_multi[procid][numlev-1];
   for(i=istart;i<=iend;i++) {
     t1a = (double *) t2a[i];
     t1b = (double *) t2b[i];
     for(j=jstart;j<=jend;j++) {
       t1a[j] = t1b[j] * ressqr;
     }
   }
   if (gp[procid].neighbors[UP] == -1) {
     t1d = (double *) t2d[0];
     t1b = (double *) t2b[0];
     for(j=jstart;j<=jend;j++) {
       t1d[j] = t1b[j];
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     t1d = (double *) t2d[im-1];
     t1b = (double *) t2b[im-1];
     for(j=jstart;j<=jend;j++) {
       t1d[j] = t1b[j];
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for(i=istart;i<=iend;i++) {
       t2d[i][0] = t2b[i][0];
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for(i=istart;i<=iend;i++) {
       t2d[i][jm-1] = t2b[i][jm-1];
     }
   }

   fac = 1.0 / (4.0 - ressqr*eig2);
   for(i=ist;i<=ien;i++) {
     t1d = (double *) t2d[i];
     t1c = (double *) t2c[i];
     for(j=jst;j<=jen;j++) {
       t1d[j] = t1c[j];
     }
   }

   if ((procid == MASTER) || (do_stats)) {
     CLOCK(multi_start);
   }

   multig(procid);

   if ((procid == MASTER) || (do_stats)) {
     CLOCK(multi_end);
     gp[procid].multi_time += (multi_end - multi_start);
   }

   for(i=istart;i<=iend;i++) {
     t1b = (double *) t2b[i];
     t1c = (double *) t2c[i];
     t1d = (double *) t2d[i];
     for(j=jstart;j<=jend;j++) {
       t1b[j] = t1d[j];
       t1c[j] = t1d[j];
     }
   }
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_8,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
/*      *******************************************************

                n i n t h   p h a s e

        *******************************************************

   put appropriate linear combinations of ga and gb in work2 and work3;
   note that here (as in most cases) the constant multipliers are made
   private variables; the specific order in which things are done is
   chosen in order to hopefully reuse things brought into the cache

   note that here again we choose to have all processes share the work
   on both matrices despite the fact that the work done per element
   is the same, because the operand matrices are the same in both cases */

   t2a = (double **) ga[procid];
   t2b = (double **) gb[procid];
   t2c = (double **) work2[procid];
   t2d = (double **) work3[procid];
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2c[0][0] = t2b[0][0]-hh1*t2a[0][0];
     t2d[0][0] = t2b[0][0]+hh3*t2a[0][0];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2c[im-1][0] = t2b[im-1][0]-hh1*t2a[im-1][0];
     t2d[im-1][0] = t2b[im-1][0]+hh3*t2a[im-1][0];
   }
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2c[0][jm-1] = t2b[0][jm-1]-hh1*t2a[0][jm-1];
     t2d[0][jm-1] = t2b[0][jm-1]+hh3*t2a[0][jm-1];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2c[im-1][jm-1] = t2b[im-1][jm-1] -
				 hh1*t2a[im-1][jm-1];
     t2d[im-1][jm-1] = t2b[im-1][jm-1] +
				 hh3*t2a[im-1][jm-1];
   }
   if (gp[procid].neighbors[UP] == -1) {
     t1a = (double *) t2a[0];
     t1b = (double *) t2b[0];
     t1c = (double *) t2c[0];
     t1d = (double *) t2d[0];
     for(j=firstcol;j<=lastcol;j++) {
       t1d[j] = t1b[j]+hh3*t1a[j];
       t1c[j] = t1b[j]-hh1*t1a[j];
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     t1a = (double *) t2a[im-1];
     t1b = (double *) t2b[im-1];
     t1c = (double *) t2c[im-1];
     t1d = (double *) t2d[im-1];
     for(j=firstcol;j<=lastcol;j++) {
       t1d[j] = t1b[j]+hh3*t1a[j];
       t1c[j] = t1b[j]-hh1*t1a[j];
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2d[j][0] = t2b[j][0]+hh3*t2a[j][0];
       t2c[j][0] = t2b[j][0]-hh1*t2a[j][0];
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2d[j][jm-1] = t2b[j][jm-1]+hh3*t2a[j][jm-1];
       t2c[j][jm-1] = t2b[j][jm-1]-hh1*t2a[j][jm-1];
     }
   }

   for(i=firstrow;i<=lastrow;i++) {
     t1a = (double *) t2a[i];
     t1b = (double *) t2b[i];
     t1c = (double *) t2c[i];
     t1d = (double *) t2d[i];
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
       t1d[iindex] = t1b[iindex] + hh3*t1a[iindex];
       t1c[iindex] = t1b[iindex] - hh1*t1a[iindex];
     }
   }
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_9,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
/*      *******************************************************

                t e n t h    p h a s e

        *******************************************************/


   timst = 2*dtau;

/* update the psi{1,3} matrices by adding 2*dtau*work3 to each */

   t2a = (double **) psi[procid][0];
   t2b = (double **) work3[procid];
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[0][0] = t2a[0][0] + timst*t2b[0][0];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[im-1][0] = t2a[im-1][0] +
			       timst*t2b[im-1][0];
   }
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[0][jm-1] = t2a[0][jm-1] +
			       timst*t2b[0][jm-1];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[im-1][jm-1] = t2a[im-1][jm-1] +
				  timst*t2b[im-1][jm-1];
   }
   if (gp[procid].neighbors[UP] == -1) {
     t1a = (double *) t2a[0];
     t1b = (double *) t2b[0];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = t1a[j] + timst*t1b[j];
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     t1a = (double *) t2a[im-1];
     t1b = (double *) t2b[im-1];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = t1a[j] + timst*t1b[j];
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][0] = t2a[j][0] + timst*t2b[j][0];
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][jm-1] = t2a[j][jm-1] +
				 timst*t2b[j][jm-1];
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     t1a = (double *) t2a[i];
     t1b = (double *) t2b[i];
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
         t1a[iindex] = t1a[iindex] + timst*t1b[iindex];
     }
   }

   t2a = (double **) psi[procid][1];
   t2b = (double **) work2[procid];
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[0][0] = t2a[0][0] + timst*t2b[0][0];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[LEFT] == -1)) {
     t2a[im-1][0] = t2a[im-1][0] +
			       timst*t2b[im-1][0];
   }
   if ((gp[procid].neighbors[UP] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[0][jm-1] = t2a[0][jm-1] +
			       timst*t2b[0][jm-1];
   }
   if ((gp[procid].neighbors[DOWN] == -1) && (gp[procid].neighbors[RIGHT] == -1)) {
     t2a[im-1][jm-1] = t2a[im-1][jm-1] +
				  timst*t2b[im-1][jm-1];
   }
   if (gp[procid].neighbors[UP] == -1) {
     t1a = (double *) t2a[0];
     t1b = (double *) t2b[0];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = t1a[j] + timst*t1b[j];
     }
   }
   if (gp[procid].neighbors[DOWN] == -1) {
     t1a = (double *) t2a[im-1];
     t1b = (double *) t2b[im-1];
     for(j=firstcol;j<=lastcol;j++) {
       t1a[j] = t1a[j] + timst*t1b[j];
     }
   }
   if (gp[procid].neighbors[LEFT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][0] = t2a[j][0] + timst*t2b[j][0];
     }
   }
   if (gp[procid].neighbors[RIGHT] == -1) {
     for(j=firstrow;j<=lastrow;j++) {
       t2a[j][jm-1] = t2a[j][jm-1] +
				 timst*t2b[j][jm-1];
     }
   }

   for(i=firstrow;i<=lastrow;i++) {
     t1a = (double *) t2a[i];
     t1b = (double *) t2b[i];
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
         t1a[iindex] = t1a[iindex] + timst*t1b[iindex];
     }
   }
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_10,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
}
