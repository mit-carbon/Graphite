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

#ifndef _Particle_H
#define _Particle_H 1

#include "defs.h"

typedef struct _Particle particle;
typedef struct _Particle_Node particle_node;

typedef enum { ONE_CLUSTER, TWO_CLUSTER } cluster_type;
typedef enum { UNIFORM, PLUMMER } model_type;

/* Every particle has :
 *    1. A unique ID number
 *    2. An x and y position
 *    3. A charge
 *    4. The field that acts on it due to every other particle
 *
 *  The force is what the algorithm actually calculates.
 */
struct _Particle
{
  long id;
  real charge;
  real mass;
  vector pos;
  vector acc;
  vector vel;
  complex field;
  long cost;
  real box;
};


/* This structure is used for linked lists of particles */
struct _Particle_Node
{
  particle *data;
  particle_node *next;
};

extern long Total_Particles;

extern void CreateParticleList(long my_id, long length);
extern void InitParticleList(long my_id, long num_assigned, long starting_id);
extern void CreateDistribution(cluster_type cluster, model_type model);
extern void PrintParticle(particle *p);
extern void PrintAllParticles(void);
extern void PrintParticleArrayIds(particle **p_array, long num_particles);


#endif /* _Particle_H */
