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

/*************************************************************************
*                                                                        *
*     macros.h:  general C macros                                        *
*                                                                        *
*************************************************************************/


#define MIN(X,Y)	((X) < (Y) ? (X) : (Y))
#define MAX(X,Y)	((X) > (Y) ? (X) : (Y))
#define ABS(X)		((X) > 0 ? (X) : -(X))
#define SIGN(X)		((X) > 0 ? 1 : -1)
#define NINT(X)		(long)((X)+0.5)				  /* flt X>=0*/
#define	ROUNDDOWN(X)	(long)(X)				  /* flt X>=0*/
#define	ROUNDUP(X)	((X) == (long)(X) ? (long)(X) : (long)(X)+1) /* flt X>=0*/
#define	STEPDOWN(X)	((X) == (long)(X) ? (long)(X)-1 : (long)(X)) /* flt X>=0*/
#define	STEPUP(X)	((long)(X)+1)				  /* flt X>=0*/
#define NLONG(X)	(long)((X)+0.5)				  /* flt X>=0*/
#define	HALF(X)		((X)>>1)				  /* int X>=0*/
#define	FRACT(X)	((X)-(long)(X))				  /* flt X>=0*/



