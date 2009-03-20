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

#include <math.h>
#include <limits.h>
#include "defs.h"
#include "memory.h"
#include "particle.h"
#include "box.h"
#include "partition_grid.h"

#define DIVISOR(x) ((x <= 20) ? 1 : ((x - 20) * 50))

typedef struct _Id_Info id_info;
struct _Id_Info
{
   long id;
   long num;
};

typedef struct _Cost_Info cost_info;
struct _Cost_Info
{
   long cost;
   long num;
};

long CheckBox(long my_id, box *b, long partition_level);

void
InitPartition (long my_id)
{
   long i;

   Local[my_id].Childless_Partition = NULL;
   for (i = 0; i < MAX_LEVEL; i++) {
      Local[my_id].Parent_Partition[i] = NULL;
   }
   Local[my_id].Max_Parent_Level = -1;
}


void
PartitionIterate (long my_id, partition_function function,
		  partition_start position)
{
   box *b;
   long i;

   if (position == CHILDREN) {
      b = Local[my_id].Childless_Partition;
      while (b != NULL) {
	 (*function)(my_id, b);
	 b = b->next;
      }
   }
   else {
      if (position == TOP) {
	 for (i = 0; i <= Local[my_id].Max_Parent_Level; i++) {
	    b = Local[my_id].Parent_Partition[i];
	    while (b != NULL) {
	       (*function)(my_id, b);
	       b = b->next;
	    }
	 }
	 b = Local[my_id].Childless_Partition;
	 while (b != NULL) {
	    (*function)(my_id, b);
	    b = b->next;
	 }
      }
      else {
	 b = Local[my_id].Childless_Partition;
	 while (b != NULL) {
	    (*function)(my_id, b);
	    b = b->next;
	 }
	 for (i = Local[my_id].Max_Parent_Level; i >= 0; i--) {
	    b = Local[my_id].Parent_Partition[i];
	    while (b != NULL) {
	       (*function)(my_id, b);
	       b = b->next;
	    }
	 }
      }
   }
}


void
InsertBoxInPartition (long my_id, box *b)
{
   box *level_list;

   if (b->type == CHILDLESS) {
      b->prev = NULL;
      if (Local[my_id].Childless_Partition != NULL)
	 Local[my_id].Childless_Partition->prev = b;
      b->next = Local[my_id].Childless_Partition;
      Local[my_id].Childless_Partition = b;
   }
   else {
      level_list = Local[my_id].Parent_Partition[b->level];
      b->prev = NULL;
      if (level_list != NULL)
	 level_list->prev = b;
      b->next = level_list;
      Local[my_id].Parent_Partition[b->level] = b;
      if (b->level > Local[my_id].Max_Parent_Level) {
	 Local[my_id].Max_Parent_Level = b->level;
      }
   }
}


void
RemoveBoxFromPartition (long my_id, box *b)
{
   if (b->type == CHILDLESS) {
      if (b->prev != NULL)
	 b->prev->next = b->next;
      else
	 Local[my_id].Childless_Partition = b->next;
      if (b->next != NULL)
	 b->next->prev = b->prev;
   }
   else {
      if (b->prev != NULL)
	 b->prev->next = b->next;
      else
	 Local[my_id].Parent_Partition[b->level] = b->next;
      if (b->next != NULL)
	 b->next->prev = b->prev;
      if ((b->level == Local[my_id].Max_Parent_Level) &&
	  (Local[my_id].Parent_Partition[b->level] == NULL)) {
	 while (Local[my_id].Parent_Partition[Local[my_id].Max_Parent_Level]
		== NULL)
	    Local[my_id].Max_Parent_Level -= 1;
      }
   }
}


void
ComputeCostOfBox (box *b)
{
   long different_costs;
   long i;
   long j;
   long new_cost;
   cost_info cost_list[MAX_PARTICLES_PER_BOX];
   cost_info winner;
   long winner_index;
   long cost_index[MAX_PARTICLES_PER_BOX];

   if (b->type == PARENT)
      b->cost = ((b->num_v_list * V_LIST_COST(Expansion_Terms))
		 / DIVISOR(Expansion_Terms)) + 1;
   else {
      different_costs = 0;
      for (i = 0; i < b->num_particles; i++) {
	 new_cost = b->particles[i]->cost;
	 for (j = 0; j < different_costs; j++) {
	    if (new_cost == cost_list[j].cost)
	       break;
	 }
	 if (j == different_costs) {
	    cost_list[different_costs].cost = new_cost;
	    cost_list[different_costs].num = 1;
	    different_costs += 1;
	 }
	 else
	    cost_list[j].num += 1;
      }

      winner.cost = cost_list[0].cost;
      winner.num = 1;
      winner_index = 0;
      cost_index[0] = 0;
      for (i = 1; i < different_costs; i++) {
	 if (cost_list[i].num > cost_list[winner_index].num) {
	    winner.cost = cost_list[i].cost;
	    winner.num = 1;
	    winner_index = i;
	    cost_index[0] = i;
	 }
	 else {
	    if (cost_list[i].num == cost_list[winner_index].num) {
	       cost_index[winner.num] = i;
	       winner.num += 1;
	    }
	 }
      }
      if (winner.num != 1) {
	 for (i = 1; i < winner.num; i++)
	    winner.cost += cost_list[cost_index[i]].cost;
	 winner.cost /= winner.num;
      }
      b->cost = (winner.cost * b->num_particles) / DIVISOR(Expansion_Terms);
      if (b->cost == 0)
	 b->cost = 1;
   }
}


void
CheckPartition (long my_id)
{
   long i;
   box *b;
   long NE, NoP, CB, PB;
   long Q1, Q2, Q3, Q4;
   long PC, CC;
   real xpos, ypos;

   NE = NoP = CB = PB = Q1 = Q2 = Q3 = Q4 = PC = CC = 0;
   for (i = 0; i <= Local[my_id].Max_Parent_Level; i++) {
      b = Local[my_id].Parent_Partition[i];
      while (b != NULL) {
	 NE += CheckBox(my_id, b, i);
	 PB += 1;
	 PC += b->cost;
	 b = b->next;
      }
   }
   b = Local[my_id].Childless_Partition;
   while (b != NULL) {
      NE += CheckBox(my_id, b, -1);
      for (i = 0; i < b->num_particles; i++) {
	 xpos = b->particles[i]->pos.x;
	 ypos = b->particles[i]->pos.y;
	 if (xpos > Grid->x_center) {
	    if (ypos > Grid->y_center)
	       Q1 += 1;
	    else
	       Q4 += 1;
	 }
	 else {
	    if (ypos > Grid->y_center)
	       Q2 += 1;
	    else
	       Q3 += 1;
	 }
      }
      NoP += b->num_particles;
      CB += 1;
      CC += b->cost;
      b = b->next;
   }
}


long
CheckBox (long my_id, box *b, long partition_level)
{
   long num_errors;

   num_errors = 0;
   if (b->type == CHILDLESS) {
      if (partition_level != -1) {
	 LOCK(G_Memory->io_lock);
	 printf("ERROR : CHILDLESS box in parent partition (B%f P%ld %ld)\n", b->id, my_id, b->proc);
	 fflush(stdout);
	 UNLOCK(G_Memory->io_lock);
	 num_errors += 1;
      }
      if (b->num_children != 0) {
	 LOCK(G_Memory->io_lock);
	 printf("ERROR : CHILDLESS box has children (B%f P%ld)\n", b->id, my_id);
	 fflush(stdout);
	 UNLOCK(G_Memory->io_lock);
	 num_errors += 1;
      }
      if (b->num_particles == 0) {
	 LOCK(G_Memory->io_lock);
	 printf("ERROR : CHILDLESS box has no particles (B%f P%ld)\n", b->id, my_id);
	 fflush(stdout);
	 UNLOCK(G_Memory->io_lock);
	 num_errors += 1;
      }
      if (b->particles[b->num_particles - 1] == NULL) {
	 LOCK(G_Memory->io_lock);
	 printf("ERROR : CHILDLESS box has fewer particles than expected ");
	 printf("(B%f P%ld)\n", b->id, my_id);
	 fflush(stdout);
	 UNLOCK(G_Memory->io_lock);
	 num_errors += 1;
      }
      if (b->particles[b->num_particles] != NULL) {
	 LOCK(G_Memory->io_lock);
	 printf("ERROR : CHILDLESS box has more particles than expected ");
	 printf("(B%f P%ld)\n", b->id, my_id);
	 fflush(stdout);
	 UNLOCK(G_Memory->io_lock);
	 num_errors += 1;
      }
   }
   else {
      if (partition_level == -1) {
	 LOCK(G_Memory->io_lock);
	 printf("ERROR : PARENT box in childless partition (B%f P%ld %ld)\n",
		b->id, my_id, b->proc);
	 fflush(stdout);
	 UNLOCK(G_Memory->io_lock);
	 num_errors += 1;
      }
      else {
	 if (partition_level != b->level) {
	    LOCK(G_Memory->io_lock);
	    printf("ERROR : PARENT box in wrong partition level ");
	    printf("(%ld vs %ld) (B%f P%ld)\n", b->level, partition_level, b->id, my_id);
	    fflush(stdout);
	    UNLOCK(G_Memory->io_lock);
	    num_errors += 1;
	 }
      }
      if (b->num_children == 0) {
	 LOCK(G_Memory->io_lock);
	 printf("ERROR : PARENT box has no children (B%f P%ld)\n", b->id, my_id);
	 fflush(stdout);
	 UNLOCK(G_Memory->io_lock);
	 num_errors += 1;
      }
      if (b->num_particles != 0) {
	 LOCK(G_Memory->io_lock);
	 printf("ERROR : PARENT box has particles (B%f P%ld)\n", b->id, my_id);
	 fflush(stdout);
	 UNLOCK(G_Memory->io_lock);
	 num_errors += 1;
      }
   }
   if (b->parent == NULL) {
      if (b != Grid) {
	 LOCK(G_Memory->io_lock);
	 if (b->type == CHILDLESS)
	    printf("ERROR : Extra CHILDLESS box in partition (B%f P%ld)\n", b->id, my_id);
	 else
	    printf("ERROR : Extra PARENT box in partition (B%f P%ld)\n", b->id, my_id);
	 fflush(stdout);
	 UNLOCK(G_Memory->io_lock);
	 num_errors += 1;
      }
   }
   else {
      if (b->parent->children[b->child_num] != b) {
	 LOCK(G_Memory->io_lock);
	 if (b->type == CHILDLESS)
	    printf("ERROR : Extra CHILDLESS box in partition (B%f P%ld)\n", b->id, my_id);
	 else
	    printf("ERROR : Extra PARENT box in partition (B%f P%ld)\n", b->id, my_id);
	 fflush(stdout);
	 UNLOCK(G_Memory->io_lock);
	 num_errors += 1;
      }
   }
   return num_errors;
}


#undef DIVISOR
