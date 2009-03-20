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

/* Does the arakawa jacobian calculation (of the x and y matrices,
   putting the results in the z matrix) for a subblock. */

EXTERN_ENV

#include <stdio.h>
#include <math.h>
#include <time.h>
#include "decs.h"

void jacobcalc2(double ****x, double ****y, double ****z, long psiindex, long pid, long firstrow, long lastrow, long firstcol, long lastcol)
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
   long jj;
   double **t2a;
   double **t2b;
   double **t2c;
   double *t1a;
   double *t1b;
   double *t1c;
   double *t1d;
   double *t1e;
   double *t1f;
   double *t1g;

   t2a = z[pid][psiindex];
   if ((gp[pid].neighbors[UP] == -1) && (gp[pid].neighbors[LEFT] == -1)) {
     t2a[0][0]=0.0;
   }
   if ((gp[pid].neighbors[DOWN] == -1) && (gp[pid].neighbors[LEFT] == -1)) {
     t2a[im-1][0]=0.0;
   }
   if ((gp[pid].neighbors[UP] == -1) && (gp[pid].neighbors[RIGHT] == -1)) {
     t2a[0][jm-1]=0.0;
   }
   if ((gp[pid].neighbors[DOWN] == -1) && (gp[pid].neighbors[RIGHT] == -1)) {
     t2a[im-1][jm-1]=0.0;
   }

   t2a = x[pid][psiindex];
   jj = gp[pid].neighbors[UPLEFT];
   if (jj != -1) {
     t2a[0][0]=x[jj][psiindex][im-2][jm-2];
   }
   jj = gp[pid].neighbors[UPRIGHT];
   if (jj != -1) {
     t2a[0][jm-1]=x[jj][psiindex][im-2][1];
   }
   jj = gp[pid].neighbors[DOWNLEFT];
   if (jj != -1) {
     t2a[im-1][0]=x[jj][psiindex][1][jm-2];
   }
   jj = gp[pid].neighbors[DOWNRIGHT];
   if (jj != -1) {
     t2a[im-1][jm-1]=x[jj][psiindex][1][1];
   }

   t2a = y[pid][psiindex];
   jj = gp[pid].neighbors[UPLEFT];
   if (jj != -1) {
     t2a[0][0]=y[jj][psiindex][im-2][jm-2];
   }
   jj = gp[pid].neighbors[UPRIGHT];
   if (jj != -1) {
     t2a[0][jm-1]=y[jj][psiindex][im-2][1];
   }
   jj = gp[pid].neighbors[DOWNLEFT];
   if (jj != -1) {
     t2a[im-1][0]=y[jj][psiindex][1][jm-2];
   }
   jj = gp[pid].neighbors[DOWNRIGHT];
   if (jj != -1) {
     t2a[im-1][jm-1]=y[jj][psiindex][1][1];
   }

   t2a = x[pid][psiindex];
   if (gp[pid].neighbors[UP] == -1) {
     jj = gp[pid].neighbors[LEFT];
     if (jj != -1) {
       t2a[0][0] = x[jj][psiindex][0][jm-2];
     } else {
       jj = gp[pid].neighbors[DOWN];
       if (jj != -1) {
         t2a[im-1][0] = x[jj][psiindex][1][0];
       }
     }
     jj = gp[pid].neighbors[RIGHT];
     if (jj != -1) {
       t2a[0][jm-1] = x[jj][psiindex][0][1];
     } else {
       jj = gp[pid].neighbors[DOWN];
       if (jj != -1) {
         t2a[im-1][jm-1] = x[jj][psiindex][1][jm-1];
       }
     }
   } else if (gp[pid].neighbors[DOWN] == -1) {
     jj = gp[pid].neighbors[LEFT];
     if (jj != -1) {
       t2a[im-1][0] = x[jj][psiindex][im-1][jm-2];
     } else {
       jj = gp[pid].neighbors[UP];
       if (jj != -1) {
         t2a[0][0] = x[jj][psiindex][im-2][0];
       }
     }
     jj = gp[pid].neighbors[RIGHT];
     if (jj != -1) {
       t2a[im-1][jm-1] = x[jj][psiindex][im-1][1];
     } else {
       jj = gp[pid].neighbors[UP];
       if (jj != -1) {
         t2a[0][jm-1] = x[jj][psiindex][im-2][jm-1];
       }
     }
   } else if (gp[pid].neighbors[LEFT] == -1) {
     jj = gp[pid].neighbors[UP];
     if (jj != -1) {
       t2a[0][0] = x[jj][psiindex][im-2][0];
     }
     jj = gp[pid].neighbors[DOWN];
     if (jj != -1) {
       t2a[im-1][0] = x[jj][psiindex][1][0];
     }
   } else if (gp[pid].neighbors[RIGHT] == -1) {
     jj = gp[pid].neighbors[UP];
     if (jj != -1) {
       t2a[0][jm-1] = x[jj][psiindex][im-2][jm-1];
     }
     jj = gp[pid].neighbors[DOWN];
     if (jj != -1) {
       t2a[im-1][jm-1] = x[jj][psiindex][1][jm-1];
     }
   }

   t2a = y[pid][psiindex];
   if (gp[pid].neighbors[UP] == -1) {
     jj = gp[pid].neighbors[LEFT];
     if (jj != -1) {
       t2a[0][0] = y[jj][psiindex][0][jm-2];
     } else {
       jj = gp[pid].neighbors[DOWN];
       if (jj != -1) {
         t2a[im-1][0] = y[jj][psiindex][1][0];
       }
     }
     jj = gp[pid].neighbors[RIGHT];
     if (jj != -1) {
       t2a[0][jm-1] = y[jj][psiindex][0][1];
     } else {
       jj = gp[pid].neighbors[DOWN];
       if (jj != -1) {
         t2a[im-1][jm-1] = y[jj][psiindex][1][jm-1];
       }
     }
   } else if (gp[pid].neighbors[DOWN] == -1) {
     jj = gp[pid].neighbors[LEFT];
     if (jj != -1) {
       t2a[im-1][0] = y[jj][psiindex][im-1][jm-2];
     } else {
       jj = gp[pid].neighbors[UP];
       if (jj != -1) {
         t2a[0][0] = y[jj][psiindex][im-2][0];
       }
     }
     jj = gp[pid].neighbors[RIGHT];
     if (jj != -1) {
       t2a[im-1][jm-1] = y[jj][psiindex][im-1][1];
     } else {
       jj = gp[pid].neighbors[UP];
       if (jj != -1) {
         t2a[0][jm-1] = y[jj][psiindex][im-2][jm-1];
       }
     }
   } else if (gp[pid].neighbors[LEFT] == -1) {
     jj = gp[pid].neighbors[UP];
     if (jj != -1) {
       t2a[0][0] = y[jj][psiindex][im-2][0];
     }
     jj = gp[pid].neighbors[DOWN];
     if (jj != -1) {
       t2a[im-1][0] = y[jj][psiindex][1][0];
     }
   } else if (gp[pid].neighbors[RIGHT] == -1) {
     jj = gp[pid].neighbors[UP];
     if (jj != -1) {
       t2a[0][jm-1] = y[jj][psiindex][im-2][jm-1];
     }
     jj = gp[pid].neighbors[DOWN];
     if (jj != -1) {
       t2a[im-1][jm-1] = y[jj][psiindex][1][jm-1];
     }
   }

   t2a = y[pid][psiindex];
   j = gp[pid].neighbors[UP];
   if (j != -1) {
     t1a = (double *) t2a[0];
     t1b = (double *) y[j][psiindex][im-2];
     for (i=1;i<=lastcol;i++) {
       t1a[i] = t1b[i];
     }
   }
   j = gp[pid].neighbors[DOWN];
   if (j != -1) {
     t1a = (double *) t2a[im-1];
     t1b = (double *) y[j][psiindex][1];
     for (i=1;i<=lastcol;i++) {
       t1a[i] = t1b[i];
     }
   }
   j = gp[pid].neighbors[LEFT];
   if (j != -1) {
     t2b = y[j][psiindex];
     for (i=1;i<=lastrow;i++) {
       t2a[i][0] = t2b[i][jm-2];
     }
   }
   j = gp[pid].neighbors[RIGHT];
   if (j != -1) {
     t2b = y[j][psiindex];
     for (i=1;i<=lastrow;i++) {
       t2a[i][jm-1] = t2b[i][1];
     }
   }

   t2a = x[pid][psiindex];
   j = gp[pid].neighbors[UP];
   if (j != -1) {
     t1a = (double *) t2a[0];
     t1b = (double *) x[j][psiindex][im-2];
     for (i=1;i<=lastcol;i++) {
       t1a[i] = t1b[i];
     }
   }
   j = gp[pid].neighbors[DOWN];
   if (j != -1) {
     t1a = (double *) t2a[im-1];
     t1b = (double *) x[j][psiindex][1];
     for (i=1;i<=lastcol;i++) {
       t1a[i] = t1b[i];
     }
   }
   j = gp[pid].neighbors[LEFT];
   if (j != -1) {
     t2b = x[j][psiindex];
     for (i=1;i<=lastrow;i++) {
       t2a[i][0] = t2b[i][jm-2];
     }
   }
   j = gp[pid].neighbors[RIGHT];
   if (j != -1) {
     t2b = x[j][psiindex];
     for (i=1;i<=lastrow;i++) {
       t2a[i][jm-1] = t2b[i][1];
     }
   }

   t2a = x[pid][psiindex];
   t2b = y[pid][psiindex];
   t2c = z[pid][psiindex];
   for (i=firstrow;i<=lastrow;i++) {
     ip1 = i+1;
     im1 = i-1;
     t1a = (double *) t2a[i];
     t1b = (double *) t2b[i];
     t1c = (double *) t2c[i];
     t1d = (double *) t2b[ip1];
     t1e = (double *) t2b[im1];
     t1f = (double *) t2a[ip1];
     t1g = (double *) t2a[im1];
     for (iindex=firstcol;iindex<=lastcol;iindex++) {
       indexp1 = iindex+1;
       indexm1 = iindex-1;
       f1 = (t1b[indexm1]+t1d[indexm1]-
             t1b[indexp1]-t1d[indexp1])*
            (t1f[iindex]-t1a[iindex]);
       f2 = (t1e[indexm1]+t1b[indexm1]-
             t1e[indexp1]-t1b[indexp1])*
            (t1a[iindex]-t1g[iindex]);
       f3 = (t1d[iindex]+t1d[indexp1]-
             t1e[iindex]-t1e[indexp1])*
            (t1a[indexp1]-t1a[iindex]);
       f4 = (t1d[indexm1]+t1d[iindex]-
             t1e[indexm1]-t1e[iindex])*
            (t1a[iindex]-t1a[indexm1]);
       f5 = (t1d[iindex]-t1b[indexp1])*
            (t1f[indexp1]-t1a[iindex]);
       f6 = (t1b[indexm1]-t1e[iindex])*
            (t1a[iindex]-t1g[indexm1]);
       f7 = (t1b[indexp1]-t1e[iindex])*
            (t1g[indexp1]-t1a[iindex]);
       f8 = (t1d[iindex]-t1b[indexm1])*
            (t1a[iindex]-t1f[indexm1]);

       t1c[iindex] = factjacob*(f1+f2+f3+f4+f5+f6+f7+f8);

     }
   }

   if (gp[pid].neighbors[UP] == -1) {
     t1c = (double *) t2c[0];
     for (j=firstcol;j<=lastcol;j++) {
       t1c[j] = 0.0;
     }
   }
   if (gp[pid].neighbors[DOWN] == -1) {
     t1c = (double *) t2c[im-1];
     for (j=firstcol;j<=lastcol;j++) {
       t1c[j] = 0.0;
     }
   }
   if (gp[pid].neighbors[LEFT] == -1) {
     for (j=firstrow;j<=lastrow;j++) {
       t2c[j][0] = 0.0;
     }
   }
   if (gp[pid].neighbors[RIGHT] == -1) {
     for (j=firstrow;j<=lastrow;j++) {
       t2c[j][jm-1] = 0.0;
     }
   }

}

