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
 *	workpool.c
 *
 * DESCRIPTION
 *	This file contains the private data definitions and code for the ray
 *	job work pool.	Each processor has its own workpool.
 *
 *	The workpool consists of a stack of pixel bundles.  Each bundle
 *	contains jobs for primary rays for a contiguous 2D pixel screen
 *	region.
 *
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"



/*
 * NAME
 *	PutJob - put another job into pid's work pool
 *
 * SYNOPSIS
 *	VOID	PutJob(xs, ys, xe, ye, xbe, ybe, pid)
 *	INT	xs,  ys;		// Start of block.
 *	INT	xe,  ye;		// Extent of block.
 *	INT	xbe, ybe;		// Extent of bundle.
 *	INT	pid;			// Process id.
 *
 *  DESCRIPTION
 *	Given a block of image screen pixels, this routine generates pixel
 *	bundle entries that are inserted into pid's work pool stack.
 *
 *	A block includes the starting pixel address, the block size in x and y
 *	dimensions and the bundle size for making pixel jobs.
 *
 *	Pixel addresses are 0 relative.
 *
 *	Locking of workpools for job insertion is not required since a given
 *	process only inserts into its own pool and since we use a BARRIER
 *	before jobs can be removed from workpools.
 *
 * RETURNS
 *	Nothing.
 */

VOID	PutJob(INT xs, INT ys, INT xe, INT ye, INT xbe, INT ybe, INT pid)
	{
	INT	i, j;
	INT	xb_addr, yb_addr;		/* Bundle pixel address.     */
	INT	xb_end,  yb_end;		/* End bundle pixels.	     */
	INT	xb_size, yb_size;		/* Bundle size. 	     */
	WPJOB	*wpentry;			/* New work pool entry.      */

	/* Starting block pixel address (upper left pixel). */

	xb_addr = xs;
	yb_addr = ys;

	/* Ending block pixel address (lower right pixel). */

	xb_end = xb_addr + xe - 1;
	yb_end = yb_addr + ye - 1;

	for (i = 0; i < ye; i += ybe)
		{
		for (j = 0; j < xe; j += xbe)
			{
			/* Determine bundle size. */

			if ((xb_addr + xbe - 1) <= xb_end)
				xb_size = xbe;
			else
				xb_size = xb_end - xb_addr + 1;

			if ((yb_addr + ybe - 1) <= yb_end)
				yb_size = ybe;
			else
				yb_size = yb_end - yb_addr + 1;


			/* Initialize work pool entry. */

			wpentry = GlobalMalloc(sizeof(WPJOB), "workpool.c");

			wpentry->xpix = xb_addr;
			wpentry->ypix = yb_addr;
			wpentry->xdim = xb_size;
			wpentry->ydim = yb_size;


			/* Add to top of work pool stack. */

			if (!gm->workpool[pid][0])
				wpentry->next = NULL;
			else
				wpentry->next = gm->workpool[pid][0];

			gm->workpool[pid][0] = wpentry;
			xb_addr += xbe;
			}

		xb_addr  = xs;
		yb_addr += ybe;
		}
	}



/*
 * NAME
 *	GetJob - get next job from pid's work pool
 *
 * SYNOPSIS
 *	INT	GetJob(job, pid)
 *	RAYJOB	*job;			// Ray job description.
 *	INT	pid;			// Process id.
 *
 * DESCRIPTION
 *	Return a primary ray job bundle from the top of the stack.  A ray job
 *	bundle consists of the starting primary ray pixel address and the size
 *	of the pixel bundle.
 *
 * RETURNS
 *	Work pool status code.
 */

INT	GetJob(RAYJOB *job, INT pid)
	{
	WPJOB	*wpentry;			/* Work pool entry.	     */

	ALOCK(gm->wplock, pid)
	wpentry = gm->workpool[pid][0];

	if (!wpentry)
		{
		gm->wpstat[pid][0] = WPS_EMPTY;
		AULOCK(gm->wplock, pid)
		return (WPS_EMPTY);
		}

	gm->workpool[pid][0] = wpentry->next;
	AULOCK(gm->wplock, pid)

	/* Set up ray job information. */

	job->x	   = wpentry->xpix;
	job->y	   = wpentry->ypix;
	job->xcurr = wpentry->xpix;
	job->ycurr = wpentry->ypix;
	job->xlen  = wpentry->xdim;
	job->ylen  = wpentry->ydim;

	GlobalFree(wpentry);
	return (WPS_VALID);
	}



/*
 * NAME
 *	GetJobs - get next job for pid (with job stealing)
 *
 * SYNOPSIS
 *	INT	GetJobs(job, pid)
 *	RAYJOB	*job;			// Ray job pointer from work pool.
 *	INT	pid;			// Process id.
 *
 * DESCRIPTION
 *
 * RETURNS
 *	Workpool status.
 */

INT	GetJobs(RAYJOB *job, INT pid)
	{
	INT	i;

	i = pid;

	/* First, try to get job from pid's own pool (or pool 0). */

	if (gm->wpstat[i][0] == WPS_VALID)
		 if (GetJob(job, i) == WPS_VALID)
			{
			return (WPS_VALID);
			}


	/*
	 *	If that failed, try to get job from another pid's work
	 *	pool.
	 */

		i = (pid + 1) % gm->nprocs;

		while (i != pid)
			{
			if (gm->wpstat[i][0] == WPS_VALID)
				if (GetJob(job, i) == WPS_VALID)
					{
					return (WPS_VALID);
					}

			i = (i + 1) % gm->nprocs;
			}

	return (WPS_EMPTY);
	}



/*
 * NAME
 *	PrintWorkPool - print out the work pool entries for a given process
 *
 * SYNOPSIS
 *	VOID	PrintWorkPool(pid)
 *	INT	pid;			// Process id.
 *
 * RETURNS
 *	Nothing.
 */

VOID	PrintWorkPool(INT pid)
	{
	WPJOB	*j;

	j = gm->workpool[pid][0];

	while (j)
		{
		printf("Workpool entry:    pid=%3ld    xs=%3ld    ys=%3ld    xe=%3ld    ye=%3ld\n", pid, j->xpix, j->ypix, j->xdim, j->ydim);
		j = j->next;
		}
	}



/*
 * NAME
 *	InitWorkPool - fill pid's work pool with jobs
 *
 * SYNOPSIS
 *	VOID	InitWorkPool(pid)
 *	INT	pid;			// Process id to initialize.
 *
 * RETURNS
 *	Nothing.
 */

VOID	InitWorkPool(INT pid)
	{
	INT	i;
	INT	x, y;
	INT	xe, ye;
	INT	xsize, ysize;

	gm->wpstat[pid][0]   = WPS_VALID;
	gm->workpool[pid][0] = NULL;

	i      = 0;
	xsize  = Display.xres/blockx;
	ysize  = Display.yres/blocky;

	for (y = 0; y < Display.yres; y += ysize)
		{
		if (y + ysize > Display.yres)
			ye = Display.yres - y;
		else
			ye = ysize;

		for (x = 0; x < Display.xres; x += xsize)
			{
			if (x + xsize > Display.xres)
				xe = Display.xres - x;
			else
				xe = xsize;

			if (i == pid)
				PutJob(x, y, xe, ye, bundlex, bundley, pid);

			i = (i + 1)%gm->nprocs;
			}
		}

	}

