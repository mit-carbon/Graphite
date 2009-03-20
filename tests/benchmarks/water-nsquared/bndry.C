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
#include "mdvar.h"
#include "parameters.h"
#include "mddata.h"
#include "split.h"
#include "global.h"

/* this routine puts the molecules back inside the box if they are out */
void BNDRY(long ProcID)
{
    long mol, dir;
    double *extra_p;

    /* for each molecule */
    for (mol = StartMol[ProcID]; mol < StartMol[ProcID+1]; mol++) {
        /* for each direction */
        for ( dir = XDIR; dir <= ZDIR; dir++ ) {
            extra_p = VAR[mol].F[DISP][dir];
            /* if the oxygen atom is out of the box */
            if (extra_p[O] > BOXL) {
                /* move all three atoms back in the box */
                extra_p[H1] -= BOXL;
                extra_p[O]  -= BOXL;
                extra_p[H2] -= BOXL;
            }
            else if (extra_p[O] < 0.00) {
                extra_p[H1] += BOXL;
                extra_p[O]  += BOXL;
                extra_p[H2] += BOXL;
            }
        } /* for dir */
    } /* for mol */
} /* end of subroutine BNDRY */
