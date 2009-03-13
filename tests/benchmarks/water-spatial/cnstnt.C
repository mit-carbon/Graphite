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
#include "water.h"
#include "wwpot.h"
#include "cnst.h"
#include "frcnst.h"
#include "fileio.h"
#include "parameters.h"
#include "mddata.h"
#include "global.h"

/* set up some constants; this routine is executed by
 * a single processor before others are created
 *
 * N : NORDER + 1 = 7 for a sixth-order method
 * C : DIMENSION C(N,N)
 */
void CNSTNT(long N, double *C)
{
    long NN,N1,K1;
    double TN,TK,CM;

    /* molecular constants for water in angstrom, radian, and a.m.u. */

    NATOMS = 3;
    ROH = 0.9572;
    ROHI = ONE/ROH;
    ROHI2 = ROHI*ROHI;
    ANGLE = 1.824218;
    OMAS = 15.99945;
    HMAS = 1.007825;
    WTMOL = OMAS+TWO*HMAS;

    /* units used to scale variables (in c.g.s.) */

    UNITT = 1.0e-15;
    UNITL = 1.0e-8;
    UNITM = 1.6605655e-24;
    BOLTZ = 1.380662e-16;
    AVGNO = 6.022045e23;

    /* force constants scaled (divided) by (UNITM/UNITT**2) */

    FC11 =  0.512596;
    FC33 =  0.048098;
    FC12 = -0.005823;
    FC13 =  0.016452;
    FC111 = -0.57191;
    FC333 = -0.007636;
    FC112 = -0.001867;
    FC113 = -0.002047;
    FC123 = -0.03083;
    FC133 = -0.0094245;
    FC1111 =  0.8431;
    FC3333 = -0.00193;
    FC1112 = -0.0030;
    FC1122 =  0.0036;
    FC1113 = -0.012;
    FC1123 =  0.0060;
    FC1133 = -0.0048;
    FC1233 =  0.0211;
    FC1333 =  0.006263;

    /* water-water interaction parameters */

    QQ = 0.07152158;
    A1 = 455.313100;
    B1 = 5.15271070;
    A2 = 0.27879839;
    B2 = 2.76084370;
    A3 = 0.60895706;
    B3 = 2.96189550;
    A4 = 0.11447336;
    B4 = 2.23326410;
    CM = 0.45682590;
    AB1 = A1*B1;
    AB2 = A2*B2;
    AB3 = A3*B3;
    AB4 = A4*B4;
    C1 = ONE-CM;
    C2 = 0.50*CM;
    QQ2 = 2.00*QQ;
    QQ4 = 2.00*QQ2;

    /*  calculate the coefficients of taylor series expansion */
    /*     for F(X), F"(X), F""(X), ...... (with DELTAT**N/N] included) */
    /*     in C(1,1),..... C(1,2),..... C(1,3),....... */

    C[1] = ONE;
    for (N1=2;N1<=N;N1++) {
        NN = N1-1;
        TN = NN;
        C[N1] = ONE;
        TK = ONE;
        for (K1=2;K1<=N1;K1++) {
            C[(K1-1)*N+NN] = C[(K1-2)*N+NN+1]*TN/TK;
            NN = NN-1;
            TN = TN-ONE;
            TK = TK+ONE;
        }
    }


    /* predictor-corrector constants for 2nd order differential equation */

    PCC[2] = ONE;
    N1 = N-1;
    switch(N1) {
    case 1:
    case 2:
        fprintf(six,"***** ERROR: THE ORDER HAS TO BE GREATER THAN 2 ****");
        break;
    case 3:
        PCC[0] = ONE/6.00;
        PCC[1] = FIVE/6.00;
        PCC[3] = ONE/3.00;
        break;
    case 4:
        PCC[0] = (double) 19.00/120.00;
        PCC[1] = (double) 3.00/4.00;
        PCC[3] = ONE/2.00;
        PCC[4] = ONE/12.00;
        break;
    case 5:
        PCC[0] = (double) 3.00/20.00;
        PCC[1] = (double) 251.00/360.00;
        PCC[3] = (double) 11.00/18.00;
        PCC[4] = ONE/6.00;
        PCC[5] = ONE/60.00;
        break;
    case 6:
        PCC[0] = (double) 863.00/6048.00;
        PCC[1] = (double) 665.00/1008.00;
        PCC[3] = (double) 25.00/36.00;
        PCC[4] = (double) 35.00/144.00;
        PCC[5] = ONE/24.00;
        PCC[6] = ONE/360.00;
        break;
    case 7:
        PCC[0] = (double) 275.00/2016.00;
        PCC[1] = (double) 19087.00/30240.00;
        PCC[3] = (double) 137.00/180.00;
        PCC[4] = FIVE/16.00;
        PCC[5] = (double) 17.00/240.00;
        PCC[6] = ONE/120.00;
        PCC[7] = ONE/2520.00;
        break;
    default:
        break;
    }
}           /* end of subroutine CNSTNT */
