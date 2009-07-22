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

/* does the arakawa jacobian calculation (of the x and y matrices,
   putting the results in the z matrix) for a subblock. */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include "decs.h"

void jacobcalc(double x[IMAX][JMAX], double y[IMAX][JMAX], double z[IMAX][JMAX], long pid, long firstrow, long lastrow, long firstcol, long lastcol, long numrows, long numcols)
{
   double f1;
   double f2;
   double f3;
   double f4;
   double f5;
   double f6;
   double f7;
   double f8;
   long iindex;
   long indexp1;
   long indexm1;
   long im1;
   long ip1;
   long i;
   long j;

   if (pid == MASTER) {
     z[0][0]=0.0;
   }
   if (pid == nprocs-xprocs) {
     z[im-1][0]=0.0;
   }
   if (pid == xprocs-1) {
     z[0][jm-1]=0.0;
   }
   if (pid == nprocs-1) {
     z[im-1][jm-1]=0.0;
   }
   for (iindex=firstcol;iindex<=lastcol;iindex++) {
     indexp1 = iindex+1;
     indexm1 = iindex-1;
     for (i=firstrow;i<=lastrow;i++) {
       ip1 = i+1;
       im1 = i-1;
       f1 = (y[i][indexm1]+y[ip1][indexm1]-y[i][indexp1]-y[ip1][indexp1])*
            (x[ip1][iindex]-x[i][iindex]);
       f2 = (y[im1][indexm1]+y[i][indexm1]-y[im1][indexp1]-y[i][indexp1])*
            (x[i][iindex]-x[im1][iindex]);
       f3 = (y[ip1][iindex]+y[ip1][indexp1]-y[im1][iindex]-y[im1][indexp1])*
            (x[i][indexp1]-x[i][iindex]);
       f4 = (y[ip1][indexm1]+y[ip1][iindex]-y[im1][indexm1]-y[im1][iindex])*
            (x[i][iindex]-x[i][indexm1]);
       f5 = (y[ip1][iindex]-y[i][indexp1])*(x[ip1][indexp1]-x[i][iindex]);
       f6 = (y[i][indexm1]-y[im1][iindex])*(x[i][iindex]-x[im1][indexm1]);
       f7 = (y[i][indexp1]-y[im1][iindex])*(x[im1][indexp1]-x[i][iindex]);
       f8 = (y[ip1][iindex]-y[i][indexm1])*(x[i][iindex]-x[ip1][indexm1]);

       z[i][iindex] = factjacob*(f1+f2+f3+f4+f5+f6+f7+f8);
     }
   }
   if (firstrow == 1) {
     for (j=firstcol;j<=lastcol;j++) {
       z[0][j] = 0.0;
     }
   }
   if ((firstrow+numrows) == im-1) {
     for (j=firstcol;j<=lastcol;j++) {
       z[im-1][j] = 0.0;
     }
   }
   if (firstcol == 1) {
     for (j=firstrow;j<=lastrow;j++) {
       z[j][0] = 0.0;
     }
   }
   if ((firstcol+numcols) == jm-1) {
     for (j=firstrow;j<=lastrow;j++) {
       z[j][jm-1] = 0.0;
     }
   }
}

