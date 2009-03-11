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

#include <math.h>
#include "mdvar.h"
#include "parameters.h"
#include "mddata.h"
#include "split.h"
#include "global.h"

/* this routine computes kinetic energy in each of the three spatial
   dimensions, and puts the computed values in the SUM array */
void KINETI(double *SUM, double HMAS, double OMAS, long ProcID)
{
    long dir, i, j, k;
    double S;
    struct link *curr_ptr;
    struct list_of_boxes *curr_box;
    double *tempptr;

    /* Loop over three directions */

    for (dir = XDIR; dir <= ZDIR; dir++) {
        S=0.0;

        /* loop over processor's boxes */
        curr_box = my_boxes[ProcID];

        while (curr_box) {

            i = curr_box->coord[XDIR];  /* X coordinate of box */
            j = curr_box->coord[YDIR];  /* Y coordinate of box */
            k = curr_box->coord[ZDIR];  /* Z coordinate of box */

            /* loop over the molecules */

            curr_ptr = BOX[i][j][k].list;
            while (curr_ptr) {
                tempptr = curr_ptr->mol.F[VEL][dir];
                S += (tempptr[H1] * tempptr[H1] +
                      tempptr[H2] * tempptr[H2] ) * HMAS +
                          (tempptr[O] * tempptr[O]) * OMAS;
                curr_ptr = curr_ptr->next_mol;
            } /* while curr_ptr */

            curr_box = curr_box->next_box;

        } /* while curr_box */

        LOCK(gl->KinetiSumLock);
        SUM[dir]+=S;
        UNLOCK(gl->KinetiSumLock);

    } /* for dir */

} /* end of subroutine KINETI */
