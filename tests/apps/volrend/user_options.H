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
*    user_options.h:  Compile-time user options                               *
*                                                                             *
******************************************************************************/


/* For  varying image tile size*/

#ifndef BLOCK_LEN
#define BLOCK_LEN                4              /* image block size          */
#endif


/* For doing different parts of the computation differently;
If RENDER_ONLY is defined, then the program assumes that the .norm, .opc
and .pyr files already exist and just does rendering, getting its input
from these files.   If RENDER_ONLY is not defined, then the program will
not use these files but start from the .den file itself.  In this case,
there are two options:  if PREPROCESS is defined, then the program will
not render, but will create the .norm, .opc and .pyr files from the .den
file for a future run of the program to render. If PREPROCESS is not
defined either, then the program will not produce these intermediate
 files:  It will start from the .den file, create the normal and opacity
tables etc as internal data structures, and render them directly.

The SERIAL_PREPROC option tells whether the preprocessing phases (computing
the normal table or .norm file, etc.), should be done serially or in
parallel, when they are done.

Clearly, if none of the three options are defined, the default is to
do parallel preprocessing and rendering without storing intermediate array
values.
*/

#if 0
#define RENDER_ONLY                             /* to just do rendering from */
#endif                                          /* .norm, .opc and .pyr files*/

#if 0
#define PREPROCESS                              /* to just do preprocessing  */
#endif                                          /* and store result to files */

#if 1
#define SERIAL_PREPROC                                  /* to do serial preprocessing*/
#endif                                          /* with parallel rendering   */

#if 0
#define DIM                                     /* render rotations along    */
#endif                                          /* all three Cartesian axes. */
          /* This means that there will be 3*ROTATE_STEPS frames rendered    */


/* Algorithmic optimization options (adaptivity, use of octree)              */
#ifndef HBOXLEN
#define HBOXLEN                  4              /* highest_boxlen            */
#endif

