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
 *	geo.c
 *
 * DESCRIPTION
 *	This file contains routines that read both ascii and binary geometry
 *	files, as well as routines to normalize, print, and make a linked
 *	list of the geometry info.
 *
 */


#include <stdio.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include "rt.h"



/*
 *	Allocate and initialize sphere function pointer array.
 */

static	PPROCS	SphProcs =
	{
	SphName,
	SphPrint,
	SphRead,
	NULL,
	SphTransform,
	SphIntersect,
	SphPeIntersect,
	SphNormal,
	SphDataNormalize,
	SphBoundBox
	};

/*
 *	Allocate and initialize polygon function pointer array.
 */

static	PPROCS	PolyProcs =
	{
	PolyName,
	PolyPrint,
	PolyRead,
	NULL,
	PolyTransform,
	PolyIntersect,
	PolyPeIntersect,
	PolyNormal,
	PolyDataNormalize,
	PolyBoundBox
	};



/*
 *	Allocate and initialize triangle function pointer array.
 */

static	PPROCS	TriProcs =
	{
	TriName,
	TriPrint,
	TriRead,
	NULL,
	TriTransform,
	TriIntersect,
	TriPeIntersect,
	TriNormal,
	TriDataNormalize,
	TriBoundBox
	};



/*
 * NAME
 *	MakeElementArray - make array of all primitive elements
 *
 * SYNOPSIS
 *	ELEMENT **MakeElementArray(totalElements)
 *	INT	*totalElements;
 *
 * RETURNS
 *	Pointer to array of primitives.
 *
 */

ELEMENT **MakeElementArray(INT *totalElements)
	{
	INT	 i;
	OBJECT	*po;				/* Ptr to object.	     */
	INT	currArrayPosition = 0;
	ELEMENT **npepa;

	po = gm->modelroot;
	*totalElements = 0;

	while (po)
		{
		(*totalElements) += po->numelements;
		po = po->next;
		}

	po    = gm->modelroot;
	npepa = ObjectMalloc(OT_PEPARRAY, *totalElements);

	while (po)
		{
		for (i = 0; i < po->numelements; i++)
			npepa[currArrayPosition++] = po->pelem + i;

		po = po->next;
		}

	return (npepa);
	}



/*
 * NAME
 *	PrintGeo - print object geometry list
 *
 * SYNOPSIS
 *	VOID	PrintGeo(po)
 *	OBJECT	*po;			// Ptr to object list to print.
 *
 * RETURNS
 *	Nothing.
 */

VOID	PrintGeo(OBJECT *po)
	{
	while (po)
		{
		fprintf(stdout, "Object  %s\n", po->name);
		fprintf(stdout, "\tcolor  %f %f %f %f %f %f\n",
			po->surf->fcolor[0], po->surf->fcolor[1],
			po->surf->fcolor[2], po->surf->bcolor[0],
			po->surf->bcolor[1], po->surf->bcolor[2]);

		fprintf(stdout, "\tcoeffs %f %f %f %f %f\n",
			po->surf->kdiff, po->surf->kspec, po->surf->ktran,
			po->surf->refrindex, po->surf->kspecn);

		(*po->procs->print)(po);
		po = po->next;
		}
	}



/*
 * NAME
 *	NormalizeGeo - normalize geometry to be within unit cube
 *
 * SYNOPSIS
 *	VOID	NormalizeData(po, model, modelInvT)
 *	OBJECT	*po;			// Ptr to object list to normalize.
 *	MATRIX	model;			// Model matrix.
 *	MATRIX	modelInvT;		// Model inverse transpose.
 *
 * RETURNS
 *	Nothing.
 */

VOID	NormalizeGeo(OBJECT *po, MATRIX model, MATRIX modelInvT)
	{
	REAL	norm_minx;			/* Normalize min values.     */
	REAL	norm_miny;
	REAL	norm_minz;
	REAL	norm_maxx;			/* Normalize max values.     */
	REAL	norm_maxy;
	REAL	norm_maxz;
	REAL	scale_min;			/* Object min max values.    */
	REAL	scale_max;
	REAL	scale;				/* Normalization scale val.  */
	REAL	trans;				/* Normalization xlat val.   */
	OBJECT	*poHead;			/* Ptr to head of object list*/
	MATRIX	normMat;			/* Normalization matrix.     */
	MATRIX	tempMat;			/* Temporary work matrix.    */


	poHead = po;				/* Save ptr to head of list.  */

	if (! (TraversalType == TT_LIST && ModelNorm == FALSE) )
		{
		/* Find global bound box min/max. */

		norm_minx = norm_miny = norm_minz =  HUGE_REAL;
		norm_maxx = norm_maxy = norm_maxz = -HUGE_REAL;

		while (po)
			{
			norm_minx = Min(norm_minx, po->bv.dnear[0]);
			norm_miny = Min(norm_miny, po->bv.dnear[1]);
			norm_minz = Min(norm_minz, po->bv.dnear[2]);
			norm_maxx = Max(norm_maxx, po->bv.dfar[0]);
			norm_maxy = Max(norm_maxy, po->bv.dfar[1]);
			norm_maxz = Max(norm_maxz, po->bv.dfar[2]);

			po = po->next;
			}


		/* Compute scale factor. */

		scale_min = Min(norm_minx, norm_miny);
		scale_min = Min(scale_min, norm_minz);
		scale_max = Max(norm_maxx, norm_maxy);
		scale_max = Max(scale_max, norm_maxz);

		scale = 1.0/(scale_max - scale_min);
		trans = (-scale_min*scale);

		Scale(tempMat, scale, scale, scale);
		Translate(normMat, trans, trans, trans);
		MatrixMult(normMat, tempMat, normMat);


		/* Now, normalize object data. */

		po = poHead;

		while (po)
			{
			(*po->procs->normalize)(po, normMat);
			po = po->next;
			}
		}
	}



/*
 * NAME
 *	ReadGeoFile - read in the geometry file
 *
 * SYNOPSIS
 *	VOID	ReadGeoFile(GeoFileName)
 *	CHAR	*GeoFileName;		// File to open.
 *
 * DESCRIPTION
 *	Read in the model file - call primitive object routines to read in
 *	their own data.  Objects are put in a linked list.
 *
 *	Transform objects by modeling transformation if any.
 *
 *	If TT_LIST traversal, ModelNorm parameter indicates whether data will
 *	be normalized.
 *
 * RETURNS
 *	Nothing.
 */

VOID	ReadGeoFile(CHAR *GeoFileName)
	{
	INT		i;
	INT		dummy;
	INT		stat;			/* Input read counter.	     */
	CHAR		comchar;
	CHAR		primop; 		/* Primitive opcode.	     */
	CHAR		objstr[NAME_LEN];	/* Object descriptions.      */
	CHAR		objname[NAME_LEN];
	FILE		*pf;			/* Ptr to geo file desc.     */
	SURF		*ps;			/* Ptr to surface desc.      */
	MATRIX		model;			/* Model matrix.	     */
	MATRIX		modelInv;		/* Model matrix inverse.     */
	MATRIX		modelInvT;		/* Model matrix inv transpose*/
	OBJECT		*prev;			/* Ptr to previous object.   */
	OBJECT		*curr;			/* Ptr to current object.    */
	ELEMENT 	*pe;			/* Ptr to the element list.  */


	/* Open the model file. */

	pf = fopen(GeoFileName, "r");
	if (!pf)
		{
		printf("Unable to open model file %s.\n", GeoFileName);
		exit(1);
		}


	/* Initialize pointers and counters. */

	curr	      = NULL;
	prev	      = NULL;
	gm->modelroot = NULL;
	prim_obj_cnt  = 0;
	prim_elem_cnt = 0;


	/* Retrieve model transform. */

	MatrixCopy(model, View.model);
	MatrixInverse(modelInv, model);
	MatrixTranspose(modelInvT, modelInv);


	/* Check for comments. */

	if ((comchar = getc(pf)) != '#')
		ungetc(comchar, pf);
	else
		{
		comchar = '\0';                 /* Set to other than '#'.    */

		while (comchar != '#')
			if ((comchar = getc(pf)) == EOF)
				{
				fprintf(stderr, "Incorrect comment in geometry file.\n");
				exit(-1);
				}
		}


	/* Process the file data for each object. */

	while ((stat = fscanf(pf, "%s %s", objstr, objname)) != EOF)
		{
		if (stat != 2 || strcmp(objstr, "object") != 0)
			{
			printf("Invalid object definition %s.\n", objstr);
			exit(-1);
			}


		/* Allocate next object and set list pointers. */

		prim_obj_cnt++;

		curr	    = GlobalMalloc(sizeof(OBJECT), "geo.c");
		curr->index = prim_obj_cnt;
		curr->next  = NULL;

		strcpy(curr->name, objname);

		if (gm->modelroot == NULL)
			gm->modelroot = curr;
		else
			prev->next = curr;


		/* Get surface characteristics. */

		ps = GlobalMalloc(sizeof(SURF), "geo.c");
		curr->surf = ps;

		stat = fscanf(pf, "%lf %lf %lf %lf %lf %lf",
			    &(ps->fcolor[0]), &(ps->fcolor[1]), &(ps->fcolor[2]),
			    &(ps->bcolor[0]), &(ps->bcolor[1]), &(ps->bcolor[2]));

		if (stat != 6)
			{
			printf("Object color incorrect.\n");
			exit(-1);
			}

		stat = fscanf(pf, "%lf %lf %lf %lf %lf",
			    &(ps->kdiff), &(ps->kspec), &(ps->ktran),
			    &(ps->refrindex), &(ps->kspecn));

		if (stat != 5)
			{
			printf("Object surface coefficients incorrect.\n");
			exit(-1);
			}


		/* Get texture and flag information. */

		stat = fscanf(pf, "%ld %ld %ld %ld\n", &dummy, &dummy, &dummy, &dummy);

		if (stat != 4)
			{
			printf("Texture and/or flag information not all present.\n");
			exit(-1);
			}


		/* Get primitive opcode. */

		stat = fscanf(pf, "%c %ld", &primop, &(curr->numelements));

		if (stat != 2)
			{
			printf("Object primitive opcode.\n");
			exit(-1);
			}

		switch (primop)
			{
			case 's':
				curr->procs = &SphProcs;
				break;

			case 'p':
				curr->procs = &PolyProcs;
				break;

			case 't':
				curr->procs = &TriProcs;
				break;

			case 'c':
			case 'q':
				printf("Code for cylinders and quadrics not implemented yet.\n");
				exit(-1);

			default:
				printf("Invalid primitive type \'%c\'.\n", primop);
				exit(-1);
			}


		/* Allocate primitive elements and create indices. */

		pe = GlobalMalloc(sizeof(ELEMENT)*curr->numelements, "geo.c");
		curr->pelem = pe;

		prim_elem_cnt += curr->numelements;

		for (i = 1; i <= curr->numelements; i++, pe++)
			pe->index = i;


		/* Read, transform, and compute bounding box for object. */

		(*curr->procs->read)(curr, pf);
		(*curr->procs->transform)(curr, model, modelInvT);
		(*curr->procs->bbox)(curr);

		prev = curr;
		}

	NormalizeGeo(gm->modelroot, model, modelInvT);
	fclose(pf);
	}

