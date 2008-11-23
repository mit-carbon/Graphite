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

/************************************************************************
*                                                                    	*
*     global.h: global variables common to entire program               *
*                                                                       *
************************************************************************/


extern long image_section[NI];
extern long voxel_section[NM];

extern PIXEL *image,*image_block,*mask_image_block;
extern long num_nodes,frame;
extern long block_xlen,block_ylen,num_blocks,num_xblocks,num_yblocks;
extern struct GlobalMemory *Global;
extern PIXEL *shd_address;
extern BOOLEAN *sbit_address;
extern long shd_length;

                                /* Option globals                            */
extern BOOLEAN adaptive;        /* YES for adaptive ray tracing, NO if not   */
                                /* Shading parameters of reflective surface: */
extern float density_opacity[MAX_DENSITY+1];
                                /*   opacity as function of density          */
extern float magnitude_opacity[MAX_MAGNITUDE+1];
                                /*  opacity as function of magnitude         */
extern long density_epsilon;	/*   minimum (density*map_divisor)           */
				/*     (>= MIN_DENSITY)                      */
extern long magnitude_epsilon;	/*   minimum (magnitude*grd_divisor)**2      */
				/*     (> MIN_MAGNITUDE)                     */
extern PIXEL background;	/*   color of background                     */
extern float light[NM];		/*   normalized vector from object to light  */
extern float ambient_color;     /*   color of ambient reflection     */
extern float diffuse_color;    	/*   color of diffuse reflection     */
extern float specular_color;  	/*   color of specular reflection    */
extern float specular_exponent; /*   exponent of specular reflection */
extern float depth_hither;	/*   percentage of full intensity at hither  */
extern float depth_yon;		/*   percentage of full intensity at yon     */
extern float depth_exponent;	/*   exponent of falloff from hither to yon  */
extern float opacity_epsilon;	/*   minimum opacity                         */
				/*     (usually >= MIN_OPACITY,              */
				/*      < MIN_OPACITY during shading shades  */
				/*      all voxels for generation of mipmap) */
extern float opacity_cutoff;	/*   cutoff opacity                          */
				/*     (<= MAX_OPACITY)                      */
extern long highest_sampling_boxlen;
                                /*   highest boxlen for adaptive sampling    */
				/*     (>= 1)                                */
extern long lowest_volume_boxlen;/*   lowest boxlen for volume data           */
				/*     (>= 1)                                */
extern long volume_color_difference;
                        	/*   minimum color diff for volume data      */
				/*     (>= MIN_PIXEL)                        */
extern float angle[NM];         /*  initial viewing angle                    */
extern long pyr_highest_level;	/*   highest level of pyramid to look at     */
				/*     (<= MAX_PYRLEVEL)                     */
extern long pyr_lowest_level;	/*   lowest level of pyramid to look at      */
				/*     (>= 0)                                */

                                /* Pre_View Globals                          */
extern long frust_len; 	/* Size of clipping frustum                  */
				/*   (mins will be 0 in this program,        */
				/*    {x,y}len will be <= IK{X,Y}SIZE)       */
extern float depth_cueing[MAX_OUTLEN];
                         	/* Pre-computed table of depth cueing        */
extern long image_zlen;	       	/* number of samples along viewing ray       */
extern float in_max[NM];        /* Pre-computed clipping aids                */
extern long opc_xlen,opc_xylen;
extern long norm_xlen,norm_xylen;

extern VOXEL *vox_address;
extern long vox_len[NM];
extern long vox_length;
extern long vox_xlen,vox_xylen;


                                /* View Globals                              */
extern float transformation_matrix[4][4];
                                /* current transformation matrix             */
extern float out_invvertex[2][2][2][NM];
                                /* Image and object space centers            */
                                /*   of outer voxels in output map           */
extern float uout_invvertex[2][2][2][NM];
                                /* Image and object space vertices           */
                                /*   of output map unit voxel                */

                                /* Render Globals                            */
extern float obslight[NM];	/*   observer transformed light vector       */
extern float obshighlight[NM];	/*   observer transformed highlight vector   */
extern float invjacobian[NM][NM];
                        	/* Jacobian matrix showing object space      */
				/*   d{x,y,z} per image space d{x,y,z}       */
				/*   [0][0] is dx(object)/dx(image)          */
				/*   [0][2] is dz(object)/dx(image)          */
				/*   [2][0] is dx(object)/dz(image)          */
extern float invinvjacobian[NM][NM];
                        	/*   [i][j] = 1.0 / invjacobian[i][j]        */


                                /* Density map globals                       */
extern short map_len[NM];	/* Size of this density map                  */
extern int map_length;		/* Total number of densities in map          */
				/*   (= product of lens)                     */
extern DENSITY *map_address;	/* Pointer to map                            */

                                /* Normal and gradient magnitude map globals */
extern short norm_len[NM];	/* Size of this normal map                   */
extern int norm_length;	/* Total number of normals in map            */
				/*   (= NM * product of lens)                */
extern NORMAL *norm_address;	/* Pointer to normal map                     */
extern float nmag_epsilon;

                                /* Opacity map globals                       */
extern short opc_len[NM];	/* Size of this opacity map                  */
extern int opc_length;		/* Total number of opacities in map          */
				/*   (= product of lens)                     */
extern OPACITY *opc_address;	/* Pointer to opacity map                    */

                                /* Octree globals                            */
extern short pyr_levels;	/* Number of levels in this pyramid          */
extern short pyr_len[MAX_PYRLEVEL+1][NM];
                        	/* Number of voxels on each level            */
extern short pyr_voxlen[MAX_PYRLEVEL+1][NM];
                          	/* Size of voxels on each level              */
extern int pyr_length[MAX_PYRLEVEL+1];
                                /* Total number of bytes on this level       */
				/* (= (long)((product of lens+7)/8))          */
extern BYTE *pyr_address[MAX_PYRLEVEL+1];
                                /* Pointer to binary pyramid                 */
				/*   (only pyr_levels sets of lens, lengths, */
				/*    and 3-D arrays are written to file)    */
extern long pyr_offset1;	/* Bit offset of desired bit within pyramid  */
extern long pyr_offset2;	/* Bit offset of bit within byte             */
extern BYTE *pyr_address2;	/* Pointer to byte containing bit            */

                                /* Image globals                             */
extern long image_len[NI];      /* Size of image                             */
extern int image_length;        /* Total number of pixels in map             */
extern PIXEL *image_address;    /* Pointer to image                          */
extern long mask_image_len[NI]; /* Size of mask image for adaptive ray trace */
extern long mask_image_length;  /* Total number of pixels in mask image      */
extern MPIXEL *mask_image_address;
                                /* Pointer to image                          */

/* adaptive.c */
void Ray_Trace(long my_node);
void Ray_Trace_Adaptively(long my_node);
void Ray_Trace_Adaptive_Box(long outx, long outy, long boxlen);
void Ray_Trace_Non_Adaptively(long my_node);
void Ray_Trace_Fast_Non_Adaptively(long my_node);
void Interpolate_Recursively(long my_node);
void Interpolate_Recursive_Box(long outx, long outy, long boxlen);

/* file.c */
int Create_File(char filename[]);
int Open_File(char filename[]);
void Write_Bytes(int fd, unsigned char array[], long length);
void Write_Shorts(int fd, unsigned char array[], long length);
void Write_Longs(int fd, unsigned char array[], long length);
void Read_Bytes(int fd, unsigned char array[], long length);
void Read_Shorts(int fd, unsigned char array[], long length);
void Read_Longs(int fd, unsigned char array[], long length);
void Close_File(int fd);

/* main.c */
void mclock(long stoptime, long starttime, long *exectime);
void Frame(void);
void Render_Loop(void);
void Error(char string[], .../*char *arg1, char *arg2, char *arg3, char *arg4, char *arg5, char *arg6, char *arg7, char *arg8*/);
void Allocate_Image(PIXEL **address, long length);
void Allocate_MImage(MPIXEL **address, long length);
void Lallocate_Image(PIXEL **address, long length);
void Store_Image(char filename[]);
void Allocate_Shading_Table(PIXEL **address1, long length);
void Init_Decomposition(void);
long WriteGrayscaleTIFF(char *filename, long width, long height, long scanbytes, unsigned char *data);

/* map.c */
void Load_Map(char filename[]);
void Allocate_Map(DENSITY **address, long length);
void Deallocate_Map(DENSITY **address);

/* normal.c */
void Compute_Normal(void);
void Allocate_Normal(NORMAL **address, long length);
void Normal_Compute(void);
void Load_Normal(char filename[]);
void Store_Normal(char filename[]);
void Deallocate_Normal(NORMAL **address);

/* octree.c */
void Compute_Octree(void);
void Compute_Base(void);
void Or_Neighbors_In_Base(void);
void Allocate_Pyramid_Level(BYTE **address, long length);
void Compute_Pyramid_Level(long level);
void Load_Octree(char filename[]);
void Store_Octree(char filename[]);

/* opacity.c */
void Compute_Opacity(void);
void Allocate_Opacity(OPACITY **address, long length);
void Opacity_Compute(void);
void Load_Opacity(char filename[]);
void Store_Opacity(char filename[]);
void Deallocate_Opacity(OPACITY **address);

/* option.c */
void Init_Options(void);
void Init_Opacity(void);
void Init_Lighting(void);
void Init_Parallelization(void);

/* raytrace.c */
void Trace_Ray(double foutx, double fouty, PIXEL *pixel_address);
void Pre_Shade(long my_node);

/* render.c */
void Render(long my_node);
void Observer_Transform_Light_Vector(void);
void Compute_Observer_Transformed_Highlight_Vector(void);

/* view.c */
void Compute_Pre_View(void);
void Select_View(double delta_angle, long axis);
void Compute_Input_Dimensions(void);
void Compute_Input_Unit_Vector(void);
void Load_Transformation_Matrix(float matrix[4][4]);
void Transform_Point(double xold, double yold, double zold, float *xnew, float *ynew, float *znew);
void Inverse_Concatenate_Translation(float matrix[4][4], double xoffset, double yoffset, double zoffset);
void Inverse_Concatenate_Scaling(float matrix[4][4], double xscale, double yscale, double zscale);
void Inverse_Concatenate_Rotation(float matrix[4][4], long axis, double angle);
void Load_Identity_Matrix(float matrix[4][4]);
void Load_Translation_Matrix(float matrix[4][4], double xoffset, double yoffset, double zoffset);
void Load_Scaling_Matrix(float matrix[4][4], double xscale, double yscale, double zscale);
void Load_Rotation_Matrix(float matrix[4][4], long axis, double angle);
void Concatenate_Transform(float composite_matrix[][4], float transformation_matrix[][4]);
void Inverse_Concatenate_Transform(float composite_matrix[][4], float transformation_matrix[][4]);
void Multiply_Matrices(float input_matrix1[][4], float input_matrix2[][4], float output_matrix[][4]);
void Copy_Matrix(float input_matrix[][4], float output_matrix[][4]);
