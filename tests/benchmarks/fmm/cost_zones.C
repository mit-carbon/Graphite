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

#include "defs.h"
#include "memory.h"
#include "box.h"
#include "partition_grid.h"
#include "cost_zones.h"

#define NUM_DIRECTIONS 4

typedef enum { RIGHT, LEFT, UP, DOWN } direction;

static long Child_Sequence[NUM_DIRECTIONS][NUM_OFFSPRING] =
{
   { 0, 1, 2, 3 },
   { 2, 3, 0, 1 },
   { 0, 3, 2, 1 },
   { 2, 1, 0, 3 },
};
static long Direction_Sequence[NUM_DIRECTIONS][NUM_OFFSPRING] =
{
   { UP, RIGHT, RIGHT, DOWN },
   { DOWN, LEFT, LEFT, UP },
   { RIGHT, UP, UP, LEFT },
   { LEFT, DOWN, DOWN, RIGHT },
};

void ComputeSubTreeCosts(long my_id, box *b);
void CostZonesHelper(long my_id, box *b, long work, direction dir);


void
CostZones (long my_id)
{
   PartitionIterate(my_id, ComputeSubTreeCosts, BOTTOM);
   BARRIER(G_Memory->synch, Number_Of_Processors);
   Local[my_id].Total_Work = Grid->subtree_cost;
   Local[my_id].Min_Work = ((Local[my_id].Total_Work / Number_Of_Processors)
			   * my_id);
   if (my_id == (Number_Of_Processors - 1))
      Local[my_id].Max_Work = Local[my_id].Total_Work;
   else
      Local[my_id].Max_Work = (Local[my_id].Min_Work
			      + (Local[my_id].Total_Work
				 / Number_Of_Processors));
   InitPartition(my_id);
   CostZonesHelper(my_id, Grid, 0, RIGHT);
   BARRIER(G_Memory->synch, Number_Of_Processors);
}


void
ComputeSubTreeCosts (long my_id, box *b)
{
   box *pb;

   if (b->type == PARENT) {
      while (b->interaction_synch != b->num_children) {
      }
   }
   b->interaction_synch = 0;
   ComputeCostOfBox(b);
   b->subtree_cost += b->cost;
   pb = b->parent;
   if (pb != NULL) {
      ALOCK(G_Memory->lock_array, pb->exp_lock_index);
      pb->subtree_cost += b->subtree_cost;
      pb->interaction_synch += 1;
      AULOCK(G_Memory->lock_array, pb->exp_lock_index);
   }
}


void
CostZonesHelper (long my_id, box *b, long work, direction dir)
{
   box *cb;
   long i;
   long *next_child;
   long *child_dir;

   if (b->type == CHILDLESS) {
      if (work >= Local[my_id].Min_Work)
	 InsertBoxInPartition(my_id, b);
   }
   else {
      next_child = Child_Sequence[dir];
      child_dir = Direction_Sequence[dir];
      for (i = 0; (i < NUM_OFFSPRING) && (work < Local[my_id].Max_Work);
	   i++) {
	 cb = b->children[next_child[i]];
	 if (cb != NULL) {
	    if ((work + cb->subtree_cost) >= Local[my_id].Min_Work)
	       CostZonesHelper(my_id, cb, work, (direction)child_dir[i]);
	    work += cb->subtree_cost;
	 }
	 if (i == 2) {
	    if ((work >= Local[my_id].Min_Work)
		&& (work < Local[my_id].Max_Work))
	       InsertBoxInPartition(my_id, b);
	    work += b->cost;
	 }
      }
   }

}


#undef DOWN
#undef UP
#undef LEFT
#undef RIGHT
#undef NUM_DIRECTIONS

