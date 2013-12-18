#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "fixed_types.h"
#include "random.h"
#include "carbon_user.h"

class Particle
{
   private:
      SInt32 _id;

   public:
      Particle();
      Particle(SInt32 id, Random& rand_num);
      ~Particle() {}
      
      float _x;
      float _y;
      float _z;
};

Particle::Particle():
   _id(-1),
   _x(0), _y(0), _z(0)
{}

Particle::Particle(SInt32 id, Random<int>& rand_num):
   _id(id)
{
   _x = rand_num.next(100);
   _y = rand_num.next(100);
   _z = rand_num.next(100);
}

void getSimulationParameters(int argc, char* argv[]);
void* simulateParticleMotion(void* threadid);
void accumulateForce(Random<int>& local_rand_num, Particle& curr_particle, Particle& force_particle);

SInt32 num_iterations;
SInt32 num_particles;
SInt32 num_threads;
Particle** particle_array;

carbon_barrier_t barrier;

int main(int argc, char* argv[])
{
   getSimulationParameters(argc, argv);
   
   printf("Starting N-Body simulation: Num Particles(%i), Num Threads(%i), Num Iterations(%i)\n",
         num_particles, num_threads, num_iterations);

   CarbonStartSim(argc, argv);

   Random<int> rand_num;
   particle_array = new Particle*[num_particles];
   for (SInt32 i = 0; i < num_particles; i++)
      particle_array[i] = new Particle(i, rand_num);

   // Initialize barrier
   CarbonBarrierInit(&barrier, num_threads);

   carbon_thread_t tid_list[num_threads-1];

   // Spawn the threads
   for (SInt32 i = 0; i < num_threads-1; i++)
      tid_list[i] = CarbonSpawnThread(simulateParticleMotion, (void*) i);
   simulateParticleMotion((void*) (num_threads-1));

   // Join all the threads
   for (SInt32 i = 0; i < num_threads-1; i++)
      CarbonJoinThread(tid_list[i]);

   CarbonStopSim();

   printf("N-Body simulation: Completed Successfully\n");

   return 0;
}

void getSimulationParameters(int argc, char* argv[])
{
   assert(argc >= 7);
   for (SInt32 i = 1; i < 7; i += 2)
   {
      if (strcmp(argv[i], "-t") == 0)
         num_threads = atoi(argv[i+1]);
      else if (strcmp(argv[i], "-n") == 0)
         num_particles = atoi(argv[i+1]);
      else if (strcmp(argv[i], "-i") == 0)
         num_iterations = atoi(argv[i+1]);
      else
      {
         fprintf(stderr, "Unrecognized Option(%s)\n", argv[i]);
         exit(-1);
      }
   }
}

void* simulateParticleMotion(void* threadid)
{
   long tid = (long) threadid;
   Random<int> local_rand_num;
   local_rand_num.seed(tid);

   assert(num_particles % num_threads == 0);
   SInt32 num_particles_per_thread = num_particles / num_threads;

   Particle local_particle_array[num_particles_per_thread];
   // Get the local postions of all the particles
   for (SInt32 i = (tid * num_particles_per_thread); i < ((tid+1) * num_particles_per_thread); i++)
   {
      local_particle_array[i - tid * num_particles_per_thread] = *particle_array[i];
   }

   for (SInt32 t = 0; t < num_iterations; t++)
   {
      // Compute the positions of particles owned by current thread
      for (SInt32 i = (tid * num_particles_per_thread); i < ((tid+1) * num_particles_per_thread); i++)
      {
         Particle& curr_particle = local_particle_array[i - tid * num_particles_per_thread];
         for (SInt32 j = 0; j < num_particles; j++)
         {
            Particle& force_particle = *particle_array[j];
            accumulateForce(local_rand_num, curr_particle, force_particle);
         }
      }

      CarbonBarrierWait(&barrier);
     
      // Commit the positions of particles owned by current thread 
      for (SInt32 i = (tid * num_particles_per_thread); i < ((tid+1) * num_particles_per_thread); i++)
      {
         *particle_array[i] = local_particle_array[i - tid * num_particles_per_thread];
      }

      CarbonBarrierWait(&barrier);
   }

   return (void*) NULL;
}

void accumulateForce(Random<int>& local_rand_num, Particle& curr_particle, Particle& force_particle)
{
   UInt32 computation_type = local_rand_num.next(2);
   if (computation_type == 0)
   {
      // Do subtraction
      curr_particle._x -= force_particle._x;
      curr_particle._y -= force_particle._y;
      curr_particle._z -= force_particle._z;
   }
   else
   {
      // Do addition
      curr_particle._x += force_particle._x;
      curr_particle._y += force_particle._y;
      curr_particle._z += force_particle._z;
   }
}
