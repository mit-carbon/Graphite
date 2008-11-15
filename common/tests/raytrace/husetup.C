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
 *	husetup.c
 *
 * DESCRIPTION
 *	This file contains routines that initialize, build, traverse, and
 *	intersect the hierarchical uniform grid (HUG) representation of the scene.
 *
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"



/*
 * NAME
 *	Huniform_defaults - setup the five HUG parameter defaults
 *
 * SYNOPSIS
 *	VOID	Huniform_defaults()
 *
 * RETURNS
 *	Nothing.
 */

VOID	Huniform_defaults()
	{
	hu_max_prims_cell   = 2;
	hu_gridsize	    = 5;
	hu_numbuckets	    = 23;
	hu_max_subdiv_level = 1;
	hu_lazy 	    = 0;
	}



/*
 * NAME
 *	BuildHierarchy_Uniform - build HU grid from model
 *
 * SYNOPSIS
 *	VOID	BuildHierarchy_Uniform()
 *
 * RETURNS
 *	Nothing.
 *
 */

VOID	BuildHierarchy_Uniform()
	{
	INT	num_pe;
	GRID	*g;
	GRID	*ng;
	VOXEL	*v;
	ELEMENT **pepa;

	init_masks();


	pepa = MakeElementArray(&num_pe);

	v = init_world_voxel(pepa,num_pe);

	gm->world_level_grid = init_world_grid(v, pepa, num_pe);
	g = gm->world_level_grid;

	ng = create_grid(v, g, num_pe);

	fprintf(stderr, "Uniform Hierarchy built.\n");
	}



/*
 * NAME
 *	IntersectHuniformPrimlist - intersect ray with objects in HU cell
 *
 * SYNOPSIS
 *	VOID	IntersectHuniformPrimlist(intersectPrim, hit, plist, r, pid)
 *	BOOL	*intersectPrim; 	// Primitive hit?
 *	IRECORD *hit;			// Intersection record.
 *	ELEMENT **plist;		// Primitive element list.
 *	RAY	*r;			// Original ray.
 *	INT	pid;			// Process id.
 *
 * RETURNS
 *	Nothing.
 *
 */

VOID	IntersectHuniformPrimlist(INT *intersectPrim, IRECORD *hit, VOXEL *v, RAY *r, INT pid)
	{
	ELEMENT **pptr; 		/* Primitive element list ptr.	     */
	OBJECT	*peParent;		/* Ptr to parent object.	     */
	ELEMENT *pe;			/* Primitive element ptr.	     */
	IRECORD newhit[ISECT_MAX];	/* Hit recorder.		     */
	INT	hitcode,i;
	REAL	t_out;

	t_out = r->ri->t_out;
	hit[0].t = HUGE_REAL;
	pptr = (ELEMENT**)v->cell;

	for (i = 0; i < v->numelements; i++)
		{
		pe	 = pptr[i];
		peParent = pe->parent;
		hitcode  = (*peParent->procs->pe_intersect)(r, pe, newhit);

		if (hitcode)
			if (newhit[0].t < hit[0].t && newhit[0].t < t_out)
				hit[0] = newhit[0];
		}

	if (hit[0].t < HUGE_REAL)
		*intersectPrim = TRUE;
	else
		*intersectPrim = FALSE;
	}



/*
 * NAME
 *	HuniformShadowIntersect - intersect shadow ray with objects in HU cell
 *
 * SYNOPSIS
 *	REAL	HuniformShadowIntersect(r, lightlength, pe, pid)
 *	RAY	*r;			// Incident ray.
 *	REAL	lightlength;		// Distance to light.
 *	ELEMENT *pe;			// Primitive element of shadow ray origin.
 *	INT	pid;			// Process number.
 *
 * RETURNS
 *	Transparency value along ray to light.
 *
 */

REAL	HuniformShadowIntersect(RAY *r, REAL lightlength, ELEMENT *pe, INT pid)
	{
	INT	status;
	INT	hitcode,i;
	REAL	trans;			/* Transparency factor. 	     */
	OBJECT	*peParent;		/* Ptr to parent object.	     */
	ELEMENT **pptr; 		/* Primitive element list ptr.	     */
	ELEMENT *pe2;			/* Ptr to element.		     */
	IRECORD newhit[ISECT_MAX];	/* Hit record.			     */
	VOXEL	*v;

	trans = 1.0;

	/* Now try uniform hierarchy. */

	r->ri = NULL;
	v = init_ray(r, gm->world_level_grid);

	if (v == NULL)
		{
		reset_rayinfo(r);
		return(trans);
		}

	newhit[0].t = HUGE_REAL;
	status	    = IN_WORLD;

	while (trans > 0.0 && status != EXITED_WORLD)
		{
		/* Intersect primitives in cell. */

		pptr = (ELEMENT**)v->cell;

		for (i = 0; (i < v->numelements) && (trans > 0.0); i++)
			{
			pe2	 = pptr[i];
			peParent = pe2->parent;
			hitcode  = (*peParent->procs->pe_intersect)(r, pe2, newhit);

			if (hitcode && newhit[0].pelem != pe && newhit[0].t < lightlength)
				trans *= newhit[0].pelem->parent->surf->ktran;

			}

		if (trans > 0.0)
			v = next_nonempty_leaf(r, STEP, &status);
		}

	reset_rayinfo(r);
	return (trans);
	}



/*
 * NAME
 *	TraverseHierarchyUniform - walk the HU grid to intersect a ray
 *
 * SYNOPSIS
 *	BOOL	TraverseHierarchyUniform(r, hit, pid)
 *	RAY	*r;
 *	IRECORD *hit;
 *	INT	pid;
 *
 * RETURNS
 *	TRUE if the ray hit something, FALSE otherwise.
 *
 */

BOOL	TraverseHierarchyUniform(RAY *r, IRECORD *hit, INT pid)
	{
	INT	status;
	INT	intersectPrim;
	VOXEL	*v;

	r->ri = NULL;
	v = init_ray(r, gm->world_level_grid);

	if (v == NULL)
		{
		reset_rayinfo(r);
		return (FALSE);
		}

	intersectPrim = FALSE;
	hit[0].t      = HUGE_REAL;
	status	      = IN_WORLD;

	while (!intersectPrim && status != EXITED_WORLD)
		{
		IntersectHuniformPrimlist(&intersectPrim, hit, v, r, pid);

		if (!intersectPrim)
			v = next_nonempty_leaf(r, STEP, &status);
		}

	reset_rayinfo(r);
	return (intersectPrim);
	}

