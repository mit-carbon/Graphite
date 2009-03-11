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

#ifndef _Partition_H
#define _Partition_H 1

#include "defs.h"
#include "box.h"

/* Void function type */
typedef void (*partition_function)(long my_id, box *b);

typedef enum { TOP, BOTTOM, CHILDREN } partition_start;
typedef enum { ORB, COST_ZONES } partition_alg;

extern void InitPartition(long my_id);
extern void PartitionIterate(long my_id, partition_function function,
			     partition_start position);
extern void InsertBoxInPartition(long my_id, box *b);
extern void RemoveBoxFromPartition(long my_id, box *b);
extern void ComputeCostOfBox(box *b);
extern void CheckPartition(long my_id);

#endif /* _Partition_H */
