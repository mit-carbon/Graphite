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
*                                                                       *
*     constants.h:  constants needed for rendering system 		*
*                                                                       *
************************************************************************/


#define PI           3.14159265358979323846

#define MAX_NUMPROC           1024
#define NODE0                  0    /* processor no. to which first process is bound */

                                /* rendering parameters                      */
#define PIXELS_PER_BLOCK_DIM   4
#define ZSCALE                 1
#ifdef DIM
#define ROTATE_STEPS          8
#define STEP_SIZE             3
#else
#define ROTATE_STEPS           8
#define STEP_SIZE              3
#endif
#define ROOT                   0

                                /* BOOLEAN constants used                    */
#define TRUE            1       /*   BOOLEAN defined as a user type          */
#define FALSE           0       /*   in my_types.h                           */
#define YES             1
#define NO              0

				/* Definition of object space coordinates:   */
#define	NM		3	/*   number of coordinates                   */
#define	X		0	/*   x-coordinate                            */
#define	Y		1	/*   y-coordinate                            */
#define	Z		2	/*   z-coordinate                            */

				/* Definition of pixel image coordinates:    */
#define	NI		2	/*   number of coordinates                   */

                                /* NORMAL constant using 16-bit signed short */
#if 0
#define NORM_LSHIFT   127.0     /*   left shift to store normal as short     */
#define NORM_RSHIFT   .00787401574803149606
#define LOOKUP_PREC   255
#define LOOKUP_HSIZE  65024     /*   = (255*127+127)*2                       */
#define LOOKUP_SIZE   130050    /*   = 255*255*2                             */
#endif
#define NORM_LSHIFT   31.0
#define NORM_RSHIFT   .03225806451612903225
#define LOOKUP_PREC   63
#define LOOKUP_HSIZE  3968
#define LOOKUP_SIZE   7938

                                /* PIXEL constants used                      */
#define NULL_PIXEL      0       /*   initialization value for pixel          */
#define PIX_CUR_VERSION 1       /*   version of .pix file used               */

				/* Definition of string sizes:               */
#define	FILENAME_SIZE		200			/* max user filename */
#define FILENAME_STRING_SIZE	(FILENAME_SIZE+1)	/* + 1 for null char */

				/* Definition of minimum and maximum values: */
#define	MIN_DENSITY	0	/*   density                                 */
#define	MAX_DENSITY	255	/*   density                                 */
#define	MIN_MAGNITUDE	0	/*   magnitude of density gradient           */
#define	MAX_MAGNITUDE	442	/*   sqrt(MAX_GRADIENT**2*3)                 */
#define MIN_OPACITY	0.0	/*   compositing opacity                     */
#define	MAX_OPACITY	1.0	/*     (0.0=transparent, 1.0=opaque)         */
#define	MIN_OPC	        0	/*                                           */
#define	MAX_OPC	        255	/*                                           */
#define	INV_MAX_OPC	.00392156862745098039
#define	MIN_PIXEL	0	/*   pixel or voxel color or opacity         */
#define	MAX_PIXEL	255	/*                                           */
#define	INV_MAX_PIXEL	.00392156862745098039
#define MAX_INLEN	1024	/*   size of any input map or space          */
#define MAX_OUTLEN	4096	/*   size of any output map or space         */
#define SMALL          0.00001  /* Don't use {SMALL,BIG} in single precision */
#define BIG            9999.999 /* expressions alongside {big,small} numbers */
/*#define MAX_INT    4294967295*/   /* 2^32-1 is the maximum integer             */

                                /* Definition of global constants assumed    */
#define INSET           1               /*   inset assumes gradient          */
                                        /*   operator not 5x5                */

#define MAX_PYRLEVEL	9	/* Maximum level in binary pyramid           */
				/*   (allows 1x1x1..512x512x512 voxels)      */

