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

#include <stdio.h>
#include <math.h>
#include "defs.h"
#include "memory.h"
#include "particle.h"
#include "box.h"

/* How many boxes can fit on one line */
#define BOXES_PER_LINE 4
#define TERMS_PER_LINE 2

box *Grid = NULL;

void ZeroBox(long my_id, box *b);

void
CreateBoxes (long my_id, long num_boxes)
{
   long i;

   LOCK(G_Memory->mal_lock);
   Local[my_id].B_Heap = (box *) G_MALLOC(num_boxes * sizeof(box));

/* POSSIBLE ENHANCEMENT:  Here is where one might distribute the
   B_Heap data across physically distributed memories as desired.

   One way to do this is as follows:

   char *starting_address;
   char *ending_address;

   starting_address = (char *) Local[my_id].B_Heap;
   ending_address = (((char *) Local[my_id].B_Heap)
		     + (num_boxes * sizeof(particle *)) - 1);

   Place all addresses x such that (starting_address <= x < ending_address)
   on node my_id

*/

   UNLOCK(G_Memory->mal_lock);
   Local[my_id].Max_B_Heap = num_boxes;
   Local[my_id].Index_B_Heap = 0;

   for (i = 0; i < num_boxes; i++) {
      Local[my_id].B_Heap[i].exp_lock_index = i % (MAX_LOCKS - 1);
      Local[my_id].B_Heap[i].particle_lock_index = i % (MAX_LOCKS - 1);
      Local[my_id].B_Heap[i].id = i + ((double) my_id / ID_LIMIT);
      ZeroBox(my_id, &Local[my_id].B_Heap[i]);
   }
}


void
FreeBoxes (long my_id)
{
   long i;
   box *b_array;

   b_array = Local[my_id].B_Heap;
   for (i = 0; i < Local[my_id].Index_B_Heap; i++)
      ZeroBox(my_id, &b_array[i]);
   Local[my_id].Index_B_Heap = 0;
}


void
ZeroBox (long my_id, box *b)
{
   long i;

   b->type = CHILDLESS;
   b->num_particles = 0;
   for (i = 0; i < MAX_PARTICLES_PER_BOX; i++)
      b->particles[i] = NULL;
   b->parent = NULL;
   for (i = 0; i < NUM_OFFSPRING; i++) {
      b->children[i] = NULL;
      b->shadow[i] = NULL;
   }
   b->num_children = 0;
   b->construct_synch = 0;
   b->interaction_synch = 0;
   b->cost = 0;
   b->proc = my_id;
   b->subtree_cost = 0;
   b->next = NULL;
   b->prev = NULL;
}


/*
 *  InitBox (long my_id, real x_center, real y_center, real length, long level, box *parent)
 *
 *  Args : the x_center and y_center of the center of the box;
 *         the length of the box;
 *         the level of the box;
 *         the address of b's parent.
 *
 *  Returns : the address of the newly created box.
 *
 *  Side Effects : Initializes num_particles to 0, all other pointers to NULL,
 *    and sets the box ID to a unique number. It also creates the space for
 *    the two expansion arrays.
 *
 */
box *
InitBox (long my_id, real x_center, real y_center, real length, box *parent)
{
   box *b;

   if (Local[my_id].Index_B_Heap == Local[my_id].Max_B_Heap) {
      LockedPrint("ERROR (P%d) : Ran out of boxes\n", my_id);
      exit(-1);
   }
   b = &Local[my_id].B_Heap[Local[my_id].Index_B_Heap++];
   b->x_center = x_center;
   b->y_center = y_center;
   b->length = length;
   b->parent = parent;
   if (parent == NULL)
      b->level = 0;
   else
      b->level = parent->level + 1;
   return b;
}


/*
 *  PrintBox (box *b)
 *
 *  Args : the address of a box, b.
 *
 *  Returns : nothing.
 *
 *  Side Effects : Prints to stdout the information stored for b.
 *
 */
void
PrintBox (box *b)
{
   LOCK(G_Memory->io_lock);
   fflush(stdout);
   if (b != NULL) {
      printf("Info for B%f :\n", b->id);
      printf("  X center       = %.40g\n", b->x_center);
      printf("  Y center       = %.40g\n", b->y_center);
      printf("  Length         = %.40g\n", b->length);
      printf("  Level          = %ld\n", b->level);
      printf("  Type           = %d\n", b->type);
      printf("  Child Num      = %ld\n", b->child_num);
      if (b->parent == NULL)
	 printf("  Parent         = NONE\n");
      else
	 printf("  Parent         = B%f\n", b->parent->id);
      printf("  Children's IDs : ");
      if (b->num_children != 0)
	 PrintBoxArrayIds(b->children, b->num_children);
      else
	 printf("NONE\n");
      printf("  Sibling's IDs : ");
      if (b->num_siblings != 0)
	 PrintBoxArrayIds(b->siblings, b->num_siblings);
      else
	 printf("NONE\n");
      printf("  Colleagues' IDs : ");
      PrintBoxArrayIds(b->colleagues, b->num_colleagues);
      printf("  U List IDs : ");
      PrintBoxArrayIds(b->u_list, b->num_u_list);
      printf("  V List IDs : ");
      PrintBoxArrayIds(b->v_list, b->num_v_list);
      printf("  W List IDs : ");
      PrintBoxArrayIds(b->w_list, b->num_w_list);
      printf("  # of Particles = %ld\n", b->num_particles);
      printf("  Particles' IDs : ");
      PrintParticleArrayIds(b->particles, b->num_particles);
      printf("  Assigned Process ID : %ld\n", b->proc);
      printf("  Cost : %ld\n", b->cost);
      printf("\n");
   }
   else
      printf("Box has not been initialized yet.\n\n");
   UNLOCK(G_Memory->io_lock);
}


/*
 *  PrintBoxArrayIds (box_node *b_array[], long array_length)
 *
 *  Args : the address of the box array, b_array;
 *         the length of the array, array_length.
 *
 *  Returns : nothing.
 *
 *  Side Effects : Prints to stdout just the id numbers for every box in
 *    b_array.
 *
 */
void
PrintBoxArrayIds (box *b_array[], long array_length)
{
   long i;
   long tab_count;

   tab_count = 0;
   for (i = 0; i < array_length; i++) {
      if (tab_count == 0) {
	 printf("\n");
	 tab_count = BOXES_PER_LINE;
      }
      if (b_array[i] != NULL)
	 printf("\tB%f", b_array[i]->id);
      tab_count -= 1;
   }
   printf("\n");
}


/*
 *  PrintExpansionTerms (real expansion[])
 *
 *  Args : the array of expansion terms, expansion.
 *
 *  Returns : nothing.
 *
 *  Side Effects : Prints to stdout the contents of expansion.
 *
 */
void
PrintExpansionTerms (complex expansion[])
{
   long i;
   long tab_count = 0;

   for (i = 0; i < Expansion_Terms; i++) {
      if (tab_count == 0) {
	 printf("\n");
	 tab_count = TERMS_PER_LINE;
      }
      if (expansion[i].i >= (real) 0.0)
	 printf("\ta%ld = %.3e + %.3ei", i, expansion[i].r, expansion[i].i);
      else
	 printf("\ta%ld = %.3e - %.3ei", i, expansion[i].r, -expansion[i].i);
      tab_count -= 1;
   }
   printf("\n");
}


void
ListIterate (long my_id, box *b, box **list, long length, list_function function)
{
   long i;

   for (i = 0; i < length; i++) {
      if (list[i] == NULL) {
	 LockedPrint("ERROR (P%d) : NULL list entry\n", my_id);
	 exit(-1);
      }
      (*function)(my_id, list[i], b);
   }
}


/*
 *  AdjacentBoxes (box *b1, box *b2)
 *
 *  Args : two potentially adjacent boxes, b1 and b2.
 *
 *  Returns : TRUE, if boxes are adjacent, FALSE if not.
 *
 *  Side Effects : none.
 *
 *  Comments : Two boxes are adjacent if their centers are separated in either
 *     the x or y directions by (1/2 the length of b1) + (1/2 length of b2),
 *     and separated in the other direction by a distance less than or equal
 *     to (1/2 the length of b1) + (1/2 the length of b2).
 *
 *     NOTE : By this definition, parents are NOT adjacent to their children.
 */
long
AdjacentBoxes (box *b1, box *b2)
{
   real exact_separation;
   real x_separation;
   real y_separation;
   long ret_val;

   exact_separation = (b1->length / (real) 2.0) + (b2->length / (real) 2.0);
   x_separation = (real) fabs((double)(b1->x_center - b2->x_center));
   y_separation = (real) fabs((double)(b1->y_center - b2->y_center));

   if ((x_separation == exact_separation) &&
       (y_separation <= exact_separation))
      ret_val = TRUE;
   else
      if ((y_separation == exact_separation) &&
	  (x_separation <= exact_separation))
	 ret_val = TRUE;
      else
	 ret_val = FALSE;

   return ret_val;
}


/*
 *  WellSeparatedBoxes (box *b1, box *b2)
 *
 *  Args : Two potentially well separated boxes, b1 and b2.
 *
 *  Returns : TRUE, if the two boxes are well separated, and FALSE if not.
 *
 *  Side Effects : none.
 *
 *  Comments : Well separated means that the two boxes are separated by the
 *     length of the boxes. If one of the boxes is bigger than the other,
 *     the smaller box is given the length of the larger box. This means
 *     that the centers of the two boxes, regardless of their relative size,
 *     must be separated in the x or y direction (or both) by at least
 *     twice the length of the biggest box.
 *
 */
long
WellSeparatedBoxes (box *b1, box *b2)
{
   real min_ws_distance;
   real x_separation;
   real y_separation;
   long ret_val;

   if (b1->length > b2->length)
      min_ws_distance = b1->length * (real) 2.0;
   else
      min_ws_distance = b2->length * (real) 2.0;

   x_separation = (real) fabs((double)(b1->x_center - b2->x_center));
   y_separation = (real) fabs((double)(b1->y_center - b2->y_center));

   if ((x_separation >= min_ws_distance) || (y_separation >= min_ws_distance))
      ret_val = TRUE;
   else
      ret_val = FALSE;

   return ret_val;
}


#undef BOXES_PER_LINE
#undef TERMS_PER_LINE

