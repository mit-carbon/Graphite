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


/*
 * NAME
 *	raystack.c
 *
 * DESCRIPTION
 *	This file contains the private data and code for managing the ray tree
 *	stack of ray messages.
 *
 *	A stack is used to eliminate recursion when processing a ray tree.
 *
 *	The ray tree stack contains ray messages for secondary reflection and
 *	refraction rays for pixel.  Shadow rays are not put on the stack; they
 *	are processed sequentially after a ray tree node hit.
 *
 *	The storage for the stack is allocated initially before processing any
 *	rays.  The size is dependent on max_ray_tree_step.  Each copy of this
 *	code needs to have its own private stack.
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"

#define PAGE_SIZE    4096

struct r_struct {
INT 	pad1[PAGE_SIZE];		/* This pad is inserted to avoid
					   false-sharing due to artifacts
					   of not having a private space
					   in the sproc model */
RAY	*Stack; 			/* Ptr to ray tree stack.	     */
INT	StackTop;			/* Top of ray tree stack.	     */
INT	StackSize;			/* Maximum size of ray tree stack.   */
INT 	pad2[PAGE_SIZE];		/* This pad is inserted to avoid
					   false-sharing due to artifacts
					   of not having a private space
					   in the sproc model */

} raystruct[MAX_PROCS];


/*
 * NAME
 *	CopyRayMsg - copy a ray structure from a source to a destination
 *
 * SYNOPSIS
 *	VOID	CopyRayMsg(rdst, rsrc)
 *	RAY	*rdst;			// Destination ray message.
 *	RAY	*rsrc;			// Source ray message.
 *
 * RETURNS
 *	Nothing.
 */

VOID	CopyRayMsg(RAY *rdst, RAY *rsrc)
	{
	rdst->id = rsrc->id;
	rdst->x  = rsrc->x;
	rdst->y  = rsrc->y;

	VecCopy(rdst->P, rsrc->P);
	VecCopy(rdst->D, rsrc->D);

	rdst->level  = rsrc->level;
	rdst->weight = rsrc->weight;

	/* Other fields are initialized with InitRay. */
	}



/*
 * NAME
 *	InitRayTreeStack - initialize the ray tree stack
 *
 * SYNOPSIS
 *	VOID	InitRayTreeStack(TreeDepth,pid)
 *	INT	TreeDepth.		// Max depth of a ray tree.
 *	INT	pid.
 *
 * DESCRIPTION
 *	Initialize the ray tree stack by setting the stack pointer to 0 and
 *	allocating memory for the stack.
 *
 * RETURNS
 *	Nothing.
 */

VOID	InitRayTreeStack(INT TreeDepth, INT pid)
	{
	raystruct[pid].StackSize   = powint(2, TreeDepth) - 1;
	raystruct[pid].StackSize  += NumSubRays;
	raystruct[pid].Stack	    = (RAY *)LocalMalloc(raystruct[pid].StackSize*sizeof(RAY), "raystack.c");
	raystruct[pid].StackTop    = -1;	  /* Empty condition. */
	}


unsigned long powint(long i, long j)
  {
  long k;
  long temp = 1;

  for (k = 0; k < j; k++)
      temp = temp*i;
  return temp;
}


/*
 * NAME
 *	PushRayTreeStack - push a ray message onto the ray tree stack
 *
 * SYNOPSIS
 *	VOID	PushRayTreeStack(rmsg,pid)
 *	RAY	*rmsg;			// Ray message.
 *	INT	pid.
 *
 * RETURNS
 *	Nothing.
 */

VOID	PushRayTreeStack(RAY *rmsg, INT pid)
	{
	raystruct[pid].StackTop++;

	if (raystruct[pid].StackTop == raystruct[pid].StackSize)
		{
		fprintf(stderr,"%s: Ray tree stack overflow.\n", ProgName);
		exit(-1);
		}

	CopyRayMsg(&(raystruct[pid].Stack[raystruct[pid].StackTop]), rmsg);
	}



/*
 * NAME
 *	PopRayTreeStack - pop a ray message from the ray tree stack
 *
 * SYNOPSIS
 *	INT	PopRayTreeStack(rmsg)
 *	RAY	*rmsg;			// Will receive popped ray message.
 *	INT 	pid;
 *
 * RETURNS
 *	Either empty or popped status code.
 */

INT	PopRayTreeStack(RAY *rmsg, INT pid)
	{
	if (raystruct[pid].StackTop < 0)
		return (RTS_EMPTY);

	CopyRayMsg(rmsg, &(raystruct[pid].Stack[raystruct[pid].StackTop]));

	raystruct[pid].StackTop--;
	return (RTS_VALID);
	}

