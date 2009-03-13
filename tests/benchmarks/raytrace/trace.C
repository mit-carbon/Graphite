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
 *	trace.c
 *
 * DESCRIPTION
 *	This file contains the routines to process ray jobs from the work pool
 *	containing primary ray jobs.
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"

/*
 * NAME
 *	frand - generate random floating point number
 *
 * SYNOPSIS
 *	REAL	frand()
 *  Used instead of standard srand for portability to other systems than Unix.
 *
 * RETURNS
 *	A random floating point number in the range 0 <= x < 1.
 */

REAL	frand()
	{
	REAL	r;
	static	LONG	lLastRand = 0;

	lLastRand = lLastRand*214013L + 2531011L;
	r  = (REAL)((lLastRand >> 16) & 0x7FFF)/32768.0;
	return (r);
	}



/*
 * NAME
 *	GetRayJobFromBundle -
 *
 * SYNOPSIS
 *	BOOL	GetRayJobFromBundle(job, x, y)
 *	RAYJOB	*job;			// Ray job from work pool.
 *	INT	*x, *y; 		// Pixel address.
 *
 * DESCRIPTION
 *	This routine determines if there are any more primary rays left to
 *	process in the ray bundle job.
 *
 *	The routine returns FALSE if the bundle processing is complete.  If
 *	there are more jobs, the current x and y pixel position are returned
 *	through procedure parameters.  Then, x and y are bundle is updated for
 *	the next inquiry and TRUE is returned.
 *
 * RETURNS
 *	See above.
 */

BOOL	GetRayJobFromBundle(RAYJOB *job, INT *x, INT *y)
	{
	*x = job->xcurr;			/* Set pixel address first.  */
	*y = job->ycurr;

	if ((job->y + job->ylen) == job->ycurr) /* All done?		     */
		return (FALSE);

	job->xcurr++;				/* Update to next pixel.     */
	if ((job->x +job->xlen) == job->xcurr )
		{
		job->xcurr = job->x;		/* Go to new scanline.	     */
		job->ycurr++;
		}

	return (TRUE);
	}



/*
 * NAME
 *	ConvertPrimRayJobToRayMsg - convert primary ray job to the ray message format
 *
 * SYNOPSIS
 *	VOID	ConvertPrimRayJobToRayMsg(ray, x, y)
 *	RAY	*ray;			// Ray message.
 *	REAL	x, y;			// Pixel.
 *
 * DESCRIPTION
 *	This routine converts the primary ray job to the ray message format.
 *	The ray origin and direction are computed and the other ray message
 *	variables are initialized.
 *
 *		Perspective Projection:
 *
 *		Calculate ray direction thru pixel and transform it back to
 *		the world coordinate system.  Origin of ray is eye position.
 *
 *		Orthographic Projection:
 *
 *		Calculate ray direction thru pixel and transform it back to
 *		the world coordinate system.  Origin of ray must also be
 *		calculated.
 * RETURNS
 *	Nothing.
 */

VOID	ConvertPrimRayJobToRayMsg(RAY *ray, REAL x, REAL y)
	{
	VEC4	dir;
	VEC4	origin;

	if (View.projection == PT_PERSP)
		{
		dir[0] = -Display.scrHalfWidth	+ (x*Display.vWscale);
		dir[1] =  Display.scrHalfHeight - (y*Display.vHscale);
		dir[2] =  Display.scrDist;
		dir[3] =  0.0;

		/* Transform ray back to world. */
		TransformViewRay(dir);

		VecNorm(dir);
		VecCopy(ray->D, dir);
		VecCopy(ray->P, View.eye);
		}
	else
		{
		/* Orthographic projection. */

		dir[0] = 0.0;
		dir[1] = 0.0;
		dir[2] = 1.0;
		dir[3] = 0.0;

		/* Transform ray back to world. */
		TransformViewRay(dir);
		VecNorm(dir);

		VecCopy(ray->D, dir);

		/* Calculate origin. */

		origin[0] = -Display.scrHalfWidth  + (x*Display.vWscale);
		origin[1] =  Display.scrHalfHeight - (y*Display.vHscale);
		origin[2] =  0.0;
		origin[3] =  1.0;

		/* Transform origin back to world. */

		TransformViewRay(origin);
		VecCopy(ray->P, origin);
		}


	/* Initialize other fields of ray message. */

	ray->level  = 0;
	ray->weight = 1.0/(REAL)NumSubRays;

	LOCK(gm->ridlock);
	ray->id = gm->rid++;
	UNLOCK(gm->ridlock);

	ray->x = (INT)x;
	ray->y = (INT)y;

	}



/*
 * NAME
 *	RayTrace - process primary ray bundle jobs from the workpool
 *
 * SYNOPSIS
 *	VOID	RayTrace(pid)
 *	INT	pid;			// Process id.
 *
 * DESCRIPTION
 *	Process primary ray bundle jobs from the workpool.  Each ray job from
 *	the ray bundle list performs the following tasks.
 *
 *		A ray bundle job is converted to a ray message and pushed
 *		on the raytree stack.  A ray job consists of a pixel address
 *		while the ray message also contains the ray origin, direction,
 *		level in tree, ray weight, ray id, and grid parameters.
 *		Processing begins with the primary ray and secondary rays being
 *		pushed onto the stack as the raytree is built and processed.
 *		The raytree stack is used to avoid recursion.
 *
 *		Processes all jobs on raytree stack.
 *
 *		Calls routines for intersecting a ray with the environment and
 *		for shading the ray.
 *
 * RETURNS
 *	Nothing.
 */

VOID	RayTrace(INT pid)
	{
	INT	j;
	INT	x, y;			/* Pixel address.		     */
	REAL	xx, yy;
	VEC3	N;			/* Normal at intersection.	     */
	VEC3	Ipoint; 		/* Intersection point.		     */
	COLOR	c;			/* Color for storing background.     */
	RAY	*ray;			/* Ray pointer. 		     */
	RAY	rmsg;			/* Ray message. 		     */
	RAYJOB	job;			/* Ray job from work pool.	     */
	OBJECT	*po;			/* Ptr to object.		     */
	BOOL	hit;			/* An object hit?		     */
	IRECORD hitrecord;		/* Intersection record. 	     */

	ray = &rmsg;

	while (GetJobs(&job, pid) != WPS_EMPTY)
		{
		while (GetRayJobFromBundle(&job, &x, &y))
			{
			/* Convert the ray job to the ray message format. */

			xx = (REAL)x;
			yy = (REAL)y;

			if (AntiAlias)
				for (j = 0; j < NumSubRays; j++)
					{
					ConvertPrimRayJobToRayMsg(ray, xx + frand(), yy + frand());
					PushRayTreeStack(ray, pid);
					}
			else
				{
				ConvertPrimRayJobToRayMsg(ray, xx, yy);
				PushRayTreeStack(ray, pid);
				}

			while (PopRayTreeStack(ray, pid) != RTS_EMPTY)
				{
				/* Find which object is closest along the ray. */

				switch (TraversalType)
					{
					case TT_LIST:
						hit = Intersect(ray, &hitrecord);
						break;

					case TT_HUG:
						hit = TraverseHierarchyUniform(ray, &hitrecord, pid);
						break;
					}

				/* Process the object ray hit. */

				if (hit)
					{
					/*
					 *  Get parent object to be able to access
					 *  object operations.
					 */

					po = hitrecord.pelem->parent;

					/* Calculate intersection point. */
					RayPoint(Ipoint, ray, hitrecord.t);

					/* Calculate normal at this point. */
					((void (*)(IRECORD *, VEC3, VEC3))(*po->procs->normal))(&hitrecord, Ipoint, N);

					/* Make sure normal is pointing toward ray origin. */
					if ((VecDot(ray->D, N)) >  0.0)
						VecNegate(N, N);

					/*
					 *  Compute shade at this point - will process
					 *  shadow rays and add secondary reflection
					 *  and refraction rays to ray tree stack
					 */

					Shade(Ipoint, N, ray, &hitrecord, pid);
					}
				else
					{
					/* Add background as pixel contribution. */

					VecCopy(c, View.bkg);
					VecScale(c, ray->weight, c);
					AddPixelColor(c, ray->x, ray->y);
					}
				}
			}
		}
	}

