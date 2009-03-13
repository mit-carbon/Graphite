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

/******************************************************************************
 *                                                                             *
 *   view.c:  Compute viewing direction.                                       *
 *                                                                             *
 ******************************************************************************/

#include "incl.h"

#define XAXIS		1	/* Positive X-axis points rightwards  */
#define YAXIS		2	/* Positive Y-axis points upwards     */
#define ZAXIS		3	/* Positive Z-axis points into screen */

float transformation_matrix[4][4];/* current transformation matrix of floats */
float out_invvertex[2][2][2][NM]; /* Image and object space centers          */
/*   of outer voxels in output map         */
float uout_invvertex[2][2][2][NM];/* Image and object space vertices         */
/*   of output map unit voxel              */

long frust_len;		/* Size of clipping frustum                  */
/*   (mins will be 0 in this program,        */
/*    {x,y}len will be <= IK{X,Y}SIZE)       */
float out_diag_len[NM];   	/* Per-axis lengths and combined length of   */
float out_diag_length;	        /*   diagonal of input map in image space    */
float depth_cueing[MAX_OUTLEN];	/* Pre-computed table of depth cueing        */
long image_zlen;			/*   number of samples along viewing ray     */
float in_max[NM];               /* Pre-computed clipping aids                */
long opc_xlen,opc_xylen;
long norm_xlen,norm_xylen;

float invmatrix[4][4];		/* Inverse of viewing matrix:                */
/* 4 x 4 viewing transformation matrix       */
/*   (orthographic projection used, so       */
/*    matrix[][3] = 0, matrix[3][3] = 1)     */

EXTERN_ENV

void Compute_Pre_View()
{
    long i, outz;

    for (i=0; i<NM; i++)
        out_diag_len[i] = opc_len[i]-1;
    out_diag_length = out_diag_len[X]*out_diag_len[X] +
        out_diag_len[Y]*out_diag_len[Y] +
            out_diag_len[Z]*out_diag_len[Z];
    out_diag_length = sqrt(out_diag_length);
    frust_len = NINT(out_diag_length)+1;
    image_zlen = frust_len - 1;

    /* Pre-compute table of depth cueing attenuation fractions as        */
    /* exponential falloff from intensity of depth_hither*full at        */
    /* hither (outz=0) to depth_yon*full at yon (frust_len-1).           */
    /* Exponent > 1.0 falls off slower than linear, < 1.0 falls faster.  */
    /* Clip fractions to valid range to allow agressive user             */
    /* to set hither or yon outside the range 0.0..1.0.                  */
    if (image_zlen > MAX_OUTLEN)
        Error ("MAX_OUTLEN exceeded in Ray_Trace.\n");
    for (outz=0; outz<=image_zlen; outz++) {
        depth_cueing[outz] = depth_hither -
            pow((float)(outz)/(float)image_zlen,depth_exponent) *
                (depth_hither - depth_yon);
        depth_cueing[outz] = MIN(MAX(depth_cueing[outz],0.0),1.0);
    }

    /* Pre-compute clipping aids                                         */
    in_max[X] = (float)(opc_len[X]-1)-SMALL-1.0/(float)MAX_PIXEL;
    in_max[Y] = (float)(opc_len[Y]-1)-SMALL-1.0/(float)MAX_PIXEL;
    in_max[Z] = (float)(opc_len[Z]-1)-SMALL-1.0/(float)MAX_PIXEL;

    /* Pre-compute subscripting aids                                     */
    opc_xlen = opc_len[X] + 1;
    opc_xylen = opc_len[X] * opc_len[Y] + 1;

    norm_xlen = norm_len[X] + 1;
    norm_xylen = norm_len[X] * norm_len[Y] + 1;
}


void Select_View(double delta_angle,long axis)
{
    Load_Identity_Matrix(invmatrix);

    /* Moves input map from all-positive octant to center of     */
    /* coordinate system so subsequent rotations spin object     */
    /* in place.  Must be first after initialization to work.    */
    Inverse_Concatenate_Translation(invmatrix,
                                    (float)(out_diag_len[X])/2.0,
                                    (float)(out_diag_len[Y])/2.0,
                                    (float)(out_diag_len[Z])/2.0);

    /* scale dataset size in Z-direction for 256x256x113 dataset */
    Inverse_Concatenate_Scaling(invmatrix,1.0,1.0,1.0/(float)ZSCALE);

    /* rotation about axis by angle */
    if (frame != 0)
        angle[axis] = angle[axis] + delta_angle;
    Inverse_Concatenate_Rotation(invmatrix,XAXIS,-angle[X]);
    Inverse_Concatenate_Rotation(invmatrix,YAXIS,-angle[Y]);
    /*  Inverse_Concatenate_Rotation(invmatrix,ZAXIS,-angle[Z]);*/
    Inverse_Concatenate_Rotation(invmatrix,XAXIS,30.0);

    /* Moves input map from center of coordinate system back     */
    /* to within all-positive octant such that any rotation      */
    /* (e.g. by using pre-matrix, rotations, and anpost-matrix)  */
    /* exactly falls in the octant, preventing low-side clipping.*/
    /* Fails if non-isotropic image space scaling is applied     */
    /* (i.e. different scaling in X,Y, or Z after rotations).    */
    Inverse_Concatenate_Translation(invmatrix,
                                    -out_diag_length/2.0,
                                    -out_diag_length/2.0,
                                    -out_diag_length/2.0);

    /* Insures that frustum entirely encloses any rotation of    */
    /* map assuming they fall within the all-positive octant     */
    /* (e.g. by using pre-matrix, rotations, and anpost-matrix), */
    Load_Transformation_Matrix(invmatrix);
    Compute_Input_Dimensions();
    Compute_Input_Unit_Vector();
}


void Compute_Input_Dimensions()
{
    long x,y,z;
    float in_invvertex[2][2][2][NM];	/* Image and object space centers    */

    in_invvertex[0][0][0][X] = 0;
    in_invvertex[0][0][0][Y] = 0;
    in_invvertex[0][0][0][Z] = 0;

    in_invvertex[0][0][1][X] = frust_len-1;
    in_invvertex[0][0][1][Y] = 0;
    in_invvertex[0][0][1][Z] = 0;

    in_invvertex[0][1][0][X] = 0;
    in_invvertex[0][1][0][Y] = frust_len-1;
    in_invvertex[0][1][0][Z] = 0;

    in_invvertex[0][1][1][X] = frust_len-1;
    in_invvertex[0][1][1][Y] = frust_len-1;
    in_invvertex[0][1][1][Z] = 0;

    in_invvertex[1][0][0][X] = 0;
    in_invvertex[1][0][0][Y] = 0;
    in_invvertex[1][0][0][Z] = frust_len-1;

    in_invvertex[1][0][1][X] = frust_len-1;
    in_invvertex[1][0][1][Y] = 0;
    in_invvertex[1][0][1][Z] = frust_len-1;

    in_invvertex[1][1][0][X] = 0;
    in_invvertex[1][1][0][Y] = frust_len-1;
    in_invvertex[1][1][0][Z] = frust_len-1;

    in_invvertex[1][1][1][X] = frust_len-1;
    in_invvertex[1][1][1][Y] = frust_len-1;
    in_invvertex[1][1][1][Z] = frust_len-1;

    for (z=0; z<2; z++) {
        for (y=0; y<2; y++) {
            for (x=0; x<2; x++) {
                Transform_Point(in_invvertex[z][y][x][X],
                                in_invvertex[z][y][x][Y],
                                in_invvertex[z][y][x][Z],
                                &out_invvertex[z][y][x][X],
                                &out_invvertex[z][y][x][Y],
                                &out_invvertex[z][y][x][Z]);
            }
        }
    }
}


void Compute_Input_Unit_Vector()
{
    long x,y,z;
    float uin_invvertex[2][2][2][NM];

    uin_invvertex[0][0][0][X] = 0;
    uin_invvertex[0][0][0][Y] = 0;
    uin_invvertex[0][0][0][Z] = 0;

    uin_invvertex[0][0][1][X] = 1.0;
    uin_invvertex[0][0][1][Y] = 0;
    uin_invvertex[0][0][1][Z] = 0;

    uin_invvertex[0][1][0][X] = 0;
    uin_invvertex[0][1][0][Y] = 1.0;
    uin_invvertex[0][1][0][Z] = 0;

    uin_invvertex[0][1][1][X] = 1.0;
    uin_invvertex[0][1][1][Y] = 1.0;
    uin_invvertex[0][1][1][Z] = 0;

    uin_invvertex[1][0][0][X] = 0;
    uin_invvertex[1][0][0][Y] = 0;
    uin_invvertex[1][0][0][Z] = 1.0;

    uin_invvertex[1][0][1][X] = 1.0;
    uin_invvertex[1][0][1][Y] = 0;
    uin_invvertex[1][0][1][Z] = 1.0;

    uin_invvertex[1][1][0][X] = 0;
    uin_invvertex[1][1][0][Y] = 1.0;
    uin_invvertex[1][1][0][Z] = 1.0;

    uin_invvertex[1][1][1][X] = 1.0;
    uin_invvertex[1][1][1][Y] = 1.0;
    uin_invvertex[1][1][1][Z] = 1.0;

    for (z=0; z<2; z++) {
        for (y=0; y<2; y++) {
            for (x=0; x<2; x++) {
                Transform_Point(uin_invvertex[z][y][x][X],
                                uin_invvertex[z][y][x][Y],
                                uin_invvertex[z][y][x][Z],
                                &uout_invvertex[z][y][x][X],
                                &uout_invvertex[z][y][x][Y],
                                &uout_invvertex[z][y][x][Z]);
            }
        }
    }
}


void Load_Transformation_Matrix(float matrix[4][4])
{
	Copy_Matrix(matrix,transformation_matrix);
}


void Transform_Point(double xold, double yold, double zold, float *xnew, float *ynew, float *znew)
{
	*xnew =
	    xold * transformation_matrix[0][0] +
            yold * transformation_matrix[1][0] +
                zold * transformation_matrix[2][0] +
                    transformation_matrix[3][0];

	*ynew =
	    xold * transformation_matrix[0][1] +
            yold * transformation_matrix[1][1] +
                zold * transformation_matrix[2][1] +
                    transformation_matrix[3][1];

	*znew =
	    xold * transformation_matrix[0][2] +
            yold * transformation_matrix[1][2] +
                zold * transformation_matrix[2][2] +
                    transformation_matrix[3][2];
}


void Inverse_Concatenate_Translation(float matrix[4][4], double xoffset, double yoffset, double zoffset)
{
	float translation_matrix[4][4];
	Load_Translation_Matrix(translation_matrix,xoffset,yoffset,zoffset);
	Inverse_Concatenate_Transform(matrix,translation_matrix);
}


void Inverse_Concatenate_Scaling(float matrix[4][4], double xscale, double yscale, double zscale)
{
	float scaling_matrix[4][4];
	Load_Scaling_Matrix(scaling_matrix,xscale,yscale,zscale);
	Inverse_Concatenate_Transform(matrix,scaling_matrix);
}


void Inverse_Concatenate_Rotation(float matrix[4][4], long axis, double angle)
{
	float rotation_matrix[4][4];
	Load_Rotation_Matrix(rotation_matrix,axis,angle);
	Inverse_Concatenate_Transform(matrix,rotation_matrix);
}


void Load_Identity_Matrix(float matrix[4][4])
{
	long i,j;
	for (i=0; i<4; i++) {
		for (j=0; j<4; j++) {
			matrix[i][j] = 0;
		}
		matrix[i][i] = 1.0;
	}
}

void Load_Translation_Matrix(float matrix[4][4], double xoffset, double yoffset, double zoffset)
{
	Load_Identity_Matrix(matrix);
	matrix[3][0] = xoffset;
	matrix[3][1] = yoffset;
	matrix[3][2] = zoffset;
}


void Load_Scaling_Matrix(float matrix[4][4], double xscale,double yscale,double zscale)
{
	Load_Identity_Matrix(matrix);
	matrix[0][0] = xscale;
	matrix[1][1] = yscale;
	matrix[2][2] = zscale;
}


void Load_Rotation_Matrix(float matrix[4][4], long axis, double angle)
{
	Load_Identity_Matrix(matrix);
	if (axis == XAXIS) {
		matrix[1][1] = cos(angle/180.0*PI);
		matrix[1][2] = sin(angle/180.0*PI);
		matrix[2][1] = -sin(angle/180.0*PI);
		matrix[2][2] = cos(angle/180.0*PI);
	}
	else if (axis == YAXIS) {
		matrix[0][0] = cos(angle/180.0*PI);
		matrix[0][2] = -sin(angle/180.0*PI);
		matrix[2][0] = sin(angle/180.0*PI);
		matrix[2][2] = cos(angle/180.0*PI);
	}
	else if (axis == ZAXIS) {
		matrix[0][0] = cos(angle/180.0*PI);
		matrix[0][1] = sin(angle/180.0*PI);
		matrix[1][0] = -sin(angle/180.0*PI);
		matrix[1][1] = cos(angle/180.0*PI);
	}
}


void Concatenate_Transform(float composite_matrix[][4], float transformation_matrix[][4])
{
	float temp_matrix[4][4];
	Multiply_Matrices(composite_matrix,transformation_matrix,temp_matrix);
	Copy_Matrix(temp_matrix,composite_matrix);
}

void Inverse_Concatenate_Transform(float composite_matrix[][4],float transformation_matrix[][4])
{
	float temp_matrix[4][4];
	Multiply_Matrices(transformation_matrix,composite_matrix,temp_matrix);
	Copy_Matrix(temp_matrix,composite_matrix);
}


void Multiply_Matrices(float input_matrix1[][4], float input_matrix2[][4], float output_matrix[][4])
{
	long i,j;
	for (i=0; i<4; i++) {
		for (j=0; j<4; j++) {
			output_matrix[i][j] =
			    input_matrix1[i][0] *
			    input_matrix2[0][j] +
			    input_matrix1[i][1] *
			    input_matrix2[1][j] +
			    input_matrix1[i][2] *
			    input_matrix2[2][j] +
			    input_matrix1[i][3] *
			    input_matrix2[3][j];
		}
	}
}


void Copy_Matrix(float input_matrix[][4],float output_matrix[][4])
{
	long i,j;
	for (i=0; i<4; i++) {
		for (j=0; j<4; j++) {
			output_matrix[i][j] = input_matrix[i][j];
		}
	}
}

