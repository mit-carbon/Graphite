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

/* some variable declarations and some macro definitions */

extern double OMAS,HMAS,WTMOL,ROH,ANGLE,FHM,FOM,ROHI,ROHI2;
extern long NATOMS;
#define max(a,b) ( (a) < (b) ) ? b : a
#define min(a,b) ( (a) > (b) ) ? b : a
#define ONE ((double) 1)
#define TWO ((double) 2)
#define FIVE ((double) 5)
