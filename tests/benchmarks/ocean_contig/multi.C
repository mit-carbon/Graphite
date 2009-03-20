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

/* Shared memory implementation of the multigrid method
   Implementation uses red-black gauss-seidel relaxation
   iterations, w cycles, and the method of half-injection for
   residual computation. */

EXTERN_ENV

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include "decs.h"

/* perform multigrid (w cycles)                                     */
void multig(long my_id)
{
   long iter;
   double wu;
   double errp;
   long m;
   long flag1;
   long flag2;
   long k;
   long my_num;
   double wmax;
   double local_err;
   double red_local_err;
   double black_local_err;
   double g_error;

   flag1 = 0;
   flag2 = 0;
   iter = 0;
   m = numlev-1;
   wmax = maxwork;
   my_num = my_id;
   wu = 0.0;

   k = m;
   g_error = 1.0e30;
   while ((!flag1) && (!flag2)) {
     errp = g_error;
     iter++;
     if (my_num == MASTER) {
       multi->err_multi = 0.0;
     }

/* barrier to make sure all procs have finished intadd or rescal   */
/* before proceeding with relaxation                               */
#if defined(MULTIPLE_BARRIERS)
     BARRIER(bars->error_barrier,nprocs)
#else
     BARRIER(bars->barrier,nprocs)
#endif
     copy_black(k,my_num);

     relax(k,&red_local_err,RED_ITER,my_num);

/* barrier to make sure all red computations have been performed   */
#if defined(MULTIPLE_BARRIERS)
     BARRIER(bars->error_barrier,nprocs)
#else
     BARRIER(bars->barrier,nprocs)
#endif
     copy_red(k,my_num);

     relax(k,&black_local_err,BLACK_ITER,my_num);

/* compute max local error from red_local_err and black_local_err  */

     if (red_local_err > black_local_err) {
       local_err = red_local_err;
     } else {
       local_err = black_local_err;
     }

/* update the global error if necessary                         */

     LOCK(locks->error_lock)
     if (local_err > multi->err_multi) {
       multi->err_multi = local_err;
     }
     UNLOCK(locks->error_lock)

/* a single relaxation sweep at the finest level is one unit of    */
/* work                                                            */

     wu+=pow((double)4.0,(double)k-m);

/* barrier to make sure all processors have checked local error    */
#if defined(MULTIPLE_BARRIERS)
     BARRIER(bars->error_barrier,nprocs)
#else
     BARRIER(bars->barrier,nprocs)
#endif
     g_error = multi->err_multi;

/* barrier to make sure master does not cycle back to top of loop  */
/* and reset global->err before we read it and decide what to do   */
#if defined(MULTIPLE_BARRIERS)
     BARRIER(bars->error_barrier,nprocs)
#else
     BARRIER(bars->barrier,nprocs)
#endif

     if (g_error >= lev_tol[k]) {
       if (wu > wmax) {
/* max work exceeded                                               */
         flag1 = 1;
	 fprintf(stderr,"ERROR: Maximum work limit %0.5f exceeded\n",wmax);
	 exit(-1);
       } else {
/* if we have not converged                                        */
         if ((k != 0) && (g_error/errp >= 0.6) &&
	     (k > minlevel)) {
/* if need to go to coarser grid                                   */

           copy_borders(k,my_num);
           copy_rhs_borders(k,my_num);

/* This bar is needed because the routine rescal uses the neighbor's
   border points to compute s4.  We must ensure that the neighbor's
   border points have been written before we try computing the new
   rescal values                                                   */

#if defined(MULTIPLE_BARRIERS)
	   BARRIER(bars->error_barrier,nprocs)
#else
	   BARRIER(bars->barrier,nprocs)
#endif

           rescal(k,my_num);

/* transfer residual to rhs of coarser grid                        */
           lev_tol[k-1] = 0.3 * g_error;
           k = k-1;
           putz(k,my_num);
/* make initial guess on coarser grid zero                         */
           g_error = 1.0e30;
         }
       }
     } else {
/* if we have converged at this level                              */
       if (k == m) {
/* if finest grid, we are done                                     */
         flag2 = 1;
       } else {
/* else go to next finest grid                                     */

         copy_borders(k,my_num);

         intadd(k,my_num);
/* changes the grid values at the finer level.  rhs at finer level */
/* remains what it already is                                      */
         k++;
         g_error = 1.0e30;
       }
     }
   }
   if (do_output) {
     if (my_num == MASTER) {
       printf("iter %ld, level %ld, residual norm %12.8e, work = %7.3f\n", iter,k,multi->err_multi,wu);
     }
   }
}

/* perform red or black iteration (not both)                    */
void relax(long k, double *err, long color, long my_num)
{
   long i;
   long j;
   long iend;
   long jend;
   long oddistart;
   long oddjstart;
   long evenistart;
   long evenjstart;
   double a;
   double h;
   double factor;
   double maxerr;
   double newerr;
   double oldval;
   double newval;
   double **t2a;
   double **t2b;
   double *t1a;
   double *t1b;
   double *t1c;
   double *t1d;

   i = 0;
   j = 0;

   *err = 0.0;
   h = lev_res[k];

/* points whose sum of row and col index is even do a red iteration, */
/* others do a black				                     */

   evenistart = gp[my_num].eist[k];
   evenjstart = gp[my_num].ejst[k];
   oddistart = gp[my_num].oist[k];
   oddjstart = gp[my_num].ojst[k];

   iend = gp[my_num].rlien[k];
   jend = gp[my_num].rljen[k];

   factor = 4.0 - eig2 * h * h ;
   maxerr = 0.0;
   t2a = (double **) q_multi[my_num][k];
   t2b = (double **) rhs_multi[my_num][k];
   if (color == RED_ITER) {
     for (i=evenistart;i<iend;i+=2) {
       t1a = (double *) t2a[i];
       t1b = (double *) t2b[i];
       t1c = (double *) t2a[i-1];
       t1d = (double *) t2a[i+1];
       for (j=evenjstart;j<jend;j+=2) {
         a = t1a[j+1] + t1a[j-1] +
	     t1c[j] + t1d[j] -
	     t1b[j] ;
         oldval = t1a[j];
         newval = a / factor;
         newerr = oldval - newval;
         t1a[j] = newval;
         if (fabs(newerr) > maxerr) {
           maxerr = fabs(newerr);
         }
       }
     }
     for (i=oddistart;i<iend;i+=2) {
       t1a = (double *) t2a[i];
       t1b = (double *) t2b[i];
       t1c = (double *) t2a[i-1];
       t1d = (double *) t2a[i+1];
       for (j=oddjstart;j<jend;j+=2) {
         a = t1a[j+1] + t1a[j-1] +
	     t1c[j] + t1d[j] -
	     t1b[j] ;
         oldval = t1a[j];
         newval = a / factor;
         newerr = oldval - newval;
         t1a[j] = newval;
         if (fabs(newerr) > maxerr) {
           maxerr = fabs(newerr);
         }
       }
     }
   } else if (color == BLACK_ITER) {
     for (i=evenistart;i<iend;i+=2) {
       t1a = (double *) t2a[i];
       t1b = (double *) t2b[i];
       t1c = (double *) t2a[i-1];
       t1d = (double *) t2a[i+1];
       for (j=oddjstart;j<jend;j+=2) {
         a = t1a[j+1] + t1a[j-1] +
	     t1c[j] + t1d[j] -
	     t1b[j] ;
         oldval = t1a[j];
         newval = a / factor;
         newerr = oldval - newval;
         t1a[j] = newval;
         if (fabs(newerr) > maxerr) {
           maxerr = fabs(newerr);
         }
       }
     }
     for (i=oddistart;i<iend;i+=2) {
       t1a = (double *) t2a[i];
       t1b = (double *) t2b[i];
       t1c = (double *) t2a[i-1];
       t1d = (double *) t2a[i+1];
       for (j=evenjstart;j<jend;j+=2) {
         a = t1a[j+1] + t1a[j-1] +
	     t1c[j] + t1d[j] -
	     t1b[j] ;
         oldval = t1a[j];
         newval = a / factor;
         newerr = oldval - newval;
         t1a[j] = newval;
         if (fabs(newerr) > maxerr) {
           maxerr = fabs(newerr);
         }
       }
     }
   }
   *err = maxerr;
}

/* perform half-injection to next coarsest level                */
void rescal(long kf, long my_num)
{
   long ic;
   long if17;
   long jf;
   long jc;
   long krc;
   long istart;
   long iend;
   long jstart;
   long jend;
   double hf;
   double hc;
   double s;
   double s1;
   double s2;
   double s3;
   double s4;
   double factor;
   double int1;
   double int2;
   double i_int_factor;
   double j_int_factor;
   double int_val;
   long i_off;
   long j_off;
   long up_proc;
   long left_proc;
   long im;
   long jm;
   double temp;
   double temp2;
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
   double *t1h;

   krc = kf - 1;
   hc = lev_res[krc];
   hf = lev_res[kf];
   i_off = gp[my_num].rownum*ypts_per_proc[krc];
   j_off = gp[my_num].colnum*xpts_per_proc[krc];
   up_proc = gp[my_num].neighbors[UP];
   left_proc = gp[my_num].neighbors[LEFT];
   im = (imx[kf]-2)/yprocs;
   jm = (jmx[kf]-2)/xprocs;

   istart = gp[my_num].rlist[krc];
   jstart = gp[my_num].rljst[krc];
   iend = gp[my_num].rlien[krc] - 1;
   jend = gp[my_num].rljen[krc] - 1;

   factor = 4.0 - eig2 * hf * hf;

   t2a = (double **) q_multi[my_num][kf];
   t2b = (double **) rhs_multi[my_num][kf];
   t2c = (double **) rhs_multi[my_num][krc];
   if17=2*(istart-1);
   for(ic=istart;ic<=iend;ic++) {
     if17+=2;
     i_int_factor = (ic+i_off) * i_int_coeff[krc] * 0.5;
     jf = 2 * (jstart - 1);
     t1a = (double *) t2a[if17];
     t1b = (double *) t2b[if17];
     t1c = (double *) t2c[ic];
     t1d = (double *) t2a[if17-1];
     t1e = (double *) t2a[if17+1];
     t1f = (double *) t2a[if17-2];
     t1g = (double *) t2a[if17-3];
     t1h = (double *) t2b[if17-2];
     for(jc=jstart;jc<=jend;jc++) {
       jf+=2;
       j_int_factor = (jc+j_off)*j_int_coeff[krc] * 0.5;

/*             method of half-injection uses 2.0 instead of 4.0 */

/* do bilinear interpolation */
       s = t1a[jf+1] + t1a[jf-1] + t1d[jf] + t1e[jf];
       s1 = 2.0 * (t1b[jf] - s + factor * t1a[jf]);
       if (((if17 == 2) && (gp[my_num].neighbors[UP] == -1)) ||
	   ((jf == 2) && (gp[my_num].neighbors[LEFT] == -1))) {
          s2 = 0;
          s3 = 0;
          s4 = 0;
       } else if ((if17 == 2) || (jf == 2)) {
	  if (jf == 2) {
	    temp = q_multi[left_proc][kf][if17][jm-1];
          } else {
            temp = t1a[jf-3];
          }
          s = t1a[jf-1] + temp + t1d[jf-2] + t1e[jf-2];
          s2 = 2.0 * (t1b[jf-2] - s + factor * t1a[jf-2]);
	  if (if17 == 2) {
	    temp = q_multi[up_proc][kf][im-1][jf];
          } else {
            temp = t1g[jf];
          }
          s = t1f[jf+1]+ t1f[jf-1]+ temp + t1d[jf];
          s3 = 2.0 * (t1h[jf] - s + factor * t1f[jf]);
	  if (jf == 2) {
	    temp = q_multi[left_proc][kf][if17-2][jm-1];
          } else {
            temp = t1f[jf-3];
          }
	  if (if17 == 2) {
	    temp2 = q_multi[up_proc][kf][im-1][jf-2];
          } else {
            temp2 = t1g[jf-2];
          }
          s = t1f[jf-1]+ temp + temp2 + t1d[jf-2];
          s4 = 2.0 * (t1h[jf-2] - s + factor * t1f[jf-2]);
       } else {
          s = t1a[jf-1] + t1a[jf-3] + t1d[jf-2] + t1e[jf-2];
          s2 = 2.0 * (t1b[jf-2] - s + factor * t1a[jf-2]);
          s = t1f[jf+1]+ t1f[jf-1]+ t1g[jf] +   t1d[jf];
          s3 = 2.0 * (t1h[jf] - s + factor * t1f[jf]);
          s = t1f[jf-1]+ t1f[jf-3]+ t1g[jf-2]+ t1d[jf-2];
          s4 = 2.0 * (t1h[jf-2] - s + factor * t1f[jf-2]);
       }
       int1 = j_int_factor*s4 + (1.0-j_int_factor)*s3;
       int2 = j_int_factor*s2 + (1.0-j_int_factor)*s1;
       int_val = i_int_factor*int1+(1.0-i_int_factor)*int2;
       t1c[jc] = i_int_factor*int1+(1.0-i_int_factor)*int2;
     }
   }
}

/* perform interpolation and addition to next finest grid       */
void intadd(long kc, long my_num)
{
   long ic;
   long if17;
   long jf;
   long jc;
   long kf;
   long istart;
   long jstart;
   long iend;
   long jend;
   double hc;
   double hf;
   double int1;
   double int2;
   double i_int_factor1;
   double j_int_factor1;
   double i_int_factor2;
   double j_int_factor2;
   long i_off;
   long j_off;
   double **t2a;
   double **t2b;
   double *t1a;
   double *t1b;
   double *t1c;
   double *t1d;
   double *t1e;

   kf = kc + 1;
   hc = lev_res[kc];
   hf = lev_res[kf];

   istart = gp[my_num].rlist[kc];
   jstart = gp[my_num].rljst[kc];
   iend = gp[my_num].rlien[kc] - 1;
   jend = gp[my_num].rljen[kc] - 1;
   i_off = gp[my_num].rownum*ypts_per_proc[kc];
   j_off = gp[my_num].colnum*xpts_per_proc[kc];

   t2a = (double **) q_multi[my_num][kc];
   t2b = (double **) q_multi[my_num][kf];
   if17 = 2*(istart-1);
   for(ic=istart;ic<=iend;ic++) {
     if17+=2;
     i_int_factor1= ((imx[kc]-2)-(ic+i_off-1)) * (i_int_coeff[kf]);
     i_int_factor2= (ic+i_off) * i_int_coeff[kf];
     jf = 2*(jstart-1);

     t1a = (double *) t2a[ic];
     t1b = (double *) t2a[ic-1];
     t1c = (double *) t2a[ic+1];
     t1d = (double *) t2b[if17];
     t1e = (double *) t2b[if17-1];
     for(jc=jstart;jc<=jend;jc++) {
       jf+=2;
       j_int_factor1= ((jmx[kc]-2)-(jc+j_off-1)) * (j_int_coeff[kf]);
       j_int_factor2= (jc+j_off) * j_int_coeff[kf];

       int1 = j_int_factor1*t1a[jc-1] + (1.0-j_int_factor1)*t1a[jc];
       int2 = j_int_factor1*t1b[jc-1] + (1.0-j_int_factor1)*t1b[jc];
       t1e[jf-1] += i_int_factor1*int2 + (1.0-i_int_factor1)*int1;
       int2 = j_int_factor1*t1c[jc-1] + (1.0-j_int_factor1)*t1c[jc];
       t1d[jf-1] += i_int_factor2*int2 + (1.0-i_int_factor2)*int1;
       int1 = j_int_factor2*t1a[jc+1] + (1.0-j_int_factor2)*t1a[jc];
       int2 = j_int_factor2*t1b[jc+1] + (1.0-j_int_factor2)*t1b[jc];
       t1e[jf] += i_int_factor1*int2 + (1.0-i_int_factor1)*int1;
       int2 = j_int_factor2*t1c[jc+1] + (1.0-j_int_factor2)*t1c[jc];
       t1d[jf] += i_int_factor2*int2 + (1.0-i_int_factor2)*int1;
     }
   }
}

/* initialize a grid to zero in parallel                        */
void putz(long k, long my_num)
{
   long i;
   long j;
   long istart;
   long jstart;
   long iend;
   long jend;
   double **t2a;
   double *t1a;

   istart = gp[my_num].rlist[k];
   jstart = gp[my_num].rljst[k];
   iend = gp[my_num].rlien[k];
   jend = gp[my_num].rljen[k];

   t2a = (double **) q_multi[my_num][k];
   for (i=istart;i<=iend;i++) {
     t1a = (double *) t2a[i];
     for (j=jstart;j<=jend;j++) {
       t1a[j] = 0.0;
     }
   }
}

void copy_borders(long k, long pid)
{
  long i;
  long j;
  long jj;
  long im;
  long jm;
  long lastrow;
  long lastcol;
  double **t2a;
  double **t2b;
  double *t1a;
  double *t1b;

   im = (imx[k]-2)/yprocs + 2;
   jm = (jmx[k]-2)/xprocs + 2;
   lastrow = (imx[k]-2)/yprocs;
   lastcol = (jmx[k]-2)/xprocs;

   t2a = (double **) q_multi[pid][k];
   jj = gp[pid].neighbors[UPLEFT];
   if (jj != -1) {
     t2a[0][0]=q_multi[jj][k][im-2][jm-2];
   }
   jj = gp[pid].neighbors[UPRIGHT];
   if (jj != -1) {
     t2a[0][jm-1]=q_multi[jj][k][im-2][1];
   }
   jj = gp[pid].neighbors[DOWNLEFT];
   if (jj != -1) {
     t2a[im-1][0]=q_multi[jj][k][1][jm-2];
   }
   jj = gp[pid].neighbors[DOWNRIGHT];
   if (jj != -1) {
     t2a[im-1][jm-1]=q_multi[jj][k][1][1];
   }

   if (gp[pid].neighbors[UP] == -1) {
     jj = gp[pid].neighbors[LEFT];
     if (jj != -1) {
       t2a[0][0] = q_multi[jj][k][0][jm-2];
     } else {
       jj = gp[pid].neighbors[DOWN];
       if (jj != -1) {
         t2a[im-1][0] = q_multi[jj][k][1][0];
       }
     }
     jj = gp[pid].neighbors[RIGHT];
     if (jj != -1) {
       t2a[0][jm-1] = q_multi[jj][k][0][1];
     } else {
       jj = gp[pid].neighbors[DOWN];
       if (jj != -1) {
         t2a[im-1][jm-1] = q_multi[jj][k][1][jm-1];
       }
     }
   } else if (gp[pid].neighbors[DOWN] == -1) {
     jj = gp[pid].neighbors[LEFT];
     if (jj != -1) {
       t2a[im-1][0] = q_multi[jj][k][im-1][jm-2];
     } else {
       jj = gp[pid].neighbors[UP];
       if (jj != -1) {
         t2a[0][0] = q_multi[jj][k][im-2][0];
       }
     }
     jj = gp[pid].neighbors[RIGHT];
     if (jj != -1) {
       t2a[im-1][jm-1] = q_multi[jj][k][im-1][1];
     } else {
       jj = gp[pid].neighbors[UP];
       if (jj != -1) {
         t2a[0][jm-1] = q_multi[jj][k][im-2][jm-1];
       }
     }
   } else if (gp[pid].neighbors[LEFT] == -1) {
     jj = gp[pid].neighbors[UP];
     if (jj != -1) {
       t2a[0][0] = q_multi[jj][k][im-2][0];
     }
     jj = gp[pid].neighbors[DOWN];
     if (jj != -1) {
       t2a[im-1][0] = q_multi[jj][k][1][0];
     }
   } else if (gp[pid].neighbors[RIGHT] == -1) {
     jj = gp[pid].neighbors[UP];
     if (jj != -1) {
       t2a[0][jm-1] = q_multi[jj][k][im-2][jm-1];
     }
     jj = gp[pid].neighbors[DOWN];
     if (jj != -1) {
       t2a[im-1][jm-1] = q_multi[jj][k][1][jm-1];
     }
   }

   j = gp[pid].neighbors[UP];
   if (j != -1) {
     t1a = (double *) t2a[0];
     t1b = (double *) q_multi[j][k][im-2];
     for (i=1;i<=lastcol;i++) {
       t1a[i] = t1b[i];
     }
   }
   j = gp[pid].neighbors[DOWN];
   if (j != -1) {
     t1a = (double *) t2a[im-1];
     t1b = (double *) q_multi[j][k][1];
     for (i=1;i<=lastcol;i++) {
       t1a[i] = t1b[i];
     }
   }
   j = gp[pid].neighbors[LEFT];
   if (j != -1) {
     t2b = (double **) q_multi[j][k];
     for (i=1;i<=lastrow;i++) {
       t2a[i][0] = t2b[i][jm-2];
     }
   }
   j = gp[pid].neighbors[RIGHT];
   if (j != -1) {
     t2b = (double **) q_multi[j][k];
     for (i=1;i<=lastrow;i++) {
       t2a[i][jm-1] = t2b[i][1];
     }
   }

}

void copy_rhs_borders(long k, long procid)
{
   long i;
   long j;
   long im;
   long jm;
   long lastrow;
   long lastcol;
   double **t2a;
   double **t2b;
   double *t1a;
   double *t1b;

   im = (imx[k]-2)/yprocs+2;
   jm = (jmx[k]-2)/xprocs+2;
   lastrow = (imx[k]-2)/yprocs;
   lastcol = (jmx[k]-2)/xprocs;

   t2a = (double **) rhs_multi[procid][k];
   if (gp[procid].neighbors[UPLEFT] != -1) {
     j = gp[procid].neighbors[UPLEFT];
     t2a[0][0] = rhs_multi[j][k][im-2][jm-2];
   }

   if (gp[procid].neighbors[UP] != -1) {
     j = gp[procid].neighbors[UP];
     if (j != -1) {
       t1a = (double *) t2a[0];
       t1b = (double *) rhs_multi[j][k][im-2];
       for (i=2;i<=lastcol;i+=2) {
         t1a[i] = t1b[i];
       }
     }
   }
   if (gp[procid].neighbors[LEFT] != -1) {
     j = gp[procid].neighbors[LEFT];
     if (j != -1) {
       t2b = (double **) rhs_multi[j][k];
       for (i=2;i<=lastrow;i+=2) {
         t2a[i][0] = t2b[i][jm-2];
       }
     }
   }
}

void copy_red(long k, long procid)
{
   long i;
   long j;
   long im;
   long jm;
   long lastrow;
   long lastcol;
   double **t2a;
   double **t2b;
   double *t1a;
   double *t1b;

   im = (imx[k]-2)/yprocs+2;
   jm = (jmx[k]-2)/xprocs+2;
   lastrow = (imx[k]-2)/yprocs;
   lastcol = (jmx[k]-2)/xprocs;

   t2a = (double **) q_multi[procid][k];
   j = gp[procid].neighbors[UP];
   if (j != -1) {
     t1a = (double *) t2a[0];
     t1b = (double *) q_multi[j][k][im-2];
     for (i=2;i<=lastcol;i+=2) {
       t1a[i] = t1b[i];
     }
   }
   j = gp[procid].neighbors[DOWN];
   if (j != -1) {
     t1a = (double *) t2a[im-1];
     t1b = (double *) q_multi[j][k][1];
     for (i=1;i<=lastcol;i+=2) {
       t1a[i] = t1b[i];
     }
   }
   j = gp[procid].neighbors[LEFT];
   if (j != -1) {
     t2b = (double **) q_multi[j][k];
     for (i=2;i<=lastrow;i+=2) {
       t2a[i][0] = t2b[i][jm-2];
     }
   }
   j = gp[procid].neighbors[RIGHT];
   if (j != -1) {
     t2b = (double **) q_multi[j][k];
     for (i=1;i<=lastrow;i+=2) {
       t2a[i][jm-1] = t2b[i][1];
     }
   }
}

void copy_black(long k, long procid)
{
   long i;
   long j;
   long im;
   long jm;
   long lastrow;
   long lastcol;
   double **t2a;
   double **t2b;
   double *t1a;
   double *t1b;

   im = (imx[k]-2)/yprocs+2;
   jm = (jmx[k]-2)/xprocs+2;
   lastrow = (imx[k]-2)/yprocs;
   lastcol = (jmx[k]-2)/xprocs;

   t2a = (double **) q_multi[procid][k];
   j = gp[procid].neighbors[UP];
   if (j != -1) {
     t1a = (double *) t2a[0];
     t1b = (double *) q_multi[j][k][im-2];
     for (i=1;i<=lastcol;i+=2) {
       t1a[i] = t1b[i];
     }
   }
   j = gp[procid].neighbors[DOWN];
   if (j != -1) {
     t1a = (double *) t2a[im-1];
     t1b = (double *) q_multi[j][k][1];
     for (i=2;i<=lastcol;i+=2) {
       t1a[i] = t1b[i];
     }
   }
   j = gp[procid].neighbors[LEFT];
   if (j != -1) {
     t2b = (double **) q_multi[j][k];
     for (i=1;i<=lastrow;i+=2) {
       t2a[i][0] = t2b[i][jm-2];
     }
   }
   j = gp[procid].neighbors[RIGHT];
   if (j != -1) {
     t2b = (double **) q_multi[j][k];
     for (i=2;i<=lastrow;i+=2) {
       t2a[i][jm-1] = t2b[i][1];
     }
   }
}

