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
 *	tri.c
 *
 * DESCRIPTION
 *	This file contains all routines that operate on triangle objects.
 *
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"



#define X_NORM			1
#define Y_NORM			2
#define Z_NORM			3

#define CLOCKWISE		1
#define COUNTER_CLOCKWISE	2

#define INSIDE(x, a)		(((a) <= 0.0 && (x) >= (a) && (x) <= 0.0) || \
				((a)  >  0.0 && (x) >= 0.0 && (x) <= (a)))


/*
 * NAME
 *	TriName - return the object name
 *
 * SYNOPSIS
 *	CHAR	*TriName()
 *
 * RETURNS
 *	A pointer to the name string.
 */

CHAR	*TriName()
	{
	return ("poly");
	}



/*
 * NAME
 *	TriPrint - print the triangle object data to stdout
 *
 * SYNOPSIS
 *	VOID	TriPrint(po)
 *	OBJECT	*po;			// Ptr to triangle object.
 *
 * RETURNS
 *	Nothing.
 */

VOID	TriPrint(OBJECT *po)
	{
	INT	i, j;
	INT	*vindex;		/* Ptr to vertex index. 	     */
	VEC3	*vlist, *vptr;		/* Ptr to vertex list.		     */
	TRI	*pt;			/* Ptr to triangle data.	     */
	ELEMENT *pe;			/* Ptr to triangle element.	     */

	pe = po->pelem;
	fprintf(stderr, "\ttriangle: %ld triangles.\n", po->numelements);

	for (i = 0; i < po->numelements; i++)
		{
		pt = (TRI *)pe->data;

		fprintf(stderr, "\t\tVertices: 3   Plane eq: %f %f %f %f\n", pt->norm[0], pt->norm[1], pt->norm[2], pt->d);

		vlist  = pt->vptr;
		vindex = pt->vindex;

		for (j = 0; j < 3; j++)
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
 *	TriElementBoundBox - compute triangle element bounding box
 *
 * SYNOPSIS
 *	VOID	TriElementBoundBox(pe, pt)
 *	ELEMENT *pe;			// Ptr to triangle element.
 *	TRI	*pt;			// Ptr to triangle data.
 *
 * RETURNS
 *	Nothing.
 */

VOID	TriElementBoundBox(ELEMENT *pe, TRI *pt)
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

	vlist  = pt->vptr;
	vindex = pt->vindex;

	for (i = 0; i < 3; i++)
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
 *	TriBoundBox - compute triangle object bounding box
 *
 * SYNOPSIS
 *	VOID	TriBoundBox(po)
 *	OBJECT	*po;			// Ptr to triangle object.
 *
 * RETURNS
 *	Nothing.
 */

VOID	TriBoundBox(OBJECT *po)
	{
	INT	i;
	TRI	*pt;			/* Ptr to triangle data.	     */
	ELEMENT *pe;			/* Ptr to triangle element.	     */
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
		pt = (TRI *)pe->data;
		TriElementBoundBox(pe, pt);

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
 *	TriNormal - compute triangle unit normal at given point on the surface
 *
 * SYNOPSIS
 *	VOID	TriNormal(hit, Pi, Ni)
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

VOID	TriNormal(IRECORD *hit, POINT Pi, POINT Ni)
	{
	ELEMENT *pe;
	TRI	*pt;			/* Ptr to triangle data.	     */
	VEC3	*pn;			/* Ptr to normal.		     */
	VEC3	*n0, *n1, *n2;

	/* Retrieve normal from hit triangle. */

	pe = hit->pelem;
	pt = (TRI *)pe->data;
	pn = pt->nptr;

	if (pt->norminterp)
		{
		if (pt->vorder == CLOCKWISE)
			{
			n0 = pn + pt->vindex[0];
			n1 = pn + pt->vindex[1];
			n2 = pn + pt->vindex[2];
			}
		else
			{
			n0 = pn + pt->vindex[0];
			n1 = pn + pt->vindex[2];
			n2 = pn + pt->vindex[1];
			}

		switch (pt->indx)
			{
			case X_NORM:
				hit->b1 /= pt->norm[0];
				hit->b2 /= pt->norm[0];
				hit->b3 /= pt->norm[0];
				break;

			case Y_NORM:
				hit->b1 /= pt->norm[1];
				hit->b2 /= pt->norm[1];
				hit->b3 /= pt->norm[1];
				break;

			case Z_NORM:
				hit->b1 /= pt->norm[2];
				hit->b2 /= pt->norm[2];
				hit->b3 /= pt->norm[2];
				break;
			}

		Ni[0] = hit->b1 * (*n0)[0] + hit->b2 * (*n1)[0] + hit->b3 * (*n2)[0];
		Ni[1] = hit->b1 * (*n0)[1] + hit->b2 * (*n1)[1] + hit->b3 * (*n2)[1];
		Ni[2] = hit->b1 * (*n0)[2] + hit->b2 * (*n1)[2] + hit->b3 * (*n2)[2];

		VecNorm(Ni);
		}
	else
		VecCopy(Ni, pt->norm);
	}



/*
 * NAME
 *	TriDataNormalize - normalize triangle data and bounding box
 *
 * SYNOPSIS
 *	VOID	TriDataNormalize(po, normMat)
 *	OBJECT	*po;			// Ptr to triangle object.
 *	MATRIX	 normMat;		// Normalization matrix.
 *
 * RETURNS
 *	Nothing.
 */

VOID	TriDataNormalize(OBJECT *po, MATRIX normMat)
	{
	INT	i;
	POINT	coord;
	VEC3	*pv;			/* Ptr to vertex list.		     */
	TRI	*pt;			/* Ptr to triangle data.	     */
	ELEMENT *pe;			/* Ptr to triangle element.	     */


	/* Normalize bounding box. */

	pe = po->pelem;
	NormalizeBoundBox(&po->bv, normMat);


	/* Normalize vertex list. */

	pt = (TRI *)pe->data;
	pv = pt->vptr;

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
		pt = (TRI *)pe->data;
		NormalizeBoundBox(&pe->bv, normMat);

		/* Find new d of plane equation. */

		pv    =   pt->vptr + (*(pt->vindex));
		pt->d = -(pt->norm[0]*(*pv)[0] + pt->norm[1]*(*pv)[1] + pt->norm[2]*(*pv)[2]);

		pe++;
		}
	}



/*
 * NAME
 *	TriPeIntersect - calculate intersection of ray with triangle
 *
 * SYNOPSIS
 *	INT	TriPeIntersect(pr, pe, hit)
 *	RAY	*pr;				// Ptr to incident ray.
 *	ELEMENT *pe;				// Triangle object
 *	IRECORD *hit;				// Intersection record.
 *
 * NOTES
 *	Check first to see if ray is parallel to plane or if the intersection
 *	point with the plane is behind the eye.
 *
 * RETURNS
 *	The number of intersection points.
 */

INT	TriPeIntersect(RAY *pr, ELEMENT *pe, IRECORD *hit)
	{
	REAL	Rd_dot_Pn;		/* Polygon normal dot ray direction. */
	REAL	Ro_dot_Pn;		/* Polygon normal dot ray origin.    */
	REAL	q1, q2;
	REAL	tval;			/* Intersection t distance value.    */
	VEC3	*v1, *v2, *v3;		/* Vertex list pointers.	     */
	VEC3	e1, e2, e3;		/* Edge vectors.		     */
	TRI	*pt;			/* Ptr to triangle data.	     */


	pt = (TRI *)pe->data;

	Rd_dot_Pn = VecDot(pt->norm, pr->D);

	if (ABS(Rd_dot_Pn) < RAYEPS)		/* Ray is parallel.	     */
		return (0);

	Ro_dot_Pn = VecDot(pt->norm, pr->P);

	tval = -(pt->d + Ro_dot_Pn)/Rd_dot_Pn;	/* Intersection distance.    */
	if (tval < RAYEPS)			/* Intersects behind ray.    */
		return (0);


	/*
	 *	This algorithm works for vertices in counter-clockwise order,
	 *	so reverse the vertices if they are clockwise.
	 */

	v1 = pt->vptr + pt->vindex[0];	/* Compute first vertex pointer.     */

	if (pt->vorder == COUNTER_CLOCKWISE)
		{
		v2 = pt->vptr + pt->vindex[2];
		v3 = pt->vptr + pt->vindex[1];
		}
	else
		{
		v2 = pt->vptr + pt->vindex[1];
		v3 = pt->vptr + pt->vindex[2];
		}

	e1[0] = (*v2)[0] - (*v1)[0];
	e1[1] = (*v2)[1] - (*v1)[1];
	e1[2] = (*v2)[2] - (*v1)[2];

	e2[0] = (*v3)[0] - (*v2)[0];
	e2[1] = (*v3)[1] - (*v2)[1];
	e2[2] = (*v3)[2] - (*v2)[2];

	e3[0] = (*v1)[0] - (*v3)[0];
	e3[1] = (*v1)[1] - (*v3)[1];
	e3[2] = (*v1)[2] - (*v3)[2];


	/* Triangle containment. */

	switch (pt->indx)
		{
		case X_NORM:
			q1 = pr->P[1] + tval*pr->D[1];
			q2 = pr->P[2] + tval*pr->D[2];

			hit->b1 = e2[1] * (q2 - (*v2)[2]) - e2[2] * (q1 - (*v2)[1]);
			if (!INSIDE(hit->b1, pt->norm[0]))
				return (0);

			hit->b2 = e3[1] * (q2 - (*v3)[2]) - e3[2] * (q1 - (*v3)[1]);
			if (!INSIDE(hit->b2, pt->norm[0]))
				return (0);

			hit->b3 = e1[1] * (q2 - (*v1)[2]) - e1[2] * (q1 - (*v1)[1]);
			if (!INSIDE(hit->b3, pt->norm[0]))
				return (0);
			break;

		case Y_NORM:
			q1 = pr->P[0] + tval*pr->D[0];
			q2 = pr->P[2] + tval*pr->D[2];

			hit->b1 = e2[2] * (q1 - (*v2)[0]) - e2[0] * (q2 - (*v2)[2]);
			if (!INSIDE(hit->b1, pt->norm[1]))
				return (0);

			hit->b2 = e3[2] * (q1 - (*v3)[0]) - e3[0] * (q2 - (*v3)[2]);
			if (!INSIDE(hit->b2, pt->norm[1]))
				return (0);

			hit->b3 = e1[2] * (q1 - (*v1)[0]) - e1[0] * (q2 - (*v1)[2]);
			if (!INSIDE(hit->b3, pt->norm[1]))
				return (0);
			break;

		case Z_NORM:
			q1 = pr->P[0] + tval*pr->D[0];
			q2 = pr->P[1] + tval*pr->D[1];

			hit->b1 = e2[0] * (q2 - (*v2)[1]) - e2[1] * (q1 - (*v2)[0]);
			if (!INSIDE(hit->b1, pt->norm[2]))
				return (0);

			hit->b2 = e3[0] * (q2 - (*v3)[1]) - e3[1] * (q1 - (*v3)[0]);
			if (!INSIDE(hit->b2, pt->norm[2]))
				return (0);

			hit->b3 = e1[0] * (q2 - (*v1)[1]) - e1[1] * (q1 - (*v1)[0]);
			if (!INSIDE(hit->b3, pt->norm[2]))
				return (0);
			break;
		}


	IsectAdd(hit, tval, pe);
	return (1);
	}



/*
 * NAME
 *	TriIntersect - call triangle object intersection routine
 *
 * SYNOPSIS
 *	INT	TriIntersect(pr, po, hit)
 *	RAY	*pr;			// Ptr to incident ray.
 *	OBJECT	*po;			// Ptr to triangle object.
 *	IRECORD *hit;			// Intersection record.
 *
 * RETURNS
 *	The number of intersections found.
 */

INT	TriIntersect(RAY *pr, OBJECT *po, IRECORD *hit)
	{
	INT	i;
	INT	nhits;			/* # hits in polyhedra. 	     */
	ELEMENT *pe;			/* Ptr to triangle element.	     */
	IRECORD newhit; 		/* Hit list.			     */

	/* Traverse triangle list to find intersections. */

	nhits	 = 0;
	pe	 = po->pelem;
	hit[0].t = HUGE_REAL;

	for (i = 0; i < po->numelements; i++)
		{
		if (TriPeIntersect(pr, pe, &newhit))
			{
			nhits++;
			if (newhit.t < hit[0].t)
				hit[0] = newhit;
			}

		pe++;
		}

	return (nhits);
	}



/*
 * NAME
 *	TriTransform - transform triangle object by a transformation matrix
 *
 * SYNOPSIS
 *	VOID	TriTransform(po, xtrans, xinvT)
 *	OBJECT	*po;			// Ptr to triangle object.
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

VOID	TriTransform(OBJECT *po, MATRIX xtrans, MATRIX xinvT)
	{
	INT	i;			/* Indices.			     */
	INT	numelems;		/* # of elements.		     */
	INT	*vindex;		/* Vertex index pointer.	     */
	VEC3	*vptr, *vp;		/* Vertex list pointers.	     */
	VEC3	*nptr, *np;		/* Normal list pointers.	     */
	VEC3	*vp1, *vp2, *vp3;
	VEC3	vec1, vec2;
	VEC4	pnorm;
	VEC4	norm, coord;		/* Transform 4 vectors. 	     */
	TRI	*pt;			/* Ptr to triangle data.	     */
	ELEMENT *pe;			/* Ptr to triangle element.	     */


	pe = po->pelem;
	numelems = po->numelements;

	/* Get vertex list address and transform vertices. */

	pt   = (TRI *)pe->data;
	vptr = pt->vptr;
	nptr = pt->nptr;

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

		/* Transform normals. */

		norm[0] = (*nptr)[0];
		norm[1] = (*nptr)[1];
		norm[2] = (*nptr)[2];
		norm[3] = 0.0;

		VecMatMult(norm, xinvT, norm);
		VecNorm(norm);

		(*nptr)[0] = norm[0];
		(*nptr)[1] = norm[1];
		(*nptr)[2] = norm[2];

		nptr++;
		}


	/* Transform triangle list. */

	for (i = 0; i < numelems; i++)
		{
		pt = (TRI *)pe->data;

		vindex = pt->vindex;
		vptr   = pt->vptr;
		vp1    = vptr + (*vindex);

		vindex++;
		vp2 = vptr + (*vindex);
		VecSub(vec1, (*vp2), (*vp1));

		vindex++;
		vp3 = vptr + (*vindex);
		VecSub(vec2, (*vp3), (*vp1));

		VecCross(pt->norm, vec1, vec2);

		/* Calculate plane equation variable D. */

		vp = pt->vptr + *(pt->vindex);

		pt->d = -(pt->norm[0]*(*vp)[0] + pt->norm[1]*(*vp)[1] + pt->norm[2]*(*vp)[2]);

		if (pt->norminterp)
			{
			vindex = pt->vindex;
			np = pt->nptr + (*vindex);

			VecCopy(pnorm, pt->norm);
			VecNorm(pnorm);

			if (VecDot(pnorm, (*np)) >= 0.0)
				pt->vorder = CLOCKWISE;
			else
				{
				pt->vorder = COUNTER_CLOCKWISE;
				VecScale(pt->norm, -1.0, pt->norm);
				pt->d = -pt->d;
				}
			}


		/* Set best axis projection. */

		norm[0] = ABS(pt->norm[0]);
		norm[1] = ABS(pt->norm[1]);
		norm[2] = ABS(pt->norm[2]);

		if (norm[0] >= norm[1] && norm[0] >= norm[2])
			pt->indx = X_NORM;
		else
		if (norm[1] >= norm[0] && norm[1] >= norm[2])
			pt->indx = Y_NORM;
		else
			pt->indx = Z_NORM;

		pe++;
		}
	}



/*
 * NAME
 *	TriRead - read triangle object data from file
 *
 * SYNOPSIS
 *	VOID	TriRead(po, pf)
 *	OBJECT	*po;			// Ptr to triangle object.
 *	FILE	*pf;			// Ptr to file.
 *
 * RETURNS
 *	Nothing.
 */

VOID	TriRead(OBJECT *po, FILE *pf)
	{
	INT	i;			/* Indices.			     */
	INT	instat; 		/* Read status. 		     */
	INT	totalverts;		/* Total # of vertices in tri mesh.  */
	CHAR	normstr[5];		/* Face/vertex normal flag string.   */
	BOOL	pnormals;		/* Face normals present?	     */
	BOOL	vnormals;		/* Vertex normals present?	     */
	VEC3	*vlist, *vptr;		/* Ptr to vertex list.		     */
	VEC3	*nlist, *nptr;		/* Ptr to normal list.		     */
	TRI	*pt;			/* Ptr to triangle data.	     */
	ELEMENT *pe;			/* Ptr to triangle element.	     */

	pe = po->pelem;

	/* Allocate space for object data. */

	instat = fscanf(pf, "%ld", &totalverts);

	if (instat != 1)
		{
		printf("Error in TriRead: totalverts.\n");
		exit(-1);
		}

	pt    = GlobalMalloc(sizeof(TRI )*po->numelements,  "tri.c");
	vlist = GlobalMalloc(sizeof(VEC3)*(totalverts + 1), "tri.c");
	nlist = GlobalMalloc(sizeof(VEC3)*totalverts,	    "tri.c");

	vptr  = vlist;
	nptr  = nlist;


	/* Are triangle face normals supplied? */

	instat = fscanf(pf, "%s\n", normstr);

	if (instat != 1)
		{
		printf("Error in TriRead: face normal indicator.\n");
		exit(-1);
		}

	pnormals = (normstr[2] == 'y' ? TRUE : FALSE);


	/* Are vertex normal supplied? */

	instat = fscanf(pf, "%s\n", normstr);
	if (instat != 1)
		{
		printf("Error in TriRead: vertex normal indicator.\n");
		exit(-1);
		}

	vnormals = (normstr[2] == 'y' ? TRUE : FALSE);


	/* Read vertex list. */

	for (i = 0; i < totalverts; i++)
		{
		instat = fscanf(pf, "%lf %lf %lf", &(*vptr)[0], &(*vptr)[1], &(*vptr)[2]);

		if (instat != 3)
			{
			printf("Error in TriRead: vertex %ld.\n", i);
			exit(-1);
			}

		if (vnormals)
			{
			instat = fscanf(pf, "%lf %lf %lf", &(*nptr)[0], &(*nptr)[1], &(*nptr)[2]);

			if (instat != 3)
				{
				printf("Error in TriRead: vertex normal %ld.\n", i);
				exit(-1);
				}

			nptr++;
			}

		vptr++;
		}


	(*vptr)[0] = HUGE_REAL;
	(*vptr)[1] = HUGE_REAL;
	(*vptr)[2] = HUGE_REAL;


	/* Read triangle list. */

	for (i = 0; i < po->numelements; i++)
		{
		if (pnormals)
			{
			instat = fscanf(pf, " %lf %lf %lf", &(pt->norm[0]), &(pt->norm[1]), &(pt->norm[2]));

			if (instat != 3)
				{
				printf("Error in TriRead: face normal %ld.\n", i);
				exit(-1);
				}
			}

		pt->vptr       = vlist;
		pt->nptr       = nlist;
		pt->norminterp = vnormals;

		instat = fscanf(pf, "%ld %ld %ld", &(pt->vindex[0]), &(pt->vindex[1]), &(pt->vindex[2]));

		if (instat != 3)
			{
			printf("Error in TriRead: vertex index %ld.\n", i);
			exit(-1);
			}

		pe->data   = (CHAR *)pt;
		pe->parent = po;

		TriElementBoundBox(pe, pt);

		pt++;
		pe++;
		}

	/* Free normal storage if not interpolating. */

	if (!vnormals)
		GlobalFree(nlist);
	}

