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
 *	isect.c
 *
 * DESCRIPTION
 *	This file contains routines that walk the model object list,
 *	intersecting both standard rays and shadow rays with objects in the
 *	list.
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"



/*
 * NAME
 *	Intersect - intersect ray with objects in linked list
 *
 * SYNOPSIS
 *	VOID	Intersect(pr, hit)
 *	RAY	*pr;			// Ptr to incident ray.
 *	IRECORD *hit;			// Intersection record.
 *
 * RETURNS
 *	TRUE if ray intersects an object, FALSE otherwise.
 */

BOOL	Intersect(RAY *pr, IRECORD *hit)
	{
	OBJECT	*po;				/* Ptr to the object.	     */
	IRECORD newhit; 			/* New intersection.	     */

	po	   = gm->modelroot;
	hit->t	   = HUGE_REAL;
	hit->pelem = NULL;

	while (po)
		{
		if (((int (*)(RAY *, OBJECT*, IRECORD*))(*po->procs->intersect))(pr, po, &newhit))
			if (newhit.t < hit[0].t)
				*hit = newhit;

		po = po->next;
		}

	if (hit->t < HUGE_REAL)
		return (TRUE);
	else
		return (FALSE);
	}



/*
 * NAME
 *	ShadowIntersect - intersect a shadow ray with objects in linked list
 *
 * SYNOPSIS
 *	REAL	ShadowIntersect(pr, lightlength, pe)
 *	RAY	*pr;			// Ptr to incident ray.
 *	REAL	lightdist;		// Distance to light.
 *	ELEMENT *pe;			// Ptr to element of ray origin.
 *
 * RETURNS
 *	The transparency value from ray origin to light source.
 *
 *	0.0 -> Fully obscured.
 *	1.0 -> Fully visible.
 */

REAL	ShadowIntersect(RAY *pr, REAL lightdist, ELEMENT *pe)
	{
	REAL	trans;				/* Transparency factor.      */
	OBJECT	*po;				/* Ptr to the object.	     */
	IRECORD newhit; 			/* New hit record.	     */

	trans = 1.0;
	po    = gm->modelroot;

	while (po && trans > 0.0)
		{
		if (((int (*)(RAY*, OBJECT*, IRECORD*)) (*po->procs->intersect))(pr, po, &newhit) && newhit.pelem != pe && newhit.t < lightdist)
			trans *= newhit.pelem->parent->surf->ktran;

		po = po->next;
		}

	return (trans);
	}

