// Jason Miller
//
// Version of the physical transport layer that uses MPI to communicate
// between cores.

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <iostream>
#include <errno.h>
#include <assert.h>
#include "pin.H"
#include "mpi.h"
#include "config.h"

extern Config* g_config;

class Transport{
   private:
      int pt_tid;
      int comm_id;
      static int pt_num_mod;
      static int* dest_ranks;

   public:	

      // This routine should be called once within in each process.
      static void ptInitQueue(int num_mod);

      // This routine should be called once within each thread.
      int ptInit(int tid, int num_mod);

      // This routine should be called once when everything is done
      static void ptFinish() { MPI_Finalize(); }

      // Return the communications ID for this node
      int ptCommID() { return comm_id; }

      // Send a message to another core.  This call returns immediately.
      int ptSend(int receiver, char *buffer, int length);

      // Receive the next incoming message from any sender.  This call is
      //  blocking and will not return until a message has been received.
      char* ptRecv();

      // Returns TRUE if there is a message waiting to be received.
      bool ptQuery();
};

#endif


