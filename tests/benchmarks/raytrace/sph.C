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
 *	sph.c
 *
 * DESCRIPTION
 *	This file contains all routines that operate on sphere objects.
 *
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"



/*
 * NAME
 *	SphName - return the object name
 *
 * SYNOPSIS
 *	CHAR	*SphName()
 *
 * RETURNS
 *	A pointer to the name string.
 */

CHAR	*SphName()
	{
	return ("sphere");
	}



/*
 * NAME
 *	SphPrint - print the sphere object data to stdout
 *
 * SYNOPSIS
 *	VOID	SphPrint(po)
 *	OBJECT	*po;			// Ptr to sphere object.
 *
 * RETURNS
 *	Nothing.
 */

VOID	SphPrint(OBJECT *po)
	{
	INT	i;
	SPHERE	*ps;			/* Ptr to sphere data.		     */
	ELEMENT *pe;			/* Ptr to sphere element.	     */

	pe = po->pelem;
	fprintf(stderr,"\tSphere object\n");

	for (i = 0; i < po->numelements; i++)
		{
		ps = (SPHERE *)(pe->data);
		fprintf(stderr,"\t\tcenter  %f %f %f\n", ps->center[0], ps->center[1], ps->center[2]);
		fprintf(stderr,"\t\t        radius %f %f\n\n", ps->rad, ps->rad2);
		pe++;
		}
	}



/*
 * NAME
 *	SphElementBoundBox - compute sphere element bounding box
 *
 * SYNOPSIS
 *	VOID	SphElementBoundBox(pe, ps)
 *	ELEMENT *pe;			// Ptr to sphere element.
 *	SPHERE	*ps;			// Ptr to sphere data.
 *
 * RETURNS
 *	Nothing.
 */

VOID	SphElementBoundBox(ELEMENT *pe, SPHERE *ps)
	{
	BBOX	*pbb;			/* Ptr to bounding box. 	     */

	pbb = &(pe->bv);

	pbb->dnear[0] = ps->center[0] - ps->rad;
	pbb->dnear[1] = ps->center[1] - ps->rad;
	pbb->dnear[2] = ps->center[2] - ps->rad;

	pbb->dfar[0]  = ps->center[0] + ps->rad;
	pbb->dfar[1]  = ps->center[1] + ps->rad;
	pbb->dfar[2]  = ps->center[2] + ps->rad;
	}



/*
 * NAME
 *	SphBoundBox - compute sphere object bounding box
 *
 * SYNOPSIS
 *	VOID	SphBoundBox(po)
 *	OBJECT	*po;			// Ptr to sphere object.
 *
 * RETURNS
 *	Nothing.
 */

VOID	SphBoundBox(OBJECT *po)
	{
	INT	i;
	SPHERE	*ps;			/* Ptr to sphere data.		     */
	ELEMENT *pe;			/* Ptr to sphere element.	     */
	BBOX	*pbb;			/* Ptr to bounding box. 	     */
	REAL	minx, maxx;
	REAL	miny, maxy;
	REAL	minz, maxz;

	pe   = po->pelem;
	pbb  = &(po->bv);

	minx = miny = minz =  HUGE_REAL;
	maxx = maxy = maxz = -HUGE_REAL;

	for (i = 0; i < po->numelements; i++)
		{
		ps = (SPHERE *)(pe->data);
		SphElementBoundBox(pe, ps);

		minx = Min(minx, pe->bv.dnear[0]);
		miny = Min(miny, pe->bv.dnear[1]);
		minz = Min(minz, pe->bv.dnear[2]);

		maxx = Max(maxx, pe->bv.dfar[0]);
		maxy = Max(maxy, pe->bv.dfar[1]);
		maxz = Max(maxz, pe->bv.dfar[2]);

		pe++;
		}

	pbb->dnear[0] = minx;
	pbb->dnear[1] = miny;
	pbb->dnear[2] = minz;

	pbb->dfar[0]  = maxx;
	pbb->dfar[1]  = maxy;
	pbb->dfar[2]  = maxz;
	}



/*
 * NAME
 *	SphNormal - compute sphere unit normal at given point on the surface
 *
 * SYNOPSIS
 *	VOID	SphNormal(hit, Pi, Ni)
 *	IRECORD *hit;			// Ptr to intersection record.
 *	POINT	Pi;			// Ipoint.
 *	POINT	Ni;			// Normal.
 *
 * NOTES
 *	The normal is the unit vector from the given point to the sphere
 *	center point.
 *
 * RETURNS
 *	Nothing.
 */

VOID	SphNormal(IRECORD *hit, POINT Pi, POINT Ni)
	{
	ELEMENT *pe;
	SPHERE	*ps;			/* Ptr to sphere data.		     */

	/*  Compute normal and make it a unit vector. */

	pe = hit->pelem;
	ps = (SPHERE *)pe->data;
	VecSub(Ni, Pi, ps->center);

	Ni[0] /= ps->rad;
	Ni[1] /= ps->rad;
	Ni[2] /= ps->rad;
	}



/*
 * NAME
 *	SphDataNormalize - normalize the sphere by the given normalization matrix
 *
 * SYNOPSIS
 *	VOID	SphDataNormalize(po, normMat)
 *	OBJECT	*po;			// Ptr to sphere object.
 *	MATRIX	normMat;		// Normalization matrix.
 *
 * RETURNS
 *	Nothing.
 */

VOID	SphDataNormalize(OBJECT *po, MATRIX normMat)
	{
	INT	i;
	SPHERE	*ps;			/* Ptr to sphere data.		     */
	ELEMENT *pe;			/* Ptr to sphere element.	     */
	POINT	surf_point;		/* Point on surface.		     */
	POINT	center_point;		/* Center point.		     */
	POINT	rad_vector;		/* Radius vector.		     */

	NormalizeBoundBox(&po->bv, normMat);

	pe = po->pelem;

	for (i = 0; i < po->numelements; i++)
		{
		ps = (SPHERE *)pe->data;

		NormalizeBoundBox(&pe->bv, normMat);

		surf_point[0]	= ps->center[0] + ps->rad;
		surf_point[1]	= ps->center[1];
		surf_point[2]	= ps->center[2];
		surf_point[3]	= 1.0;

		center_point[0] = ps->center[0];
		center_point[1] = ps->center[1];
		center_point[2] = ps->center[2];
		center_point[3] = 1.0;


		/* Transform center point. */

		VecMatMult(center_point, normMat, center_point);
		VecMatMult(surf_point, normMat, surf_point);


		/* Find new radius. */

		VecSub(rad_vector, surf_point, center_point);
		VecCopy(ps->center, center_point);

		ps->rad  = VecLen(rad_vector);
		ps->rad2 = ps->rad * ps->rad;

		pe++;
		}
	}



/*
 * NAME
 *	SphPeIntersect - calculate intersection of ray with sphere
 *
 * SYNOPSIS
 *	INT	SphPeIntersect(pr, pe, hit)
 *	RAY	*pr;				// Ptr to incident ray.
 *	ELEMENT *pe;				// Ptr to sphere element.
 *	IRECORD *hit;				// Intersection recorder.
 *
 * NOTES
 *	Calculate intersection of ray, X = P + tD with sphere,
 *	| X - C | = r2.  C = sphere center.  r2 = radius squared.
 *	This solves t2 - 2t(D*V) + (V*V - r2) = 0 at
 *	t = (D*V) +/- sqrt((D*V)2 - (V*V) + r2 ),
 *	where t2 = t squared, V = C - P and |D| = 1.
 *
 * RETURNS
 *	The number of intersection points.
 */

INT	SphPeIntersect(RAY *pr, ELEMENT *pe, IRECORD *hit)
	{
	INT	nhits;				/* Number of hits.	     */
	REAL	b, disc, t1, t2, vsq;		/* Formula variables.	     */
	SPHERE	*ps;				/* Ptr to sphere data.	     */
	POINT	V;				/* C - P		     */
	IRECORD *sphhit;

	ps  = (SPHERE *)(pe->data);
	sphhit = hit;

	VecSub(V, ps->center, pr->P);		/* Ray from origin to center.*/
	vsq = VecDot(V, V);			/* Length sq of V.	     */
	b   = VecDot(V, pr->D); 		/* Perpendicular scale of V. */

	if (vsq > ps->rad2  &&	b < RAYEPS)	/* Behind ray origin.	     */
		return (0);

	disc = b*b - vsq + ps->rad2;		/* Discriminate.	     */
	if (disc < 0.0) 			/* Misses ray.		     */
		return (0);

	disc = sqrt(disc);			/* Find intersection param.  */
	t2   = b + disc;
	t1   = b - disc;

	if (t2 <= RAYEPS)			/* Behind ray origin.	     */
		return (0);

	nhits = 0;
	if (t1 > RAYEPS)			/* Entering sphere.	     */
		{
		IsectAdd(sphhit, t1, pe);
		sphhit++;
		nhits++;
		}

	IsectAdd(sphhit, t2, pe);		/* Exiting sphere	     */
	nhits++;

	return (nhits);
	}



/*
 * NAME
 *	SphIntersect - call sphere object intersection routine
 *
 * SYNOPSIS
 *	INT	SphIntersect(pr, po, hit)
 *	RAY	*pr;			// Ptr to incident ray.
 *	OBJECT	*po;			// Ptr to sphere object.
 *	IRECORD *hit;			// Intersection record.
 *
 * RETURNS
 *	The number of intersections found.
 */

INT	SphIntersect(RAY *pr, OBJECT *po, IRECORD *hit)
	{
	INT	i;
	INT	nhits;			/* # hits in sphere.		     */
	ELEMENT *pe;			/* Ptr to sphere element.	     */
	IRECORD newhit[2];		/* Hit list.			     */

	/* Traverse sphere list to find intersections. */

	nhits	 = 0;
	pe	 = po->pelem;
	hit[0].t = HUGE_REAL;

	for (i = 0; i < po->numelements; i++)
		{
		if (SphPeIntersect(pr, pe, newhit))
			if (newhit[0].t < hit[0].t)
				{
				nhits++;
				hit[0].t     = newhit[0].t;
				hit[0].pelem = newhit[0].pelem;
				}
		pe++;
		}

	return (nhits);
	}



/*
 * NAME
 *	SphTransform - transform sphere object by a transformation matrix
 *
 * SYNOPSIS
 *	VOID	SphTransform(po, xtrans, xinvT)
 *	OBJECT	*po;			// Ptr to sphere object.
 *	MATRIX	xtrans; 		// Transformation matrix.
 *	MATRIX	xinvT;			// Transpose of inverse matrix.
 *
 * RETURNS
 *	Nothing.
 */

VOID	SphTransform(OBJECT *po, MATRIX xtrans, MATRIX xinvT)
	{
	INT	i;
	INT	numelems;		/* Number of object elements.	  */
	REAL	new_rad;
	SPHERE	*ps;			/* Ptr to sphere data.		     */
	ELEMENT *pe;			/* Ptr to sphere element.	     */
	POINT	surf_point;		/* Point on surface.		     */
	POINT	center_point;		/* Center_point.		     */
	POINT	rad_vector;		/* Radius vector.		     */

	pe	 = po->pelem;
	numelems = po->numelements;

	for (i = 0; i < numelems; i++)
		{
		ps = (SPHERE *)pe->data;

		/* See if radius has changed with a scale. */

		surf_point[0]	= ps->center[0] + ps->rad;
		surf_point[1]	= ps->center[1];
		surf_point[2]	= ps->center[2];
		surf_point[3]	= 1.0;

		center_point[0] = ps->center[0];
		center_point[1] = ps->center[1];
		center_point[2] = ps->center[2];
		center_point[3] = 1.0;

		/* Transform center point. */

		VecMatMult(center_point, xtrans, center_point);
		VecMatMult(surf_point, xtrans, surf_point);

		/* Find radius. */

		VecSub(rad_vector, surf_point, center_point);
		VecCopy(ps->center, center_point);

		new_rad = VecLen(rad_vector);

		if (new_rad != ps->rad)
			{
			ps->rad  = new_rad;
			ps->rad2 = ps->rad * ps->rad;
			}

		pe++;
		}
	}



/*
 * NAME
 *	SphRead - read sphere object data from file
 *
 * SYNOPSIS
 *	VOID	SphRead(po, pf)
 *	OBJECT	*po;			// Ptr to sphere object.
 *	FILE	*pf;			// Ptr to file.
 *
 * RETURNS
 *	Nothing.
 */

VOID	SphRead(OBJECT *po, FILE *pf)
	{
	INT	i;
	INT	instat; 		/* Input counter		     */
	SPHERE	*ps;			/* Ptr to sphere data.		     */
	ELEMENT *pe;			/* Ptr to sphere element.	     */

	pe = po->pelem;
	ps = (SPHERE*)GlobalMalloc(sizeof(SPHERE)*po->numelements, "sph.c");

	for (i = 0; i < po->numelements; i++)
		{
		instat = fscanf(pf,"%lf %lf %lf %lf", &(ps->center[0]), &(ps->center[1]), &(ps->center[2]), &(ps->rad));

		if (instat != 4)
			{
			printf("Error in SphRead: sphere %ld.\n", i);
			exit(1);
			}

		ps->center[3]  = 1.0;			/* w initialization. */
		ps->rad2       = ps->rad*ps->rad;

		pe->data       = (CHAR *)ps;
		pe->parent     = po;

		SphElementBoundBox(pe, ps);

		ps++;
		pe++;
		}
	}

