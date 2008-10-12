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

/*
 * CODE.H: define various global things for CODE.C.
 */

#ifndef _CODE_H_
#define _CODE_H_

#define PAD_SIZE (PAGE_SIZE / (sizeof(long)))

/* Defined by the input file */
global string headline; 	/* message describing calculation */
global string infile; 		/* file name for snapshot input */
global string outfile; 		/* file name for snapshot output */
global real dtime; 		/* timestep for leapfrog integrator */
global real dtout; 		/* time between data outputs */
global real tstop; 		/* time to stop calculation */
global long nbody; 		/* number of bodies in system */
global real fcells; 		/* ratio of cells/leaves allocated */
global real fleaves; 		/* ratio of leaves/bodies allocated */
global real tol; 		/* accuracy parameter: 0.0 => exact */
global real tolsq; 		/* square of previous */
global real eps; 		/* potential softening parameter */
global real epssq; 		/* square of previous */
global real dthf; 		/* half time step */
global long NPROC;		/* Number of Processors */

global long maxcell;		/* max number of cells allocated */
global long maxleaf;		/* max number of leaves allocated */
global long maxmybody;		/* max no. of bodies allocated per processor */
global long maxmycell;		/* max num. of cells to be allocated */
global long maxmyleaf;		/* max num. of leaves to be allocated */
global bodyptr bodytab; 	/* array size is exactly nbody bodies */

global struct CellLockType {
    ALOCKDEC(CL,MAXLOCK)        /* locks on the cells*/
} *CellLock;

struct GlobalMemory  {	/* all this info is for the whole system */
    long n2bcalc;       /* total number of body/cell interactions  */
    long nbccalc;       /* total number of body/body interactions  */
    long selfint;       /* number of self interactions             */
    real mtot;         /* total mass of N-body system             */
    real etot[3];      /* binding, kinetic, potential energy      */
    matrix keten;      /* kinetic energy tensor                   */
    matrix peten;      /* potential energy tensor                 */
    vector cmphase[2]; /* center of mass coordinates and velocity */
    vector amvec;      /* angular momentum vector                 */
    cellptr G_root;    /* root of the whole tree                  */
    vector rmin;       /* lower-left corner of coordinate box     */
    vector min;        /* temporary lower-left corner of the box  */
    vector max;        /* temporary upper right corner of the box */
    real rsize;        /* side-length of integer coordinate box   */
    BARDEC(Barrier)   /* barrier at the beginning of stepsystem  */
    LOCKDEC(CountLock) /* Lock on the shared variables            */
    LOCKDEC(NcellLock) /* Lock on the counter of array of cells for loadtree */
    LOCKDEC(NleafLock)/* Lock on the counter of array of leaves for loadtree */
    LOCKDEC(io_lock)
    unsigned long createstart,createend,computestart,computeend;
    unsigned long trackstart, trackend, tracktime;
    unsigned long partitionstart, partitionend, partitiontime;
    unsigned long treebuildstart, treebuildend, treebuildtime;
    unsigned long forcecalcstart, forcecalcend, forcecalctime;
    long current_id;
    volatile long k; /*for memory allocation in code.C */
};
global struct GlobalMemory *Global;

/* This structure is needed because under the sproc model there is no
 * per processor private address space.
 */
struct local_memory {
   /* Use padding so that each processor's variables are on their own page */
   long pad_begin[PAD_SIZE];

   real tnow;        	/* current value of simulation time */
   real tout;         	/* time next output is due */
   long nstep;      	/* number of integration steps so far */

   long workMin, workMax;/* interval of cost to be treated by a proc */

   vector min, max; 	/* min and max of coordinates for each Proc. */

   long mynumcell; 	/* num. of cells used for this proc in ctab */
   long mynumleaf; 	/* num. of leaves used for this proc in ctab */
   long mynbody;   	/* num bodies allocated to the processor */
   bodyptr* mybodytab;	/* array of bodies allocated / processor */
   long myncell; 	/* num cells allocated to the processor */
   cellptr* mycelltab;	/* array of cellptrs allocated to the processor */
   long mynleaf; 	/* number of leaves allocated to the processor */
   leafptr* myleaftab; 	/* array of leafptrs allocated to the processor */
   cellptr ctab;	/* array of cells used for the tree. */
   leafptr ltab;	/* array of cells used for the tree. */

   long myn2bcalc; 	/* body-body force calculations for each processor */
   long mynbccalc; 	/* body-cell force calculations for each processor */
   long myselfint; 	/* count self-interactions for each processor */
   long myn2bterm; 	/* count body-body terms for a body */
   long mynbcterm; 	/* count body-cell terms for a body */
   bool skipself; 	/* true if self-interaction skipped OK */
   bodyptr pskip;       /* body to skip in force evaluation */
   vector pos0;         /* point at which to evaluate field */
   real phi0;           /* computed potential at pos0 */
   vector acc0;         /* computed acceleration at pos0 */
   vector dr;  		/* data to be shared */
   real drsq;      	/* between gravsub and subdivp */
   nodeptr pmem;	/* remember particle data */

   nodeptr Current_Root;
   long Root_Coords[NDIM];

   real mymtot;      	/* total mass of N-body system */
   real myetot[3];   	/* binding, kinetic, potential energy */
   matrix myketen;   	/* kinetic energy tensor */
   matrix mypeten;   	/* potential energy tensor */
   vector mycmphase[2];	/* center of mass coordinates */
   vector myamvec;   	/* angular momentum vector */

   long pad_end[PAD_SIZE];
};
global struct local_memory Local[MAX_PROC];

void SlaveStart(void);
void stepsystem(long ProcessId);
void ComputeForces(long ProcessId);
void Help(void);
void ANLinit(void);
void init_root(void);
void tab_init(void);
void startrun(void);
void testdata(void);
void pickshell(real vec[], real rad);
void find_my_initial_bodies(bodyptr btab, long nbody, long ProcessId);
void find_my_bodies(nodeptr mycell, long work, long direction, long ProcessId);
void Housekeep(long ProcessId);
void setbound(void);
long  Log_base_2(long number);
long  intpow(long i, long j);

#endif
