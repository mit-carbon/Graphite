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

/* Set all the pointers to the proper locations for the q_multi and
   rhs_multi data structures */

EXTERN_ENV

#include "decs.h"

void link_all()
{
  long i;
  long j;

  for (j=0;j<nprocs;j++) {
    linkup(psium[j]);
    linkup(psilm[j]);
    linkup(psib[j]);
    linkup(ga[j]);
    linkup(gb[j]);
    linkup(work2[j]);
    linkup(work3[j]);
    linkup(work6[j]);
    linkup(tauz[j]);
    linkup(oldga[j]);
    linkup(oldgb[j]);
    for (i=0;i<=1;i++) {
      linkup(psi[j][i]);
      linkup(psim[j][i]);
      linkup(work1[j][i]);
      linkup(work4[j][i]);
      linkup(work5[j][i]);
      linkup(work7[j][i]);
      linkup(temparray[j][i]);
    }
  }
  link_multi();
}

void linkup(double **row_ptr)
{
  long i;
  double *a;
  double **row;
  double **y;
  long x_part;
  long y_part;

  x_part = (jm-2)/xprocs + 2;
  y_part = (im-2)/yprocs + 2;
  row = row_ptr;
  y = row + y_part;
  a = (double *) y;
  for (i=0;i<y_part;i++) {
    *row = (double *) a;
    row++;
    a += x_part;
  }
}

void link_multi()
{
  long i;
  long j;
  long l;
  double *a;
  double **row;
  double **y;
  unsigned long z;
  unsigned long zz;
  long x_part;
  long y_part;
  unsigned long d_size;

  z = ((unsigned long) q_multi + nprocs*sizeof(double ***));

  if (nprocs%2 == 1) {         /* To make sure that the actual data
                                  starts double word aligned, add an extra
                                  pointer */
    z += sizeof(double ***);
  }

  d_size = numlev*sizeof(double **);
  if (numlev%2 == 1) {       /* To make sure that the actual data
                                starts double word aligned, add an extra
                                pointer */
    d_size += sizeof(double **);
  }
  for (i=0;i<numlev;i++) {
    d_size += ((imx[i]-2)/yprocs+2)*((jmx[i]-2)/xprocs+2)*sizeof(double)+
             ((imx[i]-2)/yprocs+2)*sizeof(double *);
  }
  for (i=0;i<nprocs;i++) {
    q_multi[i] = (double ***) z;
    z += d_size;
  }
  for (j=0;j<nprocs;j++) {
    zz = (unsigned long) q_multi[j];
    zz += numlev*sizeof(double **);
    if (numlev%2 == 1) {       /* To make sure that the actual data
                                  starts double word aligned, add an extra
                                  pointer */
      zz += sizeof(double **);
    }
    for (i=0;i<numlev;i++) {
      d_size = ((imx[i]-2)/yprocs+2)*((jmx[i]-2)/xprocs+2)*sizeof(double)+
               ((imx[i]-2)/yprocs+2)*sizeof(double *);
      q_multi[j][i] = (double **) zz;
      zz += d_size;
    }
  }

  for (l=0;l<numlev;l++) {
    x_part = (jmx[l]-2)/xprocs + 2;
    y_part = (imx[l]-2)/yprocs + 2;
    for (j=0;j<nprocs;j++) {
      row = q_multi[j][l];
      y = row + y_part;
      a = (double *) y;
      for (i=0;i<y_part;i++) {
        *row = (double *) a;
        row++;
        a += x_part;
      }
    }
  }

  z = ((unsigned long) rhs_multi + nprocs*sizeof(double ***));
  if (nprocs%2 == 1) {         /* To make sure that the actual data
                                  starts double word aligned, add an extra
                                  pointer */
    z += sizeof(double ***);
  }

  d_size = numlev*sizeof(double **);
  if (numlev%2 == 1) {       /* To make sure that the actual data
                                starts double word aligned, add an extra
                                pointer */
    d_size += sizeof(double **);
  }
  for (i=0;i<numlev;i++) {
    d_size += ((imx[i]-2)/yprocs+2)*((jmx[i]-2)/xprocs+2)*sizeof(double)+
             ((imx[i]-2)/yprocs+2)*sizeof(double *);
  }
  for (i=0;i<nprocs;i++) {
    rhs_multi[i] = (double ***) z;
    z += d_size;
  }
  for (j=0;j<nprocs;j++) {
    zz = (unsigned long) rhs_multi[j];
    zz += numlev*sizeof(double **);
    if (numlev%2 == 1) {       /* To make sure that the actual data
                                  starts double word aligned, add an extra
                                  pointer */
      zz += sizeof(double **);
    }
    for (i=0;i<numlev;i++) {
      d_size = ((imx[i]-2)/yprocs+2)*((jmx[i]-2)/xprocs+2)*sizeof(double)+
               ((imx[i]-2)/yprocs+2)*sizeof(double *);
      rhs_multi[j][i] = (double **) zz;
      zz += d_size;
    }
  }

  for (l=0;l<numlev;l++) {
    x_part = (jmx[l]-2)/xprocs + 2;
    y_part = (imx[l]-2)/yprocs + 2;
    for (j=0;j<nprocs;j++) {
      row = rhs_multi[j][l];
      y = row + y_part;
      a = (double *) y;
      for (i=0;i<y_part;i++) {
        *row = (double *) a;
        row++;
        a += x_part;
      }
    }
  }

}


