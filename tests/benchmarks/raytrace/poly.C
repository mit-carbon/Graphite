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
 *	poly.c
 *
 * DESCRIPTION
 *	This file contains all routines that operate on polygon objects.
 *
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"



/*
 * NAME
 *	PolyName - return the object name
 *
 * SYNOPSIS
 *	CHAR	*PolyName()
 *
 * RETURNS
 *	A pointer to the name string.
 */

CHAR	*PolyName()
	{
	return ("poly");
	}



/*
 * NAME
 *	PolyPrint - print the polygon object data to stdout
 *
 * SYNOPSIS
 *	VOID	PolyPrint(po)
 *	OBJECT	*po;			// Ptr to polygon object.
 *
 * RETURNS
 *	Nothing.
 */

VOID	PolyPrint(OBJECT *po)
	{
	INT	i, j;
	INT	*vindex;		/* Ptr to vertex index. 	     */
	VEC3	*vlist, *vptr;		/* Ptr to vertex list.		     */
	POLY	*pp;			/* Ptr to polygon data. 	     */
	ELEMENT *pe;			/* Ptr to polygon element.	     */

	pe = po->pelem;
	fprintf(stderr, "\tpolygon: %ld polygons.\n", po->numelements);

	for (i = 0; i < po->numelements; i++)
		{
		pp = (POLY *)pe->data;

		fprintf(stderr, "\t\tVertices: %ld  Plane eq: %f %f %f %f\n", pp->nverts, pp->norm[0], pp->norm[1], pp->norm[2], pp->d);

		vlist  = pp->vptr;
		vindex = pp->vindex;

		for (j = 0; j < pp->nverts; j++)
			{
			vptr = vlist + (*vindex);
			fprintf(stderr, "\t\t%f %f %f \n", (*vptr)[0], (*vptr)[1], (*vptr)[2]);
			vindex++;
			}

		pe++;
		}
	}



/*
 * NAME
 *	PolyElementBoundBox - compute polygon element bounding box
 *
 * SYNOPSIS
 *	VOID	PolyElementBoundBox(pe, pp)
 *	ELEMENT *pe;			// Ptr to polygon element.
 *	POLY	*pp;			// Ptr to polygon data.
 *
 * RETURNS
 *	Nothing.
 */

VOID	PolyElementBoundBox(ELEMENT *pe, POLY *pp)
	{
	INT	i;			/* Index.			     */
	INT	*vindex;		/* Vertex index pointer.	     */
	BBOX	*pbb;			/* Ptr to bounding box. 	     */
	VEC3	*vlist, *vptr;		/* Vertex list pointer. 	     */
	REAL	 minx, maxx;
	REAL	 miny, maxy;
	REAL	 minz, maxz;

	pbb = &(pe->bv);

	minx = miny = minz =  HUGE_REAL;
	maxx = maxy = maxz = -HUGE_REAL;

	vlist  = pp->vptr;
	vindex = pp->vindex;

	for (i = 0; i < pp->nverts; i++)
		{
		vptr = vlist + (*vindex);

		minx = Min(minx, (*vptr)[0]);
		miny = Min(miny, (*vptr)[1]);
		minz = Min(minz, (*vptr)[2]);

		maxx = Max(maxx, (*vptr)[0]);
		maxy = Max(maxy, (*vptr)[1]);
		maxz = Max(maxz, (*vptr)[2]);

		vindex++;
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
 *	PolyBoundBox - compute polygon object bounding box
 *
 * SYNOPSIS
 *	VOID	PolyBoundBox(po)
 *	OBJECT	*po;			// Ptr to polygon object.
 *
 * RETURNS
 *	Nothing.
 */

VOID	PolyBoundBox(OBJECT *po)
	{
	INT	i;
	POLY	*pp;			/* Ptr to polygon data. 	     */
	ELEMENT *pe;			/* Ptr to polygon element.	     */
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
		pp = (POLY *)pe->data;
		PolyElementBoundBox(pe, pp);

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
 *	PolyNormal - compute polygon unit normal at given point on the surface
 *
 * SYNOPSIS
 *	VOID	PolyNormal(hit, Pi, Ni)
 *	IRECORD *hit;			// Ptr to intersection record.
 *	POINT	Pi;			// Ipoint.
 *	POINT	Ni;			// Normal.
 *
 * NOTES
 *	If no normal interpolation, the normal is the face normal.  Otherwise,
 *	interpolate with the normal plane equation.
 *
 * RETURNS
 *	Nothing.
 */

VOID	PolyNormal(IRECORD *hit, POINT Pi, POINT Ni)
	{
	ELEMENT *pe;
	POLY	*pp;

	/* Retrieve normal from hit polygon. */

	pe = hit->pelem;
	pp = (POLY *)pe->data;
	VecCopy(Ni, pp->norm);
	}



/*
 * NAME
 *	PolyDataNormalize - normalize polygon data and bounding box
 *
 * SYNOPSIS
 *	VOID	PolyDataNormalize(po, normMat)
 *	OBJECT	*po;			// Ptr to polygon object.
 *	MATRIX	 normMat;		// Normalization matrix.
 *
 * RETURNS
 *	Nothing.
 */

VOID	PolyDataNormalize(OBJECT *po, MATRIX normMat)
	{
	INT	i;
	POINT	coord;
	VEC3	*pv;			/* Ptr to vertex list.		     */
	POLY	*pp;			/* Ptr to polygon data. 	     */
	ELEMENT *pe;			/* Ptr to polygon element.	     */


	/* Normalize bounding box. */

	pe = po->pelem;
	NormalizeBoundBox(&po->bv, normMat);


	/* Normalize vertex list. */

	pp = (POLY *)pe->data;
	pv = pp->vptr;

	coord[0] = (*pv)[0];
	coord[1] = (*pv)[1];
	coord[2] = (*pv)[2];
	coord[3] = 1.0;


	/* Transform coordinate by xform matrix. */

	while (coord[0] != HUGE_REAL && coord[1] != HUGE_REAL && coord[2] != HUGE_REAL)
		{
		VecMatMult(coord, normMat, coord);

		(*pv)[0] = coord[0];
		(*pv)[1] = coord[1];
		(*pv)[2] = coord[2];

		pv++;

		coord[0] = (*pv)[0];
		coord[1] = (*pv)[1];
		coord[2] = (*pv)[2];
		coord[3] = 1.0;
		}


	/* Normalize bounding boxes of object elements. */

	for (i = 0; i < po->numelements; i++)
		{
		pp = (POLY *)pe->data;
		NormalizeBoundBox(&pe->bv, normMat);

		/* Find new d of plane equation. */

		pv    =   pp->vptr + (*(pp->vindex));
		pp->d = -(pp->norm[0]*(*pv)[0] + pp->norm[1]*(*pv)[1] + pp->norm[2]*(*pv)[2]);

		pe++;
		}
	}



/*
 * NAME
 *	PolyPeIntersect - calculate intersection of ray with polygon
 *
 * SYNOPSIS
 *	INT	PolyPeIntersect(pr, pe, hit)
 *	RAY	*pr;				// Ptr to incident ray.
 *	ELEMENT *pe;				// Polyangle object
 *	IRECORD *hit;				// Intersection record.
 *
 * NOTES
 *	Check first to see if ray is parallel to plane or if the intersection
 *	point with the plane is behind the eye.
 *
 *	If neither of these cases applies, determine if the intersection point
 *	is contained in the polygon.  First, project onto a plane.  Translate
 *	the intersection point to the origin.  If a ray starting from the
 *	origin along the 2 - d X axis of the plane crosses an odd number of
 *	polygon edges, the point is in the polygon.
 *
 * RETURNS
 *	The number of intersection points.
 */

INT	PolyPeIntersect(RAY *pr, ELEMENT *pe, IRECORD *hit)
	{
	INT	i;
	INT	*vindex;		/* Vertex index pointer.	     */
	INT	toright;		/* Counter.			     */
	INT	sh, nsh;		/* Sign holders.		     */
	REAL	Rd_dot_Pn;		/* Polygon normal dot ray direction. */
	REAL	Ro_dot_Pn;		/* Polygon normal dot ray origin.    */
	REAL	tval;			/* Intersection t distance value.    */
	REAL	x[MAX_VERTS + 1];	/* Projection list.		     */
	REAL	y[MAX_VERTS + 1];	/* Projection list.		     */
	REAL	ix, iy; 		/* Intersection projection point.    */
	REAL	dx, dy; 		/* Deltas between 2 vertices.	     */
	REAL	xint;			/* Intersection value.		     */
	VEC3	I;			/* Intersection point.		     */
	VEC3	*vlist, *vpos;		/* Vertex list pointer. 	     */
	POLY	*pp;			/* Ptr to polygon data. 	     */

	pp = (POLY *)pe->data;

	Rd_dot_Pn = VecDot(pp->norm, pr->D);

	if (ABS(Rd_dot_Pn) < RAYEPS)		/* Ray is parallel.	     */
		return (0);

	Ro_dot_Pn = VecDot(pp->norm, pr->P);

	tval = -(pp->d + Ro_dot_Pn)/Rd_dot_Pn;	/* Intersection distance.    */
	if (tval < RAYEPS)			/* Intersects behind ray.    */
		return (0);

	RayPoint(I, pr, tval);


	/* Polygon containment. */

	/* Project onto plane with greatest normal component. */

	vlist  = pp->vptr;
	vindex = pp->vindex;

	switch (pp->axis_proj)
		{
		case X_AXIS:
			for (i = 0; i < pp->nverts; i++)
				{
				vpos = vlist + (*vindex);
				x[i] = (*vpos)[1];
				y[i] = (*vpos)[2];
				vindex++;
				}

			ix = I[1];
			iy = I[2];
			break;

		case Y_AXIS:
			for (i = 0; i < pp->nverts; i++)
				{
				vpos = vlist + (*vindex);
				x[i] = (*vpos)[0];
				y[i] = (*vpos)[2];
				vindex++;
				}

			ix = I[0];
			iy = I[2];
			break;

		case Z_AXIS:
			for (i = 0; i < pp->nverts; i++)
				{
				vpos = vlist + (*vindex);
				x[i] = (*vpos)[0];
				y[i] = (*vpos)[1];
				vindex++;
				}

			ix = I[0];
			iy = I[1];
			break;
		}


	/* Translate to origin. */

	for (i = 0; i < pp->nverts; i++)
		{
		x[i] -= ix;
		y[i] -= iy;

		if (ABS(y[i]) < RAYEPS)
			y[i] = 0.0;
		}

	x[pp->nverts] = x[0];
	y[pp->nverts] = y[0];


	/*
	 *	If intersection point crosses an odd number of line segments,
	 *	the point is inside the polygon
	 */


	if (y[0] < 0.0)
		sh = 0;
	else
		sh = 1;

	toright = 0;

	for (i = 0; i < pp->nverts; i++)
		{
		/* Check if segment crosses in y. */

		if (y[i + 1] < 0.0)
			nsh = 0;
		else
			nsh = 1;

		if (nsh ^ sh)
			{
			dy = y[i + 1] - y[i];

			if (ABS(dy) >= RAYEPS)
				{
				dx   = x[i + 1] - x[i];
				xint = x[i] - y[i]*dx / dy;

				if (xint > 0.0)
					toright++;
				}
			}

		sh = nsh;
		}

	if (toright%2 == 1)
		{
		IsectAdd(hit, tval, pe);
		return (1);
		}
	else
		return (0);
	}



/*
 * NAME
 *	PolyIntersect - call polygon object intersection routine
 *
 * SYNOPSIS
 *	INT	PolyIntersect(pr, po, hit)
 *	RAY	*pr;			// Ptr to incident ray.
 *	OBJECT	*po;			// Ptr to polygon object.
 *	IRECORD *hit;			// Intersection record.
 *
 * RETURNS
 *	The number of intersections found.
 */

INT	PolyIntersect(RAY *pr, OBJECT *po, IRECORD *hit)
	{
	INT	i;
	INT	nhits;			/* # hits in polyhedra. 	     */
	ELEMENT *pe;			/* Ptr to polygon element.	     */
	IRECORD newhit; 		/* Hit list.			     */

	/* Traverse polygon list to find intersections. */

	nhits	 = 0;
	pe	 = po->pelem;
	hit[0].t = HUGE_REAL;

	for (i = 0; i < po->numelements; i++)
		{
		if (PolyPeIntersect(pr, pe, &newhit))
			{
			nhits++;
			if (newhit.t < hit[0].t)
				{
				hit[0].t     = newhit.t;
				hit[0].pelem = newhit.pelem;
				}
			}

		pe++;
		}

	return (nhits);
	}



/*
 * NAME
 *	PolyTransform - transform polygon object by a transformation matrix
 *
 * SYNOPSIS
 *	VOID	PolyTransform(po, xtrans, xinvT)
 *	OBJECT	*po;			// Ptr to polygon object.
 *	MATRIX	xtrans; 		// Transformation matrix.
 *	MATRIX	xinvT;			// Transpose of inverse matrix.
 *
 * NOTES
 *	Routine must:
 *		- transform face normals
 *		- transform vertices
 *		- calculate plane equation d variables
 *
 * RETURNS
 *	Nothing.
 */

VOID	PolyTransform(OBJECT *po, MATRIX xtrans, MATRIX xinvT)
	{
	INT	i;			/* Indices.			     */
	INT	numelems;		/* # of elements.		     */
	VEC3	*vptr, *vp;		/* Vertex list pointers.	     */
	VEC4	norm, coord;		/* Transform 4 vectors. 	     */
	POLY	*pp;			/* Ptr to polygon data. 	     */
	ELEMENT *pe;			/* Ptr to polygon element.	     */


	pe = po->pelem;
	numelems = po->numelements;

	/* Get vertex list address and transform vertices. */

	pp   = (POLY *)pe->data;
	vptr = pp->vptr;

	coord[0] = (*vptr)[0];
	coord[1] = (*vptr)[1];
	coord[2] = (*vptr)[2];
	coord[3] = 1.0;

	while (coord[0] != HUGE_REAL && coord[1] != HUGE_REAL && coord[2] != HUGE_REAL)
		{
		/* Transform coordinate by xform matrix. */

		VecMatMult(coord, xtrans, coord);

		(*vptr)[0] = coord[0];
		(*vptr)[1] = coord[1];
		(*vptr)[2] = coord[2];

		vptr++;

		coord[0] = (*vptr)[0];
		coord[1] = (*vptr)[1];
		coord[2] = (*vptr)[2];
		coord[3] = 1.0;
		}


	/* Transform polygon list. */

	for (i = 0; i < numelems; i++)
		{
		pp = (POLY *)pe->data;

		/* Transform face normal by view matrix inverse. */

		norm[0] = pp->norm[0];
		norm[1] = pp->norm[1];
		norm[2] = pp->norm[2];
		norm[3] = 0.0;

		VecMatMult(norm, xinvT, norm);
		VecNorm(norm);

		pp->norm[0] = norm[0];
		pp->norm[1] = norm[1];
		pp->norm[2] = norm[2];


		/* Calculate plane equation variable d. */

		vp = pp->vptr + *(pp->vindex);
		pp->d = -(pp->norm[0]*(*vp)[0] + pp->norm[1]*(*vp)[1] + pp->norm[2]*(*vp)[2]);


		/* Set best axis projection. */

		norm[0] = ABS(pp->norm[0]);
		norm[1] = ABS(pp->norm[1]);
		norm[2] = ABS(pp->norm[2]);

		if (norm[0] >= norm[1] && norm[0] >= norm[2])
			pp->axis_proj = X_AXIS;
		else
		if (norm[1] >= norm[0] && norm[1] >= norm[2])
			pp->axis_proj = Y_AXIS;
		else
			pp->axis_proj = Z_AXIS;

		pe++;
		}
	}



/*
 * NAME
 *	PolyRead - read polygon object data from file
 *
 * SYNOPSIS
 *	VOID	PolyRead(po, pf)
 *	OBJECT	*po;			// Ptr to polygon object.
 *	FILE	*pf;			// Ptr to file.
 *
 * RETURNS
 *	Nothing.
 */

VOID	PolyRead(OBJECT *po, FILE *pf)
	{
	INT	i, j;			/* Indices.			     */
	INT	instat; 		/* Read status. 		     */
	INT	*vindex;
	INT	totalverts;		/* Total # of vertices in poly mesh. */
	CHAR	normstr[5];		/* Face/vertex normal flag string.   */
	BOOL	pnormals;		/* Face normals present?	     */
	VEC3	pnorm;			/* Polygon normal accumulator.	     */
	VEC3	*vlist, *vptr, *vp;	/* Ptr to vertex list.		     */
	VEC3	*vptmp, *vptmp2;	/* Ptr to vertex list.		     */
	VEC3	tmppnt, tmppnt2, cross;
	POLY	*pp;			/* Ptr to polygon data. 	     */
	ELEMENT *pe;			/* Ptr to polygon element.	     */


	pe = po->pelem;

	/* Allocate space for object data. */

	instat = fscanf(pf, "%ld", &totalverts);

	if (instat != 1)
		{
		printf("Error in PolyRead: totalverts.\n");
		exit(-1);
		}

	pp    = (POLY*)GlobalMalloc(sizeof(POLY)*po->numelements,  "poly.c");
	vlist = (VEC3*)GlobalMalloc(sizeof(VEC3)*(totalverts + 1), "poly.c");
	vptr  = vlist;


	/* Are polygon face normals supplied? */

	instat = fscanf(pf, "%s\n", normstr);

	if (instat != 1)
		{
		printf("Error in PolyRead: face normal indicator.\n");
		exit(-1);
		}

	pnormals = (normstr[2] == 'y' ? TRUE : FALSE);


	/* Read vertex list. */

	for (i = 0; i < totalverts; i++)
		{
		instat = fscanf(pf, "%lf %lf %lf", &(*vptr)[0], &(*vptr)[1], &(*vptr)[2]);

		if (instat != 3)
			{
			printf("Error in PolyRead: vertex %ld.\n", i);
			exit(-1);
			}

		vptr++;
		}


	(*vptr)[0] = HUGE_REAL;
	(*vptr)[1] = HUGE_REAL;
	(*vptr)[2] = HUGE_REAL;


	/* Read polygon list. */

	for (i = 0; i < po->numelements; i++)
		{
		instat = fscanf(pf, "%ld", &(pp->nverts));

		if (instat != 1)
			{
			printf("Error in PolyRead: vertex count.\n");
			exit(-1);
			}

		if (pp->nverts > MAX_VERTS)
			{
			printf("Polygon vertex count, %ld, exceeds maximum.\n", pp->nverts);
			exit(-1);
			}

		if (pnormals)
			{
			instat = fscanf(pf, " %lf %lf %lf", &(pp->norm[0]), &(pp->norm[1]), &(pp->norm[2]));

			if (instat != 3)
				{
				printf("Error in PolyRead: face normal %ld.\n", i);
				exit(-1);
				}
			}

		pp->vptr   = vlist;
		pp->vindex = (INT*)GlobalMalloc(sizeof(INT)*pp->nverts, "poly.c");
		vindex	   = pp->vindex;

		for (j = 0; j < pp->nverts; j++)
			{
			instat = fscanf(pf, "%ld", vindex++);

			if (instat != 1)
				{
				printf("Error in PolyRead: vertex index %ld.\n", i);
				exit(-1);
				}
			}

		/* If not supplied, calculate plane normal. */

		vindex = pp->vindex;
		vptr   = vlist;

		if (!pnormals)
			{
			vp = vptr + (*vindex);
			VecZero(pnorm);

			for (j = 0; j < pp->nverts - 2; j++)
				{
				vptmp  = vptr + (*(vindex + 1));
				vptmp2 = vptr + (*(vindex + 2));

				VecSub(tmppnt,	(*vptmp),  (*vp));
				VecSub(tmppnt2, (*vptmp2), (*vptmp));

				VecCross(cross, tmppnt, tmppnt2);
				VecAdd(pnorm,	pnorm,	cross);

				vp = vptmp;
				vindex += 1;
				}

			VecSub(tmppnt, (*vptmp2), (*vp));
			vindex	= pp->vindex;
			vp	= vptr + (*vindex);

			VecSub(tmppnt2, (*vp), (*vptmp2));
			VecCross(cross, tmppnt, tmppnt2);
			VecAdd(pnorm, pnorm, cross);

			vp	= vptr + (*vindex);
			VecSub(tmppnt, (*vp), (*vptmp2));
			vptmp	= vptr + (*(vindex + 1));

			VecSub(tmppnt2, (*vptmp), (*vp));
			VecCross(cross, tmppnt, tmppnt2);
			VecAdd(pnorm, pnorm, cross);
			VecScale(pp->norm, 1.0/VecLen(pnorm), pnorm);
			}


		/* Calculate plane equation d. */

		vp = pp->vptr + *(pp->vindex);
		pp->d = -(pp->norm[0]*(*vp)[0] + pp->norm[1]*(*vp)[1] + pp->norm[2]*(*vp)[2]);

		pe->data   = (CHAR *)pp;
		pe->parent = po;

		PolyElementBoundBox(pe, pp);

		pp++;
		pe++;
		}
	}

