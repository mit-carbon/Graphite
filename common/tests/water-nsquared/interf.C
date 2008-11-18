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
#include "water.h"
#include "wwpot.h"
#include "cnst.h"
#include "parameters.h"
#include "mddata.h"
#include "split.h"
#include "global.h"

double ****PFORCES;

/* in this version of interf, a private force array is maintained */
/* for every process.  A process computes interactions into its   */
/* private force array, and later updates the shared destination  */
/* array with locks, but only updates those locations that it     */
/* computed something for                                         */

void INTERF(long DEST, double *VIR, long ProcID)
{
    /* This routine gets called both from main() and from mdmain().
       When called from main(), it is used to estimate the initial
       accelerations by computing intermolecular forces.  When called
       from mdmain(), it is used to compute intermolecular forces.
       The parameter DEST specifies whether results go into the
       accelerations or the forces. Uses routine UPDATE_FORCES in this
       file, and routine CSHIFT in file cshift.U */
    /*
      .....this routine calculates inter-molecular interaction forces
      the distances are arranged in the order  M-M, M-H1, M-H3, H1-M,
      H3-M, H1-H3, H1-H1, H3-H1, H3-H3, O-O, O-H1, O-H3, H1-O, H3-O,
      where the M are "centers" of the molecules.
      */

    long mol, comp, dir, icomp;
    long comp_last, half_mol;
    long    KC, K;
    double YL[15], XL[15], ZL[15], RS[15], FF[15], RL[15]; /* per-
                                                              interaction arrays that hold some computed distances */
    double  FTEMP;
    double LVIR = 0.0;
    double *temp_p;

    { /* initialize PFORCES array */

        long ct1,ct2,ct3;

        for (ct1 = 0; ct1<NMOL; ct1++)
            for (ct2 = 0; ct2<NDIR; ct2++)
                for (ct3 = 0; ct3<NATOM; ct3++)
                    PFORCES[ProcID][ct1][ct2][ct3] = 0;

    }

    half_mol = NMOL/2;
    for (mol = StartMol[ProcID]; mol < StartMol[ProcID+1]; mol++) {
        comp_last = mol + half_mol;
        if (NMOL%2 == 0) {
            if ((half_mol <= mol) && (mol%2 == 0)) {
                comp_last--;
            }
            if ((mol < half_mol) && (comp_last%2 == 1)) {
                comp_last--;
            }
        }
        for (icomp = mol+1; icomp <= comp_last; icomp++) {
            comp = icomp;
            if (comp > NMOL1) comp = comp%NMOL;

            /*  compute some intermolecular distances */

            CSHIFT(VAR[mol].F[DISP][XDIR],VAR[comp].F[DISP][XDIR],
                   VAR[mol].VM[XDIR],VAR[comp].VM[XDIR],XL,BOXH,BOXL);
            CSHIFT(VAR[mol].F[DISP][YDIR],VAR[comp].F[DISP][YDIR],
                   VAR[mol].VM[YDIR],VAR[comp].VM[YDIR],YL,BOXH,BOXL);
            CSHIFT(VAR[mol].F[DISP][ZDIR],VAR[comp].F[DISP][ZDIR],
                   VAR[mol].VM[ZDIR],VAR[comp].VM[ZDIR],ZL,BOXH,BOXL);

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

                UPDATE_FORCES(mol, comp, XL, YL, ZL, FF, ProcID);

            }  /* if KC != 9 */
        } /* for comp */
    } /* for mol */

    /*  accumulate the running sum from private
        per-interaction partial sums   */
    LOCK(gl->InterfVirLock);
    *VIR = *VIR + LVIR;
    UNLOCK(gl->InterfVirLock);

    /* at the end of the above force-computation, comp_last */
    /* contains the number of the last molecule (no modulo) */
    /* that this process touched                            */

    if (comp_last > NMOL1) {
        for (mol = StartMol[ProcID]; mol < NMOL; mol++) {
            ALOCK(gl->MolLock, mol % MAXLCKS);
            for ( dir = XDIR; dir  <= ZDIR; dir++) {
                temp_p = VAR[mol].F[DEST][dir];
                temp_p[H1] += PFORCES[ProcID][mol][dir][H1];
                temp_p[O]  += PFORCES[ProcID][mol][dir][O];
                temp_p[H2] += PFORCES[ProcID][mol][dir][H2];
            }
            AULOCK(gl->MolLock, mol % MAXLCKS);
        }
        comp = comp_last % NMOL;
        for (mol = 0; ((mol <= comp) && (mol < StartMol[ProcID])); mol++) {
            ALOCK(gl->MolLock, mol % MAXLCKS);
            for ( dir = XDIR; dir  <= ZDIR; dir++) {
                temp_p = VAR[mol].F[DEST][dir];
                temp_p[H1] += PFORCES[ProcID][mol][dir][H1];
                temp_p[O]  += PFORCES[ProcID][mol][dir][O];
                temp_p[H2] += PFORCES[ProcID][mol][dir][H2];
            }
            AULOCK(gl->MolLock, mol % MAXLCKS);
        }
    }
    else{
        for (mol = StartMol[ProcID]; mol <= comp_last; mol++) {
            ALOCK(gl->MolLock, mol % MAXLCKS);
            for ( dir = XDIR; dir  <= ZDIR; dir++) {
                temp_p = VAR[mol].F[DEST][dir];
                temp_p[H1] += PFORCES[ProcID][mol][dir][H1];
                temp_p[O]  += PFORCES[ProcID][mol][dir][O];
                temp_p[H2] += PFORCES[ProcID][mol][dir][H2];
            }
            AULOCK(gl->MolLock, mol % MAXLCKS);
        }
    }

    /* wait till all forces are updated */

    BARRIER(gl->InterfBar, NumProcs);

    /* divide final forces by masses */

    for (mol = StartMol[ProcID]; mol < StartMol[ProcID+1]; mol++) {
        for ( dir = XDIR; dir  <= ZDIR; dir++) {
            temp_p = VAR[mol].F[DEST][dir];
            temp_p[H1] = temp_p[H1] * FHM;
            temp_p[O]  = temp_p[O] * FOM;
            temp_p[H2] = temp_p[H2] * FHM;
        } /* for dir */
    } /* for mol */

}/* end of subroutine INTERF */

  /* from the computed distances etc., compute the
     intermolecular forces and update the force (or
     acceleration) locations */
void UPDATE_FORCES(long mol, long comp, double *XL, double *YL, double *ZL, double *FF, long ProcID)
{
    long K;
    double G110[3], G23[3], G45[3], TT1[3], TT[3], TT2[3];
    double GG[15][3];
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
    /* lock locations for the molecule to be updated */
    tx_p = PFORCES[ProcID][mol][XDIR];
    ty_p = PFORCES[ProcID][mol][YDIR];
    tz_p = PFORCES[ProcID][mol][ZDIR];
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

    tx_p = PFORCES[ProcID][comp][XDIR];
    ty_p = PFORCES[ProcID][comp][YDIR];
    tz_p = PFORCES[ProcID][comp][ZDIR];
    tx_p[H1] +=
        -GG[6][XDIR]-GG[8][XDIR]-GG[11][XDIR]-TT2[XDIR]-GG[2][XDIR];
    tx_p[O] +=
        -G110[XDIR]-GG[13][XDIR]-GG[14][XDIR]-C1*G45[XDIR];
    tx_p[H2] +=
        -GG[7][XDIR]-GG[9][XDIR]-GG[12][XDIR]-TT2[XDIR]-GG[3][XDIR];
    ty_p[H1] +=
        -GG[6][YDIR]-GG[8][YDIR]-GG[11][YDIR]-TT2[YDIR]-GG[2][YDIR];
    ty_p[O] +=
        -G110[YDIR]-GG[13][YDIR]-GG[14][YDIR]-C1*G45[YDIR];
    ty_p[H2] +=
        -GG[7][YDIR]-GG[9][YDIR]-GG[12][YDIR]-TT2[YDIR]-GG[3][YDIR];
    tz_p[H1] +=
        -GG[6][ZDIR]-GG[8][ZDIR]-GG[11][ZDIR]-TT2[ZDIR]-GG[2][ZDIR];
    tz_p[O] +=
        -G110[ZDIR]-GG[13][ZDIR]-GG[14][ZDIR]-C1*G45[ZDIR];
    tz_p[H2] +=
        -GG[7][ZDIR]-GG[9][ZDIR]-GG[12][ZDIR]-TT2[ZDIR]-GG[3][ZDIR];
}           /* end of subroutine UPDATE_FORCES */
