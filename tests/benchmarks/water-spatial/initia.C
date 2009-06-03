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
#include <stdio.h>
#include "mdvar.h"
#include "water.h"
#include "cnst.h"
#include "fileio.h"
#include "parameters.h"
#include "mddata.h"
#include "split.h"
#include "global.h"

void INITIA()
{
    /*   this routine initializes the positions of the molecules along
         a regular cubical lattice, and randomizes the initial velocities of
         the atoms.  The random numbers used in the initialization of velocities
         are read from the file random.in, which must be in the current working
         directory in which the program is run  */

    FILE *random_numbers;       /* points to input file containing
                                   pseudo-random numbers for initializing
                                   velocities */
    double XMAS[4];
    double SUX, SUY, SUZ, SUMX, SUMY, SUMZ, FAC;
    long X_INDEX, Y_INDEX, Z_INDEX;
    long atom=0;
    long i, j, k;
    struct link *link_ptr, *curr_ptr, *last_ptr;
#ifdef RANDOM
    long Part_per_box, Unassigned, m, pid;
#else
    long mol = 0, XT[4], YT[4], Z;
#endif

    //random_numbers = fopen("random.in","r");
    random_numbers = fopen("tests/benchmarks/water-spatial/random.in","r");
    if (random_numbers == NULL) {
        fprintf(stderr,"Error in opening file random.in\n");
        fflush(stderr);
        exit(-1);
    }

    XMAS[1]=sqrt(OMAS*HMAS);
    XMAS[0]=HMAS;
    XMAS[2]=HMAS;

    /* .....assign positions */

    {
        long deriv;

        double NS = pow((double) NMOL, 1.0/3.0) - 0.00000000000001;
        double XS = BOXL/NS;
        double ZERO = XS * 0.50;
        double WCOS = ROH * cos(ANGLE * 0.5);
        double WSIN = ROH * sin(ANGLE * 0.5);

        printf("\nNS = %.16f\n",NS);
        printf("BOXL = %10f\n",BOXL);
        printf("CUTOFF = %10f\n",CUTOFF);
        printf("BOX_LENGTH = %10f\n",BOX_LENGTH);
        printf("BOX_PER_SIDE = %ld\n",BOX_PER_SIDE);
        printf("XS = %10f\n",XS);
        printf("ZERO = %g\n",ZERO);
        printf("WCOS = %f\n",WCOS);
        printf("WSIN = %f\n",WSIN);
        fflush(stdout);

#ifdef RANDOM
        /* if we want to initialize to a random distribution of displacements
           for the molecules, rather than a distribution along a regular lattice
           spaced according to intermolecular distances in water */

        srand(1023);
        fscanf(random_numbers,"%lf",&SUX);

        SUMX=0.0;
        SUMY=0.0;
        SUMZ=0.0;

        Part_per_box = NMOL / (BOX_PER_SIDE * BOX_PER_SIDE * BOX_PER_SIDE);
        Unassigned = NMOL - (Part_per_box * BOX_PER_SIDE * BOX_PER_SIDE * BOX_PER_SIDE);
        printf("Part_per_box = %ld, BOX_PER_SIDE = %ld, Unassigned = %ld\n",Part_per_box, BOX_PER_SIDE, Unassigned);
        for (i = 0; i < BOX_PER_SIDE; i++)
            for (j = 0; j < BOX_PER_SIDE; j++)
                for (k = 0; k < BOX_PER_SIDE; k++)
                    for (m = 0; m < Part_per_box; m++) {
                        link_ptr = (struct link *) G_MALLOC(sizeof(link_type));
                        link_ptr->mol.F[DISP][XDIR][O] = xrand(BOX_LENGTH * i, BOX_LENGTH * (i+1));
                        link_ptr->mol.F[DISP][XDIR][H1] = link_ptr->mol.F[DISP][XDIR][O] + WCOS;
                        link_ptr->mol.F[DISP][XDIR][H2] = link_ptr->mol.F[DISP][XDIR][H1];
                        link_ptr->mol.F[DISP][YDIR][O] = xrand(BOX_LENGTH * j, BOX_LENGTH * (j+1));
                        link_ptr->mol.F[DISP][YDIR][H1] = link_ptr->mol.F[DISP][YDIR][O] + WSIN;
                        link_ptr->mol.F[DISP][YDIR][H2] = link_ptr->mol.F[DISP][YDIR][O] - WSIN;
                        link_ptr->mol.F[DISP][ZDIR][O] = xrand(BOX_LENGTH * k, BOX_LENGTH * (k+1));
                        link_ptr->mol.F[DISP][ZDIR][H1] = link_ptr->mol.F[DISP][ZDIR][O];
                        link_ptr->mol.F[DISP][ZDIR][H2] = link_ptr->mol.F[DISP][ZDIR][O];

                        for (atom = 0; atom < NATOMS; atom++) {
                            /* read random velocities from file random.in */
                            fscanf(random_numbers,"%lf",&link_ptr->mol.F[VEL][XDIR][atom]);
                            fscanf(random_numbers,"%lf",&link_ptr->mol.F[VEL][YDIR][atom]);
                            fscanf(random_numbers,"%lf",&link_ptr->mol.F[VEL][ZDIR][atom]);
                            SUMX = SUMX + link_ptr->mol.F[VEL][XDIR][atom];
                            SUMY = SUMY + link_ptr->mol.F[VEL][YDIR][atom];
                            SUMZ = SUMZ + link_ptr->mol.F[VEL][ZDIR][atom];
                            /* set acceleration and all higher-order derivatives to zero */
                            for (deriv = ACC; deriv < MAXODR; deriv++) {
                                link_ptr->mol.F[deriv][XDIR][atom] = 0.0;
                                link_ptr->mol.F[deriv][YDIR][atom] = 0.0;
                                link_ptr->mol.F[deriv][ZDIR][atom] = 0.0;
                            }
                        }

                        link_ptr->next_mol = NULL;        /* Terminating link */

                        /* update box indices in all three dimensions */
                        X_INDEX = (long) (link_ptr->mol.F[DISP][XDIR][O]/BOX_LENGTH);
                        Y_INDEX = (long) (link_ptr->mol.F[DISP][YDIR][O]/BOX_LENGTH);
                        Z_INDEX = (long) (link_ptr->mol.F[DISP][ZDIR][O]/BOX_LENGTH);

                        /* Put X_, Y_, and Z_INDEX back in box */

                        if (X_INDEX >=BOX_PER_SIDE) X_INDEX -= 1;
                        if (Y_INDEX >=BOX_PER_SIDE) Y_INDEX -= 1;
                        if (Z_INDEX >=BOX_PER_SIDE) Z_INDEX -= 1;

                        /* get list ptr */
                        curr_ptr = BOX[X_INDEX][Y_INDEX][Z_INDEX].list;

                        if (curr_ptr == NULL) {             /* No links in box yet */
                            BOX[X_INDEX][Y_INDEX][Z_INDEX].list = link_ptr;
                        } else {
                            while (curr_ptr) {               /* Scan to end of list */
                                last_ptr = curr_ptr;
                                curr_ptr = curr_ptr->next_mol;
                            } /* while curr_ptr */
                            last_ptr->next_mol = link_ptr;    /* Add to end of list */
                        } /* if curr_ptr */
                    }

        /* distribute the unassigned molecules evenly among processors */
        pid = 0;
        for (i = 0; i < Unassigned; i++) {
            link_ptr = (struct link *) G_MALLOC(sizeof(link_type));
            link_ptr->mol.F[DISP][XDIR][O] = xrand(BOX_LENGTH * start_end[pid]->box[XDIR][FIRST], BOX_LENGTH * (start_end[pid]->box[XDIR][LAST] + 1));
            link_ptr->mol.F[DISP][XDIR][H1] = link_ptr->mol.F[DISP][XDIR][O] + WCOS;
            link_ptr->mol.F[DISP][XDIR][H2] = link_ptr->mol.F[DISP][XDIR][H1];
            link_ptr->mol.F[DISP][YDIR][O] = xrand(BOX_LENGTH * start_end[pid]->box[YDIR][FIRST], BOX_LENGTH * (start_end[pid]->box[YDIR][LAST] + 1));
            link_ptr->mol.F[DISP][YDIR][H1] = link_ptr->mol.F[DISP][YDIR][O] + WSIN;
            link_ptr->mol.F[DISP][YDIR][H2] = link_ptr->mol.F[DISP][YDIR][O] - WSIN;
            link_ptr->mol.F[DISP][ZDIR][O] = xrand(BOX_LENGTH * start_end[pid]->box[ZDIR][FIRST], BOX_LENGTH * (start_end[pid]->box[ZDIR][LAST] + 1));
            link_ptr->mol.F[DISP][ZDIR][H1] = link_ptr->mol.F[DISP][ZDIR][O];
            link_ptr->mol.F[DISP][ZDIR][H2] = link_ptr->mol.F[DISP][ZDIR][O];

            pid = (pid + 1) % NumProcs;

            /* read random velocities from file random.in */
            for (atom = 0; atom < NATOMS; atom++) {
                fscanf(random_numbers,"%lf",&link_ptr->mol.F[VEL][XDIR][atom]);
                fscanf(random_numbers,"%lf",&link_ptr->mol.F[VEL][YDIR][atom]);
                fscanf(random_numbers,"%lf",&link_ptr->mol.F[VEL][ZDIR][atom]);
                SUMX = SUMX + link_ptr->mol.F[VEL][XDIR][atom];
                SUMY = SUMY + link_ptr->mol.F[VEL][YDIR][atom];
                SUMZ = SUMZ + link_ptr->mol.F[VEL][ZDIR][atom];
                /* set acceleration and all higher derivatives to zero */
                for (deriv = ACC; deriv < MAXODR; deriv++) {
                    link_ptr->mol.F[deriv][XDIR][atom] = 0.0;
                    link_ptr->mol.F[deriv][YDIR][atom] = 0.0;
                    link_ptr->mol.F[deriv][ZDIR][atom] = 0.0;
                }
            }

            link_ptr->next_mol = NULL;        /* Terminating link */

            /* updated box indices in all dimensions */

            X_INDEX = (long) (link_ptr->mol.F[DISP][XDIR][O]/BOX_LENGTH);
            Y_INDEX = (long) (link_ptr->mol.F[DISP][YDIR][O]/BOX_LENGTH);
            Z_INDEX = (long) (link_ptr->mol.F[DISP][ZDIR][O]/BOX_LENGTH);

            /* Put X_, Y_, and Z_INDEX back in box */

            if (X_INDEX >=BOX_PER_SIDE) X_INDEX -= 1;
            if (Y_INDEX >=BOX_PER_SIDE) Y_INDEX -= 1;
            if (Z_INDEX >=BOX_PER_SIDE) Z_INDEX -= 1;

            /* get list ptr */
            curr_ptr = BOX[X_INDEX][Y_INDEX][Z_INDEX].list;

            if (curr_ptr == NULL) {             /* No links in box yet */
                BOX[X_INDEX][Y_INDEX][Z_INDEX].list = link_ptr;
            } else {
                while (curr_ptr) {               /* Scan to end of list */
                    last_ptr = curr_ptr;
                    curr_ptr = curr_ptr->next_mol;
                } /* while curr_ptr */
                last_ptr->next_mol = link_ptr;    /* Add to end of list */
            } /* if curr_ptr */
        }

#else
        /* not random initial placement, but rather along a regular
           lattice.  This is the default and the prefered initialization
           since random does not necessarily make sense from the viewpoint
           of preserving bond distances */

        fprintf(six, "***** NEW RUN STARTING FROM REGULAR LATTICE *****\n");
        fflush(six);
        XT[2] = ZERO;
        mol = 0;
        fscanf(random_numbers,"%lf",&SUX);

        SUMX=0.0;
        SUMY=0.0;
        SUMZ=0.0;

        /* Generate displacements along a regular lattice */

        for (i=0; i < NS; i++) {
            XT[1]=XT[2]+WCOS;
            XT[3]=XT[1];
            YT[2]=ZERO;
            for (j=0; j < NS; j+=1) {
                YT[1]=YT[2]+WSIN;
                YT[3]=YT[2]-WSIN;
                Z=ZERO;
                for (k = 0; k < NS; k++) {
                    link_ptr = (struct link *) G_MALLOC(sizeof(link_type));
                    for (atom = 0; atom < NATOMS; atom++) {

                        /* displacements for atom */
                        link_ptr->mol.F[DISP][XDIR][atom] = XT[atom+1];
                        link_ptr->mol.F[DISP][YDIR][atom] = YT[atom+1];
                        link_ptr->mol.F[DISP][ZDIR][atom] = Z;

                        /* read random velocities from file random.in */
                        fscanf(random_numbers,"%lf",&link_ptr->mol.F[VEL][XDIR][atom]);
                        fscanf(random_numbers,"%lf",&link_ptr->mol.F[VEL][YDIR][atom]);
                        fscanf(random_numbers,"%lf",&link_ptr->mol.F[VEL][ZDIR][atom]);
                        SUMX = SUMX + link_ptr->mol.F[VEL][XDIR][atom];
                        SUMY = SUMY + link_ptr->mol.F[VEL][YDIR][atom];
                        SUMZ = SUMZ + link_ptr->mol.F[VEL][ZDIR][atom];
                        for (deriv = ACC; deriv < MAXODR; deriv++) {
                            link_ptr->mol.F[deriv][XDIR][atom] = 0.0;
                            link_ptr->mol.F[deriv][YDIR][atom] = 0.0;
                            link_ptr->mol.F[deriv][ZDIR][atom] = 0.0;
                        }
                    }

                    link_ptr->next_mol = NULL;        /* Terminating link */
                    mol++;
                    Z += XS;

                    /* update box numbers in all dimensions */

                    X_INDEX = (long) (link_ptr->mol.F[DISP][XDIR][O]/BOX_LENGTH);
                    Y_INDEX = (long) (link_ptr->mol.F[DISP][YDIR][O]/BOX_LENGTH);
                    Z_INDEX = (long) (link_ptr->mol.F[DISP][ZDIR][O]/BOX_LENGTH);

                    /* Put X_, Y_, and Z_INDEX back in box */

                    if (X_INDEX >=BOX_PER_SIDE) X_INDEX -= 1;
                    if (Y_INDEX >=BOX_PER_SIDE) Y_INDEX -= 1;
                    if (Z_INDEX >=BOX_PER_SIDE) Z_INDEX -= 1;

                    /* get list ptr */
                    curr_ptr = BOX[X_INDEX][Y_INDEX][Z_INDEX].list;

                    if (curr_ptr == NULL) {             /* No links in box yet */
                        BOX[X_INDEX][Y_INDEX][Z_INDEX].list = link_ptr;
                    } else {
                        while (curr_ptr) {               /* Scan to end of list */
                            last_ptr = curr_ptr;
                            curr_ptr = curr_ptr->next_mol;
                        } /* while curr_ptr */
                        last_ptr->next_mol = link_ptr;    /* Add to end of list */
                    } /* if curr_ptr */

                } /* for k */
                YT[2] += XS;
            } /* for j */
            XT[2] += XS;
        } /* for i */

        if (NMOL != mol) {
            printf("Lattice init error: total mol %ld != NMOL %ld\n", mol, NMOL);
            exit(-1);
        }
#endif
    }
    /* assign random momenta */
    /* find average momenta per atom */

    SUMX=SUMX/(NATOMS*NMOL);
    SUMY=SUMY/(NATOMS*NMOL);
    SUMZ=SUMZ/(NATOMS*NMOL);

    /*  find normalization factor so that <k.e.>=KT/2  */

    SUX=0.0;
    SUY=0.0;
    SUZ=0.0;

    for (i=0; i<BOX_PER_SIDE; i++) {
        for (j=0; j<BOX_PER_SIDE; j++) {
            for (k=0; k<BOX_PER_SIDE; k++) {
                curr_ptr = BOX[i][j][k].list;
                while (curr_ptr) {
                    SUX = SUX +
                        (pow( (curr_ptr->mol.F[VEL][XDIR][H1] - SUMX),2.0)
                         +pow( (curr_ptr->mol.F[VEL][XDIR][H2] - SUMX),2.0))/HMAS
                             +pow( (curr_ptr->mol.F[VEL][XDIR][O]  - SUMX),2.0)/OMAS;

                    SUY = SUY +
                        (pow( (curr_ptr->mol.F[VEL][YDIR][H1] - SUMY),2.0)
                         +pow( (curr_ptr->mol.F[VEL][YDIR][H2] - SUMY),2.0))/HMAS
                             +pow( (curr_ptr->mol.F[VEL][YDIR][O]  - SUMY),2.0)/OMAS;

                    SUZ = SUZ +
                        (pow( (curr_ptr->mol.F[VEL][ZDIR][H1] - SUMZ),2.0)
                         + pow( (curr_ptr->mol.F[VEL][ZDIR][H2] - SUMZ),2.0))/HMAS
                             +pow( (curr_ptr->mol.F[VEL][ZDIR][O]  - SUMZ),2.0)/OMAS;

                    curr_ptr = curr_ptr->next_mol;
                } /* while curr_ptr */
            }
        }
    } /* for boxes */

    FAC=BOLTZ*TEMP*NATMO/UNITM * pow((UNITT*TSTEP/UNITL),2.0);
    SUX=sqrt(FAC/SUX);
    SUY=sqrt(FAC/SUY);
    SUZ=sqrt(FAC/SUZ);

    /* normalize individual velocities so that there are no bulk momenta  */

    XMAS[1]=OMAS;
    for (i=0; i<BOX_PER_SIDE; i++) {
        for (j=0; j<BOX_PER_SIDE; j++) {
            for (k=0; k<BOX_PER_SIDE; k++) {
                curr_ptr = BOX[i][j][k].list;
                while (curr_ptr) {
                    for (atom = 0; atom < NATOMS; atom++) {
                        curr_ptr->mol.F[VEL][XDIR][atom] =
                            ( curr_ptr->mol.F[VEL][XDIR][atom] -
                             SUMX) * SUX/XMAS[atom];
                        curr_ptr->mol.F[VEL][YDIR][atom] =
                            ( curr_ptr->mol.F[VEL][YDIR][atom] -
                             SUMY) * SUY/XMAS[atom];
                        curr_ptr->mol.F[VEL][ZDIR][atom] =
                            ( curr_ptr->mol.F[VEL][ZDIR][atom] -
                             SUMZ) * SUZ/XMAS[atom];
                    } /* for atom */
                    curr_ptr = curr_ptr->next_mol;
                } /* while curr_ptr */
            }
        }
    } /* for box */

    fclose(random_numbers);
} /* end of subroutine INITIA */

/*
 * XRAND: generate floating-point random number.
 */

double xrand(double xl, double xh)	/* lower, upper bounds on number */
{
    double x;

    x=(xl + (xh - xl) * ((double) rand()) / 2147483647.0);
    return (x);
}

