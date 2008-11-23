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
*    my_types.h:  types used for rendering system                             *
*                                                                             *
******************************************************************************/


				/* Definition of user-defined types:         */
typedef unsigned char DENSITY;		/*   density                         */
typedef unsigned char BYTE;		/*   general purpose byte            */
typedef signed short NORMAL;            /*   density normal                  */
typedef unsigned char PIXEL;		/*   pixel or voxel color            */
typedef unsigned char MPIXEL;	        /*   volatile pixel or voxel color;
                                               make volatile on SGIs   */
typedef unsigned char OPACITY;          /*   voxel opacity                   */
typedef unsigned short BOOLEAN;		/*   boolean flag                    */
typedef short VOXEL;
