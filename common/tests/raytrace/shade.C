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
 *	shade.c
 *
 * DESCRIPTION
 *	This file contains routines that shade a ray, both nonshadowed and
 *	shadowed.
 *
 *	Generated secondary reflection and refractions rays are pushed onto
 *	the raytree stack.
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"



/*
 * NAME
 *	SpecularDirection - compute reflected ray
 *
 * SYNOPSIS
 *	VOID	SpecularDirection(R, N, I)
 *	POINT	R;			// Reflected ray.
 *	POINT	N;			// Normal.
 *	POINT	I;			// Incident ray.
 *
 * RETURNS
 *	Nothing.
 */

VOID	SpecularDirection(POINT R, POINT N, POINT I)
	{
	REAL	I_dot_N;		/* I*N				     */
	POINT	N2;			/* 2N				     */
	POINT	vprime; 		/* Scale of I			     */

	/*  Turner's calculation from first paper.  */

	I_dot_N = VecDot(I,N);
	I_dot_N = ABS(I_dot_N);
	I_dot_N = 1.0/I_dot_N;

	VecScale(vprime, I_dot_N, I);
	VecScale(N2, 2.0, N);

	VecAdd(R, vprime, N2);
	VecNorm(R);
	}



/*
 * NAME
 *	TransmissionDirection - calculate refracted ray
 *
 * SYNOPSIS
 *	BOOL	TransmissionDirection(T, N, I, kn)
 *	POINT	T;			// Transmitted ray.
 *	POINT	N;			// Normal.
 *	POINT	I;			// Incident ray.
 *	REAL	kn;			// Index of refraction.
 *
 * RETURNS
 *	TRUE if the ray was transmitted, FALSE if the ray was blocked.
 */

BOOL	TransmissionDirection(POINT T, POINT N, POINT I, REAL kn)
	{
	POINT	vprime; 		/* Parameters in calculation.	     */
	POINT	vplusn;
	REAL	I_dot_N;
	REAL	kf;
	REAL	vprime_sq;
	REAL	vplusn_sq;

	/*  Turner's calculation from first paper.  */

	I_dot_N = VecDot(I,N);
	I_dot_N = ABS(I_dot_N);
	I_dot_N = 1.0/I_dot_N;

	VecScale(vprime, I_dot_N, I);
	VecAdd(vplusn, vprime, N);

	vprime_sq = VecDot(vprime, vprime);
	vplusn_sq = VecDot(vplusn, vplusn);

	kf = kn*kn*vprime_sq - vplusn_sq;

	if (kf > RAYEPS)
		{
		kf = 1.0/sqrt(kf);

		VecScale(vplusn, kf, vplusn);
		VecSub(T, vplusn, N);
		VecNorm(T);
		}
	else
		return (FALSE);

	return (TRUE);
	}



/*
 * NAME
 *	Shade - shade a ray
 *
 * SYNOPSIS
 *	VOID	Shade(iP, N, ray, hit, pid)
 *	VEC3	iP;			// Intersection point.
 *	VEC3	N;			// Normal at intersection.
 *	RAY	*ray;			// Incident ray.
 *	IRECORD *hit;			// Intersect info.
 *	INT	pid;			// Process id number.
 *
 * RETURNS
 *	Nothing.
 */

VOID	Shade(VEC3 iP, VEC3 N, RAY *ray, IRECORD *hit, INT pid)
	{
	VEC3	Lvec;			/* Light vector.		     */
	VEC3	Hvec;			/* Highlight vector.		     */
	VEC3	Evec;			/* Eye vector.			     */
	RAY	shad_ray;		/* Shadow ray.			     */
	RAY	secondary_ray;		/* Secondary ray.		     */
	COLOR	surfcol;		/* Primitive surface color.	     */
	COLOR	col;			/* Ray color contribution.	     */
	REAL	NdotL;			/* Diffuse conritbution.	     */
	REAL	Diff;			/* Diffuse variable.		     */
	REAL	NdotH;			/* Highlight contribution.	     */
	REAL	spec;			/* Highlight variable.		     */
	OBJECT	*po;			/* Ptr to object.		     */
	SURF	*s;			/* Surface pointer.		     */
	INT	i;			/* Index variables.		     */
	REAL	lightlen;		/* Length of light vector.	     */
	REAL	shadtrans;		/* Shadow transmission. 	     */
	LIGHT	*lptr;			/* Light pointer.		     */

	/* Initialize primitive info and ray color. */

	po = hit->pelem->parent;
	s  = po->surf;
	VecCopy(surfcol, s->fcolor);

	/* Initialize color to ambient. */

	col[0] = View.ambient[0] * surfcol[0];
	col[1] = View.ambient[1] * surfcol[1];
	col[2] = View.ambient[2] * surfcol[2];

	/* Set shadow ray origin. */

	VecCopy(shad_ray.P, iP);
	VecNegate(Evec, ray->D);

	/* Account for all lights. */

	lptr = lights;
	for (i = 0; i < nlights; i++)
		{
		VecSub(Lvec, lptr->pos, iP);
		lightlen = VecLen(Lvec);
		VecNorm(Lvec);
		VecCopy(shad_ray.D, Lvec);

		LOCK(gm->ridlock);
		shad_ray.id = gm->rid++;
		UNLOCK(gm->ridlock);

		NdotL = VecDot(N, Lvec);

		if (NdotL > 0.0)
			{
			/* Test to see if point shadowed. */

			if (View.shad && !lptr->shadow)
				{
				switch (TraversalType)
					{
					case TT_LIST:
						shadtrans = ShadowIntersect(&shad_ray, lightlen, hit->pelem);
						break;

					case TT_HUG:
						shadtrans = HuniformShadowIntersect(&shad_ray, lightlen, hit->pelem, pid);
						break;
					}
				}
			else
				shadtrans = 1.0;

			/* Compute non-shadowed shades. */

			if (shadtrans > 0.0)
				{
				Diff = po->surf->kdiff * NdotL * shadtrans;

				col[0] += surfcol[0] * lptr->col[0] * Diff;
				col[1] += surfcol[1] * lptr->col[1] * Diff;
				col[2] += surfcol[2] * lptr->col[2] * Diff;

				/* Add specular. */

				if (s->kspec > 0.0)
					{
					VecAdd(Hvec,Lvec,Evec);
					VecNorm(Hvec);
					NdotH = VecDot(N,Hvec);

					if (NdotH > 0.0)
						{
						spec  = pow(NdotH, s->kspecn);
						spec *= s->kspec;

						col[0] += lptr->col[0]*spec;
						col[1] += lptr->col[1]*spec;
						col[2] += lptr->col[2]*spec;
						}
					}
				}
			}

		lptr = lptr->next;
		}

	/* Add color to pixel frame buffer. */

	VecScale(col, ray->weight, col);
	AddPixelColor(col, ray->x, ray->y);

	/* Recurse if not at maximum level. */

	if ((ray->level) + 1 < Display.maxlevel)
		{
		VecCopy(secondary_ray.P, iP);

		/* Specular. */
		secondary_ray.weight = po->surf->kspec * ray->weight;

		if (secondary_ray.weight > Display.minweight)
			{
			SpecularDirection(secondary_ray.D, N, ray->D);
			secondary_ray.level = ray->level + 1;

			LOCK(gm->ridlock);
			secondary_ray.id = gm->rid++;
			UNLOCK(gm->ridlock);

			secondary_ray.x = ray->x;
			secondary_ray.y = ray->y;

			PushRayTreeStack(&secondary_ray, pid);

			}

		/* Transmission. */
		secondary_ray.weight = po->surf->ktran * ray->weight;

		if (secondary_ray.weight > Display.minweight)
			{
			if (TransmissionDirection(secondary_ray.D, N, ray->D, po->surf->refrindex))
				{
				secondary_ray.level = ray->level + 1;

				LOCK(gm->ridlock);
				secondary_ray.id = gm->rid++;
				UNLOCK(gm->ridlock);

				secondary_ray.x = ray->x;
				secondary_ray.y = ray->y;

				PushRayTreeStack(&secondary_ray, pid);

				}
			}
		}
	}

