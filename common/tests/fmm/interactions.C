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
#include "partition_grid.h"
#include "interactions.h"

static real Inv[MAX_EXPANSION_TERMS + 1];
static real OverInc[MAX_EXPANSION_TERMS + 1];
static real C[2 * MAX_EXPANSION_TERMS][2 * MAX_EXPANSION_TERMS];
static complex One;
static complex Zero;

void InitExp(box *b);
void ComputeMPExp(box *b);
void ShiftMPExp(box *cb, box *pb);
void UListInteraction(long my_id, box *b1, box *b2);
void VListInteraction(long my_id, box *source_box, box *dest_box);
void WAndXListInteractions(long my_id, box *b1, box *b2);
void WListInteraction(box *source_box, box *dest_box);
void XListInteraction(box *source_box, box *dest_box);
void ComputeSelfInteraction(box *b);
void ShiftLocalExp(box *pb, box *cb);
void EvaluateLocalExp(box *b);


void
InitExpTables ()
{
   long i;
   long j;

   for (i = 1; i < MAX_EXPANSION_TERMS + 1; i++) {
      Inv[i] = ((real) 1) / (real) i;
      OverInc[i] = ((real) i) / ((real) i + (real) 1);
   }
   C[0][0] = (real) 1.0;
   for (i = 1; i < (2 * MAX_EXPANSION_TERMS); i++) {
      C[i][0] = (real) 1.0;
      C[i][1] = (real) i;
      C[i - 1][i] = (real) 0.0;
      for (j = 2; j <= i; j++)
	 C[i][j] = C[i - 1][j] + C[i - 1][j - 1];
   }

   One.r = (real) 1.0;
   One.i = (real) 0.0;
   Zero.r = (real) 0.0;
   Zero.i = (real) 0.0;
}


void
PrintExpTables ()
{
   long i;
   long j;

   printf("Table for the functions f(i) = 1 / i and g(i) = i / (i + 1)\n");
   printf("i\t\tf(i)\t\tg(i)\t\t\n");
   for (i = 1; i < MAX_EXPANSION_TERMS; i++)
      printf("%ld\t\t%e\t%f\t\n", i, Inv[i], OverInc[i]);
   printf("\n\nTable for the function h(i,j) = i choose j\n");
   printf("i\tj\th(i,j)\n");
   for (i = 0; i < (2 * MAX_EXPANSION_TERMS); i++) {
      for (j = 0; j <= i; j++)
	 printf("%ld\t%ld\t%g\n", i, j, C[i][j]);
      printf("\n");
   }
}


void
UpwardPass (long my_id, box *b)
{
   InitExp(b);
   if (b->type == CHILDLESS) {
      ComputeMPExp(b);
      ALOCK(G_Memory->lock_array, b->exp_lock_index);
      b->interaction_synch = 1;
      AULOCK(G_Memory->lock_array, b->exp_lock_index);
   }
   else {
      while (b->interaction_synch != b->num_children) {
	 /* wait */;
      }
   }
   if (b->parent != NULL) {
      ShiftMPExp(b, b->parent);
      ALOCK(G_Memory->lock_array, b->parent->exp_lock_index);
      b->parent->interaction_synch += 1;
      AULOCK(G_Memory->lock_array, b->parent->exp_lock_index);
   }
}


void
ComputeInteractions (long my_id, box *b)
{
   b->cost = 0;
   if (b->type == CHILDLESS) {
      ComputeSelfInteraction(b);
      ListIterate(my_id, b, b->u_list, b->num_u_list, UListInteraction);
      ListIterate(my_id, b, b->w_list, b->num_w_list, WAndXListInteractions);
   }
   ListIterate(my_id, b, b->v_list, b->num_v_list, VListInteraction);
}


void
DownwardPass (long my_id, box *b)
{
   if (b->parent != NULL) {
      while (b->parent->interaction_synch != 0) {
	 /* wait */;
      }
      ShiftLocalExp(b->parent, b);
   }
   if (b->type == CHILDLESS) {
      EvaluateLocalExp(b);
      b->interaction_synch = 0;
   }
   else {
      ALOCK(G_Memory->lock_array, b->exp_lock_index);
      b->interaction_synch = 0;
      AULOCK(G_Memory->lock_array, b->exp_lock_index);
   }
}


void
ComputeParticlePositions (long my_id, box *b)
{
   particle *p;
   vector force;
   vector new_acc;
   vector delta_acc;
   vector delta_vel;
   vector avg_vel;
   vector delta_pos;
   long i;

   for (i = 0; i < b->num_particles; i++) {
      p = b->particles[i];
      force.x = p->field.r * p->charge;
      force.y = p->field.i * p->charge;
      VECTOR_DIV(new_acc, force, p->mass);
      if (Local[my_id].Time_Step != 0) {
	 VECTOR_SUB(delta_acc, new_acc, (p->acc));
	 VECTOR_MUL(delta_vel, delta_acc, ((real) Timestep_Dur) / (real) 2.0);
	 VECTOR_ADD((p->vel), (p->vel), delta_vel);
      }
      p->acc.x = new_acc.x;
      p->acc.y = new_acc.y;
      VECTOR_MUL(delta_vel, (p->acc), ((real) Timestep_Dur) / (real) 2.0);
      VECTOR_ADD(avg_vel, (p->vel), delta_vel);
      VECTOR_MUL(delta_pos, avg_vel, (real) Timestep_Dur);
      VECTOR_ADD((p->vel), avg_vel, delta_vel);
      VECTOR_ADD((p->pos), (p->pos), delta_pos);
   }
}


void
InitExp (box *b)
{
   long i;

   for (i = 0; i < Expansion_Terms; i++) {
      b->mp_expansion[i].r = 0.0;
      b->mp_expansion[i].i = 0.0;
      b->local_expansion[i].r = 0.0;
      b->local_expansion[i].i = 0.0;
      b->x_expansion[i].r = 0.0;
      b->x_expansion[i].i = 0.0;
   }
}


/*
 *  ComputeMPExp (long my_id, box *b)
 *
 *  Args : a box, b.
 *
 *  Returns : nothing.
 *
 *  Side Effects : Computes and sets the multipole expansion array.
 *
 *  Comments : The first terms (a0) in the expansion is simply the sum of the
 *    charges in the box. This procedure first computes the distances between
 *    the particles in the box and the boxes center. At the same time, a0 is
 *    computed. Then the remaining terms are calculated by theorem 2.1.1 in
 *    Greengard's thesis.
 *
 */
void
ComputeMPExp (box *b)
{
   particle *p;
   complex charge;
   complex box_pos;
   complex particle_pos;
   complex z0;
   complex z0_pow_n;
   complex temp;
   complex result_exp[MAX_EXPANSION_TERMS];
   long i;
   long j;

   box_pos.r = b->x_center;
   box_pos.i = b->y_center;
   for (i = 0; i < Expansion_Terms; i++) {
      result_exp[i].r = (real) 0.0;
      result_exp[i].i = (real) 0.0;
   }
   for (i = 0; i < b->num_particles; i++) {
      p = b->particles[i];
      particle_pos.r = p->pos.x;
      particle_pos.i = p->pos.y;
      charge.r = p->charge;
      charge.i = (real) 0.0;
      COMPLEX_SUB(z0, particle_pos, box_pos);
      z0_pow_n.r = One.r;
      z0_pow_n.i = One.i;
      for (j = 1; j < Expansion_Terms; j++) {
	 COMPLEX_MUL(temp, z0_pow_n, charge);
	 COMPLEX_ADD(result_exp[j], result_exp[j], temp);
	 COMPLEX_MUL(z0_pow_n, z0_pow_n, z0);
      }
   }
   ALOCK(G_Memory->lock_array, b->exp_lock_index);
   for (i = 0; i < Expansion_Terms; i++) {
      b->mp_expansion[i].r = result_exp[i].r;
      b->mp_expansion[i].i = result_exp[i].i;
   }
   AULOCK(G_Memory->lock_array, b->exp_lock_index);
}


void
ShiftMPExp (box *cb, box *pb)
{
   complex z0;
   complex z0_inv;
   complex z0_pow_n;
   complex z0_pow_minus_n;
   complex temp_exp[MAX_EXPANSION_TERMS];
   complex result_exp[MAX_EXPANSION_TERMS];
   complex child_pos;
   complex parent_pos;
   complex temp;
   long i;
   long j;

   child_pos.r = cb->x_center;
   child_pos.i = cb->y_center;
   parent_pos.r = pb->x_center;
   parent_pos.i = pb->y_center;
   COMPLEX_SUB(z0, child_pos, parent_pos);
   COMPLEX_DIV(z0_inv, One, z0);
   z0_pow_n.r = One.r;
   z0_pow_n.i = One.i;
   z0_pow_minus_n.r = One.r;
   z0_pow_minus_n.i = One.i;
   result_exp[0].r = cb->mp_expansion[0].r;
   result_exp[0].i = cb->mp_expansion[0].i;
   for (i = 1; i < Expansion_Terms; i++) {
      result_exp[i].r = (real) 0.0;
      result_exp[i].i = (real) 0.0;
      COMPLEX_MUL(z0_pow_minus_n, z0_pow_minus_n, z0_inv);
      COMPLEX_MUL(temp_exp[i], z0_pow_minus_n, cb->mp_expansion[i]);
      for (j = 1; j <= i; j++) {
	 temp.r = C[i - 1][j - 1];
	 temp.i = (real) 0.0;
	 COMPLEX_MUL(temp, temp, temp_exp[j]);
	 COMPLEX_ADD(result_exp[i], result_exp[i], temp);
      }
      temp.r = Inv[i];
      temp.i = (real) 0.0;
      COMPLEX_MUL(temp, temp, cb->mp_expansion[0]);
      COMPLEX_SUB(temp, result_exp[i], temp);
      COMPLEX_MUL(z0_pow_n, z0_pow_n, z0);
      COMPLEX_MUL(result_exp[i], temp, z0_pow_n);
   }
   ALOCK(G_Memory->lock_array, pb->exp_lock_index);
   for (i = 0; i < Expansion_Terms; i++) {
      COMPLEX_ADD((pb->mp_expansion[i]), (pb->mp_expansion[i]), result_exp[i]);
   }
   AULOCK(G_Memory->lock_array, pb->exp_lock_index);
}


void
UListInteraction (long my_id, box *source_box, box *dest_box)
{
   complex result;
   complex temp_vector;
   complex temp_charge;
   complex temp_result;
   real denom;
   real x_sep;
   real y_sep;
   real dest_x;
   real dest_y;
   long i;
   long j;

   for (i = 0; i < dest_box->num_particles; i++) {
      result.r = (real) 0.0;
      result.i = (real) 0.0;
      dest_x = dest_box->particles[i]->pos.x;
      dest_y = dest_box->particles[i]->pos.y;
      for (j = 0; j < source_box->num_particles; j++) {
	 x_sep = source_box->particles[j]->pos.x - dest_x;
	 y_sep = source_box->particles[j]->pos.y - dest_y;
	 denom = ((real) 1.0) / ((x_sep * x_sep) + (y_sep * y_sep));
	 temp_vector.r = x_sep * denom;
	 temp_vector.i = y_sep * denom;
	 temp_charge.r = source_box->particles[j]->charge;
	 temp_charge.i = (real) 0.0;
	 COMPLEX_MUL(temp_result, temp_vector, temp_charge);
	 COMPLEX_SUB(result, result, temp_result);
      }
      result.i = -result.i;
      COMPLEX_ADD((dest_box->particles[i]->field),
		  (dest_box->particles[i]->field), result);
   }

   dest_box->cost += U_LIST_COST(source_box->num_particles,
				 dest_box->num_particles);
}


void
VListInteraction (long my_id, box *source_box, box *dest_box)
{
   complex z0;
   complex z0_inv;
   complex z0_pow_minus_n[MAX_EXPANSION_TERMS];
   complex temp_exp[MAX_EXPANSION_TERMS];
   complex result_exp;
   complex source_pos;
   complex dest_pos;
   complex temp;
   long i;
   long j;

   if (source_box->type == CHILDLESS) {
      while (source_box->interaction_synch != 1) {
	 /* wait */;
      }
   }
   else {
      while (source_box->interaction_synch != source_box->num_children) {
	 /* wait */;
      }
   }

   source_pos.r = source_box->x_center;
   source_pos.i = source_box->y_center;
   dest_pos.r = dest_box->x_center;
   dest_pos.i = dest_box->y_center;
   COMPLEX_SUB(z0, source_pos, dest_pos);
   COMPLEX_DIV(z0_inv, One, z0);
   z0_pow_minus_n[0].r = One.r;
   z0_pow_minus_n[0].i = One.i;
   temp_exp[0].r = source_box->mp_expansion[0].r;
   temp_exp[0].i = source_box->mp_expansion[0].i;
   for (i = 1; i < Expansion_Terms; i++) {
      COMPLEX_MUL(z0_pow_minus_n[i], z0_pow_minus_n[i - 1], z0_inv);
      COMPLEX_MUL(temp_exp[i], z0_pow_minus_n[i], source_box->mp_expansion[i]);
   }
   for (i = 0; i < Expansion_Terms; i++) {
      result_exp.r = (real) 0.0;
      result_exp.i = (real) 0.0;
      for (j = 1; j < Expansion_Terms; j++) {
	 temp.r = C[i + j - 1][j - 1];
	 temp.i = (real) 0.0;
	 COMPLEX_MUL(temp, temp, temp_exp[j]);
	 if ((j & 0x1) == 0x0) {
	    COMPLEX_ADD(result_exp, result_exp, temp);
	 }
	 else {
	    COMPLEX_SUB(result_exp, result_exp, temp);
	 }
      }
      COMPLEX_MUL(result_exp, result_exp, z0_pow_minus_n[i]);
      if (i == 0) {
	 temp.r = log(COMPLEX_ABS(z0));
	 temp.i = (real) 0.0;
	 COMPLEX_MUL(temp, temp, source_box->mp_expansion[0]);
	 COMPLEX_ADD(result_exp, result_exp, temp);
      }
      else {
	 temp.r = Inv[i];
	 temp.i = (real) 0.0;
	 COMPLEX_MUL(temp, temp, z0_pow_minus_n[i]);
	 COMPLEX_MUL(temp, temp, source_box->mp_expansion[0]);
	 COMPLEX_SUB(result_exp, result_exp, temp);
      }
      COMPLEX_ADD((dest_box->local_expansion[i]),
		  (dest_box->local_expansion[i]), result_exp);
   }
   dest_box->cost += V_LIST_COST(Expansion_Terms);
}


void
WAndXListInteractions (long my_id, box *b1, box *b2)
{
   WListInteraction(b1, b2);
   XListInteraction(b2, b1);
}


void
WListInteraction (box *source_box, box *dest_box)
{
   complex z0;
   complex z0_inv;
   complex result;
   complex source_pos;
   complex particle_pos;
   long i;
   long j;

   if (source_box->type == CHILDLESS) {
      while (source_box->interaction_synch != 1) {
	 /* wait */;
      }
   }
   else {
      while (source_box->interaction_synch != source_box->num_children) {
	 /* wait */;
      }
   }

   source_pos.r = source_box->x_center;
   source_pos.i = source_box->y_center;
   for (i = 0; i < dest_box->num_particles; i++) {
      result.r = (real) 0.0;
      result.i = (real) 0.0;
      particle_pos.r = dest_box->particles[i]->pos.x;
      particle_pos.i = dest_box->particles[i]->pos.y;
      COMPLEX_SUB(z0, particle_pos, source_pos);
      COMPLEX_DIV(z0_inv, One, z0);
      for (j = Expansion_Terms - 1; j > 0; j--) {
	 COMPLEX_ADD(result, result, (source_box->mp_expansion[j]));
	 COMPLEX_MUL(result, result, z0_inv);
      }
      COMPLEX_ADD((dest_box->particles[i]->field),
		  (dest_box->particles[i]->field), result);
   }

   dest_box->cost += W_LIST_COST(dest_box->num_particles, Expansion_Terms);
}


void
XListInteraction (box *source_box, box *dest_box)
{
   complex z0;
   complex z0_inv;
   complex z0_pow_minus_n;
   complex result_exp[MAX_EXPANSION_TERMS];
   complex source_pos;
   complex dest_pos;
   complex charge;
   complex temp;
   long i;
   long j;

   dest_pos.r = dest_box->x_center;
   dest_pos.i = dest_box->y_center;
   for (i = 0; i < Expansion_Terms; i++) {
      result_exp[i].r = (real) 0.0;
      result_exp[i].i = (real) 0.0;
   }
   for (i = 0; i < source_box->num_particles; i++) {
      source_pos.r = source_box->particles[i]->pos.x;
      source_pos.i = source_box->particles[i]->pos.y;
      charge.r = source_box->particles[i]->charge;
      charge.i = (real) 0.0;
      COMPLEX_SUB(z0, source_pos, dest_pos);
      COMPLEX_DIV(z0_inv, One, z0);
      z0_pow_minus_n.r = z0_inv.r;
      z0_pow_minus_n.i = z0_inv.i;
      for (j = 1; j < Expansion_Terms; j++) {
	 COMPLEX_MUL(z0_pow_minus_n, z0_pow_minus_n, z0_inv);
	 COMPLEX_MUL(temp, charge, z0_pow_minus_n);
	 COMPLEX_ADD(result_exp[j], result_exp[j], temp);
      }
   }
   ALOCK(G_Memory->lock_array, dest_box->exp_lock_index);
   for (i = 0; i < Expansion_Terms; i++) {
      COMPLEX_SUB((dest_box->x_expansion[i]),
		  (dest_box->x_expansion[i]), result_exp[i]);
   }
   AULOCK(G_Memory->lock_array, dest_box->exp_lock_index);
   source_box->cost += X_LIST_COST(source_box->num_particles, Expansion_Terms);
}


void
ComputeSelfInteraction (box *b)
{
   complex results[MAX_PARTICLES_PER_BOX];
   complex temp_vector;
   complex temp_charge;
   complex temp_result;
   real denom;
   real x_sep;
   real y_sep;
   long i;
   long j;

   for (i = 0; i < b->num_particles; i++) {
      results[i].r = (real) 0.0;
      results[i].i = (real) 0.0;
   }

   for (i = 0; i < b->num_particles; i++) {
      for (j = i + 1; j < b->num_particles; j++) {
	 x_sep = b->particles[i]->pos.x - b->particles[j]->pos.x;
	 y_sep = b->particles[i]->pos.y - b->particles[j]->pos.y;

	 if ((fabs(x_sep) < Softening_Param)
	    && (fabs(y_sep) < Softening_Param)) {
	    if (x_sep >= 0.0)
	       x_sep = Softening_Param;
	    else
	       x_sep = -Softening_Param;
	    if (y_sep >= 0.0)
	       y_sep = Softening_Param;
	    else
	       y_sep = -Softening_Param;
	 }
	 denom = ((real) 1.0) / ((x_sep * x_sep) + (y_sep * y_sep));
	 temp_vector.r = x_sep * denom;
	 temp_vector.i = y_sep * denom;

	 temp_charge.r = b->particles[j]->charge;
	 temp_charge.i = (real) 0.0;
	 COMPLEX_MUL(temp_result, temp_vector, temp_charge);
	 COMPLEX_ADD(results[i], results[i], temp_result);

	 temp_charge.r = b->particles[i]->charge;
	 temp_charge.i = (real) 0.0;
	 COMPLEX_MUL(temp_result, temp_vector, temp_charge);
	 COMPLEX_SUB(results[j], results[j], temp_result);
      }
      results[i].i = -results[i].i;
      COMPLEX_ADD((b->particles[i]->field),
		  (b->particles[i]->field), results[i]);
   }

   b->cost += SELF_COST(b->num_particles);
}


void
ShiftLocalExp (box *pb, box *cb)
{
   complex z0;
   complex z0_inv;
   complex z0_pow_n;
   complex z0_pow_minus_n;
   complex temp_exp[MAX_EXPANSION_TERMS];
   complex result_exp[MAX_EXPANSION_TERMS];
   complex child_pos;
   complex parent_pos;
   complex temp;
   long i;
   long j;

   child_pos.r = cb->x_center;
   child_pos.i = cb->y_center;
   parent_pos.r = pb->x_center;
   parent_pos.i = pb->y_center;
   COMPLEX_SUB(z0, child_pos, parent_pos);
   COMPLEX_DIV(z0_inv, One, z0);
   z0_pow_n.r = One.r;
   z0_pow_n.i = One.i;
   z0_pow_minus_n.r = One.r;
   z0_pow_minus_n.i = One.i;
   for (i = 0; i < Expansion_Terms; i++) {
      COMPLEX_ADD(pb->local_expansion[i], pb->local_expansion[i],
		  pb->x_expansion[i]);
      COMPLEX_MUL(temp_exp[i], z0_pow_n, pb->local_expansion[i]);
      COMPLEX_MUL(z0_pow_n, z0_pow_n, z0);
   }
   for (i = 0; i < Expansion_Terms; i++) {
      result_exp[i].r = (real) 0.0;
      result_exp[i].i = (real) 0.0;
      for (j = i; j < Expansion_Terms ; j++) {
	 temp.r = C[j][i];
	 temp.i = (real) 0.0;
	 COMPLEX_MUL(temp, temp, temp_exp[j]);
	 COMPLEX_ADD(result_exp[i], result_exp[i], temp);
      }
      COMPLEX_MUL(result_exp[i], temp, z0_pow_minus_n);
      COMPLEX_MUL(z0_pow_minus_n, z0_pow_minus_n, z0_inv);
   }
   ALOCK(G_Memory->lock_array, cb->exp_lock_index);
   for (i = 0; i < Expansion_Terms; i++) {
      COMPLEX_ADD((cb->local_expansion[i]), (cb->local_expansion[i]),
		  result_exp[i]);
   }
   AULOCK(G_Memory->lock_array, cb->exp_lock_index);
}


void
EvaluateLocalExp (box *b)
{
   complex z0;
   complex result;
   complex source_pos;
   complex particle_pos;
   complex temp;
   long i;
   long j;

   source_pos.r = b->x_center;
   source_pos.i = b->y_center;
   for (i = 0; i < b->num_particles; i++) {
      result.r = (real) 0.0;
      result.i = (real) 0.0;
      particle_pos.r = b->particles[i]->pos.x;
      particle_pos.i = b->particles[i]->pos.y;
      COMPLEX_SUB(z0, particle_pos, source_pos);
      for (j = Expansion_Terms - 1; j > 0; j--) {
	 temp.r = (real) j;
	 temp.i = (real) 0.0;
	 COMPLEX_MUL(result, result, z0);
	 COMPLEX_MUL(temp, temp, (b->local_expansion[j]));
	 COMPLEX_ADD(result, result, temp);
      }
      COMPLEX_ADD((b->particles[i]->field), (b->particles[i]->field), result);
      b->particles[i]->field.r = -(b->particles[i]->field.r);
      b->particles[i]->field.r = RoundReal(b->particles[i]->field.r);
      b->particles[i]->field.i = RoundReal(b->particles[i]->field.i);
   }
}


