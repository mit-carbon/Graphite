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

/**************************************************************
*
*       Definitions relevant to parallel processing
*
***************************************************************/

#ifndef _PARALLEL_H
#define _PARALLEL_H



/***************************************************************************
*
*    Shared lock variable
*
*    Some machines provide only a limited number of lock variables. This
*    data structure allows sharing of these lock variables.
*    The shared locks are divided into 2 segments so that different types of
*    objects are given different locks.
*
****************************************************************************/

typedef struct
{
    LOCKDEC(lock)
} Shared_Lock ;

#define SHARED_LOCK_SEG_SIZE (MAX_SHARED_LOCK / 2)

#define SHARED_LOCK_SEG0 (0)
#define SHARED_LOCK_SEG1 (1)
#define SHARED_LOCK_SEGANY (2)

/****************************************************************************
*
*    Memory Consistency Model of the machine
*
*    Some macro changes its behavior based on the memory consistency model
*
*
*****************************************************************************/

/* Set one(1) to the model used in the machine.  Set only one of these
at a time */

#define MEM_CONSISTENCY_RELEASE    (0)
#define MEM_CONSISTENCY_WEAK       (0)
#define MEM_CONSISTENCY_PROCESSOR  (1)

#endif

