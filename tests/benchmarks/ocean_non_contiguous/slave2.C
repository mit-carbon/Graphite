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
   double timst;
   double f4;
   long psiindex;
   double psiaipriv;
   long multi_start;
   long multi_end;

   ressqr = lev_res[numlev-1] * lev_res[numlev-1];

/*   ***************************************************************

          f i r s t     p h a s e   (of timestep calculation)

     ***************************************************************/

   if (procid == MASTER) {
     wrk1->ga[0][0]=0.0;
   }
   if (procid == nprocs-xprocs) {
     wrk1->ga[im-1][0]=0.0;
   }
   if (procid == xprocs-1) {
     wrk1->ga[0][jm-1]=0.0;
   }
   if (procid == nprocs-1) {
     wrk1->ga[im-1][jm-1]=0.0;
   }
   if (firstrow == 1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk1->ga[0][j] = 0.0;
     }
   }
   if ((firstrow+numrows) == im-1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk1->ga[im-1][j] = 0.0;
     }
   }
   if (firstcol == 1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk1->ga[j][0] = 0.0;
     }
   }
   if ((firstcol+numcols) == jm-1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk1->ga[j][jm-1] = 0.0;
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
         wrk1->ga[i][iindex] = 0.0;
     }
   }

   if (procid == MASTER) {
     wrk1->gb[0][0]=0.0;
   }
   if (procid == nprocs-xprocs) {
     wrk1->gb[im-1][0]=0.0;
   }
   if (procid == xprocs-1) {
     wrk1->gb[0][jm-1]=0.0;
   }
   if (procid == nprocs-1) {
     wrk1->gb[im-1][jm-1]=0.0;
   }
   if (firstrow == 1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk1->gb[0][j] = 0.0;
     }
   }
   if ((firstrow+numrows) == im-1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk1->gb[im-1][j] = 0.0;
     }
   }
   if (firstcol == 1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk1->gb[j][0] = 0.0;
     }
   }
   if ((firstcol+numcols) == jm-1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk1->gb[j][jm-1] = 0.0;
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
       wrk1->gb[i][iindex] = 0.0;
     }
   }

/* put the laplacian of psi{1,3} in work1{1,2}
   note that psi(i,j,2) represents the psi3 array in
   the original equations  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     if (procid == MASTER) {
       wrk3->work1[psiindex][0][0] = 0;
     }
     if (procid == nprocs-xprocs) {
       wrk3->work1[psiindex][im-1][0] = 0;
     }
     if (procid == xprocs-1) {
       wrk3->work1[psiindex][0][jm-1] = 0;
     }
     if (procid == nprocs-1) {
       wrk3->work1[psiindex][im-1][jm-1] = 0;
     }
     laplacalc(fields->psi[psiindex],
	       wrk3->work1[psiindex],
	       firstrow,lastrow,firstcol,lastcol,numrows,numcols);

   }


   if (procid == MASTER) {
     wrk3->work2[0][0] = fields->psi[0][0][0]-fields->psi[1][0][0];
   }
   if (procid == nprocs-xprocs) {
     wrk3->work2[im-1][0] = fields->psi[0][im-1][0]-fields->psi[1][im-1][0];
   }
   if (procid == xprocs-1) {
     wrk3->work2[0][jm-1] = fields->psi[0][0][jm-1]-fields->psi[1][0][jm-1];
   }
   if (procid == nprocs-1) {
     wrk3->work2[im-1][jm-1] = fields->psi[0][im-1][jm-1]-fields->psi[1][im-1][jm-1];
   }
   if (firstrow == 1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk3->work2[0][j] = fields->psi[0][0][j]-fields->psi[1][0][j];
     }
   }
   if ((firstrow+numrows) == im-1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk3->work2[im-1][j] = fields->psi[0][im-1][j]-fields->psi[1][im-1][j];
     }
   }
   if (firstcol == 1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk3->work2[j][0] = fields->psi[0][j][0]-fields->psi[1][j][0];
     }
   }
   if ((firstcol+numcols) == jm-1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk3->work2[j][jm-1] = fields->psi[0][j][jm-1]-fields->psi[1][j][jm-1];
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
         wrk3->work2[i][iindex] = fields->psi[0][i][iindex]-fields->psi[1][i][iindex];
     }
   }

/* set values of work3 array to h3/h * psi1 + h1/h * psi3  */

   hh3 = h3/h;
   hh1 = h1/h;
   if (procid == MASTER) {
     wrk2->work3[0][0] = hh3*fields->psi[0][0][0]+hh1*fields->psi[1][0][0];
   }
   if (procid == nprocs-xprocs) {
     wrk2->work3[im-1][0] = hh3*fields->psi[0][im-1][0]+hh1*fields->psi[1][im-1][0];
   }
   if (procid == xprocs-1) {
     wrk2->work3[0][jm-1] = hh3*fields->psi[0][0][jm-1]+hh1*fields->psi[1][0][jm-1];
   }
   if (procid == nprocs-1) {
     wrk2->work3[im-1][jm-1] = hh3*fields->psi[0][im-1][jm-1]+hh1*fields->psi[1][im-1][jm-1];
   }
   if (firstrow == 1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk2->work3[0][j] = hh3*fields->psi[0][0][j]+hh1*fields->psi[1][0][j];
     }
   }
   if ((firstrow+numrows) == im-1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk2->work3[im-1][j] = hh3*fields->psi[0][im-1][j]+hh1*fields->psi[1][im-1][j];
     }
   }
   if (firstcol == 1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk2->work3[j][0] = hh3*fields->psi[0][j][0]+hh1*fields->psi[1][j][0];
     }
   }
   if ((firstcol+numcols) == jm-1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk2->work3[j][jm-1] = hh3*fields->psi[0][j][jm-1]+hh1*fields->psi[1][j][jm-1];
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
        wrk2->work3[i][iindex] = hh3*fields->psi[0][i][iindex]+hh1*fields->psi[1][i][iindex];
     }
   }

/* set values of temparray{1,3} to psim{1,3}  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     if (procid == MASTER) {
       wrk5->temparray[psiindex][0][0] = fields->psi[psiindex][0][0];
     }
     if (procid == nprocs-xprocs) {
       wrk5->temparray[psiindex][im-1][0] = fields->psi[psiindex][im-1][0];
     }
     if (procid == xprocs-1) {
       wrk5->temparray[psiindex][0][jm-1] = fields->psi[psiindex][0][jm-1];
     }
     if (procid == nprocs-1) {
       wrk5->temparray[psiindex][im-1][jm-1] = fields->psi[psiindex][im-1][jm-1];
     }
     if (firstrow == 1) {
       for(j=firstcol;j<=lastcol;j++) {
         wrk5->temparray[psiindex][0][j] = fields->psi[psiindex][0][j];
       }
     }
     if ((firstrow+numrows) == im-1) {
       for(j=firstcol;j<=lastcol;j++) {
         wrk5->temparray[psiindex][im-1][j] = fields->psi[psiindex][im-1][j];
       }
     }
     if (firstcol == 1) {
       for(j=firstrow;j<=lastrow;j++) {
         wrk5->temparray[psiindex][j][0] = fields->psi[psiindex][j][0];
       }
     }
     if ((firstcol+numcols) == jm-1) {
       for(j=firstrow;j<=lastrow;j++) {
         wrk5->temparray[psiindex][j][jm-1] = fields->psi[psiindex][j][jm-1];
       }
     }

     for(i=firstrow;i<=lastrow;i++) {
       for(iindex=firstcol;iindex<=lastcol;iindex++) {
         wrk5->temparray[psiindex][i][iindex] = fields->psi[psiindex][i][iindex];
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
     if (procid == MASTER) {
       fields->psi[psiindex][0][0] = fields->psim[psiindex][0][0];
     }
     if (procid == xprocs-1) {
       fields->psi[psiindex][0][jm-1] = fields->psim[psiindex][0][jm-1];
     }
     if (procid == nprocs-xprocs) {
       fields->psi[psiindex][im-1][0] = fields->psim[psiindex][im-1][0];
     }
     if (procid == nprocs-1) {
       fields->psi[psiindex][im-1][jm-1] = fields->psim[psiindex][im-1][jm-1];
     }
     if (firstrow == 1) {
       for(j=firstcol;j<=lastcol;j++) {
         fields->psi[psiindex][0][j] = fields->psim[psiindex][0][j];
       }
     }
     if ((firstrow+numrows) == im-1) {
       for(j=firstcol;j<=lastcol;j++) {
         fields->psi[psiindex][im-1][j] = fields->psim[psiindex][im-1][j];
       }
     }
     if (firstcol == 1) {
       for(j=firstrow;j<=lastrow;j++) {
         fields->psi[psiindex][j][0] = fields->psim[psiindex][j][0];
       }
     }
     if ((firstcol+numcols) == jm-1) {
       for(j=firstrow;j<=lastrow;j++) {
         fields->psi[psiindex][j][jm-1] = fields->psim[psiindex][j][jm-1];
       }
     }

     for(i=firstrow;i<=lastrow;i++) {
       for(iindex=firstcol;iindex<=lastcol;iindex++) {
         fields->psi[psiindex][i][iindex] = fields->psim[psiindex][i][iindex];
       }
     }
   }

/* put the laplacian of the psim array
   into the work7 array; first part of a three-laplacian
   calculation to compute the friction terms  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     if (procid == MASTER) {
       wrk5->work7[psiindex][0][0] = 0;
     }
     if (procid == nprocs-xprocs) {
       wrk5->work7[psiindex][im-1][0] = 0;
     }
     if (procid == xprocs-1) {
       wrk5->work7[psiindex][0][jm-1] = 0;
     }
     if (procid == nprocs-1) {
       wrk5->work7[psiindex][im-1][jm-1] = 0;
     }
     laplacalc(fields->psim[psiindex],wrk5->work7[psiindex],firstrow,lastrow,firstcol,lastcol,numrows,numcols);
   }

/* to the values of the work1{1,2} arrays obtained from the
   laplacians of psi{1,2} in the previous phase, add to the
   elements of every column the corresponding value in the
   one-dimenional f array  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     if (procid == MASTER) {
       wrk3->work1[psiindex][0][0] = wrk3->work1[psiindex][0][0] + wrk2->f[0];
     }
     if (procid == nprocs-xprocs) {
       wrk3->work1[psiindex][im-1][0] = wrk3->work1[psiindex][im-1][0] + wrk2->f[0];
     }
     if (procid == xprocs-1) {
       wrk3->work1[psiindex][0][jm-1] = wrk3->work1[psiindex][0][jm-1] + wrk2->f[jm-1];
     }
     if (procid == nprocs-1) {
       wrk3->work1[psiindex][im-1][jm-1] = wrk3->work1[psiindex][im-1][jm-1] + wrk2->f[jm-1];
     }
     if (firstrow == 1) {
       for(j=firstcol;j<=lastcol;j++) {
         wrk3->work1[psiindex][0][j] = wrk3->work1[psiindex][0][j] + wrk2->f[j];
       }
     }
     if ((firstrow+numrows) == im-1) {
       for(j=firstcol;j<=lastcol;j++) {
         wrk3->work1[psiindex][im-1][j] = wrk3->work1[psiindex][im-1][j] + wrk2->f[j];
       }
     }
     if (firstcol == 1) {
       for(j=firstrow;j<=lastrow;j++) {
         wrk3->work1[psiindex][j][0] = wrk3->work1[psiindex][j][0] + wrk2->f[j];
       }
     }
     if ((firstcol+numcols) == jm-1) {
       for(j=firstrow;j<=lastrow;j++) {
         wrk3->work1[psiindex][j][jm-1] = wrk3->work1[psiindex][j][jm-1] + wrk2->f[j];
       }
     }
     for(i=firstrow;i<=lastrow;i++) {
       for(iindex=firstcol;iindex<=lastcol;iindex++) {
         wrk3->work1[psiindex][i][iindex] = wrk3->work1[psiindex][i][iindex] +
					   wrk2->f[iindex];
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
     jacobcalc(wrk3->work1[psiindex],wrk5->temparray[psiindex],
               wrk4->work5[psiindex],procid,firstrow,lastrow,firstcol,lastcol,numrows,numcols);
   }


/* set values of psim{1,3} to temparray{1,3}  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     if (procid == MASTER) {
       fields->psim[psiindex][0][0] = wrk5->temparray[psiindex][0][0];
     }
     if (procid == nprocs-xprocs) {
       fields->psim[psiindex][im-1][0] = wrk5->temparray[psiindex][im-1][0];
     }
     if (procid == xprocs-1) {
       fields->psim[psiindex][0][jm-1] = wrk5->temparray[psiindex][0][jm-1];
     }
     if (procid == nprocs-1) {
       fields->psim[psiindex][im-1][jm-1] = wrk5->temparray[psiindex][im-1][jm-1];
     }
     if (firstrow == 1) {
       for(j=firstcol;j<=lastcol;j++) {
         fields->psim[psiindex][0][j] = wrk5->temparray[psiindex][0][j];
       }
     }
     if ((firstrow+numrows) == im-1) {
       for(j=firstcol;j<=lastcol;j++) {
         fields->psim[psiindex][im-1][j] = wrk5->temparray[psiindex][im-1][j];
       }
     }
     if (firstcol == 1) {
       for(j=firstrow;j<=lastrow;j++) {
         fields->psim[psiindex][j][0] = wrk5->temparray[psiindex][j][0];
       }
     }
     if ((firstcol+numcols) == jm-1) {
       for(j=firstrow;j<=lastrow;j++) {
         fields->psim[psiindex][j][jm-1] = wrk5->temparray[psiindex][j][jm-1];
       }
     }
     for(i=firstrow;i<=lastrow;i++) {
       for(iindex=firstcol;iindex<=lastcol;iindex++) {
         fields->psim[psiindex][i][iindex] = wrk5->temparray[psiindex][i][iindex];
       }
     }
   }

/* put the laplacian of the work7{1,2} arrays in the work4{1,2}
   arrays; second step in the three-laplacian friction calculation  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     laplacalc(wrk5->work7[psiindex],
	       wrk4->work4[psiindex],
               firstrow,lastrow,firstcol,lastcol,numrows,numcols);
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

   jacobcalc(wrk3->work2,wrk2->work3,wrk6->work6,procid,firstrow,
             lastrow,firstcol,lastcol,numrows,numcols);

/* put the laplacian of the work4{1,2} arrays in the work7{1,2}
   arrays; third step in the three-laplacian friction calculation  */

   for(psiindex=0;psiindex<=1;psiindex++) {
     laplacalc(wrk4->work4[psiindex],
               wrk5->work7[psiindex],
               firstrow,lastrow,firstcol,lastcol,numrows,numcols);
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

   if (procid == MASTER) {
     wrk1->ga[0][0] = wrk4->work5[0][0][0]-wrk4->work5[1][0][0]+eig2*wrk6->work6[0][0]+h1inv*
                frcng->tauz[0][0]+lf*wrk5->work7[0][0][0]-lf*wrk5->work7[1][0][0];
     wrk1->gb[0][0] = hh1*wrk4->work5[0][0][0]+hh3*wrk4->work5[1][0][0]+hinv*frcng->tauz[0][0]+
                lf*hh1*wrk5->work7[0][0][0]+lf*hh3*wrk5->work7[1][0][0];
   }
   if (procid == nprocs-xprocs) {
     wrk1->ga[im-1][0] = wrk4->work5[0][im-1][0]-wrk4->work5[1][im-1][0]+eig2*wrk6->work6[im-1][0]+h1inv*
                   frcng->tauz[im-1][0]+lf*wrk5->work7[0][im-1][0]-lf*wrk5->work7[1][im-1][0];
     wrk1->gb[im-1][0] = hh1*wrk4->work5[0][im-1][0]+hh3*wrk4->work5[1][im-1][0]+hinv*frcng->tauz[im-1][0]+
                   lf*hh1*wrk5->work7[0][im-1][0]+lf*hh3*wrk5->work7[1][im-1][0];
   }
   if (procid == xprocs-1) {
     wrk1->ga[0][jm-1] = wrk4->work5[0][0][jm-1]-wrk4->work5[1][0][jm-1]+eig2*wrk6->work6[0][jm-1]+h1inv*
                   frcng->tauz[0][jm-1]+lf*wrk5->work7[0][0][jm-1]-lf*wrk5->work7[1][0][jm-1];
     wrk1->gb[0][jm-1] = hh1*wrk4->work5[0][0][jm-1]+hh3*wrk4->work5[1][0][jm-1]+hinv*frcng->tauz[0][jm-1]+
                   lf*hh1*wrk5->work7[0][0][jm-1]+lf*hh3*wrk5->work7[1][0][jm-1];
   }
   if (procid == nprocs-1) {
     wrk1->ga[im-1][jm-1] = wrk4->work5[0][im-1][jm-1]-wrk4->work5[1][im-1][jm-1]+eig2*wrk6->work6[im-1][jm-1]+
                      h1inv*frcng->tauz[im-1][jm-1]+lf*wrk5->work7[0][im-1][jm-1]-lf*wrk5->work7[1][im-1][jm-1];
     wrk1->gb[im-1][jm-1] = hh1*wrk4->work5[0][im-1][jm-1]+hh3*wrk4->work5[1][im-1][jm-1]+hinv*
		    frcng->tauz[im-1][jm-1]+lf*hh1*wrk5->work7[0][im-1][jm-1]+lf*hh3*wrk5->work7[1][im-1][jm-1];
   }
   if (firstrow == 1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk1->ga[0][j] = wrk4->work5[0][0][j]-wrk4->work5[1][0][j]+eig2*
                    wrk6->work6[0][j]+h1inv*frcng->tauz[0][j]+lf*wrk5->work7[0][0][j]-lf*wrk5->work7[0][0][j];
       wrk1->gb[0][j] = hh1*wrk4->work5[0][0][j]+hh3*wrk4->work5[1][0][j]+hinv*
                    frcng->tauz[0][j]+lf*hh1*wrk5->work7[0][0][j]+lf*hh3*wrk5->work7[1][0][j];
     }
   }
   if ((firstrow+numrows) == im-1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk1->ga[im-1][j] = wrk4->work5[0][im-1][j]-wrk4->work5[1][im-1][j]+eig2*
                       wrk6->work6[im-1][j]+h1inv*frcng->tauz[im-1][j]+
                       lf*wrk5->work7[0][im-1][j]-lf*wrk5->work7[1][im-1][j];
       wrk1->gb[im-1][j] = hh1*wrk4->work5[0][im-1][j]+hh3*wrk4->work5[1][im-1][j]+hinv*
                       frcng->tauz[im-1][j]+lf*hh1*wrk5->work7[0][im-1][j]+
                       lf*hh3*wrk5->work7[1][im-1][j];
     }
   }
   if (firstcol == 1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk1->ga[j][0] = wrk4->work5[0][j][0]-wrk4->work5[1][j][0]+eig2*
                  wrk6->work6[j][0]+h1inv*frcng->tauz[j][0]+lf*wrk5->work7[0][j][0]-lf*wrk5->work7[1][j][0];
       wrk1->gb[j][0] = hh1*wrk4->work5[0][j][0]+hh3*wrk4->work5[1][j][0]+hinv*
                  frcng->tauz[j][0]+lf*hh1*wrk5->work7[0][j][0]+lf*hh3*wrk5->work7[1][j][0];
     }
   }
   if ((firstcol+numcols) == jm-1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk1->ga[j][jm-1] = wrk4->work5[0][j][jm-1]-wrk4->work5[1][j][jm-1]+eig2*
                       wrk6->work6[j][jm-1]+h1inv*frcng->tauz[j][jm-1]+
                       lf*wrk5->work7[0][j][jm-1]-lf*wrk5->work7[1][j][jm-1];
       wrk1->gb[j][jm-1] = hh1*wrk4->work5[0][j][jm-1]+hh3*wrk4->work5[1][j][jm-1]+hinv*
                       frcng->tauz[j][jm-1]+lf*hh1*wrk5->work7[0][j][jm-1]+
                       lf*hh3*wrk5->work7[1][j][jm-1];
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
       wrk1->ga[i][iindex] = wrk4->work5[0][i][iindex]-wrk4->work5[1][i][iindex]+eig2*
                          wrk6->work6[i][iindex]+h1inv*frcng->tauz[i][iindex]+
                          lf*wrk5->work7[0][i][iindex]-lf*wrk5->work7[1][i][iindex];
       wrk1->gb[i][iindex] = hh1*wrk4->work5[0][i][iindex]+hh3*wrk4->work5[1][i][iindex]+hinv*
                          frcng->tauz[i][iindex]+lf*hh1*wrk5->work7[0][i][iindex]+
                          lf*hh3*wrk5->work7[1][i][iindex];
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

   istart = gp[procid].rel_start_y[numlev-1];
   iend = istart + gp[procid].rel_num_y[numlev-1] - 1;
   jstart = gp[procid].rel_start_x[numlev-1];
   jend = jstart + gp[procid].rel_num_x[numlev-1] - 1;
   ist = istart;
   ien = iend;
   jst = jstart;
   jen = jend;
   if (istart == 1) {
     istart = 0;
   }
   if (jstart == 1) {
     jstart = 0;
   }
   if (iend == im-2) {
     iend = im-1;
   }
   if (jend == jm-2) {
     jend = jm-1;
   }
   for(i=istart;i<=iend;i++) {
     for(j=jstart;j<=jend;j++) {
       multi->rhs_multi[numlev-1][i][j] = wrk1->ga[i][j] * ressqr;
     }
   }
   if (istart == 0) {
     for(j=jstart;j<=jend;j++) {
       multi->q_multi[numlev-1][0][j] = wrk1->ga[0][j];
     }
   }
   if (iend == im-1) {
     for(j=jstart;j<=jend;j++) {
       multi->q_multi[numlev-1][im-1][j] = wrk1->ga[im-1][j];
     }
   }
   if (jstart == 0) {
     for(i=istart;i<=iend;i++) {
       multi->q_multi[numlev-1][i][0] = wrk1->ga[i][0];
     }
   }
   if (jend == jm-1) {
     for(i=istart;i<=iend;i++) {
       multi->q_multi[numlev-1][i][jm-1] = wrk1->ga[i][jm-1];
     }
   }

   fac = 1.0 / (4.0 - ressqr*eig2);
   for(i=ist;i<=ien;i++) {
     for(j=jst;j<=jen;j++) {
       multi->q_multi[numlev-1][i][j] = guess->oldga[i][j];
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

   if (procid == MASTER) {
     global->psiai=0.0;
   }

/*  copy the solution for use as initial guess in next time-step  */

   for(i=istart;i<=iend;i++) {
     for(j=jstart;j<=jend;j++) {
       wrk1->ga[i][j] = multi->q_multi[numlev-1][i][j];
       guess->oldga[i][j] = multi->q_multi[numlev-1][i][j];
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
   if (procid == MASTER) {
     psiaipriv = psiaipriv + 0.25*(wrk1->ga[0][0]);
   }
   if (procid == xprocs - 1) {
     psiaipriv = psiaipriv + 0.25*(wrk1->ga[0][jm-1]);
   }
   if (procid == nprocs-xprocs) {
     psiaipriv=psiaipriv+0.25*(wrk1->ga[im-1][0]);
   }
   if (procid == nprocs-1) {
     psiaipriv=psiaipriv+0.25*(wrk1->ga[im-1][jm-1]);
   }
   if (firstrow == 1) {
     for(j=firstcol;j<=lastcol;j++) {
       psiaipriv = psiaipriv + 0.5*wrk1->ga[0][j];
     }
   }
   if ((firstrow+numrows) == im-1) {
     for(j=firstcol;j<=lastcol;j++) {
       psiaipriv = psiaipriv + 0.5*wrk1->ga[im-1][j];
     }
   }
   if (firstcol == 1) {
     for(j=firstrow;j<=lastrow;j++) {
       psiaipriv = psiaipriv + 0.5*wrk1->ga[j][0];
     }
   }
   if ((firstcol+numcols) == jm-1) {
     for(j=firstrow;j<=lastrow;j++) {
       psiaipriv = psiaipriv + 0.5*wrk1->ga[j][jm-1];
     }
   }
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
   for(i=firstrow;i<=lastrow;i++) {
       psiaipriv = psiaipriv + wrk1->ga[i][iindex];
     }
   }

/* after computing its private sum, every process adds that to the
   shared running sum psiai  */

   LOCK(locks->psibilock)
   global->psiai = global->psiai + psiaipriv;
   UNLOCK(locks->psibilock)
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_7,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
/*      *******************************************************

                e i g h t h   p h a s e

        *******************************************************

   augment ga(i,j) with [-psiai/psibi]*psib(i,j)

   %%%%%%%%%%%%%%% f4 should be private  */

   f4 = (-global->psiai)/(global->psibi);

   if (procid == MASTER) {
     wrk1->ga[0][0] = wrk1->ga[0][0]+f4*wrk1->psib[0][0];
   }
   if (procid == nprocs-xprocs) {
     wrk1->ga[im-1][0] = wrk1->ga[im-1][0]+f4*wrk1->psib[im-1][0];
   }
   if (procid == xprocs-1) {
     wrk1->ga[0][jm-1] = wrk1->ga[0][jm-1]+f4*wrk1->psib[0][jm-1];
   }
   if (procid == nprocs-1) {
     wrk1->ga[im-1][jm-1] = wrk1->ga[im-1][jm-1]+f4*wrk1->psib[im-1][jm-1];
   }
   if (firstrow == 1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk1->ga[0][j] = wrk1->ga[0][j]+f4*wrk1->psib[0][j];
     }
   }
   if ((firstrow+numrows) == im-1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk1->ga[im-1][j] = wrk1->ga[im-1][j]+f4*wrk1->psib[im-1][j];
     }
   }
   if (firstcol == 1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk1->ga[j][0] = wrk1->ga[j][0]+f4*wrk1->psib[j][0];
     }
   }
   if ((firstcol+numcols) == jm-1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk1->ga[j][jm-1] = wrk1->ga[j][jm-1]+f4*wrk1->psib[j][jm-1];
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
       wrk1->ga[i][iindex] = wrk1->ga[i][iindex]+f4*wrk1->psib[i][iindex];
     }
   }
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_8,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
   for(i=istart;i<=iend;i++) {
     for(j=jstart;j<=jend;j++) {
       multi->rhs_multi[numlev-1][i][j] = wrk1->gb[i][j] * ressqr;
     }
   }
   if (istart == 0) {
     for(j=jstart;j<=jend;j++) {
       multi->q_multi[numlev-1][0][j] = wrk1->gb[0][j];
     }
   }
   if (iend == im-1) {
     for(j=jstart;j<=jend;j++) {
       multi->q_multi[numlev-1][im-1][j] = wrk1->gb[im-1][j];
     }
   }
   if (jstart == 0) {
     for(i=istart;i<=iend;i++) {
       multi->q_multi[numlev-1][i][0] = wrk1->gb[i][0];
     }
   }
   if (jend == jm-1) {
     for(i=istart;i<=iend;i++) {
       multi->q_multi[numlev-1][i][jm-1] = wrk1->gb[i][jm-1];
     }
   }

   fac = 1.0 / (4.0 - ressqr*eig2);
   for(i=ist;i<=ien;i++) {
     for(j=jst;j<=jen;j++) {
       multi->q_multi[numlev-1][i][j] = guess->oldgb[i][j];
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
     for(j=jstart;j<=jend;j++) {
       wrk1->gb[i][j] = multi->q_multi[numlev-1][i][j];
       guess->oldgb[i][j] = multi->q_multi[numlev-1][i][j];
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

   if (procid == MASTER) {
     wrk3->work2[0][0] = wrk1->gb[0][0]-hh1*wrk1->ga[0][0];
     wrk2->work3[0][0] = wrk1->gb[0][0]+hh3*wrk1->ga[0][0];
   }
   if (procid == nprocs-xprocs) {
     wrk3->work2[im-1][0] = wrk1->gb[im-1][0]-hh1*wrk1->ga[im-1][0];
     wrk2->work3[im-1][0] = wrk1->gb[im-1][0]+hh3*wrk1->ga[im-1][0];
   }
   if (procid == xprocs-1) {
     wrk3->work2[0][jm-1] = wrk1->gb[0][jm-1]-hh1*wrk1->ga[0][jm-1];
     wrk2->work3[0][jm-1] = wrk1->gb[0][jm-1]+hh3*wrk1->ga[0][jm-1];
   }
   if (procid == nprocs-1) {
     wrk3->work2[im-1][jm-1] = wrk1->gb[im-1][jm-1]-hh1*wrk1->ga[im-1][jm-1];
     wrk2->work3[im-1][jm-1] = wrk1->gb[im-1][jm-1]+hh3*wrk1->ga[im-1][jm-1];
   }
   if (firstrow == 1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk2->work3[0][j] = wrk1->gb[0][j]+hh3*wrk1->ga[0][j];
       wrk3->work2[0][j] = wrk1->gb[0][j]-hh1*wrk1->ga[0][j];
     }
   }
   if ((firstrow+numrows) == im-1) {
     for(j=firstcol;j<=lastcol;j++) {
       wrk2->work3[im-1][j] = wrk1->gb[im-1][j]+hh3*wrk1->ga[im-1][j];
       wrk3->work2[im-1][j] = wrk1->gb[im-1][j]-hh1*wrk1->ga[im-1][j];
     }
   }
   if (firstcol == 1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk2->work3[j][0] = wrk1->gb[j][0]+hh3*wrk1->ga[j][0];
       wrk3->work2[j][0] = wrk1->gb[j][0]-hh1*wrk1->ga[j][0];
     }
   }
   if ((firstcol+numcols) == jm-1) {
     for(j=firstrow;j<=lastrow;j++) {
       wrk2->work3[j][jm-1] = wrk1->gb[j][jm-1]+hh3*wrk1->ga[j][jm-1];
       wrk3->work2[j][jm-1] = wrk1->gb[j][jm-1]-hh1*wrk1->ga[j][jm-1];
     }
   }

   for(i=firstrow;i<=lastrow;i++) {
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
       wrk2->work3[i][iindex] = wrk1->gb[i][iindex]+hh3*wrk1->ga[i][iindex];
       wrk3->work2[i][iindex] = wrk1->gb[i][iindex]-hh1*wrk1->ga[i][iindex];
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

   if (procid == MASTER) {
     fields->psi[0][0][0] = fields->psi[0][0][0] + timst*wrk2->work3[0][0];
   }
   if (procid == nprocs-xprocs) {
     fields->psi[0][im-1][0] = fields->psi[0][im-1][0] + timst*wrk2->work3[im-1][0];
   }
   if (procid == xprocs-1) {
     fields->psi[0][0][jm-1] = fields->psi[0][0][jm-1] + timst*wrk2->work3[0][jm-1];
   }
   if (procid == nprocs-1) {
     fields->psi[0][im-1][jm-1] = fields->psi[0][im-1][jm-1] + timst*wrk2->work3[im-1][jm-1];
   }
   if (firstrow == 1) {
     for(j=firstcol;j<=lastcol;j++) {
       fields->psi[0][0][j] = fields->psi[0][0][j] + timst*wrk2->work3[0][j];
     }
   }
   if ((firstrow+numrows) == im-1) {
     for(j=firstcol;j<=lastcol;j++) {
       fields->psi[0][im-1][j] = fields->psi[0][im-1][j] + timst*wrk2->work3[im-1][j];
     }
   }
   if (firstcol == 1) {
     for(j=firstrow;j<=lastrow;j++) {
       fields->psi[0][j][0] = fields->psi[0][j][0] + timst*wrk2->work3[j][0];
     }
   }
   if ((firstcol+numcols) == jm-1) {
     for(j=firstrow;j<=lastrow;j++) {
       fields->psi[0][j][jm-1] = fields->psi[0][j][jm-1] + timst*wrk2->work3[j][jm-1];
     }
   }
   for(i=firstrow;i<=lastrow;i++) {
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
         fields->psi[0][i][iindex] = fields->psi[0][i][iindex] + timst*wrk2->work3[i][iindex];
     }
   }

   if (procid == MASTER) {
     fields->psi[1][0][0] = fields->psi[1][0][0] + timst*wrk3->work2[0][0];
   }
   if (procid == nprocs-xprocs) {
     fields->psi[1][im-1][0] = fields->psi[1][im-1][0] + timst*wrk3->work2[im-1][0];
   }
   if (procid == xprocs-1) {
     fields->psi[1][0][jm-1] = fields->psi[1][0][jm-1] + timst*wrk3->work2[0][jm-1];
   }
   if (procid == nprocs-1) {
     fields->psi[1][im-1][jm-1] = fields->psi[1][im-1][jm-1] + timst*wrk3->work2[im-1][jm-1];
   }
   if (firstrow == 1) {
     for(j=firstcol;j<=lastcol;j++) {
       fields->psi[1][0][j] = fields->psi[1][0][j] + timst*wrk3->work2[0][j];
     }
   }
   if ((firstrow+numrows) == im-1) {
     for(j=firstcol;j<=lastcol;j++) {
       fields->psi[1][im-1][j] = fields->psi[1][im-1][j] + timst*wrk3->work2[im-1][j];
     }
   }
   if (firstcol == 1) {
     for(j=firstrow;j<=lastrow;j++) {
       fields->psi[1][j][0] = fields->psi[1][j][0] + timst*wrk3->work2[j][0];
     }
   }
   if ((firstcol+numcols) == jm-1) {
     for(j=firstrow;j<=lastrow;j++) {
       fields->psi[1][j][jm-1] = fields->psi[1][j][jm-1] + timst*wrk3->work2[j][jm-1];
     }
   }

   for(i=firstrow;i<=lastrow;i++) {
     for(iindex=firstcol;iindex<=lastcol;iindex++) {
         fields->psi[1][i][iindex] = fields->psi[1][i][iindex] + timst*wrk3->work2[i][iindex];
     }
   }
#if defined(MULTIPLE_BARRIERS)
   BARRIER(bars->sl_phase_10,nprocs)
#else
   BARRIER(bars->barrier,nprocs)
#endif
}
