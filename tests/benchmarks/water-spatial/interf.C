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
#include "mdvar.h"
#include "water.h"
#include "wwpot.h"
#include "cnst.h"
#include "parameters.h"
#include "mddata.h"
#include "split.h"
#include "global.h"

void INTERF(long DEST, double *VIR, long ProcID)
{

    /* This is the routine that computes intermolecular interactions and
     * hence takes up most of the execution time.  It is
     * called both from main() and from mdmain().
     * When called from main(), it is used to estimate the initial
     * accelerations by computing intermolecular forces.  When called
     * from mdmain(), it is used to compute intermolecular forces.
     * The parameter DEST specifies whether results go into the
     * accelerations or the forces. It uses the routine UPDATE_FORCES
     * which is defined in this file, and routine CSHIFT in file cshift.U
     *
     * This routine calculates inter-molecular interaction forces.
     * the distances are arranged in the order  M-M, M-H1, M-H2, H1-M,
     * H2-M, H1-H2, H1-H1, H2-H1, H2-H2, O-O, O-H1, O-H2, H1-O, H2-O,
     * where the M are "centers" of the molecules.
     */

    long dir;
    long KC, K;
    long i, j, k;

    long XBOX, YBOX, ZBOX, X_NUM, Y_NUM, Z_NUM;
    /* per interaction arrays that hold some computed distances */
    double YL[15], XL[15], ZL[15], RS[15], FF[15], RL[15];
    double  FTEMP;
    double LVIR = 0.0;
    struct link *curr_ptr, *neighbor_ptr;
    struct list_of_boxes *curr_box;
    double *temp_p;

    curr_box = my_boxes[ProcID];
    while (curr_box) {

        i = curr_box->coord[XDIR];  /* X coordinate of box */
        j = curr_box->coord[YDIR];  /* Y coordinate of box */
        k = curr_box->coord[ZDIR];  /* Z coordinate of box */

        /* loop over nearest neighbor boxes */

        for (XBOX=i-1; XBOX<=i+1; XBOX++) {
            for (YBOX=j-1; YBOX<=j+1; YBOX++) {
                for (ZBOX=k-1; ZBOX<=k+1; ZBOX++) {

                    /* Special case for two boxes per side */

                    if ((BOX_PER_SIDE == 2) && ((XBOX < 0) || (XBOX == 2) ||
                                                (YBOX < 0) || (YBOX == 2) || (ZBOX < 0) || (ZBOX == 2)))
                        continue;

                    X_NUM = XBOX;
                    Y_NUM = YBOX;
                    Z_NUM = ZBOX;

                    /* Make box number valid if out of box */

                    if (X_NUM == -1)
                        X_NUM += BOX_PER_SIDE;
                    else if (X_NUM >= BOX_PER_SIDE)
                        X_NUM -= BOX_PER_SIDE;
                    if (Y_NUM == -1)
                        Y_NUM += BOX_PER_SIDE;
                    else if (Y_NUM >= BOX_PER_SIDE)
                        Y_NUM -= BOX_PER_SIDE;
                    if (Z_NUM == -1)
                        Z_NUM += BOX_PER_SIDE;
                    else if (Z_NUM >= BOX_PER_SIDE)
                        Z_NUM -= BOX_PER_SIDE;

                    /* Don't do current box more than once */

                    if ((X_NUM == i) && (Y_NUM == j) && (Z_NUM == k) &&
                        ((XBOX != i) || (YBOX != j) || (ZBOX !=k))) {
                        continue;
                    }

                    neighbor_ptr = BOX[X_NUM][Y_NUM][Z_NUM].list;

                    while (neighbor_ptr) {

                        /* go through current box list */

                        curr_ptr = BOX[i][j][k].list;
                        while (curr_ptr) {

                            /* Don't do interaction with same molecule */

                            if (curr_ptr == neighbor_ptr) {
                                curr_ptr = curr_ptr->next_mol;
                                continue;
                            }

                            /*  compute some intermolecular distances;
                                first call CSHIFT to implement periodic boundary conditions */

                            CSHIFT(curr_ptr->mol.F[DISP][XDIR],neighbor_ptr->mol.F[DISP][XDIR],
                                   curr_ptr->mol.VM[XDIR],neighbor_ptr->mol.VM[XDIR],XL,BOXH,BOXL);
                            CSHIFT(curr_ptr->mol.F[DISP][YDIR],neighbor_ptr->mol.F[DISP][YDIR],
                                   curr_ptr->mol.VM[YDIR],neighbor_ptr->mol.VM[YDIR],YL,BOXH,BOXL);
                            CSHIFT(curr_ptr->mol.F[DISP][ZDIR],neighbor_ptr->mol.F[DISP][ZDIR],
                                   curr_ptr->mol.VM[ZDIR],neighbor_ptr->mol.VM[ZDIR],ZL,BOXH,BOXL);

                            KC=0;
                            for (K = 0; K < 9; K++) {
                                RS[K]=XL[K]*XL[K]+YL[K]*YL[K]+ZL[K]*ZL[K];
                                if (RS[K] > CUT2)
                                    KC++;
                            } /* for K */

                            if (KC != 9) {
                                for (K = 0; K < 14; K++)
                                    FF[K]=0.0;
                                if (RS[0] < CUT2) {
                                    FF[0]=QQ4/(RS[0]*sqrt(RS[0]))+REF4;
                                    LVIR = LVIR + FF[0]*RS[0];
                                } /* if */


                                for (K = 1; K < 5; K++) {
                                    if (RS[K] < CUT2) {
                                        FF[K]= -QQ2/(RS[K]*sqrt(RS[K]))-REF2;
                                        LVIR = LVIR + FF[K]*RS[K];
                                    } /* if */
                                    if (RS[K+4] <= CUT2) {
                                        RL[K+4]=sqrt(RS[K+4]);
                                        FF[K+4]=QQ/(RS[K+4]*RL[K+4])+REF1;
                                        LVIR = LVIR + FF[K+4]*RS[K+4];
                                    } /* if */
                                } /* for K */

                                if (KC == 0) {
                                    RS[9]=XL[9]*XL[9]+YL[9]*YL[9]+ZL[9]*ZL[9];
                                    RL[9]=sqrt(RS[9]);
                                    FF[9]=AB1*exp(-B1*RL[9])/RL[9];
                                    LVIR = LVIR + FF[9]*RS[9];
                                    for (K = 10; K < 14; K++) {
                                        FTEMP=AB2*exp(-B2*RL[K-5])/RL[K-5];
                                        FF[K-5]=FF[K-5]+FTEMP;
                                        LVIR= LVIR+FTEMP*RS[K-5];
                                        RS[K]=XL[K]*XL[K]+YL[K]*YL[K]+ZL[K]*ZL[K];
                                        RL[K]=sqrt(RS[K]);
                                        FF[K]=(AB3*exp(-B3*RL[K])-AB4*exp(-B4*RL[K]))/RL[K];
                                        LVIR = LVIR + FF[K]*RS[K];
                                    } /* for K */
                                } /* if KC == 0 */

                                UPDATE_FORCES(curr_ptr, DEST, XL, YL, ZL, FF);
                            }  /* if KC != 9 */

                            curr_ptr = curr_ptr->next_mol;
                        } /* while curr_ptr */

                        neighbor_ptr = neighbor_ptr->next_mol;
                    } /* while neighbor_ptr */

                }
            }
        }  /* neighbor boxes' for loops */

        curr_box = curr_box->next_box;
    }  /* while mybox */

    /*  accumulate running sum from private partial sums */

    LOCK(gl->InterfVirLock);
    *VIR = *VIR + LVIR/2.0;
    UNLOCK(gl->InterfVirLock);

    /* wait till all forces are updated */

    BARRIER(gl->InterfBar, NumProcs);

    /* divide final forces by masses */

    curr_box = my_boxes[ProcID];
    while (curr_box) {

        i = curr_box->coord[XDIR];  /* X coordinate of box */
        j = curr_box->coord[YDIR];  /* Y coordinate of box */
        k = curr_box->coord[ZDIR];  /* Z coordinate of box */

        curr_ptr = BOX[i][j][k].list;
        while (curr_ptr) {
            for ( dir = XDIR; dir <= ZDIR; dir++) {
                temp_p = curr_ptr->mol.F[DEST][dir];
                temp_p[H1] = temp_p[H1] * FHM;
                temp_p[O]  = temp_p[O] * FOM;
                temp_p[H2] = temp_p[H2] * FHM;
            } /* for dir */

            curr_ptr = curr_ptr->next_mol;
        } /* while curr_ptr */
        curr_box = curr_box->next_box;
    } /* while curr_box */

}/* end of subroutine INTERF */


/*************    UPDATE FORCES SUBROUTINE     *************/

void UPDATE_FORCES(struct link *link_ptr, long DEST, double *XL, double *YL, double *ZL, double *FF)
{
    /* From the computed distances etc., compute the
     * intermolecular forces and update the force (or
     * acceleration) locations.
     */

    long K;
    double G110[3], G23[3], G45[3], TT1[3], TT[3], TT2[3];
    double GG[15][3];

    /* tx_p, ty_p, tz_p are temporary pointers used to avoid too much
       pointer (and array) dereferencing.  Since the pointers dereferenced
       otherwise are global pointers probably stored in processor 0's memory,
       this can also have the effect of reducing hot-spotting and remote
       references when cache misses are incurred on these pointers. */

    double *tx_p, *ty_p, *tz_p;


    /*   CALCULATE X-COMPONENT FORCES */

    for (K = 0; K < 14; K++)  {
        GG[K+1][XDIR] = FF[K]*XL[K];
        GG[K+1][YDIR] = FF[K]*YL[K];
        GG[K+1][ZDIR] = FF[K]*ZL[K];
    }

    G110[XDIR] = GG[10][XDIR]+GG[1][XDIR]*C1;
    G110[YDIR] = GG[10][YDIR]+GG[1][YDIR]*C1;
    G110[ZDIR] = GG[10][ZDIR]+GG[1][ZDIR]*C1;
    G23[XDIR] = GG[2][XDIR]+GG[3][XDIR];
    G23[YDIR] = GG[2][YDIR]+GG[3][YDIR];
    G23[ZDIR] = GG[2][ZDIR]+GG[3][ZDIR];
    G45[XDIR]=GG[4][XDIR]+GG[5][XDIR];
    G45[YDIR]=GG[4][YDIR]+GG[5][YDIR];
    G45[ZDIR]=GG[4][ZDIR]+GG[5][ZDIR];
    TT1[XDIR] =GG[1][XDIR]*C2;
    TT1[YDIR] =GG[1][YDIR]*C2;
    TT1[ZDIR] =GG[1][ZDIR]*C2;
    TT[XDIR] =G23[XDIR]*C2+TT1[XDIR];
    TT[YDIR] =G23[YDIR]*C2+TT1[YDIR];
    TT[ZDIR] =G23[ZDIR]*C2+TT1[ZDIR];
    TT2[XDIR]=G45[XDIR]*C2+TT1[XDIR];
    TT2[YDIR]=G45[YDIR]*C2+TT1[YDIR];
    TT2[ZDIR]=G45[ZDIR]*C2+TT1[ZDIR];

    /* Update force or acceleration for link */

    tx_p = link_ptr->mol.F[DEST][XDIR];
    ty_p = link_ptr->mol.F[DEST][YDIR];
    tz_p = link_ptr->mol.F[DEST][ZDIR];

    tx_p[H1] +=
        GG[6][XDIR]+GG[7][XDIR]+GG[13][XDIR]+TT[XDIR]+GG[4][XDIR];
    tx_p[O] +=
        G110[XDIR] + GG[11][XDIR] +GG[12][XDIR]+C1*G23[XDIR];
    tx_p[H2] +=
        GG[8][XDIR]+GG[9][XDIR]+GG[14][XDIR]+TT[XDIR]+GG[5][XDIR];
    ty_p[H1] +=
        GG[6][YDIR]+GG[7][YDIR]+GG[13][YDIR]+TT[YDIR]+GG[4][YDIR];
    ty_p[O]  +=
        G110[YDIR]+GG[11][YDIR]+GG[12][YDIR]+C1*G23[YDIR];
    ty_p[H2] +=
        GG[8][YDIR]+GG[9][YDIR]+GG[14][YDIR]+TT[YDIR]+GG[5][YDIR];
    tz_p[H1] +=
        GG[6][ZDIR]+GG[7][ZDIR]+GG[13][ZDIR]+TT[ZDIR]+GG[4][ZDIR];
    tz_p[O]  +=
        G110[ZDIR]+GG[11][ZDIR]+GG[12][ZDIR]+C1*G23[ZDIR];
    tz_p[H2] +=
        GG[8][ZDIR]+GG[9][ZDIR]+GG[14][ZDIR]+TT[ZDIR]+GG[5][ZDIR];

} /* end of subroutine UPDATE_FORCES */
