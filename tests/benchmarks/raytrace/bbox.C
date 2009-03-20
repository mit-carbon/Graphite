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
 *	bbox.c
 *
 * DESCRIPTION
 *	This file contains routines that read, inquire, and normalize bounding
 *	boxes.
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"



/*
 * NAME
 *	InquireBoundBoxValues - return min and max bound values for each coordinate axis
 *
 * SYNOPSIS
 *	VOID	InquireBoundBoxValues(pbb, minx, miny, minz, maxx, maxy, maxz)
 *	BBOX	*pbb;				// Ptr to bounding box.
 *	REAL	*minx, *miny, *minz;		// Near planes.
 *	REAL	*maxx, *maxy, *maxz;		// Far planes.
 *
 * RETURNS
 *	Nothing.
 */

VOID	InquireBoundBoxValues(BBOX *pbb, REAL *minx, REAL *miny, REAL *minz, REAL *maxx, REAL *maxy, REAL *maxz)
	{
	*minx = pbb->dnear[0];
	*miny = pbb->dnear[1];
	*minz = pbb->dnear[2];
	*maxx = pbb->dfar[0];
	*maxy = pbb->dfar[1];
	*maxz = pbb->dfar[2];
	}



/*
 * NAME
 *	NormalizeBoundBox - normalize bounding box given a normalization matrix
 *
 * SYNOPSIS
 *	VOID	NormalizeBoundBox(bv, mat)
 *	BBOX	*pbb;				// Ptr to bounding box.
 *	MATRIX	mat;				// Normalization matrix.
 *
 * RETURNS
 *	Nothing.
 */

VOID	NormalizeBoundBox(BBOX *pbb, MATRIX mat)
	{
	POINT	tmp;

	/* dnear and dfar are only vectors of length 3 */

	tmp[0] = pbb->dnear[0];
	tmp[1] = pbb->dnear[1];
	tmp[2] = pbb->dnear[2];
	tmp[3] = 1.0;

	VecMatMult(tmp, mat, tmp);

	pbb->dnear[0] = tmp[0];
	pbb->dnear[1] = tmp[1];
	pbb->dnear[2] = tmp[2];

	tmp[0] = pbb->dfar[0];
	tmp[1] = pbb->dfar[1];
	tmp[2] = pbb->dfar[2];
	tmp[3] = 1.0;

	VecMatMult(tmp, mat, tmp);

	pbb->dfar[0] = tmp[0];
	pbb->dfar[1] = tmp[1];
	pbb->dfar[2] = tmp[2];
	}

