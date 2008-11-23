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

/* predicts new values for displacement and its five derivatives
 *  using Gear's sixth-order predictor-corrector method
 *
 * NOR1 : NOR1 = NORDER + 1 = 7 (for a sixth-order method)
 */
void PREDIC(double *C, long NOR1, long ProcID)
{
    /*   this routine calculates predicted F(X), F'(X), F''(X), ... */

    long JIZ;
    long  JI;
    long  L;
    long func, i, j, k, dir, atom;
    double S;
    struct link *curr_ptr;
    struct list_of_boxes *curr_box;

    curr_box = my_boxes[ProcID];

    while (curr_box) {

        i = curr_box->coord[XDIR];  /* X coordinate of box */
        j = curr_box->coord[YDIR];  /* Y coordinate of box */
        k = curr_box->coord[ZDIR];  /* Z coordinate of box */

        /* Loop through the current box's molecules */

        curr_ptr = BOX[i][j][k].list;

        while (curr_ptr) {

            JIZ = 2;

            /* loop over F(X), F'(X), F''(X), etc. */

            for (func = 0; func < NORDER; func++) {
                for ( dir = 0; dir < NDIR; dir++)
                    for ( atom = 0; atom < NATOM; atom++ ) {
                        JI = JIZ;
                        /* sum over Taylor Series */
                        S = 0.0;
                        for ( L = func; L < NORDER; L++) {
                            S += C[JI] * curr_ptr->mol.F[L+1][dir][atom];
                            JI++;
                        } /* for L */
                        curr_ptr->mol.F[func][dir][atom] += S;
                    } /* for atom */
                JIZ += NOR1;
            } /* for func */
            curr_ptr = curr_ptr->next_mol;
        } /* while curr_ptr */
        curr_box = curr_box->next_box;
    } /* while curr_box */

} /* end of subroutine PREDIC */

/* corrects the predicted values
 *
 * PCC  : the predictor-corrector constants
 * NOR1 : NORDER + 1 = 7 for a sixth-order method)
 */
void CORREC(double *PCC, long NOR1, long ProcID)
{

    /*   This routine calculates corrected F(X), F'(X), F"(X),
     *   from corrected F(X) = predicted F(X) + PCC(1)*(FR-SD)
     *   where SD is predicted accl. F"(X) and FR is computed
     *   accl. (force/mass) at predicted position
     */

    double Y;
    long i, j, k, dir, atom, func;
    struct link *curr_ptr;
    box_list *curr_box;

    curr_box = my_boxes[ProcID];

    while (curr_box) {

        i = curr_box->coord[XDIR];  /* X coordinate of box */
        j = curr_box->coord[YDIR];  /* Y coordinate of box */
        k = curr_box->coord[ZDIR];  /* Z coordinate of box */

        /* Loop through the current box's molecules */

        curr_ptr = BOX[i][j][k].list;
        while (curr_ptr) {

            for (dir = 0; dir < NDIR; dir++) {
                for (atom = 0; atom < NATOM; atom++) {
                    Y = curr_ptr->mol.F[FORCES][dir][atom] -
                        curr_ptr->mol.F[ACC][dir][atom];
                    for ( func = 0; func < NOR1; func++)
                        curr_ptr->mol.F[func][dir][atom] += PCC[func] * Y;
                } /* for atom */
            } /* for dir */
            curr_ptr= curr_ptr->next_mol;
        } /* while curr_ptr */
        curr_box = curr_box->next_box;
    } /* while curr_box */

} /* end of subroutine CORREC */
