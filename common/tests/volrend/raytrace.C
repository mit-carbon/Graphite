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
*   raytrace.c:  Render dataset via raytracing.                               *
*                                                                             *
******************************************************************************/

#include "incl.h"

extern long num_traced_rays_hit_volume;
extern long num_samples_trilirped;
extern long traversal_time,trilirp_time,init_time,composite_time;

#define VXEL(X,SIGN)		((SIGN) > 0 ? \
				 (long)((X)+SMALL) : \
				 (long)((X)-SMALL))

#define SBIT_ADDRESS(TA)        (sbit_address+(TA))
#define SHD_ADDRESS(TA)         (shd_address+(TA))
#define SBIT(TA)                (*SBIT_ADDRESS(TA))
#define SHD(TA)                 (*SHD_ADDRESS(TA))

EXTERN_ENV

void Trace_Ray(foutx, fouty, pixel_address)
     float foutx, fouty;
     PIXEL *pixel_address;
{
  float ray[2][NM];	        /* Frustrum and option, but not input map,   */
                                /*   clipped object space coordinates of ray */
                                /*   [0] are coordinates at hither plane     */
                                /*   [1] are coordinates at yon plane        */
  float rmin,rmax;	        /* Ray space centers of min and max voxels   */
                                /*   in input map for each coordinate        */
                                /*   (0.0 = coordinate of hither plane)      */
                                /*   (1.0 = coordinate of yon plane)         */
  float ray_min,ray_max;	/* Ray space centers of first and last voxels*/
                                /*   in input map encountered by ray         */
                                /*   (min is first, i.e. closest to eye)     */
                                /*   (max is last, i.e. farthest from eye)   */
  long segment_zmin,	        /* Image space loop terminals of segments of */
  segment_zmax;	                /*   samples between polygon intersections   */
  long span_zmin,		/* Image space loop terminals of spans of    */
  span_zmax;	        	/*   samples within pyramid boxes            */
  float sample[NM],	        /* Object space coordinates of sample        */
  sample2[NM];	                /*   before and after indenting for trilirp  */
  long outz;		        /* Image space loop index along ray          */
  long i,samplex,sampley,samplez;
  long sample2x,sample2y,sample2z;

  float box_zmin,box_zmax;
  long starting_level,level;
  float jump[NM],min_jump;
  float voxel[NM],next_voxel[NM];
  long ivoxel[NM],next_ivoxel[NM];
  BOOLEAN bit;

  float xalpha,yalpha,zalpha;
  float one_minus_xalpha,one_minus_yalpha,one_minus_zalpha;
  float weight,wopacity,wopacitysum,wcolorsum;
  float additional_opacity;

  float color;			/*   color (MIN_PIXEL..MAX_PIXEL)            */
  float opacity;		/*   opacity (0.0..1.0)                      */
  float ray_color,ray_opacity;

  OPACITY *local_opc_address,*local2_opc_address;
  NORMAL *local_norm_address,*local2_norm_address;

  long pyr_offset1;	/* Bit offset of desired bit within pyramid  */
  long pyr_offset2;	/* Bit offset of bit within byte             */
  BYTE *pyr_address2;	/* Pointer to byte containing bit            */


  /* Initialize geometry/volume ray to transparent black.              */
  ray_color = (float)MIN_PIXEL;
  ray_opacity = MIN_OPACITY;

  /* Initialize frustum and option, but not input map,                 */
  /* clipped object space coordinates of viewing ray                   */
  for (i=0; i<NM; i++) {
    ray[0][i] = out_invvertex[0][0][0][i] +
      invjacobian[X][i]*foutx +
	invjacobian[Y][i]*fouty;
    ray[1][i] = ray[0][i] + invjacobian[Z][i]*image_zlen;
  }

  /* Compute ray space centers of min and max voxels in input map      */
  /* for each coordinate.  If denominator for any coordinate is zero,  */
  /* ray is perpendicular to coordinate axis and that coordinate can   */
  /* be ignored, except that if entire ray is outside map boundaries   */
  /* as defined by centers of first and last voxels, skip it.          */
  /* Accumulate to yield ray space centers of first and last           */
  /* voxels in input map encountered by viewing ray, applying          */
  /* frustum and option clipping at the same time.                     */

  ray_min = 0.0;
  ray_max = 1.0;
  for (i=0; i<NM; i++) {
    if (ABS(ray[1][i] - ray[0][i]) < SMALL) {
      if ((ray[0][i] < 0.0 &&
	   ray[1][i] < 0.0) ||
	  (ray[0][i] > (float)(opc_len[i]-1) &&
	   ray[1][i] > (float)(opc_len[i]-1)))
	return;
      else
	continue;
    }
    rmin = (0.0 - ray[0][i]) / (ray[1][i] - ray[0][i]);
    rmax = ((float)(opc_len[i]-1) - ray[0][i]) / (ray[1][i] - ray[0][i]);
    ray_min = MAX(MIN(rmin,rmax),ray_min);
    ray_max = MIN(MAX(rmin,rmax),ray_max);
  }

  /* Use parameters computed above to obtain frustum, option and       */
  /* input map clipped image space loop terminals for viewing ray,     */
  /* rounding inwards to produce image terminals falling inside or     */
  /* coincident with input map boundaries (defined by voxel centers).  */
  /* This insures that the integerized object space coordinates of     */
  /* the first and last sample positions along ray fall within the     */
  /* map, taking advantage of the fact that input map mins = 0 and     */
  /* integerization of slightly negative numbers rounds up to zero.    */

  segment_zmin = ROUNDUP(image_zlen * ray_min);
  segment_zmax = ROUNDDOWN(image_zlen * ray_max);

  /* If loop terminals call for no work to be done, skip ray.          */
  if (segment_zmax < segment_zmin) return;

  /* Use binary pyramid to find image space loop terminals  */
  /* of spans of sample positions that may contain interesting voxels. */
  /* Unless lowest level to look at is lowest level of pyramid,        */
  /* sample spans may include some non-interesting samples,            */
  /* so binary mask should be used along with pyramid.                 */

  /* Start search at start of segment computed above, at the       */
  /* corresponding voxel coordinates, and at lower of selected     */
  /* highest level to look at and highest level present.           */
  /* If mipmapping, search will stop at higher of selected         */
  /* lowest level to look at and current mipmap level.             */
  /* Integer voxel coordinates are used to extract bits from the   */
  /* pyramid.  A pyramid bit represents data on the positive side  */
  /* of the integer voxel.  To insure that a bit represents data   */
  /* beyond the current position in the direction of motion,       */
  /* real coordinates near integers are rounded to that integer    */
  /* if moving in the positive direction, but to the next lower    */
  /* integer if moving in the negative direction.                  */

  box_zmin = (float)segment_zmin;
  voxel[X] = ray[0][X] + invjacobian[Z][X]*box_zmin;
  voxel[Y] = ray[0][Y] + invjacobian[Z][Y]*box_zmin;
  voxel[Z] = ray[0][Z] + invjacobian[Z][Z]*box_zmin;
  ivoxel[X] = VXEL(voxel[X],invjacobian[Z][X]);
  ivoxel[Y] = VXEL(voxel[Y],invjacobian[Z][Y]);
  ivoxel[Z] = VXEL(voxel[Z],invjacobian[Z][Z]);
  starting_level = MIN(pyr_highest_level,pyr_levels-1);
  level = starting_level;

  while (1) {

    /* Extract bit from pyramid.  Note:  If mipmapping,          */
    /* use of OR'ed pyramid and no trilirp is valid but wasteful,*/
    /* but use of un-OR'ed pyramid and trilirp is invalid.       */

    bit = PYR(level,ivoxel[Z]>>level,
	      ivoxel[Y]>>level,
	      ivoxel[X]>>level);
    if (bit && level > pyr_lowest_level) {

      /* This pyramid box contains something interesting,  */
      /* but we are not at lowest level to be looked at,   */
      /* so stay at current position and drop a level.     */
      level--;
      continue;
    }

    /* Compute image space position of far box boundary by       */
    /* computing image space distance from near box boundary     */
    /* to each of the three planes that bound the box in the     */
    /* ray direction, then taking the minimum of the three       */
    /* computed distances and moving that far along the ray.     */
    /* If Jacobian for any coordinate is zero, ray is parallel   */
    /* to coordinate axis and that plane can be ignored.         */
    min_jump = BIG;
    if (invjacobian[Z][X] > SMALL) {
      jump[X] = invinvjacobian[Z][X] *
	(((ROUNDDOWN(voxel[X])>>level)+1)*
	 pyr_voxlen[level][X]-voxel[X]);
      min_jump = MIN(min_jump,jump[X]);
    }
    else if (invjacobian[Z][X] < -SMALL) {
      jump[X] = invinvjacobian[Z][X] *
	((STEPDOWN(voxel[X])>>level)*
	 pyr_voxlen[level][X]-voxel[X]);
      min_jump = MIN(min_jump,jump[X]);
      }
    if (invjacobian[Z][Y] > SMALL) {
      jump[Y] = invinvjacobian[Z][Y] *
	(((ROUNDDOWN(voxel[Y])>>level)+1)*
	 pyr_voxlen[level][Y]-voxel[Y]);
      min_jump = MIN(min_jump,jump[Y]);
    }
    else if (invjacobian[Z][Y] < -SMALL) {
      jump[Y] = invinvjacobian[Z][Y] *
	((STEPDOWN(voxel[Y])>>level)*
	 pyr_voxlen[level][Y]-voxel[Y]);
      min_jump = MIN(min_jump,jump[Y]);
    }
    if (invjacobian[Z][Z] > SMALL) {
      jump[Z] = invinvjacobian[Z][Z] *
	(((ROUNDDOWN(voxel[Z])>>level)+1)*
	 pyr_voxlen[level][Z]-voxel[Z]);
      min_jump = MIN(min_jump,jump[Z]);
    }
    else if (invjacobian[Z][Z] < -SMALL) {
      jump[Z] = invinvjacobian[Z][Z] *
	((STEPDOWN(voxel[Z])>>level)*
	 pyr_voxlen[level][Z]-voxel[Z]);
      min_jump = MIN(min_jump,jump[Z]);
    }
    box_zmax = box_zmin + min_jump;

    if (bit) {
      /* This pyramid box contains something interesting,  */
      /* and we are at lowest level to be looked at, so    */
      /* break out of loop and begin rendering.            */
      break;
    }

    /* This pyramid box either contains nothing interesting,     */
    /* or contains something which has now been rendered.        */
    /* If far box boundary is beyond end of segment,             */
    /* skip to end of segment.  Otherwise, set near              */
    /* boundary of next box to far boundary of current box.      */

  next_box:
    if (box_zmax >= (float)segment_zmax) {
      goto end_of_segment;
    }
    box_zmin = box_zmax;

    /* Compute voxel coordinates of near boundary of next box    */
    next_voxel[X] = voxel[X] + invjacobian[Z][X]*min_jump;
    next_voxel[Y] = voxel[Y] + invjacobian[Z][Y]*min_jump;
    next_voxel[Z] = voxel[Z] + invjacobian[Z][Z]*min_jump;
    next_ivoxel[X] = VXEL(next_voxel[X],invjacobian[Z][X]);
    next_ivoxel[Y] = VXEL(next_voxel[Y],invjacobian[Z][Y]);
    next_ivoxel[Z] = VXEL(next_voxel[Z],invjacobian[Z][Z]);

    /* If current and next boxes do not have the same parent,    */
    /* time can be saved by popping up to parent of next box,    */
    /* which spans more image space distance than its child.     */
    /* Popping can be repeated until parent boxes match          */
    /* or until next box is at starting level.                   */
    while (level < starting_level) {
      if(next_ivoxel[X]>>(level+1) !=
	 ivoxel[X]>>(level+1) ||
	 next_ivoxel[Y]>>(level+1) !=
	 ivoxel[Y]>>(level+1) ||
	 next_ivoxel[Z]>>(level+1) !=
	 ivoxel[Z]>>(level+1)) {
	level++;
      }
      else
	break;
    }

    /* Advance voxel coordinates to near boundary of next box    */
    /* and loop for next bit from pyramid.                       */
    voxel[X] = next_voxel[X];
    voxel[Y] = next_voxel[Y];
    voxel[Z] = next_voxel[Z];
    ivoxel[X] = next_ivoxel[X];
    ivoxel[Y] = next_ivoxel[Y];
    ivoxel[Z] = next_ivoxel[Z];
  }

  /* We have now broken out of loop to begin rendering.            */
  /* Set image space loop terminals of sample span to include      */
  /* either all integer image space positions falling inside or    */
  /* coincident with box boundaries (occurring on voxel centers),  */
  /* or all positions remaining in segment as computed above.      */
  /* Use of integer positions insures even spacing of samples,     */
  /* which prevents addition of noise component to image.          */
  span_zmin = ROUNDUP(box_zmin);
  span_zmax = MIN((long)box_zmax,segment_zmax);

  /* If span contains no samples, skip it.                         */
  if (span_zmax < span_zmin) goto next_box;

  /* Compute coordinates of first sample position in span              */
  sample[X] = ray[0][X] + invjacobian[Z][X]*span_zmin;
  sample[Y] = ray[0][Y] + invjacobian[Z][Y]*span_zmin;
  sample[Z] = ray[0][Z] + invjacobian[Z][Z]*span_zmin;

  for (outz=span_zmin; outz<=span_zmax; outz++) {

    /* If binary octree is zero for all eight neighbors in the    */
    /* base level, if trilirp'ing, or lower neighbor otherwise, skip it.     */
    /* Use of OR'ed mask and no trilirp is valid but wasteful, but use of    */
    /* un-OR'ed mask and trilirp is invalid.                                 */

    samplex=(long)sample[X];
    sampley=(long)sample[Y];
    samplez=(long)sample[Z];

      bit=PYR(0,samplez,sampley,samplex);
      if (!bit) goto end_of_sample;

    /* If trilip'ing and not shadowing, then                     */
    /* extract colors and opacities of nearest eight neighbors,  */
    /* pre-multiply color by opacity, and trilirp.               */
    /* If not trilirp'ing and not shadowing, then                */
    /* extract color and opacity of lower neighbor,              */
    /* and pre-multiply color by opacity.                        */

    /* Indent object space coordinates of sample position      */
    /* by an amount unlikely to produce visible artifacts if   */
    /* necessary so that they fall strictly inside input map   */
    /* max boundaries, insuring that all neighbors needed for  */
    /* trilirp fall within map.                                */

    sample2[X] = MIN(sample[X],in_max[X]);
    sample2[Y] = MIN(sample[Y],in_max[Y]);
    sample2[Z] = MIN(sample[Z],in_max[Z]);

    sample2x = (long)sample2[X];
    sample2y = (long)sample2[Y];
    sample2z = (long)sample2[Z];

    xalpha = sample2[X]-sample2x++;
    yalpha = sample2[Y]-sample2y;
    zalpha = sample2[Z]-sample2z;

    local_opc_address = OPC_ADDRESS(sample2z,sample2y,sample2x);
    local_norm_address = NORM_ADDRESS(sample2z,sample2y,sample2x,Z);

    /* note that the code below is optimized for the particular map        */
    /* layout and is not abstracted so that if the map layout changes,     */
    /* so must the code that does the trilinear interpolation below.       */

    one_minus_xalpha = 1.0 - xalpha;
    one_minus_yalpha = 1.0 - yalpha;
    one_minus_zalpha = 1.0 - zalpha;

    weight = xalpha * one_minus_yalpha * one_minus_zalpha;
    wopacity = *local_opc_address-- * weight;
    color = SHD(*local_norm_address--);
    wcolorsum = color * wopacity;
    wopacitysum = wopacity;

    weight = one_minus_xalpha * one_minus_yalpha * one_minus_zalpha;
    wopacity = *local_opc_address * weight;
    color = SHD(*local_norm_address);
    wcolorsum += color * wopacity;
    wopacitysum += wopacity;

    weight = xalpha * yalpha * one_minus_zalpha;
    local2_opc_address = local_opc_address+opc_xlen;
    local2_norm_address = local_norm_address+norm_xlen;
    wopacity = *local2_opc_address-- * weight;
    color = SHD(*local2_norm_address--);
    wcolorsum += color * wopacity;
    wopacitysum += wopacity;

    weight = one_minus_xalpha * yalpha * one_minus_zalpha;
    wopacity = *local2_opc_address * weight;
    color = SHD(*local2_norm_address);
    wcolorsum += color * wopacity;
    wopacitysum += wopacity;

    weight = xalpha * one_minus_yalpha * zalpha;
    local_opc_address = local_opc_address+opc_xylen;
    local_norm_address = local_norm_address+norm_xylen;
    wopacity = *local_opc_address-- * weight;
    color = SHD(*local_norm_address--);
    wcolorsum += color * wopacity;
    wopacitysum += wopacity;

    weight = one_minus_xalpha * one_minus_yalpha * zalpha;
    wopacity = *local_opc_address * weight;
    color = SHD(*local_norm_address);
    wcolorsum += color * wopacity;
    wopacitysum += wopacity;

    weight = xalpha * yalpha * zalpha;
    local2_opc_address = local_opc_address+opc_xlen;
    local2_norm_address = local_norm_address+norm_xlen;
    wopacity = *local2_opc_address-- * weight;
    color = SHD(*local2_norm_address--);
    wcolorsum += color * wopacity;
    wopacitysum += wopacity;

    weight = one_minus_xalpha * yalpha * zalpha;
    wopacity = *local2_opc_address * weight;
    color = SHD(*local2_norm_address);
    wcolorsum += color * wopacity;
    wopacitysum += wopacity;

    opacity = wopacitysum * INV_MAX_OPC;
    color = wcolorsum * INV_MAX_OPC;


    /* If not shadowing, apply pre-computed depth cueing to      */
    /* color, correcting table index for local sampling rate.    */
    color *= depth_cueing[outz];

    /* Composite sample into geometry/volume ray using           */
    /* order-independent merging of pre-multiplied colors        */
    /* (Porter and Duff, "Compositing Digital Images", SIG '84,  */
    /* p. 256, formula 1, for case of "B (ray) over A (voxel)"). */
    /* Resultant color is still multiplied by opacity            */
    /* and requires normalization before display.                */
    /* Direction-sensitive initialization of inverse Jacobian    */
    /* insures that compositing will be along image space Z-axis */
    /* and from origin towards +Z axis, i.e. front-to-back.      */

    additional_opacity = opacity * (1.0-ray_opacity);
    ray_color += color * (1.0-ray_opacity);
    ray_opacity += additional_opacity;

    /* If accumulated opacity of geometry/volume ray is unity,   */
    /* if polygon list exists and adaptively ray tracing,        */
    /* bypass volume data, but continue geometry-only ray until  */
    /* its opacity becomes unity or we run out of intersections. */
    /* If no polygons or not adaptively ray tracing, ray is done.*/

    if (ray_opacity > opacity_cutoff) {
      goto end_of_ray;
    }

    /* Update object space coordinates for next sample position  */
  end_of_sample:;
    sample[X] += invjacobian[Z][X];
    sample[Y] += invjacobian[Z][Y];
    sample[Z] += invjacobian[Z][Z];
  }
  /* If re-entry is enabled,                 */
  /* process next pyramid box, else segment is done.                   */

    goto next_box;

 end_of_segment:;

  /* If accumulated opacity of geometry/volume ray is small, return.   */
  if (ray_opacity <= opacity_epsilon) return;

 end_of_ray:;

  /* Composite geometry/volume ray against existing image or 2-d mipmap*/
  /* array after which it is assumed opaque and needs no normalization.*/
  /* Array may contain background or result of previous rendering.     */
  additional_opacity = 1.0 - ray_opacity;
  ray_color += *pixel_address * additional_opacity;
  *pixel_address = NINT(ray_color);
}


void Pre_Shade(long my_node)
{
  long xnorm,ynorm,znorm,table_addr,norm_lshift;
  long shd_table_partition,zstart,zstop;
  float mag,error,inv_num_nodes;
  float normal[NM];
  float dot_product,diffuse,specular,color;
  float dpartial_product1,dpartial_product2;
  float spartial_product1,spartial_product2;
  long temp;

  inv_num_nodes = 1.0/(float)num_nodes;

  norm_lshift = NORM_LSHIFT;
  error = -2.0*NORM_RSHIFT*NORM_RSHIFT;
  /* assumed for now that z direction has enough parallelism */
  shd_table_partition = ROUNDUP((float)LOOKUP_PREC*inv_num_nodes);
  zstart = -norm_lshift + shd_table_partition * my_node;
  zstop = MIN(zstart+shd_table_partition,norm_lshift+1);

/*  POSSIBLE ENHANCEMENT:  If you did want to replicate the shade table
on all processors, then include these two lines here to set
zstart and zstop differently:

  zstart= -norm_lshift;
  zstop = norm_lshift+1;

IMPORTANT:  Note that this makes the pre-shading entirely
serial in every frame, so you won't get parallelism on that part
of the frame.
*/

  for (znorm = zstart; znorm < zstop; znorm++) {
    for (ynorm = -norm_lshift; ynorm <= norm_lshift; ynorm++) {
      normal[Z] = (float)znorm * NORM_RSHIFT;
      normal[Y] = (float)ynorm * NORM_RSHIFT;
      mag = 1.0 - normal[Z] * normal[Z] - normal[Y] * normal[Y];
      if (mag > error) {
	mag = MAX(mag,0.0);
	normal[X] = sqrt(mag);
	xnorm = (long)normal[X];
	dpartial_product1 = normal[Z] * obslight[Z] + normal[Y] * obslight[Y];
	dpartial_product2 = normal[X] * obslight[X];
	spartial_product1 = normal[Z] * obshighlight[Z] +
	  normal[Y] * obshighlight[Y];
	spartial_product2 = normal[X] * obshighlight[X];
	table_addr = LOOKUP_HSIZE+(znorm*LOOKUP_PREC+ynorm)*2;
	dot_product = dpartial_product1 + dpartial_product2;
	diffuse = MAX(dot_product,0.0);
	dot_product = spartial_product1 + spartial_product2;
	specular = MAX(dot_product,0.0);
	specular = pow(specular,specular_exponent);
	color = ambient_color + diffuse*diffuse_color +
	  specular*specular_color;
	color = NINT(MIN(color,MAX_PIXEL));
	temp = (long) color;
	SHD(table_addr+1) = (unsigned char) temp;
	if (normal[X] > 0.0) {
	  dot_product = dpartial_product1 - dpartial_product2;
	  diffuse = MAX(dot_product,0.0);
	  dot_product = spartial_product1 - spartial_product2;
	  specular = MAX(dot_product,0.0);
	  specular = pow(specular,specular_exponent);
	  color = ambient_color + diffuse*diffuse_color +
	    specular*specular_color;
	  color = NINT(MIN(color,MAX_PIXEL));
	}
	temp = (long) color;
	SHD(table_addr) = (unsigned char) temp;
      }
    }
  }
  table_addr = LOOKUP_HSIZE+(norm_lshift*LOOKUP_PREC+2)*2+1;
  temp = (long) ambient_color;
  SHD(table_addr) = (unsigned char) temp;
}
