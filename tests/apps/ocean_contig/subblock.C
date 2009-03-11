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
#include <math.h>
#include "decs.h"

void subblock()

{
   long i;
   long j;
   long k;
   long xportion;
   long xextra;
   long yportion;
   long yextra;
   long my_num;

/* Determine starting coord and number of points to process in     */
/* each direction                                                  */

   for (i=0;i<numlev;i++) {
     xportion = (jmx[i] - 2) / xprocs;
     xextra = (jmx[i] - 2) % xprocs;
     for (j=0;j<xprocs;j++) {
       for (k=0;k<yprocs;k++) {
         gp[k*xprocs+j].rel_num_x[i] = xportion;
       }
     }
     yportion = (imx[i] - 2) / yprocs;
     yextra = (imx[i] - 2) % yprocs;
     for (j=0;j<yprocs;j++) {
       for (k=0;k<xprocs;k++) {
         gp[j*xprocs+k].rel_num_y[i] = yportion;
       }
     }
   }

   for (my_num=0;my_num<nprocs;my_num++) {
     for (i=0;i<numlev;i++) {
       gp[my_num].rlist[i] = 1;
       gp[my_num].rljst[i] = 1;
       gp[my_num].rlien[i] = gp[my_num].rlist[i] + gp[my_num].rel_num_y[i];
       gp[my_num].rljen[i] = gp[my_num].rljst[i] + gp[my_num].rel_num_x[i];
       gp[my_num].eist[i] = gp[my_num].rlist[i] + 1;
       gp[my_num].oist[i] = gp[my_num].rlist[i];
       gp[my_num].ejst[i] = gp[my_num].rljst[i] + 1;
       gp[my_num].ojst[i] = gp[my_num].rljst[i];
     }
   }
  for (i=0;i<nprocs;i++) {
    gp[i].neighbors[LEFT] = -1;
    gp[i].neighbors[RIGHT] = -1;
    gp[i].neighbors[UP] = -1;
    gp[i].neighbors[DOWN] = -1;
    gp[i].neighbors[UPLEFT] = -1;
    gp[i].neighbors[UPRIGHT] = -1;
    gp[i].neighbors[DOWNLEFT] = -1;
    gp[i].neighbors[DOWNRIGHT] = -1;
    if (i >= xprocs) {
      gp[i].neighbors[UP] = i-xprocs;
    }
    if (i < nprocs-xprocs) {
      gp[i].neighbors[DOWN] = i+xprocs;
    }
    if ((i % xprocs) > 0) {
      gp[i].neighbors[LEFT] = i-1;
    }
    if ((i % xprocs) < (xprocs-1)) {
      gp[i].neighbors[RIGHT] = i+1;
    }
    j = gp[i].neighbors[UP];
    if (j != -1) {
      if ((j % xprocs) > 0) {
        gp[i].neighbors[UPLEFT] = j-1;
      }
      if ((j % xprocs) < (xprocs-1)) {
        gp[i].neighbors[UPRIGHT] = j+1;
      }
    }
    j = gp[i].neighbors[DOWN];
    if (j != -1) {
      if ((j % xprocs) > 0) {
        gp[i].neighbors[DOWNLEFT] = j-1;
      }
      if ((j % xprocs) < (xprocs-1)) {
        gp[i].neighbors[DOWNRIGHT] = j+1;
      }
    }
  }
  for (i=0;i<nprocs;i++) {
    gp[i].rownum = i/xprocs;
    gp[i].colnum = i%xprocs;
  }
}

