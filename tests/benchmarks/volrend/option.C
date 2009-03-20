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
*   option.c:  Rendering options selected.                                    *
*                                                                             *
******************************************************************************/

#include "incl.h"

long block_xlen,block_ylen;
BOOLEAN adaptive;               /* adaptive ray tracing?                     */

				/* During shading:                           */
long density_epsilon;	        /*   minimum (density*map_divisor)           */
				/*     (>= MIN_DENSITY)                      */
long magnitude_epsilon;  	/*   minimum (magnitude*grd_divisor)**2      */
				/*     (> MIN_MAGNITUDE)                     */

                                /* Shading parameters of reflective surface: */
float density_opacity[MAX_DENSITY+1];	  /* opacity as function of density  */
float magnitude_opacity[MAX_MAGNITUDE+1];/* opacity as function of magnitude*/

				/* Global lighting parameters:               */
PIXEL background;	        /*   color of background assumed to be zero  */
                                /*   because of hack for producing final     */
                                /*   image on host from node images          */
float light[NM];		/*   normalized vector from object to light  */

				/* Lighting parameters of reflective surface:*/
float ambient_color;	        /*   color of ambient reflection             */
float diffuse_color;    	/*   color of diffuse reflection             */
float specular_color;   	/*   color of specular reflection            */
float specular_exponent;       /*   exponent of specular reflection         */

				/* Depth cueing parameters:                  */
float depth_hither;		/*   percentage of full intensity at hither  */
float depth_yon;		/*   percentage of full intensity at yon     */
float depth_exponent;		/*   exponent of falloff from hither to yon  */

				/* During shading, rendering, ray tracing:   */
float opacity_epsilon;  	/*   minimum opacity                         */
				/*     (usually >= MIN_OPACITY,              */
				/*      < MIN_OPACITY during shading shades  */
				/*      all voxels for generation of mipmap) */

				/* During rendering and ray tracing:         */
float opacity_cutoff;   	/*   cutoff opacity                          */
				/*     (<= MAX_OPACITY)                      */

				/* During ray tracing:                       */
long highest_sampling_boxlen;    /*   highest boxlen for adaptive sampling    */
				/*     (>= 1)                                */
long lowest_volume_boxlen;      	/*   lowest boxlen for volume data           */
				/*     (>= 1)                                */
long volume_color_difference;   	/*   minimum color diff for volume data      */
				/*     (>= MIN_PIXEL)                        */
long pyr_highest_level; 		/*   highest level of pyramid to look at     */
				/*     (<= MAX_PYRLEVEL)                     */
long pyr_lowest_level;  		/*   lowest level of pyramid to look at      */
				/*     (>= 0)                                */
float angle[NM];                /* initial viewing angle                     */


EXTERN_ENV

void Init_Options()
{

  norm_address = NULL;
  opc_address = NULL;
  pyr_address[0] = NULL;

  background = NULL_PIXEL;

  Init_Opacity();
  Init_Lighting();

  angle[X] = 90.0;
  angle[Y] = -36.0;
  angle[Z] = 0.0;
  Init_Parallelization();

  opacity_epsilon = 0.0;
  opacity_cutoff = 0.95;
  highest_sampling_boxlen = HBOXLEN; /* this must be less than BLOCK_LEN */
                                     /* and both must be powers of 2     */

  lowest_volume_boxlen = 1;

  volume_color_difference = 16;
  pyr_highest_level = 5;
  pyr_lowest_level = 2;
}


void Init_Opacity()
{
  long i;
  float increment;

  density_epsilon = 96;
  magnitude_epsilon = 1;

  /* initialize opacity functions (hardwired for simplicity) */
  density_opacity[MIN_DENSITY] = 0.0;
  density_opacity[95] = 0.0;
  density_opacity[135] = 1.0;
  density_opacity[MAX_DENSITY] = 1.0;
  for (i=MIN_DENSITY; i<95; i++) density_opacity[i] = 0.0;
  increment = 1.0/(135.0-95.0);
  for (i=95; i<134; i++)
    density_opacity[i+1] = density_opacity[i]+increment;
  for (i=135; i<MAX_DENSITY; i++) density_opacity[i] = 1.0;
  magnitude_opacity[MIN_MAGNITUDE] = 0.0;
  magnitude_opacity[70] = 1.0;
  magnitude_opacity[MAX_MAGNITUDE] = 1.0;
  increment = 1.0/(70.0-(float)MIN_MAGNITUDE);
  for (i=MIN_MAGNITUDE; i<69; i++)
    magnitude_opacity[i+1] = magnitude_opacity[i]+increment;
  for (i=70; i<MAX_MAGNITUDE-1; i++) magnitude_opacity[i] = 1.0;
}


void Init_Lighting()
{
  float inv_magnitude;

  /* Normalize vector from object to light source            */
  light[X] = 0.5;
  light[Y] = 0.7;
  light[Z] = -1.0;
  inv_magnitude = 1.0/sqrt(light[X]*light[X] +
			 light[Y]*light[Y] +
			 light[Z]*light[Z]);
  light[X] = light[X]*inv_magnitude;
  light[Y] = light[Y]*inv_magnitude;
  light[Z] = light[Z]*inv_magnitude;

  ambient_color = 30.0;
  diffuse_color = 100.0;
  specular_color = 130.0;
  specular_exponent = 10.0;

  depth_hither = 1.0;
  depth_yon = 0.4;
  depth_exponent = 1.0;
}


void Init_Parallelization()
{
  block_xlen = BLOCK_LEN;
  block_ylen = BLOCK_LEN;
#ifdef RENDER_ONLY
  printf("Rendering only:  Starting from .norm, .opc and .pyr files\n");
#else
#ifdef PREPROCESS
  printf("Preprocessing only: From .den file to .norm, .opc, and .pyr files; no rendering\n");
#else
  printf("Both computing normals, opacities and octree as well as rendering: starting from .den file\n");
#endif
#ifdef SERIAL_PREPROC
  printf("NOTE: Preprocessing (computing normals, opacities and octree) is done serially by only one processor\n");
#endif
#endif

  printf("Gouraud shading from lookup tables used\n");
  printf("\t%ld processes\n",num_nodes);
  printf("\t%ldx%ld image block size\n",block_xlen,block_ylen);

  if (adaptive) {
    printf("\tdoing adaptive rendering\n");
  }
  else
    printf("\tdoing nonadaptive rendering\n");

  printf("\tusing precomputed normals and opacities in separate array\n");

  printf("\tusing opacity octree\n");

  printf("\tstarting angle at (%.1f, %.1f, %.1f) with rotation step of %d\n",
angle[X],angle[Y],angle[Z],STEP_SIZE);
#ifdef DIM
  printf("rotating %d steps in each of the three Cartesian directions\n",ROTATE_STEPS);
#else
  printf("rotating %d steps in the Y direction\n",ROTATE_STEPS);
#endif

  printf("\n");
}
