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
#include "math.h"
#include "mdvar.h"
#include "parameters.h"
#include "mddata.h"
#include "split.h"
#include "global.h"

  /* this routine computes kinetic energy in each of the three spatial
     dimensions, and puts the computed values in the SUM array */
void KINETI(double *SUM, double HMAS, double OMAS, long ProcID)
{
    long dir, mol;
    double S;

    /* loop over the three directions */
    for (dir = XDIR; dir <= ZDIR; dir++) {
        S=0.0;
        /* loop over the molecules */
        for (mol = StartMol[ProcID]; mol < StartMol[ProcID+1]; mol++) {
            double *tempptr = VAR[mol].F[VEL][dir];
            S += ( tempptr[H1] * tempptr[H1] +
                  tempptr[H2] * tempptr[H2] ) * HMAS
                      + (tempptr[O] * tempptr[O]) * OMAS;
        }
        LOCK(gl->KinetiSumLock);
        SUM[dir]+=S;
        UNLOCK(gl->KinetiSumLock);
    } /* for */
} /* end of subroutine KINETI */

