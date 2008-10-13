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
Usage: BARNES <options> < inputfile

Command line options:

    -h : Print out input file description

    Input parameters should be placed in a file and redirected through
    standard input.  There are a total of twelve parameters, and all of
    them have default values.

    1) infile (char*) : The name of an input file that contains particle
       data.

       The format of the file is:
         a) An int representing the number of particles in the distribution
         b) An int representing the dimensionality of the problem (3-D)
         c) A double representing the current time of the simulation
         d) Doubles representing the masses of all the particles
         e) A vector (length equal to the dimensionality) of doubles
            representing the positions of all the particles
         f) A vector (length equal to the dimensionality) of doubles
            representing the velocities of all the particles

       Each of these numbers can be separated by any amount of whitespace.
    2) nbody (int) : If no input file is specified (the first line is
       blank), this number specifies the number of particles to generate
       under a plummer model.  Default is 16384.
    3) seed (int) : The seed used by the random number generator.
       Default is 123.
    4) outfile (char*) : The name of the file that snapshots will be
       printed to. This feature has been disabled in the SPLASH release.
       Default is NULL.
    5) dtime (double) : The integration time-step.
       Default is 0.025.
    6) eps (double) : The usual potential softening
       Default is 0.05.
    7) tol (double) : The cell subdivision tolerance.
       Default is 1.0.
    8) fcells (double) : Number of cells created = fcells * number of
       leaves.
       Default is 2.0.
    9) fleaves (double) : Number of leaves created = fleaves * nbody.
       Default is 0.5.
    10) tstop (double) : The time to stop integration.
       Default is 0.075.
    11) dtout (double) : The data-output interval.
       Default is 0.25.
    12) NPROC (int) : The number of processors.
       Default is 1.
*/

MAIN_ENV

#define global  /* nada */

#include "stdinc.h"

string defv[] = {                 /* DEFAULT PARAMETER VALUES              */
    /* file names for input/output                                         */
    "in=",                        /* snapshot of initial conditions        */
    "out=",                       /* stream of output snapshots            */

    /* params, used if no input specified, to make a Plummer Model         */
    "nbody=16384",                /* number of particles to generate       */
    "seed=123",                   /* random number generator seed          */

    /* params to control N-body integration                                */
    "dtime=0.025",                /* integration time-step                 */
    "eps=0.05",                   /* usual potential softening             */
    "tol=1.0",                    /* cell subdivision tolerence            */
    "fcells=2.0",                 /* cell allocation parameter             */
    "fleaves=0.5",                 /* leaf allocation parameter             */

    "tstop=0.075",                 /* time to stop integration              */
    "dtout=0.25",                 /* data-output interval                  */

    "NPROC=1",                    /* number of processors                  */
};

/* The more complicated 3D case */
#define NUM_DIRECTIONS 32
#define BRC_FUC 0
#define BRC_FRA 1
#define BRA_FDA 2
#define BRA_FRC 3
#define BLC_FDC 4
#define BLC_FLA 5
#define BLA_FUA 6
#define BLA_FLC 7
#define BUC_FUA 8
#define BUC_FLC 9
#define BUA_FUC 10
#define BUA_FRA 11
#define BDC_FDA 12
#define BDC_FRC 13
#define BDA_FDC 14
#define BDA_FLA 15

#define FRC_BUC 16
#define FRC_BRA 17
#define FRA_BDA 18
#define FRA_BRC 19
#define FLC_BDC 20
#define FLC_BLA 21
#define FLA_BUA 22
#define FLA_BLC 23
#define FUC_BUA 24
#define FUC_BLC 25
#define FUA_BUC 26
#define FUA_BRA 27
#define FDC_BDA 28
#define FDC_BRC 29
#define FDA_BDC 30
#define FDA_BLA 31

static long Child_Sequence[NUM_DIRECTIONS][NSUB] =
{
	{ 2, 5, 6, 1, 0, 3, 4, 7},  /* BRC_FUC */
 { 2, 5, 6, 1, 0, 7, 4, 3},  /* BRC_FRA */
 { 1, 6, 5, 2, 3, 0, 7, 4},  /* BRA_FDA */
 { 1, 6, 5, 2, 3, 4, 7, 0},  /* BRA_FRC */
 { 6, 1, 2, 5, 4, 7, 0, 3},  /* BLC_FDC */
 { 6, 1, 2, 5, 4, 3, 0, 7},  /* BLC_FLA */
 { 5, 2, 1, 6, 7, 4, 3, 0},  /* BLA_FUA */
 { 5, 2, 1, 6, 7, 0, 3, 4},  /* BLA_FLC */
 { 1, 2, 5, 6, 7, 4, 3, 0},  /* BUC_FUA */
 { 1, 2, 5, 6, 7, 0, 3, 4},  /* BUC_FLC */
 { 6, 5, 2, 1, 0, 3, 4, 7},  /* BUA_FUC */
 { 6, 5, 2, 1, 0, 7, 4, 3},  /* BUA_FRA */
 { 5, 6, 1, 2, 3, 0, 7, 4},  /* BDC_FDA */
 { 5, 6, 1, 2, 3, 4, 7, 0},  /* BDC_FRC */
 { 2, 1, 6, 5, 4, 7, 0, 3},  /* BDA_FDC */
 { 2, 1, 6, 5, 4, 3, 0, 7},  /* BDA_FLA */

 { 3, 4, 7, 0, 1, 2, 5, 6},  /* FRC_BUC */
 { 3, 4, 7, 0, 1, 6, 5, 2},  /* FRC_BRA */
 { 0, 7, 4, 3, 2, 1, 6, 5},  /* FRA_BDA */
 { 0, 7, 4, 3, 2, 5, 6, 1},  /* FRA_BRC */
 { 7, 0, 3, 4, 5, 6, 1, 2},  /* FLC_BDC */
 { 7, 0, 3, 4, 5, 2, 1, 6},  /* FLC_BLA */
 { 4, 3, 0, 7, 6, 5, 2, 1},  /* FLA_BUA */
 { 4, 3, 0, 7, 6, 1, 2, 5},  /* FLA_BLC */
 { 0, 3, 4, 7, 6, 5, 2, 1},  /* FUC_BUA */
 { 0, 3, 4, 7, 6, 1, 2, 5},  /* FUC_BLC */
 { 7, 4, 3, 0, 1, 2, 5, 6},  /* FUA_BUC */
 { 7, 4, 3, 0, 1, 6, 5, 2},  /* FUA_BRA */
 { 4, 7, 0, 3, 2, 1, 6, 5},  /* FDC_BDA */
 { 4, 7, 0, 3, 2, 5, 6, 1},  /* FDC_BRC */
 { 3, 0, 7, 4, 5, 6, 1, 2},  /* FDA_BDC */
 { 3, 0, 7, 4, 5, 2, 1, 6},  /* FDA_BLA */
};

static long Direction_Sequence[NUM_DIRECTIONS][NSUB] =
{
	{ FRC_BUC, BRA_FRC, FDA_BDC, BLA_FUA, BUC_FLC, FUA_BUC, BRA_FRC, FDA_BLA },
 /* BRC_FUC */
 { FRC_BUC, BRA_FRC, FDA_BDC, BLA_FUA, BRA_FDA, FRC_BRA, BUC_FUA, FLC_BDC },
 /* BRC_FRA */
 { FRA_BDA, BRC_FRA, FUC_BUA, BLC_FDC, BDA_FLA, FDC_BDA, BRC_FRA, FUC_BLC },
 /* BRA_FDA */
 { FRA_BDA, BRC_FRA, FUC_BUA, BLC_FDC, BUC_FLC, FUA_BUC, BRA_FRC, FDA_BLA },
 /* BRA_FRC */
 { FLC_BDC, BLA_FLC, FUA_BUC, BRA_FDA, BDC_FRC, FDA_BDC, BLA_FLC, FUA_BRA },
 /* BLC_FDC */
 { FLC_BDC, BLA_FLC, FUA_BUC, BRA_FDA, BLA_FUA, FLC_BLA, BDC_FDA, FRC_BUC },
 /* BLC_FLA */
 { FLA_BUA, BLC_FLA, FDC_BDA, BRC_FUC, BUA_FRA, FUC_BUA, BLC_FLA, FDC_BRC },
 /* BLA_FUA */
 { FLA_BUA, BLC_FLA, FDC_BDA, BRC_FUC, BLC_FDC, FLA_BLC, BUA_FUC, FRA_BDA },
 /* BLA_FLC */
 { FUC_BLC, BUA_FUC, FRA_BRC, BDA_FLA, BUA_FRA, FUC_BUA, BLC_FLA, FDC_BRC },
 /* BUC_FUA */
 { FUC_BLC, BUA_FUC, FRA_BRC, BDA_FLA, BLC_FDC, FLA_BLC, BUA_FUC, FRA_BDA },
 /* BUC_FLC */
 { FUA_BRA, BUC_FUA, FLC_BLA, BDC_FRC, BUC_FLC, FUA_BUC, BRA_FRC, FDA_BLA },
 /* BUA_FUC */
 { FUA_BRA, BUC_FUA, FLC_BLA, BDC_FRC, BRA_FDA, FRC_BRA, BUC_FUA, FLC_BDC },
 /* BUA_FRA */
 { FDC_BRC, BDA_FDC, FLA_BLC, BUA_FRA, BDA_FLA, FDC_BDA, BRC_FRA, FUC_BLC },
 /* BDC_FDA */
 { FDC_BRC, BDA_FDC, FLA_BLC, BUA_FRA, BUC_FLC, FUA_BUC, BRA_FRC, FDA_BLA },
 /* BDC_FRC */
 { FDA_BLA, BDC_FDA, FRC_BRA, BUC_FLC, BDC_FRC, FDA_BDC, BLA_FLC, FUA_BRA },
 /* BDA_FDC */
 { FDA_BLA, BDC_FDA, FRC_BRA, BUC_FLC, BLA_FUA, FLC_BLA, BDC_FDA, FRC_BUC },
 /* BDA_FLA */

 { BUC_FLC, FUA_BUC, BRA_FRC, FDA_BLA, FUC_BLC, BUA_FUC, FRA_BRC, BDA_FLA },
 /* FRC_BUC */
 { BUC_FLC, FUA_BUC, BRA_FRC, FDA_BLA, FRA_BDA, BRC_FRA, FUC_BUA, BLC_FDC },
 /* FRC_BRA */
 { BRA_FDA, FRC_BRA, BUC_FUA, FLC_BDC, FDA_BLA, BDC_FDA, FRC_BRA, BUC_FLC },
 /* FRA_BDA */
 { BRA_FDA, FRC_BRA, BUC_FUA, FLC_BDC, FRC_BUC, BRA_FRC, FDA_BDC, BLA_FUA },
 /* FRA_BRC */
 { BLC_FDC, FLA_BLC, BUA_FUC, FRA_BDA, FDC_BRC, BDA_FDC, FLA_BLC, BUA_FRA },
 /* FLC_BDC */
 { BLC_FDC, FLA_BLC, BUA_FUC, FRA_BDA, FLA_BUA, BLC_FLA, FDC_BDA, BRC_FUC },
 /* FLC_BLA */
 { BLA_FUA, FLC_BLA, BDC_FDA, FRC_BUC, FUA_BRA, BUC_FUA, FLC_BLA, BDC_FRC },
 /* FLA_BUA */
 { BLA_FUA, FLC_BLA, BDC_FDA, FRC_BUC, FLC_BDC, BLA_FLC, FUA_BUC, BRA_FDA },
 /* FLA_BLC */
 { BUC_FLC, FUA_BUC, BRA_FRC, FDA_BLA, FUA_BRA, BUC_FUA, FLC_BLA, BDC_FRC },
 /* FUC_BUA */
 { BUC_FLC, FUA_BUC, BRA_FRC, FDA_BLA, FLC_BDC, BLA_FLC, FUA_BUC, BRA_FDA },
 /* FUC_BLC */
 { BUA_FRA, FUC_BUA, BLC_FLA, FDC_BRC, FUC_BLC, BUA_FUC, FRA_BRC, BDA_FLA },
 /* FUA_BUC */
 { BUA_FRA, FUC_BUA, BLC_FLA, FDC_BRC, FRA_BDA, BRC_FRA, FUC_BUA, BLC_FDC },
 /* FUA_BRA */
 { BDC_FRC, FDA_BDC, BLA_FLC, FUA_BRA, FDA_BLA, BDC_FDA, FRC_BRA, BUC_FLC },
 /* FDC_BDA */
 { BDC_FRC, FDA_BDC, BLA_FLC, FUA_BRA, FRC_BUC, BRA_FRC, FDA_BDC, BLA_FUA },
 /* FDC_BRC */
 { BDA_FLA, FDC_BDA, BRC_FRA, FUC_BLC, FDC_BRC, BDA_FDC, FLA_BLC, BUA_FRA },
 /* FDA_BDC */
 { BDA_FLA, FDC_BDA, BRC_FRA, FUC_BLC, FLA_BUA, BLC_FLA, FDC_BDA, BRC_FUC },
 /* FDA_BLA */
};

int main (int argc, string argv[])
{
   long c;

   while ((c = getopt(argc, argv, "h")) != -1) {
     switch(c) {
      case 'h':
	Help();
	exit(-1);
	break;
      default:
	fprintf(stderr, "Only valid option is \"-h\".\n");
	exit(-1);
	break;
     }
   }

   Global = NULL;
   initparam(defv);
   startrun();
   initoutput();
   tab_init();

   Global->tracktime = 0;
   Global->partitiontime = 0;
   Global->treebuildtime = 0;
   Global->forcecalctime = 0;
   Global->current_id = 0;

   CLOCK(Global->computestart);

   printf("COMPUTESTART  = %12lu\n",Global->computestart);

   CREATE(SlaveStart, NPROC);

   WAIT_FOR_END(NPROC);

   CLOCK(Global->computeend);

   printf("COMPUTEEND    = %12lu\n",Global->computeend);
   printf("COMPUTETIME   = %12lu\n",Global->computeend - Global->computestart);
   printf("TRACKTIME     = %12lu\n",Global->tracktime);
   printf("PARTITIONTIME = %12lu\t%5.2f\n",Global->partitiontime,
	  ((float)Global->partitiontime)/Global->tracktime);
   printf("TREEBUILDTIME = %12lu\t%5.2f\n",Global->treebuildtime,
	  ((float)Global->treebuildtime)/Global->tracktime);
   printf("FORCECALCTIME = %12lu\t%5.2f\n",Global->forcecalctime,
	  ((float)Global->forcecalctime)/Global->tracktime);
   printf("RESTTIME      = %12lu\t%5.2f\n",
	  Global->tracktime - Global->partitiontime -
	  Global->treebuildtime - Global->forcecalctime,
	  ((float)(Global->tracktime-Global->partitiontime-
		   Global->treebuildtime-Global->forcecalctime))/
	  Global->tracktime);
   MAIN_END;
}

/*
 * ANLINIT : initialize ANL macros
 */
void ANLinit()
{
   MAIN_INITENV(,70000000,);
   /* Allocate global, shared memory */

   Global = (struct GlobalMemory *) G_MALLOC(sizeof(struct GlobalMemory));
   if (Global==NULL) error("No initialization for Global\n");

   BARINIT(Global->Barrier, NPROC);

   LOCKINIT(Global->CountLock);
   LOCKINIT(Global->io_lock);
}

/*
 * INIT_ROOT: Processor 0 reinitialize the global root at each time step
 */
void init_root()
{
   long i;

   Global->G_root=Local[0].ctab;
   Global->G_root->seqnum = 0;
   Type(Global->G_root) = CELL;
   Done(Global->G_root) = FALSE;
   Level(Global->G_root) = IMAX >> 1;
   for (i = 0; i < NSUB; i++) {
      Subp(Global->G_root)[i] = NULL;
   }
   Local[0].mynumcell=1;
}

long Log_base_2(long number)
{
   long cumulative;
   long out;

   cumulative = 1;
   for (out = 0; out < 20; out++) {
      if (cumulative == number) {
         return(out);
      }
      else {
         cumulative = cumulative * 2;
      }
   }

   fprintf(stderr,"Log_base_2: couldn't find log2 of %ld\n", number);
   exit(-1);
}

/*
 * TAB_INIT : allocate body and cell data space
 */

void tab_init()
{
   long i;

   /*allocate leaf/cell space */
   maxleaf = (long) ((double) fleaves * nbody);
   maxcell = fcells * maxleaf;
   for (i = 0; i < NPROC; ++i) {
      Local[i].ctab = (cellptr) G_MALLOC((maxcell / NPROC) * sizeof(cell));
      Local[i].ltab = (leafptr) G_MALLOC((maxleaf / NPROC) * sizeof(leaf));
   }

   /*allocate space for personal lists of body pointers */
   maxmybody = (nbody+maxleaf*MAX_BODIES_PER_LEAF)/NPROC;
   Local[0].mybodytab = (bodyptr*) G_MALLOC(NPROC*maxmybody*sizeof(bodyptr));
   /* space is allocated so that every */
   /* process can have a maximum of maxmybody pointers to bodies */
   /* then there is an array of bodies called bodytab which is  */
   /* allocated in the distribution generation or when the distr. */
   /* file is read */
   maxmycell = maxcell / NPROC;
   maxmyleaf = maxleaf / NPROC;
   Local[0].mycelltab = (cellptr*) G_MALLOC(NPROC*maxmycell*sizeof(cellptr));
   Local[0].myleaftab = (leafptr*) G_MALLOC(NPROC*maxmyleaf*sizeof(leafptr));

   CellLock = (struct CellLockType *) G_MALLOC(sizeof(struct CellLockType));
   ALOCKINIT(CellLock->CL,MAXLOCK);
}

/*
 * SLAVESTART: main task for each processor
 */
void SlaveStart()
{
   long ProcessId;

   /* Get unique ProcessId */
   LOCK(Global->CountLock);
     ProcessId = Global->current_id++;
   UNLOCK(Global->CountLock);

   BARINCLUDE(Global->Barrier);

/* POSSIBLE ENHANCEMENT:  Here is where one might pin processes to
   processors to avoid migration */

   /* initialize mybodytabs */
   Local[ProcessId].mybodytab = Local[0].mybodytab + (maxmybody * ProcessId);
   /* note that every process has its own copy   */
   /* of mybodytab, which was initialized to the */
   /* beginning of the whole array by proc. 0    */
   /* before create                              */
   Local[ProcessId].mycelltab = Local[0].mycelltab + (maxmycell * ProcessId);
   Local[ProcessId].myleaftab = Local[0].myleaftab + (maxmyleaf * ProcessId);
/* POSSIBLE ENHANCEMENT:  Here is where one might distribute the
   data across physically distributed memories as desired.

   One way to do this is as follows:

   long i;

   if (ProcessId == 0) {
     for (i=0;i<NPROC;i++) {
       Place all addresses x such that
         &(Local[i]) <= x < &(Local[i])+
           sizeof(struct local_memory) on node i
       Place all addresses x such that
         &(Local[i].mybodytab) <= x < &(Local[i].mybodytab)+
           maxmybody * sizeof(bodyptr) - 1 on node i
       Place all addresses x such that
         &(Local[i].mycelltab) <= x < &(Local[i].mycelltab)+
           maxmycell * sizeof(cellptr) - 1 on node i
       Place all addresses x such that
         &(Local[i].myleaftab) <= x < &(Local[i].myleaftab)+
           maxmyleaf * sizeof(leafptr) - 1 on node i
     }
   }

   barrier(Global->Barstart,NPROC);

*/

   Local[ProcessId].tout = Local[0].tout;
   Local[ProcessId].tnow = Local[0].tnow;
   Local[ProcessId].nstep = Local[0].nstep;

   find_my_initial_bodies(bodytab, nbody, ProcessId);

   /* main loop */
   while (Local[ProcessId].tnow < tstop + 0.1 * dtime) {
      stepsystem(ProcessId);
//      printtree(Global->G_root);
//      printf("Going to next step!!!\n");
   }
}


/*
 * STARTRUN: startup hierarchical N-body code.
 */

void startrun()
{
   long seed;

   infile = getparam("in");
   if (*infile != '\0'/*NULL*/) {
      inputdata();
   }
   else {
      nbody = getiparam("nbody");
      if (nbody < 1) {
	 error("startrun: absurd nbody\n");
      }
      seed = getiparam("seed");
   }

   outfile = getparam("out");
   dtime = getdparam("dtime");
   dthf = 0.5 * dtime;
   eps = getdparam("eps");
   epssq = eps*eps;
   tol = getdparam("tol");
   tolsq = tol*tol;
   fcells = getdparam("fcells");
   fleaves = getdparam("fleaves");
   tstop = getdparam("tstop");
   dtout = getdparam("dtout");
   NPROC = getiparam("NPROC");
   Local[0].nstep = 0;
   pranset(seed);
   testdata();
   ANLinit();
   setbound();
   Local[0].tout = Local[0].tnow + dtout;
}

/*
 * TESTDATA: generate Plummer model initial conditions for test runs,
 * scaled to units such that M = -4E = G = 1 (Henon, Hegge, etc).
 * See Aarseth, SJ, Henon, M, & Wielen, R (1974) Astr & Ap, 37, 183.
 */

#define MFRAC  0.999                /* mass cut off at MFRAC of total */

void testdata()
{
   real rsc, vsc, r, v, x, y;
   vector cmr, cmv;
   register bodyptr p;
   long rejects = 0;
   long halfnbody, i;
   float offset;
   register bodyptr cp;

   headline = "Hack code: Plummer model";
   Local[0].tnow = 0.0;
   bodytab = (bodyptr) G_MALLOC(nbody * sizeof(body));
   if (bodytab == NULL) {
      error("testdata: not enough memory\n");
   }
   rsc = 9 * PI / 16;
   vsc = sqrt(1.0 / rsc);

   CLRV(cmr);
   CLRV(cmv);

   halfnbody = nbody / 2;
   if (nbody % 2 != 0) halfnbody++;
   for (p = bodytab; p < bodytab+halfnbody; p++) {
      Type(p) = BODY;
      Mass(p) = 1.0 / nbody;
      Cost(p) = 1;

      r = 1 / sqrt(pow(xrand(0.0, MFRAC), -2.0/3.0) - 1);
      /*   reject radii greater than 10 */
      while (r > 9.0) {
	 rejects++;
	 r = 1 / sqrt(pow(xrand(0.0, MFRAC), -2.0/3.0) - 1);
      }
      pickshell(Pos(p), rsc * r);
      ADDV(cmr, cmr, Pos(p));
      do {
	 x = xrand(0.0, 1.0);
	 y = xrand(0.0, 0.1);

      } while (y > x*x * pow(1 - x*x, 3.5));

      v = sqrt(2.0) * x / pow(1 + r*r, 0.25);
      pickshell(Vel(p), vsc * v);
      ADDV(cmv, cmv, Vel(p));
   }

   offset = 4.0;

   for (p = bodytab + halfnbody; p < bodytab+nbody; p++) {
      Type(p) = BODY;
      Mass(p) = 1.0 / nbody;
      Cost(p) = 1;

      cp = p - halfnbody;
      for (i = 0; i < NDIM; i++){
	 Pos(p)[i] = Pos(cp)[i] + offset;
	 Vel(p)[i] = Vel(cp)[i];
      }
      ADDV(cmr, cmr, Pos(p));
      ADDV(cmv, cmv, Vel(p));
   }

   DIVVS(cmr, cmr, (real) nbody);
   DIVVS(cmv, cmv, (real) nbody);

   for (p = bodytab; p < bodytab+nbody; p++) {
      SUBV(Pos(p), Pos(p), cmr);
      SUBV(Vel(p), Vel(p), cmv);
   }
}

/*
 * PICKSHELL: pick a random point on a sphere of specified radius.
 */

void pickshell(real vec[], real rad)
{
   register long k;
   double rsq, rsc;

   do {
      for (k = 0; k < NDIM; k++) {
	 vec[k] = xrand(-1.0, 1.0);
      }
      DOTVP(rsq, vec, vec);
   } while (rsq > 1.0);

   rsc = rad / sqrt(rsq);
   MULVS(vec, vec, rsc);
}



long intpow(long i, long j)
{
    long k;
    long temp = 1;

    for (k = 0; k < j; k++)
        temp = temp*i;
    return temp;
}


/*
 * STEPSYSTEM: advance N-body system one time-step.
 */

void stepsystem(long ProcessId)
{
    long i;
    real Cavg;
    bodyptr p,*pp;
    vector dvel, vel1, dpos;
    long trackstart, trackend;
    long partitionstart, partitionend;
    long treebuildstart, treebuildend;
    long forcecalcstart, forcecalcend;

    if (Local[ProcessId].nstep == 2) {
/* POSSIBLE ENHANCEMENT:  Here is where one might reset the
   statistics that one is measuring about the parallel execution */
    }

    if ((ProcessId == 0) && (Local[ProcessId].nstep >= 2)) {
        CLOCK(trackstart);
    }

    if (ProcessId == 0) {
       init_root();
    }
    else {
       Local[ProcessId].mynumcell = 0;
       Local[ProcessId].mynumleaf = 0;
    }


    /* start at same time */
    BARRIER(Global->Barrier,NPROC);

    if ((ProcessId == 0) && (Local[ProcessId].nstep >= 2)) {
        CLOCK(treebuildstart);
    }

    /* load bodies into tree   */
    maketree(ProcessId);
    if ((ProcessId == 0) && (Local[ProcessId].nstep >= 2)) {
        CLOCK(treebuildend);
        Global->treebuildtime += treebuildend - treebuildstart;
    }

    Housekeep(ProcessId);

    Cavg = (real) Cost(Global->G_root) / (real)NPROC ;
    Local[ProcessId].workMin = (long) (Cavg * ProcessId);
    Local[ProcessId].workMax = (long) (Cavg * (ProcessId + 1)
				      + (ProcessId == (NPROC - 1)));

    if ((ProcessId == 0) && (Local[ProcessId].nstep >= 2)) {
        CLOCK(partitionstart);
    }

    Local[ProcessId].mynbody = 0;
    find_my_bodies(Global->G_root, 0, BRC_FUC, ProcessId );

/*     B*RRIER(Global->Barcom,NPROC); */
    if ((ProcessId == 0) && (Local[ProcessId].nstep >= 2)) {
        CLOCK(partitionend);
        Global->partitiontime += partitionend - partitionstart;
    }

    if ((ProcessId == 0) && (Local[ProcessId].nstep >= 2)) {
        CLOCK(forcecalcstart);
    }

    ComputeForces(ProcessId);

    if ((ProcessId == 0) && (Local[ProcessId].nstep >= 2)) {
        CLOCK(forcecalcend);
        Global->forcecalctime += forcecalcend - forcecalcstart;
    }

    /* advance my bodies */
    for (pp = Local[ProcessId].mybodytab;
	 pp < Local[ProcessId].mybodytab+Local[ProcessId].mynbody; pp++) {
       p = *pp;
       MULVS(dvel, Acc(p), dthf);
       ADDV(vel1, Vel(p), dvel);
       MULVS(dpos, vel1, dtime);
       ADDV(Pos(p), Pos(p), dpos);
       ADDV(Vel(p), vel1, dvel);

       for (i = 0; i < NDIM; i++) {
          if (Pos(p)[i]<Local[ProcessId].min[i]) {
	     Local[ProcessId].min[i]=Pos(p)[i];
	  }
          if (Pos(p)[i]>Local[ProcessId].max[i]) {
	     Local[ProcessId].max[i]=Pos(p)[i] ;
	  }
       }
    }
    LOCK(Global->CountLock);
    for (i = 0; i < NDIM; i++) {
       if (Global->min[i] > Local[ProcessId].min[i]) {
	  Global->min[i] = Local[ProcessId].min[i];
       }
       if (Global->max[i] < Local[ProcessId].max[i]) {
	  Global->max[i] = Local[ProcessId].max[i];
       }
    }
    UNLOCK(Global->CountLock);

    /* bar needed to make sure that every process has computed its min */
    /* and max coordinates, and has accumulated them into the global   */
    /* min and max, before the new dimensions are computed	       */
    BARRIER(Global->Barrier,NPROC);

    if ((ProcessId == 0) && (Local[ProcessId].nstep >= 2)) {
        CLOCK(trackend);
        Global->tracktime += trackend - trackstart;
    }
    if (ProcessId==0) {
      Global->rsize=0;
      SUBV(Global->max,Global->max,Global->min);
      for (i = 0; i < NDIM; i++) {
	if (Global->rsize < Global->max[i]) {
	   Global->rsize = Global->max[i];
	}
      }
      ADDVS(Global->rmin,Global->min,-Global->rsize/100000.0);
      Global->rsize = 1.00002*Global->rsize;
      SETVS(Global->min,1E99);
      SETVS(Global->max,-1E99);
    }
    Local[ProcessId].nstep++;
    Local[ProcessId].tnow = Local[ProcessId].tnow + dtime;
}



void ComputeForces(long ProcessId)
{
   bodyptr p,*pp;
   vector acc1, dacc, dvel;

   for (pp = Local[ProcessId].mybodytab;
	pp < Local[ProcessId].mybodytab+Local[ProcessId].mynbody;pp++) {
      p = *pp;
      SETV(acc1, Acc(p));
      Cost(p)=0;
      hackgrav(p,ProcessId);
      Local[ProcessId].myn2bcalc += Local[ProcessId].myn2bterm;
      Local[ProcessId].mynbccalc += Local[ProcessId].mynbcterm;
      if (!Local[ProcessId].skipself) {       /*   did we miss self-int?  */
	 Local[ProcessId].myselfint++;        /*   count another goofup   */
      }
      if (Local[ProcessId].nstep > 0) {
	 /*   use change in accel to make 2nd order correction to vel      */
	 SUBV(dacc, Acc(p), acc1);
	 MULVS(dvel, dacc, dthf);
	 ADDV(Vel(p), Vel(p), dvel);
      }
   }
}

/*
 * FIND_MY_INITIAL_BODIES: puts into mybodytab the initial list of bodies
 * assigned to the processor.
 */

void find_my_initial_bodies(bodyptr btab, long nbody, long ProcessId)
{
  long extra,offset,i;

  Local[ProcessId].mynbody = nbody / NPROC;
  extra = nbody % NPROC;
  if (ProcessId < extra) {
    Local[ProcessId].mynbody++;
    offset = Local[ProcessId].mynbody * ProcessId;
  }
  if (ProcessId >= extra) {
    offset = (Local[ProcessId].mynbody+1) * extra + (ProcessId - extra)
       * Local[ProcessId].mynbody;
  }
  for (i=0; i < Local[ProcessId].mynbody; i++) {
     Local[ProcessId].mybodytab[i] = &(btab[offset+i]);
  }
  BARRIER(Global->Barrier,NPROC);
}


void find_my_bodies(nodeptr mycell, long work, long direction, long ProcessId)
{
   long i;
   leafptr l;
   nodeptr qptr;

   if (Type(mycell) == LEAF) {
      l = (leafptr) mycell;
      for (i = 0; i < l->num_bodies; i++) {
	 if (work >= Local[ProcessId].workMin - .1) {
	    if((Local[ProcessId].mynbody+2) > maxmybody) {
	       error("find_my_bodies: Processor %ld needs more than %ld bodies; increase fleaves\n", ProcessId, maxmybody);
	    }
	    Local[ProcessId].mybodytab[Local[ProcessId].mynbody++] =
	       Bodyp(l)[i];
	 }
	 work += Cost(Bodyp(l)[i]);
	 if (work >= Local[ProcessId].workMax-.1) {
	    break;
	 }
      }
   }
   else {
      for(i = 0; (i < NSUB) && (work < (Local[ProcessId].workMax - .1)); i++){
	 qptr = Subp(mycell)[Child_Sequence[direction][i]];
	 if (qptr!=NULL) {
	    if ((work+Cost(qptr)) >= (Local[ProcessId].workMin -.1)) {
	       find_my_bodies(qptr,work, Direction_Sequence[direction][i],
			      ProcessId);
	    }
	    work += Cost(qptr);
	 }
      }
   }
}

/*
 * HOUSEKEEP: reinitialize the different variables (in particular global
 * variables) between each time step.
 */

void Housekeep(long ProcessId)
{
   Local[ProcessId].myn2bcalc = Local[ProcessId].mynbccalc
      = Local[ProcessId].myselfint = 0;
   SETVS(Local[ProcessId].min,1E99);
   SETVS(Local[ProcessId].max,-1E99);
}

/*
 * SETBOUND: Compute the initial size of the root of the tree; only done
 * before first time step, and only processor 0 does it
 */
void setbound()
{
   long i;
   real side ;
   bodyptr p;

   SETVS(Local[0].min,1E99);
   SETVS(Local[0].max,-1E99);
   side=0;

   for (p = bodytab; p < bodytab+nbody; p++) {
      for (i=0; i<NDIM;i++) {
	 if (Pos(p)[i]<Local[0].min[i]) Local[0].min[i]=Pos(p)[i] ;
	 if (Pos(p)[i]>Local[0].max[i])  Local[0].max[i]=Pos(p)[i] ;
      }
   }

   SUBV(Local[0].max,Local[0].max,Local[0].min);
   for (i=0; i<NDIM;i++) if (side<Local[0].max[i]) side=Local[0].max[i];
   ADDVS(Global->rmin,Local[0].min,-side/100000.0);
   Global->rsize = 1.00002*side;
   SETVS(Global->max,-1E99);
   SETVS(Global->min,1E99);
}

void Help()
{
   printf("There are a total of twelve parameters, and all of them have default values.\n");
   printf("\n");
   printf("1) infile (char*) : The name of an input file that contains particle data.  \n");
   printf("    The format of the file is:\n");
   printf("\ta) An int representing the number of particles in the distribution\n");
   printf("\tb) An int representing the dimensionality of the problem (3-D)\n");
   printf("\tc) A double representing the current time of the simulation\n");
   printf("\td) Doubles representing the masses of all the particles\n");
   printf("\te) A vector (length equal to the dimensionality) of doubles\n");
   printf("\t   representing the positions of all the particles\n");
   printf("\tf) A vector (length equal to the dimensionality) of doubles\n");
   printf("\t   representing the velocities of all the particles\n");
   printf("\n");
   printf("    Each of these numbers can be separated by any amount of whitespace.\n");
   printf("\n");
   printf("2) nbody (int) : If no input file is specified (the first line is blank), this\n");
   printf("    number specifies the number of particles to generate under a plummer model.\n");
   printf("    Default is 16384.\n");
   printf("\n");
   printf("3) seed (int) : The seed used by the random number generator.\n");
   printf("    Default is 123.\n");
   printf("\n");
   printf("4) outfile (char*) : The name of the file that snapshots will be printed to. \n");
   printf("    This feature has been disabled in the SPLASH release.\n");
   printf("    Default is NULL.\n");
   printf("\n");
   printf("5) dtime (double) : The integration time-step.\n");
   printf("    Default is 0.025.\n");
   printf("\n");
   printf("6) eps (double) : The usual potential softening\n");
   printf("    Default is 0.05.\n");
   printf("\n");
   printf("7) tol (double) : The cell subdivision tolerance.\n");
   printf("    Default is 1.0.\n");
   printf("\n");
   printf("8) fcells (double) : The total number of cells created is equal to \n");
   printf("    fcells * number of leaves.\n");
   printf("    Default is 2.0.\n");
   printf("\n");
   printf("9) fleaves (double) : The total number of leaves created is equal to  \n");
   printf("    fleaves * nbody.\n");
   printf("    Default is 0.5.\n");
   printf("\n");
   printf("10) tstop (double) : The time to stop integration.\n");
   printf("    Default is 0.075.\n");
   printf("\n");
   printf("11) dtout (double) : The data-output interval.\n");
   printf("    Default is 0.25.\n");
   printf("\n");
   printf("12) NPROC (int) : The number of processors.\n");
   printf("    Default is 1.\n");
}
