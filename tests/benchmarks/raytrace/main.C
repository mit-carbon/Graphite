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
 * NAME
 *	prt - parallel ray tracer
 *
 * SYNOPSIS
 *	prt [options] envfile
 *
 *		-h	Print this usage message.
 *		-a<n>	Enable antialiasing with n subpixels (default = 1).
 *		-m<n>	Request n megabytes of global memory (default = 32).
 *		-p<n>	Run on n processors (default = 1).
 *
 * DESCRIPTION
 *
 * RETURNS
 *	PRT returns an exit code of 0 to the OS for successful operation; it
 *	returns a non-zero exit code (usually 1) if any type of error occurs.
 *
 * EXAMPLES
 *	To ray trace cube.env on 1 processor and default global memory size:
 *
 *		prt cube.env
 *
 *	To ray trace car.env on 4 processors and 72MB of global memory:
 *
 *		prt -p4 -m72 car.env
 *
 * FILES
 *
 * SEE ALSO
 *
 * DIAGNOSTICS
 *	All error messages take the form:
 *
 *	prt: Text of the error message.
 *
 *	All possible error messages are listed below, including the potential
 *	cause of the error, and the corrective action, if any.
 *
 *	FATAL ERRORS
 *		Invalid option '%c'.
 *			The command line included an option which was not
 *			recognized.  Check your command line syntax, remove
 *			the offending option, and try again.
 *
 *		Cannot open file "filename".
 *			The specified file could not be found, or some other
 *			OS error prevented it from being opened.  Check your
 *			typing and try again.
 *
 *		Cannot allocate local memory.
 *			malloc() failed for some reason.
 *
 *		Cannot allocate global memory.
 *			G_MALLOC() failed for some reason.
 *
 *		Valid range for #processors is [1, %ld].
 *			Do not exceed the ranges shown by the message.
 *
 */

#define MAIN	     /* indicate to rt.H that we need main_env for this file*/
#define VERSION 	"1.00"


#include <stdio.h>
#include <math.h>

#include "rt.h"


CHAR	*ProgName     = "RAYTRACE";          /* The program name.                 */
INT	nprocs	      = 1;		/* The number of processors to use.  */
INT	MaxGlobMem    = 32;		/* Maximum global memory needed (MB).*/
INT	NumSubRays    = 1;		/* Number of subpixel samples to calc*/
INT dostats = 0;


/*
 * NAME
 *	Usage - print proper usage message
 *
 * SYNOPSIS
 *	VOID	Usage()
 *
 * RETURNS
 *	Nothing.
 */

VOID	Usage()
	{
	fprintf(stdout, "%s - parallel ray tracer\n", ProgName);
	fprintf(stdout, "Version %s\n\n", VERSION);

	fprintf(stdout, "Usage:\t%s [options] envfile\n\n", ProgName);

	fprintf(stdout, "\t-h\tPrint this usage message.\n");
	fprintf(stdout, "\t-a<n>\tEnable antialiasing with n subpixels (default = 1).\n\tWhen using with SPLASH suite for evaluation, use default (no antialiasing)\n");
	fprintf(stdout, "\t-m<n>\tRequest n megabytes of global memory (default = 32).\n");
	fprintf(stdout, "\t-p<n>\tRun on n processors (default = 1).\n");
    fprintf(stdout, "\t-s\tMeasure and print per-process timing information.\n");
	fprintf(stdout, "\n");
	}



/*
 * NAME
 *	PrintStatistics - print out various ray tracer statistics
 *
 * SYNOPSIS
 *	VOID	PrintStatistics()
 *
 * RETURNS
 *	Nothing.
 */

VOID	PrintStatistics()
	{
	/*
	printf("\n****** Ray trace Stats ******\n");

	printf("\tResolution:\t\t%ld by %ld\n",            Display.xres+1, Display.yres+1);
	printf("\tNumber Lights:\t\t%ld\n",                nlights);
	printf("\tAnti level:\t\t%ld\n",                   Display.maxAAsubdiv);
	printf("\tTotal Rays:\t\t%ld\n",                   Stats.total_rays);
	printf("\tPrimary Rays:\t\t%ld\n",                 Stats.prim_rays);
	printf("\tShadow Rays:\t\t%ld\n",                  Stats.shad_rays);
	printf("\tShadow Rays Hit:\t%ld\n",                Stats.shad_rays_hit);
	printf("\tShadow Rays Not Hit:\t%ld\n",            Stats.shad_rays_not_hit);
	printf("\tShadow Coherence Rays:\t%ld\n",          Stats.shad_coherence_rays);
	printf("\tReflective Rays:\t%ld\n",                Stats.refl_rays);
	printf("\tTransmissiveRays:\t%ld\n",               Stats.trans_rays);
	printf("\tAnti-Aliasing Rays:\t%ld\n",             Stats.aa_rays);
	printf("\tBackground Pixels:\t%ld\n",              Stats.coverage);
	printf("\tMax Tree depth reached:\t%ld\n",         Stats.max_tree_depth);
	printf("\tMax # prims tested for a ray:\t%ld\n",   Stats.max_objs_ray);
	printf("\tMax Rays shot for a pixel:\t%ld\n",      Stats.max_rays_pixel);
	printf("\tMax # prims tested for a pixel:\t%ld\n", Stats.max_objs_pixel);
	printf("\n");
	*/

	if (TraversalType == TT_HUG)
		{
	/*	prn_ds_stats();
		prn_tv_stats();     */
		ma_print();
		}
	}



/*
 * NAME
 *	StartRayTrace - starting point for all ray tracing proceses
 *
 * SYNOPSIS
 *	VOID	StartRayTrace()
 *
 * RETURNS
 *	Nothing.
 */

VOID	StartRayTrace()
	{
	INT	pid;			/* Our internal process id number.   */
	UINT	begin;
	UINT	end;

   THREAD_INIT_FREE();

	LOCK(gm->pidlock)
	pid = gm->pid++;
	UNLOCK(gm->pidlock)

	BARINCLUDE(gm->start);

	if ((pid == 0) ||  (dostats))
        CLOCK(begin);

	/* POSSIBLE ENHANCEMENT: Here's where one might lock processes down
	to processors if need be */

	InitWorkPool(pid);
	InitRayTreeStack(Display.maxlevel, pid);

	/*
	 *	Wait for all processes to be created, initialize their work
	 *	pools, and arrive at this point; then proceed.	This BARRIER
	 *	is absolutely required.  Read comments in PutJob before
	 *	moving this barrier.
	 */

	BARRIER(gm->start, gm->nprocs)

	/* POSSIBLE ENHANCEMENT:  Here's where one would RESET STATISTICS
	and TIMING if one wanted to measure only the parallel part */

	RayTrace(pid);


	if ((pid == 0) || (dostats)) {
          CLOCK(end);
          gm->partime[pid] = (end - begin) & 0x7FFFFFFF;
          if (pid == 0) gm->par_start_time = begin;
        }
	}



/*
 * NAME
 *	main - mainline for the program
 *
 * SYNOPSIS
 *	INT	main(argc, argv)
 *	INT	argc;
 *	CHAR	*argv[];
 *
 * DESCRIPTION
 *	Main parses command line arguments, opens/closes the files involved,
 *	performs initializations, reads in the model database, partitions it
 *	as needed, and calls StartTraceRay() to do the work.
 *
 * RETURNS
 *	0 if successful.
 *	1 for any type of failure.
 */

int	main(int argc, CHAR *argv[])
	{
	INT	i;
	UINT	begin;
	UINT	end;
	UINT	lapsed;
	MATRIX	vtrans, Vinv;		/*  View transformation and inverse. */


	/*
	 *	First, process command line arguments.
	 */
	i = 1;
	while ((i < argc) && (argv[i][0] == '-')) {
		switch (argv[i][1]) {
			case '?':
			case 'h':
			case 'H':
				Usage();
				exit(1);

			case 'a':
			case 'A':
				AntiAlias = TRUE;
				if (argv[i][2] != '\0') {
					NumSubRays = atoi(&argv[i][2]);
				} else {
					NumSubRays = atoi(&argv[++i][0]);
				}
				break;

			case 'm':
				if (argv[i][2] != '\0') {
					MaxGlobMem = atoi(&argv[i][2]);
				} else {
					MaxGlobMem = atoi(&argv[++i][0]);
				}
				break;

			case 'p':
				if (argv[i][2] != '\0') {
					nprocs = atoi(&argv[i][2]);
				} else {
					nprocs = atoi(&argv[++i][0]);
				}
				break;

			case 's':
			case 'S':
				dostats = TRUE;
				break;

			default:
				fprintf(stderr, "%s: Invalid option \'%c\'.\n", ProgName, argv[i][0]);
				exit(1);
		}
		i++;
	}

	if (i == argc) {
		Usage();
		exit(1);
	}


	/*
	 *	Make sure nprocs is within valid range.
	 */

	if (nprocs < 1 || nprocs > MAX_PROCS)
		{
		fprintf(stderr, "%s: Valid range for #processors is [1, %d].\n", ProgName, MAX_PROCS);
		exit(1);
		}


	/*
	 *	Print command line parameters.
	 */

	printf("\n");
	printf("Number of processors:     \t%ld\n", nprocs);
	printf("Global shared memory size:\t%ld MB\n", MaxGlobMem);
	printf("Samples per pixel:        \t%ld\n", NumSubRays);
	printf("\n");


	/*
	 *	Initialize the shared memory environment and request the total
	 *	amount of amount of shared memory we might need.  This
	 *	includes memory for the database, grid, and framebuffer.
	 */

	MaxGlobMem <<= 20;			/* Convert MB to bytes.      */
	MAIN_INITENV(,MaxGlobMem + 512*1024)
   THREAD_INIT_FREE();
	gm = (GMEM *)G_MALLOC(sizeof(GMEM));


	/*
	 *	Perform shared environment initializations.
	 */

	gm->nprocs = nprocs;
	gm->pid    = 0;
	gm->rid    = 1;

	BARINIT(gm->start, nprocs)
	LOCKINIT(gm->pidlock)
	LOCKINIT(gm->ridlock)
	LOCKINIT(gm->memlock)
	ALOCKINIT(gm->wplock, nprocs)

/* POSSIBLE ENHANCEMENT:  Here is where one might distribute the
   raystruct data structure across physically distributed memories as
   desired.  */

	if (!GlobalHeapInit(MaxGlobMem))
		{
		fprintf(stderr, "%s: Cannot initialize global heap.\n", ProgName);
		exit(1);
		}


	/*
	 *	Initialize HUG parameters, read environment and geometry files.
	 */

	Huniform_defaults();
	ReadEnvFile(/* *argv*/argv[i]);
	ReadGeoFile(GeoFileName);
	OpenFrameBuffer();


	/*
	 *	Compute view transform and its inverse.
	 */

	CreateViewMatrix();
	MatrixCopy(vtrans, View.vtrans);
	MatrixInverse(Vinv, vtrans);
	MatrixCopy(View.vtransInv, Vinv);


	/*
	 *	Print out what we have so far.
	 */

	printf("Number of primitive objects: \t%ld\n", prim_obj_cnt);
	printf("Number of primitive elements:\t%ld\n", prim_elem_cnt);

	/*
	 *	Preprocess database into hierarchical uniform grid.
	 */

	if (TraversalType == TT_HUG)
		BuildHierarchy_Uniform();



	/*
	 *	Now create slave processes.
	 */

	CLOCK(begin)
  
   // Enable Models at the start of parallel execution
   CarbonEnableModels();

	CREATE(StartRayTrace, gm->nprocs);
	WAIT_FOR_END(gm->nprocs);
  
   // Disable Models at the end of parallel execution
   CarbonDisableModels();

	CLOCK(end)



	/*
	 *	We are finished.  Clean up, print statistics and run time.
	 */

	CloseFrameBuffer(PicFileName);
	PrintStatistics();

	lapsed = (end - begin) & 0x7FFFFFFF;



	printf("TIMING STATISTICS MEASURED BY MAIN PROCESS:\n");
	printf("        Overall start time     %20lu\n", begin);
	printf("        Overall end time   %20lu\n", end);
	printf("        Total time with initialization  %20lu\n", lapsed);
	printf("        Total time without initialization  %20lu\n", end - gm->par_start_time);

    if (dostats) {
        unsigned totalproctime, maxproctime, minproctime;

        printf("\n\n\nPER-PROCESS STATISTICS:\n");

        printf("%20s%20s\n","Proc","Time");
        printf("%20s%20s\n\n","","Tracing Rays");
        for (i = 0; i < gm->nprocs; i++)
            printf("%20ld%20ld\n",i,gm->partime[i]);

        totalproctime = gm->partime[0];
        minproctime = gm->partime[0];
        maxproctime = gm->partime[0];

        for (i = 1; i < gm->nprocs; i++) {
            totalproctime += gm->partime[i];
            if (gm->partime[i] > maxproctime)
                maxproctime = gm->partime[i];
            if (gm->partime[i] < minproctime)
                minproctime = gm->partime[i];
        }
        printf("\n\n%20s%20d\n","Max = ",maxproctime);
        printf("%20s%20d\n","Min = ",minproctime);
        printf("%20s%20d\n","Avg = ",(int) (((double) totalproctime) / ((double) (1.0 * gm->nprocs))));
    }

	MAIN_END
	}

