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
*    render.c:  Render dataset via raytracing.                                *
*                                                                             *
******************************************************************************/

#include "incl.h"

				/* Derived values:                           */
float obslight[NM];	        /*   observer transformed light vector       */
float obshighlight[NM];		/*   observer transformed highlight vector   */

EXTERN_ENV

void Render(long my_node)           /* assumes direction is +Z */
{
  if (my_node == ROOT) {
  Observer_Transform_Light_Vector();
  Compute_Observer_Transformed_Highlight_Vector();
  }
  Ray_Trace(my_node);
}


void Observer_Transform_Light_Vector()
{
  float inv_magnitude;

  /* Transform light vector by inverse of viewing matrix               */
  /* to move shading light inversely with ray tracing observer.        */
  /* Matrix should include only scaling and rotation, not translation. */
  /* If no matrix has been loaded, an identity transform is performed. */
  /* Effect of of these two observer transforms is, if computation     */
  /* of colors is repeated on each frame of sequence, and same         */
  /* scaling and rotation is used during shading and ray tracing,      */
  /* light source will appear fixed relative to observer.              */
  Transform_Point(light[X],light[Y],light[Z],
		  &obslight[X],&obslight[Y],&obslight[Z]);

  /* Normalize transformed light vector                                */
  inv_magnitude = 1.0/sqrt(obslight[X]*obslight[X] +
		   obslight[Y]*obslight[Y] +
		   obslight[Z]*obslight[Z]);
  obslight[X] = obslight[X] * inv_magnitude;
  obslight[Y] = obslight[Y] * inv_magnitude;
  obslight[Z] = obslight[Z] * inv_magnitude;
}


void Compute_Observer_Transformed_Highlight_Vector()
{
  float inv_magnitude;
  float obseye[NM];		/*   observer transformed eye vector         */
  float brightness=1.0;
  float eye[NM];		/* normalized vector from object to eye      */

  /* Transform eye vector by inverse of viewing matrix                 */
  /* to move shading observer with ray tracing observer.               */
  /* Matrix should include only scaling and rotation, not translation. */
  /* If no matrix has been loaded, an identity transform is performed. */
  eye[X] = 0.0;
  eye[Y] = 0.0;
  eye[Z] = -1.0;
  Transform_Point(eye[X],eye[Y],eye[Z],&obseye[X],&obseye[Y],&obseye[Z]);

  /* Normalize transformed eye vector                                  */
  inv_magnitude = 1.0/sqrt(obseye[X]*obseye[X] +
		   obseye[Y]*obseye[Y] +
		   obseye[Z]*obseye[Z]);
  obseye[X] = obseye[X] * inv_magnitude;
  obseye[Y] = obseye[Y] * inv_magnitude;
  obseye[Z] = obseye[Z] * inv_magnitude;

  /* Compute observer transformed maximum highlight vector             */
  /* as diagonal of rhombus formed by normalized observer transformed  */
  /* light vector and normalized observer transformed eye vector.      */
  obshighlight[X] = obslight[X] + obseye[X];
  obshighlight[Y] = obslight[Y] + obseye[Y];
  obshighlight[Z] = obslight[Z] + obseye[Z];

  /* Normalize transformed highlight vector                            */
  inv_magnitude = 1.0/sqrt(obshighlight[X]*obshighlight[X] +
		   obshighlight[Y]*obshighlight[Y] +
		   obshighlight[Z]*obshighlight[Z]);
  obshighlight[X] = obshighlight[X] * inv_magnitude * brightness;
  obshighlight[Y] = obshighlight[Y] * inv_magnitude * brightness;
  obshighlight[Z] = obshighlight[Z] * inv_magnitude * brightness;
}
