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
 *	matrix.c
 *
 * DESCRIPTION
 *	This file contains routines for matrix math, including common graphics
 *	related matrix operations, and also some vector operations.
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"



typedef REAL	GJMATRIX[4][8]; 	/* Matrix for Gauss-Jordan inversion.*/



/*
 * NAME
 *	VecNorm - normalize vector to unity length
 *
 * SYNOPSIS
 *	VOID	VecNorm(V)
 *	Point	V;			// Vector to normalize.
 *
 * RETURNS
 *	Nothing.
 */

VOID	VecNorm(POINT V)
	{
	REAL	l;

	l = VecLen(V);
	if (l > 0.0000001)
		{
		V[0] /= l;
		V[1] /= l;
		V[2] /= l;
		}
	}



/*
 * NAME
 *	VecMatMult - multiply a vector by a matrix
 *
 * SYNOPSIS
 *	VOID	VecMatMult(Vt, M, V)
 *	POINT	Vt;			// Transformed vector.
 *	MATRIX	M;			// Transformation matrix.
 *	POINT	V;			// Input vector.
 *
 * RETURNS
 *	Nothing.
 */

VOID	VecMatMult(POINT Vt, MATRIX M, POINT V)
	{
	INT	i, j;
	POINT	tvec;

	/* tvec = M * V */

	for (i = 0; i < 4; i++)
		{
		tvec[i] = 0.0;
		for (j = 0; j < 4; j++)
			tvec[i] += V[j] * M[j][i];
		}

	/* copy tvec to Vt */

	for (i = 0; i < 4; i++)
		Vt[i] = tvec[i];
	}



/*
 * NAME
 *	PrintMatrix - print values from matrix to stdout
 *
 * SYNOPSIS
 *	VOID	PrintMatrix(M, s)
 *	MATRIX	M;			// Matrix to print.
 *	CHAR	*s;			// Title string.
 *
 * RETURNS
 *	Nothing.
 */

VOID	PrintMatrix(MATRIX M, CHAR *s)
	{
	INT	i, j;

	printf("\n%s\n", s);

	for (i = 0; i < 4; i++)
		{
		printf("\t");

		for (j = 0; j < 4; j++)
			printf("%f  ", M[i][j]);

		printf("\n");
		}
	}



/*
 * NAME
 *	MatrixIdentity - set a matrix to the identity matrix
 *
 * SYNOPSIS
 *	VOID	MatrixIdentity(M)
 *	MATRIX	M;
 *
 * RETURNS
 *	Nothing.
 */

VOID	MatrixIdentity(MATRIX M)
	{
	INT	i, j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			M[i][j] = 0.0;

	M[0][0] = 1.0;
	M[1][1] = 1.0;
	M[2][2] = 1.0;
	M[3][3] = 1.0;
	}



/*
 * NAME
 *	MatrixCopy - copy one matrix to another
 *
 * SYNOPSIS
 *	VOID	MatrixCopy(A, B)
 *	MATRIX	A;			// Destination matrix.
 *	MATRIX	B;			// Source matrix.
 *
 * RETURNS
 *	Nothing.
 */

VOID	MatrixCopy(MATRIX A, MATRIX B)
	{
	INT	i, j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			A[i][j] = B[i][j];
	}



/*
 * NAME
 *	MatrixTranspose - transpose the elements of a matrix
 *
 * SYNOPSIS
 *	VOID	MatrixTranspose(MT, M)
 *	MATRIX	MT;			// Transposed matrix.
 *	MATRIX	M;			// Original matrix.
 *
 * RETURNS
 *	Nothing.
 */

VOID	MatrixTranspose(MATRIX MT, MATRIX M)
	{
	INT	i, j;
	MATRIX	tmp;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			tmp[j][i] = M[i][j];

	MatrixCopy(MT, tmp);
	}



/*
 * NAME
 *	MatrixMult - multiply two matrices
 *
 * SYNOPSIS
 *	VOID	MatrixMult(C, A, B)
 *	MATRIX	C, A, B;		// C = A*B
 *
 * RETURNS
 *	Nothing.
 */

VOID	MatrixMult(MATRIX C, MATRIX A, MATRIX B)
	{
	INT	i, j, k;
	MATRIX	T;			/* Temporary matrix.		     */

	/* T = A*B */

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			{
			T[i][j] = 0.0;
			for (k = 0; k < 4; k++)
				T[i][j] += A[i][k] * B[k][j];
			}

	/* copy T to C */

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			C[i][j] = T[i][j];
	}



/*
 * NAME
 *	MatrixInverse - invert matrix using Gaussian elimination with partial pivoting
 *
 * SYNOPSIS
 *	VOID	MatrixInverse(Minv, Mat)
 *	MATRIX	Minv;			// Inverted matrix.
 *	MATRIX	Mat;			// Original matrix.
 *
 * RETURNS
 *	Nothing.
 */

VOID	MatrixInverse(MATRIX Minv, MATRIX Mat)
	{
	INT		i, j, k;	/* Indices.			     */
	GJMATRIX	gjmat;		/* Inverse calculator.		     */
	REAL		tbuf[8];	/* Row holder.			     */
	REAL		pval, aval;	/* Pivot candidates.		     */
	INT		prow;		/* Pivot row number.		     */
	REAL		c;		/* Pivot scale factor.		     */
	MATRIX		tmp;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			gjmat[i][j] = Mat[i][j];


	k = 0;
	for (i = 4; i < 8; i++)
		{
		for (j = 4; j < 8; j++)
			if (i == j)
				gjmat[k][j] = 1.0;
			else
				gjmat[k][j] = 0.0;
		k++ ;
		}


	/* Gaussian elimination. */

	for (i = 0; i < 3; i++)
		{
		pval = ABS(gjmat[i][i]);
		prow = i;

		for (j = i + 1; j < 4; j++)
			{
			aval = ABS(gjmat[j][i]);
			if (aval > pval)
				{
				pval = aval;
				prow = j;
				}
			}

		if (i != prow)
			{
			for (k = 0; k < 8; k++)
				tbuf[k] = gjmat[i][k];

			for (k = 0; k < 8; k++)
				gjmat[i][k] = gjmat[prow][k];

			for (k = 0; k < 8; k++)
				gjmat[prow][k] = tbuf[k];
			}

		for (j = i + 1; j < 4; j++)
			{
			c = gjmat[j][i] / gjmat[i][i];
			gjmat[j][i] = 0.0;

			for (k = i + 1; k < 8; k++)
				gjmat[j][k] = gjmat[j][k] - c * gjmat[i][k];
			}
		}


	/*  Zero columns  */

	for (i = 0; i < 3; i++)
		for (j = i + 1; j < 4; j++)
			{
			c = gjmat[i][j] / gjmat[j][j];
			gjmat[i][j] = 0.0;

			for (k = j + 1; k < 8; k++)
				gjmat[i][k] = gjmat[i][k] - c * gjmat[j][k];
			}


	for (i = 0; i < 4; i++)
		for (k = 4; k < 8; k++) 		    /* normalize row */
			gjmat[i][k] /= gjmat[i][i];


	/*  Generate inverse matrix. */

	for (i = 0; i < 4; i++)
		for (j = 4; j < 8; j++)
			Minv[i][j - 4] = gjmat[i][j];

	MatrixMult(tmp, Mat, Minv);
	}



/*
 * NAME
 *	Translate - build translation matrix
 *
 * SYNOPSIS
 *	VOID	Translate(M, dx, dy, dz)
 *	MATRIX	M;			// New matrix.
 *	REAL	dx, dy, dz;		// Translation values.
 *
 * RETURNS
 *	Nothing.
 */

VOID	Translate(MATRIX M, REAL dx, REAL dy, REAL dz)
	{
	MatrixIdentity(M);

	M[3][0] = dx;
	M[3][1] = dy;
	M[3][2] = dz;
	}



/*
 * NAME
 *	Scale - build scaling matrix
 *
 * SYNOPSIS
 *	VOID	Scale(M, sx, sy, sz)
 *	MATRIX	M;			// New matrix.
 *	REAL	sx, sy, sz;		// Scaling factors.
 *
 * RETURNS
 *	Nothing.
 */

VOID	Scale(MATRIX M, REAL sx, REAL sy, REAL sz)
	{
	MatrixIdentity(M);

	M[0][0] = sx;
	M[1][1] = sy;
	M[2][2] = sz;
	}



/*
 * NAME
 *	Rotate - build rotation matrix
 *
 * SYNOPSIS
 *	VOID	Rotate(axis, M, angle)
 *	INT	axis;			// Axis of rotation.
 *	MATRIX	M;			// New matrix.
 *	REAL	angle;			// Angle of rotation.
 *
 * RETURNS
 *	Nothing.
 */

VOID	Rotate(INT axis, MATRIX M, REAL angle)
	{
	REAL	cosangle;
	REAL	sinangle;

	MatrixIdentity(M);

	cosangle = cos(angle);
	sinangle = sin(angle);

	switch (axis)
		{
		case X_AXIS:
			M[1][1] =  cosangle;
			M[1][2] =  sinangle;
			M[2][1] = -sinangle;
			M[2][2] =  cosangle;
			break;

		case Y_AXIS:
			M[0][0] =  cosangle;
			M[0][2] = -sinangle;
			M[2][0] =  sinangle;
			M[2][2] =  cosangle;
			break;

		case Z_AXIS:
			M[0][0] =  cosangle;
			M[0][1] =  sinangle;
			M[1][0] = -sinangle;
			M[1][1] =  cosangle;
			break;

		default:
			printf("Unknown rotation axis %ld.\n", axis);
			exit(5);
			break;
		}
	}
